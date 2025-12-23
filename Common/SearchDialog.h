#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#include <QFrame>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QMouseEvent>
#include <QPoint>
#include <QEvent>
#include <QResizeEvent>
#include "TAHttpHandler.h"
#include "CommonInfo.h"
#include "ImSDK/includes/TIMCloud.h"
#include "ImSDK/includes/TIMCloudDef.h"

// 自定义输入对话框类（类似SearchDialog样式，无标题栏）
class SearchInputDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SearchInputDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        // 去掉标题栏
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setModal(true);
        resize(400, 180);
        
        // 设置窗口背景色和圆角
        setStyleSheet(
            "QDialog { background-color: #565656; border-radius: 20px; } "
            "QLabel { background-color: #565656; color: #f5f5f5; } "
            "QLineEdit { background-color: #454545; color: #f5f5f5; border-radius: 15px; padding: 6px; } "
            "QPushButton { background-color: #454545; color: #f5f5f5; border-radius: 15px; } "
        );
        
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);
        
        // 标题标签
        m_titleLabel = new QLabel("输入", this);
        m_titleLabel->setStyleSheet("background-color: #565656; color: #f5f5f5; font-size: 18px; font-weight: bold;");
        m_titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(m_titleLabel);
        
        // 提示标签
        m_promptLabel = new QLabel("", this);
        m_promptLabel->setStyleSheet("background-color: #565656; color: #f5f5f5; font-size: 14px;");
        m_promptLabel->setWordWrap(true);
        mainLayout->addWidget(m_promptLabel);
        
        // 输入框
        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setStyleSheet("background-color: #454545; color: #f5f5f5; border-radius: 15px; padding: 6px;");
        mainLayout->addWidget(m_lineEdit);
        
        // 按钮布局
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        
        m_okButton = new QPushButton("确定", this);
        m_okButton->setFixedSize(80, 35);
        m_okButton->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 6px 12px; border-radius: 15px; font-weight: bold;");
        connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
        buttonLayout->addWidget(m_okButton);
        
        m_cancelButton = new QPushButton("取消", this);
        m_cancelButton->setFixedSize(80, 35);
        m_cancelButton->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 6px 12px; border-radius: 15px; font-weight: bold;");
        connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        buttonLayout->addWidget(m_cancelButton);
        
        buttonLayout->addStretch();
        mainLayout->addLayout(buttonLayout);
        
        // 关闭按钮（鼠标移入显示）
        m_closeButton = new QPushButton("×", this);
        m_closeButton->setFixedSize(30, 30);
        m_closeButton->setStyleSheet(
            "QPushButton { "
            "background-color: #707070; "
            "color: #f5f5f5; "
            "border: none; "
            "border-radius: 15px; "
            "font-size: 20px; "
            "font-weight: bold; "
            "} "
            "QPushButton:hover { "
            "background-color: #ff4444; "
            "}"
        );
        m_closeButton->move(width() - 35, 5);
        m_closeButton->hide();
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
        
        // 初始化拖动
        m_dragging = false;
        m_dragStartPos = QPoint();
        
        // 连接回车键
        connect(m_lineEdit, &QLineEdit::returnPressed, this, &QDialog::accept);
    }
    
    void setTitle(const QString& title) {
        if (m_titleLabel) {
            m_titleLabel->setText(title);
        }
    }
    
    void setPrompt(const QString& prompt) {
        if (m_promptLabel) {
            m_promptLabel->setText(prompt);
        }
    }
    
    void setText(const QString& text) {
        if (m_lineEdit) {
            m_lineEdit->setText(text);
        }
    }
    
    QString text() const {
        return m_lineEdit ? m_lineEdit->text() : QString();
    }
    
    static QString getText(QWidget* parent, const QString& title, const QString& prompt, 
                          const QString& defaultValue = "", bool* ok = nullptr)
    {
        SearchInputDialog dlg(parent);
        dlg.setTitle(title);
        dlg.setPrompt(prompt);
        dlg.setText(defaultValue);
        
        int result = dlg.exec();
        if (ok) {
            *ok = (result == QDialog::Accepted);
        }
        
        return result == QDialog::Accepted ? dlg.text() : QString();
    }
    
protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    
    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
            event->accept();
        }
    }
    
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            event->accept();
        }
    }
    
    void enterEvent(QEvent* event) override {
        if (m_closeButton) {
            m_closeButton->show();
        }
        QDialog::enterEvent(event);
    }
    
    void leaveEvent(QEvent* event) override {
        if (m_closeButton) {
            m_closeButton->hide();
        }
        QDialog::leaveEvent(event);
    }
    
    void resizeEvent(QResizeEvent* event) override {
        QDialog::resizeEvent(event);
        if (m_closeButton) {
            m_closeButton->move(width() - 35, 5);
        }
    }
    
private:
    QLabel* m_titleLabel = nullptr;
    QLabel* m_promptLabel = nullptr;
    QLineEdit* m_lineEdit = nullptr;
    QPushButton* m_okButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

// 自定义消息对话框类（类似SearchDialog样式）
class SearchMessageDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SearchMessageDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        // 去掉标题栏
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setModal(true);
        resize(400, 200);
        
        // 设置窗口背景色和圆角
        setStyleSheet(
            "QDialog { background-color: #565656; border-radius: 20px; } "
            "QLabel { background-color: #565656; color: #f5f5f5; } "
            "QPushButton { background-color: #454545; color: #f5f5f5; border-radius: 15px; } "
        );
        
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);
        
        // 标题标签
        m_titleLabel = new QLabel("提示", this);
        m_titleLabel->setStyleSheet("background-color: #565656; color: #f5f5f5; font-size: 18px; font-weight: bold;");
        m_titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(m_titleLabel);
        
        // 消息标签
        m_messageLabel = new QLabel("", this);
        m_messageLabel->setStyleSheet("background-color: #565656; color: #f5f5f5; font-size: 14px;");
        m_messageLabel->setWordWrap(true);
        m_messageLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(m_messageLabel);
        
        // 确定按钮
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        m_okButton = new QPushButton("确定", this);
        m_okButton->setFixedSize(100, 35);
        m_okButton->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 6px 12px; border-radius: 15px; font-weight: bold;");
        connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
        buttonLayout->addWidget(m_okButton);
        buttonLayout->addStretch();
        mainLayout->addLayout(buttonLayout);
        
        // 关闭按钮（鼠标移入显示）
        m_closeButton = new QPushButton("×", this);
        m_closeButton->setFixedSize(30, 30);
        m_closeButton->setStyleSheet(
            "QPushButton { "
            "background-color: #707070; "
            "color: #f5f5f5; "
            "border: none; "
            "border-radius: 15px; "
            "font-size: 20px; "
            "font-weight: bold; "
            "} "
            "QPushButton:hover { "
            "background-color: #ff4444; "
            "}"
        );
        m_closeButton->move(width() - 35, 5);
        m_closeButton->hide();
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);
        
        // 初始化拖动
        m_dragging = false;
        m_dragStartPos = QPoint();
    }
    
    void setTitle(const QString& title) {
        if (m_titleLabel) {
            m_titleLabel->setText(title);
        }
    }
    
    void setMessage(const QString& message) {
        if (m_messageLabel) {
            m_messageLabel->setText(message);
        }
    }
    
protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    
    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
            event->accept();
        }
    }
    
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            event->accept();
        }
    }
    
    void enterEvent(QEvent* event) override {
        if (m_closeButton) {
            m_closeButton->show();
        }
        QDialog::enterEvent(event);
    }
    
    void leaveEvent(QEvent* event) override {
        if (m_closeButton) {
            m_closeButton->hide();
        }
        QDialog::leaveEvent(event);
    }
    
    void resizeEvent(QResizeEvent* event) override {
        QDialog::resizeEvent(event);
        if (m_closeButton) {
            m_closeButton->move(width() - 35, 5);
        }
    }
    
private:
    QLabel* m_titleLabel = nullptr;
    QLabel* m_messageLabel = nullptr;
    QPushButton* m_okButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

class SearchDialog : public QDialog
{
    Q_OBJECT
public:
    SearchDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("查找班级/教师/群");
        resize(500, 600);
        
        // 去掉标题栏
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        
        // 设置窗口背景色和圆角，以及所有控件的背景色
        setStyleSheet(
            "QDialog { background-color: #565656; border-radius: 20px; } "
            "QWidget { background-color: #565656; } "
            "QLabel { background-color: #565656; } "
            "QLineEdit { background-color: #565656; } "
            "QPushButton { background-color: #454545; } "
            "QScrollArea { background-color: #565656; } "
            "QFrame { background-color: #565656; } "
            "QScrollBar:vertical { background-color: #565656; } "
            "QScrollBar:horizontal { background-color: #565656; } "
            "QScrollBar::handle:vertical { background-color: #707070; } "
            "QScrollBar::handle:horizontal { background-color: #707070; } "
        );

        // 初始化HTTP处理器
        m_httpHandler = new TAHttpHandler(this);
        if (m_httpHandler)
        {
            connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
                handleSearchResponse(responseString);
            });

            connect(m_httpHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
                QJsonDocument jsonDoc = QJsonDocument::fromJson(errResponseString.toUtf8());
                if (jsonDoc.isObject()) {
                    QJsonObject obj = jsonDoc.object();
                    if (obj["data"].isObject()) {
                        QJsonObject dataObj = obj["data"].toObject();
                        QString message = dataObj["message"].toString();
                        qDebug() << "搜索失败:" << message;
                    }
                }
            });
        }

        // 初始化拖动相关变量
        m_dragging = false;
        m_dragStartPos = QPoint();

        // 创建关闭按钮
        m_closeButton = new QPushButton("×", this);
        m_closeButton->setFixedSize(30, 30);
        m_closeButton->setStyleSheet(
            "QPushButton { "
            "background-color: #707070; "
            "color: #f5f5f5; "
            "border: none; "
            "border-radius: 15px; "
            "font-size: 20px; "
            "font-weight: bold; "
            "} "
            "QPushButton:hover { "
            "background-color: #ff4444; "
            "}"
        );
        m_closeButton->move(width() - 35, 5);
        m_closeButton->hide(); // 初始隐藏
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);

        // 整体布局
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 顶部数字标签
        m_lblNum = new QLabel("");
        m_lblNum->setAlignment(Qt::AlignCenter);
        m_lblNum->setFixedSize(30, 30);
        m_lblNum->setStyleSheet("background-color: #565656; color: #f5f5f5; font-weight: bold; font-size: 16px; border-radius: 15px;");
        mainLayout->addWidget(m_lblNum, 0, Qt::AlignCenter);

        // 搜索栏
        QHBoxLayout* searchLayout = new QHBoxLayout;
        m_editSearch = new QLineEdit;
        m_editSearch->setPlaceholderText("输入群ID或群名称");
        m_editSearch->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 6px; border-radius: 15px;");
        QPushButton* btnSearch = new QPushButton("搜索");
        btnSearch->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 6px 12px; border-radius: 15px;");
        searchLayout->addWidget(m_editSearch);
        searchLayout->addWidget(btnSearch);
        mainLayout->addLayout(searchLayout);

        // 连接搜索按钮点击事件
        connect(btnSearch, &QPushButton::clicked, this, &SearchDialog::onSearchClicked);
        // 连接回车键搜索
        connect(m_editSearch, &QLineEdit::returnPressed, this, &SearchDialog::onSearchClicked);

        // 分类按钮行
        QHBoxLayout* filterLayout = new QHBoxLayout;
        QPushButton* btnAll = new QPushButton("全部");
        QPushButton* btnClass = new QPushButton("班级");
        QPushButton* btnTeacher = new QPushButton("教师");
        QPushButton* btnGroup = new QPushButton("群");

        QString buttonStyle = "background-color: #454545; color: #f5f5f5; padding: 6px 12px; font-weight: bold; border-radius: 15px;";
        btnAll->setStyleSheet(buttonStyle);
        btnClass->setStyleSheet(buttonStyle);
        btnTeacher->setStyleSheet(buttonStyle);
        btnGroup->setStyleSheet(buttonStyle);

        filterLayout->addWidget(btnAll);
        filterLayout->addWidget(btnClass);
        filterLayout->addWidget(btnTeacher);
        filterLayout->addWidget(btnGroup);
        mainLayout->addLayout(filterLayout);

        // 列表区域
        m_scrollArea = new QScrollArea;
        m_scrollArea->setWidgetResizable(true);
        m_scrollArea->setStyleSheet("QScrollArea { background-color: #565656; }");
        m_listContainer = new QWidget;
        m_listContainer->setStyleSheet("QWidget { background-color: #565656; }");
        m_listLayout = new QVBoxLayout(m_listContainer);
        m_listLayout->addStretch();
        m_scrollArea->setWidget(m_listContainer);
        mainLayout->addWidget(m_scrollArea);
    }

signals:
    void groupJoined(const QString& groupId); // 加入群组成功信号

private slots:
    void onSearchClicked()
    {
        // 获取当前用户信息
        //UserInfo userInfo = CommonInfo::GetData();
        // 获取已加入的群组列表（用于判断是否已加入）
        loadJoinedGroups();

        QString searchKey = m_editSearch->text().trimmed();
        if (searchKey.isEmpty()) {
            return;
        }

        // 获取当前用户信息
        UserInfo userInfo = CommonInfo::GetData();
        m_currentUserId = userInfo.classId;
        if (userInfo.schoolId.isEmpty()) {
            qDebug() << "学校ID为空，无法搜索";
            return;
        }

        // 清空之前的搜索结果
        clearSearchResults();
        // 重置搜索结果计数
        m_groupCount = 0;
        m_teacherCount = 0;

        // 判断搜索类型：如果搜索键是纯数字或包含@TGS，可能是group_id；否则可能是group_name
        QString searchType = "group_name"; // 默认按名称搜索
        if (searchKey.contains("@TGS") || (searchKey.length() <= 10 && searchKey.toInt() > 0)) {
            searchType = "group_id";
        }

        // 构建群组搜索URL
        QUrl groupUrl("http://47.100.126.194:5000/groups/search");
        QUrlQuery groupQuery;
        // 不再需要schoolid参数，现在不同的学校也可以查询
        if (searchType == "group_id") {
            groupQuery.addQueryItem("group_id", searchKey);
        } else {
            groupQuery.addQueryItem("group_name", searchKey);
        }
        groupUrl.setQuery(groupQuery);

        // 发送群组搜索请求
        if (m_httpHandler) {
            m_httpHandler->get(groupUrl.toString());
            qDebug() << "群组搜索请求:" << groupUrl.toString();
        }

        // 构建教师搜索URL
        QUrl teacherUrl("http://47.100.126.194:5000/teachers/search");
        QUrlQuery teacherQuery;
        // 不再需要schoolid参数，现在不同的学校也可以查询
        
        // 判断教师搜索类型：如果搜索键是纯数字，可能是teacher_unique_id；否则按name搜索
        bool isNumeric = false;
        searchKey.toInt(&isNumeric);
        if (isNumeric && searchKey.length() <= 20) {
            // 可能是teacher_unique_id，尝试按teacher_unique_id搜索
            teacherQuery.addQueryItem("teacher_unique_id", searchKey);
        } else {
            // 按name模糊搜索
            teacherQuery.addQueryItem("name", searchKey);
        }
        teacherUrl.setQuery(teacherQuery);

        // 使用独立的QNetworkAccessManager发送教师搜索请求，避免与群组搜索响应冲突
        QNetworkAccessManager* teacherManager = new QNetworkAccessManager(this);
        QNetworkRequest teacherRequest(teacherUrl);
        QNetworkReply* teacherReply = teacherManager->get(teacherRequest);
        
        connect(teacherReply, &QNetworkReply::finished, this, [=]() {
            if (teacherReply->error() == QNetworkReply::NoError) {
                QByteArray data = teacherReply->readAll();
                handleTeacherSearchResponse(QString::fromUtf8(data));
            } else {
                qDebug() << "教师搜索失败:" << teacherReply->errorString();
            }
            teacherReply->deleteLater();
            teacherManager->deleteLater();
        });
        
        qDebug() << "教师搜索请求:" << teacherUrl.toString();
    }

    void handleSearchResponse(const QString& responseString)
    {
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "群组搜索JSON解析错误:" << parseError.errorString();
            return;
        }

        if (!jsonDoc.isObject()) {
            qDebug() << "群组搜索返回的不是JSON对象";
            return;
        }

        QJsonObject obj = jsonDoc.object();
        if (!obj["data"].isObject()) {
            qDebug() << "群组搜索返回数据中没有data字段";
            return;
        }

        QJsonObject dataObj = obj["data"].toObject();
        
        // 更新群组搜索结果数量
        m_groupCount = dataObj["count"].toInt();
        updateTotalCount();

        // 处理群组列表
        if (dataObj["groups"].isArray()) {
            QJsonArray groupsArray = dataObj["groups"].toArray();
            for (int i = 0; i < groupsArray.size(); i++) {
                QJsonObject groupObj = groupsArray[i].toObject();
                QString groupId = groupObj["group_id"].toString();
                QString groupName = groupObj["group_name"].toString();
                QString classid = groupObj["classid"].toString();
                QString schoolid = groupObj["schoolid"].toString();
                QString introduction = groupObj["introduction"].toString();
                QString notification = groupObj["notification"].toString();
                int memberNum = groupObj["member_num"].toInt();

                // 构建描述信息
                QString desc = introduction.isEmpty() ? notification : introduction;
                if (desc.isEmpty()) {
                    desc = "群组ID: " + groupId;
                }

                // 判断当前用户是否已经是该群的成员
                bool isMember = m_joinedGroupIds.contains(groupId);

                // 添加搜索结果项
                addListItem(m_listLayout, "", groupName, QString::number(memberNum), 
                           classid.isEmpty() ? "群组" : "班级群", desc, groupId, isMember);
            }
        }
    }

    void handleTeacherSearchResponse(const QString& responseString)
    {
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "教师搜索JSON解析错误:" << parseError.errorString();
            return;
        }

        if (!jsonDoc.isObject()) {
            qDebug() << "教师搜索返回的不是JSON对象";
            return;
        }

        QJsonObject obj = jsonDoc.object();
        if (!obj["data"].isObject()) {
            qDebug() << "教师搜索返回数据中没有data字段";
            return;
        }

        QJsonObject dataObj = obj["data"].toObject();
        
        // 更新教师搜索结果数量
        m_teacherCount = dataObj["count"].toInt();
        updateTotalCount();

        // 处理教师列表
        if (dataObj["teachers"].isArray()) {
            QJsonArray teachersArray = dataObj["teachers"].toArray();
            for (int i = 0; i < teachersArray.size(); i++) {
                QJsonObject teacherObj = teachersArray[i].toObject();
                QString teacherId = teacherObj["id"].toString();
                QString teacherName = teacherObj["name"].toString();
                QString teacherUniqueId = teacherObj["teacher_unique_id"].toString();
                QString schoolId = teacherObj["schoolId"].toString();

                // 构建描述信息
                QString desc = "教师唯一ID: " + teacherUniqueId;
                if (teacherId.toInt() > 0) {
                    desc += " | ID: " + teacherId;
                }

                // 添加教师搜索结果项（教师不需要加入按钮，所以groupId为空，isMember为false）
                addListItem(m_listLayout, "", teacherName, "1", "教师", desc, "", false);
            }
        }
    }

    void updateTotalCount()
    {
        // 更新总搜索结果数量
        int totalCount = m_groupCount + m_teacherCount;
        m_lblNum->setText(QString::number(totalCount));
    }

    void loadJoinedGroups()
    {
        // 通过/groups/by-teacher接口获取当前用户的群组列表
        UserInfo userInfo = CommonInfo::GetData();
        if (m_httpHandler && !userInfo.classId.isEmpty())
        {
            QUrl url("http://47.100.126.194:5000/groups/by-teacher");
            QUrlQuery query;
            query.addQueryItem("classId", userInfo.classId);
            url.setQuery(query);
            
            // 使用临时标志来区分这个请求是用于加载已加入群组列表的
            // 由于TAHttpHandler的success信号会触发handleSearchResponse，我们需要区分
            // 这里直接使用QNetworkAccessManager来避免冲突
            QNetworkAccessManager* tempManager = new QNetworkAccessManager(this);
            QNetworkRequest request(url);
            QNetworkReply* reply = tempManager->get(request);
            
            connect(reply, &QNetworkReply::finished, this, [=]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray data = reply->readAll();
                    QJsonParseError parseError;
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
                    if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                        QJsonObject obj = jsonDoc.object();
                        if (obj["data"].isObject()) {
                            QJsonObject dataObj = obj["data"].toObject();
                            
                            // 处理群主群组列表
                            if (dataObj["owner_groups"].isArray()) {
                                QJsonArray ownerGroups = dataObj["owner_groups"].toArray();
                                for (int i = 0; i < ownerGroups.size(); i++) {
                                    QJsonObject group = ownerGroups[i].toObject();
                                    QString groupId = group["group_id"].toString();
                                    if (!groupId.isEmpty()) {
                                        m_joinedGroupIds.insert(groupId);
                                    }
                                }
                            }
                            
                            // 处理成员群组列表
                            if (dataObj["member_groups"].isArray()) {
                                QJsonArray memberGroups = dataObj["member_groups"].toArray();
                                for (int i = 0; i < memberGroups.size(); i++) {
                                    QJsonObject group = memberGroups[i].toObject();
                                    QString groupId = group["group_id"].toString();
                                    if (!groupId.isEmpty()) {
                                        m_joinedGroupIds.insert(groupId);
                                    }
                                }
                            }
                            
                            qDebug() << "已加载已加入群组列表，共" << m_joinedGroupIds.size() << "个群组";
                        }
                    }
                } else {
                    qDebug() << "加载已加入群组列表失败:" << reply->errorString();
                }
                reply->deleteLater();
                tempManager->deleteLater();
            });
        }
    }

    void clearSearchResults()
    {
        // 清空列表容器中的所有子控件（保留最后的stretch）
        QLayoutItem* item;
        while ((item = m_listLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
        m_listLayout->addStretch(); // 重新添加stretch
    }

private slots:
    void onJoinGroupClicked(const QString& groupId, const QString& groupName)
    {
        // 获取当前用户信息
        UserInfo userInfo = CommonInfo::GetData();
        QString userId = userInfo.classId; // 使用strUserId作为用户ID
        QString userName = userInfo.strName;
        
        if (userId.isEmpty() || userName.isEmpty()) {
            QMessageBox::warning(this, "错误", "用户信息不完整，无法加入群组！");
            return;
        }
        
        // 弹出输入框让用户输入加入理由
        bool ok;
        QString reason = SearchInputDialog::getText(this, "申请加入群组", 
            QString("请输入加入群组 \"%1\" 的理由：").arg(groupName),
            "", &ok);
        
        if (!ok || reason.isEmpty()) {
            return; // 用户取消或未输入理由
        }
        
        // 创建一个结构体来保存回调需要的数据
        struct JoinGroupCallbackData {
            SearchDialog* dlg;
            QString groupId;
            QString groupName;
            QString userId;
            QString userName;
            QString reason;
        };
        
        JoinGroupCallbackData* callbackData = new JoinGroupCallbackData;
        callbackData->dlg = this;
        callbackData->groupId = groupId;
        callbackData->groupName = groupName;
        callbackData->userId = userId;
        callbackData->userName = userName;
        callbackData->reason = reason;
        
        // 先调用腾讯SDK的申请加入群组接口
        QByteArray groupIdBytes = groupId.toUtf8();
        QByteArray reasonBytes = reason.toUtf8();
        
        int ret = TIMGroupJoin(groupIdBytes.constData(), reasonBytes.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                JoinGroupCallbackData* data = (JoinGroupCallbackData*)user_data;
                SearchDialog* dlg = data->dlg;
                
                if (code == TIM_SUCC) {
                    qDebug() << "腾讯SDK申请加入群组成功:" << data->groupId;
                    
                    // 腾讯SDK成功，现在调用自己的服务器接口
                    dlg->sendJoinGroupRequestToServer(data->groupId, data->userId, data->userName, data->reason);
                    
                    // 显示成功消息
                    SearchMessageDialog* msgDlg = new SearchMessageDialog(dlg);
                    msgDlg->setTitle("申请成功");
                    msgDlg->setMessage(QString("已成功申请加入群组 \"%1\"！\n等待管理员审核。").arg(data->groupName));
                    msgDlg->exec();
                } else {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    QString errorMsg = QString("申请加入群组失败\n错误码: %1\n错误描述: %2").arg(code).arg(errorDesc);
                    qDebug() << errorMsg;
                    SearchMessageDialog* msgDlg = new SearchMessageDialog(dlg);
                    msgDlg->setTitle("申请失败");
                    msgDlg->setMessage(errorMsg);
                    msgDlg->exec();
                }
                
                // 释放回调数据
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            QString errorMsg = QString("调用TIMGroupJoin接口失败，错误码: %1").arg(ret);
            qDebug() << errorMsg;
            QMessageBox::critical(this, "错误", errorMsg);
            delete callbackData;
        }
    }
    
    void sendJoinGroupRequestToServer(const QString& groupId, const QString& userId, 
                                     const QString& userName, const QString& reason)
    {
        // 构造发送到服务器的JSON数据
        QJsonObject requestData;
        requestData["group_id"] = groupId;
        requestData["user_id"] = userId;
        requestData["user_name"] = userName;
        requestData["reason"] = reason;
        
        // 转换为JSON字符串
        QJsonDocument doc(requestData);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        // 发送POST请求到服务器
        QString url = "http://47.100.126.194:5000/groups/join";
        
        // 使用QNetworkAccessManager发送POST请求
        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QNetworkRequest networkRequest;
        networkRequest.setUrl(QUrl(url));
        networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        
        QNetworkReply* reply = manager->post(networkRequest, jsonData);
        
        connect(reply, &QNetworkReply::finished, this, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray response = reply->readAll();
                qDebug() << "服务器响应:" << QString::fromUtf8(response);
                
                // 解析响应
                QJsonParseError parseError;
                QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
                if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                    QJsonObject obj = jsonDoc.object();
                    if (obj["code"].toInt() == 200) {
                        qDebug() << "加入群组请求已发送到服务器";
                        
                        // 服务器响应成功，发出加入群组成功信号，通知父窗口刷新群组列表
                        emit this->groupJoined(groupId);
                    } else {
                        QString message = obj["message"].toString();
                        qDebug() << "服务器返回错误:" << message;
                    }
                }
            } else {
                qDebug() << "发送加入群组请求到服务器失败:" << reply->errorString();
            }
            
            reply->deleteLater();
            manager->deleteLater();
        });
    }

private:
    void addListItem(QVBoxLayout* parent, const QString& iconPath,
        const QString& name, const QString& memberCount,
        const QString& tag, const QString& desc, const QString& groupId = "", bool isMember = false)
    {
        QHBoxLayout* itemLayout = new QHBoxLayout;
        QLabel* avatar = new QLabel;
        avatar->setFixedSize(40, 40);
        avatar->setStyleSheet("background-color: #565656; border-radius: 20px;"); // 用灰色代替头像，可以换成 QPixmap 加载图片

        QVBoxLayout* infoLayout = new QVBoxLayout;
        QLabel* lblName = new QLabel(QString("%1  ⛔ %2人  %3").arg(name).arg(memberCount).arg(tag));
        lblName->setStyleSheet("background-color: #565656; color: #f5f5f5; font-weight: bold;");
        QLabel* lblDesc = new QLabel(desc);
        lblDesc->setStyleSheet("background-color: #565656; color: #d3d3d3; font-size: 12px;");
        infoLayout->addWidget(lblName);
        infoLayout->addWidget(lblDesc);

        itemLayout->addWidget(avatar);
        itemLayout->addLayout(infoLayout);
        itemLayout->addStretch();
        
        // 只有当groupId不为空时才显示"加入"按钮（用于群组搜索结果）
        if (!groupId.isEmpty()) {
            QPushButton* btnJoin = new QPushButton(isMember ? "已加入" : "加入");
            
            // 根据是否已加入设置按钮样式和状态
            if (isMember) {
                // 已加入：灰化按钮，禁用
                btnJoin->setEnabled(false);
                btnJoin->setStyleSheet("background-color: #454545; color: #d3d3d3; padding: 4px 8px; border-radius: 15px;");
            } else {
                // 未加入：可点击
                btnJoin->setEnabled(true);
                btnJoin->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 4px 8px; border-radius: 15px;");
                
                // 连接加入按钮点击事件
                connect(btnJoin, &QPushButton::clicked, this, [=]() {
                    onJoinGroupClicked(groupId, name);
                });
            }
            
            btnJoin->setProperty("group_id", groupId);
            btnJoin->setProperty("group_name", name);
            itemLayout->addWidget(btnJoin);
        }

        QFrame* frame = new QFrame;
        frame->setLayout(itemLayout);
        frame->setFrameShape(QFrame::HLine);
        frame->setStyleSheet("QFrame { background-color: #565656; }");
        parent->insertWidget(parent->count() - 1, frame); // 在stretch之前插入
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            event->accept();
        }
    }

    void enterEvent(QEvent* event) override
    {
        if (m_closeButton) {
            m_closeButton->show();
        }
        QDialog::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        if (m_closeButton) {
            m_closeButton->hide();
        }
        QDialog::leaveEvent(event);
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QDialog::resizeEvent(event);
        // 更新关闭按钮位置
        if (m_closeButton) {
            m_closeButton->move(width() - 35, 5);
        }
    }

private:
    TAHttpHandler* m_httpHandler = nullptr;
    QLineEdit* m_editSearch = nullptr;
    QLabel* m_lblNum = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_listContainer = nullptr;
    QVBoxLayout* m_listLayout = nullptr;
    QString m_currentUserId; // 当前用户ID
    QSet<QString> m_joinedGroupIds; // 已加入的群组ID集合
    int m_groupCount = 0; // 群组搜索结果数量
    int m_teacherCount = 0; // 教师搜索结果数量
    bool m_dragging = false; // 是否正在拖动
    QPoint m_dragStartPos; // 拖动起始位置
    QPushButton* m_closeButton = nullptr; // 关闭按钮
};
