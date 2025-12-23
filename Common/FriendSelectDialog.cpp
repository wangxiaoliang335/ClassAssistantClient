#pragma execution_character_set("utf-8")
#include "FriendSelectDialog.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QScrollArea>
#include <QWidget>
#include <QPointer>
#include <QMetaObject>

#include "ImSDK/includes/TIMCloud.h"
#include "ImSDK/includes/TIMCloudDef.h"
#include "TIMRestAPI.h"
#include "GenerateTestUserSig.h"

FriendSelectDialog::FriendSelectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("选择好友");
    resize(400, 500);
    setStyleSheet("background-color:#555555; font-size:14px;");

    m_httpHandler = new TAHttpHandler(this);
    if (m_httpHandler)
    {
        connect(m_httpHandler, &TAHttpHandler::success, this, &FriendSelectDialog::onHttpSuccess);
        connect(m_httpHandler, &TAHttpHandler::failed, this, &FriendSelectDialog::onHttpFailed);
    }

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 标题
    QLabel* lblTitle = new QLabel("好友列表");
    lblTitle->setStyleSheet("background-color:#3b73b8; qproperty-alignment: AlignCenter; color:white; font-weight:bold; padding:6px;");
    mainLayout->addWidget(lblTitle);

    // 滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet("background-color:transparent; border:none;");

    m_scrollWidget = new QWidget;
    m_friendsLayout = new QVBoxLayout(m_scrollWidget);
    m_friendsLayout->setSpacing(8);
    m_friendsLayout->setContentsMargins(5, 5, 5, 5);
    m_friendsLayout->addStretch();

    m_scrollArea->setWidget(m_scrollWidget);
    mainLayout->addWidget(m_scrollArea);

    // 底部按钮
    QHBoxLayout* bottomLayout = new QHBoxLayout;
    QPushButton* btnCancel = new QPushButton("取消");
    QPushButton* btnOk = new QPushButton("确定");
    btnCancel->setStyleSheet("background-color:green; color:white; padding:6px; border-radius:4px;");
    btnOk->setStyleSheet("background-color:green; color:white; padding:6px; border-radius:4px;");
    bottomLayout->addStretch();
    bottomLayout->addWidget(btnCancel);
    bottomLayout->addWidget(btnOk);
    mainLayout->addLayout(bottomLayout);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnOk, &QPushButton::clicked, this, &FriendSelectDialog::onOkClicked);
}

FriendSelectDialog::~FriendSelectDialog()
{}

void FriendSelectDialog::setUseTencentSDK(bool useTencentSDK)
{
    m_useTencentSDK = useTencentSDK;
}

void FriendSelectDialog::InitData()
{
    UserInfo userInfo = CommonInfo::GetData();
    if (m_httpHandler)
    {
        QString url = "http://47.100.126.194:5000/friends?";
        url += "id_card=";
        url += userInfo.strIdNumber;
        m_httpHandler->get(url);
    }
}

void FriendSelectDialog::setExcludedMemberIds(const QVector<QString>& memberIds)
{
    m_excludedMemberIds = memberIds;
}

void FriendSelectDialog::setGroupId(const QString& groupId)
{
    m_groupId = groupId;
}

void FriendSelectDialog::setGroupName(const QString& groupName)
{
    m_groupName = groupName;
}

QVector<QString> FriendSelectDialog::getSelectedFriendIds()
{
    QVector<QString> selectedIds;
    for (QCheckBox* checkbox : m_checkBoxes)
    {
        if (checkbox && checkbox->isChecked())
        {
            QString teacherUniqueId = checkbox->property("teacher_unique_id").toString();
            if (!teacherUniqueId.isEmpty())
            {
                selectedIds.append(teacherUniqueId);
            }
        }
    }
    return selectedIds;
}

void FriendSelectDialog::onHttpSuccess(const QString& responseString)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
    if (jsonDoc.isObject())
    {
        QJsonObject obj = jsonDoc.object();

        clearLayout(m_friendsLayout);

        if (obj["friends"].isArray())
        {
            QJsonArray friendsArray = obj.value("friends").toArray();
            
            for (int i = 0; i < friendsArray.size(); i++)
            {
                QJsonObject friendObj = friendsArray.at(i).toObject();

                // teacher_info 对象
                QJsonObject teacherInfo = friendObj.value("teacher_info").toObject();
                QString name = teacherInfo.value("name").toString();
                QString teacher_unique_id = teacherInfo.value("teacher_unique_id").toString();

                // 检查是否在排除列表中（通过 teacher_unique_id 匹配）
                bool isExcluded = false;
                for (const QString& excludedId : m_excludedMemberIds)
                {
                    if (excludedId == teacher_unique_id)
                    {
                        isExcluded = true;
                        break;
                    }
                }

                // 如果不在排除列表中，才显示这个好友
                if (!isExcluded)
                {
                    // user_details 对象
                    QJsonObject userDetails = friendObj.value("user_details").toObject();
                    QString phone = userDetails.value("phone").toString();
                    QString uname = userDetails.value("name").toString();

                    // 创建复选框
                    QCheckBox* checkbox = new QCheckBox(name.isEmpty() ? uname : name, m_scrollWidget);
                    checkbox->setProperty("teacher_unique_id", teacher_unique_id); // 使用teacher_unique_id作为唯一标识
                    checkbox->setStyleSheet("color:white; padding:4px;");
                    
                    // 存储好友信息：teacher_unique_id -> name
                    m_friendInfoMap[teacher_unique_id] = name.isEmpty() ? uname : name;
                    
                    m_checkBoxes.append(checkbox);
                    m_friendsLayout->insertWidget(m_friendsLayout->count() - 1, checkbox); // 在stretch之前插入
                }
            }
        }

        QJsonObject oTmp = obj["data"].toObject();
        QString strTmp = oTmp["message"].toString();
        qDebug() << "status:" << oTmp["code"].toString();
        qDebug() << "msg:" << oTmp["message"].toString();
    }
    else
    {
        qDebug() << "网络错误：JSON解析失败";
    }
}

void FriendSelectDialog::onHttpFailed(const QString& errResponseString)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(errResponseString.toUtf8());
    if (jsonDoc.isObject())
    {
        QJsonObject obj = jsonDoc.object();
        if (obj["data"].isObject())
        {
            QJsonObject oTmp = obj["data"].toObject();
            QString strTmp = oTmp["message"].toString();
            qDebug() << "status:" << oTmp["code"].toString();
            qDebug() << "msg:" << oTmp["message"].toString();
        }
    }
}

void FriendSelectDialog::onOkClicked()
{
    // 获取选中的好友
    QVector<QString> selectedTeacherIds; // teacher_unique_id列表
    QVector<QString> selectedNames; // 好友名称列表
    
    for (QCheckBox* checkbox : m_checkBoxes)
    {
        if (checkbox && checkbox->isChecked())
        {
            QString teacherUniqueId = checkbox->property("teacher_unique_id").toString();
            
            if (!teacherUniqueId.isEmpty())
            {
                selectedTeacherIds.append(teacherUniqueId);
                
                // 从map中获取名称
                if (m_friendInfoMap.contains(teacherUniqueId))
                {
                    selectedNames.append(m_friendInfoMap[teacherUniqueId]);
                }
                else
                {
                    selectedNames.append(teacherUniqueId);
                }
            }
        }
    }
    
    if (selectedTeacherIds.isEmpty())
    {
        QMessageBox::information(this, "提示", "请至少选择一个好友");
        return;
    }
    
    if (m_groupId.isEmpty())
    {
        QMessageBox::warning(this, "错误", "群组ID未设置");
        return;
    }
    
    // 普通群：邀请成员走腾讯 SDK；班级群：沿用原有服务器接口
    if (m_useTencentSDK) {
        QJsonObject payload;
        payload[QString::fromUtf8(kTIMGroupInviteMemberParamGroupId)] = m_groupId;

        QJsonArray idArray;
        for (const auto& id : selectedTeacherIds) {
            idArray.append(id);
        }
        payload[QString::fromUtf8(kTIMGroupInviteMemberParamIdentifierArray)] = idArray;
        payload[QString::fromUtf8(kTIMGroupInviteMemberParamUserData)] = QStringLiteral("ta_invite");

        const QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);

        struct InviteCbData {
            QPointer<FriendSelectDialog> dlg;
            QString groupId;
            QVector<QString> ids;
            QVector<QString> names;
        };
        InviteCbData* cbData = new InviteCbData;
        cbData->dlg = this;
        cbData->groupId = m_groupId;
        cbData->ids = selectedTeacherIds;
        cbData->names = selectedNames;

        int callRet = TIMGroupInviteMember(jsonData.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                InviteCbData* d = (InviteCbData*)user_data;
                if (!d) return;
                if (!d->dlg) { delete d; return; }

                const QPointer<FriendSelectDialog> dlg = d->dlg;
                const QString groupId = d->groupId;
                const QVector<QString> ids = d->ids;
                const QVector<QString> names = d->names;
                const QString errDesc = QString::fromUtf8(desc ? desc : "");
                const QByteArray payload = QByteArray(json_params ? json_params : "");

                if (dlg) {
                    QMetaObject::invokeMethod(dlg, [dlg, groupId, ids, names, code, errDesc, payload]() {
                        if (!dlg) return;
                        if (code != 0) {
                            QMessageBox::warning(dlg, QString::fromUtf8(u8"邀请失败"),
                                QString("腾讯SDK邀请失败\n错误码: %1\n错误描述: %2").arg(code).arg(errDesc));
                            return;
                        }

                        int suc = 0, included = 0, invited = 0, failed = 0;
                        QStringList failedIds;
                        QSet<QString> succeededOrIncluded;

                        QJsonParseError pe;
                        QJsonDocument doc = QJsonDocument::fromJson(payload, &pe);
                        if (pe.error == QJsonParseError::NoError && doc.isArray()) {
                            const QJsonArray arr = doc.array();
                            for (const auto& v : arr) {
                                if (!v.isObject()) continue;
                                const QJsonObject o = v.toObject();
                                const QString id = o.value(QString::fromUtf8(kTIMGroupInviteMemberResultIdentifier)).toString();
                                const int r = o.value(QString::fromUtf8(kTIMGroupInviteMemberResultResult)).toInt(0);
                                if (r == (int)kTIMGroupMember_HandledSuc) { suc++; if (!id.isEmpty()) succeededOrIncluded.insert(id); }
                                else if (r == (int)kTIMGroupMember_Included) { included++; if (!id.isEmpty()) succeededOrIncluded.insert(id); }
                                else if (r == (int)kTIMGroupMember_Invited) { invited++; }
                                else { failed++; if (!id.isEmpty()) failedIds << id; }
                            }
                        }

                        // 普通群：尽量把名字写入群名片（NameCard），避免成员列表只显示账号ID
                        // 仅对已经“成功加入/已在群”的成员设置 NameCard；Invited(待同意)此时可能还不在群里
                        if (!succeededOrIncluded.isEmpty()) {
                            std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
                            if (!adminUserId.empty()) {
                                const QString adminId = QString::fromStdString(adminUserId);
                                const QString adminSig = QString::fromStdString(GenerateTestUserSig::instance().genTestUserSig(adminUserId));

                                TIMRestAPI* rest = new TIMRestAPI(dlg);
                                rest->setAdminInfo(adminId, adminSig);

                                for (int i = 0; i < ids.size() && i < names.size(); ++i) {
                                    const QString memberId = ids[i];
                                    const QString memberName = names[i].trimmed();
                                    if (!succeededOrIncluded.contains(memberId)) continue;
                                    if (memberName.isEmpty()) continue;

                                    // 只设置 nameCard，不改角色/禁言
                                    rest->modifyGroupMemberInfo(groupId, memberId, QString(), -1, memberName,
                                        [memberId, memberName](int ec, const QString& ed, const QJsonObject& /*res*/) {
                                            if (ec != 0) {
                                                qDebug() << "设置 NameCard 失败 member:" << memberId << "name:" << memberName
                                                         << "error:" << ec << ed;
                                            } else {
                                                qDebug() << "设置 NameCard 成功 member:" << memberId << "name:" << memberName;
                                            }
                                        });
                                }
                            } else {
                                qWarning() << "管理员账号未设置，跳过 NameCard 设置";
                            }
                        }

                        QString msg = QString::fromUtf8(u8"邀请已提交。\n");
                        msg += QString("成功: %1，已在群: %2，已发送邀请: %3，失败: %4")
                                   .arg(suc).arg(included).arg(invited).arg(failed);
                        if (!failedIds.isEmpty()) {
                            msg += QString::fromUtf8(u8"\n失败成员: ") + failedIds.mid(0, 8).join(", ");
                            if (failedIds.size() > 8) msg += "...";
                        }

                        QMessageBox::information(dlg, QString::fromUtf8(u8"邀请结果"), msg);

                        // 有任何“非失败”都刷新成员列表（公共群可能是 invited，成员需要同意后才会真正进群）
                        if (suc > 0 || included > 0 || invited > 0) {
                            emit dlg->membersInvitedSuccess(groupId);
                            dlg->accept();
                        }
                    }, Qt::QueuedConnection);
                }

                delete d;
            },
            cbData);

        if (callRet != TIM_SUCC) {
            delete cbData;
            QMessageBox::warning(this, QString::fromUtf8(u8"邀请失败"),
                QString("TIMGroupInviteMember 调用失败，错误码: %1").arg(callRet));
        }
        return;
    }

    if (!m_httpHandler) {
        QMessageBox::critical(this, "错误", "HTTP处理器未初始化！");
        return;
    }
    
    // 直接调用服务器接口邀请成员
    UserInfo userInfo = CommonInfo::GetData();
    QString url = "http://47.100.126.194:5000/groups/invite";
    
    // 构造成员数组
    QJsonArray membersArray;
    for (int i = 0; i < selectedTeacherIds.size(); i++)
    {
        QJsonObject memberObj;
        memberObj["unique_member_id"] = selectedTeacherIds[i];
        memberObj["member_name"] = selectedNames[i];
        memberObj["group_role"] = 1; // 1是普通成员，300是管理员角色，400是群主
        membersArray.append(memberObj);
    }
    
    // 构造请求JSON
    QJsonObject payload;
    payload["group_id"] = m_groupId;
    payload["members"] = membersArray;
    
    // 转换为JSON字符串
    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    qDebug() << "========== 邀请成员 - 服务器接口 ==========";
    qDebug() << "群组ID:" << m_groupId;
    qDebug() << "成员数量:" << membersArray.size();
    qDebug() << "请求JSON:" << QString::fromUtf8(jsonData);
    
    // 创建HTTP处理器用于接收响应
    TAHttpHandler* inviteHandler = new TAHttpHandler(this);
    connect(inviteHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
        qDebug() << "邀请成员服务器响应:" << responseString;
        
        // 解析响应
        QJsonDocument respDoc = QJsonDocument::fromJson(responseString.toUtf8());
        bool success = false;
        QString message = QString::fromUtf8(u8"邀请成员成功");
        
        if (respDoc.isObject()) {
            QJsonObject obj = respDoc.object();
            if (obj.contains("code")) {
                int code = obj.value("code").toInt(-1);
                success = (code == 0 || code == 200);
            }
            if (obj.contains("message") && obj.value("message").isString()) {
                message = obj.value("message").toString();
            }
        } else {
            // 非JSON响应但HTTP成功，视为成功
            success = true;
        }
        
        if (success) {
            QMessageBox::information(this, QString::fromUtf8(u8"邀请成功"), message);
            emit membersInvitedSuccess(m_groupId); // 发出信号通知刷新成员列表
            accept(); // 关闭对话框
        } else {
            QMessageBox::critical(this, QString::fromUtf8(u8"邀请失败"), message);
        }
        
        inviteHandler->deleteLater();
    });
    
    connect(inviteHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
        qDebug() << "邀请成员服务器错误:" << errResponseString;
        
        QString errorMsg = QString::fromUtf8(u8"邀请成员失败");
        QJsonDocument errDoc = QJsonDocument::fromJson(errResponseString.toUtf8());
        if (errDoc.isObject()) {
            QJsonObject errObj = errDoc.object();
            if (errObj.contains("message") && errObj.value("message").isString()) {
                errorMsg = errObj.value("message").toString();
            }
        }
        
        QMessageBox::critical(this, QString::fromUtf8(u8"邀请失败"), errorMsg);
        inviteHandler->deleteLater();
    });
    
    // 发送POST请求
    inviteHandler->post(url, jsonData);
}

void FriendSelectDialog::clearLayout(QVBoxLayout* layout)
{
    if (!layout) return;
    
    // 清空复选框列表
    for (QCheckBox* checkbox : m_checkBoxes)
    {
        if (checkbox)
        {
            checkbox->deleteLater();
        }
    }
    m_checkBoxes.clear();
    m_friendInfoMap.clear(); // 清空好友信息映射

    QLayoutItem* child;
    while ((child = layout->takeAt(0)) != nullptr)
    {
        if (child->widget() && child->widget() != m_scrollWidget)
        {
            child->widget()->deleteLater();
        }
        delete child;
    }
    
    // 重新添加stretch
    layout->addStretch();
}

