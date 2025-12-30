#pragma execution_character_set("utf-8")
#include <QDateTime>
#include "TACDateTimeWidget.h"
#include "common.h"
TACDateTimeWidget::TACDateTimeWidget(QWidget *parent)
	: TAFloatingWidget(parent)
{
    this->setObjectName("TACDateTimeWidget");
    upContentLabel = new QLabel(this);
    upContentLabel->setAlignment(Qt::AlignCenter);
    upContentLabel->setObjectName("upContentLabel");

    downContentLabel = new QLabel(this);
    downContentLabel->setAlignment(Qt::AlignCenter);
    downContentLabel->setObjectName("downContentLabel");
    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->setAlignment(Qt::AlignCenter);
    vLayout->addWidget(upContentLabel);
    vLayout->addWidget(downContentLabel);
    this->setLayout(vLayout);

    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(30);
    this->setFixedSize(QSize(200, 126));

   
    m_type = 0b11; // 默认同时显示时间和日期（二进制 11 = 时间和日期都选中）
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]() {
        if (m_type == 0b11)
        {
            upContentLabel->show();
            upContentLabel->setText(QTime::currentTime().toString("hh:mm"));
            upContentLabel->setStyleSheet("font-weight: 500; font-size: 64px; color: #FFFFFF;");
            downContentLabel->show();
            QDate currentDate = QDate::currentDate();
            downContentLabel->setText(QString::fromUtf8("%1月%2日").arg(currentDate.month()).arg(currentDate.day()));
            downContentLabel->setStyleSheet("font-weight: 500; font-size: 20px; color: #FFFFFF;");
        }
        else if (m_type == 0b10)
        {
            upContentLabel->show();
            QDate currentDate = QDate::currentDate();
            upContentLabel->setText(QString::fromUtf8("%1月%2日").arg(currentDate.month()).arg(currentDate.day()));
            // 只显示日期时使用较小的字体
            upContentLabel->setStyleSheet("font-weight: 500; font-size: 32px; color: #FFFFFF;");
            downContentLabel->hide();
        }
        else
        {
            upContentLabel->show();
            upContentLabel->setText(QTime::currentTime().toString("hh:mm"));
            upContentLabel->setStyleSheet("font-weight: 500; font-size: 64px; color: #FFFFFF;");
            downContentLabel->hide();
        }
    });
    timer->start(1000);
}

TACDateTimeWidget::~TACDateTimeWidget()
{}
void TACDateTimeWidget::setType(int type)
{
    m_type = type;
}
void TACDateTimeWidget::initShow()
{
	resetToDefaultPosition();
}

void TACDateTimeWidget::resetToDefaultPosition()
{
	QRect rect = this->getScreenGeometryWithTaskbar();
	if (rect.isEmpty())
		return;
	QSize windowSize = this->size();
	int x = rect.x() + rect.width() - 30 - this->width();
	int y = rect.y() + 60;
	this->move(x, y);
}

void TACDateTimeWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		emit doubleClicked();
		event->accept();
		return;
	}
	TAFloatingWidget::mouseDoubleClickEvent(event);
}
