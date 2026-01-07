#include "ScheduleDialog.h"
#include "MidtermGradeDialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QToolTip>
#include <QCursor>
#include <QMenu>
#include <QAction>
#include <QBrush>
#include <QColor>
#include <QSet>
#include <algorithm>
#include "CommonInfo.h"
#include "ArrangeSeatDialog.h"
#include "CommentStorage.h"
#include "ScoreHeaderIdStorage.h"
#include "QXlsx/header/xlsxdocument.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QApplication>
#include <QDebug>
#include <QDate>
#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QMimeDatabase>
#include <QMimeType>
#include <QFileInfo>

static QString calcCurrentTerm()
{
    QDate currentDate = QDate::currentDate();
    int year = currentDate.year();
    int month = currentDate.month();

    QString term;
    // 9月-1月是上学期（第一学期），2月-8月是下学期（第二学期）
    if (month >= 9 || month <= 1) {
        if (month >= 9) {
            term = QString("%1-%2-1").arg(year).arg(year + 1);
        } else {
            term = QString("%1-%2-1").arg(year - 1).arg(year);
        }
    } else {
        term = QString("%1-%2-2").arg(year - 1).arg(year);
    }
    return term;
}

static QString resolveExamName(const QString& titleOrName)
{
    // 允许传入“期中成绩单.xlsx / 期中成绩表 / ...”，统一推导 exam_name
    QString s = titleOrName.trimmed();
    if (s.isEmpty()) return s;

    // 去扩展名
    QFileInfo fi(s);
    if (fi.suffix().toLower() == "xlsx" || fi.suffix().toLower() == "xls" || fi.suffix().toLower() == "csv") {
        s = fi.baseName();
    }

    // 规则：将“成绩单/成绩表”转换为“考试”（避免写死“期中考试”）
    if (s.contains(QString::fromUtf8(u8"成绩单"))) {
        s.replace(QString::fromUtf8(u8"成绩单"), QString::fromUtf8(u8"考试"));
    }
    if (s.contains(QString::fromUtf8(u8"成绩表"))) {
        s.replace(QString::fromUtf8(u8"成绩表"), QString::fromUtf8(u8"考试"));
    }
    return s.trimmed();
}

MidtermGradeDialog::MidtermGradeDialog(QString classid, QWidget* parent) : QDialog(parent)
{
    // 去掉标题栏
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setWindowTitle("期中成绩表");
    resize(1200, 800);
    setStyleSheet("background-color: #808080; font-size:14px;");
    
    // 启用鼠标跟踪以检测鼠标进入/离开
    setMouseTracking(true);

    m_classid = classid;

    // 创建关闭按钮
    m_btnClose = new QPushButton("X", this);
    m_btnClose->setFixedSize(30, 30);
    m_btnClose->setStyleSheet(
        "QPushButton { background-color: #666666; color: white; font-weight: bold; font-size: 14px; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #777777; }"
    );
    m_btnClose->hide(); // 初始隐藏
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::close);
    
    // 为关闭按钮安装事件过滤器，确保鼠标在按钮上时不会隐藏
    m_btnClose->installEventFilter(this);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    // 增加顶部边距，为关闭按钮留出空间（关闭按钮高度30，位置y=5，所以顶部至少需要40）
    mainLayout->setContentsMargins(15, 40, 15, 15);

    // 标题
    m_lblTitle = new QLabel("期中成绩表");
    m_lblTitle->setStyleSheet("background-color: #d3d3d3; color: black; font-size: 16px; font-weight: bold; padding: 8px; border-radius: 4px;");
    m_lblTitle->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_lblTitle);

    // 顶部按钮行
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(8);

    QString btnStyle = "QPushButton { background-color: green; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px; }"
                      "QPushButton:hover { background-color: #006400; }";

    btnAddRow = new QPushButton("添加行");
    btnDeleteRow = new QPushButton("删除行");
    btnDeleteColumn = new QPushButton("删除列");
    btnAddColumn = new QPushButton("添加列");
    btnFontColor = new QPushButton("字体颜色");
    btnBgColor = new QPushButton("背景色");
    btnDescOrder = new QPushButton("倒序");
    btnAscOrder = new QPushButton("正序");
    btnExport = new QPushButton("导出");
    btnUpload = new QPushButton("上传服务器");
    qDebug() << "创建上传按钮，按钮指针:" << btnUpload;

    btnAddRow->setStyleSheet(btnStyle);
    btnDeleteRow->setStyleSheet(btnStyle);
    btnDeleteColumn->setStyleSheet(btnStyle);
    btnAddColumn->setStyleSheet(btnStyle);
    btnFontColor->setStyleSheet(btnStyle);
    btnBgColor->setStyleSheet(btnStyle);
    btnDescOrder->setStyleSheet(btnStyle);
    btnAscOrder->setStyleSheet(btnStyle);
    btnExport->setStyleSheet(btnStyle);
    btnUpload->setStyleSheet("QPushButton { background-color: blue; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px; }"
                             "QPushButton:hover { background-color: #0000CD; }");

    btnLayout->addWidget(btnAddRow);
    btnLayout->addWidget(btnDeleteRow);
    btnLayout->addWidget(btnDeleteColumn);
    btnLayout->addWidget(btnAddColumn);
    btnLayout->addWidget(btnFontColor);
    btnLayout->addWidget(btnBgColor);
    btnLayout->addWidget(btnDescOrder);
    btnLayout->addWidget(btnAscOrder);
    btnLayout->addWidget(btnExport);
    btnLayout->addWidget(btnUpload);
    qDebug() << "上传按钮已添加到布局，按钮是否可见:" << btnUpload->isVisible() << "，是否启用:" << btnUpload->isEnabled();
    btnLayout->addStretch();

    mainLayout->addLayout(btnLayout);
    qDebug() << "按钮布局已添加到主布局";

    // 说明文本框
    QLabel* lblDesc = new QLabel("说明:");
    lblDesc->setStyleSheet("font-weight: bold; color: black;");
    mainLayout->addWidget(lblDesc);

    textDescription = new QTextEdit;
    textDescription->setPlaceholderText("在此处添加表格说明...");
    textDescription->setFixedHeight(60);
    textDescription->setStyleSheet("background-color: #ffa500; color: black; padding: 5px; border: 1px solid #888;");
    textDescription->setPlainText("说明:该表为期中成绩表。语文总分120、数学总分120");
    mainLayout->addWidget(textDescription);

    // 表格
    table = new MidtermGradeTableWidget(this); // 使用自定义表格控件
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);   // 允许多选
    table->setSelectionBehavior(QAbstractItemView::SelectItems);     // 支持按行/列/单元格选择
    table->horizontalHeader()->setSectionsClickable(true);           // 允许点击表头选列
    table->verticalHeader()->setSectionsClickable(true);             // 允许点击行号选行
    // 统一选中高亮颜色
    table->setStyleSheet("QTableWidget::item:selected { background-color: #4A90E2; color: white; }");

    // 添加示例数据
    if (table->item(0, 2)) table->item(0, 2)->setText("100");
    if (table->item(0, 3)) table->item(0, 3)->setText("89");
    if (table->item(1, 2)) table->item(1, 2)->setText("90");
    if (table->item(1, 3)) table->item(1, 3)->setText("78");
    if (table->item(2, 2)) table->item(2, 2)->setText("97");
    if (table->item(2, 3)) table->item(2, 3)->setText("80");
    if (table->item(3, 2)) table->item(3, 2)->setText("67");
    if (table->item(3, 3)) table->item(3, 3)->setText("97");

    mainLayout->addWidget(table);

    // 固定列索引（不能删除的列）- 初始为空，导入数据时根据实际列头动态设置
    fixedColumns.clear();
    nameColumnIndex = -1; // 姓名列索引，导入数据时设置

    // 连接信号和槽
    connect(btnAddRow, &QPushButton::clicked, this, &MidtermGradeDialog::onAddRow);
    connect(btnDeleteRow, &QPushButton::clicked, this, &MidtermGradeDialog::onDeleteRow);
    connect(btnDeleteColumn, &QPushButton::clicked, this, &MidtermGradeDialog::onDeleteColumn);
    connect(btnAddColumn, &QPushButton::clicked, this, &MidtermGradeDialog::onAddColumn);
    connect(btnFontColor, &QPushButton::clicked, this, &MidtermGradeDialog::onFontColor);
    connect(btnBgColor, &QPushButton::clicked, this, &MidtermGradeDialog::onBgColor);
    connect(btnDescOrder, &QPushButton::clicked, this, &MidtermGradeDialog::onDescOrder);
    connect(btnAscOrder, &QPushButton::clicked, this, &MidtermGradeDialog::onAscOrder);
    connect(btnExport, &QPushButton::clicked, this, &MidtermGradeDialog::onExport);
    connect(btnUpload, &QPushButton::clicked, this, [this]() {
        qDebug() << "上传按钮被点击！";
        this->onUpload();
    });
    
    // 初始化网络管理器
    networkManager = new QNetworkAccessManager(this);

    // 单元格点击和悬浮事件
    connect(table, &QTableWidget::cellClicked, this, &MidtermGradeDialog::onCellClicked);
    connect(table, &QTableWidget::cellEntered, this, &MidtermGradeDialog::onCellEntered);
    table->setMouseTracking(true);
    
    // 监听单元格内容变化，自动更新总分
    connect(table, &QTableWidget::itemChanged, this, &MidtermGradeDialog::onItemChanged);

    // 创建注释窗口
    commentWidget = new CellCommentWidget(this);
    
    // 安装事件过滤器以处理鼠标悬浮
    table->installEventFilter(this);

    // 右键菜单用于删除行
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table, &QTableWidget::customContextMenuRequested, this, &MidtermGradeDialog::onTableContextMenu);
}

void MidtermGradeDialog::importData(const QStringList& headers, const QList<QStringList>& dataRows, const QString& excelFilePath)
{
    if (!table) return;

    // 导入属于批量更新：避免 itemChanged 触发大量 set-score/总分计算
    m_isBulkUpdating = true;
    const QSignalBlocker blocker(table);

    // 保存Excel文件路径和文件名
    m_excelFilePath = excelFilePath;
    if (!excelFilePath.isEmpty()) {
        QFileInfo fileInfo(excelFilePath);
        m_excelFileName = fileInfo.fileName();
        // 更新标题标签为文件名（去掉扩展名）
        if (m_lblTitle) {
            m_lblTitle->setText(fileInfo.baseName());
        }
    } else {
        // 服务器缓存刷新模式下可能不会有本地文件路径，但我们仍需要 m_excelFileName 用于 set-score。
        if (m_excelFileName.isEmpty()) {
            m_excelFileName.clear();
        }
    }

    // 根据导入的Excel列头动态设置表格列头
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    
    // 根据实际列头动态生成说明文本
    QStringList attributeColumns; // 属性列（除了学号、姓名、总分）
    for (const QString& header : headers) {
        if (header != "学号" && header != "姓名" && header != "总分" && !header.isEmpty()) {
            attributeColumns.append(header);
        }
    }
    
    QString description = "说明:该表为统计表。";
    if (!attributeColumns.isEmpty()) {
        description += "包含以下科目/属性: " + attributeColumns.join("、");
    }
    if (textDescription) {
        textDescription->setPlainText(description);
    }

    // 清空现有数据
    table->setRowCount(0);

    // 获取列索引映射
    QMap<QString, int> headerMap;
    for (int i = 0; i < headers.size(); ++i) {
        headerMap[headers[i]] = i;
    }

    // 获取固定列的索引（根据导入的列头）
    int colId = -1, colName = -1;
    
    // 在导入的列头中找到对应的列
    for (int col = 0; col < headers.size(); ++col) {
        QString headerText = headers[col];
        
        if (headerText == "学号") {
            colId = col;
            fixedColumns.insert(col); // 学号是固定列
        }
        else if (headerText == "姓名") {
            colName = col;
            fixedColumns.insert(col); // 姓名是固定列
            nameColumnIndex = col; // 设置姓名列索引
        }
    }
    
    // 确保学号和姓名列都存在
    if (colId < 0 || colName < 0) {
        QMessageBox::warning(this, "错误", "导入的数据中必须包含'学号'和'姓名'列！");
        return;
    }

    // 导入数据
    for (const QStringList& rowData : dataRows) {
        // 检查是否为空行（所有列都为空）
        bool isEmptyRow = true;
        for (const QString& cell : rowData) {
            if (!cell.trimmed().isEmpty()) {
                isEmptyRow = false;
                break;
            }
        }
        if (isEmptyRow) continue; // 跳过空行
        
        // 允许数据行的列数少于表头列数（空列会被填充为空字符串）
        int row = table->rowCount();
        table->insertRow(row);

        // 初始化所有单元格
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* item = new QTableWidgetItem("");
            item->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, col, item);
        }

        // 填充数据 - 根据导入的列头动态填充所有列
        for (int col = 0; col < headers.size(); ++col) {
            QString headerText = headers[col];
            QTableWidgetItem* item = table->item(row, col);
            if (col < rowData.size()) {
                item->setText(rowData[col]);
            }
            
            // 学号、姓名列：不可编辑，清空注释
            if (headerText == "学号" || headerText == "姓名") {
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                item->setData(Qt::UserRole, QVariant());
                continue;
            }

            // 总分列：不可编辑（由程序/服务端计算）
            if (headerText == "总分") {
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                item->setData(Qt::UserRole, QVariant());
                continue;
            }
            
            // 如果是属性列（非学号、姓名、总分），尝试从全局存储中获取注释
            if (headerText != "总分") {
                // 获取学生学号（从学号列）
                QString studentId = "";
                if (colId >= 0 && colId < rowData.size()) {
                    studentId = rowData[colId];
                }
                
                // 如果学号为空，尝试使用姓名匹配
                if (studentId.isEmpty() && colName >= 0 && colName < rowData.size()) {
                    QString studentName = rowData[colName];
                    // 从全局存储中查找注释（使用姓名作为键的一部分）
                    // 注意：这里需要知道 classId、examName、term
                    // 暂时跳过，因为 importData 方法没有这些参数
                }
                
                // 如果学号不为空，尝试从全局存储中获取注释
                if (!studentId.isEmpty() && m_classid.isEmpty() == false) {
                    const QString term = calcCurrentTerm();
                    // exam_name 由当前表格标题/文件名推导
                    const QString examName = resolveExamName(windowTitle().isEmpty() ? m_excelFileName : windowTitle());
                    
                    // 从全局存储中获取注释（按表格名区分，避免不同表同字段覆盖）
                    QString comment = CommentStorage::getComment(m_classid, term, studentId, headerText, m_excelFileName);
                    if (!comment.isEmpty()) {
                        item->setData(Qt::UserRole, comment);
                    }
                }
            }
        }
    }
    
    // 确保表格可见并刷新显示
    table->setVisible(true);
    table->show();
    table->update();
    table->repaint();
    
    qDebug() << "导入完成，共" << table->rowCount() << "行数据";
    m_isBulkUpdating = false;
}

void MidtermGradeDialog::onAddRow()
{
    int currentRow = table->currentRow();
    if (currentRow < 0) {
        currentRow = table->rowCount() - 1;
    }
    int insertRow = currentRow + 1;
    table->insertRow(insertRow);

    // 获取学号和姓名列的索引
    int colId = -1, colName = -1;
    for (int c = 0; c < table->columnCount(); ++c) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(c);
        if (!headerItem) continue;
        QString headerText = headerItem->text();
        if (headerText == "学号") {
            colId = c;
        } else if (headerText == "姓名") {
            colName = c;
        }
    }

    // 初始化新行的所有单元格
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* item = new QTableWidgetItem("");
        item->setTextAlignment(Qt::AlignCenter);
        
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
        QString headerText = headerItem ? headerItem->text() : "";
        
        // 学号、姓名列去掉注释（保持可编辑），其他列默认值为"0"
        if (headerText == "学号" || headerText == "姓名") {
            item->setData(Qt::UserRole, QVariant());
        } else if (headerText == "总分") {
            // 总分列由程序/服务端计算，不可编辑
            item->setData(Qt::UserRole, QVariant());
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        } else {
            // 除了学号和姓名，其他字段默认值都为0
            item->setText("0");
        }
        
        table->setItem(insertRow, col, item);
    }

    // 为学号列填入当前最大且不同的学号+1（确保不与现有学号重复）
    if (colId >= 0) {
        // 收集所有现有学号（包括insertRow之后的行）
        QSet<int> existingIds;
        for (int r = 0; r < table->rowCount(); ++r) {
            if (r == insertRow) continue; // 跳过新插入的行
            QTableWidgetItem* idItem = table->item(r, colId);
            if (!idItem) continue;
            bool ok = false;
            int v = idItem->text().trimmed().toInt(&ok);
            if (ok && v > 0) {
                existingIds.insert(v);
            }
        }
        
        // 找到最大学号
        int maxId = 0;
        if (!existingIds.isEmpty()) {
            maxId = *std::max_element(existingIds.begin(), existingIds.end());
        }
        
        // 生成新的学号，确保不与现有学号重复
        int newId = maxId + 1;
        while (existingIds.contains(newId)) {
            newId++;
        }
        
        QTableWidgetItem* newIdItem = table->item(insertRow, colId);
        if (newIdItem) {
            newIdItem->setText(QString::number(newId));
        }
    }
}

void MidtermGradeDialog::onDeleteRow()
{
    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要删除的行");
        return;
    }

    // 必须整行选中（选中了该行所有列）才允许删除
    for (const auto& range : ranges) {
        if (range.leftColumn() != 0 || range.rightColumn() != table->columnCount() - 1) {
            QMessageBox::information(this, "提示", "请先整行选中后再删除该行");
            return;
        }
    }

    QSet<int> rowsToDelete;
    for (const auto& range : ranges) {
        for (int r = range.topRow(); r <= range.bottomRow(); ++r) {
            rowsToDelete.insert(r);
        }
    }

    if (rowsToDelete.isEmpty()) {
        QMessageBox::information(this, "提示", "未选择有效的行");
        return;
    }

    // 从下往上删除，避免行号变化影响
    QList<int> rowsList = rowsToDelete.values();
    std::sort(rowsList.begin(), rowsList.end(), std::greater<int>());
    for (int r : rowsList) {
        table->removeRow(r);
    }
}

void MidtermGradeDialog::onDeleteColumn()
{
    int currentCol = table->currentColumn();
    if (currentCol < 0) {
        QMessageBox::information(this, "提示", "请先选择要删除的列");
        return;
    }

    // 检查是否是固定列
    if (fixedColumns.contains(currentCol)) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(currentCol);
        QString headerText = headerItem ? headerItem->text() : "";
        QMessageBox::warning(this, "警告", QString("不能删除固定列（%1）").arg(headerText));
        return;
    }

    int ret = QMessageBox::question(this, "确认", "确定要删除这一列吗？", 
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        table->removeColumn(currentCol);
        // 更新固定列索引（删除列后，后续列的索引会变化）
        QSet<int> newFixedColumns;
        for (int col : fixedColumns) {
            if (col < currentCol) {
                newFixedColumns.insert(col);
            } else if (col > currentCol) {
                newFixedColumns.insert(col - 1);
            }
        }
        fixedColumns = newFixedColumns;
        if (nameColumnIndex > currentCol) {
            nameColumnIndex--;
        }
    }
}

void MidtermGradeDialog::onAddColumn()
{
    bool ok;
    QString columnName = QInputDialog::getText(this, "添加列", "请输入列名:", 
                                               QLineEdit::Normal, "", &ok);
    if (!ok || columnName.isEmpty()) {
        return;
    }

    // 在姓名列后添加（如果姓名列存在）
    int insertCol = nameColumnIndex >= 0 ? nameColumnIndex + 1 : table->columnCount();
    table->insertColumn(insertCol);
    table->setHorizontalHeaderItem(insertCol, new QTableWidgetItem(columnName));

    // 初始化新列的所有单元格
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* item = new QTableWidgetItem("0"); // 新列默认值为0
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, insertCol, item);
    }

    // 更新固定列索引（插入列后，姓名列之后的固定列索引会变化）
    QSet<int> newFixedColumns;
    for (int col : fixedColumns) {
        if (col <= nameColumnIndex) {
            newFixedColumns.insert(col); // 姓名列及其之前的列索引不变
        } else {
            newFixedColumns.insert(col + 1); // 姓名列之后的列索引+1
        }
    }
    fixedColumns = newFixedColumns;
    // nameColumnIndex 不变，因为姓名列索引没有变化
}

void MidtermGradeDialog::onFontColor()
{
    QTableWidgetItem* item = table->currentItem();
    if (!item) {
        QMessageBox::information(this, "提示", "请先选择要设置字体颜色的单元格");
        return;
    }

    QColor color = QColorDialog::getColor(item->foreground().color(), this, "选择字体颜色");
    if (color.isValid()) {
        item->setForeground(QBrush(color));
    }
}

void MidtermGradeDialog::onBgColor()
{
    QTableWidgetItem* item = table->currentItem();
    if (!item) {
        QMessageBox::information(this, "提示", "请先选择要设置背景色的单元格");
        return;
    }

    QColor color = QColorDialog::getColor(item->background().color(), this, "选择背景色");
    if (color.isValid()) {
        item->setBackground(QBrush(color));
    }
}

void MidtermGradeDialog::onDescOrder()
{
    sortTable(false); // false表示倒序
}

void MidtermGradeDialog::onAscOrder()
{
    sortTable(true); // true表示正序
}

void MidtermGradeDialog::onExport()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出成绩表", "", "CSV文件 (*.csv);;所有文件 (*.*)");
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法创建文件");
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    
    // 写入UTF-8 BOM以支持Excel正确显示中文
    out << "\xEF\xBB\xBF";

    // 导出说明
    QString desc = textDescription->toPlainText();
    if (!desc.isEmpty()) {
        out << desc << "\n\n";
    }

    // 导出表头
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
        QString headerText = headerItem ? headerItem->text() : "";
        if (!headerText.isEmpty()) {
            out << headerText;
        }
        if (col < table->columnCount() - 1) {
            out << ",";
        }
    }
    out << "\n";

    // 导出数据
    for (int row = 0; row < table->rowCount(); ++row) {
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* item = table->item(row, col);
            QString text = item ? item->text() : "";
            out << text;
            if (col < table->columnCount() - 1) {
                out << ",";
            }
        }
        out << "\n";
    }

    file.close();
    QMessageBox::information(this, "成功", "导出完成！");
}

void MidtermGradeDialog::onUpload()
{
    qDebug() << "onUpload() 方法被调用！";
    qDebug() << "Excel文件路径:" << m_excelFilePath;
    qDebug() << "Excel文件名:" << m_excelFileName;

    // 确保有可写入的本地Excel路径；如果不存在则创建文件并保存当前表格
    {
        QString excelPath = m_excelFilePath;
        QString excelName = m_excelFileName;

        // 目标目录：appDir/excel_files/<schoolId>/<classId>/student/（普通表格保存到student子目录）
        UserInfo userInfo = CommonInfo::GetData();
        QString schoolId = userInfo.schoolId;
        QString classId = m_classid;
        QString baseDir = QCoreApplication::applicationDirPath() + "/excel_files";
        QString targetDir;
        if (!schoolId.isEmpty() && !classId.isEmpty()) {
            targetDir = baseDir + "/" + schoolId + "/" + classId + "/student";
        }

        if (excelName.isEmpty()) {
            excelName = QString("期中成绩表_%1.xlsx").arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));
        }

        // 如果有目标目录，则强制保存到该目录
        if (!targetDir.isEmpty()) {
            QDir().mkpath(targetDir);
            excelPath = targetDir + "/" + excelName;
        } else if (excelPath.isEmpty() || !QFile::exists(excelPath)) {
            QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            if (dir.isEmpty()) dir = QCoreApplication::applicationDirPath();
            QDir().mkpath(dir);
            excelPath = dir + "/" + excelName;
        }

        m_excelFilePath = excelPath;
        m_excelFileName = excelName;

        QXlsx::Document xlsx;
        // 写表头
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
            QString headerText = headerItem ? headerItem->text() : "";
            xlsx.write(1, col + 1, headerText);
        }
        // 写数据
        for (int row = 0; row < table->rowCount(); ++row) {
            for (int col = 0; col < table->columnCount(); ++col) {
                QTableWidgetItem* item = table->item(row, col);
                QString text = item ? item->text() : "";
                xlsx.write(row + 2, col + 1, text);
            }
        }
        if (!xlsx.saveAs(m_excelFilePath)) {
            QMessageBox::warning(this, "错误", "保存本地Excel文件失败，无法上传！");
            return;
        }
    }
    
    // 检查表格是否有数据
    if (table->rowCount() == 0) {
        qDebug() << "表格中没有数据，无法上传！";
        QMessageBox::warning(this, "提示", "表格中没有数据，无法上传！");
        return;
    }
    
    qDebug() << "表格有数据，行数:" << table->rowCount();

    // 使用类中的 m_classid
    QString classId = m_classid;
    if (classId.isEmpty()) {
        QMessageBox::warning(this, "错误", "班级ID为空，无法上传！");
        return;
    }

    // 考试名称：从窗口标题/文件名推导（避免写死）
    QString examName = resolveExamName(windowTitle().isEmpty() ? m_excelFileName : windowTitle());
    if (examName.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8(u8"错误"), QString::fromUtf8(u8"考试名称为空，无法上传！"));
        return;
    }

    // 根据当前日期计算学期
    QString term = calcCurrentTerm();
    qDebug() << "计算出的学期:" << term;

    // 备注（可选）
    QDate currentDate = QDate::currentDate();
    int month = currentDate.month();
    QString remark = QString("%1年%2学期期中考试")
                     .arg(currentDate.year())
                     .arg(month >= 9 || month <= 1 ? "第一" : "第二");

    // 从表格中读取数据
    QJsonArray scoresArray;
    
    // 获取列索引（只查找学号和姓名）
    int colId = -1, colName = -1;
    QMap<QString, int> attributeColumnMap; // 存储所有属性列的映射（列名 -> 列索引）
    
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
        if (!headerItem) continue;
        QString headerText = headerItem->text();
        
        if (headerText == "学号") {
            colId = col;
        } else if (headerText == "姓名") {
            colName = col;
        } else if (!headerText.isEmpty()) {
            // 其他所有列都作为属性列
            attributeColumnMap[headerText] = col;
        }
    }

    if (colId < 0 || colName < 0) {
        QMessageBox::warning(this, "错误", "表格中缺少必要列：学号、姓名！");
        return;
    }

    // 读取每一行数据
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* idItem = table->item(row, colId);
        QTableWidgetItem* nameItem = table->item(row, colName);
        
        if (!idItem || !nameItem) continue;
        
        QString studentId = idItem->text().trimmed();
        QString studentName = nameItem->text().trimmed();
        
        // 至少要有学号或姓名
        if (studentId.isEmpty() && studentName.isEmpty()) {
            continue; // 跳过空行
        }

        QJsonObject scoreObj;
        if (!studentId.isEmpty()) {
            scoreObj["student_id"] = studentId;
        }
        if (!studentName.isEmpty()) {
            scoreObj["student_name"] = studentName;
        }

        // 动态读取所有属性列（除了学号和姓名之外的所有列）
        for (auto it = attributeColumnMap.begin(); it != attributeColumnMap.end(); ++it) {
            QString columnName = it.key();
            int col = it.value();
            
            QTableWidgetItem* item = table->item(row, col);
            if (item && !item->text().trimmed().isEmpty()) {
                QString text = item->text().trimmed();
                // 尝试转换为数字
                bool ok;
                double score = text.toDouble(&ok);
                if (ok) {
                    // 直接使用列名作为字段名（支持中文列名）
                    scoreObj[columnName] = score;
                } else {
                    // 如果不是数字，作为字符串存储（支持中文列名）
                    scoreObj[columnName] = text;
                }
            }
        }

        scoresArray.append(scoreObj);
    }

    if (scoresArray.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有有效的学生数据！");
        return;
    }

    // 校验学号与姓名非空、学号不重复
    {
        QSet<QString> idSet;
        for (int row = 0; row < table->rowCount(); ++row) {
            QTableWidgetItem* idItem = table->item(row, colId);
            QTableWidgetItem* nameItem = table->item(row, colName);
            if (!idItem || !nameItem) continue;
            QString studentId = idItem->text().trimmed();
            QString studentName = nameItem->text().trimmed();
            if (studentId.isEmpty() || studentName.isEmpty()) {
                QMessageBox::warning(this, "错误", "存在学号或姓名为空的行，无法上传！");
                return;
            }
            if (idSet.contains(studentId)) {
                QMessageBox::warning(this, "错误", QString("存在重复学号：%1，无法上传！").arg(studentId));
                return;
            }
            idSet.insert(studentId);
        }
    }

    // 构造请求 JSON
    QJsonObject requestObj;
    requestObj["class_id"] = classId;
    requestObj["exam_name"] = examName;
    requestObj["term"] = term;
    if (!remark.isEmpty()) {
        requestObj["remark"] = remark;
    }
    requestObj["operation_mode"] = "replace"; // 默认使用替换模式
    
    // 添加表格说明（从 textDescription 获取）
    if (textDescription) {
        QString description = textDescription->toPlainText().trimmed();
        if (!description.isEmpty()) {
            requestObj["excel_file_description"] = description;
        }
    }
    
    requestObj["scores"] = scoresArray;
    
    // 如果有Excel文件，添加文件名
    if (!m_excelFileName.isEmpty()) {
        requestObj["excel_file_name"] = m_excelFileName;
    }
    
    // 构建 fields 数组（替换模式需要）
    QJsonArray fieldsArray;
    int fieldOrder = 1;
    QStringList excelFieldNames;
    
    // 遍历所有属性列，构建字段定义
    for (auto it = attributeColumnMap.begin(); it != attributeColumnMap.end(); ++it) {
        QString columnName = it.key();
        
        // 跳过固定列（学号、姓名）
        if (columnName == "学号" || columnName == "姓名") {
            continue;
        }
        
        QJsonObject fieldObj;
        fieldObj["field_name"] = columnName;
        fieldObj["field_type"] = "number"; // 默认为数字类型
        fieldObj["field_order"] = fieldOrder++;
        fieldObj["is_total"] = (columnName == "总分") ? 1 : 0;
        
        fieldsArray.append(fieldObj);
        excelFieldNames.append(columnName);
    }
    
    // 如果有字段定义，添加到请求中
    if (!fieldsArray.isEmpty()) {
        requestObj["fields"] = fieldsArray;
    }
    
    // excel_files 列表（每个 Excel 的字段映射与描述）
    if (!m_excelFileName.isEmpty()) {
        QJsonArray excelFilesArray;
        QJsonObject excelObj;
        excelObj["filename"] = m_excelFileName;
        excelObj["url"] = ""; // 客户端本地上传，无现成 URL
        if (textDescription) {
            QString desc = textDescription->toPlainText().trimmed();
            if (!desc.isEmpty()) {
                excelObj["description"] = desc;
            }
        }
        // 如果 excelFieldNames 为空，尝试从 fieldsArray 提取
        QJsonArray excelFieldsJson;
        if (excelFieldNames.isEmpty()) {
            for (const auto& f : fieldsArray) {
                QJsonObject fObj = f.toObject();
                QString name = fObj.value("field_name").toString();
                if (!name.isEmpty()) {
                    excelFieldsJson.append(name);
                }
            }
        } else {
            for (const QString& f : excelFieldNames) {
                excelFieldsJson.append(f);
            }
        }
        excelObj["fields"] = excelFieldsJson;
        excelFilesArray.append(excelObj);
        requestObj["excel_files"] = excelFilesArray;
    }

    QJsonDocument doc(requestObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // 发送 POST 请求
    QString url = "http://47.100.126.194:5000/student-scores/save";
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    
    QNetworkReply* reply = nullptr;
    
    // 如果有Excel文件，使用multipart/form-data格式上传
    if (!m_excelFilePath.isEmpty() && QFile::exists(m_excelFilePath)) {
        QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
        
        // 添加JSON数据部分
        QHttpPart jsonPart;
        jsonPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data\""));
        jsonPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
        jsonPart.setBody(jsonData);
        multiPart->append(jsonPart);
        
        // 添加 excel_file_name 作为独立的表单字段（确保服务器能正确获取文件名）
        if (!m_excelFileName.isEmpty()) {
            QHttpPart fileNamePart;
            fileNamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"excel_file_name\""));
            fileNamePart.setBody(m_excelFileName.toUtf8());
            multiPart->append(fileNamePart);
        }
        
        // 添加Excel文件部分
        QFile* file = new QFile(m_excelFilePath);
        if (file->open(QIODevice::ReadOnly)) {
            QHttpPart filePart;
            filePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                QVariant(QString("form-data; name=\"excel_file\"; filename=\"%1\"").arg(m_excelFileName)));
            
            // 设置MIME类型
            QMimeDatabase mimeDb;
            QMimeType mimeType = mimeDb.mimeTypeForFile(m_excelFilePath);
            filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(mimeType.name()));
            
            filePart.setBodyDevice(file);
            file->setParent(multiPart); // 确保文件在multiPart销毁时也被删除
            multiPart->append(filePart);
        } else {
            delete file;
            delete multiPart;
            QMessageBox::warning(this, "错误", "无法打开Excel文件！");
            return;
        }
        
        reply = networkManager->post(request, multiPart);
        multiPart->setParent(reply); // 确保multiPart在reply销毁时也被删除
    } else {
        // 没有Excel文件，使用JSON格式上传
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        reply = networkManager->post(request, jsonData);
    }

    // 显示上传中提示
    QMessageBox* progressMsg = new QMessageBox(this);
    progressMsg->setWindowTitle("上传中");
    QString uploadText = "正在上传成绩数据到服务器...";
    if (!m_excelFileName.isEmpty()) {
        uploadText += QString("\n包含Excel文件：%1").arg(m_excelFileName);
    }
    progressMsg->setText(uploadText);
    progressMsg->setStandardButtons(QMessageBox::NoButton);
    progressMsg->show();

    // 处理响应
    connect(reply, &QNetworkReply::finished, this, [=]() {
        progressMsg->close();
        progressMsg->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
            
            if (responseDoc.isObject()) {
                QJsonObject responseObj = responseDoc.object();
                
                // 检查是否成功（支持 code == 200 或 success == true）
                bool isSuccess = false;
                if (responseObj.contains("code")) {
                    int code = responseObj["code"].toInt();
                    isSuccess = (code == 200);
                } else if (responseObj.contains("success")) {
                    isSuccess = responseObj["success"].toBool();
                }
                
                if (isSuccess) {
                    // 尝试从 data 对象中获取数据，如果没有 data 则直接从 responseObj 获取
                    QJsonObject dataObj;
                    if (responseObj.contains("data") && responseObj["data"].isObject()) {
                        dataObj = responseObj["data"].toObject();
                    } else {
                        dataObj = responseObj;
                    }
                    
                    QString message = dataObj["message"].toString();
                    if (message.isEmpty()) {
                        message = "上传成功！";
                    }
                    
                    int insertedCount = dataObj["inserted_count"].toInt(0);
                    int updatedCount = dataObj["updated_count"].toInt(0);
                    
                    // 尝试获取 score_header_id（支持 id 或 score_header_id 字段）
                    if (dataObj.contains("id") && dataObj["id"].isDouble()) {
                        m_scoreHeaderId = dataObj["id"].toInt();
                        qDebug() << "获取到 score_header_id:" << m_scoreHeaderId;
                    } else if (dataObj.contains("score_header_id") && dataObj["score_header_id"].isDouble()) {
                        m_scoreHeaderId = dataObj["score_header_id"].toInt();
                        qDebug() << "获取到 score_header_id:" << m_scoreHeaderId;
                    } else if (responseObj.contains("score_header_id") && responseObj["score_header_id"].isDouble()) {
                        m_scoreHeaderId = responseObj["score_header_id"].toInt();
                        qDebug() << "获取到 score_header_id:" << m_scoreHeaderId;
                    }
                    
                    QString successMsg = QString("上传成功！\n\n%1").arg(message);
                    if (insertedCount > 0 || updatedCount > 0) {
                        successMsg += QString("\n新增 %1 条记录").arg(insertedCount);
                        if (updatedCount > 0) {
                            successMsg += QString("，更新 %1 条记录").arg(updatedCount);
                        }
                    }
                    
                    QMessageBox::information(this, "上传成功", successMsg);
                } else {
                    QString errorMsg = responseObj["message"].toString();
                    if (errorMsg.isEmpty()) {
                        errorMsg = "服务器返回错误";
                    }
                    QMessageBox::warning(this, "上传失败", 
                        QString("服务器返回错误：\n%1").arg(errorMsg));
                }
            } else {
                QMessageBox::information(this, "上传成功", "数据已成功上传到服务器！");
            }
        } else {
            QString errorString = reply->errorString();
            QByteArray errorData = reply->readAll();
            QString errorMsg = QString::fromUtf8(errorData);
            
            QMessageBox::critical(this, "上传失败", 
                QString("网络错误：%1\n%2").arg(errorString).arg(errorMsg));
        }
        
        reply->deleteLater();
    });
}

void MidtermGradeDialog::onCellClicked(int row, int column)
{
    QTableWidgetItem* item = table->item(row, column);
    if (!item) {
        item = new QTableWidgetItem("");
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, column, item);
    }
    
    // 获取列头名称，检查是否是学号或姓名列
    QTableWidgetItem* headerItem = table->horizontalHeaderItem(column);
    QString headerText = headerItem ? headerItem->text() : "";

    // 总分列不允许手动编辑
    if (headerText == "总分") {
        return;
    }
    
    // 如果是学号或姓名列，直接进入编辑状态
    if (headerText == "学号" || headerText == "姓名") {
        table->editItem(item);
        return;
    }
    
    QString comment = item->data(Qt::UserRole).toString();
    // 如果已有注释，显示注释窗口；否则直接进入编辑状态（双击也是直接编辑）
    if (!comment.isEmpty() && commentWidget) {
        QRect cellRect = table->visualItemRect(item);
        commentWidget->showComment(comment, cellRect, table);
        commentWidget->cancelHide();
    } else {
        // 没有注释，直接进入编辑状态（注释编辑通过右键菜单）
        table->editItem(item);
    }
}

void MidtermGradeDialog::onCellEntered(int row, int column)
{
    // 鼠标悬浮时显示注释提示窗口
    QTableWidgetItem* item = table->item(row, column);
    if (item && commentWidget) {
        QString comment = item->data(Qt::UserRole).toString();
        // 懒加载：如果单元格还没填充注释（UserRole为空），从全局缓存读取一次
        if (comment.isEmpty() && !m_classid.isEmpty()) {
            // 固定列不支持注释
            QTableWidgetItem* headerItem = table->horizontalHeaderItem(column);
            QString fieldName = headerItem ? headerItem->text() : "";
            if (!fieldName.isEmpty() && fieldName != "学号" && fieldName != "姓名" && fieldName != "总分") {
                // 找到学号列
                int colId = -1;
                for (int c = 0; c < table->columnCount(); ++c) {
                    QTableWidgetItem* h = table->horizontalHeaderItem(c);
                    if (h && h->text() == "学号") { colId = c; break; }
                }
                QString studentId;
                if (colId >= 0) {
                    QTableWidgetItem* idItem = table->item(row, colId);
                    if (idItem) studentId = idItem->text().trimmed();
                }
                if (!studentId.isEmpty()) {
                    const QString term = calcCurrentTerm();
                    comment = CommentStorage::getComment(m_classid, term, studentId, fieldName, m_excelFileName);
                    // 兼容：如果缓存里是复合键名，也尝试读一次
                    if (comment.isEmpty() && !m_excelFileName.isEmpty()) {
                        QString compositeKey = QString("%1_%2").arg(fieldName).arg(m_excelFileName);
                        comment = CommentStorage::getComment(m_classid, term, studentId, compositeKey, m_excelFileName);
                    }
                    if (!comment.isEmpty()) {
                        item->setData(Qt::UserRole, comment);
                        item->setBackground(QBrush(QColor(255, 255, 200))); // 浅黄色提示有注释
                    }
                }
            }
        }
        QRect cellRect = table->visualItemRect(item);
        commentWidget->showComment(comment, cellRect, table);
        commentWidget->cancelHide();
    }
}

void MidtermGradeDialog::onItemChanged(QTableWidgetItem* item)
{
    if (!item) return;
    if (m_isBulkUpdating) return;
    
    int row = item->row();
    int column = item->column();
    
    // 获取列头名称
    QTableWidgetItem* headerItem = table->horizontalHeaderItem(column);
    QString headerText = headerItem ? headerItem->text() : "";
    
    // 如果修改的是成绩字段，先本地更新总分，再写回服务端（/student-scores/set-score）
    if (headerText != "总分" && headerText != "学号" && headerText != "姓名" && !headerText.isEmpty()) {
        updateRowTotal(row);

        // 获取学生信息（学号/姓名）
        int colId = -1, colName = -1, colTotal = -1;
        for (int c = 0; c < table->columnCount(); ++c) {
            QTableWidgetItem* h = table->horizontalHeaderItem(c);
            if (!h) continue;
            const QString t = h->text();
            if (t == "学号") colId = c;
            else if (t == "姓名") colName = c;
            else if (t == "总分") colTotal = c;
        }
        QString studentId;
        QString studentName;
        if (colId >= 0) {
            QTableWidgetItem* idItem = table->item(row, colId);
            if (idItem) studentId = idItem->text().trimmed();
        }
        if (colName >= 0) {
            QTableWidgetItem* nameItem = table->item(row, colName);
            if (nameItem) studentName = nameItem->text().trimmed();
        }
        if (!studentId.isEmpty() || !studentName.isEmpty()) {
            setScoreToServer(row, studentName, studentId, headerText, item->text());
        }
    }
}

void MidtermGradeDialog::setScoreToServer(int row, const QString& studentName, const QString& studentId,
                                         const QString& fieldName, const QString& cellText)
{
    if (!networkManager) return;
    if (fieldName.trimmed().isEmpty()) return;
    if (studentId.trimmed().isEmpty() && studentName.trimmed().isEmpty()) return;

    // score：空字符串/空白 -> 删除（null）
    const QString trimmed = cellText.trimmed();
    QJsonValue scoreVal(QJsonValue::Null);
    if (!trimmed.isEmpty()) {
        bool ok = false;
        const double v = trimmed.toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, "提示", QString("字段“%1”只支持数字；清空表示删除。").arg(fieldName));
            return;
        }
        scoreVal = v;
    }

    QJsonObject requestObj;
    // 写法A：不传 score_header_id，改用 class_id + term 自动定位表头
    requestObj["class_id"] = m_classid;
    const QString term = calcCurrentTerm();
    if (!term.isEmpty()) requestObj["term"] = term;
    requestObj["student_name"] = studentName.trimmed();
    if (!studentId.trimmed().isEmpty()) requestObj["student_id"] = studentId.trimmed();
    requestObj["field_name"] = fieldName;
    if (!m_excelFileName.trimmed().isEmpty()) requestObj["excel_filename"] = m_excelFileName.trimmed();
    requestObj["score"] = scoreVal;

    QJsonDocument doc(requestObj);
    const QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    const QString url = "http://47.100.126.194:5000/student-scores/set-score";
    QNetworkRequest req;
    req.setUrl(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = networkManager->post(req, jsonData);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            return;
        }
        const QJsonDocument resp = QJsonDocument::fromJson(reply->readAll());
        reply->deleteLater();
        if (!resp.isObject()) return;
        const QJsonObject root = resp.object();
        if (root.value("code").toInt(-1) != 200) return;
        const QJsonObject dataObj = root.value("data").toObject();

        // 回写当前单元格 score（delete -> 清空）与该行 total_score
        int totalCol = -1;
        int fieldCol = -1;
        for (int c = 0; c < table->columnCount(); ++c) {
            QTableWidgetItem* h = table->horizontalHeaderItem(c);
            if (!h) continue;
            const QString t = h->text();
            if (t == "总分") totalCol = c;
            if (t == fieldName) fieldCol = c;
        }

        const QSignalBlocker blocker(table);
        if (fieldCol >= 0) {
            QTableWidgetItem* cellItem = table->item(row, fieldCol);
            if (!cellItem) {
                cellItem = new QTableWidgetItem("");
                cellItem->setTextAlignment(Qt::AlignCenter);
                table->setItem(row, fieldCol, cellItem);
            }
            const QJsonValue scoreV = dataObj.value(QStringLiteral("score"));
            if (scoreV.isNull()) cellItem->setText(QString());
            else if (scoreV.isDouble()) cellItem->setText(QString::number(scoreV.toDouble()));
            else if (scoreV.isString()) cellItem->setText(scoreV.toString());
        }

        if (totalCol >= 0) {
            QTableWidgetItem* totalItem = table->item(row, totalCol);
            if (!totalItem) {
                totalItem = new QTableWidgetItem("");
                totalItem->setTextAlignment(Qt::AlignCenter);
                totalItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                table->setItem(row, totalCol, totalItem);
            }
            // “总分”列：优先使用当前 Excel 表维度总分（excel_total / excel_total_score），否则回退整体 total_score
            double totalScore = 0.0;
            if (dataObj.contains(QStringLiteral("excel_total")) && !dataObj.value(QStringLiteral("excel_total")).isNull()) {
                totalScore = dataObj.value(QStringLiteral("excel_total")).toDouble(0.0);
            } else if (dataObj.contains(QStringLiteral("excel_total_score")) && !dataObj.value(QStringLiteral("excel_total_score")).isNull()) {
                totalScore = dataObj.value(QStringLiteral("excel_total_score")).toDouble(0.0);
            } else {
                totalScore = dataObj.value(QStringLiteral("total_score")).toDouble(0.0);
            }
            totalItem->setText(QString::number(totalScore));
        }
    });
}

void MidtermGradeDialog::updateRowTotal(int row)
{
    // 查找"总分"列的索引
    int totalCol = -1;
    int nameCol = -1;
    int idCol = -1;
    for (int c = 0; c < table->columnCount(); ++c) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(c);
        if (!headerItem) continue;
        QString headerText = headerItem->text();
        if (headerText == "总分") {
            totalCol = c;
        } else if (headerText == "姓名") {
            nameCol = c;
        } else if (headerText == "学号") {
            idCol = c;
        }
    }
    
    // 如果没有"总分"列，不需要更新
    if (totalCol < 0) return;
    
    // 计算该行的总分
    // 需要计算所有数值列，但排除：学号、姓名、总分列本身
    double total = 0.0;
    
    for (int col = 0; col < table->columnCount(); ++col) {
        // 跳过学号、姓名、总分列
        if (col == idCol || col == nameCol || col == totalCol) {
            continue;
        }
        
        QTableWidgetItem* item = table->item(row, col);
        if (!item) continue;
        
        // 获取列头名称，确保不是学号、姓名、总分列
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
        if (!headerItem) continue;
        QString headerText = headerItem->text();
        if (headerText == "学号" || headerText == "姓名" || headerText == "总分") {
            continue;
        }
        
        QString text = item->text().trimmed();
        bool ok;
        double value = text.toDouble(&ok);
        if (ok) {
            total += value;
        }
    }
    
    // 更新总分单元格
    QTableWidgetItem* totalItem = table->item(row, totalCol);
    if (!totalItem) {
        totalItem = new QTableWidgetItem("");
        totalItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, totalCol, totalItem);
    }
    
    // 暂时断开信号，避免递归调用
    table->blockSignals(true);
    totalItem->setText(QString::number(total));
    table->blockSignals(false);
}

void MidtermGradeDialog::onTableContextMenu(const QPoint& pos)
{
    QTableWidgetItem* item = table->itemAt(pos);
    if (!item) return;

    int row = item->row();
    QMenu menu(this);
    
    QAction* editCommentAction = menu.addAction("编辑注释");
    QAction* deleteRowAction = menu.addAction("删除行");

    QAction* selectedAction = menu.exec(table->viewport()->mapToGlobal(pos));
    
    if (selectedAction == editCommentAction) {
        showCellComment(row, item->column());
    } else if (selectedAction == deleteRowAction) {
        int ret = QMessageBox::question(this, "确认", "确定要删除这一行吗？", 
                                        QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            table->removeRow(row);
        }
    }
}

void MidtermGradeDialog::sortTable(bool ascending)
{
    int sortColumn = table->currentColumn();
    if (sortColumn < 0) {
        QMessageBox::information(this, "提示", "请先选择要排序的列");
        return;
    }

    // 获取所有行数据
    QList<QList<QTableWidgetItem*>> rowsData;
    for (int row = 0; row < table->rowCount(); ++row) {
        QList<QTableWidgetItem*> rowData;
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* item = table->item(row, col);
            if (item) {
                // 创建新的item副本
                QTableWidgetItem* newItem = item->clone();
                rowData.append(newItem);
            } else {
                rowData.append(new QTableWidgetItem(""));
            }
        }
        rowsData.append(rowData);
    }

    // 按指定列排序
    std::sort(rowsData.begin(), rowsData.end(), [sortColumn, ascending](const QList<QTableWidgetItem*>& a, const QList<QTableWidgetItem*>& b) {
        if (sortColumn >= a.size() || sortColumn >= b.size()) return false;
        QString textA = a[sortColumn] ? a[sortColumn]->text() : "";
        QString textB = b[sortColumn] ? b[sortColumn]->text() : "";
        
        // 尝试转换为数字
        bool okA, okB;
        double numA = textA.toDouble(&okA);
        double numB = textB.toDouble(&okB);
        
        if (okA && okB) {
            return ascending ? (numA < numB) : (numA > numB);
        } else {
            return ascending ? (textA < textB) : (textA > textB);
        }
    });

    // 重新设置表格数据
    table->setRowCount(rowsData.size());
    for (int row = 0; row < rowsData.size(); ++row) {
        for (int col = 0; col < rowsData[row].size(); ++col) {
            table->setItem(row, col, rowsData[row][col]);
        }
    }
}

void MidtermGradeDialog::openSeatingArrangementDialog()
{
    // 从表格中读取学生数据
    QList<StudentInfo> students;
    
    // 获取列索引（只查找学号和姓名，其他列都作为属性列）
    int colId = -1, colName = -1;
    QMap<QString, int> attributeColumns; // 存储所有属性列（列名 -> 列索引）
    
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
        if (!headerItem) continue;
        QString headerText = headerItem->text();
        
        if (headerText == "学号") {
            colId = col;
        } else if (headerText == "姓名") {
            colName = col;
        } else if (!headerText.isEmpty()) {
            // 其他所有列都作为属性列
            attributeColumns[headerText] = col;
        }
    }
    
    if (colName < 0) {
        QMessageBox::warning(this, "错误", "表格中缺少姓名列！");
        return;
    }
    
    // 读取每一行数据
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* nameItem = table->item(row, colName);
        if (!nameItem || nameItem->text().trimmed().isEmpty()) {
            continue; // 跳过空行
        }
        
        StudentInfo student;
        student.name = nameItem->text().trimmed();
        if (colId >= 0) {
            QTableWidgetItem* idItem = table->item(row, colId);
            if (idItem) {
                student.id = idItem->text().trimmed();
            }
        }
        
        // 读取所有属性列并填充到 attributes 中
        double totalScore = 0.0;
        for (auto it = attributeColumns.begin(); it != attributeColumns.end(); ++it) {
            QString columnName = it.key();
            int col = it.value();
            QTableWidgetItem* item = table->item(row, col);
            if (item && !item->text().trimmed().isEmpty()) {
                bool ok;
                double score = item->text().toDouble(&ok);
                if (ok) {
                    student.attributes[columnName] = score;
                    // 如果列名是"总分"，则作为排序依据
                    if (columnName == "总分") {
                        student.score = score;
                    } else {
                        // 累加所有数值列作为总分（如果总分列不存在）
                        totalScore += score;
                    }
                }
            }
        }
        
        // 如果没有"总分"列，使用累加值
        if (student.score == 0 && totalScore > 0) {
            student.score = totalScore;
        }
        
        student.originalIndex = students.size();
        students.append(student);
    }
    
    if (students.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有有效的学生数据！");
        return;
    }
    
    // 找到所有打开的 ScheduleDialog 实例
    QList<ScheduleDialog*> scheduleDialogs;
    QWidgetList widgets = QApplication::allWidgets();
    for (QWidget* widget : widgets) {
        ScheduleDialog* scheduleDlg = qobject_cast<ScheduleDialog*>(widget);
        if (scheduleDlg && !scheduleDlg->isHidden()) {
            scheduleDialogs.append(scheduleDlg);
            qDebug() << "找到打开的 ScheduleDialog:" << scheduleDlg;
        }
    }
    
    qDebug() << "共找到" << scheduleDialogs.size() << "个打开的 ScheduleDialog 实例";
    
    if (scheduleDialogs.isEmpty()) {
        QMessageBox::information(this, "提示", "请先打开一个班级群聊窗口！");
        return;
    }
    
    // 显示排座方式选择对话框
    ArrangeSeatDialog* arrangeDialog = new ArrangeSeatDialog(this);
    if (arrangeDialog->exec() == QDialog::Accepted) {
        QString method = arrangeDialog->getSelectedMethod();
        qDebug() << "用户选择的排座方式:" << method;
        qDebug() << "学生数据数量:" << students.size();
        
        // 对所有打开的 ScheduleDialog 执行排座
        for (ScheduleDialog* scheduleDlg : scheduleDialogs) {
            qDebug() << "正在为 ScheduleDialog 执行排座...";
            scheduleDlg->arrangeSeats(students, method);
        }
        
        QMessageBox::information(this, "排座完成", 
            QString("已为 %1 个班级群聊窗口完成排座！\n排座方式：%2").arg(scheduleDialogs.size()).arg(method), 
            QMessageBox::Ok);
    }
    
    arrangeDialog->deleteLater();
}

void MidtermGradeDialog::showCellComment(int row, int column)
{
    QTableWidgetItem* item = table->item(row, column);
    if (!item) {
        item = new QTableWidgetItem("");
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, column, item);
    }

    // 获取学生信息
    int colId = -1, colName = -1;
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
        if (!headerItem) continue;
        QString headerText = headerItem->text();
        if (headerText == "学号") {
            colId = col;
        } else if (headerText == "姓名") {
            colName = col;
        }
    }
    
    QString studentName;
    QString studentId;
    if (colName >= 0) {
        QTableWidgetItem* nameItem = table->item(row, colName);
        if (nameItem) {
            studentName = nameItem->text().trimmed();
        }
    }
    if (colId >= 0) {
        QTableWidgetItem* idItem = table->item(row, colId);
        if (idItem) {
            studentId = idItem->text().trimmed();
        }
    }
    
    // 获取字段名称（列头）
    QString fieldName;
    QTableWidgetItem* headerItem = table->horizontalHeaderItem(column);
    if (headerItem) {
        fieldName = headerItem->text();
    }
    
    // 跳过固定列（学号、姓名、总分）
    if (fieldName == "学号" || fieldName == "姓名" || fieldName == "总分" || fieldName.isEmpty()) {
        QMessageBox::information(this, "提示", "该列不支持设置注释！");
        return;
    }

    QString currentComment = item->data(Qt::UserRole).toString();
    // 懒加载：右键编辑前，若当前为空则从全局缓存补一次（避免 importData 先于缓存写入导致的空白）
    if (currentComment.isEmpty() && !m_classid.isEmpty() && !studentId.isEmpty() && !fieldName.isEmpty()) {
        const QString term = calcCurrentTerm();
        currentComment = CommentStorage::getComment(m_classid, term, studentId, fieldName, m_excelFileName);
        if (currentComment.isEmpty() && !m_excelFileName.isEmpty()) {
            QString compositeKey = QString("%1_%2").arg(fieldName).arg(m_excelFileName);
            currentComment = CommentStorage::getComment(m_classid, term, studentId, compositeKey, m_excelFileName);
        }
        if (!currentComment.isEmpty()) {
            item->setData(Qt::UserRole, currentComment);
            item->setBackground(QBrush(QColor(255, 255, 200)));
        }
    }
    bool ok;
    QString comment = CommentInputDialog::getMultiLineText(this, "单元格注释", 
                                                           QString("单元格 (%1, %2) 的注释:").arg(row + 1).arg(column + 1),
                                                           currentComment, &ok);
    if (ok) {
        item->setData(Qt::UserRole, comment);
        // 如果有注释，用特殊背景色标记
        if (!comment.isEmpty()) {
            item->setBackground(QBrush(QColor(255, 255, 200))); // 浅黄色
        } else {
            item->setBackground(QBrush()); // 恢复默认
        }
        
        // 调用服务器接口设置注释（点击确定按钮时发送）
        if (!studentName.isEmpty() && !fieldName.isEmpty()) {
            setCommentToServer(studentName, studentId, fieldName, comment);
        }
    }
}

// 重写鼠标事件以实现窗口拖动
void MidtermGradeDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void MidtermGradeDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && !m_dragPosition.isNull()) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

// 鼠标进入窗口时显示关闭按钮
void MidtermGradeDialog::enterEvent(QEvent *event)
{
    if (m_btnClose) {
        m_btnClose->show();
    }
    QDialog::enterEvent(event);
}

// 鼠标离开窗口时隐藏关闭按钮
void MidtermGradeDialog::leaveEvent(QEvent *event)
{
    // 检查鼠标是否真的离开了窗口（包括关闭按钮）
    QPoint globalPos = QCursor::pos();
    QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
    if (!widgetRect.contains(globalPos) && m_btnClose) {
        // 如果鼠标不在窗口内，检查是否在关闭按钮上
        QRect btnRect = QRect(m_btnClose->mapToGlobal(QPoint(0, 0)), m_btnClose->size());
        if (!btnRect.contains(globalPos)) {
            m_btnClose->hide();
        }
    }
    QDialog::leaveEvent(event);
}

// 事件过滤器，处理关闭按钮的鼠标事件和表格的鼠标移动事件
bool MidtermGradeDialog::eventFilter(QObject *obj, QEvent *event)
{
    // 先处理关闭按钮的事件
    if (obj == m_btnClose) {
        if (event->type() == QEvent::Enter) {
            // 鼠标进入关闭按钮时确保显示
            m_btnClose->show();
        } else if (event->type() == QEvent::Leave) {
            // 鼠标离开关闭按钮时，检查是否还在窗口内
            QPoint globalPos = QCursor::pos();
            QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
            if (!widgetRect.contains(globalPos)) {
                m_btnClose->hide();
            }
        }
        return QDialog::eventFilter(obj, event);
    }
    
    // 处理表格的事件（鼠标移动显示注释）
    if (obj == table) {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QTableWidgetItem* item = table->itemAt(mouseEvent->pos());
            if (item && commentWidget) {
                int row = item->row();
                int col = item->column();
                QString comment = item->data(Qt::UserRole).toString();
                // 懒加载：鼠标移动到单元格时再从全局缓存补注释
                if (comment.isEmpty() && !m_classid.isEmpty()) {
                    QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
                    QString fieldName = headerItem ? headerItem->text() : "";
                    if (!fieldName.isEmpty() && fieldName != "学号" && fieldName != "姓名" && fieldName != "总分") {
                        int colId = -1;
                        for (int c = 0; c < table->columnCount(); ++c) {
                            QTableWidgetItem* h = table->horizontalHeaderItem(c);
                            if (h && h->text() == "学号") { colId = c; break; }
                        }
                        QString studentId;
                        if (colId >= 0) {
                            QTableWidgetItem* idItem = table->item(row, colId);
                            if (idItem) studentId = idItem->text().trimmed();
                        }
                        if (!studentId.isEmpty()) {
                            const QString term = calcCurrentTerm();
                            comment = CommentStorage::getComment(m_classid, term, studentId, fieldName, m_excelFileName);
                            if (comment.isEmpty() && !m_excelFileName.isEmpty()) {
                                QString compositeKey = QString("%1_%2").arg(fieldName).arg(m_excelFileName);
                                comment = CommentStorage::getComment(m_classid, term, studentId, compositeKey, m_excelFileName);
                            }
                            if (!comment.isEmpty()) {
                                item->setData(Qt::UserRole, comment);
                                item->setBackground(QBrush(QColor(255, 255, 200)));
                            }
                        }
                    }
                }
                QRect cellRect = table->visualItemRect(item);
                commentWidget->showComment(comment, cellRect, table);
                commentWidget->cancelHide();
            } else if (commentWidget) {
                commentWidget->hideWithDelay(500);
            }
        } else if (event->type() == QEvent::Leave) {
            // 鼠标离开表格时，延迟隐藏注释窗口
            if (commentWidget) {
                commentWidget->hideWithDelay(1000);
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

// 窗口大小改变时更新关闭按钮位置
void MidtermGradeDialog::resizeEvent(QResizeEvent *event)
{
    if (m_btnClose) {
        m_btnClose->move(width() - 35, 5);
    }
    QDialog::resizeEvent(event);
}

// 窗口显示时更新关闭按钮位置
void MidtermGradeDialog::showEvent(QShowEvent *event)
{
    if (m_btnClose) {
        m_btnClose->move(width() - 35, 5);
        // 窗口显示时也显示关闭按钮
        m_btnClose->show();
    }
    
    // 确保窗口位置在屏幕可见区域内
    QRect screenGeometry = QApplication::desktop()->availableGeometry();
    QRect windowGeometry = geometry();
    
    // 如果窗口完全在屏幕外，移动到屏幕中央
    if (!screenGeometry.intersects(windowGeometry)) {
        move(screenGeometry.center() - QPoint(windowGeometry.width() / 2, windowGeometry.height() / 2));
    }
    
    // 确保窗口显示在最前面
    raise();
    activateWindow();
    QDialog::showEvent(event);
}

void MidtermGradeDialog::setCommentToServer(const QString& studentName, const QString& studentId, 
                                            const QString& fieldName, const QString& comment)
{
    if (!networkManager) return;
    if (fieldName.trimmed().isEmpty()) return;
    if (studentId.trimmed().isEmpty() && studentName.trimmed().isEmpty()) return;
    
    // 构造请求 JSON
    QJsonObject requestObj;
    // 不传 score_header_id，改用 class_id + term + excel_filename 自动定位
    if (!m_classid.trimmed().isEmpty()) requestObj["class_id"] = m_classid.trimmed();
    const QString term = calcCurrentTerm();
    if (!term.trimmed().isEmpty()) requestObj["term"] = term.trimmed();
    requestObj["student_name"] = studentName.trimmed();
    if (!studentId.trimmed().isEmpty()) {
        requestObj["student_id"] = studentId.trimmed();
    }
    requestObj["field_name"] = fieldName;
    if (!m_excelFileName.trimmed().isEmpty()) requestObj["excel_filename"] = m_excelFileName.trimmed();
    requestObj["comment"] = comment;
    
    QJsonDocument doc(requestObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送 POST 请求
    QString url = "http://47.100.126.194:5000/student-scores/set-comment";
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = networkManager->post(request, jsonData);
    
    // 处理响应
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
            
            if (responseDoc.isObject()) {
                QJsonObject responseObj = responseDoc.object();
                int code = responseObj["code"].toInt();
                
                if (code == 200) {
                    qDebug() << "注释设置成功:" << studentName << fieldName;
                    // 可以更新本地存储的注释信息
                    QJsonObject dataObj = responseObj["data"].toObject();
                    if (dataObj.contains("comments_json") && dataObj["comments_json"].isObject()) {
                        QJsonObject commentsJson = dataObj["comments_json"].toObject();
                        // 可以在这里更新本地注释缓存
                    }
                } else {
                    QString errorMsg = responseObj["message"].toString();
                    qDebug() << "设置注释失败:" << errorMsg;
                    QMessageBox::warning(this, "设置注释失败", 
                        QString("服务器返回错误：\n%1").arg(errorMsg));
                }
            }
        } else {
            QString errorString = reply->errorString();
            qDebug() << "网络错误:" << errorString;
            // 不显示错误提示，避免打断用户操作
        }
        
        reply->deleteLater();
    });
}

