#include "HomeworkViewDialog.h"
#include <QDateTime>
#include <QDebug>
#include <QDate>
#include <QLocale>

HomeworkViewDialog::HomeworkViewDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(500, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 主容器（半透明灰色背景，圆角）
    QWidget* container = new QWidget(this);
    container->setStyleSheet(
        "QWidget {"
        "background-color: rgba(60, 60, 60, 240);"
        "border-radius: 12px;"
        "}"
    );
    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(20, 20, 20, 20);
    containerLayout->setSpacing(15);

    // 顶部栏：关闭按钮
    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->addStretch();
    
    QPushButton* btnCloseTop = new QPushButton("✕", container);
    btnCloseTop->setFixedSize(30, 30);
    btnCloseTop->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(100, 100, 100, 200);"
        "color: white;"
        "font-size: 18px;"
        "font-weight: bold;"
        "border: none;"
        "border-radius: 15px;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(120, 120, 120, 220);"
        "}"
    );
    connect(btnCloseTop, &QPushButton::clicked, this, &HomeworkViewDialog::onCloseClicked);
    topLayout->addWidget(btnCloseTop);
    containerLayout->addLayout(topLayout);

    // 日期标题（按图片样式：日期 + 星期 + "家庭作业"）
    dateLabel = new QLabel(container);
    dateLabel->setAlignment(Qt::AlignCenter);
    dateLabel->setStyleSheet(
        "font-size: 16px;"
        "font-weight: bold;"
        "color: white;"
        "padding: 10px;"
    );
    containerLayout->addWidget(dateLabel);

    // 科目作业显示区域（可滚动）
    QScrollArea* scrollArea = new QScrollArea(container);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    
    scrollContentWidget = new QWidget;
    this->contentLayout = new QVBoxLayout(scrollContentWidget);
    this->contentLayout->setSpacing(15);
    this->contentLayout->setContentsMargins(0, 0, 0, 0);
    scrollArea->setWidget(scrollContentWidget);
    containerLayout->addWidget(scrollArea, 1);

    mainLayout->addWidget(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
}

void HomeworkViewDialog::setDate(const QDate& date)
{
    if (dateLabel) {
        QLocale locale(QLocale::Chinese);
        QString weekDay = locale.toString(date, "dddd");
        QString dateStr = QString("%1 %2 家庭作业").arg(date.toString("yyyy年MM月dd日")).arg(weekDay);
        dateLabel->setText(dateStr);
    }
}

void HomeworkViewDialog::setHomeworkContent(const QMap<QString, QString>& content)
{
    // 先更新/创建本次内容涉及的科目
    for (auto it = content.begin(); it != content.end(); ++it) {
        QLabel* lbl = ensureSubjectLabel(it.key());
        if (lbl) lbl->setText(it.value());
    }

    // 已存在但本次没传的科目置为空提示（避免显示旧内容）
    for (auto it = subjectLabels.begin(); it != subjectLabels.end(); ++it) {
        const QString subject = it.key();
        QLabel* lbl = it.value();
        if (!content.contains(subject) || content.value(subject).trimmed().isEmpty()) {
            lbl->setText(QString::fromUtf8(u8"（暂无作业）"));
            lbl->setStyleSheet(
                "QLabel {"
                "background-color: #3b3b3b;"
                "color: #888;"
                "border: 1px solid #555;"
                "border-radius: 4px;"
                "padding: 10px;"
                "font-size: 14px;"
                "min-height: 60px;"
                "font-style: italic;"
                "}"
            );
        }
    }
}

void HomeworkViewDialog::onCloseClicked()
{
    reject();
}

void HomeworkViewDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

void HomeworkViewDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void HomeworkViewDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
        return;
    }
    QDialog::mouseReleaseEvent(event);
}

QLabel* HomeworkViewDialog::ensureSubjectLabel(const QString& subject)
{
    const QString s = subject.trimmed();
    if (s.isEmpty()) return nullptr;
    if (subjectLabels.contains(s)) return subjectLabels[s];
    if (!contentLayout || !scrollContentWidget) return nullptr;

    QLabel* labelTitle = new QLabel(QString("%1:").arg(s), scrollContentWidget);
    labelTitle->setStyleSheet("font-size: 14px; color: white; font-weight: bold; padding: 5px 0;");
    contentLayout->addWidget(labelTitle);

    QLabel* contentLbl = new QLabel(scrollContentWidget);
    contentLbl->setWordWrap(true);
    contentLbl->setStyleSheet(
        "QLabel {"
        "background-color: rgba(80, 80, 80, 200);"
        "color: white;"
        "border: 1px solid rgba(100, 100, 100, 150);"
        "border-radius: 6px;"
        "padding: 12px;"
        "font-size: 14px;"
        "min-height: 50px;"
        "}"
    );
    contentLbl->setText(QString::fromUtf8(u8"（暂无作业）"));

    subjectLabels[s] = contentLbl;
    contentLayout->addWidget(contentLbl);
    return contentLbl;
}

