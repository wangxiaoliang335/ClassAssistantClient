#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPoint>
#include <QColor>
#include <QPaintEvent>

class RandomCallMessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RandomCallMessageDialog(QWidget* parent = nullptr);
    
    // 设置对话框标题
    void setTitle(const QString& title);
    
    // 设置消息内容
    void setMessage(const QString& message);
    
    // 设置标题栏背景颜色
    void setTitleBarColor(const QColor& color);
    
    // 设置对话框背景颜色
    void setBackgroundColor(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QLabel* m_titleLabel;
    QLabel* m_messageLabel;
    QPushButton* m_okButton;
    QPushButton* m_closeButton;
    
    QColor m_titleBarColor;
    QColor m_backgroundColor;
    
    QPoint m_dragPosition;
    bool m_dragging;
    
    static const int TITLE_BAR_HEIGHT = 40;
};

