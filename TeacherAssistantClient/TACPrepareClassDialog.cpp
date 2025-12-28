#pragma execution_character_set("utf-8")
#include "TACPrepareClassDialog.h"
#include "common.h"
#include <QTextEdit>
#include <QFont>
#include <QEvent>
#include <QLabel>
#include <QResizeEvent>

TACPrepareClassDialog::TACPrepareClassDialog(QWidget *parent)
	: TABaseDialog(parent)
{
    this->setObjectName("TACPrepareClassDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(650, 700));

    // 去掉基类自带标题栏区域（真正无标题栏）
    if (titleBar) {
        titleBar->hide();
        titleBar->setFixedHeight(0);
    }

    // 关闭按钮：悬浮在右上角，默认隐藏，鼠标进入窗口才显示
    closeButton->setText(QString::fromUtf8(u8"×"));
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setStyleSheet(
        "QPushButton#closeButton{"
        "color:white;"
        "background-color:rgba(255,255,255,20);"
        "border:1px solid rgba(255,255,255,35);"
        "border-radius:11px;"
        "font-size:16px;"
        "font-weight:700;"
        "}"
        "QPushButton#closeButton:hover{background-color:rgba(255,255,255,35);}"
        "QPushButton#closeButton:pressed{background-color:rgba(255,255,255,18);}"
    );
    closeButton->hide();
    closeButton->setParent(this);
    closeButton->raise();

    // 顶部标题（内容区内居中显示，不占“标题栏”）
    m_headerLabel = new QLabel(QString::fromUtf8(u8"第三节语文  课前准备"), this);
    m_headerLabel->setAlignment(Qt::AlignCenter);
    m_headerLabel->setStyleSheet("QLabel{color:white;background:transparent;font-size:18px;font-weight:600;}");

    // 内容区：大文本框（圆角、浅色底），背景/留白匹配截图
    m_contentEdit = new QTextEdit(this);
    m_contentEdit->setObjectName("prepareContentEdit");
    m_contentEdit->setReadOnly(true);
    m_contentEdit->setPlaceholderText(QString::fromUtf8(u8"请输入课前准备内容..."));
    m_contentEdit->setStyleSheet(
        "QTextEdit#prepareContentEdit{"
        "background-color:rgba(255,255,255,10);"
        "border:1px solid rgba(255,255,255,18);"
        "border-radius:22px;"
        "color:white;"
        "font-size:18px;"
        "padding:18px;"
        "}"
        "QTextEdit#prepareContentEdit:focus{border:1px solid rgba(255,255,255,28);}"
    );
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(14);
    contentLayout->addWidget(m_headerLabel);
    contentLayout->addWidget(m_contentEdit, 1);
    // 首次定位关闭按钮
    closeButton->move(width() - closeButton->width() - 18, 14);
}

TACPrepareClassDialog::~TACPrepareClassDialog()
{}

void TACPrepareClassDialog::enterEvent(QEvent* event)
{
    TABaseDialog::enterEvent(event);
    if (closeButton) closeButton->show();
}

void TACPrepareClassDialog::leaveEvent(QEvent* event)
{
    TABaseDialog::leaveEvent(event);
    if (closeButton) closeButton->hide();
}

void TACPrepareClassDialog::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (!closeButton) return;

    // 位置：右上角（跟随窗口尺寸）
    const int marginRight = 18;
    const int marginTop = 14;
    closeButton->move(width() - closeButton->width() - marginRight, marginTop);
}
