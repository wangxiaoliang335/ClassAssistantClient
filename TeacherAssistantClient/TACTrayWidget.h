#pragma once

#include "TAFloatingWidget.h"

class TACTrayWidget  : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACTrayWidget(QWidget *parent);
	~TACTrayWidget();
	void updateAdminButtonState(); // 更新管理员按钮状态
	void updateClassInfoButtonState(); // 更新班级信息按钮状态
protected:
	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	void initShow();
	QPushButton* m_classInfoButton;     // 班级信息按钮
	QPushButton* m_desktopManagerButton; // 桌面管理按钮
	QPushButton* m_classGroupButton;    // 班级群按钮
signals:
	void navClassInfo(bool checked = false);    // 班级信息
	void navDesktopManager(bool checked = false); // 桌面管理
	void navClassGroup(bool checked = false);    // 班级群
};
