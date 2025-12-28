#pragma once
#include "TAFloatingWidget.h"
#include <QPainter>
#include <QPixmap>
#include <QPaintEvent>
#include <QLabel>
#include <QList>
#include <QHBoxLayout>
#include <QPointer>
#include <QTimer>
#include <QTime>
#include <QMap>
#include <QPair>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QDate>
#include <QWidget>
#include <QMouseEvent>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
using namespace std;

class TAHttpHandler;
class PrepareClassEditDialog;

// 自定义课程按钮，支持右上角图标显示
class CourseButtonWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CourseButtonWidget(const QString& subject, const QString& time, QWidget* parent = nullptr);
    void setHasPrepareClass(bool hasPrepare);
    void setPrepareClassContent(const QString& content);
    QString subject() const { return m_subject; }
    QString time() const { return m_time; }
    
signals:
    void clicked();
    void prepareClassClicked(const QString& subject, const QString& time);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    
private:
    QString m_subject;
    QString m_time;
    bool m_hasPrepareClass;
    QString m_prepareClassContent;
    bool m_pressed;
    bool m_hovered;
};

class TACCourseSchedule : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACCourseSchedule(QWidget* parent);
	~TACCourseSchedule();
	
	// 从服务器获取今日课表
	void fetchTodaySchedule(const QString& classId);
	
	// 设置课前准备历史数据
	void setPrepareClassHistory(const QJsonArray& history);

protected:
	void initShow() override;
	
private slots:
	void updateTimeSlots();
	void onCourseButtonClicked(const QString& subject, const QString& time);
	
private:
	void cleanLayout(QLayout* layout);
	void handleScheduleResponse(const QString& response); // 处理服务器返回的课程表响应
	void applyTodaySchedule(const QStringList& days, const QStringList& times, const QJsonArray& cells); // 应用今日课表数据到界面
	QString currentTermString() const; // 获取当前学期字符串
	QStringList jsonArrayToStringList(const QJsonArray& arr) const; // 将JSON数组转换为字符串列表
	int getTodayColumnIndex(const QStringList& days) const; // 获取今天在days数组中的索引
	QPair<QStringList, QStringList> buildDefaultDailySchedule() const; // 构建默认日程表（与ScheduleDialog保持一致）
	QStringList defaultSubjectsForWeekday(int weekday) const; // 根据星期几获取默认科目列表
	QStringList defaultTimesForSubjects(const QStringList& subjects) const; // 根据科目列表获取默认时间列表
	void applyDefaultDailySchedule(); // 应用默认日程表到界面
	QString prepareClassCacheKey(const QString& subject, const QString& time) const; // 生成课前准备缓存键
	void showPrepareClassDialog(const QString& subject, const QString& time = QString()); // 显示课前准备对话框
	QString getPrepareClassContent(const QString& subject, const QString& time) const; // 获取课前准备内容
	
private:
	QPointer<QLabel> m_titleLabel; // 标题标签（显示星期几）
	QPointer<QScrollArea> m_scrollArea;
	QPointer<QWidget> m_scrollContent;
	QPointer<QVBoxLayout> contentLayout;
	QPointer<QVBoxLayout> classLayout; // 课程列表布局
	QPointer<QTimer> timer;
	TAHttpHandler* m_httpHandler = nullptr;
	QString m_currentClassId;
	
	// 课前准备相关
	QMap<QString, QString> m_prepareClassCache; // 课前准备内容缓存（科目|时间 -> 内容）
	QPointer<PrepareClassEditDialog> m_prepareClassViewDlg; // 课前准备查看窗口
};
