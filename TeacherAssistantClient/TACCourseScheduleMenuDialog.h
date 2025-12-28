#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QEvent>
#include <QCursor>
#include <QRect>

class TACCourseScheduleMenuDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TACCourseScheduleMenuDialog(QWidget* parent = nullptr);
    ~TACCourseScheduleMenuDialog();

signals:
    void weekScheduleSelected(); // 周课表选项被选择
    void todayScheduleSelected(); // 今日课表选项被选择

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onWeekScheduleClicked();
    void onTodayScheduleClicked();

private:
    QPushButton* m_weekScheduleButton = nullptr;
    QPushButton* m_todayScheduleButton = nullptr;
    
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

