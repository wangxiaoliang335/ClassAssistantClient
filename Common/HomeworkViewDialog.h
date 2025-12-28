#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QDate>
#include <QMap>
#include <QList>
#include <QMouseEvent>
#include <QScrollArea>
#include <QFrame>
#include <QDateTime>

// 家庭作业项结构（包含时间戳）
struct HomeworkItem {
    QString subject;      // 科目
    QString content;      // 作业内容
    QString createdAt;    // 添加时间（created_at字段）
    
    HomeworkItem() {}
    HomeworkItem(const QString& s, const QString& c, const QString& time = "")
        : subject(s), content(c), createdAt(time) {}
    
    // 用于排序：按时间升序
    bool operator<(const HomeworkItem& other) const {
        return createdAt < other.createdAt;
    }
};

class HomeworkViewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HomeworkViewDialog(QWidget* parent = nullptr);
    
    // 设置日期
    void setDate(const QDate& date);
    
    // 设置作业内容（旧接口，兼容性保留）
    void setHomeworkContent(const QMap<QString, QString>& content);
    
    // 设置作业列表（按时间排序显示所有作业）
    void setHomeworkList(const QList<HomeworkItem>& homeworkList);

private slots:
    void onCloseClicked();

private:
    QLabel* dateLabel;
    QMap<QString, QLabel*> subjectLabels; // 科目 -> 作业内容标签
    QPushButton* btnClose;
    QVBoxLayout* contentLayout = nullptr;
    QWidget* scrollContentWidget = nullptr; // 滚动内容容器

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool m_dragging = false;
    QPoint m_dragStartPos;
    QLabel* ensureSubjectLabel(const QString& subject);
};

