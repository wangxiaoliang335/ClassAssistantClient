#pragma once

#include <QApplication>
#include <QDialog>

class MemberKickDialog; // 前向声明
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
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
#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QResizeEvent>
#include <QEvent>
#include <QIcon>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QFont>
#include <QFontMetrics>
#include "CommonInfo.h"
#include "CourseDialog.h"
#include "ImSDK/includes/TIMCloud.h"
#include "ImSDK/includes/TIMCloudDef.h"
#include "ImSDK/includes/TIMCloudCallback.h"
#include "TIMRestAPI.h"
#include "GenerateTestUserSig.h"
#include <QPointer>

class ClassTeacherDialog;
class ClassTeacherDelDialog;
class FriendSelectDialog;

// 开启对讲控件（自绘）
class IntercomControlWidget : public QWidget {
    Q_OBJECT
public:
    explicit IntercomControlWidget(QWidget* parent = nullptr);
    ~IntercomControlWidget();
    
    // 设置开关状态
    void setIntercomEnabled(bool enabled);
    bool isIntercomEnabled() const { return m_enabled; }
    
signals:
    void intercomToggled(bool enabled); // 开关状态改变信号
    void buttonClicked(); // 按钮点击信号
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    
private:
    bool m_enabled; // 开关状态
    bool m_buttonPressed; // 按钮是否被按下
    
    // 绘制辅助函数
    void drawBackground(QPainter& painter);
    void drawButton(QPainter& painter);
    void drawToggleSwitch(QPainter& painter);
    
    // 计算区域
    QRect getButtonRect() const;
    QRect getSwitchRect() const;
};

// 简单开关控件（用于班级端设置）
class SimpleToggleSwitch : public QWidget {
    Q_OBJECT
public:
    explicit SimpleToggleSwitch(QWidget* parent = nullptr);
    
    void setChecked(bool checked);
    bool isChecked() const { return m_checked; }
    
signals:
    void toggled(bool checked);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    
private:
    bool m_checked;
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
    // iGroupOwner: 当前用户在该群里是否为群主（由外部传入，QGroupInfo 内部统一以此判断权限）
    void initData(QString groupName, QString groupNumberId, bool iGroupOwner, QString classid = "");
    void InitGroupMember(QString group_id, QVector<GroupMemberInfo> groupMemberInfo);
    void InitGroupMember();
    QVector<GroupMemberInfo> getGroupMemberInfo() const { return m_groupMemberInfo; } // 获取当前成员列表
    bool isLinkTodayScheduleEnabled() const; // 获取"关联今日课表"开关状态
    bool isLinkPreClassPreparationEnabled() const; // 获取"关联课前准备"开关状态
    bool isLinkHomeworkEnabled() const; // 获取"关联家庭作业"开关状态
    bool isLinkDutyRosterEnabled() const; // 获取"关联值日表"开关状态
    
    // 设置群组设置字段（从服务器获取后更新UI）
    void setGroupSettings(int receiveNotification, int linkTodaySchedule, int enableIntercom, 
                         int linkHomework, int linkPreClassPreparation, int linkDutyRoster = 0);
    
    /**
     * @brief 使用REST API获取群成员列表
     * @param groupId 群组ID
     */
    void fetchGroupMemberListFromREST(const QString& groupId);

    /**
     * @brief 使用腾讯 IM SDK 获取群成员列表（无需管理员REST鉴权，适合普通群）
     * @param groupId 群组ID
     */
    void fetchGroupMemberListFromSDK(const QString& groupId);
    
signals:
    void memberLeftGroup(const QString& groupId, const QString& leftUserId); // 成员退出群聊信号，传递退出的用户ID
    void groupDismissed(const QString& groupId); // 群聊解散信号，通知父窗口刷新群列表
    void membersRefreshed(const QString& groupId); // 成员列表需要刷新信号，通知父窗口刷新成员列表
    void groupSettingChanged(); // 群组设置改变信号，通知父窗口更新相关UI（如功能键栏）

private:
    // 普通群成员区（网格布局：头像在上，名字在下）
    QWidget* m_memberGridContainer = nullptr;
    QScrollArea* m_memberScrollArea = nullptr;
    QGridLayout* m_memberGridLayout = nullptr;

    void renderNormalGroupMemberGrid(); // 普通群：渲染成员网格
    QWidget* makeMemberTile(const QString& topText, const QString& bottomText, bool isActionTile);
    void updateButtonStates(); // 根据当前用户角色更新按钮状态
    void onExitGroupClicked(); // 退出群聊按钮点击处理
    void onDismissGroupClicked(); // 解散群聊按钮点击处理
    void sendExitGroupRequestToServer(const QString& groupId, const QString& userId, const QString& leftUserId); // 发送退出群聊请求到服务器
    void sendDismissGroupRequestToServer(const QString& groupId, const QString& userId); // 发送解散群聊请求到服务器
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
    void done(int r) override; // 拦截所有关闭路径（包括 ESC / Alt+F4 / reject()）

    TIMRestAPI* m_restAPI = NULL;
    QNetworkAccessManager* m_networkManager = nullptr; // 用于HTTP请求
    QString m_groupName;
    QString m_groupNumberId;
    QString m_classId;
    bool m_isNormalGroup = false; // classid为空时，按“普通群群管理”模式展示
    bool m_iGroupOwner = false;   // 当前用户是否为群主（外部传入）
    bool m_hideTeachSubjects = false; // 班级端：不展示“任教科目”区域
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
    IntercomControlWidget* m_intercomWidget = nullptr; // 对讲控件
    // 任教科目：tag/chip 形式展示（可增删）
    QWidget* m_subjectTagContainer = nullptr;
    QHBoxLayout* m_subjectTagLayout = nullptr; // tag 容器布局（末尾固定“+ 添加”按钮）
    QPushButton* m_addSubjectBtn = nullptr;    // “+ 添加”按钮
    bool m_dragging = false; // 是否正在拖动
    QPoint m_dragStartPos; // 拖动起始位置
    bool m_initialized = false; // 是否已经初始化

    // 上一次真正渲染到 UI 的成员列表快照（用于避免重复渲染/重复刷新）
    // 注意：不要用 m_groupMemberInfo 自己来和入参比较，否则当调用方传入 m_groupMemberInfo 本体时会“永远相同”导致 UI 不刷新
    bool m_hasRenderedGroupMembers = false;
    QString m_lastRenderedGroupId;
    QVector<GroupMemberInfo> m_lastRenderedGroupMemberInfo;

    // 普通群模式下的设置区
    QLineEdit* m_editGroupName = nullptr;     // 群聊名称（默认只读显示）
    QCheckBox* m_chkReceiveNotify = nullptr;  // 接收通知开关（先做UI，后续可接入后端）

    // 班级端（截图样式）开关区
    QPointer<SimpleToggleSwitch> m_swReceiveNotify;
    QPointer<SimpleToggleSwitch> m_swLinkTodaySchedule;
    QPointer<SimpleToggleSwitch> m_swEnableIntercom;
    QPointer<SimpleToggleSwitch> m_swLinkHomework;
    QPointer<SimpleToggleSwitch> m_swLinkPreClass;
    QPointer<SimpleToggleSwitch> m_swLinkDutyRoster;
    
    // 班级端头像标签
    QLabel* m_lblAvatar = nullptr;
    
    // 根据当前用户的 is_voice_enabled 更新对讲开关状态
    void updateIntercomState();
    
    // 更新群组设置到服务器
    void updateGroupSetting(const QString& settingName, int value);
    
    // 加载头像图片
    void loadAvatarImage();

    bool validateSubjectFormat(bool showMessage = true) const; // 校验任教科目列表（至少1个非空）
    QStringList collectTeachSubjects() const; // 收集当前 UI 中的任教科目（tag）
    void postTeachSubjectsAndThenClose(int doneCode); // 调用 /groups/member/teach-subjects 后再关闭窗口

    bool m_savingTeachSubjects = false; // 是否正在提交任教科目（避免重复提交）
    bool m_subjectsDirty = false; // UI中任教科目是否被用户编辑过（避免后台刷新覆盖本地编辑）

    QWidget* makeSubjectTagWidget(const QString& subjectText); // 创建一个科目tag
    void setTeachSubjectsInUI(const QStringList& subjects); // 用指定科目刷新tag区
};


