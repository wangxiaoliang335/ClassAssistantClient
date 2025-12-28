#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QEvent>
#include <QCursor>
#include <QRect>

class TACSubjectEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TACSubjectEditDialog(QWidget* parent = nullptr);
    ~TACSubjectEditDialog();

    void setSubjectName(const QString& name);
    void setLabelText(const QString& text);
    void setInitialText(const QString& text);
    void setTitleText(const QString& text);
    QString subjectName() const;
    QString text() const; // 兼容原有接口

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    void updateCloseButtonPos();

private:
    QPushButton* m_closeButton = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_promptLabel = nullptr;
    QLineEdit* m_subjectEdit = nullptr;
    QPushButton* m_confirmButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

