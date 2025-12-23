#include <QApplication>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSpacerItem>
#include <QIcon>
#include <QInputDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QFileDialog>
#include <qpainterpath.h>
#include <qtoolbutton.h>
#include <QJsonObject>    // JSON 对象
#include <QJsonArray>     // JSON 数组
#include <QJsonDocument>  // JSON 文档（序列化/反序列化）
#include <QJsonValue>     // JSON 值类型
#include <qdebug.h>
#include <QPointer>
#include "CommonInfo.h"
#include "TABaseDialog.h"
#include "AvatarLabel.h"
#include "NameLabel.h"
#include "TAHttpHandler.h"
#include "CommonInfo.h"

class DraggableInputDialog : public QInputDialog
{
public:
    explicit DraggableInputDialog(QWidget* parent = nullptr)
        : QInputDialog(parent)
        , m_dragging(false)
    {
        setMouseTracking(true);
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        }
        QInputDialog::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
        }
        QInputDialog::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
        }
        QInputDialog::mouseReleaseEvent(event);
    }

private:
    bool m_dragging;
    QPoint m_dragStartPos;
};

class UserInfoDialog : public QDialog
{
    Q_OBJECT
public:
    UserInfoDialog(QWidget* parent = nullptr) : QDialog(parent),
        m_backgroundColor(QColor(50, 50, 50)),
        m_dragging(false),
        m_borderColor(Qt::white),
        m_borderWidth(2), m_radius(6),
        m_visibleCloseButton(true)
    {
        this->setObjectName("UserInfoDialog");
        m_httpHandler = new TAHttpHandler(this);
    }

    void InitData(UserInfo userInfo)
    {
        m_userInfo = userInfo;
    }

    void uploadAvatar(QString filePath)
    {
        // ===== 1. 读取头像图片 =====
        QFile file(filePath);  // 本地头像路径
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open image file.";
            return;
        }
        QByteArray imageData = file.readAll(); // 二进制数据
        file.close();

        // ===== 2. 图片转 Base64 =====
        QString imageBase64 = QString::fromLatin1(imageData.toBase64());
        m_userInfo.avatar = imageBase64;
        m_userInfo.strHeadImagePath = filePath;
        postUserInfoUpdate();
    }

signals:
    void userNameUpdated(const QString& newName);
    void userAvatarUpdated(const QString& newAvatarPath);
    void logoutRequested();

public:
    void postUserInfoUpdate()
    {
        if (!m_httpHandler) {
            return;
        }

        QMap<QString, QString> params = buildUserInfoParams();
        m_httpHandler->post(QString("http://47.100.126.194:5000/updateUserInfo"), params);
    }

    void postUserNameUpdate()
    {
        if (!m_httpHandler) {
            return;
        }
        QMap<QString, QString> params;
        params["phone"] = m_userInfo.strPhone;
        params["name"] = m_userInfo.strName;
        params["id_number"] = m_userInfo.strIdNumber;
        m_httpHandler->post(QString("http://47.100.126.194:5000/updateUserName"), params);
    }

    void postAdministratorStatus(const QString& status)
    {
        if (!m_httpHandler) {
            return;
        }
        QMap<QString, QString> params;
        params["phone"] = m_userInfo.strPhone;
        params["id_number"] = m_userInfo.strIdNumber;
        params["is_administrator"] = status;
        m_httpHandler->post(QString("http://47.100.126.194:5000/updateUserAdministrator"), params);
    }

    QString execStyledDialog(const QString& title, const QString& prompt,
        const QString& defaultValue, const QStringList& options,
        bool* ok, bool asChoice)
    {
        DraggableInputDialog dialog(this);
        dialog.setWindowTitle(title);
        dialog.setWindowFlag(Qt::FramelessWindowHint, true);
        dialog.setLabelText(prompt);
        if (asChoice) {
            dialog.setInputMode(QInputDialog::TextInput);
            dialog.setOption(QInputDialog::UseListViewForComboBoxItems, true);
            dialog.setComboBoxItems(options);
            dialog.setTextValue(defaultValue);
        }
        else {
            dialog.setInputMode(QInputDialog::TextInput);
            dialog.setTextValue(defaultValue);
        }
        dialog.setStyleSheet(
            "QInputDialog { background-color: #383c42; color: #ffffff; }"
            "QLabel { color: #ffffff; }"
            "QLineEdit, QComboBox { background-color: #262a2f; color: #ffffff; border: 1px solid #555; border-radius: 4px; }"
            "QPushButton { background-color: #4a4f57; color: #ffffff; border-radius: 4px; padding: 6px 16px; }"
            "QPushButton:hover { background-color: #5a5f67; }"
        );
        QString result = defaultValue;
        if (dialog.exec() == QDialog::Accepted) {
            if (ok) {
                *ok = true;
            }
            result = dialog.textValue();
        }
        else if (ok) {
            *ok = false;
        }
        return result;
    }

    void InitUI()
    {
        // 任教信息区域：先整体隐藏（后续要启用再改为 true）
        const bool kEnableTeachingsUI = false;

        // 隐藏标题栏（图片里没有标题栏）
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        // 窗口大小：任教信息隐藏时用小窗口；启用时再扩展高度
        if (kEnableTeachingsUI) {
            setFixedSize(420, 620);
        } else {
            setFixedSize(330, 360);
        }

        setWindowTitle("我的");
        if (kEnableTeachingsUI) {
            resize(420, 620);
        } else {
            resize(330, 360);
        }
        //setStyleSheet("background-color: #2a2a2a; color: white;");

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
                        //errLabel->setText(strTmp);
                        //user_id = oTmp["user_id"].toInt();
                        if (strTmp == "登录成功")
                        {
                            accept(); // 验证通过，关闭对话框并返回 Accepted
                        }
                        else if (strTmp == "获取用户信息成功")
                        {
                            /*if (oTmp["userinfo"].isArray())
                            {
                                QJsonArray oUserInfo = oTmp["userinfo"].toArray();
                                if (oUserInfo.size() > 0)
                                {
                                    m_userInfo.strPhone = oUserInfo.at(0)["phone"].toString();
                                    m_userInfo.strName = oUserInfo.at(0)["name"].toString();
                                    m_userInfo.strSex = oUserInfo.at(0)["sex"].toString();
                                    m_userInfo.strAddress = oUserInfo.at(0)["address"].toString();
                                    m_userInfo.strSchoolName = oUserInfo.at(0)["school_name"].toString();
                                    m_userInfo.strGradeLevel = oUserInfo.at(0)["grade_level"].toString();
                                    // 任教信息：优先读 teachings 数组（新模型）
                                    if (oUserInfo.at(0).isObject() && oUserInfo.at(0).toObject().contains("teachings")
                                        && oUserInfo.at(0).toObject().value("teachings").isArray()) {
                                        QJsonArray teachingsArr = oUserInfo.at(0).toObject().value("teachings").toArray();
                                        m_userInfo.teachings.clear();
                                        for (const QJsonValue& v : teachingsArr) {
                                            if (!v.isObject()) continue;
                                            QJsonObject t = v.toObject();
                                            UserTeachingInfo info;
                                            info.grade_level = t.value("grade_level").toString();
                                            info.grade = t.value("grade").toString();
                                            info.subject = t.value("subject").toString();
                                            info.class_taught = t.value("class_taught").toString();
                                            m_userInfo.teachings.append(info);
                                        }
                                    } else {
                                        m_userInfo.teachings.clear();
                                    }
                                    m_userInfo.strIsAdministrator = oUserInfo.at(0)["is_administrator"].toString();
                                    m_userInfo.avatar = oUserInfo.at(0)["avatar"].toString();
                                    m_userInfo.strIdNumber = oUserInfo.at(0)["id_number"].toString();

                                    if (userMenuDlg)
                                    {
                                        userMenuDlg->InitData(m_userInfo);
                                        userMenuDlg->InitUI();
                                    }
                                }
                            }*/
                        }
                    }
                }
                else
                {
                    //errLabel->setText("网络错误");
                }
                });

            connect(m_httpHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
                //if (errLabel)
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
                            //errLabel->setText(strTmp);
                        }
                    }
                    /*else
                    {
                        errLabel->setText("网络错误");
                    }*/
                }
                });
        }

        // 关闭按钮（右上角）
        closeButton = new QPushButton(this);
        closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
        closeButton->setIconSize(QSize(22, 22));
        closeButton->setFixedSize(QSize(22, 22));
        closeButton->setStyleSheet("background: transparent;");
        closeButton->move(width() - 24, 4);
        closeButton->hide();
        connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

        // ===================== 顶部区域 =====================
        QHBoxLayout* closeLayout = new QHBoxLayout;
        QHBoxLayout* topLayout = new QHBoxLayout;

        // 头像标签
        avatarLabel = new AvatarLabel;
        avatarLabel->setFixedSize(64, 64);
        avatarLabel->setStyleSheet("background-color: gray; border-radius: 32px;");
        avatarLabel->setAvatar(QPixmap(m_userInfo.strHeadImagePath));
        avatarLabel->setEditIcon(QPixmap(".\\res\\img\\com_ic_edit_2@2x.png"));

        QObject::connect(avatarLabel, &AvatarLabel::editIconClicked, [&]() {
            QString file = QFileDialog::getOpenFileName(
                this, "选择新头像", "", "Images (*.png *.jpg *.jpeg *.bmp)");
            if (!file.isEmpty()) {
                avatarLabel->setAvatar(QPixmap(file));
                uploadAvatar(file);
                emit userAvatarUpdated(file);
            }
        });

        QObject::connect(avatarLabel, &AvatarLabel::avatarClicked, []() {
            qDebug() << "头像被点击";
        });

        //// 编辑头像按钮
        //QToolButton* editAvatarBtn = new QToolButton;
        //editAvatarBtn->setIcon(QIcon(".\\res\\img\\com_ic_edit_2@2x.png"));
        //editAvatarBtn->setToolTip("编辑头像");

        // 姓名标签
        nameLabel = new NameLabel;

        //// 编辑姓名按钮
        //QToolButton* editNameBtn = new QToolButton;
        //editNameBtn->setIcon(QIcon(".\\res\\img\\com_ic_edit_2@2x.png"));
        //editNameBtn->setToolTip("编辑姓名");

        // 顶部横向布局：头像+编辑按钮，姓名+编辑按钮
        //QHBoxLayout* avatarLayout = new QHBoxLayout;
        //avatarLayout->addWidget(avatarLabel);
        //avatarLayout->addWidget(editAvatarBtn);
        //avatarLayout->addStretch();

        nameLabel->setText(m_userInfo.strName);
        nameLabel->setStyleSheet("color: white; font-size: 15px;");
        nameLabel->setEditIcon(QPixmap(".\\res\\img\\com_ic_edit_2.png")); // 编辑图标

        auto editNameHandler = [this]() {
            bool ok = false;
            QString currentName = nameLabel->text();
            QString newName = execStyledDialog(QString::fromUtf8("修改姓名"),
                QString::fromUtf8("请输入新的姓名："), currentName, QStringList(), &ok, false);
            if (!ok) {
                return;
            }
            newName = newName.trimmed();
            if (newName.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("姓名不能为空！"));
                return;
            }
            if (newName == m_userInfo.strName) {
                return;
            }

            nameLabel->setText(newName);
            m_userInfo.strName = newName;
            postUserNameUpdate();
            emit userNameUpdated(newName);
        };

        QObject::connect(nameLabel, &NameLabel::editIconClicked, this, editNameHandler);
        QObject::connect(nameLabel, &NameLabel::labelClicked, this, editNameHandler);

        //QHBoxLayout* nameLayout = new QHBoxLayout;
        //nameLayout->addWidget(nameLabel);
        //nameLayout->addWidget(editNameBtn);
        //nameLayout->addStretch();

        //QLabel* titleLabel = new QLabel("我的");
        //titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");

        /*QLabel* lblNumber = new QLabel("2");
        lblNumber->setAlignment(Qt::AlignCenter);
        lblNumber->setFixedSize(30, 30);
        lblNumber->setStyleSheet("background-color: yellow; border-radius: 15px; color: red; font-weight: bold;");*/

        QPushButton* btnUpgrade = new QPushButton("升级为\n管理员");
        btnUpgrade->setStyleSheet("background-color: lightgreen; color: black; padding:6px 10px; border-radius:6px;");
        QLabel* lblManager = new QLabel("管理员");
        lblManager->setAlignment(Qt::AlignCenter);
        lblManager->setFixedHeight(40);
        lblManager->setStyleSheet("background-color: yellow; color: black; border-radius:6px; padding:6px 10px; font-weight:bold;");
        auto updateAdminWidgets = [=]() {
            bool isAdmin = (m_userInfo.strIsAdministrator == "1" ||
                m_userInfo.strIsAdministrator.compare(QString::fromUtf8(u8"是"), Qt::CaseInsensitive) == 0 ||
                m_userInfo.strIsAdministrator.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0);
            btnUpgrade->setVisible(!isAdmin);
            lblManager->setVisible(isAdmin);
        };
        updateAdminWidgets();
        connect(btnUpgrade, &QPushButton::clicked, this, [=]() {
            postAdministratorStatus(QStringLiteral("1"));
            m_userInfo.strIsAdministrator = "1";
            updateAdminWidgets();
            });

        QPushButton* btnLogout = new QPushButton("退出登录");
        btnLogout->setStyleSheet("background-color: gray; color: white; padding:6px 10px; border-radius:6px;");
        connect(btnLogout, &QPushButton::clicked, this, [=]() {
            emit logoutRequested();
            });

        QVBoxLayout* pVBoxLayout = new QVBoxLayout();
        pVBoxLayout->addWidget(lblManager);
        pVBoxLayout->addWidget(btnLogout);

        QLabel* pLabel = new QLabel(this);
        pLabel->setFixedSize(QSize(22, 22));
        closeLayout->addWidget(pLabel);
        closeLayout->addStretch();
        closeLayout->addWidget(closeButton);
        //topLayout->addWidget(lblNumber);
        //topLayout->addWidget(titleLabel);
        //topLayout->addStretch();
        topLayout->addWidget(avatarLabel);  //头像
        topLayout->addWidget(nameLabel);    //姓名
        topLayout->addWidget(btnUpgrade);
        topLayout->addLayout(pVBoxLayout);
        //topLayout->addWidget(btnManager);
        //topLayout->addWidget(btnLogout);

        // ===================== 用户基本信息 =====================
        QGridLayout* infoLayout = new QGridLayout;
        infoLayout->setHorizontalSpacing(20);
        infoLayout->setVerticalSpacing(12);

        const QIcon editFieldIcon(".\\res\\img\\com_ic_edit_2.png");
        const QStringList sexOptions = {
            QString::fromUtf8(u8"男"),
            QString::fromUtf8(u8"女"),
            QString::fromUtf8(u8"未知")
        };

        auto showStyledTextDialog = [this](const QString& title, const QString& prompt,
            const QString& defaultText, bool* ok) -> QString {
                return execStyledDialog(title, prompt, defaultText, QStringList(), ok, false);
            };

        auto showStyledChoiceDialog = [&](const QString& title, const QString& prompt,
            const QStringList& options, int currentIndex, bool* ok) -> QString {
                int idx = currentIndex >= 0 && currentIndex < options.size() ? currentIndex : 0;
                QString defaultValue = options.isEmpty() ? QString() : options.at(idx);
                return execStyledDialog(title, prompt, defaultValue, options, ok, true);
            };

        int infoRow = 0;

        auto addEditableField = [&](const QString& title, QString UserInfo::*member,
            const QString& endpoint, const QString& fieldKey, int row) {
            QLabel* keyLabel = new QLabel(title);
            keyLabel->setStyleSheet("color: #cccccc;");

            NameLabel* valueLabel = new NameLabel;
            valueLabel->setText(m_userInfo.*member);
            valueLabel->setStyleSheet("color: white;");

            QToolButton* editBtn = new QToolButton;
            editBtn->setIcon(editFieldIcon);
            editBtn->setIconSize(QSize(16, 16));
            editBtn->setCursor(Qt::PointingHandCursor);
            editBtn->setAutoRaise(true);
            editBtn->setToolTip(QString::fromUtf8(u8"修改%1").arg(title));

            auto handler = [this, valueLabel, member, title, endpoint, fieldKey, showStyledTextDialog]() {
                bool ok = false;
                QString currentValue = valueLabel->text();
                QString titleText = QString::fromUtf8(u8"修改%1").arg(title);
                QString promptText = QString::fromUtf8(u8"请输入新的%1：").arg(title);
                QString newValue = showStyledTextDialog(titleText, promptText, currentValue, &ok);
                if (!ok) {
                    return;
                }
                newValue = newValue.trimmed();
                if (newValue.isEmpty()) {
                    QMessageBox::warning(this, QString::fromUtf8(u8"提示"),
                        QString::fromUtf8(u8"%1不能为空！").arg(title));
                    return;
                }
                if (newValue == currentValue) {
                    return;
                }
                valueLabel->setText(newValue);
                this->m_userInfo.*member = newValue;
                postSingleFieldUpdate(endpoint, fieldKey, newValue);
            };

            connect(valueLabel, &NameLabel::labelClicked, this, handler);
            connect(valueLabel, &NameLabel::editIconClicked, this, handler);
            connect(editBtn, &QToolButton::clicked, this, handler);

            infoLayout->addWidget(keyLabel, row, 0);
            infoLayout->addWidget(valueLabel, row, 1);
            infoLayout->addWidget(editBtn, row, 2);
            infoLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), row, 3);
        };

        auto addChoiceField = [&](const QString& title, QString UserInfo::*member,
            const QStringList& options, const QString& endpoint, const QString& fieldKey, int row) {
            QLabel* keyLabel = new QLabel(title);
            keyLabel->setStyleSheet("color: #cccccc;");

            NameLabel* valueLabel = new NameLabel;
            valueLabel->setText(m_userInfo.*member);
            valueLabel->setStyleSheet("color: white;");

            QToolButton* editBtn = new QToolButton;
            editBtn->setIcon(editFieldIcon);
            editBtn->setIconSize(QSize(16, 16));
            editBtn->setCursor(Qt::PointingHandCursor);
            editBtn->setAutoRaise(true);
            editBtn->setToolTip(QString::fromUtf8(u8"修改%1").arg(title));

            auto handler = [this, valueLabel, member, title, options, endpoint, fieldKey, showStyledChoiceDialog]() {
                bool ok = false;
                QString currentValue = valueLabel->text();
                QString titleText = QString::fromUtf8(u8"修改%1").arg(title);
                QString promptText = QString::fromUtf8(u8"请选择新的%1：").arg(title);

                QStringList optionList = options;
                int currentIndex = optionList.indexOf(currentValue);
                if (currentIndex < 0 && !currentValue.isEmpty()) {
                    optionList.prepend(currentValue);
                    currentIndex = 0;
                }

                QString newValue = showStyledChoiceDialog(titleText, promptText,
                    optionList, currentIndex < 0 ? 0 : currentIndex, &ok);
                if (!ok) {
                    return;
                }
                if (newValue == currentValue) {
                    return;
                }
                valueLabel->setText(newValue);
                this->m_userInfo.*member = newValue;
                postSingleFieldUpdate(endpoint, fieldKey, newValue);
            };

            connect(valueLabel, &NameLabel::labelClicked, this, handler);
            connect(valueLabel, &NameLabel::editIconClicked, this, handler);
            connect(editBtn, &QToolButton::clicked, this, handler);

            infoLayout->addWidget(keyLabel, row, 0);
            infoLayout->addWidget(valueLabel, row, 1);
            infoLayout->addWidget(editBtn, row, 2);
            infoLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), row, 3);
        };

        addChoiceField(QString::fromUtf8(u8"性别"), &UserInfo::strSex, sexOptions,
            QStringLiteral("http://47.100.126.194:5000/updateUserSex"), QStringLiteral("sex"), infoRow++);
        addEditableField(QString::fromUtf8(u8"地址"), &UserInfo::strAddress,
            QStringLiteral("http://47.100.126.194:5000/updateUserAddress"), QStringLiteral("address"), infoRow++);
        addEditableField(QString::fromUtf8(u8"学校名"), &UserInfo::strSchoolName,
            QStringLiteral("http://47.100.126.194:5000/updateUserSchoolName"), QStringLiteral("school_name"), infoRow++);
        addEditableField(QString::fromUtf8(u8"学段"), &UserInfo::strGradeLevel,
            QStringLiteral("http://47.100.126.194:5000/updateUserGradeLevel"), QStringLiteral("grade_level"), infoRow++);

        // ===================== 任教信息（年级/任教科目/任教班级，多行） =====================
        if (kEnableTeachingsUI) {
            QLabel* teachLabel = new QLabel(QString::fromUtf8(u8"任教信息"));
            teachLabel->setStyleSheet("color: #cccccc;");
            infoLayout->addWidget(teachLabel, infoRow, 0, Qt::AlignTop);

            QWidget* teachWidget = new QWidget(this);
            QVBoxLayout* teachLayout = new QVBoxLayout(teachWidget);
            teachLayout->setContentsMargins(0, 0, 0, 0);
            teachLayout->setSpacing(6);

            m_teachingsTable = new QTableWidget(teachWidget);
            m_teachingsTable->setColumnCount(3);
            m_teachingsTable->setHorizontalHeaderLabels({
                QString::fromUtf8(u8"年级"),
                QString::fromUtf8(u8"任教科目"),
                QString::fromUtf8(u8"任教班级")
            });
            m_teachingsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            m_teachingsTable->verticalHeader()->setVisible(false);
            m_teachingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
            m_teachingsTable->setSelectionMode(QAbstractItemView::SingleSelection);
            m_teachingsTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
            m_teachingsTable->setMinimumHeight(180);
            // 深灰主题（匹配窗口背景风格）
            m_teachingsTable->setStyleSheet(
                "QTableWidget {"
                "  background-color: #5c5c5c;"
                "  color: #ffffff;"
                "  border: 1px solid rgba(255,255,255,0.12);"
                "  gridline-color: rgba(255,255,255,0.10);"
                "  selection-background-color: rgba(255,255,255,0.18);"
                "  selection-color: #ffffff;"
                "}"
                "QHeaderView::section {"
                "  background-color: #4f4f4f;"
                "  color: #ffffff;"
                "  padding: 4px 6px;"
                "  border: 1px solid rgba(255,255,255,0.12);"
                "}"
                "QTableWidget::item {"
                "  background-color: #5c5c5c;"
                "  padding: 4px 6px;"
                "}"
                "QTableWidget::item:selected {"
                "  background-color: rgba(25,118,210,0.55);"
                "}"
                "QTableCornerButton::section {"
                "  background-color: #4f4f4f;"
                "  border: 1px solid rgba(255,255,255,0.12);"
                "}"
            );
            m_teachingsTable->setAlternatingRowColors(true);
            m_teachingsTable->setShowGrid(true);

            auto addTeachingRow = [this](const QString& grade, const QString& subject, const QString& classTaught) {
                if (!m_teachingsTable) return;
                const int r = m_teachingsTable->rowCount();
                m_teachingsTable->insertRow(r);
                m_teachingsTable->setItem(r, 0, new QTableWidgetItem(grade));
                m_teachingsTable->setItem(r, 1, new QTableWidgetItem(subject));
                m_teachingsTable->setItem(r, 2, new QTableWidgetItem(classTaught));
            };

            // 初始化：只使用多条任教记录（teachings）；若为空，则给一行空白供编辑
            if (!m_userInfo.teachings.isEmpty()) {
                for (const auto& t : m_userInfo.teachings) {
                    addTeachingRow(t.grade, t.subject, t.class_taught);
                }
            } else {
                addTeachingRow(QString(), QString(), QString());
            }

            QPushButton* btnAddTeachingRow = new QPushButton("+", teachWidget);
            btnAddTeachingRow->setFixedSize(34, 28);
            btnAddTeachingRow->setCursor(Qt::PointingHandCursor);
            btnAddTeachingRow->setStyleSheet(
                "QPushButton { background-color:#1976d2; color:white; border:none; border-radius:4px; font-weight:bold; }"
                "QPushButton:hover { background-color:#1e88e5; }"
            );
            connect(btnAddTeachingRow, &QPushButton::clicked, this, [=]() {
                addTeachingRow(QString(), QString(), QString());
                int r = m_teachingsTable->rowCount() - 1;
                if (r >= 0) {
                    m_teachingsTable->setCurrentCell(r, 0);
                    m_teachingsTable->editItem(m_teachingsTable->item(r, 0));
                }
            });

            QHBoxLayout* teachBtnRow = new QHBoxLayout();
            teachBtnRow->setContentsMargins(0, 0, 0, 0);
            teachBtnRow->addStretch();
            teachBtnRow->addWidget(btnAddTeachingRow);

            teachLayout->addWidget(m_teachingsTable);
            teachLayout->addLayout(teachBtnRow);

            infoLayout->addWidget(teachWidget, infoRow, 1, 1, 3);
            infoRow++;
        } // kEnableTeachingsUI

        // ===================== 底部按钮 =====================
        QHBoxLayout* btnBottomLayout = new QHBoxLayout;
        QPushButton* btnCancel = new QPushButton("取消");
        btnCancel->setStyleSheet("background-color: gray; color: white; padding:8px 16px; border-radius:4px;");
        QPushButton* btnOk = new QPushButton("确定");
        btnOk->setStyleSheet("background-color: #1976d2; color: white; padding:8px 16px; border-radius:4px;");

        btnBottomLayout->addStretch();
        btnBottomLayout->addWidget(btnCancel);
        btnBottomLayout->addWidget(btnOk);
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(btnOk, &QPushButton::clicked, this, [this, btnOk, kEnableTeachingsUI]() {
            if (!kEnableTeachingsUI) {
                // 任教信息功能暂时隐藏，不提交 teachings，直接关闭
                accept();
                return;
            }

            // 收集任教信息表格 -> teachings[]，并调用 /updateUserTeachings（替换整份列表）
            if (!m_teachingsTable) {
                accept();
                return;
            }

            QJsonArray teachings;
            for (int r = 0; r < m_teachingsTable->rowCount(); ++r) {
                auto cellText = [&](int c) -> QString {
                    QTableWidgetItem* it = m_teachingsTable->item(r, c);
                    return it ? it->text().trimmed() : QString();
                };
                QString grade = cellText(0);
                QString subject = cellText(1);
                QString classTaught = cellText(2);

                // 空行跳过
                if (grade.isEmpty() && subject.isEmpty() && classTaught.isEmpty()) {
                    continue;
                }
                // 半空行提示（避免写入脏数据）
                if (grade.isEmpty() || subject.isEmpty() || classTaught.isEmpty()) {
                    QMessageBox::warning(this, QString::fromUtf8(u8"提示"),
                        QString::fromUtf8(u8"第 %1 行任教信息不完整，请补全“年级/任教科目/任教班级”。").arg(r + 1));
                    return;
                }

                QJsonObject item;
                item["grade_level"] = m_userInfo.strGradeLevel.trimmed(); // 统一使用当前“学段”
                item["grade"] = grade;
                item["subject"] = subject;
                item["class_taught"] = classTaught;
                teachings.append(item);
            }

            if (teachings.isEmpty()) {
                QMessageBox::warning(this, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"请至少填写一条任教信息。"));
                return;
            }
            if (m_userInfo.strPhone.trimmed().isEmpty()) {
                QMessageBox::warning(this, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"用户手机号为空，无法提交任教信息。"));
                return;
            }

            // 回写内存态：任教信息以 teachings[] 为准（多条）
            m_userInfo.teachings.clear();
            for (const QJsonValue& v : teachings) {
                if (!v.isObject()) continue;
                QJsonObject t = v.toObject();
                UserTeachingInfo info;
                info.grade_level = t.value("grade_level").toString();
                info.grade = t.value("grade").toString();
                info.subject = t.value("subject").toString();
                info.class_taught = t.value("class_taught").toString();
                m_userInfo.teachings.append(info);
            }

            QJsonObject payload;
            payload["phone"] = m_userInfo.strPhone.trimmed();
            payload["teachings"] = teachings;
            QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);

            btnOk->setEnabled(false);

            // 使用独立的 handler，避免与头像/姓名等更新请求混淆
            TAHttpHandler* h = new TAHttpHandler(this);
            QPointer<UserInfoDialog> self(this);
            QObject::connect(h, &TAHttpHandler::success, this, [self, h, btnOk](const QString& response) {
                if (h) h->deleteLater();
                if (!self) return;
                btnOk->setEnabled(true);

                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8(), &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    int code = obj.value("code").toInt(-1);
                    if (obj.contains("data") && obj.value("data").isObject()) {
                        code = obj.value("data").toObject().value("code").toInt(code);
                    }
                    if (code == 200 || code == 0) {
                        CommonInfo::InitData(self->m_userInfo);
                        self->accept();
                        return;
                    }
                }
                QMessageBox::warning(self, QString::fromUtf8(u8"更新失败"), QString::fromUtf8(u8"任教信息更新失败，请稍后重试。"));
            });
            QObject::connect(h, &TAHttpHandler::failed, this, [self, h, btnOk](const QString&) {
                if (h) h->deleteLater();
                if (!self) return;
                btnOk->setEnabled(true);
                QMessageBox::warning(self, QString::fromUtf8(u8"网络错误"), QString::fromUtf8(u8"任教信息更新失败，请检查网络后重试。"));
            });

            h->post(QStringLiteral("http://47.100.126.194:5000/updateUserTeachings"), jsonData);
        });

        // ===================== 底部工具栏 =====================
        //QHBoxLayout* toolLayout = new QHBoxLayout;
        //QStringList toolNames = { "电话", "消息", "相册", "文档", "设置", "用户" };
        //QStringList icons = { ":/icons/phone.png", ":/icons/message.png", ":/icons/photo.png", ":/icons/doc.png", ":/icons/settings.png", ":/icons/user.png" };

        //for (int i = 0; i < toolNames.size(); ++i) {
        //    QPushButton* btn = new QPushButton;
        //    btn->setIcon(QIcon(icons[i])); // 图标文件需要在资源中准备好
        //    btn->setIconSize(QSize(24, 24));
        //    btn->setFlat(true);
        //    btn->setStyleSheet("QPushButton {border:none; color: white;} QPushButton:hover { color: lightblue; }");
        //    toolLayout->addWidget(btn);
        //    toolLayout->addStretch();
        //}

        // ===================== 主布局 =====================
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(closeLayout);
        mainLayout->addLayout(topLayout);
        mainLayout->addSpacing(10);
        mainLayout->addLayout(infoLayout);
        mainLayout->addSpacing(20);
        mainLayout->addLayout(btnBottomLayout);

        setLayout(mainLayout);
        //mainLayout->addStretch();
        //mainLayout->addLayout(toolLayout);

        // 连接信号
        //connect(btnCancel, &QPushButton::clicked, this, &TABaseDialog::reject);
        //connect(btnOk, &QPushButton::clicked, this, &TABaseDialog::accept);
    }

    void paintEvent(QPaintEvent* event) override {
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

    QString titleName() const { return m_titleName; }
    QColor backgroundColor() const { return m_backgroundColor; }
    QColor borderColor() const { return m_borderColor; }
    int borderWidth() const { return m_borderWidth; }
    int radius() const { return m_radius; }

    void mousePressEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
        }
    }

    void leaveEvent(QEvent* event)
    {
        QDialog::leaveEvent(event);
        closeButton->hide();
    }

    void enterEvent(QEvent* event)
    {
        QDialog::enterEvent(event);
        if (m_visibleCloseButton)
            closeButton->show();
    }

    void resizeEvent(QResizeEvent* event)
    {
        QDialog::resizeEvent(event);
        //initShow();
        closeButton->move(this->width() - 22, 0);
    }

    void setTitleName(const QString& name) {
        m_titleName = name;
        update();
    }

    void visibleCloseButton(bool val)
    {
        m_visibleCloseButton = val;
    }

    void setBackgroundColor(const QColor& color) {
        m_backgroundColor = color;
        update();
    }

    void setBorderColor(const QColor& color) {
        m_borderColor = color;
        update();
    }

    void setBorderWidth(int val) {
        m_borderWidth = val;
        update();
    }

    void setRadius(int val) {
        m_radius = val;
        update();
    }

    UserInfo m_userInfo;
    bool m_dragging;
    QPoint m_dragStartPos;
    QString m_titleName;
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth;
    int m_radius;
    bool m_visibleCloseButton;
    QPushButton* closeButton = NULL;
    AvatarLabel* avatarLabel = NULL;
    NameLabel* nameLabel = NULL;
    TAHttpHandler* m_httpHandler = NULL;
    QTableWidget* m_teachingsTable = nullptr;

private:
    QMap<QString, QString> buildUserInfoParams() const
    {
        QMap<QString, QString> params;
        params["avatar"] = m_userInfo.avatar;
        params["phone"] = m_userInfo.strPhone;
        params["name"] = m_userInfo.strName;
        params["sex"] = m_userInfo.strSex;
        params["address"] = m_userInfo.strAddress;
        params["school_name"] = m_userInfo.strSchoolName;
        params["grade_level"] = m_userInfo.strGradeLevel;
        // 任教信息已迁移到 /updateUserTeachings，不再通过 /updateUserInfo 传 grade/subject/class_taught
        params["is_administrator"] = m_userInfo.strIsAdministrator;
        params["id_number"] = m_userInfo.strIdNumber;
        return params;
    }

    void postSingleFieldUpdate(const QString& endpoint, const QString& fieldKey, const QString& value)
    {
        if (!m_httpHandler || endpoint.isEmpty() || fieldKey.isEmpty()) {
            return;
        }
        QMap<QString, QString> params;
        params["phone"] = m_userInfo.strPhone;
        params["id_number"] = m_userInfo.strIdNumber;
        params[fieldKey] = value;
        m_httpHandler->post(endpoint, params);
    }
};
