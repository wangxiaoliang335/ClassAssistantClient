#include "HeatmapSegmentDialog.h"
#include "ScheduleDialog.h"
#include <QHeaderView>
#include <QColorDialog>
#include <QMessageBox>
#include <QTimer>

HeatmapSegmentDialog::HeatmapSegmentDialog(QWidget* parent)
    : QDialog(parent)
{
    // 无标题栏，便于自定义外观
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    resize(800, 500);
    setMouseTracking(true);
    setStyleSheet("background-color: #515151; color: white; font-size:14px;"); // 背景颜色参考示例图

    // 关闭按钮（悬停显示）
    closeButton = new QPushButton("X", this);
    closeButton->setFixedSize(26, 26);
    closeButton->setStyleSheet(
        "QPushButton { background-color: #666666; color: white; border: none; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #777777; }"
    );
    closeButton->hide();
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    QString comboStyle =
        "QComboBox { color: white; background-color: #1f1f1f; padding: 6px 12px; border-radius: 4px; font-size: 14px; border: 1px solid #555; }"
        "QComboBox QAbstractItemView { color: white; background-color: #1f1f1f; selection-background-color: #ff8c00; }";

    // 顶部：两个下拉框
    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->setSpacing(12);

    tableComboBox = new QComboBox(this);
    tableComboBox->setStyleSheet(comboStyle);
    tableComboBox->addItem(QString::fromUtf8(u8"期中成绩表"));

    attributeComboBox = new QComboBox(this);
    attributeComboBox->setStyleSheet(comboStyle);
    attributeComboBox->addItem(QString::fromUtf8(u8"数学"));

    topLayout->addWidget(tableComboBox);
    topLayout->addWidget(attributeComboBox);
    topLayout->addStretch();
    mainLayout->addLayout(topLayout);

    // 表格（使用自定义控件以支持热力图绘制）
    table = new HeatmapTableWidget();
    table->setRowCount(3);
    table->setColumnCount(6);
    QStringList headers = { "最小值", "-", "最大值", "颜色", "人数", "百分数" };
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // 添加样式表
    table->setStyleSheet(
        "QTableWidget {"
        "   background-color: #1e1e1e;"      /* 整体背景灰 */
        "   color: #d4d4d4;"
        "   border: 1px solid #3e3e42;"
        "   gridline-color: #3e3e42;"
        "   selection-background-color: #094771;" /* 选中色，会被Delegate覆盖，但在Delegate里要处理Selected状态 */
        "}"

        /* 关键点 1：设置左上角按钮的颜色 */
        "QTableWidget QTableCornerButton::section {"
        "   background-color: #252526;"      /* 这里设置成你想要的灰色 */
        "   border: 1px solid #3e3e42;"
        "}"

        "QHeaderView::section {"
        "   background-color: #252526;"
        "   color: #cccccc;"
        "   font-weight: bold;"
        "   border: 1px solid #3e3e42;"
        "   padding: 4px;"
        "}"
    );

    // 初始化默认分段：0-60, 60-80, 80-100
    for (int row = 0; row < 3; ++row) {
        // 最小值
        QTableWidgetItem* minItem = new QTableWidgetItem();
        if (row == 0) minItem->setText("0");
        else if (row == 1) minItem->setText("61");
        else minItem->setText("81");
        table->setItem(row, 0, minItem);

        // 分隔符（只读）
        QTableWidgetItem* sepItem = new QTableWidgetItem("-");
        sepItem->setFlags(sepItem->flags() & ~Qt::ItemIsEditable);
        sepItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, 1, sepItem);

        // 最大值
        QTableWidgetItem* maxItem = new QTableWidgetItem();
        if (row == 0) maxItem->setText("60");
        else if (row == 1) maxItem->setText("80");
        else maxItem->setText("100");
        table->setItem(row, 2, maxItem);

        // 颜色按钮
        QPushButton* colorBtn = new QPushButton();
        QColor defaultColor;
        if (row == 0) defaultColor = QColor(255, 255, 200); // 浅黄色
        else if (row == 1) defaultColor = QColor(255, 165, 0); // 橙色
        else defaultColor = QColor(255, 0, 0); // 红色
        colorBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #3e3e42; }").arg(defaultColor.name()));
        colorBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        table->setCellWidget(row, 3, colorBtn);
        colorBtn->update(); // 强制更新按钮样式
        connect(colorBtn, &QPushButton::clicked, this, [this, row, colorBtn]() {
            // 从样式表中提取当前颜色
            QString style = colorBtn->styleSheet();
            QColor currentColor = QColor(255, 255, 200); // 默认颜色
            if (row == 0) currentColor = QColor(255, 255, 200);
            else if (row == 1) currentColor = QColor(255, 165, 0);
            else currentColor = QColor(255, 0, 0);
            
            QColor color = QColorDialog::getColor(currentColor, this, "选择颜色");
            if (color.isValid()) {
                updateTableHeatmap(); // 更新热力图显示
                colorBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #3e3e42; }").arg(color.name()));
                colorBtn->update(); // 强制更新按钮样式
            }
        });

        // 人数（只读）
        QTableWidgetItem* countItem = new QTableWidgetItem("0");
        countItem->setFlags(countItem->flags() & ~Qt::ItemIsEditable);
        countItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, 4, countItem);

        // 百分数（只读）
        QTableWidgetItem* percentItem = new QTableWidgetItem("0%");
        percentItem->setFlags(percentItem->flags() & ~Qt::ItemIsEditable);
        percentItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, 5, percentItem);
    }

    mainLayout->addWidget(table);

    // 按钮
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnAddRow = new QPushButton("+");
    btnAddRow->setStyleSheet("background-color: #666666; color: white; padding: 6px 12px; border-radius: 4px; font-size: 16px; font-weight: bold; min-width: 40px; border: 1px solid #555555;");
    btnConfirm = new QPushButton("确定");
    btnConfirm->setStyleSheet("background-color: #666666; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px; border: 1px solid #555555;");
    QPushButton* btnCancel = new QPushButton("取消");
    btnCancel->setStyleSheet("background-color: #666666; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px; border: 1px solid #555555;");

    btnLayout->addWidget(btnAddRow);
    btnLayout->addStretch();
    btnLayout->addWidget(btnConfirm);
    btnLayout->addWidget(btnCancel);

    mainLayout->addLayout(btnLayout);

    connect(btnAddRow, &QPushButton::clicked, this, &HeatmapSegmentDialog::onAddRow);
    connect(btnConfirm, &QPushButton::clicked, this, &HeatmapSegmentDialog::onConfirm);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(table, &QTableWidget::cellChanged, this, &HeatmapSegmentDialog::onCellChanged);
    // 当属性选择改变时，重新计算所有统计信息
    connect(attributeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        updateAllStatistics();
    });
    // 当表格选择改变时，更新属性列表
    connect(tableComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HeatmapSegmentDialog::onTableChanged);
    
    // 初始化热力图显示
    updateTableHeatmap();
}

void HeatmapSegmentDialog::enterEvent(QEvent* event)
{
    QDialog::enterEvent(event);
    if (closeButton) closeButton->show();
}

void HeatmapSegmentDialog::leaveEvent(QEvent* event)
{
    QDialog::leaveEvent(event);
    if (closeButton) closeButton->hide();
}

void HeatmapSegmentDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QDialog::mousePressEvent(event);
}

void HeatmapSegmentDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

void HeatmapSegmentDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QDialog::mouseReleaseEvent(event);
}

void HeatmapSegmentDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    if (closeButton) {
        int margin = 8;
        closeButton->move(width() - closeButton->width() - margin, margin);
        closeButton->raise();
    }
}

void HeatmapSegmentDialog::setStudentData(const QList<struct StudentInfo>& students)
{
    m_students = students;
    updateAllStatistics();
}

void HeatmapSegmentDialog::setTableOptions(const QStringList& tables)
{
    tableComboBox->clear();
    tableComboBox->addItems(tables);
    if (tableComboBox->count() > 0) tableComboBox->setCurrentIndex(0);
}

void HeatmapSegmentDialog::setAttributeOptions(const QStringList& attributes)
{
    attributeComboBox->clear();
    attributeComboBox->addItems(attributes);
    if (attributeComboBox->count() > 0) attributeComboBox->setCurrentIndex(0);
}

QString HeatmapSegmentDialog::selectedTable() const
{
    return tableComboBox ? tableComboBox->currentText() : QString();
}

QString HeatmapSegmentDialog::selectedAttribute() const
{
    return attributeComboBox ? attributeComboBox->currentText() : QString();
}

QList<SegmentRange> HeatmapSegmentDialog::getSegments() const
{
    return m_segments;
}

void HeatmapSegmentDialog::updateStatistics()
{
    updateAllStatistics();
}

void HeatmapSegmentDialog::onAddRow()
{
    int row = table->rowCount();
    table->insertRow(row);

    // 最小值
    QTableWidgetItem* minItem = new QTableWidgetItem("0");
    table->setItem(row, 0, minItem);

    // 分隔符
    QTableWidgetItem* sepItem = new QTableWidgetItem("-");
    sepItem->setFlags(sepItem->flags() & ~Qt::ItemIsEditable);
    sepItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 1, sepItem);

    // 最大值
    QTableWidgetItem* maxItem = new QTableWidgetItem("100");
    table->setItem(row, 2, maxItem);

    // 颜色按钮
    QPushButton* colorBtn = new QPushButton();
    QColor defaultColor = QColor(200, 200, 200); // 灰色
    colorBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #3e3e42; }").arg(defaultColor.name()));
    colorBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    table->setCellWidget(row, 3, colorBtn);
    colorBtn->update(); // 强制更新按钮样式
    connect(colorBtn, &QPushButton::clicked, this, [this, row, colorBtn]() {
        QColor color = QColorDialog::getColor(colorBtn->palette().button().color(), this, "选择颜色");
        if (color.isValid()) {
            colorBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #3e3e42; }").arg(color.name()));
            colorBtn->update(); // 强制更新按钮样式
        }
    });

    // 人数
    QTableWidgetItem* countItem = new QTableWidgetItem("0");
    countItem->setFlags(countItem->flags() & ~Qt::ItemIsEditable);
    countItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 4, countItem);

    // 百分数
    QTableWidgetItem* percentItem = new QTableWidgetItem("0%");
    percentItem->setFlags(percentItem->flags() & ~Qt::ItemIsEditable);
    percentItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 5, percentItem);

    // 立即计算新行的统计信息（人数和百分数）
    // 确保所有项都已设置后再计算
    updateRowStatistics(row);
}

void HeatmapSegmentDialog::onDeleteRow()
{
    int row = table->currentRow();
    if (row >= 0) {
        table->removeRow(row);
        updateAllStatistics();
    }
}

void HeatmapSegmentDialog::onColorClicked()
{
    // 颜色按钮的点击事件在创建时已经连接
}

void HeatmapSegmentDialog::onConfirm()
{
    // 首先检查区间是否重叠
    QList<QPair<double, double>> ranges; // 存储所有有效的区间 (min, max)
    
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* minItem = table->item(row, 0);
        QTableWidgetItem* maxItem = table->item(row, 2);

        if (!minItem || !maxItem) continue;

        bool ok1, ok2;
        double minValue = minItem->text().toDouble(&ok1);
        double maxValue = maxItem->text().toDouble(&ok2);

        if (!ok1 || !ok2) continue;
        
        // 检查最小值是否小于等于最大值
        if (minValue > maxValue) {
            QMessageBox::warning(this, "提示", QString("第 %1 行的最小值不能大于最大值！").arg(row + 1));
            return;
        }

        ranges.append(QPair<double, double>(minValue, maxValue));
    }

    // 检查区间是否重叠
    for (int i = 0; i < ranges.size(); ++i) {
        for (int j = i + 1; j < ranges.size(); ++j) {
            double min1 = ranges[i].first;
            double max1 = ranges[i].second;
            double min2 = ranges[j].first;
            double max2 = ranges[j].second;
            
            // 两个区间重叠的条件：max1 >= min2 && max2 >= min1
            if (max1 >= min2 && max2 >= min1) {
                QMessageBox::warning(this, "提示", 
                    QString("第 %1 行和第 %2 行的区间范围重叠了！\n"
                            "第 %1 行：[%3, %4]\n"
                            "第 %2 行：[%5, %6]")
                    .arg(i + 1).arg(j + 1)
                    .arg(min1).arg(max1)
                    .arg(min2).arg(max2));
                return;
            }
        }
    }

    // 读取所有分段区间
    m_segments.clear();
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* minItem = table->item(row, 0);
        QTableWidgetItem* maxItem = table->item(row, 2);
        QPushButton* colorBtn = qobject_cast<QPushButton*>(table->cellWidget(row, 3));
        QTableWidgetItem* countItem = table->item(row, 4);
        QTableWidgetItem* percentItem = table->item(row, 5);

        if (!minItem || !maxItem || !colorBtn) continue;

        bool ok1, ok2;
        double minValue = minItem->text().toDouble(&ok1);
        double maxValue = maxItem->text().toDouble(&ok2);

        if (!ok1 || !ok2) continue;

        SegmentRange segment;
        segment.minValue = minValue;
        segment.maxValue = maxValue;
        // 从样式表中提取颜色
        QString style = colorBtn->styleSheet();
        QColor btnColor = QColor(200, 200, 200); // 默认灰色
        int bgIndex = style.indexOf("background-color:");
        if (bgIndex >= 0) {
            int start = bgIndex + QString("background-color:").length();
            int end = style.indexOf(";", start);
            if (end < 0) end = style.indexOf(" ", start);
            if (end >= 0) {
                QString colorStr = style.mid(start, end - start).trimmed();
                btnColor = QColor(colorStr);
            }
        }
        segment.color = btnColor.isValid() ? btnColor : QColor(200, 200, 200);
        segment.count = countItem ? countItem->text().toInt() : 0;
        segment.percentage = percentItem ? percentItem->text().replace("%", "").toDouble() : 0.0;

        m_segments.append(segment);
    }

    accept();
}

void HeatmapSegmentDialog::onCellChanged(int row, int column)
{
    if (column == 0 || column == 2) {
        updateRowStatistics(row);
    }
    updateTableHeatmap(); // 更新热力图显示
}

void HeatmapSegmentDialog::updateRowStatistics(int row)
{
    QTableWidgetItem* minItem = table->item(row, 0);
    QTableWidgetItem* maxItem = table->item(row, 2);
    QTableWidgetItem* countItem = table->item(row, 4);
    QTableWidgetItem* percentItem = table->item(row, 5);

    if (!minItem || !maxItem || !countItem || !percentItem) return;

    bool ok1, ok2;
    double minValue = minItem->text().toDouble(&ok1);
    double maxValue = maxItem->text().toDouble(&ok2);

    if (!ok1 || !ok2) {
        countItem->setText("0");
        percentItem->setText("0%");
        return;
    }

    // 获取当前选择的属性名称和表格名称（Excel文件名）
    QString attributeName = attributeComboBox ? attributeComboBox->currentText() : QString();
    QString excelFileName = tableComboBox ? tableComboBox->currentText() : QString();
    if (attributeName.isEmpty()) {
        countItem->setText("0");
        percentItem->setText("0%");
        return;
    }

    // 统计该区间内的学生数量（使用选择的属性值）
    int count = 0;
    for (const auto& student : m_students) {
        double attributeValue = 0.0;
        bool hasValue = false;
        
        // 优先从 attributesByExcel 中获取（如果指定了Excel文件名）
        if (!excelFileName.isEmpty() && student.attributesByExcel.contains(excelFileName)) {
            const QMap<QString, double>& excelAttrs = student.attributesByExcel[excelFileName];
            if (excelAttrs.contains(attributeName)) {
                attributeValue = excelAttrs[attributeName];
                hasValue = true;
            }
        }
        
        // 如果没有找到，尝试从 attributesFull 中获取（复合键名）
        if (!hasValue) {
            QString compositeKey = QString("%1_%2").arg(attributeName).arg(excelFileName);
            if (student.attributesFull.contains(compositeKey)) {
                attributeValue = student.attributesFull[compositeKey];
                hasValue = true;
            }
        }
        
        // 向后兼容：如果还没有找到，从 attributes 中获取（第一个值）
        if (!hasValue && student.attributes.contains(attributeName)) {
            attributeValue = student.attributes[attributeName];
            hasValue = true;
        }
        
        if (hasValue && attributeValue >= minValue && attributeValue <= maxValue) {
            count++;
        }
    }

    countItem->setText(QString::number(count));

    // 计算百分数
    double percentage = 0.0;
    if (m_students.size() > 0) {
        percentage = (count * 100.0) / m_students.size();
    }
    percentItem->setText(QString::number(percentage, 'f', 1) + "%");
}

void HeatmapSegmentDialog::updateAllStatistics()
{
    for (int row = 0; row < table->rowCount(); ++row) {
        updateRowStatistics(row);
    }
    updateTableHeatmap(); // 更新热力图显示
}

void HeatmapSegmentDialog::updateTableHeatmap()
{
    // 从表格中读取所有分段信息
    QList<SegmentRange> segments;
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* minItem = table->item(row, 0);
        QTableWidgetItem* maxItem = table->item(row, 2);
        QPushButton* colorBtn = qobject_cast<QPushButton*>(table->cellWidget(row, 3));
        
        if (!minItem || !maxItem || !colorBtn) continue;
        
        bool ok1, ok2;
        double minValue = minItem->text().toDouble(&ok1);
        double maxValue = maxItem->text().toDouble(&ok2);
        
        if (!ok1 || !ok2) continue;
        
        SegmentRange segment;
        segment.minValue = minValue;
        segment.maxValue = maxValue;
        
        // 从样式表中提取颜色
        QString style = colorBtn->styleSheet();
        QColor btnColor = QColor(200, 200, 200); // 默认灰色
        int bgIndex = style.indexOf("background-color:");
        if (bgIndex >= 0) {
            int start = bgIndex + QString("background-color:").length();
            int end = style.indexOf(";", start);
            if (end < 0) end = style.indexOf(" ", start);
            if (end >= 0) {
                QString colorStr = style.mid(start, end - start).trimmed();
                btnColor = QColor(colorStr);
            }
        }
        segment.color = btnColor.isValid() ? btnColor : QColor(200, 200, 200);
        
        segments.append(segment);
    }
    
    // 将分段信息传递给表格控件
    table->setSegments(segments);
}

QColor HeatmapSegmentDialog::getColorForRow(int row)
{
    QPushButton* colorBtn = qobject_cast<QPushButton*>(table->cellWidget(row, 3));
    if (colorBtn) {
        QString style = colorBtn->styleSheet();
        QColor btnColor = QColor(200, 200, 200);
        int bgIndex = style.indexOf("background-color:");
        if (bgIndex >= 0) {
            int start = bgIndex + QString("background-color:").length();
            int end = style.indexOf(";", start);
            if (end < 0) end = style.indexOf(" ", start);
            if (end >= 0) {
                QString colorStr = style.mid(start, end - start).trimmed();
                btnColor = QColor(colorStr);
            }
        }
        return btnColor.isValid() ? btnColor : QColor(200, 200, 200);
    }
    return QColor(200, 200, 200);
}

void HeatmapSegmentDialog::setColorForRow(int row, const QColor& color)
{
    QPushButton* colorBtn = qobject_cast<QPushButton*>(table->cellWidget(row, 3));
    if (colorBtn) {
        colorBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #3e3e42; }").arg(color.name()));
        colorBtn->update(); // 强制更新按钮样式
    }
}

void HeatmapSegmentDialog::onTableChanged(int index)
{
    if (!tableComboBox || index < 0) return;
    
    QString tableName = tableComboBox->itemText(index);
    if (tableName.isEmpty()) return;
    
    // 通过 parent 获取 ScheduleDialog 并调用其方法获取属性
    ScheduleDialog* scheduleDlg = qobject_cast<ScheduleDialog*>(parent());
    if (scheduleDlg) {
        QStringList attributes = scheduleDlg->getAttributesForTable(tableName);
        setAttributeOptions(attributes);
        // 更新统计信息
        updateAllStatistics();
    }
}

