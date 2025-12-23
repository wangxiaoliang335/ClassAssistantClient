#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QWidget>
#include <QPainter>
#include <QDebug>
#include <QList>
#include <QColor>
#include <QMap>
#include <QPair>
#include <QMouseEvent>
#include <QResizeEvent>
#include <functional>
#include <algorithm>
#include <QStyleOption>
#include <QPaintEvent>
#include "qcustomplot.h" // QCustomPlot 库
//#include "ScheduleDialog.h" // 包含 StudentInfo 定义
#include "CommonInfo.h"
#include "HeatmapTypes.h" // 包含 SegmentRange 定义

class HeatmapViewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HeatmapViewDialog(QWidget* parent = nullptr);
    
    // 设置学生数据
    void setStudentData(const QList<struct StudentInfo>& students);
    
    // 设置座位映射（学生ID -> (行, 列)）
    void setSeatMap(const QMap<QString, QPair<int, int>>& seatMap, int maxRows, int maxCols);
    
    // 设置分段区间（兼容保留，不再使用分段图）
    void setSegments(const QList<struct SegmentRange>& segments);
    
    // 设置热力图类型（仅保留接口，强制使用渐变图）
    void setHeatmapType(int type);

    // 设置下拉选项
    void setTableOptions(const QStringList& tables);
    void setAttributeOptions(const QStringList& attributes);
    void setSelectedTable(const QString& tableName);
    void setSelectedAttribute(const QString& attributeName);
    void setAttributeFetcher(const std::function<QStringList(const QString&)>& fetcher);

protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onClose();
    void onSelectionChanged();

private:
    void updateHeatmap(); // 更新热力图数据
    void recomputeRange();
    double getStudentValue(const struct StudentInfo& stu) const;
    
    QPushButton* btnClose;
    QComboBox* tableComboBox;
    QComboBox* attributeComboBox;
    QCustomPlot* customPlot; // QCustomPlot 绘图控件
    QCPColorMap* colorMap; // 颜色映射对象
    QCPColorScale* colorScale; // 颜色刻度条
    QList<struct StudentInfo> m_students;
    QList<struct SegmentRange> m_segments;
    QMap<QString, QPair<int, int>> m_seatMap; // 学生ID -> (行, 列)
    int m_maxSeatRows = 0; // 最大座位行数
    int m_maxSeatCols = 0; // 最大座位列数
    int m_heatmapType; // 保持字段，固定为2（渐变）
    double m_minValue;
    double m_maxValue;
    QString m_selectedTable;
    QString m_selectedAttribute;
    std::function<QStringList(const QString&)> m_attrFetcher;
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

