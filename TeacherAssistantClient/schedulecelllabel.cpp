#include "schedulecelllabel.h"

#include <QApplication>
#include <QDataStream>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFontMetricsF>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QStringList>
#include <QVariantAnimation>

ScheduleCellLabel::ScheduleCellLabel(QWidget *parent)
    : QLabel(parent)
{
    setAlignment(Qt::AlignCenter);
    setWordWrap(true);
    setMargin(8);
    setMinimumHeight(48);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    baseFont = font();
    if (basePointSize() <= 0)
        baseFont.setPointSizeF(12.0);
    setFont(baseFont);
}

void ScheduleCellLabel::setCourseInfo(const CourseInfo &info)
{
    currentInfo = info;

    if (info.name.trimmed().isEmpty() && info.type == CourseType::None)
        setText(QString());
    else
        setText(info.name);

    const QColor background = resolveBackground(info);
    const QColor foreground = resolveTextColor(info.type);
    const QString border = borderStyle(info.type);

    const QString textDecoration = (info.type == CourseType::Deleted) ? "text-decoration:line-through;" : QString();
    const bool needsLeftTag = (info.type == CourseType::Temporary || info.type == CourseType::Deleted || info.type == CourseType::DeletedTemporary);
    const QString paddingStyle = needsLeftTag ? QStringLiteral("padding:6px; padding-left:32px;") : QStringLiteral("padding:6px;");
    const QString style = QString("QLabel{background:%1; color:%2; border:%3; border-radius:6px; %4 font-weight:600;%5}")
                              .arg(background.name(QColor::HexRgb), foreground.name(QColor::HexRgb), border, paddingStyle, textDecoration);
    setStyleSheet(style);
    adjustFontToContents();
}

void ScheduleCellLabel::setCellCoordinates(int row, int column)
{
    cellRow = row;
    cellColumn = column;
}

void ScheduleCellLabel::setDragEnabled(bool enabled)
{
    dragEnabled = enabled;
    setAcceptDrops(enabled);
    if (enabled)
        setCursor(Qt::OpenHandCursor);
    else
        unsetCursor();
}

void ScheduleCellLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    adjustFontToContents();
}

void ScheduleCellLabel::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);

    auto drawTag = [this](const QString &text, const QColor &color) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QRect tagRect(6, 6, 32, 18);
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(tagRect, 7, 7);
        painter.setPen(Qt::white);
        QFont tagFont = font();
        tagFont.setPointSizeF(qMax(8.0, tagFont.pointSizeF() - 3.0));
        tagFont.setBold(true);
        painter.setFont(tagFont);
        painter.drawText(tagRect, Qt::AlignCenter, text);
    };

    auto drawStripe = [this]() {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setClipRect(rect().adjusted(3, 3, -3, -3));
        QPen pen(QColor(0, 0, 0, 25), 2);
        painter.setPen(pen);
        for (int x = -height(); x < width(); x += 8)
            painter.drawLine(x, 0, x + height(), height());
        painter.setPen(QPen(QColor(0, 0, 0, 18), 1, Qt::DotLine));
        painter.drawLine(rect().left() + 4, rect().top() + 8, rect().right() - 4, rect().bottom() - 4);
    };

    auto drawBrokenPieces = [this]() {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setPen(Qt::NoPen);
        QColor shard = QColor("#9ca3af");
        shard.setAlpha(90);
        const QRect area = rect().adjusted(6, 6, -6, -6);
        for (int i = 0; i < 4; ++i)
        {
            QPoint p1(area.left() + (i * 7), area.top() + 4);
            QPoint p2(p1.x() + 10, p1.y() + 6);
            QPoint p3(p1.x() - 4, p1.y() + 14);
            painter.setBrush(shard);
            painter.drawPolygon(QPolygon() << p1 << p2 << p3);
        }
        painter.setBrush(QColor(255, 255, 255, 70));
        painter.drawRect(area.adjusted(10, 14, -12, -32));
    };

    auto drawGradient = [this](const QColor &color) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QLinearGradient grad(rect().topLeft(), rect().topRight());
        QColor start = color;
        start.setAlpha(110);
        QColor fade = color;
        fade.setAlpha(10);
        grad.setColorAt(0.0, start);
        grad.setColorAt(0.4, fade);
        grad.setColorAt(1.0, Qt::transparent);
        painter.fillRect(rect().adjusted(2, 2, -2, -2), grad);
    };

    if (currentInfo.type == CourseType::Temporary)
    {
        drawGradient(QColor("#f97316"));
        drawTag(QString::fromUtf8("临"), QColor("#f97316"));
    }
    else if (currentInfo.type == CourseType::DeletedTemporary)
    {
        drawGradient(QColor("#2563eb"));
        drawTag(QString::fromUtf8("换"), QColor("#2563eb"));
    }
    else if (currentInfo.type == CourseType::Deleted)
    {
        drawStripe();
        drawTag(QString::fromUtf8("删"), QColor("#6b7280"));
        drawBrokenPieces();
    }

    if (currentInfo.type == CourseType::Deleted)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor("#9ca3af"), 2));
        const QPoint left(6, height() / 2);
        const QPoint right(width() - 6, height() / 2);
        painter.drawLine(left, right);
    }

    if (dropGlowOpacity > 0.0)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QColor glow = QColor("#2563eb");
        glow.setAlphaF(qBound(0.0, dropGlowOpacity, 1.0));
        QPen pen(glow, 3);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        QRectF borderRect = rect().adjusted(2, 2, -2, -2);
        painter.drawRoundedRect(borderRect, 10, 10);
    }
}

void ScheduleCellLabel::mousePressEvent(QMouseEvent *event)
{
    if (dragEnabled && event->button() == Qt::LeftButton)
    {
        dragStartPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    QLabel::mousePressEvent(event);
}

void ScheduleCellLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (dragEnabled && (event->buttons() & Qt::LeftButton))
    {
        const int distance = (event->pos() - dragStartPos).manhattanLength();
        if (distance >= QApplication::startDragDistance())
            startDrag(event->pos());
    }
    QLabel::mouseMoveEvent(event);
}

void ScheduleCellLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (dragEnabled && event->button() == Qt::LeftButton)
        setCursor(Qt::OpenHandCursor);
    QLabel::mouseReleaseEvent(event);
}

void ScheduleCellLabel::dragEnterEvent(QDragEnterEvent *event)
{
    if (!dragEnabled)
    {
        event->ignore();
        return;
    }
    if (event->mimeData()->hasFormat("application/x-schedule-cell"))
        event->acceptProposedAction();
    else if (event->mimeData()->hasFormat("application/x-weekly-course"))
        event->acceptProposedAction();
    else
        event->ignore();
}

void ScheduleCellLabel::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    dropGlowOpacity = 0.0;
    update();
    QLabel::dragLeaveEvent(event);
}

void ScheduleCellLabel::dropEvent(QDropEvent *event)
{
    if (!dragEnabled || cellRow < 0 || cellColumn < 0)
    {
        event->ignore();
        return;
    }

    if (event->mimeData()->hasFormat("application/x-schedule-cell"))
    {
        QByteArray encoded = event->mimeData()->data("application/x-schedule-cell");
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        int sourceRow = -1;
        int sourceColumn = -1;
        QString name;
        QColor color;
        int typeValue = 0;
        stream >> sourceRow >> sourceColumn >> name >> color >> typeValue;

        event->acceptProposedAction();
        startDropGlow();
        emit requestDropOperation(sourceRow, sourceColumn, cellRow, cellColumn, name, color, typeValue);
        return;
    }

    if (event->mimeData()->hasFormat("application/x-weekly-course"))
    {
        QByteArray encoded = event->mimeData()->data("application/x-weekly-course");
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        QString name;
        QColor color;
        stream >> name >> color;
        event->acceptProposedAction();
        startDropGlow();
        emit requestWeeklyCourseDrop(cellRow, cellColumn, name, color);
        return;
    }

    event->ignore();
}

QColor ScheduleCellLabel::resolveBackground(const CourseInfo &info) const
{
    if (info.type == CourseType::Deleted)
        return QColor("#d1d5db");

    if (info.background.isValid())
        return info.background;

    switch (info.type)
    {
    case CourseType::Temporary:
        return QColor("#fdefc3");
    case CourseType::Deleted:
        return QColor("#d1d5db");
    case CourseType::DeletedTemporary:
        return QColor("#f8d7da");
    case CourseType::Normal:
        return QColor("#dff1f1");
    case CourseType::None:
    default:
        return QColor("#ffffff");
    }
}

QColor ScheduleCellLabel::resolveTextColor(CourseType type) const
{
    switch (type)
    {
    case CourseType::Deleted:
        return QColor("#666666");
    case CourseType::None:
        return QColor("#999999");
    default:
        return QColor("#222222");
    }
}

QString ScheduleCellLabel::borderStyle(CourseType type) const
{
    switch (type)
    {
    case CourseType::Deleted:
        return "1px dashed #c0c0c0";
    case CourseType::Temporary:
        return "1px solid #f0a500";
    case CourseType::DeletedTemporary:
        return "1px dashed #d9534f";
    case CourseType::Normal:
        return "1px solid #7cb7ad";
    case CourseType::None:
    default:
        return "1px solid #e0e0e0";
    }
}

void ScheduleCellLabel::adjustFontToContents()
{
    const QRect area = contentsRect();
    if (!area.isValid() || area.width() <= 0 || area.height() <= 0)
        return;

    const QString textValue = text();
    QFont workingFont = baseFont;
    if (textValue.isEmpty())
    {
        setFont(workingFont);
        return;
    }

    const QStringList lines = textValue.split('\n', Qt::KeepEmptyParts);
    const qreal availableWidth = qMax(1, area.width() - 6);
    const qreal availableHeight = qMax(1, area.height() - 4);

    QFontMetricsF metrics(workingFont);
    qreal maxLineWidth = 0.0;
    for (const QString &line : lines)
        maxLineWidth = qMax(maxLineWidth, metrics.horizontalAdvance(line));
    if (maxLineWidth <= 0)
        maxLineWidth = 1;

    const int lineCount = qMax(1, lines.size());
    const qreal totalHeight = metrics.lineSpacing() * lineCount;
    if (totalHeight <= 0)
        return;

    const qreal widthRatio = availableWidth / maxLineWidth;
    const qreal heightRatio = availableHeight / totalHeight;
    qreal ratio = qMin(widthRatio, heightRatio);
    if (ratio <= 0)
        ratio = 0.1;
    ratio *= 0.9; // 留出视觉缓冲让文字在背景块中显得更协调

    workingFont.setPointSizeF(basePointSize() * ratio);

    auto fitsArea = [&](const QFont &font) -> bool {
        QFontMetricsF checkMetrics(font);
        qreal widest = 0.0;
        for (const QString &line : lines)
            widest = qMax(widest, checkMetrics.horizontalAdvance(line));
        const qreal heightNeeded = checkMetrics.lineSpacing() * lineCount;
        return widest <= availableWidth && heightNeeded <= availableHeight;
    };

    while (!fitsArea(workingFont) && workingFont.pointSizeF() > 1.0)
    {
        workingFont.setPointSizeF(workingFont.pointSizeF() - 0.5);
    }

    setFont(workingFont);
}

qreal ScheduleCellLabel::basePointSize() const
{
    if (baseFont.pointSizeF() > 0)
        return baseFont.pointSizeF();
    if (baseFont.pixelSize() > 0)
        return baseFont.pixelSize();
    return 12.0;
}

void ScheduleCellLabel::startDrag(const QPoint &hotSpot)
{
    if (!dragEnabled || cellRow < 0 || cellColumn < 0)
        return;
    if ((currentInfo.type == CourseType::None && currentInfo.name.trimmed().isEmpty()) || currentInfo.type == CourseType::Deleted)
        return;

    auto *drag = new QDrag(this);
    auto *mime = new QMimeData();
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << cellRow << cellColumn << currentInfo.name << currentInfo.background << static_cast<int>(currentInfo.type);
    mime->setData("application/x-schedule-cell", payload);
    drag->setMimeData(mime);

    QPixmap pixmap(size());
    render(&pixmap);

    QPixmap shadow(pixmap.size() + QSize(16, 16));
    shadow.fill(Qt::transparent);
    QPainter painter(&shadow);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(0.18);
    painter.setBrush(Qt::black);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRect(6, 6, pixmap.width() + 4, pixmap.height() + 4), 12, 12);
    painter.setOpacity(1.0);
    painter.drawPixmap(0, 0, pixmap);
    painter.end();

    drag->setPixmap(shadow);
    drag->setHotSpot(hotSpot);
    drag->exec(Qt::MoveAction);
    if (dragEnabled)
        setCursor(Qt::OpenHandCursor);
}

void ScheduleCellLabel::startDropGlow()
{
    if (!dropAnimation)
    {
        dropAnimation = new QVariantAnimation(this);
        dropAnimation->setEasingCurve(QEasingCurve::OutQuad);
        connect(dropAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            dropGlowOpacity = value.toReal();
            update();
        });
        connect(dropAnimation, &QVariantAnimation::finished, this, [this]() {
            dropGlowOpacity = 0.0;
            update();
        });
    }
    dropAnimation->stop();
    dropAnimation->setDuration(280);
    dropAnimation->setStartValue(0.8);
    dropAnimation->setEndValue(0.0);
    dropAnimation->start();
}

void ScheduleCellLabel::setTodayHighlighted(bool highlighted)
{
    Q_UNUSED(highlighted);
}
