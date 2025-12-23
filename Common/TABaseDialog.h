#pragma once

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QLabel>
#include <QVector>

// 老师任教信息（支持多条记录）
typedef struct tagUserTeachingInfo {
    QString grade_level;   // 学段，如"小学"
    QString grade;         // 年级，如"三年级"
    QString subject;       // 任教科目，如"数学"
    QString class_taught;  // 任教班级，如"3-2"
} UserTeachingInfo;

typedef struct tagUserInfo {
    QString strUserId;
    QString strPhone;
    QString strName;
    QString strSex;
    QString strAddress;
    QString strSchoolName;
    QString strGradeLevel;
    // 多条任教记录（teachings 列表）
    QVector<UserTeachingInfo> teachings;
    QString strIsAdministrator;
    QString avatar;
    QString strIdNumber;
    QString strHeadImagePath;
    //QString teacher_unique_id;
    QString schoolId;
    QString classId;
}UserInfo;

class TABaseDialog  : public QWidget
{
	Q_OBJECT

public:
	TABaseDialog(QWidget *parent);
	~TABaseDialog();
    void setTitle(const QString& text);
    Q_PROPERTY(QString titleName READ titleName WRITE setTitleName)
        Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
        Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)
        Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth)
        Q_PROPERTY(int radius READ radius WRITE setRadius)
        QString titleName() const;
    QColor backgroundColor() const;
    QColor borderColor() const;
    QRect getScreenGeometryWithTaskbar();
    int radius();
    int borderWidth();
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void clearLayout(QLayout* layout);

public slots:
    void setTitleName(const QString& name);
    void setBackgroundColor(const QColor& color);
    void setBorderColor(const QColor& color);
    void setBorderWidth(int val);
    void setRadius(int val);

protected:
    QWidget* titleBar;
    QLabel* titleLabel;
    QPushButton* closeButton;
    QVBoxLayout* mainLayout;
    QHBoxLayout* titleLayout;
    QVBoxLayout* contentLayout;

private slots:
    void onClose();
private:
    bool m_dragging;
    QPoint m_dragStartPosition;
    QString m_titleName;
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth;
    int m_radius;
};
