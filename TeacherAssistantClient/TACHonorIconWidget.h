#pragma once

#include "TAFloatingWidget.h"
#include <QPainter>
#include <QPixmap>
#include <QPaintEvent>
#include <QMouseEvent>

class TACHonorIconWidget : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACHonorIconWidget(QWidget* parent);
	~TACHonorIconWidget();
	void updateLogo(const QString& fileName);
	
protected:
	void initShow() override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
	
signals:
	void doubleClicked();

private:
	QString m_fileName;
};

