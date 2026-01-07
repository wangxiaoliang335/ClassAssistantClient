#include "ScheduleDialog.h"
#include "HomeworkEditDialog.h"
#include "HomeworkViewDialog.h"
#include <QMessageBox>
#include <QDebug>
#include <QDate>
#include <QJsonObject>
#include <QJsonDocument>
#include "CommonInfo.h"
#include "TaQTWebSocket.h"

// 实现作业按钮的连接逻辑
void ScheduleDialog::connectHomeworkButton(QPushButton* btnTask)
{
    if (!btnTask) return;
    
    connect(btnTask, &QPushButton::clicked, this, [this]() {
        // 判断是班级端还是教师端
        bool isClassSide = CommonInfo::GetClassLoginInfo().isLoggedIn();
        
        if (isClassSide) {
            // 班级端：显示作业展示窗口
            showHomeworkViewDialog();
        } else {
            // 教师端：打开编辑作业窗口
            if (!homeworkEditDlg) {
                homeworkEditDlg = new HomeworkEditDialog(this);
                homeworkEditDlg->setDate(QDate::currentDate());
                
                // 连接发布信号
                connect(homeworkEditDlg, &HomeworkEditDialog::homeworkPublishedSingle, this,
                    [this](const QDate& date, const QString& subject, const QString& hwText) {
                        qDebug() << "作业已发布，日期:" << date.toString("yyyy-MM-dd")
                                 << "科目:" << subject << "内容:" << hwText;

                        if (m_unique_group_id.isEmpty()) {
                            QMessageBox::warning(this, QString::fromUtf8(u8"错误"), QString::fromUtf8(u8"群组ID为空，无法发布作业！"));
                            return;
                        }
                        if (!m_pWs) {
                            QMessageBox::warning(this, QString::fromUtf8(u8"错误"), QString::fromUtf8(u8"WebSocket未连接，无法发布作业！"));
                            return;
                        }

                        if (subject.trimmed().isEmpty() || hwText.trimmed().isEmpty()) return;

                        QJsonObject jsonObj;
                        jsonObj[QStringLiteral("type")] = QStringLiteral("homework"); // 消息类型：家庭作业
                        jsonObj[QStringLiteral("class_id")] = m_classid;
                        jsonObj[QStringLiteral("school_id")] = CommonInfo::GetData().schoolId;
                        jsonObj[QStringLiteral("group_name")] = m_groupName;
                        jsonObj[QStringLiteral("group_id")] = m_unique_group_id;
                        jsonObj[QStringLiteral("subject")] = subject;
                        jsonObj[QStringLiteral("content")] = hwText;
                        jsonObj[QStringLiteral("date")] = date.toString(QStringLiteral("yyyy-MM-dd"));

                        UserInfo userInfo = CommonInfo::GetData();
                        jsonObj[QStringLiteral("sender_id")] = userInfo.classId;
                        jsonObj[QStringLiteral("sender_name")] = userInfo.strName;
                        if (jsonObj[QStringLiteral("school_id")].toString().trimmed().isEmpty()) {
                            jsonObj.remove(QStringLiteral("school_id")); // 按文档：可为空/可不传
                        }

                        QJsonDocument doc(jsonObj);
                        const QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

                        // 通过WebSocket发送到群组，格式：to:群组ID:消息内容
                        const QString message = QString("to:%1:%2").arg(m_unique_group_id, jsonString);
                        TaQTWebSocket::sendPrivateMessage(message);

                        qDebug() << "已通过WebSocket发布作业到群组:" << m_unique_group_id << " subject:" << subject;

                        // 不再弹出自动提示（按需求：自动消息不要了）
                        qDebug() << "作业发布消息已发送到群组:" << m_unique_group_id;
                    });
            }

            // 每次打开前刷新可选科目：优先取当前老师在 /groups/members 返回的 teach_subjects
            QStringList subjects;
            UserInfo userInfo = CommonInfo::GetData();
            for (const auto& member : m_groupMemberInfo) {
                if (member.member_id == userInfo.classId) {
                    subjects = member.teach_subjects;
                    break;
                }
            }
            if (subjects.isEmpty()) {
                subjects << QString::fromUtf8(u8"语文") << QString::fromUtf8(u8"数学") << QString::fromUtf8(u8"英语");
            }
            homeworkEditDlg->setAvailableSubjects(subjects);

            homeworkEditDlg->setDate(QDate::currentDate());
            homeworkEditDlg->show();
            homeworkEditDlg->raise();
            homeworkEditDlg->activateWindow();
        }
    });
}

// 显示作业展示窗口（班级端）
void ScheduleDialog::showHomeworkViewDialog()
{
    if (!homeworkViewDlg) {
        homeworkViewDlg = new HomeworkViewDialog(this);
    }
    
    // 从缓存中读取当前日期的作业数据
    QDate currentDate = QDate::currentDate();
    QString dateStr = currentDate.toString(QStringLiteral("yyyy-MM-dd"));
    QList<HomeworkItem> homeworkList = m_homeworkByDate.value(dateStr);
    
    homeworkViewDlg->setDate(currentDate);
    homeworkViewDlg->setHomeworkList(homeworkList); // 传递作业列表而不是Map
    homeworkViewDlg->show();
    homeworkViewDlg->raise();
    homeworkViewDlg->activateWindow();
}

