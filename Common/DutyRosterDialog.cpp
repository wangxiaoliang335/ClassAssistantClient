#include "DutyRosterDialog.h"
#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QMouseEvent>
#include <QBrush>
#include <QColor>
#include <QUrlQuery>
#include <QAbstractItemView>
#include <QLabel>
#include <QScrollArea>
#include <QEvent>
#include <QTimer>
#include <QAbstractButton>
QT_BEGIN_NAMESPACE_XLSX
QT_END_NAMESPACE_XLSX

DutyRosterDialog::DutyRosterDialog(QWidget* parent)
    : QDialog(parent)
{
    // 移除标题栏
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    
    setWindowTitle("值日表");
    resize(1000, 700);
    // 使用与QGroupInfo一致的深色主题
    setStyleSheet("background-color: #282A2B; color: #ffffff;");
    
    // 创建HTTP处理器
    m_httpHandler = new TAHttpHandler(this);
    
    setupUI();
}

DutyRosterDialog::~DutyRosterDialog()
{
}

void DutyRosterDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
    // 顶部栏：关闭按钮和功能按钮
    QHBoxLayout* topLayout = new QHBoxLayout;
    
    // 关闭按钮（右上角）
    m_closeButton = new QPushButton("✕", this);
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(255,255,255,0.12);"
        "color: #ffffff;"
        "font-size: 18px;"
        "font-weight: bold;"
        "border: none;"
        "border-radius: 15px;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(255,0,0,0.35);"
        "}"
    );
    connect(m_closeButton, &QPushButton::clicked, this, &DutyRosterDialog::onCloseClicked);
    
    topLayout->addStretch();
    
    // 功能按钮：导入、极简
    m_importButton = new QPushButton("导入", this);
    m_minimalistButton = new QPushButton("极简", this);
    
    QString buttonStyle = 
        "QPushButton {"
        "background-color: #4169E1;"
        "color: white;"
        "padding: 8px 16px;"
        "border-radius: 4px;"
        "font-weight: bold;"
        "font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "background-color: #5B7FD8;"
        "}"
        "QPushButton:pressed {"
        "background-color: #3357C7;"
        "}";
    
    m_importButton->setStyleSheet(buttonStyle);
    m_minimalistButton->setStyleSheet(buttonStyle);
    
    connect(m_importButton, &QPushButton::clicked, this, &DutyRosterDialog::onImportClicked);
    connect(m_minimalistButton, &QPushButton::clicked, this, &DutyRosterDialog::onMinimalistClicked);
    
    topLayout->addWidget(m_importButton);
    topLayout->addWidget(m_minimalistButton);
    topLayout->addWidget(m_closeButton);
    
    m_mainLayout->addLayout(topLayout);
    
    // 表格
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setStyleSheet(
        "QTableWidget {"
        "background-color: #1E1E1E;"
        "border: 1px solid #3E3E3E;"
        "gridline-color: #3E3E3E;"
        "color: #ffffff;"
        "}"
        "QTableWidget::item {"
        "padding: 5px;"
        "border: none;"
        "color: #ffffff;"
        "background-color: #1E1E1E;"
        "}"
        "QTableWidget::item:selected {"
        "background-color: #4169E1;"
        "color: #ffffff;"
        "}"
        "QTableWidget::item:!editable {"
        "background-color: rgba(0,0,0,0.18);"
        "}"
        "QHeaderView::section {"
        "background-color: #282A2B;"
        "color: #ffffff;"
        "padding: 8px;"
        "border: 1px solid #3E3E3E;"
        "font-weight: bold;"
        "}"
        "QTableCornerButton::section {"
        "background-color: #282A2B;"
        "border: 1px solid #3E3E3E;"
        "}"
    );
    
    // 隐藏corner button（左上角白色区域）
    m_tableWidget->setCornerButtonEnabled(false);
    
    // 使用定时器延迟设置corner button样式，确保widget已创建
    QTimer::singleShot(100, this, [this]() {
        QAbstractButton* cornerBtn = m_tableWidget->findChild<QAbstractButton*>();
        if (cornerBtn) {
            cornerBtn->setStyleSheet(
                "QAbstractButton {"
                "background-color: #282A2B;"
                "border: 1px solid #3E3E3E;"
                "}"
            );
        }
    });
    
    m_tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    
    connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &DutyRosterDialog::onCellDoubleClicked);
    connect(m_tableWidget, &QTableWidget::cellChanged, this, &DutyRosterDialog::onCellChanged);
    
    // 创建极简模式的widget和滚动区域
    m_minimalistScrollArea = new QScrollArea(this);
    m_minimalistScrollArea->setWidgetResizable(true);
    m_minimalistScrollArea->setStyleSheet(
        "QScrollArea {"
        "background-color: #1E1E1E;"
        "border: 1px solid #3E3E3E;"
        "}"
        "QScrollBar:vertical {"
        "background-color: #282A2B;"
        "width: 12px;"
        "}"
        "QScrollBar::handle:vertical {"
        "background-color: #3E3E3E;"
        "min-height: 20px;"
        "border-radius: 6px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "background-color: #4E4E4E;"
        "}"
    );
    
    m_minimalistWidget = new QWidget(m_minimalistScrollArea);
    m_minimalistWidget->setStyleSheet("background-color: #1E1E1E; color: #ffffff;");
    m_minimalistLayout = new QVBoxLayout(m_minimalistWidget);
    m_minimalistLayout->setContentsMargins(10, 10, 10, 10);
    m_minimalistLayout->setSpacing(10);
    
    m_minimalistScrollArea->setWidget(m_minimalistWidget);
    m_minimalistScrollArea->hide();  // 初始隐藏
    
    // 为极简模式widget安装事件过滤器，支持双击返回完整模式
    m_minimalistWidget->installEventFilter(this);
    
    m_mainLayout->addWidget(m_tableWidget, 1);
    m_mainLayout->addWidget(m_minimalistScrollArea, 1);
    
    // 注意：初始加载值日表数据会在 setGroupId() 时调用
    // 如果已经有 groupId，则立即加载
    if (!m_groupId.isEmpty()) {
        loadDutyRosterFromServer();
    }
}

void DutyRosterDialog::onCloseClicked()
{
    close();
}

void DutyRosterDialog::onImportClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择值日表Excel文件",
        "",
        "Excel Files (*.xlsx *.xls);;All Files (*.*)"
    );
    
    if (!filePath.isEmpty()) {
        importExcelFile(filePath);
    }
}

void DutyRosterDialog::onMinimalistClicked()
{
    if (m_isMinimalistMode) {
        switchToFullMode();
    } else {
        switchToMinimalistMode();
    }
}

void DutyRosterDialog::onCellDoubleClicked(int row, int column)
{
    if (m_isMinimalistMode) {
        // 极简模式下双击返回完整模式
        switchToFullMode();
        return;
    }
    
    // 转换为数据行索引
    int dataRow = row + 1;
    
    // 完整模式下，如果双击的是要求行，不允许编辑
    if (isRequirementRow(dataRow)) {
        return;
    }
    
    // 允许编辑
    QTableWidgetItem* item = m_tableWidget->item(row, column);
    if (item) {
        m_tableWidget->editItem(item);
    }
}

void DutyRosterDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 记录鼠标按下位置
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QDialog::mousePressEvent(event);
}

void DutyRosterDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        // 移动窗口
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

void DutyRosterDialog::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (m_isMinimalistMode) {
        // 极简模式下双击窗口任意位置返回完整模式
        switchToFullMode();
        return;
    }
    
    QDialog::mouseDoubleClickEvent(event);
}

bool DutyRosterDialog::eventFilter(QObject* obj, QEvent* event)
{
    // 处理极简模式widget的双击事件
    if (m_isMinimalistMode && obj == m_minimalistWidget && event->type() == QEvent::MouseButtonDblClick) {
        switchToFullMode();
        return true;
    }
    
    return QDialog::eventFilter(obj, event);
}

void DutyRosterDialog::onCellChanged(int row, int column)
{
    if (m_isMinimalistMode) {
        return;  // 极简模式下不允许编辑
    }
    
    // 更新数据（注意：表格行列索引需要转换为数据行列索引）
    // 表格行索引 = 数据行索引 - 1（因为第一行是表头）
    // 表格列索引 = 数据列索引 - 1（因为第一列是表头）
    int dataRow = row + 1;
    int dataCol = column + 1;
    
    if (dataRow >= 1 && dataRow < m_dutyData.size() && 
        dataCol >= 1 && dataCol < m_dutyData[dataRow].size()) {
        QTableWidgetItem* item = m_tableWidget->item(row, column);
        if (item) {
            m_dutyData[dataRow][dataCol] = item->text();
            // 保存到服务器
            saveDutyRosterToServer();
        }
    }
}

void DutyRosterDialog::importExcelFile(const QString& filePath)
{
    using namespace QXlsx;
    
    Document xlsx(filePath);
    if (!xlsx.load()) {
        QMessageBox::critical(this, "错误", "无法打开Excel文件：" + filePath);
        return;
    }
    
    // 获取第一个工作表
    Worksheet* sheet = xlsx.currentWorksheet();
    if (!sheet) {
        QMessageBox::critical(this, "错误", "Excel文件中没有工作表");
        return;
    }
    
    QList<QList<QString>> data;
    int maxRow = 0;
    int maxCol = 0;
    
    // 先确定数据范围
    for (int row = 1; row <= 1000; ++row) {  // 最多读取1000行
        bool rowHasData = false;
        QList<QString> rowData;
        
        for (int col = 1; col <= 100; ++col) {  // 最多读取100列
            Cell* cell = sheet->cellAt(row, col);
            QString text;
            if (cell) {
                QVariant value = cell->value();
                text = value.toString().trimmed();
            }
            rowData.append(text);
            
            if (!text.isEmpty()) {
                rowHasData = true;
                if (col > maxCol) {
                    maxCol = col;
                }
            }
        }
        
        if (rowHasData) {
            data.append(rowData);
            maxRow = row;
        } else if (row > 1) {
            // 如果遇到空行，停止读取（但第一行可能是标题，允许为空）
            break;
        }
    }
    
    if (data.isEmpty()) {
        QMessageBox::warning(this, "提示", "Excel文件为空或格式不正确");
        return;
    }
    
    // 只保留有数据的列
    for (int i = 0; i < data.size(); ++i) {
        while (data[i].size() > maxCol) {
            data[i].removeLast();
        }
    }
    
    parseExcelData(data);
}

void DutyRosterDialog::parseExcelData(const QList<QList<QString>>& data)
{
    if (data.isEmpty()) {
        return;
    }
    
    m_dutyData.clear();
    m_requirementRowIndex = -1;
    
    // 复制数据
    m_dutyData = data;
    
    // 查找要求行（最后一行的第一列应该是"要求"或类似文本，且该行除第一列外其他列合并）
    for (int i = data.size() - 1; i >= 0; --i) {
        if (i < data.size() && !data[i].isEmpty()) {
            QString firstCell = data[i][0].trimmed();
            if (firstCell.contains("要求") || firstCell.contains("备注") || firstCell.contains("说明")) {
                m_requirementRowIndex = i;
                break;
            }
        }
    }
    
    // 更新表格显示
    updateTableDisplay();
    
    // 保存到服务器
    saveDutyRosterToServer();
}

void DutyRosterDialog::updateTableDisplay()
{
    if (m_dutyData.isEmpty()) {
        m_tableWidget->setRowCount(0);
        m_tableWidget->setColumnCount(0);
        return;
    }
    
    // 确定列数（取第一行的列数，通常是日期行）
    // 注意：第一列（任务列）不作为数据列，只作为垂直表头
    int colCount = 0;
    if (!m_dutyData.isEmpty()) {
        colCount = m_dutyData[0].size();
    }
    
    // 确定行数（第一行是日期行，不作为数据行）
    int rowCount = m_dutyData.size();
    
    // 设置表格大小：列数减1（去掉第一列），行数减1（去掉第一行）
    // 第一列作为垂直表头，第一行作为水平表头
    m_tableWidget->setRowCount(rowCount > 0 ? rowCount - 1 : 0);
    m_tableWidget->setColumnCount(colCount > 0 ? colCount - 1 : 0);
    
    // 设置水平表头（第一行作为列标题，从第二列开始）
    if (rowCount > 0 && colCount > 1) {
        QStringList headers;
        for (int col = 1; col < colCount; ++col) {
            if (col < m_dutyData[0].size()) {
                headers.append(m_dutyData[0][col]);
            } else {
                headers.append("");
            }
        }
        m_tableWidget->setHorizontalHeaderLabels(headers);
    }
    
    // 设置垂直表头（第一列作为行标题，从第二行开始）
    if (rowCount > 1 && colCount > 0) {
        QStringList rowHeaders;
        for (int row = 1; row < rowCount; ++row) {
            if (row < m_dutyData.size() && !m_dutyData[row].isEmpty() && m_dutyData[row].size() > 0) {
                rowHeaders.append(m_dutyData[row][0]);
            } else {
                rowHeaders.append("");
            }
        }
        m_tableWidget->setVerticalHeaderLabels(rowHeaders);
    }
    
    // 填充数据（从第二行第二列开始，因为第一行第一列是表头）
    for (int row = 1; row < rowCount; ++row) {
        for (int col = 1; col < colCount; ++col) {
            QString text;
            if (row < m_dutyData.size() && col < m_dutyData[row].size()) {
                text = m_dutyData[row][col];
            }
            
            // 表格中的行列索引（从0开始，不包含表头）
            int tableRow = row - 1;
            int tableCol = col - 1;
            
            QTableWidgetItem* item = new QTableWidgetItem(text);
            
            // 要求行设置为只读
            if (isRequirementRow(row) && col > 0) {
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            }
            
            m_tableWidget->setItem(tableRow, tableCol, item);
        }
    }
    
    // 合并要求行的单元格（除第一列外）
    if (m_requirementRowIndex >= 1 && m_requirementRowIndex < rowCount && colCount > 1) {
        int tableRow = m_requirementRowIndex - 1;  // 转换为表格行索引
        m_tableWidget->setSpan(tableRow, 0, 1, colCount - 1);
    }
    
    // 调整列宽
    m_tableWidget->resizeColumnsToContents();
    m_tableWidget->resizeRowsToContents();
}

void DutyRosterDialog::switchToMinimalistMode()
{
    m_isMinimalistMode = true;
    m_minimalistButton->setText("完整");
    
    // 隐藏表格，显示极简模式widget
    m_tableWidget->hide();
    m_minimalistScrollArea->show();
    
    // 清空极简模式布局
    QLayoutItem* item;
    while ((item = m_minimalistLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    // 获取今天的列索引
    int todayDataCol = getTodayColumn();
    if (todayDataCol < 0 || todayDataCol >= 5) {
        QMessageBox::information(this, "提示", "今天不是工作日（周一到周五）");
        return;
    }
    
    // 创建横排显示：任务在左，人员在右
    if (m_dutyData.size() < 2) {
        return;
    }
    
    // 遍历所有任务行（跳过第一行日期行）
    for (int dataRow = 1; dataRow < m_dutyData.size(); ++dataRow) {
        // 要求行在最后单独处理，这里先跳过
        if (isRequirementRow(dataRow)) {
            continue;
        }
        
        // 获取任务名称（第一列）
        QString taskName;
        if (dataRow < m_dutyData.size() && !m_dutyData[dataRow].isEmpty()) {
            taskName = m_dutyData[dataRow][0].trimmed();
        }
        
        if (taskName.isEmpty()) {
            continue;
        }
        
        // 获取今日的值日人员（todayDataCol列）
        QStringList dutyPersons;
        if (dataRow < m_dutyData.size() && todayDataCol < m_dutyData[dataRow].size()) {
            QString person = m_dutyData[dataRow][todayDataCol].trimmed();
            if (!person.isEmpty()) {
                dutyPersons.append(person);
            }
        }
        
        // 如果同一任务有多行（如多个扫地人员），合并显示
        // 检查下一行是否是同一任务（跳过要求行）
        int nextRow = dataRow + 1;
        while (nextRow < m_dutyData.size()) {
            // 如果遇到要求行，停止合并
            if (isRequirementRow(nextRow)) {
                break;
            }
            
            // 检查是否是同一任务
            if (nextRow < m_dutyData.size() && 
                !m_dutyData[nextRow].isEmpty() &&
                m_dutyData[nextRow][0].trimmed() == taskName) {
                // 合并同一任务的人员
                if (todayDataCol < m_dutyData[nextRow].size()) {
                    QString person = m_dutyData[nextRow][todayDataCol].trimmed();
                    if (!person.isEmpty() && !dutyPersons.contains(person)) {
                        dutyPersons.append(person);
                    }
                }
                nextRow++;
            } else {
                // 不是同一任务，停止合并
                break;
            }
        }
        
        // 创建横排布局：任务名称 | 值日人员
        QHBoxLayout* rowLayout = new QHBoxLayout;
        rowLayout->setSpacing(15);
        
        // 任务名称标签（左对齐）
        QLabel* taskLabel = new QLabel(taskName, m_minimalistWidget);
        taskLabel->setStyleSheet(
            "QLabel {"
            "background-color: #282A2B;"
            "color: #ffffff;"
            "padding: 10px 15px;"
            "border: 1px solid #3E3E3E;"
            "font-size: 14px;"
            "font-weight: bold;"
            "min-width: 120px;"
            "}"
        );
        taskLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        
        // 值日人员标签（左对齐，可以显示多个名字）
        QString personsText = dutyPersons.join("  ");
        QLabel* personLabel = new QLabel(personsText, m_minimalistWidget);
        personLabel->setStyleSheet(
            "QLabel {"
            "background-color: #1E1E1E;"
            "color: #ffffff;"
            "padding: 10px 15px;"
            "border: 1px solid #3E3E3E;"
            "font-size: 14px;"
            "}"
        );
        personLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        personLabel->setWordWrap(false);
        
        rowLayout->addWidget(taskLabel);
        rowLayout->addWidget(personLabel, 1);  // 值日人员列占据剩余空间
        
        m_minimalistLayout->addLayout(rowLayout);
        
        // 跳过已处理的行
        dataRow = nextRow - 1;
    }
    
    // 添加要求行（如果有）
    if (m_requirementRowIndex >= 1 && m_requirementRowIndex < m_dutyData.size()) {
        m_minimalistLayout->addSpacing(10);
        
        QHBoxLayout* reqLayout = new QHBoxLayout;
        
        QLabel* reqLabel = new QLabel("要求", m_minimalistWidget);
        reqLabel->setStyleSheet(
            "QLabel {"
            "background-color: #282A2B;"
            "color: #ffffff;"
            "padding: 10px 15px;"
            "border: 1px solid #3E3E3E;"
            "font-size: 14px;"
            "font-weight: bold;"
            "min-width: 120px;"
            "}"
        );
        reqLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        
        // 合并要求行的所有列内容（除第一列外）
        QString reqText;
        if (m_requirementRowIndex < m_dutyData.size()) {
            const QList<QString>& reqRow = m_dutyData[m_requirementRowIndex];
            QStringList reqParts;
            
            // 检查第一列是否包含完整内容（用逗号分隔）
            if (!reqRow.isEmpty()) {
                QString firstCell = reqRow[0].trimmed();
                // 如果第一列包含逗号，说明内容都在第一列
                if (firstCell.contains(",")) {
                    // 解析第一列：格式可能是 "要求,内容1,内容2,..."
                    QStringList parts = firstCell.split(",");
                    // 跳过第一个部分（"要求"），合并后面的内容
                    for (int i = 1; i < parts.size(); ++i) {
                        QString part = parts[i].trimmed();
                        if (!part.isEmpty()) {
                            reqParts.append(part);
                        }
                    }
                }
            }
            
            // 从第二列开始（索引1），合并所有非空内容
            for (int col = 1; col < reqRow.size(); ++col) {
                QString cellText = reqRow[col].trimmed();
                if (!cellText.isEmpty()) {
                    reqParts.append(cellText);
                }
            }
            
            // 如果多个列都有内容，用空格连接；如果只有一个列有内容，直接使用
            reqText = reqParts.join(" ");
        }
        
        QLabel* reqContentLabel = new QLabel(reqText, m_minimalistWidget);
        reqContentLabel->setStyleSheet(
            "QLabel {"
            "background-color: #1E1E1E;"
            "color: #ffffff;"
            "padding: 10px 15px;"
            "border: 1px solid #3E3E3E;"
            "font-size: 14px;"
            "min-height: 40px;"
            "}"
        );
        reqContentLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);  // 改为顶部对齐，支持多行显示
        reqContentLabel->setWordWrap(true);  // 启用自动换行
        reqContentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);  // 允许选择文本
        
        reqLayout->addWidget(reqLabel);
        reqLayout->addWidget(reqContentLabel, 1);
        
        m_minimalistLayout->addLayout(reqLayout);
    }
    
    m_minimalistLayout->addStretch();
}

void DutyRosterDialog::switchToFullMode()
{
    m_isMinimalistMode = false;
    m_minimalistButton->setText("极简");
    m_tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);  // 允许编辑
    
    // 隐藏极简模式widget，显示表格
    m_minimalistScrollArea->hide();
    m_tableWidget->show();
    
    // 显示所有列
    int colCount = m_tableWidget->columnCount();
    for (int col = 0; col < colCount; ++col) {
        m_tableWidget->showColumn(col);
    }
    
    // 显示要求行
    int rowCount = m_tableWidget->rowCount();
    if (m_requirementRowIndex >= 1) {
        int tableRow = m_requirementRowIndex - 1;  // 转换为表格行索引
        if (tableRow >= 0 && tableRow < rowCount) {
            m_tableWidget->showRow(tableRow);
        }
    }
}

void DutyRosterDialog::loadDutyRosterFromServer()
{
    if (m_httpHandler && !m_groupId.isEmpty()) {
        // 临时断开原有的信号连接
        disconnect(m_httpHandler, &TAHttpHandler::success, this, nullptr);
        disconnect(m_httpHandler, &TAHttpHandler::failed, this, nullptr);
        
        // 连接加载值日表专用的信号
        connect(m_httpHandler, &TAHttpHandler::success, this, &DutyRosterDialog::onLoadDutyRosterSuccess, Qt::UniqueConnection);
        connect(m_httpHandler, &TAHttpHandler::failed, this, &DutyRosterDialog::onLoadDutyRosterFailed, Qt::UniqueConnection);
        
        QUrl url("http://47.100.126.194:5000/duty-roster");
        QUrlQuery query;
        query.addQueryItem("group_id", m_groupId);
        url.setQuery(query);
        m_httpHandler->get(url.toString());
    } else {
        // 如果没有群组ID，使用空数据
        m_dutyData.clear();
        m_requirementRowIndex = -1;
        updateTableDisplay();
    }
}

void DutyRosterDialog::onLoadDutyRosterSuccess(const QString& resp)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &DutyRosterDialog::onLoadDutyRosterSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &DutyRosterDialog::onLoadDutyRosterFailed);
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject rootObj = doc.object();
        QJsonObject dataObj = rootObj.value("data").toObject();
        int code = dataObj.value("code").toInt(-1);
        
        if (code == 200) {
            // 解析值日表数据
            QJsonArray rowsArray = dataObj.value("rows").toArray();
            m_dutyData.clear();
            
            // 直接使用服务器返回的 requirement_row_index
            m_requirementRowIndex = dataObj.value("requirement_row_index").toInt(-1);
            
            for (int i = 0; i < rowsArray.size(); ++i) {
                QJsonArray rowArray = rowsArray[i].toArray();
                QList<QString> row;
                for (int j = 0; j < rowArray.size(); ++j) {
                    row.append(rowArray[j].toString());
                }
                m_dutyData.append(row);
            }
            
            // 更新表格显示
            updateTableDisplay();
        } else {
            QString message = dataObj.value("message").toString("加载值日表失败");
            QMessageBox::warning(this, "提示", message);
        }
    } else {
        // 解析失败，使用空数据
        m_dutyData.clear();
        m_requirementRowIndex = -1;
        updateTableDisplay();
    }
}

void DutyRosterDialog::onLoadDutyRosterFailed(const QString& err)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &DutyRosterDialog::onLoadDutyRosterSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &DutyRosterDialog::onLoadDutyRosterFailed);
    
    qWarning() << "加载值日表失败：" << err;
    // 失败时使用空数据
    m_dutyData.clear();
    m_requirementRowIndex = -1;
    updateTableDisplay();
}

void DutyRosterDialog::saveDutyRosterToServer()
{
    if (m_httpHandler && !m_groupId.isEmpty()) {
        // 临时断开原有的信号连接
        disconnect(m_httpHandler, &TAHttpHandler::success, this, nullptr);
        disconnect(m_httpHandler, &TAHttpHandler::failed, this, nullptr);
        
        // 连接保存值日表专用的信号
        connect(m_httpHandler, &TAHttpHandler::success, this, &DutyRosterDialog::onSaveDutyRosterSuccess, Qt::UniqueConnection);
        connect(m_httpHandler, &TAHttpHandler::failed, this, &DutyRosterDialog::onSaveDutyRosterFailed, Qt::UniqueConnection);
        
        // 构建请求数据
        QJsonObject payload;
        payload["group_id"] = m_groupId;
        
        // 将值日表数据转换为JSON数组
        QJsonArray rowsArray;
        for (const QList<QString>& row : m_dutyData) {
            QJsonArray rowArray;
            for (const QString& cell : row) {
                rowArray.append(cell);
            }
            rowsArray.append(rowArray);
        }
        payload["rows"] = rowsArray;
        payload["requirement_row_index"] = m_requirementRowIndex;
        
        QJsonDocument doc(payload);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        QUrl url("http://47.100.126.194:5000/duty-roster");
        m_httpHandler->post(url.toString(), jsonData);
    }
}

void DutyRosterDialog::onSaveDutyRosterSuccess(const QString& resp)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &DutyRosterDialog::onSaveDutyRosterSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &DutyRosterDialog::onSaveDutyRosterFailed);
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject rootObj = doc.object();
        QJsonObject dataObj = rootObj.value("data").toObject();
        int code = dataObj.value("code").toInt(-1);
        QString message = dataObj.value("message").toString("保存成功");
        
        if (code == 200) {
            // 保存成功，不显示提示（避免频繁弹窗）
            qDebug() << "值日表保存成功";
        } else {
            QMessageBox::warning(this, "提示", message);
        }
    }
}

void DutyRosterDialog::onSaveDutyRosterFailed(const QString& err)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &DutyRosterDialog::onSaveDutyRosterSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &DutyRosterDialog::onSaveDutyRosterFailed);
    
    QMessageBox::critical(this, "错误", "保存值日表失败：" + err);
}

bool DutyRosterDialog::isRequirementRow(int row)
{
    return row == m_requirementRowIndex;
}

int DutyRosterDialog::getTodayColumn()
{
    QDate today = QDate::currentDate();
    int dayOfWeek = today.dayOfWeek();  // 1=周一，7=周日
    
    // 转换为列索引：周一=1，周二=2，...，周五=5
    // 注意：表格第一列（索引0）是任务列，日期列从索引1开始
    if (dayOfWeek >= 1 && dayOfWeek <= 5) {
        return dayOfWeek;  // 周一对应列1（第一列是任务列，索引0）
    }
    
    return -1;  // 不是工作日
}

