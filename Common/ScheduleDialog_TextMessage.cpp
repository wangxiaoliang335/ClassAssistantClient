#include "ScheduleDialog.h"
#include "TextMessageDialog.h"
#include <QDebug>
#include "CommonInfo.h"
#include "TaQTWebSocket.h"

// 显示文本消息发送窗口
void ScheduleDialog::showTextMessageDialog()
{
    if (!textMessageDlg) {
        // 获取班级名称，用于显示在标题中
        QString className = m_groupName;
        if (className.isEmpty()) {
            // 如果没有群组名称，尝试从班级ID获取
            className = QString("班级%1").arg(m_classid);
        }
        
        textMessageDlg = new TextMessageDialog(className, this);
        
        // 连接发送按钮，发送通知消息到班级
        connect(textMessageDlg, &QDialog::accepted, this, [this]() {
            QString content = textMessageDlg->getMessage();
            if (!content.trimmed().isEmpty() && !m_classid.isEmpty()) {
                // 通过WebSocket发送通知消息（方式一：纯JSON格式，推荐）
                QJsonObject jsonObj;
                jsonObj["type"] = "notification";
                jsonObj["class_id"] = m_classid;  // 必需：班级代码（class_code）
                jsonObj["content"] = content;     // 通知内容
                jsonObj["content_text"] = "通知"; // 通知类型（可选，默认 "notification"）
                
                // 获取发送者信息
                UserInfo userInfo = CommonInfo::GetData();
                if (!userInfo.strName.isEmpty()) {
                    jsonObj["sender_name"] = userInfo.strName;  // 发送者名称
                }
                // sender_id 可选，如果不传，服务器会使用当前登录的 user_id
                
                // 可选字段：如果提供了群组信息，可以包含
                if (!m_unique_group_id.isEmpty()) {
                    jsonObj["group_id"] = m_unique_group_id;  // 群组ID（可选）
                }
                if (!m_groupName.isEmpty()) {
                    jsonObj["group_name"] = m_groupName;  // 群组名称（可选）
                }
                
                QJsonDocument doc(jsonObj);
                const QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
                
                // 方式一：纯JSON格式发送（推荐）
                TaQTWebSocket::sendPrivateMessage(jsonString);
                
                qDebug() << "已通过WebSocket发送通知消息到班级:" << m_classid << "内容:" << content;
            } else {
                if (m_classid.isEmpty()) {
                    qWarning() << "班级ID为空，无法发送通知消息";
                }
            }
        });
    }
    
    // 重置文本输入框和未读消息数
    textMessageDlg->clearMessage();
    textMessageDlg->setUnreadCount(1); // 可以动态设置未读消息数
    textMessageDlg->show();
    textMessageDlg->raise();
    textMessageDlg->activateWindow();
    
    // 居中显示
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - textMessageDlg->width()) / 2;
    int y = (screenGeometry.height() - textMessageDlg->height()) / 2;
    textMessageDlg->move(x, y);
}

