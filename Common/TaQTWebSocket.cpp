#include "TaQTWebSocket.h"
#include "CommonInfo.h"
#include <QJsonDocument>
#include <QJsonObject>

QTimer* TaQTWebSocket::heartbeatTimer = NULL;
QWebSocket* TaQTWebSocket::socket = NULL;
QVector<QString> TaQTWebSocket::m_NoticeMsg;
QVector<QDialog*> TaQTWebSocket::m_vecRecvDlg;

TaQTWebSocket::TaQTWebSocket(QObject *parent)
	: QObject(parent)
{}

TaQTWebSocket::~TaQTWebSocket()
{}

void TaQTWebSocket::regRecvDlg(QDialog* dlg)
{
    m_vecRecvDlg.push_back(dlg);
}

void TaQTWebSocket::InitWebSocket(TaQTWebSocket* wsInstance)
{
    socket = new QWebSocket();
    connect(socket, &QWebSocket::connected, wsInstance, &TaQTWebSocket::onConnected);
    connect(socket, &QWebSocket::textMessageReceived, wsInstance, &TaQTWebSocket::onMessageReceived);
    connect(socket, &QWebSocket::binaryMessageReceived, wsInstance, &TaQTWebSocket::onBinaryMessageReceived);
    //connect(btnOk, &QPushButton::clicked, this, &ClassTeacherDialog::sendBroadcast);
    //connect(btnOk, &QPushButton::clicked, this, &TaWebSocket::sendPrivateMessage);

    UserInfo userinfo = CommonInfo::GetData();
    // 建立连接
    socket->open(QUrl(QString("ws://47.100.126.194:5000/ws/%1").arg(userinfo.classId)));

    // 发送心跳
    heartbeatTimer = new QTimer(wsInstance);
    connect(heartbeatTimer, &QTimer::timeout, wsInstance, &TaQTWebSocket::sendHeartbeat);
    heartbeatTimer->start(5000); // 每 5 秒一次
}

void TaQTWebSocket::onConnected() {
    //logView->append("✅ 已连接到服务端");
}

void TaQTWebSocket::onMessageReceived(const QString& msg) {
    if (0 != msg.compare("pong") && 0 == msg.contains("不在线"))
    {
        m_NoticeMsg.push_back(msg);
        
        // 先解析 JSON，检查是否是家庭作业消息或通知消息
        // 如果是家庭作业消息或通知消息，先发射相应信号
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(msg.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
            QJsonObject obj = jsonDoc.object();
            QString type = obj.contains("type") ? obj["type"].toString() : QString();
            
            if (type == "homework") {
                // 是家庭作业消息，先发出专门信号
                emit homeworkReceived(obj);
            } else if (type == "notification" || type == "unread_notifications") {
                // 是通知消息，先发出专门信号
                emit notificationReceived(obj);
            } else if (type == "prepare_class_history") {
                // 是课前准备历史消息（登录后可能下发），先发出专门信号
                emit prepareClassHistoryReceived(obj);
            }
        }
        
        // 然后发射通用 newMessage 信号
        emit newMessage(msg);
    }
}

void TaQTWebSocket::onBinaryMessageReceived(const QByteArray& message)
{
    emit newBinaryMessage(message);
}

void TaQTWebSocket::sendBroadcast() {
    //// 先拿到当前选中的按钮
    //QAbstractButton* checked = sexGroup->checkedButton();
    //if (!checked) {
    //    qWarning() << "没有选中的教师";
    //    return;
    //}
    //// 取出按钮上绑定的私有数据
    //QString phone = checked->property("phone").toString();
    //QString teacher_unique_id = checked->property("teacher_unique_id").toString();
    //qDebug() << "当前选中教师 Phone:" << phone << "  唯一编号:" << teacher_unique_id;

    //QJsonObject obj;
    //obj["teacher_unique_id"] = teacher_unique_id;
    //obj["phone"] = phone;
    //obj["text"] = "加好友";

    //// 用 QJsonDocument 序列化
    //QJsonDocument doc(obj);
    //// 输出美化格式（有缩进）
    //QString prettyString = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
    //qDebug() << "美化格式:" << prettyString;

    //if (!prettyString.isEmpty()) {
    //    socket->sendTextMessage(prettyString);
    //    //inputEdit->clear();
    //}
}

void TaQTWebSocket::sendPrivateMessage(QString msg) {
    //// 先拿到当前选中的按钮
    //QAbstractButton* checked = sexGroup->checkedButton();
    //if (!checked) {
    //    qWarning() << "没有选中的教师";
    //    return;
    //}
    //// 取出按钮上绑定的私有数据
    //QString phone = checked->property("phone").toString();
    //QString teacher_unique_id = checked->property("teacher_unique_id").toString();
    //QString grade = checked->property("grade").toString();
    //QString class_taught = checked->property("class_taught").toString();
    //QString name = checked->property("name").toString();
    //qDebug() << "当前选中教师 Phone:" << phone << "  唯一编号:" << teacher_unique_id;

    //UserInfo userinfo = CommonInfo::GetData();
    //// 创建群
    //QJsonObject createGroupMsg;
    //createGroupMsg["type"] = "3";
    //createGroupMsg["permission_level"] = 1;
    //createGroupMsg["headImage_path"] = "/images/group.png";
    //createGroupMsg["group_type"] = 1;
    //createGroupMsg["nickname"] = grade + class_taught + "的班级群";
    //createGroupMsg["owner_id"] = userinfo.teacher_unique_id;
    //createGroupMsg["owner_name"] = userinfo.strName;

    //// 成员数组
    //QJsonArray members;
    //QJsonObject m1;
    //m1["unique_member_id"] = teacher_unique_id;
    //m1["member_name"] = name;
    //m1["group_role"] = 0;
    //members.append(m1);
    //createGroupMsg["members"] = members;

    //// 用 QJsonDocument 序列化
    //QJsonDocument doc(createGroupMsg);
    //// 输出美化格式（有缩进）
    //QString prettyString = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
    //qDebug() << "美化格式:" << prettyString;

    //if (!prettyString.isEmpty()) {
    //    socket->sendTextMessage(QString("to:%1:%2").arg(teacher_unique_id, prettyString));
    //}

    if (socket)
    {
        socket->sendTextMessage(msg);
    }
}

void TaQTWebSocket::sendBinaryMessage(QByteArray packet)
{
    if (socket)
    {
        socket->sendBinaryMessage(packet);
    }
}

void TaQTWebSocket::sendHeartbeat() {
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->sendTextMessage("ping");
    }
}

