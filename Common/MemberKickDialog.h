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
#include "CommonInfo.h"
#include "TAHttpHandler.h"

class MemberKickDialog : public QDialog
{
    Q_OBJECT

signals:
    void membersKickedSuccess(const QString& groupId); // 成员踢出成功信号，通知刷新成员列表

public:
    MemberKickDialog(QWidget* parent = nullptr);
    ~MemberKickDialog();

    void InitData(const QVector<GroupMemberInfo>& memberList); // 初始化成员列表数据
    void setUseTencentSDK(bool useTencentSDK); // 普通群：踢人走腾讯 IM SDK（不走自建服务器）
    void setGroupId(const QString& groupId); // 设置当前群组ID
    void setGroupName(const QString& groupName); // 设置当前群组名称
    QVector<QString> getSelectedMemberIds(); // 获取选中的成员ID列表

private slots:
    void onOkClicked(); // 确定按钮点击处理

private:
    void clearLayout(QVBoxLayout* layout);
    TAHttpHandler* m_httpHandler = NULL;
    QVBoxLayout* m_membersLayout = NULL;
    QScrollArea* m_scrollArea = NULL;
    QWidget* m_scrollWidget = NULL;
    QVector<QCheckBox*> m_checkBoxes; // 保存所有复选框
    QString m_groupId; // 当前群组ID
    QString m_groupName; // 当前群组名称
    QMap<QString, QString> m_memberInfoMap; // 存储成员信息：key为member_id，value为member_name

    bool m_useTencentSDK = false;
};

