#pragma once

#include <QApplication>
#include <QDialog>
#include <QStackedWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QFrame>
#include <QMouseEvent>
#include <QStyle>
#include <QDir>
#include <QPixmap>
#include <qpainterpath.h>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>
#include <QSet>
#include <QTreeWidget>
#include <QHash>
#include "FriendNotifyDialog.h"
#include "TACAddGroupWidget.h"
#include "GroupNotifyDialog.h"
#include "NotificationViewDialog.h"
#include "TAHttpHandler.h"
#include "ImSDK/includes/TIMCloud.h"
#include "CommonInfo.h"
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class ScheduleDialog;
class ChatDialog;

// 前向声明 HomeworkItem（定义在 HomeworkViewDialog.h 中）
struct HomeworkItem;

class RowItem : public QFrame {
    Q_OBJECT
public:
    explicit RowItem(const QString& text, QWidget* parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* e) override;
};

class FriendGroupDialog : public QDialog
{
    Q_OBJECT
public:
    FriendGroupDialog(QWidget* parent, TaQTWebSocket* pWs);

    // 辅助函数：为 ScheduleDialog 建立群聊退出信号连接
    void connectGroupLeftSignal(ScheduleDialog* scheduleDlg, const QString& groupId);
    // 辅助函数：为普通群 ChatDialog 建立退出/解散信号连接
    void connectNormalGroupLeftSignal(ChatDialog* chatDlg, const QString& groupId);

    void InitData();

    void GetGroupJoinedList(); // 已加入群列表

    void InitWebSocket();
    
    // 打开指定的班级群窗口
    void openClassGroup(const QString& classId, const QString& groupName = "");
    
    // 确保指定班级ID的ScheduleDialog已创建（如果不存在则创建，但不显示）
    void ensureScheduleDialogCreated(const QString& classId);
    
    // 获取指定班级ID对应的ScheduleDialog实例
    ScheduleDialog* getScheduleDialog(const QString& classId) const;

    void setTitleName(const QString& name);

    void visibleCloseButton(bool val);

    void setBackgroundColor(const QColor& color);

    void setBorderColor(const QColor& color);

    void setBorderWidth(int val);

    void setRadius(int val);

protected:
    void paintEvent(QPaintEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void leaveEvent(QEvent* event) override;

    void enterEvent(QEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

public:
    TACAddGroupWidget1* addGroupWidget = NULL;
    FriendNotifyDialog* friendNotifyDlg = NULL;
    GroupNotifyDialog* grpNotifyDlg = NULL;

private slots:
    void onWebSocketMessage(const QString& msg);
    void notifyUpdatePrepareClassButton(); // 通知更新课前准备按钮可见性
    void notifyUpdateHomeworkButton(); // 通知更新家庭作业按钮可见性
    void notifyUpdateTodayScheduleButton(); // 通知更新今日课表按钮可见性

private:
    bool m_dragging;
    QPoint m_dragStartPos;
    QString m_titleName;
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth;
    int m_radius;
    bool m_visibleCloseButton;
    QPushButton* closeButton = NULL;
    QLabel* pLabel = NULL;
    TAHttpHandler* m_httpHandler = NULL;
    QNetworkAccessManager* m_networkManager = NULL;
    QVBoxLayout* fLayout = NULL;
    QVBoxLayout* gLayout = NULL;
    TaQTWebSocket* m_pWs = NULL;
    QMap<QString, ScheduleDialog*> m_scheduleDlg;
    QMap<QString, ChatDialog*> m_normalGroupChatDlg;
    QList<Notification> notifications;
    QSet<QString> m_setClassId;

    QTreeWidget* m_friendTree = nullptr;
    QTreeWidgetItem* m_classRootItem = nullptr;
    QTreeWidgetItem* m_teacherRootItem = nullptr;
    QHash<QString, QTreeWidgetItem*> m_classItemMap;
    QHash<QString, QTreeWidgetItem*> m_teacherItemMap;

    QTreeWidget* m_groupTree = nullptr;
    QTreeWidgetItem* m_classGroupRoot = nullptr;
    QTreeWidgetItem* m_classManagedRoot = nullptr;
    QTreeWidgetItem* m_classJoinedRoot = nullptr;
    QTreeWidgetItem* m_normalGroupRoot = nullptr;
    QTreeWidgetItem* m_normalManagedRoot = nullptr;
    QTreeWidgetItem* m_normalJoinedRoot = nullptr;
    QHash<QString, QTreeWidgetItem*> m_groupItemMap;
    QHash<QString, QJsonArray> m_prepareClassHistoryCache;
    // 作业缓存：按群组ID和日期聚合 (group_id -> date(yyyy-MM-dd) -> QList<HomeworkItem>)
    // 每条作业包含科目、内容和创建时间，支持同一天同一科目的多条作业
    QMap<QString, QMap<QString, QList<HomeworkItem>>> m_homeworkCache;
    // 通知缓存：按群组ID聚合 (group_id -> NotificationItem列表)
    QMap<QString, QList<NotificationItem>> m_notificationCache;

    void setupFriendTree();
    void clearFriendTree();
    void updateFriendCounts();
    void addClassNode(const QString& displayName, const QString& groupId, const QString& classid, bool iGroupOwner, bool isClassGroup);
    void addTeacherNode(const QString& displayName, const QString& subtitle, const QString& avatarPath, const QString& teacherId);
    void handleFriendItemActivated(QTreeWidgetItem* item);
    void setupGroupTree();
    void clearGroupTree();
    void updateGroupCounts();
    void addGroupTreeNode(const QString& displayName, const QString& groupId, const QString& classid, bool iGroupOwner, bool isClassGroup);
    void handleGroupItemActivated(QTreeWidgetItem* item);
    void openScheduleForGroup(const QString& groupName, const QString& unique_group_id, const QString& classid, bool iGroupOwner, bool isClassGroup);
    void processPrepareClassHistoryMessage(const QJsonObject& rootObj);
    void fetchClassesByPrefix(const QString& schoolId);
    // 下载群组头像并保存到本地
    void downloadGroupAvatar(const QString& faceUrl, const QString& groupId);
};