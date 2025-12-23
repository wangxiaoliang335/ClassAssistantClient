#pragma execution_character_set("utf-8")
#include "PwdLoginModalDialog.h"
#include <qpainterpath>
#include <QRegExp>
#include <QMessageBox>

PwdLoginModalDialog::PwdLoginModalDialog(QWidget* parent)
    : QDialog(parent), m_dragging(false),
    m_backgroundColor(QColor(50, 50, 50)),
    m_borderColor(Qt::white),
    m_borderWidth(2), m_radius(6),
    m_visibleCloseButton(true)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(420, 320);

    m_httpHandler = new TAHttpHandler(this);
    if (m_httpHandler)
    {
        connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
            //成功消息就不发送了
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
            if (jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                if (obj["data"].isObject())
                {
                    QJsonObject oTmp = obj["data"].toObject();
                    QString strTmp = oTmp["message"].toString();
                    qDebug() << "status:" << oTmp["code"].toString();
                    qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
                    user_id = oTmp["user_id"].toInt();
                    //errLabel->setText(strTmp);
                    if (strTmp == "登录成功")
                    {
                        accept(); // 验证通过，关闭对话框并返回 Accepted
                    }
                }
            }
            else
            {
                errLabel->setText("网络错误");
            }
            });

        connect(m_httpHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
            if (errLabel)
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
                        errLabel->setText(strTmp);
                    }
                    else
                    {
                        if (obj["detail"].isString())
                        {
                            if (0 == obj["detail"].toString().compare("Not Found"))
                            {
                                errLabel->setText("网络错误");
                            }
                        }
                    }
                }
                else
                {
                    errLabel->setText("网络错误");
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

    // 标题
    titleLabel = new QLabel("教师个人电脑登录", this);
    titleLabel->setStyleSheet("color: white; font-size:16px; font-weight:bold;");

    pwdLoginButton = new QPushButton("验证码登录", this);
    pwdLoginButton->setFlat(true);
    pwdLoginButton->setCursor(Qt::PointingHandCursor);
    pwdLoginButton->setStyleSheet("color: white; font-size:14px;");

    // 关闭按钮（右上角）
    closeButton = new QPushButton(this);
    closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
    closeButton->setIconSize(QSize(22, 22));
    closeButton->setFixedSize(QSize(22, 22));
    closeButton->setStyleSheet("background: transparent;");
    closeButton->move(width() - 24, 4);
    closeButton->hide();
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

    QHBoxLayout* titleLayout = new QHBoxLayout(this);
    
    titleLayout->addWidget(titleLabel);
    titleLayout->addSpacing(152);
    titleLayout->addWidget(pwdLoginButton);
    titleLayout->addStretch();
    titleLayout->addWidget(closeButton);

    // 手机号输入
    phoneEdit = new QLineEdit(this);
    phoneEdit->setPlaceholderText("请输入手机号");
    phoneEdit->setStyleSheet(
        "QLineEdit { background-color: rgba(255,255,255,0.08);"
        "border: none; border-radius:6px; color: white; padding-left:28px; height:36px; }"
    );
    phoneEdit->setClearButtonEnabled(true);
    QLabel* phoneIcon = new QLabel(this);
    phoneIcon->setPixmap(QPixmap(":/icons/phone.png").scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    phoneIcon->setFixedSize(16, 16);
    phoneIcon->setStyleSheet("background: transparent;");
    QHBoxLayout* phoneLayout = new QHBoxLayout(this);
    phoneLayout->setContentsMargins(8, 0, 8, 0);
    phoneLayout->addWidget(phoneIcon);
    phoneLayout->addWidget(phoneEdit);
    QWidget* phoneWidget = new QWidget(this);
    phoneWidget->setLayout(phoneLayout);
    phoneWidget->setStyleSheet("background-color: rgba(255,255,255,0.08); border-radius:6px;");

    // 验证码输入
    codeEdit = new QLineEdit(this);
    codeEdit->setPlaceholderText("请输入密码");
    codeEdit->setStyleSheet(
        "QLineEdit { background-color: rgba(255,255,255,0.08);"
        "border:none; border-radius:6px; color:white; padding-left:28px; height:36px; }"
    );
    codeEdit->setClearButtonEnabled(true);
    QLabel* codeIcon = new QLabel(this);
    codeIcon->setPixmap(QPixmap(":/icons/code.png").scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    codeIcon->setFixedSize(16, 16);
    codeIcon->setStyleSheet("background: transparent;");
    //getCodeButton = new QPushButton("获取验证码", this);
    /*getCodeButton = new QPushButton(tr("获取验证码"), this);
    getCodeButton->setStyleSheet(
        "QPushButton { background-color: rgba(255,255,255,0.15); color: white; border:none; border-radius:6px; padding:0 12px; }"
        "QPushButton:hover { background-color: rgba(255,255,255,0.25); }"
    );
    getCodeButton->setCursor(Qt::PointingHandCursor);*/

    QHBoxLayout* codeLayout = new QHBoxLayout(this);
    codeLayout->setContentsMargins(8, 0, 8, 0);
    codeLayout->addWidget(codeIcon);
    codeLayout->addWidget(codeEdit);
    codeLayout->addSpacing(5);
    //codeLayout->addWidget(getCodeButton);
    QWidget* codeWidget = new QWidget(this);
    codeWidget->setLayout(codeLayout);
    codeWidget->setStyleSheet("background-color: rgba(255,255,255,0.08); border-radius:6px;");

    // 登录按钮
    loginButton = new QPushButton("登 录", this);
    loginButton->setStyleSheet(
        "QPushButton { background-color: #2E6BE6; color: white; font-size:15px; font-weight:bold; border-radius: 6px; height:38px; }"
        "QPushButton:hover { background-color: #5688E6; }"
    );
    loginButton->setCursor(Qt::PointingHandCursor);

    errLabel = new QLabel(NULL, this);
    errLabel->setStyleSheet("color: red; font-size:16px; font-weight:bold;");

    // 底部
    registerLabel = new QLabel("还没有账号？ <a style='color:#4A90E2;' href='#'>立即注册</a>", this);
    registerLabel->setTextFormat(Qt::RichText);
    registerLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    registerLabel->setStyleSheet("color: white; font-size:12px;");
    resetPwdLabel = new QLabel("<a style='color:white;' href='#'>重置密码</a>", this);
    resetPwdLabel->setTextFormat(Qt::RichText);
    resetPwdLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    resetPwdLabel->setStyleSheet("color: white; font-size:12px;");

    QHBoxLayout* bottomLayout = new QHBoxLayout(this);
    bottomLayout->addWidget(registerLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(resetPwdLabel);

    // 主布局
    //QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(titleLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(phoneWidget);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(codeWidget);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(loginButton);
    mainLayout->addStretch();
    mainLayout->addWidget(errLabel);
    mainLayout->addLayout(bottomLayout);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    setLayout(mainLayout);

    // 信号
    //connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(pwdLoginButton, &QPushButton::clicked, this, &PwdLoginModalDialog::onSwitchPwdLogin);
    connect(&countdownTimer, &QTimer::timeout, this, &PwdLoginModalDialog::onTimerTick);
    connect(loginButton, &QPushButton::clicked, this, &PwdLoginModalDialog::onLoginClicked);

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

PwdLoginModalDialog::~PwdLoginModalDialog() {}

void PwdLoginModalDialog::setTitleName(const QString& name) {
    m_titleName = name;
    update();
}

void PwdLoginModalDialog::visibleCloseButton(bool val)
{
    m_visibleCloseButton = val;
}

void PwdLoginModalDialog::setBackgroundColor(const QColor& color) {
    m_backgroundColor = color;
    update();
}

void PwdLoginModalDialog::setBorderColor(const QColor& color) {
    m_borderColor = color;
    update();
}

void PwdLoginModalDialog::setBorderWidth(int val) {
    m_borderWidth = val;
    update();
}

void PwdLoginModalDialog::setRadius(int val) {
    m_radius = val;
    update();
}

void PwdLoginModalDialog::paintEvent(QPaintEvent* event) {
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

void PwdLoginModalDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
    }
}

void PwdLoginModalDialog::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
    }
}

void PwdLoginModalDialog::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
}

void PwdLoginModalDialog::leaveEvent(QEvent* event)
{
    QDialog::leaveEvent(event);
    closeButton->hide();
}

void PwdLoginModalDialog::enterEvent(QEvent* event)
{
    QDialog::enterEvent(event);
    if (m_visibleCloseButton)
        closeButton->show();
}

void PwdLoginModalDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    //initShow();
    closeButton->move(this->width() - 22, 0);
}

void PwdLoginModalDialog::onGetCodeClicked()
{
    QRegExp phoneRegex("^1[3-9]\\d{9}$");
    if (!phoneRegex.exactMatch(phoneEdit->text())) {
        QMessageBox::warning(this, "提示", "请输入有效的11位手机号码！");
        return;
    }
    // 这里执行发送验证码逻辑
    // 模拟倒计时
    countdownValue = 60; // 60秒
    //getCodeButton->setEnabled(false);
    //getCodeButton->setText(QString("重新获取(%1)").arg(countdownValue));
    countdownTimer.start(1000);
}

void PwdLoginModalDialog::onSwitchPwdLogin()
{
    m_verification_code_Login = true;
    accept();
}

void PwdLoginModalDialog::InitData()
{
    m_verification_code_Login = false;
    m_registerLogin = false;
    m_resetPwdLogin = false;
}

void PwdLoginModalDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event); // 保留父类行为
    //counter = 0; // 初始化变量
    //updateUI();  // 刷新界面
}

//bool PwdLoginModalDialog::event(QEvent* e) {
//    if (e->type() == QEvent::Polish) {
//        //counter = 0;
//        qDebug() << "Dialog polished, variables reset.";
//    }
//    return QDialog::event(e);
//}

void PwdLoginModalDialog::onTimerTick()
{
    countdownValue--;
    if (countdownValue <= 0) {
        countdownTimer.stop();
        //getCodeButton->setText("获取验证码");
        //getCodeButton->setEnabled(true);
    }
    else {
        //getCodeButton->setText(QString("重新获取(%1)").arg(countdownValue));
    }
}

void PwdLoginModalDialog::onLoginClicked()
{
    //QRegExp codeRegex("^\\d{6}$");
    //if (!codeRegex.exactMatch(codeEdit->text())) {
    //    QMessageBox::warning(this, "提示", "请输入密码！");
    //    return;
    //}
    if (codeEdit->text().isEmpty())
    {
        QMessageBox::warning(this, "提示", "请输入密码！");
        return;
    }

    if (m_httpHandler)
    {
        QMap<QString, QString> params;
        params["phone"] = phoneEdit->text();
        params["password"] = codeEdit->text();
        m_httpHandler->post(QString("http://47.100.126.194:5000/login"), params);
    }
    //accept(); // 验证通过，关闭对话框并返回 Accepted
}