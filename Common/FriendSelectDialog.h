#pragma once
#pragma execution_character_set("utf-8")

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QScrollArea>
#include <QWidget>
#include <QMessageBox>
#include <QMap>
#include "TAHttpHandler.h"
#include "CommonInfo.h"

class FriendSelectDialog : public QDialog
{
    Q_OBJECT

signals:
    void membersInvitedSuccess(const QString& groupId); // 成员邀请成功并上传到服务器成功信号，通知刷新成员列表

public:
    FriendSelectDialog(QWidget* parent = nullptr);
    ~FriendSelectDialog();

    void InitData();
    void setUseTencentSDK(bool useTencentSDK); // 普通群：邀请成员走腾讯 IM SDK（不走自建服务器）
    void setExcludedMemberIds(const QVector<QString>& memberIds); // 设置需要排除的成员ID列表
    void setGroupId(const QString& groupId); // 设置当前群组ID
    void setGroupName(const QString& groupName); // 设置当前群组名称
    QVector<QString> getSelectedFriendIds(); // 获取选中的好友ID列表

private slots:
    void onOkClicked(); // 确定按钮点击处理
    void onHttpSuccess(const QString& responseString);
    void onHttpFailed(const QString& errResponseString);

private:
    void clearLayout(QVBoxLayout* layout);
    TAHttpHandler* m_httpHandler = NULL;
    QVBoxLayout* m_friendsLayout = NULL;
    QScrollArea* m_scrollArea = NULL;
    QWidget* m_scrollWidget = NULL;
    QVector<QCheckBox*> m_checkBoxes; // 保存所有复选框
    QVector<QString> m_excludedMemberIds; // 需要排除的成员ID列表
    QString m_groupId; // 当前群组ID
    QString m_groupName; // 当前群组名称
    QMap<QString, QString> m_friendInfoMap; // 存储好友信息：key为teacher_unique_id，value为name

    bool m_useTencentSDK = false;
};

