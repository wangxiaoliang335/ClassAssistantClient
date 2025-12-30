#pragma execution_character_set("utf-8")
#include "TACImageDialog.h"
#include "common.h"
#include <QFileDialog>
#include <QScrollArea>
#include <QPixmap>

// ImageDisplayWidget实现
ImageDisplayWidget::ImageDisplayWidget(QWidget* parent)
    : QWidget(parent), m_stretchMode(0)
{
    setAttribute(Qt::WA_TranslucentBackground, false);
}

void ImageDisplayWidget::setImage(const QPixmap& pixmap)
{
    m_pixmap = pixmap;
    update();
}

void ImageDisplayWidget::setStretchMode(int mode)
{
    m_stretchMode = mode;
    update();
}

void ImageDisplayWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    if (m_pixmap.isNull()) {
        return;
    }
    
    QPixmap scaledPixmap;
    if (m_stretchMode == 1) {
        // 左右拉伸：图片左右方向占满，上下方向等比例缩放
        scaledPixmap = m_pixmap.scaledToWidth(
            width(),
            Qt::SmoothTransformation
        );
    } else if (m_stretchMode == 2) {
        // 上下拉伸：图片上下方向占满，左右方向等比例缩放
        scaledPixmap = m_pixmap.scaledToHeight(
            height(),
            Qt::SmoothTransformation
        );
    } else {
        // 原始大小，保持比例
        scaledPixmap = m_pixmap.scaled(
            size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
    }
    
    // 居中绘制
    QRect targetRect = scaledPixmap.rect();
    targetRect.moveCenter(rect().center());
    painter.drawPixmap(targetRect, scaledPixmap);
}

TACImageDialog::TACImageDialog(QWidget *parent)
    : TADialog(parent),
    imageWidget(nullptr),
    selectImageButton(nullptr),
    stretchHorizontalButton(nullptr),
    stretchVerticalButton(nullptr),
    templateWidget(nullptr),
    backgroundLabel(nullptr),
    singleLineTextEdit(nullptr),
    multiLineTextEdit(nullptr),
    backgroundImageButton(nullptr),
    stretchMode(0)
{
    this->setObjectName("TACImageDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(1200, 800));
    this->setTitle(QString::fromUtf8("图片对话框"));

    this->contentLayout->setContentsMargins(20, 10, 20, 10);
    this->contentLayout->setSpacing(20);

    QHBoxLayout* mainContentLayout = new QHBoxLayout();
    mainContentLayout->setSpacing(20);
    mainContentLayout->setContentsMargins(0, 0, 0, 0);

    // 左侧：图片组件
    QWidget* leftPanel = new QWidget(this);
    leftPanel->setFixedWidth(600);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(10, 10, 10, 10);
    leftLayout->setSpacing(10);

    // 图片显示区域
    imageWidget = new ImageDisplayWidget(leftPanel);
    imageWidget->setFixedSize(580, 400);
    imageWidget->setStyleSheet(
        "QWidget {"
        "background-color: rgba(200, 200, 200, 50);"
        "border: 1px dashed rgba(150, 150, 150, 150);"
        "border-radius: 5px;"
        "}"
    );

    // 选择图片按钮
    selectImageButton = new QPushButton(QString::fromUtf8("选择图片"), leftPanel);
    selectImageButton->setFixedHeight(35);
    connect(selectImageButton, &QPushButton::clicked, this, &TACImageDialog::onSelectImage);

    // 拉伸按钮容器
    QHBoxLayout* stretchButtonsLayout = new QHBoxLayout();
    stretchButtonsLayout->setSpacing(10);

    stretchHorizontalButton = new QPushButton(QString::fromUtf8("左右拉伸"), leftPanel);
    stretchHorizontalButton->setFixedHeight(35);
    connect(stretchHorizontalButton, &QPushButton::clicked, this, &TACImageDialog::onStretchHorizontal);

    stretchVerticalButton = new QPushButton(QString::fromUtf8("上下拉伸"), leftPanel);
    stretchVerticalButton->setFixedHeight(35);
    connect(stretchVerticalButton, &QPushButton::clicked, this, &TACImageDialog::onStretchVertical);

    stretchButtonsLayout->addWidget(stretchHorizontalButton);
    stretchButtonsLayout->addWidget(stretchVerticalButton);

    leftLayout->addWidget(imageWidget);
    leftLayout->addWidget(selectImageButton);
    leftLayout->addLayout(stretchButtonsLayout);
    leftLayout->addStretch();

    // 右侧：班规班训模版
    QWidget* rightPanel = new QWidget(this);
    rightPanel->setFixedWidth(550);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(10, 10, 10, 10);
    rightLayout->setSpacing(10);

    // 模版容器
    templateWidget = new QWidget(rightPanel);
    templateWidget->setFixedSize(530, 350);
    templateWidget->setStyleSheet(
        "QWidget {"
        "background-color: rgba(255, 255, 255, 255);"
        "border: 1px solid rgba(150, 150, 150, 200);"
        "border-radius: 5px;"
        "}"
    );

    QVBoxLayout* templateLayout = new QVBoxLayout(templateWidget);
    templateLayout->setContentsMargins(15, 15, 15, 15);
    templateLayout->setSpacing(15);

    // 背景图片按钮
    QHBoxLayout* backgroundButtonLayout = new QHBoxLayout();
    backgroundButtonLayout->addStretch();
    backgroundImageButton = new QPushButton(QString::fromUtf8("背景图片"), templateWidget);
    backgroundImageButton->setFixedSize(100, 30);
    connect(backgroundImageButton, &QPushButton::clicked, this, &TACImageDialog::onSelectBackgroundImage);
    backgroundButtonLayout->addWidget(backgroundImageButton);

    // 背景图显示（作为边框/背景）
    backgroundLabel = new QLabel(templateWidget);
    backgroundLabel->setFixedHeight(200);
    backgroundLabel->setStyleSheet(
        "QLabel {"
        "background-color: rgba(240, 240, 240, 255);"
        "border: 2px solid rgba(200, 200, 200, 255);"
        "border-radius: 5px;"
        "}"
    );
    backgroundLabel->setAlignment(Qt::AlignCenter);
    backgroundLabel->setText(QString::fromUtf8("背景图区域"));

    // 单行文本
    QLabel* singleLineLabel = new QLabel(QString::fromUtf8("单行文本:"), templateWidget);
    singleLineTextEdit = new QLineEdit(templateWidget);
    singleLineTextEdit->setPlaceholderText(QString::fromUtf8("请输入单行文本"));
    singleLineTextEdit->setFixedHeight(35);

    // 多行文本
    QLabel* multiLineLabel = new QLabel(QString::fromUtf8("多行文本:"), templateWidget);
    multiLineTextEdit = new QTextEdit(templateWidget);
    multiLineTextEdit->setPlaceholderText(QString::fromUtf8("请输入多行文本"));
    multiLineTextEdit->setFixedHeight(80);

    templateLayout->addLayout(backgroundButtonLayout);
    templateLayout->addWidget(backgroundLabel);
    templateLayout->addWidget(singleLineLabel);
    templateLayout->addWidget(singleLineTextEdit);
    templateLayout->addWidget(multiLineLabel);
    templateLayout->addWidget(multiLineTextEdit);

    rightLayout->addWidget(templateWidget);
    rightLayout->addStretch();

    // 添加到主布局
    mainContentLayout->addWidget(leftPanel);
    mainContentLayout->addWidget(rightPanel);

    this->contentLayout->addLayout(mainContentLayout);
    this->contentLayout->addStretch();
}

TACImageDialog::~TACImageDialog()
{}

void TACImageDialog::onSelectImage()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        QString::fromUtf8("选择图片"),
        "",
        QString::fromUtf8("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)")
    );

    if (!fileName.isEmpty()) {
        imageFilePath = fileName;
        originalImage = QPixmap(fileName);
        if (!originalImage.isNull()) {
            stretchMode = 0; // 重置为原始模式
            imageWidget->setImage(originalImage);
            imageWidget->setStretchMode(stretchMode);
        }
    }
}

void TACImageDialog::onStretchHorizontal()
{
    if (originalImage.isNull()) {
        return;
    }
    stretchMode = 1; // 左右拉伸模式
    imageWidget->setStretchMode(stretchMode);
}

void TACImageDialog::onStretchVertical()
{
    if (originalImage.isNull()) {
        return;
    }
    stretchMode = 2; // 上下拉伸模式
    imageWidget->setStretchMode(stretchMode);
}

void TACImageDialog::onSelectBackgroundImage()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        QString::fromUtf8("选择背景图片"),
        "",
        QString::fromUtf8("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)")
    );

    if (!fileName.isEmpty()) {
        backgroundImagePath = fileName;
        QPixmap bgPixmap(fileName);
        if (!bgPixmap.isNull()) {
            QPixmap scaled = bgPixmap.scaled(
                backgroundLabel->size(),
                Qt::KeepAspectRatioByExpanding,
                Qt::SmoothTransformation
            );
            backgroundLabel->setPixmap(scaled);
            backgroundLabel->setScaledContents(true);
        }
    }
}


