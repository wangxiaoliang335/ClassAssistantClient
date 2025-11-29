#pragma once
#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include "CommonInfo.h"
#include "TAHttpHandler.h"

class ClassInfoDialog : public QDialog
{
    Q_OBJECT

public:
    ClassInfoDialog(QWidget *parent = nullptr);
    ~ClassInfoDialog();

    void InitData(); // 从CommonInfo加载班级信息
    
    void setBackgroundColor(const QColor& color);
    void setBorderColor(const QColor& color);
    void setBorderWidth(int val);
    void setRadius(int val);
    void visibleCloseButton(bool val);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void enterEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onEditClicked();
    void onCancelClicked();
    void onConfirmClicked();

private:
    void setupUI();
    void updateClassInfo();
    void fetchClassInfoFromServer(); // 从服务器获取班级信息

    // UI组件
    QPushButton* m_closeButton;
    QPushButton* m_editButton;
    QPushButton* m_cancelButton;
    QPushButton* m_confirmButton;
    
    QLabel* m_avatarLabel;
    QLabel* m_classNameLabel;
    QLabel* m_classCodeLabel;
    
    QLabel* m_addressLabel;
    QLabel* m_schoolNameLabel;
    QLabel* m_schoolStageLabel;
    QLabel* m_gradeLabel;
    QLabel* m_classLabel;
    
    QLabel* m_addressValueLabel;
    QLabel* m_schoolNameValueLabel;
    QLabel* m_schoolStageValueLabel;
    QLabel* m_gradeValueLabel;
    QLabel* m_classValueLabel;

    // 样式
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth;
    int m_radius;
    bool m_dragging;
    bool m_visibleCloseButton;
    QPoint m_dragStartPos;
    bool m_isEditMode;
    
    // HTTP处理器
    TAHttpHandler* m_httpHandler;
    
    // 处理服务器响应
    void handleClassInfoResponse(const QString& responseString);
};

