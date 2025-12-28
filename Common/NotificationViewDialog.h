#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QWidget>
#include <QMouseEvent>
#include <QPoint>
#include <QList>
#include <QDateTime>

// 通知数据结构
struct NotificationItem {
    QString content;          // 通知内容
    QString senderName;       // 发送者名称
    QDateTime timestamp;      // 时间戳
    QString avatarUrl;        // 头像URL（可选）
};

class NotificationViewDialog : public QDialog
{
    Q_OBJECT

public:
    enum ViewMode {
        FullMode = 2,    // 完整模式：可上下翻查看历史
        SimpleMode = 3   // 极简模式：只显示最新通知
    };

    explicit NotificationViewDialog(const QString& className, QWidget* parent = nullptr);
    
    // 设置显示模式
    void setViewMode(ViewMode mode);
    
    // 添加通知（最多保存100条）
    void addNotification(const QString& content, const QString& senderName = QString(), const QString& avatarUrl = QString());
    
    // 设置未读通知数
    void setUnreadCount(int count);
    
    // 获取未读通知数
    int getUnreadCount() const { return m_unreadCount; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onCloseClicked();
    void onAddClicked();
    void onModeToggleClicked();  // 切换模式

private:
    void updateDisplay();
    void createNotificationWidget(const NotificationItem& item, QWidget* parent);
    
    QLabel* m_titleLabel;
    QLabel* m_unreadCountLabel;
    QPushButton* m_closeButton;
    QPushButton* m_addButton;
    QPushButton* m_modeToggleButton;  // 模式切换按钮
    
    QScrollArea* m_scrollArea;
    QWidget* m_scrollContent;
    QVBoxLayout* m_contentLayout;
    
    QList<NotificationItem> m_notifications;  // 通知列表（最多100条）
    ViewMode m_viewMode;
    int m_currentIndex;  // 当前显示的通知索引（完整模式）
    int m_unreadCount;
    
    QPoint m_dragPosition;
    bool m_dragging;
    
    QString m_className;
    
    static const int MAX_NOTIFICATIONS = 100;  // 最多保存100条通知
};

