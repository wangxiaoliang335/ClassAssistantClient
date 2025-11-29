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
#include "TACDesktopManagerWidget.h"
#include "TACPrepareClassDialog.h"
#include "TACClassWeekCourseScheduleDialog.h"
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
	void Init(QString qPhone, int user_id);
	bool InitSDK();
	void Login(std::string userid);
protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	void updateBackground(const QString& fileName);
	void downloadAvatarFromUrl(const QString& avatarUrl, const QString& savePath);

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
	QPointer<TACHomeworkDialog> homeworkDialog;
	QPointer<TACIMDialog> imDialog;
	QPointer<FriendGroupDialog> friendGrpDlg;

	QPointer<AudioReceiver> m_audioReceiver;
	QPointer<TACDesktopManagerWidget> desktopManagerWidget;
	QPointer<TACPrepareClassDialog> prepareClassDialog;
	QPointer<TACClassWeekCourseScheduleDialog> classWeekCourseScheduldDialog;
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
