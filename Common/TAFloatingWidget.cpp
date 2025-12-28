#include "TAFloatingWidget.h"
#include <QPainter>
#include <QPen>
#include <QPainterPath>
#include <QBrush>
#include <QApplication>
#include <QScreen>
TAFloatingWidget::TAFloatingWidget(QWidget *parent)
	: QWidget(parent), m_dragging(false),m_backgroundColor(QColor(50, 50, 50, 0.8)),
    m_borderColor(QColor(255, 255, 255, 0.1)),m_radius(0),m_borderWidth(0), m_visibleCloseButton(true)
{
    setWindowFlags(
        Qt::FramelessWindowHint |
        Qt::Tool |
        Qt::WindowStaysOnTopHint
    );
    setAttribute(Qt::WA_TranslucentBackground);
    closeButton = new QPushButton(this);
    closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
    closeButton->setIconSize(QSize(22, 22));
    closeButton->setFixedSize(QSize(22, 22));
    closeButton->setStyleSheet("background: transparent;");
    connect(closeButton, &QPushButton::clicked, this, [=]() {
        this->close();
    });
    closeButton->hide();
    
    // 创建最小化/隐藏按钮（圆形按钮）
    minimizeButton = new QPushButton(this);
    minimizeButton->setFixedSize(QSize(22, 22));
    minimizeButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(150, 150, 150, 200);"
        "border: none;"
        "border-radius: 11px;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(180, 180, 180, 200);"
        "}"
    );
    connect(minimizeButton, &QPushButton::clicked, this, [=]() {
        this->hide(); // 隐藏窗口
    });
    minimizeButton->hide();

}
void TAFloatingWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    initShow();
}


TAFloatingWidget::~TAFloatingWidget()
{}
void TAFloatingWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

QColor TAFloatingWidget::borderColor() const
{
    return m_borderColor;
}

void TAFloatingWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPosition);
        event->accept();
    }
}

void TAFloatingWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}
int TAFloatingWidget::borderWidth()
{
    return m_borderWidth;
}
int TAFloatingWidget::radius()
{
    return m_radius;
}
void TAFloatingWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    initShow();
    // 右上角按钮：最小化按钮在左，关闭按钮在右
    minimizeButton->move(this->width() - 44, 0);
    closeButton->move(this->width() - 22, 0);
}

void TAFloatingWidget::enterEvent(QEvent* event)
{
    QWidget::enterEvent(event);
    if(m_visibleCloseButton) {
        closeButton->show();
        minimizeButton->show();
    }
}

void TAFloatingWidget::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    closeButton->hide();
    minimizeButton->hide();
}
QRect TAFloatingWidget::getScreenGeometryWithTaskbar()
{
    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    if (screen) {
        return screen->geometry();
    }
    return QRect();
}
void TAFloatingWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
   
    
    QRect rect(0, 0, width(), height());
    int cornerRadius = m_radius;
    QPainterPath path;
    path.addRoundedRect(rect, cornerRadius, cornerRadius);

    painter.fillPath(path, QBrush(m_backgroundColor));

    QPen pen;
    pen.setWidth(m_borderWidth);
    pen.setColor(m_borderColor);
    painter.strokePath(path, pen);
    
    // 绘制标题
    if (!m_titleName.isEmpty()) {
        painter.setPen(QPen(QColor(255, 255, 255))); // 白色文字
        QFont font = painter.font();
        font.setPointSize(12);
        font.setBold(true);
        painter.setFont(font);
        
        QRect titleRect(10, 5, width() - 35, 25); // 留出关闭按钮的空间
        painter.drawText(titleRect, Qt::AlignCenter | Qt::AlignVCenter, m_titleName);
    }
}
QString TAFloatingWidget::titleName() const
{
    return m_titleName;
}
void TAFloatingWidget::setBackgroundColor(const QColor& color)
{
    if (m_backgroundColor != color)
    {
        m_backgroundColor = color;
        update();
    }
}
void TAFloatingWidget::setRadius(int val)
{
    if (m_radius != val)
    {
        m_radius = val;
        update();
    }
}
void TAFloatingWidget::visibleCloseButton(bool val)
{
    m_visibleCloseButton = val;
}
void TAFloatingWidget::setBorderWidth(int val)
{
    if (m_borderWidth != val)
    {
        m_borderWidth = val;
        update();
    }
}
void TAFloatingWidget::setBorderColor(const QColor& color)
{
    if (m_borderColor != color)
    {
        m_borderColor = color;
        update();
    }
}
void TAFloatingWidget::setTitleName(const QString& name)
{
    if (m_titleName != name) {
        m_titleName = name;
        update(); // 触发重绘以显示新标题
    }
}
QColor TAFloatingWidget::backgroundColor() const
{
    return m_backgroundColor;
}