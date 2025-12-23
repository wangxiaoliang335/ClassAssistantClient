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
#include <QTimer>

class ChatDialog : public QDialog
{
    Q_OBJECT
public:
    ChatDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("聊天对话框");
        resize(480, 640);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // ===== 消息列表 =====
        m_listWidget = new QListWidget();
        m_listWidget->setStyleSheet(
            "QListWidget { background-color: #F5F5F5; border:none; }"
        );
        mainLayout->addWidget(m_listWidget, 1);

        // ===== 底部输入区 =====
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

        // 测试数据
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

        // 模拟对方Auto回复
        QTimer::singleShot(1000, this, [=]() {
            addMessage(":/avatar_teacher2.png", "语文老师", "好的，收到", false);
        });
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

        // 顶部布局: 头像 + 气泡
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setContentsMargins(0, 0, 0, 0);

        // 头像
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

        // 消息气泡
        QLabel* lblMessage = new QLabel(messageText);
        lblMessage->setWordWrap(true);
        lblMessage->setTextInteractionFlags(Qt::TextSelectableByMouse); // 允许选文字

        if (isMine)
        {
            // 自己消息（右侧绿色）
            lblMessage->setStyleSheet(
                "background-color: #A0E75A; color: black; "
                "border-radius: 12px; padding: 8px;"
            );
            hLayout->addStretch();           // 左边留空，让消息右对齐
            hLayout->addWidget(lblMessage);
            hLayout->addWidget(lblAvatar);
        }
        else
        {
            // 对方消息（左侧灰色）
            lblMessage->setStyleSheet(
                "background-color: #EAEAEA; color: black; "
                "border-radius: 12px; padding: 8px;"
            );
            hLayout->addWidget(lblAvatar);
            hLayout->addWidget(lblMessage);
            hLayout->addStretch();
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
