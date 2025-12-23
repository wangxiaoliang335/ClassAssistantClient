#pragma once

#include <QObject>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWidget>
#include <QVBoxLayout>
#include <QFileDialog>
class AvatarLabel  : public QLabel
{
	Q_OBJECT
public:
    enum class EditIconPosition {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
    };

    explicit AvatarLabel(QWidget* parent = nullptr)
        : QLabel(parent)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setMouseTracking(true);
    }

    void setAvatar(const QPixmap& pix) {
        avatar = pix;
        update();
    }

    void setEditIcon(const QPixmap& pix) {
        editIcon = pix;
        update();
    }

    void setEditIconPosition(EditIconPosition pos) {
        m_editIconPos = pos;
        update();
    }

    void setEditIconMargin(int px) {
        m_editIconMargin = qMax(0, px);
        update();
    }

signals:
    void avatarClicked();
    void editIconClicked();

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        // 圆形头像
        int side = qMin(width(), height());
        QPainterPath path;
        path.addEllipse(0, 0, side, side);
        painter.setClipPath(path);
        if (!avatar.isNull()) {
            painter.drawPixmap(0, 0, side, side, avatar);
        } else {
            painter.fillPath(path, QColor(120, 120, 120, 80));
        }

        painter.setClipping(false);

        // 覆盖编辑/上传图标
        if (!editIcon.isNull()) {
            const int minIcon = 18;
            iconSize = qMax(minIcon, side / 3);

            int x = 0, y = 0;
            const int m = m_editIconMargin;
            switch (m_editIconPos) {
            case EditIconPosition::TopLeft:
                x = m; y = m; break;
            case EditIconPosition::TopRight:
                x = side - iconSize - m; y = m; break;
            case EditIconPosition::BottomLeft:
                x = m; y = side - iconSize - m; break;
            case EditIconPosition::BottomRight:
            default:
                x = side - iconSize - m; y = side - iconSize - m; break;
            }

            iconX = x;
            iconY = y;
            m_iconRect = QRect(iconX, iconY, iconSize, iconSize);

            // 轻微底色，保证在浅色头像上也看得清
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 90));
            painter.drawEllipse(m_iconRect.adjusted(2, 2, -2, -2));

            painter.drawPixmap(m_iconRect, editIcon);
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (!editIcon.isNull()) {
            if (m_iconRect.contains(event->pos())) {
                emit editIconClicked();
                return;
            }
        }
        emit avatarClicked();
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (!editIcon.isNull() && m_iconRect.contains(event->pos())) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        QLabel::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        setCursor(Qt::ArrowCursor);
        QLabel::leaveEvent(event);
    }

private:
    QPixmap avatar;
    QPixmap editIcon;
    int iconX{ 0 };
    int iconY{ 0 };
    int iconSize{ 0 };
    QRect m_iconRect;
    EditIconPosition m_editIconPos{ EditIconPosition::BottomRight };
    int m_editIconMargin{ 0 };
};

