#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QDateEdit>
#include <QMap>
#include <QComboBox>
#include <QStackedWidget>
#include <QMouseEvent>

class HomeworkEditDialog : public QDialog
{
    Q_OBJECT

signals:
    // 新逻辑：选择科目发布
    void homeworkPublishedSingle(const QDate& date, const QString& subject, const QString& content);

public:
    explicit HomeworkEditDialog(QWidget* parent = nullptr);
    
    // 设置当前日期
    void setDate(const QDate& date);
    
    // 设置作业内容（用于编辑已有作业）
    void setHomeworkContent(const QMap<QString, QString>& content);

    // 设置可选科目（会重建编辑区）
    void setAvailableSubjects(const QStringList& subjects);

private slots:
    void onPublishClicked();
    void onCloseClicked();
    void onSubjectChanged(int idx);

private:
    QDateEdit* dateEdit;
    QComboBox* subjectCombo = nullptr;
    QStackedWidget* editorStack = nullptr;
    QMap<QString, QTextEdit*> subjectEdits; // 科目 -> 作业内容编辑框
    QPushButton* btnPublish;
    QPushButton* btnClose;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

