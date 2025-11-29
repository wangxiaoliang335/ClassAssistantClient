#pragma once
#include "RtmpMediaStreamer.h"
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QVariant>
#include <QFrame>
#include <qfiledialog.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <qdebug.h>
#include <qlineedit.h>
#include <QWidget>
#include <QObject>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QIcon>
#include <QAudioInput>
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QIODevice>
#include <QTimer>
#include <qprogressbar.h>
#include <QPoint>
#include <QBrush>
#include <QPair>
#include <QColor>
#include <QUrl>
#include <QUrlQuery>
#include <QDate>
#include <QPainter>
#include <QPainterPath>
#include <QRegion>
#include <QGroupInfo.h>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <Windows.h>
#include <shellapi.h>
#include <QRegularExpression>
#include <QSet>
#include <string>
#include <QProcess>
#include <QThread>
#include <QMenu>
#include <QTextEdit>
#include <QMessageBox>
#include "CustomListDialog.h"
#include "ClickableLabel.h"
#include "TAHttpHandler.h"
#include "ChatDialog.h"
#include "CommonInfo.h"
#include "QGroupInfo.h"
#include "TAHttpHandler.h"
#include "ArrangeSeatDialog.h"
#include "GroupNotifyDialog.h"
#include "QXlsx/header/xlsxdocument.h"
#include "QXlsx/header/xlsxworksheet.h"
#include "QXlsx/header/xlsxcell.h"
#include "QXlsx/header/xlsxglobal.h"
QT_BEGIN_NAMESPACE_XLSX
QT_END_NAMESPACE_XLSX
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
// 前向声明，避免循环依赖
class HeatmapSegmentDialog;
class HeatmapViewDialog;
// SegmentRange 定义在 HeatmapTypes.h 中
#include "HeatmapTypes.h"
#include <random>
#include <algorithm>
#include <exception>

//// 学生信息结构（用于排座）
//#ifndef STUDENT_INFO_DEFINED
//#define STUDENT_INFO_DEFINED
//struct StudentInfo {
//    QString id;      // 学号
//    QString name;    // 姓名
//    double score;    // 成绩（用于排序）
//    int originalIndex; // 原始索引
//    QMap<QString, double> attributes; // 多个属性值（如"背诵"、"语文"等）
//};
//#endif

class ClickableWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ClickableWidget(QWidget* parent = nullptr) : QWidget(parent) {}

signals:
	void clicked();  // 点击信号

protected:
	void mousePressEvent(QMouseEvent* event) override
	{
		if (event->button() == Qt::LeftButton)
		{
			emit clicked();
		}
		QWidget::mousePressEvent(event); // 可选：让父类继续处理事件
	}
};

class ScheduleDialog : public QDialog
{
	Q_OBJECT
public:
	ScheduleDialog(QString classid, QWidget* parent = nullptr, TaQTWebSocket* pWs = NULL) : QDialog(parent)
	{
		setWindowTitle("课程表");
		// 设置无边框窗口
		setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
		// 座位区域：10列×121像素=1210像素宽，6行×50像素=300像素高
		// 窗口大小：宽度1238（1251×0.99），高度669（676×0.99）
        resize(1238, 669);
        m_cornerRadius = 16;
        updateMask();
        setStyleSheet("QDialog { background-color: #5C5C5C; color: white; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QPushButton { font-size:14px; color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QLabel { font-size:14px; color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QLineEdit { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QTextEdit { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QGroupBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QTableWidget { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; gridline-color: #5C5C5C; font-weight: bold; } "
			"QTableWidget::item { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QComboBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QCheckBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QRadioButton { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QScrollArea { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QListWidget { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QSpinBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QProgressBar { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
			"QSlider { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; }");
		
		// 关闭按钮（右上角）
		closeButton = new QPushButton(this);
		closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
		closeButton->setIconSize(QSize(22, 22));
		closeButton->setFixedSize(QSize(22, 22));
		closeButton->setStyleSheet("background: transparent;");
        closeButton->move(width() - closeButton->width() - 4, 4);
		closeButton->hide();
		connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

		m_classid = classid;

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

							// user_details 对象
							QJsonObject userDetails = friendObj.value("user_details").toObject();
							QString phone = userDetails.value("phone").toString();
							QString uname = userDetails.value("name").toString();
							QString sex = userDetails.value("sex").toString();

							/********************************************/
							QString avatar = userDetails.value("avatar").toString();
							QString strIdNumber = userDetails.value("id_number").toString();
							QString avatarBase64 = userDetails.value("avatar_base64").toString();

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

							/*if (fLayout)
							{
								fLayout->addLayout(makePairBtn(filePath, name, "green", "white", "", false));
							}*/
							/********************************************/
						}

						QJsonObject oTmp = obj["data"].toObject();
						QString strTmp = oTmp["message"].toString();
						qDebug() << "status:" << oTmp["code"].toString();
						qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
						//errLabel->setText(strTmp);
						//user_id = oTmp["user_id"].toInt();
					}
					else if (obj["data"].isObject())
					{
						QJsonObject dataObj = obj["data"].toObject();
						
						// 处理成绩表数据（/student-scores/get 接口返回）
						// 数据格式：{"code": 200, "data": {"scores": [...]}}
						if (dataObj.contains("scores") && dataObj["scores"].isArray())
						{
							QJsonArray scoresArray = dataObj["scores"].toArray();
							m_students.clear();
							
							for (int i = 0; i < scoresArray.size(); i++)
							{
								QJsonObject scoreObj = scoresArray[i].toObject();
								
								StudentInfo student;
								student.id = scoreObj["student_id"].toString();
								student.name = scoreObj["student_name"].toString();
								student.originalIndex = i;
								
								// 读取各科成绩并填充到 attributes 中（处理 null 值）
								if (scoreObj.contains("chinese") && !scoreObj["chinese"].isNull()) {
									double chinese = scoreObj["chinese"].toDouble();
									student.attributes["语文"] = chinese;
								}
								if (scoreObj.contains("math") && !scoreObj["math"].isNull()) {
									double math = scoreObj["math"].toDouble();
									student.attributes["数学"] = math;
								}
								if (scoreObj.contains("english") && !scoreObj["english"].isNull()) {
									double english = scoreObj["english"].toDouble();
									student.attributes["英语"] = english;
								}
								
								// 读取总分（字段名是 total_score）
								if (scoreObj.contains("total_score") && !scoreObj["total_score"].isNull()) {
									double total = scoreObj["total_score"].toDouble();
									student.attributes["总分"] = total;
									student.score = total; // 使用总分作为排序依据
								} else {
									// 如果没有总分，计算总分
									double total = 0;
									if (student.attributes.contains("语文")) {
										total += student.attributes["语文"];
									}
									if (student.attributes.contains("数学")) {
										total += student.attributes["数学"];
									}
									if (student.attributes.contains("英语")) {
										total += student.attributes["英语"];
									}
									student.attributes["总分"] = total;
									student.score = total;
								}
								
							m_students.append(student);
						}
						
						qDebug() << "从服务器获取成绩表成功，学生数量:" << m_students.size();
						
						// 自动刷新座位表，使用"正序"排序（按成绩从高到低）
						if (!m_students.isEmpty() && seatTable) {
							arrangeSeats(m_students, "正序");
							qDebug() << "已自动刷新座位表，使用正序排序";
						}
						}
						// 处理 /groups/members 接口返回的成员列表
						else if (dataObj.contains("group_id") && dataObj["members"].isArray())
						{
							QString group_id = dataObj["group_id"].toString();
							
							m_groupMemberInfo.clear(); // 清空之前的成员列表
							
							QJsonArray memberArray = dataObj["members"].toArray();
							for (int i = 0; i < memberArray.size(); i++)
							{
								QJsonObject memberObj = memberArray[i].toObject();
								
								// 获取成员信息
								QString member_id = memberObj["user_id"].toString();
								QString member_name = memberObj["user_name"].toString();
								if (member_name.isEmpty()) {
									member_name = memberObj["member_name"].toString();
								}
								
								// 判断角色：self_role 400 表示群主（Owner），1 表示普通成员（Normal）
								int self_role = memberObj["self_role"].toInt();
								QString member_role = "成员"; // 默认是成员
								if (self_role == 400) { // kTIMMemberRole_Owner
									member_role = "群主";
								}
								
								// 如果接口返回的是 role 字符串，也可以直接使用
								if (memberObj.contains("role")) {
									QString roleStr = memberObj["role"].toString();
									if (roleStr == "owner" || roleStr == "Owner" || roleStr == "群主") {
										member_role = "群主";
									}
								}
								
								GroupMemberInfo groupMemInfo;
								groupMemInfo.member_id = member_id;
								groupMemInfo.member_name = member_name;
								groupMemInfo.member_role = member_role;
								m_groupMemberInfo.append(groupMemInfo);
							}

							// 设置到 m_groupInfo 对话框的好友列表
							if (m_groupInfo)
							{
								m_groupInfo->InitGroupMember(group_id, m_groupMemberInfo);
							}
						}
						else if (dataObj["joingroups"].isArray())
						{
							//gJoinLayout->addLayout(makeRowBtn("加入", "3", "orange", "black"));
							QJsonArray groupArray = dataObj["joingroups"].toArray();
							for (int i = 0; i < groupArray.size(); i++)
							{
								QJsonObject groupObj = groupArray.at(i).toObject();
								QString unique_group_id = groupObj.value("unique_group_id").toString();
								QString avatar_base64 = groupObj.value("avatar_base64").toString();
								QString headImage_path = groupObj.value("headImage_path").toString();
								//// 假设 avatarBase64 是 QString 类型，从 HTTP 返回的 JSON 中拿到
								//QByteArray imageData = QByteArray::fromBase64(avatar_base64.toUtf8());
								//QPixmap pixmap;
								//pixmap.loadFromData(imageData);

								// 从最后一个 "/" 之后开始截取
								QString fileName = headImage_path.section('/', -1);  // "320506197910016493_.png"
								QString saveDir = QCoreApplication::applicationDirPath() + "/group_images/" + unique_group_id; // 保存图片目录
								QDir().mkpath(saveDir);
								QString filePath = saveDir + "/" + fileName;

								if (avatar_base64.isEmpty()) {
									qWarning() << "No avatar data for" << filePath;
									//continue;
								}
								//m_userInfo.strHeadImagePath = filePath;

								// Base64 解码成图片二进制数据
								QByteArray imageData = QByteArray::fromBase64(avatar_base64.toUtf8());

								// 写入文件（覆盖旧的）
								QFile file(filePath);
								if (!file.open(QIODevice::WriteOnly)) {
									qWarning() << "Cannot open file for writing:" << filePath;
									//continue;
								}
								file.write(imageData);
								file.close();

								//lblAvatar->setPixmap(pixmap.scaled(lblAvatar->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
								//lblAvatar->setScaledContents(true);

								//gJoinLayout->addLayout(makePairBtn(filePath, groupObj.value("nickname").toString(), "white", "red", unique_group_id, false));
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

		m_pWs = pWs;
		m_chatDlg = new ChatDialog(this, m_pWs);
		customListDlg = new CustomListDialog(m_classid, this);
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 40, 20, 20);
        mainLayout->setSpacing(12);
		m_groupInfo = new QGroupInfo(this);
		
		// 连接成员退出群聊信号，当有成员退出时刷新成员列表
		connect(m_groupInfo, &QGroupInfo::memberLeftGroup, this, [this](const QString& groupId, const QString& leftUserId) {
			// 如果退出的是当前群组
			if (groupId == m_unique_group_id) {
				// 获取当前用户信息，判断是否是当前用户退出
				UserInfo userInfo = CommonInfo::GetData();
				QString currentUserId = userInfo.teacher_unique_id;
				
				// 先从本地成员列表中移除退出的用户（同步更新ScheduleDialog的成员列表）
				QVector<GroupMemberInfo> updatedMemberList;
				for (const auto& member : m_groupMemberInfo) {
					if (member.member_id != leftUserId) {
						updatedMemberList.append(member);
					}
				}
				m_groupMemberInfo = updatedMemberList; // 更新ScheduleDialog的成员列表
				
				// 判断是否是当前用户退出
				bool isCurrentUserLeft = (leftUserId == currentUserId);
				
				if (isCurrentUserLeft) {
					// 当前用户退出，关闭窗口并刷新父窗口
					qDebug() << "当前用户退出群聊，关闭群聊窗口，群组ID:" << groupId;
					
					// 发出信号通知父窗口刷新群列表
					emit this->groupLeft(groupId);
					
					// 关闭当前窗口
					this->close();
				} else {
					// 其他成员退出，只刷新成员列表UI（本地列表已更新，只需更新UI）
					qDebug() << "检测到成员退出群聊，更新成员列表UI，群组ID:" << groupId << "，退出用户ID:" << leftUserId;
					
					// 更新QGroupInfo的UI（使用更新后的成员列表）
					if (m_groupInfo) {
						m_groupInfo->InitGroupMember(groupId, m_groupMemberInfo);
					}
					
					// 可选：重新从服务器获取成员列表以确保数据同步
					if (m_httpHandler && !m_unique_group_id.isEmpty()) {
						QUrl url("http://47.100.126.194:5000/groups/members");
						QUrlQuery query;
						query.addQueryItem("group_id", m_unique_group_id);
						url.setQuery(query);
						m_httpHandler->get(url.toString());
					}
				}
			}
		});
		
		// 连接群聊解散信号，当群聊被解散时关闭窗口并刷新父窗口
		connect(m_groupInfo, &QGroupInfo::membersRefreshed, this, [this](const QString& groupId) {
			// 如果刷新的是当前群组
			if (groupId == m_unique_group_id) {
				qDebug() << "收到成员列表刷新信号，刷新成员列表，群组ID:" << groupId;
				refreshMemberList(groupId);
			}
		});
		
		connect(m_groupInfo, &QGroupInfo::groupDismissed, this, [this](const QString& groupId) {
			// 如果解散的是当前群组
			if (groupId == m_unique_group_id) {
				qDebug() << "群聊被解散，关闭群聊窗口，群组ID:" << groupId;
				
				// 发出信号通知父窗口刷新群列表
				emit this->groupLeft(groupId);
				
				// 关闭当前窗口
				this->close();
			}
		});
		
		// 连接新成员入群信号，自动刷新成员列表
		// GroupNotifyDialog是单例，通过静态方法获取实例并连接信号
		// 使用QTimer延迟连接，确保GroupNotifyDialog已经创建
		QTimer::singleShot(500, this, [this]() {
			// 通过GroupNotifyDialog的静态方法获取实例
			GroupNotifyDialog* notifyDlg = GroupNotifyDialog::instance();
			if (notifyDlg) {
				connect(notifyDlg, &GroupNotifyDialog::newMemberJoined, this, [this](const QString& groupId, const QStringList& memberIds) {
					// 如果新成员加入的是当前群组，自动刷新成员列表
					if (groupId == m_unique_group_id) {
						qDebug() << "检测到新成员入群，自动刷新成员列表，群组ID:" << groupId << "，新成员:" << memberIds.join(", ");
						refreshMemberList(groupId);
					}
				});
				qDebug() << "已连接GroupNotifyDialog的新成员入群信号";
			} else {
				qWarning() << "GroupNotifyDialog实例不存在，无法连接新成员入群信号";
			}
		});

		// 顶部：头像 + 班级信息 + 功能按钮 + 更多
		QHBoxLayout* topLayout = new QHBoxLayout;
		ClickableLabel* lblAvatar = new ClickableLabel();
		lblAvatar->setFixedSize(50, 50);

		QPixmap avatarPixmap(".\\res\\img\\home.png");
		// 如果需要缩放到控件大小：
		avatarPixmap = avatarPixmap.scaled(lblAvatar->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		lblAvatar->setPixmap(avatarPixmap);
		lblAvatar->setScaledContents(true); // 自动适应 QLabel 尺寸

		lblAvatar->setStyleSheet("background-color: lightgray; border:1px solid gray; text-align:center;");
		connect(lblAvatar, &ClickableLabel::clicked, this, [&, lblAvatar]() {
			QString file = QFileDialog::getOpenFileName(
				this, "选择新头像", "", "Images (*.png *.jpg *.jpeg *.bmp)");
			if (!file.isEmpty()) {
				lblAvatar->setPixmap(QPixmap(file));
				uploadAvatar(file);
			}
			});

		m_lblClass = new QLabel("");
		QPushButton* btnEdit = new QPushButton("✎");
		btnEdit->setFixedSize(24, 24);

	// 班级群功能按钮（普通群不显示）
	QPushButton* btnSeat = new QPushButton("座次表");
	QPushButton* btnCam = new QPushButton("摄像头");
	btnTalk = new QPushButton("按住开始对讲");
	QPushButton* btnMsg = new QPushButton("通知");
	QPushButton* btnTask = new QPushButton("作业");
	QString greenStyle = "background-color: green; color: white; padding: 4px 8px;";
	btnSeat->setStyleSheet(greenStyle);
	btnCam->setStyleSheet(greenStyle);
	btnTalk->setStyleSheet(greenStyle);
	btnMsg->setStyleSheet(greenStyle);
	btnTask->setStyleSheet(greenStyle);
	
	// 保存按钮指针，用于根据群组类型显示/隐藏
	m_btnSeat = btnSeat;
	m_btnCam = btnCam;
	m_btnMsg = btnMsg;
	m_btnTask = btnTask;

		QPushButton* btnMore = new QPushButton("...");
		btnMore->setFixedSize(48, 24);
		btnMore->setText("...");
		btnMore->setStyleSheet(
			"QPushButton {"
			"background-color: transparent;"
			"color: black;"
			"font-weight: bold;"
			"font-size: 16px;"
			"border: none;"
			"}"
			"QPushButton:hover {"
			"color: black;"
			"background-color: transparent;"
			"}"
		);

		connect(btnMore, &QPushButton::clicked, this, [=]() {
			if (m_groupInfo && m_groupInfo->isHidden())
			{
				// 使用 ScheduleDialog 的成员列表数据来初始化 QGroupInfo 的好友列表
				if (!m_groupMemberInfo.isEmpty() && !m_unique_group_id.isEmpty()) {
					m_groupInfo->InitGroupMember(m_unique_group_id, m_groupMemberInfo);
				} else {
					// 如果数据为空，尝试使用无参数版本（使用 QGroupInfo 自己的数据）
					m_groupInfo->InitGroupMember();
				}
				m_groupInfo->show();
			}
			else if (m_groupInfo && !m_groupInfo->isHidden())
			{
				m_groupInfo->hide();
			}
		});

		topLayout->addWidget(lblAvatar);
		topLayout->addWidget(m_lblClass);
		topLayout->addWidget(btnEdit);
		topLayout->addSpacing(10);
		topLayout->addWidget(btnSeat);
		topLayout->addWidget(btnCam);
		topLayout->addWidget(btnTalk);
		topLayout->addWidget(btnMsg);
		topLayout->addWidget(btnTask);
		topLayout->addStretch();
		topLayout->addWidget(btnMore);
		mainLayout->addLayout(topLayout);
		
		// 连接作业按钮（教师端编辑作业）
		connectHomeworkButton(btnTask);

		// 时间 + 科目行
		QHBoxLayout* timeLayout = new QHBoxLayout;
		m_timeButtonStyle = "background-color: royalblue; color: white; font-size:12px; min-width:40px;";
		m_subjectButtonStyle = "background-color: royalblue; color: white; font-size:12px; min-width:50px;";

		QVBoxLayout* vTimes = new QVBoxLayout;
		m_timeRowLayout = new QHBoxLayout;
		m_subjectRowLayout = new QHBoxLayout;
		m_specialSubjectRowLayout = new QHBoxLayout;
		vTimes->addLayout(m_timeRowLayout);
		vTimes->addLayout(m_subjectRowLayout);
		QHBoxLayout* specialRowsLayout = new QHBoxLayout;
		specialRowsLayout->setSpacing(4);
		specialRowsLayout->addLayout(m_specialSubjectRowLayout);
		specialRowsLayout->addStretch();
		vTimes->addLayout(specialRowsLayout);

		auto dailySchedule = buildDefaultDailySchedule();
		QMap<QString, QString> initialHighlights = extractHighlightTimes(dailySchedule.first, dailySchedule.second);
		ensureDailyScheduleButtons(dailySchedule.first.size());
		applyDailyScheduleToButtons(dailySchedule.first, dailySchedule.second, initialHighlights);
		updateSpecialSubjects(initialHighlights);
		// 包一层方便加边框
		QFrame* frameTimes = new QFrame;
		frameTimes->setLayout(vTimes);
		frameTimes->setFrameShape(QFrame::StyledPanel);
		mainLayout->addWidget(frameTimes);

		// 红色分隔线与时间箭头
		QFrame* line = new QFrame;
		line->setFrameShape(QFrame::HLine);
		line->setStyleSheet("color: red; border: 1px solid red;");
		mainLayout->addWidget(line);

		QHBoxLayout* timeIndicatorLayout = new QHBoxLayout;
		timeIndicatorLayout->setSpacing(8);
		QLabel* lblArrow = new QLabel("↓");
		lblArrow->setStyleSheet("color: white; font-weight: bold;");
		QLabel* lblTime = new QLabel("12:10");
		lblTime->setAlignment(Qt::AlignCenter);
		lblTime->setFixedSize(60, 25);
		lblTime->setStyleSheet("background-color: pink; color:red; font-weight:bold;");
		timeIndicatorLayout->addWidget(lblArrow);
		timeIndicatorLayout->addWidget(lblTime);
		timeIndicatorLayout->addStretch();
		mainLayout->addLayout(timeIndicatorLayout);


		// ===== 新增中排按钮 =====
		QHBoxLayout* middleBtnLayout = new QHBoxLayout;
		//QString greenStyle = "background-color: green; color: white; padding: 4px 8px;";

		QPushButton* btnRandom = new QPushButton("随机点名");
		QPushButton* btnAnalyse = new QPushButton("分断");
		QPushButton* btnHeatmap = new QPushButton("热力图");
		QPushButton* btnArrange = new QPushButton("排座");
		QPushButton* btnImportSeat = new QPushButton("导入学生信息");
		QPushButton* btnMoreBottom = new QPushButton("...");
		btnMoreBottom->setFixedSize(48, 24);
		btnMoreBottom->setText("...");
		btnMoreBottom->setStyleSheet(
			"QPushButton {"
			"background-color: transparent;"
			"color: black;"
			"font-weight: bold;"
			"font-size: 16px;"
			"border: none;"
			"}"
			"QPushButton:hover {"
			"color: black;"
			"background-color: transparent;"
			"}"
		);

		connect(btnMoreBottom, &QPushButton::clicked, this, [=]() {
			if (customListDlg && customListDlg->isHidden())
			{
				customListDlg->show();
			}
			else if (customListDlg && !customListDlg->isHidden())
			{
				customListDlg->hide();
			}
		});

		btnRandom->setStyleSheet(greenStyle);
		btnAnalyse->setStyleSheet(greenStyle);
		btnHeatmap->setStyleSheet(greenStyle);
		btnArrange->setStyleSheet(greenStyle);
		btnImportSeat->setStyleSheet(greenStyle);

		// 作业展示按钮（班级端快捷按钮）
		QPushButton* btnHomeworkView = new QPushButton("作业");
		btnHomeworkView->setStyleSheet(greenStyle);
		connect(btnHomeworkView, &QPushButton::clicked, this, [this]() {
			showHomeworkViewDialog();
		});
		
		middleBtnLayout->addStretch();
		middleBtnLayout->addWidget(btnRandom);
		middleBtnLayout->addWidget(btnAnalyse);
		middleBtnLayout->addWidget(btnHeatmap);
		middleBtnLayout->addWidget(btnArrange);
		middleBtnLayout->addWidget(btnImportSeat);
		middleBtnLayout->addWidget(btnHomeworkView); // 添加快捷按钮
		middleBtnLayout->addStretch();
		middleBtnLayout->addWidget(btnMoreBottom);

		mainLayout->addLayout(middleBtnLayout);

		// 创建排座对话框
		arrangeSeatDlg = new ArrangeSeatDialog(this);

		// 连接排座按钮点击事件
		connect(btnArrange, &QPushButton::clicked, this, [=]() {
			if (arrangeSeatDlg && arrangeSeatDlg->isHidden()) {
				arrangeSeatDlg->show();
			} else if (arrangeSeatDlg && !arrangeSeatDlg->isHidden()) {
				arrangeSeatDlg->hide();
			} else {
				arrangeSeatDlg = new ArrangeSeatDialog(this);
				arrangeSeatDlg->show();
			}
		});
		
		// 连接随机点名按钮点击事件
		connectRandomCallButton(btnRandom, seatTable);
		
		// 连接导入学生信息按钮点击事件
		connect(btnImportSeat, &QPushButton::clicked, this, [=]() {
			importSeatTable();
		});
		
		// 连接热力图按钮点击事件
		connect(btnHeatmap, &QPushButton::clicked, this, [=]() {
			// 检查是否有学生数据（必须上传期中成绩）
			if (m_students.isEmpty()) {
				QMessageBox::information(this, "提示", "请先上传期中成绩表！");
				return;
			}
			
			// 显示热力图选择对话框
			QDialog* typeDialog = new QDialog(this);
			typeDialog->setWindowTitle("选择热力图类型");
			typeDialog->setModal(true);
			typeDialog->resize(300, 150);
			
			QVBoxLayout* typeLayout = new QVBoxLayout(typeDialog);
			QLabel* lblTitle = new QLabel("请选择热力图类型：");
			typeLayout->addWidget(lblTitle);
			
			QPushButton* btnSegment = new QPushButton("分段图1（每一段一种颜色）");
			QPushButton* btnGradient = new QPushButton("热力图2（颜色渐变）");
			QPushButton* btnCancel = new QPushButton("取消");
			
			typeLayout->addWidget(btnSegment);
			typeLayout->addWidget(btnGradient);
			typeLayout->addWidget(btnCancel);
			
			connect(btnSegment, &QPushButton::clicked, typeDialog, [=]() {
				typeDialog->accept();
				this->showSegmentDialog();
			});
			
			connect(btnGradient, &QPushButton::clicked, typeDialog, [=]() {
				typeDialog->accept();
				this->showGradientHeatmap();
			});
			
			connect(btnCancel, &QPushButton::clicked, typeDialog, &QDialog::reject);
			
			typeDialog->exec();
			typeDialog->deleteLater();
		});

		// ===== 讲台区域 =====
		QHBoxLayout* podiumLayout = new QHBoxLayout;
		QPushButton* btnPodium = new QPushButton("讲台");
		btnPodium->setStyleSheet(greenStyle);
		btnPodium->setFixedHeight(30);
		btnPodium->setFixedWidth(80);
		podiumLayout->addStretch();
		podiumLayout->addWidget(btnPodium);
		podiumLayout->addStretch();
		mainLayout->addLayout(podiumLayout);


		// 座位表格区域 - 重新设计座位布局
		// 第1行：4个座位（过道两侧各2个）
		// 第2-8行：每行8个座位（4个数据块，每个2列，中间有3个过道）
		// 总共60个座位
		seatTable = new QTableWidget(8, 11); // 8行，11列（包含过道列）
		seatTable->horizontalHeader()->setVisible(false);
		seatTable->verticalHeader()->setVisible(false);
		seatTable->setStyleSheet(
			"QTableWidget { "
			"background-color: #5C5C5C; "
			"gridline-color: #5C5C5C; "
			"border: 1px solid #5C5C5C; "
			"}"
			"QTableWidget::item { "
			"background-color: transparent; "
			"padding: 5px; "
			"}"
		);
		
		// 设置列宽和行高（宽度扩大15%，高度扩大7%）
		for (int col = 0; col < 11; ++col) {
			seatTable->setColumnWidth(col, 105); // 列宽：100 × 1.15 = 115
		}
		for (int row = 0; row < 8; ++row) {
			seatTable->setRowHeight(row, 59); // 行高：55 × 1.07 = 59
		}
		
		// 初始化所有单元格，为每个单元格创建按钮
		for (int row = 0; row < 8; ++row) {
			for (int col = 0; col < 11; ++col) {
				QPushButton* btn = new QPushButton("");
				btn->setStyleSheet(
					"QPushButton { "
					"background-color: #dc3545; "
					"color: white; "
					"border: 1px solid #ccc; "
					"border-radius: 4px; "
					"padding: 5px; "
					"font-size: 12px; "
					"}"
					"QPushButton:hover { "
					"background-color: #c82333; "
					"}"
					"QPushButton:pressed { "
					"background-color: #bd2130; "
					"}"
				);
				btn->setProperty("row", row);
				btn->setProperty("col", col);
				
				// 连接按钮点击事件
				connect(btn, &QPushButton::clicked, this, [=]() {
					qDebug() << "座位按钮被点击: 行" << row << "列" << col;
				});
				
				seatTable->setCellWidget(row, col, btn);
			}
		}
		
		// 第1行布局：6个座位（讲台两侧）
		// 左侧2个座位：列0-1，列3（讲台左边），列7（讲台右边），右侧2个座位：列9-10
		// 设置第1行左侧2个座位（列0-1）
		for (int col = 0; col < 2; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, col));
			if (btn) {
				btn->setProperty("isSeat", true);
			}
		}
		// 设置第1行列3座位（讲台左边）
		QPushButton* btnCol3 = qobject_cast<QPushButton*>(seatTable->cellWidget(0, 3));
		if (btnCol3) {
			btnCol3->setProperty("isSeat", true);
		}
		// 设置第1行列7座位（讲台右边）
		QPushButton* btnCol7 = qobject_cast<QPushButton*>(seatTable->cellWidget(0, 7));
		if (btnCol7) {
			btnCol7->setProperty("isSeat", true);
		}
		// 设置第1行右侧2个座位（列9-10）
		for (int col = 9; col < 11; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, col));
			if (btn) {
				btn->setProperty("isSeat", true);
			}
		}
		seatTable->setSpan(0, 4, 1, 3); // 合并第1行的列2-8作为中央过道
		QPushButton* aisle1Btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, 2));
		if (aisle1Btn) {
			aisle1Btn->setEnabled(false);
			aisle1Btn->setStyleSheet(
				"QPushButton { "
				"background-color: #f0f0f0; "
				"border: 1px solid #ccc; "
				"}"
			);
		}

	    aisle1Btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, 8));
		if (aisle1Btn) {
			aisle1Btn->setEnabled(false);
			aisle1Btn->setStyleSheet(
				"QPushButton { "
				"background-color: #f0f0f0; "
				"border: 1px solid #ccc; "
				"}"
			);
		}
		
		// 第2-8行布局：每行8个座位（4个数据块，每个2列，中间有3个过道）
		// 数据块1：列0-1，过道1：列2，数据块2：列3-4，过道2：列5，数据块3：列6-7，过道3：列8，数据块4：列9-10
		for (int row = 1; row < 8; ++row) {
			// 数据块1座位：列0-1（第一个数据块）
			for (int col = 0; col < 2; ++col) {
				QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
				if (btn) {
					btn->setText(""); // 可以显示座位号或学生姓名
					btn->setProperty("isSeat", true);
				}
			}
			
			// 过道1：列2（第一个走廊）
			QPushButton* aisle1Btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, 2));
			if (aisle1Btn) {
				aisle1Btn->setEnabled(false);
				aisle1Btn->setStyleSheet(
					"QPushButton { "
					"background-color: #f0f0f0; "
					"border: 1px solid #ccc; "
					"}"
				);
			}
			
			// 数据块2座位：列3-4（第二个数据块）
			for (int col = 3; col < 5; ++col) {
				QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
				if (btn) {
					btn->setText("");
					btn->setProperty("isSeat", true);
				}
			}
			
			// 过道2：列5（第二个走廊）
			QPushButton* aisle2Btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, 5));
			if (aisle2Btn) {
				aisle2Btn->setEnabled(false);
				aisle2Btn->setStyleSheet(
					"QPushButton { "
					"background-color: #f0f0f0; "
					"border: 1px solid #ccc; "
					"}"
				);
			}
			
			// 数据块3座位：列6-7（第三个数据块）
			for (int col = 6; col < 8; ++col) {
				QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
				if (btn) {
					btn->setText("");
					btn->setProperty("isSeat", true);
				}
			}
			
			// 过道3：列8（第三个走廊）
			QPushButton* aisle3Btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, 8));
			if (aisle3Btn) {
				aisle3Btn->setEnabled(false);
				aisle3Btn->setStyleSheet(
					"QPushButton { "
					"background-color: #f0f0f0; "
					"border: 1px solid #ccc; "
					"}"
				);
			}
			
			// 数据块4座位：列9-10（第四个数据块）
			for (int col = 9; col < 11; ++col) {
				QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
				if (btn) {
					btn->setText("");
					btn->setProperty("isSeat", true);
				}
			}
		}
		
		// 第1行的座位
		// 左侧2个座位：列0-1
		for (int col = 0; col < 2; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, col));
			if (btn) {
				btn->setText("");
				btn->setProperty("isSeat", true);
			}
		}
		// 右侧2个座位：列9-10
		for (int col = 9; col < 11; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, col));
			if (btn) {
				btn->setText("");
				btn->setProperty("isSeat", true);
			}
		}
		
		// 设置表格选择模式
		seatTable->setSelectionBehavior(QAbstractItemView::SelectItems);
		seatTable->setSelectionMode(QAbstractItemView::SingleSelection);
		seatTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		
		// 设置固定高度，确保能容纳所有行而不出现滚动条
		// 8行 × 59像素 = 472像素，加上边框等额外空间，设置为503像素（扩大7%）
		seatTable->setFixedHeight(503);
		seatTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用垂直滚动条
		
		// 设置固定宽度，确保能容纳所有列而不出现水平滚动条
		// 10列 × 115像素 = 1150像素，加上边框等额外空间，设置为1173像素（扩大15%）
		seatTable->setFixedWidth(1173);
		seatTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
		
		// 创建水平布局使seatTable居中
		QHBoxLayout* seatTableLayout = new QHBoxLayout;
		seatTableLayout->addStretch();
		seatTableLayout->addWidget(seatTable);
		seatTableLayout->addStretch();
		mainLayout->addLayout(seatTableLayout);

		// 红框消息输入栏
		QHBoxLayout* inputLayout = new QHBoxLayout;

		QPushButton* btnVoice = new QPushButton("🔊");
		btnVoice->setFixedSize(30, 30);

		QLineEdit* editMessage = new QLineEdit();
		editMessage->setPlaceholderText("请输入消息...");
		editMessage->setMinimumHeight(30);
		editMessage->setEnabled(false);

		QPushButton* btnEmoji = new QPushButton("😊");
		btnEmoji->setFixedSize(30, 30);

		QPushButton* btnPlus = new QPushButton("➕");
		btnPlus->setFixedSize(30, 30);

		inputLayout->addStretch(1);
		inputLayout->addWidget(btnVoice);
		inputLayout->addWidget(editMessage, 1);
		inputLayout->addWidget(btnEmoji);
		inputLayout->addWidget(btnPlus);
		inputLayout->addStretch(1);

		ClickableWidget* inputWidget = new ClickableWidget();
		inputWidget->setLayout(inputLayout);
		inputWidget->setStyleSheet("background-color: white; border: 1px solid red;");

		// 绑定点击事件
		connect(inputWidget, &ClickableWidget::clicked, this, [=]() {
			qDebug() << "红框区域被点击！";
			// 这里可以弹出输入框、打开聊天功能等
			if (m_chatDlg)
			{
				m_chatDlg->show();
			}
		});

		mainLayout->addWidget(inputWidget);

		// 黄色圆圈数字
		QLabel* lblNum = new QLabel("3");
		lblNum->setAlignment(Qt::AlignCenter);
		lblNum->setFixedSize(30, 30);
		lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
		mainLayout->addWidget(lblNum, 0, Qt::AlignRight);

		//// ===== 音量进度条 =====
		//m_volumeBar = new QProgressBar();
		//m_volumeBar->setRange(0, 100);
		//m_volumeBar->setValue(0);
		//m_volumeBar->setTextVisible(false);
		//m_volumeBar->setStyleSheet(
		//	"QProgressBar { height:8px; border:1px solid gray; background:lightgray; }"
		//	"QProgressBar::chunk { background-color: red; }");
		//mainLayout->addWidget(m_volumeBar);


		//// 底部右下角黄色圆圈数字
		//QLabel* lblNum = new QLabel("3");
		//lblNum->setAlignment(Qt::AlignCenter);
		//lblNum->setFixedSize(30, 30);
		//lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
		//mainLayout->addWidget(lblNum, 0, Qt::AlignRight);

		// 信号与槽连接（C++11 lambda）
		//connect(btnTalk, &QPushButton::clicked, this, &ScheduleDialog::onBtnTalkClicked);
		
		// 初始化 RTMP 推流器（取代 WebRTC）
		m_rtmpStreamer = new RtmpMediaStreamer(this);
		m_rtmpStreamer->setSrsServer(QStringLiteral("47.100.126.194"), 1935);
		connect(m_rtmpStreamer, &RtmpMediaStreamer::logMessage, this, [](const QString& log) {
			qDebug() << "[RTMP]" << log;
		});
		connect(m_rtmpStreamer, &RtmpMediaStreamer::errorOccurred, this, [](const QString& err) {
			qWarning() << "[RTMP][Error]" << err;
		});
		
		// ========== 原有功能已注释 ==========
		// 按下按钮 -> 开始 RTMP 推流（已注释）
		#if 0
		connect(btnTalk, &QPushButton::pressed, this, [=]() {
			pressStartMs = QDateTime::currentMSecsSinceEpoch();
			btnTalk->setStyleSheet("background-color: red; color: white; padding: 4px 8px; font-size:14px;");
			btnTalk->setText("录音中...松开结束");
			qDebug() << "开始对讲（按钮按下）- 使用 RTMP 推流";
			
			// 注释掉原来的音频采集和发送代码（可能后面还会用到）
			// start();
			// 发送开始包（flag=0）
			// QByteArray empty;
			// encodeAndSend(empty, 0);
			
			// 使用 RTMP 协议推流到 SRS，仅采集本地音频
			if (m_unique_group_id.isEmpty() || m_userId.isEmpty()) {
				qWarning() << "RTMP 推流失败：群组ID或用户ID为空";
				btnTalk->setStyleSheet("background-color: green; color: white; padding: 4px 8px; font-size:14px;");
				btnTalk->setText("按住开始对讲");
				return;
			}

			if (!m_rtmpStreamer) {
				qWarning() << "RTMP 推流失败：推流器未初始化";
				btnTalk->setStyleSheet("background-color: green; color: white; padding: 4px 8px; font-size:14px;");
				btnTalk->setText("按住开始对讲");
				return;
			}

			QAudioDeviceInfo audioInfo = QAudioDeviceInfo::defaultInputDevice();
			if (audioInfo.isNull()) {
				qWarning() << "RTMP 推流失败：未检测到可用麦克风";
				btnTalk->setStyleSheet("background-color: green; color: white; padding: 4px 8px; font-size:14px;");
				btnTalk->setText("按住开始对讲");
				return;
			}

			auto sanitizeId = [](const QString& src) -> QString {
				QString safe = src;
				static const QRegularExpression invalidPattern(QStringLiteral("[^A-Za-z0-9_\\-]"));
				return safe.replace(invalidPattern, QStringLiteral("_"));
			};

			QString streamName = QStringLiteral("stream_%1_%2")
				.arg(sanitizeId(m_unique_group_id), sanitizeId(m_userId));
			m_rtmpStreamer->setStreamKey(streamName);
			
			QAudioFormat preferredFormat;
			preferredFormat.setSampleRate(44100);
			preferredFormat.setChannelCount(1);
			preferredFormat.setSampleSize(16);
			preferredFormat.setCodec("audio/pcm");
			preferredFormat.setByteOrder(QAudioFormat::LittleEndian);
			preferredFormat.setSampleType(QAudioFormat::SignedInt);

			if (!startAudioCapture(preferredFormat)) {
				qWarning() << "音频采集启动失败";
				btnTalk->setStyleSheet("background-color: green; color: white; padding: 4px 8px; font-size:14px;");
				btnTalk->setText("按住开始对讲");
				return;
			}

			m_rtmpStreamer->setAudioFormat(m_audioFormat.sampleRate(), m_audioFormat.channelCount());

			if (!m_rtmpStreamer->start()) {
				stopAudioCapture();
				qWarning() << "RTMP 推流启动失败";
				btnTalk->setStyleSheet("background-color: green; color: white; padding: 4px 8px; font-size:14px;");
				btnTalk->setText("按住开始对讲");
				return;
			}
			qDebug() << "RTMP 推流已启动，流名称:" << streamName;
		});

		// 松开按钮 -> 停止 RTMP 推流（已注释）
		connect(btnTalk, &QPushButton::released, this, [=]() {
			qint64 releaseMs = QDateTime::currentMSecsSinceEpoch();
			qint64 duration = releaseMs - pressStartMs;

			// 注释掉原来的音频采集和发送代码（可能后面还会用到）
			// 发送结束包（flag=2）
			// QByteArray empty;
			// encodeAndSend(empty, 2);
			// stopAudioCapture();  // 停止采集

			stopAudioCapture();
			if (m_rtmpStreamer && m_rtmpStreamer->isRunning()) {
				m_rtmpStreamer->stop();
				qDebug() << "RTMP 推流已停止";
			}

			if (duration < 500) {
				qDebug() << "录音时间过短(" << duration << "ms)，丢弃";
			}
			else {
				qDebug() << "录音完成，时长:" << duration << "ms，音视频已通过 RTMP 推送";
			}

			btnTalk->setStyleSheet("background-color: green; color: white; padding: 4px 8px; font-size:14px;");
			btnTalk->setText("按住开始对讲");
		});
		#endif

		// ========== 新功能：点击按钮弹出对讲网页 ==========
		// 点击按钮 -> 弹出对讲界面网页
		connect(btnTalk, &QPushButton::clicked, this, [=]() {
			qDebug() << "点击对讲按钮，准备打开对讲界面";
			openIntercomWebPage();
		});
	}

	void uploadAvatar(QString filePath)
	{
		// ===== 1. 读取头像图片 =====
		QFile file(filePath);  // 本地头像路径
		if (!file.open(QIODevice::ReadOnly)) {
			qDebug() << "Failed to open image file.";
			return;
		}
		QByteArray imageData = file.readAll(); // 二进制数据
		file.close();

		// ===== 2. 图片转 Base64 =====
		QString imageBase64 = QString::fromLatin1(imageData.toBase64());

		// ===== 3. 构造 JSON 数据 =====
		QMap<QString, QString> params;
		params["avatar"] = imageBase64;
		params["unique_group_id"] = m_unique_group_id;
		if (m_taHttpHandler)
		{
			m_taHttpHandler->post(QString("http://47.100.126.194:5000/updateGroupInfo"), params);
		}
	}

	void InitWebSocket()
	{
		if (m_chatDlg)
		{
			m_chatDlg->InitWebSocket();
		}
	}

	void setPrepareClassHistory(const QJsonArray& history);

	void setNoticeMsg(QList<Notification> listNoticeMsg)
	{
		if (m_chatDlg)
		{
			m_chatDlg->setNoticeMsg(listNoticeMsg);
		}
	}

signals:
	void groupLeft(const QString& groupId); // 群聊退出信号，通知父窗口刷新群列表

public:
	// 排座功能：根据学生数据自动排座
	// method: "随机排座", "正序", "倒序", "2人组排座", "4人组排座", "6人组排座"
	void arrangeSeats(const QList<StudentInfo>& students, const QString& method = "随机排座");
	void importSeatTable(); // 导入座位表格
	void importSeatFromExcel(const QString& filePath); // 从Excel导入座位表
	void importSeatFromCsv(const QString& filePath); // 从CSV导入座位表
	void fillSeatTableFromData(const QList<QStringList>& dataRows); // 将数据填充到座位表
	void uploadSeatTableToServer(); // 上传座位表到服务器
	void fetchSeatArrangementFromServer(); // 从服务器获取座位表
	void fetchCourseScheduleForDailyView();
	
	// 热力图相关方法
	void showSegmentDialog(); // 显示分段区间设置对话框
	void showGradientHeatmap(); // 显示渐变热力图
	void setSegments(const QList<struct SegmentRange>& segments); // 设置分段区间
	
	void InitData(QString groupName, QString unique_group_id, QString classid, bool iGroupOwner, bool isClassGroup = true)
	{
		m_groupName = groupName;
		m_unique_group_id = unique_group_id;
		m_classid = classid;
		m_iGroupOwner = iGroupOwner;
		m_isClassGroup = isClassGroup; // 保存群组类型
		
		// 根据群组类型显示/隐藏班级群功能按钮
		if (m_btnSeat) m_btnSeat->setVisible(isClassGroup);
		if (m_btnCam) m_btnCam->setVisible(isClassGroup);
		if (m_btnMsg) m_btnMsg->setVisible(isClassGroup);
		if (m_btnTask) m_btnTask->setVisible(isClassGroup);
		// btnTalk 对讲功能也只在班级群显示
		if (btnTalk) btnTalk->setVisible(isClassGroup);
		
		if (m_lblClass)
		{
			m_lblClass->setText(groupName);
		}
		m_chatDlg->InitData(m_unique_group_id, iGroupOwner);
		UserInfo userInfo = CommonInfo::GetData();
		m_userId = userInfo.teacher_unique_id;
		m_userName = userInfo.strName;

		if (m_groupInfo)
		{
			m_groupInfo->initData(groupName, unique_group_id, classid);
			
			// 优先使用REST API获取群成员列表（从腾讯云IM直接获取，数据更准确）
			// 如果REST API失败，可以回退到使用自己的服务器接口
			//m_groupInfo->fetchGroupMemberListFromREST(unique_group_id);
		}

		// 备用方案：调用自己的服务器接口获取群组成员列表
		// 注意：如果使用REST API成功，这个接口可以作为数据同步的备用方案
		if (m_httpHandler && !unique_group_id.isEmpty())
		{
			// 使用QUrl和QUrlQuery来正确编码URL参数（特别是#等特殊字符）
			QUrl url("http://47.100.126.194:5000/groups/members");
			QUrlQuery query;
			query.addQueryItem("group_id", unique_group_id);
			url.setQuery(query);
			m_httpHandler->get(url.toString());
		}
		
		// 从服务器获取成绩表数据（如果是班级群）
		// 初始化窗口时先不调用此接口
		/*
		if (m_httpHandler && isClassGroup && !classid.isEmpty())
		{
			// 根据当前日期计算学期
			QDate currentDate = QDate::currentDate();
			int year = currentDate.year();
			int month = currentDate.month();
			
			QString term;
			// 9月-1月是上学期（第一学期），2月-8月是下学期（第二学期）
			if (month >= 9 || month <= 1) {
				if (month >= 9) {
					term = QString("%1-%2-1").arg(year).arg(year + 1);
				} else {
					term = QString("%1-%2-1").arg(year - 1).arg(year);
				}
			} else {
				term = QString("%1-%2-2").arg(year - 1).arg(year);
			}
			
			// 获取成绩表数据
			QUrl url("http://47.100.126.194:5000/student-scores/get");
			QUrlQuery query;
			query.addQueryItem("class_id", classid);
			query.addQueryItem("exam_name", "期中考试");
			query.addQueryItem("term", term);
			url.setQuery(query);
			m_httpHandler->get(url.toString());
		}
		*/

		// 班级群初始化时自动从服务器拉取座位表
		if (isClassGroup) {
			fetchSeatArrangementFromServer();
			fetchCourseScheduleForDailyView();
		}
	}

	// 刷新成员列表（优先使用REST API从腾讯云IM获取最新成员列表）
	void refreshMemberList(QString groupId)
	{
		m_unique_group_id = groupId;
		
		// 优先使用REST API获取群成员列表
		if (m_groupInfo && !m_unique_group_id.isEmpty())
		{
			m_groupInfo->fetchGroupMemberListFromREST(m_unique_group_id);
		}
		
		// 备用方案：调用自己的服务器接口获取群组成员列表
		if (m_httpHandler && !m_unique_group_id.isEmpty())
		{
			// 使用QUrl和QUrlQuery来正确编码URL参数（特别是#等特殊字符）
			QUrl url("http://47.100.126.194:5000/groups/members");
			QUrlQuery query;
			query.addQueryItem("group_id", m_unique_group_id);
			url.setQuery(query);
			m_httpHandler->get(url.toString());
			qDebug() << "刷新成员列表，群组ID:" << m_unique_group_id;
		}
	}

	bool startAudioCapture(const QAudioFormat& preferredFormat) {
		QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
		if (!info.isNull()) {
			qDebug() << "当前输入设备:" << info.deviceName();
		}

		QAudioFormat fmt = preferredFormat;
		if (!info.isFormatSupported(fmt)) {
			qWarning() << "输入设备不支持请求的格式，使用 nearestFormat";
			fmt = info.nearestFormat(fmt);
		}

		stopAudioCapture();

		audioInput = new QAudioInput(info, fmt, this);
		audioInput->setBufferSize(4096);

		inputDevice = audioInput->start();
		if (!inputDevice) {
			qCritical() << "AudioInput start() 失败";
			delete audioInput;
			audioInput = nullptr;
			return false;
		}

		m_audioFormat = fmt;

		connect(inputDevice, &QIODevice::readyRead, this, &ScheduleDialog::onReadyRead);
		qDebug() << "AudioInput 已启动，采样率:" << fmt.sampleRate() << "声道:" << fmt.channelCount();
		return true;
	}

	void stopAudioCapture() {
		if (inputDevice) {
			inputDevice->disconnect(this);
			inputDevice = nullptr;
		}
		if (audioInput) {
			audioInput->stop();
			delete audioInput;
			audioInput = nullptr;
		}
	}

private slots:
	void onReadyRead() {
		if (!inputDevice) {
			return;
		}
		QByteArray data = inputDevice->readAll();
		if (data.isEmpty()) {
			return;
		}
		if (m_rtmpStreamer && m_rtmpStreamer->isRunning()) {
			m_rtmpStreamer->pushPcm(data);
		}
	}

protected:
	void enterEvent(QEvent* event) override
	{
		QDialog::enterEvent(event);
		if (closeButton)
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
        painter.setBrush(QColor("#5C5C5C"));
        painter.setPen(Qt::NoPen);

        QPainterPath path;
        path.addRoundedRect(rect(), m_cornerRadius, m_cornerRadius);
        painter.drawPath(path);
    }

private:
	bool m_dragging = false; // 是否正在拖动
	QPoint m_dragStartPos; // 拖动起始位置
	QPushButton* closeButton = nullptr; // 关闭按钮
    int m_cornerRadius = 16;

    void updateMask()
    {
        QPainterPath path;
        path.addRoundedRect(rect(), m_cornerRadius, m_cornerRadius);
        setMask(QRegion(path.toFillPolygon().toPolygon()));
    }
	CustomListDialog* customListDlg = NULL;
	QLabel* m_lblClass = NULL;
	QString m_groupName;
	QString m_unique_group_id;
	TAHttpHandler* m_taHttpHandler = NULL;
	ChatDialog* m_chatDlg = NULL;
	TaQTWebSocket* m_pWs = NULL;
	bool m_iGroupOwner = false;
	QString m_classid;
	QAudioInput* audioInput = nullptr;
	QIODevice* inputDevice = nullptr;
	QAudioFormat m_audioFormat;
	QString m_userId;
	QString m_userName;
	QPushButton* btnTalk = NULL;
	// 班级群功能按钮指针（普通群不显示）
	QPushButton* m_btnSeat = nullptr;
	QPushButton* m_btnCam = nullptr;
	QPushButton* m_btnMsg = nullptr;
	QPushButton* m_btnTask = nullptr;
	QHBoxLayout* m_timeRowLayout = nullptr;
	QHBoxLayout* m_subjectRowLayout = nullptr;
	QList<QPushButton*> m_timeButtons;
	QList<QPushButton*> m_subjectButtons;
	QList<QPushButton*> m_specialSubjectButtons;
	QHBoxLayout* m_specialSubjectRowLayout = nullptr;
	QString m_timeButtonStyle;
	QString m_subjectButtonStyle;
	bool m_isClassGroup = true; // 默认为班级群
	QGroupInfo* m_groupInfo;
	TAHttpHandler* m_httpHandler = NULL;
	// 用于记录按下开始时间
	qint64 pressStartMs = 0;
	//QProgressBar* m_volumeBar = nullptr;
	//QFile localRecordFile;
	//bool isLocalRecording = false;
	// RTMP 推流器
	RtmpMediaStreamer* m_rtmpStreamer = nullptr;
	QVector<GroupMemberInfo>  m_groupMemberInfo;
	QTableWidget* seatTable = nullptr; // 座位表格
	ArrangeSeatDialog* arrangeSeatDlg = nullptr; // 排座对话框
	QList<StudentInfo> m_students; // 学生数据
	
	// 热力图相关
	class HeatmapSegmentDialog* heatmapSegmentDlg = nullptr; // 分段区间对话框
	class HeatmapViewDialog* heatmapViewDlg = nullptr; // 热力图显示窗口
	QList<struct SegmentRange> m_segments; // 分段区间列表
	int m_heatmapType = 1; // 1=分段，2=渐变
	
	// 随机点名相关
	class RandomCallDialog* randomCallDlg = nullptr; // 随机点名对话框
	void connectRandomCallButton(QPushButton* btnRandom, QTableWidget* seatTable); // 连接随机点名按钮
	
	// 作业相关
	class HomeworkEditDialog* homeworkEditDlg = nullptr; // 编辑作业对话框
	class HomeworkViewDialog* homeworkViewDlg = nullptr; // 展示作业对话框
	void connectHomeworkButton(QPushButton* btnTask); // 连接作业按钮（教师端）
	void showHomeworkViewDialog(); // 显示作业展示窗口（班级端）
	
	// 课前准备相关
	void onSubjectButtonClicked(); // 科目按钮点击事件
	void showPrepareClassDialog(const QString& subject, const QString& time = QString()); // 显示课前准备对话框
	void sendPrepareClassContent(const QString& subject, const QString& content, const QString& time = QString()); // 发送课前准备内容
	QString prepareClassCacheKey(const QString& subject, const QString& time) const;
	QMap<QString, QString> m_prepareClassCache; // 课前准备内容缓存（科目|时间 -> 内容）
	QJsonArray m_prepareClassHistoryData; // 课前准备历史原始数据
	
	// 课后评价相关
	void showPostClassEvaluationDialog(const QString& subject); // 显示课后评价对话框
	void sendPostClassEvaluationContent(const QString& subject, const QString& content); // 发送课后评价内容
	
	// 更新座位颜色（根据分段或渐变）
	void updateSeatColors();
	
	void ensureDailyScheduleButtons(int count);
	void applyDailyScheduleToButtons(const QStringList& times, const QStringList& subjects, const QMap<QString, QString>& highlights = QMap<QString, QString>());
	QPair<QStringList, QStringList> buildDefaultDailySchedule() const;
	QStringList defaultSubjectsForWeekday(int weekday) const;
	QStringList defaultTimesForSubjects(const QStringList& subjects) const;
	void applyDefaultDailySchedule();
	QString currentTermString() const;
	QStringList jsonArrayToStringList(const QJsonArray& arr) const;
	bool looksLikeTimeText(const QString& text) const;
	void applyServerDailySchedule(const QStringList& days, const QStringList& times, const QJsonArray& cells);
	QMap<QString, QString> extractHighlightTimes(const QStringList& times, const QStringList& subjects) const;
	void updateSpecialSubjects(const QMap<QString, QString>& highlights);
	
	// 打开对讲界面网页
	void openIntercomWebPage();
	
	// 从文件加载对讲界面HTML模板并替换数据
	QString loadIntercomHtmlTemplate(const QString& escapedMembersJson);
};

// 实现排座方法
inline void ScheduleDialog::arrangeSeats(const QList<StudentInfo>& students, const QString& method)
{
	qDebug() << "arrangeSeats 被调用，学生数量:" << students.size() << "，排座方式:" << method;
	
	if (!seatTable) {
		qDebug() << "错误：seatTable 为空！";
		return;
	}
	
	qDebug() << "seatTable 存在，开始排座...";
	
	m_students = students;
	
	// 获取所有座位按钮（标记为 isSeat 的按钮），按行列顺序排列
	QList<QPushButton*> seatButtons;
	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 11; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
			if (btn && btn->property("isSeat").toBool()) {
				seatButtons.append(btn);
			}
		}
	}
	
	qDebug() << "找到座位按钮数量:" << seatButtons.size();
	
	// 如果学生数量为0，清空所有座位
	if (students.isEmpty()) {
		for (QPushButton* btn : seatButtons) {
			btn->setText("");
			btn->setProperty("studentId", QVariant(""));
			btn->setProperty("studentName", QVariant(""));
		}
		return;
	}
	
	QList<StudentInfo> arrangedStudents;
	
	// 根据排座方式处理学生数据
	if (method == "正序") {
		// 按成绩从高到低排序
		arrangedStudents = students;
		std::sort(arrangedStudents.begin(), arrangedStudents.end(), 
			[](const StudentInfo& a, const StudentInfo& b) {
				return a.score > b.score; // 降序
			});
	} else if (method == "倒序") {
		// 按成绩从低到高排序
		arrangedStudents = students;
		std::sort(arrangedStudents.begin(), arrangedStudents.end(), 
			[](const StudentInfo& a, const StudentInfo& b) {
				return a.score < b.score; // 升序
			});
	} else if (method == "随机排座") {
		// 随机打乱学生顺序
		arrangedStudents = students;
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(arrangedStudents.begin(), arrangedStudents.end(), g);
	} else if (method == "2人组排座") {
		// 2人组：同桌帮扶
		// 按成绩降序排列，然后相邻两人一组
		arrangedStudents = students;
		std::sort(arrangedStudents.begin(), arrangedStudents.end(), 
			[](const StudentInfo& a, const StudentInfo& b) {
				return a.score > b.score;
			});
		// 将相邻两人配对，然后打乱配对顺序
		QList<QList<StudentInfo>> pairs;
		for (int i = 0; i < arrangedStudents.size() - 1; i += 2) {
			QList<StudentInfo> pair;
			pair.append(arrangedStudents[i]);
			pair.append(arrangedStudents[i + 1]);
			pairs.append(pair);
		}
		// 如果学生数量为奇数，最后一个单独一组
		if (arrangedStudents.size() % 2 == 1) {
			QList<StudentInfo> pair;
			pair.append(arrangedStudents.last());
			pairs.append(pair);
		}
		// 打乱配对顺序
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(pairs.begin(), pairs.end(), g);
		// 展开配对
		arrangedStudents.clear();
		for (const QList<StudentInfo>& pair : pairs) {
			arrangedStudents.append(pair);
		}
	} else if (method == "4人组排座" || method == "6人组排座") {
		// 4人组或6人组：按成绩降序排列，然后分组
		int groupSize = (method == "4人组排座") ? 4 : 6;
		arrangedStudents = students;
		std::sort(arrangedStudents.begin(), arrangedStudents.end(), 
			[](const StudentInfo& a, const StudentInfo& b) {
				return a.score > b.score;
			});
		
		// 计算完整组数和剩余人数
		int totalStudents = arrangedStudents.size();
		int fullGroups = totalStudents / groupSize;
		int remaining = totalStudents % groupSize;
		
		// 将学生分成 groupSize 等份
		QList<QList<StudentInfo>> levels;
		levels.reserve(groupSize);
		for (int i = 0; i < groupSize; ++i) {
			levels.append(QList<StudentInfo>());
		}
		
		// 将前 fullGroups * groupSize 个学生分配到等份中
		for (int i = 0; i < fullGroups * groupSize; ++i) {
			int levelIndex = i % groupSize;
			levels[levelIndex].append(arrangedStudents[i]);
		}
		
		// 从每个等份中随机选取一个学生组成一组
		QList<QList<StudentInfo>> groups;
		std::random_device rd;
		std::mt19937 g(rd());
		
		for (int groupIdx = 0; groupIdx < fullGroups; ++groupIdx) {
			QList<StudentInfo> group;
			for (int levelIdx = 0; levelIdx < groupSize; ++levelIdx) {
				if (!levels[levelIdx].isEmpty()) {
					// 随机选择一个学生
					std::uniform_int_distribution<> dis(0, levels[levelIdx].size() - 1);
					int randomIdx = dis(g);
					group.append(levels[levelIdx][randomIdx]);
					levels[levelIdx].removeAt(randomIdx);
				}
			}
			groups.append(group);
		}
		
		// 剩余学生组成最后一组
		if (remaining > 0) {
			QList<StudentInfo> lastGroup;
			for (int i = fullGroups * groupSize; i < totalStudents; ++i) {
				lastGroup.append(arrangedStudents[i]);
			}
			groups.append(lastGroup);
		}
		
		// 打乱组顺序
		std::shuffle(groups.begin(), groups.end(), g);
		
		// 展开组
		arrangedStudents.clear();
		for (const QList<StudentInfo>& group : groups) {
			arrangedStudents.append(group);
		}
	} else {
		// 默认随机排座
		arrangedStudents = students;
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(arrangedStudents.begin(), arrangedStudents.end(), g);
	}
	
	// 将学生分配到座位上（从左到右，从上到下）
	int studentIndex = 0;
	for (QPushButton* btn : seatButtons) {
		if (studentIndex < arrangedStudents.size()) {
			const StudentInfo& student = arrangedStudents[studentIndex];
			btn->setText(student.name); // 显示学生姓名
			btn->setProperty("studentId", QVariant(student.id)); // 设置学号属性
			btn->setProperty("studentName", QVariant(student.name));
			qDebug() << "分配座位:" << student.name << "到按钮";
			studentIndex++;
		} else {
			btn->setText("");
			btn->setProperty("studentId", QVariant(""));
			btn->setProperty("studentName", QVariant(""));
		}
	}
	
	qDebug() << "排座完成，共分配" << studentIndex << "个学生";
}

// 导入座位表格
inline void ScheduleDialog::importSeatTable()
{
	QString fileName = QFileDialog::getOpenFileName(
		this,
		QString::fromUtf8(u8"选择座位表文件"),
		QString(),
		QString::fromUtf8(u8"Excel 文件 (*.xlsx *.xls);;CSV 文件 (*.csv);;所有文件 (*.*)")
	);
	
	if (fileName.isEmpty()) {
		return;
	}
	
	// 判断文件类型
	if (fileName.endsWith(".xlsx", Qt::CaseInsensitive) || fileName.endsWith(".xls", Qt::CaseInsensitive)) {
		importSeatFromExcel(fileName);
	} else if (fileName.endsWith(".csv", Qt::CaseInsensitive)) {
		importSeatFromCsv(fileName);
	} else {
		QMessageBox::warning(this, QString::fromUtf8(u8"提示"), 
			QString::fromUtf8(u8"不支持的文件格式，请选择 .xlsx、.xls 或 .csv 文件"));
	}
}

// 从Excel导入座位表
inline void ScheduleDialog::importSeatFromExcel(const QString& filePath)
{
	using namespace QXlsx;
	
	// 检查文件格式
	bool isXls = filePath.endsWith(".xls", Qt::CaseInsensitive) && 
	             !filePath.endsWith(".xlsx", Qt::CaseInsensitive);
	
	Document xlsx(filePath, this);
	if (!xlsx.isLoadPackage()) {
		QString errorMsg;
		if (isXls) {
			errorMsg = QString::fromUtf8(u8"无法打开 .xls 格式文件。\n\n")
			          + QString::fromUtf8(u8"QXlsx 库主要支持 .xlsx 格式（新版 Excel 格式）。\n")
			          + QString::fromUtf8(u8"请将文件另存为 .xlsx 格式后重试，或使用 Excel 打开文件后另存为 .xlsx 格式。");
		} else {
			errorMsg = QString::fromUtf8(u8"无法打开 Excel 文件：\n") + filePath 
			          + QString::fromUtf8(u8"\n\n请确认文件格式是否正确，或文件是否已损坏。");
		}
		QMessageBox::critical(this, QString::fromUtf8(u8"错误"), errorMsg);
		return;
	}
	
	// 获取第一个工作表
	QStringList sheetNames = xlsx.sheetNames();
	if (sheetNames.isEmpty()) {
		QMessageBox::warning(this, QString::fromUtf8(u8"提示"), 
			QString::fromUtf8(u8"Excel 文件中没有工作表"));
		return;
	}
	
	// 读取数据范围：从第1行、第1列开始，最多20行、15列（扩大范围以包含讲台和所有座位）
	QList<QStringList> allDataRows;
	int startRow = 1; // 从第1行开始
	int startCol = 1; // 从第1列开始
	int maxRows = 20; // 最多20行
	int maxCols = 15; // 最多15列
	
	// 先读取所有数据
	for (int row = startRow; row < startRow + maxRows; ++row) {
		QStringList rowData;
		for (int col = startCol; col < startCol + maxCols; ++col) {
			QVariant cellValue = xlsx.read(row, col);
			QString cellText = cellValue.toString().trimmed();
			rowData.append(cellText);
		}
		allDataRows.append(rowData);
	}
	
	// 查找"讲台"位置（通常在顶部中间）
	// 辅助函数：检查文本是否包含"讲台"（忽略空格）
	auto containsPodium = [](const QString& text) -> bool {
		QString normalizedText = text;
		normalizedText.remove(QRegExp("\\s+")); // 移除所有空格
		return normalizedText.contains(QString::fromUtf8(u8"讲台"));
	};
	
	int podiumRow = -1;
	int podiumStartCol = -1;
	int podiumEndCol = -1;
	QString podiumText = QString::fromUtf8(u8"讲台");
	
	for (int row = 0; row < qMin(5, allDataRows.size()); ++row) {
		for (int col = 0; col < allDataRows[row].size(); ++col) {
			if (containsPodium(allDataRows[row][col])) {
				podiumRow = row;
				// 找到讲台开始的列
				if (podiumStartCol < 0) {
					podiumStartCol = col;
				}
				// 找到讲台结束的列（讲台可能跨越多个单元格）
				podiumEndCol = col;
			}
		}
		if (podiumRow >= 0) break;
	}
	
	qDebug() << "讲台位置: 行" << podiumRow << ", 开始列" << podiumStartCol << ", 结束列" << podiumEndCol;
	
	// 处理数据：过滤掉"讲台"、"走廊"等非学生信息，提取学生数据
	QList<QStringList> dataRows;
	QString corridorText = QString::fromUtf8(u8"走廊");
	
	// 注意：走廊列会在提取完所有数据后统一处理
	
	// 处理讲台行：提取讲台左边和右边的所有单元格（包括空格），但跳过讲台（走廊列稍后统一处理）
	if (podiumRow >= 0 && podiumStartCol >= 0) {
		QStringList podiumRowData;
		// 提取讲台左边的所有单元格（从列0到讲台开始列之前），包括空格
		for (int col = 0; col < podiumStartCol && col < allDataRows[podiumRow].size(); ++col) {
			// 只跳过讲台列，走廊列稍后统一处理
			if (containsPodium(allDataRows[podiumRow][col])) {
				continue; // 跳过讲台
			}
			// 其他都提取（包括空格和走廊），保留位置信息
			QString cellText = allDataRows[podiumRow][col];
			podiumRowData.append(cellText);
		}
		// 提取讲台右边的所有单元格（从讲台结束列之后到行尾），包括空格
		if (podiumEndCol >= 0) {
			for (int col = podiumEndCol + 1; col < allDataRows[podiumRow].size(); ++col) {
				// 只跳过讲台列，走廊列稍后统一处理
				if (containsPodium(allDataRows[podiumRow][col])) {
					continue; // 跳过讲台
				}
				// 其他都提取（包括空格和走廊），保留位置信息
				QString cellText = allDataRows[podiumRow][col];
				podiumRowData.append(cellText);
			}
		}
		// 即使只有空格也添加，这样位置信息就保留了
		if (!podiumRowData.isEmpty()) {
			qDebug() << "讲台行提取到的数据（包括空格和走廊）:" << podiumRowData;
			dataRows.append(podiumRowData);
		}
	}
	
	// 处理讲台下面的行：提取学生信息（走廊列稍后统一处理）
	for (int row = (podiumRow >= 0 ? podiumRow + 1 : 0); row < allDataRows.size(); ++row) {
		QStringList studentRowData;
		bool rowHasData = false;
		for (int col = 0; col < allDataRows[row].size(); ++col) {
			// 只跳过讲台列，走廊列稍后统一处理
			if (containsPodium(allDataRows[row][col])) {
				continue;
			}
			QString cellText = allDataRows[row][col];
			// 检查是否是学生信息格式（包含中文姓名和编号，如"任崇庆13-3"）
			if (!cellText.isEmpty() && cellText.contains(QRegExp("\\d+-\\d+"))) {
				rowHasData = true;
			} else if (cellText.contains(corridorText)) {
				rowHasData = true;
			}
			// 无论是否为空，都添加到列表中，保留列位置信息
			studentRowData.append(cellText);
		}
		// 如果这一行有数据（学生或走廊），才添加到结果中
		if (rowHasData) {
			dataRows.append(studentRowData);
		}
	}
	
	// 统一处理：找出所有行中走廊列的位置，然后去掉这些列
	if (dataRows.size() > 0) {
		// 找出所有包含"走廊"的列索引（检查所有行）
		QSet<int> corridorColIndices;
		for (int row = 0; row < dataRows.size(); ++row) {
			for (int col = 0; col < dataRows[row].size(); ++col) {
				if (dataRows[row][col].contains(corridorText)) {
					corridorColIndices.insert(col);
				}
			}
		}
		
		// 如果有走廊列，从后往前删除（避免索引变化）
		if (!corridorColIndices.isEmpty()) {
			QList<int> sortedCols = corridorColIndices.toList();
			std::sort(sortedCols.begin(), sortedCols.end(), std::greater<int>()); // 降序排列
			
			for (int colIdx : sortedCols) {
				for (int row = 0; row < dataRows.size(); ++row) {
					// 如果某一行当前没有这一列，先补上空字符串保持列对齐
					while (dataRows[row].size() <= colIdx) {
						dataRows[row].append("");
					}
					dataRows[row].removeAt(colIdx);
				}
			}
			qDebug() << "统一去掉走廊列:" << sortedCols;
		}
	}
	
	// 去掉所有行全空格的前几列（从第一列开始，连续去掉所有全空格的列）
	// 只有当所有行的某一列都是空格时，才去掉该列
	if (dataRows.size() > 0) {
		bool removedAny = false;
		while (true) {
			// 检查所有行的当前第一列是否都是空格
			bool currentColAllEmpty = true;
			for (int i = 0; i < dataRows.size(); ++i) {
				// 如果这一行没有数据，或者第一列不是空格，则当前列不全空
				if (dataRows[i].size() == 0 || !dataRows[i][0].trimmed().isEmpty()) {
					currentColAllEmpty = false;
					break;
				}
			}
			// 如果所有行的第一列都是空格，去掉所有行的第一列
			if (currentColAllEmpty) {
				for (int i = 0; i < dataRows.size(); ++i) {
					if (dataRows[i].size() > 0) {
						dataRows[i].removeAt(0);
					}
				}
				removedAny = true;
			} else {
				// 如果当前第一列不全空，停止循环
				break;
			}
		}
		if (removedAny) {
			qDebug() << "去掉所有行全空格的前几列";
		}
	}
	
	// 如果数据为空，尝试从原来的位置读取（兼容旧格式）
	if (dataRows.isEmpty()) {
		qDebug() << "未找到新格式数据，尝试从旧位置读取...";
		// 从第4行、第2列开始读取
		for (int row = 3; row < qMin(19, allDataRows.size()); ++row) {
			QStringList rowData;
			bool rowHasData = false;
			for (int col = 1; col < qMin(13, allDataRows[row].size()); ++col) {
				QString cellText = allDataRows[row][col];
				if (!cellText.isEmpty() && !cellText.contains(corridorText)) {
					rowData.append(cellText);
					rowHasData = true;
				}
			}
			if (rowHasData) {
				dataRows.append(rowData);
			}
		}
	}
	
	// 先清除之前的座位数据
	if (seatTable) {
		for (int row = 0; row < 8; ++row) {
			for (int col = 0; col < 11; ++col) {
				QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
				if (btn && btn->property("isSeat").toBool()) {
					btn->setText("");
					btn->setProperty("studentId", QVariant());
					btn->setProperty("studentName", QVariant());
				}
			}
		}
		seatTable->update();
		seatTable->repaint();
		qDebug() << "已清除之前的座位数据";
	}
	
	// 填充新数据
	fillSeatTableFromData(dataRows);
	
	QMessageBox::information(this, QString::fromUtf8(u8"导入成功"), 
		QString::fromUtf8(u8"座位表导入成功！共导入 %1 行数据").arg(dataRows.size()));
	
	// 导入成功后自动上传到服务器
	uploadSeatTableToServer();
}

// 从CSV导入座位表
inline void ScheduleDialog::importSeatFromCsv(const QString& filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QMessageBox::critical(this, QString::fromUtf8(u8"错误"), 
			QString::fromUtf8(u8"无法打开 CSV 文件：") + filePath);
		return;
	}
	
	QTextStream in(&file);
	in.setCodec("UTF-8");
	
	// 读取所有行（跳过前3行，从第4行开始）
	QList<QStringList> allLines;
	int lineNumber = 0;
	while (!in.atEnd() && lineNumber < 20) { // 读取足够多的行
		QString line = in.readLine();
		lineNumber++;
		
		// 跳过前3行（行号1-3）
		if (lineNumber < 4) {
			continue;
		}
		
		QStringList rowData;
		
		// 简单的CSV解析（按逗号分割，处理引号）
		QString current;
		bool inQuotes = false;
		for (int i = 0; i < line.length(); ++i) {
			QChar ch = line[i];
			if (ch == '"') {
				inQuotes = !inQuotes;
			} else if (ch == ',' && !inQuotes) {
				rowData.append(current.trimmed());
				current.clear();
			} else {
				current += ch;
			}
		}
		rowData.append(current.trimmed());
		
		// 跳过第1列（索引0），从第2列开始（索引1）
		if (rowData.size() > 1) {
			rowData = rowData.mid(1); // 从第2列开始
		} else {
			rowData.clear();
		}
		
		// 最多取12列
		if (rowData.size() > 12) {
			rowData = rowData.mid(0, 12);
		}
		
		allLines.append(rowData);
		
		// 最多读取16行数据（加上前3行，总共不超过20行）
		if (lineNumber - 3 >= 16) {
			break;
		}
	}
	
	file.close();
	
	// 过滤掉全空的行
	QList<QStringList> allDataRows;
	for (const QStringList& rowData : allLines) {
		bool rowHasData = false;
		for (const QString& cell : rowData) {
			if (!cell.isEmpty()) {
				rowHasData = true;
				break;
			}
		}
		if (rowHasData) {
			allDataRows.append(rowData);
		}
	}
	
	// 过滤掉全空的列
	QList<QStringList> dataRows;
	if (!allDataRows.isEmpty()) {
		QList<int> validCols; // 有效列的索引
		int maxColCount = 0;
		for (const QStringList& row : allDataRows) {
			if (row.size() > maxColCount) {
				maxColCount = row.size();
			}
		}
		
		for (int col = 0; col < maxColCount; ++col) {
			bool colHasData = false;
			for (int row = 0; row < allDataRows.size(); ++row) {
				if (col < allDataRows[row].size() && !allDataRows[row][col].isEmpty()) {
					colHasData = true;
					break;
				}
			}
			if (colHasData) {
				validCols.append(col);
			}
		}
		
		// 只保留有效列的数据
		for (int row = 0; row < allDataRows.size(); ++row) {
			QStringList validRowData;
			for (int colIdx : validCols) {
				if (colIdx < allDataRows[row].size()) {
					validRowData.append(allDataRows[row][colIdx]);
				} else {
					validRowData.append("");
				}
			}
			dataRows.append(validRowData);
		}
	}
	
	// 先清除之前的座位数据
	if (seatTable) {
		for (int row = 0; row < 8; ++row) {
			for (int col = 0; col < 11; ++col) {
				QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
				if (btn && btn->property("isSeat").toBool()) {
					btn->setText("");
					btn->setProperty("studentId", QVariant());
					btn->setProperty("studentName", QVariant());
				}
			}
		}
		seatTable->update();
		seatTable->repaint();
		qDebug() << "已清除之前的座位数据（CSV导入）";
	}
	
	// 填充新数据
	fillSeatTableFromData(dataRows);
	
	QMessageBox::information(this, QString::fromUtf8(u8"导入成功"), 
		QString::fromUtf8(u8"座位表导入成功！共导入 %1 行数据").arg(dataRows.size()));
	
	// 导入成功后自动上传到服务器
	uploadSeatTableToServer();
}

// 将数据填充到座位表
inline void ScheduleDialog::fillSeatTableFromData(const QList<QStringList>& dataRows)
{
	if (!seatTable) {
		QMessageBox::warning(this, QString::fromUtf8(u8"错误"), 
			QString::fromUtf8(u8"座位表未初始化"));
		return;
	}
	
	// 清空所有座位
	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 11; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
			if (btn && btn->property("isSeat").toBool()) {
				btn->setText("");
				btn->setProperty("studentId", QVariant());
				btn->setProperty("studentName", QVariant());
			}
		}
	}
	
	if (dataRows.isEmpty()) {
		qDebug() << "导入的数据为空";
		return;
	}
	
	// 新模板格式：每个数据块有两列学生信息，被"走廊"分隔
	// 映射关系：导入的数据映射到seatTable的实际座位位置
	// 模板中讲台在第一行，所以第一行数据（讲台行）应该直接映射到seatTable的第一行
	// 不需要反转行顺序：第一行 -> seatTable第1行，最后一行 -> seatTable最后一行
	// 最多处理8行数据（seatTable只有8行）
	int maxRows = qMin(dataRows.size(), 8);
	
	for (int importRow = 0; importRow < maxRows; ++importRow) {
		// 直接映射：第一行数据 -> seatTable第1行（不需要反转）
		const QStringList& actualRowData = dataRows[importRow];
		// seatTable的行号（0-7）
		int seatTableRow = importRow;
		
		// 解析学生信息：格式为"姓名编号"（如"任崇庆13-3"）
		// 提取姓名和编号
		auto parseStudentInfo = [](const QString& text) -> QPair<QString, QString> {
			QString name, id;
			// 匹配格式：中文姓名 + 数字-数字（如"任崇庆13-3"）
			QRegExp regex("([\\u4e00-\\u9fa5]+)(\\d+-\\d+)");
			if (regex.indexIn(text) >= 0) {
				name = regex.cap(1);
				id = regex.cap(2);
			} else {
				// 如果正则匹配失败，尝试按"-"分割
				QStringList parts = text.split(QRegExp("[\\s-]+"));
				if (parts.size() >= 2) {
					name = parts[0];
					id = parts[1];
				} else {
					name = text;
				}
			}
			return qMakePair(name, id);
		};
		
		if (seatTableRow == 0) {
			// 第1行：讲台旁边的位置（如"王文1-5"）
			// 座位顺序：列0, 列1, 列3（讲台左边）, 列7（讲台右边）, 列9, 列10
			// 新格式：数据包括空格，按顺序填充，遇到空格就跳过该座位位置
			
			// 第1行的所有座位列表（按从左到右的顺序）
			QList<int> seatCols = {0, 1, 3, 7, 9, 10};
			int seatIndex = 0; // 座位索引
			
			// 遍历所有数据，按顺序填充
			for (int i = 0; i < actualRowData.size() && seatIndex < seatCols.size(); ++i) {
				QString cellText = actualRowData[i].trimmed();
				
				// 跳过空格，不填充对应座位位置
				if (cellText.isEmpty()) {
					// 空格不填充，跳过这个座位位置
					seatIndex++;
					continue;
				}
				
				// 检查是否是学生信息格式
				if (cellText.contains(QRegExp("\\d+-\\d+"))) {
					// 填充到当前座位位置
					int seatTableCol = seatCols[seatIndex];
					QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, seatTableCol));
					if (btn && btn->property("isSeat").toBool()) {
						QPair<QString, QString> student = parseStudentInfo(cellText);
						btn->setText(cellText);
						btn->setProperty("studentName", student.first);
						if (!student.second.isEmpty()) {
							btn->setProperty("studentId", student.second);
						}
						btn->update();
					}
					seatIndex++;
				}
			}
		} else {
			// 第2-8行：新格式每个数据块有两列，3个走廊分隔4个数据块
			// 布局：数据块1（列0-1），过道1（列2），数据块2（列3-4），过道2（列5），数据块3（列6-7），过道3（列8），数据块4（列9-10）
			// 新格式数据：每行可能是2列、4列、6列、8列等（多个两列数据块）
			// 按照从左到右的顺序填充：数据块1->数据块2->数据块3->数据块4
			
			int dataIndex = 0;
			
			// 数据块1座位：seatTable列0-1（2个，第一个数据块）
			for (int col = 0; col < 2 && dataIndex < actualRowData.size(); ++col) {
				if (!actualRowData[dataIndex].isEmpty()) {
					QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(seatTableRow, col));
					if (btn && btn->property("isSeat").toBool()) {
						QPair<QString, QString> student = parseStudentInfo(actualRowData[dataIndex]);
						btn->setText(actualRowData[dataIndex]);
						btn->setProperty("studentName", student.first);
						if (!student.second.isEmpty()) {
							btn->setProperty("studentId", student.second);
						}
						btn->update();
					}
				}
				dataIndex++;
			}
			
			// 数据块2座位：seatTable列3-4（2个，第二个数据块，跳过过道列2）
			for (int col = 3; col < 5 && dataIndex < actualRowData.size(); ++col) {
				if (!actualRowData[dataIndex].isEmpty()) {
					QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(seatTableRow, col));
					if (btn && btn->property("isSeat").toBool()) {
						QPair<QString, QString> student = parseStudentInfo(actualRowData[dataIndex]);
						btn->setText(actualRowData[dataIndex]);
						btn->setProperty("studentName", student.first);
						if (!student.second.isEmpty()) {
							btn->setProperty("studentId", student.second);
						}
						btn->update();
					}
				}
				dataIndex++;
			}
			
			// 数据块3座位：seatTable列6-7（2个，第三个数据块，跳过过道列5）
			for (int col = 6; col < 8 && dataIndex < actualRowData.size(); ++col) {
				if (!actualRowData[dataIndex].isEmpty()) {
					QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(seatTableRow, col));
					if (btn && btn->property("isSeat").toBool()) {
						QPair<QString, QString> student = parseStudentInfo(actualRowData[dataIndex]);
						btn->setText(actualRowData[dataIndex]);
						btn->setProperty("studentName", student.first);
						if (!student.second.isEmpty()) {
							btn->setProperty("studentId", student.second);
						}
						btn->update();
					}
				}
				dataIndex++;
			}
			
			// 数据块4座位：seatTable列9-10（2个，第四个数据块，跳过过道列8）
			for (int col = 9; col < 11 && dataIndex < actualRowData.size(); ++col) {
				if (!actualRowData[dataIndex].isEmpty()) {
					QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(seatTableRow, col));
					if (btn && btn->property("isSeat").toBool()) {
						QPair<QString, QString> student = parseStudentInfo(actualRowData[dataIndex]);
						btn->setText(actualRowData[dataIndex]);
						btn->setProperty("studentName", student.first);
						if (!student.second.isEmpty()) {
							btn->setProperty("studentId", student.second);
						}
						btn->update();
					}
				}
				dataIndex++;
			}
		}
	}
	
	// 刷新整个表格以确保界面更新
	seatTable->update();
	seatTable->repaint();
	qDebug() << "座位表数据已刷新，共导入" << dataRows.size() << "行数据";
}

// 从服务器拉取座位表
inline void ScheduleDialog::fetchSeatArrangementFromServer()
{
	if (!seatTable) {
		qWarning() << "seatTable 未初始化，无法获取座位表";
		return;
	}
	
	if (m_classid.isEmpty()) {
		qWarning() << "班级ID为空，无法获取座位表";
		return;
	}
	
	QUrl url("http://47.100.126.194:5000/seat-arrangement");
	QUrlQuery query;
	query.addQueryItem("class_id", m_classid);
	url.setQuery(query);
	
	TAHttpHandler* handler = new TAHttpHandler(this);
	if (!handler) {
		qWarning() << "创建 TAHttpHandler 失败，无法获取座位表";
		return;
	}
	
	auto clearSeats = [this]() {
		for (int row = 0; row < 8; ++row) {
			for (int col = 0; col < 11; ++col) {
				QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
				if (btn && btn->property("isSeat").toBool()) {
					btn->setText("");
					btn->setProperty("studentId", QVariant());
					btn->setProperty("studentName", QVariant());
				}
			}
		}
	};
	
	connect(handler, &TAHttpHandler::success, this, [=](const QString& responseString) {
		handler->deleteLater();
		
		QJsonDocument respDoc = QJsonDocument::fromJson(responseString.toUtf8());
		if (!respDoc.isObject()) {
			qWarning() << "获取座位表失败：响应不是JSON对象";
			return;
		}
		
		QJsonObject obj = respDoc.object();
		int code = obj.value("code").toInt(-1);
		QString message = obj.value("message").toString();
		
		if (code == 200) {
			QJsonObject dataObj = obj.value("data").toObject();
			QJsonArray seatsArray = dataObj.value("seats").toArray();
			
			clearSeats();
			
			int filled = 0;
			for (const QJsonValue& seatValue : seatsArray) {
				if (!seatValue.isObject()) continue;
				QJsonObject seatObj = seatValue.toObject();
				int rowIdx = seatObj.value("row").toInt() - 1;
				int colIdx = seatObj.value("col").toInt() - 1;
				if (rowIdx < 0 || rowIdx >= 8 || colIdx < 0 || colIdx >= 11) continue;
				
				QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(rowIdx, colIdx));
				if (!btn || !btn->property("isSeat").toBool()) continue;
				
				QString seatLabel = seatObj.value("student_name").toString();
				QString name = seatObj.value("name").toString();
				QString studentId = seatObj.value("student_id").toString();
				if (seatLabel.trimmed().isEmpty()) {
					if (!name.isEmpty()) {
						seatLabel = name;
						if (!studentId.isEmpty()) {
							seatLabel += studentId;
						}
					} else if (!studentId.isEmpty()) {
						seatLabel = studentId;
					}
				}
				
				btn->setText(seatLabel);
				btn->setProperty("studentName", name.isEmpty() ? seatLabel : name);
				btn->setProperty("studentId", studentId);
				btn->update();
				filled++;
			}
			
			seatTable->update();
			seatTable->repaint();
			qDebug() << "从服务器加载座位表成功，填充" << filled << "个座位";
		} else if (code == 404) {
			clearSeats();
			qDebug() << "服务器未找到座位信息" << message;
		} else {
			qWarning() << "获取座位表失败，code:" << code << "message:" << message;
			QMessageBox::warning(this, QString::fromUtf8(u8"提示"), 
				message.isEmpty() ? QString::fromUtf8(u8"获取座位表失败") : message);
		}
	});
	
	connect(handler, &TAHttpHandler::failed, this, [=](const QString& error) {
		handler->deleteLater();
		qWarning() << "获取座位表网络错误:" << error;
	});
	
	handler->get(url.toString());
	qDebug() << "正在请求座位表:" << url.toString();
}

// 上传座位表到服务器
inline void ScheduleDialog::uploadSeatTableToServer()
{
	if (!seatTable || m_classid.isEmpty()) {
		QMessageBox::warning(this, QString::fromUtf8(u8"错误"), 
			QString::fromUtf8(u8"座位表未初始化或班级ID为空"));
		return;
	}
	
	if (!m_httpHandler) {
		QMessageBox::warning(this, QString::fromUtf8(u8"错误"), 
			QString::fromUtf8(u8"HTTP处理器未初始化"));
		return;
	}
	
	// 从seatTable提取座位数据
	QJsonArray seatsArray;
	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 11; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
			if (btn && btn->property("isSeat").toBool()) {
				QString studentLabel = btn->text().trimmed();
				if (!studentLabel.isEmpty()) {
					QJsonObject seatObj;
					seatObj["row"] = row + 1; // 行号从1开始
					seatObj["col"] = col + 1; // 列号从1开始
					seatObj["student_name"] = studentLabel;

					QString beforeSlash = studentLabel;
					QString studentId;
					int slashIndex = studentLabel.lastIndexOf('/');
					if (slashIndex >= 0) {
						studentId = studentLabel.mid(slashIndex + 1).trimmed();
						beforeSlash = studentLabel.left(slashIndex).trimmed();
					}

					QString extractedName;
					QStringList parts = beforeSlash.split(QRegExp("[\\s-]+"), Qt::SkipEmptyParts);
					if (!parts.isEmpty()) {
						extractedName = parts.first();
						if (studentId.isEmpty() && parts.size() >= 2) {
							studentId = parts.last();
						}
					} else {
						extractedName = beforeSlash;
					}

					if (!extractedName.isEmpty()) {
						seatObj["name"] = extractedName;
					}
					if (!studentId.isEmpty()) {
						seatObj["student_id"] = studentId;
					}
					
					seatsArray.append(seatObj);
				}
			}
		}
	}
	
	if (seatsArray.isEmpty()) {
		QMessageBox::warning(this, QString::fromUtf8(u8"提示"), 
			QString::fromUtf8(u8"座位表中没有学生数据"));
		return;
	}
	
	// 构造请求JSON
	QJsonObject requestObj;
	requestObj["class_id"] = m_classid;
	requestObj["seats"] = seatsArray;
	
	QJsonDocument doc(requestObj);
	QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
	
	// 发送POST请求
	QString url = "http://47.100.126.194:5000/seat-arrangement/save";
	
	// 使用TAHttpHandler发送请求
	connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
		m_httpHandler->disconnect(this);
		
		QJsonDocument respDoc = QJsonDocument::fromJson(responseString.toUtf8());
		if (respDoc.isObject()) {
			QJsonObject obj = respDoc.object();
			int code = obj.value("code").toInt(-1);
			if (code == 200 || code == 0) {
				// 上传成功后刷新界面座位表
				if (seatTable) {
					seatTable->update();
					seatTable->repaint();
					qDebug() << "座位表界面已刷新";
				}
				QMessageBox::information(this, QString::fromUtf8(u8"上传成功"), 
					QString::fromUtf8(u8"座位表已成功上传到服务器并刷新界面！"));
			} else {
				QString errorMsg = obj.value("message").toString();
				QMessageBox::warning(this, QString::fromUtf8(u8"上传失败"), 
					QString::fromUtf8(u8"服务器返回错误：%1").arg(errorMsg));
			}
		} else {
			// 上传成功后刷新界面座位表
			if (seatTable) {
				seatTable->update();
				seatTable->repaint();
				qDebug() << "座位表界面已刷新";
			}
			QMessageBox::information(this, QString::fromUtf8(u8"上传成功"), 
				QString::fromUtf8(u8"座位表已成功上传到服务器并刷新界面！"));
		}
	});
	
	connect(m_httpHandler, &TAHttpHandler::failed, this, [=](const QString& error) {
		m_httpHandler->disconnect(this);
		QMessageBox::critical(this, QString::fromUtf8(u8"上传失败"), 
			QString::fromUtf8(u8"网络错误：%1").arg(error));
	});
	
	m_httpHandler->post(url, jsonData);
	qDebug() << "开始上传座位表到服务器，班级ID:" << m_classid << "，座位数量:" << seatsArray.size();
}

inline void ScheduleDialog::ensureDailyScheduleButtons(int count)
{
	if (!m_timeRowLayout || !m_subjectRowLayout) {
		return;
	}

	auto createButton = [](const QString& style) -> QPushButton* {
		QPushButton* btn = new QPushButton("");
		btn->setStyleSheet(style);
		return btn;
	};

	while (m_timeButtons.size() < count) {
		QPushButton* btn = createButton(m_timeButtonStyle);
		m_timeRowLayout->addWidget(btn);
		m_timeButtons.append(btn);
	}
	while (m_subjectButtons.size() < count) {
		QPushButton* btn = createButton(m_subjectButtonStyle);
		m_subjectRowLayout->addWidget(btn);
		m_subjectButtons.append(btn);
	}

	for (int i = 0; i < m_timeButtons.size(); ++i) {
		bool visible = i < count;
		m_timeButtons[i]->setVisible(visible);
		m_subjectButtons[i]->setVisible(visible);
		if (!visible) {
			m_timeButtons[i]->setText("");
			m_subjectButtons[i]->setText("");
		}
	}
}

inline void ScheduleDialog::applyDailyScheduleToButtons(const QStringList& times, const QStringList& subjects, const QMap<QString, QString>& highlights)
{
	// 创建数据结构来区分课程表和特殊时间
	// 使用三元组：时间、科目、是否来自课程表
	struct TimeSubjectItem {
		QString time;
		QString subject;
		bool isFromSchedule; // true=来自课程表, false=来自特殊时间
		bool isHighlight;    // 是否为特殊科目
	};

	QList<TimeSubjectItem> items;
	QSet<QString> highlightSubjects;

	// 添加课程表的项目
	int scheduleCount = qMin(times.size(), subjects.size());
	for (int i = 0; i < scheduleCount; ++i) {
		QString subject = subjects[i];
		bool isHighlight = highlights.contains(subject);
		TimeSubjectItem item;
		// 如果课程表中已有特殊科目，使用特殊时间的时间；否则使用课程表的时间
		item.time = isHighlight ? highlights.value(subject) : times[i];
		item.subject = subject;
		item.isFromSchedule = true;
		item.isHighlight = isHighlight;
		items.append(item);
		if (isHighlight) {
			highlightSubjects.insert(subject);
		}
	}

	// 添加不存在的特殊科目
	for (auto it = highlights.constBegin(); it != highlights.constEnd(); ++it) {
		if (!highlightSubjects.contains(it.key())) {
			TimeSubjectItem item;
			item.time = it.value();
			item.subject = it.key();
			item.isFromSchedule = false;
			item.isHighlight = true;
			items.append(item);
			highlightSubjects.insert(it.key());
		}
	}

	// 按时间字符串排序（格式如 "07:20", "08:00", "12:00" 等）
	// 将时间字符串转换为分钟数进行比较，确保正确排序
	auto timeToMinutes = [](const QString& timeStr) -> int {
		if (timeStr.isEmpty()) return 9999; // 空时间排在最后
		QStringList parts = timeStr.split(':');
		if (parts.size() < 2) return 9999;
		bool ok1, ok2;
		int hours = parts[0].toInt(&ok1);
		int minutes = parts[1].toInt(&ok2);
		if (!ok1 || !ok2) return 9999;
		return hours * 60 + minutes;
	};

	std::sort(items.begin(), items.end(), 
		[&timeToMinutes](const TimeSubjectItem& a, const TimeSubjectItem& b) {
			return timeToMinutes(a.time) < timeToMinutes(b.time);
		});

	// 对相同时间的项目进行去重：优先保留课程表的（如果科目不为空），否则保留特殊时间的
	QMap<QString, TimeSubjectItem> timeMap; // 时间 -> 项目

	for (const auto& item : items) {
		if (!timeMap.contains(item.time)) {
			// 新时间，直接添加
			timeMap[item.time] = item;
		} else {
			// 已存在相同时间的项目
			TimeSubjectItem& existing = timeMap[item.time];
			// 规则：优先保留课程表的（如果课程表科目不为空），否则保留特殊时间的
			if (item.isFromSchedule && !item.subject.trimmed().isEmpty()) {
				// 新项目是课程表且科目不为空，优先保留
				existing = item;
			} else if (!item.isFromSchedule && existing.isFromSchedule && existing.subject.trimmed().isEmpty()) {
				// 现有的是课程表但科目为空，新的是特殊时间，保留特殊时间
				existing = item;
			} else if (!item.isFromSchedule && !existing.isFromSchedule) {
				// 两个都是特殊时间，保留科目不为空的
				if (!item.subject.trimmed().isEmpty() && existing.subject.trimmed().isEmpty()) {
					existing = item;
				}
			}
			// 其他情况保持现有的
		}
	}

	// 提取去重后的时间和科目列表（按时间排序）
	QStringList sortedTimes;
	QStringList sortedSubjects;
	for (const auto& item : timeMap.values()) {
		sortedTimes.append(item.time);
		sortedSubjects.append(item.subject);
	}

	// 重新排序（因为QMap的values()可能不保持顺序）
	QList<QPair<QString, QString>> finalPairs;
	for (int i = 0; i < sortedTimes.size(); ++i) {
		finalPairs.append(qMakePair(sortedTimes[i], sortedSubjects[i]));
	}
	std::sort(finalPairs.begin(), finalPairs.end(), 
		[&timeToMinutes](const QPair<QString, QString>& a, const QPair<QString, QString>& b) {
			return timeToMinutes(a.first) < timeToMinutes(b.first);
		});

	sortedTimes.clear();
	sortedSubjects.clear();
	for (const auto& pair : finalPairs) {
		sortedTimes.append(pair.first);
		sortedSubjects.append(pair.second);
	}

	// 显示合并和排序后的数据
	int count = sortedTimes.size();
	if (count == 0) {
		return;
	}
	ensureDailyScheduleButtons(count);

	const QString highlightTimeStyle = "background-color: darkorange; color: white; font-size:12px; min-width:40px;";
	const QString highlightSubjectStyle = "background-color: darkorange; color: white; font-size:12px; min-width:50px;";

	for (int i = 0; i < count; ++i) {
		QString timeText = sortedTimes[i];
		QString subjectText = sortedSubjects[i];
		bool isHighlight = !subjectText.isEmpty() && highlightSubjects.contains(subjectText);
		// 特殊科目只显示科目名，不显示时间（时间已经在上面显示了）
		QString displaySubject = subjectText;

		// 去掉时间字符串中小时部分前面的0，如 "07:20" -> "7:20"
		QString displayTime = timeText;
		if (displayTime.length() >= 5 && displayTime.startsWith("0") && displayTime[1].isDigit()) {
			displayTime = displayTime.mid(1); // 去掉第一个字符（0）
		}

		if (i < m_timeButtons.size()) {
			m_timeButtons[i]->setVisible(true);
			m_timeButtons[i]->setText(displayTime);
			m_timeButtons[i]->setProperty("raw_time", timeText);
			m_timeButtons[i]->setStyleSheet(isHighlight ? highlightTimeStyle : m_timeButtonStyle);
		}
		if (i < m_subjectButtons.size()) {
			m_subjectButtons[i]->setVisible(true);
			m_subjectButtons[i]->setText(displaySubject);
			m_subjectButtons[i]->setStyleSheet(isHighlight ? highlightSubjectStyle : m_subjectButtonStyle);
			// 保存科目名称到按钮属性中，用于点击事件
			m_subjectButtons[i]->setProperty("subject", displaySubject);
			m_subjectButtons[i]->setProperty("time", timeText);
			// 连接点击事件（如果还没有连接）
			m_subjectButtons[i]->disconnect(); // 先断开之前的连接
			connect(m_subjectButtons[i], &QPushButton::clicked, this, &ScheduleDialog::onSubjectButtonClicked);
		}
	}

	for (int i = count; i < m_timeButtons.size(); ++i) {
		m_timeButtons[i]->setVisible(false);
	}
	for (int i = count; i < m_subjectButtons.size(); ++i) {
		m_subjectButtons[i]->setVisible(false);
	}
}

inline QMap<QString, QString> ScheduleDialog::extractHighlightTimes(const QStringList& times, const QStringList& subjects) const
{
	QMap<QString, QString> result;
	result.insert(QStringLiteral("晨读"), QStringLiteral("07:20"));
	result.insert(QStringLiteral("午饭"), QStringLiteral("12:00"));
	result.insert(QStringLiteral("午休"), QStringLiteral("12:40"));
	result.insert(QStringLiteral("晚自习"), QStringLiteral("19:00"));

	int count = qMin(times.size(), subjects.size());
	for (int i = 0; i < count; ++i) {
		const QString subject = subjects[i];
		const QString time = times[i];
		if (result.contains(subject) && !time.isEmpty()) {
			result[subject] = time;
		}
	}
	return result;
}

inline void ScheduleDialog::updateSpecialSubjects(const QMap<QString, QString>& highlights)
{
	if (!m_specialSubjectRowLayout) {
		return;
	}

	while (QLayoutItem* item = m_specialSubjectRowLayout->takeAt(0)) {
		if (item->widget()) {
			item->widget()->deleteLater();
		}
		delete item;
	}

	if (highlights.isEmpty()) {
		m_specialSubjectRowLayout->addStretch();
		return;
	}

	QStringList order = { QStringLiteral("晨读"), QStringLiteral("午饭"), QStringLiteral("午休"), QStringLiteral("晚自习") };
	QStringList keys;
	for (const QString& key : order) {
		if (highlights.contains(key)) {
			keys.append(key);
		}
	}
	for (auto it = highlights.constBegin(); it != highlights.constEnd(); ++it) {
		if (!keys.contains(it.key())) {
			keys.append(it.key());
		}
	}

	bool first = true;
	for (const QString& subject : keys) {
		if (!first) {
			QLabel* sep = new QLabel("   |   ");
			sep->setStyleSheet("color: white; font-size: 12px;");
			m_specialSubjectRowLayout->addWidget(sep);
		}
		QString display = QStringLiteral("%1 %2").arg(subject, highlights.value(subject));
		QLabel* lbl = new QLabel(display);
		lbl->setStyleSheet("color: white; font-size: 12px;");
		m_specialSubjectRowLayout->addWidget(lbl);
		first = false;
	}
	m_specialSubjectRowLayout->addStretch();
}

inline QStringList ScheduleDialog::defaultSubjectsForWeekday(int weekday) const
{
	static const QMap<int, QStringList> subjectsByWeekday = {
		{1, {"晨读","语文","数学","英语","物理","午饭","午休","数学","美术","道法","课服","晚自习",""}},
		{2, {"晨读","语文","数学","英语","化学","午饭","午休","物理","信息","体育","课服","晚自习",""}},
		{3, {"晨读","语文","数学","英语","生物","午饭","午休","数学","历史","地理","课服","晚自习",""}},
		{4, {"晨读","语文","数学","英语","政治","午饭","午休","数学","音乐","美术","课服","晚自习",""}},
		{5, {"晨读","语文","数学","英语","综合实践","午饭","午休","数学","信息","体育","课服","晚自习",""}},
		{6, {"晨读","自习","自习","自习","自习","午饭","午休","兴趣","兴趣","综合","课服","晚自习",""}},
		{7, {"晨读","休息","休息","休息","休息","午饭","午休","休息","休息","休息","课服","晚自习",""}}
	};
	return subjectsByWeekday.value(weekday, subjectsByWeekday.value(1));
}

inline QStringList ScheduleDialog::defaultTimesForSubjects(const QStringList& subjects) const
{
	QStringList slotDefaults = { "07:20","08:00","08:45","09:35","10:25","12:00","12:40","14:00","14:45","15:35","17:00","19:00","20:30" };
	QString morningRead = "07:20";
	QString lunchTime = "12:00";
	QString napTime = "12:40";
	QString tutoringTime = "17:00";
	QString eveningStudyTime = "19:00";

	QStringList times;
	times.reserve(subjects.size());
	for (int i = 0; i < subjects.size(); ++i) {
		QString subject = subjects[i];
		QString slotTime = slotDefaults.value(i, "--:--");
		if (subject == "晨读") {
			slotTime = morningRead;
		} else if (subject == "午饭") {
			slotTime = lunchTime;
		} else if (subject == "午休") {
			slotTime = napTime;
		} else if (subject == "课服") {
			slotTime = tutoringTime;
		} else if (subject == "晚自习") {
			slotTime = eveningStudyTime;
		}
		times.append(slotTime);
	}
	return times;
}

inline QPair<QStringList, QStringList> ScheduleDialog::buildDefaultDailySchedule() const
{
	int weekday = QDate::currentDate().dayOfWeek(); // 1=Monday ... 7=Sunday
	QStringList subs = defaultSubjectsForWeekday(weekday);
	QStringList times = defaultTimesForSubjects(subs);
	return qMakePair(times, subs);
}

inline void ScheduleDialog::applyDefaultDailySchedule()
{
	auto dailySchedule = buildDefaultDailySchedule();
	QMap<QString, QString> highlights = extractHighlightTimes(dailySchedule.first, dailySchedule.second);
	applyDailyScheduleToButtons(dailySchedule.first, dailySchedule.second, highlights);
	updateSpecialSubjects(highlights);
}

inline QString ScheduleDialog::currentTermString() const
{
	QDate today = QDate::currentDate();
	int year = today.year();
	int month = today.month();

	int startYear = year;
	int endYear = year + 1;
	int termIndex = 1;

	if (month >= 9) {
		startYear = year;
		endYear = year + 1;
		termIndex = 1;
	} else if (month >= 3 && month <= 8) {
		startYear = year - 1;
		endYear = year;
		termIndex = 2;
	} else {
		startYear = year - 1;
		endYear = year;
		termIndex = 1;
	}

	return QString("%1-%2-%3").arg(startYear).arg(endYear).arg(termIndex);
}

inline QStringList ScheduleDialog::jsonArrayToStringList(const QJsonArray& arr) const
{
	QStringList list;
	for (const QJsonValue& value : arr) {
		list << value.toString();
	}
	return list;
}

inline bool ScheduleDialog::looksLikeTimeText(const QString& text) const
{
	static QRegExp preciseTimePattern(QStringLiteral("^\\d{1,2}:\\d{2}(?::\\d{2}(?:\\.\\d{1,3})?)?$"));
	if (preciseTimePattern.exactMatch(text))
		return true;

	QRegExp rangePattern(QStringLiteral("^(\\d{1,2}:\\d{2})\\s*-\\s*(\\d{1,2}:\\d{2})$"));
	return rangePattern.exactMatch(text);
}

inline void ScheduleDialog::applyServerDailySchedule(const QStringList& days, const QStringList& times, const QJsonArray& cells)
{
	if (times.isEmpty()) {
		applyDefaultDailySchedule();
		return;
	}

	int weekday = QDate::currentDate().dayOfWeek() - 1; // 0-based
	if (!days.isEmpty()) {
		weekday = qBound(0, weekday, days.size() - 1);
	}

	bool hasLeadingTimeColumn = false;
	for (const QJsonValue& value : cells) {
		if (!value.isObject())
			continue;
		QJsonObject obj = value.toObject();
		int colIndex = obj.value(QStringLiteral("col_index")).toInt(-1);
		QString courseName = obj.value(QStringLiteral("course_name")).toString();
		if (colIndex == 0 && looksLikeTimeText(courseName)) {
			hasLeadingTimeColumn = true;
			break;
		}
	}

	int targetCol = weekday;
	if (hasLeadingTimeColumn) {
		targetCol += 1;
	}

	QStringList subjects;
	subjects.reserve(times.size());
	for (int i = 0; i < times.size(); ++i) {
		subjects.append("");
	}
	for (const QJsonValue& value : cells) {
		if (!value.isObject())
			continue;
		QJsonObject obj = value.toObject();
		int rowIndex = obj.value(QStringLiteral("row_index")).toInt(-1);
		int colIndex = obj.value(QStringLiteral("col_index")).toInt(-1);
		QString courseName = obj.value(QStringLiteral("course_name")).toString();
		if (rowIndex < 0 || rowIndex >= subjects.size())
			continue;
		if (colIndex != targetCol)
			continue;
		if (courseName.isEmpty())
			continue;
		if (looksLikeTimeText(courseName))
			continue;
		subjects[rowIndex] = courseName;
	}

	QMap<QString, QString> highlights = extractHighlightTimes(times, subjects);
	applyDailyScheduleToButtons(times, subjects, highlights);
	updateSpecialSubjects(highlights);
}

inline void ScheduleDialog::fetchCourseScheduleForDailyView()
{
	if (!m_isClassGroup) {
		applyDefaultDailySchedule();
		return;
	}
	if (m_classid.isEmpty()) {
		applyDefaultDailySchedule();
		return;
	}

	const QString term = currentTermString();
	QUrl url(QStringLiteral("http://47.100.126.194:5000/course-schedule"));
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("class_id"), m_classid);
	query.addQueryItem(QStringLiteral("term"), term);
	url.setQuery(query);

	TAHttpHandler* handler = new TAHttpHandler(this);
	if (!handler) {
		applyDefaultDailySchedule();
		return;
	}

	connect(handler, &TAHttpHandler::success, this, [=](const QString& responseString) {
		handler->deleteLater();
		QJsonParseError parseError;
		QJsonDocument doc = QJsonDocument::fromJson(responseString.toUtf8(), &parseError);
		if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
			qWarning() << "课程表解析失败:" << parseError.errorString();
			applyDefaultDailySchedule();
			return;
		}

		QJsonObject obj = doc.object();
		int code = obj.value(QStringLiteral("code")).toInt(-1);
		QString message = obj.value(QStringLiteral("message")).toString();
		if (code != 200) {
			qWarning() << "课程表接口返回错误:" << code << message;
			applyDefaultDailySchedule();
			return;
		}

		QJsonObject dataObj = obj.value(QStringLiteral("data")).toObject();
		QJsonObject scheduleObj = dataObj.value(QStringLiteral("schedule")).toObject();
		QStringList days = jsonArrayToStringList(scheduleObj.value(QStringLiteral("days")).toArray());
		QStringList times = jsonArrayToStringList(scheduleObj.value(QStringLiteral("times")).toArray());
		QJsonArray cells = dataObj.value(QStringLiteral("cells")).toArray();
		if (times.isEmpty() || cells.isEmpty()) {
			applyDefaultDailySchedule();
			return;
		}
		applyServerDailySchedule(days, times, cells);
	});

	connect(handler, &TAHttpHandler::failed, this, [=](const QString& error) {
		handler->deleteLater();
		qWarning() << "课程表接口请求失败:" << error;
		applyDefaultDailySchedule();
	});

	handler->get(url.toString());
	qDebug() << "正在请求课程表:" << url.toString();
}

// 打开对讲界面网页
inline void ScheduleDialog::openIntercomWebPage()
{
	qDebug() << "开始创建对讲界面网页";
	
	// 检查群成员信息是否为空，如果为空则尝试加载
	if (m_groupMemberInfo.isEmpty()) {
		qDebug() << "群成员信息为空，尝试从服务器加载...";
		
		// 如果没有群组ID，无法加载
		if (m_unique_group_id.isEmpty()) {
			QMessageBox::warning(this, "错误", "群组ID为空，无法加载群成员信息！");
			return;
		}
		
		// 从服务器获取群成员列表
		if (m_httpHandler) {
			QUrl url("http://47.100.126.194:5000/groups/members");
			QUrlQuery query;
			query.addQueryItem("group_id", m_unique_group_id);
			url.setQuery(query);
			m_httpHandler->get(url.toString());
			qDebug() << "已请求加载群成员列表，等待响应...";
		}
		
		// 显示提示信息，等待成员列表加载完成
		QMessageBox::information(this, "提示", "正在加载群成员信息，请稍后再试！");
		return;
	}
	
	// 获取当前用户信息
	UserInfo userInfo = CommonInfo::GetData();
	QString currentUserId = m_userId.isEmpty() ? userInfo.teacher_unique_id : m_userId;
	QString currentUserName = m_userName.isEmpty() ? userInfo.strName : m_userName;
	QString currentUserIcon = userInfo.strHeadImagePath.isEmpty() ? 
		(userInfo.avatar.isEmpty() ? "" : userInfo.avatar) : userInfo.strHeadImagePath;
	
	// 如果没有头像路径，使用默认头像URL
	if (currentUserIcon.isEmpty()) {
		currentUserIcon = "https://via.placeholder.com/100";  // 默认头像
	} else {
		// 如果头像路径是本地路径，转换为file://协议
		if (QFile::exists(currentUserIcon)) {
			currentUserIcon = QUrl::fromLocalFile(currentUserIcon).toString();
		}
	}
	
	// 准备成员数据（JSON格式）
	QJsonArray membersArray;
	for (const GroupMemberInfo& member : m_groupMemberInfo) {
		QJsonObject memberObj;
		memberObj["id"] = member.member_id;  // 唯一编号
		memberObj["name"] = member.member_name;  // 名字
		memberObj["role"] = member.member_role;  // 角色（可选）
		membersArray.append(memberObj);
	}
	
	// 准备传递给HTML的数据，包含成员列表和当前用户信息
	QJsonObject dataObj;
	dataObj["members"] = membersArray;
	dataObj["current_user"] = QJsonObject{
		{"id", currentUserId},
		{"name", currentUserName},
		{"icon", currentUserIcon}
	};
	// 添加班级群的唯一编号
	dataObj["group_id"] = m_unique_group_id;
	
	qDebug() << "班级群唯一编号 (m_unique_group_id):" << m_unique_group_id;
	if (m_unique_group_id.isEmpty()) {
		qWarning() << "警告：班级群唯一编号为空！";
	}
	
	QJsonDocument doc(dataObj);
	QString dataJson = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
	
	// 对JSON字符串进行转义，以便安全地插入到JavaScript的单引号字符串中
	QString escapedJson = dataJson;
	// 注意：先转义反斜杠，再转义其他字符
	escapedJson.replace("\\", "\\\\");  // 转义反斜杠（所有反斜杠都需要转义）
	escapedJson.replace("'", "\\'");    // 转义单引号
	escapedJson.replace("\n", "\\n");   // 转义换行
	escapedJson.replace("\r", "\\r");   // 转义回车
	
	qDebug() << "数据JSON:" << dataJson;
	qDebug() << "当前用户ID:" << currentUserId << "，名称:" << currentUserName;
	qDebug() << "班级群唯一编号:" << m_unique_group_id;
	
	// 验证用户ID是否存在
	if (currentUserId.isEmpty()) {
		qWarning() << "警告：当前用户ID为空！";
		qWarning() << "m_userId:" << m_userId;
		qWarning() << "userInfo.teacher_unique_id:" << userInfo.teacher_unique_id;
	}
	
	// 验证群组ID是否存在
	if (m_unique_group_id.isEmpty()) {
		qWarning() << "警告：班级群唯一编号为空，无法创建临时房间！";
		QMessageBox::warning(this, "警告", "班级群唯一编号为空，无法创建临时房间！");
		return;
	}
	
	// 创建临时HTML文件
	QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
	QDir().mkpath(tempDir);
	QString htmlFilePath = tempDir + "/intercom_page_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".html";
	
	// 读取HTML模板文件并替换成员数据
	QString htmlContent = loadIntercomHtmlTemplate(escapedJson);
	if (htmlContent.isEmpty()) {
		qWarning() << "无法加载HTML模板文件";
		QMessageBox::critical(this, "错误", "无法加载对讲界面模板文件！");
		return;
	}
	
	// 写入文件
	QFile htmlFile(htmlFilePath);
	if (!htmlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qWarning() << "无法创建HTML文件:" << htmlFilePath;
		QMessageBox::critical(this, "错误", "无法创建对讲界面文件！");
		return;
	}
	
	QTextStream out(&htmlFile);
	out.setCodec("UTF-8");
	out << htmlContent;
	out.flush(); // 确保数据写入磁盘
	htmlFile.close();
	
	// 验证文件是否成功创建
	QFileInfo fileInfo(htmlFilePath);
	if (!fileInfo.exists() || fileInfo.size() == 0) {
		qWarning() << "HTML文件创建失败或文件大小为0:" << htmlFilePath;
		QMessageBox::critical(this, "错误", QString("无法创建对讲界面文件！\n文件路径: %1").arg(htmlFilePath));
		return;
	}
	
	qDebug() << "HTML文件已创建:" << htmlFilePath << "，文件大小:" << fileInfo.size() << "字节";
	
	// 将路径转换为本地路径格式（Windows下使用反斜杠）
	QString nativePath = QDir::toNativeSeparators(htmlFilePath);
	
	qDebug() << "准备打开HTML文件，路径:" << nativePath;
	
	// 方法1：优先使用QDesktopServices打开（Qt推荐的方式）
	// 使用file://协议格式
	QUrl fileUrl = QUrl::fromLocalFile(nativePath);
	
	qDebug() << "尝试使用QDesktopServices打开，URL:" << fileUrl.toString();
	bool opened = QDesktopServices::openUrl(fileUrl);
	
	if (opened) {
		qDebug() << "成功使用QDesktopServices打开HTML文件";
		return;
	}
	
	qWarning() << "QDesktopServices返回false，尝试其他方法";
	
	// 方法2：在Windows上使用explorer直接打开文件（最可靠）
	#ifdef _WIN32
		qDebug() << "尝试使用explorer打开文件:" << nativePath;
		if (QProcess::startDetached("explorer", QStringList() << nativePath)) {
			qDebug() << "成功使用explorer打开HTML文件";
			return;
		}
		
		qWarning() << "explorer打开失败，尝试使用cmd start命令";
		
		// 方法3：使用cmd start命令
		// 注意：start后的第一个参数是窗口标题（可以为空），第二个参数是文件路径
		QStringList cmdArgs;
		cmdArgs << "/c" << "start" << "" << nativePath;
		
		qDebug() << "执行命令: cmd" << cmdArgs.join(" ");
		qDebug() << "完整命令: cmd /c start \"\" \"" << nativePath << "\"";
		if (QProcess::startDetached("cmd", cmdArgs)) {
			qDebug() << "成功使用cmd start命令打开HTML文件";
			return;
		}
		
		qWarning() << "cmd start命令失败，尝试使用rundll32";
		
		// 方法4：使用rundll32调用shell32.dll（最后的备用方案）
		QStringList rundllArgs;
		rundllArgs << "shell32.dll,ShellExec_RunDLL" << nativePath;
		if (QProcess::startDetached("rundll32.exe", rundllArgs)) {
			qDebug() << "成功使用rundll32打开HTML文件";
			return;
		}
	#endif
	
	// 方法4：如果都失败了，显示错误信息并显示文件路径
	qWarning() << "所有打开方法都失败";
	QMessageBox::critical(this, "错误", 
		QString("无法自动打开对讲界面！\n\n文件已创建在:\n%1\n\n请手动在浏览器中打开该文件。").arg(nativePath));
}

	// 从文件加载对讲界面HTML模板并替换数据
	inline QString ScheduleDialog::loadIntercomHtmlTemplate(const QString& escapedMembersJson)
{
	// HTML模板文件路径 - 优先使用应用程序目录下的文件
	QString templatePath;
	
	// 1. 尝试从应用程序目录读取
	QString appDirPath = QCoreApplication::applicationDirPath() + "/res/intercom.html";
	if (QFile::exists(appDirPath)) {
		templatePath = appDirPath;
	}
	// 2. 尝试从源码目录读取（开发环境）
	else {
		QString sourcePath = QCoreApplication::applicationDirPath() + "/../Common/res/intercom.html";
		if (QFile::exists(sourcePath)) {
			templatePath = sourcePath;
		}
		// 3. 尝试从当前工作目录读取
		else {
			QString currentPath = "./Common/res/intercom.html";
			if (QFile::exists(currentPath)) {
				templatePath = currentPath;
			}
		}
	}
	
	// 如果找不到模板文件，返回错误
	if (templatePath.isEmpty() || !QFile::exists(templatePath)) {
		qWarning() << "HTML模板文件不存在:" << templatePath;
		qWarning() << "请确保 intercom.html 文件存在于以下位置之一:";
		qWarning() << "  1. " << QCoreApplication::applicationDirPath() + "/res/intercom.html";
		qWarning() << "  2. " << QCoreApplication::applicationDirPath() + "/../Common/res/intercom.html";
		qWarning() << "  3. ./Common/res/intercom.html";
		return QString(); // 返回空字符串表示失败
	}
	
	qDebug() << "加载HTML模板文件:" << templatePath;
	
	// 读取HTML模板文件
	QFile htmlTemplateFile(templatePath);
	if (!htmlTemplateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning() << "无法打开HTML模板文件:" << templatePath;
		return QString(); // 返回空字符串表示失败
	}
	
	QTextStream in(&htmlTemplateFile);
	in.setCodec("UTF-8");
	QString htmlTemplate = in.readAll();
	htmlTemplateFile.close();
	
	// 替换占位符
	QString html = htmlTemplate.replace("{{DATA_PLACEHOLDER}}", escapedMembersJson);
	
	return html;
}


// 热力图相关方法的实现在 ScheduleDialog_Heatmap.cpp 中

// 科目按钮点击事件：弹出菜单
inline void ScheduleDialog::onSubjectButtonClicked()
{
	QPushButton* btn = qobject_cast<QPushButton*>(sender());
	if (!btn) return;
	
	QString subject = btn->property("subject").toString();
	if (subject.isEmpty()) {
		subject = btn->text();
	}
	
	// 创建弹出菜单
	QMenu menu(this);
	QAction* actionPrepare = menu.addAction(QString::fromUtf8(u8"课前准备"));
	QAction* actionEvaluation = menu.addAction(QString::fromUtf8(u8"课后评价"));
	
	// 显示菜单
	QAction* selectedAction = menu.exec(btn->mapToGlobal(QPoint(0, btn->height())));
	
	if (selectedAction == actionPrepare) {
		// 找到对应的时间按钮
		QString time = btn->property("time").toString();
		if (time.isEmpty()) {
			int subjectIndex = m_subjectButtons.indexOf(btn);
			if (subjectIndex >= 0 && subjectIndex < m_timeButtons.size()) {
				time = m_timeButtons[subjectIndex]->property("raw_time").toString();
				if (time.isEmpty()) {
					time = m_timeButtons[subjectIndex]->text();
				}
			}
		}
		showPrepareClassDialog(subject, time);
	} else if (selectedAction == actionEvaluation) {
		showPostClassEvaluationDialog(subject);
	}
}

// 显示课前准备对话框
inline void ScheduleDialog::showPrepareClassDialog(const QString& subject, const QString& time)
{
	// 创建对话框
	QDialog* dlg = new QDialog(this);
	dlg->setWindowTitle(QString::fromUtf8(u8"课前准备"));
	dlg->setFixedSize(500, 400);
	dlg->setStyleSheet(
		"QDialog { background-color: #2b2b2b; }"
		"QLabel { color: white; font-size: 14px; }"
		"QTextEdit { background-color: #3b3b3b; color: white; border: 1px solid #555; border-radius: 4px; padding: 8px; }"
		"QPushButton { background-color: #4a4a4a; color: white; border: 1px solid #666; border-radius: 4px; padding: 8px 16px; font-size: 14px; }"
		"QPushButton:hover { background-color: #5a5a5a; }"
		"QPushButton:pressed { background-color: #3a3a3a; }"
	);
	
	QVBoxLayout* mainLayout = new QVBoxLayout(dlg);
	mainLayout->setSpacing(15);
	mainLayout->setContentsMargins(20, 20, 20, 20);
	
	// 提示文字
	QLabel* lblPrompt = new QLabel(QString::fromUtf8(u8"请输入课前准备内容"));
	lblPrompt->setStyleSheet("color: white; font-size: 14px;");
	mainLayout->addWidget(lblPrompt);
	
	// 文本输入框
	QTextEdit* textEdit = new QTextEdit(dlg);
	textEdit->setPlaceholderText(QString::fromUtf8(u8"请输入课前准备内容..."));
	textEdit->setMinimumHeight(200);
	const QString cacheKey = prepareClassCacheKey(subject, time);
	if (m_prepareClassCache.contains(cacheKey)) {
		textEdit->setPlainText(m_prepareClassCache.value(cacheKey));
	}
	mainLayout->addWidget(textEdit, 1);
	
	// 按钮布局
	QHBoxLayout* btnLayout = new QHBoxLayout;
	btnLayout->addStretch();
	
	QPushButton* btnCancel = new QPushButton(QString::fromUtf8(u8"取消"));
	btnCancel->setFixedSize(80, 35);
	btnCancel->setStyleSheet(
		"QPushButton { background-color: #4a4a4a; color: white; }"
		"QPushButton:hover { background-color: #5a5a5a; }"
	);
	
	QPushButton* btnConfirm = new QPushButton(QString::fromUtf8(u8"确定"));
	btnConfirm->setFixedSize(80, 35);
	btnConfirm->setStyleSheet(
		"QPushButton { background-color: #0078d4; color: white; }"
		"QPushButton:hover { background-color: #0063b1; }"
	);
	
	btnLayout->addWidget(btnCancel);
	btnLayout->addWidget(btnConfirm);
	mainLayout->addLayout(btnLayout);
	
	// 连接按钮事件
	connect(btnCancel, &QPushButton::clicked, dlg, &QDialog::reject);
	connect(btnConfirm, &QPushButton::clicked, dlg, [=]() {
		QString content = textEdit->toPlainText().trimmed();
		if (content.isEmpty()) {
			QMessageBox::warning(dlg, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"请输入课前准备内容！"));
			return;
		}
		// 保存到缓存
		m_prepareClassCache[cacheKey] = content;
		sendPrepareClassContent(subject, content, time);
		dlg->accept();
	});
	
	// 显示对话框
	dlg->exec();
	dlg->deleteLater();
}

// 发送课前准备内容（通过WebSocket发送到群组）
inline void ScheduleDialog::sendPrepareClassContent(const QString& subject, const QString& content, const QString& time)
{
	if (m_unique_group_id.isEmpty()) {
		QMessageBox::warning(this, QString::fromUtf8(u8"错误"), QString::fromUtf8(u8"群组ID为空，无法发送！"));
		return;
	}
	
	if (!m_pWs) {
		QMessageBox::warning(this, QString::fromUtf8(u8"错误"), QString::fromUtf8(u8"WebSocket未连接，无法发送！"));
		return;
	}
	
	// 构建JSON消息体
	QJsonObject jsonObj;
	jsonObj[QStringLiteral("type")] = QStringLiteral("prepare_class"); // 消息类型：课前准备
	jsonObj[QStringLiteral("class_id")] = m_classid;
	jsonObj[QStringLiteral("subject")] = subject;
	jsonObj[QStringLiteral("content")] = content;
	jsonObj[QStringLiteral("date")] = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
	jsonObj[QStringLiteral("sender_id")] = m_userId;
	jsonObj[QStringLiteral("sender_name")] = m_userName;
	QString schoolId = CommonInfo::GetData().schoolId;
	if (!schoolId.isEmpty()) {
		jsonObj[QStringLiteral("school_id")] = schoolId;
	}
	// 添加时间参数（如果提供）
	if (!time.isEmpty()) {
		jsonObj[QStringLiteral("time")] = time;
	}
	
	QJsonDocument doc(jsonObj);
	QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
	
	// 通过WebSocket发送到群组，格式：to:群组ID:消息内容
	QString message = QString("to:%1:%2").arg(m_unique_group_id, jsonString);
	TaQTWebSocket::sendPrivateMessage(message);
	
	qDebug() << "正在通过WebSocket发送课前准备内容到群组:" << m_unique_group_id;
	qDebug() << "消息内容:" << jsonString;
	
	QMessageBox::information(this, QString::fromUtf8(u8"成功"), QString::fromUtf8(u8"课前准备内容已发送到群组！"));
}

inline QString ScheduleDialog::prepareClassCacheKey(const QString& subject, const QString& time) const
{
	return subject.trimmed() + "|" + time.trimmed();
}

inline void ScheduleDialog::setPrepareClassHistory(const QJsonArray& history)
{
	m_prepareClassHistoryData = history;
	for (const auto& value : history) {
		if (!value.isObject()) {
			continue;
		}
		QJsonObject obj = value.toObject();
		QString subject = obj.value(QStringLiteral("subject")).toString();
		QString time = obj.value(QStringLiteral("time")).toString();
		QString content = obj.value(QStringLiteral("content")).toString();
		if (subject.trimmed().isEmpty() || content.isEmpty()) {
			continue;
		}
		QString key = prepareClassCacheKey(subject, time);
		m_prepareClassCache[key] = content;
	}
}

// 显示课后评价对话框
inline void ScheduleDialog::showPostClassEvaluationDialog(const QString& subject)
{
	// 创建对话框
	QDialog* dlg = new QDialog(this);
	dlg->setWindowTitle(QString::fromUtf8(u8"课后评价"));
	dlg->setFixedSize(500, 400);
	dlg->setStyleSheet(
		"QDialog { background-color: #2b2b2b; }"
		"QLabel { color: white; font-size: 14px; }"
		"QTextEdit { background-color: #3b3b3b; color: white; border: 1px solid #555; border-radius: 4px; padding: 8px; }"
		"QPushButton { background-color: #4a4a4a; color: white; border: 1px solid #666; border-radius: 4px; padding: 8px 16px; font-size: 14px; }"
		"QPushButton:hover { background-color: #5a5a5a; }"
		"QPushButton:pressed { background-color: #3a3a3a; }"
	);
	
	QVBoxLayout* mainLayout = new QVBoxLayout(dlg);
	mainLayout->setSpacing(15);
	mainLayout->setContentsMargins(20, 20, 20, 20);
	
	// 提示文字
	QLabel* lblPrompt = new QLabel(QString::fromUtf8(u8"请输入课后评价内容"));
	lblPrompt->setStyleSheet("color: white; font-size: 14px;");
	mainLayout->addWidget(lblPrompt);
	
	// 文本输入框
	QTextEdit* textEdit = new QTextEdit(dlg);
	textEdit->setPlaceholderText(QString::fromUtf8(u8"请输入课后评价内容..."));
	textEdit->setMinimumHeight(200);
	mainLayout->addWidget(textEdit, 1);
	
	// 按钮布局
	QHBoxLayout* btnLayout = new QHBoxLayout;
	btnLayout->addStretch();
	
	QPushButton* btnCancel = new QPushButton(QString::fromUtf8(u8"取消"));
	btnCancel->setFixedSize(80, 35);
	btnCancel->setStyleSheet(
		"QPushButton { background-color: #4a4a4a; color: white; }"
		"QPushButton:hover { background-color: #5a5a5a; }"
	);
	
	QPushButton* btnConfirm = new QPushButton(QString::fromUtf8(u8"确定"));
	btnConfirm->setFixedSize(80, 35);
	btnConfirm->setStyleSheet(
		"QPushButton { background-color: #0078d4; color: white; }"
		"QPushButton:hover { background-color: #0063b1; }"
	);
	
	btnLayout->addWidget(btnCancel);
	btnLayout->addWidget(btnConfirm);
	mainLayout->addLayout(btnLayout);
	
	// 连接按钮事件
	connect(btnCancel, &QPushButton::clicked, dlg, &QDialog::reject);
	connect(btnConfirm, &QPushButton::clicked, dlg, [=]() {
		QString content = textEdit->toPlainText().trimmed();
		if (content.isEmpty()) {
			QMessageBox::warning(dlg, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"请输入课后评价内容！"));
			return;
		}
		sendPostClassEvaluationContent(subject, content);
		dlg->accept();
	});
	
	// 显示对话框
	dlg->exec();
	dlg->deleteLater();
}

// 发送课后评价内容（通过WebSocket发送到群组）
inline void ScheduleDialog::sendPostClassEvaluationContent(const QString& subject, const QString& content)
{
	if (m_unique_group_id.isEmpty()) {
		QMessageBox::warning(this, QString::fromUtf8(u8"错误"), QString::fromUtf8(u8"群组ID为空，无法发送！"));
		return;
	}
	
	if (!m_pWs) {
		QMessageBox::warning(this, QString::fromUtf8(u8"错误"), QString::fromUtf8(u8"WebSocket未连接，无法发送！"));
		return;
	}
	
	// 构建JSON消息体
	QJsonObject jsonObj;
	jsonObj[QStringLiteral("type")] = QStringLiteral("post_class_evaluation"); // 消息类型：课后评价
	jsonObj[QStringLiteral("class_id")] = m_classid;
	jsonObj[QStringLiteral("subject")] = subject;
	jsonObj[QStringLiteral("content")] = content;
	jsonObj[QStringLiteral("date")] = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
	jsonObj[QStringLiteral("sender_id")] = m_userId;
	jsonObj[QStringLiteral("sender_name")] = m_userName;
	
	QJsonDocument doc(jsonObj);
	QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
	
	// 通过WebSocket发送到群组，格式：to:群组ID:消息内容
	QString message = QString("to:%1:%2").arg(m_unique_group_id, jsonString);
	TaQTWebSocket::sendPrivateMessage(message);
	
	qDebug() << "正在通过WebSocket发送课后评价内容到群组:" << m_unique_group_id;
	qDebug() << "消息内容:" << jsonString;
	
	QMessageBox::information(this, QString::fromUtf8(u8"成功"), QString::fromUtf8(u8"课后评价内容已发送到群组！"));
}
