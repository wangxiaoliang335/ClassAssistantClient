#pragma execution_character_set("utf-8")
#include "TACSubjectSelectDialog.h"
#include "TACSubjectEditDialog.h"
#include "TACWarningDialog.h"
#include "TAHttpHandler.h"
#include "CommonInfo.h"
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

const QStringList TACSubjectSelectDialog::DEFAULT_SUBJECTS = {
    QString::fromUtf8(u8"语文"),
    QString::fromUtf8(u8"数学"),
    QString::fromUtf8(u8"英语"),
    QString::fromUtf8(u8"物理"),
    QString::fromUtf8(u8"化学"),
    QString::fromUtf8(u8"地理"),
    QString::fromUtf8(u8"生物"),
    QString::fromUtf8(u8"历史"),
    QString::fromUtf8(u8"道法"),
    QString::fromUtf8(u8"音乐"),
    QString::fromUtf8(u8"美术"),
    QString::fromUtf8(u8"体育")
};

TACSubjectSelectDialog::TACSubjectSelectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(120, 400); // 增加高度以容纳更多科目
    
    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 40, 10, 10);
    m_mainLayout->setSpacing(0);
    
    // 创建滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(
        "QScrollArea {"
        "background-color: transparent;"
        "border: none;"
        "}"
        "QScrollBar:vertical {"
        "background-color: rgba(60, 60, 60, 200);"
        "width: 8px;"
        "border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "background-color: rgba(120, 120, 120, 200);"
        "min-height: 20px;"
        "border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "background-color: rgba(150, 150, 150, 200);"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "height: 0px;"
        "}"
    );
    
    // 创建滚动内容容器
    m_scrollContent = new QWidget();
    m_scrollLayout = new QVBoxLayout(m_scrollContent);
    m_scrollLayout->setContentsMargins(0, 0, 0, 0);
    m_scrollLayout->setSpacing(8);
    
    m_scrollArea->setWidget(m_scrollContent);
    m_mainLayout->addWidget(m_scrollArea, 1); // 使用 stretch 让滚动区域占据剩余空间
    
    // 创建关闭按钮
    m_closeButton = new QPushButton(QString::fromUtf8(u8"×"), this);
    m_closeButton->setFixedSize(23, 23);
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "color: white;"
        "background-color: rgba(255,255,255,20);"
        "border: 1px solid rgba(50,50,50,200);"
        "border-radius: 11px;"
        "font-size: 16px;"
        "font-weight: 700;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(255,255,255,35);"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(255,255,255,18);"
        "}"
    );
    m_closeButton->hide();
    m_closeButton->raise();
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // 从服务器获取科目列表（如果失败则使用默认科目）
    fetchSubjectsFromServer();
    
    updateCloseButtonPos();
}

TACSubjectSelectDialog::~TACSubjectSelectDialog()
{
}

void TACSubjectSelectDialog::setSubjects(const QStringList& subjects)
{
    m_subjects = subjects;
    createSubjectButtons();
}

void TACSubjectSelectDialog::createSubjectButtons()
{
    // 清除滚动布局中的现有按钮（除了添加按钮）
    QLayoutItem* item;
    while ((item = m_scrollLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            QPushButton* btn = qobject_cast<QPushButton*>(item->widget());
            if (btn && btn != m_addButton) {
                btn->deleteLater();
            }
        }
        delete item;
    }
    
    // 创建科目按钮
    for (const QString& subject : m_subjects) {
        QPushButton* btn = new QPushButton(subject, m_scrollContent);
        btn->setFixedHeight(40);
        btn->setCheckable(true);
        btn->installEventFilter(this); // 安装事件过滤器以支持双击
        btn->setProperty("subject", subject); // 保存原始科目名称
        btn->setStyleSheet(
            "QPushButton {"
            "background-color: #3C3C3C;"
            "color: white;"
            "font-size: 14px;"
            "border: none;"
            "border-radius: 8px;"
            "text-align: center;"
            "}"
            "QPushButton:hover {"
            "background-color: #4A4A4A;"
            "}"
            "QPushButton:checked {"
            "background-color: rgba(0,120,212,255);"
            "}"
        );
        connect(btn, &QPushButton::clicked, this, &TACSubjectSelectDialog::onSubjectButtonClicked);
        m_scrollLayout->addWidget(btn);
    }
    
    // 创建添加按钮（"+"）
    if (!m_addButton) {
        m_addButton = new QPushButton(QString::fromUtf8(u8"+"), m_scrollContent);
        m_addButton->setFixedHeight(40);
        m_addButton->setStyleSheet(
            "QPushButton {"
            "background-color: #3C3C3C;"
            "color: white;"
            "font-size: 20px;"
            "font-weight: bold;"
            "border: none;"
            "border-radius: 8px;"
            "text-align: center;"
            "}"
            "QPushButton:hover {"
            "background-color: #4A4A4A;"
            "}"
        );
        connect(m_addButton, &QPushButton::clicked, this, &TACSubjectSelectDialog::onAddButtonClicked);
    }
    m_scrollLayout->addWidget(m_addButton);
}

void TACSubjectSelectDialog::onSubjectButtonClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    // 初始化定时器（延迟处理单击）
    if (!m_clickTimer) {
        m_clickTimer = new QTimer(this);
        m_clickTimer->setSingleShot(true);
        connect(m_clickTimer, &QTimer::timeout, this, [this]() {
            // 单击处理
            if (m_lastClickedBtn) {
                // 取消其他按钮的选中状态
                for (int i = 0; i < m_scrollLayout->count(); ++i) {
                    QLayoutItem* item = m_scrollLayout->itemAt(i);
                    if (item && item->widget()) {
                        QPushButton* otherBtn = qobject_cast<QPushButton*>(item->widget());
                        if (otherBtn && otherBtn != m_lastClickedBtn && otherBtn != m_addButton) {
                            otherBtn->setChecked(false);
                        }
                    }
                }
                
                m_selectedSubject = m_lastClickedBtn->text();
                emit subjectSelected(m_selectedSubject);
                accept(); // 关闭对话框
                m_lastClickedBtn = nullptr;
            }
        });
    }
    
    // 如果两次点击间隔很短且是同一个按钮，视为双击
    if (m_lastClickedBtn == btn && m_clickTimer->isActive()) {
        m_clickTimer->stop();
        onSubjectButtonDoubleClicked();
        m_lastClickedBtn = nullptr;
        return;
    }
    
    m_lastClickedBtn = btn;
    m_clickTimer->start(300); // 300ms内的第二次点击视为双击
}

void TACSubjectSelectDialog::onSubjectButtonDoubleClicked()
{
    if (!m_lastClickedBtn) return;
    
    QString oldSubject = m_lastClickedBtn->text();
    
    // 显示自定义输入对话框编辑科目名称
    TACSubjectEditDialog dialog(this);
    dialog.setTitleText(QString::fromWCharArray(L"编辑科目"));
    dialog.setLabelText(QString::fromWCharArray(L"请输入科目名称:"));
    dialog.setInitialText(oldSubject);
    
    // 定位对话框到按钮附近
    QPoint globalPos = m_lastClickedBtn->mapToGlobal(QPoint(0, m_lastClickedBtn->height()));
    QRect screenGeometry = QApplication::desktop()->availableGeometry();
    if (globalPos.x() + dialog.width() > screenGeometry.right()) {
        globalPos.setX(screenGeometry.right() - dialog.width() - 10);
    }
    if (globalPos.y() + dialog.height() > screenGeometry.bottom()) {
        globalPos.setY(screenGeometry.bottom() - dialog.height() - 10);
    }
    dialog.move(globalPos);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString newSubject = dialog.subjectName().trimmed();
        if (newSubject.isEmpty()) {
            return;
        }
        
        // 检查新科目名称是否已存在（排除当前编辑的科目）
        if (newSubject != oldSubject && m_subjects.contains(newSubject)) {
            TACWarningDialog::warning(this,
                                     QString::fromWCharArray(L"提示"),
                                     QString::fromWCharArray(L"科目\"%1\"已存在，请使用其他名称。").arg(newSubject));
            return;
        }
        
        if (newSubject != oldSubject) {
            m_lastClickedBtn->setText(newSubject);
            m_lastClickedBtn->setProperty("subject", newSubject);
            emit subjectEdited(oldSubject, newSubject);
            
            // 更新科目列表
            int index = m_subjects.indexOf(oldSubject);
            if (index >= 0) {
                m_subjects[index] = newSubject;
            }
            
            // 保存到服务器
            saveSubjectsToServer();
        }
    }
}

void TACSubjectSelectDialog::onAddButtonClicked()
{
    // 显示自定义输入对话框添加新科目
    TACSubjectEditDialog dialog(this);
    dialog.setTitleText(QString::fromWCharArray(L"添加科目"));
    dialog.setLabelText(QString::fromWCharArray(L"请输入新科目名称:"));
    dialog.setInitialText(QString());
    
    // 定位对话框到添加按钮附近
    QPoint globalPos = m_addButton->mapToGlobal(QPoint(0, m_addButton->height()));
    QRect screenGeometry = QApplication::desktop()->availableGeometry();
    if (globalPos.x() + dialog.width() > screenGeometry.right()) {
        globalPos.setX(screenGeometry.right() - dialog.width() - 10);
    }
    if (globalPos.y() + dialog.height() > screenGeometry.bottom()) {
        globalPos.setY(screenGeometry.bottom() - dialog.height() - 10);
    }
    dialog.move(globalPos);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString newSubject = dialog.text().trimmed();
        if (newSubject.isEmpty()) {
            return;
        }
        
        // 检查科目是否已存在
        if (m_subjects.contains(newSubject)) {
            TACWarningDialog::warning(this,
                                     QString::fromWCharArray(L"提示"),
                                     QString::fromWCharArray(L"科目\"%1\"已存在，请使用其他名称。").arg(newSubject));
            return;
        }
        
        // 添加新科目到列表
        m_subjects.append(newSubject);
        
        // 创建新科目的按钮
        QPushButton* btn = new QPushButton(newSubject, m_scrollContent);
        btn->setFixedHeight(40);
        btn->setCheckable(true);
        btn->installEventFilter(this);
        btn->setProperty("subject", newSubject);
        btn->setStyleSheet(
            "QPushButton {"
            "background-color: #3C3C3C;"
            "color: white;"
            "font-size: 14px;"
            "border: none;"
            "border-radius: 8px;"
            "text-align: center;"
            "}"
            "QPushButton:hover {"
            "background-color: #4A4A4A;"
            "}"
            "QPushButton:checked {"
            "background-color: rgba(0,120,212,255);"
            "}"
        );
        connect(btn, &QPushButton::clicked, this, &TACSubjectSelectDialog::onSubjectButtonClicked);
        
        // 在添加按钮之前插入新按钮
        int addButtonIndex = m_scrollLayout->indexOf(m_addButton);
        if (addButtonIndex >= 0) {
            m_scrollLayout->insertWidget(addButtonIndex, btn);
        } else {
            m_scrollLayout->addWidget(btn);
        }
        
        // 保存到服务器
        saveSubjectsToServer();
    }
}

void TACSubjectSelectDialog::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // 绘制圆角背景
    QPainterPath path;
    path.addRoundedRect(rect(), 10, 10);
    
    // 深色半透明背景
    p.fillPath(path, QColor(40, 40, 40, 240));
    
    // 边框
    QPen pen(QColor(60, 60, 60, 255));
    pen.setWidth(1);
    p.setPen(pen);
    p.drawPath(path);
}

void TACSubjectSelectDialog::enterEvent(QEvent* event)
{
    if (m_closeButton) {
        m_closeButton->show();
    }
    QDialog::enterEvent(event);
}

void TACSubjectSelectDialog::leaveEvent(QEvent* event)
{
    QPoint globalPos = QCursor::pos();
    QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
    if (!widgetRect.contains(globalPos) && m_closeButton) {
        QRect btnRect = QRect(m_closeButton->mapToGlobal(QPoint(0, 0)), m_closeButton->size());
        if (!btnRect.contains(globalPos)) {
            m_closeButton->hide();
        }
    }
    QDialog::leaveEvent(event);
}

void TACSubjectSelectDialog::resizeEvent(QResizeEvent* event)
{
    updateCloseButtonPos();
    QDialog::resizeEvent(event);
}

void TACSubjectSelectDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        m_dragging = true;
        event->accept();
    }
}

void TACSubjectSelectDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
    }
}

void TACSubjectSelectDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}

void TACSubjectSelectDialog::updateCloseButtonPos()
{
    if (m_closeButton) {
        const int marginRight = 8;
        const int marginTop = 8;
        m_closeButton->move(width() - m_closeButton->width() - marginRight, marginTop);
    }
}

void TACSubjectSelectDialog::fetchSubjectsFromServer()
{
    // 获取当前登录的班级编号
    ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
    m_classCode = loginInfo.class_code;
    
    if (m_classCode.isEmpty()) {
        qWarning() << "班级编号为空，使用默认科目列表";
        setSubjects(DEFAULT_SUBJECTS);
        return;
    }
    
    if (!m_httpHandler) {
        m_httpHandler = new TAHttpHandler(this);
    }
    
    // 构建URL：GET /classes/subjects?class_code=xxx
    QString url = QStringLiteral("http://47.100.126.194:5000/classes/subjects?class_code=%1").arg(m_classCode);
    
    connect(m_httpHandler, &TAHttpHandler::success, this, [this](const QString& responseString) {
        m_httpHandler->disconnect(this);
        handleFetchSubjectsResponse(responseString);
    });
    
    connect(m_httpHandler, &TAHttpHandler::failed, this, [this](const QString& error) {
        m_httpHandler->disconnect(this);
        qWarning() << "获取科目列表失败:" << error;
        // 使用默认科目列表
        setSubjects(DEFAULT_SUBJECTS);
    });
    
    m_httpHandler->get(url);
    qDebug() << "正在获取科目列表:" << url;
}

void TACSubjectSelectDialog::handleFetchSubjectsResponse(const QString& response)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "科目列表响应解析失败:" << parseError.errorString();
        setSubjects(DEFAULT_SUBJECTS);
        return;
    }
    
    QJsonObject obj = doc.object();
    
    // 检查响应格式：{ "data": { "message": "...", "code": 200, ... } }
    if (!obj.contains("data") || !obj["data"].isObject()) {
        qWarning() << "科目列表响应格式错误：缺少data字段";
        setSubjects(DEFAULT_SUBJECTS);
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
    qDebug() << "获取科目列表响应 - code:" << code << "message:" << message;
    
    QString successMsg = QString::fromWCharArray(L"获取班级科目列表成功");
    if (code == 200 && message == successMsg) {
        // 解析科目列表
        if (dataObj.contains("subjects") && dataObj["subjects"].isArray()) {
            QJsonArray subjectsArray = dataObj["subjects"].toArray();
            QStringList subjects;
            
            for (const QJsonValue& value : subjectsArray) {
                QString subject = value.toString().trimmed();
                if (!subject.isEmpty()) {
                    subjects.append(subject);
                }
            }
            
            if (!subjects.isEmpty()) {
                setSubjects(subjects);
                qDebug() << "成功加载科目列表，共" << subjects.size() << "个科目";
                return;
            }
        }
    }
    
    // 如果解析失败或科目列表为空，使用默认科目
    qWarning() << "科目列表为空或解析失败，使用默认科目列表";
    setSubjects(DEFAULT_SUBJECTS);
}

void TACSubjectSelectDialog::saveSubjectsToServer()
{
    if (m_classCode.isEmpty()) {
        qWarning() << "班级编号为空，无法保存科目列表";
        return;
    }
    
    if (!m_httpHandler) {
        m_httpHandler = new TAHttpHandler(this);
    }
    
    // 构建请求体：{ "class_code": "xxx", "subjects": [...] }
    QJsonObject payload;
    payload[QStringLiteral("class_code")] = m_classCode;
    
    QJsonArray subjectsArray;
    for (const QString& subject : m_subjects) {
        subjectsArray.append(subject);
    }
    payload[QStringLiteral("subjects")] = subjectsArray;
    
    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    const QString url = QStringLiteral("http://47.100.126.194:5000/classes/subjects");
    
    connect(m_httpHandler, &TAHttpHandler::success, this, [this](const QString& responseString) {
        m_httpHandler->disconnect(this);
        QJsonParseError parseError;
        QJsonDocument respDoc = QJsonDocument::fromJson(responseString.toUtf8(), &parseError);
        
        if (parseError.error == QJsonParseError::NoError && respDoc.isObject()) {
            QJsonObject obj = respDoc.object();
            if (obj.contains("data") && obj["data"].isObject()) {
                QJsonObject dataObj = obj["data"].toObject();
                int code = -1;
                if (dataObj["code"].isDouble()) {
                    code = dataObj["code"].toInt();
                } else if (dataObj["code"].isString()) {
                    code = dataObj["code"].toString().toInt();
                }
                QString message = dataObj["message"].toString();
                
                if (code == 200) {
                    qDebug() << "科目列表保存成功:" << message;
                } else {
                    qWarning() << "科目列表保存失败:" << code << message;
                }
            } else {
                qWarning() << "科目列表保存响应格式错误";
            }
        } else {
            qWarning() << "科目列表保存响应解析失败:" << parseError.errorString();
        }
    });
    
    connect(m_httpHandler, &TAHttpHandler::failed, this, [](const QString& error) {
        qWarning() << "科目列表保存请求失败:" << error;
    });
    
    m_httpHandler->post(url, jsonData);
    qDebug() << "正在保存科目列表:" << url << "data:" << QString::fromUtf8(jsonData);
}

