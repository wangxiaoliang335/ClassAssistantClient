#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QDate>
#include <QMap>
#include <QMouseEvent>
#include <QScrollArea>
#include <QFrame>

class HomeworkViewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HomeworkViewDialog(QWidget* parent = nullptr);
    
    // 设置日期
    void setDate(const QDate& date);
    
    // 设置作业内容
    void setHomeworkContent(const QMap<QString, QString>& content);

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

