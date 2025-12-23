#ifndef TERMSETTINGSDIALOG_H
#define TERMSETTINGSDIALOG_H

#include <QDialog>
#include <QDate>

class QDateEdit;
class QDialogButtonBox;

class TermSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TermSettingsDialog(QWidget *parent = nullptr);

    QDate startDate() const;
    QDate endDate() const;

    void setStartDate(const QDate &date);
    void setEndDate(const QDate &date);

protected:
    void accept() override;

private:
    QDateEdit *startEdit;
    QDateEdit *endEdit;
    QDialogButtonBox *buttonBox;
};

#endif // TERMSETTINGSDIALOG_H
