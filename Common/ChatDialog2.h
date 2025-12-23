#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPixmap>
#include <QFile>

class ChatDialog : public QDialog
{
    Q_OBJECT
public:
    ChatDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("聊天对话框");
        resize(480, 640);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 消息列表
        m_listWidget = new QListWidget();
        m_listWidget->setStyleSheet("QListWidget { background-color: #222; border:none; }");
        mainLayout->addWidget(m_listWidget, 1);

        // 底部输入区
        QHBoxLayout* inputLayout = new QHBoxLayout();
        m_lineEdit = new QLineEdit();
        m_lineEdit->setPlaceholderText("请输入文字...");
        m_lineEdit->setMinimumHeight(36);

        QPushButton* btnSend = new QPushButton("发送");
        btnSend->setFixedSize(60, 36);

        inputLayout->addWidget(m_lineEdit);
        inputLayout->addWidget(btnSend);
        mainLayout->addLayout(inputLayout);

        // 信号绑定
        connect(btnSend, &QPushButton::clicked, this, &ChatDialog::sendMyMessage);
        connect(m_lineEdit, &QLineEdit::returnPressed, this, &ChatDialog::sendMyMessage);

        // 测试：别人发消息
        addMessage(":/avatar_teacher.png", "班主任", "李老师，今天家里有事，我们调一下课吧", false);
        addMessage(":/avatar_teacher2.png", "语文老师", "可以", false);
    }

private slots:
    void sendMyMessage()
    {
        QString text = m_lineEdit->text().trimmed();
        if (text.isEmpty())
            return;

        // 自己发的消息（右对齐，绿色气泡）
        addMessage(":/avatar_me.png", "我", text, true);
        m_lineEdit->clear();

        // 这里可以模拟别人回复
        // QTimer::singleShot(1000, this, [=](){
        //     addMessage(":/avatar_teacher2.png", "语文老师", "收到", false);
        // });
    }

private:
    QListWidget* m_listWidget;
    QLineEdit* m_lineEdit;

    void addMessage(const QString& avatarPath, const QString& senderName, const QString& messageText, bool isMine)
    {
        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        QWidget* msgWidget = new QWidget();
        QVBoxLayout* vLayout = new QVBoxLayout(msgWidget);
        vLayout->setContentsMargins(5, 5, 5, 5);

        // 顶部布局: 左边或右边头像 + 名字 + 气泡
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setContentsMargins(0, 0, 0, 0);

        QLabel* lblAvatar = new QLabel();
        lblAvatar->setFixedSize(36, 36);

        QPixmap avatarPixmap;
        if (QFile::exists(avatarPath))
            avatarPixmap.load(avatarPath);
        else
            avatarPixmap.fill(Qt::gray);
        avatarPixmap = avatarPixmap.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        lblAvatar->setPixmap(avatarPixmap);
        lblAvatar->setStyleSheet("border-radius: 18px;");

        QLabel* lblMessage = new QLabel(messageText);
        lblMessage->setWordWrap(true);
        lblMessage->setTextInteractionFlags(Qt::TextSelectableByMouse);

        if (isMine)
        {
            // 自己的消息（右侧，绿色气泡）
            lblMessage->setStyleSheet(
                "background-color: #4CAF50; color: white; "
                "border-radius: 8px; padding: 6px;"
            );
            hLayout->addStretch();           // 左边留空，让头像靠右
            hLayout->addWidget(lblMessage);  // 气泡在头像左边
            hLayout->addWidget(lblAvatar);   // 头像在最右侧
        }
        else
        {
            // 别人的消息（左侧，灰色气泡）
            lblMessage->setStyleSheet(
                "background-color: #555; color: white; "
                "border-radius: 8px; padding: 6px;"
            );
            hLayout->addWidget(lblAvatar);   // 头像在最左侧
            hLayout->addWidget(lblMessage);  // 气泡在头像右边
            hLayout->addStretch();           // 右边留空
        }

        vLayout->addLayout(hLayout);

        // 设置到 QListWidget
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);

        // 滚动到底部
        m_listWidget->scrollToBottom();
    }
};