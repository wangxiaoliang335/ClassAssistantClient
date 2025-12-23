#ifndef SCHEDULECELLLABEL_H
#define SCHEDULECELLLABEL_H

#include <QColor>
#include <QFont>
#include <QLabel>

class QResizeEvent;
class QPaintEvent;
class QMouseEvent;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;
class QVariantAnimation;

class ScheduleCellLabel : public QLabel
{
    Q_OBJECT

public:
    enum class CourseType
    {
        None,
        Deleted,
        Temporary,
        DeletedTemporary,
        Normal
    };

    struct CourseInfo
    {
        QString name;
        QColor background;
        CourseType type = CourseType::None;
    };

    explicit ScheduleCellLabel(QWidget *parent = nullptr);

    void setCourseInfo(const CourseInfo &info);
    CourseInfo courseInfo() const { return currentInfo; }
    void setCellCoordinates(int row, int column);
    void setDragEnabled(bool enabled);
    void setTodayHighlighted(bool highlighted);

signals:
    void requestDropOperation(int sourceRow, int sourceColumn, int targetRow, int targetColumn, const QString &name, const QColor &color, int type);
    void requestWeeklyCourseDrop(int targetRow, int targetColumn, const QString &name, const QColor &color);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QColor resolveBackground(const CourseInfo &info) const;
    QColor resolveTextColor(CourseType type) const;
    QString borderStyle(CourseType type) const;
    void adjustFontToContents();
    qreal basePointSize() const;
    void startDrag(const QPoint &hotSpot);
    void startDropGlow();

    CourseInfo currentInfo;
    QFont baseFont;
    int cellRow = -1;
    int cellColumn = -1;
    bool dragEnabled = false;
    QPoint dragStartPos;
    qreal dropGlowOpacity = 0.0;
    QVariantAnimation *dropAnimation = nullptr;
};

#endif // SCHEDULECELLLABEL_H
