#pragma once
#include <QTableWidget>
#include <QHeaderView>
#include <QFont>
#include <QBrush>
#include <QStringList>
#include <QPainter>
#include <QPaintEvent>

/**
 * 小组管理表控件
 * 格式：第一列为组号，倒数第二列为个人总分，最后一列为小组总分
 * 默认列：组号、学号、姓名、语文、数学、英语、总分、小组总分
 */
class GroupScoreTableWidget : public QTableWidget
{
public:
    GroupScoreTableWidget(QWidget* parent = nullptr)
        : QTableWidget(parent)
    {
        // 初始化表格
        initTable();
    }

    // 初始化表格布局
    void initTable()
    {
        setRowCount(6);
        setColumnCount(8);

        // 默认列：组号、学号、姓名、语文、数学、英语、总分、小组总分
        QStringList headers = { "组号", "学号", "姓名", "语文", "数学", "英语", "总分", "小组总分" };
        setHorizontalHeaderLabels(headers);

        // 表格样式
        setStyleSheet(
            "QTableWidget { background-color: white; gridline-color: #ddd; }"
            "QTableWidget::item { padding: 5px; }"
            "QHeaderView::section { background-color: #808080; color: white; font-weight: bold; padding: 8px; }"
            "QTableWidget QTableCornerButton::section { background-color: #808080; }"
            "QHeaderView::section:vertical { background-color: #808080; color: white; font-weight: bold; padding: 8px; }"
        );
        setAlternatingRowColors(true);
        setStyleSheet(styleSheet() + 
            "QTableWidget { alternate-background-color: #e6f3ff; }"
        );
        
        // 确保垂直表头也应用样式
        if (verticalHeader()) {
            verticalHeader()->setStyleSheet("QHeaderView::section { background-color: #808080; color: white; font-weight: bold; padding: 8px; }");
            // 为垂直表头安装事件过滤器以便自绘
            verticalHeader()->installEventFilter(this);
        }

        // 确保水平表头也应用样式
        if (horizontalHeader()) {
            horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #808080; color: white; font-weight: bold; padding: 8px; }");
            // 为水平表头安装事件过滤器以便自绘
            horizontalHeader()->installEventFilter(this);
        }

        // 尝试找到角按钮并安装事件过滤器，用于自定义绘制
        QAbstractButton* cornerBtn = findChild<QAbstractButton*>();
        if (cornerBtn) {
            cornerBtn->installEventFilter(this);
        }

        // 表格设置
        horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
        setSelectionBehavior(QAbstractItemView::SelectItems);
        setSelectionMode(QAbstractItemView::SingleSelection);

        // 初始化所有单元格
        for (int row = 0; row < rowCount(); ++row) {
            for (int col = 0; col < columnCount(); ++col) {
                QTableWidgetItem* item = new QTableWidgetItem("");
                item->setTextAlignment(Qt::AlignCenter);
                setItem(row, col, item);
            }
        }
    }

protected:
    // 重写paintEvent来自绘背景颜色
    void paintEvent(QPaintEvent* event) override
    {
        // 先绘制整体背景，避免角落区域被白色覆盖
        QPainter bgPainter(this);
        bgPainter.fillRect(rect(), QColor(128, 128, 128)); // #808080

        // 再绘制数据区 viewport 背景
        QPainter vpPainter(viewport());
        vpPainter.fillRect(viewport()->rect(), QColor(128, 128, 128)); // #808080

        // 调用父类绘制单元格、表头等
        QTableWidget::paintEvent(event);
    }

    // 事件过滤器，用于自定义绘制垂直表头
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Paint) {
            QAbstractButton* btn = qobject_cast<QAbstractButton*>(watched);
            if (btn) { // 角按钮区域自绘
                QPainter p(btn);
                p.fillRect(btn->rect(), QColor(128, 128, 128)); // #808080
                p.setPen(QColor(62, 62, 66)); // #3e3e42
                p.drawRect(btn->rect().adjusted(0, 0, -1, -1));
                return true; // 阻止默认绘制
            }
        }
        return QTableWidget::eventFilter(watched, event);
    }

private:
    bool validIndex(int row, int col) const
    {
        return row >= 0 && row < rowCount() && col >= 0 && col < columnCount();
    }
};

