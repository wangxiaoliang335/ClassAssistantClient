#pragma once

#include "TAFloatingWidget.h"
#include "common.h"
enum TACNavigationBarWidgetType
{
	FOLDER = 0,
	MESSAGE,
	IM,
	HOMEWORK,
	PREPARE_CLASS,
	CLASS_GROUP,
	CLASS_SCHEDULE,
	USER,
	WALLPAPER,
	CALENDAR,
	TIMER,
	USER1
};
class TACNavigationBarWidget  : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACNavigationBarWidget(QWidget *parent);
	~TACNavigationBarWidget();
protected:
	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

signals:
	void navType(TACNavigationBarWidgetType type,bool checked=false);

public:
	// 设置"课前准备"功能键的可见性
	void setPrepareClassButtonVisible(bool visible);
	// 设置"家庭作业"功能键的可见性
	void setHomeworkButtonVisible(bool visible);
	// 设置"今日课表"功能键的可见性
	void setTodayScheduleButtonVisible(bool visible);
	// 设置"值日表"功能键的可见性
	void setDutyRosterButtonVisible(bool visible);

private:
	void initShow();
	void updatePrepareClassButtonVisibility(); // 根据群组设置更新按钮可见性

private:
	QString m_className;
	QPushButton* m_prepareClassButton = nullptr; // 课前准备功能键
	QPushButton* m_homeworkButton = nullptr; // 家庭作业功能键
	QPushButton* m_todayScheduleButton = nullptr; // 今日课表功能键
	QPushButton* m_dutyRosterButton = nullptr; // 值日表功能键
};
