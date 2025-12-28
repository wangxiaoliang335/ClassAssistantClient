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

class TACSubjectInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TACSubjectInputDialog(QWidget* parent = nullptr);
    ~TACSubjectInputDialog();

    void setLabelText(const QString& text);
    void setInitialText(const QString& text);
    QString text() const { return m_lineEdit->text(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    QLineEdit* m_lineEdit = nullptr;
    QPushButton* m_confirmButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QLabel* m_label = nullptr;
    QPushButton* m_closeButton = nullptr;
    
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

