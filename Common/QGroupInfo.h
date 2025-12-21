#pragma once

#include <QApplication>
#include <QDialog>

class MemberKickDialog; // 前向声明
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QSpacerItem>
#include <QDebug>
#include <QCheckBox>
#include <QGroupBox>
#include <QMenu>
#include <QAction>
#include <QCheckBox>
#include <QMouseEvent>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QResizeEvent>
#include <QEvent>
#include <QIcon>
#include <QPainter>
#include <QPropertyAnimation>
#include "CommonInfo.h"
#include "CourseDialog.h"
#include "ImSDK/includes/TIMCloud.h"
#include "ImSDK/includes/TIMCloudDef.h"
#include "ImSDK/includes/TIMCloudCallback.h"
#include "TIMRestAPI.h"
#include "GenerateTestUserSig.h"

class ClassTeacherDialog;
class ClassTeacherDelDialog;
class FriendSelectDialog;

// 自定义Toggle Switch控件（自绘）
class ToggleSwitch : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
public:
    explicit ToggleSwitch(QWidget* parent = nullptr);
    bool isChecked() const { return m_checked; }
    void setChecked(bool checked);

signals:
    void toggled(bool checked);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    bool m_checked;
    QColor m_onColor;
    QColor m_offColor;
    QColor m_thumbColor;
    int m_thumbRadius;
    int m_trackHeight;
    QPoint m_thumbPosition;
    void updateThumbPosition();
};

// 自定义设置行控件（蓝色标签 + Toggle Switch）
class SettingRow : public QWidget {
    Q_OBJECT
public:
    explicit SettingRow(const QString& labelText, QWidget* parent = nullptr);
    bool isChecked() const { return m_toggle->isChecked(); }
    void setChecked(bool checked) { m_toggle->setChecked(checked); }
    void setHighlighted(bool highlighted); // 设置是否显示红色边框

signals:
    void toggled(bool checked);

private:
    QLabel* m_label;
    ToggleSwitch* m_toggle;
    bool m_highlighted;
    
protected:
    void paintEvent(QPaintEvent* event) override;
};
class FriendButton : public QPushButton {
    Q_OBJECT
public:
    FriendButton(const QString& text = "", QWidget* parent = nullptr) : QPushButton(text, parent) {
        setFixedSize(50, 50);
        setStyleSheet("background-color:blue; border-radius:25px; color:white; font-weight:bold;");
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &FriendButton::customContextMenuRequested, this, &FriendButton::showContextMenu);
        m_memberRole = "成员"; // 默认为普通成员
    }

    void setMemberRole(const QString& role) {
        m_memberRole = role;
    }

    QString getMemberRole() const {
        return m_memberRole;
    }

    void setContextMenuEnabled(bool enabled) {
        m_contextMenuEnabled = enabled;
    }

signals:
    void setLeaderRequested();
    void cancelLeaderRequested();

private slots:
    void showContextMenu(const QPoint& pos) {
        // 如果右键菜单被禁用，不显示菜单
        if (!m_contextMenuEnabled) {
            return;
        }

        QMenu menu;
        QAction* actSet = menu.addAction("设为班主任");
        QAction* actCancel = menu.addAction("取消班主任");

        // 根据成员角色设置菜单项状态
        // 如果已经是管理员，则"设为班主任"灰化
        if (m_memberRole == "管理员") {
            actSet->setEnabled(false);
        }
        // 如果是普通成员，则"取消班主任"灰化
        else if (m_memberRole == "成员") {
            actCancel->setEnabled(false);
        }
        // 如果是群主，两个菜单项都灰化（群主不能修改自己的角色）
        else if (m_memberRole == "群主") {
            actSet->setEnabled(false);
            actCancel->setEnabled(false);
        }

        QAction* chosen = menu.exec(mapToGlobal(pos));
        if (chosen == actSet) {
            emit setLeaderRequested();
        }
        else if (chosen == actCancel) {
            emit cancelLeaderRequested();
        }
    }

private:
    QString m_memberRole; // 成员角色：群主、管理员、成员
    bool m_contextMenuEnabled = true; // 是否启用右键菜单，默认启用
};

class QGroupInfo : public QDialog {
    Q_OBJECT
public:
    QGroupInfo(QWidget* parent = nullptr);
    ~QGroupInfo();

public:
    void initData(QString groupName, QString groupNumberId, QString classid = "");
    void InitGroupMember(QString group_id, QVector<GroupMemberInfo> groupMemberInfo);
    void InitGroupMember();
    QVector<GroupMemberInfo> getGroupMemberInfo() const { return m_groupMemberInfo; } // 获取当前成员列表
    
    /**
     * @brief 使用REST API获取群成员列表
     * @param groupId 群组ID
     */
    void fetchGroupMemberListFromREST(const QString& groupId);
    
signals:
    void memberLeftGroup(const QString& groupId, const QString& leftUserId); // 成员退出群聊信号，传递退出的用户ID
    void groupDismissed(const QString& groupId); // 群聊解散信号，通知父窗口刷新群列表
    void membersRefreshed(const QString& groupId); // 成员列表需要刷新信号，通知父窗口刷新成员列表

private:
    void updateButtonStates(); // 根据当前用户角色更新按钮状态
    void onExitGroupClicked(); // 退出群聊按钮点击处理
    void onDismissGroupClicked(); // 解散群聊按钮点击处理
    void sendExitGroupRequestToServer(const QString& groupId, const QString& userId, const QString& leftUserId); // 发送退出群聊请求到服务器
    void sendDismissGroupRequestToServer(const QString& groupId, const QString& userId, void* callbackData = nullptr); // 发送解散群聊请求到服务器
    void refreshMemberList(const QString& groupId); // 刷新成员列表（通知父窗口刷新）
    void onSetLeaderRequested(const QString& memberId); // 设置管理员（设为班主任）
    void onCancelLeaderRequested(const QString& memberId); // 取消管理员（取消班主任）
    void sendSetAdminRoleRequestToServer(const QString& groupId, const QString& memberId, const QString& role); // 发送设置/取消管理员请求到自己的服务器
    void transferOwnerAndQuit(const QString& newOwnerId, const QString& newOwnerName); // 转让群主并退出群聊
    void sendTransferOwnerRequestToServer(const QString& groupId, const QString& oldOwnerId, const QString& newOwnerId); // 发送转让群主请求到自己的服务器

protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    TIMRestAPI* m_restAPI = NULL;
    QString m_groupName;
    QString m_groupNumberId;
    QVector<GroupMemberInfo> m_groupMemberInfo;
    QHBoxLayout* circlesLayout = NULL;
    FriendButton* m_circlePlus = nullptr; // + 按钮（添加成员）
    FriendButton* m_circleMinus = nullptr; // - 按钮（移除成员）
    FriendSelectDialog* m_friendSelectDlg = NULL;
    MemberKickDialog* m_memberKickDlg = NULL; // 踢出成员对话框
    //ClassTeacherDelDialog* m_classTeacherDelDlg = NULL;
    CourseDialog* m_courseDlg;
    QPushButton* m_btnDismiss = nullptr; // 解散群聊按钮
    QPushButton* m_btnExit = nullptr; // 退出群聊按钮
    QPushButton* m_closeButton = nullptr; // 关闭按钮
    bool m_dragging = false; // 是否正在拖动
    QPoint m_dragStartPos; // 拖动起始位置
};


