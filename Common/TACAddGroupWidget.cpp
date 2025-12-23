#pragma execution_character_set("utf-8")
#include "TACAddGroupWidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
//#include "common.h"

#define WIDGET_BACKGROUND_COLOR_C QColor(50,50,50,200)
#define WIDGET_BORDER_COLOR_C QColor(255,255,255,25)
#define WIDGET_BORDER_WIDTH_C 1

TACAddGroupWidget1::TACAddGroupWidget1(QWidget *parent, TaQTWebSocket* pWs)
	: TAFloatingWidget(parent)
{
	m_searchDlg = new SearchDialog(this);
	// 连接SearchDialog的加入群组成功信号，转发给FriendGroupDialog
	connect(m_searchDlg, &SearchDialog::groupJoined, this, &TACAddGroupWidget1::groupJoined);
	m_classTeacherDlg = new ClassTeacherDialog(this, pWs);
	// 连接ClassTeacherDialog的群组创建成功信号，转发给FriendGroupDialog
	connect(m_classTeacherDlg, &ClassTeacherDialog::groupCreated, this, &TACAddGroupWidget1::groupCreated);
	// 连接ClassTeacherDialog的ScheduleDialog相关信号，转发给FriendGroupDialog
	connect(m_classTeacherDlg, &ClassTeacherDialog::scheduleDialogNeeded, this, &TACAddGroupWidget1::scheduleDialogNeeded);
	connect(m_classTeacherDlg, &ClassTeacherDialog::scheduleDialogRefreshNeeded, this, &TACAddGroupWidget1::scheduleDialogRefreshNeeded);
	
	m_normalGroupDlg = new NormalGroupDialog(this);
	// 连接NormalGroupDialog的群组创建成功信号，转发给FriendGroupDialog
	connect(m_normalGroupDlg, &NormalGroupDialog::groupCreated, this, &TACAddGroupWidget1::groupCreated);
	this->setObjectName("TACDesktopManagerWidget");

	this->setBackgroundColor(WIDGET_BACKGROUND_COLOR_C);
	this->setBorderColor(WIDGET_BORDER_COLOR_C);
	this->setBorderWidth(0);
	this->setRadius(12);
	//this->visibleCloseButton(true);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(24, 24, 24, 24);
	layout->setSpacing(16);

	QLabel* label = new QLabel("", this);
	label->setAlignment(Qt::AlignCenter);
	layout->addWidget(label);

	QPushButton* fileManagerButton = new QPushButton("创建班级群", this);
	layout->addWidget(fileManagerButton);
	connect(fileManagerButton, &QPushButton::clicked, this, [=]() {
		if (m_classTeacherDlg && m_classTeacherDlg->isHidden())
		{
			m_classTeacherDlg->show();
		}
		});

	QPushButton* createFolderButton = new QPushButton("创建普通群", this);
	layout->addWidget(createFolderButton);
	connect(createFolderButton, &QPushButton::clicked, this, [=]() {
		if (m_normalGroupDlg && m_normalGroupDlg->isHidden())
		{
			m_normalGroupDlg->show();
		}
	});

	QPushButton* countdowButton = new QPushButton("加好友/群", this);
	layout->addWidget(countdowButton);

	connect(countdowButton, &QPushButton::clicked, this, [=]() {
		if (m_searchDlg && m_searchDlg->isHidden())
		{
			m_searchDlg->show();
		}
	});

	setLayout(layout);
	resize(230, 380);

}

TACAddGroupWidget1::~TACAddGroupWidget1()
{}

void TACAddGroupWidget1::InitWebSocket()
{
	if (m_classTeacherDlg)
	{
		m_classTeacherDlg->InitWebSocket();
	}
}

void TACAddGroupWidget1::InitData(QSet<QString> setclassId)
{
	if (m_classTeacherDlg)
	{
		m_classTeacherDlg->InitData(setclassId);
	}
}

QVector<QString> TACAddGroupWidget1::getNoticeMsg()
{
	if (m_classTeacherDlg)
	{
		return m_classTeacherDlg->getNoticeMsg();
	}
	return QVector<QString>();
}

//void TACAddGroupWidget1::showEvent(QShowEvent* event)
//{
//	QWidget::showEvent(event);
//	initShow();
//}
//
//void TACAddGroupWidget1::resizeEvent(QResizeEvent* event)
//{
//	QWidget::resizeEvent(event);
//	initShow();
//}

void TACAddGroupWidget1::initShow()
{
	//QRect rect = this->getScreenGeometryWithTaskbar();
	//if (rect.isEmpty())
	//	return;
	//QSize windowSize = this->size();
	//int x = rect.x() + (rect.width() - windowSize.width()) * 5 / 6;
	////int x = rect.x() + rect.width() - windowSize.width();
	//int y = rect.y() + rect.height() - windowSize.height() - 140;
	//this->move(x, y);
}