#pragma execution_character_set("utf-8")
#include "TACalendarDialog.h"
#include <QPainterPath>
#include <QVariant>
#include <QResizeEvent>
#include <QAbstractButton>
#include <QHeaderView>
#include <QFont>
#include <QFontMetrics>
#include <QScrollBar>
TACalendarWidget::TACalendarWidget(QWidget* parent)
    : QWidget(parent)
    , m_currentDate(QDate::currentDate())
    , m_selectedDate(QDate::currentDate())
{
    setWindowFlags(
        Qt::FramelessWindowHint |
        Qt::Tool |
        Qt::WindowStaysOnTopHint
    );
    setAttribute(Qt::WA_TranslucentBackground);
    this->setObjectName("TACalendarWidget");
    setMouseTracking(true);

    // 关闭按钮（默认隐藏，鼠标移入显示）
    m_closeButton = new QToolButton(this);
    m_closeButton->setText(QStringLiteral("×"));
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setStyleSheet(
        "QToolButton {"
        "  border: none;"
        "  color: rgba(255,255,255,0.9);"
        "  background: rgba(0,0,0,0.25);"
        "  border-radius: 12px;"
        "  font-weight: bold;"
        "}"
        "QToolButton:hover { background: rgba(255,0,0,0.35); }"
    );
    m_closeButton->hide();
    m_closeButton->raise();
    connect(m_closeButton, &QToolButton::clicked, this, [this]() {
        this->hide(); // 不销毁，便于复用指针
    });
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(20,20,20,20);

    QHBoxLayout* navLayout = new QHBoxLayout();
    navLayout->setSpacing(10);
    navLayout->addStretch();
    m_yearButton = new QPushButton(this);
    m_yearButton->setObjectName("yearButton");
    connect(m_yearButton, &QPushButton::clicked, this, [=]() {
        QPoint pos = m_yearButton->mapToGlobal(QPoint(0, m_yearButton->height()));
        m_yearMenu->exec(pos);
    });
    m_yearMenu = new QMenu(this);

    m_monthButton = new QPushButton(this);
    m_monthButton->setObjectName("monthButton");
    connect(m_monthButton, &QPushButton::clicked, this, [=]() {
        QPoint pos = m_monthButton->mapToGlobal(QPoint(0, m_monthButton->height()));
        m_monthMenu->exec(pos);
        });
    m_monthMenu = new QMenu(this);

    m_yearButton->setIcon(QIcon(":/res/img/arrow-down.png"));
    m_yearButton->setLayoutDirection(Qt::RightToLeft);
    m_monthButton->setIcon(QIcon(":/res/img/arrow-down.png"));
    m_monthButton->setLayoutDirection(Qt::RightToLeft);
    navLayout->addWidget(m_yearButton);
    navLayout->addWidget(m_monthButton);
    navLayout->addStretch();

    m_calendarLayout = new QGridLayout();
    m_calendarLayout->setSpacing(2);
    m_calendarLayout->setContentsMargins(0, 0, 0, 0);

    m_mainLayout->addLayout(navLayout);
    m_mainLayout->addLayout(m_calendarLayout);

    createWeekdayLabels();
    populateYearCombo();
    populateMonthCombo();
    updateCalendar();

    onYearChanged(m_currentDate.year());
    onMonthChanged(m_currentDate.month());
}

void TACalendarWidget::enterEvent(QEvent* event)
{
    QWidget::enterEvent(event);
    if (m_closeButton) m_closeButton->show();
}

void TACalendarWidget::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    if (m_closeButton) m_closeButton->hide();
}

void TACalendarWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_closeButton) {
        const int margin = 10;
        m_closeButton->move(width() - m_closeButton->width() - margin, margin);
    }
}

void TACalendarWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 避免与按钮点击冲突：点到按钮就交给按钮处理
        QWidget* child = childAt(event->pos());
        if (!qobject_cast<QAbstractButton*>(child)) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void TACalendarWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void TACalendarWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

// ==================== TASchoolCalendarWidget（学期校历表） ====================

static QSet<QDate> buildRangeDates(const QDate& start, const QDate& endInclusive)
{
    QSet<QDate> s;
    if (!start.isValid() || !endInclusive.isValid()) return s;
    QDate d = start;
    while (d.isValid() && d <= endInclusive) {
        s.insert(d);
        d = d.addDays(1);
    }
    return s;
}

TASchoolCalendarWidget::TASchoolCalendarWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setObjectName("TASchoolCalendarWidget");

    // 默认：深色底（与桌面管理一致时文字需白色）
    m_backgroundColor = QColor(43, 43, 43);
    m_borderColor = QColor(120, 120, 120);
    m_borderWidth = 1;
    m_radius = 10;

    // 关闭按钮（默认隐藏，鼠标移入显示）
    m_closeButton = new QToolButton(this);
    m_closeButton->setText(QStringLiteral("×"));
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setStyleSheet(
        "QToolButton { border:none; color:#ffffff; background: rgba(255,255,255,0.12); border-radius:12px; font-weight:bold; }"
        "QToolButton:hover { background: rgba(255,0,0,0.35); }"
    );
    m_closeButton->hide();
    m_closeButton->raise();
    connect(m_closeButton, &QToolButton::clicked, this, [this]() { hide(); });

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 14);
    mainLayout->setSpacing(8);

    m_schoolLabel = new QLabel(this);
    m_schoolLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    QFont f1 = m_schoolLabel->font();
    f1.setPointSize(18);
    f1.setBold(true);
    m_schoolLabel->setFont(f1);
    m_schoolLabel->setStyleSheet("color: rgba(255,255,255,0.92);");

    m_termLabel = new QLabel(this);
    m_termLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    QFont f2 = m_termLabel->font();
    f2.setPointSize(14);
    f2.setBold(true);
    m_termLabel->setFont(f2);
    m_termLabel->setStyleSheet("color: rgba(255,255,255,0.92);");

    mainLayout->addWidget(m_schoolLabel);
    mainLayout->addWidget(m_termLabel);

    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_table->setShowGrid(true);
    m_table->setWordWrap(true);
    m_table->setStyleSheet(
        "QTableWidget { background: transparent; gridline-color: rgba(255,255,255,0.25); color: rgba(255,255,255,0.92); }"
        // 表头背景用灰色（与窗口背景一致），避免白字看不清
        "QHeaderView::section { background-color: #2b2b2b; color: rgba(255,255,255,0.92); font-weight:bold; border:1px solid rgba(255,255,255,0.25); }"
    );
    mainLayout->addWidget(m_table, 1);

    m_legendLabel = new QLabel(this);
    m_legendLabel->setText(QString::fromUtf8(u8"注：蓝色表示工作日，绿色表示节假日。"));
    m_legendLabel->setStyleSheet("color: rgba(255,255,255,0.85); font-size:12px;");
    mainLayout->addWidget(m_legendLabel);

    // 先用你给的示例做一个默认模板（后续可从服务端/文件加载）
    const QString school = QString::fromUtf8(u8"费县第五中学");
    const QString term = QString::fromUtf8(u8"2025～2026学年度第一学期校历");
    const QDate startMonday(2025, 9, 1); // 周一
    const int weeks = 22;

    // 备注：按日期组织，表格会按周汇总显示在“备注”列中
    QMap<QDate, QStringList> remarksByDate;
    remarksByDate[QDate(2025, 9, 1)] << QString::fromUtf8(u8"9.1 开学");
    remarksByDate[QDate(2025, 9, 3)] << QString::fromUtf8(u8"9.3 中国人民抗日战争胜利纪念日");
    remarksByDate[QDate(2025, 9, 10)] << QString::fromUtf8(u8"9.10 教师节");
    remarksByDate[QDate(2025, 9, 18)] << QString::fromUtf8(u8"9.18 九一八事变");
    remarksByDate[QDate(2025, 10, 1)] << QString::fromUtf8(u8"10.1 国庆节（10.1-10.8 休 8 天，9.21、10.11 班）");
    remarksByDate[QDate(2025, 10, 29)] << QString::fromUtf8(u8"10.29 重阳节");
    remarksByDate[QDate(2025, 12, 13)] << QString::fromUtf8(u8"12.13 国家公祭日");
    remarksByDate[QDate(2026, 1, 1)] << QString::fromUtf8(u8"2026.1.1 元旦");
    remarksByDate[QDate(2026, 1, 26)] << QString::fromUtf8(u8"1.26 腊八节");

    QSet<QDate> holidays = buildRangeDates(QDate(2025, 10, 1), QDate(2025, 10, 8));
    // 其他节日（单日）：这里只按示例标为“节假日色”，如果你想仍按工作日色也可调整
    holidays.insert(QDate(2025, 9, 3));
    holidays.insert(QDate(2025, 10, 29));
    holidays.insert(QDate(2025, 12, 13));
    holidays.insert(QDate(2026, 1, 1));
    holidays.insert(QDate(2026, 1, 26));

    QSet<QDate> makeup;
    makeup.insert(QDate(2025, 9, 21)); // 周日补班
    makeup.insert(QDate(2025, 10, 11)); // 周六补班

    setCalendarInfo(school, term, startMonday, weeks, QString(), holidays, makeup);
    setRemarksByDate(remarksByDate);
    resize(1100, 820);
}

void TASchoolCalendarWidget::setBackgroundColor(const QColor& color) { m_backgroundColor = color; update(); }
void TASchoolCalendarWidget::setBorderColor(const QColor& color) { m_borderColor = color; update(); }
void TASchoolCalendarWidget::setBorderWidth(int val) { m_borderWidth = val; update(); }
void TASchoolCalendarWidget::setRadius(int val) { m_radius = val; update(); }

void TASchoolCalendarWidget::setCalendarInfo(const QString& schoolName,
                                             const QString& termTitle,
                                             const QDate& termStartMonday,
                                             int weekCount,
                                             const QString& remarkText,
                                             const QSet<QDate>& holidayDates,
                                             const QSet<QDate>& makeupWorkDates)
{
    m_schoolName = schoolName;
    m_termTitle = termTitle;
    m_termStartMonday = termStartMonday;
    m_weekCount = weekCount;
    m_remarkText = remarkText;
    m_holidayDates = holidayDates;
    m_makeupWorkDates = makeupWorkDates;

    if (m_schoolLabel) m_schoolLabel->setText(m_schoolName);
    if (m_termLabel) m_termLabel->setText(m_termTitle);

    rebuildTable();
}

void TASchoolCalendarWidget::setRemarksByDate(const QMap<QDate, QStringList>& remarksByDate)
{
    m_remarksByDate = remarksByDate;
    rebuildTable();
}

void TASchoolCalendarWidget::rebuildTable()
{
    if (!m_table || !m_termStartMonday.isValid() || m_weekCount <= 0) return;

    const QStringList headers = {
        QString::fromUtf8(u8"周次"),
        QString::fromUtf8(u8"一"),
        QString::fromUtf8(u8"二"),
        QString::fromUtf8(u8"三"),
        QString::fromUtf8(u8"四"),
        QString::fromUtf8(u8"五"),
        QString::fromUtf8(u8"六"),
        QString::fromUtf8(u8"日"),
        QString::fromUtf8(u8"备注")
    };

    m_table->clear();
    m_table->setColumnCount(headers.size());
    m_table->setRowCount(m_weekCount);
    m_table->setHorizontalHeaderLabels(headers);

    m_table->setColumnWidth(0, 56);  // 周次
    for (int c = 1; c <= 7; ++c) m_table->setColumnWidth(c, 62);
    m_table->setColumnWidth(8, 260); // 备注

    for (int r = 0; r < m_weekCount; ++r) {
        // 周次
        QTableWidgetItem* weekItem = new QTableWidgetItem(weekName(r + 1));
        weekItem->setTextAlignment(Qt::AlignCenter);
        weekItem->setForeground(QBrush(Qt::white));
        m_table->setItem(r, 0, weekItem);

        for (int d = 0; d < 7; ++d) {
            const QDate date = m_termStartMonday.addDays(r * 7 + d);
            QTableWidgetItem* item = new QTableWidgetItem(formatDateCell(date));
            item->setTextAlignment(Qt::AlignCenter);
            applyCellStyle(item, date);
            m_table->setItem(r, 1 + d, item);
        }

        // 备注：按日期汇总到对应周（避免全部挤在第一行）
        QStringList weekRemarks;
        for (int d = 0; d < 7; ++d) {
            const QDate date = m_termStartMonday.addDays(r * 7 + d);
            if (m_remarksByDate.contains(date)) {
                weekRemarks.append(m_remarksByDate.value(date));
            }
        }
        // 兼容：如果没有按日期备注，但传了整体 remarkText，则仍放第一周
        const QString remarkTextForWeek = !weekRemarks.isEmpty()
            ? weekRemarks.join("\n")
            : ((r == 0) ? m_remarkText : QString());

        QTableWidgetItem* remarkItem = new QTableWidgetItem(remarkTextForWeek);
        remarkItem->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
        remarkItem->setForeground(QBrush(QColor(255, 255, 255, 220)));
        remarkItem->setFlags(remarkItem->flags() & ~Qt::ItemIsSelectable);
        m_table->setItem(r, 8, remarkItem);

        // 行高：有备注则自适应更高
        int lines = remarkTextForWeek.isEmpty() ? 1 : (remarkTextForWeek.count('\n') + 1);
        int rowH = qMax(28, 18 * lines + 10);
        m_table->setRowHeight(r, rowH);
    }
}

QString TASchoolCalendarWidget::weekName(int idx) const
{
    static const QStringList names = {
        QString::fromUtf8(u8"一"), QString::fromUtf8(u8"二"), QString::fromUtf8(u8"三"), QString::fromUtf8(u8"四"),
        QString::fromUtf8(u8"五"), QString::fromUtf8(u8"六"), QString::fromUtf8(u8"七"), QString::fromUtf8(u8"八"),
        QString::fromUtf8(u8"九"), QString::fromUtf8(u8"十"), QString::fromUtf8(u8"十一"), QString::fromUtf8(u8"十二"),
        QString::fromUtf8(u8"十三"), QString::fromUtf8(u8"十四"), QString::fromUtf8(u8"十五"), QString::fromUtf8(u8"十六"),
        QString::fromUtf8(u8"十七"), QString::fromUtf8(u8"十八"), QString::fromUtf8(u8"十九"), QString::fromUtf8(u8"二十"),
        QString::fromUtf8(u8"二十一"), QString::fromUtf8(u8"二十二")
    };
    if (idx >= 1 && idx <= names.size()) return names[idx - 1];
    return QString::number(idx);
}

QString TASchoolCalendarWidget::formatDateCell(const QDate& date) const
{
    if (!date.isValid()) return QString();
    // 起始日显示 YYYY/M/D；每月1号显示 M/D；跨年1月1号显示 YYYY/M/D
    if (date == m_termStartMonday) {
        return QString("%1/%2/%3").arg(date.year()).arg(date.month()).arg(date.day());
    }
    if (date.day() == 1) {
        // 跨年时显示年份
        if (date.year() != m_termStartMonday.year()) {
            return QString("%1/%2/%3").arg(date.year()).arg(date.month()).arg(date.day());
        }
        return QString("%1/%2").arg(date.month()).arg(date.day());
    }
    return QString::number(date.day());
}

void TASchoolCalendarWidget::applyCellStyle(QTableWidgetItem* item, const QDate& date) const
{
    if (!item || !date.isValid()) return;

    // 基础色：深色底，配白字（周一~周五工作日蓝；周末/节假日绿）
    QColor workdayColor(38, 72, 116);          // 深蓝
    QColor weekendOrHolidayColor(34, 104, 72); // 深绿

    const int dow = date.dayOfWeek(); // 1=Mon..7=Sun
    bool isWeekend = (dow == 6 || dow == 7);

    bool isHoliday = m_holidayDates.contains(date);
    bool isMakeupWork = m_makeupWorkDates.contains(date);

    QColor bg = isWeekend ? weekendOrHolidayColor : workdayColor;
    if (isHoliday) bg = weekendOrHolidayColor;
    if (isMakeupWork) bg = workdayColor;

    item->setBackground(QBrush(bg));
    item->setForeground(QBrush(Qt::white));
}

void TASchoolCalendarWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect r(0, 0, width(), height());
    QPainterPath path;
    path.addRoundedRect(r.adjusted(0, 0, -1, -1), m_radius, m_radius);
    painter.fillPath(path, QBrush(m_backgroundColor));

    QPen pen(m_borderColor);
    pen.setWidth(m_borderWidth);
    painter.strokePath(path, pen);
}

void TASchoolCalendarWidget::enterEvent(QEvent* event)
{
    QWidget::enterEvent(event);
    if (m_closeButton) m_closeButton->show();
}

void TASchoolCalendarWidget::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    if (m_closeButton) m_closeButton->hide();
}

void TASchoolCalendarWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_closeButton) {
        const int margin = 10;
        m_closeButton->move(width() - m_closeButton->width() - margin, margin);
    }
}

void TASchoolCalendarWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget* child = childAt(event->pos());
        if (!qobject_cast<QAbstractButton*>(child) && !qobject_cast<QAbstractItemView*>(child)) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void TASchoolCalendarWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void TASchoolCalendarWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}
void TACalendarWidget::populateYearCombo()
{
    m_yearMenu->clear();

    int currentYear = m_currentDate.year();
    int startYear = currentYear;
    int endYear = currentYear + 6;

   
    for (int year = startYear; year <= endYear; ++year) {
        QAction* action = m_yearMenu->addAction(QString::number(year));
        action->setData(year);
    }

    connect(m_yearMenu, &QMenu::triggered, this, [this](QAction* action) {
        bool ok;
        int selectedYear = action->data().toInt(&ok);
        if (ok) {
            onYearChanged(selectedYear);
        }
        });
}

void TACalendarWidget::populateMonthCombo()
{
    m_monthMenu->clear();
    for (int month = 1; month <= 12; ++month) {
        QString monthName = QDate(2000, month, 1).toString("MMMM");
        QAction* action = m_monthMenu->addAction(monthName);
        action->setData(month);
    }
    connect(m_monthMenu, &QMenu::triggered, this, [this](QAction* action) {
        bool ok;
        int selectedMonth = action->data().toInt(&ok);
        if (ok) {
            onMonthChanged(selectedMonth);
        }
        });
   
}
void TACalendarWidget::onYearChanged(int year)
{
    if (year > 0) {
        m_currentDate = QDate(year, m_currentDate.month(), 1);
        updateYearButton();
        updateCalendar();
    }
}
void TACalendarWidget::onMonthChanged(int month)
{
    if (month > 0) {
        m_currentDate = QDate(m_currentDate.year(), month, 1);
        updateMonthButton();
        updateCalendar();
    }
}
void TACalendarWidget::updateYearButton()
{
    m_yearButton->setText(QString::number(m_currentDate.year())+"年");
}
void TACalendarWidget::updateMonthButton()
{
    m_monthButton->setText(QString::number(m_currentDate.month())+"月");
}
void TACalendarWidget::createWeekdayLabels()
{
    QStringList weekdayNames = {
         tr("一"),
         tr("二"),
         tr("三"),
         tr("四"),
         tr("五"),
         tr("六"),
         tr("日") 
    };
    for (int i = 0; i < 7; ++i) {
        QLabel* label = new QLabel(weekdayNames[i], this);
        label->setAlignment(Qt::AlignCenter);
        label->setProperty("isWeekdayLabel", true);
        m_calendarLayout->addWidget(label, 0, i);
    }
}

void TACalendarWidget::updateCalendar()
{
    clearDateButtons();
    QDate firstDayOfMonth(m_currentDate.year(), m_currentDate.month(), 1);
    int dayOfWeek = firstDayOfMonth.dayOfWeek();
    int startCol = dayOfWeek - 1;

    int daysInMonth = m_currentDate.daysInMonth();
    int totalCells = daysInMonth + startCol;
    int numRows = (totalCells + 6) / 7;
    int currentRow = 1;
    int currentCol = 0;
    QDate prevMonth = m_currentDate.addMonths(-1);
    int daysInPrevMonth = prevMonth.daysInMonth();
    int prevMonthStartDay = daysInPrevMonth - startCol + 1;

    for (int d = prevMonthStartDay; d <= daysInPrevMonth; ++d) {
        QPushButton* btn = new QPushButton(QString::number(d), this);
        btn->setEnabled(false);
        m_calendarLayout->addWidget(btn, currentRow, currentCol);
        m_dateButtons.append(btn);

        currentCol++;
        if (currentCol >= 7) {
            currentCol = 0;
            currentRow++;
        }
    }
    for (int d = 1; d <= daysInMonth; ++d) {
        QPushButton* btn = new QPushButton(QString::number(d), this);
        QDate dateObj(m_currentDate.year(), m_currentDate.month(), d);

        // 设置按钮的样式
        QString style = "";
        if (dateObj == QDate::currentDate()) {
            style += "background-color: lightblue; font-weight: bold;";
        }
        if (dateObj == m_selectedDate) {
            style += " border: 2px solid red;";
        }
        btn->setStyleSheet(style);
        connect(btn, &QPushButton::clicked, this, [this, dateObj]() {
            //selectDate(dateObj);
            });

        m_calendarLayout->addWidget(btn, currentRow, currentCol);
        m_dateButtons.append(btn);

        currentCol++;
        if (currentCol >= 7) {
            currentCol = 0;
            currentRow++;
        }
    }

    int totalGridCells = numRows * 7;
    int remainingCells = totalGridCells - (startCol + daysInMonth);

    for (int d = 1; d <= remainingCells; ++d) {
        QPushButton* btn = new QPushButton(QString::number(d), this);
        btn->setStyleSheet("color: lightgray;");
        btn->setEnabled(false);
        m_calendarLayout->addWidget(btn, currentRow, currentCol);
        m_dateButtons.append(btn);

        currentCol++;
        if (currentCol >= 7) {
            currentCol = 0;
            currentRow++;
        }
    }

    for (int r = 1; r < numRows; ++r) {
        m_calendarLayout->setRowStretch(r, 1);
    }
}

void TACalendarWidget::clearDateButtons()
{
    qDeleteAll(m_dateButtons);
    m_dateButtons.clear();
}


void TACalendarWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);


    QRect rect(0, 0, width(), height());
    int cornerRadius = m_radius;
    QPainterPath path;
    path.addRoundedRect(rect, cornerRadius, cornerRadius);

    painter.fillPath(path, QBrush(m_backgroundColor));

    QPen pen;
    pen.setWidth(m_borderWidth);
    pen.setColor(m_borderColor);
    painter.strokePath(path, pen);
}

QDate TACalendarWidget::selectedDate() const
{
    return m_selectedDate;
}

void TACalendarWidget::setSelectedDate(const QDate& date)
{
    if (date.isValid() && date != m_selectedDate) {
        m_selectedDate = date;
        m_currentDate = date;
        updateCalendar();
        emit selectionChanged();
    }
}
void TACalendarWidget::setRadius(int val)
{
    if (m_radius != val)
    {
        m_radius = val;
        update();
    }
}
void TACalendarWidget::setBorderWidth(int val)
{
    if (m_borderWidth != val)
    {
        m_borderWidth = val;
        update();
    }
}
void TACalendarWidget::setBorderColor(const QColor& color)
{
    if (m_borderColor != color)
    {
        m_borderColor = color;
        update();
    }
}
void TACalendarWidget::setBackgroundColor(const QColor& color)
{
    if (m_backgroundColor != color)
    {
        m_backgroundColor = color;
        update();
    }
}