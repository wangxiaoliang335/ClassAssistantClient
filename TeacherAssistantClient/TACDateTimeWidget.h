#pragma once

#include "TAFloatingWidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QMouseEvent>

class TACDateTimeWidget  : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACDateTimeWidget(QWidget *parent);
	~TACDateTimeWidget();
	//00000011,00000001,00000010
	void setType(int type);
	void resetToDefaultPosition(); // 重置到默认位置的公共方法
signals:
	void doubleClicked(); // 双击切换到完整模式的信号

protected:
	void initShow() override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	
private:
	
	QLabel* upContentLabel;
	QLabel* downContentLabel;
	QTimer* timer;
	int m_type;
	
};
