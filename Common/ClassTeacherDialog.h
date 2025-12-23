#include <QApplication>
#include <QGuiApplication>
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
#include "ImSDK/includes/TIMCloud.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QDateTime>
#include <QPointer>
#include <QUrl>
#include <QUrlQuery>
#include <QMetaObject>
#include <QEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRegion>
#include <QScrollArea>
#include <QScreen>
#include "ScheduleDialog.h"  // 包含以使用 TempRoomStorage

// 自定义消息对话框类
class CustomMessageDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CustomMessageDialog(QWidget* parent = nullptr, const QString& title = "", const QString& message = "")
        : QDialog(parent), m_dragging(false), m_cornerRadius(16)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        resize(400, 200);
        
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(16);
        
        // 标题
        QLabel* titleLabel = new QLabel(title, this);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold; padding: 8px 0px;");
        mainLayout->addWidget(titleLabel);
        
        // 消息内容
        QLabel* messageLabel = new QLabel(message, this);
        messageLabel->setAlignment(Qt::AlignCenter);
        messageLabel->setWordWrap(true);
        messageLabel->setStyleSheet("color: white; font-size: 14px; padding: 8px;");
        mainLayout->addWidget(messageLabel);
        
        // 确定按钮
        QPushButton* okButton = new QPushButton(QString::fromUtf8(u8"确定"), this);
        okButton->setStyleSheet("background-color: #1976d2; color: white; padding: 8px 16px; border-radius: 4px; font-size: 14px;");
        okButton->setFixedHeight(36);
        connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
        
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addStretch();
        mainLayout->addLayout(buttonLayout);
        
        setStyleSheet("QDialog { background-color: #323232; border-radius: 16px; }");
        updateMask();
    }
    
    static int showMessage(QWidget* parent, const QString& title, const QString& message)
    {
        CustomMessageDialog dialog(parent, title, message);
        return dialog.exec();
    }
    
protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRect rect(0, 0, width(), height());
        QPainterPath path;
        path.addRoundedRect(rect, m_cornerRadius, m_cornerRadius);
        p.fillPath(path, QBrush(QColor(50, 50, 50)));
        QPen pen(Qt::white, 2);
        p.strokePath(path, pen);
    }
    
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        }
        QDialog::mousePressEvent(event);
    }
    
    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
        }
        QDialog::mouseMoveEvent(event);
    }
    
    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
        }
        QDialog::mouseReleaseEvent(event);
    }
    
    void resizeEvent(QResizeEvent* event) override
    {
        QDialog::resizeEvent(event);
        updateMask();
    }
    
private:
    void updateMask()
    {
        QPainterPath path;
        path.addRoundedRect(0, 0, width(), height(), m_cornerRadius, m_cornerRadius);
        QRegion region(path.toFillPolygon().toPolygon());
        setMask(region);
    }
    
    bool m_dragging;
    QPoint m_dragStartPos;
    int m_cornerRadius;
};

class ClassTeacherDialog : public QDialog
{
    Q_OBJECT
public:
    ClassTeacherDialog(QWidget* parent = nullptr, TaQTWebSocket* pWs = NULL) : QDialog(parent)
    {
        setWindowTitle("班级 / 教师选择");
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        resize(420, 300);
        setStyleSheet("font-size:14px;");
        m_cornerRadius = 16;
        updateMask();

        closeButton = new QPushButton(this);
        closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
        closeButton->setIconSize(QSize(22, 22));
        closeButton->setFixedSize(QSize(22, 22));
        closeButton->setStyleSheet("background: transparent;");
        closeButton->move(width() - 24, 4);
        closeButton->hide();
        connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

        m_pWs = pWs;
        m_httpHandler = new TAHttpHandler(this);
        if (m_httpHandler)
        {
            connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
                //成功消息就不发送了
                QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
                if (jsonDoc.isObject()) {
                    QJsonObject obj = jsonDoc.object();

                    clearLayout(teacherListLayout);   //获取到好友之后，先清空一下列表在添加

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

        // m_scheduleDlg 已移至 FriendGroupDialog 中管理

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 40, 20, 20);
        mainLayout->setSpacing(16);

        // 统一设置深灰背景，便于控件辨识
        this->setStyleSheet(
            "QDialog { background-color: #565656; }"
            "QScrollArea { background: #565656; border: none; }"
            "QScrollArea > QWidget > QWidget { background: #565656; }"
            "QLabel { color: #f5f5f5; }"
        );

        // 窗口标题标签
        QLabel* lblWindowTitle = new QLabel(QString::fromUtf8(u8"班级 / 教师选择"));
        lblWindowTitle->setAlignment(Qt::AlignCenter);
        lblWindowTitle->setStyleSheet("color: white; font-size: 16px; font-weight: bold; padding: 8px 0px;");
        mainLayout->addWidget(lblWindowTitle);

        // 顶部黄色圆形数字标签
        //QLabel* lblNum = new QLabel("2");
        //lblNum->setAlignment(Qt::AlignCenter);
        //lblNum->setFixedSize(30, 30);
        //lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
        //mainLayout->addWidget(lblNum, 0, Qt::AlignCenter);

        // 班级区域
        QVBoxLayout* classLayout = new QVBoxLayout;
        QLabel* lblClassTitle = new QLabel("班级");
        lblClassTitle->setStyleSheet("background-color:#3b73b8; qproperty-alignment: AlignCenter; color:white; font-weight:bold; padding:6px;");
        lblClassTitle->setFixedHeight(32);
        classLayout->addWidget(lblClassTitle);

        QWidget* classListWidget = new QWidget;
        classListWidget->setStyleSheet("background-color: #606060;");
        classListLayout = new QVBoxLayout(classListWidget);
        classListLayout->setSpacing(8);
        // 初始化班级单选分组，保证互斥
        classGroup = new QButtonGroup(this);
        classGroup->setExclusive(true);
        // 为班级列表增加滚动区域，避免超出屏幕
        QScrollArea* classScrollArea = new QScrollArea;
        classScrollArea->setWidget(classListWidget);
        classScrollArea->setWidgetResizable(true);
        classScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        classScrollArea->setFrameShape(QFrame::NoFrame);
        classLayout->addWidget(classScrollArea);
        mainLayout->addLayout(classLayout);
        // 班级与教师区域之间留白，避免遮挡（再向下移动一些）
        mainLayout->addSpacing(40);

        // 教师区域
        QVBoxLayout* teacherLayout = new QVBoxLayout;
        QLabel* lblTeacherTitle = new QLabel("教师");
        lblTeacherTitle->setStyleSheet("background-color:#3b73b8; qproperty-alignment: AlignCenter; color:white; font-weight:bold; padding:6px;");
        lblTeacherTitle->setFixedHeight(32);
        teacherLayout->addWidget(lblTeacherTitle);

        QWidget* teacherListWidget = new QWidget;
        teacherListWidget->setStyleSheet("background-color: #606060;");
        teacherListLayout = new QVBoxLayout(teacherListWidget);
        teacherListLayout->setSpacing(8);
        // 为教师列表增加滚动区域，避免超出屏幕
        QScrollArea* teacherScrollArea = new QScrollArea;
        teacherScrollArea->setWidget(teacherListWidget);
        teacherScrollArea->setWidgetResizable(true);
        teacherScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        teacherScrollArea->setFrameShape(QFrame::NoFrame);
        teacherLayout->addWidget(teacherScrollArea);
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
            QAbstractButton* checked = classGroup->checkedButton();
            if (!checked) {
                qWarning() << "没有选中的教师";
                return;
            }

            // 不再发送 scheduleDialogNeeded 信号，ScheduleDialog 只在点击按钮时创建
            accept();
        });

        // 限制窗口高度，避免超过屏幕高度（保留约20%边距）
        if (QGuiApplication::primaryScreen()) {
            int maxH = static_cast<int>(QGuiApplication::primaryScreen()->availableGeometry().height() * 0.8);
            setMaximumHeight(maxH);
        }

        // 增加窗口和列表的最小高度，确保选项清晰可见
        setMinimumHeight(520);
        classScrollArea->setMinimumHeight(180);
        teacherScrollArea->setMinimumHeight(180);
    }

    void InitData(QSet<QString> setclassId)
    {
        UserInfo userInfo = CommonInfo::GetData();
        if (m_httpHandler)
        {
            QString url = "http://47.100.126.194:5000/friends?";
            url += "id_card=";
            url += userInfo.strIdNumber;
            m_httpHandler->get(url);
        }

        // 如已有学校ID，则拉取班级列表
        if (!userInfo.schoolId.isEmpty())
        {
            fetchClassesByPrefix(setclassId, userInfo.schoolId);
        }
    }

    QVector<QString> getNoticeMsg()
    {
        return m_NoticeMsg;
    }

    //// 辅助函数：为 ScheduleDialog 建立群聊退出信号连接
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
                this, &ClassTeacherDialog::onWebSocketMessage);
        }

        //socket = new QWebSocket();
        //connect(socket, &QWebSocket::connected, this, &ClassTeacherDialog::onConnected);
        //connect(socket, &QWebSocket::textMessageReceived, this, &ClassTeacherDialog::onMessageReceived);
        ////connect(btnOk, &QPushButton::clicked, this, &ClassTeacherDialog::sendBroadcast);
        connect(btnOk, &QPushButton::clicked, this, &ClassTeacherDialog::sendPrivateMessage);

        //UserInfo userinfo = CommonInfo::GetData();
        //// 建立连接
        //socket->open(QUrl(QString("ws://47.100.126.194:5000/ws/%1").arg(userinfo.teacher_unique_id)));

        //// 发送心跳
        //heartbeatTimer = new QTimer(this);
        //connect(heartbeatTimer, &QTimer::timeout, this, &ClassTeacherDialog::sendHeartbeat);
        //heartbeatTimer->start(5000); // 每 5 秒一次
    }

signals:
    void groupCreated(const QString& groupId); // 群组创建并上传成功信号，通知父窗口刷新群列表
    void scheduleDialogNeeded(const QString& groupId, const QString& groupName); // 需要创建/初始化ScheduleDialog的信号
    void scheduleDialogRefreshNeeded(const QString& groupId); // 需要刷新ScheduleDialog成员列表的信号

private:
    void clearLayout(QVBoxLayout* layout)
    {
        if (!layout) return;
        QLayoutItem* child;
        while ((child = layout->takeAt(0)) != nullptr)
        {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }
    }

protected:
    void enterEvent(QEvent* event) override
    {
        QDialog::enterEvent(event);
        if (m_visibleCloseButton && closeButton)
            closeButton->show();
    }

    void leaveEvent(QEvent* event) override
    {
        QDialog::leaveEvent(event);
        if (closeButton)
            closeButton->hide();
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QDialog::resizeEvent(event);
        if (closeButton)
            closeButton->move(this->width() - closeButton->width() - 4, 4);
        updateMask();
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        }
        QDialog::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
        }
        QDialog::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
        }
        QDialog::mouseReleaseEvent(event);
    }

    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor("#555555"));
        painter.setPen(Qt::NoPen);

        QPainterPath path;
        path.addRoundedRect(rect(), m_cornerRadius, m_cornerRadius);
        painter.drawPath(path);
    }


private:

    void fetchClassesByPrefix(QSet<QString> setclassId, const QString& schoolId)
    {
        if (schoolId.isEmpty()) return;
        QString prefix = schoolId;
        if (prefix.length() != 6 || !prefix.toInt()) {
            CustomMessageDialog::showMessage(this, QString::fromUtf8(u8"错误"), QString::fromUtf8(u8"请输入6位数字前缀"));
            return;
        }

        QJsonObject jsonObj;
        jsonObj["prefix"] = prefix;
        QJsonDocument doc(jsonObj);
        QByteArray reqData = doc.toJson(QJsonDocument::Compact);

        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QNetworkRequest request(QUrl("http://47.100.126.194:5000/getClassesByPrefix"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply* reply = manager->post(request, reqData);
        connect(reply, &QNetworkReply::finished, this, [this, reply, setclassId]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray response_data = reply->readAll();
                QJsonDocument respDoc = QJsonDocument::fromJson(response_data);
                if (respDoc.isObject()) {
                    QJsonObject root = respDoc.object();
                    QJsonObject dataObj = root.value("data").toObject();
                    int code = dataObj.value("code").toInt();
                    QString message = dataObj.value("message").toString();
                    if (code != 200) {
                        CustomMessageDialog::showMessage(this, QString::fromUtf8(u8"查询失败"), message);
                        reply->deleteLater();
                        return;
                    }
                    QJsonArray classes = dataObj.value("classes").toArray();
                    clearLayout(classListLayout);
                    bool first = true;
                    for (const QJsonValue& val : classes) {
                        QJsonObject cls = val.toObject();
                        QString stage = cls.value("school_stage").toString();
                        QString grade = cls.value("grade").toString();
                        QString className = cls.value("class_name").toString();
                        QString classCode = cls.value("class_code").toString();
                        if (setclassId.find(classCode) != setclassId.end())
                        {
                            continue;
                        }

                        // 展示名称：学段+年级+班名 或 仅班名
                        QString display = className.isEmpty() ? (grade.isEmpty() ? stage : (stage + grade)) : (stage + grade + className);
                        addPersonRow(classListLayout, ":/icons/avatar1.png", display, "", classCode, grade, className, classGroup, first);
                        if (first) first = false;
                    }
                }
            } else {
                CustomMessageDialog::showMessage(this, QString::fromUtf8(u8"网络错误"), reply->errorString());
            }
            reply->deleteLater();
        });
    }

    void addPersonRow(QVBoxLayout* parentLayout, const QString& iconPath, const QString& name, const QString phone, const QString teacher_unique_id,
        QString grade, QString class_taught, QButtonGroup* pBtnGroup, bool checked = false)
    {
        QHBoxLayout* rowLayout = new QHBoxLayout;
        rowLayout->setContentsMargins(10, 6, 10, 6);
        rowLayout->setSpacing(10);
        QRadioButton* radio = new QRadioButton;
        radio->setChecked(checked);
        radio->setProperty("phone", phone); // 可以是 int / QString / QVariant
        radio->setProperty("teacher_unique_id", teacher_unique_id); // 可以是 int / QString / QVariant 班级是班级id, 教师是教师id
        radio->setProperty("grade", grade);
        radio->setProperty("class_taught", class_taught);
        radio->setProperty("name", name);

        QLabel* avatar = new QLabel;
        avatar->setFixedSize(36, 36);
        avatar->setStyleSheet("background-color: #707070; border-radius: 18px;");
        // 如果有头像图片资源，可以这样设置：
        QPixmap pix(iconPath);
        avatar->setPixmap(pix.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        QLabel* lblName = new QLabel(name);
        lblName->setStyleSheet("color: #f5f5f5;");
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
        rowWidget->setStyleSheet("background-color: #5c5c5c;");
        parentLayout->addWidget(rowWidget);
    }
    private slots:
        // ChatDialog.cpp
        void onWebSocketMessage(const QString& msg)
        {
            qDebug() << " ClassTeacherDialog msg:" << msg; // 发信号;
            
            // 解析JSON消息
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &parseError);
            if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                // 不是JSON对象，按原逻辑处理
                m_NoticeMsg.push_back(msg);
                return;
            }
            
            QJsonObject rootObj = doc.object();
            QString type = rootObj.value(QStringLiteral("type")).toString();
            
            // 处理创建班级群返回消息（type: "3"）
            if (type == QStringLiteral("3")) {
                QString message = rootObj.value(QStringLiteral("message")).toString();
                QString groupId = rootObj.value(QStringLiteral("group_id")).toString();
                QString groupName = rootObj.value(QStringLiteral("groupname")).toString();
                
                if (!groupId.isEmpty()) {
                    qDebug() << "收到创建群组成功消息，群组ID:" << groupId << "，群组名称:" << groupName;
                    
                    // 解析并保存临时房间信息到全局存储
                    // 这样即使 ScheduleDialog 还没有打开，也能获取到临时房间信息
                    if (rootObj.contains(QStringLiteral("temp_room")) && rootObj.value(QStringLiteral("temp_room")).isObject()) {
                        QJsonObject tempRoomObj = rootObj.value(QStringLiteral("temp_room")).toObject();
                        
                        // 构建临时房间信息
                        TempRoomInfo tempRoomInfo;
                        tempRoomInfo.room_id = tempRoomObj.value(QStringLiteral("room_id")).toString();
                        tempRoomInfo.whip_url = tempRoomObj.value(QStringLiteral("whip_url")).toString();
                        tempRoomInfo.whep_url = tempRoomObj.value(QStringLiteral("whep_url")).toString();
                        tempRoomInfo.stream_name = tempRoomObj.value(QStringLiteral("stream_name")).toString();
                        tempRoomInfo.group_id = groupId;
                        tempRoomInfo.owner_id = tempRoomObj.value(QStringLiteral("owner_id")).toString();
                        tempRoomInfo.owner_name = tempRoomObj.value(QStringLiteral("owner_name")).toString();
                        tempRoomInfo.owner_icon = tempRoomObj.value(QStringLiteral("owner_icon")).toString();
                        
                        // 保存到全局存储
                        TempRoomStorage::saveTempRoomInfo(groupId, tempRoomInfo);
                        
                        qDebug() << "已保存临时房间信息到全局存储：";
                        qDebug() << "  群组ID:" << groupId;
                        qDebug() << "  房间ID:" << tempRoomInfo.room_id;
                        qDebug() << "  推流地址:" << tempRoomInfo.whip_url;
                        qDebug() << "  拉流地址:" << tempRoomInfo.whep_url;
                        qDebug() << "  流名称:" << tempRoomInfo.stream_name;
                    }
                    
                    // 显示成功消息
                    QString finalMessage = message.isEmpty() ? QString::fromUtf8(u8"群组创建成功！") : message;
                    if (!groupId.isEmpty()) {
                        finalMessage = QString("%1\n群组ID: %2").arg(finalMessage, groupId);
                    }
                    CustomMessageDialog::showMessage(this, QString::fromUtf8(u8"创建群组成功"), finalMessage);
                    
                    // 发出群组创建成功信号，通知父窗口刷新群列表
                    emit groupCreated(groupId);
                } else {
                    // 检查是否有错误信息
                    QString errorMsg = rootObj.value(QStringLiteral("error")).toString();
                    if (errorMsg.isEmpty()) {
                        errorMsg = rootObj.value(QStringLiteral("message")).toString();
                    }
                    if (errorMsg.isEmpty()) {
                        errorMsg = QString::fromUtf8(u8"创建群组失败：未知错误");
                    }
                    CustomMessageDialog::showMessage(this, QString::fromUtf8(u8"创建群组失败"), errorMsg);
                }
                return;
            }
            
            // 其他消息按原逻辑处理
            m_NoticeMsg.push_back(msg);
            // m_scheduleDlg 已移至 FriendGroupDialog 中管理，通知处理移至 FriendGroupDialog
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

            QAbstractButton* classChecked = classGroup->checkedButton();
            if (!classChecked)
            {
                qWarning() << "没有选中的班级";
                return;
            }

			// 取出按钮上绑定的私有数据
			QString phone = checked->property("phone").toString();
			QString teacher_unique_id = checked->property("teacher_unique_id").toString();
            QString grade = classChecked->property("grade").toString();
            QString class_taught = classChecked->property("class_taught").toString();
            QString teacher_name = checked->property("name").toString();
			qDebug() << "当前选中教师 Phone:" << phone << "  唯一编号:" << teacher_unique_id;

            QString class_unique_id = classChecked->property("teacher_unique_id").toString();

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
            QString groupName = grade + class_taught + "的班级群";
            
            // 使用腾讯SDK创建群组
            createGroupWithTIMSDK(groupName, teacher_unique_id, teacher_name, class_unique_id, userinfo);
            
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
    // 将TIMResult错误码转换为错误描述字符串
    static QString getTIMResultErrorString(int errorCode) {
        switch (errorCode) {
            case TIM_SUCC:
                return "成功";
            case TIM_ERR_SDKUNINIT:
                return "ImSDK未初始化";
            case TIM_ERR_NOTLOGIN:
                return "用户未登录";
            case TIM_ERR_JSON:
                return "错误的Json格式或Json Key - 请检查JSON参数格式是否正确，特别是字段名称和数据类型";
            case TIM_ERR_PARAM:
                return "参数错误 - 请检查传入的参数是否有效";
            case TIM_ERR_CONV:
                return "无效的会话";
            case TIM_ERR_GROUP:
                return "无效的群组";
            default:
                return QString("未知错误码: %1").arg(errorCode);
        }
    }
    
    // 通过WebSocket创建群组（由服务器调用腾讯IM并自动创建临时语音房间）
    void createGroupWithTIMSDK(const QString& groupName, const QString& teacherUniqueId, const QString teacher_name,
                               const QString& classUniqueId, const UserInfo& userinfo) {
        QString normalizedName = groupName.trimmed();
        if (normalizedName.isEmpty()) {
            CustomMessageDialog::showMessage(this, QString::fromUtf8(u8"错误"), QString::fromUtf8(u8"群组名称不能为空！"));
            return;
        }

        if (classUniqueId.isEmpty()) {
            CustomMessageDialog::showMessage(this, QString::fromUtf8(u8"错误"), QString::fromUtf8(u8"班级唯一标识为空，无法创建群组"));
            return;
        }

        if (!m_pWs) {
            CustomMessageDialog::showMessage(this, QString::fromUtf8(u8"错误"), QString::fromUtf8(u8"WebSocket未连接，无法创建群组"));
            return;
        }

        qint64 nowTs = QDateTime::currentDateTime().toSecsSinceEpoch();

        // 构建WebSocket消息（按照服务器要求的格式）
        QJsonObject wsMessage;
        wsMessage["type"] = "3";  // 创建群组消息类型

        // ========== 群组基本信息（必需） ==========
        wsMessage["group_name"] = normalizedName;
        wsMessage["group_type"] = "Meeting";  // 会议群
        wsMessage["owner_identifier"] = userinfo.classId;  // 创建者ID

        // ========== 班级信息（必需，用于创建班级群） ==========
        wsMessage["classid"] = classUniqueId;
        wsMessage["schoolid"] = userinfo.schoolId;
        wsMessage["is_class_group"] = 1;  // 1=班级群
        // 班级群唯一ID = 班级唯一ID + "01"
        QString classGroupUniqueId = classUniqueId + "01";
        wsMessage["group_id"] = classGroupUniqueId;
        qDebug() << "创建班级群 - 班级唯一ID:" << classUniqueId << "，班级群唯一ID:" << classGroupUniqueId;

        // ========== 群组详细信息（可选） ==========
        wsMessage["face_url"] = "/images/img_group.png";
        wsMessage["introduction"] = QString::fromUtf8(u8"班级群：%1").arg(normalizedName);
        wsMessage["notification"] = QString::fromUtf8(u8"欢迎加入%1").arg(normalizedName);
        wsMessage["max_member_num"] = 2000;
        wsMessage["searchable"] = 1;  // 可搜索
        wsMessage["visible"] = 1;  // 可见
        wsMessage["add_option"] = 0;  // 加群选项

        // ========== 群主信息（必需） ==========
        QJsonObject memberInfo;
        memberInfo["user_id"] = userinfo.classId;
        memberInfo["user_name"] = userinfo.strName;
        memberInfo["self_role"] = 400;  // 400=群主
        memberInfo["join_time"] = nowTs;
        memberInfo["msg_flag"] = 0;
        memberInfo["self_msg_flag"] = 0;
        memberInfo["readed_seq"] = 0;
        memberInfo["unread_num"] = 0;
        wsMessage["member_info"] = memberInfo;

        // ========== 其他成员信息（可选，管理员等） ==========
        QJsonArray members;
        if (!teacherUniqueId.isEmpty() && teacherUniqueId != userinfo.classId) {
            QJsonObject teacherMember;
            teacherMember["user_id"] = teacherUniqueId;  // 使用新字段名
            teacherMember["user_name"] = teacher_name;  // 使用新字段名
            teacherMember["group_role"] = 300;  // 300=管理员
            teacherMember["join_time"] = nowTs;
            teacherMember["msg_flag"] = 0;
            teacherMember["self_msg_flag"] = 0;
            teacherMember["readed_seq"] = 0;
            teacherMember["unread_num"] = 0;
            members.append(teacherMember);
        }
        if (!members.isEmpty()) {
            wsMessage["members"] = members;
        }

        // ========== 创建者额外信息（可选，用于临时语音群） ==========
        wsMessage["owner_name"] = userinfo.strName;
        if (!userinfo.strHeadImagePath.isEmpty()) {
            wsMessage["owner_icon"] = userinfo.strHeadImagePath;
        } else if (!userinfo.avatar.isEmpty()) {
            wsMessage["owner_icon"] = userinfo.avatar;
        }

        // 转换为JSON字符串
        QJsonDocument doc(wsMessage);
        QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

        qDebug() << "========== 通过WebSocket创建群组 ==========";
        qDebug() << "消息内容:" << jsonString;

        // 通过WebSocket发送消息，格式：to:目标ID:消息
        // 创建班级群不需要目标ID，使用 to::消息 格式（两个冒号，中间为空）
        QString message = QString("to::%1").arg(jsonString);
        TaQTWebSocket::sendPrivateMessage(message);

        qDebug() << "已通过WebSocket发送创建群组消息，格式: to::消息内容";
        // 注意：返回消息会在 onWebSocketMessage 中处理
    }
    
    // 上传群组信息到服务器
    void uploadGroupInfoToServer(const QString& groupId, const QString& groupName, const QString teacher_name,
                                 const QString& teacherUniqueId, const QString& classUniqueId,
                                 const UserInfo& userinfo, bool isClassGroup = true) {
        // 构造群组信息对象（参考FriendGroupDialog的格式）
        QJsonObject groupObj;
        
        // 群组基础信息
        groupObj["group_id"] = groupId;
        groupObj["group_name"] = groupName;
        groupObj["group_type"] = kTIMGroup_Public; // 公开群（支持设置管理员）
        groupObj["face_url"] = ""; // 创建时未设置头像
        groupObj["info_seq"] = 0;
        groupObj["latest_seq"] = 0;
        groupObj["is_shutup_all"] = false;
        
        // 群组详细信息
        groupObj["detail_group_id"] = groupId;
        groupObj["detail_group_name"] = groupName;
        groupObj["detail_group_type"] = kTIMGroup_Private; // 私有群（只有私有群可以直接拉用户入群）
        groupObj["detail_face_url"] = "";
        groupObj["create_time"] = QDateTime::currentDateTime().toSecsSinceEpoch(); // 当前时间戳
        groupObj["detail_info_seq"] = 0;
        groupObj["introduction"] = QString("班级群：%1").arg(groupName);
        groupObj["notification"] = QString("欢迎加入%1").arg(groupName);
        groupObj["last_info_time"] = QDateTime::currentDateTime().toSecsSinceEpoch();
        groupObj["last_msg_time"] = 0;
        groupObj["next_msg_seq"] = 0;
        // 计算成员数量：群主 + 被邀请的教师（如果有）
        int memberCount = 1; // 只有群主
        if (teacherUniqueId != userinfo.classId && !teacherUniqueId.isEmpty()) {
            memberCount = 2; // 群主 + 被邀请的教师
        }
        groupObj["member_num"] = memberCount;
        groupObj["max_member_num"] = 2000;
        groupObj["online_member_num"] = 0;
        groupObj["owner_identifier"] = userinfo.classId; // 群主ID
        groupObj["add_option"] = kTIMGroupAddOpt_Any;
        groupObj["visible"] = 2; // 默认可见
        groupObj["searchable"] = 2; // 默认可搜索
        groupObj["detail_is_shutup_all"] = false;
        // 添加自定义字段：班级ID和学校ID（这些字段会从腾讯IM SDK的自定义字段中读取）
        groupObj["classid"] = classUniqueId;
        groupObj["schoolid"] = userinfo.schoolId;
        // 添加 is_class_group 字段：1表示班级群，0表示普通群
        groupObj["is_class_group"] = isClassGroup ? 1 : 0;
        
        // 注意：腾讯IM SDK的自定义字段存储在 group_detial_info_custom_info 中
        // 可以通过以下方式访问：
        // QJsonArray customInfo = group[kTIMGroupDetialInfoCustomInfo].toArray();
        // 遍历 customInfo 数组，查找 key 为 "class_id" 和 "school_id" 的项
        
        // 用户在该群组中的信息（当前登录用户的信息，即群主信息）
        // 注意：member_info 是当前用户（上传者）在该群组中的信息
        QJsonObject memberInfo;
        memberInfo["user_id"] = userinfo.classId; // 当前用户的ID（群主）
        memberInfo["readed_seq"] = 0; // 刚创建群组，已读消息序列号为0
        memberInfo["msg_flag"] = 0; // 消息接收选项，默认为0
        memberInfo["join_time"] = QDateTime::currentDateTime().toSecsSinceEpoch(); // 加入时间（创建时间）
        memberInfo["self_role"] = (int)400; //kTIMMemberRole_Owner; // 群主角色，确保是整数类型 400是群主
        memberInfo["self_msg_flag"] = 0; // 自己的消息接收选项
        memberInfo["unread_num"] = 0; // 未读消息数，刚创建时为0
        memberInfo["user_name"] = userinfo.strName;
        
        groupObj["member_info"] = memberInfo;
        
        // 构造groups数组（当前用户/群主）
        QJsonArray groupsArray;
        groupsArray.append(groupObj);
        
        // 构造最终的上传JSON（当前用户/群主）
        QJsonObject uploadData;
        uploadData["classId"] = userinfo.classId; // 当前用户的ID（群主）
        uploadData["groups"] = groupsArray;
        
        // 转换为JSON字符串
        QJsonDocument uploadDoc(uploadData);
        QByteArray jsonData = uploadDoc.toJson(QJsonDocument::Compact);
        
        // 上传当前用户（群主）的群组信息到服务器
        QString url = "http://47.100.126.194:5000/groups/sync";
        
        // 创建一个临时连接来处理群组上传的响应
        // 使用 QPointer 来安全地管理连接，避免悬空指针
        QPointer<ClassTeacherDialog> self = this;
        QMetaObject::Connection* conn = new QMetaObject::Connection;
        *conn = connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
            // 检查响应是否包含群组信息（通过检查是否包含 "groups" 或 "group_id" 来判断）
            // 这里简单判断：如果响应包含我们上传的 groupId，就认为是群组上传的响应
            if (responseString.contains(groupId) || responseString.contains("\"code\":200") || responseString.contains("\"code\": 200")) {
                qDebug() << "群组信息上传成功，响应:" << responseString;
                
                // 断开临时连接
                if (conn && *conn) {
                    disconnect(*conn);
                    delete conn;
                }
                
                // 发出群组创建成功信号，通知父窗口刷新群列表
                if (self) {
                    emit self->groupCreated(groupId);
                }
            }
        }, Qt::UniqueConnection);
        
        m_httpHandler->post(url, jsonData);
        qDebug() << "上传群主群组信息到服务器，群组ID:" << groupId;
        qDebug() << "上传JSON:" << QString::fromUtf8(jsonData);
        
        // 如果有被邀请的成员，也需要为被邀请的成员创建群组信息并上传
        // 现在创建群组时会添加被邀请的教师作为管理员
        if (teacherUniqueId != userinfo.classId && !teacherUniqueId.isEmpty()) {
            // 为被邀请的成员创建群组信息对象
            QJsonObject invitedGroupObj = groupObj; // 复制群组基本信息
            
            // 创建被邀请成员的 member_info
            QJsonObject invitedMemberInfo;
            invitedMemberInfo["user_id"] = teacherUniqueId; // 被邀请成员的ID
            invitedMemberInfo["readed_seq"] = 0;
            invitedMemberInfo["msg_flag"] = 0;
            invitedMemberInfo["join_time"] = QDateTime::currentDateTime().toSecsSinceEpoch(); // 加入时间（创建时间）
            invitedMemberInfo["self_role"] = 300; // 管理员角色（Admin），群主是400，管理员是300
            invitedMemberInfo["self_msg_flag"] = 0;
            invitedMemberInfo["unread_num"] = 0;
            invitedMemberInfo["user_name"] = teacher_name;
            
            invitedGroupObj["member_info"] = invitedMemberInfo;
            
            // 构造被邀请成员的groups数组
            QJsonArray invitedGroupsArray;
            invitedGroupsArray.append(invitedGroupObj);
            
            // 构造被邀请成员的上传JSON
            QJsonObject invitedUploadData;
            invitedUploadData["user_id"] = teacherUniqueId; // 被邀请成员的ID
            invitedUploadData["groups"] = invitedGroupsArray;
            
            // 转换为JSON字符串
            QJsonDocument invitedUploadDoc(invitedUploadData);
            QByteArray invitedJsonData = invitedUploadDoc.toJson(QJsonDocument::Compact);
            
            // 上传被邀请成员的群组信息到服务器
            m_httpHandler->post(url, invitedJsonData);
            qDebug() << "上传被邀请成员群组信息到服务器，群组ID:" << groupId << "，成员ID:" << teacherUniqueId;
            qDebug() << "上传JSON:" << QString::fromUtf8(invitedJsonData);
        }
    }

private:
    TAHttpHandler* m_httpHandler = NULL;
    QVBoxLayout* teacherListLayout = NULL;
    QVBoxLayout* classListLayout = NULL;
    QButtonGroup* sexGroup = new QButtonGroup(this);
    QButtonGroup* classGroup = NULL;
    QString m_schoolId;
    //QWebSocket* socket = NULL;
    //QTimer* heartbeatTimer;
    QPushButton* btnOk = NULL;
    QPushButton* btnCancel = NULL;
    QVector<QString> m_NoticeMsg;
    TaQTWebSocket* m_pWs = NULL;
    QPushButton* closeButton = nullptr;
    bool m_visibleCloseButton = true;
    bool m_dragging = false;
    QPoint m_dragStartPos;
    int m_cornerRadius = 16;

    void updateMask()
    {
        QPainterPath path;
        path.addRoundedRect(rect(), m_cornerRadius, m_cornerRadius);
        setMask(QRegion(path.toFillPolygon().toPolygon()));
    }
};
