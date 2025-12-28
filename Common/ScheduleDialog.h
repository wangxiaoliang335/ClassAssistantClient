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
#include <QInputDialog>
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
#include <QMetaObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <qprogressbar.h>
#include <QPoint>
#include <QBrush>
#include <QPair>
#include <QColor>
#include <QUrl>
#include <QUrlQuery>
#include <QDate>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>
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
#include <QMap>
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
#include "StudentAttributeDialog.h"
#include "HomeworkViewDialog.h"
#include "NotificationViewDialog.h"
#include "ScoreHeaderIdStorage.h"
#include "CommentStorage.h"
#include "QXlsx/header/xlsxdocument.h"
#include "QXlsx/header/xlsxworksheet.h"
#include "QXlsx/header/xlsxcell.h"
#include <QRandomGenerator>
#include "QXlsx/header/xlsxglobal.h"
QT_BEGIN_NAMESPACE_XLSX
QT_END_NAMESPACE_XLSX
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
// 独立课前准备编辑对话框
#include "PrepareClassEditDialog.h"
// 前向声明，避免循环依赖
class HeatmapSegmentDialog;
class HeatmapViewDialog;
// SegmentRange 定义在 HeatmapTypes.h 中
#include "HeatmapTypes.h"
#include <random>
#include <algorithm>
#include <exception>
#include <functional>
#include <QScreen>
#include <QGraphicsDropShadowEffect>

// 自定义编辑昵称对话框类 - 无标题栏
class EditNicknameDialog : public QDialog
{
	Q_OBJECT
public:
	explicit EditNicknameDialog(const QString& currentNickname, QWidget* parent = nullptr)
		: QDialog(parent)
	{
		setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
		setModal(true);
		setStyleSheet(
			"QDialog { background-color: #282A2B; color: white; border: 1px solid #5C5C5C; } "
			"QLabel { color: white; font-size: 14px; background-color: transparent; } "
			"QLineEdit { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; padding: 5px; font-size: 14px; } "
			"QPushButton { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; padding: 5px 15px; font-size: 14px; } "
			"QPushButton:hover { background-color: #6C6C6C; } "
			"QPushButton:pressed { background-color: #4C4C4C; }"
		);
		
		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		mainLayout->setContentsMargins(20, 20, 20, 20);
		mainLayout->setSpacing(15);
		
		// 标题
		QLabel* titleLabel = new QLabel("编辑群昵称", this);
		titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
		mainLayout->addWidget(titleLabel);
		
		// 输入框
		m_lineEdit = new QLineEdit(this);
		m_lineEdit->setText(currentNickname);
		m_lineEdit->selectAll(); // 选中所有文本以便快速编辑
		m_lineEdit->setMinimumWidth(300);
		mainLayout->addWidget(m_lineEdit);
		
		// 按钮布局
		QHBoxLayout* buttonLayout = new QHBoxLayout();
		buttonLayout->addStretch();
		
		QPushButton* cancelBtn = new QPushButton("取消", this);
		connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
		buttonLayout->addWidget(cancelBtn);
		
		QPushButton* okBtn = new QPushButton("确定", this);
		okBtn->setStyleSheet("background-color: #4169E1;");
		connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
		connect(m_lineEdit, &QLineEdit::returnPressed, this, &QDialog::accept);
		buttonLayout->addWidget(okBtn);
		
		mainLayout->addLayout(buttonLayout);
		
		// 设置默认焦点到输入框
		m_lineEdit->setFocus();
	}
	
	QString getNickname() const {
		return m_lineEdit->text().trimmed();
	}
	
private:
	QLineEdit* m_lineEdit;
};

// 自定义提示窗口类 - 显示在屏幕右下角
class ToastNotification : public QDialog
{
public:
    explicit ToastNotification(const QString& message, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);
        
        // 设置样式（使用与ScheduleDialog相同的背景色）
        setStyleSheet(
            "QDialog { background-color: transparent; }"
            "QLabel { "
            "background-color: #282A2B; "
            "color: white; "
            "padding: 15px 20px; "
            "border-radius: 8px; "
            "font-size: 14px; "
            "font-weight: bold; "
            "border: 1px solid #5C5C5C; "
            "}"
        );
        
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        
        QLabel* label = new QLabel(message, this);
        label->setAlignment(Qt::AlignCenter);
        label->setWordWrap(true);
        layout->addWidget(label);
        
        // 设置固定大小
        adjustSize();
        setFixedSize(sizeHint().width() + 40, sizeHint().height() + 20);
        
        // 移动到屏幕右下角
        moveToBottomRight();
        
        // 添加阴影效果
        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(15);
        shadow->setColor(QColor(0, 0, 0, 100));
        shadow->setOffset(0, 3);
        label->setGraphicsEffect(shadow);
        
        // 3秒后自动关闭
        QTimer::singleShot(3000, this, &QDialog::close);
    }
    
private:
    void moveToBottomRight()
    {
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            QRect screenGeometry = screen->availableGeometry();
            int x = screenGeometry.right() - width() - 20;
            int y = screenGeometry.bottom() - height() - 20;
            move(x, y);
        }
    }
};

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

// 临时房间信息结构
struct TempRoomInfo {
	QString room_id;
	QString whip_url;
	QString whep_url;
	QString stream_name;
	QString group_id;
	QString owner_id;
	QString owner_name;
	QString owner_icon;
};

// 座位信息结构
struct SeatInfo {
	int row;                    // 行索引（0-based）
	int col;                    // 列索引（0-based）
	QString studentName;        // 学生姓名
	QString studentId;          // 学号
	QString seatLabel;          // 座位标签（显示文本）
	
	SeatInfo() : row(-1), col(-1) {}
	SeatInfo(int r, int c, const QString& name, const QString& id, const QString& label)
		: row(r), col(c), studentName(name), studentId(id), seatLabel(label) {}
};

// 全局临时房间信息存储（群组ID -> 临时房间信息）
// 用于在创建班级群时保存临时房间信息，即使 ScheduleDialog 还没有打开也能保存
class TempRoomStorage {
public:
	static void saveTempRoomInfo(const QString& groupId, const TempRoomInfo& info) {
		s_tempRooms[groupId] = info;
		qDebug() << "已保存临时房间信息到全局存储，群组ID:" << groupId << "，房间ID:" << info.room_id;
	}
	
	static TempRoomInfo getTempRoomInfo(const QString& groupId) {
		return s_tempRooms.value(groupId, TempRoomInfo());
	}
	
	static bool hasTempRoomInfo(const QString& groupId) {
		return s_tempRooms.contains(groupId) && !s_tempRooms[groupId].room_id.isEmpty();
	}
	
	static void removeTempRoomInfo(const QString& groupId) {
		s_tempRooms.remove(groupId);
		qDebug() << "已从全局存储中移除临时房间信息，群组ID:" << groupId;
	}

private:
	static QMap<QString, TempRoomInfo> s_tempRooms;
};


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
        resize(1448, 669);
        m_cornerRadius = 16;
        updateMask();
        setStyleSheet("QDialog { background-color: #282A2B; color: white; border: 1px solid #5C5C5C; font-weight: bold; } "
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
		closeButton->setText("X");
		closeButton->setFixedSize(26, 26);
		closeButton->setStyleSheet(
			"QPushButton { background-color: #666666; color: white; border: none; border-radius: 4px; font-weight: bold; }"
			"QPushButton:hover { background-color: #777777; }"
		);
        closeButton->move(width() - closeButton->width() - 4, 4);
		closeButton->hide();
		connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

		m_classid = classid;

		m_httpHandler = new TAHttpHandler(this);
		m_networkManager = new QNetworkAccessManager(this);
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

                        // 统一的学期（用于本地 CommentStorage/ScoreHeaderIdStorage 缓存 key）
                        // 优先使用服务端返回的 term（例如 data.headers[0].term），避免用本地时间推算不一致
                        QString termForCache = dataObj.value(QStringLiteral("term")).toString();
                        if (termForCache.isEmpty() && dataObj.contains(QStringLiteral("headers")) && dataObj[QStringLiteral("headers")].isArray()) {
                            QJsonArray headersArr = dataObj[QStringLiteral("headers")].toArray();
                            if (!headersArr.isEmpty() && headersArr.first().isObject()) {
                                QJsonObject header0 = headersArr.first().toObject();
                                termForCache = header0.value(QStringLiteral("term")).toString();
                            }
                        }
                        // 兼容 /group-scores 返回：data.header.term
                        if (termForCache.isEmpty() && dataObj.contains(QStringLiteral("header")) && dataObj[QStringLiteral("header")].isObject()) {
                            QJsonObject headerObj = dataObj.value(QStringLiteral("header")).toObject();
                            termForCache = headerObj.value(QStringLiteral("term")).toString();
                        }
                        if (termForCache.isEmpty()) {
                            termForCache = currentTermString();
                        }
						
                        // 处理成绩表数据（/group-scores 或 /student-scores 接口返回）
                        // 注意：两个接口都可能更新 m_students；若并发请求，后返回的会覆盖前者。
                        // 我们在 fetchStudentScoresFromServer 里改为“先 student 后 group”的串行请求，保证最终以 group-scores 为准。
                        bool hasScores = false;
                        bool isGroupScoresResp = false;
                        bool isStudentScoresResp = false;
                        if (dataObj.contains("group_scores") && dataObj["group_scores"].isArray()) {
                            isGroupScoresResp = true;
                            hasScores = true;
                            m_students.clear();
                            QJsonArray groupScores = dataObj["group_scores"].toArray();
                            int idx = 0;
                            for (const auto& g : groupScores) {
                                QJsonObject groupObj = g.toObject();
                                QString groupName = groupObj.value("group_name").toString();
                                double groupTotalScore = groupObj.value("group_total_score").toDouble();
                                QJsonArray studentsArray = groupObj.value("students").toArray();
                                for (const auto& stu : studentsArray) {
                                    QJsonObject scoreObj = stu.toObject();
                                    StudentInfo student;
                                    student.groupName = groupName;
                                    student.groupTotalScore = groupTotalScore;
                                    // 兼容不同字段命名：student_id/studentId/id 以及 student_name/name
                                    student.id = scoreObj.value(QStringLiteral("student_id")).toString();
                                    if (student.id.isEmpty()) student.id = scoreObj.value(QStringLiteral("studentId")).toString();
                                    if (student.id.isEmpty() && scoreObj.contains(QStringLiteral("id")) && scoreObj.value(QStringLiteral("id")).isDouble()) {
                                        student.id = QString::number(scoreObj.value(QStringLiteral("id")).toInt());
                                    }
                                    student.name = scoreObj.value(QStringLiteral("student_name")).toString();
                                    if (student.name.isEmpty()) student.name = scoreObj.value(QStringLiteral("name")).toString();
                                    student.originalIndex = idx++;

                                    // 系统字段列表（需要排除的字段）
                                    QSet<QString> systemFields;
                                    systemFields << "id" << "student_id" << "student_name" << "total_score"
                                                 << "comments" << "group_name" << "group_total_score" << "scores"
                                                 << "scores_json_full" << "field_sources"; // 新增系统字段

                                    bool hasScoresJsonFull = false;
                                    // 解析 scores_json_full（包含所有复合键名的完整数据）
                                    QJsonObject scoresFullObj;
                                    if (scoreObj.contains(QStringLiteral("scores_json_full"))) {
                                        const QJsonValue v = scoreObj.value(QStringLiteral("scores_json_full"));
                                        if (v.isObject()) {
                                            scoresFullObj = v.toObject();
                                        } else if (v.isString()) {
                                            // 兼容后端把 scores_json_full 作为 JSON 字符串返回的情况
                                            QJsonParseError pe;
                                            const QJsonDocument d = QJsonDocument::fromJson(v.toString().toUtf8(), &pe);
                                            if (pe.error == QJsonParseError::NoError && d.isObject()) {
                                                scoresFullObj = d.object();
                                            }
                                        }
                                    }
                                    if (!scoresFullObj.isEmpty()) {
                                        hasScoresJsonFull = true;
                                        for (auto it = scoresFullObj.begin(); it != scoresFullObj.end(); ++it) {
                                            QString compositeKey = it.key(); // 如 "语文_期中成绩单.xlsx"
                                            QJsonValue value = it.value();
                                            if (!value.isNull()) {
                                                double numValue = 0.0;
                                                if (value.isDouble()) {
                                                    numValue = value.toDouble();
                                                } else if (value.isString()) {
                                                    bool ok;
                                                    numValue = value.toString().toDouble(&ok);
                                                    if (!ok) continue;
                                                } else {
                                                    continue;
                                                }
                                                
                                                // 保存到 attributesFull
                                                student.attributesFull[compositeKey] = numValue;
                                                
                                                // 解析复合键名，提取字段名和Excel文件名
                                                int underscorePos = compositeKey.lastIndexOf('_');
                                                if (underscorePos > 0 && underscorePos < compositeKey.length() - 1) {
                                                    QString fieldName = compositeKey.left(underscorePos);
                                                    QString excelFileName = compositeKey.mid(underscorePos + 1);
                                                    
                                                    // 保存到 attributesByExcel
                                                    student.attributesByExcel[excelFileName][fieldName] = numValue;
                                                    
                                                    // 向后兼容：如果 attributes 中还没有该字段，使用第一个值
                                                    if (!student.attributes.contains(fieldName)) {
                                                        student.attributes[fieldName] = numValue;
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    // 解析 field_sources（字段来源详细信息）
                                    // 按需求：优先使用 scores_json_full；仅当没有 scores_json_full 时才回退解析 field_sources
                                    if (!hasScoresJsonFull && scoreObj.contains("field_sources") && scoreObj["field_sources"].isObject()) {
                                        QJsonObject fieldSourcesObj = scoreObj["field_sources"].toObject();
                                        for (auto it = fieldSourcesObj.begin(); it != fieldSourcesObj.end(); ++it) {
                                            QString fieldName = it.key();
                                            QJsonObject fieldSourceObj = it.value().toObject();
                                            
                                            // 获取默认值
                                            if (fieldSourceObj.contains("value")) {
                                                double defaultValue = fieldSourceObj["value"].toDouble();
                                                if (!student.attributes.contains(fieldName)) {
                                                    student.attributes[fieldName] = defaultValue;
                                                }
                                            }
                                            
                                            // 获取所有来源
                                            if (fieldSourceObj.contains("sources") && fieldSourceObj["sources"].isArray()) {
                                                QJsonArray sourcesArray = fieldSourceObj["sources"].toArray();
                                                for (const auto& source : sourcesArray) {
                                                    QJsonObject sourceObj = source.toObject();
                                                    QString excelFileName = sourceObj["excel_filename"].toString();
                                                    double value = sourceObj["value"].toDouble();
                                                    
                                                    // 保存到 attributesByExcel
                                                    student.attributesByExcel[excelFileName][fieldName] = value;
                                                    
                                                    // 保存到 attributesFull
                                                    QString compositeKey = QString("%1_%2").arg(fieldName).arg(excelFileName);
                                                    student.attributesFull[compositeKey] = value;
                                                }
                                            }
                                        }
                                    }

                                    // 动态读取所有属性字段（排除系统字段）- 向后兼容
                                    for (auto it = scoreObj.begin(); it != scoreObj.end(); ++it) {
                                        QString key = it.key();
                                        if (systemFields.contains(key)) {
                                            continue;
                                        }
                                        if (key.endsWith("_comment")) {
                                            QString fieldName = key.left(key.length() - 8);
                                            QString comment = it.value().toString();
                                            if (!comment.isEmpty()) {
                                                student.comments[fieldName] = comment;
                                            }
                                            continue;
                                        }

                                        QJsonValue value = it.value();
                                        if (!value.isNull()) {
                                            if (value.isDouble()) {
                                                // 向后兼容：如果还没有该字段，才添加
                                                if (!student.attributes.contains(key)) {
                                                    student.attributes[key] = value.toDouble();
                                                }
                                            } else if (value.isString()) {
                                                bool ok;
                                                double numValue = value.toString().toDouble(&ok);
                                                if (ok && !student.attributes.contains(key)) {
                                                    student.attributes[key] = numValue;
                                                }
                                            }
                                        }
                                    }

                                    // 解析 scores 对象（如果存在）
                                    if (scoreObj.contains("scores") && scoreObj["scores"].isObject()) {
                                        QJsonObject scoresObj = scoreObj["scores"].toObject();
                                        for (auto it = scoresObj.begin(); it != scoresObj.end(); ++it) {
                                            QString key = it.key();
                                            QJsonValue value = it.value();
                                            if (!value.isNull()) {
                                                if (value.isDouble()) {
                                                    student.attributes[key] = value.toDouble();
                                                } else if (value.isString()) {
                                                    bool ok;
                                                    double numValue = value.toString().toDouble(&ok);
                                                    if (ok) {
                                                        student.attributes[key] = numValue;
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    // 解析 comments（兼容对象或 JSON 字符串）
                                    // 新增：优先解析 comments_json_full（对象/JSON字符串），其 key 为  字段_Excel文件名.xlsx
                                    QJsonObject commentsObj;
                                    bool hasCommentsObj = false;
                                    auto mergeCommentsValue = [&](const QJsonValue& v) {
                                        if (v.isObject()) {
                                            const QJsonObject o = v.toObject();
                                            for (auto it = o.begin(); it != o.end(); ++it) {
                                                commentsObj[it.key()] = it.value();
                                            }
                                            hasCommentsObj = hasCommentsObj || !o.isEmpty();
                                        } else if (v.isString()) {
                                            QJsonParseError pe;
                                            const QJsonDocument d = QJsonDocument::fromJson(v.toString().toUtf8(), &pe);
                                            if (pe.error == QJsonParseError::NoError && d.isObject()) {
                                                const QJsonObject o = d.object();
                                                for (auto it = o.begin(); it != o.end(); ++it) {
                                                    commentsObj[it.key()] = it.value();
                                                }
                                                hasCommentsObj = hasCommentsObj || !o.isEmpty();
                                            }
                                        }
                                    };
                                    if (scoreObj.contains(QStringLiteral("comments_json_full"))) {
                                        mergeCommentsValue(scoreObj.value(QStringLiteral("comments_json_full")));
                                    }
                                    if (scoreObj.contains(QStringLiteral("comments"))) {
                                        mergeCommentsValue(scoreObj.value(QStringLiteral("comments")));
                                    }
                                    if (hasCommentsObj) {
                                        for (auto it = commentsObj.begin(); it != commentsObj.end(); ++it) {
                                            QString key = it.key();
                                            QString comment = it.value().toString();
                                            if (comment.isEmpty()) continue;
                                            
                                            // 判断是否是复合键名（包含下划线和可能的Excel文件名）
                                            int underscorePos = key.lastIndexOf('_');
                                            if (underscorePos > 0 && underscorePos < key.length() - 1) {
                                                // 可能是复合键名，检查是否以 .xlsx 或 .xls 结尾
                                                QString possibleFileName = key.mid(underscorePos + 1);
                                                if (possibleFileName.endsWith(".xlsx") || possibleFileName.endsWith(".xls") || 
                                                    possibleFileName.endsWith(".csv")) {
                                                    // 是复合键名注释
                                                    QString fieldName = key.left(underscorePos);
                                                    QString excelFileName = possibleFileName;
                                                    
                                                    // 保存到 commentsFull
                                                    student.commentsFull[key] = comment;
                                                    
                                                    // 保存到 commentsByExcel
                                                    student.commentsByExcel[excelFileName][fieldName] = comment;
                                                    
                                                    // 向后兼容：如果还没有该字段的注释，使用第一个
                                                    if (!student.comments.contains(fieldName)) {
                                                        student.comments[fieldName] = comment;
                                                    }

                                                    // 同步到全局注释缓存：不再保存复合键 key（字段_Excel文件名）
                                                    // 只保存“纯字段名 + 表格名(excelFileName)”这一份，避免不同表同字段互相覆盖
                                                    CommentStorage::saveComment(m_classid, termForCache, student.id, fieldName, excelFileName, comment);
                                                } else {
                                                    // 普通字段注释
                                                    student.comments[key] = comment;
                                                    CommentStorage::saveComment(
                                                        m_classid, termForCache, student.id,
                                                        key, CommentStorage::inferTableNameFromFieldKey(key), comment);
                                                }
                                            } else {
                                                // 普通字段注释
                                                student.comments[key] = comment;
                                                CommentStorage::saveComment(m_classid, termForCache, student.id, key, QString(), comment);
                                            }
                                        }
                                    }

                                    // 读取总分（total_score）
                                    if (scoreObj.contains("total_score") && !scoreObj["total_score"].isNull()) {
                                        double total = scoreObj["total_score"].toDouble();
                                        student.score = total;
                                        if (!student.attributes.contains("总分")) {
                                            student.attributes["总分"] = total;
                                        }
                                    } else {
                                        // 使用新的辅助函数获取"总分"（优先级：attributesByExcel → attributesFull → attributes）
                                        double total = student.getAttributeValue("总分");
                                        if (total > 0.0) {
                                            student.score = total;
                                        } else {
                                            student.score = 0;
                                        }
                                    }

                                    // 记录小组总分
                                    if (student.groupTotalScore > 0) {
                                        student.attributes["小组总分"] = student.groupTotalScore;
                                    }

                                    m_students.append(student);
                                }
                            }
                            // 缓存 group-scores 结果，避免后续被 student 覆盖/丢失
                            m_studentsGroupScores = m_students;
                            if (customListDlg) {
                                customListDlg->setServerScoreCaches(m_studentsStudentScores, m_studentsGroupScores, termForCache);
                            }
                        } else if (dataObj.contains("student_scores") && dataObj["student_scores"].isArray()) {
                            // /student-scores 接口扁平返回
                            isStudentScoresResp = true;
                            hasScores = true;
                            QJsonArray scoresArray = dataObj["student_scores"].toArray();
                            m_students.clear();
                            
                            for (int i = 0; i < scoresArray.size(); i++)
                            {
                                QJsonObject scoreObj = scoresArray[i].toObject();
                                
                                StudentInfo student;
                                // 兼容不同字段命名：student_id/studentId/id 以及 student_name/name
                                student.id = scoreObj.value(QStringLiteral("student_id")).toString();
                                if (student.id.isEmpty()) student.id = scoreObj.value(QStringLiteral("studentId")).toString();
                                if (student.id.isEmpty() && scoreObj.contains(QStringLiteral("id")) && scoreObj.value(QStringLiteral("id")).isDouble()) {
                                    student.id = QString::number(scoreObj.value(QStringLiteral("id")).toInt());
                                }
                                student.name = scoreObj.value(QStringLiteral("student_name")).toString();
                                if (student.name.isEmpty()) student.name = scoreObj.value(QStringLiteral("name")).toString();
                                student.originalIndex = i;
                                if (scoreObj.contains("group_name")) {
                                    student.groupName = scoreObj.value("group_name").toString();
                                }
                                if (scoreObj.contains("group_total_score")) {
                                    student.groupTotalScore = scoreObj.value("group_total_score").toDouble();
                                }
                                
                                // 系统字段列表（需要排除的字段）
                                QSet<QString> systemFields;
                                systemFields << "id" << "student_id" << "student_name" << "total_score" << "comments"
                                             << "comments_json_full"
                                             << "group_name" << "group_total_score" << "scores"
                                             << "scores_json_full" << "field_sources"; // 新增系统字段

                                bool hasScoresJsonFull = false;
                                // 解析 scores_json_full（包含所有复合键名的完整数据）
                                QJsonObject scoresFullObj;
                                if (scoreObj.contains(QStringLiteral("scores_json_full"))) {
                                    const QJsonValue v = scoreObj.value(QStringLiteral("scores_json_full"));
                                    if (v.isObject()) {
                                        scoresFullObj = v.toObject();
                                    } else if (v.isString()) {
                                        QJsonParseError pe;
                                        const QJsonDocument d = QJsonDocument::fromJson(v.toString().toUtf8(), &pe);
                                        if (pe.error == QJsonParseError::NoError && d.isObject()) {
                                            scoresFullObj = d.object();
                                        }
                                    }
                                }
                                if (!scoresFullObj.isEmpty()) {
                                    hasScoresJsonFull = true;
                                    for (auto it = scoresFullObj.begin(); it != scoresFullObj.end(); ++it) {
                                        QString compositeKey = it.key(); // 如 "语文_期中成绩单.xlsx"
                                        QJsonValue value = it.value();
                                        if (!value.isNull()) {
                                            double numValue = 0.0;
                                            if (value.isDouble()) {
                                                numValue = value.toDouble();
                                            } else if (value.isString()) {
                                                bool ok;
                                                numValue = value.toString().toDouble(&ok);
                                                if (!ok) continue;
                                            } else {
                                                continue;
                                            }
                                            
                                            // 保存到 attributesFull
                                            student.attributesFull[compositeKey] = numValue;
                                            
                                            // 解析复合键名，提取字段名和Excel文件名
                                            int underscorePos = compositeKey.lastIndexOf('_');
                                            if (underscorePos > 0 && underscorePos < compositeKey.length() - 1) {
                                                QString fieldName = compositeKey.left(underscorePos);
                                                QString excelFileName = compositeKey.mid(underscorePos + 1);
                                                
                                                // 保存到 attributesByExcel
                                                student.attributesByExcel[excelFileName][fieldName] = numValue;
                                                
                                                // 向后兼容：如果 attributes 中还没有该字段，使用第一个值
                                                if (!student.attributes.contains(fieldName)) {
                                                    student.attributes[fieldName] = numValue;
                                                }
                                            }
                                        }
                                    }
                                }

                                // 解析 field_sources（字段来源详细信息）
                                // 按需求：优先使用 scores_json_full；仅当没有 scores_json_full 时才回退解析 field_sources
                                if (!hasScoresJsonFull && scoreObj.contains("field_sources") && scoreObj["field_sources"].isObject()) {
                                    QJsonObject fieldSourcesObj = scoreObj["field_sources"].toObject();
                                    for (auto it = fieldSourcesObj.begin(); it != fieldSourcesObj.end(); ++it) {
                                        QString fieldName = it.key();
                                        QJsonObject fieldSourceObj = it.value().toObject();
                                        
                                        // 获取默认值
                                        if (fieldSourceObj.contains("value")) {
                                            double defaultValue = fieldSourceObj["value"].toDouble();
                                            if (!student.attributes.contains(fieldName)) {
                                                student.attributes[fieldName] = defaultValue;
                                            }
                                        }
                                        
                                        // 获取所有来源
                                        if (fieldSourceObj.contains("sources") && fieldSourceObj["sources"].isArray()) {
                                            QJsonArray sourcesArray = fieldSourceObj["sources"].toArray();
                                            for (const auto& source : sourcesArray) {
                                                QJsonObject sourceObj = source.toObject();
                                                QString excelFileName = sourceObj["excel_filename"].toString();
                                                double value = sourceObj["value"].toDouble();
                                                
                                                // 保存到 attributesByExcel
                                                student.attributesByExcel[excelFileName][fieldName] = value;
                                                
                                                // 保存到 attributesFull
                                                QString compositeKey = QString("%1_%2").arg(fieldName).arg(excelFileName);
                                                student.attributesFull[compositeKey] = value;
                                            }
                                        }
                                    }
                                }
                                
                                // 动态读取所有属性字段（排除系统字段）- 向后兼容
                                for (auto it = scoreObj.begin(); it != scoreObj.end(); ++it) {
                                    QString key = it.key();
                                    // 跳过系统字段
                                    if (systemFields.contains(key)) {
                                        continue;
                                    }
                                    
                                    // 跳过注释字段（以 _comment 结尾的字段）
                                    if (key.endsWith("_comment")) {
                                        // 提取字段名（去掉 _comment 后缀）
                                        QString fieldName = key.left(key.length() - 8); // "_comment" 长度为 8
                                        QString comment = it.value().toString();
                                        if (!comment.isEmpty()) {
                                            student.comments[fieldName] = comment;
                                        }
                                        continue;
                                    }
                                    
                                    // 读取属性值（处理 null 值）
                                    QJsonValue value = it.value();
                                    if (!value.isNull()) {
                                        if (value.isDouble()) {
                                            // 向后兼容：如果还没有该字段，才添加
                                            if (!student.attributes.contains(key)) {
                                                student.attributes[key] = value.toDouble();
                                            }
                                        } else if (value.isString()) {
                                            // 尝试转换为数字
                                            bool ok;
                                            double numValue = value.toString().toDouble(&ok);
                                            if (ok && !student.attributes.contains(key)) {
                                                student.attributes[key] = numValue;
                                            }
                                        }
                                    }
                                }

                                // 解析 scores 对象（旧数据可能没有）
                                if (scoreObj.contains("scores") && scoreObj["scores"].isObject()) {
                                    QJsonObject scoresObj = scoreObj["scores"].toObject();
                                    for (auto it = scoresObj.begin(); it != scoresObj.end(); ++it) {
                                        QString key = it.key();
                                        QJsonValue value = it.value();
                                        if (value.isDouble()) {
                                            if (!student.attributes.contains(key)) {
                                                student.attributes[key] = value.toDouble();
                                            }
                                        } else if (value.isString()) {
                                            bool ok = false;
                                            double numValue = value.toString().toDouble(&ok);
                                            if (ok && !student.attributes.contains(key)) {
                                                student.attributes[key] = numValue;
                                            }
                                        }
                                    }
                                }
                                
                                // 解析 comments（兼容对象或 JSON 字符串）
                                // 新增：优先解析 comments_json_full（对象/JSON字符串）
                                QJsonObject commentsObj;
                                bool hasCommentsObj = false;
                                auto mergeCommentsValue = [&](const QJsonValue& v) {
                                    if (v.isObject()) {
                                        const QJsonObject o = v.toObject();
                                        for (auto it = o.begin(); it != o.end(); ++it) {
                                            commentsObj[it.key()] = it.value();
                                        }
                                        hasCommentsObj = hasCommentsObj || !o.isEmpty();
                                    } else if (v.isString()) {
                                        QJsonParseError pe;
                                        const QJsonDocument d = QJsonDocument::fromJson(v.toString().toUtf8(), &pe);
                                        if (pe.error == QJsonParseError::NoError && d.isObject()) {
                                            const QJsonObject o = d.object();
                                            for (auto it = o.begin(); it != o.end(); ++it) {
                                                commentsObj[it.key()] = it.value();
                                            }
                                            hasCommentsObj = hasCommentsObj || !o.isEmpty();
                                        }
                                    }
                                };
                                if (scoreObj.contains(QStringLiteral("comments_json_full"))) {
                                    mergeCommentsValue(scoreObj.value(QStringLiteral("comments_json_full")));
                                }
                                if (scoreObj.contains(QStringLiteral("comments"))) {
                                    mergeCommentsValue(scoreObj.value(QStringLiteral("comments")));
                                }
                                if (hasCommentsObj) {
                                    for (auto it = commentsObj.begin(); it != commentsObj.end(); ++it) {
                                        QString key = it.key();
                                        QString comment = it.value().toString();
                                        if (comment.isEmpty()) continue;
                                        
                                        // 判断是否是复合键名（包含下划线和可能的Excel文件名）
                                        int underscorePos = key.lastIndexOf('_');
                                        if (underscorePos > 0 && underscorePos < key.length() - 1) {
                                            // 可能是复合键名，检查是否以 .xlsx 或 .xls 结尾
                                            QString possibleFileName = key.mid(underscorePos + 1);
                                            if (possibleFileName.endsWith(".xlsx") || possibleFileName.endsWith(".xls") || 
                                                possibleFileName.endsWith(".csv")) {
                                                // 是复合键名注释
                                                QString fieldName = key.left(underscorePos);
                                                QString excelFileName = possibleFileName;
                                                
                                                // 保存到 commentsFull
                                                student.commentsFull[key] = comment;
                                                
                                                // 保存到 commentsByExcel
                                                student.commentsByExcel[excelFileName][fieldName] = comment;
                                                
                                                // 向后兼容：如果还没有该字段的注释，使用第一个
                                                if (!student.comments.contains(fieldName)) {
                                                    student.comments[fieldName] = comment;
                                                }

                                                // 同步到全局注释缓存：同时保存复合键与纯字段名（便于不同界面读取）
                                                CommentStorage::saveComment(m_classid, termForCache, student.id, key, excelFileName, comment);
                                                if (CommentStorage::getComment(m_classid, termForCache, student.id, fieldName, excelFileName).isEmpty()) {
                                                    CommentStorage::saveComment(m_classid, termForCache, student.id, fieldName, excelFileName, comment);
                                                }
                                            } else {
                                                // 普通字段注释
                                                student.comments[key] = comment;
                                                CommentStorage::saveComment(
                                                    m_classid, termForCache, student.id,
                                                    key, CommentStorage::inferTableNameFromFieldKey(key), comment);
                                            }
                                        } else {
                                            // 普通字段注释
                                            student.comments[key] = comment;
                                            CommentStorage::saveComment(
                                                m_classid, termForCache, student.id,
                                                key, CommentStorage::inferTableNameFromFieldKey(key), comment);
                                        }
                                    }
                                }
                                
                                // 读取总分（字段名是 total_score），用于排序
                                if (scoreObj.contains("total_score") && !scoreObj["total_score"].isNull()) {
                                    double total = scoreObj["total_score"].toDouble();
                                    student.score = total; // 使用总分作为排序依据
                                    // 如果attributes中没有"总分"，则添加
                                    if (!student.attributes.contains("总分")) {
                                        student.attributes["总分"] = total;
                                    }
                                } else {
                                    // 如果没有total_score，使用新的辅助函数获取"总分"（优先级：attributesByExcel → attributesFull → attributes）
                                    double total = student.getAttributeValue("总分");
                                    if (total > 0.0) {
                                        student.score = total;
                                    } else {
                                        student.score = 0;
                                    }
                                }

                                if (student.groupTotalScore > 0) {
                                    student.attributes["小组总分"] = student.groupTotalScore;
                                }
                                
                                m_students.append(student);
                            }
                            // 缓存 student-scores 结果
                            m_studentsStudentScores = m_students;
                            if (customListDlg) {
                                customListDlg->setServerScoreCaches(m_studentsStudentScores, m_studentsGroupScores, termForCache);
                            }
                        } else if (dataObj.contains("scores") && dataObj["scores"].isArray()) {
                            hasScores = true;
                            QJsonArray scoresArray = dataObj["scores"].toArray();
                            m_students.clear();
                            
                            for (int i = 0; i < scoresArray.size(); i++)
                            {
                                QJsonObject scoreObj = scoresArray[i].toObject();
                                
                                StudentInfo student;
                                // 兼容不同字段命名：student_id/studentId/id 以及 student_name/name
                                student.id = scoreObj.value(QStringLiteral("student_id")).toString();
                                if (student.id.isEmpty()) student.id = scoreObj.value(QStringLiteral("studentId")).toString();
                                if (student.id.isEmpty() && scoreObj.contains(QStringLiteral("id")) && scoreObj.value(QStringLiteral("id")).isDouble()) {
                                    student.id = QString::number(scoreObj.value(QStringLiteral("id")).toInt());
                                }
                                student.name = scoreObj.value(QStringLiteral("student_name")).toString();
                                if (student.name.isEmpty()) student.name = scoreObj.value(QStringLiteral("name")).toString();
                                student.originalIndex = i;
                                if (scoreObj.contains("group_name")) {
                                    student.groupName = scoreObj.value("group_name").toString();
                                }
                                if (scoreObj.contains("group_total_score")) {
                                    student.groupTotalScore = scoreObj.value("group_total_score").toDouble();
                                }
                                
                                // 系统字段列表（需要排除的字段）
                                QSet<QString> systemFields;
                                systemFields << "id" << "student_id" << "student_name" << "total_score" << "comments"
                                             << "comments_json_full"
                                             << "group_name" << "group_total_score" << "scores"
                                             << "scores_json_full" << "field_sources"; // 新增系统字段

                                bool hasScoresJsonFull = false;
                                // 解析 scores_json_full（包含所有复合键名的完整数据）
                                QJsonObject scoresFullObj;
                                if (scoreObj.contains(QStringLiteral("scores_json_full"))) {
                                    const QJsonValue v = scoreObj.value(QStringLiteral("scores_json_full"));
                                    if (v.isObject()) {
                                        scoresFullObj = v.toObject();
                                    } else if (v.isString()) {
                                        QJsonParseError pe;
                                        const QJsonDocument d = QJsonDocument::fromJson(v.toString().toUtf8(), &pe);
                                        if (pe.error == QJsonParseError::NoError && d.isObject()) {
                                            scoresFullObj = d.object();
                                        }
                                    }
                                }
                                if (!scoresFullObj.isEmpty()) {
                                    hasScoresJsonFull = true;
                                    for (auto it = scoresFullObj.begin(); it != scoresFullObj.end(); ++it) {
                                        QString compositeKey = it.key(); // 如 "语文_期中成绩单.xlsx"
                                        QJsonValue value = it.value();
                                        if (!value.isNull()) {
                                            double numValue = 0.0;
                                            if (value.isDouble()) {
                                                numValue = value.toDouble();
                                            } else if (value.isString()) {
                                                bool ok;
                                                numValue = value.toString().toDouble(&ok);
                                                if (!ok) continue;
                                            } else {
                                                continue;
                                            }
                                            
                                            // 保存到 attributesFull
                                            student.attributesFull[compositeKey] = numValue;
                                            
                                            // 解析复合键名，提取字段名和Excel文件名
                                            int underscorePos = compositeKey.lastIndexOf('_');
                                            if (underscorePos > 0 && underscorePos < compositeKey.length() - 1) {
                                                QString fieldName = compositeKey.left(underscorePos);
                                                QString excelFileName = compositeKey.mid(underscorePos + 1);
                                                
                                                // 保存到 attributesByExcel
                                                student.attributesByExcel[excelFileName][fieldName] = numValue;
                                                
                                                // 向后兼容：如果 attributes 中还没有该字段，使用第一个值
                                                if (!student.attributes.contains(fieldName)) {
                                                    student.attributes[fieldName] = numValue;
                                                }
                                            }
                                        }
                                    }
                                }

                                // 解析 field_sources（字段来源详细信息）
                                // 按需求：优先使用 scores_json_full；仅当没有 scores_json_full 时才回退解析 field_sources
                                if (!hasScoresJsonFull && scoreObj.contains("field_sources") && scoreObj["field_sources"].isObject()) {
                                    QJsonObject fieldSourcesObj = scoreObj["field_sources"].toObject();
                                    for (auto it = fieldSourcesObj.begin(); it != fieldSourcesObj.end(); ++it) {
                                        QString fieldName = it.key();
                                        QJsonObject fieldSourceObj = it.value().toObject();
                                        
                                        // 获取默认值
                                        if (fieldSourceObj.contains("value")) {
                                            double defaultValue = fieldSourceObj["value"].toDouble();
                                            if (!student.attributes.contains(fieldName)) {
                                                student.attributes[fieldName] = defaultValue;
                                            }
                                        }
                                        
                                        // 获取所有来源
                                        if (fieldSourceObj.contains("sources") && fieldSourceObj["sources"].isArray()) {
                                            QJsonArray sourcesArray = fieldSourceObj["sources"].toArray();
                                            for (const auto& source : sourcesArray) {
                                                QJsonObject sourceObj = source.toObject();
                                                QString excelFileName = sourceObj["excel_filename"].toString();
                                                double value = sourceObj["value"].toDouble();
                                                
                                                // 保存到 attributesByExcel
                                                student.attributesByExcel[excelFileName][fieldName] = value;
                                                
                                                // 保存到 attributesFull
                                                QString compositeKey = QString("%1_%2").arg(fieldName).arg(excelFileName);
                                                student.attributesFull[compositeKey] = value;
                                            }
                                        }
                                    }
                                }
                                
                                // 动态读取所有属性字段（排除系统字段）- 向后兼容
                                for (auto it = scoreObj.begin(); it != scoreObj.end(); ++it) {
                                    QString key = it.key();
                                    // 跳过系统字段
                                    if (systemFields.contains(key)) {
                                        continue;
                                    }
                                    
                                    // 跳过注释字段（以 _comment 结尾的字段）
                                    if (key.endsWith("_comment")) {
                                        // 提取字段名（去掉 _comment 后缀）
                                        QString fieldName = key.left(key.length() - 8); // "_comment" 长度为 8
                                        QString comment = it.value().toString();
                                        if (!comment.isEmpty()) {
                                            student.comments[fieldName] = comment;
                                        }
                                        continue;
                                    }
                                    
                                    // 读取属性值（处理 null 值）
                                    QJsonValue value = it.value();
                                    if (!value.isNull()) {
                                        if (value.isDouble()) {
                                            // 向后兼容：如果还没有该字段，才添加
                                            if (!student.attributes.contains(key)) {
                                                student.attributes[key] = value.toDouble();
                                            }
                                        } else if (value.isString()) {
                                            // 尝试转换为数字
                                            bool ok;
                                            double numValue = value.toString().toDouble(&ok);
                                            if (ok && !student.attributes.contains(key)) {
                                                student.attributes[key] = numValue;
                                            }
                                        }
                                    }
                                }

                                // 解析 scores 对象（旧数据可能没有）
                                if (scoreObj.contains("scores") && scoreObj["scores"].isObject()) {
                                    QJsonObject scoresObj = scoreObj["scores"].toObject();
                                    for (auto it = scoresObj.begin(); it != scoresObj.end(); ++it) {
                                        QString key = it.key();
                                        QJsonValue value = it.value();
                                        if (value.isDouble()) {
                                            if (!student.attributes.contains(key)) {
                                                student.attributes[key] = value.toDouble();
                                            }
                                        } else if (value.isString()) {
                                            bool ok = false;
                                            double numValue = value.toString().toDouble(&ok);
                                            if (ok && !student.attributes.contains(key)) {
                                                student.attributes[key] = numValue;
                                            }
                                        }
                                    }
                                }
                                
                                // 解析 comments（兼容对象或 JSON 字符串）
                                QJsonObject commentsObj;
                                bool hasCommentsObj = false;
                                if (scoreObj.contains("comments")) {
                                    if (scoreObj["comments"].isObject()) {
                                        commentsObj = scoreObj["comments"].toObject();
                                        hasCommentsObj = true;
                                    } else if (scoreObj["comments"].isString()) {
                                        QByteArray commentsBytes = scoreObj["comments"].toString().toUtf8();
                                        QJsonParseError pe;
                                        QJsonDocument cd = QJsonDocument::fromJson(commentsBytes, &pe);
                                        if (pe.error == QJsonParseError::NoError && cd.isObject()) {
                                            commentsObj = cd.object();
                                            hasCommentsObj = true;
                                        }
                                    }
                                }
                                if (hasCommentsObj) {
                                    for (auto it = commentsObj.begin(); it != commentsObj.end(); ++it) {
                                        QString key = it.key();
                                        QString comment = it.value().toString();
                                        if (comment.isEmpty()) continue;
                                        
                                        // 判断是否是复合键名（包含下划线和可能的Excel文件名）
                                        int underscorePos = key.lastIndexOf('_');
                                        if (underscorePos > 0 && underscorePos < key.length() - 1) {
                                            // 可能是复合键名，检查是否以 .xlsx 或 .xls 结尾
                                            QString possibleFileName = key.mid(underscorePos + 1);
                                            if (possibleFileName.endsWith(".xlsx") || possibleFileName.endsWith(".xls") || 
                                                possibleFileName.endsWith(".csv")) {
                                                // 是复合键名注释
                                                QString fieldName = key.left(underscorePos);
                                                QString excelFileName = possibleFileName;
                                                
                                                // 保存到 commentsFull
                                                student.commentsFull[key] = comment;
                                                
                                                // 保存到 commentsByExcel
                                                student.commentsByExcel[excelFileName][fieldName] = comment;
                                                
                                                // 向后兼容：如果还没有该字段的注释，使用第一个
                                                if (!student.comments.contains(fieldName)) {
                                                    student.comments[fieldName] = comment;
                                                }

                                                // 同步到全局注释缓存：同时保存复合键与纯字段名（便于不同界面读取）
                                                CommentStorage::saveComment(m_classid, termForCache, student.id, key, excelFileName, comment);
                                                if (CommentStorage::getComment(m_classid, termForCache, student.id, fieldName, excelFileName).isEmpty()) {
                                                    CommentStorage::saveComment(m_classid, termForCache, student.id, fieldName, excelFileName, comment);
                                                }
                                            } else {
                                                // 普通字段注释
                                                student.comments[key] = comment;
                                                CommentStorage::saveComment(m_classid, termForCache, student.id, key, QString(), comment);
                                            }
                                        } else {
                                            // 普通字段注释
                                            student.comments[key] = comment;
                                            CommentStorage::saveComment(
                                                m_classid, termForCache, student.id,
                                                key, CommentStorage::inferTableNameFromFieldKey(key), comment);
                                        }
                                    }
                                }
                                
                                // 读取总分（字段名是 total_score），用于排序
                                if (scoreObj.contains("total_score") && !scoreObj["total_score"].isNull()) {
                                    double total = scoreObj["total_score"].toDouble();
                                    student.score = total; // 使用总分作为排序依据
                                    // 如果attributes中没有"总分"，则添加
                                    if (!student.attributes.contains("总分")) {
                                        student.attributes["总分"] = total;
                                    }
                                } else {
                                    // 如果没有total_score，使用新的辅助函数获取"总分"（优先级：attributesByExcel → attributesFull → attributes）
                                    double total = student.getAttributeValue("总分");
                                    if (total > 0.0) {
                                        student.score = total;
                                    } else {
                                        student.score = 0;
                                    }
                                }

                                if (student.groupTotalScore > 0) {
                                    student.attributes["小组总分"] = student.groupTotalScore;
                                }
                                
                                m_students.append(student);
                            }
                            // 该分支属于旧的 student-scores 扁平返回，按 student 缓存
                            m_studentsStudentScores = m_students;
                            if (customListDlg) {
                                customListDlg->setServerScoreCaches(m_studentsStudentScores, m_studentsGroupScores, termForCache);
                            }
                        } else if (dataObj.contains("headers") && dataObj["headers"].isArray()) {
                            // 兼容 headers 内嵌 scores 的场景
                            QJsonArray headersArray = dataObj["headers"].toArray();
                            for (const auto& h : headersArray) {
                                QJsonObject headerObj = h.toObject();
                                if (headerObj.contains("scores") && headerObj["scores"].isArray()) {
                                    // /student-scores 的一种返回：data.headers[].scores
                                    // 需要标记为 student 响应，才能在 hasScores 末尾触发“先 student 后 group”的串行链路
                                    isStudentScoresResp = true;
                                    hasScores = true;
                                    QJsonArray scoresArray = headerObj["scores"].toArray();
                                    m_students.clear();
                                    
                                    for (int i = 0; i < scoresArray.size(); i++) {
                                        QJsonObject scoreObj = scoresArray[i].toObject();
                                        
                                        StudentInfo student;
                                        // 兼容不同字段命名：student_id/studentId/id 以及 student_name/name
                                        student.id = scoreObj.value(QStringLiteral("student_id")).toString();
                                        if (student.id.isEmpty()) student.id = scoreObj.value(QStringLiteral("studentId")).toString();
                                        if (student.id.isEmpty() && scoreObj.contains(QStringLiteral("id")) && scoreObj.value(QStringLiteral("id")).isDouble()) {
                                            student.id = QString::number(scoreObj.value(QStringLiteral("id")).toInt());
                                        }
                                        student.name = scoreObj.value(QStringLiteral("student_name")).toString();
                                        if (student.name.isEmpty()) student.name = scoreObj.value(QStringLiteral("name")).toString();
                                        student.originalIndex = i;
                                        if (scoreObj.contains("group_name")) {
                                            student.groupName = scoreObj.value("group_name").toString();
                                        }
                                        if (scoreObj.contains("group_total_score")) {
                                            student.groupTotalScore = scoreObj.value("group_total_score").toDouble();
                                        }
                                        
                                        // 系统字段列表（需要排除的字段）
                                        QSet<QString> systemFields;
                                        systemFields << "id" << "student_id" << "student_name" << "total_score" << "comments"
                                                     << "comments_json_full"
                                                     << "group_name" << "group_total_score" << "scores"
                                                     << "scores_json_full" << "field_sources"; // 新增系统字段

                                        bool hasScoresJsonFull = false;
                                        // 解析 scores_json_full（包含所有复合键名的完整数据）
                                        QJsonObject scoresFullObj;
                                        if (scoreObj.contains(QStringLiteral("scores_json_full"))) {
                                            const QJsonValue v = scoreObj.value(QStringLiteral("scores_json_full"));
                                            if (v.isObject()) {
                                                scoresFullObj = v.toObject();
                                            } else if (v.isString()) {
                                                QJsonParseError pe;
                                                const QJsonDocument d = QJsonDocument::fromJson(v.toString().toUtf8(), &pe);
                                                if (pe.error == QJsonParseError::NoError && d.isObject()) {
                                                    scoresFullObj = d.object();
                                                }
                                            }
                                        }
                                        if (!scoresFullObj.isEmpty()) {
                                            hasScoresJsonFull = true;
                                            for (auto it = scoresFullObj.begin(); it != scoresFullObj.end(); ++it) {
                                                QString compositeKey = it.key(); // 如 "语文_期中成绩单.xlsx"
                                                QJsonValue value = it.value();
                                                if (!value.isNull()) {
                                                    double numValue = 0.0;
                                                    if (value.isDouble()) {
                                                        numValue = value.toDouble();
                                                    } else if (value.isString()) {
                                                        bool ok;
                                                        numValue = value.toString().toDouble(&ok);
                                                        if (!ok) continue;
                                                    } else {
                                                        continue;
                                                    }
                                                    
                                                    // 保存到 attributesFull
                                                    student.attributesFull[compositeKey] = numValue;
                                                    
                                                    // 解析复合键名，提取字段名和Excel文件名
                                                    int underscorePos = compositeKey.lastIndexOf('_');
                                                    if (underscorePos > 0 && underscorePos < compositeKey.length() - 1) {
                                                        QString fieldName = compositeKey.left(underscorePos);
                                                        QString excelFileName = compositeKey.mid(underscorePos + 1);
                                                        
                                                        // 保存到 attributesByExcel
                                                        student.attributesByExcel[excelFileName][fieldName] = numValue;
                                                        
                                                        // 向后兼容：如果 attributes 中还没有该字段，使用第一个值
                                                        if (!student.attributes.contains(fieldName)) {
                                                            student.attributes[fieldName] = numValue;
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        // 解析 field_sources（字段来源详细信息）
                                        // 按需求：优先使用 scores_json_full；仅当没有 scores_json_full 时才回退解析 field_sources
                                        if (!hasScoresJsonFull && scoreObj.contains("field_sources") && scoreObj["field_sources"].isObject()) {
                                            QJsonObject fieldSourcesObj = scoreObj["field_sources"].toObject();
                                            for (auto it = fieldSourcesObj.begin(); it != fieldSourcesObj.end(); ++it) {
                                                QString fieldName = it.key();
                                                QJsonObject fieldSourceObj = it.value().toObject();
                                                
                                                // 获取默认值
                                                if (fieldSourceObj.contains("value")) {
                                                    double defaultValue = fieldSourceObj["value"].toDouble();
                                                    if (!student.attributes.contains(fieldName)) {
                                                        student.attributes[fieldName] = defaultValue;
                                                    }
                                                }
                                                
                                                // 获取所有来源
                                                if (fieldSourceObj.contains("sources") && fieldSourceObj["sources"].isArray()) {
                                                    QJsonArray sourcesArray = fieldSourceObj["sources"].toArray();
                                                    for (const auto& source : sourcesArray) {
                                                        QJsonObject sourceObj = source.toObject();
                                                        QString excelFileName = sourceObj["excel_filename"].toString();
                                                        double value = sourceObj["value"].toDouble();
                                                        
                                                        // 保存到 attributesByExcel
                                                        student.attributesByExcel[excelFileName][fieldName] = value;
                                                        
                                                        // 保存到 attributesFull
                                                        QString compositeKey = QString("%1_%2").arg(fieldName).arg(excelFileName);
                                                        student.attributesFull[compositeKey] = value;
                                                    }
                                                }
                                            }
                                        }
                                        
                                        // 动态读取所有属性字段（排除系统字段）- 向后兼容
                                        for (auto it = scoreObj.begin(); it != scoreObj.end(); ++it) {
                                            QString key = it.key();
                                            // 跳过系统字段
                                            if (systemFields.contains(key)) {
                                                continue;
                                            }
                                            
                                            // 跳过注释字段（以 _comment 结尾的字段）
                                            if (key.endsWith("_comment")) {
                                                // 提取字段名（去掉 _comment 后缀）
                                                QString fieldName = key.left(key.length() - 8); // "_comment" 长度为 8
                                                QString comment = it.value().toString();
                                                if (!comment.isEmpty()) {
                                                    student.comments[fieldName] = comment;
                                                }
                                                continue;
                                            }
                                            
                                            // 读取属性值（处理 null 值）
                                            QJsonValue value = it.value();
                                            if (!value.isNull()) {
                                                if (value.isDouble()) {
                                                    // 向后兼容：如果还没有该字段，才添加
                                                    if (!student.attributes.contains(key)) {
                                                        student.attributes[key] = value.toDouble();
                                                    }
                                                } else if (value.isString()) {
                                                    // 尝试转换为数字
                                                    bool ok;
                                                    double numValue = value.toString().toDouble(&ok);
                                                    if (ok && !student.attributes.contains(key)) {
                                                        student.attributes[key] = numValue;
                                                    }
                                                }
                                            }
                                        }

                                        // 解析 scores 对象（旧数据可能没有）
                                        if (scoreObj.contains("scores") && scoreObj["scores"].isObject()) {
                                            QJsonObject scoresObj = scoreObj["scores"].toObject();
                                            for (auto it = scoresObj.begin(); it != scoresObj.end(); ++it) {
                                                QString key = it.key();
                                                QJsonValue value = it.value();
                                                if (value.isDouble()) {
                                                    if (!student.attributes.contains(key)) {
                                                        student.attributes[key] = value.toDouble();
                                                    }
                                                } else if (value.isString()) {
                                                    bool ok = false;
                                                    double numValue = value.toString().toDouble(&ok);
                                                    if (ok && !student.attributes.contains(key)) {
                                                        student.attributes[key] = numValue;
                                                    }
                                                }
                                            }
                                        }
                                        
                                        // 解析 comments（兼容对象或 JSON 字符串）
                                        // 新增：优先解析 comments_json_full（对象/JSON字符串）
                                        QJsonObject commentsObj;
                                        bool hasCommentsObj = false;
                                        auto mergeCommentsValue = [&](const QJsonValue& v) {
                                            if (v.isObject()) {
                                                const QJsonObject o = v.toObject();
                                                for (auto it = o.begin(); it != o.end(); ++it) {
                                                    commentsObj[it.key()] = it.value();
                                                }
                                                hasCommentsObj = hasCommentsObj || !o.isEmpty();
                                            } else if (v.isString()) {
                                                QJsonParseError pe;
                                                const QJsonDocument d = QJsonDocument::fromJson(v.toString().toUtf8(), &pe);
                                                if (pe.error == QJsonParseError::NoError && d.isObject()) {
                                                    const QJsonObject o = d.object();
                                                    for (auto it = o.begin(); it != o.end(); ++it) {
                                                        commentsObj[it.key()] = it.value();
                                                    }
                                                    hasCommentsObj = hasCommentsObj || !o.isEmpty();
                                                }
                                            }
                                        };
                                        if (scoreObj.contains(QStringLiteral("comments_json_full"))) {
                                            mergeCommentsValue(scoreObj.value(QStringLiteral("comments_json_full")));
                                        }
                                        if (scoreObj.contains(QStringLiteral("comments"))) {
                                            mergeCommentsValue(scoreObj.value(QStringLiteral("comments")));
                                        }
                                        if (hasCommentsObj) {
                                            for (auto it = commentsObj.begin(); it != commentsObj.end(); ++it) {
                                                QString key = it.key();
                                                QString comment = it.value().toString();
                                                if (comment.isEmpty()) continue;

                                                // 判断是否是复合键名（包含下划线和可能的Excel文件名）
                                                int underscorePos = key.lastIndexOf('_');
                                                if (underscorePos > 0 && underscorePos < key.length() - 1) {
                                                    // 可能是复合键名，检查是否以 .xlsx 或 .xls 结尾
                                                    QString possibleFileName = key.mid(underscorePos + 1);
                                                    if (possibleFileName.endsWith(".xlsx") || possibleFileName.endsWith(".xls") ||
                                                        possibleFileName.endsWith(".csv")) {
                                                        // 是复合键名注释
                                                        QString fieldName = key.left(underscorePos);
                                                        QString excelFileName = possibleFileName;

                                                        // 保存到 commentsFull / commentsByExcel
                                                        student.commentsFull[key] = comment;
                                                        student.commentsByExcel[excelFileName][fieldName] = comment;

                                                        // 向后兼容：如果还没有该字段的注释，使用第一个
                                                        if (!student.comments.contains(fieldName)) {
                                                            student.comments[fieldName] = comment;
                                                        }
                                                    } else {
                                                        student.comments[key] = comment;
                                                    }
                                                } else {
                                                    student.comments[key] = comment;
                                                }
                                            }
                                        }
                                        
                                        // 读取总分（字段名是 total_score），用于排序
                                        if (scoreObj.contains("total_score") && !scoreObj["total_score"].isNull()) {
                                            double total = scoreObj["total_score"].toDouble();
                                            student.score = total; // 使用总分作为排序依据
                                            // 如果attributes中没有"总分"，则添加
                                            if (!student.attributes.contains("总分")) {
                                                student.attributes["总分"] = total;
                                            }
                                        } else {
                                            // 如果没有total_score，使用新的辅助函数获取"总分"（优先级：attributesByExcel → attributesFull → attributes）
                                            double total = student.getAttributeValue("总分");
                                            if (total > 0.0) {
                                                student.score = total;
                                            } else {
                                                student.score = 0;
                                            }
                                        }

                                        if (student.groupTotalScore > 0) {
                                            student.attributes["小组总分"] = student.groupTotalScore;
                                        }
                                        
                                        m_students.append(student);
                                    }
                                    // headers[].scores 属于 student-scores 的一种返回
                                    m_studentsStudentScores = m_students;
                                    if (customListDlg) {
                                        customListDlg->setServerScoreCaches(m_studentsStudentScores, m_studentsGroupScores, termForCache);
                                    }
                                    break; // 已处理，退出 headers 循环
                                }
                            }
                        }

                        // 判断是来自哪个接口的响应（用于区分保存目录）
                        bool isGroupScores = dataObj.contains("group_scores");
                        bool isStudentScores = dataObj.contains("student_scores");
                        
                        // 处理Excel文件URL（无论是否有成绩数据，都需要同步Excel文件）
                        // 兼容多种格式：
                        // - data.excel_file_url: array/object
                        // - data.headers[].excel_file_url: array/object（student-scores headers 场景）
                        // - data.header.excel_file_url: object（group-scores 场景，key=filename）
                        QList<QPair<QString, QString>> excelFiles; // filename, url
                        if (dataObj.contains("excel_file_url")) {
                            if (dataObj["excel_file_url"].isArray()) {
                                QJsonArray excelFileUrlArray = dataObj["excel_file_url"].toArray();
                                for (const auto& f : excelFileUrlArray) {
                                    QJsonObject fileObj = f.toObject();
                                    QString filename = fileObj["filename"].toString();
                                    QString url = fileObj["url"].toString();
                                    if (!filename.isEmpty() && !url.isEmpty()) {
                                        excelFiles.append(qMakePair(filename, url));
                                    }
                                }
                            } else if (dataObj["excel_file_url"].isObject()) {
                                QJsonObject excelFilesObj = dataObj["excel_file_url"].toObject();
                                for (auto it = excelFilesObj.begin(); it != excelFilesObj.end(); ++it) {
                                    QString filename = it.key();
                                    QString url;
                                    if (it.value().isObject()) {
                                        QJsonObject detailObj = it.value().toObject();
                                        url = detailObj.value(QStringLiteral("url")).toString();
                                    } else if (it.value().isString()) {
                                        // 兼容 excel_file_url 直接是字符串 URL 的精简格式
                                        url = it.value().toString();
                                    }
                                    if (!filename.isEmpty() && !url.isEmpty()) {
                                        excelFiles.append(qMakePair(filename, url));
                                    }
                                }
                            }
                        }
                        // group-scores header 场景：excel_file_url 在 data.header.excel_file_url（对象）
                        if (dataObj.contains(QStringLiteral("header")) && dataObj[QStringLiteral("header")].isObject()) {
                            QJsonObject headerObj = dataObj.value(QStringLiteral("header")).toObject();
                            if (headerObj.contains(QStringLiteral("excel_file_url"))) {
                                const QJsonValue v = headerObj.value(QStringLiteral("excel_file_url"));
                                if (v.isObject()) {
                                    QJsonObject excelFilesObj = v.toObject();
                                    for (auto it = excelFilesObj.begin(); it != excelFilesObj.end(); ++it) {
                                        const QString filename = it.key();
                                        QString url;
                                        if (it.value().isObject()) {
                                            const QJsonObject detailObj = it.value().toObject();
                                            url = detailObj.value(QStringLiteral("url")).toString();
                                        } else if (it.value().isString()) {
                                            url = it.value().toString();
                                        }
                                        if (!filename.isEmpty() && !url.isEmpty()) {
                                            excelFiles.append(qMakePair(filename, url));
                                        }
                                    }
                                } else if (v.isArray()) {
                                    QJsonArray arr = v.toArray();
                                    for (const auto& f : arr) {
                                        const QJsonObject fileObj = f.toObject();
                                        const QString filename = fileObj.value(QStringLiteral("filename")).toString();
                                        const QString url = fileObj.value(QStringLiteral("url")).toString();
                                        if (!filename.isEmpty() && !url.isEmpty()) {
                                            excelFiles.append(qMakePair(filename, url));
                                        }
                                    }
                                }
                            }
                        }
                        // student-scores headers 场景：excel_file_url 在 headers 数组里（追加，不依赖是否已有）
                        if (dataObj.contains("headers") && dataObj["headers"].isArray()) {
                            QJsonArray headersArray = dataObj["headers"].toArray();
                            for (const auto& h : headersArray) {
                                QJsonObject headerObj = h.toObject();
                                if (headerObj.contains("excel_file_url")) {
                                    if (headerObj["excel_file_url"].isArray()) {
                                        QJsonArray arr = headerObj["excel_file_url"].toArray();
                                        for (const auto& f : arr) {
                                            QJsonObject fileObj = f.toObject();
                                            QString filename = fileObj["filename"].toString();
                                            QString url = fileObj["url"].toString();
                                            if (!filename.isEmpty() && !url.isEmpty()) {
                                                excelFiles.append(qMakePair(filename, url));
                                            }
                                        }
                                    } else if (headerObj["excel_file_url"].isObject()) {
                                        QJsonObject excelFilesObj = headerObj["excel_file_url"].toObject();
                                        for (auto it = excelFilesObj.begin(); it != excelFilesObj.end(); ++it) {
                                            QString filename = it.key();
                                            QString url;
                                            if (it.value().isObject()) {
                                                QJsonObject detailObj = it.value().toObject();
                                                url = detailObj.value(QStringLiteral("url")).toString();
                                            } else if (it.value().isString()) {
                                                url = it.value().toString();
                                            }
                                            if (!filename.isEmpty() && !url.isEmpty()) {
                                                excelFiles.append(qMakePair(filename, url));
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        // 如果无法通过字段判断类型，尝试通过Excel文件URL来判断
                        if (!isGroupScores && !isStudentScores && !excelFiles.isEmpty()) {
                            // 检查第一个Excel文件的URL，如果包含 group-scores 就是小组表格，包含 student-scores 就是普通表格
                            QString firstUrl = excelFiles.first().second;
                            if (firstUrl.contains("/group-scores/")) {
                                isGroupScores = true;
                            } else if (firstUrl.contains("/student-scores/")) {
                                isStudentScores = true;
                            }
                            // 如果还是无法判断，默认当作普通表格（student-scores）
                            if (!isGroupScores && !isStudentScores) {
                                isStudentScores = true;
                            }
                        }

                        qDebug() << "获取到Excel文件数量:" << excelFiles.size() << (isGroupScores ? "(小组表格)" : (isStudentScores ? "(普通表格)" : "(未知类型)"));
                        
                        // 获取学校ID和班级ID
                        UserInfo userInfo = CommonInfo::GetData();
                        QString schoolId = userInfo.schoolId;
                        QString classId = m_classid;
                        
                        if (!schoolId.isEmpty() && !classId.isEmpty()) {
                            // 根据接口类型确定子目录：group-scores -> group/，student-scores -> student/
                            QString subDir;
                            if (isGroupScores) {
                                subDir = "group";
                            } else if (isStudentScores) {
                                subDir = "student";
                            } else {
                                // 如果无法确定类型，使用默认目录（向后兼容）
                                subDir = "";
                            }
                            
                            // 创建文件夹结构：学校ID/班级ID/[group|student]/
                            QString baseDir = QCoreApplication::applicationDirPath() + "/excel_files";
                            QString schoolDir = baseDir + "/" + schoolId;
                            QString classDir = schoolDir + "/" + classId;
                            QString targetDir = subDir.isEmpty() ? classDir : (classDir + "/" + subDir);
                            
                            // 确保targetDir是绝对路径
                            QDir dir;
                            QString absoluteTargetDir = QDir(targetDir).absolutePath();
                            
                            // 验证并创建目录
                            if (!dir.exists(absoluteTargetDir)) {
                                if (dir.mkpath(absoluteTargetDir)) {
                                    QString msg = QString("[Excel下载] 创建文件夹: %1").arg(absoluteTargetDir);
                                    qInfo() << msg;
                                    writeLogToFile(msg);
                                } else {
                                    QString msg = QString("[Excel下载] 创建文件夹失败: %1").arg(absoluteTargetDir);
                                    qWarning() << msg;
                                    writeLogToFile(msg);
                                }
                            } else {
                                // 先删除本地对应目录的所有Excel文件，确保与服务器完全同步
                                QString typeName = isGroupScores ? "小组表格" : (isStudentScores ? "普通表格" : "未知类型");
                                QString msg = QString("[Excel下载] 先删除本地%1目录的所有Excel文件，目录: %2").arg(typeName).arg(absoluteTargetDir);
                                qInfo() << msg;
                                writeLogToFile(msg);
                                
                                QDir targetDirObj(absoluteTargetDir);
                                if (targetDirObj.exists()) {
                                    QStringList filters;
                                    filters << "*.xlsx" << "*.xls" << "*.csv";
                                    QFileInfoList fileList = targetDirObj.entryInfoList(filters, QDir::Files);
                                    int deletedCount = 0;
                                    for (const QFileInfo& fi : fileList) {
                                        QString filePath = fi.absoluteFilePath();
                                        if (QFile::remove(filePath)) {
                                            deletedCount++;
                                        } else {
                                            QString msg = QString("[Excel下载] 删除文件失败: %1").arg(filePath);
                                            qWarning() << msg;
                                            writeLogToFile(msg);
                                        }
                                    }
                                    if (deletedCount > 0) {
                                        QString msg = QString("[Excel下载] 已删除%1个本地文件").arg(deletedCount);
                                        qInfo() << msg;
                                        writeLogToFile(msg);
                                    }
                                }
                            }
                            
                            // 构建服务器Excel文件名集合（用于后续比对，如果需要）
                            QSet<QString> serverFileNames;
                            for (const auto& pair : excelFiles) {
                                serverFileNames.insert(pair.first);
                            }
                            
                            // 下载服务器上的Excel文件（如果列表不为空）
                            if (!excelFiles.isEmpty()) {
                                // 确保targetDir包含子目录路径（使用绝对路径）
                                QString typeName = isGroupScores ? "小组表格" : (isStudentScores ? "普通表格" : "未知类型");
                                QString msg = QString("[Excel下载] 准备下载%1个Excel文件到目录: %2 (%3)").arg(excelFiles.size()).arg(absoluteTargetDir).arg(typeName);
                                qInfo() << msg;
                                writeLogToFile(msg);
                                
                                // 下载每个Excel文件（使用计数器跟踪下载完成状态）
                                int totalFiles = excelFiles.size();
                                int* completedCount = new int(0); // 使用指针以便在lambda中修改
                                for (const auto& pair : excelFiles) {
                                    // 使用绝对路径
                                    downloadExcelFile(pair.second, absoluteTargetDir, pair.first, [this, totalFiles, completedCount]() {
                                        (*completedCount)++;
                                        QString msg = QString("[Excel下载] 下载进度: %1/%2").arg(*completedCount).arg(totalFiles);
                                        qInfo() << msg;
                                        writeLogToFile(msg);
                                        // 当所有文件下载完成后，刷新热力图选项（如果对话框已打开）
                                        if (*completedCount >= totalFiles) {
                                            QString msg = "[Excel下载] 所有Excel文件下载完成，刷新热力图选项";
                                            qInfo() << msg;
                                            writeLogToFile(msg);
                                            // 使用 QTimer::singleShot 延迟刷新，调用成员函数避免在 lambda 中直接访问未定义类型
                                            QTimer::singleShot(100, this, &ScheduleDialog::refreshHeatmapOptionsIfNeeded);
                                            delete completedCount; // 清理内存
                                        }
                                    });
                                }
                            } else {
                                QString typeName = isGroupScores ? "小组表格" : (isStudentScores ? "普通表格" : "未知类型");
                                QString msg = QString("[Excel下载] 服务器没有Excel文件（headers为空），已删除所有本地%1文件").arg(typeName);
                                qInfo() << msg;
                                writeLogToFile(msg);
                                // 即使服务器没有文件，也刷新热力图选项（清除选项）
                                QTimer::singleShot(100, this, &ScheduleDialog::refreshHeatmapOptionsIfNeeded);
                            }
                        } else if (schoolId.isEmpty() || classId.isEmpty()) {
                            qWarning() << "学校ID或班级ID为空，无法创建文件夹";
                        }

                        if (hasScores) {
                            qDebug() << "从服务器获取成绩表成功，学生数量:" << m_students.size();
                            
                            // 仅在“最终落地”的那次响应执行副作用：
                            // - 当前链路为：先 /student-scores，再 /group-scores
                            // - student 响应阶段会填充 m_students，但随后会被 group 覆盖
                            // 因此：当还处于“student 后还要继续拉 group”的阶段时，跳过写入缓存/写回座位按钮，避免重复与覆盖噪音。
                            const bool shouldApplyScoreSideEffects = isGroupScoresResp || !m_scoresNeedFetchGroupAfterStudent;
                            // student 阶段也保留一份：写入时加前缀，避免后续 group 覆盖
                            const bool shouldApplyStudentPrefixedSideEffects = isStudentScoresResp && m_scoresNeedFetchGroupAfterStudent;
                            const QString studentFieldPrefix = QStringLiteral("student::");

                            // 获取 score_header_id（兼容新旧字段）
                            int scoreHeaderId = -1;
                            // 新接口优先：data.headers[0].score_header_id / data.headers[0].id
                            if (dataObj.contains("headers") && dataObj["headers"].isArray()) {
                                QJsonArray headersArr = dataObj["headers"].toArray();
                                if (!headersArr.isEmpty() && headersArr.first().isObject()) {
                                    QJsonObject headerObj = headersArr.first().toObject();
                                    if (headerObj.contains("score_header_id")) {
                                        scoreHeaderId = headerObj["score_header_id"].toInt();
                                    } else if (headerObj.contains("id")) {
                                        scoreHeaderId = headerObj["id"].toInt();
                                    }
                                }
                            }
                            if (dataObj.contains("header") && dataObj["header"].isObject()) {
                                QJsonObject headerObj = dataObj["header"].toObject();
                                if (headerObj.contains("score_header_id")) {
                                    scoreHeaderId = headerObj["score_header_id"].toInt();
                                } else if (headerObj.contains("id")) {
                                    scoreHeaderId = headerObj["id"].toInt();
                                }
                            }
                            if (scoreHeaderId < 0 && dataObj.contains("score_header_id")) {
                                scoreHeaderId = dataObj["score_header_id"].toInt();
                            }
                            if (scoreHeaderId < 0 && dataObj.contains("id") && dataObj["id"].isDouble()) {
                                scoreHeaderId = dataObj["id"].toInt();
                            }

                            if (scoreHeaderId > 0 && shouldApplyScoreSideEffects) {
                                qDebug() << "获取到 score_header_id:" << scoreHeaderId;

                                // 使用前面统一解析出来的 termForCache，避免用本地时间推算导致学期不一致
                                const QString term = termForCache;
                                
                                // 保存到全局存储（按 classId + term，不区分 exam_name）
                                ScoreHeaderIdStorage::saveScoreHeaderId(m_classid, term, scoreHeaderId);
                                
                                // 清除该班级、学期的旧注释，但保留 student:: 前缀的注释（用于同时保留 student-scores 的字段）
                                CommentStorage::clearCommentsExceptFieldPrefix(m_classid, term, studentFieldPrefix);
                                
                                // 保存所有学生的注释信息到全局存储
                                for (const StudentInfo& student : m_students) {
                                    for (auto it = student.comments.begin(); it != student.comments.end(); ++it) {
                                        QString fieldName = it.key();
                                        QString comment = it.value();
                                        CommentStorage::saveComment(
                                            m_classid, term, student.id,
                                            fieldName, CommentStorage::inferTableNameFromFieldKey(fieldName), comment);
                                    }
                                }
                            }
                            // student 阶段：把注释也保存一份到 CommentStorage（加 student:: 前缀避免覆盖）
                            if (shouldApplyStudentPrefixedSideEffects) {
                                const QString term = termForCache;
                                for (const StudentInfo& student : m_students) {
                                    // 优先使用 commentsFull（复合键：字段_Excel文件名），否则回退 comments（向后兼容）
                                    const QMap<QString, QString>& src = !student.commentsFull.isEmpty()
                                        ? student.commentsFull
                                        : student.comments;
                                    for (auto it = src.begin(); it != src.end(); ++it) {
                                        const QString fieldKey = it.key();
                                        const QString comment = it.value();
                                        if (fieldKey.isEmpty()) continue;

                                        // commentsFull 的 key 形如：字段_Excel文件名.xlsx
                                        // 写入 CommentStorage 时使用 buildKey(..., tableName, fieldName)：
                                        // - fieldName 用“字段”（如：早读）
                                        // - tableName 用“Excel文件名.xlsx”（如：学生体质统计表.xlsx）
                                        QString fieldName = fieldKey;
                                        QString tableName = CommentStorage::inferTableNameFromFieldKey(fieldKey);
                                        if (!tableName.isEmpty()) {
                                            const int underscorePos = fieldKey.lastIndexOf('_');
                                            if (underscorePos > 0) {
                                                fieldName = fieldKey.left(underscorePos);
                                            }
                                        }

                                        CommentStorage::saveComment(m_classid, term, student.id,
                                                                   fieldName, tableName, comment);
                                    }
                                }
                            }
                            
                            // 将成绩数据设置到座位表对应单元格学生的属性中
                            if ((shouldApplyScoreSideEffects || shouldApplyStudentPrefixedSideEffects) && !m_students.isEmpty() && seatTable) {
                                const bool useStudentPrefix = shouldApplyStudentPrefixedSideEffects;
                                // 遍历座位表，找到对应的学生并设置成绩属性
                                for (int row = 0; row < 8; ++row) {
                                    for (int col = 0; col < 11; ++col) {
                                        QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
                                        if (!btn || !btn->property("isSeat").toBool()) continue;
                                        
                                        QString studentId = btn->property("studentId").toString();
                                        QString studentName = btn->property("studentName").toString();
                                        
                                        // 如果座位上有学生，查找对应的成绩数据
                                        if (!studentId.isEmpty() || !studentName.isEmpty()) {
                                            // 在成绩列表中查找匹配的学生
                                            for (const StudentInfo& student : m_students) {
                                                bool matched = false;
                                                // 优先匹配学号，如果学号为空则匹配姓名
                                                if (!studentId.isEmpty() && !student.id.isEmpty()) {
                                                    matched = (studentId == student.id);
                                                } else if (!studentName.isEmpty() && !student.name.isEmpty()) {
                                                    matched = (studentName == student.name);
                                                }
                                                
                                                if (matched) {
                                                    // 动态将所有属性数据设置到座位按钮的属性中
                                                    // 优先从 attributesByExcel 中获取（支持不同Excel文件的相同字段名）
                                                    QSet<QString> processedAttrs; // 避免重复设置
                                                    
                                                    // 遍历所有Excel文件的属性
                                                    for (auto excelIt = student.attributesByExcel.begin(); excelIt != student.attributesByExcel.end(); ++excelIt) {
                                                        const QMap<QString, double>& excelAttrs = excelIt.value();
                                                        for (auto it = excelAttrs.begin(); it != excelAttrs.end(); ++it) {
                                                            QString attrName = it.key();
                                                            if (!processedAttrs.contains(attrName)) {
                                                                double attrValue = it.value();
                                                                const QString propName = useStudentPrefix ? (QStringLiteral("student_") + attrName) : attrName;
                                                                btn->setProperty(propName.toUtf8().constData(), attrValue);
                                                                processedAttrs.insert(attrName);
                                                            }
                                                        }
                                                    }
                                                    
                                                    // 从 attributesFull 中获取（复合键名）
                                                    for (auto it = student.attributesFull.begin(); it != student.attributesFull.end(); ++it) {
                                                        QString compositeKey = it.key();
                                                        int underscorePos = compositeKey.lastIndexOf('_');
                                                        if (underscorePos > 0) {
                                                            QString attrName = compositeKey.left(underscorePos);
                                                            if (!processedAttrs.contains(attrName)) {
                                                                double attrValue = it.value();
                                                                const QString propName = useStudentPrefix ? (QStringLiteral("student_") + attrName) : attrName;
                                                                btn->setProperty(propName.toUtf8().constData(), attrValue);
                                                                processedAttrs.insert(attrName);
                                                            }
                                                        }
                                                    }
                                                    
                                                    // 向后兼容：从 attributes 中获取（如果还没有设置）
                                                    for (auto it = student.attributes.begin(); it != student.attributes.end(); ++it) {
                                                        QString attrName = it.key();
                                                        if (!processedAttrs.contains(attrName)) {
                                                            double attrValue = it.value();
                                                            const QString propName = useStudentPrefix ? (QStringLiteral("student_") + attrName) : attrName;
                                                            btn->setProperty(propName.toUtf8().constData(), attrValue);
                                                            processedAttrs.insert(attrName);
                                                        }
                                                    }

                                                    // 保存小组信息到按钮属性，便于后续使用
                                                    if (!student.groupName.isEmpty()) {
                                                        if (useStudentPrefix) {
                                                            btn->setProperty("student_group_name", student.groupName);
                                                        } else {
                                                            btn->setProperty("group_name", student.groupName);
                                                        }
                                                    }
                                                    if (student.groupTotalScore > 0) {
                                                        if (useStudentPrefix) {
                                                            btn->setProperty("student_group_total_score", student.groupTotalScore);
                                                        } else {
                                                            btn->setProperty("group_total_score", student.groupTotalScore);
                                                        }
                                                    }
                                                    
                                                    qDebug() << "已为学生" << studentName << "(" << studentId << ")设置成绩属性，属性数量:" << student.attributes.size();
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                                qDebug() << "已将成绩数据设置到座位表对应单元格学生的属性中";
                            }

                            // 串行拉取：student-scores 成功后再拉 group-scores，避免并发导致“小组表偶发拉不下来/被覆盖”
                            if (isStudentScoresResp && m_scoresNeedFetchGroupAfterStudent && !m_scoresPendingGroupUrl.isEmpty()) {
                                const QString nextUrl = m_scoresPendingGroupUrl;
                                m_scoresNeedFetchGroupAfterStudent = false;
                                m_scoresPendingGroupUrl.clear();
                                qDebug() << "学生成绩表拉取完成，继续请求小组成绩表:" << nextUrl;
                                if (m_httpHandler) {
                                    m_httpHandler->get(nextUrl);
                                }
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
								
								// 获取 is_voice_enabled 字段
								bool is_voice_enabled = false;
								if (memberObj.contains("is_voice_enabled")) {
									is_voice_enabled = memberObj["is_voice_enabled"].toInt();
								}

								// 解析 teach_subjects（任教科目）
								QStringList teachSubjects;
								if (memberObj.contains("teach_subjects") && memberObj["teach_subjects"].isArray()) {
									QJsonArray subjectArr = memberObj["teach_subjects"].toArray();
									for (const auto& v : subjectArr) {
										QString s = v.toString().trimmed();
										if (!s.isEmpty()) teachSubjects.append(s);
									}
								}
								
								GroupMemberInfo groupMemInfo;
								groupMemInfo.member_id = member_id;
								groupMemInfo.member_name = member_name;
								groupMemInfo.member_role = member_role;
								groupMemInfo.is_voice_enabled = is_voice_enabled;
								groupMemInfo.teach_subjects = teachSubjects;
								m_groupMemberInfo.append(groupMemInfo);
							}

							// 设置到 m_groupInfo 对话框的好友列表
							if (m_groupInfo)
							{
								m_groupInfo->InitGroupMember(group_id, m_groupMemberInfo);
								
								// 解析群组设置字段并更新UI
								if (dataObj.contains("group_info") && dataObj["group_info"].isObject()) {
									QJsonObject groupInfoObj = dataObj["group_info"].toObject();
									int receiveNotification = groupInfoObj.value("receive_notification").toInt(0);
									int linkTodaySchedule = groupInfoObj.value("link_today_schedule").toInt(0);
									int enableIntercom = groupInfoObj.value("enable_intercom").toInt(0);
									int linkHomework = groupInfoObj.value("link_homework").toInt(0);
									int linkPreClassPreparation = groupInfoObj.value("link_pre_class_preparation").toInt(0);
									
									m_groupInfo->setGroupSettings(receiveNotification, linkTodaySchedule, enableIntercom,
																  linkHomework, linkPreClassPreparation);
									
									// 发出信号通知父窗口群组设置已更新
									emit groupSettingsUpdated();
								}
							}
							
							// 根据当前用户的 is_voice_enabled 更新对讲按钮状态
							updateBtnTalkState();
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
		
		// 连接WebSocket消息信号，用于接收创建班级群和临时房间的消息
		if (m_pWs) {
				connect(m_pWs, &TaQTWebSocket::newMessage, this, [this](const QString& msg) {
					// 解析JSON消息
					QJsonParseError parseError;
					QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &parseError);
					if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
						return; // 不是JSON对象，忽略
					}
					
					QJsonObject rootObj = doc.object();
					QString type = rootObj.value(QStringLiteral("type")).toString();
					
					// 处理创建班级群消息（type: "3"），服务器会自动创建临时房间
					if (type == QStringLiteral("3")) {
						QString groupId = rootObj.value(QStringLiteral("group_id")).toString();
						QString groupName = rootObj.value(QStringLiteral("groupname")).toString();
						
						// 检查是否是当前群组
						if (groupId == m_unique_group_id) {
							qDebug() << "收到创建班级群消息，群组ID:" << groupId << "，群组名称:" << groupName;
							
							// 解析临时房间信息
							if (rootObj.contains(QStringLiteral("temp_room")) && rootObj.value(QStringLiteral("temp_room")).isObject()) {
								QJsonObject tempRoom = rootObj.value(QStringLiteral("temp_room")).toObject();
								
								m_roomId = tempRoom.value(QStringLiteral("room_id")).toString();
								m_whipUrl = tempRoom.value(QStringLiteral("whip_url")).toString();
								m_whepUrl = tempRoom.value(QStringLiteral("whep_url")).toString();
								m_streamName = tempRoom.value(QStringLiteral("stream_name")).toString();
								
								qDebug() << "临时房间信息已更新：";
								qDebug() << "  房间ID:" << m_roomId;
								qDebug() << "  推流地址:" << m_whipUrl;
								qDebug() << "  拉流地址:" << m_whepUrl;
								qDebug() << "  流名称:" << m_streamName;
								
								// 如果页面已打开，开始拉流（注释：拉流在HTML页面上进行）
								//if (!m_whepUrl.isEmpty() && !m_isPullingStream) {
								//	qDebug() << "房间创建成功，开始拉流，拉流地址:" << m_whepUrl;
								//	startPullStream();
								//}
							}
						}
					}
					// 兼容旧格式：处理room_created消息
					else if (type == QStringLiteral("room_created") || type == QStringLiteral("6")) {
						QString roomId = rootObj.value(QStringLiteral("room_id")).toString();
						if (!roomId.isEmpty()) {
							m_roomId = roomId;
							qDebug() << "收到房间创建成功消息，房间ID:" << m_roomId;
							
							// 如果消息中包含拉流地址，也尝试拉流（注释：拉流在HTML页面上进行）
							//QString whepUrl = rootObj.value(QStringLiteral("whep_url")).toString();
							//if (!whepUrl.isEmpty()) {
							//	m_whepUrl = whepUrl;
							//	if (!m_isPullingStream) {
							//		qDebug() << "收到拉流地址，开始拉流:" << m_whepUrl;
							//		startPullStream();
							//	}
							//}
						}
					}
					// 注意：家庭作业消息已通过 FriendGroupDialog 的 homeworkReceived 信号处理并缓存
					// 当 ScheduleDialog 创建时，会从 FriendGroupDialog 传递缓存的作业数据
					// 注意：通知消息已通过 FriendGroupDialog 的 notificationReceived 信号处理并缓存
					// 当 ScheduleDialog 创建时，会从 FriendGroupDialog 传递缓存的通知数据
				});
			}
			
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
				QString currentUserId = userInfo.classId;
				
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
					
					// 根据当前用户的 is_voice_enabled 更新对讲按钮状态
					updateBtnTalkState();
					
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
		QHBoxLayout* topLayout = new QHBoxLayout(this);
		m_lblAvatar = new ClickableLabel(this); // 使用ClickableLabel，支持点击编辑
		m_lblAvatar->setFixedSize(50, 50);
		m_lblAvatar->setAlignment(Qt::AlignCenter); // 文字居中
		m_lblAvatar->setStyleSheet("background-color: #4169E1; color: white; border:1px solid #4169E1; text-align:center; font-size:14px; font-weight:bold; border-radius: 8px;");
		m_lblAvatar->setCursor(Qt::PointingHandCursor); // 设置鼠标样式为手型
		QFont font = m_lblAvatar->font();
		font.setBold(true);
		font.setWeight(QFont::Bold);
		m_lblAvatar->setFont(font);

		m_lblClass = new ClickableLabel(this);
		m_lblClass->setText(""); // 设置初始文本
		m_lblClass->setCursor(Qt::PointingHandCursor); // 设置鼠标样式为手型
		
		// 在m_lblClass右上角添加编辑图标
		QLabel* editIcon = new QLabel("✎", m_lblClass);
		editIcon->setStyleSheet("color: #888888; font-size: 12px; background: transparent;");
		editIcon->setAlignment(Qt::AlignCenter);
		editIcon->setFixedSize(16, 16);
		editIcon->setAttribute(Qt::WA_TransparentForMouseEvents, true); // 让图标不拦截鼠标事件
		
		// 定位编辑图标到右上角的函数
		auto updateEditIconPos = [editIcon]() {
			QWidget* parent = editIcon->parentWidget();
			if (parent && parent->width() > 0) {
				int iconSize = 16;
				int margin = 2; // 距离边缘的间距
				editIcon->move(parent->width() - iconSize - margin, margin);
			}
		};
		
		// 使用事件过滤器来监听m_lblClass的resize和show事件
		class ResizeEventFilter : public QObject {
		public:
			std::function<void()> updateFunc;
			ResizeEventFilter(std::function<void()> func, QObject* parent) : QObject(parent), updateFunc(func) {}
			bool eventFilter(QObject* obj, QEvent* event) override {
				if ((event->type() == QEvent::Resize || event->type() == QEvent::Show) && updateFunc) {
					updateFunc();
				}
				return QObject::eventFilter(obj, event);
			}
		};
		m_lblClass->installEventFilter(new ResizeEventFilter(updateEditIconPos, m_lblClass));
		
		// 初始定位（延迟执行以确保m_lblClass已初始化）
		QTimer::singleShot(100, this, updateEditIconPos);

		// 班级群功能按钮（普通群不显示）
		QPushButton* btnSeat = new QPushButton("座次表", this);
		QPushButton* btnCam = new QPushButton("摄像头", this);
		btnTalk = new QPushButton("按住开始对讲", this);
		QPushButton* btnMsg = new QPushButton("通知", this);
		QPushButton* btnTask = new QPushButton("作业", this);
		QString greenStyle = "background-color: #2D2E2D; color: white; padding: 4px 8px; border: none;";
		btnSeat->setStyleSheet(greenStyle);
		btnCam->setStyleSheet(greenStyle);
		btnTalk->setStyleSheet(greenStyle);
		btnMsg->setStyleSheet(greenStyle);
		btnTask->setStyleSheet(greenStyle);
		btnSeat->setIcon(QIcon(":/res/img/class_card_ic_seating chart@2x.png"));
		btnCam->setIcon(QIcon(":/res/img/class_card_ic_camera@2x.png"));
		btnTalk->setIcon(QIcon(":/res/img/class_card_ic_intercom@2x.png"));
		btnMsg->setIcon(QIcon(":/res/img/class_card_ic_notice@2x.png"));
		btnTask->setIcon(QIcon(":/res/img/class_card_ic_school@2x.png"));
		
		// 保存按钮指针，用于根据群组类型显示/隐藏
		m_btnSeat = btnSeat;
		m_btnCam = btnCam;
		m_btnMsg = btnMsg;
		m_btnTask = btnTask;

		QPushButton* btnMore = new QPushButton("...", this);
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
					// 根据当前用户的 is_voice_enabled 更新对讲按钮状态
					updateBtnTalkState();
				} else {
					// 如果数据为空，尝试使用无参数版本（使用 QGroupInfo 自己的数据）
					m_groupInfo->InitGroupMember();
					// 根据当前用户的 is_voice_enabled 更新对讲按钮状态
					updateBtnTalkState();
				}
				m_groupInfo->show();
			}
			else if (m_groupInfo && !m_groupInfo->isHidden())
			{
				m_groupInfo->hide();
			}
		});

		// 为头像添加点击事件（编辑头像）
		connect(m_lblAvatar, &ClickableLabel::clicked, this, [=]() {
			// 弹出文件选择对话框选择图片
			QString fileName = QFileDialog::getOpenFileName(this, "选择头像图片", "", "图片文件 (*.png *.jpg *.jpeg *.bmp)");
			if (!fileName.isEmpty()) {
				uploadAvatar(fileName);
			}
		});

		// 为昵称标签添加点击事件（编辑昵称）
		connect(m_lblClass, &ClickableLabel::clicked, this, [=]() {
			// 弹出自定义编辑对话框
			QString currentNickname = m_lblClass ? m_lblClass->text() : m_groupName;
			EditNicknameDialog* editDlg = new EditNicknameDialog(currentNickname, this);
			if (editDlg->exec() == QDialog::Accepted) {
				QString newNickname = editDlg->getNickname();
				if (!newNickname.isEmpty() && newNickname != currentNickname) {
					updateGroupNickname(newNickname);
				}
			}
			editDlg->deleteLater();
		});

		topLayout->addWidget(m_lblAvatar);
		topLayout->addWidget(m_lblClass);
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
		
		// 连接通知按钮（班级端显示通知接收窗口）
		connect(btnMsg, &QPushButton::clicked, this, [this]() {
			// 判断是班级端还是教师端
			bool isClassSide = CommonInfo::GetClassLoginInfo().isLoggedIn();
			
			if (isClassSide) {
				// 班级端：显示通知接收窗口
				showNotificationViewDialog();
			}
		});

		// 时间 + 科目行
		QHBoxLayout* timeLayout = new QHBoxLayout(this);
		m_timeButtonStyle = "background-color: #2D2E2D; color: white; font-size:12px; min-width:40px;";
		m_subjectButtonStyle = "background-color: #2D2E2D; color: white; font-size:12px; min-width:50px;";

		QVBoxLayout* vTimes = new QVBoxLayout(this);
		m_timeRowLayout = new QHBoxLayout(this);
		m_subjectRowLayout = new QHBoxLayout(this);
		m_specialSubjectRowLayout = new QHBoxLayout(this);
		vTimes->addLayout(m_timeRowLayout);
		vTimes->addLayout(m_subjectRowLayout);
		QHBoxLayout* specialRowsLayout = new QHBoxLayout(this);
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

		QHBoxLayout* timeIndicatorLayout = new QHBoxLayout(this);
		timeIndicatorLayout->setSpacing(8);
		QLabel* lblArrow = new QLabel("↓", this);
		lblArrow->setStyleSheet("color: white; font-weight: bold;");
		QLabel* lblTime = new QLabel("12:10", this);
		lblTime->setAlignment(Qt::AlignCenter);
		lblTime->setFixedSize(60, 25);
		lblTime->setStyleSheet("background-color: pink; color:red; font-weight:bold;");
		timeIndicatorLayout->addWidget(lblArrow);
		timeIndicatorLayout->addWidget(lblTime);
		timeIndicatorLayout->addStretch();
		mainLayout->addLayout(timeIndicatorLayout);


		// ===== 新增中排按钮 =====
		QHBoxLayout* middleBtnLayout = new QHBoxLayout(this);
		//QString greenStyle = "background-color: green; color: white; padding: 4px 8px;";

		QPushButton* btnRandom = new QPushButton("随机点名", this);
		QPushButton* btnAnalyse = new QPushButton("分断", this);
		QPushButton* btnHeatmap = new QPushButton("热力图", this);
		QPushButton* btnArrange = new QPushButton("排座", this);
		QPushButton* btnImportSeat = new QPushButton("导入学生信息", this);
		QPushButton* btnMoreBottom = new QPushButton("...", this);
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

		btnRandom->setIcon(QIcon(":/res/img/class_card_ic_random@2x.png"));
		btnAnalyse->setIcon(QIcon(":/res/img/class_card_ic_breaking@2x.png"));
		btnHeatmap->setIcon(QIcon(":/res/img/class_card_ic_hot@2x.png"));
		btnArrange->setIcon(QIcon(":/res/img/class_card_ic_seat@2x.png"));

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
		QPushButton* btnHomeworkView = new QPushButton("作业", this);
		btnHomeworkView->setStyleSheet(greenStyle);
		// 需求：隐藏“作业”快捷按钮（功能保留，暂不对外展示）
		btnHomeworkView->hide();
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
		if (arrangeSeatDlg) {
			arrangeSeatDlg->setClassId(m_classid);
			arrangeSeatDlg->loadExcelFiles(m_classid);
			connect(arrangeSeatDlg, &ArrangeSeatDialog::arrangeRequested, this,
				[this](const QString& fileName, const QString& fieldName, const QString& mode, bool isGroup) {
					Q_UNUSED(fileName);
					arrangeSeatsByField(fieldName, mode, isGroup);
					uploadSeatTableToServer();
				});
		}

		// 连接排座按钮点击事件
		connect(btnArrange, &QPushButton::clicked, this, [=]() {
			if (arrangeSeatDlg && arrangeSeatDlg->isHidden()) {
				arrangeSeatDlg->setClassId(m_classid);
				arrangeSeatDlg->loadExcelFiles(m_classid);
				arrangeSeatDlg->show();
			} else if (arrangeSeatDlg && !arrangeSeatDlg->isHidden()) {
				arrangeSeatDlg->hide();
			} else {
				arrangeSeatDlg = new ArrangeSeatDialog(this);
				arrangeSeatDlg->setClassId(m_classid);
				arrangeSeatDlg->loadExcelFiles(m_classid);
				connect(arrangeSeatDlg, &ArrangeSeatDialog::arrangeRequested, this,
					[this](const QString& fileName, const QString& fieldName, const QString& mode, bool isGroup) {
						Q_UNUSED(fileName);
						arrangeSeatsByField(fieldName, mode, isGroup);
						uploadSeatTableToServer();
					});
				arrangeSeatDlg->show();
			}
		});
		
		// 连接随机点名按钮点击事件
		connectRandomCallButton(btnRandom, seatTable);
		
		// 连接导入学生信息按钮点击事件
		connect(btnImportSeat, &QPushButton::clicked, this, [=]() {
			importSeatTable();
		});
		
		// 连接分断按钮点击事件
		connect(btnAnalyse, &QPushButton::clicked, this, [=]() {
			// 检查是否有学生数据（必须上传期中成绩）
			if (m_students.isEmpty()) {
				QMessageBox::information(this, "提示", "请先上传期中成绩表！");
				return;
			}
			this->showSegmentDialog();
		});
		
		// 连接热力图按钮点击事件
		connect(btnHeatmap, &QPushButton::clicked, this, [=]() {
			// 检查是否有学生数据（必须上传期中成绩）
			if (m_students.isEmpty()) {
				QMessageBox::information(this, "提示", "请先上传期中成绩表！");
				return;
			}
			this->showGradientHeatmap();
		});

		// ===== 讲台区域 =====
		QHBoxLayout* podiumLayout = new QHBoxLayout(this);
		QPushButton* btnPodium = new QPushButton("讲台", this);
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
		seatTable = new QTableWidget(8, 11, this); // 8行，11列（包含过道列）
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
		// 加载图标并裁剪为只显示左边到中间的一半
		QPixmap originalPixmap("./res/img/class_ic_seat@2x.png");
		QPixmap croppedPixmap = originalPixmap.copy(0, 0, originalPixmap.width() / 2 - 9, originalPixmap.height());
		QIcon seatIcon(croppedPixmap);
		for (int row = 0; row < 8; ++row) {
			for (int col = 0; col < 11; ++col) {
				QPushButton* btn = new QPushButton("", this);
				// 不在初始化时设置图标，只有座位按钮才设置图标
				btn->setStyleSheet(
					"QPushButton { "
					"background-color: #dc3545; "
					"color: white; "
					"border: 1px solid #ccc; "
					"border-radius: 4px; "
					"padding: 5px; "
					"font-size: 12px; "
					"text-align: center; "
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
				
				// 安装事件过滤器以处理双击事件和拖拽事件
				btn->installEventFilter(this);
				btn->setProperty("seatRow", row);
				btn->setProperty("seatCol", col);
				
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
				btn->setIcon(seatIcon);
				btn->setIconSize(QSize(50, 50)); // 设置图标大小
			}
		}
		// 设置第1行列3座位（讲台左边）
		QPushButton* btnCol3 = qobject_cast<QPushButton*>(seatTable->cellWidget(0, 3));
		if (btnCol3) {
			btnCol3->setProperty("isSeat", true);
			btnCol3->setIcon(seatIcon);
			btnCol3->setIconSize(QSize(50, 50)); // 设置图标大小
		}
		// 设置第1行列7座位（讲台右边）
		QPushButton* btnCol7 = qobject_cast<QPushButton*>(seatTable->cellWidget(0, 7));
		if (btnCol7) {
			btnCol7->setProperty("isSeat", true);
			btnCol7->setIcon(seatIcon);
			btnCol7->setIconSize(QSize(50, 50)); // 设置图标大小
		}
		// 设置第1行右侧2个座位（列9-10）
		for (int col = 9; col < 11; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, col));
			if (btn) {
				btn->setProperty("isSeat", true);
				btn->setIcon(seatIcon);
				btn->setIconSize(QSize(50, 50)); // 设置图标大小
			}
		}
		seatTable->setSpan(0, 4, 1, 3); // 合并第1行的列2-8作为中央过道
		QPushButton* aisle1Btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, 2));
		if (aisle1Btn) {
			aisle1Btn->setEnabled(false);
			aisle1Btn->setIcon(QIcon()); // 移除图标
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
			aisle1Btn->setIcon(QIcon()); // 移除图标
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
					btn->setIcon(seatIcon);
					btn->setIconSize(QSize(50, 50)); // 设置图标大小
				}
			}
			
			// 过道1：列2（第一个走廊）
			QPushButton* aisle1Btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, 2));
			if (aisle1Btn) {
				aisle1Btn->setEnabled(false);
				aisle1Btn->setIcon(QIcon()); // 移除图标
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
					btn->setIcon(seatIcon);
					btn->setIconSize(QSize(50, 50)); // 设置图标大小
				}
			}
			
			// 过道2：列5（第二个走廊）
			QPushButton* aisle2Btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, 5));
			if (aisle2Btn) {
				aisle2Btn->setEnabled(false);
				aisle2Btn->setIcon(QIcon()); // 移除图标
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
					btn->setIcon(seatIcon);
					btn->setIconSize(QSize(50, 50)); // 设置图标大小
				}
			}
			
			// 过道3：列8（第三个走廊）
			QPushButton* aisle3Btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, 8));
			if (aisle3Btn) {
				aisle3Btn->setEnabled(false);
				aisle3Btn->setIcon(QIcon()); // 移除图标
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
					btn->setIcon(seatIcon);
					btn->setIconSize(QSize(50, 50)); // 设置图标大小
				}
			}
		}
		
		// 第1行的座位（这部分代码与前面重复，但保留以确保一致性）
		// 左侧2个座位：列0-1
		for (int col = 0; col < 2; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, col));
			if (btn) {
				btn->setText("");
				btn->setProperty("isSeat", true);
				// 图标已在前面设置，这里不需要重复设置
			}
		}
		// 右侧2个座位：列9-10
		for (int col = 9; col < 11; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(0, col));
			if (btn) {
				btn->setText("");
				btn->setProperty("isSeat", true);
				// 图标已在前面设置，这里不需要重复设置
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
		QHBoxLayout* seatTableLayout = new QHBoxLayout(this);
		seatTableLayout->addStretch();
		seatTableLayout->addWidget(seatTable);
		seatTableLayout->addStretch();
		mainLayout->addLayout(seatTableLayout);

		// 红框消息输入栏
		QHBoxLayout* inputLayout = new QHBoxLayout(this);

		QPushButton* btnVoice = new QPushButton("🔊", this);
		btnVoice->setFixedSize(30, 30);

		QLineEdit* editMessage = new QLineEdit(this);
		editMessage->setPlaceholderText("请输入消息...");
		editMessage->setMinimumHeight(30);
		editMessage->setEnabled(false);

		QPushButton* btnEmoji = new QPushButton("😊", this);
		btnEmoji->setFixedSize(30, 30);

		QPushButton* btnPlus = new QPushButton("➕", this);
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
		QLabel* lblNum = new QLabel("3", this);
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
		if (m_unique_group_id.isEmpty()) {
			qWarning() << "群组ID为空，无法上传头像";
			QMessageBox::warning(this, "错误", "群组ID为空，无法上传头像");
			return;
		}

		// ===== 1. 读取头像图片 =====
		QFile file(filePath);  // 本地头像路径
		if (!file.open(QIODevice::ReadOnly)) {
			qDebug() << "Failed to open image file.";
			QMessageBox::warning(this, "错误", "无法打开图片文件");
			return;
		}
		QByteArray imageData = file.readAll(); // 二进制数据
		file.close();

		// ===== 2. 图片转 Base64 =====
		QString imageBase64 = QString::fromLatin1(imageData.toBase64());
		// 添加 data:image 前缀（如果需要）
		if (!imageBase64.startsWith("data:image")) {
			// 根据文件扩展名判断图片类型
			QString suffix = QFileInfo(filePath).suffix().toLower();
			QString mimeType = "image/png";
			if (suffix == "jpg" || suffix == "jpeg") {
				mimeType = "image/jpeg";
			} else if (suffix == "gif") {
				mimeType = "image/gif";
			} else if (suffix == "bmp") {
				mimeType = "image/bmp";
			}
			imageBase64 = QString("data:%1;base64,%2").arg(mimeType).arg(imageBase64);
		}

		// ===== 3. 构造 JSON 数据 =====
		QMap<QString, QString> params;
		params["avatar"] = imageBase64;
		params["unique_group_id"] = m_unique_group_id;
		
		if (m_httpHandler)
		{
			// 创建临时HTTP处理器用于接收响应
			TAHttpHandler* avatarHandler = new TAHttpHandler(this);
			connect(avatarHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
				qDebug() << "上传头像响应:" << responseString;
				QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
				if (jsonDoc.isObject()) {
					QJsonObject obj = jsonDoc.object();
					if (obj.contains("data") && obj["data"].isObject()) {
						QJsonObject dataObj = obj["data"].toObject();
						int code = dataObj["code"].toInt();
						QString message = dataObj["message"].toString();
						
						if (code == 200) {
							QString faceUrl = dataObj["face_url"].toString();
							if (!faceUrl.isEmpty()) {
								// 更新头像显示
								updateAvatarDisplay(faceUrl);
								QMessageBox::information(this, "成功", "头像更新成功");
							} else {
								QMessageBox::information(this, "成功", message);
							}
						} else {
							QMessageBox::warning(this, "错误", QString("上传头像失败: %1").arg(message));
						}
					}
				}
				avatarHandler->deleteLater();
			});
			connect(avatarHandler, &TAHttpHandler::failed, this, [=](const QString& errorString) {
				qWarning() << "上传头像失败:" << errorString;
				QMessageBox::warning(this, "错误", QString("上传头像失败: %1").arg(errorString));
				avatarHandler->deleteLater();
			});
			
			avatarHandler->post(QString("http://47.100.126.194:5000/groups/update-avatar"), params);
		}
	}

	void updateGroupNickname(const QString& nickname)
	{
		if (m_unique_group_id.isEmpty()) {
			qWarning() << "群组ID为空，无法更新昵称";
			QMessageBox::warning(this, "错误", "群组ID为空，无法更新昵称");
			return;
		}

		// 构造 JSON 数据
		QMap<QString, QString> params;
		params["unique_group_id"] = m_unique_group_id;
		params["nickname"] = nickname.trimmed(); // 去除首尾空格
		
		if (m_httpHandler)
		{
			// 创建临时HTTP处理器用于接收响应
			TAHttpHandler* nicknameHandler = new TAHttpHandler(this);
			connect(nicknameHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
				qDebug() << "更新昵称响应:" << responseString;
				QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
				if (jsonDoc.isObject()) {
					QJsonObject obj = jsonDoc.object();
					if (obj.contains("data") && obj["data"].isObject()) {
						QJsonObject dataObj = obj["data"].toObject();
						int code = dataObj["code"].toInt();
						QString message = dataObj["message"].toString();
						
						if (code == 200) {
							// 更新界面显示
							if (m_lblClass) {
								m_lblClass->setText(nickname.trimmed());
								m_groupName = nickname.trimmed();
							}
							// 显示Toast提示（屏幕右下角）
							ToastNotification* toast = new ToastNotification("昵称更新成功", this);
							toast->show();
						} else {
							QMessageBox::warning(this, "错误", QString("更新昵称失败: %1").arg(message));
						}
					}
				}
				nicknameHandler->deleteLater();
			});
			connect(nicknameHandler, &TAHttpHandler::failed, this, [=](const QString& errorString) {
				qWarning() << "更新昵称失败:" << errorString;
				QMessageBox::warning(this, "错误", QString("更新昵称失败: %1").arg(errorString));
				nicknameHandler->deleteLater();
			});
			
			nicknameHandler->post(QString("http://47.100.126.194:5000/groups/update-nickname"), params);
		}
	}

	void updateAvatarDisplay(const QString& avatarUrl)
	{
		if (!m_lblAvatar) return;
		
		// 如果有头像URL，尝试加载并显示图片
		if (!avatarUrl.isEmpty()) {
			QPixmap pixmap;
			bool loaded = false;
			
			// 1. 尝试作为本地文件路径加载
			if (QFile::exists(avatarUrl)) {
				loaded = pixmap.load(avatarUrl);
			}
			// 2. 尝试作为HTTP/HTTPS URL下载
			else if (avatarUrl.startsWith("http://") || avatarUrl.startsWith("https://")) {
				if (m_networkManager) {
					QUrl url(avatarUrl);
					QNetworkRequest request(url);
					QNetworkReply* reply = m_networkManager->get(request);
					// 注意：这里使用同步方式可能阻塞，实际应该使用异步
					// 为了简化，先使用异步下载，但暂时不等待结果
					// 后续可以通过信号槽更新
					connect(reply, &QNetworkReply::finished, this, [this, reply]() {
						if (reply->error() == QNetworkReply::NoError) {
							QByteArray imageData = reply->readAll();
							QPixmap pixmap;
							if (pixmap.loadFromData(imageData)) {
								pixmap = pixmap.scaled(m_lblAvatar->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
								m_lblAvatar->setPixmap(pixmap);
								m_lblAvatar->setText(""); // 清空文字
							}
						}
						reply->deleteLater();
					});
					return; // 异步下载，先返回
				}
			}
			// 3. 尝试作为base64数据加载
			else {
				// 检查是否是base64数据（可能包含data:image前缀）
				QString base64Data = avatarUrl;
				if (base64Data.contains("base64,")) {
					base64Data = base64Data.section("base64,", 1);
				}
				QByteArray imageData = QByteArray::fromBase64(base64Data.toLatin1());
				if (!imageData.isEmpty()) {
					loaded = pixmap.loadFromData(imageData);
				}
			}
			
			// 如果加载成功，显示图片
			if (loaded && !pixmap.isNull()) {
				// 缩放图片以适应标签大小，保持宽高比
				pixmap = pixmap.scaled(m_lblAvatar->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
				m_lblAvatar->setPixmap(pixmap);
				m_lblAvatar->setText(""); // 清空文字
			}
			// 如果加载失败，保持原有的文字显示方式
		}
		// 如果URL为空，保持原有的文字显示方式
	}

	void InitWebSocket()
	{
		if (m_chatDlg)
		{
			m_chatDlg->InitWebSocket();
		}
	}

	void setPrepareClassHistory(const QJsonArray& history);
	
	// 设置作业数据（从 FriendGroupDialog 传递）
	void setHomeworkData(const QString& dateStr, const QString& subject, const QString& content);
	
	// 设置通知数据（从 FriendGroupDialog 传递）
	void setNotificationData(const QList<NotificationItem>& notifications);
	// 添加通知（从 FriendGroupDialog 的信号中接收）
	void addNotification(const QString& content, const QString& senderName = QString(), const QString& avatarUrl = QString());

	void setNoticeMsg(QList<Notification> listNoticeMsg)
	{
		if (m_chatDlg)
		{
			m_chatDlg->setNoticeMsg(listNoticeMsg);
		}
	}

signals:
	void groupLeft(const QString& groupId); // 群聊退出信号，通知父窗口刷新群列表
	void groupSettingsUpdated(); // 群组设置更新信号，通知父窗口更新相关UI

public:
	// 排座功能：根据学生数据自动排座
	// method: "随机排座", "正序", "倒序", "2人组排座", "4人组排座", "6人组排座"
	void arrangeSeats(const QList<StudentInfo>& students, const QString& method = "随机排座");
	double extractStudentValue(const StudentInfo& stu, const QString& field) const;
	QVector<StudentInfo> groupWheelAssign(QVector<StudentInfo> sorted, int groupSize);
	void arrangeSeatsByField(const QString& fieldName, const QString& modeText, bool isGroupMode);
	void importSeatTable(); // 导入座位表格
	void importSeatFromExcel(const QString& filePath); // 从Excel导入座位表
	void importSeatFromCsv(const QString& filePath); // 从CSV导入座位表
	void fillSeatTableFromData(const QList<QStringList>& dataRows); // 将数据填充到座位表
	void uploadSeatTableToServer(); // 上传座位表到服务器
	// 设置座位按钮的文本和图标：有文本时不显示图标，无文本时显示图标
	void setSeatButtonTextAndIcon(QPushButton* btn, const QString& text);
	void fetchSeatArrangementFromServer(); // 从服务器获取座位表
	void swapSeats(QPushButton* seat1, QPushButton* seat2); // 交换两个座位的内容
	void resetDragState(); // 重置拖拽状态
	void highlightSeat(QPushButton* btn, bool highlight); // 高亮或取消高亮座位
	QPushButton* findSeatAtPosition(const QPoint& globalPos); // 根据全局坐标查找座位按钮
	void fetchStudentScoresFromServer(); // 从服务器获取成绩表数据
	void fetchCourseScheduleForDailyView();
	void downloadExcelFile(const QString& url, const QString& saveDir, const QString& filename, std::function<void()> onCompleted = nullptr); // 下载Excel文件
	void writeLogToFile(const QString& message); // 写入日志到文件
	void showStudentAttributeDialog(int row, int col); // 显示学生属性对话框
	
	// 获取座位信息列表（用于比对和更新姓名）
	QList<SeatInfo> getSeatInfoList() const { return m_seatInfoList; }
	
	// 热力图相关方法
	void showSegmentDialog(); // 显示分段区间设置对话框
	void refreshHeatmapOptionsIfNeeded(); // 如果需要，刷新热力图选项（在实现文件中定义，避免前向声明问题）
	void showGradientHeatmap(); // 显示渐变热力图
	void setSegments(const QList<struct SegmentRange>& segments); // 设置分段区间
	void buildHeatmapOptions(QStringList& tables, QStringList& attributes); // 构建热力图下拉选项
	QStringList getAttributesForTable(const QString& tableName); // 根据表格名称获取属性列表
	QStringList getHeadersFromFile(const QFileInfo& fi); // 从文件中读取表头（辅助函数）
	
	void InitData(QString groupName, QString unique_group_id, QString classid, bool iGroupOwner, bool isClassGroup = true)
	{
		m_groupName = groupName;
		m_unique_group_id = unique_group_id;
		m_classid = classid;
		m_iGroupOwner = iGroupOwner;
		m_isClassGroup = isClassGroup; // 保存群组类型
		// 说明：这里不需要重置成绩表请求标记。
		// fetchStudentScoresFromServer 内部用 (class_id|term) 做幂等去重：
		// - 同 key 重复调用会直接 return
		// - key 变化会自动触发新一轮请求并更新 key
		// 如果 InitData 被重复调用（比如打开窗口时被多处触发），在这里重置反而会导致重复请求。
		
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
		
		// 从班级群名称中提取班级信息（如"一年级三班的班级群" -> "三班"）
		if (m_lblAvatar && isClassGroup && !groupName.isEmpty()) {
			QString classText = groupName;
			// 查找"年级"和"班"字的位置
			int nianjiIndex = classText.indexOf(QString::fromUtf8(u8"年级"));
			int banIndex = classText.indexOf(QString::fromUtf8(u8"班"));
			
			if (nianjiIndex >= 0 && banIndex > nianjiIndex) {
				// 如果包含"年级"且"班"字在"年级"之后，取"年级"后面到"班"字之前的内容+"班"
				// 例如："一年级三班的班级群" -> "三班"
				// "年级"占2个字符，所以从 nianjiIndex + 2 开始，取 banIndex - (nianjiIndex + 2) 个字符
				QString afterNianji = classText.mid(nianjiIndex + 2, banIndex - nianjiIndex - 2); // "年级"后面到"班"字之前（不包含"班"字）
				classText = afterNianji + QString::fromUtf8(u8"班");
			} else if (banIndex >= 0) {
				// 如果不包含"年级"但包含"班"字，取"班"字前一个字符+"班"
				if (banIndex > 0) {
					classText = classText.mid(banIndex - 1, 2); // 取"班"字前一个字符和"班"字
				} else {
					// 如果"班"字在开头，只取"班"字
					classText = QString::fromUtf8(u8"班");
				}
			} else {
				// 如果没有"班"字，使用原名称
				classText = groupName;
			}
			m_lblAvatar->setText(classText);
		}
		
		// 检查是否已经有下载的头像文件，如果有则显示
		// 头像文件保存在 group_images/{groupId}/ 目录下
		QString avatarDir = QCoreApplication::applicationDirPath() + "/group_images/" + unique_group_id;
		QDir dir(avatarDir);
		if (dir.exists()) {
			// 查找目录下的图片文件（png, jpg, jpeg等）
			QStringList filters;
			filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
			QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);
			if (!fileList.isEmpty()) {
				// 使用第一个找到的图片文件
				QString avatarPath = fileList.first().absoluteFilePath();
				updateAvatarDisplay(avatarPath);
			}
		}
		
		// ChatDialog 同时被班级群/普通群复用：这里是班级群场景，必须设置上下文
		m_chatDlg->setGroupContext(m_classid, isClassGroup);
		m_chatDlg->InitData(m_unique_group_id, iGroupOwner);
		UserInfo userInfo = CommonInfo::GetData();
		m_userId = userInfo.classId;
		m_userName = userInfo.strName;

		if (m_groupInfo)
		{
			m_groupInfo->initData(groupName, unique_group_id, m_iGroupOwner, classid);
			
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
		
		// 窗口初始化时拉取成绩表数据（内部带去重：同一个 class_id + term 只请求一次）
		if (m_httpHandler && isClassGroup && !classid.isEmpty()) {
			fetchStudentScoresFromServer();
		}

		// 班级群初始化时自动从服务器拉取座位表
		if (isClassGroup) {
			fetchSeatArrangementFromServer();
			fetchCourseScheduleForDailyView();
		}
		
		// 检查全局存储中是否已有临时房间信息（在创建班级群时保存的）
		// 这样即使 ScheduleDialog 在创建班级群时还没有打开，也能获取到临时房间信息
		if (!unique_group_id.isEmpty() && TempRoomStorage::hasTempRoomInfo(unique_group_id)) {
			TempRoomInfo tempRoomInfo = TempRoomStorage::getTempRoomInfo(unique_group_id);
			m_roomId = tempRoomInfo.room_id;
			m_whipUrl = tempRoomInfo.whip_url;
			m_whepUrl = tempRoomInfo.whep_url;
			m_streamName = tempRoomInfo.stream_name;
			
			qDebug() << "从全局存储中读取到临时房间信息：";
			qDebug() << "  群组ID:" << unique_group_id;
			qDebug() << "  房间ID:" << m_roomId;
			qDebug() << "  推流地址:" << m_whipUrl;
			qDebug() << "  拉流地址:" << m_whepUrl;
			qDebug() << "  流名称:" << m_streamName;
		}
		
		// 注意：临时房间现在由服务器在创建班级群时自动创建
		// 不再需要在这里调用 createTemporaryRoom()
		// 临时房间信息会通过 WebSocket 消息（type: "3"）返回，或者从全局存储中读取
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
	bool eventFilter(QObject* obj, QEvent* event) override
	{
		QPushButton* btn = qobject_cast<QPushButton*>(obj);
		if (!btn) {
			return QDialog::eventFilter(obj, event);
		}
		
		// 处理座位按钮的事件
		if (!btn->property("isSeat").toBool()) {
			return QDialog::eventFilter(obj, event);
		}
		
		// 处理座位按钮的双击事件
		if (event->type() == QEvent::MouseButtonDblClick) {
			int row = btn->property("seatRow").toInt();
			int col = btn->property("seatCol").toInt();
			showStudentAttributeDialog(row, col);
			return true;
		}
		
		// 处理鼠标按下事件 - 开始拖拽
		if (event->type() == QEvent::MouseButtonPress) {
			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			if (mouseEvent->button() == Qt::LeftButton) {
				m_dragSourceSeat = btn;
				m_isDragging = true;
				// 高亮显示起始座位
				highlightSeat(btn, true);
				return true;
			}
		}
		
		// 处理鼠标移动事件 - 跟踪拖拽目标
		if (event->type() == QEvent::MouseMove && m_isDragging && m_dragSourceSeat) {
			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			// 使用全局坐标查找鼠标下的座位按钮
			QPoint globalPos = QCursor::pos();
			QPushButton* targetBtn = findSeatAtPosition(globalPos);
			
			// 如果目标座位改变，更新高亮
			if (targetBtn != m_dragTargetSeat) {
				// 取消之前目标的高亮
				if (m_dragTargetSeat && m_dragTargetSeat != m_dragSourceSeat) {
					highlightSeat(m_dragTargetSeat, false);
				}
				// 高亮新目标
				m_dragTargetSeat = targetBtn;
				if (m_dragTargetSeat && m_dragTargetSeat != m_dragSourceSeat) {
					highlightSeat(m_dragTargetSeat, true);
				}
			}
		}
		
		// 处理鼠标释放事件 - 完成交换
		if (event->type() == QEvent::MouseButtonRelease) {
			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			if (mouseEvent->button() == Qt::LeftButton && m_isDragging && m_dragSourceSeat) {
				// 使用全局坐标查找释放位置的座位按钮
				QPoint globalPos = QCursor::pos();
				QPushButton* releaseBtn = findSeatAtPosition(globalPos);
				
				// 如果释放在不同的座位上，执行交换
				if (releaseBtn && releaseBtn != m_dragSourceSeat && releaseBtn->property("isSeat").toBool()) {
					swapSeats(m_dragSourceSeat, releaseBtn);
				}
				
				// 重置拖拽状态
				resetDragState();
				return true;
			}
		}
		
		return QDialog::eventFilter(obj, event);
	}
	
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
        painter.setBrush(QColor("#282A2B"));
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
	
	// 根据当前用户的 is_voice_enabled 更新对讲按钮状态
	void updateBtnTalkState() {
		if (!btnTalk) return;
		
		// 获取当前用户信息
		UserInfo userInfo = CommonInfo::GetData();
		QString currentUserId = userInfo.classId;
		
		// 在成员列表中查找当前用户
		bool isVoiceEnabled = false;
		for (const auto& member : m_groupMemberInfo) {
			if (member.member_id == currentUserId) {
				isVoiceEnabled = member.is_voice_enabled;
				break;
			}
		}
		
		// 根据 is_voice_enabled 设置按钮启用状态
		btnTalk->setEnabled(isVoiceEnabled);
		
		// 如果禁用，可以设置灰色样式提示用户
		if (!isVoiceEnabled) {
			btnTalk->setStyleSheet("background-color: #555555; color: #999999; padding: 4px 8px; border: none;");
			btnTalk->setToolTip("您没有开启语音权限，无法使用对讲功能");
		} else {
			// 恢复原来的样式
			btnTalk->setStyleSheet("background-color: #2D2E2D; color: white; padding: 4px 8px; border: none;");
			btnTalk->setToolTip("");
		}
		
		qDebug() << "更新对讲按钮状态，当前用户ID:" << currentUserId << "，is_voice_enabled:" << isVoiceEnabled;
	}

    void updateMask()
    {
        QPainterPath path;
        path.addRoundedRect(rect(), m_cornerRadius, m_cornerRadius);
        setMask(QRegion(path.toFillPolygon().toPolygon()));
    }
	CustomListDialog* customListDlg = NULL;
	ClickableLabel* m_lblClass = NULL; // 显示群昵称（可点击编辑）
	ClickableLabel* m_lblAvatar = NULL; // 显示头像（可点击编辑）
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
public:
	QGroupInfo* getGroupInfo() const { return m_groupInfo; } // 获取群信息对话框实例
	QPair<QString, QString> getNextClassInfo(const QTime& currentTime) const; // 获取下节课的科目和时间
	void showNextClassPrepareDialog(const QString& subject, const QString& time); // 显示下节课的课前准备
private:
	TAHttpHandler* m_httpHandler = NULL;
	QNetworkAccessManager* m_networkManager = nullptr; // 用于下载Excel文件
	// 成绩表拉取去重：同一个 class_id + term 在同一窗口生命周期只请求一次
	bool m_scoresFetchTriggered = false;
	QString m_scoresFetchKey;
	// 成绩表串行链路：先拉 student-scores，成功后再拉 group-scores
	bool m_scoresNeedFetchGroupAfterStudent = false;
	QString m_scoresPendingGroupUrl;
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
	QPushButton* m_dragSourceSeat = nullptr; // 拖拽起始座位
	QPushButton* m_dragTargetSeat = nullptr; // 拖拽目标座位
	bool m_isDragging = false; // 是否正在拖拽
	// 分别缓存两套来源的数据，避免 /student-scores 与 /group-scores 相互覆盖
	QList<StudentInfo> m_studentsStudentScores; // /student-scores
	QList<StudentInfo> m_studentsGroupScores;   // /group-scores
	QList<SeatInfo> m_seatInfoList; // 座位表信息列表
	
	// 热力图相关
	class HeatmapSegmentDialog* heatmapSegmentDlg = nullptr; // 分段区间对话框
	class HeatmapViewDialog* heatmapViewDlg = nullptr; // 热力图显示窗口
	QList<struct SegmentRange> m_segments; // 分段区间列表
	int m_heatmapType = 1; // 1=分段，2=渐变
	QString m_selectedAttribute; // 选定的属性名称（用于分段图）
	QString m_selectedTable; // 选定的表格名称（Excel文件名，用于分段图）
	
	// 随机点名相关
	class RandomCallDialog* randomCallDlg = nullptr; // 随机点名对话框
	void connectRandomCallButton(QPushButton* btnRandom, QTableWidget* seatTable); // 连接随机点名按钮
	
	// 作业相关
	class HomeworkEditDialog* homeworkEditDlg = nullptr; // 编辑作业对话框
	class HomeworkViewDialog* homeworkViewDlg = nullptr; // 展示作业对话框
	void connectHomeworkButton(QPushButton* btnTask); // 连接作业按钮（教师端）
	void showHomeworkViewDialog(); // 显示作业展示窗口（班级端）
	// 作业缓存：按日期聚合（当前ScheduleDialog实例对应一个群）
	QMap<QString, QMap<QString, QString>> m_homeworkByDate; // date(yyyy-MM-dd) -> (subject -> content)
	
	// 通知接收相关（班级端）
	class NotificationViewDialog* notificationViewDlg = nullptr; // 通知接收对话框
	QList<NotificationItem> m_notificationCache; // 通知缓存列表
	void showNotificationViewDialog(); // 显示通知接收窗口
	
	// 课前准备相关
	void onSubjectButtonClicked(); // 科目按钮点击事件
	void showPrepareClassDialog(const QString& subject, const QString& time = QString()); // 显示课前准备对话框
	void sendPrepareClassContent(const QString& subject, const QString& content, const QString& time = QString()); // 发送课前准备内容
	QString prepareClassCacheKey(const QString& subject, const QString& time) const;
	class PrepareClassEditDialog* prepareClassViewDlg = nullptr; // 课前准备查看窗口（复用显示/隐藏）
	QMap<QString, QString> m_prepareClassCache; // 课前准备内容缓存（科目|时间 -> 内容）
	QJsonArray m_prepareClassHistoryData; // 课前准备历史原始数据

	// 对讲房间相关
	QString m_roomId; // 临时房间ID
	QString m_whipUrl; // 推流地址
	QString m_whepUrl; // 拉流地址
	QString m_streamName; // 流名称
	
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
	
	// 创建临时房间
	void createTemporaryRoom();
	
	// 开始拉流（WHEP）
	void startPullStream();
	
	// 停止拉流
	void stopPullStream();
	
	// 拉流相关成员变量
	QMediaPlayer* m_pullStreamPlayer = nullptr;
	bool m_isPullingStream = false;
};

// 设置座位按钮的文本和图标：有文本时不显示图标，无文本时显示图标
inline void ScheduleDialog::setSeatButtonTextAndIcon(QPushButton* btn, const QString& text)
{
	if (!btn) return;
	
	btn->setText(text);
	
	// 如果有文本，不显示图标；如果没有文本，显示图标
	if (!text.isEmpty()) {
		btn->setIcon(QIcon()); // 移除图标
	} else {
		// 如果没有文本，显示图标（仅对座位按钮）
		if (btn->property("isSeat").toBool()) {
			QPixmap originalPixmap("./res/img/class_ic_seat@2x.png");
			QPixmap croppedPixmap = originalPixmap.copy(0, 0, originalPixmap.width() / 2 - 9, originalPixmap.height());
			QIcon seatIcon(croppedPixmap);
			btn->setIcon(seatIcon);
			btn->setIconSize(QSize(50, 50));
		}
	}
}

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
			btn->setProperty("studentId", QVariant(student.id)); // 设置学号属性
			btn->setProperty("studentName", QVariant(student.name));
			// 使用辅助函数设置文本和图标
			setSeatButtonTextAndIcon(btn, student.name);
			qDebug() << "分配座位:" << student.name << "到按钮";
			studentIndex++;
		} else {
			btn->setProperty("studentId", QVariant(""));
			btn->setProperty("studentName", QVariant(""));
			// 使用辅助函数设置文本和图标（空文本会显示图标）
			setSeatButtonTextAndIcon(btn, "");
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
				btn->setProperty("studentId", QVariant());
				btn->setProperty("studentName", QVariant());
				// 使用辅助函数设置文本和图标（空文本会显示图标）
				setSeatButtonTextAndIcon(btn, "");
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
			// 支持格式示例：
			// - 任崇庆13-3          => name=任崇庆, id=13-3
			// - 王文1-5/7           => name=王文,   id=7   （/ 后为学号）
			// - 王文1-5             => name=王文,   id=1-5 （兜底：无 / 时取数字-数字）
			QString s = text.trimmed();
			QString idFromSlash;

			// 先解析最后一个 '/' 之后的学号
			const int slashIndex = s.lastIndexOf('/');
			if (slashIndex >= 0) {
				idFromSlash = s.mid(slashIndex + 1).trimmed();
				s = s.left(slashIndex).trimmed();
			}

			QString name;
			QString id;

			// 提取开头中文作为姓名（避免 "王文1" 这种误判）
			QRegExp nameRe("^([\\u4e00-\\u9fa5]+)");
			if (nameRe.indexIn(s) >= 0) {
				name = nameRe.cap(1);
			}

			// 学号优先取 / 后
			if (!idFromSlash.isEmpty()) {
				id = idFromSlash;
			} else {
				// 否则尝试提取数字-数字（旧模板兼容）
				QRegExp dashRe("(\\d+-\\d+)");
				if (dashRe.indexIn(s) >= 0) {
					id = dashRe.cap(1);
				}
			}

			// 兜底：按空格/短横分割
			if (name.isEmpty() || (id.isEmpty() && slashIndex < 0)) {
				QStringList parts = s.split(QRegExp("[\\s-]+"), Qt::SkipEmptyParts);
				if (name.isEmpty() && !parts.isEmpty()) {
					name = parts.first();
				}
				if (id.isEmpty() && parts.size() >= 2) {
					id = parts.last();
				}
			}

			if (name.isEmpty()) {
				name = text.trimmed();
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
						btn->setProperty("studentName", student.first);
						if (!student.second.isEmpty()) {
							btn->setProperty("studentId", student.second);
						}
						// 使用辅助函数设置文本和图标
						setSeatButtonTextAndIcon(btn, cellText);
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
						btn->setProperty("studentName", student.first);
						if (!student.second.isEmpty()) {
							btn->setProperty("studentId", student.second);
						}
						// 使用辅助函数设置文本和图标
						setSeatButtonTextAndIcon(btn, actualRowData[dataIndex]);
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
						btn->setProperty("studentName", student.first);
						if (!student.second.isEmpty()) {
							btn->setProperty("studentId", student.second);
						}
						// 使用辅助函数设置文本和图标
						setSeatButtonTextAndIcon(btn, actualRowData[dataIndex]);
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
						btn->setProperty("studentName", student.first);
						if (!student.second.isEmpty()) {
							btn->setProperty("studentId", student.second);
						}
						// 使用辅助函数设置文本和图标
						setSeatButtonTextAndIcon(btn, actualRowData[dataIndex]);
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
						btn->setProperty("studentName", student.first);
						if (!student.second.isEmpty()) {
							btn->setProperty("studentId", student.second);
						}
						// 使用辅助函数设置文本和图标
						setSeatButtonTextAndIcon(btn, actualRowData[dataIndex]);
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
					btn->setProperty("studentId", QVariant());
					btn->setProperty("studentName", QVariant());
					// 使用辅助函数设置文本和图标（空文本会显示图标）
					setSeatButtonTextAndIcon(btn, "");
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
			
			// 清空座位信息列表
			m_seatInfoList.clear();
			
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
				
				// 保存到结构体
				QString finalName = name.isEmpty() ? seatLabel : name;
				SeatInfo seatInfo(rowIdx, colIdx, finalName, studentId, seatLabel);
				m_seatInfoList.append(seatInfo);
				
				btn->setProperty("studentName", finalName);
				btn->setProperty("studentId", studentId);
				// 使用辅助函数设置文本和图标
				setSeatButtonTextAndIcon(btn, seatLabel);
				btn->update();
				filled++;
			}
			
		seatTable->update();
		seatTable->repaint();
		qDebug() << "从服务器加载座位表成功，填充" << filled << "个座位";
		
		// 获取座位表成功后，可选触发成绩表拉取：
		// 窗口初始化时 InitData 已经会拉一次，这里仅在“尚未触发过”时才补拉，避免重复请求。
		if (m_httpHandler && m_isClassGroup && !m_classid.isEmpty() && !m_scoresFetchTriggered) {
			fetchStudentScoresFromServer();
		}
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

// 从服务器获取成绩表数据
inline void ScheduleDialog::fetchStudentScoresFromServer()
{
	if (!m_httpHandler || !m_isClassGroup || m_classid.isEmpty()) {
		qWarning() << "HTTP处理器未初始化或班级ID为空，无法获取成绩表";
		return;
	}
	
	// 根据当前日期计算学期（统一用 currentTermString，避免多处拷贝逻辑）
	const QString term = currentTermString();

	// 去重：同一个 class_id + term 在同一窗口生命周期只拉一次，避免打开窗口时被不同路径触发两次
	const QString key = m_classid + "|" + term;
	if (m_scoresFetchTriggered && m_scoresFetchKey == key) {
		return;
	}
	m_scoresFetchTriggered = true;
	m_scoresFetchKey = key;
	
	// 1) 获取小组成绩表数据
	QUrl urlGroup("http://47.100.126.194:5000/group-scores");
	QUrlQuery queryGroup;
	queryGroup.addQueryItem("class_id", m_classid);
	//queryGroup.addQueryItem("exam_name", "期中考试");
	queryGroup.addQueryItem("term", term);
	urlGroup.setQuery(queryGroup);
	
	qDebug() << "正在请求小组成绩表:" << urlGroup.toString();

	// 2) 获取学生个人成绩表数据
	QUrl urlStudent("http://47.100.126.194:5000/student-scores");
	QUrlQuery queryStudent;
	queryStudent.addQueryItem("class_id", m_classid);
	//queryStudent.addQueryItem("exam_name", "期中考试");
	queryStudent.addQueryItem("term", term);
	urlStudent.setQuery(queryStudent);

	// 串行请求：先 student-scores，成功后再请求 group-scores（由 success 解析逻辑触发）
	m_scoresNeedFetchGroupAfterStudent = true;
	m_scoresPendingGroupUrl = urlGroup.toString();

	qDebug() << "正在请求学生成绩表:" << urlStudent.toString();
	m_httpHandler->get(urlStudent.toString());
}

// 显示学生属性对话框
inline void ScheduleDialog::showStudentAttributeDialog(int row, int col)
{
	if (!seatTable) {
		qWarning() << "座位表未初始化";
		return;
	}
	
	QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
	if (!btn || !btn->property("isSeat").toBool()) {
		return;
	}
	
	// 获取学生信息
	QString studentId = btn->property("studentId").toString();
	QString studentName = btn->property("studentName").toString();
	
	// 如果座位上没有学生，不显示对话框
	if (studentId.isEmpty() && studentName.isEmpty()) {
		return;
	}

	// 若按钮文本包含“姓名/学号”，补齐缺失信息
	QString btnText = btn->text().trimmed();
	if ((!btnText.isEmpty()) && (studentId.isEmpty() || studentName.isEmpty())) {
		int slashIndex = btnText.lastIndexOf('/');
		if (slashIndex >= 0) {
			QString maybeName = btnText.left(slashIndex).trimmed();
			QString maybeId = btnText.mid(slashIndex + 1).trimmed();
			if (studentName.isEmpty() && !maybeName.isEmpty()) studentName = maybeName;
			if (studentId.isEmpty() && !maybeId.isEmpty()) studentId = maybeId;
		} else {
			if (studentName.isEmpty()) studentName = btnText;
		}
	}
	
	// 合并两套来源：优先 group-scores，其次 student-scores（避免互相覆盖导致弹窗数据不全）
	StudentInfo student;
	auto findInList = [&](const QList<StudentInfo>& list) -> StudentInfo {
		if (!studentId.isEmpty()) {
			for (const auto& s : list) {
				if (s.id == studentId) return s;
			}
		}
		if (!studentName.isEmpty()) {
			for (const auto& s : list) {
				if (s.name == studentName) return s;
			}
		}
		return StudentInfo();
	};
	auto mergeStudent = [&](StudentInfo& base, const StudentInfo& extra) {
		// 基本信息
		if (base.id.isEmpty()) base.id = extra.id;
		if (base.name.isEmpty()) base.name = extra.name;
		if (base.groupName.isEmpty()) base.groupName = extra.groupName;
		if (base.groupTotalScore <= 0 && extra.groupTotalScore > 0) base.groupTotalScore = extra.groupTotalScore;
		if (base.score <= 0 && extra.score > 0) base.score = extra.score;
		if (base.originalIndex < 0 && extra.originalIndex >= 0) base.originalIndex = extra.originalIndex;

		// attributes（向后兼容）
		for (auto it = extra.attributes.begin(); it != extra.attributes.end(); ++it) {
			if (!base.attributes.contains(it.key())) base.attributes[it.key()] = it.value();
		}
		// attributesFull（复合键）
		for (auto it = extra.attributesFull.begin(); it != extra.attributesFull.end(); ++it) {
			if (!base.attributesFull.contains(it.key())) base.attributesFull[it.key()] = it.value();
		}
		// attributesByExcel（按表格聚合）
		for (auto excelIt = extra.attributesByExcel.begin(); excelIt != extra.attributesByExcel.end(); ++excelIt) {
			const QString& excelName = excelIt.key();
			const QMap<QString, double>& excelAttrs = excelIt.value();
			for (auto it = excelAttrs.begin(); it != excelAttrs.end(); ++it) {
				if (!base.attributesByExcel[excelName].contains(it.key())) {
					base.attributesByExcel[excelName][it.key()] = it.value();
				}
			}
		}
		// comments
		for (auto it = extra.comments.begin(); it != extra.comments.end(); ++it) {
			if (!base.comments.contains(it.key())) base.comments[it.key()] = it.value();
		}
		for (auto it = extra.commentsFull.begin(); it != extra.commentsFull.end(); ++it) {
			if (!base.commentsFull.contains(it.key())) base.commentsFull[it.key()] = it.value();
		}
		for (auto excelIt = extra.commentsByExcel.begin(); excelIt != extra.commentsByExcel.end(); ++excelIt) {
			const QString& excelName = excelIt.key();
			const QMap<QString, QString>& excelComments = excelIt.value();
			for (auto it = excelComments.begin(); it != excelComments.end(); ++it) {
				if (!base.commentsByExcel[excelName].contains(it.key())) {
					base.commentsByExcel[excelName][it.key()] = it.value();
				}
			}
		}
	};

	const StudentInfo sGroup = findInList(m_studentsGroupScores);
	const StudentInfo sStudent = findInList(m_studentsStudentScores);

	// 优先 group-scores 作为 base
	bool found = false;
	if (!sGroup.id.isEmpty() || !sGroup.name.isEmpty()) {
		student = sGroup;
		found = true;
	}
	if (!sStudent.id.isEmpty() || !sStudent.name.isEmpty()) {
		if (!found) {
			student = sStudent;
			found = true;
		} else {
			mergeStudent(student, sStudent);
		}
	}

	// 兜底：若都没找到，用座位信息构造一个最小 StudentInfo
	if (!found) {
		student.id = studentId;
		student.name = studentName.isEmpty() ? btnText : studentName;
		student.originalIndex = -1;
		student.score = 0;
	}

	// 读取已下载Excel列表（用于弹窗下拉框）
	QStringList excelTables;
	QStringList dummyAttributes;
	buildHeatmapOptions(excelTables, dummyAttributes);
	
	// 系统属性列表（需要排除的属性）
	QSet<QString> systemProperties;
	systemProperties << "isSeat" << "row" << "col" << "seatRow" << "seatCol" 
	                 << "studentId" << "studentName";
	
	// 动态读取所有按钮属性
	QList<QByteArray> propertyNames = btn->dynamicPropertyNames();
	for (const QByteArray& propName : propertyNames) {
		QString propNameStr = QString::fromUtf8(propName);
		
		// 跳过系统属性
		if (systemProperties.contains(propNameStr)) {
			continue;
		}
		
		// 获取属性值
		QVariant propValue = btn->property(propName);
		if (!propValue.isValid()) {
			continue;
		}
		
		// 如果属性值是数字，作为成绩属性
		if (propValue.canConvert<double>()) {
			double value = propValue.toDouble();
			
			// 尝试解析“字段_表格名”复合键
			const int underscorePos = propNameStr.lastIndexOf('_');
			if (underscorePos > 0) {
				QString fieldName = propNameStr.left(underscorePos).trimmed();
				QString tableName = propNameStr.mid(underscorePos + 1).trimmed();
				// 只要像表格名，就写入 attributesByExcel / attributesFull
				if (!fieldName.isEmpty() && !tableName.isEmpty()) {
					student.attributesFull[propNameStr] = value;
					student.attributesByExcel[tableName][fieldName] = value;
					if (fieldName == "总分") {
						student.score = value;
					}
					continue;
				}
			}

			// 否则：直接使用属性名作为属性名称（向后兼容）
			QString attributeName = propNameStr;
			student.attributes[attributeName] = value;
			if (attributeName == "总分") {
				student.score = value;
			}
		}
	}
	
	// 创建并显示学生属性对话框
	StudentAttributeDialog* dialog = new StudentAttributeDialog(this);

	// 当弹窗内修改属性并成功写回时，同步更新本地缓存（m_students）与座位按钮的动态属性，保证再次双击显示最新值
	{
		QPointer<QPushButton> seatBtn(btn);
		const QString seatStudentId = student.id;
		const QString seatStudentName = student.name;
		connect(dialog, &StudentAttributeDialog::attributeUpdated, this,
		        [this, seatBtn, seatStudentId, seatStudentName](const QString& studentId,
		                                                        const QString& attributeName,
		                                                        double newValue,
		                                                        const QString& excelFileName) {
			        // 1) 更新 seatTable 按钮动态属性（showStudentAttributeDialog 会读取这些属性）
			        if (seatBtn) {
				        const QString key = excelFileName.isEmpty()
					        ? attributeName
					        : QString("%1_%2").arg(attributeName, excelFileName);
				        const QByteArray keyBytes = key.toUtf8();
				        seatBtn->setProperty(keyBytes.constData(), newValue);
			        }

			        // 2) 更新本地缓存（m_students + 两套来源缓存），保证下次打开对话框拿到最新值
			        auto applyUpdateToStudent = [&](StudentInfo& s) {
				        if (excelFileName.isEmpty()) {
					        s.attributes[attributeName] = newValue;
				        } else {
					        s.attributesByExcel[excelFileName][attributeName] = newValue;
					        s.attributesFull[QString("%1_%2").arg(attributeName, excelFileName)] = newValue;
				        }
				        if (attributeName == QString::fromUtf8(u8"总分")) {
					        s.score = newValue;
				        }
			        };

			        auto applyUpdateToList = [&](QList<StudentInfo>& list) {
				        bool updated = false;
				        if (!studentId.isEmpty()) {
					        for (auto& s : list) {
						        if (s.id == studentId) {
							        applyUpdateToStudent(s);
							        updated = true;
							        break;
						        }
					        }
				        }
				        if (!updated) {
					        // 兜底：按座位上的姓名匹配（某些模板可能缺学号）
					        for (auto& s : list) {
						        if (!seatStudentName.isEmpty() && s.name == seatStudentName) {
							        applyUpdateToStudent(s);
							        break;
						        }
					        }
				        }
			        };

			        applyUpdateToList(m_students);
			        applyUpdateToList(m_studentsStudentScores);
			        applyUpdateToList(m_studentsGroupScores);
		        });
	}
	
	// 动态收集所有属性名称（从所有Excel文件的属性中获取）
	QSet<QString> attributeSet; // 使用Set避免重复
	
	// 从 attributesByExcel 中收集
	for (auto excelIt = student.attributesByExcel.begin(); excelIt != student.attributesByExcel.end(); ++excelIt) {
		const QMap<QString, double>& excelAttrs = excelIt.value();
		for (auto it = excelAttrs.begin(); it != excelAttrs.end(); ++it) {
			QString attrName = it.key();
			if (!attrName.isEmpty()) {
				attributeSet.insert(attrName);
			}
		}
	}
	
	// 从 attributesFull 中收集（复合键名）
	for (auto it = student.attributesFull.begin(); it != student.attributesFull.end(); ++it) {
		QString compositeKey = it.key();
		int underscorePos = compositeKey.lastIndexOf('_');
		if (underscorePos > 0) {
			QString attrName = compositeKey.left(underscorePos);
			if (!attrName.isEmpty()) {
				attributeSet.insert(attrName);
			}
		}
	}
	
	// 向后兼容：从 attributes 中收集
	for (auto it = student.attributes.begin(); it != student.attributes.end(); ++it) {
		QString attrName = it.key();
		if (!attrName.isEmpty()) {
			attributeSet.insert(attrName);
		}
	}
	
	QList<QString> availableAttributes = attributeSet.values();
	
	// 如果没有属性，至少显示学生姓名（但这种情况应该很少见）
	if (availableAttributes.isEmpty()) {
		qWarning() << "学生" << student.name << "没有任何属性数据";
	}
	
	dialog->setTitle(QString::fromUtf8(u8"学生统计信息 - %1").arg(student.name));

	// 传入成绩表上下文（用于弹窗里保存注释到服务器）
	{
		QDate currentDate = QDate::currentDate();
		int year = currentDate.year();
		int month = currentDate.month();
		QString term;
		if (month >= 9 || month <= 1) {
			if (month >= 9) {
				term = QString("%1-%2-1").arg(year).arg(year + 1);
			} else {
				term = QString("%1-%2-1").arg(year - 1).arg(year);
			}
		} else {
			term = QString("%1-%2-2").arg(year - 1).arg(year);
		}
		// 注释保存只依赖 classId + term（exam_name 仍用于服务器请求，但本地缓存不再区分）
		dialog->setScoreContext(m_classid, QString::fromUtf8(u8"期中考试"), term);
	}
	dialog->setExcelTables(excelTables);
	// 默认选中：当前热力图已选表格（若有）；否则“全部”
	if (!m_selectedTable.isEmpty()) {
		dialog->setSelectedExcelTable(m_selectedTable);
	}
	dialog->setAvailableAttributes(availableAttributes);
	dialog->setStudentInfo(student);
	dialog->exec();
	dialog->deleteLater();
}

// 从学生信息中提取字段值（若不存在则返回0）
// 使用新的优先级：attributesByExcel → attributesFull → attributes
inline double ScheduleDialog::extractStudentValue(const StudentInfo& stu, const QString& field) const
{
	if (field == "总分" || field == "total_score") {
		// 使用辅助函数获取"总分"的值
		double total = stu.getAttributeValue("总分");
		if (total > 0.0) return total;
		// 尝试 total_score
		total = stu.getAttributeValue("total_score");
		if (total > 0.0) return total;
		return stu.score;
	}
	// 使用辅助函数获取字段值（优先级：attributesByExcel → attributesFull → attributes）
	return stu.getAttributeValue(field);
}

// 按“等分→轮抽”方式组队（groupSize=2/4/6）
inline QVector<StudentInfo> ScheduleDialog::groupWheelAssign(QVector<StudentInfo> sorted, int groupSize)
{
	QVector<StudentInfo> result;
	if (sorted.isEmpty() || groupSize <= 0) return result;

	int n = sorted.size();
	int bucketCnt = groupSize;
	QVector<QVector<StudentInfo>> buckets;
	buckets.resize(bucketCnt);

	// 将已按降序的列表切成 bucketCnt 份，前 remainder 份+1
	int base = n / bucketCnt;
	int rem = n % bucketCnt;
	int idx = 0;
	for (int b = 0; b < bucketCnt; ++b) {
		int take = base + (b < rem ? 1 : 0);
		for (int k = 0; k < take && idx < n; ++k, ++idx) {
			buckets[b].append(sorted[idx]);
		}
	}

	// 轮抽：依次从每个桶随机取一人（无放回），直到桶空
	bool any = true;
	while (any) {
		any = false;
		for (int b = 0; b < bucketCnt; ++b) {
			if (buckets[b].isEmpty()) continue;
			any = true;
			int pick = QRandomGenerator::global()->bounded(buckets[b].size());
			result.append(buckets[b][pick]);
			buckets[b].removeAt(pick);
		}
	}

	return result;
}

// 根据字段与模式排座：modeText 来自 rightComboBox，isGroupMode 对应左侧是否“小组”
inline void ScheduleDialog::arrangeSeatsByField(const QString& fieldName, const QString& modeText, bool isGroupMode)
{
	if (m_students.isEmpty() || !seatTable) {
		qWarning() << "没有学生数据或座位表未初始化";
		return;
	}

	QList<StudentInfo> students = m_students;
	// 基准：降序
	std::sort(students.begin(), students.end(), [=](const StudentInfo& a, const StudentInfo& b) {
		return extractStudentValue(a, fieldName) > extractStudentValue(b, fieldName);
	});

	QList<StudentInfo> ordered;
	if (!isGroupMode) {
		if (modeText.contains("随机")) {
			// 全体随机
			QVector<StudentInfo> vec = students.toVector();
			for (int i = vec.size() - 1; i > 0; --i) {
				int j = QRandomGenerator::global()->bounded(i + 1);
				std::swap(vec[i], vec[j]);
			}
			ordered = vec.toList();
		} else if (modeText.contains("正序")) {
			std::sort(students.begin(), students.end(), [=](const StudentInfo& a, const StudentInfo& b) {
				return extractStudentValue(a, fieldName) < extractStudentValue(b, fieldName);
			});
			ordered = students;
		} else if (modeText.contains("倒序")) {
			ordered = students; // 已降序
		} else {
			ordered = students;
		}
	} else {
		// 小组模式
		if (modeText.contains("2人组")) {
			ordered = groupWheelAssign(students.toVector(), 2).toList();
		} else if (modeText.contains("4人组")) {
			ordered = groupWheelAssign(students.toVector(), 4).toList();
		} else if (modeText.contains("6人组")) {
			ordered = groupWheelAssign(students.toVector(), 6).toList();
		} else {
			ordered = students;
		}
	}

	// 收集座位按钮（只取 isSeat=true），按行左->右
	QVector<QPushButton*> seats;
	for (int row = 0; row < seatTable->rowCount(); ++row) {
		for (int col = 0; col < seatTable->columnCount(); ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
			if (btn && btn->property("isSeat").toBool()) {
				seats.append(btn);
			}
		}
	}

	// 填充座位
	int n = qMin(seats.size(), ordered.size());
	for (int i = 0; i < seats.size(); ++i) {
		QPushButton* btn = seats[i];
		if (!btn) continue;
		if (i < n) {
			const StudentInfo& stu = ordered[i];
			QString text;
			if (!stu.name.isEmpty() && !stu.id.isEmpty()) {
				text = QString("%1/%2").arg(stu.name).arg(stu.id); // 同时显示姓名和学号
			} else if (!stu.name.isEmpty()) {
				text = stu.name;
			} else {
				text = stu.id;
			}
			setSeatButtonTextAndIcon(btn, text);
			btn->setProperty("studentId", stu.id);
			btn->setProperty("studentName", stu.name);
		} else {
			setSeatButtonTextAndIcon(btn, "");
			btn->setProperty("studentId", QVariant());
			btn->setProperty("studentName", QVariant());
		}
	}
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
				QString propId = btn->property("studentId").toString().trimmed();
				QString propName = btn->property("studentName").toString().trimmed();

				if (!studentLabel.isEmpty() || !propName.isEmpty() || !propId.isEmpty()) {
					QJsonObject seatObj;
					seatObj["row"] = row + 1; // 行号从1开始
					seatObj["col"] = col + 1; // 列号从1开始

					QString resolvedName = propName;
					QString resolvedId = propId;

					if (!studentLabel.isEmpty()) {
						seatObj["student_name"] = studentLabel;

						// 解析显示文本，兼容 "王文1-5/7" 这种：姓名=王文，学号=7
						auto parseSeatLabel = [](const QString& label) -> QPair<QString, QString> {
							QString s = label.trimmed();
							QString idFromSlash;
							const int slashIndex = s.lastIndexOf('/');
							if (slashIndex >= 0) {
								idFromSlash = s.mid(slashIndex + 1).trimmed();
								s = s.left(slashIndex).trimmed();
							}

							QString name;
							QString id;

							QRegExp nameRe("^([\\u4e00-\\u9fa5]+)");
							if (nameRe.indexIn(s) >= 0) {
								name = nameRe.cap(1);
							}

							if (!idFromSlash.isEmpty()) {
								id = idFromSlash;
							} else {
								QRegExp dashRe("(\\d+-\\d+)");
								if (dashRe.indexIn(s) >= 0) {
									id = dashRe.cap(1);
								} else {
									QStringList parts = s.split(QRegExp("[\\s-]+"), Qt::SkipEmptyParts);
									if (!parts.isEmpty() && name.isEmpty()) name = parts.first();
									if (parts.size() >= 2) id = parts.last();
								}
							}

							if (name.isEmpty()) name = s;
							return qMakePair(name, id);
						};

						const QPair<QString, QString> parsed = parseSeatLabel(studentLabel);
						if (resolvedName.isEmpty()) resolvedName = parsed.first;
						if (resolvedId.isEmpty()) resolvedId = parsed.second;
					} else {
						if (!propName.isEmpty()) seatObj["student_name"] = propName;
					}

					if (!resolvedName.isEmpty()) {
						seatObj["name"] = resolvedName;
					}
					if (!resolvedId.isEmpty()) {
						seatObj["student_id"] = resolvedId;
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
				// 显示自定义提示窗口（屏幕右下角）
				ToastNotification* toast = new ToastNotification(
					QString::fromUtf8(u8"座位表已成功上传到服务器并刷新界面！"), this);
				toast->show();
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
			// 显示自定义提示窗口（屏幕右下角）
			ToastNotification* toast = new ToastNotification(
				QString::fromUtf8(u8"座位表已成功上传到服务器并刷新界面！"), this);
			toast->show();
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

	auto createButton = [this](const QString& style) -> QPushButton* {
		QPushButton* btn = new QPushButton("", this);
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

	// 从时间字符串中提取时间用于排序的辅助函数（内联定义，供timeToMinutes使用）
	auto extractTimeForSorting = [](const QString& timeText) -> QString {
		if (timeText.isEmpty()) return "";
		
		// 先统一将全角冒号转换为半角冒号
		QString normalizedText = timeText;
		normalizedText.replace(QStringLiteral("："), QStringLiteral(":"));
		
		// 匹配时间范围格式：如 "7:00-7:40" 或 "早读 7:00-7:40" 或 "午休 11:10-13:30"
		QRegExp timeRangePattern("(\\d{1,2})\\s*:\\s*(\\d{2})\\s*-\\s*(\\d{1,2})\\s*:\\s*(\\d{2})");
		if (timeRangePattern.indexIn(normalizedText) != -1) {
			QString hour = timeRangePattern.cap(1);  // 提取开始时间的小时部分，如"11"
			QString min = timeRangePattern.cap(2);   // 提取开始时间的分钟部分，如"10"
			// 返回开始时间用于排序，格式化为 HH:MM（如"11:10"），确保排序正确
			return QString("%1:%2").arg(hour.toInt(), 2, 10, QChar('0')).arg(min);
		}
		
		// 匹配单个时间格式：如 "7:00" 或 "早读 7:00"
		QRegExp singleTimePattern("(\\d{1,2})\\s*:\\s*(\\d{2})");
		if (singleTimePattern.indexIn(normalizedText) != -1) {
			QString hour = singleTimePattern.cap(1);
			QString min = singleTimePattern.cap(2);
			// 格式化为 HH:MM
			return QString("%1:%2").arg(hour.toInt(), 2, 10, QChar('0')).arg(min);
		}
		
		return "";
	};

	// 按时间字符串排序（格式如 "07:20", "08:00", "12:00" 或 "午休 11：10-13：30" 等）
	// 先将时间字符串提取出时间部分，再转换为分钟数进行比较，确保正确排序
	auto timeToMinutes = [&extractTimeForSorting](const QString& timeStr) -> int {
		if (timeStr.isEmpty()) return 9999; // 空时间排在最后
		
		// 先从时间字符串中提取时间（如从"午休 11：10-13：30"中提取"11:10"）
		QString extractedTime = extractTimeForSorting(timeStr);
		if (extractedTime.isEmpty()) return 9999; // 无法提取时间，排在最后
		
		// 将提取的时间转换为分钟数
		QStringList parts = extractedTime.split(':');
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

// 从时间字符串中提取时间用于排序（如"早读 7:00-7:40"提取"7:00"）
inline QString extractTimeForSorting(const QString& timeText) {
	if (timeText.isEmpty()) return "";
	
	// 先统一将全角冒号转换为半角冒号
	QString normalizedText = timeText;
	normalizedText.replace(QStringLiteral("："), QStringLiteral(":"));
	
	// 匹配时间范围格式：如 "7:00-7:40" 或 "早读 7:00-7:40" 或 "早读 早读 7:00-7:40"
	// 提取开始时间（第一个时间）用于排序，如从"早读 7:00-7:40"中提取"7:00"
	QRegExp timeRangePattern("(\\d{1,2})\\s*:\\s*(\\d{2})\\s*-\\s*(\\d{1,2})\\s*:\\s*(\\d{2})");
	if (timeRangePattern.indexIn(normalizedText) != -1) {
		QString hour = timeRangePattern.cap(1);  // 提取开始时间的小时部分，如"7"
		QString min = timeRangePattern.cap(2);   // 提取开始时间的分钟部分，如"00"
		// 返回开始时间用于排序，格式化为 HH:MM（如"07:00"），确保排序正确
		return QString("%1:%2").arg(hour.toInt(), 2, 10, QChar('0')).arg(min);
	}
	
	// 匹配单个时间格式：如 "7:00" 或 "早读 7:00"
	QRegExp singleTimePattern("(\\d{1,2})\\s*:\\s*(\\d{2})");
	if (singleTimePattern.indexIn(normalizedText) != -1) {
		QString hour = singleTimePattern.cap(1);
		QString min = singleTimePattern.cap(2);
		// 格式化为 HH:MM
		return QString("%1:%2").arg(hour.toInt(), 2, 10, QChar('0')).arg(min);
	}
	
	return "";
}

inline QMap<QString, QString> ScheduleDialog::extractHighlightTimes(const QStringList& times, const QStringList& subjects) const
{
	// 移除硬编码的默认值，以服务器下发的为准
	QStringList specialSubjects = { QStringLiteral("晨读"), QStringLiteral("午饭"), QStringLiteral("午休"), QStringLiteral("晚自习") };

	// 先收集所有数据到列表中
	QList<QPair<QString, QString>> items;
	int count = qMin(times.size(), subjects.size());
	for (int i = 0; i < count; ++i) {
		const QString subject = subjects[i];
		const QString time = times[i];
		// 只提取服务器下发的特殊科目时间，不设置默认值
		if (specialSubjects.contains(subject) && !time.isEmpty()) {
			items.append(qMakePair(subject, time));
		}
	}
	
	// 按照提取的时间进行排序
	std::sort(items.begin(), items.end(), [](const QPair<QString, QString>& a, const QPair<QString, QString>& b) {
		QString timeA = extractTimeForSorting(a.second);
		QString timeB = extractTimeForSorting(b.second);
		return timeA < timeB;
	});
	
	// 转换为QMap返回（虽然QMap会按键排序，但我们已经按时间排序了，调用方会使用排序后的列表）
	QMap<QString, QString> result;
	for (const auto& item : items) {
		result[item.first] = item.second;
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

	// 按照时间排序显示，而不是使用固定顺序
	QList<QPair<QString, QString>> sortedItems;
	for (auto it = highlights.constBegin(); it != highlights.constEnd(); ++it) {
		sortedItems.append(qMakePair(it.key(), it.value()));
	}
	
	// 按照提取的时间进行排序
	std::sort(sortedItems.begin(), sortedItems.end(), [](const QPair<QString, QString>& a, const QPair<QString, QString>& b) {
		QString timeA = extractTimeForSorting(a.second);
		QString timeB = extractTimeForSorting(b.second);
		return timeA < timeB;
	});

	bool first = true;
	for (const auto& item : sortedItems) {
		const QString& subject = item.first;
		if (!first) {
			QLabel* sep = new QLabel("   |   ", this);
			sep->setStyleSheet("color: white; font-size: 12px;");
			m_specialSubjectRowLayout->addWidget(sep);
		}
		QString display = QStringLiteral("%1 %2").arg(subject, highlights.value(subject));
		QLabel* lbl = new QLabel(display, this);
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
	// 移除硬编码的特殊科目时间，以服务器下发的为准
	QString tutoringTime = "17:00"; // 课服时间保留，因为不在移除列表中

	QStringList times;
	times.reserve(subjects.size());
	for (int i = 0; i < subjects.size(); ++i) {
		QString subject = subjects[i];
		QString slotTime = slotDefaults.value(i, "--:--");
		// 移除晨读、午饭、午休、晚自习的硬编码时间设置
		if (subject == "课服") {
			slotTime = tutoringTime;
		}
		// 其他科目使用默认时间，特殊科目（晨读、午饭、午休、晚自习）也使用默认时间，由服务器数据覆盖
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
	QString currentUserId = m_userId.isEmpty() ? userInfo.classId : m_userId;
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
		memberObj["is_voice_enabled"] = member.is_voice_enabled;  // 是否开启语音
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
	// 添加临时房间信息（如果已创建）
	if (!m_roomId.isEmpty()) {
		QJsonObject tempRoomObj;
		tempRoomObj["room_id"] = m_roomId;
		if (!m_whipUrl.isEmpty()) {
			tempRoomObj["whip_url"] = m_whipUrl;
		}
		if (!m_whepUrl.isEmpty()) {
			tempRoomObj["whep_url"] = m_whepUrl;
		}
		if (!m_streamName.isEmpty()) {
			tempRoomObj["stream_name"] = m_streamName;
		}
		tempRoomObj["group_id"] = m_unique_group_id;
		tempRoomObj["owner_id"] = currentUserId;
		tempRoomObj["owner_name"] = currentUserName;
		tempRoomObj["owner_icon"] = currentUserIcon;
		dataObj["temp_room"] = tempRoomObj;
	}
	
	qDebug() << "班级群唯一编号 (m_unique_group_id):" << m_unique_group_id;
	qDebug() << "房间ID (m_roomId):" << m_roomId;
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
		qWarning() << "userInfo.teacher_unique_id:" << userInfo.classId;
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
		// 页面打开成功后，如果已有拉流地址，开始拉流（注释：拉流在HTML页面上进行）
		//QTimer::singleShot(1000, this, [this]() {
		//	if (!m_whepUrl.isEmpty() && !m_isPullingStream) {
		//		qDebug() << "页面已打开，开始拉流，拉流地址:" << m_whepUrl;
		//		startPullStream();
		//	}
		//});
		return;
	}
	
	qWarning() << "QDesktopServices返回false，尝试其他方法";
	
	// 方法2：在Windows上使用explorer直接打开文件（最可靠）
	#ifdef _WIN32
		qDebug() << "尝试使用explorer打开文件:" << nativePath;
		if (QProcess::startDetached("explorer", QStringList() << nativePath)) {
			qDebug() << "成功使用explorer打开HTML文件";
			// 页面打开成功后，如果已有拉流地址，开始拉流（注释：拉流在HTML页面上进行）
			//QTimer::singleShot(1000, this, [this]() {
			//	if (!m_whepUrl.isEmpty() && !m_isPullingStream) {
			//		qDebug() << "页面已打开，开始拉流，拉流地址:" << m_whepUrl;
			//		startPullStream();
			//	}
			//});
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
			// 页面打开成功后，如果已有拉流地址，开始拉流（注释：拉流在HTML页面上进行）
			//QTimer::singleShot(1000, this, [this]() {
			//	if (!m_whepUrl.isEmpty() && !m_isPullingStream) {
			//		qDebug() << "页面已打开，开始拉流，拉流地址:" << m_whepUrl;
			//		startPullStream();
			//	}
			//});
			return;
		}
		
		qWarning() << "cmd start命令失败，尝试使用rundll32";
		
		// 方法4：使用rundll32调用shell32.dll（最后的备用方案）
		QStringList rundllArgs;
		rundllArgs << "shell32.dll,ShellExec_RunDLL" << nativePath;
		if (QProcess::startDetached("rundll32.exe", rundllArgs)) {
			qDebug() << "成功使用rundll32打开HTML文件";
			// 页面打开成功后，如果已有拉流地址，开始拉流（注释：拉流在HTML页面上进行）
			//QTimer::singleShot(1000, this, [this]() {
			//	if (!m_whepUrl.isEmpty() && !m_isPullingStream) {
			//		qDebug() << "页面已打开，开始拉流，拉流地址:" << m_whepUrl;
			//		startPullStream();
			//	}
			//});
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

// 创建临时房间
inline void ScheduleDialog::createTemporaryRoom()
{
	if (m_unique_group_id.isEmpty()) {
		qWarning() << "警告：班级群唯一编号为空，无法创建临时房间！";
		return;
	}
	
	if (!m_pWs) {
		qWarning() << "警告：WebSocket未连接，无法创建临时房间！";
		return;
	}
	
	// 生成房间ID（使用时间戳）
	m_roomId = QString("room_%1").arg(QDateTime::currentMSecsSinceEpoch());
	qDebug() << "生成房间ID:" << m_roomId;
	
	// 生成stream_url：使用班级群唯一编号作为stream参数
	// 格式：https://47.100.126.194/rtc/v1/whep/?app=live&stream=<班级群的唯一编号>
	QString streamUrl = QString("https://47.100.126.194/rtc/v1/whep/?app=live&stream=%1").arg(m_unique_group_id);
	qDebug() << "生成stream_url:" << streamUrl;
	
	// 获取当前用户信息
	UserInfo userInfo = CommonInfo::GetData();
	QString ownerName = m_userName.isEmpty() ? userInfo.strName : m_userName;
	if (ownerName.isEmpty()) {
		ownerName = "用户";
	}
	
	// 构建创建房间的消息
	QJsonObject createRoomMessage;
	createRoomMessage["type"] = "6";
	createRoomMessage["invited_users"] = QJsonArray();  // 初始为空数组
	createRoomMessage["stream_url"] = streamUrl;
	createRoomMessage["owner_name"] = ownerName;
	createRoomMessage["group_id"] = m_unique_group_id;  // 班级群的唯一编号
	
	QJsonDocument doc(createRoomMessage);
	QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
	
	qDebug() << "准备发送创建房间消息:";
	qDebug() << "完整消息对象:" << jsonString;
	qDebug() << "group_id字段值:" << m_unique_group_id;
	
	// 通过WebSocket发送消息（直接发送JSON格式，不通过群组）
	// 注意：这里直接发送JSON，服务器应该能够处理
	TaQTWebSocket::sendPrivateMessage(jsonString);
	
	qDebug() << "创建房间消息已发送，房间ID:" << m_roomId;
	qDebug() << "发送的消息字符串:" << jsonString;
}

// 开始拉流（WHEP）- 注释：拉流在HTML页面上进行，不在C++代码中
/*
inline void ScheduleDialog::startPullStream()
{
	if (m_whepUrl.isEmpty()) {
		qWarning() << "拉流地址为空，无法开始拉流";
		return;
	}
	
	if (m_isPullingStream) {
		qDebug() << "已经在拉流中，无需重复开始";
		return;
	}
	
	qDebug() << "开始拉流，拉流地址:" << m_whepUrl;
	
	// 创建媒体播放器
	if (!m_pullStreamPlayer) {
		m_pullStreamPlayer = new QMediaPlayer(this);
		QAudioOutput* audioOutput = new QAudioOutput(this);
		m_pullStreamPlayer->setAudioOutput(audioOutput);
		
		// 连接信号
		connect(m_pullStreamPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
			if (status == QMediaPlayer::LoadedMedia) {
				qDebug() << "拉流媒体已加载";
			} else if (status == QMediaPlayer::BufferingMedia) {
				qDebug() << "拉流正在缓冲";
			} else if (status == QMediaPlayer::BufferedMedia) {
				qDebug() << "拉流缓冲完成";
			} else if (status == QMediaPlayer::EndOfMedia) {
				qDebug() << "拉流播放结束";
			}
		});
		
		connect(m_pullStreamPlayer, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error error, const QString& errorString) {
			qWarning() << "拉流播放错误:" << error << errorString;
			// 如果播放失败，尝试使用其他方法
			if (error == QMediaPlayer::ResourceError || error == QMediaPlayer::FormatError) {
				qWarning() << "QMediaPlayer不支持WHEP协议，可能需要使用WebRTC或其他方法";
			}
		});
	}
	
	// 设置媒体源并播放
	QUrl mediaUrl(m_whepUrl);
	m_pullStreamPlayer->setSource(mediaUrl);
	m_pullStreamPlayer->play();
	
	m_isPullingStream = true;
	qDebug() << "拉流已开始";
}

// 停止拉流
inline void ScheduleDialog::stopPullStream()
{
	if (!m_isPullingStream) {
		return;
	}
	
	qDebug() << "停止拉流";
	
	if (m_pullStreamPlayer) {
		m_pullStreamPlayer->stop();
	}
	
	m_isPullingStream = false;
	qDebug() << "拉流已停止";
}
*/


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
	const QString cacheKey = prepareClassCacheKey(subject, time);
	// 复用同一个窗口：关闭按钮只隐藏，下次点菜单再显示
	if (!prepareClassViewDlg) {
		prepareClassViewDlg = new PrepareClassEditDialog(this);
	}

	const QString header = time.trimmed().isEmpty()
		? QString::fromUtf8(u8"课前准备 - ") + subject
		: QString::fromUtf8(u8"课前准备 - ") + subject + QString::fromUtf8(u8"  ") + time;
	prepareClassViewDlg->setHeaderText(header);
	prepareClassViewDlg->setInitialContent(m_prepareClassCache.value(cacheKey));

	prepareClassViewDlg->show();
	prepareClassViewDlg->raise();
	prepareClassViewDlg->activateWindow();
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

// 获取下节课的科目和时间
inline QPair<QString, QString> ScheduleDialog::getNextClassInfo(const QTime& currentTime) const
{
	// 从实际显示的按钮中获取课程信息
	if (m_timeButtons.isEmpty() || m_subjectButtons.isEmpty() || m_timeButtons.size() != m_subjectButtons.size()) {
		// 如果按钮列表为空或数量不匹配，使用默认课程表
		auto dailySchedule = buildDefaultDailySchedule();
		const QStringList& subjects = dailySchedule.second;
		const QStringList& times = dailySchedule.first;
		
		// 定义需要跳过的科目（这些科目不算是"课"）
		QSet<QString> skipSubjects;
		skipSubjects << QStringLiteral("午饭") << QStringLiteral("午休") 
		             << QStringLiteral("大课间") << QStringLiteral("眼保健操");
		
		// 找到当前时间之后的第一节课
		for (int i = 0; i < times.size() && i < subjects.size(); ++i) {
			QString timeStr = times[i];
			QString subject = subjects[i];
			
			// 跳过空科目和特殊科目
			if (subject.isEmpty() || skipSubjects.contains(subject)) {
				continue;
			}
			
			// 解析时间字符串（格式可能是 "07:20" 或 "7:20"）
			QTime classTime;
			if (timeStr.contains(QStringLiteral(":"))) {
				QStringList parts = timeStr.split(QStringLiteral(":"));
				if (parts.size() >= 2) {
					bool ok1, ok2;
					int hour = parts[0].toInt(&ok1);
					int minute = parts[1].toInt(&ok2);
					if (ok1 && ok2) {
						classTime = QTime(hour, minute);
						if (classTime.isValid() && classTime > currentTime) {
							return qMakePair(subject, timeStr);
						}
					}
				}
			}
		}
	} else {
		// 从按钮中获取实际显示的课程信息
		QSet<QString> skipSubjects;
		skipSubjects << QStringLiteral("午饭") << QStringLiteral("午休") 
		             << QStringLiteral("大课间") << QStringLiteral("眼保健操");
		
		for (int i = 0; i < m_timeButtons.size() && i < m_subjectButtons.size(); ++i) {
			QPushButton* timeBtn = m_timeButtons[i];
			QPushButton* subjectBtn = m_subjectButtons[i];
			if (!timeBtn || !subjectBtn) continue;
			
			QString timeStr = timeBtn->property("raw_time").toString();
			if (timeStr.isEmpty()) {
				timeStr = timeBtn->text();
			}
			
			QString subject = subjectBtn->property("subject").toString();
			if (subject.isEmpty()) {
				subject = subjectBtn->text();
			}
			
			// 跳过空科目和特殊科目
			if (subject.isEmpty() || skipSubjects.contains(subject)) {
				continue;
			}
			
			// 解析时间字符串
			QTime classTime;
			if (timeStr.contains(QStringLiteral(":"))) {
				QStringList parts = timeStr.split(QStringLiteral(":"));
				if (parts.size() >= 2) {
					bool ok1, ok2;
					int hour = parts[0].toInt(&ok1);
					int minute = parts[1].toInt(&ok2);
					if (ok1 && ok2) {
						classTime = QTime(hour, minute);
						if (classTime.isValid() && classTime > currentTime) {
							return qMakePair(subject, timeStr);
						}
					}
				}
			}
		}
	}
	
	// 如果没有找到下节课，返回空对
	return qMakePair(QString(), QString());
}

// 显示下节课的课前准备
inline void ScheduleDialog::showNextClassPrepareDialog(const QString& subject, const QString& time)
{
	showPrepareClassDialog(subject, time);
}

inline void ScheduleDialog::setHomeworkData(const QString& dateStr, const QString& subject, const QString& content)
{
	if (!dateStr.isEmpty() && !subject.isEmpty() && !content.isEmpty()) {
		m_homeworkByDate[dateStr][subject] = content;
		qDebug() << "ScheduleDialog: 设置作业数据，日期:" << dateStr << "科目:" << subject;
	}
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
	QLabel* lblPrompt = new QLabel(QString::fromUtf8(u8"请输入课后评价内容"), dlg);
	lblPrompt->setStyleSheet("color: white; font-size: 14px;");
	mainLayout->addWidget(lblPrompt);
	
	// 文本输入框
	QTextEdit* textEdit = new QTextEdit(dlg);
	textEdit->setPlaceholderText(QString::fromUtf8(u8"请输入课后评价内容..."));
	textEdit->setMinimumHeight(200);
	mainLayout->addWidget(textEdit, 1);
	
	// 按钮布局
	QHBoxLayout* btnLayout = new QHBoxLayout(dlg);
	btnLayout->addStretch();
	
	QPushButton* btnCancel = new QPushButton(QString::fromUtf8(u8"取消"), dlg);
	btnCancel->setFixedSize(80, 35);
	btnCancel->setStyleSheet(
		"QPushButton { background-color: #4a4a4a; color: white; }"
		"QPushButton:hover { background-color: #5a5a5a; }"
	);
	
	QPushButton* btnConfirm = new QPushButton(QString::fromUtf8(u8"确定"), dlg);
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

// 下载Excel文件
inline void ScheduleDialog::downloadExcelFile(const QString& url, const QString& saveDir, const QString& filename, std::function<void()> onCompleted)
{
	if (!m_networkManager) {
		qWarning() << "网络管理器未初始化";
		return;
	}
	
	QUrl fileUrl(url);
	QNetworkRequest request(fileUrl);
	QNetworkReply* reply = m_networkManager->get(request);
	
	// 连接下载完成信号
	connect(reply, &QNetworkReply::finished, this, [=]() {
		bool success = false;
		if (reply->error() == QNetworkReply::NoError) {
			// 确保目录存在（使用绝对路径）
			QDir dir;
			QString absoluteSaveDir = QDir(saveDir).absolutePath();
			if (!dir.exists(absoluteSaveDir)) {
				dir.mkpath(absoluteSaveDir);
				qDebug() << "创建保存目录:" << absoluteSaveDir;
			}
			
			// 保存文件（使用绝对路径）
			QString absoluteFilePath = QDir(saveDir).absoluteFilePath(filename);
			QFile file(absoluteFilePath);
			if (file.open(QIODevice::WriteOnly)) {
				file.write(reply->readAll());
				file.close();
				QString msg = QString("[Excel下载] Excel文件下载成功: %1").arg(absoluteFilePath);
				qDebug() << msg;
				writeLogToFile(msg);
				success = true;
			} else {
				QString msg = QString("[Excel下载] 无法保存文件: %1").arg(absoluteFilePath);
				qWarning() << msg;
				writeLogToFile(msg);
			}
		} else {
			QString msg = QString("[Excel下载] 下载Excel文件失败: %1 URL: %2").arg(reply->errorString()).arg(url);
			qWarning() << msg;
			writeLogToFile(msg);
		}
		
		// 调用完成回调（无论成功或失败都调用，以便跟踪下载进度）
		if (onCompleted) {
			onCompleted();
		}
		
		reply->deleteLater();
	});
	
	QString filePath = QDir(saveDir).absoluteFilePath(filename);
	QString msg = QString("[Excel下载] 开始下载Excel文件: %1 保存到: %2").arg(url).arg(filePath);
	qDebug() << msg;
	writeLogToFile(msg);
}

// 写入日志到文件
inline void ScheduleDialog::writeLogToFile(const QString& message)
{
	QString logDir = QCoreApplication::applicationDirPath() + "/logs";
	QDir dir;
	if (!dir.exists(logDir)) {
		dir.mkpath(logDir);
	}
	
	QString logFile = logDir + "/excel_download_" + QDate::currentDate().toString("yyyyMMdd") + ".log";
	QFile file(logFile);
	if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
		QTextStream out(&file);
		out.setCodec("UTF-8");
		QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
		out << "[" << timestamp << "] " << message << "\n";
		file.close();
	}
}

// 高亮或取消高亮座位
inline void ScheduleDialog::highlightSeat(QPushButton* btn, bool highlight)
{
	if (!btn) return;
	
	if (highlight) {
		// 高亮样式
		btn->setStyleSheet(
			"QPushButton { "
			"background-color: #007bff; "  // 蓝色高亮
			"color: white; "
			"border: 3px solid #0056b3; "  // 加粗边框
			"border-radius: 4px; "
			"padding: 5px; "
			"font-size: 12px; "
			"text-align: center; "
			"}"
			"QPushButton:hover { "
			"background-color: #0056b3; "
			"}"
		);
	} else {
		// 恢复原始样式
		QString originalStyle = 
			"QPushButton { "
			"background-color: #dc3545; "
			"color: white; "
			"border: 1px solid #ccc; "
			"border-radius: 4px; "
			"padding: 5px; "
			"font-size: 12px; "
			"text-align: center; "
			"}"
			"QPushButton:hover { "
			"background-color: #c82333; "
			"}"
			"QPushButton:pressed { "
			"background-color: #bd2130; "
			"}";
		btn->setStyleSheet(originalStyle);
	}
}

// 根据全局坐标查找座位按钮
inline QPushButton* ScheduleDialog::findSeatAtPosition(const QPoint& globalPos)
{
	if (!seatTable) return nullptr;
	
	// 遍历所有座位按钮，检查鼠标是否在其范围内
	for (int row = 0; row < seatTable->rowCount(); ++row) {
		for (int col = 0; col < seatTable->columnCount(); ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
			if (btn && btn->property("isSeat").toBool()) {
				// 将按钮的局部坐标转换为全局坐标
				QPoint btnTopLeft = btn->mapToGlobal(QPoint(0, 0));
				QRect btnRect(btnTopLeft, btn->size());
				
				// 检查全局坐标是否在按钮范围内
				if (btnRect.contains(globalPos)) {
					return btn;
				}
			}
		}
	}
	
	return nullptr;
}

// 重置拖拽状态
inline void ScheduleDialog::resetDragState()
{
	// 取消所有高亮
	if (m_dragSourceSeat) {
		highlightSeat(m_dragSourceSeat, false);
		m_dragSourceSeat = nullptr;
	}
	if (m_dragTargetSeat && m_dragTargetSeat != m_dragSourceSeat) {
		highlightSeat(m_dragTargetSeat, false);
		m_dragTargetSeat = nullptr;
	}
	m_isDragging = false;
}

// 交换两个座位的内容
inline void ScheduleDialog::swapSeats(QPushButton* seat1, QPushButton* seat2)
{
	if (!seat1 || !seat2 || seat1 == seat2) {
		return;
	}
	
	// 获取第一个座位的信息
	QString text1 = seat1->text();
	QString studentId1 = seat1->property("studentId").toString();
	QString studentName1 = seat1->property("studentName").toString();
	QIcon icon1 = seat1->icon();
	
	// 获取第二个座位的信息
	QString text2 = seat2->text();
	QString studentId2 = seat2->property("studentId").toString();
	QString studentName2 = seat2->property("studentName").toString();
	QIcon icon2 = seat2->icon();
	
	// 交换文本
	seat1->setText(text2);
	seat2->setText(text1);
	
	// 交换属性
	seat1->setProperty("studentId", studentId2);
	seat1->setProperty("studentName", studentName2);
	seat2->setProperty("studentId", studentId1);
	seat2->setProperty("studentName", studentName1);
	
	// 交换图标
	seat1->setIcon(icon2);
	seat2->setIcon(icon1);
	
	// 更新座位信息列表（如果存在）
	// 获取座位位置
	int row1 = seat1->property("seatRow").toInt();
	int col1 = seat1->property("seatCol").toInt();
	int row2 = seat2->property("seatRow").toInt();
	int col2 = seat2->property("seatCol").toInt();
	
	// 更新 m_seatInfoList
	for (SeatInfo& seatInfo : m_seatInfoList) {
		if (seatInfo.row == row1 && seatInfo.col == col1) {
			seatInfo.studentName = text2.isEmpty() ? studentName2 : text2;
			seatInfo.studentId = studentId2;
		} else if (seatInfo.row == row2 && seatInfo.col == col2) {
			seatInfo.studentName = text1.isEmpty() ? studentName1 : text1;
			seatInfo.studentId = studentId1;
		}
	}
	
	// 自动上传到服务器
	uploadSeatTableToServer();
}


