#pragma once
#pragma execution_character_set("utf-8")
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include "CommonInfo.h"
#include "TIMRestAPI.h"
#include "GenerateTestUserSig.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QPointer>
#include <QMetaObject>

class NormalGroupDialog : public QDialog
{
    Q_OBJECT
public:
    NormalGroupDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("创建普通群"));
        resize(400, 200);
        setStyleSheet("background-color:#555555; font-size:14px;");

        m_restAPI = new TIMRestAPI(this);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        // 群组名称输入
        QLabel* nameLabel = new QLabel(QStringLiteral("群组名称:"), this);
        nameLabel->setStyleSheet("color: white;");
        m_nameEdit = new QLineEdit(this);
        m_nameEdit->setPlaceholderText(QStringLiteral("请输入群组名称"));
        m_nameEdit->setStyleSheet("background-color: white; padding: 8px;");

        // 按钮
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        QPushButton* btnCreate = new QPushButton(QStringLiteral("创建"), this);
        QPushButton* btnCancel = new QPushButton(QStringLiteral("取消"), this);
        btnCreate->setStyleSheet("background-color: green; color: white; padding: 8px;");
        btnCancel->setStyleSheet("background-color: gray; color: white; padding: 8px;");

        mainLayout->addWidget(nameLabel);
        mainLayout->addWidget(m_nameEdit);
        mainLayout->addLayout(buttonLayout);
        buttonLayout->addStretch();
        buttonLayout->addWidget(btnCreate);
        buttonLayout->addWidget(btnCancel);

        connect(btnCreate, &QPushButton::clicked, this, &NormalGroupDialog::onCreateClicked);
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    }

signals:
    void groupCreated(const QString& groupId); // 群组创建成功信号

private slots:
    void onCreateClicked()
    {
        QString groupName = m_nameEdit->text().trimmed();
        if (groupName.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("群组名称不能为空！"));
            return;
        }

        UserInfo userinfo = CommonInfo::GetData();
        createNormalGroup(groupName, userinfo);
    }

private:
    void createNormalGroup(const QString& groupName, const UserInfo& userinfo)
    {
        if (!m_restAPI) {
            QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("REST API未初始化！"));
            return;
        }

        // 在使用REST API前设置管理员账号信息
        std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
        if (!adminUserId.empty()) {
            std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
            m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
        } else {
            QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("管理员账号未设置！\n请确保用户已登录且CommonInfo已初始化用户信息。"));
            return;
        }

        // 构造初始成员数组（只有创建者）
        QJsonArray memberArray;
        QString creatorId = userinfo.classId;
        QJsonObject ownerMemberInfo;
        ownerMemberInfo["Member_Account"] = creatorId;
        ownerMemberInfo["Role"] = "Admin";
        // 设置创建者群名片（NameCard），否则拉取成员列表时可能只显示账号ID
        // 优先使用本地用户姓名；为空时可不传（由腾讯侧返回 Nick/Identifier）
        if (!userinfo.strName.trimmed().isEmpty()) {
            ownerMemberInfo["NameCard"] = userinfo.strName.trimmed();
        }
        memberArray.append(ownerMemberInfo);

        // 创建回调数据结构
        struct CreateGroupCallbackData {
            NormalGroupDialog* dlg;
            QString groupName;
            UserInfo userInfo;
        };

        CreateGroupCallbackData* callbackData = new CreateGroupCallbackData;
        callbackData->dlg = this;
        callbackData->groupName = groupName;
        callbackData->userInfo = userinfo;

        // 调用腾讯云 IM REST API 创建“普通群”（公开群）
        m_restAPI->createGroup(groupName, "Public", memberArray,
            [=](int errorCode, const QString& errorDesc, const QJsonObject& result) {
                if (errorCode != 0) {
                    QString errorMsg = QString(QStringLiteral("创建群组失败: %1 (错误码: %2)")).arg(errorDesc).arg(errorCode);
                    qDebug() << errorMsg;
                    QMessageBox::critical(callbackData->dlg, QStringLiteral("创建群组失败"), errorMsg);
                    delete callbackData;
                    return;
                }

                // 创建群组成功，解析返回的群组ID
                QString groupId = result["GroupId"].toString();
                if (groupId.isEmpty()) {
                    QJsonObject groupInfo = result["GroupInfo"].toObject();
                    groupId = groupInfo["GroupId"].toString();
                }

                qDebug() << "创建普通群成功，群组ID:" << groupId;

                if (!groupId.isEmpty()) {
                    // 注意：部分腾讯 REST API 场景下，create_group 的 MemberList 可能不会真正写入 NameCard。
                    // 为确保后续 get_group_member_info 能拿到成员名字，这里在创建成功后补一次 modify_group_member_info 写入 NameCard。
                    const QString creatorId = callbackData->userInfo.classId;
                    const QString creatorName = callbackData->userInfo.strName.trimmed();
                    if (!creatorId.isEmpty() && !creatorName.isEmpty() && callbackData->dlg && callbackData->dlg->m_restAPI) {
                        callbackData->dlg->m_restAPI->modifyGroupMemberInfo(groupId, creatorId, QString(), -1, creatorName,
                            [creatorId, creatorName](int ec, const QString& ed, const QJsonObject& /*res*/) {
                                if (ec != 0) {
                                    qWarning() << "创建普通群后设置创建者 NameCard 失败，member:" << creatorId
                                               << "name:" << creatorName << "error:" << ec << ed;
                                } else {
                                    qDebug() << "创建普通群后设置创建者 NameCard 成功，member:" << creatorId << "name:" << creatorName;
                                }
                            });
                    }

                    QMessageBox::information(callbackData->dlg, QStringLiteral("创建群组成功"),
                        QString(QStringLiteral("群组创建成功！\n群组ID: %1")).arg(groupId));

                    // 发出群组创建成功信号
                    emit callbackData->dlg->groupCreated(groupId);
                    callbackData->dlg->accept();
                } else {
                    qWarning() << "创建群组成功但未获取到群组ID";
                    QMessageBox::warning(callbackData->dlg, QStringLiteral("警告"), QStringLiteral("创建群组成功但未获取到群组ID"));
                }

                delete callbackData;
            });
    }

private:
    QLineEdit* m_nameEdit = nullptr;
    TIMRestAPI* m_restAPI = nullptr;
};

