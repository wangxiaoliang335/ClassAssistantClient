#pragma execution_character_set("utf-8")
#include "TACSubjectInputDialog.h"
#include <QDebug>

TACSubjectInputDialog::TACSubjectInputDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(400, 200);
    
    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 50, 20, 20);
    mainLayout->setSpacing(20);
    
    // 创建关闭按钮
    m_closeButton = new QPushButton(QString::fromUtf8(u8"×"), this);
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
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // 创建标签
    m_label = new QLabel(QString::fromUtf8(u8"请输入科目名称:"), this);
    m_label->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 16px;"
        "font-weight: 600;"
        "background: transparent;"
        "}"
    );
    mainLayout->addWidget(m_label);
    
    // 创建输入框
    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setStyleSheet(
        "QLineEdit {"
        "background-color: rgba(255,255,255,0.1);"
        "color: white;"
        "border: 1px solid rgba(255,255,255,0.2);"
        "border-radius: 10px;"
        "padding: 10px;"
        "font-size: 14px;"
        "}"
        "QLineEdit:focus {"
        "border: 1px solid rgba(255,255,255,0.4);"
        "}"
    );
    mainLayout->addWidget(m_lineEdit);
    
    // 创建按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton(QString::fromUtf8(u8"取消"), this);
    m_cancelButton->setFixedSize(80, 35);
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(255,255,255,0.1);"
        "color: white;"
        "border: 1px solid rgba(255,255,255,0.2);"
        "border-radius: 8px;"
        "font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(255,255,255,0.2);"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(255,255,255,0.15);"
        "}"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &TACSubjectInputDialog::onCancelClicked);
    
    m_confirmButton = new QPushButton(QString::fromUtf8(u8"确定"), this);
    m_confirmButton->setFixedSize(80, 35);
    m_confirmButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(0,120,212,255);"
        "color: white;"
        "border: none;"
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
    connect(m_confirmButton, &QPushButton::clicked, this, &TACSubjectInputDialog::onConfirmClicked);
    
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_confirmButton);
    mainLayout->addLayout(buttonLayout);
    
    // 设置焦点到输入框
    m_lineEdit->setFocus();
}

TACSubjectInputDialog::~TACSubjectInputDialog()
{
}

void TACSubjectInputDialog::setLabelText(const QString& text)
{
    if (m_label) {
        m_label->setText(text);
    }
}

void TACSubjectInputDialog::setInitialText(const QString& text)
{
    if (m_lineEdit) {
        m_lineEdit->setText(text);
        m_lineEdit->selectAll();
    }
}

void TACSubjectInputDialog::onConfirmClicked()
{
    if (m_lineEdit && !m_lineEdit->text().trimmed().isEmpty()) {
        accept();
    }
}

void TACSubjectInputDialog::onCancelClicked()
{
    reject();
}

void TACSubjectInputDialog::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // 绘制圆角背景（与主窗口背景一致）
    QPainterPath path;
    path.addRoundedRect(rect(), 10, 10);
    
    // 使用与主窗口一致的背景色（深色半透明）
    p.fillPath(path, QColor(50, 50, 50, 240));
    
    // 边框
    QPen pen(QColor(255, 255, 255, 25));
    pen.setWidth(1);
    p.setPen(pen);
    p.drawPath(path);
}

void TACSubjectInputDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        m_dragging = true;
        event->accept();
    }
}

void TACSubjectInputDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
    }
}

void TACSubjectInputDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}

