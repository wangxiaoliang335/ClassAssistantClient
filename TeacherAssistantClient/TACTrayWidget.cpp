#pragma execution_character_set("utf-8")
#include "TACTrayWidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include "common.h"
#include "../Common/CommonInfo.h"
TACTrayWidget::TACTrayWidget(QWidget *parent)
	: TAFloatingWidget(parent)
{
	this->setObjectName("TACTrayWidget");

	this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
	this->setBorderColor(WIDGET_BORDER_COLOR);
	this->setBorderWidth(WIDGET_BORDER_WIDTH);
	this->setRadius(15);
	this->visibleCloseButton(false);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(5);


	m_classInfoButton = new QPushButton(QString::fromUtf8(u8"班级信息"), this);
	m_classInfoButton->setCheckable(true);
	m_classInfoButton->setEnabled(false); // 默认禁用，等待用户信息加载后更新
	layout->addWidget(m_classInfoButton);
	
	// LambdaΪ¼
	connect(m_classInfoButton, &QPushButton::toggled, this, [=](bool checked) {
		emit navClassInfo(checked);
		});

	m_desktopManagerButton = new QPushButton(QString::fromUtf8(u8"桌面管理"), this);
	layout->addWidget(m_desktopManagerButton);
	m_desktopManagerButton->setCheckable(true);

	// LambdaΪ¼
	connect(m_desktopManagerButton, &QPushButton::toggled, this, [=](bool checked) {
		emit navDesktopManager(checked);
	});

	m_classGroupButton = new QPushButton(QString::fromUtf8(u8"班级群"), this);
	layout->addWidget(m_classGroupButton);
	m_classGroupButton->setCheckable(true);

	connect(m_classGroupButton, &QPushButton::toggled, this, [=](bool checked) {
		emit navClassGroup(checked);
	});

	//QPushButton* countdowButton = new QPushButton("Խ", this);
	//layout->addWidget(countdowButton);

	//QPushButton* timeButton = new QPushButton("Ϣ", this);
	//layout->addWidget(timeButton);

	setLayout(layout);
	resize(230, 180); // 调整高度以容纳3个按钮

}

TACTrayWidget::~TACTrayWidget()
{}
void TACTrayWidget::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	initShow();
}

void TACTrayWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	initShow();
}
void TACTrayWidget::initShow()
{
}

void TACTrayWidget::updateAdminButtonState()
{
	if (!m_classInfoButton) {
		return;
	}
	
	// 根据管理员字段控制按钮启用状态
	UserInfo userInfo = CommonInfo::GetData();
	bool isAdmin = (userInfo.strIsAdministrator == "1" ||
		userInfo.strIsAdministrator.compare(QString::fromUtf8(u8"是"), Qt::CaseInsensitive) == 0 ||
		userInfo.strIsAdministrator.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0);
	m_classInfoButton->setEnabled(isAdmin);
}

void TACTrayWidget::updateClassInfoButtonState()
{
	if (!m_classInfoButton) {
		return;
	}
	
	// 检查班级登录信息是否已加载
	ClassLoginInfo loginInfo = CommonInfo::GetClassLoginInfo();
	if (loginInfo.isLoggedIn()) {
		// 班级信息已加载，启用按钮
		m_classInfoButton->setEnabled(true);
	} else {
		// 班级信息未加载，禁用按钮
		m_classInfoButton->setEnabled(false);
	}
}
