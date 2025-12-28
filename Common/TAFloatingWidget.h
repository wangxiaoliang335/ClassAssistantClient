#pragma once
#include <QWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
class TAFloatingWidget  : public QWidget
{
	Q_OBJECT

public:
	explicit TAFloatingWidget(QWidget *parent);
	~TAFloatingWidget();
    Q_PROPERTY(QString titleName READ titleName WRITE setTitleName)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)
    Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth)
    Q_PROPERTY(int radius READ radius WRITE setRadius)

    QString titleName() const;
    QColor backgroundColor() const;
    QColor borderColor() const;
    QRect getScreenGeometryWithTaskbar();
    int radius();
    int borderWidth();
    void visibleCloseButton(bool val);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    virtual void initShow() = 0;
   
 
public slots:
    void setTitleName(const QString& name);
    void setBackgroundColor(const QColor& color);
    void setBorderColor(const QColor& color);
    void setBorderWidth(int val);
    void setRadius(int val);
private:
    bool m_dragging;
    QPoint m_dragStartPosition;
    QString m_titleName;
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth;
    int m_radius;
    QPushButton* closeButton;
    QPushButton* minimizeButton; // 最小化/隐藏按钮
    bool m_visibleCloseButton;
};
