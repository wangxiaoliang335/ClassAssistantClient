#pragma once

#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QDateTime>
#include <QDebug>
#include <QMetaObject>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QEvent>
#include <QPoint>
#include <QCursor>
#include <QRect>
#include <QDesktopWidget>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include "ImSDK/includes/TIMCloud.h"
#include "ImSDK/includes/TIMCloudDef.h"
#include "ImSDK/includes/TIMCloudCallback.h"

class FriendNotifyDialog : public QDialog
{
    Q_OBJECT
public:
    // 静态方法：提前注册好友添加请求回调（应在TIMInit之前调用）
    static void ensureCallbackRegistered()
    {
        static bool s_callbackRegistered = false;
        if (!s_callbackRegistered) {
            TIMSetFriendAddRequestCallback(staticFriendAddRequestCallback, nullptr);
            s_callbackRegistered = true;
            qDebug() << "FriendNotifyDialog: 好友添加请求回调已注册（TIMInit之前）";
        }
    }

    FriendNotifyDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        // 去掉标题栏
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground); // 设置透明背景以支持圆角
        setWindowTitle("好友通知");
        resize(750, 700);
        m_radius = 20; // 圆角半径
        setStyleSheet("QDialog { background-color: transparent; color: white; font-size: 14px; }"); // 透明背景，白色文字
        
        // 启用鼠标跟踪以检测鼠标进入/离开
        setMouseTracking(true);

        // 创建关闭按钮
        m_btnClose = new QPushButton("X", this);
        m_btnClose->setFixedSize(30, 30);
        m_btnClose->setStyleSheet(
            "QPushButton { background-color: orange; color: white; font-weight:bold; font-size: 14px; border: 1px solid #555; border-radius: 4px; }"
            "QPushButton:hover { background-color: #cc6600; }"
        );
        m_btnClose->hide(); // 初始隐藏
        connect(m_btnClose, &QPushButton::clicked, this, &QDialog::close);
        
        // 为关闭按钮安装事件过滤器，确保鼠标在按钮上时不会隐藏
        m_btnClose->installEventFilter(this);

        mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(15, 40, 15, 15); // 增加顶部边距，为关闭按钮留出空间
        
        // 设置静态实例指针（用于全局回调发出信号）
        s_instance() = this;
        deliverPendingRequests();
        
        // 连接全局信号，接收好友添加请求
        connect(this, &FriendNotifyDialog::friendAddRequestReceived, this, &FriendNotifyDialog::onFriendAddRequestReceived);
    }

signals:
    // 好友添加请求信号（用于全局回调通知）
    void friendAddRequestReceived(const QString& name, const QString& remark, 
                                 const QString& date, const QString& identifier);

private:
    // 静态回调函数（在TIMInit之前注册）
    static void staticFriendAddRequestCallback(const char* json_friend_add_request_pendency_array, const void* user_data)
    {
        if (!s_instance()) return;
        
        // 解析JSON数组
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json_friend_add_request_pendency_array), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "好友添加请求JSON解析失败:" << parseError.errorString();
            return;
        }
        
        if (!doc.isArray()) {
            qDebug() << "好友添加请求数据不是数组";
            return;
        }
        
        QJsonArray pendencyArray = doc.array();
        for (const QJsonValue& value : pendencyArray) {
            if (!value.isObject()) continue;
            
            QJsonObject pendencyObj = value.toObject();
            
            // 获取请求信息
            QString identifier = pendencyObj[kTIMFriendAddPendencyIdentifier].toString();
            QString nickName = pendencyObj[kTIMFriendAddPendencyNickName].toString();
            QString addSource = pendencyObj[kTIMFriendAddPendencyAddSource].toString();
            QString addWording = pendencyObj[kTIMFriendAddPendencyAddWording].toString();
            
            // 如果昵称为空，使用UserID
            if (nickName.isEmpty()) {
                nickName = identifier;
            }
            
            // 获取当前时间
            QDateTime currentTime = QDateTime::currentDateTime();
            QString timeStr = currentTime.toString("yyyy/MM/dd");
            
            FriendRequest req{ nickName, addWording, timeStr, identifier };
            
            // 发出全局信号，通知所有FriendNotifyDialog实例（使用QMetaObject::invokeMethod确保线程安全）
            if (s_instance()) {
                QMetaObject::invokeMethod(s_instance(), "onFriendAddRequestReceived", Qt::QueuedConnection,
                    Q_ARG(QString, req.nickName),
                    Q_ARG(QString, req.addWording),
                    Q_ARG(QString, req.timeStr),
                    Q_ARG(QString, req.identifier));
            } else {
                pendingFriendRequests().append(req);
            }
        }
    }
    
    // 信号槽处理函数
    void onFriendAddRequestReceived(const QString& name, const QString& remark, 
                                   const QString& date, const QString& identifier)
    {
        addFriendRequestItem(name, remark, date, identifier);
    }
    
    // 静态实例指针（用于发出信号）- 使用函数返回引用避免C++17要求
    static FriendNotifyDialog*& s_instance()
    {
        static FriendNotifyDialog* instance = nullptr;
        return instance;
    }

public:
    void InitData(QVector<QString> vecMsg = QVector<QString>())
    {
        if (m_bInit == true)
        {
            return;
        }

        // ===== 滚动区域 =====
        scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet("background-color: #808080; color: white;"); // 灰色背景，白色文字
        container = new QWidget;
        container->setStyleSheet("background-color: #808080; color: white;"); // 灰色背景，白色文字
        listLayout = new QVBoxLayout(container);
        listLayout->setSpacing(10);
        for (int i = 0; i < vecMsg.size(); i++)
        {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(vecMsg[i].toUtf8(), &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "JSON parse error:" << parseError.errorString();
            }
            else {
                if (doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj["data"].isArray())
                    {
                        // 4. 取出数组
                        QJsonArray arr = obj["data"].toArray();
                        // 5. 遍历数组
                        for (const QJsonValue& value : arr) {
                            if (value.isObject()) { // 每个元素是一个对象
                                QJsonObject obj = value.toObject();
                                int qSender_id = obj["sender_id"].toInt();
                                QString strSender_id = QString("%1").arg(qSender_id, 6, 10, QChar('0'));
                                QString SenderName = obj["sender_name"].toString();
                                int iContent_text = obj["content_text"].toInt();
                                QString updated_at = obj["updated_at"].toString();
                                int is_agreed = obj["is_agreed"].toInt();
                                QString msgContent;
                                if (1 == iContent_text)
                                {
                                    msgContent = "请求加为好友";
                                }
                                if (1 == iContent_text || 2 == iContent_text)
                                {
                                    addItem(listLayout, "头像", SenderName, msgContent, updated_at,
                                        "", is_agreed);
                                }
                            }
                        }
                    }
                }
            }
            
            // 添加示例条目
            /*addItem(listLayout, "头像", "英语-周老师", "请求加为好友", "2025/09/09",
                "我是来自群聊“集鑫中学2025级3班”的英语-周老师", "已同意");
            addItem(listLayout, "头像", "零声教育【Mark老师】", "请求加为好友", "2025/07/13",
                "我是来自群聊“C++音视频服务器开发交流群”的零声教育", "已同意");
            addItem(listLayout, "头像", "廖深意", "请求加为好友", "2024/11/13",
                "我是来自群聊“驱动交流群”的", "已同意");
            addItem(listLayout, "头像", "a", "请求加为好友", "2024/11/13",
                "我是来自群聊“驱动交流群”的", "已同意");
            addItem(listLayout, "头像", "世纪无成", "请求加为好友", "2024/11/13",
                "我是来自群聊“WINDOWS驱动开发”的世纪无成", "已同意");
            addItem(listLayout, "头像", "Miky", "请求加为好友", "2024/11/02",
                "我是Miky Sunny，APP，小程序", "已同意");*/
        }
        listLayout->addStretch();
        scroll->setWidget(container);
        mainLayout->addWidget(scroll);
        m_bInit = true;
    }

private slots:
    // 添加好友请求项（在主线程中调用）
    void addFriendRequestItem(const QString& name, const QString& remark, 
                             const QString& date, const QString& identifier)
    {
        // 检查是否已经初始化了UI
        if (!listLayout) {
            // 如果还没有初始化，先初始化UI
            scroll = new QScrollArea;
            scroll->setWidgetResizable(true);
            scroll->setStyleSheet("background-color: #808080; color: white;"); // 灰色背景，白色文字
            container = new QWidget;
            container->setStyleSheet("background-color: #808080; color: white;"); // 灰色背景，白色文字
            listLayout = new QVBoxLayout(container);
            listLayout->setSpacing(10);
            listLayout->addStretch();
            scroll->setWidget(container);
            mainLayout->addWidget(scroll);
        }
        
        // 在stretch之前插入新项
        int insertIndex = listLayout->count() - 1; // 在stretch之前
        if (insertIndex < 0) insertIndex = 0;
        
        // 创建临时布局来插入
        QFrame* itemFrame = new QFrame;
        itemFrame->setStyleSheet("background-color: #808080; border-radius: 8px;"); // 灰色背景
        QVBoxLayout* outerLayout = new QVBoxLayout(itemFrame);

        // 第一行：头像 + 名称 + 请求 + 日期
        QHBoxLayout* topLayout = new QHBoxLayout;
        QLabel* avatar = new QLabel("头像");
        avatar->setFixedSize(50, 50);
        avatar->setStyleSheet("background-color: lightgray; border-radius: 25px; text-align:center;");
        QLabel* lblName = new QLabel(QString("<b style='color:#66b3ff'>%1</b> <span style='color:#ffffff'>请求加为好友</span>").arg(name));
        lblName->setStyleSheet("background-color: #808080; color: #ffffff;"); // 灰色背景，白色文字
        QLabel* lblDate = new QLabel(date);
        lblDate->setStyleSheet("background-color: #808080; color: #ffffff;"); // 灰色背景，白色文字
        topLayout->addWidget(avatar);
        topLayout->addWidget(lblName);
        topLayout->addStretch();
        topLayout->addWidget(lblDate);
        outerLayout->addLayout(topLayout);

        // 备注信息
        if (!remark.isEmpty()) {
            QLabel* lblRemark = new QLabel("留言: " + remark);
            lblRemark->setWordWrap(true);
            lblRemark->setStyleSheet("background-color: #808080; color: #ffffff;"); // 灰色背景，白色文字
            outerLayout->addWidget(lblRemark);
        }

        // 同意按钮
        QPushButton* agreeBtn = new QPushButton("同意");
        agreeBtn->setStyleSheet(
            "QPushButton {"
            "    color: gray;"
            "    border: 1px solid gray;"
            "    border-radius: 6px;"
            "    padding: 6px 12px;"
            "    background-color: white;"
            "}"
            "QPushButton:hover {"
            "    color: black;"
            "    border: 1px solid #409EFF;"
            "}"
            "QPushButton:pressed {"
            "    background-color: #f2f2f2;"
            "    border: 1px solid #66b1ff;"
            "}"
        );
        outerLayout->addWidget(agreeBtn, 0, Qt::AlignRight);
        
        // 连接同意按钮的点击事件
        QObject::connect(agreeBtn, &QPushButton::clicked, [agreeBtn, outerLayout, identifier, this]() {
            // TODO: 调用腾讯IM SDK的同意好友请求接口
            qDebug() << "同意好友请求，UserID:" << identifier;
            
            // 替换为"已同意"标签
            QLabel* label = new QLabel("已同意");
            label->setStyleSheet("color: green; font-weight: bold;");
            outerLayout->replaceWidget(agreeBtn, label);
            agreeBtn->deleteLater();
        });
        
        listLayout->insertWidget(insertIndex, itemFrame);
    }

private:
    void addItem(QVBoxLayout* parent,
        const QString& avatarText,
        const QString& name,
        const QString& action,
        const QString& date,
        const QString& remark,
        const int& is_agreed)
    {
        QFrame* itemFrame = new QFrame;
        itemFrame->setStyleSheet("background-color: #808080; border-radius: 8px;"); // 灰色背景
        QVBoxLayout* outerLayout = new QVBoxLayout(itemFrame);

        // 第一行：头像 + 名称 + 请求 + 日期
        QHBoxLayout* topLayout = new QHBoxLayout;
        QLabel* avatar = new QLabel(avatarText);
        avatar->setFixedSize(50, 50);
        avatar->setStyleSheet("background-color: lightgray; border-radius: 25px; text-align:center;");
        QLabel* lblName = new QLabel(QString("<b style='color:#66b3ff'>%1</b> <span style='color:#ffffff'>%2</span>").arg(name, action));
        lblName->setStyleSheet("background-color: #808080; color: #ffffff;"); // 灰色背景，白色文字
        QLabel* lblDate = new QLabel(date);
        lblDate->setStyleSheet("background-color: #808080; color: #ffffff;"); // 灰色背景，白色文字
        topLayout->addWidget(avatar);
        topLayout->addWidget(lblName);
        topLayout->addStretch();
        topLayout->addWidget(lblDate);
        outerLayout->addLayout(topLayout);

        // 备注信息
        QLabel* lblRemark = new QLabel("留言: " + remark);
        lblRemark->setWordWrap(true);
        lblRemark->setStyleSheet("background-color: #808080; color: #ffffff;"); // 灰色背景，白色文字
        outerLayout->addWidget(lblRemark);

        if (1 == is_agreed)
        {
            // 状态
            QLabel* lblStatus = new QLabel("已同意");
            lblStatus->setStyleSheet("background-color: #808080; color: #ffffff;"); // 灰色背景，白色文字
            outerLayout->addWidget(lblStatus, 0, Qt::AlignRight);
        }
        else
        {
            // 状态
            QPushButton* lblStatusBtn = new QPushButton("同意");
            //veclblStatusBtn.push_back(lblStatusBtn);
            lblStatusBtn->setStyleSheet(
                "QPushButton {"
                "    color: gray;"                  /* 默认文字颜色 */
                "    border: 1px solid gray;"        /* 默认边框 */
                "    border-radius: 6px;"            /* 圆角半径 */
                "    padding: 6px 12px;"             /* 内边距: 上下6px, 左右12px */
                "    background-color: white;"       /* 默认背景色 */
                "}"
                "QPushButton:hover {"
                "    color: black;"                  /* 悬停时文字变黑 */
                "    border: 1px solid #409EFF;"      /* 悬停时边框变蓝 */
                "}"
                "QPushButton:pressed {"
                "    background-color: #f2f2f2;"     /* 点击时浅灰背景 */
                "    border: 1px solid #66b1ff;"      /* 点击时浅蓝边框 */
                "}"
            );
            outerLayout->addWidget(lblStatusBtn, 0, Qt::AlignRight);

            //QLabel* lblStatus = new QLabel("已同意");
            //lblStatus->setStyleSheet("color: gray;");
            //outerLayout->addWidget(lblStatus, 0, Qt::AlignRight);
            //lblStatus->hide();
            QObject::connect(lblStatusBtn, &QPushButton::clicked, [lblStatusBtn, outerLayout]() {
                //lblStatus->show();
                //lblStatusBtn->hide();
                // 1. 从布局中移除按钮
                //outerLayout->removeWidget(lblStatusBtn);
                // 2. 删除 QPushButton（可选，如果不想复用）
                //lblStatusBtn->deleteLater();

                // 3. 创建新的 QLabel
                QLabel* label = new QLabel("已同意");
                label->setStyleSheet("color: green; font-weight: bold;"); // 样式可选

                // 4. 添加到布局
                outerLayout->addWidget(label);
                outerLayout->replaceWidget(lblStatusBtn, label);
                lblStatusBtn->deleteLater();  // 这里更安全，因为布局不会再持有按钮
                });
        }
        parent->addWidget(itemFrame);
    }
    QVBoxLayout* mainLayout = NULL;
    QScrollArea* scroll = NULL;
    QWidget* container = NULL;
    QVBoxLayout* listLayout = NULL;
    bool m_bInit = false;
    QPushButton* m_btnClose = nullptr; // 关闭按钮
    QPoint m_dragPosition; // 用于窗口拖动
    int m_radius; // 圆角半径

    struct FriendRequest {
        QString nickName;
        QString addWording;
        QString timeStr;
        QString identifier;
    };

    static QList<FriendRequest>& pendingFriendRequests()
    {
        static QList<FriendRequest> cache;
        return cache;
    }

    void deliverPendingRequests()
    {
        auto& cache = pendingFriendRequests();
        for (const auto& req : cache) {
            onFriendAddRequestReceived(req.nickName, req.addWording, req.timeStr, req.identifier);
        }
        cache.clear();
    }

protected:
    // 绘制圆角窗口
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        QRect rect(0, 0, width(), height());
        QPainterPath path;
        path.addRoundedRect(rect, m_radius, m_radius);
        
        p.fillPath(path, QBrush(QColor(128, 128, 128))); // 灰色背景
    }
    
    // 鼠标进入窗口时显示关闭按钮
    void enterEvent(QEvent* event) override
    {
        if (m_btnClose) {
            m_btnClose->show();
        }
        QDialog::enterEvent(event);
    }
    
    // 鼠标离开窗口时隐藏关闭按钮
    void leaveEvent(QEvent* event) override
    {
        // 检查鼠标是否真的离开了窗口（包括关闭按钮）
        QPoint globalPos = QCursor::pos();
        QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
        if (!widgetRect.contains(globalPos) && m_btnClose) {
            // 如果鼠标不在窗口内，检查是否在关闭按钮上
            QRect btnRect = QRect(m_btnClose->mapToGlobal(QPoint(0, 0)), m_btnClose->size());
            if (!btnRect.contains(globalPos)) {
                m_btnClose->hide();
            }
        }
        QDialog::leaveEvent(event);
    }
    
    // 窗口大小改变时更新关闭按钮位置
    void resizeEvent(QResizeEvent* event) override
    {
        if (m_btnClose) {
            m_btnClose->move(width() - 35, 5);
        }
        QDialog::resizeEvent(event);
    }
    
    // 窗口显示时更新关闭按钮位置
    void showEvent(QShowEvent* event) override
    {
        if (m_btnClose) {
            m_btnClose->move(width() - 35, 5);
            // 窗口显示时也显示关闭按钮
            m_btnClose->show();
        }
        
        // 确保窗口位置在屏幕可见区域内
        QRect screenGeometry = QApplication::desktop()->availableGeometry();
        QRect windowGeometry = geometry();
        
        // 如果窗口完全在屏幕外，移动到屏幕中央
        if (!screenGeometry.intersects(windowGeometry)) {
            move(screenGeometry.center() - QPoint(windowGeometry.width() / 2, windowGeometry.height() / 2));
        }
        
        // 确保窗口显示在最前面
        raise();
        activateWindow();
        QDialog::showEvent(event);
    }
    
    // 事件过滤器，处理关闭按钮的鼠标事件
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (obj == m_btnClose) {
            if (event->type() == QEvent::Enter) {
                // 鼠标进入关闭按钮时确保显示
                m_btnClose->show();
            } else if (event->type() == QEvent::Leave) {
                // 鼠标离开关闭按钮时，检查是否还在窗口内
                QPoint globalPos = QCursor::pos();
                QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
                if (!widgetRect.contains(globalPos)) {
                    m_btnClose->hide();
                }
            }
        }
        return QDialog::eventFilter(obj, event);
    }
    
    // 重写鼠标事件以实现窗口拖动
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    
    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (event->buttons() & Qt::LeftButton && !m_dragPosition.isNull()) {
            move(event->globalPos() - m_dragPosition);
            event->accept();
        }
    }
};

