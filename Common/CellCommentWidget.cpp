#include "CellCommentWidget.h"
#include <QString>
#include <QApplication>
#include <QScreen>
#include <QRect>
#include <QPoint>
#include <algorithm>

CellCommentWidget::CellCommentWidget(QWidget* parent) : QWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet(
        "QWidget { background-color: #ffa500; color: white; border: 1px solid #888; border-radius: 4px; padding: 8px; }"
    );
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    
    commentLabel = new QLabel("");
    commentLabel->setWordWrap(true);
    commentLabel->setStyleSheet("color: white; font-size: 12px;");
    layout->addWidget(commentLabel);
    
    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    connect(hideTimer, &QTimer::timeout, this, &QWidget::hide);
}

void CellCommentWidget::showComment(const QString& text, const QRect& cellRect, QWidget* parentWidget, int spanCols)
{
    commentLabel->setText(text.isEmpty() ? "(无注释)" : text);
    commentLabel->adjustSize();
    
    // 计算窗口大小
    int width = cellRect.width() * spanCols + 16;
    int height = commentLabel->height() + 16;
    resize(width, qMax(height, 40));
    
    // 计算窗口位置（在单元格右边，稍微下移）
    QPoint globalPos = parentWidget->mapToGlobal(cellRect.topRight());
    int x = globalPos.x() + 5; // 在单元格右边5像素
    int y = globalPos.y() + 50; // 下移50像素，使窗口更靠下
    
    // 确保窗口不超出屏幕
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenRect = screen->geometry();
        // 如果右边放不下，放到左边
        if (x + width > screenRect.right()) {
            QPoint leftPos = parentWidget->mapToGlobal(cellRect.topLeft());
            x = leftPos.x() - width - 5; // 在单元格左边5像素
        }
        if (x < screenRect.left()) {
            x = screenRect.left();
        }
        // 如果下边放不下，调整到上方
        if (y + height > screenRect.bottom()) {
            y = globalPos.y() - height - 5; // 改为在单元格上方
        }
        if (y < screenRect.top()) {
            y = screenRect.top();
        }
    }
    
    move(x, y);
    show();
}

void CellCommentWidget::hideWithDelay(int ms)
{
    hideTimer->start(ms);
}

void CellCommentWidget::cancelHide()
{
    hideTimer->stop();
}

