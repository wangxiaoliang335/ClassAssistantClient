#pragma execution_character_set("utf-8")
#include "TADialog.h"
#include <QPainter>
#include <QPen>
#include <QPainterPath>
#include <QBrush>
#include <QApplication>
#include <QScreen>
TADialog::TADialog(QWidget *parent)
	: TABaseDialog(parent),
    buttonWidget(new QWidget(this)),
    cancelButton(new QPushButton(QString::fromUtf8("取消"), this)),
    enterButton(new QPushButton(QString::fromUtf8("确定"), this)),
    buttonLayout(new QHBoxLayout(buttonWidget))
{
    this->titleLabel->setAlignment(Qt::AlignCenter);
    buttonWidget->setObjectName("buttonWidget");
    buttonLayout->setContentsMargins(5, 5, 5, 5);
    buttonLayout->setSpacing(20);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(enterButton);
    buttonLayout->addStretch();
    mainLayout->addWidget(buttonWidget);
    enterButton->setObjectName("enterButton");
    connect(cancelButton, &QPushButton::clicked, this, &TADialog::onCancelClicked);
    connect(enterButton, &QPushButton::clicked, this, &TADialog::onEnterClicked);
}
void TADialog::onCancelClicked()
{
    emit cancelClicked();
}

void TADialog::onEnterClicked()
{
    emit enterClicked();
}

TADialog::~TADialog()
{}
