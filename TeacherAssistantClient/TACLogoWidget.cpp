#pragma execution_character_set("utf-8")
#include "TACLogoWidget.h"
#include "common.h"
#include <QPixmap>
#include <QHBoxLayout>
TACLogoWidget::TACLogoWidget(QWidget *parent)
	: TAFloatingWidget(parent)
{
    this->setObjectName("TACLogoWidget");
    
    this->resize(QSize(140, 70));
}

TACLogoWidget::~TACLogoWidget()
{}
void TACLogoWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (!m_fileName.isEmpty())
    {
        QPixmap m_backgroundPixmap = QPixmap(m_fileName);
        QPixmap scaled = m_backgroundPixmap.scaled(
            size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        QRect r = scaled.rect();
        r.moveCenter(rect().center());
        painter.drawPixmap(r, scaled);
    }
}
void TACLogoWidget::initShow()
{
    QRect rect = this->getScreenGeometryWithTaskbar();
    if (rect.isEmpty())
        return;
    int x = 50;
    int y = 55;
    this->move(x, y);
}
void TACLogoWidget::updateLogo(const QString & fileName)
{
    m_fileName = fileName;
    update();
}

QString TACLogoWidget::getLogoFileName() const
{
    return m_fileName;
}
void TACLogoWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked();
    }
}

TACSchoolLabelWidget::TACSchoolLabelWidget(QWidget* parent) : TAFloatingWidget(parent)
{
    this->setObjectName("TACSchoolLabelWidget");
    label = new QLabel("学校名称",this);
    label->setAlignment(Qt::AlignCenter);
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    this->setLayout(layout);

    this->setBackgroundColor(QColor(61, 64, 64));
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(30);
    this->resize(140,70);

}

TACSchoolLabelWidget::~TACSchoolLabelWidget()
{
}

void TACSchoolLabelWidget::setContent(const QString& text)
{
    label->setText(text);
}

void TACSchoolLabelWidget::initShow()
{
    QRect rect = this->getScreenGeometryWithTaskbar();
    if (rect.isEmpty())
        return;
    int x = 200;
    int y = 55;
    this->move(x, y);
}
void TACSchoolLabelWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked();
    }
}
TACClassLabelWidget::TACClassLabelWidget(QWidget* parent) : TAFloatingWidget(parent)
{
    this->setObjectName("TACClassLabelWidget");
    label = new QLabel("班级名称",this);
    label->setAlignment(Qt::AlignCenter);
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    this->setLayout(layout);

    this->setBackgroundColor(QColor(61, 64, 64));
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(30);
    this->resize(140, 70);
}

TACClassLabelWidget::~TACClassLabelWidget()
{
}

void TACClassLabelWidget::setContent(const QString& text)
{
    label->setText(text);
}

QString TACClassLabelWidget::getContent() const
{
    return label->text();
}
void TACClassLabelWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked();
    }
}
void TACClassLabelWidget::initShow()
{
    QRect rect = this->getScreenGeometryWithTaskbar();
    if (rect.isEmpty())
        return;
    // 学校logo: x=50, width=140, right=190
    // 缩小间隔，从170改为50
    int x = 240; // 190 + 50 = 240
    int y = 55;
    this->move(x, y);
}

TACTrayLabelWidget::TACTrayLabelWidget(QWidget* parent) : TAFloatingWidget(parent)
{
    this->setObjectName("TACTrayLabelWidget");
    //label = new QLabel("班级名称", this);
    //label->setAlignment(Qt::AlignCenter);
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setAlignment(Qt::AlignCenter);
    //layout->addWidget(label);
    this->setLayout(layout);

    // 背景透明，和学校logo风格一样（不设置背景色）
    // this->setBackgroundColor(WIDGET_BACKGROUND_COLOR); // 移除，让背景透明
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(15);
    this->resize(140, 70); // 荣誉图标大小和学校logo一样，学校logo是140x70

    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    //setWindowFlags(
    //    Qt::WindowStaysOnTopHint  // 置顶标志
    //    | Qt::WindowCloseButtonHint  // 保留关闭按钮（可选，避免窗口无关闭按钮）
    //);
}

TACTrayLabelWidget::~TACTrayLabelWidget()
{
}

void TACTrayLabelWidget::setContent(const QString& text)
{
    //label->setText(text);
}
void TACTrayLabelWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked();
    }
}

void TACTrayLabelWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
}

void TACTrayLabelWidget::paintEvent(QPaintEvent* event)
{
    TAFloatingWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (!m_fileName.isEmpty())
    {
        QPixmap m_backgroundPixmap = QPixmap(m_fileName);
        const double scaleFactor = 0.4; // 缩放到窗口的 40%
        QSize targetSize(qRound(width() * scaleFactor), qRound(height() * scaleFactor));
        QPixmap scaled = m_backgroundPixmap.scaled(
            targetSize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
        QRect r = scaled.rect();
        r.moveCenter(rect().center());
        painter.drawPixmap(r, scaled);
    }
}

void TACTrayLabelWidget::updateLogo(const QString& fileName)
{
    m_fileName = fileName;
    update();
}

void TACTrayLabelWidget::initShow()
{
    // 根据屏幕分辨率，将位置设置到屏幕右下角
    QRect rect = this->getScreenGeometryWithTaskbar();
    if (rect.isEmpty())
        return;
    // 计算右下角位置：屏幕宽度 - widget宽度，屏幕高度 - widget高度
    int x = rect.right() - this->width();
    int y = rect.bottom() - 1.5 * this->height();
    this->move(x, y);
}