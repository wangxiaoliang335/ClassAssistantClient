#pragma once

#include "TADialog.h"
#include "TACalendarDialog.h"
#include <QLineEdit>
#include <QPushButton>
#include <QDate>
#include <QMouseEvent>

class TACCountDownDialog  : public TADialog
{
	Q_OBJECT

public:
	TACCountDownDialog(QWidget *parent);
	~TACCountDownDialog();
	int daysLeft();
	QString content();
	void setTargetDate(const QDate& date);
	QDate getTargetDate() const;
	void setHeaderText(const QString& text);
	QString getHeaderText() const;

signals:
	void switchToMinimalMode(); // 切换到极简模式的信号

protected:
	void showEvent(QShowEvent* event) override;
	bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
	void onMinimalButtonClicked(); // 极简按钮点击槽
	void onCalendarDateSelected(const QDate& date); // 日历选择日期
	void onHeaderEditFinished(); // 提示文本编辑完成

private:
	void setupUI();
	void updateDisplay();
	void editHeaderText(); // 编辑提示文本
	void showCalendar(); // 显示日历选择器

	QLabel* countDownLabel;
	QPushButton* minimalButton; // 极简按钮
	QDate targetDate; // 目标日期
	QLabel* targetDateLabel; // 目标日期显示标签
	QLabel* headerLabel; // 顶部标题标签（如"结婚纪念日还有"）
	QLineEdit* headerLineEdit; // 用于编辑提示文本的输入框
	TACalendarWidget* calendar;
	QWidget* headerWidget; // 顶部标题区域
};
