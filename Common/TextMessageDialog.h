#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPoint>

class TextMessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TextMessageDialog(const QString& className, QWidget* parent = nullptr);
    
    // 获取输入的文本消息
    QString getMessage() const;
    
    // 设置未读消息数
    void setUnreadCount(int count);
    
    // 清空文本输入框
    void clearMessage();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onSendClicked();
    void onCancelClicked();

private:
    QLabel* m_titleLabel;
    QLabel* m_unreadCountLabel;
    QPushButton* m_closeButton;
    QTextEdit* m_textEdit;
    QPushButton* m_cancelButton;
    QPushButton* m_sendButton;
    
    QPoint m_dragPosition;
    bool m_dragging;
    
    QString m_className;
};

