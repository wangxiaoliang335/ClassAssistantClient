#include "RandomCallMessageDialog.h"
#include <QPainter>
#include <QApplication>
#include <QDesktopWidget>

RandomCallMessageDialog::RandomCallMessageDialog(QWidget* parent)
    : QDialog(parent)
    , m_titleBarColor(85, 85, 85)  // RGB(85, 85, 85)
    , m_backgroundColor(85, 85, 85)  // RGB(85, 85, 85)
    , m_dragging(false)
{
    // 设置无边框窗口
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose);
    
    resize(400, 200);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 标题栏
    QWidget* titleBar = new QWidget(this);
    titleBar->setObjectName("titleBar");  // 设置对象名称以便查找
    titleBar->setFixedHeight(TITLE_BAR_HEIGHT);
    titleBar->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
                           .arg(m_titleBarColor.red())
                           .arg(m_titleBarColor.green())
                           .arg(m_titleBarColor.blue()));
    
    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 10, 0);
    titleLayout->setSpacing(10);
    
    m_titleLabel = new QLabel("提示", titleBar);
    m_titleLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold; background: transparent;");
    titleLayout->addWidget(m_titleLabel);
    
    titleLayout->addStretch();
    
    // 关闭按钮
    m_closeButton = new QPushButton("✕", titleBar);
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "background-color: #666666;"
        "color: white;"
        "font-size: 18px;"
        "font-weight: bold;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #777777;"
        "}"
    );
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    titleLayout->addWidget(m_closeButton);
    
    mainLayout->addWidget(titleBar);
    
    // 内容区域
    QWidget* contentWidget = new QWidget(this);
    contentWidget->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
                                 .arg(m_backgroundColor.red())
                                 .arg(m_backgroundColor.green())
                                 .arg(m_backgroundColor.blue()));
    
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(20);
    
    // 消息标签
    m_messageLabel = new QLabel("", contentWidget);
    m_messageLabel->setStyleSheet("color: white; font-size: 14px; background: transparent;");
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(m_messageLabel);
    
    // 确定按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    
    m_okButton = new QPushButton("确定", contentWidget);
    m_okButton->setFixedSize(100, 35);
    m_okButton->setStyleSheet(
        "QPushButton {"
        "background-color: green;"
        "color: white;"
        "font-size: 14px;"
        "font-weight: bold;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #00cc00;"
        "}"
    );
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_okButton);
    
    buttonLayout->addStretch();
    contentLayout->addLayout(buttonLayout);
    
    mainLayout->addWidget(contentWidget);
    
    // 设置整体样式
    setStyleSheet(QString("QDialog { background-color: rgb(%1, %2, %3); }")
                 .arg(m_backgroundColor.red())
                 .arg(m_backgroundColor.green())
                 .arg(m_backgroundColor.blue()));
}

void RandomCallMessageDialog::setTitle(const QString& title)
{
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

void RandomCallMessageDialog::setMessage(const QString& message)
{
    if (m_messageLabel) {
        m_messageLabel->setText(message);
    }
}

void RandomCallMessageDialog::setTitleBarColor(const QColor& color)
{
    m_titleBarColor = color;
    // 更新标题栏样式 - 通过对象名称查找
    QWidget* titleBar = findChild<QWidget*>("titleBar");
    if (titleBar) {
        titleBar->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
                               .arg(color.red())
                               .arg(color.green())
                               .arg(color.blue()));
    }
    update();
}

void RandomCallMessageDialog::setBackgroundColor(const QColor& color)
{
    m_backgroundColor = color;
    setStyleSheet(QString("QDialog { background-color: rgb(%1, %2, %3); }")
                 .arg(color.red())
                 .arg(color.green())
                 .arg(color.blue()));
    update();
}

void RandomCallMessageDialog::paintEvent(QPaintEvent* event)
{
    QDialog::paintEvent(event);
    
    // 绘制标题栏
    QPainter painter(this);
    painter.fillRect(0, 0, width(), TITLE_BAR_HEIGHT, m_titleBarColor);
}

void RandomCallMessageDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 检查是否点击在标题栏区域
        if (event->pos().y() <= TITLE_BAR_HEIGHT) {
            m_dragging = true;
            m_dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    QDialog::mousePressEvent(event);
}

void RandomCallMessageDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

