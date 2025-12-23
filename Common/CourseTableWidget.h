#pragma once
#include <QTableWidget>
#include <QHeaderView>
#include <QFont>
#include <QBrush>
#include <QStringList>
#include <QPainter>
#include <QPaintEvent>
#include <QAbstractButton>
#include <QEvent>

/**
 * 可编辑课程表控件
 * 支持横向星期列、纵向时间行、单元格编辑与高亮
 */
class CourseTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    CourseTableWidget(QWidget* parent = nullptr)
        : QTableWidget(parent),
        m_times(QStringList() 
            << QStringLiteral("早读 7:00-7:40")
            << QStringLiteral("第一节 7:50-8:30")
            << QStringLiteral("第二节 8:40-9:20")
            << QStringLiteral("大课间 9:20-9:40")
            << QStringLiteral("第三节 9:40-10:20")
            << QStringLiteral("第四节 10:30-11:10")
            << QStringLiteral("午休 11:10-13:30")
            << QStringLiteral("第五节 13:30-14:10")
            << QStringLiteral("第六节 14:20-15:00")
            << QStringLiteral("眼保健操 15:00-15:20")
            << QStringLiteral("第七节 15:20-16:00")
            << QStringLiteral("课服1 16:10-16:50")
            << QStringLiteral("课服2 17:00-17:40")
            << QStringLiteral("晚自习1 19:00-19:40")
            << QStringLiteral("晚自习2 19:50-20:30")),
        m_days(QStringList() << "周一" << "周二" << "周三"
            << "周四" << "周五" << "周六" << "周日")
    {
        // 设置深色主题背景色（类似IDE深色主题）
        setStyleSheet(
            "QTableWidget {"
            "background-color: #1e1e1e;"
            "color: #d4d4d4;"
            "border: 1px solid #3e3e42;"
            "gridline-color: #3e3e42;"
            "}"
            "QHeaderView::section {"
            "background-color: #252526;"
            "color: #cccccc;"
            "font-weight: bold;"
            "border: 1px solid #3e3e42;"
            "padding: 4px;"
            "}"
            "QTableWidget::item {"
            "border: 1px solid #3e3e42;"
            "color: #d4d4d4;"
            "}"
            "QTableWidget::item:selected {"
            "background-color: #094771;"
            "color: #ffffff;"
            "}"
        );

        // 尝试找到角按钮并安装事件过滤器，用于自定义绘制
        QAbstractButton* cornerBtn = findChild<QAbstractButton*>();
        if (cornerBtn) {
            cornerBtn->installEventFilter(this);
        }

        // 初始化表格
        initTable();
    }

    // 初始化表格布局
    void initTable()
    {
        setRowCount(m_times.size());
        setColumnCount(m_days.size());

        setHorizontalHeaderLabels(m_days);
        setVerticalHeaderLabels(m_times);

        // 表格样式
        horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        verticalHeader()->setSectionResizeMode(QHeaderView::Fixed); // 改为Fixed以便设置固定行高
        setEditTriggers(QAbstractItemView::DoubleClicked); // 双击可编辑

        // 设置行高（让每个单元格更高）
        verticalHeader()->setDefaultSectionSize(50); // 设置默认行高为50像素

        // 启用角按钮，使用eventFilter来自绘（而不是隐藏）
        setCornerButtonEnabled(true);

        // 默认填充为空
        for (int row = 0; row < rowCount(); ++row) {
            // 为每一行设置行高
            setRowHeight(row, 50);
            for (int col = 0; col < columnCount(); ++col) {
                QTableWidgetItem* item = new QTableWidgetItem("");
                item->setTextAlignment(Qt::AlignCenter);
                setItem(row, col, item);
            }
        }
    }

    // 设置课程
    void setCourse(int row, int col, const QString& name, bool highlight = false)
    {
        if (!validIndex(row, col)) return;

        QTableWidgetItem* item = this->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            setItem(row, col, item);
        }

        item->setText(name);
        item->setTextAlignment(Qt::AlignCenter);

        QFont font;
        font.setPointSize(12);
        item->setFont(font);

        if (highlight) {
            item->setBackground(QBrush(QColor(51, 153, 255))); // 蓝色背景
            item->setForeground(QBrush(Qt::white));           // 白色文字
        }
        else {
            item->setBackground(QBrush(QColor(30, 30, 30)));   // 深色背景 #1e1e1e（与样式表一致）
            item->setForeground(QBrush(QColor(212, 212, 212))); // 浅灰色文字 #d4d4d4
        }
    }

    // 获取课程
    QString courseAt(int row, int col) const
    {
        if (!validIndex(row, col)) return "";
        auto item = this->item(row, col);
        return item ? item->text() : "";
    }

protected:
    // 重写paintEvent来自绘背景颜色
    void paintEvent(QPaintEvent* event) override
    {
        // 先绘制整体背景，避免角落区域被白色覆盖
        QPainter bgPainter(this);
        bgPainter.fillRect(rect(), QColor(30, 30, 30)); // #1e1e1e
        
        // 再绘制数据区 viewport 背景
        QPainter vpPainter(viewport());
        vpPainter.fillRect(viewport()->rect(), QColor(30, 30, 30)); // #1e1e1e
        
        // 调用父类绘制单元格、表头等
        QTableWidget::paintEvent(event);
    }

    // 事件过滤器，用于自定义绘制角按钮
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Paint) {
            QAbstractButton* btn = qobject_cast<QAbstractButton*>(watched);
            if (btn) { // 角按钮区域自绘
                QPainter p(btn);
                p.fillRect(btn->rect(), QColor(30, 30, 30)); // #1e1e1e
                p.setPen(QColor(62, 62, 66)); // #3e3e42
                p.drawRect(btn->rect().adjusted(0, 0, -1, -1));
                return true; // 阻止默认绘制
            }
        }
        return QTableWidget::eventFilter(watched, event);
    }

private:
    QStringList m_times;
    QStringList m_days;

    bool validIndex(int row, int col) const
    {
        return row >= 0 && row < rowCount() && col >= 0 && col < columnCount();
    }
};
#pragma once
