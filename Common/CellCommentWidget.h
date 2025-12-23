#pragma once
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QApplication>
#include <QScreen>
#include <QRect>
#include <QPoint>

// 单元格注释窗口
class CellCommentWidget : public QWidget
{
    Q_OBJECT
public:
    CellCommentWidget(QWidget* parent = nullptr);
    
    void showComment(const QString& text, const QRect& cellRect, QWidget* parentWidget, int spanCols = 1);
    
    void hideWithDelay(int ms = 3000);
    
    void cancelHide();
    
private:
    QLabel* commentLabel;
    QTimer* hideTimer;
};

