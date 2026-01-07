#pragma execution_character_set("utf-8")
#include "TIMRestAPI.h"
#include "GenerateTestUserSig.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

TIMRestAPI::TIMRestAPI(QObject* parent)
    : QObject(parent)
    , m_sdkAppId(GenerateTestUserSig::instance().getSDKAppID())
    , m_networkManager(new QNetworkAccessManager(this))
{
    // 诊断SSL支持情况
    qDebug() << "========== SSL/TLS 诊断信息 ==========";
    qDebug() << "QSslSocket::supportsSsl():" << QSslSocket::supportsSsl();
    qDebug() << "SSL库构建版本:" << QSslSocket::sslLibraryBuildVersionString();
    qDebug() << "SSL库运行时版本:" << QSslSocket::sslLibraryVersionString();
    
    // 检查OpenSSL DLL文件
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList sslDllNames = {
        "libeay32.dll", "ssleay32.dll",  // OpenSSL 1.0.x
        "libcrypto-1_1-x64.dll", "libssl-1_1-x64.dll",  // OpenSSL 1.1.x (64位)
        "libcrypto-1_1.dll", "libssl-1_1.dll",  // OpenSSL 1.1.x (32位)
        "libcrypto-3-x64.dll", "libssl-3-x64.dll"  // OpenSSL 3.x (64位)
    };
    
    qDebug() << "应用程序目录:" << appDir;
    qDebug() << "检查OpenSSL DLL文件:";
    bool foundSslDll = false;
    for (const QString& dllName : sslDllNames) {
        QString dllPath = appDir + "/" + dllName;
        QFileInfo fileInfo(dllPath);
        if (fileInfo.exists()) {
            qDebug() << "  找到:" << dllPath;
            foundSslDll = true;
        }
    }
    
    if (!foundSslDll) {
        qWarning() << "  未在应用程序目录找到OpenSSL DLL文件";
    }
    
    // 检查Qt安装目录（检查两个可能的路径）
    QStringList qtBinPaths = {
        "D:/Qt/5.15.2/msvc2019_64/bin",  // Debug配置中的路径
        "C:/Qt/5.15.2/msvc2019_64/bin"   // Release配置中的路径
    };
    
    for (const QString& qtBinPath : qtBinPaths) {
        QFileInfo qtDirInfo(qtBinPath);
        if (qtDirInfo.exists()) {
            qDebug() << "检查Qt安装目录:" << qtBinPath;
            for (const QString& dllName : sslDllNames) {
                QString dllPath = qtBinPath + "/" + dllName;
                QFileInfo fileInfo(dllPath);
                if (fileInfo.exists()) {
                    qDebug() << "  在Qt目录找到:" << dllPath;
                    qWarning() << "  建议：将此文件复制到应用程序目录:" << appDir;
                }
            }
        }
    }
    
    if (!QSslSocket::supportsSsl()) {
        qWarning() << "警告：Qt检测到系统不支持SSL/TLS";
        qWarning() << "这可能导致HTTPS请求失败";
        qWarning() << "解决方案：";
        qWarning() << "1. 从Qt安装目录（通常是 D:/Qt/5.15.2/msvc2019_64/bin 或 C:/Qt/5.15.2/msvc2019_64/bin）";
        qWarning() << "   复制OpenSSL DLL文件（libeay32.dll和ssleay32.dll，或libcrypto-*.dll和libssl-*.dll）";
        qWarning() << "   到应用程序目录:" << appDir;
        qWarning() << "2. 或从OpenSSL官网下载：https://slproweb.com/products/Win32OpenSSL.html";
        qWarning() << "3. 或将OpenSSL DLL文件所在目录添加到系统PATH环境变量";
    }
    qDebug() << "=====================================";
}

TIMRestAPI::~TIMRestAPI()
{
}

void TIMRestAPI::setAdminInfo(const QString& adminUserId, const QString& adminUserSig)
{
    m_adminUserId = adminUserId;
    m_adminUserSig = adminUserSig;
}

QString TIMRestAPI::getBaseURL() const
{
    // 腾讯云IM REST API基础URL
    // 格式：https://console.tim.qq.com/v4/{servicename}/{command}?sdkappid={sdkappid}&identifier={identifier}&usersig={usersig}&random={random}&contenttype=json
    return QString("https://console.tim.qq.com/v4");
}

void TIMRestAPI::sendRestAPIRequest(const QString& apiName, const QJsonObject& requestBody, RestAPICallback callback)
{
    if (m_adminUserId.isEmpty() || m_adminUserSig.isEmpty()) {
        qWarning() << "管理员账号信息未设置，无法调用REST API";
        if (callback) {
            callback(-1, "管理员账号信息未设置", QJsonObject());
        }
        return;
    }

    // 构造URL
    QUrl url(QString("%1/%2").arg(getBaseURL()).arg(apiName));
    QUrlQuery query;
    query.addQueryItem("sdkappid", QString::number(m_sdkAppId));
    query.addQueryItem("identifier", m_adminUserId);
    query.addQueryItem("usersig", m_adminUserSig);
    query.addQueryItem("random", QString::number(QDateTime::currentMSecsSinceEpoch()));
    query.addQueryItem("contenttype", "json");
    url.setQuery(query);

    qDebug() << "========== REST API 请求 ==========";
    qDebug() << "管理员账号(identifier):" << m_adminUserId;
    qDebug() << "URL:" << url.toString();
    qDebug() << "请求体:" << QJsonDocument(requestBody).toJson(QJsonDocument::Indented);

    // 构造请求
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 如果是HTTPS，设置SSL配置
    if (url.scheme().toLower() == "https") {
        // 检查SSL支持（仅用于日志，不阻止请求）
        if (!QSslSocket::supportsSsl()) {
            qWarning() << "警告：QSslSocket::supportsSsl()返回false，但将继续尝试HTTPS请求";
            qWarning() << "SSL库版本信息:" << QSslSocket::sslLibraryVersionString();
            qWarning() << "如果请求失败，可能需要安装OpenSSL库或重新编译Qt以支持SSL";
        }
        
        // 尝试设置SSL配置（即使supportsSsl返回false，也可能可以工作）
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone); // 忽略证书验证（开发环境）
        sslConfig.setProtocol(QSsl::TlsV1_2OrLater); // 使用TLS 1.2或更高版本
        request.setSslConfiguration(sslConfig);
    }

    // 发送POST请求
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(requestBody).toJson(QJsonDocument::Compact));
    
    // 连接错误信号，以便在TLS初始化失败时提供更详细的诊断信息
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred), 
            this, [=](QNetworkReply::NetworkError error) {
        if (error == QNetworkReply::SslHandshakeFailedError || 
            error == QNetworkReply::UnknownNetworkError) {
            qDebug() << "========== 网络请求错误诊断 ==========";
            qDebug() << "错误类型:" << error;
            qDebug() << "错误描述:" << reply->errorString();
            qDebug() << "URL:" << url.toString();
            qDebug() << "SSL支持:" << QSslSocket::supportsSsl();
            qDebug() << "SSL库版本:" << QSslSocket::sslLibraryVersionString();
            qDebug() << "=====================================";
        }
    });

    // 忽略SSL错误（开发环境常用做法，即使supportsSsl返回false也可能需要）
    connect(reply, &QNetworkReply::sslErrors, this, [=](const QList<QSslError>& errors) {
        qDebug() << "检测到SSL错误，将忽略证书验证，URL:" << url.toString() << "，错误数:" << errors.size();
        for (const QSslError& error : errors) {
            qDebug() << "SSL错误详情:" << error.errorString();
        }
        reply->ignoreSslErrors();
    });

    // 连接响应处理
    connect(reply, &QNetworkReply::finished, [=]() {
        parseResponse(reply, callback);
        reply->deleteLater();
    });
}

void TIMRestAPI::parseResponse(QNetworkReply* reply, RestAPICallback callback)
{
    if (!callback) {
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("网络请求失败: %1").arg(reply->errorString());
        qDebug() << errorMsg;
        qDebug() << "错误码:" << reply->error();
        qDebug() << "SSL支持状态:" << (QSslSocket::supportsSsl() ? "支持" : "不支持");
        qDebug() << "SSL库版本:" << QSslSocket::sslLibraryVersionString();
        
        // 如果是SSL相关错误，提供更详细的提示
        if (errorMsg.contains("TLS") || errorMsg.contains("SSL") || errorMsg.contains("certificate")) {
            errorMsg += "\n\n提示：这可能是SSL/TLS配置问题。\n";
            errorMsg += "请检查：\n";
            errorMsg += "1. Qt是否正确编译了SSL支持\n";
            errorMsg += "2. 系统是否安装了OpenSSL库\n";
            errorMsg += "3. Qt是否能找到OpenSSL库文件";
        }
        
        callback(-1, errorMsg, QJsonObject());
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QString errorMsg = QString("JSON解析失败: %1").arg(parseError.errorString());
        qDebug() << errorMsg;
        qDebug() << "响应数据:" << QString::fromUtf8(responseData);
        callback(-1, errorMsg, QJsonObject());
        return;
    }

    QJsonObject responseObj = doc.object();
    QString actionStatus = responseObj["ActionStatus"].toString();
    int errorCode = responseObj["ErrorCode"].toInt();
    QString errorInfo = responseObj["ErrorInfo"].toString();

    qDebug() << "========== REST API 响应 ==========";
    qDebug() << "ActionStatus:" << actionStatus;
    qDebug() << "ErrorCode:" << errorCode;
    qDebug() << "ErrorInfo:" << errorInfo;
    qDebug() << "响应数据:" << QJsonDocument(responseObj).toJson(QJsonDocument::Indented);

    if (actionStatus == "OK" && errorCode == 0) {
        // 成功
        callback(0, "成功", responseObj);
    } else {
        // 失败
        callback(errorCode, errorInfo, responseObj);
    }
}

void TIMRestAPI::createGroup(const QString& groupName, const QString& groupType, 
                             const QJsonArray& memberList, RestAPICallback callback,
                             const QString& groupId)
{
    QJsonObject requestBody;
    requestBody["Name"] = groupName;
    requestBody["Type"] = groupType; // Private, Public, ChatRoom, AVChatRoom
    if (!groupId.isEmpty()) {
        requestBody["GroupId"] = groupId;
    }
    
    // 群组配置
    QJsonObject groupConfig;
    groupConfig["MaxMemberCount"] = 2000;
    groupConfig["ApplyJoinOption"] = "FreeAccess"; // 加群选项：FreeAccess(自由加入), NeedPermission(需要验证), DisableApply(禁止加群)
    requestBody["GroupConfig"] = groupConfig;

    // 初始成员列表（REST API格式）
    // 注意：对于Private群组，MemberList中不能包含Role字段，否则会报错"this group can not set admin"
    // Public、ChatRoom、AVChatRoom等群组类型支持Role字段，可以设置Admin等角色
    // 注意：如果使用Owner_Account指定群主，MemberList中该成员的Role字段会被忽略，不能同时设置为Owner和Admin
    if (!memberList.isEmpty()) {
        // 获取创建者账号（第一个成员）
        QJsonObject firstMember = memberList[0].toObject();
        QString ownerAccount = firstMember["Member_Account"].toString();
        
        // 处理MemberList，根据群组类型决定是否保留Role字段
        QJsonArray processedMemberList;
        for (int i = 0; i < memberList.size(); i++) {
            QJsonObject member = memberList[i].toObject();
            QString memberAccount = member["Member_Account"].toString();
            
            // 如果是Private群组，只保留Member_Account，移除Role字段
            if (groupType == "Private") {
                QJsonObject processedMember;
                processedMember["Member_Account"] = memberAccount;
                processedMemberList.append(processedMember);
            } else {
                // Public、ChatRoom、AVChatRoom等群组类型
                QJsonObject processedMember = member;
                
                // 如果使用Owner_Account指定群主，且当前成员是创建者，则移除Role字段
                // 避免"you can not set owner to admin"错误
                if (!ownerAccount.isEmpty() && memberAccount == ownerAccount) {
                    processedMember.remove("Role");
                }
                
                processedMemberList.append(processedMember);
            }
        }
        requestBody["MemberList"] = processedMemberList;
        
        // 使用Owner_Account指定群主（创建者）
        // 对于Private群组，必须使用Owner_Account指定群主
        // 对于Public群组，使用Owner_Account指定群主时，MemberList中该成员的Role会被忽略
        if (!ownerAccount.isEmpty()) {
            requestBody["Owner_Account"] = ownerAccount;
        }
    }

    // 群组简介和公告（根据群类型给更贴合的文案）
    if (groupType == "Public") {
        requestBody["Introduction"] = QString("普通群：%1").arg(groupName);
        requestBody["Notification"] = QString("欢迎加入%1").arg(groupName);
    } else if (groupType == "Private") {
        requestBody["Introduction"] = QString("私有群：%1").arg(groupName);
        requestBody["Notification"] = QString("欢迎加入%1").arg(groupName);
    } else if (groupType == "ChatRoom" || groupType == "AVChatRoom") {
        requestBody["Introduction"] = QString("聊天室：%1").arg(groupName);
        requestBody["Notification"] = QString("欢迎加入%1").arg(groupName);
    } else {
        // 其他/历史类型兜底
        requestBody["Introduction"] = QString("群：%1").arg(groupName);
        requestBody["Notification"] = QString("欢迎加入%1").arg(groupName);
    }

    sendRestAPIRequest("group_open_http_svc/create_group", requestBody, callback);
}

void TIMRestAPI::inviteGroupMember(const QString& groupId, const QJsonArray& memberList, RestAPICallback callback)
{
    QJsonObject requestBody;
    requestBody["GroupId"] = groupId;
    // REST API格式：MemberList是对象数组，每个对象包含Member_Account
    QJsonArray memberArray;
    for (int i = 0; i < memberList.size(); i++) {
        QJsonObject member;
        member["Member_Account"] = memberList[i].toString();
        memberArray.append(member);
    }
    requestBody["MemberList"] = memberArray;

    sendRestAPIRequest("group_open_http_svc/add_group_member", requestBody, callback);
}

void TIMRestAPI::deleteGroupMember(const QString& groupId, const QJsonArray& memberList, 
                                   const QString& reason, RestAPICallback callback)
{
    QJsonObject requestBody;
    requestBody["GroupId"] = groupId;
    requestBody["MemberToDel_Account"] = memberList; // REST API使用MemberToDel_Account字段
    
    if (!reason.isEmpty()) {
        requestBody["Reason"] = reason;
    }

    sendRestAPIRequest("group_open_http_svc/delete_group_member", requestBody, callback);
}

void TIMRestAPI::destroyGroup(const QString& groupId, RestAPICallback callback)
{
    QJsonObject requestBody;
    requestBody["GroupId"] = groupId;

    sendRestAPIRequest("group_open_http_svc/destroy_group", requestBody, callback);
}

void TIMRestAPI::quitGroup(const QString& groupId, const QString& userId, RestAPICallback callback)
{
    // 退出群组实际上也是删除成员，删除指定的用户
    QJsonObject requestBody;
    requestBody["GroupId"] = groupId;
    
    QJsonArray memberList;
    memberList.append(userId); // 删除指定的用户
    requestBody["MemberToDel_Account"] = memberList;

    sendRestAPIRequest("group_open_http_svc/delete_group_member", requestBody, callback);
}

void TIMRestAPI::getGroupInfo(const QStringList& groupIdList, RestAPICallback callback)
{
    QJsonObject requestBody;
    QJsonArray groupIdArray;
    for (const QString& groupId : groupIdList) {
        groupIdArray.append(groupId);
    }
    requestBody["GroupIdList"] = groupIdArray;

    sendRestAPIRequest("group_open_http_svc/get_group_info", requestBody, callback);
}

void TIMRestAPI::getGroupMemberList(const QString& groupId, int limit, int offset, RestAPICallback callback)
{
    QJsonObject requestBody;
    requestBody["GroupId"] = groupId;
    requestBody["Limit"] = limit;
    requestBody["Offset"] = offset;

    // 显式指定需要返回的成员字段。
    // 如果不传 MemberInfoFilter，服务端可能只返回基础字段，导致 NameCard（群名片）缺失，从而 UI 只能退回显示 Member_Account。
    QJsonArray memberInfoFilter;
    memberInfoFilter.append("Role");
    memberInfoFilter.append("JoinTime");
    memberInfoFilter.append("LastSendMsgTime");
    memberInfoFilter.append("MsgFlag");
    memberInfoFilter.append("MsgSeq");
    memberInfoFilter.append("MuteUntil");
    memberInfoFilter.append("NameCard");
    memberInfoFilter.append("ShutUpUntil");
    requestBody["MemberInfoFilter"] = memberInfoFilter;

    sendRestAPIRequest("group_open_http_svc/get_group_member_info", requestBody, callback);
}

void TIMRestAPI::modifyGroupInfo(const QString& groupId, 
                                const QString& groupName,
                                const QString& introduction,
                                const QString& notification,
                                const QString& faceUrl,
                                int maxMemberCount,
                                const QString& applyJoinOption,
                                RestAPICallback callback)
{
    QJsonObject requestBody;
    requestBody["GroupId"] = groupId;
    
    // 只添加非空的字段
    if (!groupName.isEmpty()) {
        requestBody["Name"] = groupName;
    }
    if (!introduction.isEmpty()) {
        requestBody["Introduction"] = introduction;
    }
    if (!notification.isEmpty()) {
        requestBody["Notification"] = notification;
    }
    if (!faceUrl.isEmpty()) {
        requestBody["FaceUrl"] = faceUrl;
    }
    if (maxMemberCount > 0) {
        requestBody["MaxMemberCount"] = maxMemberCount;
    }
    if (!applyJoinOption.isEmpty()) {
        requestBody["ApplyJoinOption"] = applyJoinOption;
    }

    sendRestAPIRequest("group_open_http_svc/modify_group_base_info", requestBody, callback);
}

void TIMRestAPI::modifyGroupMemberInfo(const QString& groupId, 
                                      const QString& memberAccount,
                                      const QString& role,
                                      int shutUpTime,
                                      const QString& nameCard,
                                      RestAPICallback callback)
{
    QJsonObject requestBody;
    requestBody["GroupId"] = groupId;
    requestBody["Member_Account"] = memberAccount;
    
    // 只添加非空的字段
    if (!role.isEmpty()) {
        requestBody["Role"] = role; // Admin 或 Member
    }
    if (shutUpTime >= 0) {
        requestBody["ShutUpTime"] = shutUpTime; // 禁言时间，单位秒，0表示取消禁言
    }
    if (!nameCard.isEmpty()) {
        requestBody["NameCard"] = nameCard;
    }

    sendRestAPIRequest("group_open_http_svc/modify_group_member_info", requestBody, callback);
}

void TIMRestAPI::changeGroupOwner(const QString& groupId, const QString& newOwnerAccount, RestAPICallback callback)
{
    QJsonObject requestBody;
    requestBody["GroupId"] = groupId;
    requestBody["NewOwner_Account"] = newOwnerAccount;

    sendRestAPIRequest("group_open_http_svc/change_group_owner", requestBody, callback);
}

