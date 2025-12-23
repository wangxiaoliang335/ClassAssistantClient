#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QColorDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>
#include <QColor>
#include <QList>
#include <QComboBox>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QPaintEvent>
//#include "ScheduleDialog.h" // 包含 StudentInfo 定义
#include "CommonInfo.h"
#include "HeatmapTypes.h" // 包含 SegmentRange 定义

// 自定义表格控件，用于绘制热力图背景
class HeatmapTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    explicit HeatmapTableWidget(QWidget* parent = nullptr) : QTableWidget(parent) {
        //// 尝试找到角按钮并安装事件过滤器，用于自定义绘制
        //QAbstractButton* cornerBtn = findChild<QAbstractButton*>();
        //if (cornerBtn) {
        //    cornerBtn->installEventFilter(this);
        //}
    }
    
    void setSegments(const QList<SegmentRange>& segments) {
        m_segments = segments;
        viewport()->update(); // 触发重绘
    }
    
protected:
    //void paintEvent(QPaintEvent* event) override {
    //    // 先绘制整体背景，避免角落区域被白色覆盖
    //    QPainter bgPainter(this);
    //    bgPainter.fillRect(rect(), QColor(30, 30, 30)); // #1e1e1e
    //    
    //    // 再绘制数据区 viewport 背景
    //    QPainter vpPainter(viewport());
    //    vpPainter.fillRect(viewport()->rect(), QColor(30, 30, 30)); // #1e1e1e
    //    
    //    // 调用父类绘制单元格、表头等
    //    QTableWidget::paintEvent(event);
    //    
    //    // 注意：颜色列（第3列）保持按钮的原始颜色，其他列通过 ensureCellColors() 设置为灰色
    //}

    //// 事件过滤器，用于自定义绘制角按钮
    //bool eventFilter(QObject* watched, QEvent* event) override
    //{
    //    if (event->type() == QEvent::Paint) {
    //        QAbstractButton* btn = qobject_cast<QAbstractButton*>(watched);
    //        if (btn) { // 角按钮区域自绘
    //            QPainter p(btn);
    //            p.fillRect(btn->rect(), QColor(30, 30, 30)); // #1e1e1e
    //            p.setPen(QColor(62, 62, 66)); // #3e3e42
    //            p.drawRect(btn->rect().adjusted(0, 0, -1, -1));
    //            return true; // 阻止默认绘制
    //        }
    //    }
    //    return QTableWidget::eventFilter(watched, event);
    //}
    
private:
    QList<SegmentRange> m_segments;
    
    QColor getRowColor(int row) const {
        if (row < 0 || row >= rowCount()) return QColor();
        
        // 获取该行的最小值（用于确定颜色）
        QTableWidgetItem* minItem = item(row, 0);
        if (!minItem) return QColor();
        
        bool ok;
        double minValue = minItem->text().toDouble(&ok);
        if (!ok) return QColor();
        
        // 根据最小值找到对应的分段颜色
        for (const auto& segment : m_segments) {
            if (minValue >= segment.minValue && minValue <= segment.maxValue) {
                return segment.color;
            }
        }
        
        return QColor();
    }
};

class HeatmapSegmentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HeatmapSegmentDialog(QWidget* parent = nullptr);
    
    // 设置学生数据（用于计算人数和百分数）
    void setStudentData(const QList<struct StudentInfo>& students);
    void setTableOptions(const QStringList& tables);
    void setAttributeOptions(const QStringList& attributes);
    QString selectedTable() const;
    QString selectedAttribute() const;
    
    // 获取分段区间列表
    QList<SegmentRange> getSegments() const;
    
    // 更新分段统计（重新计算人数和百分数）
    void updateStatistics();

private slots:
    void onAddRow();
    void onDeleteRow();
    void onColorClicked();
    void onConfirm();
    void onCellChanged(int row, int column);
    void onTableChanged(int index); // 当表格选择改变时

private:
    void updateRowStatistics(int row);
    void updateAllStatistics();
    void updateTableHeatmap(); // 更新表格热力图显示
    QColor getColorForRow(int row);
    void setColorForRow(int row, const QColor& color);
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
    HeatmapTableWidget* table;
    QPushButton* btnAddRow;
    QPushButton* btnConfirm;
    QComboBox* tableComboBox;
    QComboBox* attributeComboBox;
    QPushButton* closeButton = nullptr;
    bool m_dragging = false;
    QPoint m_dragStartPos;
    QList<struct StudentInfo> m_students;
    QList<SegmentRange> m_segments;
};

