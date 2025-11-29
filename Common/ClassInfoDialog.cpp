#pragma execution_character_set("utf-8")
#include "ClassInfoDialog.h"
#include <QPainterPath>
#include <QPixmap>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

ClassInfoDialog::ClassInfoDialog(QWidget *parent)
    : QDialog(parent),
    m_backgroundColor(QColor(50, 50, 50)),
    m_borderColor(Qt::white),
    m_borderWidth(2),
    m_radius(6),
    m_dragging(false),
    m_visibleCloseButton(true),
    m_isEditMode(false),
    m_httpHandler(nullptr)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowTitle("班级信息");
    resize(500, 600);

    // 初始化HTTP处理器
    m_httpHandler = new TAHttpHandler(this);
    if (m_httpHandler) {
        connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
            handleClassInfoResponse(responseString);
        });
        
        connect(m_httpHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
            qDebug() << "获取班级信息失败:" << errResponseString;
            // 失败时仍然显示本地缓存的信息
            updateClassInfo();
        });
    }

    setupUI();
    InitData();
}

ClassInfoDialog::~ClassInfoDialog()
{
}

void ClassInfoDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 50, 20, 20);
    mainLayout->setSpacing(15);

    // 标题栏
    QHBoxLayout* titleLayout = new QHBoxLayout;
    titleLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* titleLabel = new QLabel("班级信息", this);
    titleLabel->setStyleSheet("color: white; font-size: 18px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    
    m_editButton = new QPushButton("班级端", this);
    m_editButton->setStyleSheet(
        "QPushButton { background-color: transparent; color: #FF8C00; border: 1px solid #FF8C00; border-radius: 4px; padding: 4px 12px; font-size: 12px; }"
        "QPushButton:hover { background-color: rgba(255, 140, 0, 0.2); }"
    );
    m_editButton->setFixedSize(60, 24);
    
    titleLayout->addStretch();
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(m_editButton);
    
    mainLayout->addLayout(titleLayout);

    // 关闭按钮（右上角）
    m_closeButton = new QPushButton(this);
    m_closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
    m_closeButton->setIconSize(QSize(22, 22));
    m_closeButton->setFixedSize(QSize(22, 22));
    m_closeButton->setStyleSheet("background: transparent;");
    m_closeButton->move(width() - 24, 4);
    m_closeButton->hide();
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);

    // 班级标识区域
    QHBoxLayout* classIdLayout = new QHBoxLayout;
    classIdLayout->setSpacing(15);
    
    // 头像
    m_avatarLabel = new QLabel(this);
    m_avatarLabel->setFixedSize(80, 80);
    m_avatarLabel->setStyleSheet(
        "QLabel { border: 2px solid white; border-radius: 40px; background-color: rgba(255,255,255,0.1); }"
    );
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setScaledContents(true);
    
    // 头像编辑图标（使用铅笔图标，如果有的话，否则使用占位符）
    QLabel* editIconLabel = new QLabel(this);
    editIconLabel->setFixedSize(20, 20);
    editIconLabel->setStyleSheet("background: transparent;");
    // 可以添加点击事件来编辑头像
    // 暂时不显示图标，如果需要可以添加图标资源
    
    QVBoxLayout* avatarLayout = new QVBoxLayout;
    avatarLayout->setAlignment(Qt::AlignCenter);
    avatarLayout->addWidget(m_avatarLabel);
    avatarLayout->addWidget(editIconLabel, 0, Qt::AlignCenter);
    
    QWidget* avatarWidget = new QWidget;
    avatarWidget->setLayout(avatarLayout);
    
    // 班级名称和编号
    QVBoxLayout* classInfoLayout = new QVBoxLayout;
    classInfoLayout->setSpacing(5);
    
    m_classNameLabel = new QLabel("", this);
    m_classNameLabel->setStyleSheet("color: white; font-size: 20px; font-weight: bold;");
    
    m_classCodeLabel = new QLabel("", this);
    m_classCodeLabel->setStyleSheet("color: rgba(255,255,255,0.7); font-size: 14px;");
    
    classInfoLayout->addWidget(m_classNameLabel);
    classInfoLayout->addWidget(m_classCodeLabel);
    classInfoLayout->addStretch();
    
    classIdLayout->addWidget(avatarWidget);
    classIdLayout->addLayout(classInfoLayout);
    classIdLayout->addStretch();
    
    mainLayout->addLayout(classIdLayout);
    mainLayout->addSpacing(20);

    // 详细信息列表
    QVBoxLayout* infoLayout = new QVBoxLayout;
    infoLayout->setSpacing(12);

    // 地址
    QHBoxLayout* addressLayout = new QHBoxLayout;
    m_addressLabel = new QLabel("地址", this);
    m_addressLabel->setStyleSheet("color: rgba(255,255,255,0.7); font-size: 14px; min-width: 80px;");
    m_addressValueLabel = new QLabel("", this);
    m_addressValueLabel->setStyleSheet("color: white; font-size: 14px;");
    m_addressValueLabel->setWordWrap(true);
    addressLayout->addWidget(m_addressLabel);
    addressLayout->addWidget(m_addressValueLabel);
    addressLayout->addStretch();
    infoLayout->addLayout(addressLayout);

    // 学校名
    QHBoxLayout* schoolNameLayout = new QHBoxLayout;
    m_schoolNameLabel = new QLabel("学校名", this);
    m_schoolNameLabel->setStyleSheet("color: rgba(255,255,255,0.7); font-size: 14px; min-width: 80px;");
    m_schoolNameValueLabel = new QLabel("", this);
    m_schoolNameValueLabel->setStyleSheet("color: white; font-size: 14px;");
    m_schoolNameValueLabel->setWordWrap(true);
    schoolNameLayout->addWidget(m_schoolNameLabel);
    schoolNameLayout->addWidget(m_schoolNameValueLabel);
    schoolNameLayout->addStretch();
    infoLayout->addLayout(schoolNameLayout);

    // 学段
    QHBoxLayout* schoolStageLayout = new QHBoxLayout;
    m_schoolStageLabel = new QLabel("学段", this);
    m_schoolStageLabel->setStyleSheet("color: rgba(255,255,255,0.7); font-size: 14px; min-width: 80px;");
    m_schoolStageValueLabel = new QLabel("", this);
    m_schoolStageValueLabel->setStyleSheet("color: white; font-size: 14px;");
    schoolStageLayout->addWidget(m_schoolStageLabel);
    schoolStageLayout->addWidget(m_schoolStageValueLabel);
    schoolStageLayout->addStretch();
    infoLayout->addLayout(schoolStageLayout);

    // 年级
    QHBoxLayout* gradeLayout = new QHBoxLayout;
    m_gradeLabel = new QLabel("年级", this);
    m_gradeLabel->setStyleSheet("color: rgba(255,255,255,0.7); font-size: 14px; min-width: 80px;");
    m_gradeValueLabel = new QLabel("", this);
    m_gradeValueLabel->setStyleSheet("color: white; font-size: 14px;");
    gradeLayout->addWidget(m_gradeLabel);
    gradeLayout->addWidget(m_gradeValueLabel);
    gradeLayout->addStretch();
    infoLayout->addLayout(gradeLayout);

    // 班级
    QHBoxLayout* classLayout = new QHBoxLayout;
    m_classLabel = new QLabel("班级", this);
    m_classLabel->setStyleSheet("color: rgba(255,255,255,0.7); font-size: 14px; min-width: 80px;");
    m_classValueLabel = new QLabel("", this);
    m_classValueLabel->setStyleSheet("color: white; font-size: 14px;");
    classLayout->addWidget(m_classLabel);
    classLayout->addWidget(m_classValueLabel);
    classLayout->addStretch();
    infoLayout->addLayout(classLayout);

    mainLayout->addLayout(infoLayout);
    mainLayout->addStretch();

    // 底部按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(15);
    
    m_cancelButton = new QPushButton("取消", this);
    m_cancelButton->setStyleSheet(
        "QPushButton { background-color: rgba(255,255,255,0.2); color: white; border: none; border-radius: 6px; padding: 10px; font-size: 14px; }"
        "QPushButton:hover { background-color: rgba(255,255,255,0.3); }"
    );
    m_cancelButton->setFixedHeight(40);
    
    m_confirmButton = new QPushButton("确定", this);
    m_confirmButton->setStyleSheet(
        "QPushButton { background-color: #2E6BE6; color: white; border: none; border-radius: 6px; padding: 10px; font-size: 14px; }"
        "QPushButton:hover { background-color: #5688E6; }"
    );
    m_confirmButton->setFixedHeight(40);
    
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_confirmButton);
    
    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(m_editButton, &QPushButton::clicked, this, &ClassInfoDialog::onEditClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &ClassInfoDialog::onCancelClicked);
    connect(m_confirmButton, &QPushButton::clicked, this, &ClassInfoDialog::onConfirmClicked);
}

void ClassInfoDialog::InitData()
{
    // 先显示本地缓存的信息
    updateClassInfo();
    
    // 从服务器获取最新的班级信息
    fetchClassInfoFromServer();
}

void ClassInfoDialog::fetchClassInfoFromServer()
{
    if (!m_httpHandler) {
        return;
    }
    
    // 获取班级编号
    ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
    if (loginInfo.class_code.isEmpty()) {
        qDebug() << "班级编号为空，无法获取班级信息";
        return;
    }
    
    // 构建请求URL，使用GET方法获取班级信息
    // 假设接口为: GET /api/class/info?class_code=xxx
    QUrl url("http://47.100.126.194:5000/api/class/info");
    QUrlQuery query;
    query.addQueryItem("class_code", loginInfo.class_code);
    url.setQuery(query);
    
    // 添加认证token（如果有）
    if (!loginInfo.access_token.isEmpty()) {
        m_httpHandler->addHeader("Authorization", QString("Bearer %1").arg(loginInfo.access_token));
    }
    
    // 发送GET请求
    m_httpHandler->get(url.toString());
}

void ClassInfoDialog::handleClassInfoResponse(const QString& responseString)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
    if (!jsonDoc.isObject()) {
        qDebug() << "响应格式错误：不是有效的JSON对象";
        return;
    }
    
    QJsonObject obj = jsonDoc.object();
    
    // 服务器返回格式：{ "data": { "message": "...", "code": 200, ... } }
    if (!obj.contains("data") || !obj["data"].isObject()) {
        qDebug() << "响应格式错误：缺少data字段";
        return;
    }
    
    QJsonObject dataObj = obj["data"].toObject();
    
    // 检查响应码
    int code = -1;
    if (dataObj["code"].isDouble()) {
        code = dataObj["code"].toInt();
    } else if (dataObj["code"].isString()) {
        code = dataObj["code"].toString().toInt();
    }
    
    QString message = dataObj["message"].toString();
    qDebug() << "获取班级信息响应 - code:" << code << "message:" << message;
    
    if (code == 200 && message == "获取班级信息成功") {
        // 解析班级详细信息（从ta_classes表）
        QString classCode = dataObj["class_code"].toString();
        QString className = dataObj["class_name"].toString();
        QString schoolStage = dataObj["school_stage"].toString();
        QString grade = dataObj["grade"].toString();
        QString schoolid = dataObj["schoolid"].toString();
        QString remark = dataObj["remark"].toString(); // 备注信息
        
        // 解析学校信息（从ta_school表）
        QString schoolName = dataObj["school_name"].toString(); // 学校名称
        QString address = dataObj["address"].toString(); // 学校地址
        
        // 更新UI显示
        if (!className.isEmpty()) {
            m_classNameLabel->setText(className);
        }
        
        if (!classCode.isEmpty()) {
            m_classCodeLabel->setText(QString("班级编号: %1").arg(classCode));
        }
        
        // 更新详细信息
        m_schoolStageValueLabel->setText(schoolStage.isEmpty() ? "未设置" : schoolStage);
        m_gradeValueLabel->setText(grade.isEmpty() ? "未设置" : grade);
        
        // 学校名称优先显示school_name，如果没有则显示schoolid
        if (!schoolName.isEmpty()) {
            m_schoolNameValueLabel->setText(schoolName);
        } else if (!schoolid.isEmpty()) {
            m_schoolNameValueLabel->setText(schoolid);
        } else {
            m_schoolNameValueLabel->setText("未设置");
        }
        
        // 地址（从学校表获取）
        m_addressValueLabel->setText(address.isEmpty() ? "未设置" : address);
        
        // 班级名称
        m_classValueLabel->setText(className.isEmpty() ? "未设置" : className);
        
        // 更新全局信息（同步更新CommonInfo中的班级信息）
        ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
        if (!classCode.isEmpty()) loginInfo.class_code = classCode;
        if (!className.isEmpty()) loginInfo.class_name = className;
        if (!schoolStage.isEmpty()) loginInfo.school_stage = schoolStage;
        if (!grade.isEmpty()) loginInfo.grade = grade;
        if (!schoolid.isEmpty()) loginInfo.schoolid = schoolid;
        CommonInfo::InitClassLoginInfo(loginInfo);
        
        qDebug() << "班级信息更新成功 - 班级:" << className 
                 << "学校:" << schoolName 
                 << "地址:" << address;
    } else {
        // 处理错误情况
        qDebug() << "获取班级信息失败 - code:" << code << "message:" << message;
        // 错误时保持显示本地缓存的信息，不更新UI
    }
}

void ClassInfoDialog::updateClassInfo()
{
    ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
    
    // 更新班级名称
    if (!loginInfo.class_name.isEmpty()) {
        m_classNameLabel->setText(loginInfo.class_name);
    } else {
        m_classNameLabel->setText("未设置");
    }
    
    // 更新班级编号
    if (!loginInfo.class_code.isEmpty()) {
        m_classCodeLabel->setText(QString("班级编号: %1").arg(loginInfo.class_code));
    } else {
        m_classCodeLabel->setText("班级编号: 未设置");
    }
    
    // 更新详细信息
    m_schoolStageValueLabel->setText(loginInfo.school_stage.isEmpty() ? "未设置" : loginInfo.school_stage);
    m_gradeValueLabel->setText(loginInfo.grade.isEmpty() ? "未设置" : loginInfo.grade);
    m_schoolNameValueLabel->setText(loginInfo.schoolid.isEmpty() ? "未设置" : loginInfo.schoolid);
    
    // 地址和班级信息需要从其他地方获取，暂时显示占位符
    m_addressValueLabel->setText("未设置");
    m_classValueLabel->setText(loginInfo.class_name.isEmpty() ? "未设置" : loginInfo.class_name);
}

void ClassInfoDialog::onEditClicked()
{
    m_isEditMode = !m_isEditMode;
    if (m_isEditMode) {
        m_editButton->setText("保存");
        // 可以在这里添加编辑功能，将标签改为输入框
    } else {
        m_editButton->setText("班级端");
        // 保存编辑的内容
    }
}

void ClassInfoDialog::onCancelClicked()
{
    reject();
}

void ClassInfoDialog::onConfirmClicked()
{
    accept();
}

void ClassInfoDialog::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect rect(0, 0, width(), height());
    QPainterPath path;
    path.addRoundedRect(rect, m_radius, m_radius);

    p.fillPath(path, QBrush(m_backgroundColor));
    QPen pen(m_borderColor, m_borderWidth);
    p.strokePath(path, pen);
}

void ClassInfoDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
    }
}

void ClassInfoDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
    }
}

void ClassInfoDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
}

void ClassInfoDialog::leaveEvent(QEvent* event)
{
    QDialog::leaveEvent(event);
    m_closeButton->hide();
}

void ClassInfoDialog::enterEvent(QEvent* event)
{
    QDialog::enterEvent(event);
    if (m_visibleCloseButton)
        m_closeButton->show();
}

void ClassInfoDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    if (m_closeButton) {
        m_closeButton->move(this->width() - m_closeButton->width() - 4, 4);
    }
}

void ClassInfoDialog::setBackgroundColor(const QColor& color)
{
    m_backgroundColor = color;
    update();
}

void ClassInfoDialog::setBorderColor(const QColor& color)
{
    m_borderColor = color;
    update();
}

void ClassInfoDialog::setBorderWidth(int val)
{
    m_borderWidth = val;
    update();
}

void ClassInfoDialog::setRadius(int val)
{
    m_radius = val;
    update();
}

void ClassInfoDialog::visibleCloseButton(bool val)
{
    m_visibleCloseButton = val;
}

