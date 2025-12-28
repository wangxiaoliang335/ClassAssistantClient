#include "ScheduleDialog.h"
#include "ChatDialog.h"
#include "FriendGroupDialog.h"
#include <algorithm>
#include <QDateTime>
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
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <cstring>
#include "CommonInfo.h"

// 定义 TempRoomStorage 的静态成员变量
QMap<QString, TempRoomInfo> TempRoomStorage::s_tempRooms;

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

    connect(m_friendTree, &QTreeWidget::itemDoubleClicked, this, &FriendGroupDialog::handleFriendItemActivated);
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
    // 在"好友"标签页下，"班级"节点只显示班级（非班级群）
    // 班级群应该只在"群聊"标签页下显示
    if (isClassGroup) {
        return; // 班级群不添加到好友树的"班级"节点
    }
    
    if (!m_classRootItem || groupId.isEmpty())
        return;
    
    // 更严格的验证：只有通过 fetchClassesByPrefix 获取的班级才应该显示
    // 如果 classid 为空，说明这不是一个真正的班级，不应该添加到"班级"节点
    // 另外，如果 groupId 和 classid 不相等，说明这是群组而不是班级
    if (classid.isEmpty() || classid != groupId) {
        return; // 不是真正的班级，不添加到好友树的"班级"节点
    }
    
    if (m_classItemMap.contains(groupId))
        return;

    QString title = displayName.trimmed().isEmpty() ? QStringLiteral("未命名班级") : displayName.trimmed();
    
    // 额外的验证：确保显示名称不是空的或无效的
    if (title.isEmpty() || title.length() < 2) {
        return; // 名称太短，可能是无效数据
    }
    
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

    connect(m_groupTree, &QTreeWidget::itemDoubleClicked, this, &FriendGroupDialog::handleGroupItemActivated);
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
    //QString badge = classid.trimmed().isEmpty() ? QString() : classid.right(2);
    //QString label = badge.isEmpty() ? title : QStringLiteral("%1   %2").arg(badge, title);
    QString label = title;

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

    // 分流逻辑：
    // - 班级群：isClassGroup=true -> 继续走 ScheduleDialog
    // - 班级（好友树里的班级节点）：classid == groupId -> 继续走 ScheduleDialog
    // - 其余（普通群）：走普通群聊天窗口
    //
    // 备注：普通群当前也可能带 classid，因此不能用“classid 是否为空”来判断普通群。
    const bool isPureClassNode = !classid.trimmed().isEmpty() && (classid == unique_group_id);
    const bool openSchedule = isClassGroup || isPureClassNode;

    if (openSchedule) {
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
            // 传递缓存的作业数据到 ScheduleDialog
            if (m_homeworkCache.contains(unique_group_id)) {
                const QMap<QString, QMap<QString, QString>>& homeworkByDate = m_homeworkCache[unique_group_id];
                for (auto it = homeworkByDate.begin(); it != homeworkByDate.end(); ++it) {
                    const QString& dateStr = it.key();
                    const QMap<QString, QString>& subjectContent = it.value();
                    for (auto subIt = subjectContent.begin(); subIt != subjectContent.end(); ++subIt) {
                        dlg->setHomeworkData(dateStr, subIt.key(), subIt.value());
                    }
                }
            }
            // 传递缓存的通知数据到 ScheduleDialog
            if (m_notificationCache.contains(unique_group_id)) {
                dlg->setNotificationData(m_notificationCache[unique_group_id]);
            }
            connectGroupLeftSignal(dlg, unique_group_id);
            // 连接群组设置更新信号
            connect(dlg, &ScheduleDialog::groupSettingsUpdated, this, [this]() {
                notifyUpdatePrepareClassButton();
            });
            m_scheduleDlg[unique_group_id] = dlg;
        }

        if (dlg->isHidden()) {
            dlg->InitData(groupName, unique_group_id, classid, iGroupOwner, isClassGroup);
            dlg->show();
        } else {
            dlg->hide();
        }
        return;
    }

    // 普通群：打开聊天窗口（圆角弹窗）
    ChatDialog* chatDlg = m_normalGroupChatDlg.value(unique_group_id, nullptr);
    if (!chatDlg) {
        chatDlg = new ChatDialog(this, m_pWs);
        chatDlg->InitWebSocket();
        chatDlg->InitData(unique_group_id, iGroupOwner);
        // 普通群：显式设置上下文（即使当前 classid 可能不为空，也按普通群处理）
        chatDlg->setGroupContext(classid, false);
        connectNormalGroupLeftSignal(chatDlg, unique_group_id);
        m_normalGroupChatDlg[unique_group_id] = chatDlg;
    } else {
        // 重新确保 owner 状态最新
        chatDlg->InitData(unique_group_id, iGroupOwner);
        chatDlg->setGroupContext(classid, false);
    }

    if (!groupName.trimmed().isEmpty()) {
        chatDlg->setWindowTitle(groupName.trimmed());
    }

    if (chatDlg->isHidden()) {
        chatDlg->show();
        chatDlg->raise();
        chatDlg->activateWindow();
    } else {
        chatDlg->hide();
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
    m_networkManager = new QNetworkAccessManager(this);
    if (m_httpHandler)
    {
        connect(m_httpHandler, &TAHttpHandler::success, this, [this](const QString& responseString) {
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
                                // 如果face_url包含阿里云地址，下载头像
                                downloadGroupAvatar(faceUrl, groupId);
                                
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
                            
                            // 解析并保存临时房间信息（如果有）
                            if (groupObj.contains("temp_room") && groupObj["temp_room"].isObject()) {
                                QJsonObject tempRoomObj = groupObj["temp_room"].toObject();
                                TempRoomInfo tempRoomInfo;
                                tempRoomInfo.room_id = tempRoomObj["room_id"].toString();
                                tempRoomInfo.whip_url = tempRoomObj["publish_url"].toString();  // 推流地址
                                tempRoomInfo.whep_url = tempRoomObj["play_url"].toString();     // 拉流地址
                                tempRoomInfo.stream_name = tempRoomObj["stream_name"].toString();
                                tempRoomInfo.group_id = groupId;
                                tempRoomInfo.owner_id = tempRoomObj["owner_id"].toString();
                                tempRoomInfo.owner_name = tempRoomObj["owner_name"].toString();
                                tempRoomInfo.owner_icon = tempRoomObj["owner_icon"].toString();
                                
                                // 保存到全局存储
                                TempRoomStorage::saveTempRoomInfo(groupId, tempRoomInfo);
                                
                                qDebug() << "从服务器获取到临时房间信息，群组ID:" << groupId;
                                qDebug() << "  房间ID:" << tempRoomInfo.room_id;
                                qDebug() << "  推流地址:" << tempRoomInfo.whip_url;
                                qDebug() << "  拉流地址:" << tempRoomInfo.whep_url;
                            }
                            
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
                                // 如果face_url包含阿里云地址，下载头像
                                downloadGroupAvatar(faceUrl, groupId);
                                
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
                            
                            // 解析并保存临时房间信息（如果有）
                            if (groupObj.contains("temp_room") && groupObj["temp_room"].isObject()) {
                                QJsonObject tempRoomObj = groupObj["temp_room"].toObject();
                                TempRoomInfo tempRoomInfo;
                                tempRoomInfo.room_id = tempRoomObj["room_id"].toString();
                                tempRoomInfo.whip_url = tempRoomObj["publish_url"].toString();  // 推流地址
                                tempRoomInfo.whep_url = tempRoomObj["play_url"].toString();     // 拉流地址
                                tempRoomInfo.stream_name = tempRoomObj["stream_name"].toString();
                                tempRoomInfo.group_id = groupId;
                                tempRoomInfo.owner_id = tempRoomObj["owner_id"].toString();
                                tempRoomInfo.owner_name = tempRoomObj["owner_name"].toString();
                                tempRoomInfo.owner_icon = tempRoomObj["owner_icon"].toString();
                                
                                // 保存到全局存储
                                TempRoomStorage::saveTempRoomInfo(groupId, tempRoomInfo);
                                
                                qDebug() << "从服务器获取到临时房间信息，群组ID:" << groupId;
                                qDebug() << "  房间ID:" << tempRoomInfo.room_id;
                                qDebug() << "  推流地址:" << tempRoomInfo.whip_url;
                                qDebug() << "  拉流地址:" << tempRoomInfo.whep_url;
                            }
                            
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
                            
                            // 解析并保存临时房间信息（如果有）
                            if (groupObj.contains("temp_room") && groupObj["temp_room"].isObject()) {
                                QJsonObject tempRoomObj = groupObj["temp_room"].toObject();
                                TempRoomInfo tempRoomInfo;
                                tempRoomInfo.room_id = tempRoomObj["room_id"].toString();
                                tempRoomInfo.whip_url = tempRoomObj["publish_url"].toString();  // 推流地址
                                tempRoomInfo.whep_url = tempRoomObj["play_url"].toString();     // 拉流地址
                                tempRoomInfo.stream_name = tempRoomObj["stream_name"].toString();
                                tempRoomInfo.group_id = unique_group_id;
                                tempRoomInfo.owner_id = tempRoomObj["owner_id"].toString();
                                tempRoomInfo.owner_name = tempRoomObj["owner_name"].toString();
                                tempRoomInfo.owner_icon = tempRoomObj["owner_icon"].toString();
                                
                                // 保存到全局存储
                                TempRoomStorage::saveTempRoomInfo(unique_group_id, tempRoomInfo);
                                
                                qDebug() << "从服务器获取到临时房间信息（groups），群组ID:" << unique_group_id;
                                qDebug() << "  房间ID:" << tempRoomInfo.room_id;
                                qDebug() << "  推流地址:" << tempRoomInfo.whip_url;
                                qDebug() << "  拉流地址:" << tempRoomInfo.whep_url;
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
                            
                            // 解析并保存临时房间信息（如果有）
                            if (groupObj.contains("temp_room") && groupObj["temp_room"].isObject()) {
                                QJsonObject tempRoomObj = groupObj["temp_room"].toObject();
                                TempRoomInfo tempRoomInfo;
                                tempRoomInfo.room_id = tempRoomObj["room_id"].toString();
                                tempRoomInfo.whip_url = tempRoomObj["publish_url"].toString();  // 推流地址
                                tempRoomInfo.whep_url = tempRoomObj["play_url"].toString();     // 拉流地址
                                tempRoomInfo.stream_name = tempRoomObj["stream_name"].toString();
                                tempRoomInfo.group_id = unique_group_id;
                                tempRoomInfo.owner_id = tempRoomObj["owner_id"].toString();
                                tempRoomInfo.owner_name = tempRoomObj["owner_name"].toString();
                                tempRoomInfo.owner_icon = tempRoomObj["owner_icon"].toString();
                                
                                // 保存到全局存储
                                TempRoomStorage::saveTempRoomInfo(unique_group_id, tempRoomInfo);
                                
                                qDebug() << "从服务器获取到临时房间信息（joingroups），群组ID:" << unique_group_id;
                                qDebug() << "  房间ID:" << tempRoomInfo.room_id;
                                qDebug() << "  推流地址:" << tempRoomInfo.whip_url;
                                qDebug() << "  拉流地址:" << tempRoomInfo.whep_url;
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

        connect(m_httpHandler, &TAHttpHandler::failed, this, [this](const QString& errResponseString) {
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

    connect(friendNotify, &RowItem::clicked, this, [this] {
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
    connect(groupNotify, &RowItem::clicked, this, [this] {
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
    closeButton->setText("X");
    closeButton->setFixedSize(26, 26);
    closeButton->setStyleSheet(
        "QPushButton { background-color: #666666; color: white; border: none; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #777777; }"
    );
    closeButton->move(width() - closeButton->width() - 6, 6);
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

    connect(btnAdd, &QPushButton::clicked, this, [this] {
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
    connect(btnFriend, &QPushButton::clicked, this, [this, btnFriend, btnGroup, stack] {
        btnFriend->setChecked(true);
        btnGroup->setChecked(false);
        stack->setCurrentIndex(0);
        });
    connect(btnGroup, &QPushButton::clicked, this, [this, btnFriend, btnGroup, stack] {
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

void FriendGroupDialog::connectNormalGroupLeftSignal(ChatDialog* chatDlg, const QString& groupId)
{
    if (!chatDlg) return;

    // 普通群：退出/解散后刷新群列表并清理缓存窗口
    connect(chatDlg, &ChatDialog::normalGroupLeft, this, [this](const QString& leftGroupId) {
        qDebug() << "收到普通群退出信号，刷新群列表，群组ID:" << leftGroupId;
        if (m_normalGroupChatDlg.contains(leftGroupId)) {
            m_normalGroupChatDlg[leftGroupId]->deleteLater();
            m_normalGroupChatDlg.remove(leftGroupId);
        }
        this->InitData();
    }, Qt::UniqueConnection);

    connect(chatDlg, &ChatDialog::normalGroupDismissed, this, [this](const QString& dismissedGroupId) {
        qDebug() << "收到普通群解散信号，刷新群列表，群组ID:" << dismissedGroupId;
        if (m_normalGroupChatDlg.contains(dismissedGroupId)) {
            m_normalGroupChatDlg[dismissedGroupId]->deleteLater();
            m_normalGroupChatDlg.remove(dismissedGroupId);
        }
        this->InitData();
    }, Qt::UniqueConnection);
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

    GetGroupJoinedList(); //客户端不用从腾讯服务器获取群列表了

    // 班级端：登录成功后立即创建 ScheduleDialog 并调用接口获取群组信息
    ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
    if (loginInfo.isLoggedIn() && !loginInfo.class_id.isEmpty()) {
        // 确保 ScheduleDialog 已创建
        ensureScheduleDialogCreated(loginInfo.class_id);
        
        // 调用接口获取群组信息（/groups/members）
        if (m_httpHandler) {
            QString groupId = loginInfo.class_id + "01";
            QUrl url("http://47.100.126.194:5000/groups/members");
            QUrlQuery query;
            query.addQueryItem("group_id", groupId);
            url.setQuery(query);
            m_httpHandler->get(url.toString());
            qDebug() << "登录成功后立即获取群组信息:" << url.toString();
        }
    }

    if (m_httpHandler)
    {
        UserInfo userInfo = CommonInfo::GetData();
        QString url = "http://47.100.126.194:5000/friends?";
        url += "id_card=";
        url += userInfo.strIdNumber;
        m_httpHandler->get(url);

        // 调用获取群组信息的接口
        //QString groupUrl = "http://47.100.126.194:5000/groups/by-teacher?";
        //groupUrl += "teacher_unique_id=";
        //groupUrl += userInfo.teacher_unique_id;
        //m_httpHandler->get(groupUrl);
        
        // 获取班级列表并显示在"好友"标签页下的"班级"节点中
        if (!userInfo.schoolId.isEmpty()) {
            fetchClassesByPrefix(userInfo.schoolId);
        }
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

        // 收集群组ID，用于查询临时语音房间
        QSet<QString> groupIdSet;
        
        // 获取当前用户信息
        UserInfo userInfo = CommonInfo::GetData();
        QString userId = userInfo.classId; // 或使用其他用户ID字段
        
        // 构造上传到服务器的JSON数据
        QJsonArray groupsArray;
        
        auto normalizeGroupType = [](const QJsonValue& v) -> QString {
            // SDK callback examples return string values: "Public"/"Private"/"ChatRoom"/"AVChatRoom"/...
            // But we also defensively handle numeric enum values.
            if (v.isString()) {
                return v.toString().trimmed();
            }
            if (v.isDouble()) {
                const int t = v.toInt();
                switch (t) {
                case kTIMGroup_Public:    return QStringLiteral("Public");
                case kTIMGroup_Private:   return QStringLiteral("Private");
                case kTIMGroup_ChatRoom:  return QStringLiteral("ChatRoom");
                case kTIMGroup_BChatRoom: return QStringLiteral("BChatRoom");
                case kTIMGroup_AVChatRoom:return QStringLiteral("AVChatRoom");
                default:                  return QString::number(t);
                }
            }
            return QString();
        };

        // 统一规则：腾讯 IM 群类型 groupType == "Public" 视为普通群，其它类型一律视为班级群
        // （后端 classid / 文案兜底不再参与判定，避免误判）

        for (int i = 0; i < json_group_list.size(); i++) {
            QJsonObject group = json_group_list[i].toObject();
            
            // 获取群组基础信息
            QString groupid = group[kTIMGroupBaseInfoGroupId].toString();
            QString groupName = group[kTIMGroupBaseInfoGroupName].toString();
            if (groupName.isEmpty()) {
                groupName = group[kTIMGroupDetialInfoGroupName].toString();
            }
            
            // 读取自定义字段（班级ID）
            QString classid;
            QJsonArray customInfo = group[kTIMGroupDetialInfoCustomInfo].toArray();
            for (int j = 0; j < customInfo.size(); j++) {
                QJsonObject customItem = customInfo[j].toObject();
                QString key = customItem[kTIMGroupInfoCustemStringInfoKey].toString();
                QString value = customItem[kTIMGroupInfoCustemStringInfoValue].toString();
                // 兼容旧的带下划线格式和新的无下划线格式
                if (key == "class_id" || key == "classid") {
                    classid = value;
                    break;
                }
            }
            
            // 群组类型：根据腾讯群类型区分班级群 / 普通群
            const QString groupType = normalizeGroupType(group.value(kTIMGroupBaseInfoGroupType));

            // 判断是否是群主（通过 self_role 判断）
            // - 400 或 "Owner" -> "Owner" (群主)
            // - 300 或 "Admin" -> "Admin" (管理员)
            // - 其他 -> "Member" (普通成员)
            QJsonObject selfInfo = group[kTIMGroupBaseInfoSelfInfo].toObject();
            QJsonValue roleValue = selfInfo[kTIMGroupSelfInfoRole];
            bool isGroupOwner = false;
            
            // 支持整数和字符串两种格式
            if (roleValue.isDouble()) {
                int selfRole = roleValue.toInt();
                isGroupOwner = (selfRole == 400); // 400 表示群主
            } else if (roleValue.isString()) {
                QString roleStr = roleValue.toString();
                isGroupOwner = (roleStr == "Owner" || roleStr == "400");
            }
            
            // 获取群组的 Introduction 和 Notification 字段
            QString introduction = group[kTIMGroupDetialInfoIntroduction].toString();
            QString notification = group[kTIMGroupDetialInfoNotification].toString();
            
            // 判断是否是班级群：按统一规则，仅 Public 为普通群，其它类型都认为是班级群
            const bool isClassGroup = (groupType.compare(QStringLiteral("Public"), Qt::CaseInsensitive) != 0);
            
            // 仅班级群：如果班级ID为空，则将群组ID去掉末尾的两位作为班级ID（兜底）
            if (isClassGroup && classid.isEmpty() && !groupid.isEmpty() && groupid.length() >= 2) {
                classid = groupid.left(groupid.length() - 2);
            }
            
            // 直接刷新到界面
            ths->addClassNode(groupName, groupid, classid, isGroupOwner, isClassGroup);
            ths->addGroupTreeNode(groupName, groupid, classid, isGroupOwner, isClassGroup);
            
            if (!classid.isEmpty()) {
                ths->m_setClassId.insert(classid);
            }

            // 记录群组ID
            groupIdSet.insert(groupid.trimmed());
            
            // 构造群组信息对象（用于后续可能的同步，暂时保留但不使用）
            QJsonObject groupObj;
            
            // 群组基础信息
            groupObj["group_id"] = groupid;
            groupObj["group_name"] = groupName;
            groupObj["group_type"] = groupType;
            
            // 优先使用详细信息中的FaceUrl，如果没有则使用基础信息中的FaceUrl
            QString detailFaceUrl = group[kTIMGroupDetialInfoFaceUrl].toString();
            QString baseFaceUrl = group[kTIMGroupBaseInfoFaceUrl].toString();
            QString faceUrl = !detailFaceUrl.isEmpty() ? detailFaceUrl : baseFaceUrl;
            groupObj["face_url"] = faceUrl;
            groupObj["detail_face_url"] = detailFaceUrl;
            
            // 如果face_url不为空，下载头像并显示到班级群界面
            if (!faceUrl.isEmpty()) {
                ths->downloadGroupAvatar(faceUrl, groupid);
            }
            
            groupObj["info_seq"] = group[kTIMGroupBaseInfoInfoSeq].toInt();
            groupObj["latest_seq"] = group[kTIMGroupBaseInfoLastestSeq].toInt();
            groupObj["is_shutup_all"] = group[kTIMGroupBaseInfoIsShutupAll].toBool();
            
            // 群组详细信息
            groupObj["detail_group_id"] = group[kTIMGroupDetialInfoGroupId].toString();
            groupObj["detail_group_name"] = group[kTIMGroupDetialInfoGroupName].toString();
            groupObj["detail_group_type"] = normalizeGroupType(group.value(kTIMGroupDetialInfoGroupType));
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
            groupObj["classid"] = classid;
            
            // 用户在该群组中的信息
            QJsonObject memberInfo;
            memberInfo["user_id"] = userId;
            memberInfo["user_name"] = userInfo.strName;
            memberInfo["readed_seq"] = group[kTIMGroupBaseInfoReadedSeq].toInt();
            memberInfo["msg_flag"] = group[kTIMGroupBaseInfoMsgFlag].toInt();
            memberInfo["join_time"] = selfInfo[kTIMGroupSelfInfoJoinTime].toInt();
            memberInfo["self_role"] = selfInfo[kTIMGroupSelfInfoRole].toInt();
            memberInfo["self_msg_flag"] = selfInfo[kTIMGroupSelfInfoMsgFlag].toInt();
            memberInfo["unread_num"] = selfInfo[kTIMGroupSelfInfoUnReadNum].toInt();
            
            groupObj["member_info"] = memberInfo;
            
            groupsArray.append(groupObj);
        }
        
        qDebug() << "从腾讯服务器获取群组列表成功，共" << groupsArray.size() << "个群组，已直接刷新到界面";

        // 调用临时语音房间查询接口 /temp_rooms/query
        if (!groupIdSet.isEmpty()) {
            QJsonArray groupIdArray;
            for (const QString& gid : groupIdSet) {
                QString trimmed = gid.trimmed();
                if (!trimmed.isEmpty()) {
                    groupIdArray.append(trimmed);
                }
            }

            if (!groupIdArray.isEmpty()) {
                QJsonObject payload;
                payload["group_ids"] = groupIdArray;
                QByteArray postData = QJsonDocument(payload).toJson(QJsonDocument::Compact);

                QNetworkAccessManager* mgr = new QNetworkAccessManager(ths);
                QNetworkRequest req(QUrl("http://47.100.126.194:5000/temp_rooms/query"));
                req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                QNetworkReply* r = mgr->post(req, postData);

                // 使用 ths 作为接收者，确保信号能正确触发
                QObject::connect(r, &QNetworkReply::finished, ths, [mgr, r, ths]() {
                    auto cleanup = [mgr, r]() {
                        if (r) r->deleteLater();
                        if (mgr) mgr->deleteLater();
                    };

                    if (!r) { cleanup(); return; }
                    if (r->error() != QNetworkReply::NoError) {
                        qWarning() << "查询临时语音房间失败:" << r->errorString();
                        cleanup();
                        return;
                    }

                    QByteArray resp = r->readAll();
                    QJsonParseError perr;
                    QJsonDocument doc = QJsonDocument::fromJson(resp, &perr);
                    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
                        qWarning() << "解析临时语音房间响应失败:" << perr.errorString();
                        cleanup();
                        return;
                    }

                    QJsonObject obj = doc.object();
                    if (obj["code"].toInt() != 200) {
                        qWarning() << "查询临时语音房间返回非200:" << resp;
                        cleanup();
                        return;
                    }

                    if (obj.contains("data") && obj["data"].isObject()) {
                        QJsonObject dataObj = obj["data"].toObject();
                        if (dataObj.contains("rooms") && dataObj["rooms"].isArray()) {
                            QJsonArray rooms = dataObj["rooms"].toArray();
                            for (const QJsonValue& v : rooms) {
                                if (!v.isObject()) continue;
                                QJsonObject roomObj = v.toObject();
                                TempRoomInfo info;
                                info.group_id = roomObj["group_id"].toString();
                                info.room_id = roomObj["room_id"].toString();
                                info.whip_url = roomObj["publish_url"].toString();
                                info.whep_url = roomObj["play_url"].toString();
                                info.stream_name = roomObj["stream_name"].toString();
                                info.owner_id = roomObj["owner_id"].toString();
                                info.owner_name = roomObj["owner_name"].toString();
                                info.owner_icon = roomObj["owner_icon"].toString();
                                if (!info.group_id.isEmpty()) {
                                    TempRoomStorage::saveTempRoomInfo(info.group_id, info);
                                    qDebug() << "已缓存语音房间:" << info.group_id << info.room_id;
                                }
                            }
                        }
                    }

                    cleanup();
                });
            } else {
                qWarning() << "group_ids 为空，跳过 temp_rooms/query 调用";
            }
        } else {
            qWarning() << "未收集到群组ID，跳过 temp_rooms/query 调用";
        }
        
        // 初始化 addGroupWidget（如果有群组数据）
        if (ths->addGroupWidget /*&& !ths->m_setClassId.isEmpty()*/) {
            ths->addGroupWidget->InitData(ths->m_setClassId);
        }
        
        // 注释掉：同步到服务器的代码（暂时不需要）
        /*
        // 构造最终的上传JSON
        QJsonObject uploadData;
        uploadData["user_id"] = userId;
        uploadData["groups"] = groupsArray;
        
        // 转换为JSON字符串
        QJsonDocument uploadDoc(uploadData);
        QByteArray jsonData = uploadDoc.toJson(QJsonDocument::Compact);
        
        const char* strJsonT = jsonData.toStdString().c_str();

        // 上传到服务器
        if (ths->m_httpHandler) {
            QString url = "http://47.100.126.194:5000/groups/sync";
            ths->m_httpHandler->post(url, jsonData);
            qDebug() << "上传群组信息到服务器，共" << groupsArray.size() << "个群组";
        }
        */
        
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
        
        // 连接家庭作业消息信号（先于 newMessage 信号发射）
        connect(m_pWs, &TaQTWebSocket::homeworkReceived, this, [this](const QJsonObject& homeworkData) {
            // 响应消息：包含 status 字段
            if (homeworkData.contains(QStringLiteral("status"))) {
                const QString status = homeworkData.value(QStringLiteral("status")).toString();
                const QString message = homeworkData.value(QStringLiteral("message")).toString();
                qDebug() << "作业发布响应:" << status << message;
                return;
            }

            const QString groupId = homeworkData.value(QStringLiteral("group_id")).toString();
            if (groupId.isEmpty()) {
                return;
            }

            const QString dateStr = homeworkData.value(QStringLiteral("date")).toString();
            const QString subject = homeworkData.value(QStringLiteral("subject")).toString().trimmed();
            const QString content = homeworkData.value(QStringLiteral("content")).toString().trimmed();
            if (dateStr.isEmpty() || subject.isEmpty() || content.isEmpty()) {
                return;
            }

            // 缓存作业数据：按群组ID和日期聚合
            m_homeworkCache[groupId][dateStr][subject] = content;
            qDebug() << "FriendGroupDialog: 收到作业消息，已缓存到群组:" << groupId << "日期:" << dateStr << "科目:" << subject;
            
            // 如果对应的 ScheduleDialog 已创建，更新其缓存
            if (m_scheduleDlg.contains(groupId)) {
                ScheduleDialog* dlg = m_scheduleDlg[groupId];
                if (dlg) {
                    dlg->setHomeworkData(dateStr, subject, content);
                }
            }
        });
        
        // 连接通知消息信号（先于 newMessage 信号发射）
        connect(m_pWs, &TaQTWebSocket::notificationReceived, this, [this](const QJsonObject& notificationData) {
            QString type = notificationData.value("type").toString();
            
            // 处理未读通知列表（班级端登录时接收）
            if (type == "unread_notifications") {
                if (notificationData.contains("data") && notificationData.value("data").isArray()) {
                    QJsonArray notifications = notificationData.value("data").toArray();
                    
                    // 按群组ID分组缓存通知
                    for (const QJsonValue& value : notifications) {
                        if (!value.isObject()) continue;
                        
                        QJsonObject notifObj = value.toObject();
                        QString groupId = notifObj.value("unique_group_id").toString();
                        QString receiverId = notifObj.value("receiver_id").toString();
                        
                        if (groupId.isEmpty()) {
                            continue;
                        }
                        
                        QString content = notifObj.value("content").toString().trimmed();
                        QString senderName = notifObj.value("sender_name").toString();
                        QString createdAt = notifObj.value("created_at").toString();
                        
                        if (content.isEmpty()) {
                            continue;
                        }
                        
                        // 创建通知项
                        NotificationItem item;
                        item.content = content;
                        item.senderName = senderName.isEmpty() ? "系统" : senderName;
                        
                        // 解析时间戳
                        if (!createdAt.isEmpty()) {
                            item.timestamp = QDateTime::fromString(createdAt, "yyyy-MM-dd HH:mm:ss");
                        } else {
                            item.timestamp = QDateTime::currentDateTime();
                        }
                        
                        item.avatarUrl = QString(); // 暂时为空
                        
                        // 添加到对应群组的缓存
                        m_notificationCache[groupId].append(item);
                    }
                    
                    // 对每个群组的通知按时间倒序排序（最新的在前面）
                    for (auto& notifications : m_notificationCache) {
                        std::sort(notifications.begin(), notifications.end(),
                            [](const NotificationItem& a, const NotificationItem& b) {
                                return a.timestamp > b.timestamp;
                            });
                        
                        // 最多保存100条
                        if (notifications.size() > 100) {
                            notifications = notifications.mid(0, 100);
                        }
                    }
                    
                    qDebug() << "FriendGroupDialog: 收到未读通知列表，已缓存到" << m_notificationCache.size() << "个群组";
                    
                    // 如果对应的 ScheduleDialog 已创建，更新其缓存
                    for (auto it = m_notificationCache.begin(); it != m_notificationCache.end(); ++it) {
                        QString groupId = it.key();
                        if (m_scheduleDlg.contains(groupId)) {
                            ScheduleDialog* dlg = m_scheduleDlg[groupId];
                            if (dlg) {
                                dlg->setNotificationData(it.value());
                            }
                        }
                    }
                }
            }
            // 处理单个通知消息（班级端接收）
            else if (type == "notification") {
                // 检查是否是响应消息（包含 status 字段）
                if (notificationData.contains("status")) {
                    // 这是发送通知的响应，不需要处理
                    return;
                }
                
                QString groupId = notificationData.value("group_id").toString();
                if (groupId.isEmpty()) {
                    groupId = notificationData.value("unique_group_id").toString();
                }
                
                if (groupId.isEmpty()) {
                    return;
                }
                
                QString content = notificationData.value("content").toString().trimmed();
                QString senderName = notificationData.value("sender_name").toString();
                
                if (content.isEmpty()) {
                    return;
                }
                
                // 创建通知项
                NotificationItem item;
                item.content = content;
                item.senderName = senderName.isEmpty() ? "系统" : senderName;
                item.timestamp = QDateTime::currentDateTime();
                item.avatarUrl = QString();
                
                // 添加到对应群组的缓存（最新的在前面）
                m_notificationCache[groupId].prepend(item);
                
                // 最多保存100条
                if (m_notificationCache[groupId].size() > 100) {
                    m_notificationCache[groupId].removeLast();
                }
                
                qDebug() << "FriendGroupDialog: 收到通知消息，已缓存到群组:" << groupId;
                
                // 如果对应的 ScheduleDialog 已创建，更新其缓存
                if (m_scheduleDlg.contains(groupId)) {
                    ScheduleDialog* dlg = m_scheduleDlg[groupId];
                    if (dlg) {
                        dlg->addNotification(content, senderName);
                    }
                }
            }
        });

        // 连接课前准备历史消息信号（登录后可能下发）
        connect(m_pWs, &TaQTWebSocket::prepareClassHistoryReceived, this, [this](const QJsonObject& prepareData) {
            processPrepareClassHistoryMessage(prepareData);
        });
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
    closeButton->move(this->width() - closeButton->width() - 6, 6);
}

ScheduleDialog* FriendGroupDialog::getScheduleDialog(const QString& classId) const
{
    // m_scheduleDlg 使用 unique_group_id (classId + "01") 作为key
    QString groupId = classId + "01";
    return m_scheduleDlg.value(groupId, nullptr);
}

void FriendGroupDialog::ensureScheduleDialogCreated(const QString& classId)
{
    if (classId.isEmpty()) {
        return;
    }
    
    // 生成班级群ID：班级ID + "01"
    QString groupId = classId + "01";
    
    // 如果已经存在，直接返回
    if (m_scheduleDlg.contains(groupId) && m_scheduleDlg[groupId]) {
        return;
    }
    
    // 直接创建ScheduleDialog，不显示（复制openScheduleForGroup中的创建逻辑，但不调用show）
    ScheduleDialog* dlg = new ScheduleDialog(classId, this, m_pWs);
    dlg->InitWebSocket();
    QList<Notification> curNotification;
    for (const auto& iter : notifications) {
        if (iter.unique_group_id == groupId) {
            curNotification.append(iter);
        }
    }
    dlg->setNoticeMsg(curNotification);
    if (m_prepareClassHistoryCache.contains(groupId)) {
        dlg->setPrepareClassHistory(m_prepareClassHistoryCache.value(groupId));
    }
    // 传递缓存的作业数据到 ScheduleDialog
    if (m_homeworkCache.contains(groupId)) {
        const QMap<QString, QMap<QString, QString>>& homeworkByDate = m_homeworkCache[groupId];
        for (auto it = homeworkByDate.begin(); it != homeworkByDate.end(); ++it) {
            const QString& dateStr = it.key();
            const QMap<QString, QString>& subjectContent = it.value();
            for (auto subIt = subjectContent.begin(); subIt != subjectContent.end(); ++subIt) {
                dlg->setHomeworkData(dateStr, subIt.key(), subIt.value());
            }
        }
    }
    // 传递缓存的通知数据到 ScheduleDialog
    if (m_notificationCache.contains(groupId)) {
        dlg->setNotificationData(m_notificationCache[groupId]);
    }
    connectGroupLeftSignal(dlg, groupId);
    // 连接群组设置更新信号
    connect(dlg, &ScheduleDialog::groupSettingsUpdated, this, [this]() {
        notifyUpdatePrepareClassButton();
    });
    m_scheduleDlg[groupId] = dlg;
    
    // 初始化数据，但不显示窗口
    dlg->InitData(QString("班级群"), groupId, classId, true, true);
    dlg->hide(); // 确保窗口是隐藏的
}

// 通知 TACMainDialog 更新课前准备按钮状态
void FriendGroupDialog::notifyUpdatePrepareClassButton()
{
    // 通过 parentWidget() 查找 TACMainDialog
    QWidget* parentWidget = this->parentWidget();
    qDebug() << "FriendGroupDialog::notifyUpdatePrepareClassButton() called, parentWidget:" << parentWidget;
    while (parentWidget) {
        QString className = parentWidget->metaObject()->className();
        qDebug() << "Checking parent widget class:" << className;
        // 使用类名匹配
        if (className == QString("TACMainDialog")) {
            qDebug() << "Found TACMainDialog, calling updatePrepareClassButtonVisibility";
            // 使用 QMetaObject 调用方法
            bool result = QMetaObject::invokeMethod(parentWidget, "updatePrepareClassButtonVisibility", Qt::QueuedConnection);
            qDebug() << "QMetaObject::invokeMethod result:" << result;
            if (!result) {
                qWarning() << "Failed to invoke updatePrepareClassButtonVisibility method";
            }
            break;
        }
        parentWidget = parentWidget->parentWidget();
    }
    if (!parentWidget) {
        qWarning() << "TACMainDialog not found in parent widget chain";
    }
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
        // prepare_class_history 已通过 TaQTWebSocket::prepareClassHistoryReceived 专门信号处理
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

void FriendGroupDialog::fetchClassesByPrefix(const QString& schoolId)
{
    if (schoolId.isEmpty()) return;

    QString prefix = schoolId;
    if (prefix.length() != 6 || !prefix.toInt()) {
        qWarning() << "学校ID格式错误，应为6位数字:" << prefix;
        return;
    }

    QJsonObject jsonObj;
    jsonObj["prefix"] = prefix;
    QJsonDocument doc(jsonObj);
    QByteArray reqData = doc.toJson(QJsonDocument::Compact);

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl("http://47.100.126.194:5000/getClassesByPrefix"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = manager->post(request, reqData);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response_data = reply->readAll();
            QJsonDocument respDoc = QJsonDocument::fromJson(response_data);
            if (respDoc.isObject()) {
                QJsonObject root = respDoc.object();
                QJsonObject dataObj = root.value("data").toObject();
                int code = dataObj.value("code").toInt();
                QString message = dataObj.value("message").toString();
                if (code != 200) {
                    qWarning() << "获取班级列表失败:" << message;
                    reply->deleteLater();
                    manager->deleteLater();
                    return;
                }
                QJsonArray classes = dataObj.value("classes").toArray();
                
                // 将班级列表添加到好友树的"班级"节点中
                for (const QJsonValue& val : classes) {
                    QJsonObject cls = val.toObject();
                    QString stage = cls.value("school_stage").toString();
                    QString grade = cls.value("grade").toString();
                    QString className = cls.value("class_name").toString();
                    QString classCode = cls.value("class_code").toString();
                    
                    // 验证班级代码不为空
                    if (classCode.isEmpty()) {
                        continue;
                    }
                    
                    // 只显示已经加入的班级（在 m_setClassId 中的），跳过未加入的班级
                    if (m_setClassId.find(classCode) == m_setClassId.end()) {
                        continue; // 未加入的班级不显示
                    }

                    // 展示名称：学段+年级+班名 或 仅班名
                    QString display = className.isEmpty() ? (grade.isEmpty() ? stage : (stage + grade)) : (stage + grade + className);
                    
                    // 验证显示名称不为空且长度合理
                    if (display.trimmed().isEmpty() || display.trimmed().length() < 2) {
                        continue;
                    }
                    
                    // 添加到好友树的"班级"节点中（isClassGroup = false，因为这是班级，不是班级群）
                    addClassNode(display, classCode, classCode, false, false);
                }
            }
        } else {
            qWarning() << "网络错误:" << reply->errorString();
        }
        reply->deleteLater();
        manager->deleteLater();
    });
}

void FriendGroupDialog::downloadGroupAvatar(const QString& faceUrl, const QString& groupId)
{
    if (faceUrl.isEmpty() || groupId.isEmpty()) {
        return;
    }
    
    // 检查是否是阿里云OSS地址
    if (!faceUrl.contains("oss-cn-beijing.aliyuncs.com") && 
        !faceUrl.contains("aliyuncs.com")) {
        // 不是阿里云地址，跳过
        return;
    }
    
    // 检查是否已经下载过（文件已存在）
    QString fileName = faceUrl.section('/', -1);
    QString saveDir = QCoreApplication::applicationDirPath() + "/group_images/" + groupId;
    QDir().mkpath(saveDir);
    QString localPath = saveDir + "/" + fileName;
    
    // 如果文件已存在，直接更新界面显示
    if (QFile::exists(localPath)) {
        // 如果ScheduleDialog已打开，更新其头像显示
        if (m_scheduleDlg.contains(groupId)) {
            ScheduleDialog* dlg = m_scheduleDlg[groupId];
            if (dlg) {
                dlg->updateAvatarDisplay(localPath);
            }
        }
        return;
    }
    
    // 下载图片
    if (!m_networkManager) {
        m_networkManager = new QNetworkAccessManager(this);
    }
    
    QUrl url(faceUrl);
    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, groupId, localPath, faceUrl]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "下载群组头像失败:" << reply->errorString() << "URL:" << faceUrl;
            reply->deleteLater();
            return;
        }
        
        // 读取图片数据
        QByteArray imageData = reply->readAll();
        reply->deleteLater();
        
        if (imageData.isEmpty()) {
            qWarning() << "下载的群组头像数据为空，URL:" << faceUrl;
            return;
        }
        
        // 保存到本地文件
        QFile file(localPath);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "无法创建群组头像文件:" << localPath;
            return;
        }
        
        file.write(imageData);
        file.close();
        
        qDebug() << "群组头像下载成功，保存到:" << localPath;
        
        // 如果ScheduleDialog已打开，更新其头像显示
        if (m_scheduleDlg.contains(groupId)) {
            ScheduleDialog* dlg = m_scheduleDlg[groupId];
            if (dlg) {
                dlg->updateAvatarDisplay(localPath);
            }
        }
    });
}
