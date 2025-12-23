#include "courseeditdialog.h"

#include <QButtonGroup>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QString>

namespace
{
struct ColorOption
{
    QString name;
    QString color;
};

static const ColorOption kColorOptions[] = {
    {QString::fromUtf8("\xe8\xaf\xad\xe6\x96\x87\xe7\xba\xa2"), QStringLiteral("#f28b82")},
    {QString::fromUtf8("\xe6\x95\xb0\xe5\xad\xa6\xe8\x93\x9d"), QStringLiteral("#aecbfa")},
    {QString::fromUtf8("\xe8\x8b\xb1\xe8\xaf\xad\xe7\xbb\xbf"), QStringLiteral("#a7ffeb")},
    {QString::fromUtf8("\xe7\x89\xa9\xe7\x90\x86\xe6\xa9\x99"), QStringLiteral("#ffd180")},
    {QString::fromUtf8("\xe5\x8c\x96\xe5\xad\xa6\xe7\xb2\x89"), QStringLiteral("#ffe4e1")},
    {QString::fromUtf8("\xe7\x94\x9f\xe7\x89\xa9\xe7\xbb\xbf"), QStringLiteral("#c8e6c9")},
    {QString::fromUtf8("\xe6\x94\xbf\xe6\xb2\xbb\xe9\xbb\x84"), QStringLiteral("#fff59d")},
    {QString::fromUtf8("\xe5\x8e\x86\xe5\x8f\xb2\xe6\xa3\x95"), QStringLiteral("#d7ccc8")},
    {QString::fromUtf8("\xe5\x9c\xb0\xe7\x90\x86\xe7\xb2\x89"), QStringLiteral("#f8bbd0")},
    {QString::fromUtf8("\xe4\xbf\xa1\xe6\x81\xaf\xe7\xb4\xab"), QStringLiteral("#b39ddb")},
};
}

CourseEditDialog::CourseEditDialog(QWidget *parent)
    : QDialog(parent),
      nameEdit(nullptr),
      classEdit(nullptr),
      previewLabel(nullptr),
      customColorButton(nullptr),
      clearButton(nullptr),
      currentColor(Qt::transparent),
      selectedButton(nullptr),
      removeCourse(false)
{
    setWindowTitle(QString::fromUtf8("\xe7\xbc\x96\xe8\xbe\x91\xe8\xaf\xbe\xe7\xa8\x8b"));
    setModal(true);
    buildUi();
    resize(360, 220);
}

void CourseEditDialog::buildUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QLabel *hint = new QLabel(QString::fromUtf8("\xe8\xaf\xb7\xe5\x8f\x91\xe5\xb1\x95\xe4\xbd\xa0\xe7\x9a\x84\xe7\x89\xb9\xe8\x89\xb2\xe8\xaf\xbe\xe8\xa1\xa8\xef\xbc\x8c\xe8\xbe\x93\xe5\x85\xa5\xe8\xaf\xbe\xe7\xa8\x8b\xe5\x90\x8d\xe7\xa7\xb0\xe5\xb9\xb6\xe6\x8b\xa9\xe4\xb8\x80\xe7\xa7\x8d\xe9\xa2\x9c\xe8\x89\xb2"), this);
    hint->setWordWrap(true);
    mainLayout->addWidget(hint);

    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText(QString::fromUtf8("\xe8\xaf\xbe\xe7\xa8\x8b\xe5\x90\x8d\xe7\xa7\xb0"));
    connect(nameEdit, &QLineEdit::textChanged, this, [this]() {
        removeCourse = false;
        updatePreview();
    });
    mainLayout->addWidget(nameEdit);

    classEdit = new QLineEdit(this);
    classEdit->setPlaceholderText(QString::fromUtf8(u8"班级名称（可选，例如：初一2班）"));
    connect(classEdit, &QLineEdit::textChanged, this, [this]() {
        removeCourse = false;
        updatePreview();
    });
    mainLayout->addWidget(classEdit);

    QGridLayout *colorLayout = new QGridLayout();
    colorLayout->setSpacing(6);
    int row = 0;
    int col = 0;
    for (const ColorOption &opt : kColorOptions)
    {
        QPushButton *btn = new QPushButton(opt.name, this);
        btn->setCheckable(true);
        btn->setProperty("color", QColor(opt.color));
        const QString baseStyle = QString("QPushButton{background:%1; border-radius:6px; padding:6px;}").arg(opt.color);
        btn->setProperty("baseStyle", baseStyle);
        btn->setStyleSheet(baseStyle);
        connect(btn, &QPushButton::clicked, this, [this, btn]() { selectColorButton(btn); });
        colorButtons << btn;
        colorLayout->addWidget(btn, row, col);
        if (++col >= 3)
        {
            col = 0;
            ++row;
        }
    }
    mainLayout->addLayout(colorLayout);

    QHBoxLayout *previewLayout = new QHBoxLayout();
    previewLayout->setSpacing(8);
    previewLabel = new QLabel(QString::fromUtf8("\xe5\x89\x8d\xe6\x98\xbe"), this);
    previewLabel->setFixedHeight(36);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setStyleSheet("border:1px solid #cccccc; border-radius:6px;");
    previewLayout->addWidget(previewLabel, 1);

    customColorButton = new QPushButton(QString::fromUtf8("\xe8\x87\xaa\xe5\xae\x9a\xe4\xb9\x89\xe9\xa2\x9c\xe8\x89\xb2"), this);
    connect(customColorButton, &QPushButton::clicked, this, &CourseEditDialog::handleCustomColor);
    previewLayout->addWidget(customColorButton);
    mainLayout->addLayout(previewLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    clearButton = buttonBox->addButton(QString::fromUtf8("\xe6\xb8\x85\xe7\xa9\xba"), QDialogButtonBox::ActionRole);
    connect(clearButton, &QPushButton::clicked, this, &CourseEditDialog::handleClear);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CourseEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &CourseEditDialog::reject);
    mainLayout->addWidget(buttonBox);

    updatePreview();
}

void CourseEditDialog::selectColorButton(QPushButton *button)
{
    if (!button)
        return;

    for (QPushButton *btn : colorButtons)
    {
        const QString baseStyle = btn->property("baseStyle").toString();
        if (btn == button)
        {
            btn->setChecked(true);
            btn->setStyleSheet(baseStyle + " QPushButton{border:2px solid #333333;}");
        }
        else
        {
            btn->setChecked(false);
            btn->setStyleSheet(baseStyle);
        }
    }

    selectedButton = button;
    currentColor = button->property("color").value<QColor>();
    removeCourse = false;
    updatePreview();
}

void CourseEditDialog::setCourseName(const QString &name)
{
    if (nameEdit)
        nameEdit->setText(name);
    updatePreview();
}

void CourseEditDialog::setClassName(const QString &name)
{
    if (classEdit)
        classEdit->setText(name);
    updatePreview();
}

void CourseEditDialog::setCourseColor(const QColor &color)
{
    currentColor = color;
    bool matched = false;
    for (QPushButton *btn : colorButtons)
    {
        const QColor btnColor = btn->property("color").value<QColor>();
        if (btnColor == color)
        {
            selectColorButton(btn);
            matched = true;
            break;
        }
    }
    if (!matched)
        selectedButton = nullptr;
    updatePreview();
}

QString CourseEditDialog::courseName() const
{
    return nameEdit ? nameEdit->text().trimmed() : QString();
}

QString CourseEditDialog::className() const
{
    return classEdit ? classEdit->text().trimmed() : QString();
}

QColor CourseEditDialog::courseColor() const
{
    return currentColor;
}

bool CourseEditDialog::removeRequested() const
{
    return removeCourse || courseName().isEmpty();
}

void CourseEditDialog::updatePreview()
{
    if (!previewLabel)
        return;

    QString displayName = courseName().isEmpty() ? QString::fromUtf8("\xe6\x97\xa0\xe8\xaf\xbe") : courseName();
    if (!className().isEmpty())
        displayName = displayName + "\n" + className();
    QString style = "border:1px solid #cccccc; border-radius:6px; padding:6px;";
    if (currentColor.isValid())
    {
        style += QString("background:%1;").arg(currentColor.name(QColor::HexRgb));
    }
    else
    {
        style += "background:#f5f5f5;";
    }
    previewLabel->setStyleSheet(style);
    previewLabel->setText(displayName);
}

void CourseEditDialog::handleClear()
{
    removeCourse = true;
    nameEdit->clear();
    if (classEdit)
        classEdit->clear();
    currentColor = QColor();
    selectedButton = nullptr;
    for (QPushButton *btn : colorButtons)
    {
        btn->setChecked(false);
        const QString baseStyle = btn->property("baseStyle").toString();
        if (!baseStyle.isEmpty())
            btn->setStyleSheet(baseStyle);
    }
    updatePreview();
}

void CourseEditDialog::handleCustomColor()
{
    QColor chosen = QColorDialog::getColor(currentColor.isValid() ? currentColor : QColor("#ffffff"), this);
    if (!chosen.isValid())
        return;
    currentColor = chosen;
    selectedButton = nullptr;
    for (QPushButton *btn : colorButtons)
        btn->setChecked(false);
    removeCourse = false;
    updatePreview();
}
