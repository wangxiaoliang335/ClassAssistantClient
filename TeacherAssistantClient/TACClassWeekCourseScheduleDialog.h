#pragma once

#include "TABaseDialog.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include "TACToolWidget.h"
#include "TAHttpHandler.h"
#include "CommonInfo.h"
#include "TACSubjectSelectDialog.h"

class TACClassWeekCourseScheduleDialog  : public TABaseDialog
{
	Q_OBJECT

public:
	TACClassWeekCourseScheduleDialog(QWidget *parent);
	~TACClassWeekCourseScheduleDialog();
	void updateClassList();
	void fetchCourseScheduleFromServer(const QString& classId); // 从服务器获取课程表
	void saveCourseScheduleToServer(); // 保存课程表到服务器

public slots:
	void classClick();
private:
	void init();
	void handleScheduleResponse(const QString& resp); // 处理服务器返回的课程表响应
	void applySchedule(const QStringList& days, const QStringList& times, const QJsonArray& cells); // 应用课程表数据到界面
	QString currentTermString() const; // 获取当前学期字符串
	QStringList jsonArrayToStringList(const QJsonArray& arr) const; // 将JSON数组转换为字符串列表
	bool looksLikeTimeText(const QString& text) const; // 判断文本是否像时间格式
private:
	QGridLayout* gridLayout;
	QPushButton* currentClassButton;
	QVector<QVector<QPushButton*>> classVecotr;
	TAHttpHandler* m_httpHandler;
	QPointer<TACSubjectSelectDialog> m_subjectSelectDialog; // 科目选择对话框
	QString m_currentClassId; // 当前班级ID
};
