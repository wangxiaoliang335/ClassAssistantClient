#pragma once

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QJsonObject>
#include <qvector.h>
#include <qdialog.h>

class TaQTWebSocket  : public QObject
{
	Q_OBJECT

public:
	explicit TaQTWebSocket(QObject *parent);
	~TaQTWebSocket();
	static void InitWebSocket(TaQTWebSocket* wsInstance);
	static void regRecvDlg(QDialog* dlg);
	static void sendPrivateMessage(QString msg);
	static void sendBinaryMessage(QByteArray packet);
signals:
	void newMessage(QString msg);
	void newBinaryMessage(const QByteArray& msg);
	void homeworkReceived(const QJsonObject& homeworkData); // 家庭作业消息信号
	void notificationReceived(const QJsonObject& notificationData); // 通知消息信号
	void prepareClassHistoryReceived(const QJsonObject& prepareData); // 课前准备历史（登录后下发）

private slots:
	void onConnected();
	void onMessageReceived(const QString& msg);
	void onBinaryMessageReceived(const QByteArray& message);
	void sendBroadcast();
	void sendHeartbeat();
private:
	static QTimer* heartbeatTimer;
	static QWebSocket* socket;
	static QVector<QString> m_NoticeMsg;
	static QVector<QDialog*> m_vecRecvDlg;
};

