#pragma once

#include "mainwindow.h"

// 对外统一的教师课程表窗口名（不使用 Q_OBJECT，因此不需要单独 moc）
class TACTeacherCourseScheduleWindow : public MainWindow
{
public:
    explicit TACTeacherCourseScheduleWindow(QWidget* parent = nullptr)
        : MainWindow(parent)
    {
        setObjectName(QStringLiteral("TACTeacherCourseScheduleWindow"));
        setWindowTitle(QString::fromUtf8(u8"我的课表"));
    }
};


