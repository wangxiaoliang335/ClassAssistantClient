#pragma execution_character_set("utf-8")
#include <QScreen>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslSocket>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QDate>
#include <QApplication>
#include <QTimer>
#include <windows.h>
#include <tchar.h>
#include "TACMainDialog.h"
#include "ChatDialog.h"
#include "CommonInfo.h"
#include "common/Base.h"
#include "GroupNotifyDialog.h"
#include "FriendNotifyDialog.h"

TaQTWebSocket* TACMainDialog::m_ws = NULL;

TACMainDialog::TACMainDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	this->setWindowState(Qt::WindowFullScreen);
	this->setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
	m_networkManager = new QNetworkAccessManager(this);
}

TACMainDialog::~TACMainDialog()
{

}

//void TACMainDialog::InitWebSocket()
//{
//    socket = new QWebSocket();
//    connect(socket, &QWebSocket::connected, this, &TACMainDialog::onConnected);
//    connect(socket, &QWebSocket::textMessageReceived, this, &TACMainDialog::onMessageReceived);
//    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
//        this, &TACMainDialog::onError);
//
//    // 建立 WebSocket 连接
//    QString url = QString("ws://47.100.126.194:5000/ws/%1").arg(m_userInfo.strIdNumber);
//    socket->open(QUrl(url));
//
//    // 定时发送心跳
//    heartbeatTimer = new QTimer(this);
//    connect(heartbeatTimer, &QTimer::timeout, this, &TACMainDialog::sendHeartbeat);
//    heartbeatTimer->start(5000); // 每 5 秒一次
//}

void TACMainDialog::Init(QString classId, int user_id)
{
    navBarWidget = new TACNavigationBarWidget(this);
    navBarWidget->visibleCloseButton(false);

    if (InitSDK())
    {
        Login(classId.toStdString());
    }

    m_ws = new TaQTWebSocket(this);
    
    // 连接家庭作业消息信号，缓存作业数据
    connect(m_ws, &TaQTWebSocket::homeworkReceived, this, [=](const QJsonObject& homeworkData) {
        // 响应消息：包含 status 字段
        if (homeworkData.contains("status")) {
            const QString status = homeworkData.value("status").toString();
            const QString message = homeworkData.value("message").toString();
            qDebug() << "作业发布响应:" << status << message;
            return;
        }

        const QString dateStr = homeworkData.value("date").toString();
        const QString subject = homeworkData.value("subject").toString().trimmed();
        const QString content = homeworkData.value("content").toString().trimmed();
        const QString createdAt = homeworkData.value("created_at").toString(); // 获取创建时间
        if (dateStr.isEmpty() || subject.isEmpty() || content.isEmpty()) {
            return;
        }

        // 创建作业项并缓存：按日期聚合，支持同一天同一科目的多条作业
        HomeworkItem item(subject, content, createdAt);
        m_homeworkByDate[dateStr].append(item);
        qDebug() << "TACMainDialog: 收到作业消息，已缓存到日期:" << dateStr << "科目:" << subject << "创建时间:" << createdAt;
        
        // 如果 homeworkViewDialog 已创建，更新其内容
        if (homeworkViewDialog) {
            QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
            if (!date.isValid()) {
                date = QDate::currentDate();
            }
            homeworkViewDialog->setDate(date);
            homeworkViewDialog->setHomeworkList(m_homeworkByDate.value(dateStr));
        }
    });
    
    m_httpHandler = new TAHttpHandler(this);
    if (m_httpHandler)
    {
        connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
            //成功消息就不发送了
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
            if (jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                if (obj["data"].isObject())
                {
                    QJsonObject oTmp = obj["data"].toObject();
                    QString strTmp = oTmp["message"].toString();
                    qDebug() << "status:" << oTmp["code"].toString();
                    qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
                    //errLabel->setText(strTmp);
                    //user_id = oTmp["user_id"].toInt();
                    if (strTmp == "登录成功")
                    {
                        accept(); // 验证通过，关闭对话框并返回 Accepted
                    }
                    else if (strTmp == "获取用户信息成功")
                    {
                        if (oTmp["userinfo"].isArray())
                        {
                            QJsonArray oUserInfo = oTmp["userinfo"].toArray();
                            if (oUserInfo.size() > 0)
                            {
                                m_userInfo.strUserId = std::to_string(user_id).c_str();
                                m_userInfo.strPhone = oUserInfo.at(0)["phone"].toString();
                                m_userInfo.strName = oUserInfo.at(0)["name"].toString();
                                m_userInfo.strSex = oUserInfo.at(0)["sex"].toString();
                                m_userInfo.strAddress = oUserInfo.at(0)["address"].toString();
                                m_userInfo.strSchoolName = oUserInfo.at(0)["school_name"].toString();
                                m_userInfo.strGradeLevel = oUserInfo.at(0)["grade_level"].toString();
                                // 任教信息：优先读 teachings 数组（新模型）；旧字段不再作为任教信息来源
                                if (oUserInfo.at(0).isObject() && oUserInfo.at(0).toObject().contains("teachings") && oUserInfo.at(0).toObject().value("teachings").isArray()) {
                                    QJsonArray teachingsArr = oUserInfo.at(0).toObject().value("teachings").toArray();
                                    m_userInfo.teachings.clear();
                                    for (const QJsonValue& v : teachingsArr) {
                                        if (!v.isObject()) continue;
                                        QJsonObject t = v.toObject();
                                        UserTeachingInfo info;
                                        info.grade_level = t.value("grade_level").toString();
                                        info.grade = t.value("grade").toString();
                                        info.subject = t.value("subject").toString();
                                        info.class_taught = t.value("class_taught").toString();
                                        m_userInfo.teachings.append(info);
                                    }
                                } else {
                                    m_userInfo.teachings.clear();
                                }
                                m_userInfo.strIsAdministrator = oUserInfo.at(0)["is_administrator"].toString();
                                m_userInfo.avatar = oUserInfo.at(0)["avatar"].toString();
                                m_userInfo.strIdNumber = oUserInfo.at(0)["id_number"].toString();
                                m_userInfo.schoolId = oUserInfo.at(0)["schoolId"].toString();
                                //int iteacher_unique_id = oUserInfo.at(0)["teacher_unique_id"].toString();
                                //m_userInfo.teacher_unique_id = QString("%1").arg(iteacher_unique_id, 6, 10, QChar('0'));

                                m_userInfo.classId = oUserInfo.at(0)["classId"].toString();
                                QString avatarBase64 = oUserInfo.at(0)["avatar_base64"].toString();

                                // 更新 CommonInfo 中的用户信息
                                CommonInfo::InitData(m_userInfo);
                                
                                // 初始化学校课程表数据（schoolId 已准备好）
                                if (desktopManagerWidget) {
                                    desktopManagerWidget->initSchoolCourseScheduleData();
                                }
                                
                                // 更新托盘组件中的管理员按钮状态（如果已创建）
                                if (trayWidget) {
                                    trayWidget->updateAdminButtonState();
                                }

                                // 确定保存路径
                                QString fileName;
                                QString saveDir = QCoreApplication::applicationDirPath() + "/avatars/" + m_userInfo.strIdNumber;
                                QDir().mkpath(saveDir);
                                
                                QString filePath;
                                
                                // 如果 avatarBase64 不为空，使用 Base64 数据
                                if (!avatarBase64.isEmpty()) {
                                    // 没有文件名就用手机号或ID代替
                                    if (m_userInfo.avatar.isEmpty())
                                        m_userInfo.avatar = oUserInfo.at(0)["id_number"].toString() + "_" + ".png";

                                    // 从最后一个 "/" 之后开始截取
                                    fileName = m_userInfo.avatar.section('/', -1);
                                    filePath = saveDir + "/" + fileName;
                                    m_userInfo.strHeadImagePath = filePath;

                                    // Base64 解码成图片二进制数据
                                    QByteArray imageData = QByteArray::fromBase64(avatarBase64.toUtf8());

                                    // 写入文件（覆盖旧的）
                                    QFile file(filePath);
                                    if (!file.open(QIODevice::WriteOnly)) {
                                        qWarning() << "Cannot open file for writing:" << filePath;
                                    } else {
                                        file.write(imageData);
                                        file.close();
                                    }
                                }
                                // 如果 avatarBase64 为空，但 avatar 是 URL，则从 URL 下载
                                else if (!m_userInfo.avatar.isEmpty() && 
                                         (m_userInfo.avatar.startsWith("http://") || m_userInfo.avatar.startsWith("https://"))) {
                                    // 从 URL 中提取文件名
                                    QUrl url(m_userInfo.avatar);
                                    fileName = url.fileName();
                                    
                                    // 如果无法从URL提取文件名，或文件名没有扩展名，使用时间戳生成文件名
                                    if (fileName.isEmpty() || !fileName.contains('.')) {
                                        // 尝试从路径中提取最后一部分
                                        QString pathPart = m_userInfo.avatar.section('/', -1);
                                        if (!pathPart.isEmpty() && pathPart.contains('.')) {
                                            fileName = pathPart;
                                        } else {
                                            // 使用ID和时间戳生成文件名
                                            fileName = m_userInfo.strIdNumber + "_" + QString::number(QDateTime::currentSecsSinceEpoch()) + ".png";
                                        }
                                    }
                                    
                                    filePath = saveDir + "/" + fileName;
                                    m_userInfo.strHeadImagePath = filePath;
                                    
                                    qDebug() << "Downloading avatar from URL:" << m_userInfo.avatar << "to:" << filePath;
                                    
                                    // 从 URL 下载头像
                                    downloadAvatarFromUrl(m_userInfo.avatar, filePath);
                                }
                                // 如果都没有，使用默认文件名
                                else {
                                    fileName = m_userInfo.strIdNumber + "_" + ".png";
                                    filePath = saveDir + "/" + fileName;
                                    m_userInfo.strHeadImagePath = filePath;
                                    qWarning() << "No avatar data (neither base64 nor URL) for" << filePath;
                                }

                                /*if (userMenuDlg)
                                {
                                    userMenuDlg->InitData(m_userInfo);
                                    userMenuDlg->InitUI();
                                    CommonInfo::InitData(m_userInfo);

                                    if (m_ws)
                                    {
                                        TaQTWebSocket::InitWebSocket(m_ws);
                                    }
                                    
                                    if (friendGrpDlg)
                                    {
                                        friendGrpDlg->InitWebSocket();
                                        friendGrpDlg->InitData();
                                    }
                                }

                                if (schoolInfoDlg)
                                {
                                    schoolInfoDlg->InitData(m_userInfo);
                                }*/

                            }
                        }
                    }
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

    connect(navBarWidget, &TACNavigationBarWidget::navType, this, [=](TACNavigationBarWidgetType type,bool checked) {
        if (type == TACNavigationBarWidgetType::TIMER)
        {
            if (datetimeDialog)
            {
                datetimeDialog->show();
            }
            if (datetimeWidget)
                datetimeWidget->show();
        }
        else if (type == TACNavigationBarWidgetType::FOLDER)
        {
            if (desktopManagerWidget)
            {
                QRect rect = navBarWidget->geometry();
                desktopManagerWidget->move(rect.x(), rect.y() - desktopManagerWidget->height() - 10);
                if (checked)
                    desktopManagerWidget->show();
                else
                    desktopManagerWidget->hide();
            }
        }
        else if (type == TACNavigationBarWidgetType::CLASS_GROUP)
        {
            // 检查"关联今日课表"是否开启
            bool linkTodayScheduleEnabled = false;
            if (friendGrpDlg) {
                ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
                if (!loginInfo.class_id.isEmpty()) {
                    // 确保ScheduleDialog已创建（如果不存在则先创建，但不显示）
                    friendGrpDlg->ensureScheduleDialogCreated(loginInfo.class_id);
                    ScheduleDialog* scheduleDlg = friendGrpDlg->getScheduleDialog(loginInfo.class_id);
                    if (scheduleDlg) {
                        QGroupInfo* groupInfo = scheduleDlg->getGroupInfo();
                        if (groupInfo) {
                            linkTodayScheduleEnabled = groupInfo->isLinkTodayScheduleEnabled();
                        }
                    }
                }
            }
            
            //// 更新"课前准备"功能键的可见性：如果"关联今日课表"为true，则显示；否则隐藏
            //if (navBarWidget) {
            //    navBarWidget->setPrepareClassButtonVisible(linkTodayScheduleEnabled);
            //}
            
            // 如果"关联今日课表"未开启，直接打开周课表（旧行为）
            if (!linkTodayScheduleEnabled) {
                if (!classWeekCourseScheduldDialog) {
                    classWeekCourseScheduldDialog = new TACClassWeekCourseScheduleDialog(this);
                }
                
                ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
                if (!loginInfo.class_id.isEmpty()) {
                    classWeekCourseScheduldDialog->fetchCourseScheduleFromServer(loginInfo.class_id);
                }
                
                QScreen* screen = QApplication::primaryScreen();
                QRect screenGeometry = screen->geometry();
                int x = (screenGeometry.width() - classWeekCourseScheduldDialog->width()) / 2;
                int y = (screenGeometry.height() - classWeekCourseScheduldDialog->height()) / 2;
                classWeekCourseScheduldDialog->move(x, y);
                
                classWeekCourseScheduldDialog->show();
                classWeekCourseScheduldDialog->raise();
                classWeekCourseScheduldDialog->activateWindow();
                return;
            }
            
            // "关联今日课表"已开启，显示课程表菜单（包含"课程表"和"今日课表"选项）
            if (!courseScheduleMenuDialog) {
                courseScheduleMenuDialog = new TACCourseScheduleMenuDialog(this);
                
                // 连接周课表信号
                connect(courseScheduleMenuDialog, &TACCourseScheduleMenuDialog::weekScheduleSelected, this, [this]() {
                    // 显示周课表
                    if (!classWeekCourseScheduldDialog) {
                        classWeekCourseScheduldDialog = new TACClassWeekCourseScheduleDialog(this);
                    }
                    
                    ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
                    if (!loginInfo.class_id.isEmpty()) {
                        classWeekCourseScheduldDialog->fetchCourseScheduleFromServer(loginInfo.class_id);
                    }
                    
                    QScreen* screen = QApplication::primaryScreen();
                    QRect screenGeometry = screen->geometry();
                    int x = (screenGeometry.width() - classWeekCourseScheduldDialog->width()) / 2;
                    int y = (screenGeometry.height() - classWeekCourseScheduldDialog->height()) / 2;
                    classWeekCourseScheduldDialog->move(x, y);
                    
                    classWeekCourseScheduldDialog->show();
                    classWeekCourseScheduldDialog->raise();
                    classWeekCourseScheduldDialog->activateWindow();
                });
                
                // 连接今日课表信号
                connect(courseScheduleMenuDialog, &TACCourseScheduleMenuDialog::todayScheduleSelected, this, [this]() {
                    // 显示今日课表
                    if (!courseSchedule) {
                        courseSchedule = new TACCourseSchedule(this);
                    }
                    
                    ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
                    if (!loginInfo.class_id.isEmpty()) {
                        courseSchedule->fetchTodaySchedule(loginInfo.class_id);
                    }
                    
                    courseSchedule->show();
                    courseSchedule->raise();
                    courseSchedule->activateWindow();
                });
            }
            
            // 定位菜单到CLASS_GROUP按钮的下方
            // 导航栏按钮顺序：folder(0), homework(1), class(2), schedule(3), user(4)
            // 按钮大小：60x60，间距：20
            QPoint navBarPos = navBarWidget->mapToGlobal(QPoint(0, 0));
            QRect navBarRect = navBarWidget->geometry();
            
            // CLASS_GROUP按钮是第3个按钮（索引2）
            // 计算按钮位置：folder(0) + homework(1) + class(2)
            int buttonIndex = 2; // CLASS_GROUP按钮的索引
            int buttonWidth = 60;
            int buttonSpacing = 20;
            int buttonX = buttonIndex * (buttonWidth + buttonSpacing);
            
            // 菜单显示在按钮上方，居中对齐
            int menuX = navBarPos.x() + buttonX + (buttonWidth - courseScheduleMenuDialog->width()) / 2;
            int menuY = navBarPos.y() - courseScheduleMenuDialog->height() - 3; // 按钮上方3像素
            courseScheduleMenuDialog->move(menuX, menuY);
            
            // 切换显示/隐藏状态
            if (courseScheduleMenuDialog->isVisible()) {
                courseScheduleMenuDialog->hide();
            } else {
                courseScheduleMenuDialog->show();
                courseScheduleMenuDialog->raise();
                courseScheduleMenuDialog->activateWindow();
            }
        }
        else if (type == TACNavigationBarWidgetType::USER)
        {
            if (friendGrpDlg)
                friendGrpDlg->show();
        }
        else if (type == TACNavigationBarWidgetType::WALLPAPER)
        {
            if (wallpaperLibraryDialog)
                wallpaperLibraryDialog->show();
        }
        else if (type == TACNavigationBarWidgetType::HOMEWORK)
        {
            // 使用 HomeworkViewDialog 显示作业（与 ScheduleDialog 中的窗口一致）
            if (!homeworkViewDialog) {
                homeworkViewDialog = new HomeworkViewDialog(this);
            }
            
            // 从缓存中读取当前日期的作业数据
            QDate currentDate = QDate::currentDate();
            QString dateStr = currentDate.toString("yyyy-MM-dd");
            QList<HomeworkItem> homeworkList = m_homeworkByDate.value(dateStr);
            
            homeworkViewDialog->setDate(currentDate);
            homeworkViewDialog->setHomeworkList(homeworkList); // 使用新的列表接口
            
            // 获取主屏幕几何信息，居中显示
            QScreen* screen = QApplication::primaryScreen();
            QRect screenGeometry = screen->geometry();
            int x = (screenGeometry.width() - homeworkViewDialog->width()) / 2;
            int y = (screenGeometry.height() - homeworkViewDialog->height()) / 2;
            homeworkViewDialog->move(x, y);
            
            homeworkViewDialog->show();
            homeworkViewDialog->raise();
            homeworkViewDialog->activateWindow();
        }
        else if (type == TACNavigationBarWidgetType::MESSAGE)
        {
            
        }
        else if (type == TACNavigationBarWidgetType::IM)
        {
            if (imDialog)
                imDialog->show();
        }
        else if (type == TACNavigationBarWidgetType::PREPARE_CLASS)
        {
            // 查看下节课的课前准备
            if (friendGrpDlg) {
                ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
                if (!loginInfo.class_id.isEmpty()) {
                    // 确保ScheduleDialog已创建
                    friendGrpDlg->ensureScheduleDialogCreated(loginInfo.class_id);
                    ScheduleDialog* scheduleDlg = friendGrpDlg->getScheduleDialog(loginInfo.class_id);
                    if (scheduleDlg) {
                        // 获取下节课的科目和时间
                        QTime currentTime = QTime::currentTime();
                        QPair<QString, QString> nextClass = scheduleDlg->getNextClassInfo(currentTime);
                        if (!nextClass.first.isEmpty()) {
                            // 显示下节课的课前准备
                            scheduleDlg->showNextClassPrepareDialog(nextClass.first, nextClass.second);
                        } else {
                            // 如果没有下节课，显示提示
                            TACStyledMessageDialog* msgDialog = new TACStyledMessageDialog(
                                QString::fromUtf8(u8"提示"), 
                                QString::fromUtf8(u8"今日暂无下一节课"), 
                                this);
                            // 居中显示
                            QRect parentRect = this->geometry();
                            int x = parentRect.x() + (parentRect.width() - msgDialog->width()) / 2;
                            int y = parentRect.y() + (parentRect.height() - msgDialog->height()) / 2;
                            msgDialog->move(x, y);
                            msgDialog->show();
                            msgDialog->raise();
                            msgDialog->activateWindow();
                        }
                    }
                }
            }
        }
        else if (type == TACNavigationBarWidgetType::CLASS_SCHEDULE)
        {
            // 如果"关联值日表"开启，显示值日表
            // 显示值日表窗口
            if (!dutyRosterDialog) {
                dutyRosterDialog = new DutyRosterDialog(this);
            }
            
            // 获取群组ID和班级ID
            if (friendGrpDlg) {
                ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
                if (!loginInfo.class_id.isEmpty()) {
                    // 群组ID通常是 class_id + "01"
                    QString groupId = loginInfo.class_id + "01";
                    dutyRosterDialog->setGroupId(groupId);
                    dutyRosterDialog->setClassId(loginInfo.class_id);
                }
            }
            
            // 居中显示
            QScreen* screen = QApplication::primaryScreen();
            QRect screenGeometry = screen->geometry();
            int x = (screenGeometry.width() - dutyRosterDialog->width()) / 2;
            int y = (screenGeometry.height() - dutyRosterDialog->height()) / 2;
            dutyRosterDialog->move(x, y);
            
            dutyRosterDialog->show();
            dutyRosterDialog->raise();
            dutyRosterDialog->activateWindow();
        }
        else if (type == TACNavigationBarWidgetType::CALENDAR)
        {
            if (classWeekCourseScheduldDialog)
                classWeekCourseScheduldDialog->show();
        }
        else if (type == TACNavigationBarWidgetType::USER1)
        {
            if (userMenuDlg)
            {
                userMenuDlg->show();
            }
        }
    });
    navBarWidget->show();
    
    //// 登录成功后，延迟更新课前准备按钮状态（等待群组设置加载完成）
    //QTimer::singleShot(2000, this, [this]() {
    //    updatePrepareClassButtonVisibility();
    //});
    //
    //// 登录成功后，延迟更新课前准备按钮状态（等待群组设置加载完成）
    //QTimer::singleShot(2000, this, [this]() {
    //    updatePrepareClassButtonVisibility();
    //});

    prepareClassDialog = new TACPrepareClassDialog(this);
    desktopManagerWidget = new TACDesktopManagerWidget(this);
    schoolInfoDlg = new SchoolInfoDialog(this);
    classInfoDlg = new ClassInfoDialog(this);
    classInfoDlg->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    classInfoDlg->setBorderColor(WIDGET_BORDER_COLOR);
    classInfoDlg->setBorderWidth(WIDGET_BORDER_WIDTH);
    classInfoDlg->setRadius(30);
    trayWidget = new TACTrayWidget(this);

    //trayWidget->show();
    
    // 用户信息已加载后，更新管理员按钮状态
    if (CommonInfo::GetData().strIsAdministrator.isEmpty() == false) {
        trayWidget->updateAdminButtonState();
    }
    
    // 班级信息已加载后，更新班级信息按钮状态
    ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
    if (loginInfo.isLoggedIn()) {
        trayWidget->updateClassInfoButtonState();
    }
    
    // 班级信息
    connect(trayWidget, &TACTrayWidget::navClassInfo, this, [=](bool checked) {
        if (classInfoDlg)
        {
            // 获取主屏幕几何信息（Qt 5.14+ 推荐 QScreen）
            QScreen* screen = QApplication::primaryScreen();
            QRect screenGeometry = screen->geometry();
            int x = (screenGeometry.width() - classInfoDlg->width()) / 2;
            int y = (screenGeometry.height() - classInfoDlg->height()) / 2;
            classInfoDlg->move(x, y);
            if (checked) {
                classInfoDlg->InitData(); // 刷新数据
                classInfoDlg->show();
            } else {
                classInfoDlg->hide();
            }
        }
        });

    // 桌面管理
    connect(trayWidget, &TACTrayWidget::navDesktopManager, this, [=](bool checked) {
        if (desktopManagerWidget)
        {
            QRect rect = trayWidget->geometry();
            desktopManagerWidget->move(rect.x() - desktopManagerWidget->width() - 10, rect.y() - desktopManagerWidget->height() + 180);
            if (checked)
                desktopManagerWidget->show();
            else
                desktopManagerWidget->hide();
        }
     });

    // 班级群
    connect(trayWidget, &TACTrayWidget::navClassGroup, this, [=](bool checked) {
        if (checked && friendGrpDlg)
        {
            // 获取班级ID
            ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
            if (loginInfo.class_id.isEmpty()) {
                qDebug() << "班级ID为空，无法打开班级群";
                return;
            }
            
            // 组合完整的班级群显示名称：年级 + 班级名称 + "的班级群"
            // 例如："一年级六班的班级群"
            QString displayName;
            if (!loginInfo.grade.isEmpty() && !loginInfo.class_name.isEmpty()) {
                displayName = loginInfo.grade + loginInfo.class_name + "的班级群";
            } else if (!loginInfo.class_name.isEmpty()) {
                displayName = loginInfo.class_name + "的班级群";
            } else {
                displayName = "班级群";
            }
            
            // 直接打开对应的班级群窗口
            friendGrpDlg->openClassGroup(loginInfo.class_id, displayName);
        }
        });

    imDialog = new TACIMDialog(this);
    friendGrpDlg = new FriendGroupDialog(this, m_ws);
    friendGrpDlg->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    friendGrpDlg->setBorderColor(WIDGET_BORDER_COLOR);
    friendGrpDlg->setBorderWidth(WIDGET_BORDER_WIDTH);
    friendGrpDlg->setRadius(30);

    m_audioReceiver = new AudioReceiver(this, m_ws);
    //m_audioReceiver->attachWebSocket(m_ws);

    homeworkDialog = new TACHomeworkDialog(this);
    homeworkDialog->setContent("语文<br>背诵《荷塘月色》第二段");

    wallpaperLibraryDialog = new TACWallpaperLibraryDialog(this);
    userMenuDlg = new TAUserMenuDialog(this);

    countDownDialog = new TACCountDownDialog(this);
    countDownWidget = new TACCountDownWidget(this);
    
    // 双击极简模式切换到完整模式
    connect(countDownWidget, &TACCountDownWidget::doubleClicked, this, [=]() {
        if (countDownDialog && countDownWidget) {
            countDownWidget->hide();
            countDownDialog->show();
            countDownDialog->raise();
            countDownDialog->activateWindow();
            
            // 居中显示完整模式对话框
            QScreen* screen = QApplication::primaryScreen();
            QRect screenGeometry = screen->geometry();
            int x = (screenGeometry.width() - countDownDialog->width()) / 2;
            int y = (screenGeometry.height() - countDownDialog->height()) / 2;
            countDownDialog->move(x, y);
        }
    });
    
    // 点击完整模式的"极简"按钮切换到极简模式
    connect(countDownDialog, &TACCountDownDialog::switchToMinimalMode, this, [=]() {
        if (countDownWidget && countDownDialog) {
            countDownDialog->hide();
            // 更新极简模式的内容
            countDownWidget->setContent(countDownDialog->content());
            countDownWidget->show();
            countDownWidget->raise();
            countDownWidget->activateWindow();
        }
    });
    
    // 初始化：显示极简模式
    countDownWidget->setContent(countDownDialog->content());
    countDownWidget->show();

    datetimeWidget = new TACDateTimeWidget(this);
    datetimeWidget->show();

    datetimeDialog = new TACDateTimeDialog(this);
    datetimeDialog->hide(); // 初始隐藏，默认显示极简模式
    connect(datetimeDialog, &TACDateTimeDialog::updateType, this, [=](int type) {
        datetimeWidget->setType(type);
    });
    
    // 连接极简模式双击事件 - 切换到完整模式
    connect(datetimeWidget, &TACDateTimeWidget::doubleClicked, this, [=]() {
        datetimeWidget->hide();
        
        // 将完整模式窗口居中显示
        QRect screenRect = datetimeWidget->getScreenGeometryWithTaskbar();
        if (!screenRect.isEmpty()) {
            QSize dialogSize = datetimeDialog->size();
            int x = screenRect.x() + (screenRect.width() - dialogSize.width()) / 2;
            int y = screenRect.y() + (screenRect.height() - dialogSize.height()) / 2;
            datetimeDialog->move(x, y);
        }
        
        datetimeDialog->show();
    });
    
    // 连接完整模式极简按钮 - 切换回极简模式
    connect(datetimeDialog, &TACDateTimeDialog::switchToMinimalMode, this, [=]() {
        datetimeDialog->hide();
        // 重置极简模式窗口到默认位置
        datetimeWidget->resetToDefaultPosition();
        datetimeWidget->show();
    });

    logoWidget = new TACLogoWidget(this);
    logoWidget->updateLogo(".\\res\\img\\qinghua.png");

    connect(logoWidget, &TACLogoWidget::doubleClicked, this, [=]() {
        // 如果logoWidget已经设置了图标，则在打开对话框时显示
        QString currentLogo = logoWidget->getLogoFileName();
        if (!currentLogo.isEmpty()) {
            logoDialog->setLogoFileName(currentLogo);
        }
        logoDialog->show();
    });

    logoWidget->show();

    // 隐藏学校名称按钮
    schoolLabelWidget = new TACSchoolLabelWidget(this);
    schoolLabelWidget->hide(); // 隐藏学校名称按钮

    classLabelWidget = new TACClassLabelWidget(this);
    connect(classLabelWidget, &TACClassLabelWidget::doubleClicked, this, [=]() {
        // 如果classLabelWidget已经设置了班级名称，则在打开对话框时显示
        QString currentClassName = classLabelWidget->getContent();
        if (!currentClassName.isEmpty()) {
            logoDialog->setClassName(currentClassName);
        }
        logoDialog->show();
    });
    classLabelWidget->show();

    // 初始化荣誉图标widgets列表（最多3个，初始为空）
    honorIconWidgets.clear();
    
    // 保留原来的trayLabelWidget用于系统托盘功能（隐藏，不使用）
    trayLabelWidget = new TACTrayLabelWidget(this);
    trayLabelWidget->updateLogo(".\\res\\img\\com_bottom_ic_component@2x.png");
    //trayLabelWidget->hide();

    trayLabelWidget->show();

    connect(trayLabelWidget, &TACTrayLabelWidget::clicked, this, [=]() {
        //logoDialog->show();
        if (trayWidget)
        {
            QRect rect = trayLabelWidget->geometry();
            trayWidget->move(rect.x() - trayWidget->width(), rect.y() - trayWidget->height() - 10);
            if (trayWidget->isHidden())
                trayWidget->show();
            else
                trayWidget->hide();
        }
    });

    logoDialog = new TACLogoDialog(this);
    connect(logoDialog, &TACLogoDialog::enterClicked, this, [=]() {
        // 更新学校logo
        if (logoWidget && !logoDialog->getLogoFileName().isEmpty())
        {
            logoWidget->updateLogo(logoDialog->getLogoFileName());
        }
        
        // 更新班级名称
        if (classLabelWidget && !logoDialog->getClassName().isEmpty())
        {
            classLabelWidget->setContent(logoDialog->getClassName());
        }
        
        // 更新荣誉图标（最多3个）
        QStringList honorIcons = logoDialog->getHonorIconFileNames();
        
        // 删除多余的widgets（如果荣誉图标数量减少了）
        while (honorIconWidgets.size() > honorIcons.size()) {
            TACHonorIconWidget* widget = honorIconWidgets.takeLast();
            if (widget) {
                widget->deleteLater();
            }
        }
        
        // 更新或创建荣誉图标widgets
        // 计算间隔：学校logo到班级名称的间隔已缩小为50
        // 班级名称到荣誉图标的间隔也缩小
        const int spacingBetweenLogoAndClass = 50; // 学校logo到班级名称的间隔（已缩小）
        const int spacingBetweenClassAndHonor = 50; // 班级名称到荣誉图标的间隔（缩小）
        const int honorIconSpacing = 5; // 荣誉图标之间的间隔（再缩小，从10改为5）
        const int honorIconWidth = 140; // 荣誉图标宽度（和学校logo一样，140x70）
        const int honorIconHeight = 70; // 荣誉图标高度（和学校logo一样，140x70）
        
        for (int i = 0; i < honorIcons.size() && i < 3; ++i) {
            TACHonorIconWidget* widget = nullptr;
            if (i < honorIconWidgets.size() && honorIconWidgets[i]) {
                widget = honorIconWidgets[i];
            } else {
                widget = new TACHonorIconWidget(this);
                honorIconWidgets.append(widget);
                widget->show();
            }
            
            // 设置位置：放在班级名称右边，间隔缩小
            // 班级名称到第一个荣誉图标: spacingBetweenClassAndHonor(50)
            // 荣誉图标之间的间隔: honorIconSpacing(5)
            if (classLabelWidget) {
                QRect classRect = classLabelWidget->geometry();
                // 班级名称右边 + 间隔 + 前面所有荣誉图标的(宽度+荣誉图标间隔)
                int x = classRect.right() + spacingBetweenClassAndHonor + i * (honorIconWidth + honorIconSpacing);
                int y = classRect.y(); // 与班级名称同一行
                widget->move(x, y);
            } else if (logoWidget) {
                // 如果班级名称不存在，使用学校logo作为参考（这种情况不应该发生）
                QRect logoRect = logoWidget->geometry();
                int x = logoRect.right() + spacingBetweenClassAndHonor + i * (honorIconWidth + honorIconSpacing);
                int y = logoRect.y();
                widget->move(x, y);
            }
            
            // 更新图标
            widget->updateLogo(honorIcons[i]);
        }
        
        // 点击确定按钮后隐藏窗口
        logoDialog->hide();
    });
   
    folderDialog = new TACFolderDialog(this);
    folderWidget = new TACFolderWidget(this);
    connect(folderWidget, &TACFolderWidget::clicked, this, [=]() {
        folderDialog->show();
    });
    folderWidget->show();

    classWeekCourseScheduldDialog = new TACClassWeekCourseScheduleDialog(this);

    if (m_httpHandler)
    {
        //QMap<QString, QString> params;
        //params["phone"] = "13621907363";
        m_httpHandler->get(QString("http://47.100.126.194:5000/userInfo?userid=" + QString::number(user_id)));
    }
    //updateBackground("C:/workspace/obs/TeacherAssistant/TeacherAssistantClient/res/bg/5.jpg");
}
void TACMainDialog::updateBackground(const QString & fileName)
{
    m_background = QPixmap(fileName);
    update();
}
void TACMainDialog::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (m_background.isNull()) {
        return;
    }
    QPixmap scaled = m_background.scaled(
        size(),
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation
    );
    QRect r = scaled.rect();
    r.moveCenter(rect().center());
    painter.drawPixmap(r, scaled);
}

void TACMainDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    this->update();
}

bool TACMainDialog::InitSDK() { //初始化ImSDK
    //获取配置
    QJsonObject json_user_config;
    json_user_config[kTIMUserConfigIsReadReceipt] = true;  // 开启已读回执

    QJsonObject json_socks5_value;
    json_socks5_value[kTIMSocks5ProxyInfoIp] = "111.222.333.444";
    json_socks5_value[kTIMSocks5ProxyInfoPort] = 8888;
    json_socks5_value[kTIMSocks5ProxyInfoUserName] = "";
    json_socks5_value[kTIMSocks5ProxyInfoPassword] = "";
    QJsonObject json_config;
    json_config[kTIMSetConfigUserConfig] = json_user_config;
    //json_config[kTIMSetConfigSocks5ProxyInfo] = json_socks5_value;
    json_config[kTIMSetConfigUserDefineData] = "1.3.4.5.6.7";

    QJsonDocument jsonConfigDoc(json_config);
    QByteArray jsonConfigBytes = jsonConfigDoc.toJson(QJsonDocument::Compact);
    TIMSetConfig(jsonConfigBytes.constData(), [](int32_t code, const char* desc, const char* json_param, const void* user_data) {
        //CIMWnd* ths = (CIMWnd*)user_data;
        //ths->Logf("config", kTIMLog_Info, json_param);
        }, this);

    // 初始化
    std::string sdkappid = std::to_string(GenerateTestUserSig::instance().getSDKAppID());
    //std::string path = Wide2UTF8(m_LogPath->GetText().GetData());

    TCHAR szTPath[MAX_PATH] = { 0 };
    ::GetModuleFileName(NULL, szTPath, MAX_PATH); //获取工具目录
    int len = _tcslen(szTPath);
    for (int i = len; i >= 0; i--) {
        if ((szTPath[i] == '/') || (szTPath[i] == '\\')) {
            szTPath[i] = 0;
            break;
        }
    }

    uint64_t sdk_app_id = atoi(sdkappid.c_str());

    std::string json_init_cfg;

    QJsonObject json_value_init;
    json_value_init[kTIMSdkConfigLogFilePath] = QString::fromStdString(Wide2UTF8(szTPath));
    json_value_init[kTIMSdkConfigConfigFilePath] = QString::fromStdString(Wide2UTF8(szTPath));

    // 在TIMInit之前注册消息接收回调（确保能接收到离线消息）
    ChatDialog::ensureCallbackRegistered();
    
    QJsonDocument jsonInitDoc(json_value_init);
    QByteArray jsonInitBytes = jsonInitDoc.toJson(QJsonDocument::Compact);
    if (TIM_SUCC == TIMInit(sdk_app_id, jsonInitBytes.constData()))
    {
        qDebug() << "TIMInit succeeded!";
        
        // 在TIMInit之后、登录之前注册群组和好友回调
        GroupNotifyDialog::ensureCallbackRegistered();
        FriendNotifyDialog::ensureCallbackRegistered();
        
        return true;
    }
    else
    {
        qDebug() << "TIMInit failed!";
        return false;
    }
    //ChangeMainView(INITSDK_VIEW, LOGIN_VIEW);
    //Logf("InitSdk", kTIMLog_Info, "sdkappid:%s Log&Cfg path:%s", sdkappid.c_str(), path.c_str());
}

void TACMainDialog::Login(std::string classId) { //登入
    UserInfo userinfo = CommonInfo::GetData();
    userinfo.classId = classId.c_str();
    CommonInfo::InitData(userinfo);
    m_userInfo.classId = classId.c_str();

    std::string usersig = GenerateTestUserSig::instance().genTestUserSig(classId);
    //login_id = userid;
    TIMLogin(classId.c_str(), usersig.c_str(), [](int32_t code, const char* desc, const char* json_param, const void* user_data) {
        TACMainDialog* ths = (TACMainDialog*)user_data;
        if (code != ERR_SUCC) { // 登入失败
            //ths->Logf("Login", kTIMLog_Error, "Failure!code:%d desc", code, desc);
            return;
        }

        if (ths->userMenuDlg)
        {
            ths->userMenuDlg->InitData(ths->m_userInfo);
            ths->userMenuDlg->InitUI();
            CommonInfo::InitData(ths->m_userInfo);

            if (ths->friendGrpDlg)
            {
                ths->friendGrpDlg->InitWebSocket();
                ths->friendGrpDlg->InitData();
            }

            // 注意：先让各个对话框把 WebSocket 信号 connect 好，再真正打开 WebSocket 连接，
            // 避免登录后服务端立即推送的数据在 UI 尚未 connect 前被“错过”。
            if (TACMainDialog::m_ws)
            {
                TaQTWebSocket::InitWebSocket(TACMainDialog::m_ws);
            }
        }

        if (ths->schoolInfoDlg)
        {
            ths->schoolInfoDlg->InitData(ths->m_userInfo);
        }

        ////登入成功
        //ths->Log("Login", kTIMLog_Info, "Success!");
        //ths->ChangeMainView(LOGIN_VIEW, MAIN_LOGIC_VIEW);
        //ths->SetControlVisible(_T("logout_btn"), true);
        //ths->SetControlVisible(_T("test_btn"), true);

        //ths->SetControlVisible(_T("cur_login_info"), true);
        //ths->SetControlText(_T("cur_login_info"), UTF82Wide(ths->login_id).c_str());
        //ths->InitConvList();
        //ths->GetGroupJoinedList();
        }, this);

    //Logf("Login", kTIMLog_Info, "User Id:%s Sig:%s", userid.c_str(), usersig.c_str());
    //SetControlText(_T("cur_loginid_lbl"), UTF82Wide(userid).c_str());
}

void TACMainDialog::downloadAvatarFromUrl(const QString& avatarUrl, const QString& savePath)
{
    if (!m_networkManager) {
        qWarning() << "Network manager is not initialized";
        return;
    }

    QUrl url(avatarUrl);
    if (!url.isValid()) {
        qWarning() << "Invalid avatar URL:" << avatarUrl;
        return;
    }

    QNetworkRequest networkRequest;
    networkRequest.setUrl(url);
    
    // 如果是 HTTPS，设置 SSL 配置
    if (url.scheme().toLower() == "https") {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
        networkRequest.setSslConfiguration(sslConfig);
    }

    QNetworkReply* reply = m_networkManager->get(networkRequest);
    
    // 忽略 SSL 错误（开发环境常用做法）
    connect(reply, &QNetworkReply::sslErrors, this, [=](const QList<QSslError>& errors) {
        qDebug() << "SSL errors ignored for avatar download, URL:" << avatarUrl;
        reply->ignoreSslErrors();
    });

    // 处理下载完成
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray imageData = reply->readAll();
            if (imageData.isEmpty()) {
                qWarning() << "Avatar image data is empty from URL:" << avatarUrl;
            } else {
                // 保存图片到本地
                QFile file(savePath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(imageData);
                    file.close();
                    qDebug() << "Avatar downloaded successfully:" << savePath;
                    
                    // 更新用户信息中的头像路径
                    m_userInfo.strHeadImagePath = savePath;
                    
                    // 更新 CommonInfo 中的用户信息
                    CommonInfo::InitData(m_userInfo);
                    
                    // 如果 userMenuDlg 已创建，更新其头像显示
                    if (userMenuDlg) {
                        userMenuDlg->updateAvatar(savePath);
                    }
                    
                    // 如果 schoolInfoDlg 已创建，也更新其显示
                    if (schoolInfoDlg) {
                        schoolInfoDlg->InitData(m_userInfo);
                    }
                } else {
                    qWarning() << "Cannot open file for writing:" << savePath;
                }
            }
        } else {
            qWarning() << "Failed to download avatar from URL:" << avatarUrl 
                       << ", error:" << reply->errorString();
        }
        reply->deleteLater();
    });
}

// 更新课前准备按钮的可见性（根据群组设置）
void TACMainDialog::updatePrepareClassButtonVisibility()
{
    qDebug() << "TACMainDialog::updatePrepareClassButtonVisibility() called";
    if (!navBarWidget) {
        qDebug() << "navBarWidget is null, returning";
        return;
    }
    
    bool linkPreClassPreparationEnabled = false;
    if (friendGrpDlg) {
        ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
        if (!loginInfo.class_id.isEmpty()) {
            // 确保ScheduleDialog已创建（如果不存在则先创建，但不显示）
            friendGrpDlg->ensureScheduleDialogCreated(loginInfo.class_id);
            ScheduleDialog* scheduleDlg = friendGrpDlg->getScheduleDialog(loginInfo.class_id);
            if (scheduleDlg) {
                QGroupInfo* groupInfo = scheduleDlg->getGroupInfo();
                if (groupInfo) {
                    // 根据"关联课前准备"来判断是否显示课前准备功能键
                    linkPreClassPreparationEnabled = groupInfo->isLinkPreClassPreparationEnabled();
                }
            }
        }
    }
    
    // 更新"课前准备"功能键的可见性：如果"关联课前准备"为true，则显示；否则隐藏
    navBarWidget->setPrepareClassButtonVisible(linkPreClassPreparationEnabled);
}

// 更新家庭作业按钮的可见性（根据群组设置）
void TACMainDialog::updateHomeworkButtonVisibility()
{
    qDebug() << "TACMainDialog::updateHomeworkButtonVisibility() called";
    if (!navBarWidget) {
        qDebug() << "navBarWidget is null, returning";
        return;
    }
    
    bool linkHomeworkEnabled = false;
    if (friendGrpDlg) {
        ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
        if (!loginInfo.class_id.isEmpty()) {
            // 确保ScheduleDialog已创建（如果不存在则先创建，但不显示）
            friendGrpDlg->ensureScheduleDialogCreated(loginInfo.class_id);
            ScheduleDialog* scheduleDlg = friendGrpDlg->getScheduleDialog(loginInfo.class_id);
            if (scheduleDlg) {
                QGroupInfo* groupInfo = scheduleDlg->getGroupInfo();
                if (groupInfo) {
                    // 根据"关联家庭作业"来判断是否显示家庭作业功能键
                    linkHomeworkEnabled = groupInfo->isLinkHomeworkEnabled();
                    qDebug() << "Group ID:" << loginInfo.class_id << ", link_homework enabled:" << linkHomeworkEnabled;
                } else {
                    qDebug() << "QGroupInfo is null for group ID:" << loginInfo.class_id;
                }
            } else {
                qDebug() << "ScheduleDialog is null for group ID:" << loginInfo.class_id;
            }
        } else {
            qDebug() << "Class ID is empty, cannot check group settings.";
        }
    } else {
        qDebug() << "FriendGroupDialog is null.";
    }
    
    // 更新"家庭作业"功能键的可见性：如果"关联家庭作业"为true，则显示；否则隐藏
    navBarWidget->setHomeworkButtonVisible(linkHomeworkEnabled);
    qDebug() << "Set homework button visibility to:" << linkHomeworkEnabled;
}

// 更新今日课表按钮的可见性（根据群组设置）
void TACMainDialog::updateTodayScheduleButtonVisibility()
{
    // 这个方法现在用于更新值日表按钮的可见性（根据"关联值日表"toggle）
    // 直接调用 updateDutyRosterButtonVisibility 方法
    updateDutyRosterButtonVisibility();
}

// 更新值日表按钮的可见性（根据群组设置）
void TACMainDialog::updateDutyRosterButtonVisibility()
{
    qDebug() << "TACMainDialog::updateDutyRosterButtonVisibility() called";
    if (!navBarWidget) {
        qDebug() << "navBarWidget is null, returning";
        return;
    }
    
    bool linkDutyRosterEnabled = false;
    if (friendGrpDlg) {
        ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
        if (!loginInfo.class_id.isEmpty()) {
            // 确保ScheduleDialog已创建（如果不存在则先创建，但不显示）
            friendGrpDlg->ensureScheduleDialogCreated(loginInfo.class_id);
            ScheduleDialog* scheduleDlg = friendGrpDlg->getScheduleDialog(loginInfo.class_id);
            if (scheduleDlg) {
                QGroupInfo* groupInfo = scheduleDlg->getGroupInfo();
                if (groupInfo) {
                    // 根据"关联值日表"来判断是否显示值日表功能键
                    linkDutyRosterEnabled = groupInfo->isLinkDutyRosterEnabled();
                    qDebug() << "Group ID:" << loginInfo.class_id << ", link_duty_roster enabled:" << linkDutyRosterEnabled;
                } else {
                    qDebug() << "QGroupInfo is null for group ID:" << loginInfo.class_id;
                }
            } else {
                qDebug() << "ScheduleDialog is null for group ID:" << loginInfo.class_id;
            }
        } else {
            qDebug() << "Class ID is empty, cannot check group settings.";
        }
    } else {
        qDebug() << "FriendGroupDialog is null.";
    }
    
    // 更新"值日表"功能键的可见性：如果"关联值日表"为true，则显示；否则隐藏
    navBarWidget->setTodayScheduleButtonVisible(linkDutyRosterEnabled);
    qDebug() << "Set duty roster button visibility to:" << linkDutyRosterEnabled;
}

// 显示壁纸对话框
void TACMainDialog::showWallpaperDialog()
{
    if (wallpaperLibraryDialog) {
        QScreen* screen = QApplication::primaryScreen();
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - wallpaperLibraryDialog->width()) / 2;
        int y = (screenGeometry.height() - wallpaperLibraryDialog->height()) / 2;
        wallpaperLibraryDialog->move(x, y);
        wallpaperLibraryDialog->show();
        wallpaperLibraryDialog->raise();
        wallpaperLibraryDialog->activateWindow();
    }
}
