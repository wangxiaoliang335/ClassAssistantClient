#include "termsettingsdialog.h"

#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>

TermSettingsDialog::TermSettingsDialog(QWidget *parent)
    : QDialog(parent),
      startEdit(new QDateEdit(QDate::currentDate(), this)),
      endEdit(new QDateEdit(QDate::currentDate().addMonths(4), this)),
      buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this))
{
    setWindowTitle(QString::fromUtf8("\xe8\xae\xbe\xe7\xbd\xae\xe5\xad\xa6\xe6\x9c\x9f\xe6\x97\xb6\xe9\x97\xb4"));
    startEdit->setCalendarPopup(true);
    startEdit->setDisplayFormat("yyyy-MM-dd");
    endEdit->setCalendarPopup(true);
    endEdit->setDisplayFormat("yyyy-MM-dd");

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(QString::fromUtf8("\xe5\xad\xa6\xe6\x9c\x9f\xe5\xbc\x80\xe5\xa7\x8b\xe6\x97\xa5\xe6\x9c\x9f"), startEdit);
    formLayout->addRow(QString::fromUtf8("\xe5\xad\xa6\xe6\x9c\x9f\xe7\xbb\x93\xe6\x9d\x9f\xe6\x97\xa5\xe6\x9c\x9f"), endEdit);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &TermSettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TermSettingsDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
}

QDate TermSettingsDialog::startDate() const
{
    return startEdit->date();
}

QDate TermSettingsDialog::endDate() const
{
    return endEdit->date();
}

void TermSettingsDialog::setStartDate(const QDate &date)
{
    if (date.isValid())
        startEdit->setDate(date);
}

void TermSettingsDialog::setEndDate(const QDate &date)
{
    if (date.isValid())
        endEdit->setDate(date);
}

void TermSettingsDialog::accept()
{
    if (startEdit->date() > endEdit->date())
    {
        QMessageBox::warning(this,
                             QString::fromUtf8("\xe6\x8f\x90\xe7\xa4\xba"),
                             QString::fromUtf8("\xe5\xbc\x80\xe5\xa7\x8b\xe6\x97\xa5\xe6\x9c\x9f\xe4\xb8\x8d\xe8\x83\xbd\xe6\x99\x9a\xe4\xba\x8e\xe7\xbb\x93\xe6\x9d\x9f\xe6\x97\xa5\xe6\x9c\x9f"));
        return;
    }

    QDialog::accept();
}
