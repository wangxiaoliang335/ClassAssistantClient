#pragma once

#include "TAFloatingWidget.h"

class TACDesktopManagerWidget  : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACDesktopManagerWidget(QWidget *parent);
	~TACDesktopManagerWidget();
	void initSchoolCourseScheduleData(); // 初始化学校课程表数据（在 schoolId 准备好后调用）
protected:
	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	void initShow();
	void initSchoolCourseSchedule(); // 初始化学校课程表对话框

private:
	class TASchoolCalendarWidget* m_schoolCalendarWidget = nullptr;
	class TACTeacherCourseScheduleWindow* m_teacherCourseScheduleWindow = nullptr; // 教师课程表窗口（mainwindow.cpp 的实现）
	class SchoolCourseScheduleDialog* m_schoolCourseScheduleDialog = nullptr; // 学校/年级课程表总览
	bool m_schoolCourseScheduleInitialized = false; // 标记课程表是否已初始化
};
