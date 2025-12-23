#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QColorDialog>
#include <QMessageBox>
#include <QToolTip>
#include <QHeaderView>
#include <QMenu>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QSet>
#include <QTextStream>
#include <QFile>
#include <QIODevice>
#include <QLineEdit>
#include <QCursor>
#include <QMap>
#include <QVector>
#include <QPair>
#include <QMouseEvent>
#include <QEvent>
#include <QBrush>
#include <QColor>
#include <QTimer>
#include <QApplication>
#include <QScreen>
#include <QResizeEvent>
#include <QShowEvent>
#include <QPoint>
#include <QRect>
#include <QDesktopWidget>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include "CommonInfo.h"
#include <QJsonArray>
#include <algorithm>
#include <QDebug>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include "QXlsx/header/xlsxdocument.h"
#include <QFileInfo>
#include <QDate>
#include "StudentPhysiqueTableWidget.h"
#include "CellCommentWidget.h"
#include "CommentInputDialog.h"
#include "CommentStorage.h"

class StudentPhysiqueDialog : public QDialog
{
    Q_OBJECT

public:
    StudentPhysiqueDialog(QString classid = "", QWidget* parent = nullptr) : QDialog(parent), m_classid(classid)
    {
        // 去掉标题栏
        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        setWindowTitle("学生体质统计表");
        resize(1200, 800);
        setStyleSheet("background-color: #808080; font-size:14px;");
        
        // 启用鼠标跟踪以检测鼠标进入/离开
        setMouseTracking(true);

        // 创建关闭按钮
        m_btnClose = new QPushButton("X", this);
        m_btnClose->setFixedSize(30, 30);
        m_btnClose->setStyleSheet(
            "QPushButton { background-color: #666666; color: white; font-weight: bold; font-size: 14px; border: none; border-radius: 4px; }"
            "QPushButton:hover { background-color: #777777; }"
        );
        // 设置初始位置（窗口宽度1200，按钮在右上角）
        m_btnClose->move(1200 - 35, 5);
        // 初始显示，用于调试
        m_btnClose->show();
        m_btnClose->raise();
        // 确保按钮在最上层，不被其他控件遮挡
        m_btnClose->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        connect(m_btnClose, &QPushButton::clicked, this, &QDialog::close);
        
        // 为关闭按钮安装事件过滤器，确保鼠标在按钮上时不会隐藏
        m_btnClose->installEventFilter(this);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        // 增加顶部边距，为关闭按钮留出空间（关闭按钮高度30，位置y=5，所以顶部至少需要40）
        mainLayout->setContentsMargins(15, 40, 15, 15);

        // 标题
        m_lblTitle = new QLabel("小组积分表");
        m_lblTitle->setStyleSheet("background-color: #add8e6; color: black; font-size: 16px; font-weight: bold; padding: 8px; border-radius: 4px;");
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
        btnExport = new QPushButton("导出");
        btnUpload = new QPushButton("上传服务器");

        btnAddRow->setStyleSheet(btnStyle);
        btnDeleteRow->setStyleSheet(btnStyle);
        btnDeleteColumn->setStyleSheet(btnStyle);
        btnAddColumn->setStyleSheet(btnStyle);
        btnFontColor->setStyleSheet(btnStyle);
        btnBgColor->setStyleSheet(btnStyle);
        btnExport->setStyleSheet(btnStyle);
        btnUpload->setStyleSheet("QPushButton { background-color: blue; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px; }"
                                 "QPushButton:hover { background-color: #0000CD; }");

        btnLayout->addWidget(btnAddRow);
        btnLayout->addWidget(btnDeleteRow);
        btnLayout->addWidget(btnDeleteColumn);
        btnLayout->addWidget(btnAddColumn);
        btnLayout->addWidget(btnFontColor);
        btnLayout->addWidget(btnBgColor);
        btnLayout->addWidget(btnExport);
        btnLayout->addWidget(btnUpload);
        btnLayout->addStretch();

        mainLayout->addLayout(btnLayout);

        // 说明文本框
        QLabel* lblDesc = new QLabel("说明:");
        lblDesc->setStyleSheet("font-weight: bold; color: black;");
        mainLayout->addWidget(lblDesc);

        textDescription = new QTextEdit;
        textDescription->setPlaceholderText("在此处添加表格说明...");
        textDescription->setFixedHeight(60);
        textDescription->setStyleSheet("background-color: #ffa500; color: white; padding: 5px; border: 1px solid #888;");
        textDescription->setPlainText("该表为小组积分表。作业优秀为3,良好为2,未完成为0");
        mainLayout->addWidget(textDescription);

        // 表格
        // 固定列：小组(0)、学号(1)、姓名(2)、总分(总分数-2)、小组总分(总分数-1)
        // 可添加列在姓名后插入
        table = new StudentPhysiqueTableWidget(this); // 使用自定义表格控件
        table->setSelectionMode(QAbstractItemView::ExtendedSelection);   // 允许多选
        table->setSelectionBehavior(QAbstractItemView::SelectItems);     // 支持按行/列/单元格选择
        table->horizontalHeader()->setSectionsClickable(true);           // 允许点击表头选列
        table->verticalHeader()->setSectionsClickable(true);             // 允许点击行号选行
        // 统一选中高亮颜色
        table->setStyleSheet("QTableWidget::item:selected { background-color: #4A90E2; color: white; }");

        // 固定列索引（不能删除的列）
        // 小组(0)、学号(1)、姓名(2)、总分(总分数-2)、小组总分(总分数-1)
        fixedColumns = { 0, 1, 2 }; // 小组、学号、姓名
        nameColumnIndex = 2; // 姓名列索引
        groupColumnIndex = 0; // 小组列索引
        
        // 将表格添加到布局
        mainLayout->addWidget(table);
        
        // 初始化表格数据并合并小组单元格
        initializeTableData();
        
        // 连接信号和槽
        connect(btnAddRow, &QPushButton::clicked, this, &StudentPhysiqueDialog::onAddRow);
        connect(btnDeleteRow, &QPushButton::clicked, this, &StudentPhysiqueDialog::onDeleteRow);
        connect(btnDeleteColumn, &QPushButton::clicked, this, &StudentPhysiqueDialog::onDeleteColumn);
        connect(btnAddColumn, &QPushButton::clicked, this, &StudentPhysiqueDialog::onAddColumn);
        connect(btnFontColor, &QPushButton::clicked, this, &StudentPhysiqueDialog::onFontColor);
        connect(btnBgColor, &QPushButton::clicked, this, &StudentPhysiqueDialog::onBgColor);
        connect(btnExport, &QPushButton::clicked, this, &StudentPhysiqueDialog::onExport);
        connect(btnUpload, &QPushButton::clicked, this, &StudentPhysiqueDialog::onUpload);
        
        // 初始化网络管理器
        networkManager = new QNetworkAccessManager(this);

        // 单元格点击和悬浮事件
        connect(table, &QTableWidget::cellClicked, this, &StudentPhysiqueDialog::onCellClicked);
        connect(table, &QTableWidget::cellChanged, this, &StudentPhysiqueDialog::onCellChanged);
        
        // 创建注释窗口
        commentWidget = new CellCommentWidget(this);
        
        // 启用鼠标跟踪以显示悬浮提示
        table->setMouseTracking(true);
        
        // 右键菜单用于删除行和编辑注释
        table->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(table, &QTableWidget::customContextMenuRequested, this, &StudentPhysiqueDialog::onTableContextMenu);
        
        // 监听单元格内容变化，自动更新总分和小组总分
        connect(table, &QTableWidget::itemChanged, this, &StudentPhysiqueDialog::onItemChanged);
    }

    // 设置成绩表头ID
    void setScoreHeaderId(int scoreHeaderId) { m_scoreHeaderId = scoreHeaderId; }
    
    // 导入Excel数据
    void importData(const QStringList& headers, const QList<QStringList>& dataRows, const QString& excelFilePath = QString())
    {
        if (!table) return;

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
            m_excelFileName.clear();
        }

        // 根据导入的Excel列头动态设置表格列头
        // 如果导入的列头中没有"总分"，则添加；如果没有"小组总分"且带小组列，则添加
        QStringList tableHeaders = headers;
        bool hasTotal = tableHeaders.contains("总分");
        bool hasGroupTotal = tableHeaders.contains("小组总分");
        bool hasGroup = tableHeaders.contains("小组");
        
        // 确保有"总分"列（如果导入的数据中没有，则添加）
        if (!hasTotal) {
            tableHeaders.append("总分");
        }
        // 如果有"小组"列但没有"小组总分"列，则添加
        if (hasGroup && !hasGroupTotal) {
            tableHeaders.append("小组总分");
        }
        
        table->setColumnCount(tableHeaders.size());
        table->setHorizontalHeaderLabels(tableHeaders);

        // 清空现有数据
        table->setRowCount(0);

        // 获取列索引映射
        QMap<QString, int> headerMap;
        for (int i = 0; i < headers.size(); ++i) {
            headerMap[headers[i]] = i;
        }

        // 获取固定列的索引（根据导入的列头）
        int colGroup = -1, colId = -1, colName = -1;
        QMap<QString, int> scoreColumnMap; // 存储其他评分列的映射
        int colTotal = -1, colGroupTotal = -1;
        
        // 在表格列头中找到对应的列
        for (int col = 0; col < tableHeaders.size(); ++col) {
            QString headerText = tableHeaders[col];
            
            if (headerText == "小组") colGroup = col;
            else if (headerText == "学号") colId = col;
            else if (headerText == "姓名") colName = col;
            else if (headerText == "总分") colTotal = col;
            else if (headerText == "小组总分") colGroupTotal = col;
            else {
                // 其他列可能是评分列
                if (headerMap.contains(headerText)) {
                    scoreColumnMap[headerText] = col;
                }
            }
        }

        // 根据实际列头动态生成说明文本
        QStringList attributeColumns; // 属性列（除了小组、学号、姓名、总分、小组总分）
        for (const QString& header : headers) {
            if (header != "小组" && header != "学号" && header != "姓名" && 
                header != "总分" && header != "小组总分" && !header.isEmpty()) {
                attributeColumns.append(header);
            }
        }
        
        QString description = "说明:该表为统计表。";
        if (!attributeColumns.isEmpty()) {
            description += "包含以下评分项: " + attributeColumns.join("、");
        }
        if (textDescription) {
            textDescription->setPlainText(description);
        }
        
        // 导入数据
        QString currentGroup = "";
        int groupStartRow = -1;
        
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

            // 填充数据
            if (colGroup >= 0 && headerMap.contains("小组")) {
                int srcCol = headerMap["小组"];
                if (srcCol < rowData.size() && !rowData[srcCol].isEmpty()) {
                    QString group = rowData[srcCol];
                    table->item(row, colGroup)->setText(group);
                    currentGroup = group;
                    if (groupStartRow < 0) {
                        groupStartRow = row;
                    }
                } else if (!currentGroup.isEmpty()) {
                    table->item(row, colGroup)->setText(currentGroup);
                }
            }
            if (colId >= 0 && headerMap.contains("学号")) {
                int srcCol = headerMap["学号"];
                if (srcCol < rowData.size()) {
                    QTableWidgetItem* idItem = table->item(row, colId);
                    idItem->setText(rowData[srcCol]);
                    idItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                    idItem->setData(Qt::UserRole, QVariant()); // 学号列不需要注释
                }
            }
            if (colName >= 0 && headerMap.contains("姓名")) {
                int srcCol = headerMap["姓名"];
                if (srcCol < rowData.size()) {
                    QTableWidgetItem* nameItem = table->item(row, colName);
                    nameItem->setText(rowData[srcCol]);
                    nameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                    nameItem->setData(Qt::UserRole, QVariant()); // 姓名列不需要注释
                }
            }

            // 填充所有导入的列数据（除了已经填充的固定列）
            for (int col = 0; col < headers.size(); ++col) {
                QString headerText = headers[col];
                // 跳过已经填充的固定列（小组、学号、姓名）
                if (headerText == "小组" || headerText == "学号" || headerText == "姓名") {
                    continue;
                }
                // 找到该列在表格中的位置
                int targetCol = tableHeaders.indexOf(headerText);
                if (targetCol >= 0 && col < rowData.size()) {
                    table->item(row, targetCol)->setText(rowData[col]);
                    
                    // 如果是属性列（非小组、学号、姓名、总分、小组总分），尝试从全局存储中获取注释
                    if (headerText != "总分" && headerText != "小组总分") {
                        // 获取学生学号（从学号列）
                        QString studentId = "";
                        if (colId >= 0 && headerMap.contains("学号")) {
                            int srcCol = headerMap["学号"];
                            if (srcCol < rowData.size()) {
                                studentId = rowData[srcCol];
                            }
                        }
                        
                        // 如果学号不为空，尝试从全局存储中获取注释
                        if (!studentId.isEmpty() && !m_classid.isEmpty()) {
                            // 计算学期
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
                            
                            // 从全局存储中获取注释
                            QString comment = CommentStorage::getComment(m_classid, term, studentId, headerText, m_excelFileName);
                            if (!comment.isEmpty()) {
                                table->item(row, targetCol)->setData(Qt::UserRole, comment);
                            }
                        }
                    }
                }
            }
        }

        // 重新合并小组单元格并更新总分
        mergeGroupCells();
        updateAllTotals();
        
        // 确保表格可见并刷新显示
        table->setVisible(true);
        table->show();
        table->update();
        table->repaint();
        
        qDebug() << "导入完成，共" << table->rowCount() << "行数据";
    }

protected:
    // 重写鼠标事件以实现窗口拖动
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (event->buttons() & Qt::LeftButton && !m_dragPosition.isNull()) {
            move(event->globalPos() - m_dragPosition);
            event->accept();
        }
    }
    
    // 鼠标进入窗口时显示关闭按钮
    void enterEvent(QEvent *event) override
    {
        if (m_btnClose) {
            m_btnClose->show();
        }
        QDialog::enterEvent(event);
    }
    
    // 鼠标离开窗口时隐藏关闭按钮
    void leaveEvent(QEvent *event) override
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
    
    // 事件过滤器，处理关闭按钮的鼠标事件
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        // 先处理关闭按钮的事件
        if (obj == m_btnClose) {
            if (event->type() == QEvent::Enter) {
                // 鼠标进入关闭按钮时确保显示
                m_btnClose->show();
            } else if (event->type() == QEvent::Leave) {
                // 鼠标离开关闭按钮时，检查是否还在窗口内
                // 注意：不要在这里隐藏按钮，让 leaveEvent 来处理
                // 这样可以避免窗口刚显示时按钮被立即隐藏
            }
            // 不返回 true，让事件继续传播，这样按钮可以正常响应点击等事件
            return false;
        }
        
        // 处理表格的事件（原有的逻辑）
        if (obj == table) {
            if (event->type() == QEvent::MouseMove) {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                QTableWidgetItem* item = table->itemAt(mouseEvent->pos());
                if (item) {
                    int row = item->row();
                    int col = item->column();
                    
                    // 检查是否是跨列注释的起始单元格
                    int spanCols = item->data(Qt::UserRole + 1).toInt();
                    if (spanCols > 1) {
                        // 这是跨列注释的起始单元格
                        QString comment = item->data(Qt::UserRole).toString();
                        if (!comment.isEmpty() || true) { // 即使没有注释也显示窗口
                            QRect cellRect = table->visualItemRect(item);
                            // 计算跨列的宽度
                            int totalWidth = 0;
                            for (int i = 0; i < spanCols && col + i < table->columnCount(); ++i) {
                                totalWidth += table->columnWidth(col + i);
                            }
                            cellRect.setWidth(totalWidth);
                            
                            commentWidget->showComment(comment, cellRect, table, spanCols);
                            commentWidget->cancelHide();
                        }
                    } else {
                        // 检查是否是跨列注释的一部分
                        bool isInSpan = false;
                        QString comment = "";
                        int spanCols2 = 0;
                        for (int c = 0; c < col; ++c) {
                            QTableWidgetItem* checkItem = table->item(row, c);
                            if (checkItem) {
                                int sc = checkItem->data(Qt::UserRole + 1).toInt();
                                if (sc > 1 && c + sc > col) {
                                    // 当前单元格在跨列注释范围内
                                    isInSpan = true;
                                    comment = checkItem->data(Qt::UserRole).toString();
                                    spanCols2 = sc;
                                    break;
                                }
                            }
                        }
                        
                        if (isInSpan) {
                            QTableWidgetItem* startItem = table->item(row, col - 1);
                            for (int c = col - 1; c >= 0; --c) {
                                QTableWidgetItem* checkItem = table->item(row, c);
                                if (checkItem) {
                                    int sc = checkItem->data(Qt::UserRole + 1).toInt();
                                    if (sc > 1) {
                                        startItem = checkItem;
                                        comment = checkItem->data(Qt::UserRole).toString();
                                        spanCols2 = sc;
                                        break;
                                    }
                                }
                            }
                            
                            if (startItem) {
                                QRect cellRect = table->visualItemRect(startItem);
                                int startCol = table->column(startItem);
                                int totalWidth = 0;
                                for (int i = 0; i < spanCols2 && startCol + i < table->columnCount(); ++i) {
                                    totalWidth += table->columnWidth(startCol + i);
                                }
                                cellRect.setWidth(totalWidth);
                                commentWidget->showComment(comment, cellRect, table, spanCols2);
                                commentWidget->cancelHide();
                            }
                        } else {
                            // 单单元格注释
                            QString comment = item->data(Qt::UserRole).toString();
                            QRect cellRect = table->visualItemRect(item);
                            commentWidget->showComment(comment, cellRect, table, 1);
                            commentWidget->cancelHide();
                        }
                    }
                } else {
                    commentWidget->hideWithDelay(500);
                }
            } else if (event->type() == QEvent::Leave) {
                // 鼠标离开表格时，延迟隐藏注释窗口
                commentWidget->hideWithDelay(1000);
            }
        }
        return QDialog::eventFilter(obj, event);
    }
    
    // 窗口大小改变时更新关闭按钮位置
    void resizeEvent(QResizeEvent *event) override
    {
        if (m_btnClose) {
            m_btnClose->move(width() - 35, 5);
        }
        QDialog::resizeEvent(event);
    }
    
    // 窗口显示时更新关闭按钮位置
    void showEvent(QShowEvent *event) override
    {
        if (m_btnClose) {
            m_btnClose->move(width() - 35, 5);
            // 窗口显示时也显示关闭按钮
            m_btnClose->show();
            // 确保按钮在最上层
            m_btnClose->raise();
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
        
        // 调试：确保关闭按钮始终显示
        if (m_btnClose) {
            // 重新设置位置和显示
            m_btnClose->move(width() - 35, 5);
            m_btnClose->show();
            m_btnClose->raise();
            m_btnClose->setVisible(true);
            m_btnClose->update(); // 强制更新
            qDebug() << "StudentPhysiqueDialog - 关闭按钮位置:" << m_btnClose->pos() << "窗口大小:" << size() << "按钮可见:" << m_btnClose->isVisible() << "按钮父窗口:" << m_btnClose->parent();
        }
        
        QDialog::showEvent(event);
    }

private slots:
    void onAddRow()
    {
        int currentRow = table->currentRow();
        if (currentRow < 0) {
            currentRow = table->rowCount() - 1;
        }
        if (currentRow < 0) {
            currentRow = 0;
        }
        
        // 获取当前行所属的小组
        QTableWidgetItem* groupItem = table->item(currentRow, groupColumnIndex);
        QString groupName = groupItem ? groupItem->text() : "";
        
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
            
            // 学号、姓名、小组列清空注释，保持可编辑；其他列默认值为"0"
            if (headerText == "学号" || headerText == "姓名" || headerText == "小组") {
                item->setData(Qt::UserRole, QVariant());
            } else if (headerText != "总分" && headerText != "小组总分") {
                // 除了学号、姓名、小组、总分、小组总分，其他字段默认值都为0
                item->setText("0");
            }
            
            table->setItem(insertRow, col, item);
        }
        
        // 设置新行的小组名称（与当前行相同）
        if (!groupName.isEmpty()) {
            QTableWidgetItem* newGroupItem = table->item(insertRow, groupColumnIndex);
            if (newGroupItem) {
                newGroupItem->setText(groupName);
            }
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
        
        // 重新合并小组单元格
        mergeGroupCells();
        // 更新小组总分
        updateGroupTotals();
    }

    void onDeleteColumn()
    {
        int currentCol = table->currentColumn();
        if (currentCol < 0) {
            QMessageBox::information(this, "提示", "请先选择要删除的列");
            return;
        }

        // 检查是否是固定列
        int totalColumns = table->columnCount();
        int totalScoreCol = totalColumns - 2; // 总分列
        int groupTotalScoreCol = totalColumns - 1; // 小组总分列
        
        if (fixedColumns.contains(currentCol) || currentCol == totalScoreCol || currentCol == groupTotalScoreCol) {
            QMessageBox::warning(this, "警告", "不能删除固定列（小组、学号、姓名、总分、小组总分）");
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
            
            // 更新姓名列索引
            if (nameColumnIndex > currentCol) {
                nameColumnIndex--;
            }
            if (groupColumnIndex > currentCol) {
                groupColumnIndex--;
            }
            
            // 重新合并小组单元格并更新总分
            mergeGroupCells();
            updateAllTotals();
        }
    }

    void onAddColumn()
    {
        bool ok;
        QString columnName = QInputDialog::getText(this, "添加列", "请输入列名:", 
                                                   QLineEdit::Normal, "", &ok);
        if (!ok || columnName.isEmpty()) {
            return;
        }

        // 在姓名列后添加（但要在总分列之前）
        int totalColumns = table->columnCount();
        int totalScoreCol = totalColumns - 2; // 总分列
        
        int insertCol = nameColumnIndex + 1;
        if (insertCol >= totalScoreCol) {
            insertCol = totalScoreCol; // 确保在总分列之前
        }
        
        table->insertColumn(insertCol);
        table->setHorizontalHeaderItem(insertCol, new QTableWidgetItem(columnName));

        // 初始化新列的所有单元格，默认值0
        for (int row = 0; row < table->rowCount(); ++row) {
            QTableWidgetItem* item = new QTableWidgetItem("0");
            item->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, insertCol, item);
        }

        // 更新固定列索引（插入列后，姓名列之后的列索引会变化）
        QSet<int> newFixedColumns;
        for (int col : fixedColumns) {
            if (col <= nameColumnIndex) {
                newFixedColumns.insert(col); // 姓名列及其之前的列索引不变
            } else {
                newFixedColumns.insert(col + 1); // 姓名列之后的列索引+1
            }
        }
        fixedColumns = newFixedColumns;
        
        // 重新合并小组单元格（因为列索引变化了）
        mergeGroupCells();
    }

    void onFontColor()
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

    void onBgColor()
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

    void onExport()
    {
        QString fileName = QFileDialog::getSaveFileName(this, "导出统计表", "", "CSV文件 (*.csv);;所有文件 (*.*)");
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
                // 处理CSV中的逗号和引号
                if (text.contains(",") || text.contains("\"") || text.contains("\n")) {
                    text = "\"" + text.replace("\"", "\"\"") + "\"";
                }
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

    void onUpload()
    {
        qDebug() << "StudentPhysiqueDialog::onUpload() 方法被调用！";
        qDebug() << "Excel文件路径:" << m_excelFilePath;
        qDebug() << "Excel文件名:" << m_excelFileName;
        
        // 检查表格是否有数据
        if (table->rowCount() == 0) {
            QMessageBox::warning(this, "提示", "表格中没有数据，无法上传！");
            return;
        }

        // 创建输入对话框
        QDialog* inputDialog = new QDialog(this);
        inputDialog->setWindowTitle("上传小组积分表");
        inputDialog->setModal(true);
        inputDialog->resize(400, 250);

        QVBoxLayout* dialogLayout = new QVBoxLayout(inputDialog);

        QLabel* lblClassId = new QLabel("班级ID (class_id):");
        QLineEdit* editClassId = new QLineEdit();
        editClassId->setPlaceholderText("例如: class_1001");

        QLabel* lblTerm = new QLabel("学期 (term):");
        QLineEdit* editTerm = new QLineEdit();
        editTerm->setPlaceholderText("例如: 2025-2026-1");

        QLabel* lblRemark = new QLabel("备注 (remark):");
        QLineEdit* editRemark = new QLineEdit();
        editRemark->setPlaceholderText("例如: 2025年春季学期小组评价");

        dialogLayout->addWidget(lblClassId);
        dialogLayout->addWidget(editClassId);
        dialogLayout->addWidget(lblTerm);
        dialogLayout->addWidget(editTerm);
        dialogLayout->addWidget(lblRemark);
        dialogLayout->addWidget(editRemark);

        QHBoxLayout* btnLayout = new QHBoxLayout();
        QPushButton* btnOk = new QPushButton("确定");
        QPushButton* btnCancel = new QPushButton("取消");
        btnLayout->addStretch();
        btnLayout->addWidget(btnOk);
        btnLayout->addWidget(btnCancel);
        dialogLayout->addLayout(btnLayout);

        connect(btnOk, &QPushButton::clicked, inputDialog, &QDialog::accept);
        connect(btnCancel, &QPushButton::clicked, inputDialog, &QDialog::reject);

        if (inputDialog->exec() != QDialog::Accepted) {
            delete inputDialog;
            return;
        }

        QString classId = editClassId->text().trimmed();
        QString term = editTerm->text().trimmed();
        QString remark = editRemark->text().trimmed();

        if (classId.isEmpty() || term.isEmpty()) {
            QMessageBox::warning(this, "错误", "请填写必填项：班级ID、学期！");
            delete inputDialog;
            return;
        }

        delete inputDialog;

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
                excelName = QString("学生体质统计表_%1.xlsx").arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));
            }

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

        // 从表格中读取数据
        QJsonArray groupScoresArray;
        
        // 获取列索引
        int colGroup = -1, colId = -1, colName = -1;
        QMap<QString, int> scoreColumnMap; // 存储评分列的映射（列名 -> 列索引）
        
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
            if (!headerItem) continue;
            QString headerText = headerItem->text();
            
            if (headerText == "小组") colGroup = col;
            else if (headerText == "学号") colId = col;
            else if (headerText == "姓名") colName = col;
            else if (headerText != "总分" && headerText != "小组总分" && !headerText.isEmpty()) {
                // 其他列都是评分列（除了固定的总分和小组总分）
                scoreColumnMap[headerText] = col;
            }
        }

        if (colGroup < 0 || colId < 0 || colName < 0) {
            QMessageBox::warning(this, "错误", "表格中缺少必要列：小组、学号、姓名！");
            return;
        }

        // 读取每一行数据
        for (int row = 0; row < table->rowCount(); ++row) {
            QTableWidgetItem* groupItem = table->item(row, colGroup);
            QTableWidgetItem* idItem = table->item(row, colId);
            QTableWidgetItem* nameItem = table->item(row, colName);
            
            if (!groupItem || !idItem || !nameItem) continue;
            
            QString groupNumber = groupItem->text().trimmed();
            QString studentId = idItem->text().trimmed();
            QString studentName = nameItem->text().trimmed();
            
            // 至少要有小组号和学号或姓名
            if (groupNumber.isEmpty() || (studentId.isEmpty() && studentName.isEmpty())) {
                continue; // 跳过空行
            }

            QJsonObject scoreObj;
            
            // 转换小组号为整数
            bool ok;
            int groupNum = groupNumber.toInt(&ok);
            if (!ok) {
                // 如果不是纯数字，尝试提取数字（例如"1组" -> 1）
                QString numStr = groupNumber;
                numStr.remove(QRegExp("[^0-9]"));
                if (!numStr.isEmpty()) {
                    groupNum = numStr.toInt();
                } else {
                    continue; // 无法解析小组号，跳过
                }
            }
            scoreObj["group_number"] = groupNum;
            
            if (!studentId.isEmpty()) {
                scoreObj["student_id"] = studentId;
            }
            if (!studentName.isEmpty()) {
                scoreObj["student_name"] = studentName;
            }

            // 读取所有评分列
            // 根据协议，需要映射列名：早读->hygiene, 课堂发言->participation, 纪律->discipline, 作业->homework, 背诵->recitation
            QMap<QString, QString> headerToFieldMap;
            headerToFieldMap["早读"] = "hygiene";
            headerToFieldMap["课堂发言"] = "participation";
            headerToFieldMap["纪律"] = "discipline";
            headerToFieldMap["作业"] = "homework";
            headerToFieldMap["背诵"] = "recitation";
            
            for (auto it = scoreColumnMap.begin(); it != scoreColumnMap.end(); ++it) {
                QString headerName = it.key();
                int col = it.value();
                
                QTableWidgetItem* item = table->item(row, col);
                if (item && !item->text().trimmed().isEmpty()) {
                    bool ok;
                    double score = item->text().toDouble(&ok);
                    if (ok) {
                        // 根据列名映射到字段名
                        QString fieldName = headerToFieldMap.value(headerName, headerName.toLower());
                        scoreObj[fieldName] = score;
                    }
                }
            }

            groupScoresArray.append(scoreObj);
        }

        if (groupScoresArray.isEmpty()) {
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
        
        // 遍历所有评分列，构建字段定义
        for (auto it = scoreColumnMap.begin(); it != scoreColumnMap.end(); ++it) {
            QString columnName = it.key();
            
            QJsonObject fieldObj;
            fieldObj["field_name"] = columnName;
            fieldObj["field_type"] = "number"; // 默认为数字类型
            fieldObj["field_order"] = fieldOrder++;
            fieldObj["is_total"] = 0;
            
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
        QString uploadText = "正在上传小组积分数据到服务器...";
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
                    int code = responseObj["code"].toInt();
                    
                    if (code == 200) {
                        QJsonObject dataObj = responseObj["data"].toObject();
                        QString message = dataObj["message"].toString();
                        int insertedCount = dataObj.value("inserted_count").toInt();
                        
                        // 尝试获取 score_header_id（如果服务器返回）
                        if (dataObj.contains("id") && dataObj["id"].isDouble()) {
                            m_scoreHeaderId = dataObj["id"].toInt();
                            qDebug() << "获取到 score_header_id:" << m_scoreHeaderId;
                        }
                        
                        QString successMsg = QString("上传成功！\n\n%1").arg(message);
                        if (insertedCount > 0) {
                            successMsg += QString("\n共插入 %1 条记录").arg(insertedCount);
                        }
                        QMessageBox::information(this, "上传成功", successMsg);
                    } else {
                        QString errorMsg = responseObj["message"].toString();
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

    void onCellClicked(int row, int column)
    {
        // 点击单元格时，如果已有注释则显示注释窗口，否则弹出编辑对话框
        QTableWidgetItem* item = table->item(row, column);
        if (!item) {
            item = new QTableWidgetItem("");
            item->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, column, item);
        }
        
        // 获取列头名称，检查是否是学号或姓名列
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(column);
        QString headerText = headerItem ? headerItem->text() : "";
        
        // 如果是学号或姓名列，直接进入编辑状态
        if (headerText == "学号" || headerText == "姓名") {
            table->editItem(item);
            return;
        }
        
        // 检查是否是跨列注释的起始单元格
        int spanCols = item->data(Qt::UserRole + 1).toInt();
        QString comment = item->data(Qt::UserRole).toString();
        
        // 如果不是起始单元格，检查是否在跨列注释范围内
        if (spanCols <= 1 && comment.isEmpty()) {
            for (int c = column - 1; c >= 0; --c) {
                QTableWidgetItem* checkItem = table->item(row, c);
                if (checkItem) {
                    int sc = checkItem->data(Qt::UserRole + 1).toInt();
                    if (sc > 1 && c + sc > column) {
                        // 当前单元格在跨列注释范围内
                        item = checkItem;
                        comment = checkItem->data(Qt::UserRole).toString();
                        spanCols = sc;
                        break;
                    }
                }
            }
        }
        
        // 如果已有注释，显示注释窗口；否则直接进入编辑状态（双击也是直接编辑）
        if (!comment.isEmpty() && commentWidget) {
            QRect cellRect = table->visualItemRect(item);
            // 计算跨列的宽度
            int totalWidth = 0;
            int startCol = table->column(item);
            for (int i = 0; i < spanCols && startCol + i < table->columnCount(); ++i) {
                totalWidth += table->columnWidth(startCol + i);
            }
            cellRect.setWidth(totalWidth);
            commentWidget->showComment(comment, cellRect, table, spanCols);
            commentWidget->cancelHide();
        } else {
            // 没有注释，直接进入编辑状态（注释编辑通过右键菜单）
            table->editItem(item);
        }
    }

    void onCellChanged(int row, int column)
    {
        // 单元格内容改变时，更新总分和小组总分
        updateAllTotals();
        mergeGroupCells();
    }

    void onItemChanged(QTableWidgetItem* item)
    {
        if (!item) return;
        
        int row = item->row();
        int column = item->column();
        
        // 如果修改的是小组列，需要重新合并单元格
        if (column == groupColumnIndex) {
            mergeGroupCells();
            updateGroupTotals();
        } else {
            // 获取列头名称
            QTableWidgetItem* headerItem = table->horizontalHeaderItem(column);
            QString headerText = headerItem ? headerItem->text() : "";
            
            // 如果修改的不是"总分"列、"小组总分"列、"学号"列、"姓名"列，则更新该行的总分
            if (headerText != "总分" && headerText != "小组总分" && headerText != "学号" && headerText != "姓名" && headerText != "小组") {
                updateRowTotal(row);
                // 更新小组总分（因为该行的总分改变了）
                updateGroupTotals();
            }
        }
    }

    void onTableContextMenu(const QPoint& pos)
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
            int ret = QMessageBox::question(this, "确认", "确定要删除这一行吗？删除后该小组将少一人。", 
                                            QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                table->removeRow(row);
                mergeGroupCells();
                updateGroupTotals();
            }
        }
    }

    void onDeleteRow()
    {
        QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
        if (ranges.isEmpty()) {
            QMessageBox::information(this, "提示", "请先选择要删除的行");
            return;
        }

        // 必须整行选中（覆盖所有列）才允许删除
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

        // 删除后重新合并小组单元格并更新总分
        mergeGroupCells();
        updateAllTotals();
    }

private:
    void initializeTableData()
    {
        // 初始化示例数据
        for (int row = 0; row < table->rowCount(); ++row) {
            for (int col = 0; col < table->columnCount(); ++col) {
                QTableWidgetItem* item = new QTableWidgetItem("");
                item->setTextAlignment(Qt::AlignCenter);
                table->setItem(row, col, item);
            }
        }
        
        // 设置示例数据 - 第一组（3行）
        // 第1行
        if (table->item(0, 0)) table->item(0, 0)->setText("1");
        if (table->item(0, 1)) table->item(0, 1)->setText("1");
        if (table->item(0, 3)) table->item(0, 3)->setText("100"); // 早读
        
        // 添加跨列注释示例：第1行的课堂发言和纪律列（列4-5）跨列注释
        QTableWidgetItem* commentItem0 = table->item(0, 4); // 课堂发言列
        if (commentItem0) {
            commentItem0->setData(Qt::UserRole, "声音大");
            commentItem0->setData(Qt::UserRole + 1, 2); // 跨2列
            commentItem0->setBackground(QBrush(QColor(255, 255, 200))); // 浅黄色标记
        }
        
        if (table->item(0, 8)) table->item(0, 8)->setText("123"); // 总分
        
        // 第2行
        if (table->item(1, 0)) table->item(1, 0)->setText("1");
        if (table->item(1, 1)) table->item(1, 1)->setText("2");
        if (table->item(1, 2)) table->item(1, 2)->setText("李四");
        if (table->item(1, 3)) table->item(1, 3)->setText("95"); // 早读
        if (table->item(1, 8)) table->item(1, 8)->setText("345"); // 总分
        
        // 第3行
        if (table->item(2, 0)) table->item(2, 0)->setText("1");
        if (table->item(2, 1)) table->item(2, 1)->setText("3");
        if (table->item(2, 2)) table->item(2, 2)->setText("张三");
        if (table->item(2, 3)) table->item(2, 3)->setText("75"); // 早读
        if (table->item(2, 4)) table->item(2, 4)->setText("80"); // 课堂发言
        
        // 添加跨列注释示例：第3行的纪律和作业列（列5-6）跨列注释
        QTableWidgetItem* commentItem1 = table->item(2, 5); // 纪律列
        if (commentItem1) {
            commentItem1->setData(Qt::UserRole, "积极踊跃");
            commentItem1->setData(Qt::UserRole + 1, 2); // 跨2列
            commentItem1->setBackground(QBrush(QColor(255, 255, 200))); // 浅黄色标记
        }
        
        if (table->item(2, 8)) table->item(2, 8)->setText("234"); // 总分
        
        // 设置示例数据 - 第二组（3行）
        // 第4行
        if (table->item(3, 0)) table->item(3, 0)->setText("2");
        if (table->item(3, 1)) table->item(3, 1)->setText("4");
        if (table->item(3, 3)) table->item(3, 3)->setText("67"); // 早读
        if (table->item(3, 4)) table->item(3, 4)->setText("97"); // 课堂发言
        
        // 第5行
        if (table->item(4, 0)) table->item(4, 0)->setText("2");
        if (table->item(4, 1)) table->item(4, 1)->setText("5");
        
        // 第6行
        if (table->item(5, 0)) table->item(5, 0)->setText("2");
        if (table->item(5, 1)) table->item(5, 1)->setText("6");
        
        // 合并小组单元格并计算总分
        mergeGroupCells();
        updateAllTotals();
        
        // 确保表格可见
        table->show();
        table->setVisible(true);
        
        // 安装事件过滤器以处理鼠标悬浮
        table->installEventFilter(this);
    }

    void mergeGroupCells()
    {
        // 清除所有合并
        table->clearSpans();
        
        int groupCol = groupColumnIndex;
        if (groupCol < 0 || groupCol >= table->columnCount()) return;
        
        QString currentGroup;
        int startRow = -1;
        
        for (int row = 0; row < table->rowCount(); ++row) {
            QTableWidgetItem* item = table->item(row, groupCol);
            QString groupName = item ? item->text() : "";
            
            if (groupName.isEmpty()) {
                // 如果当前行小组为空，结束上一个合并
                if (startRow >= 0 && row > startRow) {
                    table->setSpan(startRow, groupCol, row - startRow, 1);
                }
                startRow = -1;
                currentGroup = "";
                continue;
            }
            
            if (groupName != currentGroup) {
                // 如果小组名称变化，先合并上一个小组
                if (startRow >= 0 && row > startRow) {
                    table->setSpan(startRow, groupCol, row - startRow, 1);
                }
                // 开始新小组
                startRow = row;
                currentGroup = groupName;
            }
        }
        
        // 处理最后一组
        if (startRow >= 0 && table->rowCount() > startRow) {
            int endRow = table->rowCount();
            table->setSpan(startRow, groupCol, endRow - startRow, 1);
        }
    }

    void updateAllTotals()
    {
        // 更新所有行的个人总分
        int totalColumns = table->columnCount();
        int totalScoreCol = totalColumns - 2; // 总分列
        
        for (int row = 0; row < table->rowCount(); ++row) {
            double total = 0.0;
            
            // 从姓名列之后到总分列之前的所有列都是计分列
            for (int col = nameColumnIndex + 1; col < totalScoreCol; ++col) {
                // 检查该单元格是否被合并（可能是注释单元格）
                QTableWidgetItem* item = table->item(row, col);
                if (!item) continue;
                
                // 检查该单元格是否是跨列注释的起始单元格
                int spanCols = item->data(Qt::UserRole + 1).toInt();
                if (spanCols > 1) {
                    // 这是跨列注释的起始单元格，跳过（不计算注释单元格）
                    // 同时跳过该注释跨过的所有列
                    col += (spanCols - 1);
                    continue;
                }
                
                // 检查该单元格是否在某个跨列注释的范围内
                bool isInCommentSpan = false;
                for (int c = col - 1; c >= nameColumnIndex + 1 && c >= 0; --c) {
                    QTableWidgetItem* checkItem = table->item(row, c);
                    if (checkItem) {
                        int sc = checkItem->data(Qt::UserRole + 1).toInt();
                        if (sc > 1 && c + sc > col) {
                            // 当前单元格在跨列注释范围内
                            isInCommentSpan = true;
                            break;
                        }
                    }
                }
                if (isInCommentSpan) continue;
                
                QString text = item->text();
                bool ok;
                double value = text.toDouble(&ok);
                if (ok) {
                    total += value;
                }
            }
            
            // 设置总分
            QTableWidgetItem* totalItem = table->item(row, totalScoreCol);
            if (!totalItem) {
                totalItem = new QTableWidgetItem("");
                totalItem->setTextAlignment(Qt::AlignCenter);
                table->setItem(row, totalScoreCol, totalItem);
            }
            totalItem->setText(QString::number(total));
        }
        
        // 更新小组总分
        updateGroupTotals();
    }

    void updateRowTotal(int row)
    {
        // 查找"总分"列的索引
        int totalScoreCol = -1;
        int groupCol = groupColumnIndex;
        int nameCol = nameColumnIndex;
        int idCol = -1;
        
        // 查找学号列索引
        for (int c = 0; c < table->columnCount(); ++c) {
            QTableWidgetItem* headerItem = table->horizontalHeaderItem(c);
            if (!headerItem) continue;
            if (headerItem->text() == "学号") {
                idCol = c;
                break;
            }
        }
        
        // 查找总分列索引
        for (int c = 0; c < table->columnCount(); ++c) {
            QTableWidgetItem* headerItem = table->horizontalHeaderItem(c);
            if (!headerItem) continue;
            if (headerItem->text() == "总分") {
                totalScoreCol = c;
                break;
            }
        }
        
        // 如果没有"总分"列，不需要更新
        if (totalScoreCol < 0) return;
        
        double total = 0.0;
        
        // 遍历所有列，计算所有数值列（排除：小组、学号、姓名、总分、小组总分列）
        for (int col = 0; col < table->columnCount(); ++col) {
            // 跳过固定列和总分列
            if (col == groupCol || col == idCol || col == nameCol || col == totalScoreCol) {
                continue;
            }
            
            // 获取列头名称，确保不是固定列
            QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
            if (!headerItem) continue;
            QString headerText = headerItem->text();
            if (headerText == "小组" || headerText == "学号" || headerText == "姓名" || 
                headerText == "总分" || headerText == "小组总分") {
                continue;
            }
            
            // 检查该单元格是否被合并（可能是注释单元格）
            QTableWidgetItem* item = table->item(row, col);
            if (!item) continue;
            
            // 检查该单元格是否是跨列注释的起始单元格
            int spanCols = item->data(Qt::UserRole + 1).toInt();
            if (spanCols > 1) {
                // 这是跨列注释的起始单元格，跳过（不计算注释单元格）
                // 同时跳过该注释跨过的所有列
                col += (spanCols - 1);
                continue;
            }
            
            // 检查该单元格是否在某个跨列注释的范围内
            bool isInCommentSpan = false;
            for (int c = col - 1; c >= 0; --c) {
                QTableWidgetItem* checkItem = table->item(row, c);
                if (checkItem) {
                    int sc = checkItem->data(Qt::UserRole + 1).toInt();
                    if (sc > 1 && c + sc > col) {
                        // 当前单元格在跨列注释范围内
                        isInCommentSpan = true;
                        break;
                    }
                }
            }
            if (isInCommentSpan) continue;
            
            QString text = item->text();
            bool ok;
            double value = text.toDouble(&ok);
            if (ok) {
                total += value;
            }
        }
        
        // 设置总分
        QTableWidgetItem* totalItem = table->item(row, totalScoreCol);
        if (!totalItem) {
            totalItem = new QTableWidgetItem("");
            totalItem->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, totalScoreCol, totalItem);
        }
        
        // 暂时断开信号，避免递归调用
        table->blockSignals(true);
        totalItem->setText(QString::number(total));
        table->blockSignals(false);
    }

    void updateGroupTotals()
    {
        int totalColumns = table->columnCount();
        int totalScoreCol = totalColumns - 2; // 总分列
        int groupTotalScoreCol = totalColumns - 1; // 小组总分列
        int groupCol = groupColumnIndex;
        
        // 按小组汇总总分
        QMap<QString, double> groupTotals;
        QMap<QString, QVector<int>> groupRows;
        
        // 收集每个小组的行号和总分
        for (int row = 0; row < table->rowCount(); ++row) {
            QTableWidgetItem* groupItem = table->item(row, groupCol);
            QTableWidgetItem* totalItem = table->item(row, totalScoreCol);
            
            if (groupItem && totalItem) {
                QString groupName = groupItem->text();
                if (!groupName.isEmpty()) {
                    QString totalText = totalItem->text();
                    bool ok;
                    double total = totalText.toDouble(&ok);
                    if (ok) {
                        groupTotals[groupName] += total;
                        if (!groupRows.contains(groupName)) {
                            groupRows[groupName] = QVector<int>();
                        }
                        groupRows[groupName].append(row);
                    }
                }
            }
        }
        
        // 设置每个小组的小组总分（只在每个小组的第一行显示）
        for (auto it = groupRows.begin(); it != groupRows.end(); ++it) {
            QString groupName = it.key();
            QVector<int> rows = it.value();
            
            if (!rows.isEmpty()) {
                double groupTotal = groupTotals[groupName];
                
                // 只在该小组的第一行显示小组总分
                int firstRow = rows[0];
                QTableWidgetItem* groupTotalItem = table->item(firstRow, groupTotalScoreCol);
                if (!groupTotalItem) {
                    groupTotalItem = new QTableWidgetItem("");
                    groupTotalItem->setTextAlignment(Qt::AlignCenter);
                    table->setItem(firstRow, groupTotalScoreCol, groupTotalItem);
                }
                groupTotalItem->setText(QString::number(groupTotal));
                
                // 合并小组总分列
                if (rows.size() > 1) {
                    table->setSpan(firstRow, groupTotalScoreCol, rows.size(), 1);
                }
                
                // 清除其他行的小组总分显示
                for (int i = 1; i < rows.size(); ++i) {
                    QTableWidgetItem* item = table->item(rows[i], groupTotalScoreCol);
                    if (item) {
                        item->setText("");
                    }
                }
            }
        }
    }

    void showCellComment(int row, int column)
    {
        QTableWidgetItem* item = table->item(row, column);
        if (!item) {
            item = new QTableWidgetItem("");
            item->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, column, item);
        }

        QString currentComment = item->data(Qt::UserRole).toString();
        int currentSpanCols = item->data(Qt::UserRole + 1).toInt();
        
        // 询问是否需要跨列注释
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("注释设置");
        msgBox.setText("选择注释显示方式：");
        QPushButton* singleBtn = msgBox.addButton("单单元格", QMessageBox::ActionRole);
        QPushButton* spanBtn = msgBox.addButton("跨列注释", QMessageBox::ActionRole);
        QPushButton* cancelBtn = msgBox.addButton("取消", QMessageBox::RejectRole);
        msgBox.exec();

        if (msgBox.clickedButton() == cancelBtn) {
            return;
        }

        bool isSpanMode = (msgBox.clickedButton() == spanBtn);
        int spanCols = currentSpanCols > 1 ? currentSpanCols : 1;
        
        if (isSpanMode) {
            bool ok;
            spanCols = QInputDialog::getInt(this, "跨列注释", 
                                            QString("请输入要跨越的列数（从当前列开始）:"),
                                            currentSpanCols > 1 ? currentSpanCols : 2, 
                                            1, table->columnCount() - column, 1, &ok);
            if (!ok) return;
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
        
        // 跳过固定列（小组、学号、姓名、总分、小组总分）
        if (fieldName == "小组" || fieldName == "学号" || fieldName == "姓名" || 
            fieldName == "总分" || fieldName == "小组总分" || fieldName.isEmpty()) {
            QMessageBox::information(this, "提示", "该列不支持设置注释！");
            return;
        }
        
        bool ok;
        QString comment = CommentInputDialog::getMultiLineText(this, "单元格注释", 
                                                               QString("单元格 (%1, %2) 的注释:").arg(row + 1).arg(column + 1),
                                                               currentComment, &ok);
        if (ok) {
            // 清除该行之前的跨列注释
            clearSpanComment(row);
            
            if (!comment.isEmpty()) {
                // 存储注释（在起始单元格）
                item->setData(Qt::UserRole, comment);
                item->setData(Qt::UserRole + 1, spanCols); // 存储跨列数
                
                // 设置背景色标记（浅黄色，表示有注释）
                item->setBackground(QBrush(QColor(255, 255, 200))); // 浅黄色标记
                
                if (isSpanMode && spanCols > 1) {
                    // 不合并单元格，只标记起始单元格
                    // 注释通过悬浮窗口显示
                    item->setBackground(QBrush(QColor(255, 255, 200)));
                } else {
                    // 单单元格注释
                    item->setBackground(QBrush(QColor(255, 255, 200)));
                }
                
                // 重新合并小组单元格（保持小组合并）
                mergeGroupCells();
            } else {
                // 清除注释
                item->setData(Qt::UserRole, "");
                item->setData(Qt::UserRole + 1, 0);
                item->setBackground(QBrush()); // 恢复默认
                
                // 重新合并小组单元格
                mergeGroupCells();
            }
            
            // 调用服务器接口设置注释（点击确定按钮时发送，单单元格注释才调用，跨列注释暂不支持）
            if (!isSpanMode && !studentName.isEmpty() && !fieldName.isEmpty()) {
                setCommentToServer(studentName, studentId, fieldName, comment);
            }
        }
    }

    void clearSpanComment(int row)
    {
        // 清除该行所有的跨列注释标记
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* item = table->item(row, col);
            if (item) {
                int spanCols = item->data(Qt::UserRole + 1).toInt();
                if (spanCols > 1) {
                    // 清除跨列注释标记
                    item->setData(Qt::UserRole, "");
                    item->setData(Qt::UserRole + 1, 0);
                    item->setBackground(QBrush()); // 恢复默认背景
                }
            }
        }
    }

private:
    StudentPhysiqueTableWidget* table;
    QTextEdit* textDescription;
    QLabel* m_lblTitle; // 标题标签
    QPushButton* btnAddRow;
    QPushButton* btnDeleteColumn;
    QPushButton* btnAddColumn;
    QPushButton* btnDeleteRow;
    QPushButton* btnFontColor;
    QPushButton* btnBgColor;
    QPushButton* btnExport;
    QPushButton* btnUpload;
    QNetworkAccessManager* networkManager;
    
    QSet<int> fixedColumns; // 固定列索引集合（不能删除的列）
    int nameColumnIndex; // 姓名列索引
    int groupColumnIndex; // 小组列索引
    CellCommentWidget* commentWidget = nullptr; // 注释窗口
    QPushButton* m_btnClose = nullptr; // 关闭按钮
    QPoint m_dragPosition; // 用于窗口拖动
    QString m_excelFilePath; // Excel文件路径
    QString m_excelFileName; // Excel文件名
    int m_scoreHeaderId = -1; // 成绩表头ID，用于设置注释
    QString m_classid; // 班级ID
    
    void setCommentToServer(const QString& studentName, const QString& studentId, 
                           const QString& fieldName, const QString& comment)
    {
        // 构造请求 JSON
        QJsonObject requestObj;
        // 不传 score_header_id，改用 class_id + term (+ excel_filename) 自动定位
        if (!m_classid.trimmed().isEmpty()) requestObj["class_id"] = m_classid.trimmed();
        // 这里没有显式保存 term（上传时由用户输入），先按当前日期推断学期，保持与本地 CommentStorage 一致
        const auto calcTerm = []() -> QString {
            QDate currentDate = QDate::currentDate();
            int year = currentDate.year();
            int month = currentDate.month();
            if (month >= 9 || month <= 1) {
                if (month >= 9) return QString("%1-%2-1").arg(year).arg(year + 1);
                return QString("%1-%2-1").arg(year - 1).arg(year);
            }
            return QString("%1-%2-2").arg(year - 1).arg(year);
        };
        const QString term = calcTerm();
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
        QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
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
};