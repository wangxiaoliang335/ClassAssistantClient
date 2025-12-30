#pragma execution_character_set("utf-8")
#include "TACLogoDialog.h"
#include <QToolButton>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include "common.h"

TACLogoDialog::TACLogoDialog(QWidget *parent)
	: TADialog(parent),
    schoolLogoLabel(nullptr),
    schoolLogoContainer(nullptr),
    schoolLogoButton(nullptr),
    schoolLogoRemoveButton(nullptr),
    classNameEdit(nullptr),
    honorIconLabel(nullptr),
    honorIconsLayout(nullptr),
    honorIconsContainer(nullptr)
{
	this->setObjectName("TACLogoDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(650, 850)); // 再增加窗口高度，从800改为850
    this->setTitle("文本/图标");

    this->contentLayout->setContentsMargins(20, 10, 20, 10);
    this->contentLayout->setSpacing(20);
    
    // 学校LOGO部分
    schoolLogoLabel = new QLabel(QString::fromUtf8("学校LOGO"), this);
    schoolLogoLabel->setStyleSheet("font-size: 14px; color: white;");
    
    schoolLogoContainer = new QWidget(this);
    schoolLogoContainer->setFixedSize(140, 140);
    schoolLogoContainer->setStyleSheet(
        "QWidget {"
        "background-color: rgba(100, 100, 100, 100);"
        "border: 1px solid rgba(200, 200, 200, 100);"
        "border-radius: 5px;"
        "}"
    );
    
    // 使用相对定位来放置删除按钮（在容器右上角）
    schoolLogoContainer->setLayout(nullptr); // 不设置布局，使用绝对定位
    
    // 添加/显示logo按钮
    schoolLogoButton = new QToolButton(schoolLogoContainer);
    schoolLogoButton->setObjectName("schoolLogoButton");
    schoolLogoButton->setIcon(QIcon(":/res/img/text_popup_ic_add_nor.png"));
    schoolLogoButton->setIconSize(QSize(40, 40));
    schoolLogoButton->setText(QString::fromUtf8("添加学校图标"));
    schoolLogoButton->setFixedSize(130, 130);
    schoolLogoButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    schoolLogoButton->move(5, 5); // 居中位置 (140-130)/2 = 5
    connect(schoolLogoButton, &QToolButton::clicked, this, &TACLogoDialog::onSchoolLogoButtonClicked);
    
    // 删除按钮（初始隐藏，当有logo时显示，定位在右上角）
    schoolLogoRemoveButton = new QToolButton(schoolLogoContainer);
    schoolLogoRemoveButton->setText("×");
    schoolLogoRemoveButton->setFixedSize(20, 20);
    schoolLogoRemoveButton->move(115, 5); // 右上角位置 (140-20-5) = 115
    schoolLogoRemoveButton->setStyleSheet(
        "QToolButton {"
        "background-color: rgba(200, 0, 0, 200);"
        "color: white;"
        "border: none;"
        "border-radius: 10px;"
        "font-size: 14px;"
        "font-weight: bold;"
        "}"
        "QToolButton:hover {"
        "background-color: rgba(255, 0, 0, 255);"
        "}"
    );
    schoolLogoRemoveButton->hide();
    connect(schoolLogoRemoveButton, &QToolButton::clicked, this, &TACLogoDialog::onSchoolLogoRemoveClicked);
    
    this->contentLayout->addWidget(schoolLogoLabel);
    this->contentLayout->addWidget(schoolLogoContainer);

    QWidget* spacer0 = new QWidget(this);
    spacer0->setFixedSize(10, 10);
    this->contentLayout->addWidget(spacer0);

    // 班级名称部分
    QLabel* classNameLabel = new QLabel(QString::fromUtf8("班级名称"), this);
    classNameLabel->setStyleSheet("font-size: 14px; color: white;");
    classNameEdit = new QLineEdit(this);
    classNameEdit->setPlaceholderText(QString::fromUtf8("请输入班级名称"));
    this->contentLayout->addWidget(classNameLabel);
    this->contentLayout->addWidget(classNameEdit);

    QWidget* spacer1 = new QWidget(this);
    spacer1->setFixedSize(10, 10);
    this->contentLayout->addWidget(spacer1);

    // 荣誉图标部分
    honorIconLabel = new QLabel(QString::fromUtf8("荣誉图标"), this);
    honorIconLabel->setStyleSheet("font-size: 14px; color: white;");
    
    honorIconsContainer = new QWidget(this);
    honorIconsLayout = new QHBoxLayout(honorIconsContainer);
    honorIconsLayout->setContentsMargins(0, 0, 0, 0);
    honorIconsLayout->setSpacing(20);
    
    updateHonorIconsDisplay();
    
    this->contentLayout->addWidget(honorIconLabel);
    this->contentLayout->addWidget(honorIconsContainer);
    this->contentLayout->addStretch();
    // 添加额外间距，让确定、取消按钮往下移动
    QWidget* bottomSpacer = new QWidget(this);
    bottomSpacer->setFixedSize(10, 70); // 再增加底部间距，从50改为70
    this->contentLayout->addWidget(bottomSpacer);
}

TACLogoDialog::~TACLogoDialog()
{}

void TACLogoDialog::onSchoolLogoButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        QString::fromUtf8("选择学校图标"),
        "",
        QString::fromUtf8("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)")
    );
    
    if (!fileName.isEmpty()) {
        logoFileName = fileName;
        updateSchoolLogoDisplay();
    }
}

void TACLogoDialog::onSchoolLogoRemoveClicked()
{
    logoFileName.clear();
    updateSchoolLogoDisplay();
}

void TACLogoDialog::updateSchoolLogoDisplay()
{
    if (logoFileName.isEmpty()) {
        // 显示添加按钮
        schoolLogoButton->setIcon(QIcon(":/res/img/text_popup_ic_add_nor.png"));
        schoolLogoButton->setText(QString::fromUtf8("添加学校图标"));
        schoolLogoRemoveButton->hide();
    } else {
        // 显示logo图片
        QPixmap pixmap(logoFileName);
        if (!pixmap.isNull()) {
            QPixmap scaled = pixmap.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            schoolLogoButton->setIcon(QIcon(scaled));
            schoolLogoButton->setText("");
            schoolLogoRemoveButton->show();
        }
    }
}

void TACLogoDialog::onAddHonorIconClicked()
{
    if (honorIconFileNames.size() >= 3) {
        return; // 最多3个
    }
    
    QString fileName = QFileDialog::getOpenFileName(
        this,
        QString::fromUtf8("选择荣誉图标"),
        "",
        QString::fromUtf8("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)")
    );
    
    if (!fileName.isEmpty()) {
        honorIconFileNames.append(fileName);
        updateHonorIconsDisplay();
    }
}

void TACLogoDialog::onRemoveHonorIcon(int index)
{
    if (index >= 0 && index < honorIconFileNames.size()) {
        honorIconFileNames.removeAt(index);
        updateHonorIconsDisplay();
    }
}

void TACLogoDialog::updateHonorIconsDisplay()
{
    // 清除现有按钮
    QLayoutItem* item;
    while ((item = honorIconsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    // 添加已存在的荣誉图标
    for (int i = 0; i < honorIconFileNames.size(); ++i) {
        addHonorIconButton(honorIconFileNames[i], i);
    }
    
    // 如果少于3个，添加"添加"按钮
    if (honorIconFileNames.size() < 3) {
        QToolButton* addButton = new QToolButton(honorIconsContainer);
        addButton->setObjectName("addHonorIconButton");
        addButton->setIcon(QIcon(":/res/img/text_popup_ic_add_nor.png"));
        addButton->setIconSize(QSize(40, 40));
        addButton->setText(QString::fromUtf8("添加荣誉图标"));
        addButton->setFixedSize(140, 140);
        addButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        connect(addButton, &QToolButton::clicked, this, &TACLogoDialog::onAddHonorIconClicked);
        honorIconsLayout->addWidget(addButton);
    }
}

void TACLogoDialog::addHonorIconButton(const QString& fileName, int index)
{
    QWidget* iconContainer = new QWidget(honorIconsContainer);
    iconContainer->setFixedSize(140, 140);
    iconContainer->setStyleSheet(
        "QWidget {"
        "background-color: rgba(100, 100, 100, 100);"
        "border: 1px solid rgba(200, 200, 200, 100);"
        "border-radius: 5px;"
        "}"
    );
    iconContainer->setLayout(nullptr); // 不设置布局，使用绝对定位
    
    // 图标按钮
    QToolButton* iconButton = new QToolButton(iconContainer);
    QPixmap pixmap(fileName);
    if (!pixmap.isNull()) {
        QPixmap scaled = pixmap.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        iconButton->setIcon(QIcon(scaled));
        iconButton->setIconSize(QSize(120, 120));
        iconButton->setText("");
    }
    iconButton->setFixedSize(130, 130);
    iconButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    iconButton->move(5, 5); // 居中位置
    
    // 删除按钮（定位在右上角）
    QToolButton* removeButton = new QToolButton(iconContainer);
    removeButton->setText("×");
    removeButton->setFixedSize(20, 20);
    removeButton->move(115, 5); // 右上角位置
    removeButton->setStyleSheet(
        "QToolButton {"
        "background-color: rgba(200, 0, 0, 200);"
        "color: white;"
        "border: none;"
        "border-radius: 10px;"
        "font-size: 14px;"
        "font-weight: bold;"
        "}"
        "QToolButton:hover {"
        "background-color: rgba(255, 0, 0, 255);"
        "}"
    );
    connect(removeButton, &QToolButton::clicked, this, [this, index]() {
        onRemoveHonorIcon(index);
    });
    
    honorIconsLayout->addWidget(iconContainer);
}

QString TACLogoDialog::getSchoolName() const
{
    return QString(); // 不再返回学校名称，只返回logo文件名
}

QString TACLogoDialog::getClassName()
{
    return classNameEdit->text();
}

QString TACLogoDialog::getLogoFileName()
{
    return logoFileName;
}

QStringList TACLogoDialog::getHonorIconFileNames()
{
    return honorIconFileNames;
}

QString TACLogoDialog::getOtherFileName()
{
    // 保留以兼容旧代码，但已废弃，建议使用 getHonorIconFileNames()
    return honorIconFileNames.isEmpty() ? QString() : honorIconFileNames.first();
}

void TACLogoDialog::setLogoFileName(const QString& fileName)
{
    logoFileName = fileName;
    updateSchoolLogoDisplay();
}

void TACLogoDialog::setClassName(const QString& className)
{
    if (classNameEdit) {
        classNameEdit->setText(className);
    }
}

void TACLogoDialog::setHonorIconFileNames(const QStringList& fileNames)
{
    honorIconFileNames = fileNames;
    // 限制最多3个
    while (honorIconFileNames.size() > 3) {
        honorIconFileNames.removeLast();
    }
    updateHonorIconsDisplay();
}
