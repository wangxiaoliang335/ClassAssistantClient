#include "TextMessageDialog.h"
#include <QPainter>
#include <QDebug>

TextMessageDialog::TextMessageDialog(const QString& className, QWidget* parent)
    : QDialog(parent), m_dragging(false), m_className(className)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(500, 400);
    
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 主容器（深灰色背景，圆角）
    QWidget* container = new QWidget(this);
    container->setStyleSheet(
        "QWidget {"
        "background-color: #3C3C3C;"
        "border-radius: 8px;"
        "}"
    );
    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    
    // 标题栏
    QWidget* titleBar = new QWidget(container);
    titleBar->setFixedHeight(50);
    titleBar->setStyleSheet("background-color: #2D2D2D; border-top-left-radius: 8px; border-top-right-radius: 8px;");
    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 15, 0);
    titleLayout->setSpacing(10);
    
    // 关闭按钮
    m_closeButton = new QPushButton("✕", titleBar);
    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "background-color: transparent;"
        "color: white;"
        "font-size: 16px;"
        "font-weight: bold;"
        "border: none;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(255, 255, 255, 30);"
        "border-radius: 12px;"
        "}"
    );
    connect(m_closeButton, &QPushButton::clicked, this, &TextMessageDialog::onCancelClicked);
    titleLayout->addWidget(m_closeButton);
    
    // 标题文本
    m_titleLabel = new QLabel(QString("文本消息 | %1").arg(className), titleBar);
    m_titleLabel->setStyleSheet("color: white; font-size: 14px; font-weight: bold;");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    titleLayout->addWidget(m_titleLabel, 1);
    
    // 未读消息数（红色圆圈）
    m_unreadCountLabel = new QLabel("1", titleBar);
    m_unreadCountLabel->setFixedSize(24, 24);
    m_unreadCountLabel->setAlignment(Qt::AlignCenter);
    m_unreadCountLabel->setStyleSheet(
        "QLabel {"
        "background-color: #FF4444;"
        "color: white;"
        "font-size: 12px;"
        "font-weight: bold;"
        "border-radius: 12px;"
        "}"
    );
    titleLayout->addWidget(m_unreadCountLabel);
    
    containerLayout->addWidget(titleBar);
    
    // 文本输入区域
    m_textEdit = new QTextEdit(container);
    m_textEdit->setPlaceholderText("请输入需要发送的文本消息");
    m_textEdit->setStyleSheet(
        "QTextEdit {"
        "background-color: #2D2D2D;"
        "color: #CCCCCC;"
        "border: none;"
        "padding: 15px;"
        "font-size: 14px;"
        "border-radius: 0px;"
        "}"
    );
    containerLayout->addWidget(m_textEdit, 1);
    
    // 底部按钮栏
    QWidget* buttonBar = new QWidget(container);
    buttonBar->setFixedHeight(60);
    buttonBar->setStyleSheet("background-color: #2D2D2D; border-bottom-left-radius: 8px; border-bottom-right-radius: 8px;");
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonBar);
    buttonLayout->setContentsMargins(20, 10, 20, 10);
    buttonLayout->setSpacing(15);
    
    // 取消按钮
    m_cancelButton = new QPushButton("取消", buttonBar);
    m_cancelButton->setFixedHeight(36);
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "background-color: #4A4A4A;"
        "color: white;"
        "font-size: 14px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #5A5A5A;"
        "}"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &TextMessageDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);
    
    // 发送按钮
    m_sendButton = new QPushButton("发送", buttonBar);
    m_sendButton->setFixedHeight(36);
    m_sendButton->setStyleSheet(
        "QPushButton {"
        "background-color: #4169E1;"
        "color: white;"
        "font-size: 14px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #5B7AE1;"
        "}"
    );
    connect(m_sendButton, &QPushButton::clicked, this, &TextMessageDialog::onSendClicked);
    buttonLayout->addWidget(m_sendButton);
    
    containerLayout->addWidget(buttonBar);
    
    mainLayout->addWidget(container);
}

QString TextMessageDialog::getMessage() const
{
    return m_textEdit ? m_textEdit->toPlainText() : QString();
}

void TextMessageDialog::setUnreadCount(int count)
{
    if (m_unreadCountLabel) {
        if (count > 0) {
            m_unreadCountLabel->setText(QString::number(count));
            m_unreadCountLabel->setVisible(true);
        } else {
            m_unreadCountLabel->setVisible(false);
        }
    }
}

void TextMessageDialog::clearMessage()
{
    if (m_textEdit) {
        m_textEdit->clear();
    }
}

void TextMessageDialog::onSendClicked()
{
    QString message = getMessage().trimmed();
    if (!message.isEmpty()) {
        accept();
    } else {
        // 如果消息为空，可以显示提示或直接关闭
        reject();
    }
}

void TextMessageDialog::onCancelClicked()
{
    reject();
}

void TextMessageDialog::paintEvent(QPaintEvent* event)
{
    QDialog::paintEvent(event);
}

void TextMessageDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QDialog::mousePressEvent(event);
}

void TextMessageDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

void TextMessageDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
    QDialog::mouseReleaseEvent(event);
}

