#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QEvent>
#include <QCursor>
#include <QRect>

class TACWarningDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TACWarningDialog(QWidget* parent = nullptr);
    ~TACWarningDialog();
    
    void setTitle(const QString& title);
    void setMessage(const QString& message);
    
    static int warning(QWidget* parent, const QString& title, const QString& message);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onOkClicked();

private:
    void updateCloseButtonPos();

private:
    QPushButton* m_closeButton = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_messageLabel = nullptr;
    QPushButton* m_okButton = nullptr;
    
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

