#include "ClassTeacherDelDialog.h"
#include "QGroupInfo.h"
#include "ClassTeacherDialog.h"
#include "FriendSelectDialog.h"
#include "MemberKickDialog.h"
#include <QPainterPath>
#include <QPropertyAnimation>

// 解散群聊回调数据结构
struct DismissGroupCallbackData {
    QGroupInfo* dlg;
    QString groupId;
    QString userId;
    QString userName;
};

// ToggleSwitch 实现
ToggleSwitch::ToggleSwitch(QWidget* parent)
    : QWidget(parent)
    , m_checked(false)
    , m_onColor(QColor(76, 175, 80))  // 绿色（开启时）
    , m_offColor(QColor(158, 158, 158))  // 灰色（关闭时）
    , m_thumbColor(QColor(255, 255, 255))  // 白色滑块
    , m_thumbRadius(10)
    , m_trackHeight(20)
{
    setFixedSize(50, 24);  // 设置固定大小
    updateThumbPosition();
}

void ToggleSwitch::setChecked(bool checked)
{
    if (m_checked != checked) {
        m_checked = checked;
        updateThumbPosition();
        update();
        emit toggled(checked);
    }
}

void ToggleSwitch::updateThumbPosition()
{
    int trackWidth = width() - 4;  // 减去左右边距
    int maxX = trackWidth - m_thumbRadius * 2;
    if (m_checked) {
        m_thumbPosition = QPoint(maxX + 2, 2);  // 右边（开启）
    } else {
        m_thumbPosition = QPoint(2, 2);  // 左边（关闭）
    }
}

void ToggleSwitch::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制轨道（背景）
    QRect trackRect(2, (height() - m_trackHeight) / 2, width() - 4, m_trackHeight);
    QColor trackColor = m_checked ? m_onColor : m_offColor;
    painter.setBrush(trackColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(trackRect, m_trackHeight / 2, m_trackHeight / 2);
    
    // 绘制滑块
    QRect thumbRect(m_thumbPosition.x(), m_thumbPosition.y(), m_thumbRadius * 2, m_thumbRadius * 2);
    painter.setBrush(m_thumbColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(thumbRect);
}

void ToggleSwitch::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        setChecked(!m_checked);
    }
    QWidget::mousePressEvent(event);
}

void ToggleSwitch::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateThumbPosition();
}

// SettingRow 实现
SettingRow::SettingRow(const QString& labelText, QWidget* parent)
    : QWidget(parent)
    , m_highlighted(false)
{
    setFixedHeight(40);
    
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // 蓝色标签区域
    m_label = new QLabel(labelText, this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setStyleSheet("background-color: #4169E1; color: white; font-size: 14px; font-weight: bold; padding: 8px;");
    m_label->setFixedWidth(120);  // 固定宽度
    
    // 白色区域（包含Toggle Switch）
    QWidget* toggleContainer = new QWidget(this);
    toggleContainer->setStyleSheet("background-color: white;");
    QHBoxLayout* toggleLayout = new QHBoxLayout(toggleContainer);
    toggleLayout->setContentsMargins(10, 0, 10, 0);
    toggleLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    m_toggle = new ToggleSwitch(toggleContainer);
    toggleLayout->addWidget(m_toggle);
    
    layout->addWidget(m_label);
    layout->addWidget(toggleContainer, 1);  // 白色区域占据剩余空间
    
    connect(m_toggle, &ToggleSwitch::toggled, this, &SettingRow::toggled);
}

void SettingRow::setHighlighted(bool highlighted)
{
    if (m_highlighted != highlighted) {
        m_highlighted = highlighted;
        update();
    }
}

void SettingRow::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    // 如果高亮，绘制红色边框
    if (m_highlighted) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(255, 0, 0), 2));  // 红色边框，2像素宽
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}

QGroupInfo::QGroupInfo(QWidget* parent)
	: QDialog(parent)
{
    // 初始化时不显示，由外部调用 show() 时才显示
    //setVisible(false);
    
    // 初始化成员变量
    m_circlePlus = nullptr;
    m_circleMinus = nullptr;
    m_btnDismiss = nullptr;
    m_btnExit = nullptr;
}

void QGroupInfo::initData(QString groupName, QString groupNumberId, QString classid)
{
    setWindowTitle("班级管理");
    // 设置无边框窗口
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    resize(300, 600);
    setStyleSheet("QDialog { background-color: #5C5C5C; color: white; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QPushButton { font-size:14px; color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QLabel { font-size:14px; color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QLineEdit { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QTextEdit { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QGroupBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QTableWidget { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; gridline-color: #5C5C5C; font-weight: bold; } "
        "QTableWidget::item { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QComboBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QCheckBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QRadioButton { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QScrollArea { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QListWidget { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QSpinBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QProgressBar { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QSlider { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; }");
    
    // 关闭按钮（右上角）
    m_closeButton = new QPushButton(this);
    m_closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
    m_closeButton->setIconSize(QSize(22, 22));
    m_closeButton->setFixedSize(QSize(22, 22));
    m_closeButton->setStyleSheet("background: transparent;");
    m_closeButton->move(width() - 22, 0);
    m_closeButton->hide();
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);

    m_groupName = groupName;
    m_groupNumberId = groupNumberId;

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 初始化REST API
    m_restAPI = new TIMRestAPI(this);
    // 注意：管理员账号信息在使用REST API时再设置，因为此时用户可能还未登录
    
    m_friendSelectDlg = new FriendSelectDialog(this);
    // 连接成员邀请成功信号，刷新成员列表
    if (m_friendSelectDlg) {
        connect(m_friendSelectDlg, &FriendSelectDialog::membersInvitedSuccess, this, [this](const QString& groupId) {
            // 刷新成员列表（通知父窗口刷新）
            refreshMemberList(groupId);
        });
    }
    m_memberKickDlg = new MemberKickDialog(this);
    // 连接成员踢出成功信号，刷新成员列表
    if (m_memberKickDlg) {
        connect(m_memberKickDlg, &MemberKickDialog::membersKickedSuccess, this, [this](const QString& groupId) {
            // 刷新成员列表（通知父窗口刷新）
            refreshMemberList(groupId);
        });
    }
    //m_classTeacherDelDlg = new ClassTeacherDelDialog(this);
    m_courseDlg = new CourseDialog();
    m_courseDlg->setWindowTitle("课程表");
    m_courseDlg->resize(800, 600);
    // 设置群组ID和班级ID
    m_courseDlg->setGroupId(groupNumberId);
    m_courseDlg->setClassId(classid);

    // 设置一些课程
    //m_courseDlg->setCourse(1, 0, "数学");
    //m_courseDlg->setCourse(1, 1, "音乐");
    //m_courseDlg->setCourse(2, 1, "语文", true); // 高亮
    //m_courseDlg->setCourse(3, 4, "体育", true);

    // 顶部用户信息
    QHBoxLayout* topLayout = new QHBoxLayout(this);
    QLabel* lblAvatar = new QLabel(this);
    lblAvatar->setFixedSize(50, 50);
    lblAvatar->setStyleSheet("background-color: lightgray; border-radius: 25px;");
    
    // 用户信息：名称和ID
    QVBoxLayout* infoLayout = new QVBoxLayout(this);
    QLabel* lblName = new QLabel(groupName, this);
    lblName->setStyleSheet("color: white; font-size: 14px;");
    
    // ID显示（带锁图标）
    QHBoxLayout* idLayout = new QHBoxLayout(this);
    QLabel* lblLock = new QLabel("🔒", this);
    lblLock->setFixedSize(16, 16);
    QLabel* lblId = new QLabel(groupNumberId, this);
    lblId->setStyleSheet("color: white; font-size: 12px;");
    idLayout->addWidget(lblLock);
    idLayout->addWidget(lblId);
    idLayout->addStretch();
    
    infoLayout->addWidget(lblName);
    infoLayout->addLayout(idLayout);
    infoLayout->setSpacing(2);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    
    QPushButton* btnMore = new QPushButton("☰", this);
    btnMore->setFixedSize(30, 30);
    btnMore->setStyleSheet("background-color: transparent; color: white; font-size: 16px;");
    
    topLayout->addWidget(lblAvatar);
    topLayout->addLayout(infoLayout, 1);
    topLayout->addStretch();
    topLayout->addWidget(btnMore);
    mainLayout->addLayout(topLayout);

    // 班级编号（蓝色按钮 + 输入框）
    QPushButton* btnClassNum = new QPushButton("班级编号", this);
    btnClassNum->setStyleSheet("background-color: #4169E1; color: white; padding: 4px 8px; font-weight: bold;");
    mainLayout->addWidget(btnClassNum);
    
    QLineEdit* editClassNum = new QLineEdit("2349235", this);
    editClassNum->setAlignment(Qt::AlignCenter);
    editClassNum->setStyleSheet("color: red; font-size: 18px; font-weight: bold; background-color: white; border: 1px solid #ccc; padding: 4px;");
    mainLayout->addWidget(editClassNum);
    

    // 好友列表（蓝色按钮 + 圆形图标网格）
    QPushButton* btnFriends = new QPushButton("好友列表", this);
    btnFriends->setStyleSheet("background-color: #4169E1; color: white; padding: 4px 8px; font-weight: bold;");
    mainLayout->addWidget(btnFriends);
    
    QGroupBox* groupFriends = new QGroupBox(this);
    groupFriends->setTitle(""); // 移除标题
    groupFriends->setMinimumHeight(80); // 设置最小高度，确保有足够空间显示按钮
    QVBoxLayout* friendsLayout = new QVBoxLayout(groupFriends);
    friendsLayout->setContentsMargins(10, 10, 10, 10); // 设置 friendsLayout 的边距
    friendsLayout->setSpacing(5); // 设置 friendsLayout 的间距
    circlesLayout = new QHBoxLayout(this);
    // 设置布局的间距和边距
    circlesLayout->setSpacing(8);
    circlesLayout->setContentsMargins(5, 5, 5, 5);

    //QString blueStyle = "background-color:blue; border-radius:15px; color:white; font-weight:bold;";
    //QString redStyle = "background-color:red; border-radius:15px; color:white; font-weight:bold;";

    //FriendButton* circleRed = new FriendButton("", this);
    //circleRed->setStyleSheet(redStyle);
    //FriendButton* circleBlue1 = new FriendButton("", this);
    //FriendButton* circleBlue2 = new FriendButton("", this);
    //FriendButton* circlePlus = new FriendButton("+", this);
    //FriendButton* circleMinus = new FriendButton("-", this);

    //// 接收右键菜单信号
    //connect(circleRed, &FriendButton::setLeaderRequested, this, []() {
    //    qDebug() << "设为班主任";
    //    });
    //connect(circleRed, &FriendButton::cancelLeaderRequested, this, []() {
    //    qDebug() << "取消班主任";
    //    });
    //connect(circleBlue1, &FriendButton::setLeaderRequested, this, []() {
    //    qDebug() << "设为班主任";
    //    });
    //connect(circleBlue1, &FriendButton::cancelLeaderRequested, this, []() {
    //    qDebug() << "取消班主任";
    //    });
    //connect(circleBlue2, &FriendButton::setLeaderRequested, this, []() {
    //    qDebug() << "设为班主任";
    //    });
    //connect(circleBlue2, &FriendButton::cancelLeaderRequested, this, []() {
    //    qDebug() << "取消班主任";
    //    });

    //circlesLayout->addWidget(circleRed);
    //circlesLayout->addWidget(circleBlue1);
    //circlesLayout->addWidget(circleBlue2);
    //circlesLayout->addWidget(circlePlus);
    //circlesLayout->addWidget(circleMinus);

    // 在初始化时就创建 + 和 - 按钮，确保它们总是可见
    // 注意：按钮的父控件应该是 groupFriends，这样它们才会显示在正确的容器内
    m_circlePlus = new FriendButton("+", groupFriends);
    m_circlePlus->setFixedSize(50, 50);
    m_circlePlus->setMinimumSize(50, 50);
    m_circlePlus->setContextMenuEnabled(false);
    m_circlePlus->setVisible(true);
    circlesLayout->addWidget(m_circlePlus);
    
    m_circleMinus = new FriendButton("-", groupFriends);
    m_circleMinus->setFixedSize(50, 50);
    m_circleMinus->setMinimumSize(50, 50);
    m_circleMinus->setContextMenuEnabled(false);
    m_circleMinus->setVisible(true);
    circlesLayout->addWidget(m_circleMinus);
    
    // 连接按钮点击事件（这些连接会在 InitGroupMember 中重新设置，但先设置确保可用）
    connect(m_circlePlus, &FriendButton::clicked, this, [this]() {
        if (m_friendSelectDlg)
        {
            if (m_friendSelectDlg->isHidden())
            {
                // 获取当前群组成员ID列表，用于排除
                QVector<QString> memberIds;
                for (const auto& member : m_groupMemberInfo)
                {
                    memberIds.append(member.member_id);
                }
                m_friendSelectDlg->setExcludedMemberIds(memberIds);
                m_friendSelectDlg->setGroupId(m_groupNumberId);
                m_friendSelectDlg->setGroupName(m_groupName);
                m_friendSelectDlg->InitData();
                m_friendSelectDlg->show();
            }
            else
            {
                m_friendSelectDlg->hide();
            }
        }
    });
    
    connect(m_circleMinus, &FriendButton::clicked, this, [this]() {
        if (m_memberKickDlg)
        {
            if (m_memberKickDlg->isHidden())
            {
                m_memberKickDlg->setGroupId(m_groupNumberId);
                m_memberKickDlg->setGroupName(m_groupName);
                m_memberKickDlg->InitData(m_groupMemberInfo);
                m_memberKickDlg->show();
            }
            else
            {
                m_memberKickDlg->hide();
            }
        }
    });

    friendsLayout->addLayout(circlesLayout);
    mainLayout->addWidget(groupFriends);

    // 设置开关区域（使用自定义控件）
    QFrame* settingsFrame = new QFrame(this);
    settingsFrame->setStyleSheet("padding: 10px; background-color: #5C5C5C;");
    QVBoxLayout* settingsLayout = new QVBoxLayout(settingsFrame);
    settingsLayout->setSpacing(5);
    settingsLayout->setContentsMargins(10, 10, 10, 10);
    
    // 创建5个设置行（使用自定义SettingRow控件）
    SettingRow* rowIntercom = new SettingRow("开启对讲", this);
    rowIntercom->setHighlighted(false);  // 移除红色边框高亮
    rowIntercom->setChecked(false);
    
    SettingRow* rowSchedule = new SettingRow("开启浮动今日课表", this);
    rowSchedule->setChecked(false);
    
    SettingRow* rowPrepare = new SettingRow("关联课前准备", this);
    rowPrepare->setChecked(false);
    
    SettingRow* rowHomework = new SettingRow("关联家庭作业", this);
    rowHomework->setChecked(false);
    
    SettingRow* rowNotify = new SettingRow("接收通知", this);
    rowNotify->setChecked(false);
    
    // 添加到布局
    settingsLayout->addWidget(rowIntercom);
    settingsLayout->addWidget(rowSchedule);
    settingsLayout->addWidget(rowPrepare);
    settingsLayout->addWidget(rowHomework);
    settingsLayout->addWidget(rowNotify);
    
    mainLayout->addWidget(settingsFrame);

    // 解散群聊 / 退出群聊 - 隐藏这些按钮
    // 如果按钮已经存在，先删除它们（防止重复创建）
    if (m_btnDismiss) {
        m_btnDismiss->deleteLater();
        m_btnDismiss = nullptr;
    }
    if (m_btnExit) {
        m_btnExit->deleteLater();
        m_btnExit = nullptr;
    }
    
    // 创建按钮但不显示（保留功能以备后用）
    QHBoxLayout* bottomBtns = new QHBoxLayout(this);
    m_btnDismiss = new QPushButton("解散群聊", this);
    m_btnExit = new QPushButton("退出群聊", this);
    bottomBtns->addWidget(m_btnDismiss);
    bottomBtns->addWidget(m_btnExit);
    // 隐藏按钮
    m_btnDismiss->hide();
    m_btnExit->hide();
    // 不添加到主布局，这样它们就不会显示
    // mainLayout->addLayout(bottomBtns);
    
    // 初始状态：默认都禁用，等InitGroupMember调用后再更新
    m_btnDismiss->setEnabled(false);
    m_btnExit->setEnabled(false);
    
    // 连接退出群聊按钮的点击事件
    connect(m_btnExit, &QPushButton::clicked, this, &QGroupInfo::onExitGroupClicked);
    // 连接解散群聊按钮的点击事件
    connect(m_btnDismiss, &QPushButton::clicked, this, &QGroupInfo::onDismissGroupClicked);
}

void QGroupInfo::InitGroupMember(QString group_id, QVector<GroupMemberInfo> groupMemberInfo)
{
    // 检查传入的参数是否与已保存的参数相同
    if (m_groupNumberId == group_id && m_groupMemberInfo.size() == groupMemberInfo.size()) {
        // 检查成员列表是否完全相同
        bool isSame = true;
        for (int i = 0; i < groupMemberInfo.size(); ++i) {
            if (m_groupMemberInfo[i].member_id != groupMemberInfo[i].member_id ||
                m_groupMemberInfo[i].member_name != groupMemberInfo[i].member_name ||
                m_groupMemberInfo[i].member_role != groupMemberInfo[i].member_role) {
                isSame = false;
                break;
            }
        }
        if (isSame) {
            qDebug() << "QGroupInfo::InitGroupMember 参数相同，跳过更新。群组ID:" << group_id << "，成员数量:" << groupMemberInfo.size();
            return;
        }
    }
    
    m_groupNumberId = group_id;
    m_groupMemberInfo = groupMemberInfo;

    qDebug() << "QGroupInfo::InitGroupMember 被调用，群组ID:" << group_id << "，成员数量:" << groupMemberInfo.size();

    // 确保 circlesLayout 已初始化
    if (!circlesLayout) {
        qWarning() << "circlesLayout 未初始化，无法显示好友列表！";
        return;
    }
    
    // 找到 groupFriends 容器，成员按钮的父控件应该是它
    QGroupBox* groupFriends = nullptr;
    QWidget* parentWidget = circlesLayout->parentWidget();
    if (parentWidget) {
        groupFriends = qobject_cast<QGroupBox*>(parentWidget);
    }
    if (!groupFriends) {
        // 如果找不到，尝试从 circlesLayout 的父布局查找
        QLayout* parentLayout = qobject_cast<QLayout*>(circlesLayout->parent());
        if (parentLayout && parentLayout->parentWidget()) {
            groupFriends = qobject_cast<QGroupBox*>(parentLayout->parentWidget());
        }
    }
    // 如果还是找不到，使用 this 作为父控件
    if (!groupFriends) {
        // 尝试通过对象名查找
        QList<QGroupBox*> groupBoxes = this->findChildren<QGroupBox*>();
        for (QGroupBox* gb : groupBoxes) {
            if (gb->title() == "好友列表") {
                groupFriends = gb;
                break;
            }
        }
    }
    // 最后使用 this 作为父控件
    QWidget* buttonParent = groupFriends ? static_cast<QWidget*>(groupFriends) : this;
    qDebug() << "成员按钮的父控件:" << (groupFriends ? "groupFriends" : "this");

    // 清空之前的成员圆圈（保留 + 和 - 按钮）
    {
        // 遍历布局，找到 + 和 - 按钮，删除其他成员圆圈
        QList<QLayoutItem*> itemsToRemove;
        for (int i = 0; i < circlesLayout->count(); i++) {
            QLayoutItem* item = circlesLayout->itemAt(i);
            if (item && item->widget()) {
                FriendButton* btn = qobject_cast<FriendButton*>(item->widget());
                if (btn) {
                    if (btn->text() == "+") {
                        m_circlePlus = btn;
                        m_circlePlus->setContextMenuEnabled(false); // 禁用右键菜单
                    } else if (btn->text() == "-") {
                        m_circleMinus = btn;
                        m_circleMinus->setContextMenuEnabled(false); // 禁用右键菜单
                    } else {
                        // 成员圆圈，标记为删除
                        itemsToRemove.append(item);
                    }
                }
            }
        }
        
        // 删除成员圆圈
        for (auto item : itemsToRemove) {
            circlesLayout->removeItem(item);
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
        
        // 如果 + 和 - 按钮不存在，创建它们
        if (!m_circlePlus) {
            m_circlePlus = new FriendButton("+", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            m_circlePlus->setFixedSize(50, 50);
            m_circlePlus->setMinimumSize(50, 50);
            m_circlePlus->setContextMenuEnabled(false); // 禁用右键菜单
            connect(m_circlePlus, &FriendButton::clicked, this, [this]() {
                if (m_friendSelectDlg)
                {
                    if (m_friendSelectDlg->isHidden())
                    {
                        // 获取当前群组成员ID列表，用于排除
                        QVector<QString> memberIds;
                        for (const auto& member : m_groupMemberInfo)
                        {
                            memberIds.append(member.member_id);
                        }
                        m_friendSelectDlg->setExcludedMemberIds(memberIds);
                        m_friendSelectDlg->setGroupId(m_groupNumberId); // 设置群组ID
                        m_friendSelectDlg->setGroupName(m_groupName); // 设置群组名称
                        m_friendSelectDlg->InitData();
                        m_friendSelectDlg->show();
                    }
                    else
                    {
                        m_friendSelectDlg->hide();
                    }
                }
            });
            circlesLayout->addWidget(m_circlePlus);
        }
        if (!m_circleMinus) {
            m_circleMinus = new FriendButton("-", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            m_circleMinus->setFixedSize(50, 50);
            m_circleMinus->setMinimumSize(50, 50);
            m_circleMinus->setContextMenuEnabled(false); // 禁用右键菜单
            connect(m_circleMinus, &FriendButton::clicked, this, [this]() {
                if (m_memberKickDlg)
                {
                    if (m_memberKickDlg->isHidden())
                    {
                        // 设置群组ID和名称
                        m_memberKickDlg->setGroupId(m_groupNumberId);
                        m_memberKickDlg->setGroupName(m_groupName);
                        // 初始化成员列表数据（排除群主）
                        m_memberKickDlg->InitData(m_groupMemberInfo);
                        m_memberKickDlg->show();
                    }
                    else
                    {
                        m_memberKickDlg->hide();
                    }
                }
            });
            circlesLayout->addWidget(m_circleMinus);
        }
    }

    // 检查成员数据是否为空
    if (m_groupMemberInfo.isEmpty()) {
        qDebug() << "成员列表为空，不显示任何成员";
        updateButtonStates();
        return;
    }

    QString redStyle = "background-color:red; border-radius:25px; color:white; font-weight:bold;";
    QString blueStyle = "background-color:blue; border-radius:25px; color:white; font-weight:bold;";

    // 检查当前用户是否是群主
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    bool isOwner = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == currentUserId && member.member_role == "群主") {
            isOwner = true;
            break;
        }
    }

    // 找到 + 按钮的位置（应该在倒数第二个位置，- 按钮在最后）
    int plusButtonIndex = -1;
    int minusButtonIndex = -1;
    for (int i = 0; i < circlesLayout->count(); ++i) {
        QLayoutItem* item = circlesLayout->itemAt(i);
        if (item && item->widget()) {
            FriendButton* btn = qobject_cast<FriendButton*>(item->widget());
            if (btn) {
                if (btn->text() == "+") {
                    plusButtonIndex = i;
                } else if (btn->text() == "-") {
                    minusButtonIndex = i;
                }
            }
        }
    }

    // 确定插入位置：在 + 按钮之前插入，如果找不到 + 按钮，则在布局开始位置插入
    int insertIndex = (plusButtonIndex >= 0) ? plusButtonIndex : 0;
    qDebug() << "插入位置计算：+ 按钮索引:" << plusButtonIndex << "，- 按钮索引:" << minusButtonIndex << "，最终插入位置:" << insertIndex;
    
    // 使用之前找到的 buttonParent（应该是 groupFriends）
    for (auto iter : m_groupMemberInfo)
    {
        FriendButton* circleBtn = nullptr;
        if (iter.member_role == "群主")
        {
            circleBtn = new FriendButton("", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            circleBtn->setFixedSize(50, 50);
            circleBtn->setMinimumSize(50, 50);
            circleBtn->setStyleSheet(redStyle); // 群主用红色圆圈
        }
        else
        {
            circleBtn = new FriendButton("", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            circleBtn->setFixedSize(50, 50);
            circleBtn->setMinimumSize(50, 50);
            circleBtn->setStyleSheet(blueStyle); // 其他成员用蓝色圆圈
        }
        
        // 设置成员角色
        circleBtn->setMemberRole(iter.member_role);
        
        // 只有当前用户是群主时，才启用右键菜单
        circleBtn->setContextMenuEnabled(isOwner);
        
        // 接收右键菜单信号，传递成员ID
        QString memberId = iter.member_id;
        connect(circleBtn, &FriendButton::setLeaderRequested, this, [this, memberId]() {
            onSetLeaderRequested(memberId);
        });
        connect(circleBtn, &FriendButton::cancelLeaderRequested, this, [this, memberId]() {
            onCancelLeaderRequested(memberId);
        });
        
        // 设置按钮文本（完整成员名字）
        circleBtn->setText(iter.member_name);
        circleBtn->setProperty("member_id", iter.member_id);
        
        // 确保按钮可见并显示
        circleBtn->setVisible(true);
        circleBtn->show();
        
        // 在 + 按钮之前插入成员圆圈
        circlesLayout->insertWidget(insertIndex, circleBtn);
        insertIndex++; // 更新插入位置
        
        qDebug() << "添加成员按钮:" << iter.member_name << "，角色:" << iter.member_role 
                 << "，插入位置:" << (insertIndex - 1) << "，按钮尺寸:" << circleBtn->size() 
                 << "，是否可见:" << circleBtn->isVisible() << "，父窗口:" << circleBtn->parent();
    }
    
    // 确保所有按钮都可见并强制刷新布局
    qDebug() << "开始检查布局中的所有按钮，布局项总数:" << circlesLayout->count();
    /*for (int i = 0; i < circlesLayout->count(); ++i) {
        QLayoutItem* item = circlesLayout->itemAt(i);
        if (item && item->widget()) {
            QWidget* widget = item->widget();
            FriendButton* btn = qobject_cast<FriendButton*>(widget);
            if (btn) {
                widget->setVisible(true);
                widget->show();
                widget->raise();
                widget->update();
                qDebug() << "按钮[" << i << "] 文本:" << btn->text() << "，尺寸:" << widget->size() 
                         << "，可见:" << widget->isVisible() << "，父窗口:" << widget->parent();
            } else {
                widget->setVisible(true);
                widget->show();
                widget->update();
            }
        }
    }*/
    
    // 强制刷新布局和父容器
    //circlesLayout->update();
    //QWidget* parentWidget = circlesLayout->parentWidget();
    //if (parentWidget) {
    //    parentWidget->update();
    //    parentWidget->repaint();
    //    qDebug() << "父容器已刷新:" << parentWidget->objectName() << "，尺寸:" << parentWidget->size();
    //}
    //

    qDebug() << "好友列表显示完成，共添加" << m_groupMemberInfo.size() << "个成员，当前布局项数:" << circlesLayout->count();
    
    // 更新按钮状态（根据当前用户是否是群主）
    updateButtonStates();

    // 刷新整个对话框
    this->update();
    this->repaint();
}


void QGroupInfo::InitGroupMember()
{
    if (m_groupMemberInfo.size() == 0)
    {
        return;
    }

    //m_groupNumberId = group_id;
    //m_groupMemberInfo = groupMemberInfo;

    // 清空之前的成员圆圈（保留 + 和 - 按钮）
    if (circlesLayout) {
        // 遍历布局，找到 + 和 - 按钮，删除其他成员圆圈
        QList<QLayoutItem*> itemsToRemove;
        for (int i = 0; i < circlesLayout->count(); i++) {
            QLayoutItem* item = circlesLayout->itemAt(i);
            if (item && item->widget()) {
                FriendButton* btn = qobject_cast<FriendButton*>(item->widget());
                if (btn) {
                    if (btn->text() == "+") {
                        m_circlePlus = btn;
                        m_circlePlus->setContextMenuEnabled(false); // 禁用右键菜单
                    }
                    else if (btn->text() == "-") {
                        m_circleMinus = btn;
                        m_circleMinus->setContextMenuEnabled(false); // 禁用右键菜单
                    }
                    else {
                        // 成员圆圈，标记为删除
                        itemsToRemove.append(item);
                    }
                }
            }
        }

        // 删除成员圆圈
        for (auto item : itemsToRemove) {
            circlesLayout->removeItem(item);
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }

        // 找到 groupFriends 容器，按钮的父控件应该是它
        QGroupBox* groupFriends = nullptr;
        QWidget* parentWidget = circlesLayout->parentWidget();
        if (parentWidget) {
            groupFriends = qobject_cast<QGroupBox*>(parentWidget);
        }
        if (!groupFriends) {
            QLayout* parentLayout = qobject_cast<QLayout*>(circlesLayout->parent());
            if (parentLayout && parentLayout->parentWidget()) {
                groupFriends = qobject_cast<QGroupBox*>(parentLayout->parentWidget());
            }
        }
        if (!groupFriends) {
            QList<QGroupBox*> groupBoxes = this->findChildren<QGroupBox*>();
            for (QGroupBox* gb : groupBoxes) {
                if (gb->title() == "好友列表") {
                    groupFriends = gb;
                    break;
                }
            }
        }
        QWidget* buttonParent = groupFriends ? static_cast<QWidget*>(groupFriends) : this;
        
        // 如果 + 和 - 按钮不存在，创建它们
        if (!m_circlePlus) {
            m_circlePlus = new FriendButton("+", buttonParent);
            m_circlePlus->setFixedSize(50, 50);
            m_circlePlus->setMinimumSize(50, 50);
            m_circlePlus->setContextMenuEnabled(false); // 禁用右键菜单
            connect(m_circlePlus, &FriendButton::clicked, this, [this]() {
                if (m_friendSelectDlg)
                {
                    if (m_friendSelectDlg->isHidden())
                    {
                        // 获取当前群组成员ID列表，用于排除
                        QVector<QString> memberIds;
                        for (const auto& member : m_groupMemberInfo)
                        {
                            memberIds.append(member.member_id);
                        }
                        m_friendSelectDlg->setExcludedMemberIds(memberIds);
                        m_friendSelectDlg->setGroupId(m_groupNumberId); // 设置群组ID
                        m_friendSelectDlg->setGroupName(m_groupName); // 设置群组名称
                        m_friendSelectDlg->InitData();
                        m_friendSelectDlg->show();
                    }
                    else
                    {
                        m_friendSelectDlg->hide();
                    }
                }
                });
            circlesLayout->addWidget(m_circlePlus);
        }
        if (!m_circleMinus) {
            m_circleMinus = new FriendButton("-", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            m_circleMinus->setFixedSize(50, 50);
            m_circleMinus->setMinimumSize(50, 50);
            m_circleMinus->setContextMenuEnabled(false); // 禁用右键菜单
            connect(m_circleMinus, &FriendButton::clicked, this, [this]() {
                if (m_memberKickDlg)
                {
                    if (m_memberKickDlg->isHidden())
                    {
                        // 设置群组ID和名称
                        m_memberKickDlg->setGroupId(m_groupNumberId);
                        m_memberKickDlg->setGroupName(m_groupName);
                        // 初始化成员列表数据（排除群主）
                        m_memberKickDlg->InitData(m_groupMemberInfo);
                        m_memberKickDlg->show();
                    }
                    else
                    {
                        m_memberKickDlg->hide();
                    }
                }
                });
            circlesLayout->addWidget(m_circleMinus);
        }
    }

    QString redStyle = "background-color:red; border-radius:25px; color:white; font-weight:bold;";
    QString blueStyle = "background-color:blue; border-radius:25px; color:white; font-weight:bold;";

    // 检查当前用户是否是群主
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    bool isOwner = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == currentUserId && member.member_role == "群主") {
            isOwner = true;
            break;
        }
    }

    // 找到 + 按钮的位置（应该在倒数第二个位置，- 按钮在最后）
    int plusButtonIndex = -1;
    int minusButtonIndex = -1;
    for (int i = 0; i < circlesLayout->count(); ++i) {
        QLayoutItem* item = circlesLayout->itemAt(i);
        if (item && item->widget()) {
            FriendButton* btn = qobject_cast<FriendButton*>(item->widget());
            if (btn) {
                if (btn->text() == "+") {
                    plusButtonIndex = i;
                } else if (btn->text() == "-") {
                    minusButtonIndex = i;
                }
            }
        }
    }

    // 找到 groupFriends 容器，成员按钮的父控件应该是它
    QGroupBox* groupFriends = nullptr;
    QWidget* parentWidget = circlesLayout->parentWidget();
    if (parentWidget) {
        groupFriends = qobject_cast<QGroupBox*>(parentWidget);
    }
    if (!groupFriends) {
        // 如果找不到，尝试从 circlesLayout 的父布局查找
        QLayout* parentLayout = qobject_cast<QLayout*>(circlesLayout->parent());
        if (parentLayout && parentLayout->parentWidget()) {
            groupFriends = qobject_cast<QGroupBox*>(parentLayout->parentWidget());
        }
    }
    // 如果还是找不到，使用 this 作为父控件
    if (!groupFriends) {
        // 尝试通过对象名查找
        QList<QGroupBox*> groupBoxes = this->findChildren<QGroupBox*>();
        for (QGroupBox* gb : groupBoxes) {
            if (gb->title() == "好友列表") {
                groupFriends = gb;
                break;
            }
        }
    }
    // 最后使用 this 作为父控件
    QWidget* buttonParent = groupFriends ? static_cast<QWidget*>(groupFriends) : this;
    qDebug() << "成员按钮的父控件:" << (groupFriends ? "groupFriends" : "this");
    
    // 确定插入位置：在 + 按钮之前插入，如果找不到 + 按钮，则在布局开始位置插入
    int insertIndex = (plusButtonIndex >= 0) ? plusButtonIndex : 0;
    qDebug() << "插入位置计算：+ 按钮索引:" << plusButtonIndex << "，- 按钮索引:" << minusButtonIndex << "，最终插入位置:" << insertIndex;
    
    for (auto iter : m_groupMemberInfo)
    {
        FriendButton* circleBtn = nullptr;
        if (iter.member_role == "群主")
        {
            circleBtn = new FriendButton("", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            circleBtn->setFixedSize(50, 50);
            circleBtn->setMinimumSize(50, 50);
            circleBtn->setStyleSheet(redStyle); // 群主用红色圆圈
        }
        else
        {
            circleBtn = new FriendButton("", buttonParent);
            // FriendButton 构造函数已设置 setFixedSize(50, 50)，这里确保尺寸正确
            circleBtn->setFixedSize(50, 50);
            circleBtn->setMinimumSize(50, 50);
            circleBtn->setStyleSheet(blueStyle); // 其他成员用蓝色圆圈
        }
        
        // 设置成员角色
        circleBtn->setMemberRole(iter.member_role);
        
        // 只有当前用户是群主时，才启用右键菜单
        circleBtn->setContextMenuEnabled(isOwner);
        
        // 接收右键菜单信号，传递成员ID
        QString memberId = iter.member_id;
        connect(circleBtn, &FriendButton::setLeaderRequested, this, [this, memberId]() {
            onSetLeaderRequested(memberId);
        });
        connect(circleBtn, &FriendButton::cancelLeaderRequested, this, [this, memberId]() {
            onCancelLeaderRequested(memberId);
        });
        
        circleBtn->setVisible(true);
        circleBtn->show();
        circleBtn->setText(iter.member_name);
        circleBtn->setProperty("member_id", iter.member_id);
        
        // 在 + 按钮之前插入成员圆圈
        circlesLayout->insertWidget(insertIndex, circleBtn);
        insertIndex++; // 更新插入位置
        
        qDebug() << "添加成员按钮:" << iter.member_name << "，角色:" << iter.member_role << "，插入位置:" << (insertIndex - 1);
    }

    // 更新按钮状态（根据当前用户是否是群主）
    updateButtonStates();
}

void QGroupInfo::updateButtonStates()
{
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    
    // 查找当前用户在成员列表中的角色
    bool isOwner = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == currentUserId) {
            if (member.member_role == "群主") {
                isOwner = true;
            }
            break;
        }
    }
    
    // 根据当前用户是否是群主来设置按钮状态
    if (m_btnDismiss && m_btnExit) {
        if (isOwner) {
            // 当前用户是群主：解散群聊按钮可用，退出群聊按钮也可用（需要先转让群主）
            m_btnDismiss->setEnabled(true);
            m_btnExit->setEnabled(true);
            qDebug() << "当前用户是群主，解散群聊按钮可用，退出群聊按钮可用（需要先转让群主）";
        } else {
            // 当前用户不是群主：解散群聊按钮禁用，退出群聊按钮可用
            m_btnDismiss->setEnabled(false);
            m_btnExit->setEnabled(true);
            qDebug() << "当前用户不是群主，解散群聊按钮禁用，退出群聊按钮可用";
        }
    }
}

void QGroupInfo::onExitGroupClicked()
{
    if (m_groupNumberId.isEmpty()) {
        QMessageBox::warning(this, "错误", "群组ID为空，无法退出群聊");
        return;
    }
    
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString userId = userInfo.teacher_unique_id;
    QString userName = userInfo.strName;
    
    // 检查当前用户是否是群主
    bool isOwner = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == userId && member.member_role == "群主") {
            isOwner = true;
            break;
        }
    }
    
    // 如果是群主，需要检查是否有其他管理员
    if (isOwner) {
        // 查找所有管理员（排除群主自己）
        QVector<GroupMemberInfo> adminList;
        for (const auto& member : m_groupMemberInfo) {
            if (member.member_id != userId && member.member_role == "管理员") {
                adminList.append(member);
            }
        }
        
        // 如果没有其他管理员，提示错误
        if (adminList.isEmpty()) {
            QMessageBox::warning(this, "无法退出", "只有一个班主任。请先设置新班主任，再退出。");
            return;
        }
        
        // 有管理员，按好友列表顺序排序（使用成员名称排序）
        std::sort(adminList.begin(), adminList.end(), [](const GroupMemberInfo& a, const GroupMemberInfo& b) {
            return a.member_name < b.member_name;
        });
        
        // 获取第一个管理员
        QString newOwnerId = adminList[0].member_id;
        QString newOwnerName = adminList[0].member_name;
        
        // 确认对话框
        int ret = QMessageBox::question(this, "确认退出", 
            QString("确定要退出群聊 \"%1\" 吗？\n\n退出前将自动将群主身份转让给 %2（第一个管理员）。").arg(m_groupName).arg(newOwnerName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        
        if (ret != QMessageBox::Yes) {
            return;
        }
        
        // 转让群主并退出
        transferOwnerAndQuit(newOwnerId, newOwnerName);
        return;
    }
    
    // 不是群主，直接退出
    // 确认对话框
    int ret = QMessageBox::question(this, "确认退出", 
        QString("确定要退出群聊 \"%1\" 吗？").arg(m_groupName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    qDebug() << "开始退出群聊，群组ID:" << m_groupNumberId << "，用户ID:" << userId;
    
    // 构造回调数据结构
    struct ExitGroupCallbackData {
        QGroupInfo* dlg;
        QString groupId;
        QString userId;
        QString userName;
    };
    
    ExitGroupCallbackData* callbackData = new ExitGroupCallbackData;
    callbackData->dlg = this;
    callbackData->groupId = m_groupNumberId;
    callbackData->userId = userId;
    callbackData->userName = userName;
    
    // 检查REST API是否初始化
    if (!m_restAPI) {
        QMessageBox::critical(this, "错误", "REST API未初始化！");
        delete callbackData;
        return;
    }
    
    // 在使用REST API前设置管理员账号信息
    // 注意：REST API需要使用应用管理员账号，使用当前登录用户的teacher_unique_id
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    }
    
    // 调用REST API退出群聊接口
    m_restAPI->quitGroup(m_groupNumberId, userId,
        [=](int errorCode, const QString& errorDesc, const QJsonObject& result) {
            if (errorCode != 0) {
                QString errorMsg = QString("退出群聊失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                qDebug() << errorMsg;
                QMessageBox::critical(callbackData->dlg, "退出失败", errorMsg);
                delete callbackData;
                return;
            }
            
            qDebug() << "REST API退出群聊成功:" << callbackData->groupId;
            
            // 立即从本地成员列表中移除当前用户
            QVector<GroupMemberInfo> updatedMemberList;
            QString leftUserId = callbackData->userId; // 记录退出的用户ID
            for (const auto& member : callbackData->dlg->m_groupMemberInfo) {
                if (member.member_id != callbackData->userId) {
                    updatedMemberList.append(member);
                }
            }
            callbackData->dlg->m_groupMemberInfo = updatedMemberList;
            
            // 立即更新UI（移除退出的成员）
            callbackData->dlg->InitGroupMember(callbackData->dlg->m_groupNumberId, callbackData->dlg->m_groupMemberInfo);
            
            // REST API成功，现在调用自己的服务器接口（传递退出的用户ID，用于后续处理）
            callbackData->dlg->sendExitGroupRequestToServer(callbackData->groupId, callbackData->userId, leftUserId);
            
            // 释放回调数据
            delete callbackData;
        });
}

void QGroupInfo::sendExitGroupRequestToServer(const QString& groupId, const QString& userId, const QString& leftUserId)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["user_id"] = userId;
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器
    QString url = "http://47.100.126.194:5000/groups/leave";
    
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
                    qDebug() << "退出群聊请求已发送到服务器";
                    
                    // 服务器响应成功，发出成员退出群聊信号，通知父窗口刷新成员列表（传递退出的用户ID）
                    emit this->memberLeftGroup(groupId, leftUserId);
                    
                    // 显示成功消息
                    QMessageBox::information(this, "退出成功", 
                        QString("已成功退出群聊 \"%1\"！").arg(m_groupName));
                    
                    // 关闭对话框
                    this->accept();
                } else {
                    QString message = obj["message"].toString();
                    qDebug() << "服务器返回错误:" << message;
                    QMessageBox::warning(this, "退出失败", 
                        QString("服务器返回错误: %1").arg(message));
                }
            } else {
                qDebug() << "解析服务器响应失败";
                QMessageBox::warning(this, "退出失败", "解析服务器响应失败");
            }
        } else {
            qDebug() << "发送退出群聊请求到服务器失败:" << reply->errorString();
            QMessageBox::warning(this, "退出失败", 
                QString("网络错误: %1").arg(reply->errorString()));
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void QGroupInfo::onDismissGroupClicked()
{
    if (m_groupNumberId.isEmpty()) {
        QMessageBox::warning(this, "错误", "群组ID为空，无法解散群聊");
        return;
    }
    
    // 确认对话框
    int ret = QMessageBox::question(this, "确认解散", 
        QString("确定要解散群聊 \"%1\" 吗？\n解散后所有成员将被移除，此操作不可恢复！").arg(m_groupName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString userId = userInfo.teacher_unique_id;
    QString userName = userInfo.strName;
    
    qDebug() << "开始解散群聊，群组ID:" << m_groupNumberId << "，用户ID:" << userId;
    
    // 构造回调数据结构
    DismissGroupCallbackData* callbackData = new DismissGroupCallbackData;
    callbackData->dlg = this;
    callbackData->groupId = m_groupNumberId;
    callbackData->userId = userId;
    callbackData->userName = userName;
    
    // 检查REST API是否初始化
    if (!m_restAPI) {
        QMessageBox::critical(this, "错误", "REST API未初始化！");
        delete callbackData;
        return;
    }
    
    // 在使用REST API前设置管理员账号信息
    // 注意：REST API需要使用应用管理员账号，使用当前登录用户的teacher_unique_id
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    }
    
    // 调用REST API解散群聊接口
    m_restAPI->destroyGroup(m_groupNumberId,
        [=](int errorCode, const QString& errorDesc, const QJsonObject& result) {
            if (errorCode != 0) {
                QString errorMsg;
                
                // 特殊处理常见的错误码
                if (errorCode == 10004) {
                    errorMsg = QString("解散群聊失败\n错误码: %1\n错误描述: %2\n\n注意：私有群无法解散群组。\n只有公开群、聊天室和直播大群的群主可以解散群组。").arg(errorCode).arg(errorDesc);
                } else {
                    errorMsg = QString("解散群聊失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                }
                
                qDebug() << errorMsg;
                QMessageBox::critical(callbackData->dlg, "解散失败", errorMsg);
                
                // 释放回调数据
                delete callbackData;
                return;
            }
            
            qDebug() << "REST API解散群聊成功:" << callbackData->groupId;
            
            // REST API成功，现在调用自己的服务器接口（传递回调数据以便后续释放）
            callbackData->dlg->sendDismissGroupRequestToServer(callbackData->groupId, callbackData->userId, callbackData);
        });
}

void QGroupInfo::sendDismissGroupRequestToServer(const QString& groupId, const QString& userId, void* callbackData)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["user_id"] = userId;
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器（假设解散接口为 /groups/dismiss，如果不同请修改）
    QString url = "http://47.100.126.194:5000/groups/dismiss";
    
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
                    qDebug() << "解散群聊请求已发送到服务器";
                    
                    // 服务器响应成功，发出群聊解散信号，通知父窗口刷新群列表
                    emit this->groupDismissed(groupId);
                    
                    // 显示成功消息
                    QMessageBox::information(this, "解散成功", 
                        QString("已成功解散群聊 \"%1\"！").arg(m_groupName));
                    
                    // 关闭对话框
                    this->accept();
                } else {
                    QString message = obj["message"].toString();
                    qDebug() << "服务器返回错误:" << message;
                    QMessageBox::warning(this, "解散失败", 
                        QString("服务器返回错误: %1").arg(message));
                }
            } else {
                qDebug() << "解析服务器响应失败";
                QMessageBox::warning(this, "解散失败", "解析服务器响应失败");
            }
        } else {
            qDebug() << "发送解散群聊请求到服务器失败:" << reply->errorString();
            QMessageBox::warning(this, "解散失败", 
                QString("网络错误: %1").arg(reply->errorString()));
        }
        
        // 释放回调数据（如果提供了）
        if (callbackData) {
            delete (DismissGroupCallbackData*)callbackData;
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void QGroupInfo::refreshMemberList(const QString& groupId)
{
    // 通知父窗口（ScheduleDialog）刷新成员列表
    emit membersRefreshed(groupId);
    qDebug() << "发出成员列表刷新信号，群组ID:" << groupId;
}

void QGroupInfo::fetchGroupMemberListFromREST(const QString& groupId)
{
    if (!m_restAPI) {
        qWarning() << "REST API未初始化，无法获取群成员列表";
        return;
    }
    
    // 设置管理员账号信息
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    } else {
        qWarning() << "管理员账号未设置，无法获取群成员列表";
        return;
    }
    
    // 使用REST API获取群成员列表
    m_restAPI->getGroupMemberList(groupId, 100, 0, 
        [this](int errorCode, const QString& errorDesc, const QJsonObject& result) {
            if (errorCode != 0) {
                qDebug() << "获取群成员列表失败，错误码:" << errorCode << "，描述:" << errorDesc;
                return;
            }
            
            // 解析成员列表
            QVector<GroupMemberInfo> memberList;
            QJsonArray memberArray = result["MemberList"].toArray();
            
            for (const QJsonValue& value : memberArray) {
                if (!value.isObject()) continue;
                
                QJsonObject memberObj = value.toObject();
                GroupMemberInfo memberInfo;
                
                // 获取成员账号
                memberInfo.member_id = memberObj["Member_Account"].toString();
                
                // 获取成员角色
                QString role = memberObj["Role"].toString();
                if (role == "Owner") {
                    memberInfo.member_role = "群主";
                } else if (role == "Admin") {
                    memberInfo.member_role = "管理员";
                } else {
                    memberInfo.member_role = "成员";
                }
                
                // 获取成员名称（如果有）
                if (memberObj.contains("NameCard") && !memberObj["NameCard"].toString().isEmpty()) {
                    memberInfo.member_name = memberObj["NameCard"].toString();
                } else {
                    // 如果没有群名片，使用账号ID
                    memberInfo.member_name = memberInfo.member_id;
                }
                
                memberList.append(memberInfo);
            }
            
            // 合并更新成员列表（保留原有成员名称等信息）
            // 从REST API获取的成员列表可能没有成员名称，所以需要合并而不是直接替换
            int existingCount = m_groupMemberInfo.size();
            int newCount = 0;
            
            for (const GroupMemberInfo& newMember : memberList) {
                // 检查 m_groupMemberInfo 中是否已存在该成员
                bool found = false;
                for (GroupMemberInfo& existingMember : m_groupMemberInfo) {
                    if (existingMember.member_id == newMember.member_id) {
                        // 成员已存在，只更新角色信息（保留原有的成员名称）
                        existingMember.member_role = newMember.member_role;
                        found = true;
                        break;
                    }
                }
                
                // 如果成员不存在，添加到列表中
                if (!found) {
                    m_groupMemberInfo.append(newMember);
                    newCount++;
                }
            }
            
            // 刷新UI（使用更新后的完整成员列表）
            InitGroupMember(m_groupNumberId, m_groupMemberInfo);
            
            qDebug() << "成功更新群成员列表，原有" << existingCount << "个成员，新增" 
                     << newCount << "个成员，当前共" << m_groupMemberInfo.size() << "个成员";
        });
}

void QGroupInfo::onSetLeaderRequested(const QString& memberId)
{
    // 检查当前用户是否是群主
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    
    bool isOwner = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == currentUserId && member.member_role == "群主") {
            isOwner = true;
            break;
        }
    }
    
    if (!isOwner) {
        QMessageBox::warning(this, "权限不足", "只有群主可以设置管理员！");
        return;
    }
    
    // 检查成员是否已经是管理员
    bool isAlreadyAdmin = false;
    QString memberName;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == memberId) {
            memberName = member.member_name;
            if (member.member_role == "管理员") {
                isAlreadyAdmin = true;
            }
            break;
        }
    }
    
    if (isAlreadyAdmin) {
        QMessageBox::information(this, "提示", "该成员已经是管理员！");
        return;
    }
    
    // 检查REST API是否初始化
    if (!m_restAPI) {
        QMessageBox::critical(this, "错误", "REST API未初始化！");
        return;
    }
    
    // 设置管理员账号信息
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    } else {
        QMessageBox::critical(this, "错误", "管理员账号未设置！");
        return;
    }
    
    // 先获取群组信息，检查群组类型
    QStringList groupIdList;
    groupIdList.append(m_groupNumberId);
    m_restAPI->getGroupInfo(groupIdList, [this, memberId, memberName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
        if (errorCode != 0) {
            QString errorMsg = QString("获取群组信息失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
            qDebug() << errorMsg;
            QMessageBox::critical(this, "错误", errorMsg);
            return;
        }
        
        // 检查群组类型
        QJsonArray groupArray = result["GroupInfo"].toArray();
        if (groupArray.isEmpty()) {
            QMessageBox::critical(this, "错误", "无法获取群组信息！");
            return;
        }
        
        QJsonObject groupInfo = groupArray[0].toObject();
        QString groupType = groupInfo["Type"].toString();
        
        // Private群组不支持设置管理员
        if (groupType == "Private") {
            QMessageBox::warning(this, "不支持的操作", 
                "私有群（Private）不支持设置管理员功能。\n\n只有公开群（Public）、聊天室（ChatRoom）和直播群（AVChatRoom）支持设置管理员。");
            return;
        }
        
        // 群组类型支持设置管理员，继续执行设置操作
        m_restAPI->modifyGroupMemberInfo(m_groupNumberId, memberId, "Admin", -1, QString(),
            [this, memberId, memberName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
                if (errorCode != 0) {
                    QString errorMsg;
                    // 特殊处理10004错误码（群组类型不支持设置管理员）
                    if (errorCode == 10004) {
                        errorMsg = QString("设置管理员失败\n错误码: %1\n错误描述: %2\n\n该群组类型不支持设置管理员功能。\n只有公开群（Public）、聊天室（ChatRoom）和直播群（AVChatRoom）支持设置管理员。").arg(errorCode).arg(errorDesc);
                    } else {
                        errorMsg = QString("设置管理员失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                    }
                    qDebug() << errorMsg;
                    QMessageBox::critical(this, "设置失败", errorMsg);
                    return;
                }
            
            qDebug() << "REST API设置管理员成功，成员ID:" << memberId;
            
            // 更新本地成员列表中的角色
            for (auto& member : m_groupMemberInfo) {
                if (member.member_id == memberId) {
                    member.member_role = "管理员";
                    break;
                }
            }
            
            // 刷新UI
            InitGroupMember(m_groupNumberId, m_groupMemberInfo);
            
            // REST API成功，现在调用自己的服务器接口
            sendSetAdminRoleRequestToServer(m_groupNumberId, memberId, "管理员");
            
            // 刷新成员列表（通知父窗口刷新）
            refreshMemberList(m_groupNumberId);
            
            QMessageBox::information(this, "成功", QString("已将 %1 设为管理员（班主任）！").arg(memberName));
        });
    });
}

void QGroupInfo::onCancelLeaderRequested(const QString& memberId)
{
    // 检查当前用户是否是群主
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    
    bool isOwner = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == currentUserId && member.member_role == "群主") {
            isOwner = true;
            break;
        }
    }
    
    if (!isOwner) {
        QMessageBox::warning(this, "权限不足", "只有群主可以取消管理员！");
        return;
    }
    
    // 检查成员是否是管理员
    bool isAdmin = false;
    QString memberName;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == memberId) {
            memberName = member.member_name;
            if (member.member_role == "管理员") {
                isAdmin = true;
            }
            break;
        }
    }
    
    if (!isAdmin) {
        QMessageBox::information(this, "提示", "该成员不是管理员！");
        return;
    }
    
    // 检查REST API是否初始化
    if (!m_restAPI) {
        QMessageBox::critical(this, "错误", "REST API未初始化！");
        return;
    }
    
    // 设置管理员账号信息
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    } else {
        QMessageBox::critical(this, "错误", "管理员账号未设置！");
        return;
    }
    
    // 先获取群组信息，检查群组类型
    QStringList groupIdList;
    groupIdList.append(m_groupNumberId);
    m_restAPI->getGroupInfo(groupIdList, [this, memberId, memberName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
        if (errorCode != 0) {
            QString errorMsg = QString("获取群组信息失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
            qDebug() << errorMsg;
            QMessageBox::critical(this, "错误", errorMsg);
            return;
        }
        
        // 检查群组类型
        QJsonArray groupArray = result["GroupInfo"].toArray();
        if (groupArray.isEmpty()) {
            QMessageBox::critical(this, "错误", "无法获取群组信息！");
            return;
        }
        
        QJsonObject groupInfo = groupArray[0].toObject();
        QString groupType = groupInfo["Type"].toString();
        
        // Private群组不支持取消管理员
        if (groupType == "Private") {
            QMessageBox::warning(this, "不支持的操作", 
                "私有群（Private）不支持管理员功能。\n\n只有公开群（Public）、聊天室（ChatRoom）和直播群（AVChatRoom）支持管理员功能。");
            return;
        }
        
        // 群组类型支持管理员功能，继续执行取消操作
        m_restAPI->modifyGroupMemberInfo(m_groupNumberId, memberId, "Member", -1, QString(),
            [this, memberId, memberName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
                if (errorCode != 0) {
                    QString errorMsg;
                    // 特殊处理10004错误码（群组类型不支持设置管理员）
                    if (errorCode == 10004) {
                        errorMsg = QString("取消管理员失败\n错误码: %1\n错误描述: %2\n\n该群组类型不支持管理员功能。\n只有公开群（Public）、聊天室（ChatRoom）和直播群（AVChatRoom）支持管理员功能。").arg(errorCode).arg(errorDesc);
                    } else {
                        errorMsg = QString("取消管理员失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                    }
                    qDebug() << errorMsg;
                    QMessageBox::critical(this, "取消失败", errorMsg);
                    return;
                }
            
            qDebug() << "REST API取消管理员成功，成员ID:" << memberId;
            
            // 更新本地成员列表中的角色
            for (auto& member : m_groupMemberInfo) {
                if (member.member_id == memberId) {
                    member.member_role = "成员";
                    break;
                }
            }
            
            // 刷新UI
            InitGroupMember(m_groupNumberId, m_groupMemberInfo);
            
            // REST API成功，现在调用自己的服务器接口
            sendSetAdminRoleRequestToServer(m_groupNumberId, memberId, "成员");
            
            // 刷新成员列表（通知父窗口刷新）
            refreshMemberList(m_groupNumberId);
            
            QMessageBox::information(this, "成功", QString("已取消 %1 的管理员身份（班主任）！").arg(memberName));
        });
    });
}

void QGroupInfo::sendSetAdminRoleRequestToServer(const QString& groupId, const QString& memberId, const QString& role)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["user_id"] = memberId;
    requestData["role"] = role; // "管理员" 或 "成员"
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器
    QString url = "http://47.100.126.194:5000/groups/set_admin_role";
    
    // 使用QNetworkAccessManager发送POST请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->post(networkRequest, jsonData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "设置管理员角色服务器响应:" << QString::fromUtf8(response);
            
            // 解析响应
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                if (obj["code"].toInt() == 200) {
                    qDebug() << "设置管理员角色请求已发送到服务器，群组ID:" << groupId << "，成员ID:" << memberId << "，角色:" << role;
                } else {
                    QString message = obj["message"].toString();
                    qDebug() << "服务器返回错误:" << message;
                    // 不显示错误消息框，因为腾讯IM已经成功，只是服务器同步失败
                }
            } else {
                qDebug() << "解析服务器响应失败";
            }
        } else {
            qDebug() << "发送设置管理员角色请求到服务器失败:" << reply->errorString();
            // 不显示错误消息框，因为腾讯IM已经成功，只是服务器同步失败
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void QGroupInfo::transferOwnerAndQuit(const QString& newOwnerId, const QString& newOwnerName)
{
    // 检查REST API是否初始化
    if (!m_restAPI) {
        QMessageBox::critical(this, "错误", "REST API未初始化！");
        return;
    }
    
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    
    // 设置管理员账号信息
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    } else {
        QMessageBox::critical(this, "错误", "管理员账号未设置！");
        return;
    }
    
    // 调用腾讯IM REST API转让群主
    m_restAPI->changeGroupOwner(m_groupNumberId, newOwnerId,
        [this, newOwnerId, newOwnerName, currentUserId](int errorCode, const QString& errorDesc, const QJsonObject& result) {
            if (errorCode != 0) {
                QString errorMsg = QString("转让群主失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                qDebug() << errorMsg;
                QMessageBox::critical(this, "转让失败", errorMsg);
                return;
            }
            
            qDebug() << "REST API转让群主成功，新群主ID:" << newOwnerId;
            
            // 更新本地成员列表中的角色
            for (auto& member : m_groupMemberInfo) {
                if (member.member_id == newOwnerId) {
                    member.member_role = "群主";
                } else if (member.member_id == currentUserId) {
                    member.member_role = "成员"; // 原群主变为普通成员
                }
            }
            
            // 调用自己的服务器接口，修改新管理员为群主，并让原群主退出群组
            // 服务器接口会处理这两个操作，所以这里只需要调用服务器接口
            sendTransferOwnerRequestToServer(m_groupNumberId, currentUserId, newOwnerId);
            
            // 转让成功后，原群主退出群聊（腾讯IM层面）
            m_restAPI->quitGroup(m_groupNumberId, currentUserId,
                [this, currentUserId, newOwnerName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
                    if (errorCode != 0) {
                        QString errorMsg = QString("退出群聊失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                        qDebug() << errorMsg;
                        QMessageBox::critical(this, "退出失败", errorMsg);
                        return;
                    }
                    
                    qDebug() << "REST API退出群聊成功，用户ID:" << currentUserId;
                    
                    // 立即从本地成员列表中移除当前用户
                    QVector<GroupMemberInfo> updatedMemberList;
                    for (const auto& member : m_groupMemberInfo) {
                        if (member.member_id != currentUserId) {
                            updatedMemberList.append(member);
                        }
                    }
                    m_groupMemberInfo = updatedMemberList;
                    
                    // 立即更新UI（移除退出的成员）
                    InitGroupMember(m_groupNumberId, m_groupMemberInfo);
                    
                    // 刷新成员列表（通知父窗口刷新）
                    refreshMemberList(m_groupNumberId);
                    
                    // 发出成员退出群聊信号，通知父窗口刷新群列表
                    emit this->memberLeftGroup(m_groupNumberId, currentUserId);
                    
                    // 显示成功消息
                    QMessageBox::information(this, "退出成功", 
                        QString("已成功退出群聊 \"%1\"！\n群主身份已转让给 %2。").arg(m_groupName).arg(newOwnerName));
                    
                    // 关闭对话框
                    this->accept();
                });
        });
}

void QGroupInfo::sendTransferOwnerRequestToServer(const QString& groupId, const QString& oldOwnerId, const QString& newOwnerId)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["old_owner_id"] = oldOwnerId;
    requestData["new_owner_id"] = newOwnerId;
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器
    QString url = "http://47.100.126.194:5000/groups/transfer_owner";
    
    // 使用QNetworkAccessManager发送POST请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->post(networkRequest, jsonData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "转让群主服务器响应:" << QString::fromUtf8(response);
            
            // 解析响应
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                int code = obj["code"].toInt();
                QString message = obj["message"].toString();
                
                if (code == 200) {
                    // 解析data字段
                    if (obj.contains("data") && obj["data"].isObject()) {
                        QJsonObject dataObj = obj["data"].toObject();
                        QString groupIdFromServer = dataObj["group_id"].toString();
                        QString groupNameFromServer = dataObj["group_name"].toString();
                        QString oldOwnerIdFromServer = dataObj["old_owner_id"].toString();
                        QString newOwnerIdFromServer = dataObj["new_owner_id"].toString();
                        QString newOwnerNameFromServer = dataObj["new_owner_name"].toString();
                        
                        qDebug() << "转让群主请求已发送到服务器成功";
                        qDebug() << "群组ID:" << groupIdFromServer;
                        qDebug() << "群组名称:" << groupNameFromServer;
                        qDebug() << "原群主ID:" << oldOwnerIdFromServer;
                        qDebug() << "新群主ID:" << newOwnerIdFromServer;
                        qDebug() << "新群主名称:" << newOwnerNameFromServer;
                    } else {
                        qDebug() << "转让群主请求已发送到服务器，群组ID:" << groupId << "，原群主ID:" << oldOwnerId << "，新群主ID:" << newOwnerId;
                    }
                } else {
                    qDebug() << "服务器返回错误，code:" << code << "，message:" << message;
                    // 不显示错误消息框，因为腾讯IM已经成功，只是服务器同步失败
                }
            } else {
                qDebug() << "解析服务器响应失败";
            }
        } else {
            qDebug() << "发送转让群主请求到服务器失败:" << reply->errorString();
            // 不显示错误消息框，因为腾讯IM已经成功，只是服务器同步失败
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

QGroupInfo::~QGroupInfo()
{}

void QGroupInfo::enterEvent(QEvent* event)
{
    QDialog::enterEvent(event);
    if (m_closeButton)
        m_closeButton->show();
}

void QGroupInfo::leaveEvent(QEvent* event)
{
    QDialog::leaveEvent(event);
    if (m_closeButton)
        m_closeButton->hide();
}

void QGroupInfo::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    if (m_closeButton)
        m_closeButton->move(this->width() - 22, 0);
}

void QGroupInfo::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(event);
}

void QGroupInfo::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
    }
    QDialog::mouseMoveEvent(event);
}

void QGroupInfo::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QDialog::mouseReleaseEvent(event);
}

