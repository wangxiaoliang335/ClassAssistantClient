#ifndef COURSEEDITDIALOG_H
#define COURSEEDITDIALOG_H

#include <QColor>
#include <QDialog>

class QLineEdit;
class QPushButton;
class QLabel;

class CourseEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CourseEditDialog(QWidget *parent = nullptr);

    void setCourseName(const QString &name);
    void setClassName(const QString &name);
    void setCourseColor(const QColor &color);

    QString courseName() const;
    QString className() const;
    QColor courseColor() const;
    bool removeRequested() const;

private:
    void buildUi();
    void selectColorButton(QPushButton *button);
    void updatePreview();
    void handleClear();
    void handleCustomColor();

    QLineEdit *nameEdit;
    QLineEdit *classEdit;
    QLabel *previewLabel;
    QList<QPushButton *> colorButtons;
    QPushButton *customColorButton;
    QPushButton *clearButton;
    QColor currentColor;
    QPushButton *selectedButton;
    bool removeCourse;
};

#endif // COURSEEDITDIALOG_H
