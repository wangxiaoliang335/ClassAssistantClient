#pragma execution_character_set("utf-8")
#include "TACCountDownDialog.h"
#include <QLineEdit>
#include <QDate>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QMouseEvent>
#include <QEvent>
#include <QApplication>
#include "common.h"

TACCountDownDialog::TACCountDownDialog(QWidget *parent)
	: TADialog(parent),
    countDownLabel(nullptr),
    minimalButton(nullptr),
    targetDate(2026, 6, 7),
    targetDateLabel(nullptr),
    headerLabel(nullptr),
    headerLineEdit(nullptr),
    calendar(nullptr),
    headerWidget(nullptr)
{
    setupUI();
}

void TACCountDownDialog::setupUI()
{
    this->setObjectName("TACCountDownDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(15);
    this->setFixedSize(QSize(600, 500));
    
    // 清除标题
    this->setTitle("");
    
    // 隐藏确定和取消按钮（TADialog 自动添加的按钮）
    // 通过遍历 mainLayout 找到 buttonWidget 并隐藏
    QLayoutItem* buttonItem;
    for (int i = 0; i < this->mainLayout->count(); ++i) {
        buttonItem = this->mainLayout->itemAt(i);
        if (buttonItem && buttonItem->widget()) {
            QWidget* widget = buttonItem->widget();
            if (widget->objectName() == "buttonWidget") {
                widget->hide();
                break;
            }
        }
    }
    
    // 清空内容布局
    QLayoutItem* item;
    while ((item = this->contentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    // 创建顶部标题区域（蓝色背景）
    headerWidget = new QWidget(this);
    headerWidget->setFixedHeight(60);
    headerWidget->setStyleSheet(
        "QWidget {"
        "background-color: #00CED1;"
        "border-radius: 0px;"
        "}"
    );
    
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 10, 20, 10);
    headerLayout->setSpacing(15);
    
    // 红色圆圈标记
    QLabel* circle1 = new QLabel("①", headerWidget);
    circle1->setFixedSize(25, 25);
    circle1->setStyleSheet(
        "QLabel {"
        "color: red;"
        "border: 2px solid red;"
        "border-radius: 12px;"
        "background-color: transparent;"
        "font-size: 14px;"
        "font-weight: bold;"
        "}"
    );
    circle1->setAlignment(Qt::AlignCenter);
    
    headerLabel = new QLabel(QString::fromUtf8("结婚纪念日还有"), headerWidget);
    headerLabel->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 16px;"
        "font-weight: bold;"
        "background-color: transparent;"
        "}"
    );
    headerLabel->setCursor(Qt::PointingHandCursor);
    // 双击事件处理提示文本编辑
    headerLabel->installEventFilter(this);
    
    // 创建用于编辑的 QLineEdit（初始隐藏，不添加到布局中，使用绝对定位）
    headerLineEdit = new QLineEdit(headerWidget);
    headerLineEdit->setStyleSheet(
        "QLineEdit {"
        "color: white;"
        "font-size: 16px;"
        "font-weight: bold;"
        "background-color: rgba(255, 255, 255, 30);"
        "border: 2px solid white;"
        "border-radius: 3px;"
        "padding: 2px 5px;"
        "}"
    );
    headerLineEdit->hide();
    connect(headerLineEdit, &QLineEdit::editingFinished, this, &TACCountDownDialog::onHeaderEditFinished);
    connect(headerLineEdit, &QLineEdit::returnPressed, this, &TACCountDownDialog::onHeaderEditFinished);
    
    headerLayout->addWidget(circle1);
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();
    
    // 在顶部标题区域右上角添加"极简"按钮
    minimalButton = new QPushButton(QString::fromUtf8("极简"), headerWidget);
    minimalButton->setFixedSize(60, 30);
    minimalButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(138, 43, 226, 200);"
        "color: white;"
        "border: 2px solid #90EE90;"
        "border-radius: 5px;"
        "font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(138, 43, 226, 230);"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(138, 43, 226, 255);"
        "}"
    );
    connect(minimalButton, &QPushButton::clicked, this, &TACCountDownDialog::onMinimalButtonClicked);
    headerLayout->addWidget(minimalButton);
    
    this->contentLayout->addWidget(headerWidget);
    
    // 创建中间倒计时显示区域（深灰蓝色背景）
    QWidget* countdownWidget = new QWidget(this);
    countdownWidget->setFixedHeight(300);
    countdownWidget->setStyleSheet(
        "QWidget {"
        "background-color: #2F4F4F;"
        "border-radius: 0px;"
        "}"
    );
    
    QVBoxLayout* countdownLayout = new QVBoxLayout(countdownWidget);
    countdownLayout->setContentsMargins(0, 0, 0, 0);
    countdownLayout->setSpacing(0);
    
    // 顶部右侧圆圈标记
    QHBoxLayout* topRightLayout = new QHBoxLayout();
    topRightLayout->setContentsMargins(20, 20, 20, 0);
    topRightLayout->addStretch();
    //QLabel* circle3 = new QLabel("③", countdownWidget);
    //circle3->setFixedSize(25, 25);
    //circle3->setStyleSheet(
    //    "QLabel {"
    //    "color: red;"
    //    "border: 2px solid red;"
    //    "border-radius: 12px;"
    //    "background-color: transparent;"
    //    "font-size: 14px;"
    //    "font-weight: bold;"
    //    "}"
    //);
    //circle3->setAlignment(Qt::AlignCenter);
    //topRightLayout->addWidget(circle3);
    countdownLayout->addLayout(topRightLayout);
    
    // 大号数字和"天"字
    QHBoxLayout* numberLayout = new QHBoxLayout();
    numberLayout->setContentsMargins(40, 20, 40, 20);
    numberLayout->addStretch();
    
    countDownLabel = new QLabel("195", countdownWidget);
    countDownLabel->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 120px;"
        "font-weight: bold;"
        "background-color: transparent;"
        "}"
    );
    countDownLabel->setAlignment(Qt::AlignCenter);
    
    QLabel* dayLabel = new QLabel(QString::fromUtf8("天"), countdownWidget);
    dayLabel->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 80px;"
        "font-weight: bold;"
        "background-color: transparent;"
        "}"
    );
    dayLabel->setAlignment(Qt::AlignCenter);
    
    numberLayout->addWidget(countDownLabel);
    numberLayout->addWidget(dayLabel);
    numberLayout->addStretch();
    countdownLayout->addLayout(numberLayout);
    
    countdownLayout->addStretch();
    this->contentLayout->addWidget(countdownWidget);
    
    // 创建底部目标日期区域（深灰蓝色背景，带虚线分隔）
    QWidget* dateWidget = new QWidget(this);
    dateWidget->setFixedHeight(100);
    dateWidget->setStyleSheet(
        "QWidget {"
        "background-color: #2F4F4F;"
        "border-radius: 0px;"
        "}"
    );
    
    QVBoxLayout* dateLayout = new QVBoxLayout(dateWidget);
    dateLayout->setContentsMargins(20, 10, 20, 20);
    dateLayout->setSpacing(10);
    
    // 虚线分隔线
    QFrame* line = new QFrame(dateWidget);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet(
        "QFrame {"
        "border: 1px dashed #708090;"
        "background-color: transparent;"
        "}"
    );
    dateLayout->addWidget(line);
    
    QHBoxLayout* dateInfoLayout = new QHBoxLayout();
    dateInfoLayout->setContentsMargins(0, 0, 0, 0);
    dateInfoLayout->setSpacing(15);
    
    targetDateLabel = new QLabel(QString::fromUtf8("目标日: 2026-06-07 星期三"), dateWidget);
    targetDateLabel->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 14px;"
        "background-color: transparent;"
        "}"
    );
    targetDateLabel->setCursor(Qt::PointingHandCursor);
    // 单击事件处理打开日期选择器
    targetDateLabel->installEventFilter(this);
    
    dateInfoLayout->addWidget(targetDateLabel);
    dateInfoLayout->addStretch();
    
    //// 底部右侧圆圈标记
    //QLabel* circle2 = new QLabel("②", dateWidget);
    //circle2->setFixedSize(25, 25);
    //circle2->setStyleSheet(
    //    "QLabel {"
    //    "color: red;"
    //    "border: 2px solid red;"
    //    "border-radius: 12px;"
    //    "background-color: transparent;"
    //    "font-size: 14px;"
    //    "font-weight: bold;"
    //    "}"
    //);
    //circle2->setAlignment(Qt::AlignCenter);
    //dateInfoLayout->addWidget(circle2);
    
    dateLayout->addLayout(dateInfoLayout);
    this->contentLayout->addWidget(dateWidget);
    
    // 创建日历组件（隐藏，用于选择日期）
    calendar = new TACalendarWidget(nullptr); // 使用 nullptr 作为父窗口，使其成为独立窗口
    calendar->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    calendar->setBorderColor(WIDGET_BORDER_COLOR);
    calendar->setBorderWidth(WIDGET_BORDER_WIDTH);
    calendar->setRadius(40);
    calendar->setWindowFlags(
        Qt::FramelessWindowHint |
        Qt::Tool |
        Qt::WindowStaysOnTopHint // 确保显示在最上层
    );
    calendar->hide();
    
    // 连接日历信号 - 双击选择日期
    connect(calendar, &TACalendarWidget::dateDoubleClicked, this, [this](const QDate& date) {
        if (calendar) {
            onCalendarDateSelected(date);
        }
    });
    
    updateDisplay();
}

TACCountDownDialog::~TACCountDownDialog()
{}

int TACCountDownDialog::daysLeft()
{
    QDate today = QDate::currentDate();
    int days = today.daysTo(targetDate);
    return days > 0 ? days : 0;
}

QString TACCountDownDialog::content()
{
    QString headerText = headerLabel ? headerLabel->text() : QString::fromUtf8("结婚纪念日还有");
    return headerText + QString::number(daysLeft()) + QString::fromUtf8("天");
}

void TACCountDownDialog::setTargetDate(const QDate& date)
{
    targetDate = date;
    updateDisplay();
}

QDate TACCountDownDialog::getTargetDate() const
{
    return targetDate;
}

void TACCountDownDialog::updateDisplay()
{
    if (countDownLabel) {
        countDownLabel->setText(QString::number(daysLeft()));
    }
    
    if (targetDateLabel) {
        QString weekDay = targetDate.toString("dddd");
        // 转换为中文星期
        QMap<QString, QString> weekMap;
        weekMap["Monday"] = QString::fromUtf8("星期一");
        weekMap["Tuesday"] = QString::fromUtf8("星期二");
        weekMap["Wednesday"] = QString::fromUtf8("星期三");
        weekMap["Thursday"] = QString::fromUtf8("星期四");
        weekMap["Friday"] = QString::fromUtf8("星期五");
        weekMap["Saturday"] = QString::fromUtf8("星期六");
        weekMap["Sunday"] = QString::fromUtf8("星期日");
        
        QString chineseWeekDay = weekMap.value(weekDay, weekDay);
        targetDateLabel->setText(QString::fromUtf8("目标日: %1 %2").arg(targetDate.toString("yyyy-MM-dd")).arg(chineseWeekDay));
    }
}

void TACCountDownDialog::showEvent(QShowEvent * event)
{
    updateDisplay();
    QWidget::showEvent(event);
}

void TACCountDownDialog::onMinimalButtonClicked()
{
    emit switchToMinimalMode();
    this->hide();
}

void TACCountDownDialog::onCalendarDateSelected(const QDate& date)
{
    setTargetDate(date);
    if (calendar) {
        calendar->hide();
    }
}

void TACCountDownDialog::onHeaderEditFinished()
{
    if (headerLineEdit && headerLabel) {
        QString newText = headerLineEdit->text().trimmed();
        if (!newText.isEmpty()) {
            headerLabel->setText(newText);
        }
        headerLineEdit->hide();
        headerLabel->show();
    }
}

void TACCountDownDialog::editHeaderText()
{
    if (headerLabel && headerLineEdit && headerWidget) {
        // 获取 headerLabel 在 headerWidget 中的位置和大小
        QRect labelRect = headerLabel->geometry();
        headerLineEdit->setGeometry(labelRect);
        headerLineEdit->setText(headerLabel->text());
        headerLabel->hide();
        headerLineEdit->show();
        headerLineEdit->setFocus();
        headerLineEdit->selectAll();
    }
}

void TACCountDownDialog::showCalendar()
{
    if (calendar && targetDateLabel) {
        // 获取 targetDateLabel 的位置，在其下方显示日历
        QPoint labelPos = targetDateLabel->mapToGlobal(QPoint(0, targetDateLabel->height()));
        calendar->move(labelPos.x(), labelPos.y() + 5);
        calendar->setSelectedDate(targetDate);
        calendar->show();
        calendar->raise();
        calendar->activateWindow();
    }
}

bool TACCountDownDialog::eventFilter(QObject* obj, QEvent* event)
{
    // 处理 headerLabel 的双击事件
    if (obj == headerLabel && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            editHeaderText();
            return true;
        }
    }
    
    // 处理 targetDateLabel 的单击事件
    if (obj == targetDateLabel && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            showCalendar();
            return true;
        }
    }
    
    return TADialog::eventFilter(obj, event);
}

void TACCountDownDialog::setHeaderText(const QString& text)
{
    if (headerLabel) {
        headerLabel->setText(text);
    }
}

QString TACCountDownDialog::getHeaderText() const
{
    return headerLabel ? headerLabel->text() : QString();
}
