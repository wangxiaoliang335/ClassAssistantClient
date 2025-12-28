#include "HomeworkViewDialog.h"
#include <QDateTime>
#include <QDebug>
#include <QDate>
#include <QLocale>
#include <algorithm>

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
    // 兼容旧接口：将Map转换为HomeworkItem列表
    QList<HomeworkItem> homeworkList;
    for (auto it = content.begin(); it != content.end(); ++it) {
        HomeworkItem item(it.key(), it.value());
        homeworkList.append(item);
    }
    setHomeworkList(homeworkList);
}

void HomeworkViewDialog::setHomeworkList(const QList<HomeworkItem>& homeworkList)
{
    // 清空现有内容
    if (contentLayout && scrollContentWidget) {
        QLayoutItem* item;
        while ((item = contentLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
        subjectLabels.clear();
    }
    
    if (homeworkList.isEmpty()) {
        // 如果没有作业，显示提示
        QLabel* emptyLabel = new QLabel(QString::fromUtf8(u8"（暂无作业）"), scrollContentWidget);
        emptyLabel->setStyleSheet(
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
        emptyLabel->setAlignment(Qt::AlignCenter);
        contentLayout->addWidget(emptyLabel);
        return;
    }
    
    // 按创建时间排序（升序：最早的在前面）
    QList<HomeworkItem> sortedList = homeworkList;
    std::sort(sortedList.begin(), sortedList.end(), [](const HomeworkItem& a, const HomeworkItem& b) {
        return a.createdAt < b.createdAt;
    });
    
    // 按时间顺序显示所有作业
    for (const HomeworkItem& item : sortedList) {
        // 科目标题
        QLabel* labelTitle = new QLabel(QString("%1:").arg(item.subject), scrollContentWidget);
        labelTitle->setStyleSheet("font-size: 14px; color: white; font-weight: bold; padding: 5px 0;");
        contentLayout->addWidget(labelTitle);
        
        // 时间标签（如果有created_at）
        if (!item.createdAt.isEmpty()) {
            QLabel* timeLabel = new QLabel(scrollContentWidget);
            // 解析时间并格式化显示（格式：2025-12-23 14:07:51）
            QDateTime dt = QDateTime::fromString(item.createdAt, "yyyy-MM-dd HH:mm:ss");
            QString timeStr;
            if (dt.isValid()) {
                timeStr = dt.toString("HH:mm");
            } else {
                // 如果标准格式解析失败，尝试其他格式
                dt = QDateTime::fromString(item.createdAt, Qt::ISODate);
                if (dt.isValid()) {
                    timeStr = dt.toString("HH:mm");
                } else {
                    timeStr = item.createdAt; // 如果无法解析，直接显示原字符串
                }
            }
            timeLabel->setText(QString::fromUtf8(u8"时间: %1").arg(timeStr));
            timeLabel->setStyleSheet("font-size: 12px; color: #aaa; padding: 2px 0 5px 0;");
            contentLayout->addWidget(timeLabel);
        }
        
        // 作业内容
        QLabel* contentLbl = new QLabel(item.content, scrollContentWidget);
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
        contentLayout->addWidget(contentLbl);
        
        // 添加间距
        contentLayout->addSpacing(10);
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

