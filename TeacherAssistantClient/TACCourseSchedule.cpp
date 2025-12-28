#pragma execution_character_set("utf-8")
#include "TACCourseSchedule.h"
#include "TAHttpHandler.h"
#include "CommonInfo.h"
#include "PrepareClassEditDialog.h"
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QDebug>
#include <QDate>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include "common.h"

// CourseButtonWidget 实现
CourseButtonWidget::CourseButtonWidget(const QString& subject, const QString& time, QWidget* parent)
    : QWidget(parent), m_subject(subject), m_time(time), m_hasPrepareClass(false), m_pressed(false), m_hovered(false)
{
    setFixedHeight(40);
    setCursor(Qt::PointingHandCursor);
}

void CourseButtonWidget::setHasPrepareClass(bool hasPrepare)
{
    m_hasPrepareClass = hasPrepare;
    update();
}

void CourseButtonWidget::setPrepareClassContent(const QString& content)
{
    m_prepareClassContent = content;
}

void CourseButtonWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    QColor bgColor = m_hovered ? QColor(0x4A, 0x4A, 0x4A) : QColor(0x3C, 0x3C, 0x3C);
    if (m_pressed) {
        bgColor = QColor(0x35, 0x35, 0x35);
    }
    
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);
    painter.fillPath(path, bgColor);
    
    // 绘制科目文本
    painter.setPen(Qt::white);
    QFont font;
    font.setPointSize(14);
    painter.setFont(font);
    QRect textRect = rect();
    painter.drawText(textRect, Qt::AlignCenter, m_subject);
    
    // 如果有课前准备，在右上角绘制绿色图标
    if (m_hasPrepareClass) {
        QRect iconRect(width() - 18, 4, 14, 14);
        painter.setBrush(QColor(0x4CAF50)); // 绿色
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(iconRect);
        
        // 绘制白色星形图标
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(Qt::white);
        QPainterPath starPath;
        QPoint center = iconRect.center();
        // 绘制一个简单的星形（5角星）
        for (int i = 0; i < 5; ++i) {
            double angle = (i * 4 * M_PI / 5) - M_PI / 2;
            double x = center.x() + 4 * cos(angle);
            double y = center.y() + 4 * sin(angle);
            if (i == 0) {
                starPath.moveTo(x, y);
            } else {
                starPath.lineTo(x, y);
            }
        }
        starPath.closeSubpath();
        painter.drawPath(starPath);
    }
}

void CourseButtonWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        update();
    }
}

void CourseButtonWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_pressed) {
        m_pressed = false;
        update();
        
        QPoint pos = event->pos();
        // 如果点击的是右上角的图标区域
        QRect iconRect(width() - 18, 4, 14, 14);
        if (iconRect.contains(pos) && m_hasPrepareClass) {
            emit prepareClassClicked(m_subject, m_time);
        } else {
            emit clicked();
        }
    }
}

void CourseButtonWidget::enterEvent(QEvent* event)
{
    Q_UNUSED(event);
    m_hovered = true;
    update();
}

void CourseButtonWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    m_hovered = false;
    m_pressed = false;
    update();
}

TACCourseSchedule::TACCourseSchedule(QWidget* parent) : TAFloatingWidget(parent), m_httpHandler(nullptr)
{
    this->setObjectName("TACCourseSchedule");

    this->resize(QSize(200, 500));
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(30);

    contentLayout = new QVBoxLayout();
    this->contentLayout->setContentsMargins(10, 50, 10, 10);
    this->contentLayout->setSpacing(0);
    this->setLayout(this->contentLayout);
    
    // 标题标签（显示今天的星期几）
    QDate today = QDate::currentDate();
    QStringList weekdays = {
        QString::fromUtf8(u8"星期一"),
        QString::fromUtf8(u8"星期二"),
        QString::fromUtf8(u8"星期三"),
        QString::fromUtf8(u8"星期四"),
        QString::fromUtf8(u8"星期五"),
        QString::fromUtf8(u8"星期六"),
        QString::fromUtf8(u8"星期日")
    };
    int dayOfWeek = today.dayOfWeek(); // 1=Monday, 7=Sunday
    QString weekdayName = weekdays.value(dayOfWeek - 1, QString::fromUtf8(u8"未知"));
    
    m_titleLabel = new QLabel(weekdayName, this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold; background: transparent; padding: 10px;");
    this->contentLayout->addWidget(m_titleLabel);
    
    // 创建滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(
        "QScrollArea {"
        "background-color: transparent;"
        "border: none;"
        "}"
        "QScrollBar:vertical {"
        "background-color: rgba(60, 60, 60, 200);"
        "width: 8px;"
        "border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "background-color: rgba(120, 120, 120, 200);"
        "min-height: 20px;"
        "border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "background-color: rgba(150, 150, 150, 200);"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "height: 0px;"
        "}"
    );
    
    // 创建滚动内容容器
    m_scrollContent = new QWidget();
    classLayout = new QVBoxLayout(m_scrollContent);
    classLayout->setContentsMargins(0, 0, 0, 0);
    classLayout->setSpacing(8);
    
    m_scrollArea->setWidget(m_scrollContent);
    this->contentLayout->addWidget(m_scrollArea, 1);
    
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &TACCourseSchedule::updateTimeSlots);
    //timer->start(60 * 1000);
}

TACCourseSchedule::~TACCourseSchedule()
{
}

void TACCourseSchedule::fetchTodaySchedule(const QString& classId)
{
    m_currentClassId = classId;
    
    if (classId.isEmpty()) {
        qWarning() << "Class ID is empty, cannot fetch today schedule";
        return;
    }
    
    if (!m_httpHandler) {
        m_httpHandler = new TAHttpHandler(this);
    }
    
    const QString term = currentTermString();
    QUrl url(QStringLiteral("http://47.100.126.194:5000/course-schedule"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("class_id"), classId);
    query.addQueryItem(QStringLiteral("term"), term);
    url.setQuery(query);
    
    connect(m_httpHandler, &TAHttpHandler::success, this, [this](const QString& responseString) {
        m_httpHandler->disconnect(this);
        handleScheduleResponse(responseString);
    });
    
    connect(m_httpHandler, &TAHttpHandler::failed, this, [this](const QString& error) {
        m_httpHandler->disconnect(this);
        qWarning() << "Today schedule request failed:" << error;
        applyDefaultDailySchedule(); // 请求失败时使用默认日程表
    });
    
    m_httpHandler->get(url.toString());
    qDebug() << "Fetching today schedule:" << url.toString();
}

QString TACCourseSchedule::currentTermString() const
{
    QDate today = QDate::currentDate();
    int year = today.year();
    int month = today.month();
    
    int startYear = year;
    int endYear = year + 1;
    int termIndex = 1;
    
    if (month >= 9) {
        startYear = year;
        endYear = year + 1;
        termIndex = 1;
    } else if (month >= 3 && month <= 8) {
        startYear = year - 1;
        endYear = year;
        termIndex = 2;
    } else {
        startYear = year - 1;
        endYear = year;
        termIndex = 1;
    }
    
    return QString("%1-%2-%3").arg(startYear).arg(endYear).arg(termIndex);
}

QStringList TACCourseSchedule::jsonArrayToStringList(const QJsonArray& arr) const
{
    QStringList list;
    for (const QJsonValue& value : arr) {
        list << value.toString();
    }
    return list;
}

int TACCourseSchedule::getTodayColumnIndex(const QStringList& days) const
{
    QDate today = QDate::currentDate();
    int dayOfWeek = today.dayOfWeek(); // 1=Monday, 7=Sunday
    
    // 如果今天是周六或周日，返回-1（不在周一到周五范围内）
    if (dayOfWeek > 5) {
        return -1;
    }
    
    // 查找对应的列索引（周一=0, 周二=1, ...）
    QStringList weekdays = {
        QString::fromUtf8(u8"周一"),
        QString::fromUtf8(u8"周二"),
        QString::fromUtf8(u8"周三"),
        QString::fromUtf8(u8"周四"),
        QString::fromUtf8(u8"周五")
    };
    
    QString expectedDay = weekdays.value(dayOfWeek - 1);
    return days.indexOf(expectedDay);
}

QStringList TACCourseSchedule::defaultSubjectsForWeekday(int weekday) const
{
    static const QMap<int, QStringList> subjectsByWeekday = {
        {1, {QString::fromUtf8(u8"晨读"), QString::fromUtf8(u8"语文"), QString::fromUtf8(u8"数学"), QString::fromUtf8(u8"英语"), QString::fromUtf8(u8"物理"), QString::fromUtf8(u8"午饭"), QString::fromUtf8(u8"午休"), QString::fromUtf8(u8"数学"), QString::fromUtf8(u8"美术"), QString::fromUtf8(u8"道法"), QString::fromUtf8(u8"课服"), QString::fromUtf8(u8"晚自习"), QString()}},
        {2, {QString::fromUtf8(u8"晨读"), QString::fromUtf8(u8"语文"), QString::fromUtf8(u8"数学"), QString::fromUtf8(u8"英语"), QString::fromUtf8(u8"化学"), QString::fromUtf8(u8"午饭"), QString::fromUtf8(u8"午休"), QString::fromUtf8(u8"物理"), QString::fromUtf8(u8"信息"), QString::fromUtf8(u8"体育"), QString::fromUtf8(u8"课服"), QString::fromUtf8(u8"晚自习"), QString()}},
        {3, {QString::fromUtf8(u8"晨读"), QString::fromUtf8(u8"语文"), QString::fromUtf8(u8"数学"), QString::fromUtf8(u8"英语"), QString::fromUtf8(u8"生物"), QString::fromUtf8(u8"午饭"), QString::fromUtf8(u8"午休"), QString::fromUtf8(u8"数学"), QString::fromUtf8(u8"历史"), QString::fromUtf8(u8"地理"), QString::fromUtf8(u8"课服"), QString::fromUtf8(u8"晚自习"), QString()}},
        {4, {QString::fromUtf8(u8"晨读"), QString::fromUtf8(u8"语文"), QString::fromUtf8(u8"数学"), QString::fromUtf8(u8"英语"), QString::fromUtf8(u8"政治"), QString::fromUtf8(u8"午饭"), QString::fromUtf8(u8"午休"), QString::fromUtf8(u8"数学"), QString::fromUtf8(u8"音乐"), QString::fromUtf8(u8"美术"), QString::fromUtf8(u8"课服"), QString::fromUtf8(u8"晚自习"), QString()}},
        {5, {QString::fromUtf8(u8"晨读"), QString::fromUtf8(u8"语文"), QString::fromUtf8(u8"数学"), QString::fromUtf8(u8"英语"), QString::fromUtf8(u8"综合实践"), QString::fromUtf8(u8"午饭"), QString::fromUtf8(u8"午休"), QString::fromUtf8(u8"数学"), QString::fromUtf8(u8"信息"), QString::fromUtf8(u8"体育"), QString::fromUtf8(u8"课服"), QString::fromUtf8(u8"晚自习"), QString()}},
        {6, {QString::fromUtf8(u8"晨读"), QString::fromUtf8(u8"自习"), QString::fromUtf8(u8"自习"), QString::fromUtf8(u8"自习"), QString::fromUtf8(u8"自习"), QString::fromUtf8(u8"午饭"), QString::fromUtf8(u8"午休"), QString::fromUtf8(u8"兴趣"), QString::fromUtf8(u8"兴趣"), QString::fromUtf8(u8"综合"), QString::fromUtf8(u8"课服"), QString::fromUtf8(u8"晚自习"), QString()}},
        {7, {QString::fromUtf8(u8"晨读"), QString::fromUtf8(u8"休息"), QString::fromUtf8(u8"休息"), QString::fromUtf8(u8"休息"), QString::fromUtf8(u8"休息"), QString::fromUtf8(u8"午饭"), QString::fromUtf8(u8"午休"), QString::fromUtf8(u8"休息"), QString::fromUtf8(u8"休息"), QString::fromUtf8(u8"休息"), QString::fromUtf8(u8"课服"), QString::fromUtf8(u8"晚自习"), QString()}}
    };
    return subjectsByWeekday.value(weekday, subjectsByWeekday.value(1));
}

QStringList TACCourseSchedule::defaultTimesForSubjects(const QStringList& subjects) const
{
    QStringList slotDefaults = { QStringLiteral("07:20"), QStringLiteral("08:00"), QStringLiteral("08:45"), QStringLiteral("09:35"), QStringLiteral("10:25"), QStringLiteral("12:00"), QStringLiteral("12:40"), QStringLiteral("14:00"), QStringLiteral("14:45"), QStringLiteral("15:35"), QStringLiteral("17:00"), QStringLiteral("19:00"), QStringLiteral("20:30") };
    QString tutoringTime = QStringLiteral("17:00");

    QStringList times;
    times.reserve(subjects.size());
    for (int i = 0; i < subjects.size(); ++i) {
        QString subject = subjects[i];
        QString slotTime = slotDefaults.value(i, QStringLiteral("--:--"));
        if (subject == QString::fromUtf8(u8"课服")) {
            slotTime = tutoringTime;
        }
        times.append(slotTime);
    }
    return times;
}

QPair<QStringList, QStringList> TACCourseSchedule::buildDefaultDailySchedule() const
{
    int weekday = QDate::currentDate().dayOfWeek(); // 1=Monday ... 7=Sunday
    QStringList subs = defaultSubjectsForWeekday(weekday);
    QStringList times = defaultTimesForSubjects(subs);
    return qMakePair(times, subs);
}

void TACCourseSchedule::applyDefaultDailySchedule()
{
    // 清除现有的课程按钮
    cleanLayout(classLayout);
    
    // 检查是否为周六或周日
    QDate today = QDate::currentDate();
    int dayOfWeek = today.dayOfWeek(); // 1=Monday, 7=Sunday
    if (dayOfWeek > 5) {
        // 周六周日不显示科目
        classLayout->addStretch();
        return;
    }
    
    auto dailySchedule = buildDefaultDailySchedule();
    const QStringList& defaultTimes = dailySchedule.first;
    // 显示默认日程表
    for (int i = 0; i < dailySchedule.second.size(); ++i) {
        const QString& subject = dailySchedule.second[i];
        if (subject.isEmpty()) continue;
        
        QString timeStr = defaultTimes.value(i, QString());
        CourseButtonWidget* btn = new CourseButtonWidget(subject, timeStr, m_scrollContent);
        
        // 检查是否有课前准备
        QString prepareKey = prepareClassCacheKey(subject, timeStr);
        bool hasPrepare = m_prepareClassCache.contains(prepareKey) && !m_prepareClassCache[prepareKey].isEmpty();
        btn->setHasPrepareClass(hasPrepare);
        if (hasPrepare) {
            btn->setPrepareClassContent(m_prepareClassCache[prepareKey]);
        }
        
        connect(btn, &CourseButtonWidget::prepareClassClicked, this, &TACCourseSchedule::onCourseButtonClicked);
        classLayout->addWidget(btn);
    }
    classLayout->addStretch();
}

void TACCourseSchedule::handleScheduleResponse(const QString& resp)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Today schedule response parse failed:" << parseError.errorString();
        applyDefaultDailySchedule(); // 使用默认日程表
        return;
    }
    
    QJsonObject obj = doc.object();
    int code = obj.value(QStringLiteral("code")).toInt(-1);
    QString message = obj.value(QStringLiteral("message")).toString();
    
    if (code != 200) {
        qWarning() << "Today schedule API returned error:" << code << message;
        applyDefaultDailySchedule(); // 使用默认日程表
        return;
    }
    
    QJsonObject dataObj = obj.value(QStringLiteral("data")).toObject();
    QJsonObject scheduleObj = dataObj.value(QStringLiteral("schedule")).toObject();
    QStringList days = jsonArrayToStringList(scheduleObj.value(QStringLiteral("days")).toArray());
    QStringList times = jsonArrayToStringList(scheduleObj.value(QStringLiteral("times")).toArray());
    QJsonArray cells = dataObj.value(QStringLiteral("cells")).toArray();
    
    if (times.isEmpty()) {
        applyDefaultDailySchedule(); // 如果服务器返回空数据，使用默认日程表
        return;
    }
    
    applyTodaySchedule(days, times, cells);
}

void TACCourseSchedule::applyTodaySchedule(const QStringList& days, const QStringList& times, const QJsonArray& cells)
{
    // 清除现有的课程按钮
    cleanLayout(classLayout);
    
    // 获取今天的列索引
    int todayCol = getTodayColumnIndex(days);
    if (todayCol < 0 || todayCol >= days.size()) {
        qWarning() << "Today is not in the course schedule range (may be weekend)";
        // 周六周日不显示科目
        classLayout->addStretch();
        return;
    }
    
    // 使用与ScheduleDialog相同的数据源
    auto defaultSchedule = buildDefaultDailySchedule();
    const QStringList& defaultSubjects = defaultSchedule.second;
    const QStringList& defaultTimes = defaultSchedule.first;
    
    // 解析单元格数据，提取今天的课程
    QMap<int, QString> todayCourses; // rowIndex -> courseName
    bool hasLeadingTimeColumn = false;
    for (const QJsonValue& value : cells) {
        if (!value.isObject()) continue;
        QJsonObject cell = value.toObject();
        int colIndex = cell.value(QStringLiteral("col_index")).toInt(-1);
        QString courseName = cell.value(QStringLiteral("course_name")).toString();
        if (colIndex == 0 && !courseName.isEmpty()) {
            // 检查是否是时间列（简单检查：包含冒号或时间格式）
            if (courseName.contains(QStringLiteral(":")) || courseName.contains(QStringLiteral("-"))) {
                hasLeadingTimeColumn = true;
            }
        }
    }
    
    int targetCol = todayCol;
    if (hasLeadingTimeColumn) {
        targetCol += 1; // 如果第一列是时间列，今天的数据列索引需要+1
    }
    
    for (const QJsonValue& value : cells) {
        if (!value.isObject()) continue;
        QJsonObject cell = value.toObject();
        int colIndex = cell.value(QStringLiteral("col_index")).toInt(-1);
        int rowIndex = cell.value(QStringLiteral("row_index")).toInt(-1);
        QString courseName = cell.value(QStringLiteral("course_name")).toString();
        
        if (colIndex == targetCol && !courseName.isEmpty() && rowIndex >= 0 && rowIndex < defaultSubjects.size()) {
            todayCourses[rowIndex] = courseName;
        }
    }
    
    // 创建课程列表按钮（使用与ScheduleDialog相同的科目和时间列表）
    for (int i = 0; i < defaultSubjects.size(); ++i) {
        const QString& defaultSubject = defaultSubjects[i];
        if (defaultSubject.isEmpty()) continue;
        
        QString courseName = todayCourses.value(i, QString());
        // 如果服务器没有返回该行的课程，使用默认科目
        if (courseName.isEmpty()) {
            courseName = defaultSubject;
        }
        
        QString timeStr = defaultTimes.value(i, QString());
        CourseButtonWidget* btn = new CourseButtonWidget(courseName, timeStr, m_scrollContent);
        
        // 检查是否有课前准备
        QString prepareKey = prepareClassCacheKey(courseName, timeStr);
        bool hasPrepare = m_prepareClassCache.contains(prepareKey) && !m_prepareClassCache[prepareKey].isEmpty();
        btn->setHasPrepareClass(hasPrepare);
        if (hasPrepare) {
            btn->setPrepareClassContent(m_prepareClassCache[prepareKey]);
        }
        
        connect(btn, &CourseButtonWidget::prepareClassClicked, this, &TACCourseSchedule::onCourseButtonClicked);
        classLayout->addWidget(btn);
    }
    
    classLayout->addStretch();
}

void TACCourseSchedule::initShow()
{
    QRect rect = this->getScreenGeometryWithTaskbar();
    if (rect.isEmpty())
        return;
    int x = rect.x()+rect.width()-this->width()-30;
    int y = rect.y()+200;
    this->move(x, y);
}

void TACCourseSchedule::cleanLayout(QLayout *layout)
{
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QLayout* childLayout = item->layout()) {
            cleanLayout(childLayout);
            delete childLayout;
        }
        if (QWidget* widget = item->widget()) {
            delete widget;
        }
        delete item;
    }
}

void TACCourseSchedule::updateTimeSlots()
{
    // 保留空实现，如果需要可以添加时间高亮逻辑
}

void TACCourseSchedule::setPrepareClassHistory(const QJsonArray& history)
{
    m_prepareClassCache.clear();
    for (const auto& value : history) {
        if (!value.isObject()) {
            continue;
        }
        QJsonObject obj = value.toObject();
        QString subject = obj.value(QStringLiteral("subject")).toString();
        QString time = obj.value(QStringLiteral("time")).toString();
        QString content = obj.value(QStringLiteral("content")).toString();
        if (subject.trimmed().isEmpty() || content.isEmpty()) {
            continue;
        }
        QString key = prepareClassCacheKey(subject, time);
        m_prepareClassCache[key] = content;
    }
    // 更新UI，重新应用日程表以显示课前准备图标
    applyDefaultDailySchedule();
}

QString TACCourseSchedule::prepareClassCacheKey(const QString& subject, const QString& time) const
{
    return subject.trimmed() + "|" + time.trimmed();
}

void TACCourseSchedule::showPrepareClassDialog(const QString& subject, const QString& time)
{
    const QString cacheKey = prepareClassCacheKey(subject, time);
    // 复用同一个窗口：关闭按钮只隐藏，下次点菜单再显示
    if (!m_prepareClassViewDlg) {
        m_prepareClassViewDlg = new PrepareClassEditDialog(this);
    }

    const QString header = time.trimmed().isEmpty()
        ? QString::fromUtf8(u8"课前准备 - ") + subject
        : QString::fromUtf8(u8"课前准备 - ") + subject + QString::fromUtf8(u8"  ") + time;
    m_prepareClassViewDlg->setHeaderText(header);
    m_prepareClassViewDlg->setInitialContent(m_prepareClassCache.value(cacheKey));

    m_prepareClassViewDlg->show();
    m_prepareClassViewDlg->raise();
    m_prepareClassViewDlg->activateWindow();
}

void TACCourseSchedule::onCourseButtonClicked(const QString& subject, const QString& time)
{
    showPrepareClassDialog(subject, time);
}

QString TACCourseSchedule::getPrepareClassContent(const QString& subject, const QString& time) const
{
    const QString cacheKey = prepareClassCacheKey(subject, time);
    return m_prepareClassCache.value(cacheKey);
}
