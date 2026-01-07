#pragma execution_character_set("utf-8")
#include "ModalDialog.h"
#include "CommonInfo.h"
#include <qpainterpath>
#include <QRegExp>
#include <QMessageBox>

ModalDialog::ModalDialog(QWidget* parent)
    : QDialog(parent), m_dragging(false),
    m_backgroundColor(QColor(50, 50, 50)),
    m_borderColor(Qt::white),
    m_borderWidth(2), m_radius(6),
    m_visibleCloseButton(true)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(400, 250); // 调整窗口大小，使其更紧凑，类似图片样式

    m_httpHandler = new TAHttpHandler(this);
    if (m_httpHandler)
    {
        connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
            // 处理登录成功响应
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
            if (jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                // 服务器返回格式：{ "data": { "code": 200, "message": "登录成功", ... } }
                QJsonObject dataObj;
                if (obj.contains("data") && obj["data"].isObject()) {
                    dataObj = obj["data"].toObject();
                } else {
                    dataObj = obj;
                }
                
                // 获取code（可能是数字或字符串）
                int code = -1;
                if (dataObj["code"].isDouble()) {
                    code = dataObj["code"].toInt();
                } else {
                    code = dataObj["code"].toString().toInt();
                }
                
                QString message = dataObj["message"].toString();
                qDebug() << "登录响应 - code:" << code << "message:" << message;
                
                // 判断登录是否成功（code为200）
                if (code == 200 && message == "登录成功") {
                    // 登录成功，解析并保存登录信息
                    ClassLoginInfo loginInfo;
                    
                    // 解析返回的字段（新格式：从ta_classes表查询）
                    // 服务器返回user_id字段，我们保存为class_id
                    if (dataObj.contains("user_id")) {
                        if (dataObj["user_id"].isString()) {
                            loginInfo.class_id = dataObj["user_id"].toString();
                        } else {
                            loginInfo.class_id = QString::number(dataObj["user_id"].toInt());
                        }
                    }
                    
                    if (dataObj.contains("class_code")) {
                        loginInfo.class_code = dataObj["class_code"].toString();
                    }
                    
                    if (dataObj.contains("class_name")) {
                        loginInfo.class_name = dataObj["class_name"].toString();
                    }
                    
                    if (dataObj.contains("school_stage")) {
                        loginInfo.school_stage = dataObj["school_stage"].toString();
                    }
                    
                    if (dataObj.contains("grade")) {
                        loginInfo.grade = dataObj["grade"].toString();
                    }
                    
                    if (dataObj.contains("schoolid")) {
                        loginInfo.schoolid = dataObj["schoolid"].toString();
                    }
                    
                    if (dataObj.contains("access_token")) {
                        loginInfo.access_token = dataObj["access_token"].toString();
                    }
                    
                    if (dataObj.contains("token_type")) {
                        loginInfo.token_type = dataObj["token_type"].toString();
                    }
                    
                    // 保存到全局结构体
                    CommonInfo::InitClassLoginInfo(loginInfo);
                    
                    // 兼容旧代码，设置user_id（尝试转换为整数，如果失败则使用0）
                    if (!loginInfo.class_id.isEmpty()) {
                        bool ok;
                        int classIdInt = loginInfo.class_id.toInt(&ok);
                        if (ok) {
                            user_id = classIdInt;
                        } else {
                            // 如果class_id不是纯数字，使用0或保持原值
                            user_id = 0;
                        }
                    }
                    
                    qDebug() << "登录成功 - class_id:" << loginInfo.class_id 
                             << "class_code:" << loginInfo.class_code 
                             << "class_name:" << loginInfo.class_name
                             << "school_stage:" << loginInfo.school_stage
                             << "grade:" << loginInfo.grade
                             << "schoolid:" << loginInfo.schoolid;
                    
                    accept(); // 关闭对话框并返回 Accepted
                } else {
                    // 登录失败，显示错误信息
                    if (errLabel) {
                        errLabel->setText(message.isEmpty() ? "登录失败" : message);
                    }
                }
            }
            else
            {
                if (errLabel) {
                    errLabel->setText("网络错误：响应格式错误");
                }
            }
        });

        connect(m_httpHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
            if (errLabel)
            {
                QJsonDocument jsonDoc = QJsonDocument::fromJson(errResponseString.toUtf8());
                if (jsonDoc.isObject()) {
                    QJsonObject obj = jsonDoc.object();
                    QJsonObject dataObj;
                    // 检查响应格式
                    if (obj.contains("data") && obj["data"].isObject()) {
                        dataObj = obj["data"].toObject();
                    } else {
                        dataObj = obj;
                    }
                    
                    QString message = dataObj["message"].toString();
                    qDebug() << "登录失败 - message:" << message;
                    
                    if (!message.isEmpty()) {
                        errLabel->setText(message);
                    } else {
                        errLabel->setText("登录失败，请检查网络连接");
                    }
                }
                else
                {
                    errLabel->setText("网络错误：无法连接到服务器");
                }
            }
        });
    }

    // 主布局
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 40, 10, 10); // 预留标题空间

    //// 内容标签
    //label = new QLabel("请输入内容或选择操作", this);
    //label->setStyleSheet("color:white;");
    //mainLayout->addWidget(label);

    //// 按钮布局
    //QHBoxLayout* btnLayout = new QHBoxLayout();
    //okButton = new QPushButton("确认", this);
    //cancelButton = new QPushButton("取消", this);
    //btnLayout->addStretch();
    //btnLayout->addWidget(okButton);
    //btnLayout->addWidget(cancelButton);

    //mainLayout->addLayout(btnLayout);

    // 信号连接
    //connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    //connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // 顶部关闭按钮
    //closeButton = new QPushButton(this);
    //closeButton->setIcon(QIcon(":/icons/close_white.png"));
    //closeButton->setIconSize(QSize(16, 16));
    //closeButton->setFlat(true);
    //closeButton->setCursor(Qt::PointingHandCursor);

    // 标题 - 居中显示
    titleLabel = new QLabel("登录", this);
    titleLabel->setStyleSheet("color: white; font-size:16px; font-weight:bold;");
    titleLabel->setAlignment(Qt::AlignCenter);

    // 关闭按钮（右上角）
    closeButton = new QPushButton(this);
    closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
    closeButton->setIconSize(QSize(22, 22));
    closeButton->setFixedSize(QSize(22, 22));
    closeButton->setStyleSheet("background: transparent;");
    closeButton->move(width() - 24, 4);
    closeButton->hide();
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

    QHBoxLayout* titleLayout = new QHBoxLayout;
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->addStretch();
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    // 在布局中添加一个占位空间，用于放置关闭按钮
    QWidget* closeButtonPlaceholder = new QWidget(this);
    closeButtonPlaceholder->setFixedSize(22, 22);
    titleLayout->addWidget(closeButtonPlaceholder);

    // 班级编号输入 - 简化样式，类似图片中的输入框
    phoneEdit = new QLineEdit(this);
    phoneEdit->setPlaceholderText("请输入系统唯一班级编号");
    phoneEdit->setStyleSheet(
        "QLineEdit { background-color: rgba(255,255,255,0.1);"
        "border: none; border-radius:8px; color: white; padding: 12px; height:40px; font-size:14px; }"
        "QLineEdit:focus { background-color: rgba(255,255,255,0.15); }"
    );
    phoneEdit->setClearButtonEnabled(true);

    // 验证码输入 - 保留但隐藏，用于兼容旧代码
    codeEdit = new QLineEdit(this);
    codeEdit->setPlaceholderText("请输入验证码");
    codeEdit->setStyleSheet(
        "QLineEdit { background-color: rgba(255,255,255,0.1);"
        "border:none; border-radius:8px; color:white; padding: 12px; height:40px; font-size:14px; }"
        "QLineEdit:focus { background-color: rgba(255,255,255,0.15); }"
    );
    codeEdit->setClearButtonEnabled(true);
    codeEdit->hide(); // 隐藏验证码输入框，简化界面
    
    //getCodeButton = new QPushButton("获取验证码", this);
    getCodeButton = new QPushButton(tr("获取验证码"), this);
    getCodeButton->setStyleSheet(
        "QPushButton { background-color: rgba(255,255,255,0.15); color: white; border:none; border-radius:6px; padding:0 12px; }"
        "QPushButton:hover { background-color: rgba(255,255,255,0.25); }"
    );
    getCodeButton->setCursor(Qt::PointingHandCursor);
    getCodeButton->hide(); // 隐藏获取验证码按钮

    // 确定按钮 - 蓝色样式，类似图片中的按钮
    loginButton = new QPushButton("确定", this);
    loginButton->setStyleSheet(
        "QPushButton { background-color: #2E6BE6; color: white; font-size:15px; font-weight:bold; border-radius: 8px; height:42px; }"
        "QPushButton:hover { background-color: #5688E6; }"
        "QPushButton:pressed { background-color: #1E5BD6; }"
    );
    loginButton->setCursor(Qt::PointingHandCursor);

    // 底部 - 移除注册和重置密码链接
    registerLabel = new QLabel("", this);
    registerLabel->hide();
    resetPwdLabel = new QLabel("", this);
    resetPwdLabel->hide();

    QHBoxLayout* bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(registerLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(resetPwdLabel);

    errLabel = new QLabel(NULL, this);
    errLabel->setStyleSheet("color: red; font-size:16px; font-weight:bold;");

    // 主布局 - 简化布局，类似图片样式
    mainLayout->addLayout(titleLayout);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(phoneEdit);
    // 验证码输入框初始隐藏
    mainLayout->addWidget(codeEdit);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(loginButton);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(errLabel);
    mainLayout->addStretch();
    mainLayout->addLayout(bottomLayout);
    mainLayout->setContentsMargins(24, 20, 24, 24);
    setLayout(mainLayout);

    // 信号
    //connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(getCodeButton, &QPushButton::clicked, this, &ModalDialog::onGetCodeClicked);
    connect(&countdownTimer, &QTimer::timeout, this, &ModalDialog::onTimerTick);
    connect(loginButton, &QPushButton::clicked, this, &ModalDialog::onLoginClicked);
    // 移除密码登录按钮的信号连接
    // 连接 linkActivated 信号
    connect(registerLabel, &QLabel::linkActivated, this, [=](const QString& link) {
        //qDebug() << "用户点击了链接，href=" << link;
        if (link == "#") {
            // 执行注册逻辑
            //QMessageBox::information(this, "提示", "打开注册页面");
            m_registerLogin = true;
            accept();
        }
        });

    connect(resetPwdLabel, &QLabel::linkActivated, this, [=](const QString& link) {
        //qDebug() << "用户点击了链接，href=" << link;
        if (link == "#") {
            // 执行注册逻辑
            //QMessageBox::information(this, "提示", "打开注册页面");
            m_resetPwdLogin = true;
            accept();
        }
        });
}

ModalDialog::~ModalDialog() {}

void ModalDialog::setTitleName(const QString& name) {
    m_titleName = name;
    update();
}

void ModalDialog::visibleCloseButton(bool val)
{
    m_visibleCloseButton = val;
}

void ModalDialog::setBackgroundColor(const QColor& color) {
    m_backgroundColor = color;
    update();
}

void ModalDialog::setBorderColor(const QColor& color) {
    m_borderColor = color;
    update();
}

void ModalDialog::setBorderWidth(int val) {
    m_borderWidth = val;
    update();
}

void ModalDialog::setRadius(int val) {
    m_radius = val;
    update();
}

void ModalDialog::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect rect(0, 0, width(), height());
    QPainterPath path;
    path.addRoundedRect(rect, m_radius, m_radius);

    p.fillPath(path, QBrush(m_backgroundColor));
    QPen pen(m_borderColor, m_borderWidth);
    p.strokePath(path, pen);

    // 标题
    p.setPen(Qt::white);
    p.drawText(10, 25, m_titleName);
}

void ModalDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
    }
}

void ModalDialog::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
    }
}

void ModalDialog::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
}

void ModalDialog::leaveEvent(QEvent* event)
{
    QDialog::leaveEvent(event);
    closeButton->hide();
}

void ModalDialog::enterEvent(QEvent* event)
{
    QDialog::enterEvent(event);
    if (m_visibleCloseButton)
        closeButton->show();
}

void ModalDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    //initShow();
    // 关闭按钮位置：右上角，留出一些边距
    if (closeButton) {
        closeButton->move(this->width() - closeButton->width() - 4, 4);
    }
}

void ModalDialog::InitData()
{
    m_pwdLogin = false;
    m_registerLogin = false;
    m_resetPwdLogin = false;
}

void ModalDialog::onGetCodeClicked()
{
    // 验证班级编号不为空
    if (phoneEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入系统唯一班级编号！");
        return;
    }
    // 这里执行发送验证码逻辑
    // 模拟倒计时
    countdownValue = 60; // 60秒
    getCodeButton->setEnabled(false);
    getCodeButton->setText(QString("重新获取(%1)").arg(countdownValue));
    countdownTimer.start(1000);

    if (m_httpHandler)
    {
        QMap<QString, QString> params;
        params["phone"] = phoneEdit->text();
        m_httpHandler->post(QString("http://47.100.126.194:5000/send_verification_code"), params);
    }
}

void ModalDialog::onTimerTick()
{
    countdownValue--;
    if (countdownValue <= 0) {
        countdownTimer.stop();
        getCodeButton->setText("获取验证码");
        getCodeButton->setEnabled(true);
    }
    else {
        getCodeButton->setText(QString("重新获取(%1)").arg(countdownValue));
    }
}

void ModalDialog::onPwdLoginClicked()
{
    m_pwdLogin = true;
    accept();
}

void ModalDialog::onLoginClicked()
{
    // 验证班级编号不为空
    if (phoneEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入系统唯一班级编号！");
        return;
    }

    // 执行登录 - 发送班级编号和登录类型到服务器
    if (m_httpHandler)
    {
        QMap<QString, QString> params;
        params["class_number"] = phoneEdit->text();  // 班级唯一编号
        params["login_type"] = "class";  // 登录类型：班级端登录
        // 服务器登录接口地址
        m_httpHandler->post(QString("http://47.100.126.194:5000/login"), params);
    }
}