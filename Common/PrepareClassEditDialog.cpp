#pragma execution_character_set("utf-8")

#include "PrepareClassEditDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {
// 背景更深、透明度更小（更不透明）
constexpr int kBgR = 25;
constexpr int kBgG = 25;
constexpr int kBgB = 25;
constexpr int kBgA = 235;
inline QColor panelBg() { return QColor(kBgR, kBgG, kBgB, kBgA); }
}

PrepareClassEditDialog::PrepareClassEditDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	setFixedSize(500, 400);

	applyUi();
	updateCloseButtonPos();
}

void PrepareClassEditDialog::applyUi()
{
	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(20, 20, 20, 20);
	mainLayout->setSpacing(14);

	// 关闭按钮：悬浮右上角，默认隐藏，鼠标进入窗口显示
	m_closeButton = new QPushButton(QString::fromUtf8(u8"×"), this);
	m_closeButton->setObjectName("prepareCloseButton");
	m_closeButton->setFixedSize(QSize(23, 23));
	m_closeButton->setCursor(Qt::PointingHandCursor);
	m_closeButton->setStyleSheet(
		"QPushButton#prepareCloseButton{"
		"color:white;"
		"background-color: rgba(25,25,25,235);"
		"border: 1px solid rgba(25,25,25,235);" // 边框与窗口背景一致
		"border-radius:11px;"
		"font-size:16px;"
		"font-weight:700;"
		"}"
		"QPushButton#prepareCloseButton:hover{background-color: rgba(35,35,35,245);}"
		"QPushButton#prepareCloseButton:pressed{background-color: rgba(30,30,30,245);}"
	);
	m_closeButton->hide();
	m_closeButton->raise();
	// 关闭按钮：隐藏窗口（用于复用，下次菜单再 show）
	connect(m_closeButton, &QPushButton::clicked, this, [this]() { onCloseClicked(); });

	m_headerLabel = new QLabel(QString::fromUtf8(u8"课前准备"), this);
	m_headerLabel->setAlignment(Qt::AlignCenter);
	// 背景/边框等与窗口一致：这里背景透明即可（显示为窗口背景）
	m_headerLabel->setStyleSheet(
		"QLabel{"
		"color:white;"
		"background:transparent;"
		"border: 1px solid rgba(25,25,25,235);" // 边框与窗口背景一致
		"font-size:16px;"
		"font-weight:600;"
		"}"
	);
	mainLayout->addWidget(m_headerLabel);

	m_textEdit = new QTextEdit(this);
	m_textEdit->setMinimumHeight(200);
	m_textEdit->setReadOnly(true); // 编辑框不可编辑（只读）
	m_textEdit->setStyleSheet(
		"QTextEdit{"
		"background-color: rgba(25,25,25,235);"
		"color: white;"
		"border: 1px solid rgba(25,25,25,235);" // 边框与窗口背景一致
		"border-radius: 14px;"
		"padding: 14px;"
		"}"
		"QTextEdit:focus{border: 1px solid rgba(25,25,25,235);}"
	);
	mainLayout->addWidget(m_textEdit, 1);
}

void PrepareClassEditDialog::setHeaderText(const QString& text)
{
	if (m_headerLabel) {
		m_headerLabel->setText(text);
	}
}

void PrepareClassEditDialog::setInitialContent(const QString& content)
{
	if (m_textEdit) {
		m_textEdit->setPlainText(content);
	}
}

QString PrepareClassEditDialog::content() const
{
	return m_content;
}

void PrepareClassEditDialog::onCloseClicked()
{
	hide();
}

void PrepareClassEditDialog::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);

	const QRect r = rect();
	const int radius = 20;
	QPainterPath path;
	path.addRoundedRect(r.adjusted(0, 0, -1, -1), radius, radius);

	p.fillPath(path, panelBg());
	// 边框颜色与背景一致（视觉上“没有边框”）
	QPen pen(panelBg());
	pen.setWidth(1);
	p.setPen(pen);
	p.drawPath(path);
}

void PrepareClassEditDialog::enterEvent(QEvent* event)
{
	QDialog::enterEvent(event);
	if (m_closeButton) m_closeButton->show();
}

void PrepareClassEditDialog::leaveEvent(QEvent* event)
{
	QDialog::leaveEvent(event);
	if (m_closeButton) m_closeButton->hide();
}

void PrepareClassEditDialog::resizeEvent(QResizeEvent* event)
{
	QDialog::resizeEvent(event);
	updateCloseButtonPos();
}

void PrepareClassEditDialog::updateCloseButtonPos()
{
	if (!m_closeButton) return;
	const int marginRight = 18;
	const int marginTop = 14;
	m_closeButton->move(width() - m_closeButton->width() - marginRight, marginTop);
	m_closeButton->raise(); // 确保在最上层，避免被内容控件遮挡导致“点了没反应”
}

void PrepareClassEditDialog::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		m_dragging = true;
		m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
		event->accept();
	}
	QDialog::mousePressEvent(event);
}

void PrepareClassEditDialog::mouseMoveEvent(QMouseEvent* event)
{
	if (m_dragging && (event->buttons() & Qt::LeftButton)) {
		move(event->globalPos() - m_dragStartPos);
		event->accept();
	}
	QDialog::mouseMoveEvent(event);
}

void PrepareClassEditDialog::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		m_dragging = false;
		event->accept();
	}
	QDialog::mouseReleaseEvent(event);
}


