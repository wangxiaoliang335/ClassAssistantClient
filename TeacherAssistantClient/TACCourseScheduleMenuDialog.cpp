#pragma execution_character_set("utf-8")
#include "TACCourseScheduleMenuDialog.h"
#include <QApplication>
#include <QDesktopWidget>

// 使用深色背景色
static const QColor DIALOG_BACKGROUND_COLOR(40, 40, 40, 240);

TACCourseScheduleMenuDialog::TACCourseScheduleMenuDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(200, 120);
    
    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);
    
    // 周课表按钮
    m_weekScheduleButton = new QPushButton(this);
    m_weekScheduleButton->setText(QString::fromUtf8(u8"课程表"));
    m_weekScheduleButton->setFixedHeight(40);
    m_weekScheduleButton->setStyleSheet(
        "QPushButton {"
        "background-color: #4CAF50;"
        "color: white;"
        "border: 1px solid rgba(255,255,255,0.18);"
        "border-radius: 8px;"
        "font-size: 14px;"
        "font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "background-color: #3d8b40;"
        "}"
    );
    connect(m_weekScheduleButton, &QPushButton::clicked, this, &TACCourseScheduleMenuDialog::onWeekScheduleClicked);
    mainLayout->addWidget(m_weekScheduleButton);
    
    // 今日课表按钮
    m_todayScheduleButton = new QPushButton(this);
    m_todayScheduleButton->setText(QString::fromUtf8(u8"今日课表"));
    m_todayScheduleButton->setFixedHeight(40);
    m_todayScheduleButton->setStyleSheet(
        "QPushButton {"
        "background-color: #4CAF50;"
        "color: white;"
        "border: 1px solid rgba(255,255,255,0.18);"
        "border-radius: 8px;"
        "font-size: 14px;"
        "font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "background-color: #3d8b40;"
        "}"
    );
    connect(m_todayScheduleButton, &QPushButton::clicked, this, &TACCourseScheduleMenuDialog::onTodayScheduleClicked);
    mainLayout->addWidget(m_todayScheduleButton);
    
    setLayout(mainLayout);
}

TACCourseScheduleMenuDialog::~TACCourseScheduleMenuDialog()
{
}

void TACCourseScheduleMenuDialog::onWeekScheduleClicked()
{
    emit weekScheduleSelected();
    accept();
}

void TACCourseScheduleMenuDialog::onTodayScheduleClicked()
{
    emit todayScheduleSelected();
    accept();
}

void TACCourseScheduleMenuDialog::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // 绘制圆角背景
    QPainterPath path;
    path.addRoundedRect(rect(), 10, 10);
    
    // 深色半透明背景
    p.fillPath(path, DIALOG_BACKGROUND_COLOR);
    
    // 边框
    QPen pen(QColor(60, 60, 60, 255));
    pen.setWidth(1);
    p.setPen(pen);
    p.drawPath(path);
}

void TACCourseScheduleMenuDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
}

void TACCourseScheduleMenuDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        m_dragging = true;
        event->accept();
    }
}

void TACCourseScheduleMenuDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
    }
}

void TACCourseScheduleMenuDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}

