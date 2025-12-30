#pragma execution_character_set("utf-8")
#include <QDateTime>
#include <QCloseEvent>
#include "TACDateTimeDialog.h"
#include "TACTypeButton.h"
#include "common.h"
TACDateTimeDialog::TACDateTimeDialog(QWidget *parent)
	: TADialog(parent),
    minimalButton(nullptr)
{
    this->setObjectName("TACDateTimeDialog");
    
    // 连接确定和取消按钮的信号，使其与极简按钮效果相同
    connect(this, &TADialog::cancelClicked, this, &TACDateTimeDialog::onMinimalButtonClicked);
    connect(this, &TADialog::enterClicked, this, &TACDateTimeDialog::onMinimalButtonClicked);
    
    // 在标题栏右上角添加"极简"按钮
    minimalButton = new QPushButton(QString::fromUtf8("极简"), this);
    minimalButton->setFixedSize(60, 30);
    minimalButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(76, 175, 80, 200);"
        "color: white;"
        "border: 2px solid #90EE90;"
        "border-radius: 5px;"
        "font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(76, 175, 80, 230);"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(76, 175, 80, 255);"
        "}"
    );
    connect(minimalButton, &QPushButton::clicked, this, &TACDateTimeDialog::onMinimalButtonClicked);
    
    // 将极简按钮添加到标题布局的右侧
    this->titleLayout->addStretch();
    this->titleLayout->addWidget(minimalButton);
    
    typeLayout = new QHBoxLayout();
    typeLayout->setContentsMargins(20, 10, 10, 20);
    typeLayout->setSpacing(20);
    TACTypeButton* dateButton = new TACTypeButton(this);
    dateButton->setCheckable(true);
    dateButton->setText("日期");
    dateButton->setCornerIcon(QIcon(":/res/img/type-button-checked.png"));
    dateButton->setChecked(true); // 默认选中日期
    
    TACTypeButton* timeButton = new TACTypeButton(this);
    timeButton->setCheckable(true);
    timeButton->setText("时间");
    timeButton->setCornerIcon(QIcon(":/res/img/type-button-checked.png"));
    timeButton->setChecked(true); // 默认选中时间
    
    typeLayout->addWidget(dateButton);
    typeLayout->addWidget(timeButton);
    this->contentLayout->addLayout(typeLayout);
    connect(dateButton, &QPushButton::toggled, this, [=](bool checked) {
        int type = (static_cast<int>(dateButton->isChecked()) << 1) | static_cast<int>(timeButton->isChecked());
        emit updateType(type);
    });
    connect(timeButton, &QPushButton::toggled, this, [=](bool checked) {
        int type = (static_cast<int>(dateButton->isChecked()) << 1) | static_cast<int>(timeButton->isChecked());
        emit updateType(type);
    });

    contentWidget = new QWidget(this);
    contentWidget->setObjectName("contentWidget");
    upContentLabel = new QLabel(contentWidget);
    upContentLabel->setAlignment(Qt::AlignCenter);
    upContentLabel->setObjectName("upContentLabel");
    downContentLabel = new QLabel(contentWidget);
    downContentLabel->setAlignment(Qt::AlignCenter);
    downContentLabel->setObjectName("downContentLabel");
    QVBoxLayout* vLayout = new QVBoxLayout(contentWidget);
    vLayout->setAlignment(Qt::AlignCenter);
    vLayout->addWidget(upContentLabel);
    vLayout->addWidget(downContentLabel);
    contentWidget->setLayout(vLayout);
    contentWidget->setFixedSize(QSize(460, 140));
    this->contentLayout->addWidget(contentWidget);
    this->contentLayout->addStretch();
    this->contentLayout->setAlignment(Qt::AlignCenter);
    
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->resize(520, 415);
    this->setTitle("选择时间模式");

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]() {
        if (dateButton->isChecked() && timeButton->isChecked())
        {
            upContentLabel->show();
            upContentLabel->setText(QTime::currentTime().toString("hh:mm"));
            downContentLabel->show();
            downContentLabel->setText(QDate::currentDate().toString("MM月dd日"));
        }
        else if (dateButton->isChecked())
        {
            upContentLabel->show();
            upContentLabel->setText(QDate::currentDate().toString("MM月dd日"));
            downContentLabel->hide();
        }
        else
        {
            upContentLabel->show();
            upContentLabel->setText(QTime::currentTime().toString("hh:mm"));
            downContentLabel->hide();
        }
    });
    timer->start(100);
}

TACDateTimeDialog::~TACDateTimeDialog()
{}

void TACDateTimeDialog::onMinimalButtonClicked()
{
    emit switchToMinimalMode();
    this->hide();
}

void TACDateTimeDialog::closeEvent(QCloseEvent* event)
{
    // 关闭按钮点击时，也切换到极简模式
    emit switchToMinimalMode();
    this->hide();
    event->accept();
}

