#include "ScheduleDialog.h"
#include "NotificationViewDialog.h"
#include <QDebug>
#include "CommonInfo.h"
#include "TaQTWebSocket.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QScreen>
#include <QApplication>
#include <algorithm>

// 显示通知接收窗口（班级端）
void ScheduleDialog::showNotificationViewDialog()
{
    if (!notificationViewDlg) {
        // 获取班级名称，用于显示在标题中
        QString className = m_groupName;
        if (className.isEmpty()) {
            // 如果没有群组名称，尝试从班级ID获取
            className = QString("班级%1").arg(m_classid);
        }
        
        notificationViewDlg = new NotificationViewDialog(className, this);
        
        // 默认使用完整模式（可以后续根据设置切换）
        notificationViewDlg->setViewMode(NotificationViewDialog::FullMode);
        
        // 从缓存中加载所有通知到窗口
        // 缓存中最新的是第一个（索引0），addNotification 使用 prepend 把新通知添加到开头
        // 所以我们需要从最新到最旧添加（从索引0开始），这样最新的会在最前面
        for (const auto& item : m_notificationCache) {
            notificationViewDlg->addNotification(item.content, item.senderName, item.avatarUrl);
        }
        
        qDebug() << "通知窗口已创建，从缓存加载了" << m_notificationCache.size() << "条通知";
    }
    
    notificationViewDlg->show();
    notificationViewDlg->raise();
    notificationViewDlg->activateWindow();
    
    // 居中显示
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - notificationViewDlg->width()) / 2;
    int y = (screenGeometry.height() - notificationViewDlg->height()) / 2;
    notificationViewDlg->move(x, y);
}

// 添加通知（从WebSocket消息中接收）
void ScheduleDialog::addNotification(const QString& content, const QString& senderName, const QString& avatarUrl)
{
    // 创建通知项并缓存
    NotificationItem item;
    item.content = content;
    item.senderName = senderName.isEmpty() ? "系统" : senderName;
    item.timestamp = QDateTime::currentDateTime();
    item.avatarUrl = avatarUrl;
    
    // 添加到缓存列表开头（最新的在前面）
    m_notificationCache.prepend(item);
    
    // 最多保存100条
    if (m_notificationCache.size() > 100) {
        m_notificationCache.removeLast();
    }
    
    // 如果窗口已创建，直接添加到窗口
    if (notificationViewDlg) {
        notificationViewDlg->addNotification(content, senderName, avatarUrl);
    }
}

// 设置通知数据（从 FriendGroupDialog 传递）
void ScheduleDialog::setNotificationData(const QList<NotificationItem>& notifications)
{
    m_notificationCache = notifications;
    qDebug() << "ScheduleDialog: 已设置通知数据，数量:" << m_notificationCache.size();
    
    // 如果窗口已创建，删除旧窗口，下次显示时会重新创建并加载缓存
    if (notificationViewDlg) {
        notificationViewDlg->deleteLater();
        notificationViewDlg = nullptr;
    }
}

