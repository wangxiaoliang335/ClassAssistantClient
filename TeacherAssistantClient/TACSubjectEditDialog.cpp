#pragma execution_character_set("utf-8")
#include "TACSubjectEditDialog.h"
#include <QApplication>
#include <QDesktopWidget>

// 使用与 TACMainDialog 类似的深色背景色
static const QColor DIALOG_BACKGROUND_COLOR(40, 40, 40, 240);

TACSubjectEditDialog::TACSubjectEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(350, 180);
    
    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 50, 20, 20);
    mainLayout->setSpacing(15);
    
    // 创建关闭按钮
    m_closeButton = new QPushButton(this);
    m_closeButton->setText(QString::fromUtf8(u8"×"));
    m_closeButton->setFixedSize(23, 23);
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "color: white;"
        "background-color: rgba(255,255,255,20);"
        "border: 1px solid rgba(50,50,50,200);"
        "border-radius: 11px;"
        "font-size: 16px;"
        "font-weight: 700;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(255,255,255,35);"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(255,255,255,18);"
        "}"
    );
    m_closeButton->hide();
    m_closeButton->raise();
    connect(m_closeButton, &QPushButton::clicked, this, &TACSubjectEditDialog::onCancelClicked);
    
    // 标题标签（可选，用于显示"编辑科目"或"添加科目"）
    m_titleLabel = new QLabel(this);
    m_titleLabel->setText(QString::fromWCharArray(L"编辑科目"));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold; background: transparent;");
    mainLayout->addWidget(m_titleLabel);
    
    // 提示标签
    m_promptLabel = new QLabel(this);
    m_promptLabel->setText(QString::fromWCharArray(L"请输入科目名称:"));
    m_promptLabel->setStyleSheet("color: white; font-size: 14px; background: transparent;");
    mainLayout->addWidget(m_promptLabel);
    
    // 输入框
    m_subjectEdit = new QLineEdit(this);
    m_subjectEdit->setFixedHeight(35);
    m_subjectEdit->setStyleSheet(
        "QLineEdit {"
        "background-color: rgba(25,25,25,235);"
        "color: white;"
        "border: 1px solid rgba(25,25,25,235);"
        "border-radius: 8px;"
        "padding: 8px;"
        "font-size: 14px;"
        "}"
        "QLineEdit:focus {"
        "border: 1px solid rgba(0,120,212,255);"
        "}"
    );
    mainLayout->addWidget(m_subjectEdit);
    
    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    
    m_cancelButton = new QPushButton(this);
    m_cancelButton->setText(QString::fromWCharArray(L"取消"));
    m_cancelButton->setFixedHeight(35);
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(60,60,60,200);"
        "color: white;"
        "border: 1px solid rgba(60,60,60,200);"
        "border-radius: 8px;"
        "font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(80,80,80,200);"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(50,50,50,200);"
        "}"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &TACSubjectEditDialog::onCancelClicked);
    
    m_confirmButton = new QPushButton(this);
    m_confirmButton->setText(QString::fromWCharArray(L"确定"));
    m_confirmButton->setFixedHeight(35);
    m_confirmButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(0,120,212,255);"
        "color: white;"
        "border: 1px solid rgba(0,120,212,255);"
        "border-radius: 8px;"
        "font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(0,140,232,255);"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(0,100,192,255);"
        "}"
    );
    connect(m_confirmButton, &QPushButton::clicked, this, &TACSubjectEditDialog::onConfirmClicked);
    
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_confirmButton);
    mainLayout->addLayout(buttonLayout);
    
    updateCloseButtonPos();
}

TACSubjectEditDialog::~TACSubjectEditDialog()
{
}

void TACSubjectEditDialog::setSubjectName(const QString& name)
{
    m_subjectEdit->setText(name);
    m_subjectEdit->selectAll();
}

void TACSubjectEditDialog::setLabelText(const QString& text)
{
    m_promptLabel->setText(text);
}

void TACSubjectEditDialog::setInitialText(const QString& text)
{
    m_subjectEdit->setText(text);
    m_subjectEdit->selectAll();
}

void TACSubjectEditDialog::setTitleText(const QString& text)
{
    if (m_titleLabel) {
        m_titleLabel->setText(text);
    }
}

QString TACSubjectEditDialog::subjectName() const
{
    return m_subjectEdit->text().trimmed();
}

QString TACSubjectEditDialog::text() const
{
    return m_subjectEdit->text().trimmed();
}

void TACSubjectEditDialog::onConfirmClicked()
{
    if (!m_subjectEdit->text().trimmed().isEmpty()) {
        accept();
    }
}

void TACSubjectEditDialog::onCancelClicked()
{
    reject();
}

void TACSubjectEditDialog::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // 绘制圆角背景
    QPainterPath path;
    path.addRoundedRect(rect(), 10, 10);
    
    // 深色半透明背景（与主窗口一致）
    p.fillPath(path, DIALOG_BACKGROUND_COLOR);
    
    // 边框
    QPen pen(QColor(60, 60, 60, 255));
    pen.setWidth(1);
    p.setPen(pen);
    p.drawPath(path);
}

void TACSubjectEditDialog::enterEvent(QEvent* event)
{
    if (m_closeButton) {
        m_closeButton->show();
    }
    QDialog::enterEvent(event);
}

void TACSubjectEditDialog::leaveEvent(QEvent* event)
{
    QPoint globalPos = QCursor::pos();
    QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
    if (!widgetRect.contains(globalPos) && m_closeButton) {
        QRect btnRect = QRect(m_closeButton->mapToGlobal(QPoint(0, 0)), m_closeButton->size());
        if (!btnRect.contains(globalPos)) {
            m_closeButton->hide();
        }
    }
    QDialog::leaveEvent(event);
}

void TACSubjectEditDialog::resizeEvent(QResizeEvent* event)
{
    updateCloseButtonPos();
    QDialog::resizeEvent(event);
}

void TACSubjectEditDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        m_dragging = true;
        event->accept();
    }
}

void TACSubjectEditDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
    }
}

void TACSubjectEditDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}

void TACSubjectEditDialog::updateCloseButtonPos()
{
    if (m_closeButton) {
        const int marginRight = 8;
        const int marginTop = 8;
        m_closeButton->move(width() - m_closeButton->width() - marginRight, marginTop);
    }
}
