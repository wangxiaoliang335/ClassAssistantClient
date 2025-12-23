#pragma execution_character_set("utf-8")
#include <QHBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QIcon>
#include <QLabel>
#include <QSize>
#include "TACNavigationBarWidget.h"
#define BUTTON_SIZE QSize(60,60)
#define ICON_SIZE QSize(26,26)
TACNavigationBarWidget::TACNavigationBarWidget(QWidget *parent)
	: TAFloatingWidget(parent)
{
    this->setObjectName("TACNavigationBarWidget");

    m_className = "一班(班主任) ";
    QHBoxLayout* layout = new QHBoxLayout(this);
    QButtonGroup* buttonGroup = new QButtonGroup(this);


    QPushButton* folderButton = new QPushButton(this);
    folderButton->setFixedSize(BUTTON_SIZE);
    folderButton->setIcon(QIcon(":/res/img/folder.png"));
    folderButton->setIconSize(ICON_SIZE);
    folderButton->setCheckable(true);
    connect(folderButton, &QPushButton::toggled, this, [=](bool checked) {
        emit navType(TACNavigationBarWidgetType::FOLDER,checked);
        });
    //buttonGroup->addButton(folderButton);
    layout->addWidget(folderButton);

    /*QPushButton* messageButton = new QPushButton(this);
    messageButton->setFixedSize(BUTTON_SIZE);
    messageButton->setIcon(QIcon(":/res/img/message.png"));
    messageButton->setIconSize(ICON_SIZE);
    connect(messageButton, &QPushButton::clicked, this, [=]() {
        emit navType(TACNavigationBarWidgetType::MESSAGE);
        });
    buttonGroup->addButton(messageButton);
    layout->addWidget(messageButton);

    QPushButton* phoneButton = new QPushButton(this);
    phoneButton->setFixedSize(BUTTON_SIZE);
    phoneButton->setIcon(QIcon(":/res/img/phone.png"));
    phoneButton->setIconSize(ICON_SIZE);
    connect(phoneButton, &QPushButton::clicked, this, [=]() {
        emit navType(TACNavigationBarWidgetType::IM);
        });
    buttonGroup->addButton(phoneButton);
    layout->addWidget(phoneButton);*/

    QPushButton* homeworkButton = new QPushButton(this);
    homeworkButton->setFixedSize(BUTTON_SIZE);
    homeworkButton->setIcon(QIcon(":/res/img/homework.png"));
    homeworkButton->setIconSize(ICON_SIZE);
    connect(homeworkButton, &QPushButton::clicked, this, [=]() {
        emit navType(TACNavigationBarWidgetType::HOMEWORK);
        });
    buttonGroup->addButton(homeworkButton);
    layout->addWidget(homeworkButton);


    //QPushButton* editButton = new QPushButton(this);
    //editButton->setFixedSize(BUTTON_SIZE);
    //editButton->setIcon(QIcon(":/res/img/edit.png"));
    //editButton->setIconSize(ICON_SIZE);
    //connect(editButton, &QPushButton::clicked, this, [=]() {
    //    emit navType(TACNavigationBarWidgetType::PREPARE_CLASS);
    //    });
    //buttonGroup->addButton(editButton);
    //layout->addWidget(editButton);

	QPushButton* classButton = new QPushButton(this);
    classButton->setFixedSize(BUTTON_SIZE);
    classButton->setIcon(QIcon(":/res/img/home_bottom_ic_class@2x.png"));
    classButton->setIconSize(ICON_SIZE);
	connect(classButton, &QPushButton::clicked, this, [=]() {
		emit navType(TACNavigationBarWidgetType::CLASS_GROUP);
		});
	buttonGroup->addButton(classButton);
	layout->addWidget(classButton);

    QPushButton* scheduleButton = new QPushButton(this);
    scheduleButton->setFixedSize(BUTTON_SIZE);
    scheduleButton->setIcon(QIcon(":/res/img/home_bottom_ic_crowd@2x.png"));
    scheduleButton->setIconSize(ICON_SIZE);
    connect(scheduleButton, &QPushButton::clicked, this, [=]() {
        emit navType(TACNavigationBarWidgetType::CLASS_SCHEDULE);
        });
    buttonGroup->addButton(scheduleButton);
    layout->addWidget(scheduleButton);

    QPushButton* userButton = new QPushButton(this);
    userButton->setFixedSize(BUTTON_SIZE);
    userButton->setIcon(QIcon(":/res/img/user.png"));
    userButton->setIconSize(ICON_SIZE);
    connect(userButton, &QPushButton::clicked, this, [=]() {
        emit navType(TACNavigationBarWidgetType::USER);
        });
    buttonGroup->addButton(userButton);
    layout->addWidget(userButton);

    QLabel* separatorLineLabel0 = new QLabel(this);
    separatorLineLabel0->setObjectName("separatorLineLabel");
    separatorLineLabel0->setFixedSize(QSize(1, 30));
    layout->addWidget(separatorLineLabel0);

    /*QPushButton* wallpaperButton = new QPushButton(this);
    wallpaperButton->setFixedSize(BUTTON_SIZE);
    wallpaperButton->setIcon(QIcon(":/res/img/palette.png"));
    connect(wallpaperButton, &QPushButton::clicked, this, [=]() {
        emit navType(TACNavigationBarWidgetType::WALLPAPER);
        });
    wallpaperButton->setIconSize(ICON_SIZE);
    buttonGroup->addButton(wallpaperButton);
    layout->addWidget(wallpaperButton);

    QPushButton* calendarButton = new QPushButton(this);
    calendarButton->setFixedSize(BUTTON_SIZE);
    calendarButton->setIcon(QIcon(":/res/img/calendar.png"));
    calendarButton->setIconSize(ICON_SIZE);
    connect(calendarButton, &QPushButton::clicked, this, [=]() {
        emit navType(TACNavigationBarWidgetType::CALENDAR);
        });
    buttonGroup->addButton(calendarButton);
    layout->addWidget(calendarButton);

    QPushButton* timerButton = new QPushButton(this);
    timerButton->setFixedSize(BUTTON_SIZE);
    timerButton->setIcon(QIcon(":/res/img/timer.png"));
    timerButton->setIconSize(ICON_SIZE);
    connect(timerButton, &QPushButton::clicked, this, [=]() {
        emit navType(TACNavigationBarWidgetType::TIMER);
    });
    buttonGroup->addButton(timerButton);
    layout->addWidget(timerButton);*/

    QPushButton* classNameButton = new QPushButton(this);
    classNameButton->setText(m_className);
    classNameButton->setObjectName("classNameButton");
    classNameButton->setIcon(QIcon(":/res/img/arrow-down.png"));
    classNameButton->setLayoutDirection(Qt::RightToLeft);
    
    layout->addWidget(classNameButton);

    QLabel* separatorLineLabel = new QLabel(this);
    separatorLineLabel->setObjectName("separatorLineLabel");
    separatorLineLabel->setFixedSize(QSize(1,30));
    layout->addWidget(separatorLineLabel);

    QPushButton* teacherIconButton = new QPushButton(this);
    teacherIconButton->setFixedSize(BUTTON_SIZE);
    teacherIconButton->setIcon(QIcon(":/res/img/user1.png"));
    teacherIconButton->setIconSize(BUTTON_SIZE);
    layout->addWidget(teacherIconButton);
    connect(teacherIconButton, &QPushButton::clicked, this, [=]() {
        emit navType(TACNavigationBarWidgetType::USER1);
        });

    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignLeft);
    setLayout(layout);
    resize(668, 88);
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(45);
}

TACNavigationBarWidget::~TACNavigationBarWidget()
{}

void TACNavigationBarWidget::showEvent(QShowEvent * event)
{
    QWidget::showEvent(event);
    initShow();
}

void TACNavigationBarWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    initShow();
}

void TACNavigationBarWidget::initShow()
{
    QRect rect = this->getScreenGeometryWithTaskbar();
    if (rect.isEmpty())
        return;
    QSize windowSize = this->size();
    int x = rect.x() + (rect.width() - windowSize.width()) / 2;
    int y = rect.y() + rect.height() - windowSize.height() - 60;
    this->move(x, y);
}
