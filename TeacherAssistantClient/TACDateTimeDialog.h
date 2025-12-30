#pragma once

#include "TADialog.h"
#include <QTimer>
#include <QPushButton>

class TACDateTimeDialog : public TADialog
{
	Q_OBJECT

public:
	TACDateTimeDialog(QWidget *parent);
	~TACDateTimeDialog();
signals:
	void updateType(int type);
	void switchToMinimalMode(); // 切换到极简模式的信号

protected:
	void closeEvent(QCloseEvent* event) override;

private slots:
	void onMinimalButtonClicked(); // 极简按钮点击槽

private:
	QHBoxLayout* typeLayout;
	QWidget* contentWidget;
	QLabel* upContentLabel;
	QLabel* downContentLabel;
	QTimer* timer;
	QPushButton* minimalButton; // 极简按钮
};
