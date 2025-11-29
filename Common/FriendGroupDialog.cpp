#include "ScheduleDialog.h"
#include "FriendGroupDialog.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QJsonParseError>
#include <QJsonValue>
#include <QIcon>
#include <QResizeEvent>
#include <QEvent>
#include <QDebug>
#include <QSize>
#include <QPixmap>
#include <cstring>

// RowItem 类实现
RowItem::RowItem(const QString& text, QWidget* parent) : QFrame(parent)
{
    setObjectName("RowItem");
    setFixedHeight(28);
    setStyleSheet(
        "QFrame#RowItem {"
        " background-color: rgb(50,50,50);"
        " border: 1px solid rgba(255,255,255,0.08);"
        " border-radius: 6px;"
        "}"
        "QFrame#RowItem:hover { background-color: rgb(60,60,60); }"
        "QLabel { color:#ffffff; font-size:15px; font-weight:bold; }"
        "QToolButton { border:none; color:#e0e0e0; }"
    );
    auto h = new QHBoxLayout(this);
    h->setContentsMargins(16, 0, 16, 0);

    auto lbl = new QLabel(text);
    auto arrow = new QToolButton;
    arrow->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    arrow->setAutoRaise(true);
    arrow->setEnabled(false); // 仅作为装饰

    h->addWidget(lbl);
    h->addStretch();
    h->addWidget(arrow);

    setCursor(Qt::PointingHandCursor);
}

void RowItem::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) emit clicked();
    QFrame::mousePressEvent(e);
}

void FriendGroupDialog::setupFriendTree()
{
    if (!fLayout || m_friendTree)
        return;

    fLayout->setContentsMargins(6, 6, 6, 6);
    fLayout->setSpacing(6);

    m_friendTree = new QTreeWidget;
    m_friendTree->setHeaderHidden(true);
    m_friendTree->setStyleSheet(
        "QTreeWidget {"
        " background: transparent;"
        " border: none;"
        " color: #f5f5f5;"
        " font-size: 15px;"
        "}"
        "QTreeWidget::item {"
        " margin: 2px;"
        " padding: 8px;"
        " border-radius: 10px;"
        "}"
        "QTreeWidget::item:selected {"
        " background-color: rgba(255,255,255,0.12);"
        "}"
    );
    m_friendTree->setIndentation(18);
    m_friendTree->setExpandsOnDoubleClick(false);

    fLayout->addWidget(m_friendTree);

    m_classRootItem = new QTreeWidgetItem(QStringList() << QStringLiteral("班级 (0)"));
    m_teacherRootItem = new QTreeWidgetItem(QStringList() << QStringLiteral("教师 (0)"));
    m_friendTree->addTopLevelItem(m_classRootItem);
    m_friendTree->addTopLevelItem(m_teacherRootItem);
    m_classRootItem->setExpanded(true);
    m_teacherRootItem->setExpanded(true);

    connect(m_friendTree, &QTreeWidget::itemActivated, this, &FriendGroupDialog::handleFriendItemActivated);
    connect(m_friendTree, &QTreeWidget::itemClicked, this, &FriendGroupDialog::handleFriendItemActivated);
}

void FriendGroupDialog::clearFriendTree()
{
    auto clearChildren = [](QTreeWidgetItem* root) {
        if (!root)
            return;
        while (root->childCount() > 0) {
            delete root->takeChild(0);
        }
    };
    clearChildren(m_classRootItem);
    clearChildren(m_teacherRootItem);
    m_classItemMap.clear();
    m_teacherItemMap.clear();
    updateFriendCounts();
}

void FriendGroupDialog::updateFriendCounts()
{
    if (m_classRootItem) {
        m_classRootItem->setText(0, QStringLiteral("班级 (%1)").arg(m_classItemMap.size()));
    }
    if (m_teacherRootItem) {
        m_teacherRootItem->setText(0, QStringLiteral("教师 (%1)").arg(m_teacherItemMap.size()));
    }
}

void FriendGroupDialog::addClassNode(const QString& displayName, const QString& groupId, const QString& classid, bool iGroupOwner, bool isClassGroup)
{
    if (!m_classRootItem || groupId.isEmpty())
        return;
    if (m_classItemMap.contains(groupId))
        return;

    QString title = displayName.trimmed().isEmpty() ? QStringLiteral("未命名班级") : displayName.trimmed();
    QTreeWidgetItem* item = new QTreeWidgetItem(m_classRootItem);
    item->setText(0, title);
    item->setData(0, Qt::UserRole, QStringLiteral("class"));
    item->setData(0, Qt::UserRole + 1, groupId);
    item->setData(0, Qt::UserRole + 2, classid);
    item->setData(0, Qt::UserRole + 3, iGroupOwner);
    item->setData(0, Qt::UserRole + 4, isClassGroup);
    m_classItemMap.insert(groupId, item);
    updateFriendCounts();
}

void FriendGroupDialog::addTeacherNode(const QString& displayName, const QString& subtitle, const QString& avatarPath, const QString& teacherId)
{
    if (!m_teacherRootItem)
        return;

    QString key = teacherId.isEmpty() ? displayName : teacherId;
    if (key.trimmed().isEmpty() || m_teacherItemMap.contains(key))
        return;

    QString title = displayName.trimmed().isEmpty() ? QStringLiteral("未命名教师") : displayName.trimmed();
    QString line = subtitle.trimmed().isEmpty() ? title : QString("%1  ·  %2").arg(title, subtitle.trimmed());

    QTreeWidgetItem* item = new QTreeWidgetItem(m_teacherRootItem);
    item->setText(0, line);
    item->setData(0, Qt::UserRole, QStringLiteral("teacher"));
    if (!avatarPath.isEmpty() && QFile::exists(avatarPath)) {
        item->setIcon(0, QIcon(avatarPath));
    }
    m_teacherItemMap.insert(key, item);
    updateFriendCounts();
}

void FriendGroupDialog::handleFriendItemActivated(QTreeWidgetItem* item)
{
    if (!item || item == m_classRootItem || item == m_teacherRootItem)
        return;

    const QString type = item->data(0, Qt::UserRole).toString();
    if (type == QStringLiteral("class")) {
        QString groupId = item->data(0, Qt::UserRole + 1).toString();
        QString classid = item->data(0, Qt::UserRole + 2).toString();
        bool owner = item->data(0, Qt::UserRole + 3).toBool();
        bool isClassGroup = item->data(0, Qt::UserRole + 4).toBool();
        openScheduleForGroup(item->text(0), groupId, classid, owner, isClassGroup);
    } else if (type == QStringLiteral("teacher")) {
        // 预留：可在此弹出教师详情或聊天窗口
    }
}

void FriendGroupDialog::setupGroupTree()
{
    if (m_groupTree || !gLayout)
        return;

    m_groupTree = new QTreeWidget;
    m_groupTree->setHeaderHidden(true);
    m_groupTree->setIndentation(18);
    m_groupTree->setStyleSheet(
        "QTreeWidget {"
        " background: transparent;"
        " border: none;"
        " color: #f5f5f5;"
        " font-size: 15px;"
        "}"
        "QTreeWidget::item {"
        " margin: 2px;"
        " padding: 8px;"
        " border-radius: 10px;"
        "}"
        "QTreeWidget::item:selected {"
        " background-color: rgba(255,255,255,0.12);"
        "}"
    );

    m_classGroupRoot = new QTreeWidgetItem(QStringList() << QStringLiteral("班级群 (0)"));
    m_classManagedRoot = new QTreeWidgetItem(QStringList() << QStringLiteral("我管理的群 (0)"));
    m_classJoinedRoot = new QTreeWidgetItem(QStringList() << QStringLiteral("我加入的群 (0)"));
    m_classGroupRoot->addChild(m_classManagedRoot);
    m_classGroupRoot->addChild(m_classJoinedRoot);

    m_normalGroupRoot = new QTreeWidgetItem(QStringList() << QStringLiteral("普通群 (0)"));
    m_normalManagedRoot = new QTreeWidgetItem(QStringList() << QStringLiteral("我管理的群 (0)"));
    m_normalJoinedRoot = new QTreeWidgetItem(QStringList() << QStringLiteral("我加入的群 (0)"));
    m_normalGroupRoot->addChild(m_normalManagedRoot);
    m_normalGroupRoot->addChild(m_normalJoinedRoot);

    m_groupTree->addTopLevelItem(m_classGroupRoot);
    m_groupTree->addTopLevelItem(m_normalGroupRoot);

    m_classGroupRoot->setExpanded(true);
    m_classManagedRoot->setExpanded(true);
    m_classJoinedRoot->setExpanded(true);
    m_normalGroupRoot->setExpanded(true);
    m_normalManagedRoot->setExpanded(true);
    m_normalJoinedRoot->setExpanded(true);

    gLayout->addWidget(m_groupTree);

    connect(m_groupTree, &QTreeWidget::itemActivated, this, &FriendGroupDialog::handleGroupItemActivated);
    connect(m_groupTree, &QTreeWidget::itemClicked, this, &FriendGroupDialog::handleGroupItemActivated);
}

void FriendGroupDialog::clearGroupTree()
{
    auto clearNode = [](QTreeWidgetItem* node) {
        if (!node)
            return;
        while (node->childCount() > 0) {
            delete node->takeChild(0);
        }
    };
    clearNode(m_classManagedRoot);
    clearNode(m_classJoinedRoot);
    clearNode(m_normalManagedRoot);
    clearNode(m_normalJoinedRoot);
    m_groupItemMap.clear();
    updateGroupCounts();
}

void FriendGroupDialog::updateGroupCounts()
{
    auto countChildren = [](QTreeWidgetItem* node) -> int {
        return node ? node->childCount() : 0;
    };
    if (m_classManagedRoot) {
        m_classManagedRoot->setText(0, QStringLiteral("我管理的群 (%1)").arg(countChildren(m_classManagedRoot)));
    }
    if (m_classJoinedRoot) {
        m_classJoinedRoot->setText(0, QStringLiteral("我加入的群 (%1)").arg(countChildren(m_classJoinedRoot)));
    }
    if (m_normalManagedRoot) {
        m_normalManagedRoot->setText(0, QStringLiteral("我管理的群 (%1)").arg(countChildren(m_normalManagedRoot)));
    }
    if (m_normalJoinedRoot) {
        m_normalJoinedRoot->setText(0, QStringLiteral("我加入的群 (%1)").arg(countChildren(m_normalJoinedRoot)));
    }
    if (m_classGroupRoot) {
        int total = countChildren(m_classManagedRoot) + countChildren(m_classJoinedRoot);
        m_classGroupRoot->setText(0, QStringLiteral("班级群 (%1)").arg(total));
    }
    if (m_normalGroupRoot) {
        int total = countChildren(m_normalManagedRoot) + countChildren(m_normalJoinedRoot);
        m_normalGroupRoot->setText(0, QStringLiteral("普通群 (%1)").arg(total));
    }
}

void FriendGroupDialog::addGroupTreeNode(const QString& displayName, const QString& groupId, const QString& classid, bool iGroupOwner, bool isClassGroup)
{
    if (!m_groupTree || groupId.isEmpty())
        return;
    if (m_groupItemMap.contains(groupId))
        return;

    QTreeWidgetItem* parent = nullptr;
    if (isClassGroup) {
        parent = iGroupOwner ? m_classManagedRoot : m_classJoinedRoot;
    } else {
        parent = iGroupOwner ? m_normalManagedRoot : m_normalJoinedRoot;
    }
    if (!parent)
        return;

    QString title = displayName.trimmed().isEmpty() ? QStringLiteral("未命名群聊") : displayName.trimmed();
    QString badge = classid.trimmed().isEmpty() ? QString() : classid.right(2);
    QString label = badge.isEmpty() ? title : QStringLiteral("%1   %2").arg(badge, title);

    QTreeWidgetItem* item = new QTreeWidgetItem(parent);
    item->setText(0, label);
    item->setData(0, Qt::UserRole, QStringLiteral("group"));
    item->setData(0, Qt::UserRole + 1, groupId);
    item->setData(0, Qt::UserRole + 2, classid);
    item->setData(0, Qt::UserRole + 3, iGroupOwner);
    item->setData(0, Qt::UserRole + 4, isClassGroup);

    parent->setExpanded(true);
    if (parent->parent())
        parent->parent()->setExpanded(true);

    m_groupItemMap.insert(groupId, item);
    updateGroupCounts();
}

void FriendGroupDialog::handleGroupItemActivated(QTreeWidgetItem* item)
{
    if (!item)
        return;
    const QString type = item->data(0, Qt::UserRole).toString();
    if (type != QStringLiteral("group"))
        return;

    QString groupId = item->data(0, Qt::UserRole + 1).toString();
    QString classid = item->data(0, Qt::UserRole + 2).toString();
    bool owner = item->data(0, Qt::UserRole + 3).toBool();
    bool isClassGroup = item->data(0, Qt::UserRole + 4).toBool();
    openScheduleForGroup(item->text(0), groupId, classid, owner, isClassGroup);
}

void FriendGroupDialog::openScheduleForGroup(const QString& groupName, const QString& unique_group_id, const QString& classid, bool iGroupOwner, bool isClassGroup)
{
    if (unique_group_id.isEmpty())
        return;

    ScheduleDialog* dlg = m_scheduleDlg.value(unique_group_id, nullptr);
    if (!dlg) {
        dlg = new ScheduleDialog(classid, this, m_pWs);
        dlg->InitWebSocket();
        QList<Notification> curNotification;
        for (const auto& iter : notifications) {
            if (iter.unique_group_id == unique_group_id) {
                curNotification.append(iter);
            }
        }
        dlg->setNoticeMsg(curNotification);
        if (m_prepareClassHistoryCache.contains(unique_group_id)) {
            dlg->setPrepareClassHistory(m_prepareClassHistoryCache.value(unique_group_id));
        }
        connectGroupLeftSignal(dlg, unique_group_id);
        m_scheduleDlg[unique_group_id] = dlg;
    }

    if (dlg->isHidden()) {
        dlg->InitData(groupName, unique_group_id, classid, iGroupOwner, isClassGroup);
        dlg->show();
    } else {
        dlg->hide();
    }
}

// FriendGroupDialog 类实现
FriendGroupDialog::FriendGroupDialog(QWidget* parent, TaQTWebSocket* pWs)
    : QDialog(parent), m_dragging(false),
    m_backgroundColor(QColor(50, 50, 50)),
    m_borderColor(Qt::white),
    m_borderWidth(2), m_radius(6),
    m_visibleCloseButton(true)
{
    FriendNotifyDialog::ensureCallbackRegistered();
    GroupNotifyDialog::ensureCallbackRegistered();
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    //setModal(true);
    setAttribute(Qt::WA_TranslucentBackground);
    m_pWs = pWs;

    setWindowTitle("好友 / 群聊");
    resize(500, 650);

    //m_scheduleDlg = new ScheduleDialog(this, pWs);
    //m_scheduleDlg->InitWebSocket();

    //fLayout->addLayout(makePairBtn("老师头像", "老师昵称", "green", "white"));
    //fLayout->addLayout(makePairBtn("老师头像", "老师昵称", "green", "white"));
    m_httpHandler = new TAHttpHandler(this);
    if (m_httpHandler)
    {
        connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
            //成功消息就不发送了
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
            if (jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                
                // 处理群组信息（/groups/by-teacher 接口返回）
                if (obj["data"].isObject()) {
                    m_setClassId.clear();
                    QJsonObject dataObj = obj["data"].toObject();

                    bool isGrouplist = false;
                    
                    // 处理群主群组列表（owner_groups）
                    if (dataObj["owner_groups"].isArray()) {
                        QJsonArray ownerGroupsArray = dataObj["owner_groups"].toArray();
                        for (int i = 0; i < ownerGroupsArray.size(); i++) {
                            QString classid;
                            QJsonObject groupObj = ownerGroupsArray[i].toObject();
                            QString groupId = groupObj["group_id"].toString();
                            QString groupName = groupObj["group_name"].toString();
                            if (!groupObj["classid"].isNull())
                            {
                                classid = groupObj["classid"].toString();
                            }
                            
                            // 处理群组头像（如果有）
                            QString avatarPath = "";
                            QString faceUrl = groupObj["face_url"].toString();
                            if (!faceUrl.isEmpty()) {
                                // 从 face_url 中提取文件名
                                QString fileName = faceUrl.section('/', -1);
                                QString saveDir = QCoreApplication::applicationDirPath() + "/group_images/" + groupId;
                                QDir().mkpath(saveDir);
                                avatarPath = saveDir + "/" + fileName;
                            }
                            
                            // 如果没有头像，使用默认路径或空字符串
                            if (avatarPath.isEmpty()) {
                                avatarPath = ""; // 或者设置一个默认头像路径
                            }
                            
                            // 读取 is_class_group 字段（默认为1，表示班级群）
                            bool isClassGroup = groupObj.contains("is_class_group") ? (groupObj["is_class_group"].toInt() == 1) : true;
                            
                            addClassNode(groupName, groupId, classid, true, isClassGroup);
                            addGroupTreeNode(groupName, groupId, classid, true, isClassGroup);
                            if (!classid.isEmpty())
                            {
                                m_setClassId.insert(classid);
                            }
                        }
                        isGrouplist = true;
                    }
                    
                    // 处理成员群组列表（member_groups）
                    if (dataObj["member_groups"].isArray()) {
                        QJsonArray memberGroupsArray = dataObj["member_groups"].toArray();
                        for (int i = 0; i < memberGroupsArray.size(); i++) {
                            QString classid;
                            QJsonObject groupObj = memberGroupsArray[i].toObject();
                            QString groupId = groupObj["group_id"].toString();
                            QString groupName = groupObj["group_name"].toString();
                            if (!groupObj["classid"].isNull())
                            {
                                classid = groupObj["classid"].toString();
                            }
                            
                            // 处理群组头像（如果有）
                            QString avatarPath = "";
                            QString faceUrl = groupObj["face_url"].toString();
                            if (!faceUrl.isEmpty()) {
                                // 从 face_url 中提取文件名
                                QString fileName = faceUrl.section('/', -1);
                                QString saveDir = QCoreApplication::applicationDirPath() + "/group_images/" + groupId;
                                QDir().mkpath(saveDir);
                                avatarPath = saveDir + "/" + fileName;
                            }
                            
                            // 如果没有头像，使用默认路径或空字符串
                            if (avatarPath.isEmpty()) {
                                avatarPath = ""; // 或者设置一个默认头像路径
                            }
                            
                            // 读取 is_class_group 字段（默认为1，表示班级群）
                            bool isClassGroup = groupObj.contains("is_class_group") ? (groupObj["is_class_group"].toInt() == 1) : true;
                            
                            addClassNode(groupName, groupId, classid, false, isClassGroup);
                            addGroupTreeNode(groupName, groupId, classid, false, isClassGroup);
                            if (!classid.isEmpty())
                            {
                                m_setClassId.insert(classid);
                            }
                        }
                        isGrouplist = true;
                    }
                    //init data
					if (addGroupWidget && isGrouplist)
					{
						addGroupWidget->InitData(m_setClassId);
					}
                }
                
                // 处理好友信息（原有逻辑）
                if (obj["friends"].isArray())
                {
                    QJsonArray friendsArray = obj.value("friends").toArray();
                    for (int i = 0; i < friendsArray.size(); i++)
                    {
                        QJsonObject friendObj = friendsArray.at(i).toObject();

                        // teacher_info 对象
                        QJsonObject teacherInfo = friendObj.value("teacher_info").toObject();
                        int id = teacherInfo.value("id").toInt();
                        QString name = teacherInfo.value("name").toString();
                        QString subject = teacherInfo.value("subject").toString();
                        QString idCard = teacherInfo.value("id_card").toString();

                        // user_details 对象
                        QJsonObject userDetails = friendObj.value("user_details").toObject();
                        QString phone = userDetails.value("phone").toString();
                        QString uname = userDetails.value("name").toString();
                        QString sex = userDetails.value("sex").toString();

                        /********************************************/
                        QString avatar = userDetails.value("avatar").toString();
                        QString strIdNumber = userDetails.value("id_number").toString();
                        QString avatarBase64 = userDetails.value("avatar_base64").toString();

                        // 没有文件名就用手机号或ID代替
                        if (avatar.isEmpty())
                            avatar = userDetails.value("id_number").toString() + "_" + ".png";

                        // 从最后一个 "/" 之后开始截取
                        QString fileName = avatar.section('/', -1);  // "320506197910016493_.png"
                        QString saveDir = QCoreApplication::applicationDirPath() + "/avatars/" + strIdNumber; // 保存图片目录
                        QDir().mkpath(saveDir);
                        QString filePath = saveDir + "/" + fileName;

                        if (avatarBase64.isEmpty()) {
                            qWarning() << "No avatar data for" << filePath;
                            //continue;
                        }
                        //m_userInfo.strHeadImagePath = filePath;

                        // Base64 解码成图片二进制数据
                        QByteArray imageData = QByteArray::fromBase64(avatarBase64.toUtf8());

                        // 写入文件（覆盖旧的）
                        QFile file(filePath);
                        if (!file.open(QIODevice::WriteOnly)) {
                            qWarning() << "Cannot open file for writing:" << filePath;
                            //continue;
                        }
                        file.write(imageData);
                        file.close();

                        QString displayName = name.isEmpty() ? uname : name;
                        QString subjectLabel = subject.isEmpty() ? QString() : QStringLiteral("%1老师").arg(subject);
                        QString primaryText = subjectLabel.isEmpty() ? displayName : subjectLabel + displayName;
                        QString teacherUniqueId = teacherInfo.value("teacher_unique_id").toString();
                        QString subtitle = phone.isEmpty() ? QString() : phone;
                        addTeacherNode(primaryText, subtitle, filePath, teacherUniqueId);
                        /********************************************/
                    }

                    QJsonObject oTmp = obj["data"].toObject();
                    QString strTmp = oTmp["message"].toString();
                    qDebug() << "status:" << oTmp["code"].toString();
                    qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
                    //errLabel->setText(strTmp);
                    //user_id = oTmp["user_id"].toInt();

                }
                else if (obj["data"].isObject())
                {
                    QJsonObject dataObj = obj["data"].toObject();
                    if (dataObj["groups"].isArray())
                    {
                        QJsonArray groupArray = dataObj["groups"].toArray();
                        for (int i = 0; i < groupArray.size(); i++)
                        {
                            QJsonObject groupObj = groupArray.at(i).toObject();
                            QString unique_group_id = groupObj.value("unique_group_id").toString();
                            QString avatar_base64 = groupObj.value("avatar_base64").toString();
                            QString headImage_path = groupObj.value("headImage_path").toString();
                            QString classid = groupObj["classid"].toString();

                            //// 假设 avatarBase64 是 QString 类型，从 HTTP 返回的 JSON 中拿到
                            //QByteArray imageData = QByteArray::fromBase64(avatar_base64.toUtf8());
                            //QPixmap pixmap;
                            //pixmap.loadFromData(imageData);

                            // 从最后一个 "/" 之后开始截取
                            QString fileName = headImage_path.section('/', -1);  // "320506197910016493_.png"
                            QString saveDir = QCoreApplication::applicationDirPath() + "/group_images/" + unique_group_id; // 保存图片目录
                            QDir().mkpath(saveDir);
                            QString filePath = saveDir + "/" + fileName;

                            if (avatar_base64.isEmpty()) {
                                qWarning() << "No avatar data for" << filePath;
                                //continue;
                            }
                            //m_userInfo.strHeadImagePath = filePath;

                            // Base64 解码成图片二进制数据
                            QByteArray imageData = QByteArray::fromBase64(avatar_base64.toUtf8());

                            // 写入文件（覆盖旧的）
                            QFile file(filePath);
                            if (!file.open(QIODevice::WriteOnly)) {
                                qWarning() << "Cannot open file for writing:" << filePath;
                                //continue;
                            }
                            file.write(imageData);
                            file.close();

                            //lblAvatar->setPixmap(pixmap.scaled(lblAvatar->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                            //lblAvatar->setScaledContents(true);

                            // 读取 is_class_group 字段（默认为1，表示班级群）
                            bool isClassGroup = groupObj.contains("is_class_group") ? (groupObj["is_class_group"].toInt() == 1) : true;
                            QString displayName = groupObj.value("nickname").toString();
                            if (displayName.isEmpty()) {
                                displayName = groupObj.value("group_name").toString();
                            }
                            
                            addClassNode(displayName, unique_group_id, classid, true, isClassGroup);
                            addGroupTreeNode(displayName, unique_group_id, classid, true, isClassGroup);
                        }
                    }
                    else if (dataObj["joingroups"].isArray())
                    {
                        QJsonArray groupArray = dataObj["joingroups"].toArray();
                        for (int i = 0; i < groupArray.size(); i++)
                        {
                            QJsonObject groupObj = groupArray.at(i).toObject();
                            QString unique_group_id = groupObj.value("unique_group_id").toString();
                            QString avatar_base64 = groupObj.value("avatar_base64").toString();
                            QString headImage_path = groupObj.value("headImage_path").toString();
                            QString classid = groupObj["classid"].toString();

                            //// 假设 avatarBase64 是 QString 类型，从 HTTP 返回的 JSON 中拿到
                            //QByteArray imageData = QByteArray::fromBase64(avatar_base64.toUtf8());
                            //QPixmap pixmap;
                            //pixmap.loadFromData(imageData);

                            // 从最后一个 "/" 之后开始截取
                            QString fileName = headImage_path.section('/', -1);  // "320506197910016493_.png"
                            QString saveDir = QCoreApplication::applicationDirPath() + "/group_images/" + unique_group_id; // 保存图片目录
                            QDir().mkpath(saveDir);
                            QString filePath = saveDir + "/" + fileName;

                            if (avatar_base64.isEmpty()) {
                                qWarning() << "No avatar data for" << filePath;
                                //continue;
                            }
                            //m_userInfo.strHeadImagePath = filePath;

                            // Base64 解码成图片二进制数据
                            QByteArray imageData = QByteArray::fromBase64(avatar_base64.toUtf8());

                            // 写入文件（覆盖旧的）
                            QFile file(filePath);
                            if (!file.open(QIODevice::WriteOnly)) {
                                qWarning() << "Cannot open file for writing:" << filePath;
                                //continue;
                            }
                            file.write(imageData);
                            file.close();

                            //lblAvatar->setPixmap(pixmap.scaled(lblAvatar->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                            //lblAvatar->setScaledContents(true);

                            // 读取 is_class_group 字段（默认为1，表示班级群）
                            bool isClassGroup = groupObj.contains("is_class_group") ? (groupObj["is_class_group"].toInt() == 1) : true;
                            QString displayName = groupObj.value("nickname").toString();
                            if (displayName.isEmpty()) {
                                displayName = groupObj.value("group_name").toString();
                            }
                            
                            addClassNode(displayName, unique_group_id, classid, false, isClassGroup);
                            addGroupTreeNode(displayName, unique_group_id, classid, false, isClassGroup);
                        }
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

    // 顶部通知区 + 添加按钮
    QHBoxLayout* topLayout = new QHBoxLayout;
    QVBoxLayout* notifyLayout = new QVBoxLayout;
    RowItem* friendNotify = new RowItem("好友通知");
    RowItem* groupNotify = new RowItem("群通知");
    //friendNotify->setStyleSheet("font-size:16px;");
    //groupNotify->setStyleSheet("font-size:16px;");
    notifyLayout->addWidget(friendNotify);
    notifyLayout->addWidget(groupNotify);

    connect(friendNotify, &RowItem::clicked, this, [=] {
        qDebug("好友通知 clicked");
        QVector<QString> vstrNotice = addGroupWidget->getNoticeMsg();
        if (friendNotifyDlg)
        {
            friendNotifyDlg->InitData(vstrNotice);
        }

        if (friendNotifyDlg && friendNotifyDlg->isHidden())
        {
            friendNotifyDlg->show();
        }
        else if (friendNotifyDlg && !friendNotifyDlg->isHidden())
        {
            friendNotifyDlg->hide();
        }
        });
    connect(groupNotify, &RowItem::clicked, this, [=] {
        qDebug("群通知 clicked");
        QVector<QString> vstrNotice = addGroupWidget->getNoticeMsg();
        if (grpNotifyDlg)
        {
            grpNotifyDlg->InitData(vstrNotice);
        }
        if (grpNotifyDlg && grpNotifyDlg->isHidden())
        {
            grpNotifyDlg->show();
        }
        else if (grpNotifyDlg && !grpNotifyDlg->isHidden())
        {
            grpNotifyDlg->hide();
        }
        });

    QHBoxLayout* closeLayout = new QHBoxLayout;
    pLabel = new QLabel(NULL);
    pLabel->setFixedSize(QSize(22, 22));

    // 关闭按钮（右上角）
    closeButton = new QPushButton(this);
    closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
    closeButton->setIconSize(QSize(22, 22));
    closeButton->setFixedSize(QSize(22, 22));
    closeButton->setStyleSheet("background: transparent;");
    closeButton->move(width() - 24, 4);
    closeButton->hide();
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

    closeLayout->addWidget(pLabel);
    closeLayout->addStretch();
    closeLayout->addWidget(closeButton);

    QPushButton* btnAdd = new QPushButton("+");
    btnAdd->setFixedSize(40, 40);
    btnAdd->setStyleSheet("background-color: blue; color: white; font-size: 20px; border-radius: 20px;");

    addGroupWidget = new TACAddGroupWidget1(this, pWs);
    // 连接加入群组成功信号，刷新群组列表
    connect(addGroupWidget, &TACAddGroupWidget1::groupJoined, this, [this](const QString& groupId) {
        qDebug() << "收到加入群组成功信号，刷新群列表，群组ID:" << groupId;
        this->InitData();
        });
    // 连接群组创建成功信号，刷新群组列表并关闭所有群聊窗口
    connect(addGroupWidget, &TACAddGroupWidget1::groupCreated, this, [this](const QString& groupId) {
        qDebug() << "收到群组创建成功信号，刷新群列表，群组ID:" << groupId;
        // 关闭所有已打开的群聊窗口
        for (auto it = m_scheduleDlg.begin(); it != m_scheduleDlg.end(); ++it) {
            if (it.value() && !it.value()->isHidden()) {
                it.value()->hide();
            }
        }
        // 刷新群组列表
        this->InitData();
        });
    // ScheduleDialog 不再自动创建，只在点击按钮时创建（在 makePairBtn 中处理）
    friendNotifyDlg = new FriendNotifyDialog(this);
    grpNotifyDlg = new GroupNotifyDialog(this);
    topLayout->addLayout(notifyLayout, 13);
    topLayout->addStretch();
    topLayout->addWidget(btnAdd, 1);
    //topLayout->addStretch();
    //topLayout->addWidget(closeButton);

    connect(btnAdd, &QPushButton::clicked, this, [=] {
        if (addGroupWidget)
        {
            addGroupWidget->show();
        }
        });

    // 中间切换按钮
    QHBoxLayout* switchLayout = new QHBoxLayout;
    switchLayout->setSpacing(0);
    switchLayout->setContentsMargins(0, 0, 0, 0);
    QPushButton* btnGroup = new QPushButton(QStringLiteral("群聊"));
    QPushButton* btnFriend = new QPushButton(QStringLiteral("好友"));
    btnFriend->setCheckable(true);
    btnGroup->setCheckable(true);

    QButtonGroup* switchGroup = new QButtonGroup(this);
    switchGroup->setExclusive(true);
    switchGroup->addButton(btnFriend);
    switchGroup->addButton(btnGroup);

    const QString leftStyle = QStringLiteral(
        "QPushButton {"
        " background-color: rgb(50,50,50);"
        " color: #f7f7f7;"
        " padding: 8px 28px;"
        " font-size: 16px;"
        " font-weight: bold;"
        " border: none;"
        " border-top-left-radius: 26px;"
        " border-bottom-left-radius: 26px;"
        " border-top-right-radius: 0px;"
        " border-bottom-right-radius: 0px;"
        "}"
        "QPushButton:hover { background-color: rgb(60,60,60); }"
        "QPushButton:checked { background-color: #ffffff; color: #3569ff; }"
    );

    const QString rightStyle = QStringLiteral(
        "QPushButton {"
        " background-color: rgb(50,50,50);"
        " color: #f7f7f7;"
        " padding: 8px 28px;"
        " font-size: 16px;"
        " font-weight: bold;"
        " border: none;"
        " border-top-right-radius: 26px;"
        " border-bottom-right-radius: 26px;"
        " border-top-left-radius: 0px;"
        " border-bottom-left-radius: 0px;"
        "}"
        "QPushButton:hover { background-color: rgb(60,60,60); }"
        "QPushButton:checked { background-color: #ffffff; color: #3569ff; }"
    );

    btnFriend->setStyleSheet(leftStyle);
    btnGroup->setStyleSheet(rightStyle);

    btnFriend->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    btnGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    switchLayout->addWidget(btnFriend);
    switchLayout->addSpacing(1);
    switchLayout->addWidget(btnGroup);

    // 内容切换区域
    QStackedWidget* stack = new QStackedWidget;

    // 好友界面
    QWidget* friendPage = new QWidget;
    fLayout = new QVBoxLayout(friendPage);
    setupFriendTree();

    // 群聊界面
    QWidget* groupPage = new QWidget;
    gLayout = new QVBoxLayout(groupPage);
    gLayout->setContentsMargins(6, 6, 6, 6);
    gLayout->setSpacing(6);
    setupGroupTree();

    stack->addWidget(friendPage);
    stack->addWidget(groupPage);
    stack->setCurrentIndex(0);

    // 点击切换
    connect(btnFriend, &QPushButton::clicked, this, [=] {
        btnFriend->setChecked(true);
        btnGroup->setChecked(false);
        stack->setCurrentIndex(0);
        });
    connect(btnGroup, &QPushButton::clicked, this, [=] {
        btnFriend->setChecked(false);
        btnGroup->setChecked(true);
        stack->setCurrentIndex(1);
        });

    // 默认选中好友
    btnFriend->setChecked(true);

    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(closeLayout);
    mainLayout->addLayout(topLayout);
    mainLayout->addSpacing(5);
    mainLayout->addLayout(switchLayout);
    mainLayout->addWidget(stack);
}

// 帮助函数：生成一行左右两个按钮布局（相同颜色）
// 辅助函数：为 ScheduleDialog 建立群聊退出信号连接
void FriendGroupDialog::connectGroupLeftSignal(ScheduleDialog* scheduleDlg, const QString& groupId)
{
    if (!scheduleDlg) return;
    
    // 连接群聊退出信号，当用户退出群聊时刷新群列表
    connect(scheduleDlg, &ScheduleDialog::groupLeft, this, [this](const QString& leftGroupId) {
        qDebug() << "收到群聊退出信号，刷新群列表，群组ID:" << leftGroupId;
        // 从map中移除已关闭的群聊窗口
        if (m_scheduleDlg.contains(leftGroupId)) {
            m_scheduleDlg[leftGroupId]->deleteLater();
            m_scheduleDlg.remove(leftGroupId);
        }
        // 刷新群列表
        this->InitData();
    }, Qt::UniqueConnection); // 使用 UniqueConnection 避免重复连接
}

// 帮助函数：生成一行两个不同用途的按钮（如头像+昵称）
void FriendGroupDialog::InitData()
{
    /*if (addGroupWidget)
    {
        addGroupWidget->InitData(m_setClassId);
    }*/

    clearFriendTree();
    clearGroupTree();

    // 确保所有已存在的 ScheduleDialog 都建立了群聊退出信号连接
    for (auto it = m_scheduleDlg.begin(); it != m_scheduleDlg.end(); ++it) {
        if (it.value()) {
            connectGroupLeftSignal(it.value(), it.key());
        }
    }

    GetGroupJoinedList();
    if (m_httpHandler)
    {
        UserInfo userInfo = CommonInfo::GetData();
        QString url = "http://47.100.126.194:5000/friends?";
        url += "id_card=";
        url += userInfo.strIdNumber;
        m_httpHandler->get(url);

        // 调用获取群组信息的接口
        QString groupUrl = "http://47.100.126.194:5000/groups/by-teacher?";
        groupUrl += "teacher_unique_id=";
        groupUrl += userInfo.teacher_unique_id;
        m_httpHandler->get(groupUrl);
    }
}

void FriendGroupDialog::openClassGroup(const QString& classId, const QString& groupName)
{
    if (classId.isEmpty()) {
        qDebug() << "班级ID为空，无法打开班级群";
        return;
    }
    
    // 生成班级群ID：班级ID + "01"
    QString groupId = classId + "01";
    
    // 使用班级名称，如果没有提供则使用默认名称
    QString displayName = groupName.isEmpty() ? QString("班级群") : groupName;
    
    // 调用openScheduleForGroup打开群组窗口
    // 参数：groupName, unique_group_id, classid, iGroupOwner, isClassGroup
    // 对于班级端，假设是群主（可以根据实际情况调整）
    openScheduleForGroup(displayName, groupId, classId, true, true);
}

void FriendGroupDialog::GetGroupJoinedList() { // 已加入群列表
    // 获取群列表
    int ret = TIMGroupGetJoinedGroupList([](int32_t code, const char* desc, const char* json_param, const void* user_data) {
        FriendGroupDialog* ths = (FriendGroupDialog*)user_data;
        
        if (strlen(json_param) == 0) {
            return;
        }
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(QByteArray(json_param), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "JSON Parse Error:" << parseError.errorString();
            return;
        }
        if (!jsonDoc.isArray()) {
            qDebug() << "JSON is not an array!";
            return;
        }
        QJsonArray json_group_list = jsonDoc.array();
        
        // 获取当前用户信息
        UserInfo userInfo = CommonInfo::GetData();
        QString userId = userInfo.teacher_unique_id; // 或使用其他用户ID字段
        
        // 构造上传到服务器的JSON数据
        QJsonArray groupsArray;
        
        for (int i = 0; i < json_group_list.size(); i++) {
            QJsonObject group = json_group_list[i].toObject();
            
            // 获取群组基础信息
            QString groupid = group[kTIMGroupBaseInfoGroupId].toString();
            QString groupName = group[kTIMGroupBaseInfoGroupName].toString();
            
            // 构造群组信息对象
            QJsonObject groupObj;
            
            // 群组基础信息
            groupObj["group_id"] = groupid;
            groupObj["group_name"] = groupName.isEmpty() ? group[kTIMGroupDetialInfoGroupName].toString() : groupName;
            groupObj["group_type"] = group[kTIMGroupBaseInfoGroupType].toInt();
            groupObj["face_url"] = group[kTIMGroupBaseInfoFaceUrl].toString();
            groupObj["info_seq"] = group[kTIMGroupBaseInfoInfoSeq].toInt();
            groupObj["latest_seq"] = group[kTIMGroupBaseInfoLastestSeq].toInt();
            groupObj["is_shutup_all"] = group[kTIMGroupBaseInfoIsShutupAll].toBool();
            
            // 群组详细信息
            groupObj["detail_group_id"] = group[kTIMGroupDetialInfoGroupId].toString();
            groupObj["detail_group_name"] = group[kTIMGroupDetialInfoGroupName].toString();
            groupObj["detail_group_type"] = group[kTIMGroupDetialInfoGroupType].toInt();
            groupObj["detail_face_url"] = group[kTIMGroupDetialInfoFaceUrl].toString();
            groupObj["create_time"] = group[kTIMGroupDetialInfoCreateTime].toInt();
            groupObj["detail_info_seq"] = group[kTIMGroupDetialInfoInfoSeq].toInt();
            groupObj["introduction"] = group[kTIMGroupDetialInfoIntroduction].toString();
            groupObj["notification"] = group[kTIMGroupDetialInfoNotification].toString();
            groupObj["last_info_time"] = group[kTIMGroupDetialInfoLastInfoTime].toInt();
            groupObj["last_msg_time"] = group[kTIMGroupDetialInfoLastMsgTime].toInt();
            groupObj["next_msg_seq"] = group[kTIMGroupDetialInfoNextMsgSeq].toInt();
            groupObj["member_num"] = group[kTIMGroupDetialInfoMemberNum].toInt();
            groupObj["max_member_num"] = group[kTIMGroupDetialInfoMaxMemberNum].toInt();
            groupObj["online_member_num"] = group[kTIMGroupDetialInfoOnlineMemberNum].toInt();
            //groupObj["owner_identifier"] = group[kTIMGroupDetialInfoOwnerIdentifier].toString();
            groupObj["add_option"] = group[kTIMGroupDetialInfoAddOption].toInt();
            groupObj["visible"] = group[kTIMGroupDetialInfoVisible].toInt();
            groupObj["searchable"] = group[kTIMGroupDetialInfoSearchable].toInt();
            groupObj["detail_is_shutup_all"] = group[kTIMGroupDetialInfoIsShutupAll].toBool();
            
            // 读取自定义字段（班级ID和学校ID）
            QJsonArray customInfo = group[kTIMGroupDetialInfoCustomInfo].toArray();
            for (int j = 0; j < customInfo.size(); j++) {
                QJsonObject customItem = customInfo[j].toObject();
                QString key = customItem[kTIMGroupInfoCustemStringInfoKey].toString();
                QString value = customItem[kTIMGroupInfoCustemStringInfoValue].toString();
                // 兼容旧的带下划线格式和新的无下划线格式
                if (key == "class_id" || key == "classid") {
                    groupObj["classid"] = value;
                } else if (key == "school_id" || key == "schoolid") {
                    groupObj["schoolid"] = value;
                }
            }
            
            // 用户在该群组中的信息
            QJsonObject selfInfo = group[kTIMGroupBaseInfoSelfInfo].toObject();
            QJsonObject memberInfo;
            memberInfo["user_id"] = userId;
            memberInfo["readed_seq"] = group[kTIMGroupBaseInfoReadedSeq].toInt();
            memberInfo["msg_flag"] = group[kTIMGroupBaseInfoMsgFlag].toInt();
            memberInfo["join_time"] = selfInfo[kTIMGroupSelfInfoJoinTime].toInt();
            memberInfo["self_role"] = selfInfo[kTIMGroupSelfInfoRole].toInt();
            memberInfo["self_msg_flag"] = selfInfo[kTIMGroupSelfInfoMsgFlag].toInt();
            memberInfo["unread_num"] = selfInfo[kTIMGroupSelfInfoUnReadNum].toInt();
            
            groupObj["member_info"] = memberInfo;
            
            groupsArray.append(groupObj);
        }
        
        // 构造最终的上传JSON
        QJsonObject uploadData;
        uploadData["user_id"] = userId;
        uploadData["groups"] = groupsArray;
        
        // 转换为JSON字符串
        QJsonDocument uploadDoc(uploadData);
        QByteArray jsonData = uploadDoc.toJson(QJsonDocument::Compact);
        
        // 上传到服务器
        if (ths->m_httpHandler) {
            QString url = "http://47.100.126.194:5000/groups/sync";
            ths->m_httpHandler->post(url, jsonData);
            qDebug() << "上传群组信息到服务器，共" << groupsArray.size() << "个群组";
        }
        
        //CIMWnd::GetInst().Logf("GroupList", kTIMLog_Info, json_param);
        }, this);
    //Logf("Init", kTIMLog_Info, "TIMGroupGetJoinedGroupList ret %d", ret);
    qDebug() << "TIMGroupGetJoinedGroupList ret:" << ret;
}

void FriendGroupDialog::InitWebSocket()
{
    if (addGroupWidget)
    {
        addGroupWidget->InitWebSocket();
    }

    TaQTWebSocket::regRecvDlg(this);
    if (m_pWs)
    {
        connect(m_pWs, &TaQTWebSocket::newMessage,
            this, &FriendGroupDialog::onWebSocketMessage);
    }
}

void FriendGroupDialog::setTitleName(const QString& name) {
    m_titleName = name;
    update();
}

void FriendGroupDialog::visibleCloseButton(bool val)
{
    m_visibleCloseButton = val;
}

void FriendGroupDialog::setBackgroundColor(const QColor& color) {
    m_backgroundColor = color;
    update();
}

void FriendGroupDialog::setBorderColor(const QColor& color) {
    m_borderColor = color;
    update();
}

void FriendGroupDialog::setBorderWidth(int val) {
    m_borderWidth = val;
    update();
}

void FriendGroupDialog::setRadius(int val) {
    m_radius = val;
    update();
}

void FriendGroupDialog::paintEvent(QPaintEvent* event) {
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

void FriendGroupDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
    }
}

void FriendGroupDialog::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
    }
}

void FriendGroupDialog::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
}

void FriendGroupDialog::leaveEvent(QEvent* event)
{
    QDialog::leaveEvent(event);
    closeButton->hide();
}

void FriendGroupDialog::enterEvent(QEvent* event)
{
    QDialog::enterEvent(event);
    if (m_visibleCloseButton)
        closeButton->show();
}

void FriendGroupDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    //initShow();
    closeButton->move(this->width() - 22, 0);
}

void FriendGroupDialog::onWebSocketMessage(const QString& msg)
{
    // 解析 JSON 文本
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析失败:" << parseError.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "根节点不是 JSON 对象";
        return;
    }

    QJsonObject rootObj = doc.object();

    const QString type = rootObj.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("prepare_class_history")) {
        processPrepareClassHistoryMessage(rootObj);
        return;
    }

    // 提取 data 数组
    if (rootObj.contains("data") && rootObj["data"].isArray()) {
        QJsonArray dataArray = rootObj["data"].toArray();
        for (auto item : dataArray) {
            if (!item.isObject()) continue;
            QJsonObject obj = item.toObject();

            Notification n;
            n.id = obj["id"].toInt();
            n.sender_id = obj["sender_id"].toInt();
            n.sender_name = obj["sender_name"].isNull() ? "" : obj["sender_name"].toString();
            n.receiver_id = obj["receiver_id"].toInt();
            n.unique_group_id = obj["unique_group_id"].isNull() ? "" : obj["unique_group_id"].toString();
            n.group_name = obj["group_name"].isNull() ? "" : obj["group_name"].toString();
            n.content = obj["content"].toString();
            n.content_text = obj["content_text"].toInt();
            n.is_read = obj["is_read"].toInt();
            n.is_agreed = obj["is_agreed"].toInt();
            n.remark = obj["remark"].isNull() ? "" : obj["remark"].toString();
            n.created_at = obj["created_at"].toString();
            n.updated_at = obj["updated_at"].toString();

            notifications.append(n);
        }
    }
}

void FriendGroupDialog::processPrepareClassHistoryMessage(const QJsonObject& rootObj)
{
    if (!rootObj.contains(QStringLiteral("data")) || !rootObj.value(QStringLiteral("data")).isArray()) {
        qWarning() << "prepare_class_history 消息缺少 data 数组";
        return;
    }

    QJsonArray dataArray = rootObj.value(QStringLiteral("data")).toArray();
    if (dataArray.isEmpty()) {
        qDebug() << "prepare_class_history 数据为空";
        return;
    }

    QHash<QString, QJsonArray> groupedByGroupId;
    for (const auto& item : dataArray) {
        if (!item.isObject()) {
            continue;
        }
        QJsonObject obj = item.toObject();
        QString groupId = obj.value(QStringLiteral("group_id")).toString();
        if (groupId.isEmpty()) {
            groupId = obj.value(QStringLiteral("unique_group_id")).toString();
        }
        if (groupId.isEmpty()) {
            continue;
        }
        groupedByGroupId[groupId].append(item);
    }

    for (auto it = groupedByGroupId.constBegin(); it != groupedByGroupId.constEnd(); ++it) {
        const QString& groupId = it.key();
        m_prepareClassHistoryCache[groupId] = it.value();
        if (ScheduleDialog* dlg = m_scheduleDlg.value(groupId, nullptr)) {
            dlg->setPrepareClassHistory(it.value());
        }
    }
}
