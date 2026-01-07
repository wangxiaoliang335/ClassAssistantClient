#include "ScheduleDialog.h"
#include "GroupScoreDialog.h"
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
#include "QXlsx/header/xlsxdocument.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QApplication>
#include <QDebug>
#include <QDate>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QMimeDatabase>
#include <QMimeType>
#include <QCoreApplication>

static QString computeCurrentTermString()
{
    QDate currentDate = QDate::currentDate();
    int year = currentDate.year();
    int month = currentDate.month();
    QString term;
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

GroupScoreDialog::GroupScoreDialog(QString classid, QWidget* parent) : QDialog(parent)
{
    // 去掉标题栏
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setWindowTitle("小组管理表");
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
    m_lblTitle = new QLabel("小组管理表");
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
    textDescription->setPlainText("说明:该表为小组管理表。第一列为组号或小组，倒数第二列为个人总分，最后一列为小组总分");
    mainLayout->addWidget(textDescription);

    // 表格
    table = new GroupScoreTableWidget(this); // 使用自定义表格控件
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);   // 允许多选
    table->setSelectionBehavior(QAbstractItemView::SelectItems);     // 支持按行/列/单元格选择
    table->horizontalHeader()->setSectionsClickable(true);           // 允许点击表头选列
    table->verticalHeader()->setSectionsClickable(true);             // 允许点击行号选行
    // 统一选中高亮颜色
    table->setStyleSheet("QTableWidget::item:selected { background-color: #4A90E2; color: white; }");

    mainLayout->addWidget(table);

    // 固定列索引（不能删除的列）- 初始为空，导入数据时根据实际列头动态设置
    fixedColumns.clear();
    nameColumnIndex = -1; // 姓名列索引，导入数据时设置
    groupColumnIndex = -1; // 组号列索引
    totalColumnIndex = -1; // 个人总分列索引
    groupTotalColumnIndex = -1; // 小组总分列索引

    // 连接信号和槽
    connect(btnAddRow, &QPushButton::clicked, this, &GroupScoreDialog::onAddRow);
    connect(btnDeleteRow, &QPushButton::clicked, this, &GroupScoreDialog::onDeleteRow);
    connect(btnDeleteColumn, &QPushButton::clicked, this, &GroupScoreDialog::onDeleteColumn);
    connect(btnAddColumn, &QPushButton::clicked, this, &GroupScoreDialog::onAddColumn);
    connect(btnFontColor, &QPushButton::clicked, this, &GroupScoreDialog::onFontColor);
    connect(btnBgColor, &QPushButton::clicked, this, &GroupScoreDialog::onBgColor);
    connect(btnDescOrder, &QPushButton::clicked, this, &GroupScoreDialog::onDescOrder);
    connect(btnAscOrder, &QPushButton::clicked, this, &GroupScoreDialog::onAscOrder);
    connect(btnExport, &QPushButton::clicked, this, &GroupScoreDialog::onExport);
    connect(btnUpload, &QPushButton::clicked, this, [this]() {
        qDebug() << "上传按钮被点击！";
        this->onUpload();
    });
    
    // 初始化网络管理器
    networkManager = new QNetworkAccessManager(this);

    // 单元格点击和悬浮事件
    connect(table, &QTableWidget::cellClicked, this, &GroupScoreDialog::onCellClicked);
    connect(table, &QTableWidget::cellEntered, this, &GroupScoreDialog::onCellEntered);
    table->setMouseTracking(true);
    
    // 监听单元格内容变化，自动更新总分
    connect(table, &QTableWidget::itemChanged, this, &GroupScoreDialog::onItemChanged);

    // 创建注释窗口
    commentWidget = new CellCommentWidget(this);
    
    // 安装事件过滤器以处理鼠标悬浮
    table->installEventFilter(this);

    // 右键菜单用于删除行
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table, &QTableWidget::customContextMenuRequested, this, &GroupScoreDialog::onTableContextMenu);
}

void GroupScoreDialog::importData(const QStringList& headers, const QList<QStringList>& dataRows, const QString& excelFilePath)
{
    if (!table) return;

    // 导入属于批量更新：避免 itemChanged 触发大量 updateGroupTotal
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
        // 服务器缓存刷新模式下可能不会有本地文件路径，但我们仍需要 m_excelFileName 用于 set-score/set-comment。
        // 因此：仅当当前也没有已知文件名时才清空。
        if (m_excelFileName.isEmpty()) {
            m_excelFileName.clear();
        }
    }

    // 根据导入的Excel列头动态设置表格列头
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    
    // 根据实际列头动态生成说明文本
    QStringList attributeColumns; // 属性列（除了组号/小组、学号、姓名、总分、小组总分）
    for (const QString& header : headers) {
        if (header != "组号" && header != "小组" && header != "学号" && header != "姓名" && header != "总分" && header != "小组总分" && !header.isEmpty()) {
            attributeColumns.append(header);
        }
    }
    
    QString description = "说明:该表为小组管理表。";
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
    // 小组管理表：第一列为组号或小组，倒数第二列为个人总分，最后一列为小组总分
    int colGroup = -1, colId = -1, colName = -1, colTotal = -1, colGroupTotal = -1;
    
    // 在导入的列头中找到对应的列
    for (int col = 0; col < headers.size(); ++col) {
        QString headerText = headers[col];
        
        if (headerText == "组号" || headerText == "小组") {
            colGroup = col;
            groupColumnIndex = col;
            fixedColumns.insert(col); // 组号/小组是固定列
        }
        else if (headerText == "学号") {
            colId = col;
            fixedColumns.insert(col); // 学号是固定列
        }
        else if (headerText == "姓名") {
            colName = col;
            fixedColumns.insert(col); // 姓名是固定列
            nameColumnIndex = col; // 设置姓名列索引
        }
        else if (headerText == "总分") {
            colTotal = col;
            totalColumnIndex = col;
            fixedColumns.insert(col); // 个人总分是固定列
        }
        else if (headerText == "小组总分") {
            colGroupTotal = col;
            groupTotalColumnIndex = col;
            fixedColumns.insert(col); // 小组总分是固定列
        }
    }
    
    // 确保必要的列都存在（组号、学号、姓名是必需的）
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
            
            // 组号/小组、学号、姓名列：不可编辑，清空注释
            if (headerText == "组号" || headerText == "小组" || headerText == "学号" || headerText == "姓名") {
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                item->setData(Qt::UserRole, QVariant());
                continue;
            }

            // 总分/小组总分列：不可编辑（由程序/服务端计算），清空注释
            if (headerText == "总分" || headerText == "小组总分") {
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                item->setData(Qt::UserRole, QVariant());
                continue;
            }
            
            // 如果是属性列（非组号/小组、学号、姓名、总分、小组总分），尝试从全局存储中获取注释
            if (headerText != "组号" && headerText != "小组" && headerText != "总分" && headerText != "小组总分") {
                // 获取学生学号（从学号列）
                QString studentId = "";
                if (colId >= 0 && colId < rowData.size()) {
                    studentId = rowData[colId];
                }
                
                // 如果学号不为空，尝试从全局存储中获取注释
                if (!studentId.isEmpty() && m_classid.isEmpty() == false) {
                    const QString term = m_term.isEmpty() ? computeCurrentTermString() : m_term;
                    
                    // 从全局存储中获取注释：需要带表格名，避免同名字段在不同表覆盖
                    const QString tableNameForComments = !m_excelFileName.isEmpty() ? m_excelFileName : QString();
                    QString comment = CommentStorage::getComment(m_classid, term, studentId, headerText, tableNameForComments);
                    if (!comment.isEmpty()) {
                        item->setData(Qt::UserRole, comment);
                    }
                }
            }
        }
    }
    
    // 导入完成后，更新所有行的个人总分和小组总分
    for (int row = 0; row < table->rowCount(); ++row) {
        updateRowTotal(row);
    }
    updateGroupTotal();

    // 批量更新结束：恢复正常行为
    m_isBulkUpdating = false;
    
    // 确保表格可见并刷新显示
    table->setVisible(true);
    table->show();
    table->update();
    table->repaint();
    
    qDebug() << "导入完成，共" << table->rowCount() << "行数据";
}

void GroupScoreDialog::onAddRow()
{
    int currentRow = table->currentRow();
    if (currentRow < 0) {
        currentRow = table->rowCount() - 1;
    }
    int insertRow = currentRow + 1;
    table->insertRow(insertRow);

    // 获取组号、学号、姓名列的索引
    int colGroup = -1, colId = -1, colName = -1;
    for (int c = 0; c < table->columnCount(); ++c) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(c);
        if (!headerItem) continue;
        QString headerText = headerItem->text();
        if (headerText == "组号" || headerText == "小组") {
            colGroup = c;
        } else if (headerText == "学号") {
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
        
        // 组号/小组、学号、姓名列去掉注释（保持可编辑），其他列默认值为"0"
        if (headerText == "组号" || headerText == "小组" || headerText == "学号" || headerText == "姓名") {
            item->setData(Qt::UserRole, QVariant());
        } else if (headerText == "总分" || headerText == "小组总分") {
            // 总分和小组总分列初始为空，会自动计算
            item->setData(Qt::UserRole, QVariant());
            // 总分/小组总分列不可编辑
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        } else {
            // 除了组号、学号、姓名、总分、小组总分，其他字段默认值都为0
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
    
    // 更新该行的个人总分和小组总分
    updateRowTotal(insertRow);
    updateGroupTotal();
}

void GroupScoreDialog::onDeleteRow()
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
    
    // 删除行后，更新小组总分
    updateGroupTotal();
}

void GroupScoreDialog::onDeleteColumn()
{
    int currentCol = table->currentColumn();
    if (currentCol < 0) {
        QMessageBox::information(this, "提示", "请先选择要删除的列");
        return;
    }

    // 检查是否是固定列（组号、学号、姓名、总分、小组总分）
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
        if (groupColumnIndex > currentCol) {
            groupColumnIndex--;
        }
        if (totalColumnIndex > currentCol) {
            totalColumnIndex--;
        }
        if (groupTotalColumnIndex > currentCol) {
            groupTotalColumnIndex--;
        }
        
        // 删除列后，更新所有行的个人总分和小组总分
        for (int row = 0; row < table->rowCount(); ++row) {
            updateRowTotal(row);
        }
        updateGroupTotal();
    }
}

void GroupScoreDialog::onAddColumn()
{
    bool ok;
    QString columnName = QInputDialog::getText(this, "添加列", "请输入列名:", 
                                               QLineEdit::Normal, "", &ok);
    if (!ok || columnName.isEmpty()) {
        return;
    }

    // 在姓名列后添加（如果姓名列存在），但要确保在总分列之前
    int insertCol = nameColumnIndex >= 0 ? nameColumnIndex + 1 : table->columnCount();
    // 如果总分列存在，确保新列插入在总分列之前
    if (totalColumnIndex >= 0 && insertCol > totalColumnIndex) {
        insertCol = totalColumnIndex;
    }
    table->insertColumn(insertCol);
    table->setHorizontalHeaderItem(insertCol, new QTableWidgetItem(columnName));

    // 初始化新列的所有单元格
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* item = new QTableWidgetItem("0"); // 新列默认值为0
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, insertCol, item);
    }

    // 更新固定列索引（插入列后，插入位置之后的固定列索引会变化）
    QSet<int> newFixedColumns;
    for (int col : fixedColumns) {
        if (col < insertCol) {
            newFixedColumns.insert(col); // 插入位置之前的列索引不变
        } else {
            newFixedColumns.insert(col + 1); // 插入位置之后的列索引+1
        }
    }
    fixedColumns = newFixedColumns;
    if (nameColumnIndex >= insertCol) {
        nameColumnIndex++;
    }
    if (groupColumnIndex >= insertCol) {
        groupColumnIndex++;
    }
    if (totalColumnIndex >= insertCol) {
        totalColumnIndex++;
    }
    if (groupTotalColumnIndex >= insertCol) {
        groupTotalColumnIndex++;
    }
    
    // 添加列后，更新所有行的个人总分和小组总分
    for (int row = 0; row < table->rowCount(); ++row) {
        updateRowTotal(row);
    }
    updateGroupTotal();
}

void GroupScoreDialog::onFontColor()
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

void GroupScoreDialog::onBgColor()
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

void GroupScoreDialog::onDescOrder()
{
    sortTable(false); // false表示倒序
}

void GroupScoreDialog::onAscOrder()
{
    sortTable(true); // true表示正序
}

void GroupScoreDialog::onExport()
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

void GroupScoreDialog::onUpload()
{
    qDebug() << "onUpload() 方法被调用！";
    qDebug() << "Excel文件路径:" << m_excelFilePath;
    qDebug() << "Excel文件名:" << m_excelFileName;

    // 确保有可写入的本地Excel路径；如果不存在则创建文件并保存当前表格
    {
        QString excelPath = m_excelFilePath;
        QString excelName = m_excelFileName;

        // 目标目录：appDir/excel_files/<schoolId>/<classId>/group/（小组表格保存到group子目录）
        UserInfo userInfo = CommonInfo::GetData();
        QString schoolId = userInfo.schoolId;
        QString classId = m_classid;
        QString baseDir = QCoreApplication::applicationDirPath() + "/excel_files";
        QString targetDir;
        if (!schoolId.isEmpty() && !classId.isEmpty()) {
            targetDir = baseDir + "/" + schoolId + "/" + classId + "/group";
        }

        if (excelName.isEmpty()) {
            excelName = QString("小组管理表_%1.xlsx").arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));
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

    // 考试名称默认为"小组管理"
    QString examName = "小组管理";

    // 根据当前日期计算学期
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
    
    qDebug() << "当前日期:" << currentDate.toString("yyyy-MM-dd") << "，计算出的学期:" << term;

    // 备注（可选）
    QString remark = QString("%1年%2学期小组管理")
                     .arg(currentDate.year())
                     .arg(month >= 9 || month <= 1 ? "第一" : "第二");

    // 获取列索引
    int colGroup = -1, colId = -1, colName = -1, colTotal = -1, colGroupTotal = -1;
    QMap<QString, int> attributeColumnMap; // 存储所有属性列的映射（列名 -> 列索引）
    
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
        if (!headerItem) continue;
        QString headerText = headerItem->text();
        
        if (headerText == "组号" || headerText == "小组") {
            colGroup = col;
        } else if (headerText == "学号") {
            colId = col;
        } else if (headerText == "姓名") {
            colName = col;
        } else if (headerText == "总分") {
            colTotal = col;
        } else if (headerText == "小组总分") {
            colGroupTotal = col;
        } else if (!headerText.isEmpty()) {
            // 其他所有列都作为属性列
            attributeColumnMap[headerText] = col;
        }
    }

    if (colId < 0 || colName < 0) {
        QMessageBox::warning(this, "错误", "表格中缺少必要列：学号、姓名！");
        return;
    }

    // 按组号分组数据
    QMap<QString, QJsonArray> groupScoresMap; // 组号 -> 该组的学生成绩数组
    
    qDebug() << "开始遍历表格，总行数:" << table->rowCount();
    
    QString currentGroupName = ""; // 当前组号，用于向下填充
    
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

        // 获取组号
        QString groupName = "";
        if (colGroup >= 0) {
            QTableWidgetItem* groupItem = table->item(row, colGroup);
            if (groupItem) {
                QString groupText = groupItem->text().trimmed();
                // 如果当前行的组号不为空，更新当前组号
                if (!groupText.isEmpty()) {
                    currentGroupName = groupText;
                    groupName = currentGroupName;
                } else {
                    // 如果当前行的组号为空，使用之前保存的组号（向下填充）
                    groupName = currentGroupName;
                }
            } else {
                // 如果单元格不存在，使用之前保存的组号（向下填充）
                groupName = currentGroupName;
            }
        }
        
        // 如果组号仍为空（说明从第一行开始就没有组号），跳过该行
        if (groupName.isEmpty()) {
            qDebug() << "第" << row << "行组号为空且没有可用的前序组号，跳过";
            continue;
        }
        
        qDebug() << "处理第" << row << "行，组号:" << groupName << "，学号:" << studentId << "，姓名:" << studentName;

        QJsonObject scoreObj;
        if (!studentId.isEmpty()) {
            scoreObj["student_id"] = studentId;
        }
        if (!studentName.isEmpty()) {
            scoreObj["student_name"] = studentName;
        }

        // 构建嵌套的 scores 对象
        QJsonObject scoresNested;
        
        // 读取所有属性列（除了组号/小组、学号、姓名、总分、小组总分）
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
                    scoresNested[columnName] = score;
                } else {
                    // 如果不是数字，作为字符串存储（支持中文列名）
                    scoresNested[columnName] = text;
                }
            }
        }
        
        // 添加个人总分（如果存在）
        if (colTotal >= 0) {
            QTableWidgetItem* totalItem = table->item(row, colTotal);
            if (totalItem && !totalItem->text().trimmed().isEmpty()) {
                QString totalText = totalItem->text().trimmed();
                bool ok;
                double totalScore = totalText.toDouble(&ok);
                if (ok) {
                    scoresNested["总分"] = totalScore;
                }
            }
        }
        
        // 将嵌套的 scores 对象添加到 scoreObj（即使为空也添加，确保学生信息被上传）
        scoreObj["scores"] = scoresNested;

        // 按组号分组
        if (!groupScoresMap.contains(groupName)) {
            groupScoresMap[groupName] = QJsonArray();
        }
        groupScoresMap[groupName].append(scoreObj);
        
        qDebug() << "添加学生到组" << groupName << ":" << studentId << studentName << "，当前组学生数:" << groupScoresMap[groupName].size();
    }

    if (groupScoresMap.isEmpty()) {
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

    // 构造请求 JSON - 按组号分组
    QJsonArray groupScoresArray;
    qDebug() << "开始构造group_scores数组，共有" << groupScoresMap.size() << "个组";
    for (auto it = groupScoresMap.begin(); it != groupScoresMap.end(); ++it) {
        QString groupName = it.key();
        QJsonArray studentsArray = it.value();
        
        qDebug() << "组" << groupName << "包含" << studentsArray.size() << "个学生";
        
        // 获取小组总分（从该组第一行的数据中获取，或者计算）
        double groupTotalScore = 0.0;
        if (colGroupTotal >= 0) {
            // 找到该组的第一行
            for (int row = 0; row < table->rowCount(); ++row) {
                QTableWidgetItem* groupItem = table->item(row, colGroup);
                if (groupItem && groupItem->text().trimmed() == groupName) {
                    QTableWidgetItem* groupTotalItem = table->item(row, colGroupTotal);
                    if (groupTotalItem && !groupTotalItem->text().trimmed().isEmpty()) {
                        bool ok;
                        groupTotalScore = groupTotalItem->text().trimmed().toDouble(&ok);
                        if (!ok) groupTotalScore = 0.0;
                    }
                    break;
                }
            }
        }
        
        QJsonObject groupObj;
        groupObj["group_name"] = groupName;
        groupObj["group_total_score"] = groupTotalScore;
        groupObj["students"] = studentsArray;
        groupScoresArray.append(groupObj);
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
    
    requestObj["group_scores"] = groupScoresArray;
    
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
        
        // 跳过固定列（组号/小组、学号、姓名、总分、小组总分）
        if (columnName == "组号" || columnName == "小组" || columnName == "学号" || columnName == "姓名" || 
            columnName == "总分" || columnName == "小组总分") {
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
    
    // 添加总分字段（如果存在）
    if (colTotal >= 0) {
        QJsonObject fieldObj;
        fieldObj["field_name"] = "总分";
        fieldObj["field_type"] = "number";
        fieldObj["field_order"] = fieldOrder++;
        fieldObj["is_total"] = 1;
        fieldsArray.append(fieldObj);
        excelFieldNames.append("总分");
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
        QJsonArray excelFieldsJson;
        for (const QString& f : excelFieldNames) {
            excelFieldsJson.append(f);
        }
        excelObj["fields"] = excelFieldsJson;
        excelFilesArray.append(excelObj);
        requestObj["excel_files"] = excelFilesArray;
    }

    QJsonDocument doc(requestObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // 发送 POST 请求 - 使用 /group-scores/save 接口
    QString url = "http://47.100.126.194:5000/group-scores/save";
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

void GroupScoreDialog::onCellClicked(int row, int column)
{
    QTableWidgetItem* item = table->item(row, column);
    if (!item) {
        item = new QTableWidgetItem("");
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, column, item);
    }
    
    // 获取列头名称，检查是否是组号、学号或姓名列
    QTableWidgetItem* headerItem = table->horizontalHeaderItem(column);
    QString headerText = headerItem ? headerItem->text() : "";

    // 总分/小组总分列不允许手动编辑
    if (headerText == "总分" || headerText == "小组总分") {
        return;
    }
    
    // 如果是组号/小组、学号或姓名列，直接进入编辑状态
    if (headerText == "组号" || headerText == "小组" || headerText == "学号" || headerText == "姓名") {
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

void GroupScoreDialog::onCellEntered(int row, int column)
{
    // 鼠标悬浮时显示注释提示窗口
    QTableWidgetItem* item = table->item(row, column);
    if (item && commentWidget) {
        QString comment = item->data(Qt::UserRole).toString();
        QRect cellRect = table->visualItemRect(item);
        commentWidget->showComment(comment, cellRect, table);
        commentWidget->cancelHide();
    }
}

void GroupScoreDialog::onItemChanged(QTableWidgetItem* item)
{
    if (!item) return;
    if (m_isBulkUpdating) return;
    
    int row = item->row();
    int column = item->column();
    
    // 获取列头名称
    QTableWidgetItem* headerItem = table->horizontalHeaderItem(column);
    QString headerText = headerItem ? headerItem->text() : "";

    // 如果是属性列（可编辑分数），同步到服务器（单字段更新）
    if (headerText != "总分" && headerText != "小组总分" && headerText != "组号" && headerText != "小组" &&
        headerText != "学号" && headerText != "姓名" && !headerText.isEmpty()) {

        // 获取学生信息（学号优先）
        int colId = -1, colName = -1;
        for (int c = 0; c < table->columnCount(); ++c) {
            QTableWidgetItem* h = table->horizontalHeaderItem(c);
            if (!h) continue;
            const QString t = h->text();
            if (t == "学号") colId = c;
            else if (t == "姓名") colName = c;
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

        // 注意：这里不强制要求姓名，但至少需要 studentId 或 studentName 之一
        if (!studentId.isEmpty() || !studentName.isEmpty()) {
            setScoreToServer(row, studentName, studentId, headerText, item->text());
        }
    }
    
    // 如果修改的不是"总分"列、"小组总分"列、"组号/小组"列、"学号"列、"姓名"列，则更新该行的总分和小组总分
    if (headerText != "总分" && headerText != "小组总分" && headerText != "组号" && headerText != "小组" &&
        headerText != "学号" && headerText != "姓名") {
        updateRowTotal(row);
        updateGroupTotal();
    } else if (headerText == "总分") {
        // 如果修改的是个人总分，只更新小组总分
        updateGroupTotal();
    }
}

void GroupScoreDialog::setScoreToServer(int row, const QString& studentName, const QString& studentId,
                                       const QString& fieldName, const QString& cellText)
{
    if (!networkManager) return;
    if (m_classid.trimmed().isEmpty()) return;
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

    // table_name / excel_filename：优先使用 excel_filename（带 .xlsx），其次使用 table_name（不带扩展名）
    const QString excelFilename = m_excelFileName.trimmed();
    // 优先使用 windowTitle（CustomListDialog 会把它设为真实表名）；其次才用 m_lblTitle
    QString tableName = windowTitle().trimmed();
    if (tableName.isEmpty()) tableName = m_lblTitle ? m_lblTitle->text().trimmed() : QString();

    QJsonObject requestObj;
    requestObj["class_id"] = m_classid;
    const QString term = m_term.isEmpty() ? computeCurrentTermString() : m_term;
    if (!term.isEmpty()) requestObj["term"] = term;
    requestObj["field_name"] = fieldName;
    requestObj["score"] = scoreVal;
    if (!studentId.trimmed().isEmpty()) requestObj["student_id"] = studentId.trimmed();
    else requestObj["student_name"] = studentName.trimmed();
    if (!excelFilename.isEmpty()) requestObj["excel_filename"] = excelFilename;
    else if (!tableName.isEmpty()) requestObj["table_name"] = tableName;
    else return;

    QJsonDocument doc(requestObj);
    const QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    const QString url = "http://47.100.126.194:5000/group-scores/set-score";
    // 避免 C++ “most vexing parse”：不要写成 QNetworkRequest request(QUrl(url));
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
        const int code = root.value("code").toInt(-1);
        if (code != 200) return;

        const QJsonObject dataObj = root.value("data").toObject();
        // excel_total_score：当前 Excel 表下的总分（更适合写回此表的“总分”列）
        const bool hasExcelTotalScore = dataObj.contains("excel_total_score") && !dataObj.value("excel_total_score").isNull();
        const double excelTotalScore = dataObj.value("excel_total_score").toDouble(0.0);
        const double totalScore = dataObj.value("total_score").toDouble(0.0);
        const bool hasGroupTotalScore = dataObj.contains("group_total_score") && !dataObj.value("group_total_score").isNull();
        const double groupTotalScore = dataObj.value("group_total_score").toDouble(0.0);

        // 用服务端返回的 score 强制回写当前单元格（delete -> 清空；避免本地格式化差异）
        {
            int fieldCol = -1;
            for (int c = 0; c < table->columnCount(); ++c) {
                QTableWidgetItem* h = table->horizontalHeaderItem(c);
                if (!h) continue;
                if (h->text() == fieldName) {
                    fieldCol = c;
                    break;
                }
            }
            if (fieldCol >= 0) {
                const QSignalBlocker blocker(table);
                QTableWidgetItem* cellItem = table->item(row, fieldCol);
                if (!cellItem) {
                    cellItem = new QTableWidgetItem("");
                    cellItem->setTextAlignment(Qt::AlignCenter);
                    table->setItem(row, fieldCol, cellItem);
                }

                const QJsonValue scoreV = dataObj.value(QStringLiteral("score"));
                if (scoreV.isNull()) {
                    cellItem->setText(QString());
                } else if (scoreV.isDouble()) {
                    cellItem->setText(QString::number(scoreV.toDouble()));
                } else if (scoreV.isString()) {
                    cellItem->setText(scoreV.toString());
                }
            }
        }

        // 将服务端重算的总分写回“总分”列（避免与服务端规则不一致）
        int totalCol = -1;
        int groupCol = -1;
        int groupTotalCol = -1;
        for (int c = 0; c < table->columnCount(); ++c) {
            QTableWidgetItem* h = table->horizontalHeaderItem(c);
            if (!h) continue;
            const QString t = h->text();
            if (t == "总分") totalCol = c;
            else if (t == "组号" || t == "小组") groupCol = c;
            else if (t == "小组总分") groupTotalCol = c;
        }

        if (totalCol >= 0) {
            const QSignalBlocker blocker(table);
            QTableWidgetItem* totalItem = table->item(row, totalCol);
            if (!totalItem) {
                totalItem = new QTableWidgetItem("");
                totalItem->setTextAlignment(Qt::AlignCenter);
                table->setItem(row, totalCol, totalItem);
            }
            // 优先写 excel_total_score（对应该 Excel 表）；否则回退 total_score
            totalItem->setText(QString::number(hasExcelTotalScore ? excelTotalScore : totalScore));
        }

        // 服务端会重算并回写同组所有行：这里把 UI 同步一下
        if (groupCol >= 0 && groupTotalCol >= 0 && hasGroupTotalScore) {
            // 取当前行的组号（支持向上寻找填充）
            QString gName;
            for (int r = row; r >= 0; --r) {
                QTableWidgetItem* gItem = table->item(r, groupCol);
                if (gItem && !gItem->text().trimmed().isEmpty()) {
                    gName = gItem->text().trimmed();
                    break;
                }
            }
            if (!gName.isEmpty()) {
                const QSignalBlocker blocker(table);
                QString currentGroup = "";
                bool firstRowWrittenForTargetGroup = false;
                for (int r = 0; r < table->rowCount(); ++r) {
                    QTableWidgetItem* gItem = table->item(r, groupCol);
                    if (gItem && !gItem->text().trimmed().isEmpty()) {
                        currentGroup = gItem->text().trimmed();
                    }
                    if (currentGroup == gName) {
                        QTableWidgetItem* gtItem = table->item(r, groupTotalCol);
                        if (!gtItem) {
                            gtItem = new QTableWidgetItem("");
                            gtItem->setTextAlignment(Qt::AlignCenter);
                            table->setItem(r, groupTotalCol, gtItem);
                        }
                        // 仅在该小组的第一行显示“小组总分”，其它行清空。
                        // 注意：不能依赖“组号列是否为空”来判断第一行（有些表每行都会填组号）。
                        if (!firstRowWrittenForTargetGroup) {
                            gtItem->setText(QString::number(groupTotalScore));
                            firstRowWrittenForTargetGroup = true;
                        } else {
                            gtItem->setText(QString());
                        }
                    }
                }
            }
        }
    });
}

void GroupScoreDialog::updateRowTotal(int row)
{
    // 查找"总分"列的索引
    int totalCol = -1;
    int nameCol = -1;
    int idCol = -1;
    int groupCol = -1;
    int groupTotalCol = -1;
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
        } else if (headerText == "组号" || headerText == "小组") {
            groupCol = c;
        } else if (headerText == "小组总分") {
            groupTotalCol = c;
        }
    }
    
    // 如果没有"总分"列，不需要更新
    if (totalCol < 0) return;
    
    // 计算该行的个人总分
    // 需要计算所有数值列，但排除：组号、学号、姓名、总分、小组总分列本身
    double total = 0.0;
    
    for (int col = 0; col < table->columnCount(); ++col) {
        // 跳过组号、学号、姓名、总分、小组总分列
        if (col == idCol || col == nameCol || col == totalCol || col == groupCol || col == groupTotalCol) {
            continue;
        }
        
        QTableWidgetItem* item = table->item(row, col);
        if (!item) continue;
        
        // 获取列头名称，确保不是组号、学号、姓名、总分、小组总分列
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
        if (!headerItem) continue;
        QString headerText = headerItem->text();
        if (headerText == "组号" || headerText == "小组" || headerText == "学号" || headerText == "姓名" || 
            headerText == "总分" || headerText == "小组总分") {
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

void GroupScoreDialog::updateGroupTotal()
{
    // 查找"小组总分"列和"组号"列的索引
    int groupTotalCol = -1;
    int groupCol = -1;
    int totalCol = -1;
    for (int c = 0; c < table->columnCount(); ++c) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(c);
        if (!headerItem) continue;
        QString headerText = headerItem->text();
        if (headerText == "小组总分") {
            groupTotalCol = c;
        } else if (headerText == "组号" || headerText == "小组") {
            groupCol = c;
        } else if (headerText == "总分") {
            totalCol = c;
        }
    }
    
    // 如果没有"小组总分"列或"组号"列，不需要更新
    if (groupTotalCol < 0 || groupCol < 0 || totalCol < 0) return;
    
    // 按组号分组，计算每组的个人总分之和
    QMap<QString, double> groupTotals; // 组号 -> 该组的总分之和
    QString currentGroupName = ""; // 当前组号，用于向下填充
    
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* totalItem = table->item(row, totalCol);
        if (!totalItem) continue;
        
        // 获取组号（支持向下填充）
        QString groupName = "";
        if (groupCol >= 0) {
            QTableWidgetItem* groupItem = table->item(row, groupCol);
            if (groupItem) {
                QString groupText = groupItem->text().trimmed();
                // 如果当前行的组号不为空，更新当前组号
                if (!groupText.isEmpty()) {
                    currentGroupName = groupText;
                    groupName = currentGroupName;
                } else {
                    // 如果当前行的组号为空，使用之前保存的组号（向下填充）
                    groupName = currentGroupName;
                }
            } else {
                // 如果单元格不存在，使用之前保存的组号（向下填充）
                groupName = currentGroupName;
            }
        }
        
        // 如果组号为空，跳过该行
        if (groupName.isEmpty()) continue;
        
        QString totalText = totalItem->text().trimmed();
        bool ok;
        double total = totalText.toDouble(&ok);
        if (ok) {
            groupTotals[groupName] += total;
        }
    }
    
    // 更新每组的"小组总分"列（只写在每个小组的第一行）
    table->blockSignals(true);
    currentGroupName = ""; // 重置当前组号
    QString lastGroupName = ""; // 上一行的组号，用于判断是否是小组的第一行
    
    for (int row = 0; row < table->rowCount(); ++row) {
        // 获取组号（支持向下填充）
        QString groupName = "";
        if (groupCol >= 0) {
            QTableWidgetItem* groupItem = table->item(row, groupCol);
            if (groupItem) {
                QString groupText = groupItem->text().trimmed();
                // 如果当前行的组号不为空，更新当前组号
                if (!groupText.isEmpty()) {
                    currentGroupName = groupText;
                    groupName = currentGroupName;
                } else {
                    // 如果当前行的组号为空，使用之前保存的组号（向下填充）
                    groupName = currentGroupName;
                }
            } else {
                // 如果单元格不存在，使用之前保存的组号（向下填充）
                groupName = currentGroupName;
            }
        }
        
        // 如果组号为空，跳过该行
        if (groupName.isEmpty()) continue;
        
        // 确保小组总分单元格存在
        QTableWidgetItem* groupTotalItem = table->item(row, groupTotalCol);
        if (!groupTotalItem) {
            groupTotalItem = new QTableWidgetItem("");
            groupTotalItem->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, groupTotalCol, groupTotalItem);
        }
        
        // 只在该小组的第一行写入小组总分
        if (groupName != lastGroupName && groupTotals.contains(groupName)) {
            groupTotalItem->setText(QString::number(groupTotals[groupName]));
        } else {
            // 如果不是第一行，清空小组总分
            groupTotalItem->setText("");
        }
        
        lastGroupName = groupName; // 更新上一行的组号
    }
    table->blockSignals(false);
}

void GroupScoreDialog::onTableContextMenu(const QPoint& pos)
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
            // 删除行后，更新小组总分
            updateGroupTotal();
        }
    }
}

void GroupScoreDialog::sortTable(bool ascending)
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

void GroupScoreDialog::showCellComment(int row, int column)
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
    
    // 跳过固定列（组号/小组、学号、姓名、总分、小组总分）
    if (fieldName == "组号" || fieldName == "小组" || fieldName == "学号" || fieldName == "姓名" || 
        fieldName == "总分" || fieldName == "小组总分" || fieldName.isEmpty()) {
        QMessageBox::information(this, "提示", "该列不支持设置注释！");
        return;
    }

    QString currentComment = item->data(Qt::UserRole).toString();
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
        if ((!studentId.isEmpty() || !studentName.isEmpty()) && !fieldName.isEmpty()) {
            setCommentToServer(studentName, studentId, fieldName, comment);
        }
    }
}

void GroupScoreDialog::setCommentToServer(const QString& studentName, const QString& studentId, 
                                            const QString& fieldName, const QString& comment)
{
    if (!networkManager) return;
    if (m_classid.trimmed().isEmpty()) return;
    if (fieldName.trimmed().isEmpty()) return;
    if (studentId.trimmed().isEmpty() && studentName.trimmed().isEmpty()) return;
    
    // 构造请求 JSON
    QJsonObject requestObj;
    requestObj["class_id"] = m_classid;
    const QString term = m_term.isEmpty() ? computeCurrentTermString() : m_term;
    if (!term.isEmpty()) requestObj["term"] = term;
    if (!studentId.trimmed().isEmpty()) requestObj["student_id"] = studentId.trimmed();
    else requestObj["student_name"] = studentName.trimmed();
    requestObj["field_name"] = fieldName;

    // table_name / excel_filename：优先使用 excel_filename（带 .xlsx），其次使用 table_name（不带扩展名）
    const QString excelFilename = m_excelFileName.trimmed();
    // 优先使用 windowTitle（CustomListDialog 会把它设为真实表名）；其次才用 m_lblTitle
    QString tableName = windowTitle().trimmed();
    if (tableName.isEmpty()) tableName = m_lblTitle ? m_lblTitle->text().trimmed() : QString();
    if (!excelFilename.isEmpty()) requestObj["excel_filename"] = excelFilename;
    else if (!tableName.isEmpty()) requestObj["table_name"] = tableName;
    else return;

    // 空字符串表示删除注释
    requestObj["comment"] = comment;
    
    QJsonDocument doc(requestObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送 POST 请求 - 使用 /group-scores/set-comment 接口
    QString url = "http://47.100.126.194:5000/group-scores/set-comment";
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
                    qDebug() << "注释设置成功:" << (studentId.isEmpty() ? studentName : studentId) << fieldName;

                    // 同步到全局 CommentStorage（优先用服务端返回的 comment_key）
                    const QJsonObject dataObj = responseObj.value("data").toObject();
                    const QString commentKey = dataObj.value("comment_key").toString();
                    const QString termForCache = m_term.isEmpty() ? computeCurrentTermString() : m_term;
                    const QString sid = studentId.trimmed();
                    if (!sid.isEmpty()) {
                        // 若服务端返回整段 comments_json，则用它覆盖本地缓存（可正确反映删除后的“不存在”）
                        if (dataObj.contains("comments_json") && dataObj.value("comments_json").isObject()) {
                            const QJsonObject commentsJson = dataObj.value("comments_json").toObject();
                            CommentStorage::clearStudentComments(m_classid, termForCache, sid);
                            for (auto it = commentsJson.begin(); it != commentsJson.end(); ++it) {
                                const QString k = it.key();
                                const QString v = it.value().toString();
                                if (!k.isEmpty() && !v.isEmpty()) {
                                    const QString inferredTable = CommentStorage::inferTableNameFromFieldKey(k);
                                    CommentStorage::saveComment(m_classid, termForCache, sid, k, inferredTable, v);
                                }
                            }
                            // 兼容：也存一份纯字段名（用于旧界面）
                            const QString keyToSave = commentKey.isEmpty() ? fieldName : commentKey;
                            const QString v = commentsJson.value(keyToSave).toString();
                            if (!fieldName.isEmpty() && !v.isEmpty()) {
                                // 纯字段名需要带表格名避免冲突
                                CommentStorage::saveComment(m_classid, termForCache, sid, fieldName, m_excelFileName, v);
                            } else if (!fieldName.isEmpty() && v.isEmpty()) {
                                // 删除时也清理纯字段名
                                CommentStorage::saveComment(m_classid, termForCache, sid, fieldName, m_excelFileName, "");
                            }
                        } else {
                            // 没有整段 comments_json：只更新本次字段
                            const QString keyToSave = commentKey.isEmpty() ? fieldName : commentKey;
                            const QString tableNameForKey = !commentKey.isEmpty() ? CommentStorage::inferTableNameFromFieldKey(keyToSave) : m_excelFileName;
                            CommentStorage::saveComment(m_classid, termForCache, sid, keyToSave, tableNameForKey, comment);
                            // 兼容：同时存一份“纯字段名”，便于旧界面读取
                            if (CommentStorage::getComment(m_classid, termForCache, sid, fieldName, m_excelFileName).isEmpty()) {
                                CommentStorage::saveComment(m_classid, termForCache, sid, fieldName, m_excelFileName, comment);
                            }
                        }
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

// 重写鼠标事件以实现窗口拖动
void GroupScoreDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void GroupScoreDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && !m_dragPosition.isNull()) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

// 鼠标进入窗口时显示关闭按钮
void GroupScoreDialog::enterEvent(QEvent *event)
{
    if (m_btnClose) {
        m_btnClose->show();
    }
    QDialog::enterEvent(event);
}

// 鼠标离开窗口时隐藏关闭按钮
void GroupScoreDialog::leaveEvent(QEvent *event)
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
bool GroupScoreDialog::eventFilter(QObject *obj, QEvent *event)
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
void GroupScoreDialog::resizeEvent(QResizeEvent *event)
{
    if (m_btnClose) {
        m_btnClose->move(width() - 35, 5);
    }
    QDialog::resizeEvent(event);
}

// 窗口显示时更新关闭按钮位置
void GroupScoreDialog::showEvent(QShowEvent *event)
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

