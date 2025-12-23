#pragma execution_character_set("utf-8")
#include "MemberKickDialog.h"
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

MemberKickDialog::MemberKickDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("踢出群成员");
    resize(400, 500);
    setStyleSheet("background-color:#555555; font-size:14px;");
    
    m_httpHandler = new TAHttpHandler(this);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 标题
    QLabel* lblTitle = new QLabel("群成员列表");
    lblTitle->setStyleSheet("background-color:#3b73b8; qproperty-alignment: AlignCenter; color:white; font-weight:bold; padding:6px;");
    mainLayout->addWidget(lblTitle);

    // 滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet("background-color:transparent; border:none;");

    m_scrollWidget = new QWidget;
    m_membersLayout = new QVBoxLayout(m_scrollWidget);
    m_membersLayout->setSpacing(8);
    m_membersLayout->setContentsMargins(5, 5, 5, 5);
    m_membersLayout->addStretch();

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
    connect(btnOk, &QPushButton::clicked, this, &MemberKickDialog::onOkClicked);
}

MemberKickDialog::~MemberKickDialog()
{}

void MemberKickDialog::setUseTencentSDK(bool useTencentSDK)
{
    m_useTencentSDK = useTencentSDK;
}

void MemberKickDialog::InitData(const QVector<GroupMemberInfo>& memberList)
{
    clearLayout(m_membersLayout);
    
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.classId;
    
    // 遍历成员列表，排除群主（角色为"群主"的成员）
    for (const auto& member : memberList)
    {
        // 排除群主
        if (member.member_role == "群主")
        {
            continue;
        }
        
        // 创建复选框
        QCheckBox* checkbox = new QCheckBox(member.member_name, m_scrollWidget);
        checkbox->setProperty("member_id", member.member_id);
        checkbox->setStyleSheet("color:white; padding:4px;");
        
        // 存储成员信息
        m_memberInfoMap[member.member_id] = member.member_name;
        
        m_checkBoxes.append(checkbox);
        m_membersLayout->insertWidget(m_membersLayout->count() - 1, checkbox); // 插入到stretch之前
    }
}

void MemberKickDialog::setGroupId(const QString& groupId)
{
    m_groupId = groupId;
}

void MemberKickDialog::setGroupName(const QString& groupName)
{
    m_groupName = groupName;
}

QVector<QString> MemberKickDialog::getSelectedMemberIds()
{
    QVector<QString> selectedIds;
    for (QCheckBox* checkbox : m_checkBoxes)
    {
        if (checkbox && checkbox->isChecked())
        {
            QString memberId = checkbox->property("member_id").toString();
            if (!memberId.isEmpty())
            {
                selectedIds.append(memberId);
            }
        }
    }
    return selectedIds;
}

void MemberKickDialog::onOkClicked()
{
    // 获取选中的成员
    QVector<QString> selectedMemberIds;
    QVector<QString> selectedMemberNames;
    
    for (QCheckBox* checkbox : m_checkBoxes)
    {
        if (checkbox && checkbox->isChecked())
        {
            QString memberId = checkbox->property("member_id").toString();
            
            if (!memberId.isEmpty())
            {
                selectedMemberIds.append(memberId);
                
                // 获取名称
                if (m_memberInfoMap.contains(memberId))
                {
                    selectedMemberNames.append(m_memberInfoMap[memberId]);
                }
                else
                {
                    selectedMemberNames.append(memberId);
                }
            }
        }
    }
    
    if (selectedMemberIds.isEmpty())
    {
        QMessageBox::information(this, "提示", "请至少选择一个成员");
        return;
    }
    
    if (m_groupId.isEmpty())
    {
        QMessageBox::warning(this, "错误", "群组ID未设置");
        return;
    }
    
    // 普通群：踢人走腾讯 SDK；班级群：沿用原有服务器接口
    if (m_useTencentSDK) {
        QJsonObject payload;
        payload[QString::fromUtf8(kTIMGroupDeleteMemberParamGroupId)] = m_groupId;

        QJsonArray idArray;
        for (const auto& id : selectedMemberIds) {
            idArray.append(id);
        }
        payload[QString::fromUtf8(kTIMGroupDeleteMemberParamIdentifierArray)] = idArray;
        payload[QString::fromUtf8(kTIMGroupDeleteMemberParamUserData)] = QStringLiteral("ta_kick");

        const QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);

        struct KickCbData {
            QPointer<MemberKickDialog> dlg;
            QString groupId;
        };
        KickCbData* cbData = new KickCbData;
        cbData->dlg = this;
        cbData->groupId = m_groupId;

        int callRet = TIMGroupDeleteMember(jsonData.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                KickCbData* d = (KickCbData*)user_data;
                if (!d) return;
                if (!d->dlg) { delete d; return; }

                const QPointer<MemberKickDialog> dlg = d->dlg;
                const QString groupId = d->groupId;
                const QString errDesc = QString::fromUtf8(desc ? desc : "");
                const QByteArray payload = QByteArray(json_params ? json_params : "");

                if (dlg) {
                    QMetaObject::invokeMethod(dlg, [dlg, groupId, code, errDesc, payload]() {
                        if (!dlg) return;
                        if (code != 0) {
                            QMessageBox::warning(dlg, QString::fromUtf8(u8"踢出失败"),
                                QString("腾讯SDK踢出失败\n错误码: %1\n错误描述: %2").arg(code).arg(errDesc));
                            return;
                        }

                        int suc = 0, included = 0, invited = 0, failed = 0;
                        QStringList failedIds;

                        QJsonParseError pe;
                        QJsonDocument doc = QJsonDocument::fromJson(payload, &pe);
                        if (pe.error == QJsonParseError::NoError && doc.isArray()) {
                            const QJsonArray arr = doc.array();
                            for (const auto& v : arr) {
                                if (!v.isObject()) continue;
                                const QJsonObject o = v.toObject();
                                const QString id = o.value(QString::fromUtf8(kTIMGroupDeleteMemberResultIdentifier)).toString();
                                const int r = o.value(QString::fromUtf8(kTIMGroupDeleteMemberResultResult)).toInt(0);
                                if (r == (int)kTIMGroupMember_HandledSuc) suc++;
                                else if (r == (int)kTIMGroupMember_Included) included++; // 对 delete 来说通常不会出现
                                else if (r == (int)kTIMGroupMember_Invited) invited++;   // 对 delete 来说通常不会出现
                                else { failed++; if (!id.isEmpty()) failedIds << id; }
                            }
                        }

                        QString msg = QString::fromUtf8(u8"踢出已提交。\n");
                        msg += QString("成功: %1，失败: %2").arg(suc).arg(failed);
                        if (!failedIds.isEmpty()) {
                            msg += QString::fromUtf8(u8"\n失败成员: ") + failedIds.mid(0, 8).join(", ");
                            if (failedIds.size() > 8) msg += "...";
                        }

                        QMessageBox::information(dlg, QString::fromUtf8(u8"踢出结果"), msg);

                        if (suc > 0) {
                            emit dlg->membersKickedSuccess(groupId);
                            dlg->accept();
                        }
                    }, Qt::QueuedConnection);
                }

                delete d;
            },
            cbData);

        if (callRet != TIM_SUCC) {
            delete cbData;
            QMessageBox::warning(this, QString::fromUtf8(u8"踢出失败"),
                QString("TIMGroupDeleteMember 调用失败，错误码: %1").arg(callRet));
        }
        return;
    }

    if (!m_httpHandler) {
        QMessageBox::critical(this, "错误", "HTTP处理器未初始化！");
        return;
    }
    
    // 直接调用服务器接口踢出成员
    QString url = "http://47.100.126.194:5000/groups/remove-member";
    
    // 构造成员ID数组
    QJsonArray membersArray;
    for (const QString& memberId : selectedMemberIds)
    {
        membersArray.append(memberId);
    }
    
    // 构造请求JSON
    QJsonObject payload;
    payload["group_id"] = m_groupId;
    payload["members"] = membersArray;
    
    // 转换为JSON字符串
    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    qDebug() << "========== 踢出成员 - 服务器接口 ==========";
    qDebug() << "群组ID:" << m_groupId;
    qDebug() << "成员数量:" << membersArray.size();
    qDebug() << "请求JSON:" << QString::fromUtf8(jsonData);
    
    // 创建HTTP处理器用于接收响应
    TAHttpHandler* kickHandler = new TAHttpHandler(this);
    connect(kickHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
        qDebug() << "踢出成员服务器响应:" << responseString;
        
        // 解析响应
        QJsonDocument respDoc = QJsonDocument::fromJson(responseString.toUtf8());
        bool success = false;
        QString message = QString::fromUtf8(u8"成员已成功踢出群组");
        
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
            QMessageBox::information(this, QString::fromUtf8(u8"踢出成功"), message);
            emit membersKickedSuccess(m_groupId); // 发出信号通知刷新成员列表
            accept(); // 关闭对话框
        } else {
            QMessageBox::critical(this, QString::fromUtf8(u8"踢出失败"), message);
        }
        
        kickHandler->deleteLater();
    });
    
    connect(kickHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
        qDebug() << "踢出成员服务器错误:" << errResponseString;
        
        QString errorMsg = QString::fromUtf8(u8"踢出成员失败");
        QJsonDocument errDoc = QJsonDocument::fromJson(errResponseString.toUtf8());
        if (errDoc.isObject()) {
            QJsonObject errObj = errDoc.object();
            if (errObj.contains("message") && errObj.value("message").isString()) {
                errorMsg = errObj.value("message").toString();
            }
        }
        
        QMessageBox::critical(this, QString::fromUtf8(u8"踢出失败"), errorMsg);
        kickHandler->deleteLater();
    });
    
    // 发送POST请求
    kickHandler->post(url, jsonData);
}

void MemberKickDialog::clearLayout(QVBoxLayout* layout)
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
    m_memberInfoMap.clear(); // 清空成员信息映射

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

