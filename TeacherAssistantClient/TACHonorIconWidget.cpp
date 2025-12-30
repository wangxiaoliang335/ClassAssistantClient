#pragma execution_character_set("utf-8")
#include "TACHonorIconWidget.h"
#include "common.h"
#include <QPixmap>

TACHonorIconWidget::TACHonorIconWidget(QWidget* parent) 
	: TAFloatingWidget(parent)
{
	this->setObjectName("TACHonorIconWidget");
	
	// 背景透明，和学校logo风格一样
	this->setBorderColor(WIDGET_BORDER_COLOR);
	this->setBorderWidth(WIDGET_BORDER_WIDTH);
	this->setRadius(15);
	this->resize(140, 70); // 荣誉图标大小和学校logo一样，学校logo是140x70
	
	setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}

TACHonorIconWidget::~TACHonorIconWidget()
{
}

void TACHonorIconWidget::initShow()
{
	// 注意：对于荣誉图标，位置会在TACMainDialog中手动设置
	// 这里只提供默认位置（如果位置没有被手动设置的话）
	QRect rect = this->getScreenGeometryWithTaskbar();
	if (rect.isEmpty())
		return;
	// 默认位置：左上角（会被主窗口代码覆盖）
	int x = 1700;
	int y = 980;
	this->move(x, y);
}

void TACHonorIconWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		emit doubleClicked();
	}
}

void TACHonorIconWidget::paintEvent(QPaintEvent* event)
{
	TAFloatingWidget::paintEvent(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	if (!m_fileName.isEmpty())
	{
		QPixmap m_backgroundPixmap = QPixmap(m_fileName);
		QPixmap scaled = m_backgroundPixmap.scaled(
			size(),
			Qt::KeepAspectRatio,
			Qt::SmoothTransformation);
		QRect r = scaled.rect();
		r.moveCenter(rect().center());
		painter.drawPixmap(r, scaled);
	}
}

void TACHonorIconWidget::updateLogo(const QString& fileName)
{
	m_fileName = fileName;
	update();
}

