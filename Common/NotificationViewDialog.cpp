#include "NotificationViewDialog.h"
#include <QPainter>
#include <QDebug>
#include <QPixmap>
#include <QDateTime>
#include <QFontMetrics>

NotificationViewDialog::NotificationViewDialog(const QString& className, QWidget* parent)
    : QDialog(parent), m_dragging(false), m_className(className), 
      m_viewMode(FullMode), m_currentIndex(0), m_unreadCount(0)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(500, 600);
    
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
    titleBar->setStyleSheet("background-color: #3C3C3C; border-top-left-radius: 8px; border-top-right-radius: 8px;");
    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 15, 0);
    titleLayout->setSpacing(10);
    
    // 标题文本
    m_titleLabel = new QLabel(QString("通知 | %1").arg(className), titleBar);
    m_titleLabel->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 14px;"
        "font-weight: bold;"
        "background-color: #3C3C3C;"
        "border: 1px solid #3C3C3C;"
        "border-radius: 4px;"
        "padding: 5px;"
        "}"
    );
    m_titleLabel->setAlignment(Qt::AlignCenter);
    titleLayout->addWidget(m_titleLabel, 1);
    
    //// 未读通知数（黄色圆圈，红色数字）
    //m_unreadCountLabel = new QLabel("0", titleBar);
    //m_unreadCountLabel->setFixedSize(24, 24);
    //m_unreadCountLabel->setAlignment(Qt::AlignCenter);
    //m_unreadCountLabel->setStyleSheet(
    //    "QLabel {"
    //    "background-color: #FFD700;"
    //    "color: #FF0000;"
    //    "font-size: 12px;"
    //    "font-weight: bold;"
    //    "border-radius: 12px;"
    //    "}"
    //);
    //m_unreadCountLabel->setVisible(false);
    //titleLayout->addWidget(m_unreadCountLabel);
    
    // 模式切换按钮
    m_modeToggleButton = new QPushButton("完整", titleBar);
    m_modeToggleButton->setFixedSize(50, 24);
    m_modeToggleButton->setStyleSheet(
        "QPushButton {"
        "background-color: #3C3C3C;"
        "color: white;"
        "font-size: 12px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #4A4A4A;"
        "}"
    );
    connect(m_modeToggleButton, &QPushButton::clicked, this, &NotificationViewDialog::onModeToggleClicked);
    titleLayout->addWidget(m_modeToggleButton);

    // 关闭按钮（放在右上边）
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
    connect(m_closeButton, &QPushButton::clicked, this, &NotificationViewDialog::onCloseClicked);
    titleLayout->addWidget(m_closeButton);
    
    containerLayout->addWidget(titleBar);
    
    // 通知内容区域（可滚动）
    m_scrollArea = new QScrollArea(container);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    
    m_scrollContent = new QWidget;
    m_contentLayout = new QVBoxLayout(m_scrollContent);
    m_contentLayout->setSpacing(10);
    m_contentLayout->setContentsMargins(15, 15, 15, 15);
    m_contentLayout->addStretch();
    
    m_scrollArea->setWidget(m_scrollContent);
    containerLayout->addWidget(m_scrollArea, 1);
    
    // 底部导航栏（完整模式显示）
    m_navBar = new QWidget(container);
    m_navBar->setFixedHeight(50);
    m_navBar->setStyleSheet("background-color: #2D2D2D; border-bottom-left-radius: 8px; border-bottom-right-radius: 8px;");
    QHBoxLayout* navLayout = new QHBoxLayout(m_navBar);
    navLayout->setContentsMargins(20, 10, 20, 10);
    navLayout->setSpacing(15);
    
    m_prevButton = new QPushButton("上一条", m_navBar);
    m_prevButton->setFixedHeight(30);
    m_prevButton->setStyleSheet(
        "QPushButton {"
        "background-color: #3C3C3C;"
        "color: white;"
        "font-size: 12px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #4A4A4A;"
        "}"
        "QPushButton:disabled {"
        "background-color: #3C3C3C;"
        "color: #888888;"
        "}"
    );
    connect(m_prevButton, &QPushButton::clicked, this, &NotificationViewDialog::onPrevClicked);
    navLayout->addWidget(m_prevButton);
    
    navLayout->addStretch();
    
    m_nextButton = new QPushButton("下一条", m_navBar);
    m_nextButton->setFixedHeight(30);
    m_nextButton->setStyleSheet(
        "QPushButton {"
        "background-color: #3C3C3C;"
        "color: white;"
        "font-size: 12px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #4A4A4A;"
        "}"
        "QPushButton:disabled {"
        "background-color: #3C3C3C;"
        "color: #888888;"
        "}"
    );
    connect(m_nextButton, &QPushButton::clicked, this, &NotificationViewDialog::onNextClicked);
    navLayout->addWidget(m_nextButton);
    
    containerLayout->addWidget(m_navBar);
    
    mainLayout->addWidget(container);
    
    // 初始化为完整模式
    setViewMode(FullMode);
}

void NotificationViewDialog::setViewMode(ViewMode mode)
{
    m_viewMode = mode;
    
    // 更新模式切换按钮文本
    if (m_modeToggleButton) {
        if (mode == FullMode) {
            m_modeToggleButton->setText("完整");
        } else {
            m_modeToggleButton->setText("极简");
        }
    }
    
    // 显示/隐藏导航栏（完整模式显示，极简模式隐藏）
    if (m_navBar) {
        m_navBar->setVisible(mode == FullMode);
    }
    
    // 重置索引
    if (mode == SimpleMode) {
        m_currentIndex = 0;  // 极简模式显示最新的一条
    } else {
        m_currentIndex = 0;  // 完整模式从第一条开始
    }
    
    updateDisplay();
}

void NotificationViewDialog::addNotification(const QString& content, const QString& senderName, const QString& avatarUrl)
{
    NotificationItem item;
    item.content = content;
    item.senderName = senderName.isEmpty() ? "系统" : senderName;
    item.timestamp = QDateTime::currentDateTime();
    item.avatarUrl = avatarUrl;
    
    // 添加到列表开头（最新的在前面）
    m_notifications.prepend(item);
    
    // 最多保存100条
    if (m_notifications.size() > MAX_NOTIFICATIONS) {
        m_notifications.removeLast();
    }
    
    // 更新未读计数
    m_unreadCount++;
    setUnreadCount(m_unreadCount);
    
    // 更新显示（显示所有通知）
    updateDisplay();
}

void NotificationViewDialog::setUnreadCount(int count)
{
    //m_unreadCount = count;
    //if (m_unreadCountLabel) {
    //    if (count > 0) {
    //        m_unreadCountLabel->setText(QString::number(count));
    //        m_unreadCountLabel->setVisible(true);
    //    } else {
    //        m_unreadCountLabel->setVisible(false);
    //    }
    //}
}

void NotificationViewDialog::updateDisplay()
{
    // 清空现有内容
    QLayoutItem* item;
    while ((item = m_contentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    if (m_notifications.isEmpty()) {
        QLabel* emptyLabel = new QLabel("暂无通知", m_scrollContent);
        emptyLabel->setStyleSheet("color: #888888; font-size: 14px; padding: 20px;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        m_contentLayout->addWidget(emptyLabel);
        m_contentLayout->addStretch();
        return;
    }
    
    if (m_viewMode == SimpleMode) {
        // 极简模式：只显示最新的一条通知
        if (!m_notifications.isEmpty()) {
            createNotificationWidget(m_notifications.first(), m_scrollContent);
        }
    } else {
        // 完整模式：显示所有通知（从最新到最旧），可通过上一条/下一条按钮翻页查看历史
        // 显示所有通知
        for (const auto& notification : m_notifications) {
            createNotificationWidget(notification, m_scrollContent);
        }
        
        // 更新导航按钮状态（完整模式下显示所有通知，导航按钮可以用于滚动到特定位置）
        // 但根据图片，完整模式应该是显示所有通知，所以导航按钮可能不需要
        // 这里保留导航按钮，但可以设置为禁用或隐藏
        if (m_prevButton) {
            m_prevButton->setEnabled(false);  // 完整模式显示所有，不需要上一条
        }
        if (m_nextButton) {
            m_nextButton->setEnabled(false);  // 完整模式显示所有，不需要下一条
        }
    }
    
    m_contentLayout->addStretch();
}

void NotificationViewDialog::createNotificationWidget(const NotificationItem& item, QWidget* parent)
{
    // 通知项容器
    QWidget* itemWidget = new QWidget(parent);
    itemWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
    itemLayout->setContentsMargins(10, 10, 10, 10);
    itemLayout->setSpacing(10);
    
    // 头像（圆形）
    QLabel* avatarLabel = new QLabel(itemWidget);
    avatarLabel->setFixedSize(40, 40);
    avatarLabel->setStyleSheet(
        "QLabel {"
        "background-color: #4A4A4A;"
        "border-radius: 20px;"
        "border: 2px solid #666666;"
        "}"
    );
    // TODO: 如果有头像URL，可以加载头像图片
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setText("👤");  // 默认图标
    itemLayout->addWidget(avatarLabel);
    
    // 通知内容
    QLabel* contentLabel = new QLabel(item.content, itemWidget);
    contentLabel->setWordWrap(true);
    contentLabel->setStyleSheet(
        "QLabel {"
        "background-color: #3C3C3C;"
        "color: white;"
        "border: 1px solid #3C3C3C;"
        "border-radius: 6px;"
        "padding: 12px;"
        "font-size: 14px;"
        "min-height: 50px;"
        "}"
    );
    itemLayout->addWidget(contentLabel, 1);
    
    m_contentLayout->addWidget(itemWidget);
}

void NotificationViewDialog::onCloseClicked()
{
    reject();
}

void NotificationViewDialog::onAddClicked()
{
    // 添加按钮功能（可以扩展，比如添加新通知或设置）
    qDebug() << "添加按钮被点击";
}

void NotificationViewDialog::onModeToggleClicked()
{
    // 切换模式
    if (m_viewMode == FullMode) {
        setViewMode(SimpleMode);
    } else {
        setViewMode(FullMode);
    }
}

void NotificationViewDialog::onPrevClicked()
{
    if (m_currentIndex > 0) {
        m_currentIndex--;
        updateDisplay();
    }
}

void NotificationViewDialog::onNextClicked()
{
    if (m_currentIndex < m_notifications.size() - 1) {
        m_currentIndex++;
        updateDisplay();
    }
}

void NotificationViewDialog::paintEvent(QPaintEvent* event)
{
    QDialog::paintEvent(event);
}

void NotificationViewDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QDialog::mousePressEvent(event);
}

void NotificationViewDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

void NotificationViewDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
    QDialog::mouseReleaseEvent(event);
}

