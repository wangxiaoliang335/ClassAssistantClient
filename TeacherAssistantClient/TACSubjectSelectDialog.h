#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QEvent>
#include <QPoint>
#include <QCursor>
#include <QRect>
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollArea>
#include <QWidget>
#include <QTimer>

class TAHttpHandler;
class TACSubjectSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TACSubjectSelectDialog(QWidget* parent = nullptr);
    ~TACSubjectSelectDialog();

    // 设置可用科目列表
    void setSubjects(const QStringList& subjects);
    
    // 获取选中的科目（如果用户点击了某个科目）
    QString selectedSubject() const { return m_selectedSubject; }
    
    // 从服务器获取科目列表
    void fetchSubjectsFromServer();
    
    // 保存科目列表到服务器
    void saveSubjectsToServer();

signals:
    void subjectSelected(const QString& subject);
    void subjectEdited(const QString& oldSubject, const QString& newSubject); // 科目编辑信号

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onSubjectButtonClicked();
    void onSubjectButtonDoubleClicked();
    void onAddButtonClicked();

private:
    void updateCloseButtonPos();
    void createSubjectButtons();
    void handleFetchSubjectsResponse(const QString& response); // 处理获取科目的响应

private:
    QPushButton* m_closeButton = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr; // 滚动区域
    QWidget* m_scrollContent = nullptr; // 滚动内容容器
    QVBoxLayout* m_scrollLayout = nullptr; // 滚动内容布局
    QStringList m_subjects;
    QString m_selectedSubject;
    QPushButton* m_addButton = nullptr;
    QPushButton* m_lastClickedBtn = nullptr; // 记录最后点击的按钮
    QTimer* m_clickTimer = nullptr; // 用于区分单击和双击
    TAHttpHandler* m_httpHandler = nullptr; // HTTP请求处理器
    QString m_classCode; // 当前班级编号
    
    bool m_dragging = false;
    QPoint m_dragStartPos;
    
    static const QStringList DEFAULT_SUBJECTS;
};

