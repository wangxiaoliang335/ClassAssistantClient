#pragma once

#include <QDialog>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QResizeEvent>
#include <QWebSocket>
#include <QTimer>
#include <QDebug>
#include "TAFloatingWidget.h"
#include "TACNavigationBarWidget.h"
#include "TACCountDownWidget.h"
#include "TACDateTimeDialog.h"
#include "TACDateTimeWidget.h"
#include "TACLogoWidget.h"
#include "TACLogoDialog.h"
#include "TACFolderWidget.h"
#include "TACCourseSchedule.h"
#include "TACFolderDialog.h"
#include "TACCountDownDialog.h"
#include "TACWallpaperLibraryDialog.h"
#include "TACHomeworkDialog.h"
#include "TACIMDialog.h"
#include "../Common/HomeworkViewDialog.h"

// 前向声明作业项结构（与HomeworkViewDialog.h中的定义一致）
struct HomeworkItem;
#include "TACDesktopManagerWidget.h"
#include "TACPrepareClassDialog.h"
#include "TACClassWeekCourseScheduleDialog.h"
#include "TACCourseScheduleMenuDialog.h"
#include "ScheduleDialog.h"
#include "ui_TACMainDialog.h"
#include "TACTrayWidget.h"
#include "SchoolInfoDialog.h"
#include "ClassInfoDialog.h"
#include "TAUserMenuDialog.h"
#include "FriendGroupDialog.h"
#include "AudioReceiver.h"
#include "TAHttpHandler.h"
#include "TaQTWebSocket.h"
#include "TIMCloud.h"
#include "GenerateTestUserSig.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslConfiguration>

class TACMainDialog : public QDialog
{
	Q_OBJECT

public:
	TACMainDialog(QWidget *parent = nullptr);
	~TACMainDialog();
	void Init(QString classId, int user_id);
	bool InitSDK();
	void Login(std::string userid);
	
public slots:
	// 更新课前准备按钮的可见性（根据群组设置）
	void updatePrepareClassButtonVisibility();
	// 更新家庭作业按钮的可见性（根据群组设置）
	void updateHomeworkButtonVisibility();
	// 更新今日课表按钮的可见性（根据群组设置）
	void updateTodayScheduleButtonVisibility();
	// 显示壁纸对话框
	void showWallpaperDialog();
	
protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

public slots:
	// 更新背景壁纸
	void updateBackground(const QString& fileName);
	
private:
	void downloadAvatarFromUrl(const QString& avatarUrl, const QString& savePath);

// 更新课前准备按钮的可见性（根据群组设置）- 已移动到 public slots

//private slots:
//    void onConnected() {
//        qDebug() << "✅ 已连接到 WebSocket 服务";
//    }
//
//    void onMessageReceived(const QString& message) {
//        // 根据消息内容做不同处理
//        if (message == "pong") {
//            qDebug() << "💓 收到心跳回应: pong";
//        }
//        else if (message.startsWith("[私信来自")) {
//            qDebug() << "📩 收到私信:" << message;
//        }
//        else if (message.startsWith("[")) {
//            // 广播格式：[用户ID 广播] 消息
//            qDebug() << "📢 收到广播:" << message;
//        }
//        else {
//            // 普通消息
//            qDebug() << "📨 收到消息:" << message;
//        }
//    }
//
//    void onError(QAbstractSocket::SocketError error) {
//        qWarning() << "连接错误:" << socket->errorString();
//    }
//
//    void sendHeartbeat() {
//        if (socket->state() == QAbstractSocket::ConnectedState) {
//            socket->sendTextMessage("ping");
//        }
//    }
//
//public slots:
//    void sendBroadcast(const QString& text) {
//        if (socket->state() == QAbstractSocket::ConnectedState) {
//            socket->sendTextMessage(text);
//        }
//    }
//
//    void sendPrivate(const QString& targetId, const QString& text) {
//        if (socket->state() == QAbstractSocket::ConnectedState) {
//            QString msg = QString("to:%1:%2").arg(targetId, text);
//            socket->sendTextMessage(msg);
//        }
//    }

private:
	Ui::TACMainDialogClass ui;
	QPixmap m_background;
	QPointer<TACNavigationBarWidget> navBarWidget;
	QPointer<TACCountDownWidget> countDownWidget;
	QPointer<TACCountDownDialog> countDownDialog;
	QPointer<TACDateTimeDialog> datetimeDialog;
	QPointer<TACDateTimeWidget> datetimeWidget;
	QPointer<TACLogoWidget> logoWidget;
	QPointer<TACLogoDialog> logoDialog;
	QPointer<TACSchoolLabelWidget> schoolLabelWidget;
	QPointer<TACClassLabelWidget> classLabelWidget;
	QPointer<TACTrayLabelWidget>  trayLabelWidget;

	QPointer<TACFolderWidget> folderWidget;
	QPointer<TACCourseSchedule> courseSchedule;
	QPointer<TACFolderDialog> folderDialog;
	QPointer<TACWallpaperLibraryDialog> wallpaperLibraryDialog;
	QPointer<TAUserMenuDialog> userMenuDlg;
	QPointer<TACHomeworkDialog> homeworkDialog; // 保留用于兼容
	QPointer<HomeworkViewDialog> homeworkViewDialog; // 作业展示窗口（使用 ScheduleDialog 中的窗口）
	QPointer<TACIMDialog> imDialog;
	
	// 作业缓存：按日期聚合 (date(yyyy-MM-dd) -> QList<HomeworkItem>)
	// 每条作业包含科目、内容和创建时间，支持同一天同一科目的多条作业
	QMap<QString, QList<HomeworkItem>> m_homeworkByDate;
	QPointer<FriendGroupDialog> friendGrpDlg;

	QPointer<AudioReceiver> m_audioReceiver;
	QPointer<TACDesktopManagerWidget> desktopManagerWidget;
	QPointer<TACPrepareClassDialog> prepareClassDialog;
	QPointer<TACClassWeekCourseScheduleDialog> classWeekCourseScheduldDialog;
	QPointer<TACCourseScheduleMenuDialog> courseScheduleMenuDialog;
	QPointer<TACTrayWidget> trayWidget;
	QPointer<SchoolInfoDialog> schoolInfoDlg;
	QPointer<ClassInfoDialog> classInfoDlg;
	TAHttpHandler* m_httpHandler = NULL;
	UserInfo m_userInfo;
	QNetworkAccessManager* m_networkManager = NULL;

	static TaQTWebSocket* m_ws;

	//QString m_userId;
	//QWebSocket* socket;
	//QTimer* heartbeatTimer;
};
