#include "HeatmapViewDialog.h"
#include <QPainter>
#include <algorithm>
#include <cmath>

HeatmapViewDialog::HeatmapViewDialog(QWidget* parent)
    : QDialog(parent)
    , customPlot(nullptr)
    , colorMap(nullptr)
    , colorScale(nullptr)
    , m_heatmapType(2)
    , m_minValue(0)
    , m_maxValue(100)
{
    // 无标题栏，背景与分段对话框一致，可拖动
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setMouseTracking(true);
    resize(800, 600);
    setStyleSheet(
        "QDialog { background-color: #515151; color: white; font-size: 14px; }"
        "QLabel { color: white; }"
        //"QPushButton { background-color: #666; color: white; padding: 6px 12px; border-radius: 3px; font-size: 14px; border: 1px solid #555; }"
        //"QPushButton#btnClose { background-color: #cc4b4b; color: white; border: none; border-radius: 4px; font-weight: bold; font-size: 42px; }"
        //"QPushButton#btnClose:hover { background-color: #e25b5b; }"
        "QComboBox { color: white; background-color: #3a3a3a; padding: 6px 12px; border-radius: 3px; font-size: 14px; border: 1px solid #555; min-width: 140px; }"
        "QComboBox QAbstractItemView { color: white; background-color: #2d2f32; selection-background-color: #ff8c00; }"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // 顶部控件：表格下拉、属性下拉
    QHBoxLayout* controlLayout = new QHBoxLayout;
    tableComboBox = new QComboBox;
    attributeComboBox = new QComboBox;
    controlLayout->addWidget(tableComboBox);
    controlLayout->addWidget(attributeComboBox);
    controlLayout->addStretch();

    // 关闭按钮改为悬浮右上角显示/隐藏，不加入布局
    btnClose = new QPushButton("X", this);
    //btnClose->setObjectName("btnClose");
    btnClose->setStyleSheet(
        "QPushButton { background-color: #666666; color: white; border: none; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #777777; }"
    );
    btnClose->setFixedSize(26, 26);
    btnClose->hide();

    mainLayout->addLayout(controlLayout);

    // 创建 QCustomPlot 控件
    customPlot = new QCustomPlot(this);
    customPlot->setBackground(QBrush(QColor("#515151"))); // 设置背景色与对话框一致
    
    // 创建颜色映射对象
    colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
    
    // 创建颜色刻度条（右侧）
    colorScale = new QCPColorScale(customPlot);
    customPlot->plotLayout()->addElement(0, 1, colorScale); // 添加到右侧
    colorScale->setType(QCPAxis::atRight); // 刻度显示在右边
    colorMap->setColorScale(colorScale); // 关联热力图和刻度条
    
    // 设置颜色渐变为 Jet（蓝->青->黄->红）
    colorMap->setGradient(QCPColorGradient::gpJet);
    
    // 开启平滑插值（关键：实现平滑过渡效果）
    colorMap->setInterpolate(true);
    
    // 让热力图紧贴坐标轴
    colorMap->setTightBoundary(true);
    
    // 设置坐标轴样式（白色文字）
    customPlot->xAxis->setLabelColor(Qt::white);
    customPlot->yAxis->setLabelColor(Qt::white);
    customPlot->xAxis->setTickLabelColor(Qt::white);
    customPlot->yAxis->setTickLabelColor(Qt::white);
    customPlot->xAxis->setBasePen(QPen(Qt::white));
    customPlot->yAxis->setBasePen(QPen(Qt::white));
    customPlot->xAxis->setTickPen(QPen(Qt::white));
    customPlot->yAxis->setTickPen(QPen(Qt::white));
    customPlot->xAxis->setSubTickPen(QPen(Qt::white));
    customPlot->yAxis->setSubTickPen(QPen(Qt::white));
    colorScale->axis()->setLabelColor(Qt::white);
    colorScale->axis()->setTickLabelColor(Qt::white);
    colorScale->axis()->setBasePen(QPen(Qt::white));
    colorScale->axis()->setTickPen(QPen(Qt::white));
    colorScale->axis()->setSubTickPen(QPen(Qt::white));
    
    // 允许用户拖拽和缩放
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    
    mainLayout->addWidget(customPlot, 1); // 添加到布局，占据剩余空间

    // 底部确认按钮靠右
    QHBoxLayout* bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    QPushButton* btnConfirm = new QPushButton("确定");
    btnConfirm->setStyleSheet("QPushButton { background-color: #666666; color: white; padding: 6px 18px; border: 1px solid #555555; font-weight: bold; border-radius: 3px; }"
                              "QPushButton:hover { background-color: #777777; }");
    bottomLayout->addWidget(btnConfirm);
    mainLayout->addLayout(bottomLayout);

    connect(btnClose, &QPushButton::clicked, this, &HeatmapViewDialog::onClose);
    connect(btnConfirm, &QPushButton::clicked, this, &HeatmapViewDialog::onClose);
    connect(tableComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HeatmapViewDialog::onSelectionChanged);
    connect(attributeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HeatmapViewDialog::onSelectionChanged);
}

void HeatmapViewDialog::setStudentData(const QList<struct StudentInfo>& students)
{
    m_students = students;
    recomputeRange();
    updateHeatmap();
}

void HeatmapViewDialog::setSeatMap(const QMap<QString, QPair<int, int>>& seatMap, int maxRows, int maxCols)
{
    m_seatMap = seatMap;
    m_maxSeatRows = maxRows;
    m_maxSeatCols = maxCols;
    updateHeatmap();
}

void HeatmapViewDialog::setSegments(const QList<struct SegmentRange>& segments)
{
    m_segments = segments;
    // 分段图已移除，保留数据但不刷新
}

void HeatmapViewDialog::setHeatmapType(int type)
{
    Q_UNUSED(type);
    m_heatmapType = 2; // 固定为渐变图
    updateHeatmap();
}

void HeatmapViewDialog::setTableOptions(const QStringList& tables)
{
    tableComboBox->clear();
    tableComboBox->addItems(tables);
    if (!tables.isEmpty()) {
        tableComboBox->setCurrentIndex(0);
        m_selectedTable = tableComboBox->currentText();
        // 当表格变更时，动态刷新属性
        if (m_attrFetcher) {
            auto attrs = m_attrFetcher(m_selectedTable);
            setAttributeOptions(attrs);
        }
    }
}

void HeatmapViewDialog::setAttributeOptions(const QStringList& attributes)
{
    attributeComboBox->clear();
    attributeComboBox->addItems(attributes);
    if (!attributes.isEmpty()) {
        attributeComboBox->setCurrentIndex(0);
        m_selectedAttribute = attributeComboBox->currentText();
        recomputeRange();
        updateHeatmap();
    }
}

void HeatmapViewDialog::setSelectedTable(const QString& tableName)
{
    int idx = tableComboBox->findText(tableName);
    if (idx >= 0) {
        tableComboBox->setCurrentIndex(idx);
        m_selectedTable = tableName;
        if (m_attrFetcher) {
            auto attrs = m_attrFetcher(tableName);
            setAttributeOptions(attrs);
        }
        recomputeRange();
        updateHeatmap();
    }
}

void HeatmapViewDialog::setSelectedAttribute(const QString& attributeName)
{
    int idx = attributeComboBox->findText(attributeName);
    if (idx >= 0) {
        attributeComboBox->setCurrentIndex(idx);
        m_selectedAttribute = attributeName;
        recomputeRange();
        updateHeatmap();
    }
}

void HeatmapViewDialog::setAttributeFetcher(const std::function<QStringList(const QString&)>& fetcher)
{
    m_attrFetcher = fetcher;
}
void HeatmapViewDialog::updateHeatmap()
{
    if (!customPlot || !colorMap || m_students.isEmpty()) {
        return;
    }

    int cols, rows;
    
    // 如果有座位映射，使用实际座位布局；否则使用默认布局
    if (!m_seatMap.isEmpty() && m_maxSeatRows > 0 && m_maxSeatCols > 0) {
        // 使用实际座位布局
        rows = m_maxSeatRows;
        cols = m_maxSeatCols;
    } else {
        // 默认布局：根据学生数量动态调整
        int studentCount = m_students.size();
        cols = 20; // 固定列数
        rows = (studentCount + cols - 1) / cols; // 向上取整
        if (rows < 1) rows = 1;
        if (cols < 1) cols = 1;
    }

    // 设置数据的维度
    colorMap->data()->setSize(cols, rows);
    
    // 设置坐标轴范围（对应行列号）
    colorMap->data()->setRange(QCPRange(0, cols), QCPRange(0, rows));
    
    // 初始化所有单元格为最小值（用于空白座位）
    for (int x = 0; x < cols; ++x) {
        for (int y = 0; y < rows; ++y) {
            colorMap->data()->setCell(x, y, m_minValue);
        }
    }
    
    // 填充数据：根据座位位置或默认顺序
    if (!m_seatMap.isEmpty()) {
        // 使用实际座位位置
        for (const auto& student : m_students) {
            if (m_seatMap.contains(student.id)) {
                QPair<int, int> seat = m_seatMap[student.id];
                int col = seat.second; // 列号
                int row = seat.first;  // 行号
                
                // 检查范围
                if (col >= 0 && col < cols && row >= 0 && row < rows) {
                    // 获取学生属性值
                    double value = getStudentValue(student);
                    // 填入数据（注意：QCPColorMap 的坐标系统，x 是列，y 是行）
                    colorMap->data()->setCell(col, row, value);
                }
            }
        }
        
        // 填充空白座位：使用周围值的平均值
        for (int x = 0; x < cols; ++x) {
            for (int y = 0; y < rows; ++y) {
                // 检查这个位置是否有学生
                bool hasStudent = false;
                for (const auto& student : m_students) {
                    if (m_seatMap.contains(student.id)) {
                        QPair<int, int> seat = m_seatMap[student.id];
                        if (seat.second == x && seat.first == y) {
                            hasStudent = true;
                            break;
                        }
                    }
                }
                
                if (!hasStudent) {
                    // 空白座位：使用周围值的平均值
                    double sum = 0;
                    int count = 0;
                    for (int dx = -1; dx <= 1; ++dx) {
                        for (int dy = -1; dy <= 1; ++dy) {
                            if (dx == 0 && dy == 0) continue;
                            int nx = x + dx;
                            int ny = y + dy;
                            if (nx >= 0 && nx < cols && ny >= 0 && ny < rows) {
                                double cellValue = colorMap->data()->cell(nx, ny);
                                if (cellValue != m_minValue) { // 如果有实际值
                                    sum += cellValue;
                                    count++;
                                }
                            }
                        }
                    }
                    if (count > 0) {
                        colorMap->data()->setCell(x, y, sum / count);
                    }
                    // 如果 count == 0，保持为 m_minValue（已在初始化时设置）
                }
            }
        }
    } else {
        // 默认布局：按顺序排列
        QList<struct StudentInfo> sortedStudents = m_students;
        std::sort(sortedStudents.begin(), sortedStudents.end(),
            [](const struct StudentInfo& a, const struct StudentInfo& b) {
                return a.id < b.id; // 按学号排序，保持一致性
            });

        for (int i = 0; i < sortedStudents.size(); ++i) {
            int x = i % cols; // 列号
            int y = i / cols; // 行号
            
            if (x >= cols || y >= rows) break; // 超出范围
            
            // 获取学生属性值
            double value = getStudentValue(sortedStudents[i]);
            
            // 填入数据（注意：QCPColorMap 的坐标系统，x 是列，y 是行）
            colorMap->data()->setCell(x, y, value);
        }
        
        // 填充空白区域（如果有的话）：使用周围值的平均值
        for (int x = 0; x < cols; ++x) {
            for (int y = 0; y < rows; ++y) {
                int index = y * cols + x;
                if (index >= sortedStudents.size()) {
                    // 空白区域：使用周围值的平均值
                    double sum = 0;
                    int count = 0;
                    for (int dx = -1; dx <= 1; ++dx) {
                        for (int dy = -1; dy <= 1; ++dy) {
                            if (dx == 0 && dy == 0) continue;
                            int nx = x + dx;
                            int ny = y + dy;
                            if (nx >= 0 && nx < cols && ny >= 0 && ny < rows) {
                                int nIndex = ny * cols + nx;
                                if (nIndex < sortedStudents.size()) {
                                    sum += getStudentValue(sortedStudents[nIndex]);
                                    count++;
                                }
                            }
                        }
                    }
                    if (count > 0) {
                        colorMap->data()->setCell(x, y, sum / count);
                    } else {
                        // 如果没有周围值，使用最小值
                        colorMap->data()->setCell(x, y, m_minValue);
                    }
                }
            }
        }
    }
    
    // 自动调整颜色范围以匹配数据的最大最小值
    colorMap->rescaleDataRange(true);
    
    // 设置坐标轴标签
    customPlot->xAxis->setLabel("座位列号");
    customPlot->yAxis->setLabel("座位行号");
    
    // 设置颜色刻度条标签（使用当前选中的属性名）
    QString labelText = m_selectedAttribute.isEmpty() ? "成绩" : m_selectedAttribute;
    colorScale->axis()->setLabel(labelText);
    
    // 重绘
    customPlot->replot();
}

void HeatmapViewDialog::onClose()
{
    close();
}

void HeatmapViewDialog::enterEvent(QEvent* event)
{
    QDialog::enterEvent(event);
    if (btnClose) btnClose->show();
}

void HeatmapViewDialog::leaveEvent(QEvent* event)
{
    QDialog::leaveEvent(event);
    if (btnClose) btnClose->hide();
}

void HeatmapViewDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QDialog::mousePressEvent(event);
}

void HeatmapViewDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

void HeatmapViewDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QDialog::mouseReleaseEvent(event);
}

void HeatmapViewDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    if (btnClose) {
        int margin = 6;
        btnClose->move(width() - btnClose->width() - margin, margin);
        btnClose->raise();
    }
}

void HeatmapViewDialog::onSelectionChanged()
{
    QObject* s = sender();
    if (s == tableComboBox) {
        m_selectedTable = tableComboBox->currentText();
        if (m_attrFetcher) {
            auto attrs = m_attrFetcher(m_selectedTable);
            setAttributeOptions(attrs);
        }
    } else if (s == attributeComboBox) {
        m_selectedAttribute = attributeComboBox->currentText();
    } else {
        m_selectedTable = tableComboBox->currentText();
        m_selectedAttribute = attributeComboBox->currentText();
    }
    // 确保属性选择有值
    if (m_selectedAttribute.isEmpty() && attributeComboBox->count() > 0) {
        m_selectedAttribute = attributeComboBox->currentText();
    }
    recomputeRange();
    updateHeatmap();
}

void HeatmapViewDialog::recomputeRange()
{
    // 计算当前选中属性的最小值和最大值（缺失则为0）
    m_minValue = 0;
    m_maxValue = 0;
    bool hasValue = false;
    for (const auto& stu : m_students) {
        double val = getStudentValue(stu);
        if (!hasValue) {
            m_minValue = m_maxValue = val;
            hasValue = true;
        } else {
            if (val < m_minValue) m_minValue = val;
            if (val > m_maxValue) m_maxValue = val;
        }
    }
}

double HeatmapViewDialog::getStudentValue(const struct StudentInfo& stu) const
{
    // 使用学生信息的 getAttributeValue，支持按表格名
    if (!m_selectedAttribute.isEmpty()) {
        return stu.getAttributeValue(m_selectedAttribute, m_selectedTable);
    }
    // 如果未选择属性，回退使用 score
    return stu.score;
}

