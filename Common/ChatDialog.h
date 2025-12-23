#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QListWidget>
#include <QListWidgetItem>
#include <QList>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPainterPath>
#include <QRegion>
#include <QResizeEvent>
#include <QPixmap>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTimer>
#include <QDateTime>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonParseError>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSslError>
#include <QStandardPaths>
#include <QDir>
#include <QProgressBar>
#include <QMap>
#include <QPair>
#include <QAudioInput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QIODevice>
#include <QMediaPlayer>
#include <QMediaContent>
#include "TaQTWebSocket.h"
#include "CommonInfo.h"
#include "ImSDK/includes/TIMCloud.h"
#include "ImSDK/includes/TIMCloudDef.h"
#include "ImSDK/includes/TIMCloudCallback.h"
#include "GroupNotifyDialog.h"
#include "QGroupInfo.h"

struct ChatMessage {
    QString avatarPath;
    QString senderName;
    QString messageText;
    bool isMine;
    QDateTime time;
};

class ClickableLabelEx : public QLabel {
    Q_OBJECT
public:
    explicit ClickableLabelEx(QWidget* parent = nullptr) : QLabel(parent) {}
signals:
    void clicked();
    void doubleClicked();
protected:
    void mousePressEvent(QMouseEvent* event) override {
        emit clicked();
        QLabel::mousePressEvent(event);
    }
    void mouseDoubleClickEvent(QMouseEvent* event) override {
        emit doubleClicked();
        QLabel::mouseDoubleClickEvent(event);
    }
};

class ChatDialog : public QDialog
{
    Q_OBJECT
signals:
    // 当前用户退出/群解散（用于通知外部刷新列表/关闭窗口）
    void groupLeft(const QString& groupId);
    void groupDismissed(const QString& groupId);

    // 兼容：普通群专用信号
    void normalGroupLeft(const QString& groupId);
    void normalGroupDismissed(const QString& groupId);
public:
    ChatDialog(QWidget* parent = nullptr, TaQTWebSocket* pWs = NULL) : QDialog(parent)
    {
        setWindowTitle("多类型聊天对话框");
        resize(500, 660);

        // 圆角无边框窗口（普通群弹窗需要四角椭圆/圆角效果）
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        m_cornerRadius = 16;
        updateMask();

        m_pWs = pWs;

        // 外层容器：负责背景/圆角边框
        QVBoxLayout* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);

        m_container = new QFrame(this);
        m_container->setObjectName("chatContainer");
        m_container->setStyleSheet(
            "#chatContainer {"
            "  background-color: #1E1F22;"
            "  border: 1px solid rgba(255,255,255,0.14);"
            "  border-radius: 16px;"
            "}"
        );
        rootLayout->addWidget(m_container);

        // 容器内布局：顶部栏 + 内容区（列表/输入）
        QVBoxLayout* mainLayout = new QVBoxLayout(m_container);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // 顶部栏（更像微信：单独背景 + 分割线 + 圆角顶边）
        m_titleBar = new QFrame(m_container);
        m_titleBar->setObjectName("chatTitleBar");
        m_titleBar->setFixedHeight(48);
        m_titleBar->setStyleSheet(
            "#chatTitleBar {"
            "  background-color: #1E1F22;"
            "  border-bottom: 1px solid rgba(255,255,255,0.10);"
            "  border-top-left-radius: 16px;"
            "  border-top-right-radius: 16px;"
            "}"
        );

        QHBoxLayout* titleLayout = new QHBoxLayout(m_titleBar);
        titleLayout->setContentsMargins(12, 0, 12, 0);
        titleLayout->setSpacing(8);

        // 为了让标题真正居中：左侧放一个占位宽度（等于关闭按钮宽度）
        m_titleLeftSpacer = new QWidget(m_titleBar);
        // 默认按普通群（m_isClassGroup=false）显示“...”，右侧为“...+×”
        m_titleLeftSpacer->setFixedWidth(60);

        m_titleLabel = new QLabel(windowTitle(), m_titleBar);
        m_titleLabel->setAlignment(Qt::AlignCenter);
        // 标题文字 + 背景色（按 UI 设计稿/截图）
        // 若希望整个标题栏都同色，请同步修改 m_titleBar 的 background-color
        m_titleLabel->setStyleSheet("background: transparent; color:#ffffff; font-size:14px; font-weight:600;");
        m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        m_btnMore = new QPushButton(QStringLiteral("..."), m_titleBar);
        m_btnMore->setFixedSize(26, 26);
        m_btnMore->setCursor(Qt::PointingHandCursor);
        m_btnMore->setStyleSheet(
            "QPushButton { background-color: rgba(255,255,255,0.06); color: rgba(255,255,255,0.92); border: 1px solid rgba(255,255,255,0.10); border-radius: 8px; font-weight: 800; }"
            "QPushButton:hover { background-color: rgba(255,255,255,0.10); }"
        );

        m_btnClose = new QPushButton(QStringLiteral("×"), m_titleBar);
        m_btnClose->setFixedSize(26, 26);
        m_btnClose->setCursor(Qt::PointingHandCursor);
        m_btnClose->setStyleSheet(
            "QPushButton { background-color: rgba(255,255,255,0.06); color: rgba(255,255,255,0.92); border: 1px solid rgba(255,255,255,0.10); border-radius: 8px; font-weight: 900; }"
            "QPushButton:hover { background-color: rgba(255,255,255,0.10); }"
        );
        connect(m_btnClose, &QPushButton::clicked, this, &QDialog::reject);

        // 默认普通群显示“...”按钮（由 setGroupContext 决定）
        m_btnMore->setVisible(true);
        connect(m_btnMore, &QPushButton::clicked, this, [this]() {
            if (m_unique_group_id.trimmed().isEmpty()) return;
            // 群管理弹窗：班级群进入“班级管理”，普通群进入“群管理”
            QGroupInfo* dlg = new QGroupInfo(this);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            const QString classIdForDlg = m_isClassGroup ? m_classId : QString();
            dlg->initData(this->windowTitle(), m_unique_group_id, m_iGroupOwner, classIdForDlg);

            if (!m_isClassGroup) {
                // 普通群：优先用腾讯IM SDK拉成员（无需管理员REST鉴权）；同时先渲染一次，避免成员区空白
                dlg->InitGroupMember(m_unique_group_id, QVector<GroupMemberInfo>());
                dlg->fetchGroupMemberListFromSDK(m_unique_group_id);
            } else {
                // 班级群：成员列表来自自家服务器（/groups/members），不走腾讯 SDK
                QPointer<QGroupInfo> dlgPtr = dlg;
                QNetworkAccessManager* manager = new QNetworkAccessManager(dlg);
                QUrl url("http://47.100.126.194:5000/groups/members");
                QUrlQuery query;
                query.addQueryItem("group_id", m_unique_group_id);
                url.setQuery(query);

                QNetworkRequest req(url);
                QNetworkReply* reply = manager->get(req);
                connect(reply, &QNetworkReply::finished, dlg, [reply, manager, dlgPtr, gid = m_unique_group_id]() {
                    const QByteArray raw = reply->readAll();
                    const QNetworkReply::NetworkError err = reply->error();
                    reply->deleteLater();
                    manager->deleteLater();

                    if (!dlgPtr) return;
                    if (err != QNetworkReply::NoError) {
                        qWarning() << "班级群获取成员失败:" << gid << err << reply->errorString();
                        return;
                    }

                    QJsonParseError pe;
                    QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
                    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
                        qWarning() << "班级群成员响应JSON解析失败:" << gid << pe.errorString();
                        return;
                    }

                    QJsonObject root = doc.object();
                    QJsonObject dataObj = root.value("data").toObject();
                    if (dataObj.isEmpty()) dataObj = root;

                    const QString groupId = dataObj.value("group_id").toString(gid);
                    const QJsonArray membersArr = dataObj.value("members").toArray();

                    QVector<GroupMemberInfo> members;
                    members.reserve(membersArr.size());

                    for (const auto& v : membersArr) {
                        if (!v.isObject()) continue;
                        const QJsonObject memberObj = v.toObject();

                        QString member_id = memberObj.value("user_id").toString();
                        QString member_name = memberObj.value("user_name").toString();
                        if (member_name.isEmpty()) member_name = memberObj.value("member_name").toString();
                        if (member_id.isEmpty()) continue;
                        if (member_name.isEmpty()) member_name = member_id;

                        int self_role = memberObj.value("self_role").toInt();
                        QString member_role = QStringLiteral("成员");
                        if (self_role == 400) member_role = QStringLiteral("群主");

                        if (memberObj.contains("role")) {
                            const QString roleStr = memberObj.value("role").toString();
                            if (roleStr == "owner" || roleStr == "Owner" || roleStr == "群主") {
                                member_role = QStringLiteral("群主");
                            }
                        }

                        bool is_voice_enabled = false;
                        if (memberObj.contains("is_voice_enabled")) {
                            is_voice_enabled = memberObj.value("is_voice_enabled").toInt();
                        }

                        QStringList teachSubjects;
                        if (memberObj.contains("teach_subjects") && memberObj.value("teach_subjects").isArray()) {
                            const QJsonArray subjectArr = memberObj.value("teach_subjects").toArray();
                            for (const auto& sv : subjectArr) {
                                const QString s = sv.toString().trimmed();
                                if (!s.isEmpty()) teachSubjects.append(s);
                            }
                        }

                        GroupMemberInfo info;
                        info.member_id = member_id;
                        info.member_name = member_name;
                        info.member_role = member_role;
                        info.is_voice_enabled = is_voice_enabled;
                        info.teach_subjects = teachSubjects;
                        members.append(info);
                    }

                    dlgPtr->InitGroupMember(groupId, members);
                });
            }

            // 退出/解散后：关闭聊天窗口，并通知外部刷新群列表
            connect(dlg, &QGroupInfo::groupDismissed, this, [this](const QString& groupId) {
                emit groupDismissed(groupId);
                emit normalGroupDismissed(groupId);
                this->close();
            }, Qt::UniqueConnection);
            connect(dlg, &QGroupInfo::memberLeftGroup, this, [this](const QString& groupId, const QString& leftUserId) {
                UserInfo ui = CommonInfo::GetData();
                if (leftUserId == ui.classId) {
                    emit groupLeft(groupId);
                    emit normalGroupLeft(groupId);
                    this->close();
                }
            }, Qt::UniqueConnection);

            dlg->show();
            dlg->raise();
            dlg->activateWindow();
        });

        titleLayout->addWidget(m_titleLeftSpacer);
        titleLayout->addWidget(m_titleLabel);
        titleLayout->addWidget(m_btnMore);
        titleLayout->addWidget(m_btnClose);

        mainLayout->addWidget(m_titleBar);

        // 内容区（左右内边距一致）
        m_contentWidget = new QWidget(m_container);
        QVBoxLayout* contentLayout = new QVBoxLayout(m_contentWidget);
        contentLayout->setContentsMargins(12, 10, 12, 10);
        contentLayout->setSpacing(10);

        // 消息列表
        m_listWidget = new QListWidget();
        // 彻底关闭“行背景/交替行”导致的灰条，并让 viewport 透明显示容器背景
        m_listWidget->setAlternatingRowColors(false);
        m_listWidget->setAutoFillBackground(false);
        m_listWidget->setAttribute(Qt::WA_TranslucentBackground, true);
        if (m_listWidget->viewport()) {
            m_listWidget->viewport()->setAutoFillBackground(false);
            m_listWidget->viewport()->setAttribute(Qt::WA_TranslucentBackground, true);
            m_listWidget->viewport()->setStyleSheet("background: transparent;");
        }
        m_listWidget->setStyleSheet(
            "QListWidget { background: transparent; border:none; color:#ffffff; outline: none; }"
            // item 背景强制透明，避免 setItemWidget 后出现“整行灰底条”（尤其是名字行所在区域）
            "QListWidget::item { background: transparent; border:none; }"
            "QListWidget::item:selected { background: transparent; }"
            "QListWidget::item:hover { background: transparent; }"
            "QScrollBar:vertical { background: transparent; width: 8px; margin: 6px 2px 6px 2px; }"
            "QScrollBar::handle:vertical { background: rgba(255,255,255,0.18); border-radius: 4px; min-height: 24px; }"
            "QScrollBar::handle:vertical:hover { background: rgba(255,255,255,0.26); }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        );
        m_listWidget->setFrameShape(QFrame::NoFrame);
        m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        contentLayout->addWidget(m_listWidget, 1);

        // 输入区
        QHBoxLayout* inputLayout = new QHBoxLayout();
        inputLayout->setContentsMargins(0, 0, 0, 0);
        inputLayout->setSpacing(8);
        m_lineEdit = new QLineEdit();
        m_lineEdit->setPlaceholderText("请输入文字...");
        m_lineEdit->setMinimumHeight(36);
        m_lineEdit->setStyleSheet(
            "QLineEdit {"
            "  background-color: rgba(255,255,255,0.06);"
            "  border: 1px solid rgba(255,255,255,0.10);"
            "  border-radius: 18px;"
            "  padding: 8px 12px;"
            "  color: #ffffff;"
            "}"
            "QLineEdit:focus { border: 1px solid rgba(255,255,255,0.18); }"
        );

        QPushButton* btnSend = new QPushButton("发送");
        btnSend->setFixedSize(60, 36);
        btnSend->setCursor(Qt::PointingHandCursor);
        btnSend->setStyleSheet(
            "QPushButton { background-color: #2D7DFF; color: #ffffff; border: none; border-radius: 18px; font-weight: 700; }"
            "QPushButton:hover { background-color: #3A8BFF; }"
        );

        QPushButton* btnImage = new QPushButton("📷");
        btnImage->setFixedSize(36, 36);
        btnImage->setCursor(Qt::PointingHandCursor);
        btnImage->setStyleSheet("QPushButton { background-color: rgba(255,255,255,0.06); color: white; border: 1px solid rgba(255,255,255,0.10); border-radius: 18px; } QPushButton:hover { background-color: rgba(255,255,255,0.10); }");

        QPushButton* btnFile = new QPushButton("📎");
        btnFile->setFixedSize(36, 36);
        btnFile->setCursor(Qt::PointingHandCursor);
        btnFile->setStyleSheet("QPushButton { background-color: rgba(255,255,255,0.06); color: white; border: 1px solid rgba(255,255,255,0.10); border-radius: 18px; } QPushButton:hover { background-color: rgba(255,255,255,0.10); }");

        QPushButton* btnVoice = new QPushButton("🎤");
        btnVoice->setFixedSize(36, 36);
        btnVoice->setCursor(Qt::PointingHandCursor);
        btnVoice->setStyleSheet("QPushButton { background-color: rgba(255,255,255,0.06); color: white; border: 1px solid rgba(255,255,255,0.10); border-radius: 18px; } QPushButton:hover { background-color: rgba(255,255,255,0.10); }");

        inputLayout->addWidget(m_lineEdit);
        inputLayout->addWidget(btnSend);
        inputLayout->addWidget(btnImage);
        inputLayout->addWidget(btnFile);
        inputLayout->addWidget(btnVoice);

        m_inputBar = new QFrame(m_contentWidget);
        m_inputBar->setObjectName("chatInputBar");
        m_inputBar->setStyleSheet(
            "#chatInputBar {"
            "  background-color: rgba(255,255,255,0.04);"
            "  border: 1px solid rgba(255,255,255,0.08);"
            "  border-radius: 22px;"
            "  padding: 8px;"
            "}"
        );
        QHBoxLayout* inputBarLayout = new QHBoxLayout(m_inputBar);
        inputBarLayout->setContentsMargins(10, 6, 10, 6);
        inputBarLayout->setSpacing(8);
        inputBarLayout->addLayout(inputLayout);
        contentLayout->addWidget(m_inputBar);
        
        // 录音波形显示区域（初始隐藏）
        m_voiceWaveformWidget = new QWidget(m_container);
        m_voiceWaveformWidget->setFixedHeight(40);
        m_voiceWaveformWidget->setStyleSheet("background-color: rgba(255,255,255,0.06); border: 1px solid rgba(255,255,255,0.12); border-radius: 8px;");
        m_voiceWaveformWidget->hide();
        
        QHBoxLayout* waveformLayout = new QHBoxLayout(m_voiceWaveformWidget);
        waveformLayout->setContentsMargins(10, 5, 10, 5);
        waveformLayout->setSpacing(10);
        
        QLabel* waveformLabel = new QLabel("🎤 录音中...", m_voiceWaveformWidget);
        waveformLabel->setStyleSheet("color: #666; font-size: 12px;");
        waveformLayout->addWidget(waveformLabel);
        
        // 波形显示进度条
        m_voiceWaveformBar = new QProgressBar(m_voiceWaveformWidget);
        m_voiceWaveformBar->setFixedHeight(20);
        m_voiceWaveformBar->setRange(0, 100);
        m_voiceWaveformBar->setValue(0);
        m_voiceWaveformBar->setTextVisible(false);
        m_voiceWaveformBar->setStyleSheet(
            "QProgressBar {"
            "    border: 1px solid #ccc;"
            "    border-radius: 10px;"
            "    background-color: #e0e0e0;"
            "}"
            "QProgressBar::chunk {"
            "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
            "        stop:0 #4CAF50, stop:0.5 #8BC34A, stop:1 #FFC107);"
            "    border-radius: 10px;"
            "}"
        );
        waveformLayout->addWidget(m_voiceWaveformBar, 1);
        
        QLabel* durationLabel = new QLabel("0s", m_voiceWaveformWidget);
        durationLabel->setStyleSheet("color: #666; font-size: 12px; min-width: 30px;");
        durationLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_voiceDurationLabel = durationLabel;
        waveformLayout->addWidget(durationLabel);
        
        contentLayout->addWidget(m_voiceWaveformWidget);

        connect(btnSend, &QPushButton::clicked, this, &ChatDialog::sendMyTextMessage);
        connect(m_lineEdit, &QLineEdit::returnPressed, this, &ChatDialog::sendMyTextMessage);
        connect(btnImage, &QPushButton::clicked, this, &ChatDialog::sendMyImageMessage);
        connect(btnFile, &QPushButton::clicked, this, &ChatDialog::sendMyFileMessage);
        // 按住按钮开始录音，松开按钮停止录音并发送
        connect(btnVoice, &QPushButton::pressed, this, &ChatDialog::startVoiceRecording);
        connect(btnVoice, &QPushButton::released, this, &ChatDialog::stopVoiceRecordingAndSend);

        // 标题跟随 windowTitle 变化（外部 setWindowTitle 时同步到顶部栏）
        connect(this, &QWidget::windowTitleChanged, this, [=](const QString& t) {
            if (m_titleLabel) m_titleLabel->setText(t);
        });

        // 线上使用：不插入任何演示消息，历史消息由 InitData() -> loadHistoryMessages() 加载

        mainLayout->addWidget(m_contentWidget);
    }

    void InitData(QString unique_group_id, bool iGroupOwner)
    {
        const bool groupChanged = (m_unique_group_id != unique_group_id);
        m_unique_group_id = unique_group_id;
        m_iGroupOwner = iGroupOwner;
        
        // 注册到静态实例列表
        registerInstance();
        
        // 加载历史消息：避免同一群重复 InitData 时反复加载导致重复显示
        if (groupChanged) {
            // 群切换时可在此清理 UI（目前业务里一个窗口只对应一个群）
            if (m_listWidget) m_listWidget->clear();
            m_historyLoaded = false;
        }
        if (!m_historyLoaded) {
            loadHistoryMessages();
            m_historyLoaded = true;
        }
    }

    // 由外部设置群类型上下文（ChatDialog 同时被班级群/普通群复用）
    void setGroupContext(const QString& classid, bool isClassGroup)
    {
        m_classId = classid;
        m_isClassGroup = isClassGroup;

        // UI：普通群显示“...”，班级群隐藏“...”
        if (m_btnMore) {
            m_btnMore->setVisible(!m_isClassGroup);
        }
        // 标题居中占位：右侧如果有“...+×”则占位 60，否则占位 26
        if (m_titleLeftSpacer) {
            m_titleLeftSpacer->setFixedWidth(m_isClassGroup ? 26 : 60);
        }
    }
    
    ~ChatDialog()
    {
        // 从静态实例列表中移除
        unregisterInstance();
    }

    // 供外部（如 ScheduleDialog/FriendGroupDialog）调用的公共接口
    void InitWebSocket()
    {
        /*TaQTWebSocket::regRecvDlg(this);
        if (m_pWs)
        {
            connect(m_pWs, &TaQTWebSocket::newMessage,
                this, &ChatDialog::onWebSocketMessage);
        }*/
    }

    void setNoticeMsg(QList<Notification> listNoticeMsg)
    {
        UserInfo userInfo = CommonInfo::GetData();
        for (auto iter : listNoticeMsg)
        {
            if (QString::number(iter.sender_id) == userInfo.classId)
            {
                addTextMessage(":/res/img/home.png", iter.sender_name, iter.content, true);
            }
            else
            {
                addTextMessage(":/res/img/home.png", iter.sender_name, iter.content, false);
            }
        }
    }

    // 静态方法：提前注册消息回调（应在登录前调用，确保能接收到离线消息）
    static void ensureCallbackRegistered()
    {
        static bool s_callbackRegistered = false;
        if (!s_callbackRegistered) {
            TIMAddRecvNewMsgCallback(staticRecvNewMsgCallback, nullptr);
            s_callbackRegistered = true;
            qDebug() << "ChatDialog: 消息接收回调已注册（登录前）";
        }
    }

protected:
    void resizeEvent(QResizeEvent* e) override
    {
        QDialog::resizeEvent(e);
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
        m_dragging = false;
        QDialog::mouseReleaseEvent(event);
    }

private:
    void updateMask()
    {
        const int r = qMax(0, m_cornerRadius);
        QPainterPath path;
        path.addRoundedRect(rect(), r, r);
        setMask(QRegion(path.toFillPolygon().toPolygon()));
    }
    
private slots:
    void sendMyTextMessage()
    {
        QString text = m_lineEdit->text().trimmed();
        if (text.isEmpty() || m_unique_group_id.isEmpty()) return;

        UserInfo userinfo = CommonInfo::GetData();
        
        // 先显示在界面上（乐观更新）
        addTextMessage(":/res/img/home.png", userinfo.strName, text, true);
        m_lineEdit->clear();

        // 使用腾讯SDK发送消息
        sendTextMessageViaTIMSDK(text, userinfo);
    }

    //// ChatDialog.cpp
    //void onWebSocketMessage(const QString& msg)
    //{
    //    qDebug() << " ClassTeacherDialog msg:" << msg; // 发信号;
    //    // 这里可以解析 JSON 或直接追加到聊天窗口
    //    //addTextMessage(":/avatar_teacher.png", "对方", msg, false);
    //    //m_NoticeMsg.push_back(msg);

    //    QJsonParseError parseError;
    //    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &parseError);

    //    if (parseError.error != QJsonParseError::NoError) {
    //        qDebug() << "JSON parse error:" << parseError.errorString();
    //    }
    //    else {
    //        if (doc.isObject()) {
    //            QJsonObject obj = doc.object();
    //            if (obj["data"].isArray())
    //            {
    //                // 4. 取出数组
    //                QJsonArray arr = obj["data"].toArray();
    //                // 5. 遍历数组
    //                for (const QJsonValue& value : arr) {
    //                    if (value.isObject()) { // 每个元素是一个对象
    //                        QJsonObject obj = value.toObject();
    //                        int qSender_id = obj["sender_id"].toInt();
    //                        QString strSender_id = QString("%1").arg(qSender_id, 6, 10, QChar('0'));
    //                        QString SenderName = obj["sender_name"].toString();
    //                        int iContent_text = obj["content_text"].toInt();
    //                        QString updated_at = obj["updated_at"].toString();
    //                        int is_agreed = obj["is_agreed"].toInt();
    //                        QString content = obj["content"].toString();
    //                        QString GroupName = obj["group_name"].toString();
    //                        if (1 != iContent_text && 2 != iContent_text && 3 != iContent_text && 4 != iContent_text)
    //                        {
    //                            addTextMessage(":/avatar_me.png", "我", content, true);
    //                        }
    //                    }
    //                }
    //            }
    //        }
    //    }
    //}

    void sendMyImageMessage()
    {
        if (m_unique_group_id.isEmpty()) {
            QMessageBox::warning(this, "错误", "群组ID未设置，无法发送图片！");
            return;
        }
        
        UserInfo userinfo = CommonInfo::GetData();
        QString imgPath = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.jpg *.jpeg *.bmp)");
        if (!imgPath.isEmpty()) {
            // 先显示在界面上
            addImageMessage(":/res/img/home.png", userinfo.strName, imgPath, true);
            // 使用腾讯SDK发送图片
            sendImageMessageViaTIMSDK(imgPath, userinfo);
        }
    }

    void sendMyFileMessage()
    {
        if (m_unique_group_id.isEmpty()) {
            QMessageBox::warning(this, "错误", "群组ID未设置，无法发送文件！");
            return;
        }
        
        UserInfo userinfo = CommonInfo::GetData();
        QString filePath = QFileDialog::getOpenFileName(this, "选择文件");
        if (!filePath.isEmpty()) {
            // 先显示在界面上（带进度条），进度条和标签在 addFileMessage 中创建
            QPair<QProgressBar*, QLabel*> progressWidgets = addFileMessageWithProgress(":/res/img/home.png", userinfo.strName, filePath, true);
            
            // 使用腾讯SDK发送文件（传入进度条和状态标签）
            sendFileMessageViaTIMSDK(filePath, userinfo, progressWidgets.first, progressWidgets.second);
        }
    }

    // 开始实时录音
    void startVoiceRecording()
    {
        if (m_unique_group_id.isEmpty()) {
            QMessageBox::warning(this, "错误", "群组ID未设置，无法发送语音！");
            return;
        }
        
        // 检查是否正在录音
        if (m_audioInput && m_audioInput->state() == QAudio::ActiveState) {
            qDebug() << "已经在录音中";
            return;
        }
        
        // 创建录音文件路径
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir dir;
        if (!dir.exists(cacheDir)) {
            dir.mkpath(cacheDir);
        }
        
        m_currentVoiceFile = QDir(cacheDir).filePath(QString("voice_%1.wav").arg(QDateTime::currentMSecsSinceEpoch()));
        m_voiceRecordFile = new QFile(m_currentVoiceFile, this);
        
        if (!m_voiceRecordFile->open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, "错误", "无法创建录音文件！");
            delete m_voiceRecordFile;
            m_voiceRecordFile = nullptr;
            return;
        }
        
        // 设置音频格式
        QAudioFormat format;
        format.setSampleRate(16000); // 16kHz采样率
        format.setChannelCount(1);   // 单声道
        format.setSampleSize(16);    // 16位
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::SignedInt); // 使用有符号整数，便于计算振幅
        
        // 检查格式是否支持
        QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
        if (!info.isFormatSupported(format)) {
            qDebug() << "默认格式不支持，使用最接近的格式";
            format = info.nearestFormat(format);
        }
        
        // 保存音频格式，用于写入WAV头
        m_audioFormat = format;
        
        // 写入WAV文件头（文件大小位置先留空，录音结束后更新）
        writeWavHeader(m_voiceRecordFile, format, 0);
        
        // 记录PCM数据开始位置
        m_pcmDataStartPos = m_voiceRecordFile->pos();
        
        // 创建音频输入
        if (m_audioInput) {
            delete m_audioInput;
        }
        m_audioInput = new QAudioInput(format, this);
        
        // 创建用于读取音频数据的设备（用于计算音量）
        m_audioInputDevice = m_audioInput->start();
        if (!m_audioInputDevice) {
            QMessageBox::warning(this, "错误", "无法启动音频输入设备！");
            delete m_audioInput;
            m_audioInput = nullptr;
            if (m_voiceRecordFile) {
                m_voiceRecordFile->close();
                delete m_voiceRecordFile;
                m_voiceRecordFile = nullptr;
            }
            return;
        }
        
        // 同时将数据写入文件
        connect(m_audioInputDevice, &QIODevice::readyRead, this, [this]() {
            if (m_voiceRecordFile && m_audioInputDevice) {
                QByteArray data = m_audioInputDevice->readAll();
                if (m_voiceRecordFile->isOpen()) {
                    m_voiceRecordFile->write(data);
                    
                    // 计算音频音量（振幅）
                    updateVoiceWaveform(data);
                }
            }
        });
        
        // 记录开始时间
        m_voiceRecordStartTime = QDateTime::currentMSecsSinceEpoch();
        
        // 显示波形显示区域
        if (m_voiceWaveformWidget) {
            m_voiceWaveformWidget->show();
        }
        if (m_voiceWaveformBar) {
            m_voiceWaveformBar->setValue(0);
        }
        if (m_voiceDurationLabel) {
            m_voiceDurationLabel->setText("0s");
        }
        
        // 启动定时器更新录音时长
        if (!m_voiceWaveformTimer) {
            m_voiceWaveformTimer = new QTimer(this);
            connect(m_voiceWaveformTimer, &QTimer::timeout, this, &ChatDialog::updateVoiceRecordingDuration);
        }
        m_voiceWaveformTimer->start(100); // 每100ms更新一次
        
        qDebug() << "开始录音，保存到:" << m_currentVoiceFile;
        qDebug() << "音频格式:" << format.sampleRate() << "Hz," << format.channelCount() << "声道," << format.sampleSize() << "bit";
    }
    
    // 更新音频波形显示
    void updateVoiceWaveform(const QByteArray& audioData)
    {
        if (!m_voiceWaveformBar || audioData.isEmpty()) {
            return;
        }
        
        // 计算音频数据的平均振幅
        // 对于16位PCM，每个样本是2字节
        int sampleCount = audioData.size() / 2;
        if (sampleCount == 0) return;
        
        qint64 sum = 0;
        const qint16* samples = reinterpret_cast<const qint16*>(audioData.constData());
        
        for (int i = 0; i < sampleCount; i++) {
            qint16 sample = qAbs(samples[i]);
            sum += sample;
        }
        
        // 计算平均振幅并转换为0-100的百分比
        qint64 averageAmplitude = sum / sampleCount;
        int volume = (int)((averageAmplitude * 100) / 32768); // 16位音频最大值为32768
        
        // 限制在0-100范围内
        volume = qBound(0, volume, 100);
        
        // 更新进度条
        m_voiceWaveformBar->setValue(volume);
    }
    
    // 更新录音时长显示
    void updateVoiceRecordingDuration()
    {
        if (!m_voiceDurationLabel || m_voiceRecordStartTime == 0) {
            return;
        }
        
        qint64 duration = QDateTime::currentMSecsSinceEpoch() - m_voiceRecordStartTime;
        int seconds = (int)(duration / 1000);
        m_voiceDurationLabel->setText(QString("%1s").arg(seconds));
    }
    
    // 写入WAV文件头
    void writeWavHeader(QFile* file, const QAudioFormat& format, quint32 dataSize)
    {
        if (!file || !file->isOpen()) {
            return;
        }
        
        // WAV文件头结构
        // RIFF头（12字节）
        file->write("RIFF", 4); // ChunkID
        quint32 fileSize = 36 + dataSize; // ChunkSize = 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
        file->write(reinterpret_cast<const char*>(&fileSize), 4);
        file->write("WAVE", 4); // Format
        
        // fmt子块（24字节）
        file->write("fmt ", 4); // Subchunk1ID
        quint32 subchunk1Size = 16; // PCM格式为16
        file->write(reinterpret_cast<const char*>(&subchunk1Size), 4);
        quint16 audioFormat = 1; // PCM = 1
        file->write(reinterpret_cast<const char*>(&audioFormat), 2);
        quint16 numChannels = format.channelCount();
        file->write(reinterpret_cast<const char*>(&numChannels), 2);
        quint32 sampleRate = format.sampleRate();
        file->write(reinterpret_cast<const char*>(&sampleRate), 4);
        quint32 byteRate = sampleRate * numChannels * (format.sampleSize() / 8);
        file->write(reinterpret_cast<const char*>(&byteRate), 4);
        quint16 blockAlign = numChannels * (format.sampleSize() / 8);
        file->write(reinterpret_cast<const char*>(&blockAlign), 2);
        quint16 bitsPerSample = format.sampleSize();
        file->write(reinterpret_cast<const char*>(&bitsPerSample), 2);
        
        // data子块（8字节 + 数据）
        file->write("data", 4); // Subchunk2ID
        file->write(reinterpret_cast<const char*>(&dataSize), 4); // Subchunk2Size
    }
    
    // 更新WAV文件头（录音结束后更新文件大小）
    void updateWavHeader(QFile* file, const QAudioFormat& format, quint32 dataSize)
    {
        if (!file || !file->isOpen()) {
            return;
        }
        
        qint64 currentPos = file->pos();
        
        // 更新RIFF ChunkSize（位置4，4字节）
        file->seek(4);
        quint32 fileSize = 36 + dataSize;
        file->write(reinterpret_cast<const char*>(&fileSize), 4);
        
        // 更新data Subchunk2Size（位置40，4字节）
        file->seek(40);
        file->write(reinterpret_cast<const char*>(&dataSize), 4);
        
        // 恢复文件位置
        file->seek(currentPos);
    }
    
    // 停止录音并发送
    void stopVoiceRecordingAndSend()
    {
        if (!m_audioInput || m_audioInput->state() != QAudio::ActiveState) {
            qDebug() << "当前没有在录音";
            return;
        }
        
        // 停止定时器
        if (m_voiceWaveformTimer) {
            m_voiceWaveformTimer->stop();
        }
        
        // 隐藏波形显示区域
        if (m_voiceWaveformWidget) {
            m_voiceWaveformWidget->hide();
        }
        if (m_voiceWaveformBar) {
            m_voiceWaveformBar->setValue(0);
        }
        
        // 断开音频数据读取连接
        if (m_audioInputDevice) {
            disconnect(m_audioInputDevice, nullptr, this, nullptr);
        }
        
        // 停止录音
        if (m_audioInput) {
            m_audioInput->stop();
        }
        
        // 获取PCM数据大小
        qint64 pcmDataSize = 0;
        if (m_voiceRecordFile && m_voiceRecordFile->isOpen()) {
            pcmDataSize = m_voiceRecordFile->pos() - m_pcmDataStartPos;
        }
        
        // 更新WAV文件头（写入正确的文件大小和数据大小）
        if (m_voiceRecordFile && m_voiceRecordFile->isOpen() && pcmDataSize > 0) {
            updateWavHeader(m_voiceRecordFile, m_audioFormat, pcmDataSize);
        }
        
        // 关闭文件
        if (m_voiceRecordFile) {
            m_voiceRecordFile->close();
            delete m_voiceRecordFile;
            m_voiceRecordFile = nullptr;
        }
        
        // 清理音频输入设备
        m_audioInputDevice = nullptr;
        
        // 计算录音时长
        qint64 duration = QDateTime::currentMSecsSinceEpoch() - m_voiceRecordStartTime;
        int durationSeconds = (int)(duration / 1000);
        
        qDebug() << "录音结束，时长:" << durationSeconds << "秒，文件:" << m_currentVoiceFile;
        qDebug() << "PCM数据大小:" << pcmDataSize << "字节";
        
        // 检查录音时长（至少0.5秒）
        if (durationSeconds < 1) {
            QMessageBox::information(this, "提示", "录音时间太短，请至少录音1秒");
            // 删除录音文件
            QFile::remove(m_currentVoiceFile);
            m_currentVoiceFile.clear();
            return;
        }
        
        // 检查文件大小
        QFileInfo fileInfo(m_currentVoiceFile);
        if (!fileInfo.exists() || fileInfo.size() <= 0) {
            QMessageBox::warning(this, "错误", "录音文件无效！");
            QFile::remove(m_currentVoiceFile);
            m_currentVoiceFile.clear();
            return;
        }
        
        UserInfo userinfo = CommonInfo::GetData();
        
        // 先显示语音消息UI（带进度条）
        QPair<QProgressBar*, QLabel*> progressWidgets = addVoiceMessageWithProgress(":/res/img/home.png", userinfo.strName, durationSeconds, m_currentVoiceFile, true);
        
        // 使用腾讯SDK发送语音（传入进度条和状态标签）
        sendVoiceMessageViaTIMSDK(m_currentVoiceFile, durationSeconds, userinfo, progressWidgets.first, progressWidgets.second);
        
        // 清空录音文件路径（发送后不清除，等待发送完成后再处理）
        // m_currentVoiceFile 会在发送成功的回调中清理
    }

private:
    QListWidget* m_listWidget;
    QLineEdit* m_lineEdit;

    // 窗口外观/交互（圆角、关闭、拖拽）
    QFrame* m_container = nullptr;
    QFrame* m_titleBar = nullptr;
    QWidget* m_contentWidget = nullptr;
    QFrame* m_inputBar = nullptr;
    QLabel* m_titleLabel = nullptr;
    QPushButton* m_btnClose = nullptr;
    QPushButton* m_btnMore = nullptr;
    QWidget* m_titleLeftSpacer = nullptr;
    int m_cornerRadius = 16;
    bool m_dragging = false;
    QPoint m_dragStartPos;
    bool m_historyLoaded = false;

    ChatMessage m_lastMessage;
    bool m_hasLastMessage = false;
    
    // 实时录音相关
    QAudioInput* m_audioInput = nullptr;
    QFile* m_voiceRecordFile = nullptr;
    QIODevice* m_audioInputDevice = nullptr; // 用于读取音频数据计算音量
    QString m_currentVoiceFile;
    qint64 m_voiceRecordStartTime = 0;
    QWidget* m_voiceWaveformWidget = nullptr; // 波形显示区域
    QMediaPlayer* m_voicePlayer = nullptr; // 语音播放器（后台播放）
    QProgressBar* m_voiceWaveformBar = nullptr; // 波形进度条
    QLabel* m_voiceDurationLabel = nullptr; // 录音时长标签
    QTimer* m_voiceWaveformTimer = nullptr; // 波形更新定时器
    QAudioFormat m_audioFormat; // 保存音频格式，用于写入WAV头
    qint64 m_pcmDataStartPos = 0; // PCM数据开始位置（WAV头之后）

    void addTimeLabel(const QDateTime& time)
    {
        QListWidgetItem* timeItem = new QListWidgetItem(m_listWidget);
        QLabel* lblTime = new QLabel(time.toString("yyyy-MM-dd hh:mm"));
        lblTime->setAlignment(Qt::AlignCenter);
        lblTime->setStyleSheet(
            "QLabel {"
            "  color: rgba(255,255,255,0.55);"
            "  background-color: rgba(255,255,255,0.06);"
            "  border: 1px solid rgba(255,255,255,0.08);"
            "  border-radius: 10px;"
            "  padding: 2px 10px;"
            "  font-size: 11px;"
            "}"
        );
        timeItem->setSizeHint(QSize(0, 28));
        m_listWidget->addItem(timeItem);
        m_listWidget->setItemWidget(timeItem, lblTime);
    }

    bool shouldHideAvatar(const QString& senderName, bool isMine, const QDateTime& now)
    {
        return m_hasLastMessage &&
            m_lastMessage.senderName == senderName &&
            m_lastMessage.isMine == isMine &&
            m_lastMessage.time.secsTo(now) <= 180;
    }

    //如果 hideAvatar = true 就不显示头像但仍显示发送者名字。
    QWidget* buildMessageWidget(const QString& avatarPath, const QString& senderName, QWidget* contentWidget, bool isMine, bool hideAvatar)
    {
        QWidget* msgWidget = new QWidget();
        // 确保消息 widget 自身不绘制背景，否则会在名字行/空白处形成“灰条”
        msgWidget->setStyleSheet("background: transparent;");
        msgWidget->setAttribute(Qt::WA_TranslucentBackground, true);
        QVBoxLayout* vLayout = new QVBoxLayout(msgWidget);
        vLayout->setContentsMargins(5, 5, 5, 5);
        vLayout->setSpacing(2);

        // ====== 第一行：发送者名称 ======
        QLabel* lblName = new QLabel(senderName);
        lblName->setStyleSheet("background: transparent; color: rgba(255,255,255,0.55); font-size: 12px;");
        if (isMine) {
            lblName->setAlignment(Qt::AlignRight);
        }
        else {
            lblName->setAlignment(Qt::AlignLeft);
        }
        vLayout->addWidget(lblName);

        // ====== 第二行：头像 + 气泡 ======
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setContentsMargins(0, 0, 0, 0);

        QLabel* lblAvatar = new QLabel();
        lblAvatar->setFixedSize(36, 36);
        if (!hideAvatar)
        {
            QPixmap avatarPixmap;
            if (QFile::exists(avatarPath))
                avatarPixmap.load(avatarPath);
            else
                avatarPixmap.fill(Qt::gray);
            avatarPixmap = avatarPixmap.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            lblAvatar->setPixmap(avatarPixmap);
            lblAvatar->setStyleSheet("border-radius: 18px;");
        }

        if (isMine)
        {
            // 我方气泡：右侧蓝色（适配暗色背景）
            contentWidget->setStyleSheet(
                "background-color: #2D7DFF;"
                "color: #ffffff;"
                "border-radius: 14px;"
                "padding: 8px 12px;"
            );
            hLayout->addStretch();
            hLayout->addWidget(contentWidget);
            if (!hideAvatar) hLayout->addWidget(lblAvatar);
        }
        else
        {
            // 对方气泡：左侧灰色（适配暗色背景）
            contentWidget->setStyleSheet(
                "background-color: rgba(255,255,255,0.10);"
                "color: #ffffff;"
                "border-radius: 14px;"
                "padding: 8px 12px;"
            );
            if (!hideAvatar) hLayout->addWidget(lblAvatar);
            hLayout->addWidget(contentWidget);
            hLayout->addStretch();
        }

        vLayout->addLayout(hLayout);
        return msgWidget;
    }


    void addTextMessage(const QString& avatarPath, const QString& senderName, const QString& text, bool isMine)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_hasLastMessage || m_lastMessage.time.secsTo(now) > 180) addTimeLabel(now);
        bool hideAvatar = shouldHideAvatar(senderName, isMine, now);

        QLabel* lblMessage = new QLabel(text);
        lblMessage->setWordWrap(true);
        lblMessage->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QWidget* msgWidget = buildMessageWidget(avatarPath, senderName, lblMessage, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);

        m_lastMessage = { avatarPath, senderName, text, isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
    }

    void addImageMessage(const QString& avatarPath, const QString& senderName, const QString& imgPath, bool isMine)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_hasLastMessage || m_lastMessage.time.secsTo(now) > 180) addTimeLabel(now);
        bool hideAvatar = shouldHideAvatar(senderName, isMine, now);

        ClickableLabelEx* lblImage = new ClickableLabelEx();
        QPixmap pix(imgPath);
        pix = pix.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        lblImage->setPixmap(pix);
        lblImage->setScaledContents(true);
        // 双击图片时用系统默认图片查看器打开
        connect(lblImage, &ClickableLabelEx::doubleClicked, this, [=]() {
            QFileInfo fileInfo(imgPath);
            if (fileInfo.exists()) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(imgPath));
            } else {
                QMessageBox::warning(this, "错误", "图片文件不存在：" + imgPath);
            }
        });

        QWidget* msgWidget = buildMessageWidget(avatarPath, senderName, lblImage, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);

        m_lastMessage = { avatarPath, senderName, "[图片]", isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
    }

    // 添加带进度条的文件消息（用于发送文件）
    QPair<QProgressBar*, QLabel*> addFileMessageWithProgress(const QString& avatarPath, const QString& senderName, 
                                                             const QString& filePath, bool isMine)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_hasLastMessage || m_lastMessage.time.secsTo(now) > 180) addTimeLabel(now);
        bool hideAvatar = shouldHideAvatar(senderName, isMine, now);

        QFileInfo fi(filePath);
        QString fileName = fi.fileName();
        qint64 fileSizeBytes = fi.size();
        QString fileSize = fileSizeBytes < 1024 ? QString("%1 字节").arg(fileSizeBytes) :
                          fileSizeBytes < 1024 * 1024 ? QString("%1 KB").arg(fileSizeBytes / 1024.0, 0, 'f', 1) :
                          QString("%1 MB").arg(fileSizeBytes / (1024.0 * 1024.0), 0, 'f', 1);

        // 创建文件消息容器
        QWidget* fileWidget = new QWidget();
        QVBoxLayout* fileLayout = new QVBoxLayout(fileWidget);
        fileLayout->setContentsMargins(5, 5, 5, 5);
        fileLayout->setSpacing(5);
        
        // 文件名和大小标签
        ClickableLabelEx* lblFile = new ClickableLabelEx(fileWidget);
        lblFile->setText(fileName + "\n" + fileSize);
        lblFile->setStyleSheet("qproperty-alignment: AlignCenter;");
        lblFile->setMinimumSize(120, 50);
        connect(lblFile, &ClickableLabelEx::clicked, this, [=]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        });
        fileLayout->addWidget(lblFile);
        
        // 创建进度条（设置父对象为fileWidget，确保生命周期）
        QProgressBar* progressBar = new QProgressBar(fileWidget);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        progressBar->setTextVisible(true);
        progressBar->setFormat("%p%");
        fileLayout->addWidget(progressBar);
        
        // 创建状态标签（设置父对象为fileWidget，确保生命周期）
        QLabel* statusLabel = new QLabel(fileWidget);
        statusLabel->setText("准备发送...");
        statusLabel->setStyleSheet("color: gray; font-size: 10px;");
        fileLayout->addWidget(statusLabel);

        QWidget* msgWidget = buildMessageWidget(avatarPath, senderName, fileWidget, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);

        m_lastMessage = { avatarPath, senderName, "[文件] " + fileName, isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
        
        // 返回进度条和状态标签的指针
        return qMakePair(progressBar, statusLabel);
    }

    void addFileMessage(const QString& avatarPath, const QString& senderName, const QString& filePath, bool isMine)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_hasLastMessage || m_lastMessage.time.secsTo(now) > 180) addTimeLabel(now);
        bool hideAvatar = shouldHideAvatar(senderName, isMine, now);

        QFileInfo fi(filePath);
        QString fileName = fi.fileName();
        QString fileSize = QString::number(fi.size() / 1024.0, 'f', 1) + " KB";

        ClickableLabelEx* lblFile = new ClickableLabelEx();
        lblFile->setText(fileName + "\n" + fileSize);
        lblFile->setStyleSheet("qproperty-alignment: AlignCenter;");
        lblFile->setMinimumSize(120, 50);
        connect(lblFile, &ClickableLabelEx::clicked, this, [=]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        });

        QWidget* msgWidget = buildMessageWidget(avatarPath, senderName, lblFile, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);

        m_lastMessage = { avatarPath, senderName, "[文件] " + fileName, isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
    }

    void addVoiceMessage(const QString& avatarPath, const QString& senderName, int seconds, bool isMine, 
                        const QString& voicePath = QString())
    {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_hasLastMessage || m_lastMessage.time.secsTo(now) > 180) addTimeLabel(now);
        bool hideAvatar = shouldHideAvatar(senderName, isMine, now);

        // 创建语音消息容器
        QWidget* voiceWidget = new QWidget();
        QVBoxLayout* voiceLayout = new QVBoxLayout(voiceWidget);
        voiceLayout->setContentsMargins(5, 5, 5, 5);
        voiceLayout->setSpacing(5);
        
        // 语音标签
        ClickableLabelEx* lblVoice = new ClickableLabelEx(voiceWidget);
        lblVoice->setText(QString("🎵 语音 %1 s").arg(seconds));
        lblVoice->setMinimumSize(80 + seconds * 5, 36);
        lblVoice->setStyleSheet("background-color: #E3F2FD; border: 1px solid #2196F3; border-radius: 4px; padding: 5px;");
        
        // 保存语音文件路径到label的property中，以便播放
        if (!voicePath.isEmpty()) {
            lblVoice->setProperty("voicePath", voicePath);
        }
        
        connect(lblVoice, &ClickableLabelEx::clicked, this, [=]() {
            QString path = lblVoice->property("voicePath").toString();
            if (!path.isEmpty()) {
                playVoiceMessage(path);
            } else {
                qDebug() << "语音文件路径为空，无法播放";
                QMessageBox::information(this, "提示", "语音文件尚未下载完成，请稍后重试");
            }
        });
        voiceLayout->addWidget(lblVoice);
        
        // 创建播放进度条（初始隐藏，播放时显示）
        QProgressBar* playProgressBar = new QProgressBar(voiceWidget);
        playProgressBar->setRange(0, 100);
        playProgressBar->setValue(0);
        playProgressBar->setTextVisible(false); // 不显示文字，只用颜色条
        playProgressBar->setFixedHeight(4); // 更细的进度条
        playProgressBar->setStyleSheet("QProgressBar { border: none; border-radius: 2px; background-color: #E3F2FD; } QProgressBar::chunk { background-color: #2196F3; border-radius: 2px; }");
        playProgressBar->hide(); // 初始隐藏
        voiceLayout->addWidget(playProgressBar);
        
        // 创建播放状态标签（初始隐藏）
        QLabel* playStatusLabel = new QLabel(voiceWidget);
        playStatusLabel->setText("播放中...");
        playStatusLabel->setStyleSheet("color: #2196F3; font-size: 10px;");
        playStatusLabel->hide(); // 初始隐藏
        voiceLayout->addWidget(playStatusLabel);
        
        // 保存播放进度映射和时长
        if (!voicePath.isEmpty()) {
            QString normalizedPath = QDir::toNativeSeparators(voicePath);
            m_voicePlayProgressMap[normalizedPath] = qMakePair(playProgressBar, playStatusLabel);
            m_voiceDurationMap[normalizedPath] = seconds; // 保存语音时长
        }

        QWidget* msgWidget = buildMessageWidget(avatarPath, senderName, voiceWidget, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);
        
        // 保存语音路径到item的data中
        if (!voicePath.isEmpty()) {
            item->setData(Qt::UserRole + 3, voicePath);
        }

        m_lastMessage = { avatarPath, senderName, QString("[语音] %1秒").arg(seconds), isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
    }
    
    // 添加带进度条的语音消息（用于发送和接收）
    QPair<QProgressBar*, QLabel*> addVoiceMessageWithProgress(const QString& avatarPath, const QString& senderName, 
                                                              int seconds, const QString& voicePath, bool isMine)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_hasLastMessage || m_lastMessage.time.secsTo(now) > 180) addTimeLabel(now);
        bool hideAvatar = shouldHideAvatar(senderName, isMine, now);

        // 创建语音消息容器
        QWidget* voiceWidget = new QWidget();
        QVBoxLayout* voiceLayout = new QVBoxLayout(voiceWidget);
        voiceLayout->setContentsMargins(5, 5, 5, 5);
        voiceLayout->setSpacing(5);
        
        // 语音标签
        ClickableLabelEx* lblVoice = new ClickableLabelEx(voiceWidget);
        lblVoice->setText(QString("🎵 语音 %1 s").arg(seconds));
        lblVoice->setMinimumSize(80 + seconds * 5, 36);
        lblVoice->setStyleSheet("background-color: #E3F2FD; border: 1px solid #2196F3; border-radius: 4px; padding: 5px;");
        
        // 保存语音文件路径到label的property中，以便播放
        if (!voicePath.isEmpty()) {
            lblVoice->setProperty("voicePath", voicePath);
        }
        
        connect(lblVoice, &ClickableLabelEx::clicked, this, [=]() {
            QString path = lblVoice->property("voicePath").toString();
            if (!path.isEmpty()) {
                playVoiceMessage(path);
            } else {
                qDebug() << "语音文件路径为空，无法播放";
                QMessageBox::information(this, "提示", "语音文件尚未下载完成，请稍后重试");
            }
        });
        voiceLayout->addWidget(lblVoice);
        
        // 创建进度条（设置父对象为voiceWidget，确保生命周期）
        QProgressBar* progressBar = new QProgressBar(voiceWidget);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        progressBar->setTextVisible(true);
        progressBar->setFormat("%p%");
        progressBar->setFixedHeight(20);
        voiceLayout->addWidget(progressBar);
        
        // 创建状态标签（设置父对象为voiceWidget，确保生命周期）
        QLabel* statusLabel = new QLabel(voiceWidget);
        statusLabel->setText(isMine ? "准备发送..." : "准备下载...");
        statusLabel->setStyleSheet("color: gray; font-size: 10px;");
        voiceLayout->addWidget(statusLabel);
        
        // 创建播放进度条（初始隐藏，播放时显示）
        QProgressBar* playProgressBar = new QProgressBar(voiceWidget);
        playProgressBar->setRange(0, 100);
        playProgressBar->setValue(0);
        playProgressBar->setTextVisible(false); // 不显示文字，只用颜色条
        playProgressBar->setFixedHeight(4); // 更细的进度条
        playProgressBar->setStyleSheet("QProgressBar { border: none; border-radius: 2px; background-color: #E3F2FD; } QProgressBar::chunk { background-color: #2196F3; border-radius: 2px; }");
        playProgressBar->hide(); // 初始隐藏
        voiceLayout->addWidget(playProgressBar);
        
        // 创建播放状态标签（初始隐藏）
        QLabel* playStatusLabel = new QLabel(voiceWidget);
        playStatusLabel->setText("播放中...");
        playStatusLabel->setStyleSheet("color: #2196F3; font-size: 10px;");
        playStatusLabel->hide(); // 初始隐藏
        voiceLayout->addWidget(playStatusLabel);
        
        // 保存播放进度映射和时长
        if (!voicePath.isEmpty()) {
            QString normalizedPath = QDir::toNativeSeparators(voicePath);
            m_voicePlayProgressMap[normalizedPath] = qMakePair(playProgressBar, playStatusLabel);
            m_voiceDurationMap[normalizedPath] = seconds; // 保存语音时长
        }

        QWidget* msgWidget = buildMessageWidget(avatarPath, senderName, voiceWidget, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);
        
        // 保存语音路径到item的data中
        if (!voicePath.isEmpty()) {
            item->setData(Qt::UserRole + 3, voicePath);
        }

        m_lastMessage = { avatarPath, senderName, QString("[语音] %1秒").arg(seconds), isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
        
        return qMakePair(progressBar, statusLabel);
    }

    // 使用腾讯SDK发送文本消息
    void sendTextMessageViaTIMSDK(const QString& text, const UserInfo& userinfo)
    {
        if (m_unique_group_id.isEmpty()) return;
        
        // 构造消息元素
        QJsonObject textElem;
        textElem[kTIMElemType] = (int)kTIMElem_Text;
        textElem[kTIMTextElemContent] = text;
        
        // 构造消息
        QJsonObject msgObj;
        QJsonArray elemArray;
        elemArray.append(textElem);
        msgObj[kTIMMsgElemArray] = elemArray;
        msgObj[kTIMMsgSender] = userinfo.strUserId;
        qint64 currentTime = QDateTime::currentDateTime().toSecsSinceEpoch();
        msgObj[kTIMMsgClientTime] = currentTime;
        msgObj[kTIMMsgServerTime] = currentTime;
        
        // 转换为JSON字符串
        QJsonDocument doc(msgObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        // 创建回调数据结构
        struct SendMsgCallbackData {
            ChatDialog* dlg;
            QString text;
            QString senderName;
        };
        SendMsgCallbackData* callbackData = new SendMsgCallbackData;
        callbackData->dlg = this;
        callbackData->text = text;
        callbackData->senderName = userinfo.strName;
        
        // 发送消息
        QByteArray groupIdBytes = m_unique_group_id.toUtf8();
        int ret = TIMMsgSendNewMsg(groupIdBytes.constData(), kTIMConv_Group, jsonData.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                SendMsgCallbackData* data = (SendMsgCallbackData*)user_data;
                if (code != TIM_SUCC) {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    qDebug() << "发送消息失败，错误码:" << code << "，描述:" << errorDesc;
                    QMessageBox::critical(data->dlg, "发送失败", QString("消息发送失败\n错误码: %1\n错误描述: %2").arg(code).arg(errorDesc));
                } else {
                    qDebug() << "消息发送成功";
                }
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            qDebug() << "调用TIMMsgSendNewMsg失败，错误码:" << ret;
            QMessageBox::critical(this, "错误", QString("调用发送接口失败，错误码: %1").arg(ret));
            delete callbackData;
        }
    }
    
    // 使用腾讯SDK发送语音消息
    void sendVoiceMessageViaTIMSDK(const QString& voicePath, int durationSeconds, const UserInfo& userinfo,
                                   QProgressBar* progressBar = nullptr, QLabel* statusLabel = nullptr)
    {
        if (m_unique_group_id.isEmpty() || voicePath.isEmpty()) return;
        
        // 检查文件是否存在
        QFileInfo fileInfo(voicePath);
        if (!fileInfo.exists()) {
            QMessageBox::warning(this, "错误", QString("语音文件不存在：%1").arg(voicePath));
            return;
        }
        
        qint64 fileSize = fileInfo.size();
        if (fileSize <= 0) {
            QMessageBox::warning(this, "错误", "语音文件大小为0，无法发送！");
            return;
        }
        
        // 将路径转换为本地路径格式
        QString normalizedPath = QDir::toNativeSeparators(voicePath);
        
        // 保存进度条和状态标签到映射中（用于上传进度回调）
        if (progressBar && statusLabel) {
            m_voiceUploadProgressMap[normalizedPath] = qMakePair(progressBar, statusLabel);
            qDebug() << "保存语音上传进度映射，路径:" << normalizedPath;
        }
        
        // 注册上传进度回调（如果还没有注册）
        static bool voiceUploadProgressCallbackRegistered = false;
        if (!voiceUploadProgressCallbackRegistered) {
            TIMSetMsgElemUploadProgressCallback([](const char* json_msg, uint32_t index, uint32_t cur_size, uint32_t total_size, const void* user_data) {
                // 解析消息JSON，找到对应的语音文件路径
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json_msg), &parseError);
                if (parseError.error != QJsonParseError::NoError) {
                    return;
                }
                
                QJsonObject msgObj = doc.object();
                if (!msgObj.contains(kTIMMsgElemArray) || !msgObj[kTIMMsgElemArray].isArray()) {
                    return;
                }
                
                QJsonArray elemArray = msgObj[kTIMMsgElemArray].toArray();
                if (index >= (uint32_t)elemArray.size()) {
                    return;
                }
                
                QJsonValue elemValue = elemArray[index];
                if (!elemValue.isObject()) {
                    return;
                }
                
                QJsonObject elemObj = elemValue.toObject();
                int elemType = elemObj[kTIMElemType].toInt();
                
                // 只处理语音元素
                if (elemType == kTIMElem_Sound) {
                    QString filePath = elemObj[kTIMSoundElemFilePath].toString();
                    QString normalizedPath = QDir::toNativeSeparators(filePath);
                    
                    // 查找所有ChatDialog实例，更新进度
                    QList<ChatDialog*> instances = getInstanceList();
                    for (ChatDialog* dlg : instances) {
                        if (dlg && dlg->m_voiceUploadProgressMap.contains(normalizedPath)) {
                            QProgressBar* progressBar = dlg->m_voiceUploadProgressMap[normalizedPath].first;
                            QLabel* statusLabel = dlg->m_voiceUploadProgressMap[normalizedPath].second;
                            
                            if (progressBar && progressBar->parent()) {
                                int progress = total_size > 0 ? (int)((double)cur_size / total_size * 100) : 0;
                                progressBar->setValue(progress);
                                
                                if (statusLabel && statusLabel->parent()) {
                                    QString curStr = cur_size < 1024 ? QString("%1 字节").arg(cur_size) :
                                                   cur_size < 1024 * 1024 ? QString("%1 KB").arg(cur_size / 1024.0, 0, 'f', 1) :
                                                   QString("%1 MB").arg(cur_size / (1024.0 * 1024.0), 0, 'f', 1);
                                    QString totalStr = total_size < 1024 ? QString("%1 字节").arg(total_size) :
                                                      total_size < 1024 * 1024 ? QString("%1 KB").arg(total_size / 1024.0, 0, 'f', 1) :
                                                      QString("%1 MB").arg(total_size / (1024.0 * 1024.0), 0, 'f', 1);
                                    statusLabel->setText(QString("上传中: %1 / %2 (%3%)").arg(curStr).arg(totalStr).arg(progress));
                                    statusLabel->setStyleSheet("color: blue; font-size: 10px;");
                                }
                            }
                        }
                    }
                }
            }, nullptr);
            voiceUploadProgressCallbackRegistered = true;
        }
        
        // 构造语音元素
        QJsonObject soundElem;
        soundElem[kTIMElemType] = (int)kTIMElem_Sound;
        soundElem[kTIMSoundElemFilePath] = normalizedPath; // 语音文件路径（必填）
        soundElem[kTIMSoundElemFileSize] = (int)fileSize; // 文件大小
        soundElem[kTIMSoundElemFileTime] = durationSeconds; // 语音时长（秒，必填）
        
        // 构造消息
        QJsonObject msgObj;
        QJsonArray elemArray;
        elemArray.append(soundElem);
        msgObj[kTIMMsgElemArray] = elemArray;
        msgObj[kTIMMsgSender] = userinfo.strUserId;
        qint64 currentTime = QDateTime::currentDateTime().toSecsSinceEpoch();
        msgObj[kTIMMsgClientTime] = currentTime;
        msgObj[kTIMMsgServerTime] = currentTime;
        
        // 转换为JSON字符串
        QJsonDocument doc(msgObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        qDebug() << "发送语音消息，路径:" << normalizedPath;
        qDebug() << "文件大小:" << fileSize << "字节，时长:" << durationSeconds << "秒";
        qDebug() << "消息JSON:" << QString::fromUtf8(jsonData);
        
        // 创建回调数据结构
        struct SendVoiceMsgCallbackData {
            ChatDialog* dlg;
            QString voicePath;
            QString senderName;
            bool isTempFile; // 标记是否为临时录音文件
        };
        SendVoiceMsgCallbackData* callbackData = new SendVoiceMsgCallbackData;
        callbackData->dlg = this;
        callbackData->voicePath = voicePath;
        callbackData->senderName = userinfo.strName;
        // 如果是当前录音文件，标记为临时文件，发送成功后可以选择删除
        callbackData->isTempFile = (voicePath == m_currentVoiceFile);
        
        // 发送消息
        QByteArray groupIdBytes = m_unique_group_id.toUtf8();
        int ret = TIMMsgSendNewMsg(groupIdBytes.constData(), kTIMConv_Group, jsonData.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                SendVoiceMsgCallbackData* data = (SendVoiceMsgCallbackData*)user_data;
                
                // 从map中获取进度条和状态标签
                QString normalizedKey = QDir::toNativeSeparators(data->voicePath);
                QProgressBar* progressBar = nullptr;
                QLabel* statusLabel = nullptr;
                
                if (data->dlg->m_voiceUploadProgressMap.contains(normalizedKey)) {
                    progressBar = data->dlg->m_voiceUploadProgressMap[normalizedKey].first;
                    statusLabel = data->dlg->m_voiceUploadProgressMap[normalizedKey].second;
                }
                
                if (code != TIM_SUCC) {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    qDebug() << "发送语音消息失败，错误码:" << code << "，描述:" << errorDesc;
                    
                    // 更新状态为失败
                    if (statusLabel && statusLabel->parent()) {
                        statusLabel->setText("发送失败");
                        statusLabel->setStyleSheet("color: red; font-size: 10px;");
                    }
                    if (progressBar && progressBar->parent()) {
                        progressBar->setValue(0);
                    }
                    
                    QMessageBox::critical(data->dlg, "发送失败", QString("语音消息发送失败\n错误码: %1\n错误描述: %2").arg(code).arg(errorDesc));
                    
                    // 从进度映射中移除
                    data->dlg->m_voiceUploadProgressMap.remove(normalizedKey);
                } else {
                    qDebug() << "语音消息发送成功";
                    
                    // 更新状态为成功
                    if (statusLabel && statusLabel->parent()) {
                        statusLabel->setText("发送成功");
                        statusLabel->setStyleSheet("color: green; font-size: 10px;");
                    }
                    if (progressBar && progressBar->parent()) {
                        progressBar->setValue(100);
                        // 3秒后隐藏进度条
                        QTimer::singleShot(3000, progressBar, [progressBar, statusLabel]() {
                            if (progressBar && progressBar->parent()) {
                                progressBar->hide();
                            }
                            if (statusLabel && statusLabel->parent()) {
                                statusLabel->hide();
                            }
                        });
                    }
                    
                    // 从进度映射中移除
                    data->dlg->m_voiceUploadProgressMap.remove(normalizedKey);
                }
                
                // 清空当前录音文件路径
                if (data->isTempFile && data->dlg->m_currentVoiceFile == data->voicePath) {
                    data->dlg->m_currentVoiceFile.clear();
                }
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            qDebug() << "调用TIMMsgSendNewMsg失败，错误码:" << ret;
            QString errorDesc = QString("错误码: %1").arg(ret);
            QMessageBox::critical(this, "发送失败", QString("语音消息发送失败\n%1").arg(errorDesc));
            delete callbackData;
        }
    }
    
    // 使用腾讯SDK发送图片消息
    void sendImageMessageViaTIMSDK(const QString& imgPath, const UserInfo& userinfo)
    {
        if (m_unique_group_id.isEmpty() || imgPath.isEmpty()) return;
        
        // 检查文件是否存在
        QFileInfo fileInfo(imgPath);
        if (!fileInfo.exists()) {
            QMessageBox::warning(this, "错误", QString("图片文件不存在：%1").arg(imgPath));
            return;
        }
        
        // 将路径转换为本地路径格式（Windows需要使用反斜杠）
        QString normalizedPath = QDir::toNativeSeparators(imgPath);
        
        // 构造图片元素
        QJsonObject imageElem;
        imageElem[kTIMElemType] = (int)kTIMElem_Image;
        imageElem[kTIMImageElemOrigPath] = normalizedPath; // 图片路径（必填）
        imageElem[kTIMImageElemLevel] = (int)kTIMImageLevel_Compression; // 压缩图发送（默认值，图片较小）
        
        // 可选：设置图片格式（根据文件扩展名判断）
        QString suffix = fileInfo.suffix().toLower();
        if (suffix == "png") {
            imageElem[kTIMImageElemFormat] = 1; // PNG格式
        } else if (suffix == "jpg" || suffix == "jpeg") {
            imageElem[kTIMImageElemFormat] = 2; // JPG格式
        } else if (suffix == "bmp") {
            imageElem[kTIMImageElemFormat] = 3; // BMP格式
        } else if (suffix == "gif") {
            imageElem[kTIMImageElemFormat] = 4; // GIF格式
        }
        // 如果不设置，SDK会自动识别
        
        // 构造消息
        QJsonObject msgObj;
        QJsonArray elemArray;
        elemArray.append(imageElem);
        msgObj[kTIMMsgElemArray] = elemArray;
        msgObj[kTIMMsgSender] = userinfo.strUserId;
        qint64 currentTime = QDateTime::currentDateTime().toSecsSinceEpoch();
        msgObj[kTIMMsgClientTime] = currentTime;
        msgObj[kTIMMsgServerTime] = currentTime;
        
        // 转换为JSON字符串
        QJsonDocument doc(msgObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        qDebug() << "发送图片消息，路径:" << normalizedPath;
        qDebug() << "消息JSON:" << QString::fromUtf8(jsonData);
        
        // 创建回调数据结构
        struct SendImageMsgCallbackData {
            ChatDialog* dlg;
            QString imgPath;
            QString senderName;
        };
        SendImageMsgCallbackData* callbackData = new SendImageMsgCallbackData;
        callbackData->dlg = this;
        callbackData->imgPath = imgPath;
        callbackData->senderName = userinfo.strName;
        
        // 发送消息
        QByteArray groupIdBytes = m_unique_group_id.toUtf8();
        int ret = TIMMsgSendNewMsg(groupIdBytes.constData(), kTIMConv_Group, jsonData.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                SendImageMsgCallbackData* data = (SendImageMsgCallbackData*)user_data;
                if (code != TIM_SUCC) {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    qDebug() << "发送图片消息失败，错误码:" << code << "，描述:" << errorDesc;
                    QMessageBox::critical(data->dlg, "发送失败", QString("图片消息发送失败\n错误码: %1\n错误描述: %2").arg(code).arg(errorDesc));
                } else {
                    qDebug() << "图片消息发送成功";
                    // 可以选择在成功后显示成功提示，或者静默处理
                }
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            qDebug() << "调用TIMMsgSendNewMsg失败，错误码:" << ret;
            QMessageBox::critical(this, "错误", QString("调用发送接口失败，错误码: %1").arg(ret));
            delete callbackData;
        }
    }
    
    // 使用腾讯SDK发送文件消息
    void sendFileMessageViaTIMSDK(const QString& filePath, const UserInfo& userinfo, 
                                   QProgressBar* progressBar = nullptr, QLabel* statusLabel = nullptr)
    {
        if (m_unique_group_id.isEmpty() || filePath.isEmpty()) return;
        
        // 检查文件是否存在
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            QMessageBox::warning(this, "错误", QString("文件不存在：%1").arg(filePath));
            return;
        }
        
        // 检查文件大小（腾讯SDK可能有大小限制，这里可以添加检查）
        qint64 fileSize = fileInfo.size();
        if (fileSize <= 0) {
            QMessageBox::warning(this, "错误", "文件大小为0，无法发送！");
            return;
        }
        
        // 将路径转换为本地路径格式（Windows需要使用反斜杠）
        QString normalizedPath = QDir::toNativeSeparators(filePath);
        QString fileName = fileInfo.fileName(); // 获取文件名（不含路径）
        
        // 构造文件元素
        QJsonObject fileElem;
        fileElem[kTIMElemType] = (int)kTIMElem_File;
        fileElem[kTIMFileElemFilePath] = normalizedPath; // 文件路径（必填）
        fileElem[kTIMFileElemFileName] = fileName; // 文件名（必填）
        fileElem[kTIMFileElemFileSize] = (int)fileSize; // 文件大小（必填，转换为int）
        
        // 构造消息
        QJsonObject msgObj;
        QJsonArray elemArray;
        elemArray.append(fileElem);
        msgObj[kTIMMsgElemArray] = elemArray;
        msgObj[kTIMMsgSender] = userinfo.strUserId;
        qint64 currentTime = QDateTime::currentDateTime().toSecsSinceEpoch();
        msgObj[kTIMMsgClientTime] = currentTime;
        msgObj[kTIMMsgServerTime] = currentTime;
        
        // 转换为JSON字符串
        QJsonDocument doc(msgObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        qDebug() << "发送文件消息，路径:" << normalizedPath;
        qDebug() << "文件名:" << fileName << "，大小:" << fileSize << "字节";
        qDebug() << "消息JSON:" << QString::fromUtf8(jsonData);
        
        // 注册上传进度回调（全局注册一次）
        static bool s_uploadProgressCallbackRegistered = false;
        if (!s_uploadProgressCallbackRegistered) {
            TIMSetMsgElemUploadProgressCallback([](const char* json_msg, uint32_t index, uint32_t cur_size, uint32_t total_size, const void* user_data) {
                // 解析消息JSON，找到对应的文件路径
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json_msg), &parseError);
                if (parseError.error != QJsonParseError::NoError) {
                    return;
                }
                
                QJsonObject msgObj = doc.object();
                if (!msgObj.contains(kTIMMsgElemArray) || !msgObj[kTIMMsgElemArray].isArray()) {
                    return;
                }
                
                QJsonArray elemArray = msgObj[kTIMMsgElemArray].toArray();
                if (index >= (uint32_t)elemArray.size()) {
                    return;
                }
                
                QJsonObject elemObj = elemArray[index].toObject();
                int elemType = elemObj[kTIMElemType].toInt();
                
                // 只处理文件元素
                if (elemType == kTIMElem_File) {
                    QString filePath = elemObj[kTIMFileElemFilePath].toString();
                    QString normalizedPath = QDir::toNativeSeparators(filePath); // 规范化路径
                    
                    qDebug() << "文件上传进度回调，路径:" << normalizedPath << "，当前大小:" << cur_size << "，总大小:" << total_size;
                    
                    // 在所有ChatDialog实例中查找匹配的进度条
                    QList<ChatDialog*>& instances = ChatDialog::getInstanceList();
                    for (ChatDialog* dlg : instances) {
                        if (!dlg) continue;
                        
                        // 尝试匹配路径（可能格式不同）
                        bool found = false;
                        QProgressBar* progressBar = nullptr;
                        QLabel* statusLabel = nullptr;
                        
                        // 首先尝试精确匹配
                        if (dlg->m_fileUploadProgressMap.contains(normalizedPath)) {
                            progressBar = dlg->m_fileUploadProgressMap[normalizedPath].first;
                            statusLabel = dlg->m_fileUploadProgressMap[normalizedPath].second;
                            found = true;
                        } else {
                            // 如果精确匹配失败，尝试匹配所有key（可能路径格式不同）
                            for (auto it = dlg->m_fileUploadProgressMap.begin(); it != dlg->m_fileUploadProgressMap.end(); ++it) {
                                QString key = it.key();
                                // 转换为相同格式后比较
                                QString normalizedKey = QDir::toNativeSeparators(key);
                                if (QFileInfo(normalizedKey).canonicalFilePath() == QFileInfo(normalizedPath).canonicalFilePath() ||
                                    normalizedKey == normalizedPath || key == filePath || normalizedKey == filePath) {
                                    progressBar = it.value().first;
                                    statusLabel = it.value().second;
                                    found = true;
                                    qDebug() << "找到匹配的文件路径，key:" << key << "，回调路径:" << normalizedPath;
                                    break;
                                }
                            }
                        }
                        
                        if (found && progressBar && total_size > 0) {
                            int progress = (int)((double)cur_size / total_size * 100);
                            qDebug() << "更新上传进度:" << progress << "% (" << cur_size << "/" << total_size << ")";
                            qDebug() << "进度条指针:" << progressBar << "，状态标签指针:" << statusLabel;
                            qDebug() << "进度条父对象:" << (progressBar ? progressBar->parent() : nullptr);
                            
                            // 直接更新（SDK回调通常在主线程中）
                            if (progressBar && progressBar->parent()) { // 检查对象是否仍然有效
                                progressBar->setValue(progress);
                                
                                // 更新状态标签显示进度
                                if (statusLabel && statusLabel->parent()) {
                                    QString sizeStr = cur_size < 1024 ? QString("%1 字节").arg(cur_size) :
                                                     cur_size < 1024 * 1024 ? QString("%1 KB").arg(cur_size / 1024.0, 0, 'f', 1) :
                                                     QString("%1 MB").arg(cur_size / (1024.0 * 1024.0), 0, 'f', 1);
                                    QString totalStr = total_size < 1024 ? QString("%1 字节").arg(total_size) :
                                                      total_size < 1024 * 1024 ? QString("%1 KB").arg(total_size / 1024.0, 0, 'f', 1) :
                                                      QString("%1 MB").arg(total_size / (1024.0 * 1024.0), 0, 'f', 1);
                                    statusLabel->setText(QString("上传中: %1 / %2 (%3%)").arg(sizeStr).arg(totalStr).arg(progress));
                                    statusLabel->setStyleSheet("color: blue; font-size: 10px;");
                                }
                            } else {
                                qDebug() << "进度条或状态标签无效，无法更新";
                            }
                        } else {
                            qDebug() << "未找到匹配的进度条";
                            qDebug() << "回调路径:" << normalizedPath;
                            qDebug() << "map大小:" << (dlg ? dlg->m_fileUploadProgressMap.size() : 0);
                            if (dlg) {
                                qDebug() << "map中的key列表:";
                                for (auto it = dlg->m_fileUploadProgressMap.begin(); it != dlg->m_fileUploadProgressMap.end(); ++it) {
                                    qDebug() << "  key:" << it.key();
                                }
                            }
                        }
                    }
                }
            }, nullptr);
            s_uploadProgressCallbackRegistered = true;
            qDebug() << "文件上传进度回调已注册";
        }
        
        // 创建回调数据结构
        struct SendFileMsgCallbackData {
            ChatDialog* dlg;
            QString filePath;
            QString senderName;
            QProgressBar* progressBar;
            QLabel* statusLabel;
            qint64 totalSize;
        };
        SendFileMsgCallbackData* callbackData = new SendFileMsgCallbackData;
        callbackData->dlg = this;
        callbackData->filePath = filePath;
        callbackData->senderName = userinfo.strName;
        callbackData->progressBar = progressBar;
        callbackData->statusLabel = statusLabel;
        callbackData->totalSize = fileSize;
        
        // 更新状态为"正在上传"
        if (statusLabel) {
            statusLabel->setText("正在上传...");
            statusLabel->setStyleSheet("color: blue; font-size: 10px;");
        }
        if (progressBar) {
            progressBar->setValue(0);
        }
        
        // 保存进度条映射（用于进度回调更新）
        // 使用规范化路径作为key，确保路径格式一致
        QString normalizedKey = QDir::toNativeSeparators(filePath);
        m_fileUploadProgressMap[normalizedKey] = qMakePair(progressBar, statusLabel);
        qDebug() << "保存文件上传进度映射，路径:" << normalizedKey << "，进度条:" << progressBar << "，状态标签:" << statusLabel;
        
        // 发送消息
        QByteArray groupIdBytes = m_unique_group_id.toUtf8();
        int ret = TIMMsgSendNewMsg(groupIdBytes.constData(), kTIMConv_Group, jsonData.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                SendFileMsgCallbackData* data = (SendFileMsgCallbackData*)user_data;
                if (code != TIM_SUCC) {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    qDebug() << "发送文件消息失败，错误码:" << code << "，描述:" << errorDesc;
                    
                    // 从map中获取进度条和状态标签
                    QString normalizedKey = QDir::toNativeSeparators(data->filePath);
                    QProgressBar* progressBar = nullptr;
                    QLabel* statusLabel = nullptr;
                    
                    if (data->dlg->m_fileUploadProgressMap.contains(normalizedKey)) {
                        progressBar = data->dlg->m_fileUploadProgressMap[normalizedKey].first;
                        statusLabel = data->dlg->m_fileUploadProgressMap[normalizedKey].second;
                    }
                    
                    // 更新状态为失败
                    if (statusLabel && statusLabel->parent()) {
                        statusLabel->setText("发送失败");
                        statusLabel->setStyleSheet("color: red; font-size: 10px;");
                    }
                    if (progressBar && progressBar->parent()) {
                        progressBar->setValue(0);
                    }
                    
                    QMessageBox::critical(data->dlg, "发送失败", QString("文件消息发送失败\n错误码: %1\n错误描述: %2").arg(code).arg(errorDesc));
                    
                    // 从进度映射中移除
                    data->dlg->m_fileUploadProgressMap.remove(normalizedKey);
                } else {
                    qDebug() << "文件消息发送成功";
                    
                    // 从map中获取进度条和状态标签（确保使用正确的路径key）
                    QString normalizedKey = QDir::toNativeSeparators(data->filePath);
                    QProgressBar* progressBar = nullptr;
                    QLabel* statusLabel = nullptr;
                    
                    if (data->dlg->m_fileUploadProgressMap.contains(normalizedKey)) {
                        progressBar = data->dlg->m_fileUploadProgressMap[normalizedKey].first;
                        statusLabel = data->dlg->m_fileUploadProgressMap[normalizedKey].second;
                    }
                    
                    // 更新状态为成功
                    if (statusLabel && statusLabel->parent()) { // 检查对象是否仍然有效
                        statusLabel->setText("发送成功");
                        statusLabel->setStyleSheet("color: green; font-size: 10px;");
                    }
                    if (progressBar && progressBar->parent()) { // 检查对象是否仍然有效
                        progressBar->setValue(100);
                        // 3秒后隐藏进度条
                        QTimer::singleShot(3000, progressBar, [progressBar, statusLabel]() {
                            if (progressBar && progressBar->parent()) {
                                progressBar->hide();
                            }
                            if (statusLabel && statusLabel->parent()) {
                                statusLabel->hide();
                            }
                        });
                    }
                    
                    // 从进度映射中移除
                    data->dlg->m_fileUploadProgressMap.remove(normalizedKey);
                }
                
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            qDebug() << "调用TIMMsgSendNewMsg失败，错误码:" << ret;
            QString errorDesc = QString("错误码: %1").arg(ret);
            QMessageBox::critical(this, "发送失败", QString("文件消息发送失败\n%1").arg(errorDesc));
            delete callbackData;
        }
    }
    
    // 接收新消息的回调处理
    void onRecvNewMsg(const char* json_msg_array)
    {
        // 解析JSON消息数组
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json_msg_array), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "解析消息JSON失败:" << parseError.errorString();
            return;
        }
        
        if (!doc.isArray()) {
            qDebug() << "消息JSON不是数组格式";
            return;
        }
        
        QJsonArray msgArray = doc.array();
        UserInfo userInfo = CommonInfo::GetData();
        
        for (const QJsonValue& value : msgArray) {
            if (!value.isObject()) continue;
            
            QJsonObject msgObj = value.toObject();

            // 1. 将 QJsonObject 包装成 QJsonDocument
            QJsonDocument doc(msgObj);

            // 2. 转换为 QString（紧凑格式）
            QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

            qDebug() << jsonString;
            /*
                输出：
                "{\"age\":25,\"name\":\"Tom\",\"score\":88.5}"
            */

            // 3. 如果想美化打印（带缩进）
            QString prettyString = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
            qDebug().noquote() << prettyString;
            
            // 检查是否是当前群组的消息
            QString convId = msgObj[kTIMMsgConvId].toString();
            int convType = msgObj[kTIMMsgConvType].toInt();
            
            if (convType != kTIMConv_Group || convId != m_unique_group_id) {
                continue; // 不是当前群组的消息，跳过
            }
            
            // 检查是否是自己的消息（自己发送的消息已经在发送时显示，避免重复）
            bool isFromSelf = msgObj[kTIMMsgIsFormSelf].toBool();
            QString senderId = msgObj[kTIMMsgSender].toString();
            
            // 解析消息元素
            if (!msgObj.contains(kTIMMsgElemArray) || !msgObj[kTIMMsgElemArray].isArray()) {
                continue;
            }
            
            QJsonArray elemArray = msgObj[kTIMMsgElemArray].toArray();
            for (const QJsonValue& elemValue : elemArray) {
                if (!elemValue.isObject()) continue;
                
                QJsonObject elemObj = elemValue.toObject();
                int elemType = elemObj[kTIMElemType].toInt();
                
                QString senderName = senderId;
                // 尝试从发送者信息中获取名称
                if (msgObj.contains(kTIMMsgSenderProfile)) {
                    QJsonObject senderProfile = msgObj[kTIMMsgSenderProfile].toObject();
                    if (senderProfile.contains("user_profile_nick_name")) {
                        senderName = senderProfile["user_profile_nick_name"].toString();
                    }
                }
                
                // 如果是自己的消息，已经显示过了，跳过
                if (isFromSelf || senderId == userInfo.classId) {
                    continue;
                }
                
                // 根据元素类型处理
                if (elemType == kTIMElem_Text) {
                    QString text = elemObj[kTIMTextElemContent].toString();
                    addTextMessage(":/res/img/home.png", senderName, text, false);
                } else if (elemType == kTIMElem_Image) {
                    // 处理图片消息
                    handleImageMessage(elemObj, senderName, false);
                } else if (elemType == kTIMElem_File) {
                    // 处理文件消息
                    handleFileMessage(elemObj, senderName, false);
                } else if (elemType == kTIMElem_Sound) {
                    // 处理语音消息
                    handleVoiceMessage(elemObj, senderName, false);
                }
            }
        }
    }
    
    // 加载历史消息（包括离线消息）
    void loadHistoryMessages()
    {
        if (m_unique_group_id.isEmpty()) return;
        
        // 构造获取消息参数
        QJsonObject getMsgParam;
        // 不指定 LastMsg，表示获取最新的消息
        getMsgParam[kTIMMsgGetMsgListParamCount] = 100; // 获取最近100条消息
        getMsgParam[kTIMMsgGetMsgListParamIsRamble] = true; // 开启漫游消息，会从云端拉取离线消息
        getMsgParam[kTIMMsgGetMsgListParamIsForward] = false; // false表示获取比最新消息更旧的消息（历史消息）
        
        QJsonDocument doc(getMsgParam);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        // 创建回调数据结构
        struct LoadHistoryCallbackData {
            ChatDialog* dlg;
            QString groupId;
        };
        LoadHistoryCallbackData* callbackData = new LoadHistoryCallbackData;
        callbackData->dlg = this;
        callbackData->groupId = m_unique_group_id;
        
        // 调用获取消息列表接口
        QByteArray groupIdBytes = m_unique_group_id.toUtf8();
        int ret = TIMMsgGetMsgList(groupIdBytes.constData(), kTIMConv_Group, jsonData.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                LoadHistoryCallbackData* data = (LoadHistoryCallbackData*)user_data;
                ChatDialog* dlg = data->dlg;
                
                if (code != TIM_SUCC) {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    qDebug() << "加载历史消息失败，群组ID:" << data->groupId << "，错误码:" << code << "，描述:" << errorDesc;
                    delete data;
                    return;
                }
                
                // 解析返回的消息列表
                if (json_params) {
                    QJsonParseError parseError;
                    QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json_params), &parseError);
                    if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                        QJsonArray msgArray = doc.array();
                        qDebug() << "加载历史消息成功，群组ID:" << data->groupId << "，消息数量:" << msgArray.size();
                        
                        // 将历史消息添加到界面（按时间倒序，因为获取的是从新到旧）
                        // 需要反转顺序，从旧到新显示
                        QVector<QJsonObject> msgList;
                        for (const QJsonValue& value : msgArray) {
                            if (value.isObject()) {
                                msgList.prepend(value.toObject()); // 使用prepend保持从旧到新的顺序
                            }
                        }
                        
                        // 批量显示历史消息
                        UserInfo userInfo = CommonInfo::GetData();
                        for (const QJsonObject& msgObj : msgList) {
                            QString convId = msgObj[kTIMMsgConvId].toString();
                            if (convId != data->groupId) continue; // 只处理当前群组的消息
                            
                            bool isFromSelf = msgObj[kTIMMsgIsFormSelf].toBool();
                            QString senderId = msgObj[kTIMMsgSender].toString();
                            
                            if (!msgObj.contains(kTIMMsgElemArray) || !msgObj[kTIMMsgElemArray].isArray()) {
                                continue;
                            }
                            
                            QJsonArray elemArray = msgObj[kTIMMsgElemArray].toArray();
                            for (const QJsonValue& elemValue : elemArray) {
                                if (!elemValue.isObject()) continue;
                                
                                QJsonObject elemObj = elemValue.toObject();
                                int elemType = elemObj[kTIMElemType].toInt();
                                
                                QString senderName = senderId;
                                if (msgObj.contains(kTIMMsgSenderProfile)) {
                                    QJsonObject senderProfile = msgObj[kTIMMsgSenderProfile].toObject();
                                    if (senderProfile.contains("user_profile_nick_name")) {
                                        senderName = senderProfile["user_profile_nick_name"].toString();
                                    }
                                }
                                
                                bool isMine = (isFromSelf || senderId == userInfo.classId);
                                
                                if (elemType == kTIMElem_Text) {
                                    QString text = elemObj[kTIMTextElemContent].toString();
                                    dlg->addTextMessage(":/res/img/home.png", senderName, text, isMine);
                                } else if (elemType == kTIMElem_Image) {
                                    // 处理历史消息中的图片
                                    dlg->handleImageMessage(elemObj, senderName, isMine);
                                } else if (elemType == kTIMElem_File) {
                                    // 处理历史消息中的文件
                                    dlg->handleFileMessage(elemObj, senderName, isMine);
                                } else if (elemType == kTIMElem_Sound) {
                                    // 处理历史消息中的语音
                                    dlg->handleVoiceMessage(elemObj, senderName, isMine);
                                }
                            }
                        }
                    } else {
                        qDebug() << "解析历史消息JSON失败:" << parseError.errorString();
                    }
                }
                
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            qDebug() << "调用TIMMsgGetMsgList失败，错误码:" << ret;
            delete callbackData;
        }
    }

    // 静态实例列表和回调注册标志（用于消息分发）
    static QList<ChatDialog*>& getInstanceList()
    {
        static QList<ChatDialog*> s_instances;
        return s_instances;
    }

    static QList<QByteArray>& getPendingMessages()
    {
        static QList<QByteArray> s_pendingMessages;
        return s_pendingMessages;
    }
    
    // 静态函数：接收新消息回调（用于分发消息到所有实例）
    static void staticRecvNewMsgCallback(const char* json_msg_array, const void* user_data)
    {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json_msg_array), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "ChatDialog::staticRecvNewMsgCallback 解析失败:" << parseError.errorString();
            return;
        }
        if (!doc.isArray()) {
            qDebug() << "ChatDialog::staticRecvNewMsgCallback 数据不是数组";
            return;
        }

        QJsonArray msgArray = doc.array();
        QJsonArray forwardArray;

        for (const QJsonValue& value : msgArray) {
            if (!value.isObject()) continue;
            QJsonObject msgObj = value.toObject();
            QJsonArray elemArray = msgObj.value("message_elem_array").toArray();
            QJsonArray groupTips;
            for (const QJsonValue& elemValue : elemArray) {
                if (!elemValue.isObject()) continue;
                QJsonObject elemObj = elemValue.toObject();
                int elemType = elemObj.value("elem_type").toInt();
                if (elemType == kTIMElem_GroupTips) {
                    groupTips.append(elemObj);
                }
            }
            if (!groupTips.isEmpty()) {
                GroupNotifyDialog::processIncomingGroupTips(groupTips);
            } else {
                forwardArray.append(msgObj);
            }
        }

        if (forwardArray.isEmpty())
            return;

        QJsonDocument forwardDoc(forwardArray);
        QByteArray forwardBytes = forwardDoc.toJson(QJsonDocument::Compact);

        QList<ChatDialog*>& instances = getInstanceList();
        if (instances.isEmpty()) {
            getPendingMessages().append(forwardBytes);
            return;
        }

        for (ChatDialog* dlg : instances) {
            if (dlg) {
                dlg->onRecvNewMsg(forwardBytes.constData());
            }
        }
    }
    
    // 注册/注销实例到静态列表（用于消息分发）
    void registerInstance()
    {
        // 确保回调已注册（如果还没注册的话）
        ensureCallbackRegistered();
        
        QList<ChatDialog*>& instances = getInstanceList();
        
        if (!instances.contains(this)) {
            instances.append(this);
            QList<QByteArray>& pending = getPendingMessages();
            if (!pending.isEmpty()) {
                for (const QByteArray& bytes : pending) {
                    onRecvNewMsg(bytes.constData());
                }
                pending.clear();
            }
        }
    }
    
    void unregisterInstance()
    {
        QList<ChatDialog*>& instances = getInstanceList();
        instances.removeAll(this);
    }

    // 处理接收到的图片消息
    void handleImageMessage(const QJsonObject& imageElem, const QString& senderName, bool isMine)
    {
        // 获取图片URL（优先使用大图，PC端建议使用大图）
        QString imageUrl = imageElem[kTIMImageElemLargeUrl].toString(); // 大图URL
        QString imageId = imageElem[kTIMImageElemLargeId].toString();   // 大图ID
        
        // 如果大图URL为空，尝试使用缩略图
        if (imageUrl.isEmpty()) {
            imageUrl = imageElem[kTIMImageElemThumbUrl].toString();
            imageId = imageElem[kTIMImageElemThumbId].toString();
        }
        
        // 如果还是为空，尝试使用原图
        if (imageUrl.isEmpty()) {
            imageUrl = imageElem[kTIMImageElemOrigUrl].toString();
            imageId = imageElem[kTIMImageElemOrigId].toString();
        }
        
        if (imageUrl.isEmpty() && imageId.isEmpty()) {
            qDebug() << "图片消息URL和ID都为空，无法下载";
            addTextMessage(":/res/img/home.png", senderName, "[图片]（无法获取）", isMine);
            return;
        }
        
        // 创建临时目录保存下载的图片
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir dir;
        if (!dir.exists(cacheDir)) {
            dir.mkpath(cacheDir);
        }
        
        // 生成本地保存路径
        QString fileName = QString("image_%1_%2.jpg").arg(imageId.isEmpty() ? "unknown" : imageId).arg(QDateTime::currentDateTime().toSecsSinceEpoch());
        QString localPath = QDir(cacheDir).filePath(fileName);
        
        // 如果URL不为空，直接使用HTTP下载（SDK回调不可靠，优先使用HTTP）
        if (!imageUrl.isEmpty()) {
            downloadImageFromUrl(imageUrl, localPath, senderName, isMine);
        } else if (!imageId.isEmpty()) {
            // 只有ID没有URL，尝试使用SDK下载
            downloadImageFromSDK(imageElem, localPath, senderName, isMine);
        } else {
            qDebug() << "图片消息URL和ID都为空，无法下载";
            addTextMessage(":/res/img/home.png", senderName, "[图片]（无法获取）", isMine);
        }
    }
    
    // 处理接收到的语音消息
    void handleVoiceMessage(const QJsonObject& soundElem, const QString& senderName, bool isMine)
    {
        // 获取语音信息
        int duration = soundElem[kTIMSoundElemFileTime].toInt(); // 语音时长（秒）
        QString voiceUrl = soundElem[kTIMSoundElemUrl].toString(); // 语音下载URL
        QString voiceId = soundElem[kTIMSoundElemFileId].toString(); // 语音ID
        int voiceFileSize = soundElem[kTIMSoundElemFileSize].toInt(); // 文件大小
        
        if (duration <= 0 && voiceUrl.isEmpty() && voiceId.isEmpty()) {
            qDebug() << "语音消息缺少必要信息，无法处理";
            addTextMessage(":/res/img/home.png", senderName, "[语音]（信息不完整）", isMine);
            return;
        }
        
        // 创建临时目录保存下载的语音
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir dir;
        if (!dir.exists(cacheDir)) {
            dir.mkpath(cacheDir);
        }
        
        // 生成本地保存路径
        QString localFileName = voiceId.isEmpty() ? 
            QString("voice_%1.amr").arg(QDateTime::currentMSecsSinceEpoch()) :
            QString("voice_%1.amr").arg(voiceId);
        QString localPath = QDir(cacheDir).filePath(localFileName);
        
        // 先显示语音消息UI（带进度条）
        QPair<QProgressBar*, QLabel*> progressWidgets = addVoiceMessageWithProgress(":/res/img/home.png", senderName, duration > 0 ? duration : 1, localPath, isMine);
        
        // 如果语音URL不为空，下载语音
        if (!voiceUrl.isEmpty()) {
            downloadVoiceFromUrl(voiceUrl, localPath, senderName, duration, isMine, progressWidgets.first, progressWidgets.second, voiceFileSize);
        } else if (!voiceId.isEmpty()) {
            // 如果只有voiceId没有URL，尝试使用腾讯SDK下载
            downloadVoiceFromSDK(soundElem, localPath, senderName, duration, isMine, progressWidgets.first, progressWidgets.second);
        } else {
            // 如果既没有URL也没有ID，显示语音信息（可能语音还在上传中）
            qDebug() << "语音消息URL和ID都为空，无法下载";
            if (progressWidgets.second) {
                progressWidgets.second->setText("无法获取语音文件");
                progressWidgets.second->setStyleSheet("color: red; font-size: 10px;");
            }
        }
    }
    
    // 从URL下载语音
    void downloadVoiceFromUrl(const QString& voiceUrl, const QString& savePath, const QString& senderName,
                             int duration, bool isMine, QProgressBar* progressBar = nullptr, QLabel* statusLabel = nullptr, int voiceFileSize = 0)
    {
        QUrl url(voiceUrl);
        if (!url.isValid()) {
            qDebug() << "语音URL无效:" << voiceUrl;
            return;
        }
        
        // 检查SSL支持
        if (url.scheme().toLower() == "https") {
            if (!QSslSocket::supportsSsl()) {
                qDebug() << "系统不支持SSL，尝试将HTTPS改为HTTP或使用其他下载方式";
                QString httpUrl = voiceUrl;
                httpUrl.replace("https://", "http://");
                if (httpUrl != voiceUrl) {
                    qDebug() << "尝试使用HTTP替代HTTPS:" << httpUrl;
                    url = QUrl(httpUrl);
                }
            }
        }
        
        // 检查文件是否已部分下载（断点续传）
        qint64 existingFileSize = 0;
        bool isResume = false;
        QFileInfo fileInfo(savePath);
        if (fileInfo.exists()) {
            existingFileSize = fileInfo.size();
            if (existingFileSize > 0 && voiceFileSize > 0 && existingFileSize < voiceFileSize) {
                isResume = true;
                qDebug() << "检测到已部分下载的语音文件，大小:" << existingFileSize << "字节，将断点续传";
            } else if (voiceFileSize > 0 && existingFileSize >= voiceFileSize) {
                // 文件已完整下载
                qDebug() << "语音文件已完整下载，大小:" << existingFileSize << "字节";
                if (statusLabel && statusLabel->parent()) {
                    statusLabel->setText("文件已存在");
                    statusLabel->setStyleSheet("color: green; font-size: 10px;");
                }
                if (progressBar && progressBar->parent()) {
                    progressBar->setValue(100);
                }
                // 更新语音消息的路径，以便播放
                updateVoiceMessagePath(senderName, duration, savePath);
                return;
            }
        }
        
        // 保存this指针，以便在lambda中使用
        ChatDialog* dlg = this;
        
        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QNetworkRequest networkRequest;
        networkRequest.setUrl(url);
        
        // 如果是断点续传，设置 Range 请求头
        if (isResume) {
            QString rangeHeader = QString("bytes=%1-").arg(existingFileSize);
            networkRequest.setRawHeader("Range", rangeHeader.toUtf8());
            qDebug() << "设置 Range 请求头:" << rangeHeader;
        }
        
        // 如果是HTTPS，设置SSL配置
        if (url.scheme().toLower() == "https") {
            QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
            sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
            sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
            networkRequest.setSslConfiguration(sslConfig);
        }
        
        // 保存下载进度映射
        QString normalizedKey = QDir::toNativeSeparators(savePath);
        if (progressBar && statusLabel) {
            m_voiceDownloadProgressMap[normalizedKey] = qMakePair(progressBar, statusLabel);
        }
        
        // 更新状态为"正在下载"或"正在续传"
        if (statusLabel) {
            statusLabel->setText(isResume ? "正在续传..." : "正在下载...");
            statusLabel->setStyleSheet("color: blue; font-size: 10px;");
        }
        if (progressBar) {
            if (isResume && voiceFileSize > 0) {
                // 设置初始进度（已下载部分）
                int initialProgress = (int)((double)existingFileSize / voiceFileSize * 100);
                progressBar->setValue(initialProgress);
            } else {
                progressBar->setValue(0);
            }
        }
        
        qDebug() << "保存语音下载进度映射，路径:" << normalizedKey << "，断点续传:" << isResume;
        
        QNetworkReply* reply = manager->get(networkRequest);
        
        // 忽略SSL错误
        connect(reply, &QNetworkReply::sslErrors, this, [=](const QList<QSslError>& errors) {
            qDebug() << "SSL错误，忽略证书验证，URL:" << voiceUrl;
            reply->ignoreSslErrors();
        });
        
        // 保存断点续传信息到lambda中
        qint64 resumeOffset = existingFileSize;
        bool isResumeDownload = isResume;
        
        // 连接下载进度信号（考虑断点续传）
        connect(reply, &QNetworkReply::downloadProgress, this, [progressBar, statusLabel, voiceFileSize, resumeOffset, isResumeDownload](qint64 bytesReceived, qint64 bytesTotal) {
            if (progressBar && progressBar->parent()) {
                // 计算总进度（已下载部分 + 本次下载部分）
                qint64 totalReceived = isResumeDownload ? resumeOffset + bytesReceived : bytesReceived;
                qint64 totalSize = voiceFileSize > 0 ? voiceFileSize : (isResumeDownload ? resumeOffset + bytesTotal : bytesTotal);
                
                if (totalSize > 0) {
                    int progress = (int)((double)totalReceived / totalSize * 100);
                    progressBar->setValue(progress);
                    
                    if (statusLabel && statusLabel->parent()) {
                        QString receivedStr = totalReceived < 1024 ? QString("%1 字节").arg(totalReceived) :
                                             totalReceived < 1024 * 1024 ? QString("%1 KB").arg(totalReceived / 1024.0, 0, 'f', 1) :
                                             QString("%1 MB").arg(totalReceived / (1024.0 * 1024.0), 0, 'f', 1);
                        QString totalStr = totalSize < 1024 ? QString("%1 字节").arg(totalSize) :
                                          totalSize < 1024 * 1024 ? QString("%1 KB").arg(totalSize / 1024.0, 0, 'f', 1) :
                                          QString("%1 MB").arg(totalSize / (1024.0 * 1024.0), 0, 'f', 1);
                        QString statusText = isResumeDownload ? 
                            QString("续传中: %1 / %2 (%3%)").arg(receivedStr).arg(totalStr).arg(progress) :
                            QString("下载中: %1 / %2 (%3%)").arg(receivedStr).arg(totalStr).arg(progress);
                        statusLabel->setText(statusText);
                        statusLabel->setStyleSheet("color: blue; font-size: 10px;");
                    }
                }
            }
        });
        
        connect(reply, &QNetworkReply::finished, this, [this, reply, manager, voiceUrl, dlg, savePath, senderName, duration, resumeOffset, isResumeDownload]() {
            int error = reply->error();
            QString normalizedKey = QDir::toNativeSeparators(savePath);
            QProgressBar* progressBar = nullptr;
            QLabel* statusLabel = nullptr;

            if (m_voiceDownloadProgressMap.contains(normalizedKey)) {
                progressBar = m_voiceDownloadProgressMap[normalizedKey].first;
                statusLabel = m_voiceDownloadProgressMap[normalizedKey].second;
            }

            // 检查HTTP状态码（206表示部分内容，支持断点续传）
            int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            bool isPartialContent = (httpStatusCode == 206); // HTTP 206 Partial Content
            
            if (reply->error() == QNetworkReply::NoError || isPartialContent) {
                QByteArray voiceData = reply->readAll();
                if (voiceData.isEmpty() && !isResumeDownload) {
                    qDebug() << "语音数据为空";
                    if (statusLabel && statusLabel->parent()) {
                        statusLabel->setText("语音数据为空");
                        statusLabel->setStyleSheet("color: red; font-size: 10px;");
                    }
                }
                else {
                    // 断点续传时使用追加模式，否则使用覆盖模式
                    QFile file(savePath);
                    QIODevice::OpenMode openMode = isResumeDownload ? QIODevice::Append : QIODevice::WriteOnly;
                    
                    if (file.open(openMode)) {
                        file.write(voiceData);
                        file.close();
                        
                        // 获取最终文件大小
                        QFileInfo finalFileInfo(savePath);
                        qint64 finalSize = finalFileInfo.exists() ? finalFileInfo.size() : 0;
                        qDebug() << "语音下载成功:" << savePath;
                        qDebug() << "本次下载:" << voiceData.size() << "字节";
                        if (isResumeDownload) {
                            qDebug() << "断点续传: 原有" << resumeOffset << "字节 + 本次" << voiceData.size() << "字节 = 总计" << finalSize << "字节";
                        }

                        // 更新状态为成功
                        if (statusLabel && statusLabel->parent()) {
                            statusLabel->setText("下载成功");
                            statusLabel->setStyleSheet("color: green; font-size: 10px;");
                        }
                        if (progressBar && progressBar->parent()) {
                            progressBar->setValue(100);
                            // 3秒后隐藏进度条
                            QTimer::singleShot(3000, progressBar, [progressBar, statusLabel]() {
                                if (progressBar && progressBar->parent()) {
                                    progressBar->hide();
                                }
                                if (statusLabel && statusLabel->parent()) {
                                    statusLabel->hide();
                                }
                            });
                        }
                        
                        // 更新语音消息的路径，以便播放
                        updateVoiceMessagePath(senderName, duration, savePath);
                    } else {
                        qDebug() << "保存语音失败:" << savePath;
                        if (statusLabel && statusLabel->parent()) {
                            statusLabel->setText("保存失败");
                            statusLabel->setStyleSheet("color: red; font-size: 10px;");
                        }
                    }
                }
            } else {
                qDebug() << "语音下载失败，URL:" << voiceUrl << "，错误:" << reply->errorString();
                if (statusLabel && statusLabel->parent()) {
                    statusLabel->setText(QString("下载失败: %1").arg(reply->errorString()));
                    statusLabel->setStyleSheet("color: red; font-size: 10px;");
                }
                if (progressBar && progressBar->parent()) {
                    progressBar->setValue(0);
                }
            }
            
            // 从进度映射中移除
            m_voiceDownloadProgressMap.remove(normalizedKey);
            
            reply->deleteLater();
            manager->deleteLater();
        });
    }
    
    // 使用腾讯SDK下载语音（如果只有voiceId没有URL）
    void downloadVoiceFromSDK(const QJsonObject& soundElem, const QString& savePath, const QString& senderName,
                             int duration, bool isMine, QProgressBar* progressBar = nullptr, QLabel* statusLabel = nullptr)
    {
        qDebug() << "使用腾讯SDK下载语音，voiceId:" << soundElem[kTIMSoundElemFileId].toString();
        
        // 构造语音元素JSON（用于下载）
        QJsonDocument elemDoc(soundElem);
        QByteArray elemJsonData = elemDoc.toJson(QJsonDocument::Compact);
        QByteArray pathBytes = savePath.toUtf8();
        
        // 保存下载进度映射
        QString normalizedKey = QDir::toNativeSeparators(savePath);
        if (progressBar && statusLabel) {
            m_voiceDownloadProgressMap[normalizedKey] = qMakePair(progressBar, statusLabel);
        }
        
        // 更新状态为正在下载
        if (statusLabel) {
            statusLabel->setText("正在下载...");
            statusLabel->setStyleSheet("color: blue; font-size: 10px;");
        }
        if (progressBar) {
            progressBar->setValue(0);
        }
        
        // 创建回调数据结构
        struct DownloadVoiceCallbackData {
            ChatDialog* dlg;
            QString savePath;
            QString senderName;
            int duration;
        };
        DownloadVoiceCallbackData* callbackData = new DownloadVoiceCallbackData;
        callbackData->dlg = this;
        callbackData->savePath = savePath;
        callbackData->senderName = senderName;
        callbackData->duration = duration;
        
        // 调用腾讯SDK下载接口
        int ret = TIMMsgDownloadElemToPath(elemJsonData.constData(), pathBytes.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                DownloadVoiceCallbackData* data = (DownloadVoiceCallbackData*)user_data;
                ChatDialog* dlg = data->dlg;
                
                // 从map中获取进度条和状态标签
                QString normalizedKey = QDir::toNativeSeparators(data->savePath);
                QProgressBar* progressBar = nullptr;
                QLabel* statusLabel = nullptr;
                
                if (dlg->m_voiceDownloadProgressMap.contains(normalizedKey)) {
                    progressBar = dlg->m_voiceDownloadProgressMap[normalizedKey].first;
                    statusLabel = dlg->m_voiceDownloadProgressMap[normalizedKey].second;
                }
                
                if (code == TIM_SUCC) {
                    // 下载成功，检查文件是否存在
                    QFile file(data->savePath);
                    if (file.exists()) {
                        qDebug() << "SDK语音下载成功:" << data->savePath;
                        
                        // 更新状态为成功
                        if (statusLabel && statusLabel->parent()) {
                            statusLabel->setText("下载成功");
                            statusLabel->setStyleSheet("color: green; font-size: 10px;");
                        }
                        if (progressBar && progressBar->parent()) {
                            progressBar->setValue(100);
                            // 3秒后隐藏进度条
                            QTimer::singleShot(3000, progressBar, [progressBar, statusLabel]() {
                                if (progressBar && progressBar->parent()) {
                                    progressBar->hide();
                                }
                                if (statusLabel && statusLabel->parent()) {
                                    statusLabel->hide();
                                }
                            });
                        }
                        
                        // 更新语音消息的路径，以便播放
                        dlg->updateVoiceMessagePath(data->senderName, data->duration, data->savePath);
                    } else {
                        qDebug() << "SDK下载成功但文件不存在:" << data->savePath;
                        if (statusLabel && statusLabel->parent()) {
                            statusLabel->setText("文件不存在");
                            statusLabel->setStyleSheet("color: red; font-size: 10px;");
                        }
                    }
                } else {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    qDebug() << "SDK语音下载失败，错误码:" << code << "，描述:" << errorDesc;
                    
                    // 更新状态为失败
                    if (statusLabel && statusLabel->parent()) {
                        statusLabel->setText(QString("下载失败: %1").arg(errorDesc));
                        statusLabel->setStyleSheet("color: red; font-size: 10px;");
                    }
                    if (progressBar && progressBar->parent()) {
                        progressBar->setValue(0);
                    }
                }
                
                // 从进度映射中移除
                dlg->m_voiceDownloadProgressMap.remove(normalizedKey);
                
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            qDebug() << "调用TIMMsgDownloadElemToPath失败，错误码:" << ret;
            delete callbackData;
        }
    }
    
    // 更新语音消息的路径（下载完成后调用）
    void updateVoiceMessagePath(const QString& senderName, int duration, const QString& voicePath)
    {
        // 遍历列表项，找到对应的语音消息并更新路径
        for (int i = m_listWidget->count() - 1; i >= 0; i--) {
            QListWidgetItem* item = m_listWidget->item(i);
            if (!item) continue;
            
            QWidget* widget = m_listWidget->itemWidget(item);
            if (!widget) continue;
            
            // 查找包含语音标签的widget
            ClickableLabelEx* voiceLabel = widget->findChild<ClickableLabelEx*>();
            if (voiceLabel && voiceLabel->text().contains("语音")) {
                // 检查是否是匹配的语音消息（通过senderName和duration）
                // 这里简化处理，更新第一个找到的未设置路径的语音消息
                QString existingPath = voiceLabel->property("voicePath").toString();
                if (existingPath.isEmpty()) {
                    voiceLabel->setProperty("voicePath", voicePath);
                    qDebug() << "更新语音消息路径:" << voicePath;
                    break;
                }
            }
        }
    }
    
    // 播放语音消息（使用QMediaPlayer，后台播放，显示播放进度）
    void playVoiceMessage(const QString& voicePath)
    {
        if (voicePath.isEmpty()) {
            QMessageBox::information(this, "提示", "语音文件路径为空");
            return;
        }
        
        QFileInfo fileInfo(voicePath);
        if (!fileInfo.exists()) {
            QMessageBox::information(this, "提示", QString("语音文件不存在：%1").arg(voicePath));
            return;
        }
        
        qDebug() << "播放语音:" << voicePath;
        
        // 如果已有播放器正在播放，立即停止并删除
        if (m_voicePlayer) {
            // 断开所有信号连接，避免回调干扰
            m_voicePlayer->disconnect();
            // 立即停止播放
            m_voicePlayer->stop();
            // 立即删除，不使用deleteLater，确保立即释放资源
            delete m_voicePlayer;
            m_voicePlayer = nullptr;
            qDebug() << "已停止并删除前一个播放器";
        }
        
        // 创建新的媒体播放器
        m_voicePlayer = new QMediaPlayer(this);
        
        // 设置音量（0-100，默认100）
        m_voicePlayer->setVolume(100);
        
        // 设置媒体内容
        QUrl fileUrl = QUrl::fromLocalFile(voicePath);
        m_voicePlayer->setMedia(QMediaContent(fileUrl));
        qDebug() << "设置媒体内容，URL:" << fileUrl.toString();
        
        // 连接媒体状态变化信号（用于自动播放和状态监控）
        connect(m_voicePlayer, &QMediaPlayer::mediaStatusChanged, this, [this, voicePath](QMediaPlayer::MediaStatus status) {
            qDebug() << "媒体状态变化:" << status;
            // 确保是当前播放器（防止切换时旧播放器的回调）
            if (m_voicePlayer && m_voicePlayer->media().canonicalUrl().toLocalFile() == voicePath) {
                switch (status) {
                    case QMediaPlayer::LoadedMedia:
                        qDebug() << "媒体已加载，准备播放";
                        // 媒体已加载，立即开始播放
                        if (m_voicePlayer->state() != QMediaPlayer::PlayingState) {
                            m_voicePlayer->play();
                            qDebug() << "媒体已加载，开始播放";
                        }
                        break;
                    case QMediaPlayer::BufferedMedia:
                        qDebug() << "媒体已缓冲";
                        // 如果还没开始播放，现在开始
                        if (m_voicePlayer->state() != QMediaPlayer::PlayingState) {
                            m_voicePlayer->play();
                            qDebug() << "媒体已缓冲，开始播放";
                        }
                        break;
                    case QMediaPlayer::InvalidMedia:
                        qDebug() << "无效的媒体文件";
                        QMessageBox::warning(this, "播放错误", "无法加载语音文件，可能是文件格式不支持");
                        break;
                    default:
                        break;
                }
            }
        });
        
        // 连接播放状态变化信号
        connect(m_voicePlayer, &QMediaPlayer::stateChanged, this, [this, voicePath](QMediaPlayer::State state) {
            qDebug() << "播放状态变化:" << state;
            QString normalizedPath = QDir::toNativeSeparators(voicePath);
            QProgressBar* playProgressBar = nullptr;
            QLabel* playStatusLabel = nullptr;
            
            if (m_voicePlayProgressMap.contains(normalizedPath)) {
                playProgressBar = m_voicePlayProgressMap[normalizedPath].first;
                playStatusLabel = m_voicePlayProgressMap[normalizedPath].second;
            }
            
            switch (state) {
                case QMediaPlayer::PlayingState:
                    qDebug() << "正在播放";
                    if (playProgressBar && playProgressBar->parent()) {
                        playProgressBar->show();
                    }
                    if (playStatusLabel && playStatusLabel->parent()) {
                        playStatusLabel->setText("播放中...");
                        playStatusLabel->setStyleSheet("color: #2196F3; font-size: 10px;");
                        playStatusLabel->show();
                    }
                    break;
                case QMediaPlayer::StoppedState:
                    qDebug() << "播放已停止";
                    if (playProgressBar && playProgressBar->parent()) {
                        playProgressBar->setValue(0);
                        playProgressBar->hide();
                    }
                    if (playStatusLabel && playStatusLabel->parent()) {
                        playStatusLabel->hide();
                    }
                    break;
                case QMediaPlayer::PausedState:
                    qDebug() << "播放已暂停";
                    if (playStatusLabel && playStatusLabel->parent()) {
                        playStatusLabel->setText("已暂停");
                        playStatusLabel->setStyleSheet("color: #FF9800; font-size: 10px;");
                    }
                    break;
            }
        });
        
        // 连接播放进度信号
        connect(m_voicePlayer, &QMediaPlayer::positionChanged, this, [this, voicePath](qint64 position) {
            QString normalizedPath = QDir::toNativeSeparators(voicePath);
            if (m_voicePlayProgressMap.contains(normalizedPath)) {
                QProgressBar* playProgressBar = m_voicePlayProgressMap[normalizedPath].first;
                if (playProgressBar && playProgressBar->parent() && m_voicePlayer) {
                    qint64 duration = m_voicePlayer->duration();
                    if (duration > 0) {
                        int progress = (int)((double)position / duration * 100);
                        playProgressBar->setValue(progress);
                        
                        // 更新状态标签显示时间
                        QLabel* playStatusLabel = m_voicePlayProgressMap[normalizedPath].second;
                        if (playStatusLabel && playStatusLabel->parent()) {
                            int currentSec = position / 1000;
                            int totalSec = duration / 1000;
                            playStatusLabel->setText(QString("播放中: %1/%2 秒").arg(currentSec).arg(totalSec));
                        }
                    }
                }
            }
        });
        
        // 连接错误信号
        connect(m_voicePlayer, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), 
                this, [this](QMediaPlayer::Error error) {
            QString errorMsg;
            switch (error) {
                case QMediaPlayer::ResourceError:
                    errorMsg = "资源错误，无法播放语音文件";
                    break;
                case QMediaPlayer::FormatError:
                    errorMsg = "格式错误，不支持该音频格式";
                    break;
                case QMediaPlayer::NetworkError:
                    errorMsg = "网络错误";
                    break;
                case QMediaPlayer::AccessDeniedError:
                    errorMsg = "访问被拒绝";
                    break;
                default:
                    errorMsg = "未知错误";
                    break;
            }
            qDebug() << "语音播放错误:" << errorMsg << "，错误码:" << error;
            QMessageBox::warning(this, "播放错误", errorMsg);
        });
        
        // 尝试立即播放（不等待状态变化，让状态变化回调处理）
        // 如果媒体已经加载，立即播放
        QMediaPlayer::MediaStatus currentStatus = m_voicePlayer->mediaStatus();
        if (currentStatus == QMediaPlayer::LoadedMedia || currentStatus == QMediaPlayer::BufferedMedia) {
            m_voicePlayer->play();
            qDebug() << "媒体已就绪，立即播放";
        } else {
            // 即使媒体还没加载，也尝试播放（QMediaPlayer会自动等待加载完成）
            m_voicePlayer->play();
            qDebug() << "尝试立即播放，等待媒体加载，当前状态:" << currentStatus;
        }
    }
    
    // 处理接收到的文件消息
    void handleFileMessage(const QJsonObject& fileElem, const QString& senderName, bool isMine)
    {
        // 获取文件信息
        QString fileName = fileElem[kTIMFileElemFileName].toString();
        int fileSize = fileElem[kTIMFileElemFileSize].toInt();
        QString fileUrl = fileElem[kTIMFileElemUrl].toString();
        QString fileId = fileElem[kTIMFileElemFileId].toString();
        
        if (fileName.isEmpty() && fileUrl.isEmpty() && fileId.isEmpty()) {
            qDebug() << "文件消息缺少必要信息，无法处理";
            addTextMessage(":/res/img/home.png", senderName, "[文件]（信息不完整）", isMine);
            return;
        }
        
        // 创建临时目录保存下载的文件
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir dir;
        if (!dir.exists(cacheDir)) {
            dir.mkpath(cacheDir);
        }
        
        // 生成本地保存路径（使用原始文件名）
        QString localFileName = fileName.isEmpty() ? QString("file_%1").arg(fileId.isEmpty() ? QString::number(QDateTime::currentMSecsSinceEpoch()) : fileId) : fileName;
        QString localPath = QDir(cacheDir).filePath(localFileName);
        
        // 先创建文件消息UI（带进度条和状态标签）
        QPair<QProgressBar*, QLabel*> progressWidgets = addFileMessageWithDownloadProgress(":/res/img/home.png", senderName, localPath, fileName, fileSize, isMine);
        
        // 保存文件下载信息，用于重试
        QString normalizedKey = QDir::toNativeSeparators(localPath);
        FileDownloadInfo downloadInfo;
        downloadInfo.fileUrl = fileUrl;
        downloadInfo.fileElem = fileElem;
        downloadInfo.savePath = localPath;
        downloadInfo.senderName = senderName;
        downloadInfo.fileName = fileName;
        downloadInfo.fileSize = fileSize;
        downloadInfo.isMine = isMine;
        downloadInfo.progressBar = progressWidgets.first;
        downloadInfo.statusLabel = progressWidgets.second;
        m_fileDownloadInfoMap[normalizedKey] = downloadInfo;
        
        // 如果文件URL不为空，使用HTTP下载
        if (!fileUrl.isEmpty()) {
            // 下载文件（使用HTTP下载，带进度显示）
            downloadFileFromUrl(fileUrl, localPath, senderName, fileName, fileSize, isMine, progressWidgets.first, progressWidgets.second);
        } else if (!fileId.isEmpty()) {
            // 如果只有fileId没有URL，尝试使用腾讯SDK下载
            downloadFileFromSDK(fileElem, localPath, senderName, fileName, fileSize, isMine, progressWidgets.first, progressWidgets.second);
        } else {
            // 如果既没有URL也没有fileId，显示文件信息（可能文件还在上传中）
            if (progressWidgets.second) {
                progressWidgets.second->setText("等待文件上传完成...");
                progressWidgets.second->setStyleSheet("color: orange; font-size: 10px;");
            }
            if (progressWidgets.first) {
                progressWidgets.first->setValue(0);
            }
        }
    }
    
    // 添加带下载进度条的文件消息（用于接收文件）
    QPair<QProgressBar*, QLabel*> addFileMessageWithDownloadProgress(const QString& avatarPath, const QString& senderName,
                                                                     const QString& filePath, const QString& fileName,
                                                                     int fileSize, bool isMine)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_hasLastMessage || m_lastMessage.time.secsTo(now) > 180) addTimeLabel(now);
        bool hideAvatar = shouldHideAvatar(senderName, isMine, now);

        QString displayFileName = fileName.isEmpty() ? QFileInfo(filePath).fileName() : fileName;
        QString fileSizeStr = fileSize < 1024 ? QString("%1 字节").arg(fileSize) :
                             fileSize < 1024 * 1024 ? QString("%1 KB").arg(fileSize / 1024.0, 0, 'f', 1) :
                             QString("%1 MB").arg(fileSize / (1024.0 * 1024.0), 0, 'f', 1);

        // 创建文件消息容器
        QWidget* fileWidget = new QWidget();
        QVBoxLayout* fileLayout = new QVBoxLayout(fileWidget);
        fileLayout->setContentsMargins(5, 5, 5, 5);
        fileLayout->setSpacing(5);
        
        // 文件名和大小标签
        ClickableLabelEx* lblFile = new ClickableLabelEx(fileWidget);
        lblFile->setText(displayFileName + "\n" + fileSizeStr);
        lblFile->setStyleSheet("qproperty-alignment: AlignCenter;");
        lblFile->setMinimumSize(120, 50);
        connect(lblFile, &ClickableLabelEx::clicked, this, [=]() {
            QFileInfo fi(filePath);
            if (fi.exists()) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
            } else {
                QMessageBox::information(this, "提示", "文件尚未下载完成");
            }
        });
        fileLayout->addWidget(lblFile);
        
        // 创建下载进度条（设置父对象为fileWidget，确保生命周期）
        QProgressBar* progressBar = new QProgressBar(fileWidget);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        progressBar->setTextVisible(true);
        progressBar->setFormat("%p%");
        fileLayout->addWidget(progressBar);
        
        // 创建状态标签（设置父对象为fileWidget，确保生命周期）
        QLabel* statusLabel = new QLabel(fileWidget);
        statusLabel->setText("准备下载...");
        statusLabel->setStyleSheet("color: gray; font-size: 10px;");
        fileLayout->addWidget(statusLabel);
        
        // 创建重试按钮（初始隐藏，下载失败时显示）
        QPushButton* retryButton = new QPushButton("重试", fileWidget);
        retryButton->setStyleSheet("background-color: #4CAF50; color: white; padding: 4px 8px; font-size: 10px; border: none; border-radius: 3px;");
        retryButton->setMaximumWidth(60);
        retryButton->hide(); // 初始隐藏
        fileLayout->addWidget(retryButton);

        QWidget* msgWidget = buildMessageWidget(avatarPath, senderName, fileWidget, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);

        m_lastMessage = { avatarPath, senderName, "[文件] " + displayFileName, isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
        
        // 返回进度条、状态标签和重试按钮的指针
        // 使用QVBoxLayout存储重试按钮，以便后续访问
        return qMakePair(progressBar, statusLabel);
    }
    
    // 连接重试按钮（辅助函数，避免在lambda中使用QObject::connect）
    void connectRetryButton(QPushButton* btn, const QString& normalizedKey)
    {
        if (!btn || !m_fileDownloadInfoMap.contains(normalizedKey)) {
            return;
        }
        // 断开之前的连接
        btn->disconnect();
        // 连接重试按钮的点击事件
        QObject::connect(btn, &QPushButton::clicked, this, [this, normalizedKey]() {
            retryFileDownload(normalizedKey);
        });
    }
    
    // 重试文件下载
    void retryFileDownload(const QString& normalizedKey)
    {
        if (!m_fileDownloadInfoMap.contains(normalizedKey)) {
            qDebug() << "找不到文件下载信息，无法重试";
            return;
        }
        
        FileDownloadInfo& info = m_fileDownloadInfoMap[normalizedKey];
        
        qDebug() << "重试下载文件:" << info.fileName << "，路径:" << info.savePath;
        
        // 重置进度条和状态标签
        if (info.progressBar && info.progressBar->parent()) {
            info.progressBar->setValue(0);
            info.progressBar->show();
        }
        if (info.statusLabel && info.statusLabel->parent()) {
            info.statusLabel->setText("正在重试...");
            info.statusLabel->setStyleSheet("color: blue; font-size: 10px;");
            info.statusLabel->show();
        }
        
        // 隐藏重试按钮
        if (info.statusLabel && info.statusLabel->parent()) {
            QWidget* parentWidget = qobject_cast<QWidget*>(info.statusLabel->parent());
            if (parentWidget) {
                QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(parentWidget->layout());
                if (layout) {
                    for (int i = 0; i < layout->count(); i++) {
                        QPushButton* btn = qobject_cast<QPushButton*>(layout->itemAt(i)->widget());
                        if (btn && btn->text() == "重试") {
                            btn->hide();
                            break;
                        }
                    }
                }
            }
        }
        
        // 重新开始下载
        if (!info.fileUrl.isEmpty()) {
            downloadFileFromUrl(info.fileUrl, info.savePath, info.senderName, info.fileName, 
                              info.fileSize, info.isMine, info.progressBar, info.statusLabel);
        } else if (!info.fileElem.isEmpty()) {
            downloadFileFromSDK(info.fileElem, info.savePath, info.senderName, info.fileName,
                              info.fileSize, info.isMine, info.progressBar, info.statusLabel);
        }
    }
    
    // 从URL下载文件（带进度显示）
    void downloadFileFromUrl(const QString& fileUrl, const QString& savePath, const QString& senderName, 
                             const QString& fileName, int fileSize, bool isMine,
                             QProgressBar* progressBar = nullptr, QLabel* statusLabel = nullptr)
    {
        QUrl url(fileUrl);
        if (!url.isValid()) {
            qDebug() << "文件URL无效:" << fileUrl;
            if (statusLabel) {
                statusLabel->setText("URL无效");
                statusLabel->setStyleSheet("color: red; font-size: 10px;");
            }
            return;
        }
        
        // 检查SSL支持（如果系统不支持SSL，尝试将HTTPS改为HTTP或继续下载）
        if (url.scheme().toLower() == "https") {
            if (!QSslSocket::supportsSsl()) {
                qDebug() << "系统不支持SSL，尝试将HTTPS改为HTTP或使用其他下载方式";
                // 尝试将HTTPS改为HTTP（可能不工作，但试试）
                QString httpUrl = fileUrl;
                httpUrl.replace("https://", "http://");
                if (httpUrl != fileUrl) {
                    qDebug() << "尝试使用HTTP替代HTTPS:" << httpUrl;
                    url = QUrl(httpUrl);
                    if (!url.isValid()) {
                        qDebug() << "HTTP URL无效，继续使用HTTPS并忽略SSL错误";
                        url = QUrl(fileUrl); // 恢复原始URL
                    }
                } else {
                    qDebug() << "无法将HTTPS改为HTTP，继续使用HTTPS并忽略SSL错误";
                }
            }
        }
        
        // 检查文件是否已部分下载（断点续传）
        qint64 existingFileSize = 0;
        bool isResume = false;
        QFileInfo fileInfo(savePath);
        if (fileInfo.exists()) {
            existingFileSize = fileInfo.size();
            if (existingFileSize > 0 && existingFileSize < fileSize) {
                isResume = true;
                qDebug() << "检测到已部分下载的文件，大小:" << existingFileSize << "字节，将断点续传";
            } else if (existingFileSize >= fileSize) {
                // 文件已完整下载
                qDebug() << "文件已完整下载，大小:" << existingFileSize << "字节";
                if (statusLabel && statusLabel->parent()) {
                    statusLabel->setText("文件已存在");
                    statusLabel->setStyleSheet("color: green; font-size: 10px;");
                }
                if (progressBar && progressBar->parent()) {
                    progressBar->setValue(100);
                }
                return;
            }
        }
        
        // 更新状态为"正在下载"或"正在续传"
        if (statusLabel) {
            statusLabel->setText(isResume ? "正在续传..." : "正在下载...");
            statusLabel->setStyleSheet("color: blue; font-size: 10px;");
        }
        if (progressBar) {
            if (isResume && fileSize > 0) {
                // 设置初始进度（已下载部分）
                int initialProgress = (int)((double)existingFileSize / fileSize * 100);
                progressBar->setValue(initialProgress);
            } else {
                progressBar->setValue(0);
            }
        }
        
        // 保存下载进度映射
        QString normalizedKey = QDir::toNativeSeparators(savePath);
        m_fileDownloadProgressMap[normalizedKey] = qMakePair(progressBar, statusLabel);
        qDebug() << "保存文件下载进度映射，路径:" << normalizedKey << "，断点续传:" << isResume;
        
        // 保存this指针，以便在lambda中使用
        ChatDialog* dlg = this;
        
        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QNetworkRequest networkRequest;
        networkRequest.setUrl(url);
        
        // 如果是断点续传，设置 Range 请求头
        if (isResume) {
            QString rangeHeader = QString("bytes=%1-").arg(existingFileSize);
            networkRequest.setRawHeader("Range", rangeHeader.toUtf8());
            qDebug() << "设置 Range 请求头:" << rangeHeader;
        }
        
        // 如果是HTTPS，设置SSL配置
        if (url.scheme().toLower() == "https") {
            QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
            sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
            sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
            networkRequest.setSslConfiguration(sslConfig);
        }
        
        QNetworkReply* reply = manager->get(networkRequest);
        
        // 忽略SSL错误
        connect(reply, &QNetworkReply::sslErrors, this, [=](const QList<QSslError>& errors) {
            qDebug() << "SSL错误，忽略证书验证，URL:" << fileUrl;
            reply->ignoreSslErrors();
        });
        
        // 保存断点续传信息到lambda中
        qint64 resumeOffset = existingFileSize;
        bool isResumeDownload = isResume;
        
        // 连接下载进度信号（考虑断点续传）
        connect(reply, &QNetworkReply::downloadProgress, this, [progressBar, statusLabel, fileSize, resumeOffset, isResumeDownload](qint64 bytesReceived, qint64 bytesTotal) {
            if (progressBar && progressBar->parent()) {
                // 计算总进度（已下载部分 + 本次下载部分）
                qint64 totalReceived = isResumeDownload ? resumeOffset + bytesReceived : bytesReceived;
                qint64 totalSize = fileSize > 0 ? fileSize : (isResumeDownload ? resumeOffset + bytesTotal : bytesTotal);
                
                if (totalSize > 0) {
                    int progress = (int)((double)totalReceived / totalSize * 100);
                    progressBar->setValue(progress);
                    
                    if (statusLabel && statusLabel->parent()) {
                        QString receivedStr = totalReceived < 1024 ? QString("%1 字节").arg(totalReceived) :
                                             totalReceived < 1024 * 1024 ? QString("%1 KB").arg(totalReceived / 1024.0, 0, 'f', 1) :
                                             QString("%1 MB").arg(totalReceived / (1024.0 * 1024.0), 0, 'f', 1);
                        QString totalStr = totalSize < 1024 ? QString("%1 字节").arg(totalSize) :
                                          totalSize < 1024 * 1024 ? QString("%1 KB").arg(totalSize / 1024.0, 0, 'f', 1) :
                                          QString("%1 MB").arg(totalSize / (1024.0 * 1024.0), 0, 'f', 1);
                        QString statusText = isResumeDownload ? 
                            QString("续传中: %1 / %2 (%3%)").arg(receivedStr).arg(totalStr).arg(progress) :
                            QString("下载中: %1 / %2 (%3%)").arg(receivedStr).arg(totalStr).arg(progress);
                        statusLabel->setText(statusText);
                        statusLabel->setStyleSheet("color: blue; font-size: 10px;");
                    }
                }
            }
        });
        
        connect(reply, &QNetworkReply::finished, this, [this, reply, manager, fileUrl, dlg, savePath, fileSize, resumeOffset, isResumeDownload]() {
            int error = reply->error();
            QString normalizedKey = QDir::toNativeSeparators(savePath);
            QProgressBar* progressBar = nullptr;
            QLabel* statusLabel = nullptr;

            if (m_fileDownloadProgressMap.contains(normalizedKey)) {
                progressBar = m_fileDownloadProgressMap[normalizedKey].first;
                statusLabel = m_fileDownloadProgressMap[normalizedKey].second;
            }

            // 检查HTTP状态码（206表示部分内容，支持断点续传）
            int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            bool isPartialContent = (httpStatusCode == 206); // HTTP 206 Partial Content
            
            if (reply->error() == QNetworkReply::NoError || isPartialContent) {
                QByteArray fileData = reply->readAll();
                if (fileData.isEmpty() && !isResumeDownload) {
                    qDebug() << "文件数据为空";
                    if (statusLabel && statusLabel->parent()) {
                        statusLabel->setText("文件数据为空");
                        statusLabel->setStyleSheet("color: red; font-size: 10px;");
                    }
                }
                else {
                    // 断点续传时使用追加模式，否则使用覆盖模式
                    QFile file(savePath);
                    QIODevice::OpenMode openMode = isResumeDownload ? QIODevice::Append : QIODevice::WriteOnly;
                    
                    if (file.open(openMode)) {
                        file.write(fileData);
                        file.close();
                        
                        // 获取最终文件大小
                        QFileInfo finalFileInfo(savePath);
                        qint64 finalSize = finalFileInfo.exists() ? finalFileInfo.size() : 0;
                        qDebug() << "文件下载成功:" << savePath;
                        qDebug() << "本次下载:" << fileData.size() << "字节";
                        if (isResumeDownload) {
                            qDebug() << "断点续传: 原有" << resumeOffset << "字节 + 本次" << fileData.size() << "字节 = 总计" << finalSize << "字节";
                        }

                        // 更新状态为成功
                        if (statusLabel && statusLabel->parent()) {
                            statusLabel->setText("下载成功");
                            statusLabel->setStyleSheet("color: green; font-size: 10px;");
                        }
                        if (progressBar && progressBar->parent()) {
                            progressBar->setValue(100);
                            // 3秒后隐藏进度条
                            QTimer::singleShot(3000, progressBar, [progressBar, statusLabel]() {
                                if (progressBar && progressBar->parent()) {
                                    progressBar->hide();
                                }
                                if (statusLabel && statusLabel->parent()) {
                                    statusLabel->hide();
                                }
                                });
                        }
                    }
                    else {
                        qDebug() << "保存文件失败:" << savePath;
                        if (statusLabel && statusLabel->parent()) {
                            statusLabel->setText("保存失败");
                            statusLabel->setStyleSheet("color: red; font-size: 10px;");
                        }
                    }
                }
            }
            else {
                QString qerror = reply->errorString();
                qDebug() << "文件下载失败，URL:" << fileUrl;
                qDebug() << "错误码:" << error << "，错误描述:" << qerror;
                if (statusLabel && statusLabel->parent()) {
                    statusLabel->setText(QString("下载失败: %1").arg(qerror));
                    statusLabel->setStyleSheet("color: red; font-size: 10px;");
                }

                // 下载失败时，显示重试按钮
                if (dlg && dlg->m_fileDownloadInfoMap.contains(normalizedKey)) {
                    FileDownloadInfo& info = dlg->m_fileDownloadInfoMap[normalizedKey];
                    // 查找重试按钮（在状态标签的父布局中）
                    if (statusLabel && statusLabel->parent()) {
                        QWidget* parentWidget = qobject_cast<QWidget*>(statusLabel->parent());
                        if (parentWidget) {
                            QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(parentWidget->layout());
                            if (layout) {
                                // 查找重试按钮
                                for (int i = 0; i < layout->count(); i++) {
                                    QPushButton* btn = qobject_cast<QPushButton*>(layout->itemAt(i)->widget());
                                    if (btn && btn->text() == "重试") {
                                        btn->show();
                                        // 使用QTimer::singleShot延迟连接，避免在回调lambda中直接调用QObject::connect
                                        QString retryKey = normalizedKey;
                                        QTimer::singleShot(0, dlg, [dlg, btn, retryKey]() {
                                            dlg->connectRetryButton(btn, retryKey);
                                        });
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                // 从下载进度映射中移除（但保留下载信息映射，以便重试）
                m_fileDownloadProgressMap.remove(normalizedKey);

                reply->deleteLater();
                manager->deleteLater();
            }
        });
    }
    
    // 使用腾讯SDK下载文件（如果只有fileId没有URL）
    void downloadFileFromSDK(const QJsonObject& fileElem, const QString& savePath, const QString& senderName,
                             const QString& fileName, int fileSize, bool isMine,
                             QProgressBar* progressBar = nullptr, QLabel* statusLabel = nullptr)
    {
        qDebug() << "使用腾讯SDK下载文件，fileId:" << fileElem[kTIMFileElemFileId].toString();
        
        // 更新状态为"正在下载"
        if (statusLabel) {
            statusLabel->setText("正在下载（SDK）...");
            statusLabel->setStyleSheet("color: blue; font-size: 10px;");
        }
        if (progressBar) {
            progressBar->setValue(0);
        }
        
        // 保存下载进度映射
        QString normalizedKey = QDir::toNativeSeparators(savePath);
        m_fileDownloadProgressMap[normalizedKey] = qMakePair(progressBar, statusLabel);
        
        // 构造文件元素JSON（用于下载）
        QJsonDocument elemDoc(fileElem);
        QByteArray elemJsonData = elemDoc.toJson(QJsonDocument::Compact);
        QByteArray pathBytes = savePath.toUtf8();
        
        // 创建回调数据结构
        struct DownloadFileCallbackData {
            ChatDialog* dlg;
            QString savePath;
            QString senderName;
            QString fileName;
            QProgressBar* progressBar;
            QLabel* statusLabel;
        };
        DownloadFileCallbackData* callbackData = new DownloadFileCallbackData;
        callbackData->dlg = this;
        callbackData->savePath = savePath;
        callbackData->senderName = senderName;
        callbackData->fileName = fileName;
        callbackData->progressBar = progressBar;
        callbackData->statusLabel = statusLabel;
        
        // 调用腾讯SDK下载接口
        int ret = TIMMsgDownloadElemToPath(elemJsonData.constData(), pathBytes.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                DownloadFileCallbackData* data = (DownloadFileCallbackData*)user_data;
                ChatDialog* dlg = data->dlg;
                
                QString normalizedKey = QDir::toNativeSeparators(data->savePath);
                QProgressBar* progressBar = nullptr;
                QLabel* statusLabel = nullptr;
                
                if (dlg->m_fileDownloadProgressMap.contains(normalizedKey)) {
                    progressBar = dlg->m_fileDownloadProgressMap[normalizedKey].first;
                    statusLabel = dlg->m_fileDownloadProgressMap[normalizedKey].second;
                }
                
                if (code == TIM_SUCC) {
                    // 下载成功，检查文件是否存在
                    QFile file(data->savePath);
                    if (file.exists()) {
                        qDebug() << "SDK文件下载成功:" << data->savePath;
                        
                        // 更新状态为成功
                        if (statusLabel && statusLabel->parent()) {
                            statusLabel->setText("下载成功");
                            statusLabel->setStyleSheet("color: green; font-size: 10px;");
                        }
                        if (progressBar && progressBar->parent()) {
                            progressBar->setValue(100);
                            // 3秒后隐藏进度条
                            QTimer::singleShot(3000, progressBar, [progressBar, statusLabel]() {
                                if (progressBar && progressBar->parent()) {
                                    progressBar->hide();
                                }
                                if (statusLabel && statusLabel->parent()) {
                                    statusLabel->hide();
                                }
                            });
                        }
                    } else {
                        qDebug() << "SDK下载成功但文件不存在:" << data->savePath;
                        if (statusLabel && statusLabel->parent()) {
                            statusLabel->setText("文件不存在");
                            statusLabel->setStyleSheet("color: red; font-size: 10px;");
                        }
                    }
                } else {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    qDebug() << "SDK文件下载失败，错误码:" << code << "，描述:" << errorDesc;
                    if (statusLabel && statusLabel->parent()) {
                        statusLabel->setText(QString("下载失败: %1").arg(errorDesc));
                        statusLabel->setStyleSheet("color: red; font-size: 10px;");
                    }
                    
                    // SDK下载失败时，也显示重试按钮
                    if (dlg && dlg->m_fileDownloadInfoMap.contains(normalizedKey)) {
                        FileDownloadInfo& info = dlg->m_fileDownloadInfoMap[normalizedKey];
                        if (statusLabel && statusLabel->parent()) {
                            QWidget* parentWidget = qobject_cast<QWidget*>(statusLabel->parent());
                            if (parentWidget) {
                                QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(parentWidget->layout());
                                if (layout) {
                                    for (int i = 0; i < layout->count(); i++) {
                                        QPushButton* btn = qobject_cast<QPushButton*>(layout->itemAt(i)->widget());
                                        if (btn && btn->text() == "重试") {
                                            btn->show();
                                            // 使用QTimer::singleShot延迟连接，避免在回调lambda中直接调用QObject::connect
                                            QString retryKey = normalizedKey;
                                            QTimer::singleShot(0, dlg, [dlg, btn, retryKey]() {
                                                dlg->connectRetryButton(btn, retryKey);
                                            });
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                // 从下载进度映射中移除（但保留下载信息映射，以便重试）
                dlg->m_fileDownloadProgressMap.remove(normalizedKey);
                
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            qDebug() << "调用TIMMsgDownloadElemToPath失败，错误码:" << ret;
            if (statusLabel && statusLabel->parent()) {
                statusLabel->setText("下载初始化失败");
                statusLabel->setStyleSheet("color: red; font-size: 10px;");
            }
            m_fileDownloadProgressMap.remove(normalizedKey);
            delete callbackData;
        }
    }
    
    // 从URL下载图片（使用HTTP/HTTPS）
    void downloadImageFromUrl(const QString& imageUrl, const QString& savePath, const QString& senderName, bool isMine)
    {
        QUrl url(imageUrl);
        if (!url.isValid()) {
            qDebug() << "图片URL无效:" << imageUrl;
            addTextMessage(":/res/img/home.png", senderName, "[图片]（URL无效）", isMine);
            return;
        }
        
        // 检查SSL支持
        if (url.scheme().toLower() == "https") {
            if (!QSslSocket::supportsSsl()) {
                qDebug() << "系统不支持SSL，尝试将HTTPS改为HTTP或使用其他下载方式";
                // 尝试将HTTPS改为HTTP（可能不工作，但试试）
                QString httpUrl = imageUrl;
                httpUrl.replace("https://", "http://");
                if (httpUrl != imageUrl) {
                    qDebug() << "尝试使用HTTP替代HTTPS:" << httpUrl;
                    url = QUrl(httpUrl);
                } else {
                    addTextMessage(":/res/img/home.png", senderName, "[图片]（系统不支持SSL）", isMine);
                    return;
                }
            }
        }
        
        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QNetworkRequest networkRequest;
        networkRequest.setUrl(url);
        
        // 如果是HTTPS，设置SSL配置以忽略证书错误
        if (url.scheme().toLower() == "https") {
            QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
            sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
            sslConfig.setProtocol(QSsl::TlsV1_2OrLater); // 使用TLS 1.2或更高版本
            networkRequest.setSslConfiguration(sslConfig);
        }
        
        QNetworkReply* reply = manager->get(networkRequest);
        
        // 忽略SSL错误（开发环境常用做法，如果图片URL是HTTPS）
        connect(reply, &QNetworkReply::sslErrors, this, [=](const QList<QSslError>& errors) {
            qDebug() << "SSL错误，忽略证书验证，URL:" << imageUrl << "，错误数:" << errors.size();
            for (const QSslError& error : errors) {
                qDebug() << "SSL错误详情:" << error.errorString();
            }
            reply->ignoreSslErrors();
        });
        
        connect(reply, &QNetworkReply::finished, this, [=]() {
            int error = reply->error();
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray imageData = reply->readAll();
                if (imageData.isEmpty()) {
                    qDebug() << "图片数据为空";
                    addTextMessage(":/res/img/home.png", senderName, "[图片]（数据为空）", isMine);
                } else {
                    QFile file(savePath);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(imageData);
                        file.close();
                        
                        // 下载成功，显示图片
                        addImageMessage(":/res/img/home.png", senderName, savePath, isMine);
                        qDebug() << "图片下载成功:" << savePath << "，大小:" << imageData.size() << "字节";
                    } else {
                        qDebug() << "保存图片失败:" << savePath;
                        addTextMessage(":/res/img/home.png", senderName, "[图片]（保存失败）", isMine);
                    }
                }
            } else {
                QString qerror = reply->errorString();
                qDebug() << "图片下载失败，URL:" << imageUrl;
                qDebug() << "错误码:" << error << "，错误描述:" << qerror;
                addTextMessage(":/res/img/home.png", senderName, QString("[图片]（下载失败: %1）").arg(qerror), isMine);
            }
            
            reply->deleteLater();
            manager->deleteLater();
        });
    }
    
    // 使用SDK接口下载图片（优先使用，避免TLS问题）
    void downloadImageFromSDK(const QJsonObject& imageElem, const QString& savePath, const QString& senderName, bool isMine)
    {
        // 获取图片ID和URL
        QString imageId = imageElem[kTIMImageElemLargeId].toString();
        QString imageUrl = imageElem[kTIMImageElemLargeUrl].toString();
        
        if (imageId.isEmpty()) {
            imageId = imageElem[kTIMImageElemThumbId].toString();
            imageUrl = imageElem[kTIMImageElemThumbUrl].toString();
        }
        if (imageId.isEmpty()) {
            imageId = imageElem[kTIMImageElemOrigId].toString();
            imageUrl = imageElem[kTIMImageElemOrigUrl].toString();
        }
        
        // 优先尝试使用SDK的下载接口（推荐方式，避免TLS问题）
        // 如果URL和ID都不为空，使用SDK下载
        if (!imageUrl.isEmpty() && !imageId.isEmpty()) {
            // 构造SDK下载参数
            QJsonObject downloadParam;
            downloadParam[kTIMMsgDownloadElemParamUrl] = imageUrl;
            downloadParam[kTIMMsgDownloadElemParamId] = imageId;
            downloadParam[kTIMMsgDownloadElemParamType] = (int)kTIMDownload_File; // 使用File类型下载图片
            downloadParam[kTIMMsgDownloadElemParamFlag] = 0; // 默认值
            downloadParam[kTIMMsgDownloadElemParamBusinessId] = 0; // 默认值
            
            QJsonDocument doc(downloadParam);
            QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
            
            // 输出调试信息
            qDebug() << "=== SDK图片下载参数 ===";
            qDebug() << "图片URL:" << imageUrl;
            qDebug() << "图片ID:" << imageId;
            qDebug() << "下载参数JSON:" << QString::fromUtf8(jsonData);
            
            // 将保存路径转换为本地路径格式
            QString normalizedPath = QDir::toNativeSeparators(savePath);
            qDebug() << "保存路径:" << normalizedPath;
            
            // 创建超时定时器（在回调数据结构中保存，以便在回调中停止）
            QTimer* timeoutTimer = new QTimer(this);
            timeoutTimer->setSingleShot(true);
            
            // 创建回调数据结构
            struct DownloadImageCallbackData {
                ChatDialog* dlg;
                QString savePath;
                QString senderName;
                bool isMine;
                QString imageUrl; // 备用：如果SDK下载失败，使用HTTP下载
                QTimer* timeoutTimer; // 超时定时器，回调被调用时停止
            };
            DownloadImageCallbackData* callbackData = new DownloadImageCallbackData;
            callbackData->dlg = this;
            callbackData->savePath = normalizedPath;
            callbackData->senderName = senderName;
            callbackData->isMine = isMine;
            callbackData->imageUrl = imageUrl;
            callbackData->timeoutTimer = timeoutTimer;
            
            // 将路径转换为UTF-8编码的字节数组（Windows路径可能需要）
            QByteArray pathBytes = normalizedPath.toUtf8();
            
            qDebug() << "准备调用TIMMsgDownloadElemToPath...";
            
            // 调用SDK下载接口
            int ret = TIMMsgDownloadElemToPath(jsonData.constData(), pathBytes.constData(),
                [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                    qDebug() << "=== SDK下载回调函数被调用 ===";
                    qDebug() << "回调错误码:" << code;
                    qDebug() << "回调错误描述:" << (desc ? QString::fromUtf8(desc) : "无");
                    qDebug() << "回调JSON参数:" << (json_params ? QString::fromUtf8(json_params) : "无");
                    
                    DownloadImageCallbackData* data = (DownloadImageCallbackData*)user_data;
                    if (!data) {
                        qDebug() << "回调数据为空，退出";
                        return;
                    }
                    ChatDialog* dlg = data->dlg;
                    
                    // 停止超时定时器（如果回调被调用）
                    if (data->timeoutTimer) {
                        data->timeoutTimer->stop();
                        data->timeoutTimer->deleteLater();
                        data->timeoutTimer = nullptr;
                    }
                    
                    if (code == TIM_SUCC) {
                        // 下载成功，检查文件是否存在
                        QFile file(data->savePath);
                        if (file.exists()) {
                            // 显示图片
                            dlg->addImageMessage(":/res/img/home.png", data->senderName, data->savePath, data->isMine);
                            qDebug() << "SDK图片下载成功:" << data->savePath;
                        } else {
                            qDebug() << "SDK下载成功但文件不存在:" << data->savePath;
                            // 如果SDK下载失败，尝试HTTP下载
                            if (!data->imageUrl.isEmpty()) {
                                qDebug() << "尝试使用HTTP下载作为备用方案";
                                dlg->downloadImageFromUrl(data->imageUrl, data->savePath, data->senderName, data->isMine);
                            } else {
                                dlg->addTextMessage(":/res/img/home.png", data->senderName, "[图片]（下载失败）", data->isMine);
                            }
                        }
                    } else {
                        QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                        qDebug() << "SDK图片下载失败，错误码:" << code << "，描述:" << errorDesc;
                        // SDK下载失败，尝试HTTP下载作为备用方案
                        if (!data->imageUrl.isEmpty()) {
                            qDebug() << "SDK下载失败，尝试使用HTTP下载作为备用方案";
                            dlg->downloadImageFromUrl(data->imageUrl, data->savePath, data->senderName, data->isMine);
                        } else {
                            dlg->addTextMessage(":/res/img/home.png", data->senderName, QString("[图片]（下载失败: %1）").arg(errorDesc), data->isMine);
                        }
                    }
                    
                    delete data;
                }, callbackData);
            
            qDebug() << "TIMMsgDownloadElemToPath调用返回，错误码:" << ret << "（TIM_SUCC=" << TIM_SUCC << "）";
            
            if (ret != TIM_SUCC) {
                QString errorDesc = QString("错误码: %1").arg(ret);
                if (ret == -3) {
                    errorDesc = "JSON格式错误或字段错误";
                } else if (ret == -1) {
                    errorDesc = "参数错误";
                }
                qDebug() << "调用TIMMsgDownloadElemToPath失败，错误码:" << ret << "，错误描述:" << errorDesc;
                qDebug() << "注意：只有当返回TIM_SUCC(0)时，回调函数才会被调用";
                qDebug() << "JSON参数:" << QString::fromUtf8(jsonData);
                qDebug() << "路径参数:" << normalizedPath;
                // SDK接口调用失败，尝试HTTP下载
                if (!imageUrl.isEmpty()) {
                    qDebug() << "SDK接口调用失败，尝试使用HTTP下载作为备用方案";
                    downloadImageFromUrl(imageUrl, savePath, senderName, isMine);
                } else {
                    addTextMessage(":/res/img/home.png", senderName, QString("[图片]（无法下载: %1）").arg(errorDesc), isMine);
                }
                delete callbackData;
            } else {
                qDebug() << "TIMMsgDownloadElemToPath调用成功（返回TIM_SUCC），但回调可能不可靠，添加超时检查...";
                qDebug() << "回调函数将在下载完成或失败时被调用";
                
                // 添加超时检查：如果3秒后回调没有被调用，使用HTTP下载作为备用
                connect(timeoutTimer, &QTimer::timeout, this, [=]() {
                    // 检查文件是否已经下载成功
                    QFile file(normalizedPath);
                    if (!file.exists()) {
                        qDebug() << "SDK下载超时（3秒），文件不存在，使用HTTP下载作为备用方案";
                        if (!imageUrl.isEmpty()) {
                            downloadImageFromUrl(imageUrl, savePath, senderName, isMine);
                        } else {
                            addTextMessage(":/res/img/home.png", senderName, "[图片]（下载超时）", isMine);
                        }
                    } else {
                        qDebug() << "SDK下载超时，但文件已存在，显示图片";
                        addImageMessage(":/res/img/home.png", senderName, normalizedPath, isMine);
                    }
                    timeoutTimer->deleteLater();
                });
                timeoutTimer->start(3000); // 3秒超时
                
                // 注意：回调函数会在下载完成或失败时被调用，如果回调被调用，应该停止定时器
                // 但由于回调可能不会被调用，我们依赖超时机制
            }
        } else if (!imageUrl.isEmpty()) {
            // 只有URL，没有ID，直接使用HTTP下载
            qDebug() << "图片只有URL，使用HTTP下载";
            downloadImageFromUrl(imageUrl, savePath, senderName, isMine);
        } else {
            qDebug() << "图片URL和ID都为空，无法下载";
            addTextMessage(":/res/img/home.png", senderName, "[图片]（无法获取）", isMine);
        }
    }

    // 更新文件上传进度
    void updateFileUploadProgress(uint32_t cur_size, uint32_t total_size)
    {
        if (total_size == 0) return;
        
        // 更新所有正在上传的文件进度
        for (auto it = m_fileUploadProgressMap.begin(); it != m_fileUploadProgressMap.end(); ++it) {
            QProgressBar* progressBar = it.value().first;
            QLabel* statusLabel = it.value().second;
            
            if (progressBar) {
                int progress = (int)((double)cur_size / total_size * 100);
                progressBar->setValue(progress);
                
                // 更新状态标签显示进度
                if (statusLabel) {
                    QString sizeStr = cur_size < 1024 ? QString("%1 字节").arg(cur_size) :
                                     cur_size < 1024 * 1024 ? QString("%1 KB").arg(cur_size / 1024.0, 0, 'f', 1) :
                                     QString("%1 MB").arg(cur_size / (1024.0 * 1024.0), 0, 'f', 1);
                    QString totalStr = total_size < 1024 ? QString("%1 字节").arg(total_size) :
                                      total_size < 1024 * 1024 ? QString("%1 KB").arg(total_size / 1024.0, 0, 'f', 1) :
                                      QString("%1 MB").arg(total_size / (1024.0 * 1024.0), 0, 'f', 1);
                    statusLabel->setText(QString("上传中: %1 / %2 (%3%)").arg(sizeStr).arg(totalStr).arg(progress));
                    statusLabel->setStyleSheet("color: blue; font-size: 10px;");
                }
            }
        }
    }

    TaQTWebSocket* m_pWs = NULL;
    QString m_unique_group_id;
    bool m_iGroupOwner = false;

    // ChatDialog 同时服务于班级群/普通群：用于控制“...”群管理入口的模式
    QString m_classId;
    bool m_isClassGroup = false;
    // 文件上传进度映射：文件路径 -> (进度条, 状态标签)
    QMap<QString, QPair<QProgressBar*, QLabel*>> m_fileUploadProgressMap;
    // 文件下载进度映射：文件路径 -> (进度条, 状态标签)
    QMap<QString, QPair<QProgressBar*, QLabel*>> m_fileDownloadProgressMap;
    // 语音上传进度映射：语音文件路径 -> (进度条, 状态标签)
    QMap<QString, QPair<QProgressBar*, QLabel*>> m_voiceUploadProgressMap;
    // 语音下载进度映射：语音文件路径 -> (进度条, 状态标签)
    QMap<QString, QPair<QProgressBar*, QLabel*>> m_voiceDownloadProgressMap;
    // 语音播放进度映射：语音文件路径 -> (播放进度条, 播放状态标签)
    QMap<QString, QPair<QProgressBar*, QLabel*>> m_voicePlayProgressMap;
    // 语音时长映射：语音文件路径 -> 时长（秒）
    QMap<QString, int> m_voiceDurationMap;
    
    // 文件下载信息结构（用于重试）
    struct FileDownloadInfo {
        QString fileUrl;           // 文件URL
        QJsonObject fileElem;      // 文件元素JSON（用于SDK下载）
        QString savePath;          // 保存路径
        QString senderName;        // 发送者名称
        QString fileName;          // 文件名
        int fileSize;              // 文件大小
        bool isMine;               // 是否是自己发送的
        QProgressBar* progressBar; // 进度条指针
        QLabel* statusLabel;       // 状态标签指针
    };
    // 文件下载信息映射：文件路径 -> 下载信息（用于重试）
    QMap<QString, FileDownloadInfo> m_fileDownloadInfoMap;
};
