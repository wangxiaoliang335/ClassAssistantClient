#pragma execution_character_set("utf-8")
#include "TACWarningDialog.h"
#include <QApplication>
#include <QDesktopWidget>

// 使用与 TACSubjectEditDialog 一致的深色背景色
static const QColor DIALOG_BACKGROUND_COLOR(40, 40, 40, 240);

TACWarningDialog::TACWarningDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(350, 180);
    
    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 50, 20, 20);
    mainLayout->setSpacing(15);
    
    // 创建关闭按钮
    m_closeButton = new QPushButton(this);
    m_closeButton->setText(QString::fromUtf8(u8"×"));
    m_closeButton->setFixedSize(23, 23);
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "color: white;"
        "background-color: rgba(255,255,255,20);"
        "border: 1px solid rgba(50,50,50,200);"
        "border-radius: 11px;"
        "font-size: 16px;"
        "font-weight: 700;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(255,255,255,35);"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(255,255,255,18);"
        "}"
    );
    m_closeButton->hide();
    m_closeButton->raise();
    connect(m_closeButton, &QPushButton::clicked, this, &TACWarningDialog::onOkClicked);
    
    // 标题标签
    m_titleLabel = new QLabel(this);
    m_titleLabel->setText(QString::fromUtf8(u8"提示"));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold; background: transparent;");
    mainLayout->addWidget(m_titleLabel);
    
    // 消息标签
    m_messageLabel = new QLabel(this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setStyleSheet("color: white; font-size: 14px; background: transparent;");
    mainLayout->addWidget(m_messageLabel);
    
    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->addStretch();
    
    m_okButton = new QPushButton(this);
    m_okButton->setText(QString::fromUtf8(u8"确定"));
    m_okButton->setFixedHeight(35);
    m_okButton->setFixedWidth(100);
    m_okButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(0,120,212,255);"
        "color: white;"
        "border: 1px solid rgba(0,120,212,255);"
        "border-radius: 8px;"
        "font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(0,140,232,255);"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(0,100,192,255);"
        "}"
    );
    connect(m_okButton, &QPushButton::clicked, this, &TACWarningDialog::onOkClicked);
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    updateCloseButtonPos();
}

TACWarningDialog::~TACWarningDialog()
{
}

void TACWarningDialog::setTitle(const QString& title)
{
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

void TACWarningDialog::setMessage(const QString& message)
{
    if (m_messageLabel) {
        m_messageLabel->setText(message);
    }
}

int TACWarningDialog::warning(QWidget* parent, const QString& title, const QString& message)
{
    TACWarningDialog dialog(parent);
    dialog.setTitle(title);
    dialog.setMessage(message);
    
    // 定位到父窗口中心
    if (parent) {
        QPoint parentCenter = parent->mapToGlobal(parent->rect().center());
        dialog.move(parentCenter.x() - dialog.width() / 2, parentCenter.y() - dialog.height() / 2);
    } else {
        QRect screenGeometry = QApplication::desktop()->availableGeometry();
        dialog.move(screenGeometry.center().x() - dialog.width() / 2, 
                   screenGeometry.center().y() - dialog.height() / 2);
    }
    
    return dialog.exec();
}

void TACWarningDialog::onOkClicked()
{
    accept();
}

void TACWarningDialog::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // 绘制圆角背景
    QPainterPath path;
    path.addRoundedRect(rect(), 10, 10);
    
    // 深色半透明背景（与主窗口一致）
    p.fillPath(path, DIALOG_BACKGROUND_COLOR);
    
    // 边框
    QPen pen(QColor(60, 60, 60, 255));
    pen.setWidth(1);
    p.setPen(pen);
    p.drawPath(path);
}

void TACWarningDialog::enterEvent(QEvent* event)
{
    if (m_closeButton) {
        m_closeButton->show();
    }
    QDialog::enterEvent(event);
}

void TACWarningDialog::leaveEvent(QEvent* event)
{
    QPoint globalPos = QCursor::pos();
    QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
    if (!widgetRect.contains(globalPos) && m_closeButton) {
        QRect btnRect = QRect(m_closeButton->mapToGlobal(QPoint(0, 0)), m_closeButton->size());
        if (!btnRect.contains(globalPos)) {
            m_closeButton->hide();
        }
    }
    QDialog::leaveEvent(event);
}

void TACWarningDialog::resizeEvent(QResizeEvent* event)
{
    updateCloseButtonPos();
    QDialog::resizeEvent(event);
}

void TACWarningDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        m_dragging = true;
        event->accept();
    }
}

void TACWarningDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
    }
}

void TACWarningDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}

void TACWarningDialog::updateCloseButtonPos()
{
    if (m_closeButton) {
        const int marginRight = 8;
        const int marginTop = 8;
        m_closeButton->move(width() - m_closeButton->width() - marginRight, marginTop);
    }
}

