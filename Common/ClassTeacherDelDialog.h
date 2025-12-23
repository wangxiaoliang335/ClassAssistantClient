#include "ScheduleDialog.h"
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QPushButton>
#include <QFrame>
#include <QDir>
#include <QWebSocket>
#include <QTimer>
#include <qbuttongroup.h>
#include <qmessagebox.h>
#include "TAHttpHandler.h"
#include "CommonInfo.h"
#include "TaQTWebSocket.h"

class ClassTeacherDelDialog : public QDialog
{
    Q_OBJECT
public:
    ClassTeacherDelDialog(QWidget* parent = nullptr, TaQTWebSocket* pWs = NULL) : QDialog(parent)
    {
        setWindowTitle(" ");
        resize(420, 300);
        setStyleSheet("background-color:#dde2f0; font-size:14px;");

        m_pWs = pWs;
        m_httpHandler = new TAHttpHandler(this);
        if (m_httpHandler)
        {
            connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
                //成功消息就不发送了
                QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
                if (jsonDoc.isObject()) {
                    QJsonObject obj = jsonDoc.object();
                    if (obj["friends"].isArray())
                    {
                        QJsonArray friendsArray = obj.value("friends").toArray();
                        bool bFirst = false;
                        //fLayout->addLayout(makeRowBtn("教师", QString::number(friendsArray.size()), "blue", "white"));
                        for (int i = 0; i < friendsArray.size(); i++)
                        {
                            QJsonObject friendObj = friendsArray.at(i).toObject();

                            // teacher_info 对象
                            QJsonObject teacherInfo = friendObj.value("teacher_info").toObject();
                            int id = teacherInfo.value("id").toInt();
                            QString name = teacherInfo.value("name").toString();
                            QString subject = teacherInfo.value("subject").toString();
                            QString idCard = teacherInfo.value("id_card").toString();
                            //int iteacher_unique_id = teacherInfo.value("teacher_unique_id").toInt();
                            //QString teacher_unique_id = QString("%1").arg(iteacher_unique_id, 6, 10, QChar('0'));
                            QString teacher_unique_id = teacherInfo.value("teacher_unique_id").toString();

                            // user_details 对象
                            QJsonObject userDetails = friendObj.value("user_details").toObject();
                            QString phone = userDetails.value("phone").toString();
                            QString uname = userDetails.value("name").toString();
                            QString sex = userDetails.value("sex").toString();

                            /********************************************/
                            QString avatar = userDetails.value("avatar").toString();
                            QString strIdNumber = userDetails.value("id_number").toString();
                            QString avatarBase64 = userDetails.value("avatar_base64").toString();
                            QString grade = userDetails.value("grade").toString();
                            QString class_taught = userDetails.value("class_taught").toString();

                            // 没有文件名就用手机号或ID代替
                            if (avatar.isEmpty())
                                avatar = userDetails.value("id_number").toString() + "_" + ".png";

                            // 从最后一个 "/" 之后开始截取
                            QString fileName = avatar.section('/', -1);  // "320506197910016493_.png"
                            QString saveDir = QCoreApplication::applicationDirPath() + "/avatars/" + strIdNumber; // 保存图片目录
                            QDir().mkpath(saveDir);
                            QString filePath = saveDir + "/" + fileName;

                            if (avatarBase64.isEmpty()) {
                                qWarning() << "No avatar data for" << filePath;
                                //continue;
                            }
                            //m_userInfo.strHeadImagePath = filePath;

                            // Base64 解码成图片二进制数据
                            QByteArray imageData = QByteArray::fromBase64(avatarBase64.toUtf8());

                            // 写入文件（覆盖旧的）
                            QFile file(filePath);
                            if (!file.open(QIODevice::WriteOnly)) {
                                qWarning() << "Cannot open file for writing:" << filePath;
                                //continue;
                            }
                            file.write(imageData);
                            file.close();

                            if (teacherListLayout)
                            {
                                if (false == bFirst)
                                {
                                    addPersonRow(teacherListLayout, filePath, name, phone, teacher_unique_id, grade, class_taught, sexGroup, true);
                                }
                                else
                                {
                                    addPersonRow(teacherListLayout, filePath, name, phone, teacher_unique_id, grade, class_taught, sexGroup);
                                }
                                bFirst = true;
                                //fLayout->addLayout(makePairBtn(filePath, name, "green", "white"));
                            }
                            /********************************************/
                        }

                        QJsonObject oTmp = obj["data"].toObject();
                        QString strTmp = oTmp["message"].toString();
                        qDebug() << "status:" << oTmp["code"].toString();
                        qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
                        //errLabel->setText(strTmp);
                        //user_id = oTmp["user_id"].toInt();
                    }
                }
                else
                {
                    //errLabel->setText("网络错误");
                }
                });

            connect(m_httpHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
                //if (errLabel)
                {
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(errResponseString.toUtf8());
                    if (jsonDoc.isObject()) {
                        QJsonObject obj = jsonDoc.object();
                        if (obj["data"].isObject())
                        {
                            QJsonObject oTmp = obj["data"].toObject();
                            QString strTmp = oTmp["message"].toString();
                            qDebug() << "status:" << oTmp["code"].toString();
                            qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
                            //errLabel->setText(strTmp);
                        }
                    }
                    /*else
                    {
                        errLabel->setText("网络错误");
                    }*/
                }
                });
        }
        if (m_httpHandler)
        {
            //QMap<QString, QString> params;
            //params["id_card"] = "320506197910016493";
            QString url = "http://47.100.126.194:5000/friends?";
            url += "id_card=";
            url += "320506197910016493";
            m_httpHandler->get(url);
        }

        m_scheduleDlg = new ScheduleDialog("", this, pWs);
        m_scheduleDlg->InitWebSocket();

        // 在创建对话框后立即建立连接，而不是等到按钮点击
        //connectGroupLeftSignal(m_scheduleDlg, unique_group_id);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        //// 顶部黄色圆形数字标签
        //QLabel* lblNum = new QLabel("2");
        //lblNum->setAlignment(Qt::AlignCenter);
        //lblNum->setFixedSize(30, 30);
        //lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
        //mainLayout->addWidget(lblNum, 0, Qt::AlignCenter);

        //// 班级区域
        //QVBoxLayout* classLayout = new QVBoxLayout;
        //QLabel* lblClassTitle = new QLabel("班级");
        //lblClassTitle->setStyleSheet("background-color:#3b73b8; color:white; font-weight:bold; padding:6px;");
        //classLayout->addWidget(lblClassTitle);

        //QWidget* classListWidget = new QWidget;
        //classListLayout = new QVBoxLayout(classListWidget);
        //classListLayout->setSpacing(8);
        //addPersonRow(classListLayout, ":/icons/avatar1.png", "软件开发工程师", "", "", "", "", NULL);
        //addPersonRow(classListLayout, ":/icons/avatar2.png", "苏州-UI-已入职", "", "", "", "", NULL);
        //addPersonRow(classListLayout, ":/icons/avatar3.png", "平平淡淡", "", "", "", "", NULL);
        //classLayout->addWidget(classListWidget);
        //mainLayout->addLayout(classLayout);

        // 教师区域
        QVBoxLayout* teacherLayout = new QVBoxLayout;
        QLabel* lblTeacherTitle = new QLabel("当前群聊");
        lblTeacherTitle->setStyleSheet("background-color:#3b73b8; qproperty-alignment: AlignCenter; color:white; font-weight:bold; padding:6px;");
        teacherLayout->addWidget(lblTeacherTitle);

        QWidget* teacherListWidget = new QWidget;
        teacherListLayout = new QVBoxLayout(teacherListWidget);
        teacherListLayout->setSpacing(8);
        //addPersonRow(teacherListLayout, ":/icons/avatar1.png", "软件开发工程师", true); // 这里演示一个选中状态
        //addPersonRow(teacherListLayout, ":/icons/avatar2.png", "苏州-UI-已入职");
        //addPersonRow(teacherListLayout, ":/icons/avatar3.png", "平平淡淡");
        teacherLayout->addWidget(teacherListWidget);
        mainLayout->addLayout(teacherLayout);

        // 底部按钮
        QHBoxLayout* bottomLayout = new QHBoxLayout;
        btnCancel = new QPushButton("取消");
        btnOk = new QPushButton("确定");
        btnCancel->setStyleSheet("background-color:green; color:white; padding:6px; border-radius:4px;");
        btnOk->setStyleSheet("background-color:green; color:white; padding:6px; border-radius:4px;");
        bottomLayout->addStretch();
        bottomLayout->addWidget(btnCancel);
        bottomLayout->addWidget(btnOk);
        mainLayout->addLayout(bottomLayout);

        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        //connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
        connect(btnOk, &QPushButton::clicked, this, [=]() {
            if (m_scheduleDlg && m_scheduleDlg->isHidden())
            {
            QAbstractButton* checked = sexGroup->checkedButton();
            if (!checked) {
                qWarning() << "没有选中的教师";
                return;
            }

            QString grade = checked->property("grade").toString();
            QString class_taught = checked->property("class_taught").toString();
            m_scheduleDlg->InitData(grade + class_taught + "的班级群", "", "", true);
            m_scheduleDlg->show();
            }
            accept();
        });
    }

    QVector<QString> getNoticeMsg()
    {
        return m_NoticeMsg;
    }

    // 辅助函数：为 ScheduleDialog 建立群聊退出信号连接
    //void connectGroupLeftSignal(ScheduleDialog* scheduleDlg, const QString& groupId)
    //{
    //    if (!scheduleDlg) return;

    //    // 连接群聊退出信号，当用户退出群聊时刷新群列表
    //    connect(scheduleDlg, &ScheduleDialog::groupLeft, this, [this](const QString& leftGroupId) {
    //        qDebug() << "收到群聊退出信号，刷新群列表，群组ID:" << leftGroupId;
    //        // 从map中移除已关闭的群聊窗口
    //        if (m_scheduleDlg.contains(leftGroupId)) {
    //            m_scheduleDlg[leftGroupId]->deleteLater();
    //            m_scheduleDlg.remove(leftGroupId);
    //        }
    //        // 刷新群列表
    //        this->InitData();
    //        }, Qt::UniqueConnection); // 使用 UniqueConnection 避免重复连接
    //}

    void InitWebSocket()
    {
        TaQTWebSocket::regRecvDlg(this);
        if (m_pWs)
        {
            connect(m_pWs, &TaQTWebSocket::newMessage,
                this, &ClassTeacherDelDialog::onWebSocketMessage);
        }

        //socket = new QWebSocket();
        //connect(socket, &QWebSocket::connected, this, &ClassTeacherDelDialog::onConnected);
        //connect(socket, &QWebSocket::textMessageReceived, this, &ClassTeacherDelDialog::onMessageReceived);
        ////connect(btnOk, &QPushButton::clicked, this, &ClassTeacherDelDialog::sendBroadcast);
        connect(btnOk, &QPushButton::clicked, this, &ClassTeacherDelDialog::sendPrivateMessage);

        //UserInfo userinfo = CommonInfo::GetData();
        //// 建立连接
        //socket->open(QUrl(QString("ws://47.100.126.194:5000/ws/%1").arg(userinfo.teacher_unique_id)));

        //// 发送心跳
        //heartbeatTimer = new QTimer(this);
        //connect(heartbeatTimer, &QTimer::timeout, this, &ClassTeacherDelDialog::sendHeartbeat);
        //heartbeatTimer->start(5000); // 每 5 秒一次
    }

private:
    void addPersonRow(QVBoxLayout* parentLayout, const QString& iconPath, const QString& name, const QString phone, const QString teacher_unique_id,
        QString grade, QString class_taught, QButtonGroup* pBtnGroup, bool checked = false)
    {
        QHBoxLayout* rowLayout = new QHBoxLayout;
        QRadioButton* radio = new QRadioButton;
        radio->setChecked(checked);
        radio->setProperty("phone", phone); // 可以是 int / QString / QVariant
        radio->setProperty("teacher_unique_id", teacher_unique_id); // 可以是 int / QString / QVariant
        radio->setProperty("grade", grade);
        radio->setProperty("class_taught", class_taught);
        radio->setProperty("name", name);

        QLabel* avatar = new QLabel;
        avatar->setFixedSize(36, 36);
        avatar->setStyleSheet("background-color: lightgray; border-radius: 18px;");
        // 如果有头像图片资源，可以这样设置：
        QPixmap pix(iconPath);
        avatar->setPixmap(pix.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        QLabel* lblName = new QLabel(name);
        rowLayout->addWidget(radio);
        rowLayout->addWidget(avatar);
        rowLayout->addWidget(lblName);
        rowLayout->addStretch();

        if (pBtnGroup)
        {
            pBtnGroup->addButton(radio);
        }

        QWidget* rowWidget = new QWidget;
        rowWidget->setLayout(rowLayout);
        parentLayout->addWidget(rowWidget);
    }
    private slots:
        // ChatDialog.cpp
        void onWebSocketMessage(const QString& msg)
        {
            qDebug() << " ClassTeacherDelDialog msg:" << msg; // 发信号;
            // 这里可以解析 JSON 或直接追加到聊天窗口
            //addTextMessage(":/avatar_teacher.png", "对方", msg, false);
            m_NoticeMsg.push_back(msg);
            if (m_scheduleDlg)
            {
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &parseError);
                if (parseError.error != QJsonParseError::NoError) {
                    qDebug() << "JSON parse error:" << parseError.errorString();
                }
                else {
                    if (doc.isObject()) {
                        QJsonObject obj = doc.object();
                        if (obj["type"].toString() == 3)
                        {
                            obj["group_id"].toString();
                        }
                    }
                }
            }
        }

        void onConnected() {
            //logView->append("✅ 已连接到服务端");
        }

        void onMessageReceived(const QString& msg) {
            //logView->append("📩 收到消息: " + msg);
            if (0 != msg.compare("pong") && 0 == msg.contains("不在线"))
            {
                //QMessageBox::information(NULL, "提示", msg);
                m_NoticeMsg.push_back(msg);
            }
        }

        void sendBroadcast() {
            // 先拿到当前选中的按钮
            QAbstractButton* checked = sexGroup->checkedButton();
            if (!checked) {
                qWarning() << "没有选中的教师";
                return;
            }
            // 取出按钮上绑定的私有数据
            QString phone = checked->property("phone").toString();
            QString teacher_unique_id = checked->property("teacher_unique_id").toString();
            qDebug() << "当前选中教师 Phone:" << phone << "  唯一编号:" << teacher_unique_id;

            QJsonObject obj;
            obj["teacher_unique_id"] = teacher_unique_id;
            obj["phone"] = phone;
            obj["text"] = "加好友";

            // 用 QJsonDocument 序列化
            QJsonDocument doc(obj);
            // 输出美化格式（有缩进）
            QString prettyString = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
            qDebug() << "美化格式:" << prettyString;

            if (!prettyString.isEmpty()) {
                //socket->sendTextMessage(prettyString);
            }
        }

        void sendPrivateMessage() {
			// 先拿到当前选中的按钮
			QAbstractButton* checked = sexGroup->checkedButton();
			if (!checked) {
				qWarning() << "没有选中的教师";
				return;
			}
			// 取出按钮上绑定的私有数据
			QString phone = checked->property("phone").toString();
			QString teacher_unique_id = checked->property("teacher_unique_id").toString();
            QString grade = checked->property("grade").toString();
            QString class_taught = checked->property("class_taught").toString();
            QString name = checked->property("name").toString();
			qDebug() << "当前选中教师 Phone:" << phone << "  唯一编号:" << teacher_unique_id;

			//QJsonObject obj;
			//obj["teacher_unique_id"] = teacher_unique_id;
			//obj["phone"] = phone;
			//obj["text"] = "加好友";
			//obj["type"] = "1";   //1 加好友，2 解除好友, 3.创建群，4.解散群

			//// 用 QJsonDocument 序列化
			//QJsonDocument doc(obj);
			//// 输出美化格式（有缩进）
			//QString prettyString = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
			//qDebug() << "美化格式:" << prettyString;

			//if (!prettyString.isEmpty()) {
			//	socket->sendTextMessage(QString("to:%1:%2").arg(teacher_unique_id, prettyString));
			//	//inputEdit->clear();
			//}

            UserInfo userinfo = CommonInfo::GetData();
            // 创建群
            QJsonObject createGroupMsg;
            createGroupMsg["type"] = "3";
            createGroupMsg["permission_level"] = 1;
            createGroupMsg["headImage_path"] = "/images/group.png";
            createGroupMsg["group_type"] = 0; // 0=公开群（Public），支持设置管理员
            createGroupMsg["nickname"] = grade + class_taught + "的班级群";
            createGroupMsg["owner_id"] = userinfo.classId;
            createGroupMsg["owner_name"] = userinfo.strName;

            // 成员数组
            QJsonArray members;
            QJsonObject m1;
            m1["unique_member_id"] = teacher_unique_id;
            m1["member_name"] = name;
            m1["group_role"] = 0;
            members.append(m1);
            createGroupMsg["members"] = members;

            // 用 QJsonDocument 序列化
            QJsonDocument doc(createGroupMsg);
            // 输出美化格式（有缩进）
            QString prettyString = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
            qDebug() << "美化格式:" << prettyString;

            if (!prettyString.isEmpty()) {
            	//socket->sendTextMessage(QString("to:%1:%2").arg(teacher_unique_id, prettyString));
                TaQTWebSocket::sendPrivateMessage(QString("to:%1:%2").arg(teacher_unique_id, prettyString));
            }
        }

        //void sendPrivateMessage() {
        //    // 先拿到当前选中的按钮
        //    QAbstractButton* checked = sexGroup->checkedButton();
        //    if (!checked) {
        //        qWarning() << "没有选中的教师";
        //        return;
        //    }
        //    // 取出按钮上绑定的私有数据
        //    QString phone = checked->property("phone").toString();
        //    QString teacher_unique_id = checked->property("teacher_unique_id").toString();
        //    qDebug() << "当前选中教师 Phone:" << phone << "  唯一编号:" << teacher_unique_id;

        //    QJsonObject obj;
        //    obj["teacher_unique_id"] = teacher_unique_id;
        //    obj["phone"] = phone;
        //    obj["text"] = "加好友";
        //    obj["type"] = "1";   //1 加好友，2 解除好友, 3.创建群，4.解散群
 
        //    // 用 QJsonDocument 序列化
        //    QJsonDocument doc(obj);
        //    // 输出美化格式（有缩进）
        //    QString prettyString = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
        //    qDebug() << "美化格式:" << prettyString;

        //    if (!prettyString.isEmpty()) {
        //        socket->sendTextMessage(QString("to:%1:%2").arg(teacher_unique_id, prettyString));
        //        //inputEdit->clear();
        //    }
        //}

        void sendHeartbeat() {
            //if (socket->state() == QAbstractSocket::ConnectedState) {
            //    socket->sendTextMessage("ping");
            //}
        }

private:
    ScheduleDialog* m_scheduleDlg = NULL;
    TAHttpHandler* m_httpHandler = NULL;
    QVBoxLayout* teacherListLayout = NULL;
    QVBoxLayout* classListLayout = NULL;
    QButtonGroup* sexGroup = new QButtonGroup(this);
    //QWebSocket* socket = NULL;
    //QTimer* heartbeatTimer;
    QPushButton* btnOk = NULL;
    QPushButton* btnCancel = NULL;
    QVector<QString> m_NoticeMsg;
    TaQTWebSocket* m_pWs = NULL;
};
