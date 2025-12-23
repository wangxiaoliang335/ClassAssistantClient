#include "mainwindow.h"
#include "termsettingsdialog.h"
#include "courseeditdialog.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QDate>
#include <QFrame>
#include <QFont>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QFontMetricsF>
#include <QModelIndex>
#include <QPushButton>
#include <QSettings>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLabel>
#include <QMargins>
#include <QVBoxLayout>
#include <QEvent>
#include <QKeyEvent>
#include <QGridLayout>
#include <QMouseEvent>
#include <QDateEdit>
#include <QPainter>
#include <QPixmap>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QTimeEdit>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QColorDialog>
#include <QListWidget>
#include <QSignalBlocker>
#include <QFile>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QStatusBar>
#include <algorithm>
#include <QApplication>
#include <QScrollArea>
#include <QDrag>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QVariantAnimation>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QScrollBar>
#include <QDockWidget>
#include <QTabWidget>
#include <QToolButton>
#include <QStyle>
#include <QTimer>
#include <QCursor>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QMap>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include "xlsxdocument.h"
#include "xlsxworksheet.h"
#include "xlsxformat.h"

QXLSX_USE_NAMESPACE

namespace
{
constexpr int kLargeScheduleMinColumnWidth = 48;
constexpr int kLargeScheduleMinRowHeight = 44;
constexpr int kTeacherRosterMinColumnWidth = 48;
constexpr int kTeacherRosterMinRowHeight = 40;
}

static QString normalizedClassIdentifier(const QString &text)
{
    QString digits;
    const QString trimmed = text.trimmed();
    for (const QChar &ch : trimmed)
    {
        if (ch.isDigit())
            digits.append(ch);
    }
    if (!digits.isEmpty())
        return digits;
    QString result = trimmed;
    if (result.endsWith(QStringLiteral("班")))
    {
        result.chop(1);
        result = result.trimmed();
    }
    return result;
}

class WeeklyStatsCard : public QWidget
{
public:
    WeeklyStatsCard(const QString &courseName, const QColor &color, int standardCount, int actualCount, QWidget *parent = nullptr)
        : QWidget(parent),
          course(courseName),
          courseColor(color),
          standard(standardCount),
          actual(actualCount)
    {
        setCursor(Qt::OpenHandCursor);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setFixedHeight(70);
        setFixedWidth(112);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(10, 8, 10, 8);
        layout->setSpacing(3);

        QColor fillColor = courseColor.isValid() ? courseColor : QColor("#dbeafe");
        QString textColor = (fillColor.lightness() < 110) ? "#ffffff" : "#0f172a";
        QString style = QString("QWidget{background:%1; border-radius:8px; border:1px solid rgba(15,23,42,0.15);}").arg(fillColor.name(QColor::HexRgb));
        setStyleSheet(style);

        QLabel *nameLabel = new QLabel(course, this);
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setStyleSheet(QString("font-weight:600; color:%1;").arg(textColor));
        layout->addWidget(nameLabel);

        QHBoxLayout *statsRow = new QHBoxLayout();
        QLabel *standardLabel = new QLabel(QString::fromUtf8("原 %1").arg(standard), this);
        standardLabel->setStyleSheet("color:#475569; font-size:11px;");
        statsRow->addWidget(standardLabel);
        statsRow->addStretch();

        QLabel *actualLabel = new QLabel(QString::fromUtf8("实际 %1").arg(actual), this);
        QColor ratioColor;
        if (standard <= 0 && actual > 0)
            ratioColor = QColor("#f97316");
        else if (actual > standard)
            ratioColor = QColor("#f97316");
        else if (actual == standard)
            ratioColor = QColor("#16a34a");
        else
            ratioColor = QColor("#2563eb");
        actualLabel->setStyleSheet(QString("color:white; font-weight:600; padding:1px 6px; border-radius:10px; background:%1; font-size:11px;")
                                       .arg(ratioColor.name(QColor::HexRgb)));
        statsRow->addWidget(actualLabel);
        layout->addLayout(statsRow);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
            dragStartPos = event->pos();
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!(event->buttons() & Qt::LeftButton))
            return;
        if ((event->pos() - dragStartPos).manhattanLength() < QApplication::startDragDistance())
            return;
        startDrag();
    }

private:
    void startDrag()
    {
        auto *drag = new QDrag(this);
        auto *mime = new QMimeData();
        QByteArray payload;
        QDataStream stream(&payload, QIODevice::WriteOnly);
        stream << course << courseColor;
        mime->setData("application/x-weekly-course", payload);
        drag->setMimeData(mime);

        QPixmap pixmap(size());
        render(&pixmap);
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
        drag->exec(Qt::CopyAction);
    }

    QString course;
    QColor courseColor;
    int standard;
    int actual;
    QPoint dragStartPos;
};

class ReminderCardWidget : public QWidget
{
public:
    ReminderCardWidget(const QString &title, const QString &timeText, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFixedSize(120, 42);
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 6, 8, 6);
        layout->setSpacing(2);
        QHBoxLayout *header = new QHBoxLayout();
        header->setContentsMargins(0, 0, 0, 0);
        header->setSpacing(4);
        QLabel *iconLabel = new QLabel(QStringLiteral("🔔"), this);
        iconLabel->setStyleSheet("font-size:12px;");
        QLabel *timeLabel = new QLabel(timeText, this);
        timeLabel->setStyleSheet("color:#1d4ed8; font-size:11px; font-weight:600;");
        header->addWidget(iconLabel);
        header->addWidget(timeLabel);
        header->addStretch();
        QLabel *titleLabel = new QLabel(title, this);
        titleLabel->setWordWrap(true);
        titleLabel->setStyleSheet("color:#0f172a; font-weight:600; font-size:12px;");
        layout->addLayout(header);
        layout->addWidget(titleLabel);
        setStyleSheet("QWidget{background:rgba(254,243,199,0.95); border:1px solid #fcd34d; border-radius:8px;}");
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      mainToolbarWidget(nullptr),
      previousWeekButton(nullptr),
      nextWeekButton(nullptr),
      currentWeekButton(nullptr),
      temporaryAdjustButton(nullptr),
      addReminderButton(nullptr),
      configButton(nullptr),
      displayModeButton(nullptr),
      minimalToggleButton(nullptr),
      statisticsButton(nullptr),
      addMergedRowButton(nullptr),
      addLessonRowButton(nullptr),
      deleteRowButton(nullptr),
      saveScheduleButton(nullptr),
      discardScheduleButton(nullptr),
      resetTemporaryAdjustButton(nullptr),
      weekLabel(nullptr),
      teacherRosterButton(nullptr),
      quickCourseWidget(nullptr),
      quickCourseLayout(nullptr),
      activePaletteButton(nullptr),
      activePaletteColor(QColor()),
      activePaletteCourse(),
      clearPaletteButton(nullptr),
      addBaseScheduleButton(nullptr),
      baseStartDateEdit(nullptr),
      baseEndDateEdit(nullptr),
      baseScheduleList(nullptr),
      centralContainer(nullptr),
      mainLayoutRoot(nullptr),
      secondaryContainer(nullptr),
      secondaryStack(nullptr),
      configPanel(nullptr),
      temporaryAdjustPanel(nullptr),
      reminderPanel(nullptr),
      displayModePanel(nullptr),
      reminderListWidget(nullptr),
      reminderTitleEdit(nullptr),
      reminderTypeCombo(nullptr),
      reminderOnceDateTimeEdit(nullptr),
      reminderStartDateEdit(nullptr),
      reminderEndDateEdit(nullptr),
      reminderTimeEdit(nullptr),
      reminderWeekdayCombo(nullptr),
      addReminderItemButton(nullptr),
      removeReminderItemButton(nullptr),
      reminderOnceFields(nullptr),
      reminderRepeatFields(nullptr),
      reminderWeeklyExtraFields(nullptr),
      reminderOverlay(nullptr),
      largeScheduleDialog(nullptr),
      statisticsDialog(nullptr),
      largeScheduleTable(nullptr),
      toggleLargeScheduleSelectorsButton(nullptr),
      configureLargeScheduleColorsButton(nullptr),
      conflictDetectionButton(nullptr),
      conflictDetectionEnabled(false),
      associationDataLoaded(false),
      associateRosterButton(nullptr),
      teacherRosterDock(nullptr),
      teacherRosterTable(nullptr),
      largeScheduleSelectedColumns(),
      largeScheduleSelectedRows(),
      largeScheduleSelectorDragging(false),
      largeScheduleRowDragging(false),
      largeScheduleSelectorAnchorColumn(-1),
      largeScheduleSelectorLastColumn(-1),
      largeScheduleRowAnchor(-1),
      largeScheduleRowLast(-1),
      largeScheduleSelectorsVisible(true),
      weeklyStatsLayout(nullptr),
      weeklyStatsPlaceholder(nullptr),
      weeklyStatsViewport(nullptr),
      customTemporaryCourseLabel(nullptr),
      showWeekendToggle(nullptr),
      showChromeToggle(nullptr),
      chromeAutoHideTimer(nullptr),
      toolbarHideTimer(nullptr),
      showWeekendInDisplay(true),
      showChromeInDisplay(true),
      chromeHiddenInDisplay(false),
      manualMinimalMode(false),
      savedFrameGeometryValid(false),
      displayDragging(false),
      resizeInProgress(false),
      resizeArea(ResizeArea::None),
      displayDragOffset(QPoint()),
      normalWindowFlags(Qt::WindowFlags()),
      statisticsChartViewActual(nullptr),
      statisticsChartViewBase(nullptr),
      statisticsListLayoutActual(nullptr),
      statisticsListLayoutBase(nullptr),
      statisticsEvaluationLabelActual(nullptr),
      statisticsEvaluationLabelBase(nullptr),
      statisticsLessonSumLabelActual(nullptr),
      statisticsLessonSumLabelBase(nullptr),
      activePanel(SecondaryPanel::None),
      scheduleTable(nullptr),
      termStartDate(),
      termEndDate(),
      hasTermRange(false),
      selectedTimeRow(-1),
      baseScheduleDirty(false),
      suppressDateEditSignal(false),
      forcedBaseScheduleIndex(-1),
      configSelectedBaseScheduleIndex(-1),
      editingNewBaseSchedule(false),
      unsavedBaseScheduleIndex(-1)
{
    lessonTimes << "08:00-08:45"
                << "09:00-09:45"
                << "10:00-10:45"
                << "11:00-11:45"
                << "13:30-14:15"
                << "14:30-15:15"
                << "15:30-16:15"
                << "16:30-17:15";

    customTemporaryCourseName = QString::fromUtf8("临时课程");
    customTemporaryCourseColor = QColor("#f97316");

    currentWeekStart = mondayOf(QDate::currentDate());
    normalWindowFlags = windowFlags();

    loadTermSettings();
    initializeScheduleData();
    setupUi();
    refreshTimeColumn();
    updateWeekLabel();
    refreshDateHeader();
    refreshScheduleCells();
    showSecondaryPanel(SecondaryPanel::None);
    chromeAutoHideTimer = new QTimer(this);
    chromeAutoHideTimer->setInterval(3000);
    chromeAutoHideTimer->setSingleShot(true);
    toolbarHideTimer = new QTimer(this);
    toolbarHideTimer->setInterval(350);
    toolbarHideTimer->setSingleShot(true);
    initializeQuickCoursePalette();
    applyDisplayPreferences();
}

MainWindow::~MainWindow()
{
    if (largeScheduleDialog)
    {
        largeScheduleDialog->close();
        delete largeScheduleDialog;
        largeScheduleDialog = nullptr;
        largeScheduleTable = nullptr;
        teacherRosterDock = nullptr;
        teacherRosterTable = nullptr;
    }
}

void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    centralContainer = central;
    centralContainer->setObjectName(QStringLiteral("centralContainer"));
    mainLayoutRoot = new QVBoxLayout(central);
    mainLayoutRoot->setContentsMargins(12, 12, 12, 12);
    mainLayoutRoot->setSpacing(8);
    central->installEventFilter(this);

    mainToolbarWidget = buildMainToolbar();
    mainLayoutRoot->addWidget(mainToolbarWidget);
    if (mainToolbarWidget)
        mainToolbarWidget->setVisible(false);

    secondaryContainer = new QFrame(this);
    secondaryContainer->setFrameShape(QFrame::StyledPanel);
    secondaryContainer->setVisible(false);
    QVBoxLayout *secondaryLayout = new QVBoxLayout(secondaryContainer);
    secondaryLayout->setContentsMargins(12, 8, 12, 8);
    secondaryLayout->setSpacing(6);

    secondaryStack = new QStackedWidget(secondaryContainer);
    configPanel = buildConfigPanel();
    temporaryAdjustPanel = buildTemporaryPanel();
    updateWeeklyStatistics();
    reminderPanel = buildReminderPanel();
    displayModePanel = buildDisplayModePanel();

    secondaryStack->addWidget(configPanel);
    secondaryStack->addWidget(temporaryAdjustPanel);
    secondaryStack->addWidget(reminderPanel);
    secondaryStack->addWidget(displayModePanel);

    secondaryLayout->addWidget(secondaryStack);
    mainLayoutRoot->addWidget(secondaryContainer);

    setupScheduleTable();
    mainLayoutRoot->addWidget(scheduleTable, 1);

    setCentralWidget(central);
    if (statusBar())
        statusBar()->hide();
    connectSignals();
    updateToolbarInteractivity();
}

QWidget *MainWindow::buildMainToolbar()
{
    QWidget *toolbar = new QWidget(this);
    toolbar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QHBoxLayout *layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto createCircleIcon = [](const QColor &bg, const QString &symbol) -> QIcon {
        QPixmap pix(30, 30);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(bg);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(0, 0, 30, 30);
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(12);
        painter.setFont(font);
        painter.drawText(pix.rect(), Qt::AlignCenter, symbol);
        return QIcon(pix);
    };
    const QSize iconSize(22, 22);

    previousWeekButton = new QPushButton(QString(), toolbar);
    previousWeekButton->setIcon(createCircleIcon(QColor("#2563eb"), QStringLiteral("<")));
    previousWeekButton->setIconSize(iconSize);
    previousWeekButton->setToolTip(QString::fromUtf8("上一周"));
    weekLabel = new QPushButton(QString::fromUtf8("\xe7\xac\xac\x31\xe5\x91\xa8"), toolbar);
    weekLabel->setCursor(Qt::PointingHandCursor);
    weekLabel->setCheckable(false);
    weekLabel->setMinimumWidth(0);
    weekLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    weekLabel->setStyleSheet("QPushButton{font-weight:600; border:none; padding:2px 6px;} QPushButton:hover{color:#2563eb;}");

    nextWeekButton = new QPushButton(QString(), toolbar);
    nextWeekButton->setIcon(createCircleIcon(QColor("#2563eb"), QStringLiteral(">")));
    nextWeekButton->setIconSize(iconSize);
    nextWeekButton->setToolTip(QString::fromUtf8("下一周"));

    currentWeekButton = new QPushButton(QString(), toolbar);
    currentWeekButton->setIcon(createCircleIcon(QColor("#0ea5e9"), QStringLiteral("•")));
    currentWeekButton->setIconSize(iconSize);
    currentWeekButton->setToolTip(QString::fromUtf8("定位当前周"));

    temporaryAdjustButton = new QPushButton(QString::fromUtf8("\xe4\xb8\xb4\xe6\x97\xb6\xe8\xb0\x83\xe8\xaf\xbe"), toolbar);
    temporaryAdjustButton->setToolTip(QString::fromUtf8("临时调课"));

    addReminderButton = new QPushButton(QString::fromUtf8("\xe6\xb7\xbb\xe5\x8a\xa0\xe6\x8f\x90\xe9\x86\x92"), toolbar);
    addReminderButton->setToolTip(QString::fromUtf8("添加提醒"));

    configButton = new QPushButton(QString::fromUtf8("\xe9\x85\x8d\xe7\xbd\xae"), toolbar);
    configButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    configButton->setStyleSheet("QPushButton{padding:4px 8px;}");
    configButton->setToolTip(QString::fromUtf8("配置基础课表"));

    displayModeButton = new QPushButton(QString(), toolbar);
    displayModeButton->setIcon(createCircleIcon(QColor("#22c55e"), QStringLiteral("视")));
    displayModeButton->setIconSize(iconSize);
    displayModeButton->setToolTip(QString::fromUtf8("显示方式"));

    minimalToggleButton = new QPushButton(QString::fromUtf8("\xe6\x9e\x81\xe7\xae\x80"), toolbar);
    minimalToggleButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    minimalToggleButton->setStyleSheet("QPushButton{padding:4px 8px;}");
    minimalToggleButton->setCheckable(true);
    minimalToggleButton->setToolTip(QString::fromUtf8("极简视图：仅显示课程表"));

    statisticsButton = new QPushButton(QString::fromUtf8("\xe7\xbb\x9f\xe8\xae\xa1"), toolbar);
    statisticsButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    statisticsButton->setStyleSheet("QPushButton{padding:4px 8px;}");
    statisticsButton->setToolTip(QString::fromUtf8("查看统计"));

    layout->addWidget(previousWeekButton);
    layout->addWidget(weekLabel);
    layout->addWidget(nextWeekButton);
    layout->addWidget(currentWeekButton);
    layout->addStretch();
    layout->addWidget(temporaryAdjustButton);
    layout->addWidget(addReminderButton);
    layout->addWidget(configButton);
    layout->addWidget(displayModeButton);
    layout->addWidget(statisticsButton);
    layout->addWidget(minimalToggleButton);

    return toolbar;
}

QWidget *MainWindow::buildTemporaryPanel()
{
    QWidget *panel = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QFrame *statsBlock = new QFrame(panel);
    statsBlock->setFrameShape(QFrame::StyledPanel);
    statsBlock->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QVBoxLayout *statsLayout = new QVBoxLayout(statsBlock);
    statsLayout->setContentsMargins(8, 8, 8, 8);
    statsLayout->setSpacing(6);

    QHBoxLayout *statsHeaderLayout = new QHBoxLayout();
    statsHeaderLayout->setContentsMargins(0, 0, 0, 0);
    statsHeaderLayout->setSpacing(8);
    QLabel *statsHeader = new QLabel(QString::fromUtf8("一周统计"), statsBlock);
    statsHeader->setStyleSheet("font-weight:600;");
    statsHeaderLayout->addWidget(statsHeader);

    resetTemporaryAdjustButton = new QPushButton(QString::fromUtf8("重置"), statsBlock);
    resetTemporaryAdjustButton->setFixedWidth(72);
    resetTemporaryAdjustButton->setCursor(Qt::PointingHandCursor);
    resetTemporaryAdjustButton->setStyleSheet("QPushButton{background:#fef3c7;border:1px solid #fcd34d;border-radius:8px;padding:4px 8px;font-weight:600;color:#92400e;} QPushButton:hover{background:#fde68a;}");
    statsHeaderLayout->addWidget(resetTemporaryAdjustButton);
    statsHeaderLayout->addStretch();
    QPushButton *largeScheduleButton = new QPushButton(QString::fromUtf8("参考课表"), statsBlock);
    largeScheduleButton->setCursor(Qt::PointingHandCursor);
    largeScheduleButton->setStyleSheet("QPushButton{padding:4px 12px; border-radius:6px; background:#2563eb; color:white;} QPushButton:hover{background:#1d4ed8;}");
    connect(largeScheduleButton, &QPushButton::clicked, this, &MainWindow::showLargeScheduleDialog);
    statsHeaderLayout->addWidget(largeScheduleButton);

    statsLayout->addLayout(statsHeaderLayout);

    QScrollArea *scrollArea = new QScrollArea(statsBlock);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    weeklyStatsContainer = new QWidget(scrollArea);
    weeklyStatsContainer->setAcceptDrops(true);
    weeklyStatsContainer->installEventFilter(this);
    QVBoxLayout *containerLayout = new QVBoxLayout(weeklyStatsContainer);
    containerLayout->setContentsMargins(6, 6, 6, 6);
    containerLayout->setSpacing(10);

    weeklyStatsPlaceholder = new QLabel(QString::fromUtf8("本周暂无课程统计"), weeklyStatsContainer);
    weeklyStatsPlaceholder->setAlignment(Qt::AlignCenter);
    weeklyStatsPlaceholder->setStyleSheet("color:#94a3b8; font-size:13px; padding:16px 0;");
    containerLayout->addWidget(weeklyStatsPlaceholder);

    weeklyStatsLayout = new QHBoxLayout();
    weeklyStatsLayout->setSpacing(6);
    weeklyStatsLayout->setContentsMargins(0, 0, 0, 0);
    weeklyStatsLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    containerLayout->addLayout(weeklyStatsLayout);
    containerLayout->addStretch();

    scrollArea->setWidget(weeklyStatsContainer);
    weeklyStatsViewport = scrollArea->viewport();
    if (weeklyStatsViewport)
    {
        weeklyStatsViewport->setAcceptDrops(true);
        weeklyStatsViewport->installEventFilter(this);
    }
    statsLayout->addWidget(scrollArea);

    layout->addWidget(statsBlock, 4);

    QFrame *customBlock = new QFrame(panel);
    customBlock->setFrameShape(QFrame::StyledPanel);
    customBlock->setFixedWidth(130);
    QVBoxLayout *customLayout = new QVBoxLayout(customBlock);
    customLayout->setContentsMargins(8, 8, 8, 8);
    customLayout->setSpacing(6);

    QLabel *customTitle = new QLabel(QString::fromUtf8("自定义临时课"), customBlock);
    customTitle->setStyleSheet("font-weight:600;");
    customLayout->addWidget(customTitle);

    customTemporaryCourseLabel = new QLabel(customBlock);
    customTemporaryCourseLabel->setAlignment(Qt::AlignCenter);
    customTemporaryCourseLabel->setFixedHeight(70);
    customTemporaryCourseLabel->setMinimumWidth(80);
    customTemporaryCourseLabel->setCursor(Qt::OpenHandCursor);
    customTemporaryCourseLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    customTemporaryCourseLabel->setWordWrap(true);
    customTemporaryCourseLabel->installEventFilter(this);
    customLayout->addWidget(customTemporaryCourseLabel);

    QLabel *customHint = new QLabel(QString::fromUtf8("双击编辑颜色/名称，拖拽到课表即可添加临时课程"), customBlock);
    customHint->setWordWrap(true);
    customHint->setStyleSheet("color:#94a3b8; font-size:12px;");
    customLayout->addWidget(customHint);
    customLayout->addStretch();

    layout->addWidget(customBlock, 1);

    updateCustomTemporaryCourseLabel();
    return panel;
}

QWidget *MainWindow::buildReminderPanel()
{
    QWidget *panel = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QFrame *formBlock = new QFrame(panel);
    formBlock->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout *formLayout = new QVBoxLayout(formBlock);
    formLayout->setContentsMargins(12, 12, 12, 12);
    formLayout->setSpacing(8);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("添加提醒"), formBlock);
    titleLabel->setStyleSheet("font-weight:600;");
    formLayout->addWidget(titleLabel);

    reminderTitleEdit = new QLineEdit(formBlock);
    reminderTitleEdit->setPlaceholderText(QString::fromUtf8("提醒内容"));
    formLayout->addWidget(reminderTitleEdit);

    reminderTypeCombo = new QComboBox(formBlock);
    reminderTypeCombo->addItem(QString::fromUtf8("一次提醒"), static_cast<int>(ReminderEntry::Type::Once));
    reminderTypeCombo->addItem(QString::fromUtf8("每天提醒"), static_cast<int>(ReminderEntry::Type::Daily));
    reminderTypeCombo->addItem(QString::fromUtf8("每周提醒"), static_cast<int>(ReminderEntry::Type::Weekly));
    formLayout->addWidget(reminderTypeCombo);

    reminderOnceFields = new QWidget(formBlock);
    QVBoxLayout *onceLayout = new QVBoxLayout(reminderOnceFields);
    onceLayout->setContentsMargins(0, 0, 0, 0);
    reminderOnceDateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), reminderOnceFields);
    reminderOnceDateTimeEdit->setCalendarPopup(true);
    reminderOnceDateTimeEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm"));
    onceLayout->addWidget(new QLabel(QString::fromUtf8("提醒时间"), reminderOnceFields));
    onceLayout->addWidget(reminderOnceDateTimeEdit);
    formLayout->addWidget(reminderOnceFields);

    reminderRepeatFields = new QWidget(formBlock);
    QVBoxLayout *repeatLayout = new QVBoxLayout(reminderRepeatFields);
    repeatLayout->setContentsMargins(0, 0, 0, 0);
    reminderStartDateEdit = new QDateEdit(QDate::currentDate(), reminderRepeatFields);
    reminderStartDateEdit->setCalendarPopup(true);
    reminderStartDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    reminderEndDateEdit = new QDateEdit(QDate::currentDate().addDays(7), reminderRepeatFields);
    reminderEndDateEdit->setCalendarPopup(true);
    reminderEndDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    reminderTimeEdit = new QTimeEdit(QTime::currentTime(), reminderRepeatFields);
    reminderTimeEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    repeatLayout->addWidget(new QLabel(QString::fromUtf8("开始日期"), reminderRepeatFields));
    repeatLayout->addWidget(reminderStartDateEdit);
    repeatLayout->addWidget(new QLabel(QString::fromUtf8("结束日期"), reminderRepeatFields));
    repeatLayout->addWidget(reminderEndDateEdit);
    repeatLayout->addWidget(new QLabel(QString::fromUtf8("提醒时间"), reminderRepeatFields));
    repeatLayout->addWidget(reminderTimeEdit);
    formLayout->addWidget(reminderRepeatFields);

    reminderWeeklyExtraFields = new QWidget(formBlock);
    QVBoxLayout *weeklyLayout = new QVBoxLayout(reminderWeeklyExtraFields);
    weeklyLayout->setContentsMargins(0, 0, 0, 0);
    reminderWeekdayCombo = new QComboBox(reminderWeeklyExtraFields);
    reminderWeekdayCombo->addItems({QString::fromUtf8("周一"), QString::fromUtf8("周二"), QString::fromUtf8("周三"),
                                    QString::fromUtf8("周四"), QString::fromUtf8("周五"), QString::fromUtf8("周六"), QString::fromUtf8("周日")});
    weeklyLayout->addWidget(new QLabel(QString::fromUtf8("每周星期"), reminderWeeklyExtraFields));
    weeklyLayout->addWidget(reminderWeekdayCombo);
    formLayout->addWidget(reminderWeeklyExtraFields);

    QHBoxLayout *reminderButtons = new QHBoxLayout();
    reminderButtons->setContentsMargins(0, 0, 0, 0);
    reminderButtons->setSpacing(8);
    addReminderItemButton = new QPushButton(QString::fromUtf8("添加提醒"), formBlock);
    saveReminderItemButton = new QPushButton(QString::fromUtf8("保存修改"), formBlock);
    removeReminderItemButton = new QPushButton(QString::fromUtf8("删除提醒"), formBlock);
    saveReminderItemButton->setEnabled(false);
    removeReminderItemButton->setEnabled(false);
    reminderButtons->addWidget(addReminderItemButton);
    reminderButtons->addWidget(saveReminderItemButton);
    reminderButtons->addWidget(removeReminderItemButton);
    reminderButtons->addStretch();
    formLayout->addLayout(reminderButtons);
    formLayout->addStretch();

    layout->addWidget(formBlock, 2);

    QFrame *listBlock = new QFrame(panel);
    listBlock->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout *listLayout = new QVBoxLayout(listBlock);
    listLayout->setContentsMargins(12, 12, 12, 12);
    listLayout->setSpacing(8);
    QLabel *listTitle = new QLabel(QString::fromUtf8("提醒列表"), listBlock);
    listTitle->setStyleSheet("font-weight:600;");
    listLayout->addWidget(listTitle);
    reminderListWidget = new QListWidget(listBlock);
    reminderListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    listLayout->addWidget(reminderListWidget, 1);
    QHBoxLayout *visibilityButtons = new QHBoxLayout();
    visibilityButtons->setContentsMargins(0, 0, 0, 0);
    visibilityButtons->setSpacing(6);
    showSelectedRemindersButton = new QPushButton(QString::fromUtf8("显示选中"), listBlock);
    hideSelectedRemindersButton = new QPushButton(QString::fromUtf8("隐藏选中"), listBlock);
    showAllRemindersButton = new QPushButton(QString::fromUtf8("全部显示"), listBlock);
    hideAllRemindersButton = new QPushButton(QString::fromUtf8("全部隐藏"), listBlock);
    visibilityButtons->addWidget(showSelectedRemindersButton);
    visibilityButtons->addWidget(hideSelectedRemindersButton);
    visibilityButtons->addWidget(showAllRemindersButton);
    visibilityButtons->addWidget(hideAllRemindersButton);
    visibilityButtons->addStretch();
    listLayout->addLayout(visibilityButtons);
    layout->addWidget(listBlock, 3);

    connect(reminderTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::handleReminderTypeChanged);
    connect(addReminderItemButton, &QPushButton::clicked, this, &MainWindow::handleAddReminder);
    connect(saveReminderItemButton, &QPushButton::clicked, this, &MainWindow::handleSaveReminderChanges);
    connect(removeReminderItemButton, &QPushButton::clicked, this, &MainWindow::handleRemoveReminder);
    connect(reminderListWidget, &QListWidget::currentRowChanged, this, &MainWindow::populateReminderForm);
    connect(reminderListWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::updateReminderActionStates);
    connect(showSelectedRemindersButton, &QPushButton::clicked, this, &MainWindow::handleShowSelectedReminders);
    connect(hideSelectedRemindersButton, &QPushButton::clicked, this, &MainWindow::handleHideSelectedReminders);
    connect(showAllRemindersButton, &QPushButton::clicked, this, &MainWindow::handleShowAllReminders);
    connect(hideAllRemindersButton, &QPushButton::clicked, this, &MainWindow::handleHideAllReminders);

    handleReminderTypeChanged(reminderTypeCombo->currentIndex());
    refreshReminderList();
    return panel;
}

QWidget *MainWindow::buildDisplayModePanel()
{
    QWidget *panel = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QFrame *block = new QFrame(panel);
    block->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout *blockLayout = new QVBoxLayout(block);
    blockLayout->setContentsMargins(12, 12, 12, 12);
    blockLayout->setSpacing(8);

    QLabel *title = new QLabel(QString::fromUtf8("显示方式"), block);
    title->setStyleSheet("font-weight:600;");
    blockLayout->addWidget(title);

    showWeekendToggle = new QCheckBox(QString::fromUtf8("展示状态下显示周六、周日"), block);
    showWeekendToggle->setChecked(showWeekendInDisplay);
    blockLayout->addWidget(showWeekendToggle);
    QLabel *weekendHint = new QLabel(QString::fromUtf8("仅影响展示状态，配置/临时调课/提醒模式总是显示周末列"), block);
    weekendHint->setWordWrap(true);
    weekendHint->setStyleSheet("color:#94a3b8; font-size:12px;");
    blockLayout->addWidget(weekendHint);

    blockLayout->addStretch();
    layout->addWidget(block, 3);
    layout->addStretch();

    return panel;
}

QWidget *MainWindow::buildConfigPanel()
{
    QWidget *panel = new QWidget(this);
    panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QHBoxLayout *layout = new QHBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto createBadgeIcon = [](const QColor &bgColor, const QString &symbol) {
        QPixmap pix(32, 32);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(bgColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(pix.rect().adjusted(1, 1, -1, -1));
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(12);
        painter.setFont(font);
        painter.drawText(pix.rect(), Qt::AlignCenter, symbol);
        return QIcon(pix);
    };

    QWidget *blockList = new QWidget(panel);
    QVBoxLayout *blockListLayout = new QVBoxLayout(blockList);
    blockListLayout->setContentsMargins(0, 0, 0, 0);
    blockListLayout->setSpacing(6);
    QHBoxLayout *listHeaderLayout = new QHBoxLayout();
    listHeaderLayout->setSpacing(8);
    listHeaderLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *listTitle = new QLabel(QString::fromUtf8("\xe8\xaf\xbe\xe7\xa8\x8b\xe8\xa1\xa8"), blockList);
    listTitle->setStyleSheet("font-weight:600;");
    listHeaderLayout->addWidget(listTitle);
    addBaseScheduleButton = new QPushButton(QString::fromUtf8("\xe6\x96\xb0\xe5\xbb\xba\xe8\xaf\xbe\xe7\xa8\x8b\xe8\xa1\xa8"), blockList);
    addBaseScheduleButton->setToolTip(QString::fromUtf8("\xe5\x9f\xba\xe4\xba\x8e\xe5\xbd\x93\xe5\x89\x8d\xe8\xa1\xa8\xe6\xa0\xbc\xe5\x88\x9b\xe5\xbb\xba\xe4\xb8\x80\xe4\xb8\xaa\xe6\x96\xb0\xe7\x9a\x84\xe5\x9f\xba\xe7\xa1\x80\xe8\xaf\xbe\xe8\xa1\xa8\xe6\x9d\xa1\xe7\x9b\xae"));
    addBaseScheduleButton->setFixedWidth(120);
    addBaseScheduleButton->setStyleSheet("QPushButton{background:#2563eb;color:white;border:0;border-radius:6px;padding:6px 12px;} QPushButton:hover{background:#1d4ed8;}");
    listHeaderLayout->addWidget(addBaseScheduleButton);
    resetAllDataButton = new QPushButton(QString::fromUtf8("重置"), blockList);
    resetAllDataButton->setToolTip(QString::fromUtf8("清除所有已保存的数据"));
    resetAllDataButton->setCursor(Qt::PointingHandCursor);
    resetAllDataButton->setStyleSheet("QPushButton{padding:6px 12px; border-radius:6px; background:#ef4444; color:white;} QPushButton:hover{background:#dc2626;}");
    listHeaderLayout->addWidget(resetAllDataButton);
    listHeaderLayout->addStretch();
    blockListLayout->addLayout(listHeaderLayout);
    baseScheduleList = new QListWidget(blockList);
    baseScheduleList->setSelectionMode(QAbstractItemView::SingleSelection);
    baseScheduleList->setMinimumHeight(140);
    baseScheduleList->setStyleSheet("QListWidget{background:#f9fafb;border:1px solid #d0d7de;border-radius:4px;} QListWidget::item{padding:6px;}");
    baseScheduleList->setMaximumHeight(120);
    blockListLayout->addWidget(baseScheduleList);
    QWidget *block1 = new QWidget(panel);
    QVBoxLayout *block1Layout = new QVBoxLayout(block1);
    block1Layout->setContentsMargins(0, 0, 0, 0);
    block1Layout->setSpacing(8);
    QLabel *rowOpsTitle = new QLabel(QString::fromUtf8("\xe8\xa1\x8c\xe6\x93\x8d\xe4\xbd\x9c"), block1);
    rowOpsTitle->setStyleSheet("font-weight:600;");
    block1Layout->addWidget(rowOpsTitle, 0, Qt::AlignLeft);

    QVBoxLayout *rowButtonLayout = new QVBoxLayout();
    rowButtonLayout->setContentsMargins(0, 0, 0, 0);
    rowButtonLayout->setSpacing(3);
    addMergedRowButton = new QPushButton(QString(), block1);
    addMergedRowButton->setFixedWidth(60);
    addMergedRowButton->setStyleSheet("QPushButton{background:#eef2ff;border:1px solid #c7d2fe;border-radius:6px;padding:6px;} QPushButton:hover{background:#e0e7ff;}");
    addMergedRowButton->setIcon(createBadgeIcon(QColor("#6366f1"), QString::fromUtf8("合")));
    addMergedRowButton->setIconSize(QSize(22, 22));
    addLessonRowButton = new QPushButton(QString(), block1);
    addLessonRowButton->setFixedWidth(60);
    addLessonRowButton->setStyleSheet("QPushButton{background:#eef2ff;border:1px solid #c7d2fe;border-radius:6px;padding:6px;} QPushButton:hover{background:#e0e7ff;}");
    addLessonRowButton->setIcon(createBadgeIcon(QColor("#22d3ee"), QString::fromUtf8("+")));
    addLessonRowButton->setIconSize(QSize(22, 22));
    deleteRowButton = new QPushButton(QString(), block1);
    deleteRowButton->setFixedWidth(60);
    deleteRowButton->setStyleSheet("QPushButton{background:#fee2e2;border:1px solid #fecaca;border-radius:6px;padding:6px;} QPushButton:hover{background:#fecaca;}");
    deleteRowButton->setIcon(createBadgeIcon(QColor("#ef4444"), QString::fromUtf8("×")));
    deleteRowButton->setIconSize(QSize(22, 22));

    rowButtonLayout->addWidget(addMergedRowButton);
    rowButtonLayout->addWidget(addLessonRowButton);
    rowButtonLayout->addWidget(deleteRowButton);
    block1Layout->addLayout(rowButtonLayout);
    block1Layout->addStretch();

    QWidget *block2 = new QWidget(panel);
    QVBoxLayout *block2Layout = new QVBoxLayout(block2);
    block2Layout->setContentsMargins(0, 0, 0, 0);
    block2Layout->setSpacing(6);
    QLabel *block2Title = new QLabel(QString::fromUtf8("\xe9\x80\x82\xe7\x94\xa8\xe6\x97\xa5\xe6\x9c\x9f"), block2);
    block2Title->setStyleSheet("font-weight:600;");
    block2Layout->addWidget(block2Title);

    QLabel *startLabel = new QLabel(QString::fromUtf8("\xe5\xbc\x80\xe5\xa7\x8b\xe6\x97\xa5\xe6\x9c\x9f"), block2);
    baseStartDateEdit = new QDateEdit(block2);
    baseStartDateEdit->setCalendarPopup(true);
    baseStartDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    baseStartDateEdit->setMaximumWidth(130);
    QLabel *endLabel = new QLabel(QString::fromUtf8("\xe7\xbb\x93\xe6\x9d\x9f\xe6\x97\xa5\xe6\x9c\x9f"), block2);
    baseEndDateEdit = new QDateEdit(block2);
    baseEndDateEdit->setCalendarPopup(true);
    baseEndDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    baseEndDateEdit->setMaximumWidth(130);

    QVBoxLayout *dateLayout = new QVBoxLayout();
    dateLayout->setSpacing(4);
    QHBoxLayout *startRow = new QHBoxLayout();
    startRow->setSpacing(4);
    startRow->addWidget(startLabel);
    startRow->addWidget(baseStartDateEdit, 1);
    QHBoxLayout *endRow = new QHBoxLayout();
    endRow->setSpacing(4);
    endRow->addWidget(endLabel);
    endRow->addWidget(baseEndDateEdit, 1);
    dateLayout->addLayout(startRow);
    dateLayout->addLayout(endRow);
    block2Layout->addLayout(dateLayout);
    block2Layout->addSpacing(4);

    addMergedRowButton->setToolTip(QString::fromUtf8("\xe9\x80\x89\xe4\xb8\xad\xe6\x97\xb6\xe9\x97\xb4\xe8\xa1\x8c\xe5\x90\x8e\xe6\x8f\x92\xe5\x85\xa5\xe6\x96\xb0\xe7\x9a\x84\xe5\x90\x88\xe5\xb9\xb6\xe8\xa1\x8c"));
    addLessonRowButton->setToolTip(QString::fromUtf8("\xe9\x80\x89\xe4\xb8\xad\xe6\x97\xb6\xe9\x97\xb4\xe8\xa1\x8c\xe5\x90\x8e\xe6\x8f\x92\xe5\x85\xa5\xe6\x96\xb0\xe7\x9a\x84\xe8\xaf\xbe\xe7\xa8\x8b\xe8\xa1\x8c"));
    deleteRowButton->setToolTip(QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4\xe5\xbd\x93\xe5\x89\x8d\xe9\x80\x89\xe4\xb8\xad\xe7\x9a\x84\xe6\x97\xb6\xe9\x97\xb4\xe8\xa1\x8c"));
    startLabel->setToolTip(QString::fromUtf8("\xe8\xae\xbe\xe7\xbd\xae\xe8\xaf\xa5\xe5\x9f\xba\xe7\xa1\x80\xe8\xaf\xbe\xe8\xa1\xa8\xe7\x9a\x84\xe8\xb5\xb7\xe5\xa7\x8b\xe6\x97\xa5\xef\xbc\x8c\xe5\xbf\x85\xe9\xa1\xbb\xe4\xb8\xba\xe5\x91\xa8\xe4\xb8\x80"));
    endLabel->setToolTip(QString::fromUtf8("\xe8\xae\xbe\xe7\xbd\xae\xe8\xaf\xa5\xe5\x9f\xba\xe7\xa1\x80\xe8\xaf\xbe\xe8\xa1\xa8\xe7\x9a\x84\xe7\xbb\x93\xe6\x9d\x9f\xe6\x97\xa5\xef\xbc\x8c\xe5\xbf\x85\xe9\xa1\xbb\xe4\xb8\xba\xe5\x91\xa8\xe6\x97\xa5"));

    QHBoxLayout *saveLayout = new QHBoxLayout();
    saveLayout->setSpacing(8);
    saveScheduleButton = new QPushButton(QString::fromUtf8("\xe4\xbf\x9d\xe5\xad\x98\xe8\xaf\xbe\xe8\xa1\xa8"), block2);
    saveScheduleButton->setToolTip(QString::fromUtf8("\xe4\xbf\x9d\xe5\xad\x98\xe5\xbd\x93\xe5\x89\x8d\xe7\x9a\x84\xe5\x9f\xba\xe7\xa1\x80\xe8\xaf\xbe\xe8\xa1\xa8\xe8\xae\xbe\xe7\xbd\xae"));
    saveScheduleButton->setStyleSheet("QPushButton{background:#10b981;color:white;border:0;border-radius:4px;padding:8px 14px;} QPushButton:hover{background:#0ea374;}");
    discardScheduleButton = new QPushButton(QString::fromUtf8("\xe6\x94\xbe\xe5\xbc\x83\xe6\x9b\xb4\xe6\x94\xb9"), block2);
    discardScheduleButton->setToolTip(QString::fromUtf8("\xe8\xbf\x98\xe5\x8e\x9f\xe5\xb7\xb2\xe4\xbf\x9d\xe5\xad\x98\xe7\x9a\x84\xe5\x9f\xba\xe7\xa1\x80\xe8\xaf\xbe\xe8\xa1\xa8"));
    discardScheduleButton->setStyleSheet("QPushButton{background:#f97316;color:white;border:0;border-radius:4px;padding:8px 14px;} QPushButton:hover{background:#ea580c;}");
    saveLayout->addWidget(saveScheduleButton);
    saveLayout->addWidget(discardScheduleButton);
    block2Layout->addLayout(saveLayout);
    block2Layout->addStretch();

    QWidget *block3 = new QWidget(panel);
    QVBoxLayout *block3Layout = new QVBoxLayout(block3);
    block3Layout->setContentsMargins(0, 0, 0, 0);
    block3Layout->setSpacing(4);
    QLabel *block3Title = new QLabel(QString::fromUtf8("\xe5\xb8\xb8\xe7\x94\xa8\xe8\xaf\xbe\xe7\xa8\x8b"), block3);
    block3Title->setStyleSheet("font-weight:600;");
    block3Layout->addWidget(block3Title, 0, Qt::AlignLeft);
    quickCourseWidget = new QWidget(block3);
    quickCourseLayout = new QGridLayout(quickCourseWidget);
    quickCourseLayout->setContentsMargins(0, 0, 0, 0);
    quickCourseLayout->setHorizontalSpacing(6);
    quickCourseLayout->setVerticalSpacing(6);
    block3Layout->addWidget(quickCourseWidget, 0, Qt::AlignTop);
    block3Layout->addStretch();

    layout->addWidget(blockList, 4, Qt::AlignTop);
    layout->addWidget(block1, 1, Qt::AlignTop);
    layout->addWidget(block3, 1, Qt::AlignTop);
    layout->addWidget(block2, 1, Qt::AlignTop);

    panel->setMaximumHeight(220);

    connect(addMergedRowButton, &QPushButton::clicked, this, &MainWindow::handleAddMergedRow);
    connect(addLessonRowButton, &QPushButton::clicked, this, &MainWindow::handleAddLessonRow);
    connect(deleteRowButton, &QPushButton::clicked, this, &MainWindow::handleDeleteRow);
    connect(saveScheduleButton, &QPushButton::clicked, this, &MainWindow::handleSaveBaseSchedule);
    connect(discardScheduleButton, &QPushButton::clicked, this, &MainWindow::handleDiscardBaseSchedule);
    connect(baseStartDateEdit, &QDateEdit::dateChanged, this, &MainWindow::handleEffectiveDateChanged);
    connect(baseEndDateEdit, &QDateEdit::dateChanged, this, &MainWindow::handleEffectiveDateChanged);
    if (baseScheduleList)
        connect(baseScheduleList, &QListWidget::currentRowChanged, this, &MainWindow::handleBaseScheduleSelectionChanged);
    if (addBaseScheduleButton)
        connect(addBaseScheduleButton, &QPushButton::clicked, this, &MainWindow::handleCreateBaseSchedule);
    if (resetAllDataButton)
        connect(resetAllDataButton, &QPushButton::clicked, this, &MainWindow::handleResetAllData);

    updateQuickCourseButtons();
    updateSaveButtons();
    updateEffectiveDateEditors();
    refreshBaseScheduleList();
    return panel;
}

void MainWindow::setupScheduleTable()
{
    const int rowCount = lessonTimes.size() + 1;
    const int columnCount = 8;
    scheduleTable = new QTableWidget(rowCount, columnCount, this);
    scheduleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    scheduleTable->setSelectionMode(QAbstractItemView::NoSelection);
    scheduleTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    scheduleTable->setFocusPolicy(Qt::NoFocus);
    scheduleTable->setAlternatingRowColors(true);
    scheduleTable->horizontalHeader()->setVisible(false);
    scheduleTable->verticalHeader()->setVisible(false);
    scheduleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    scheduleTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    scheduleTable->horizontalHeader()->setMinimumSectionSize(72);
    scheduleTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    scheduleTable->setStyleSheet(QStringLiteral("QTableWidget{gridline-color:#d0d0d0;} QTableWidget::item{padding:6px;}"));

    scheduleHeaderBaseFont = scheduleTable->font();
    if (scheduleHeaderBaseFont.pointSizeF() <= 0)
    {
        const int pixelSize = scheduleHeaderBaseFont.pixelSize();
        scheduleHeaderBaseFont.setPointSizeF(pixelSize > 0 ? pixelSize : 12.0);
    }

    if (scheduleTable->horizontalHeader())
    {
        connect(scheduleTable->horizontalHeader(), &QHeaderView::sectionResized, this, [this](int, int, int) {
            updateAdaptiveCellFonts();
            updateReminderCards();
        });
    }
    if (scheduleTable->verticalHeader())
    {
        connect(scheduleTable->verticalHeader(), &QHeaderView::sectionResized, this, [this](int, int, int) {
            updateAdaptiveCellFonts();
            updateReminderCards();
        });
    }
    if (scheduleTable->model())
    {
        connect(scheduleTable->model(), &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex &, int, int) {
            updateAdaptiveCellFonts();
            updateReminderCards();
        });
        connect(scheduleTable->model(), &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex &, int, int) {
            updateAdaptiveCellFonts();
            updateReminderCards();
        });
    }
    if (scheduleTable->verticalScrollBar())
    {
        connect(scheduleTable->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            updateReminderCards();
        });
    }
    if (scheduleTable->horizontalScrollBar())
    {
        connect(scheduleTable->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            updateReminderCards();
        });
    }

    // initialize empty items to control alignment later
    for (int row = 0; row < scheduleTable->rowCount(); ++row)
    {
        for (int col = 0; col < scheduleTable->columnCount(); ++col)
        {
            auto *item = new QTableWidgetItem();
            item->setTextAlignment(Qt::AlignCenter);
            scheduleTable->setItem(row, col, item);
        }
    }

    ensureCellWidgets();
    if (scheduleTable->viewport())
        scheduleTable->viewport()->installEventFilter(this);

    reminderOverlay = new QWidget(scheduleTable);
    reminderOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    reminderOverlay->setAttribute(Qt::WA_NoSystemBackground);
    reminderOverlay->setAttribute(Qt::WA_TranslucentBackground);
    reminderOverlay->hide();
}

void MainWindow::connectSignals()
{
    connect(previousWeekButton, &QPushButton::clicked, this, [this]() {
        handleWeekNavigation(-7);
    });
    connect(nextWeekButton, &QPushButton::clicked, this, [this]() {
        handleWeekNavigation(7);
    });
    connect(currentWeekButton, &QPushButton::clicked, this, [this]() {
        currentWeekStart = mondayOf(QDate::currentDate());
        updateWeekLabel();
        refreshDateHeader();
        refreshTimeColumn();
        refreshScheduleCells();
    });
    connect(weekLabel, &QPushButton::clicked, this, [this]() {
        if (openTermSettingsDialog())
        {
            updateWeekLabel();
            refreshDateHeader();
            refreshTimeColumn();
            refreshScheduleCells();
        }
    });

    connect(configButton, &QPushButton::clicked, this, [this]() { showSecondaryPanel(SecondaryPanel::Config); });
    connect(temporaryAdjustButton, &QPushButton::clicked, this, [this]() { showSecondaryPanel(SecondaryPanel::TemporaryAdjust); });
    connect(addReminderButton, &QPushButton::clicked, this, [this]() { showSecondaryPanel(SecondaryPanel::Reminder); });
    if (displayModeButton)
    {
        connect(displayModeButton, &QPushButton::clicked, this, [this]() { showSecondaryPanel(SecondaryPanel::DisplayMode); });
    }
    if (statisticsButton)
    {
        connect(statisticsButton, &QPushButton::clicked, this, &MainWindow::showStatisticsDialog);
    }
    if (minimalToggleButton)
    {
        connect(minimalToggleButton, &QPushButton::toggled, this, [this](bool checked) {
            applyManualMinimalMode(checked);
        });
    }
    connect(scheduleTable, &QTableWidget::cellClicked, this, &MainWindow::handleTableCellClicked);
    connect(scheduleTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::handleCourseCellDoubleClicked);
    if (resetTemporaryAdjustButton)
    {
        connect(resetTemporaryAdjustButton, &QPushButton::clicked, this, &MainWindow::handleResetTemporaryAdjustments);
    }
    if (showWeekendToggle)
    {
        connect(showWeekendToggle, &QCheckBox::toggled, this, [this](bool checked) {
            showWeekendInDisplay = checked;
            applyDisplayPreferences();
        });
    }
}

void MainWindow::applyChromeVisibility(bool visible)
{
    if (mainToolbarWidget)
        mainToolbarWidget->setVisible(visible);

    if (scheduleTable)
    {
        // 日期行（首行）始终显示，保持展示区完整
        scheduleTable->setRowHidden(0, false);
        updateAdaptiveCellFonts();
        updateReminderCards();
    }

    if (statusBar())
        statusBar()->setVisible(false); // 始终隐藏状态栏

    // 具体的标题栏隐藏/恢复在 applyManualMinimalMode 控制，这里不切换 frameless
}

bool MainWindow::isDisplayViewActive() const
{
    if (activePanel != SecondaryPanel::None)
        return false;
    if (!secondaryContainer)
        return true;
    return !secondaryContainer->isVisible();
}

bool MainWindow::shouldAutoHideChrome() const
{
    return false;
}

void MainWindow::applyDisplayPreferences()
{
    const bool inDisplayView = isDisplayViewActive();
    constexpr int kNormalMargin = 12;
    constexpr int kNormalSpacing = 8;
    chromeHiddenInDisplay = manualMinimalMode;

    if (scheduleTable)
    {
        const int colCount = scheduleTable->columnCount();
        const bool hideWeekend = inDisplayView && !showWeekendInDisplay;
        if (colCount > 6)
            scheduleTable->setColumnHidden(6, hideWeekend);
        if (colCount > 7)
            scheduleTable->setColumnHidden(7, hideWeekend);
    }

    // 控制标题栏/工具栏显隐，保持边距不变，避免闪烁
    applyChromeVisibility(!manualMinimalMode);
    if (mainLayoutRoot)
    {
        int topMargin = kNormalMargin;
        if (manualMinimalMode && mainToolbarWidget)
            topMargin = kNormalMargin; // 上下保持一致高度
        mainLayoutRoot->setContentsMargins(kNormalMargin, topMargin, kNormalMargin, kNormalMargin);
        mainLayoutRoot->setSpacing(kNormalSpacing);
    }
    if (centralContainer)
        centralContainer->setStyleSheet(QString());
    if (scheduleTable)
    {
        scheduleTable->setStyleSheet(QStringLiteral("QTableWidget{gridline-color:#d0d0d0;} QTableWidget::item{padding:6px;}"));
    }

    if (mainToolbarWidget)
        mainToolbarWidget->setVisible(!manualMinimalMode);

    updateReminderCards();
    updateAdaptiveCellFonts();
}

void MainWindow::applyManualMinimalMode(bool minimal)
{
    if (manualMinimalMode == minimal)
        return;

    // 记录当前课程表在屏幕上的位置和尺寸，切换标题栏/工具栏后保持视觉不跳动
    QPoint targetTopLeftGlobal = scheduleTable ? scheduleTable->mapToGlobal(QPoint(0, 0)) : frameGeometry().topLeft();
    QSize targetTableSize = scheduleTable ? scheduleTable->size() : size();

    if (minimal)
    {
        savedFrameGeometry = frameGeometry();
        savedFrameGeometryValid = true;

        setUpdatesEnabled(false);
        setCaptionVisible(false); // 隐藏标题栏但保留可缩放边框
        setVisible(true);
        adjustWindowToKeepTable(targetTopLeftGlobal, targetTableSize); // 对齐窗口以保持课程表位置/大小
        setUpdatesEnabled(true);

        if (mainToolbarWidget)
            mainToolbarWidget->setVisible(false);
        if (centralContainer)
            centralContainer->setStyleSheet(QStringLiteral("#centralContainer{background:white; border-radius:12px;}"));
    }
    else
    {
        setCaptionVisible(true);
        setVisible(true);
        adjustWindowToKeepTable(targetTopLeftGlobal, targetTableSize); // 恢复标题栏后仍保持课程表位置
        if (mainToolbarWidget)
            mainToolbarWidget->setVisible(true);
        if (centralContainer)
            centralContainer->setStyleSheet(QString());
    }

    manualMinimalMode = minimal;
    chromeHiddenInDisplay = minimal;
    if (minimalToggleButton)
        minimalToggleButton->setChecked(minimal);

    applyDisplayPreferences();
}

void MainWindow::adjustWindowToKeepTable(const QPoint &targetTopLeftGlobal, const QSize &targetSize)
{
    if (!scheduleTable)
        return;

    // 根据目标位置/尺寸平移并微调窗口，保持课程表在屏幕上的视觉稳定
    const QPoint currentTopLeft = scheduleTable->mapToGlobal(QPoint(0, 0));
    const QSize currentSize = scheduleTable->size();
    QPoint moveDelta = targetTopLeftGlobal - currentTopLeft;
    QSize sizeDelta = targetSize - currentSize;

    QRect newGeom = frameGeometry();
    newGeom.translate(moveDelta);
    newGeom.setSize(newGeom.size() + sizeDelta);
    setGeometry(newGeom);
}

void MainWindow::setCaptionVisible(bool visible)
{
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    if (!hwnd)
        return;
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    if (visible)
        style |= (WS_CAPTION | WS_THICKFRAME);
    else
        style &= ~(WS_CAPTION); // 保留可缩放边框
    SetWindowLong(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE);
#else
    setWindowFlag(Qt::FramelessWindowHint, !visible);
    setGeometry(frameGeometry());
    show();
#endif
}

QRect MainWindow::clientRectFromFrame(const QRect &frame) const
{
    const int frameW = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, this);
    const int titleH = style()->pixelMetric(QStyle::PM_TitleBarHeight, nullptr, this);
    return frame.adjusted(frameW, frameW + titleH, -frameW, -frameW);
}
void MainWindow::scheduleChromeAutoHide()
{
    // 自动隐藏逻辑已停用（只显示课程表模式由按钮控制）
}

void MainWindow::cancelChromeAutoHide()
{
    if (chromeAutoHideTimer)
        chromeAutoHideTimer->stop();
}

bool MainWindow::canUseFramelessResize() const
{
    return manualMinimalMode;
}

MainWindow::ResizeArea MainWindow::hitTestResizeArea(const QPoint &pos) const
{
    const int margin = 8;
    const int w = width();
    const int h = height();
    const bool left = pos.x() <= margin;
    const bool right = (w - pos.x()) <= margin;
    const bool top = pos.y() <= margin;
    const bool bottom = (h - pos.y()) <= margin;

    if (top && left)
        return ResizeArea::TopLeft;
    if (top && right)
        return ResizeArea::TopRight;
    if (bottom && left)
        return ResizeArea::BottomLeft;
    if (bottom && right)
        return ResizeArea::BottomRight;
    if (left)
        return ResizeArea::Left;
    if (right)
        return ResizeArea::Right;
    if (top)
        return ResizeArea::Top;
    if (bottom)
        return ResizeArea::Bottom;
    return ResizeArea::None;
}

bool MainWindow::handleFramelessResizeEvent(QObject *, QEvent *event)
{
    if (!canUseFramelessResize())
        return false;

    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            resizeArea = hitTestResizeArea(mapFromGlobal(mouseEvent->globalPos()));
            if (resizeArea != ResizeArea::None)
            {
                resizeInProgress = true;
                resizeStartPos = mouseEvent->globalPos();
                resizeStartGeometry = geometry();
                return true;
            }
        }
    }
    else if (event->type() == QEvent::MouseMove)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPoint globalPos = mouseEvent->globalPos();

        if (resizeInProgress && (mouseEvent->buttons() & Qt::LeftButton))
        {
            const QPoint delta = globalPos - resizeStartPos;
            QRect newGeom = resizeStartGeometry;
            const int minW = minimumWidth();
            const int minH = minimumHeight();

            auto applyDelta = [&](ResizeArea area) {
                switch (area)
                {
                case ResizeArea::Left:
                case ResizeArea::TopLeft:
                case ResizeArea::BottomLeft:
                    newGeom.setLeft(newGeom.left() + delta.x());
                    if (newGeom.width() < minW)
                        newGeom.setLeft(newGeom.right() - minW + 1);
                    break;
                default:
                    break;
                }

                switch (area)
                {
                case ResizeArea::Right:
                case ResizeArea::TopRight:
                case ResizeArea::BottomRight:
                    newGeom.setRight(newGeom.right() + delta.x());
                    if (newGeom.width() < minW)
                        newGeom.setRight(newGeom.left() + minW - 1);
                    break;
                default:
                    break;
                }

                switch (area)
                {
                case ResizeArea::Top:
                case ResizeArea::TopLeft:
                case ResizeArea::TopRight:
                    newGeom.setTop(newGeom.top() + delta.y());
                    if (newGeom.height() < minH)
                        newGeom.setTop(newGeom.bottom() - minH + 1);
                    break;
                default:
                    break;
                }

                switch (area)
                {
                case ResizeArea::Bottom:
                case ResizeArea::BottomLeft:
                case ResizeArea::BottomRight:
                    newGeom.setBottom(newGeom.bottom() + delta.y());
                    if (newGeom.height() < minH)
                        newGeom.setBottom(newGeom.top() + minH - 1);
                    break;
                default:
                    break;
                }
            };

            applyDelta(resizeArea);
            setGeometry(newGeom.normalized());
            return true;
        }
        else
        {
            ResizeArea area = hitTestResizeArea(mapFromGlobal(globalPos));
            switch (area)
            {
            case ResizeArea::TopLeft:
            case ResizeArea::BottomRight:
                setCursor(Qt::SizeFDiagCursor);
                break;
            case ResizeArea::TopRight:
            case ResizeArea::BottomLeft:
                setCursor(Qt::SizeBDiagCursor);
                break;
            case ResizeArea::Left:
            case ResizeArea::Right:
                setCursor(Qt::SizeHorCursor);
                break;
            case ResizeArea::Top:
            case ResizeArea::Bottom:
                setCursor(Qt::SizeVerCursor);
                break;
            case ResizeArea::None:
            default:
                unsetCursor();
                break;
            }
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease)
    {
        if (resizeInProgress)
        {
            resizeInProgress = false;
            resizeArea = ResizeArea::None;
            unsetCursor();
            return true;
        }
    }

    return false;
}

void MainWindow::handleWeekNavigation(int offsetDays)
{
    currentWeekStart = currentWeekStart.addDays(offsetDays);
    updateWeekLabel();
    refreshDateHeader();
    refreshTimeColumn();
    refreshScheduleCells();
}

void MainWindow::updateWeekLabel()
{
    if (!weekLabel)
        return;

    if (!hasTermRange)
    {
        weekLabel->setText(QString::fromUtf8("\xe7\xac\xac\x31\xe5\x91\xa8"));
        return;
    }

    const QDate termMonday = mondayOf(termStartDate);
    const int diffDays = termMonday.daysTo(currentWeekStart);
    int weekIndex = diffDays >= 0 ? (diffDays / 7 + 1) : 1;

    QString labelText = QString::fromUtf8("\xe7\xac\xac\x25\x31\xe5\x91\xa8").arg(weekIndex);
    weekLabel->setText(labelText);
}

void MainWindow::refreshDateHeader()
{
    static const QStringList weekdays = {
        QString::fromUtf8("\xe6\x98\x9f\xe6\x9c\x9f\xe4\xb8\x80"), QString::fromUtf8("\xe6\x98\x9f\xe6\x9c\x9f\xe4\xba\x8c"), QString::fromUtf8("\xe6\x98\x9f\xe6\x9c\x9f\xe4\xb8\x89"),
        QString::fromUtf8("\xe6\x98\x9f\xe6\x9c\x9f\xe5\x9b\x9b"), QString::fromUtf8("\xe6\x98\x9f\xe6\x9c\x9f\xe4\xba\x94"), QString::fromUtf8("\xe6\x98\x9f\xe6\x9c\x9f\xe5\x85\xad"),
        QString::fromUtf8("\xe6\x98\x9f\xe6\x9c\x9f\xe6\x97\xa5")};

    // Clear the top-left cell
    scheduleTable->item(0, 0)->setText("");

    QDate day = currentWeekStart;
    const QDate today = QDate::currentDate();
    for (int col = 1; col < scheduleTable->columnCount(); ++col)
    {
        const QString weekdayName = weekdays.at(col - 1);
        const QString dateText = day.toString("MM/dd");
        QTableWidgetItem *headerItem = scheduleTable->item(0, col);
        if (!headerItem)
            continue;
        headerItem->setText(QString("%1\n%2").arg(weekdayName, dateText));
        headerItem->setTextAlignment(Qt::AlignCenter);
        const bool isToday = (day == today);
        if (isToday)
        {
            headerItem->setBackground(QColor("#dbeafe"));
            headerItem->setForeground(QColor("#0f172a"));
            QPixmap indicator(12, 12);
            indicator.fill(Qt::transparent);
            QPainter painter(&indicator);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setBrush(QColor("#f97316"));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(0, 0, 12, 12);
            painter.end();
            headerItem->setData(Qt::DecorationRole, indicator);
        }
        else
        {
            headerItem->setBackground(Qt::white);
            headerItem->setForeground(QColor("#0f172a"));
            headerItem->setData(Qt::DecorationRole, QVariant());
        }
        day = day.addDays(1);
    }

    updateAdaptiveCellFonts();
}

void MainWindow::refreshTimeColumn()
{
    if (!scheduleTable)
        return;

    synchronizeScheduleRowCount();

    if (auto *headerItem = scheduleTable->item(0, 0))
        headerItem->setText("");

    const MainWindow::BaseScheduleEntry *base = activeBaseSchedule();
    const int totalRows = scheduleTable->rowCount();
    for (int row = 1; row < totalRows; ++row)
    {
        QString text;
        if (base && row - 1 < base->timeColumnTexts.size())
        {
            text = base->timeColumnTexts.at(row - 1);
        }
        else if (row - 1 < lessonTimes.size())
        {
            QString periodText = QString::fromUtf8("\xe7\xac\xac") + QString::number(row) + QString::fromUtf8("\xe8\x8a\x82");
            text = QString("%1\n%2").arg(periodText, lessonTimes.at(row - 1));
        }
        else
        {
            text = QString::fromUtf8("\xe7\xac\xac") + QString::number(row) + QString::fromUtf8("\xe8\x8a\x82");
        }

        QTableWidgetItem *item = scheduleTable->item(row, 0);
        if (!item)
        {
            item = new QTableWidgetItem();
            item->setTextAlignment(Qt::AlignCenter);
            scheduleTable->setItem(row, 0, item);
        }
        item->setText(text);
        item->setTextAlignment(Qt::AlignCenter);
    }
    updateEffectiveDateEditors();
    updateAdaptiveCellFonts();
}

void MainWindow::updateEffectiveDateEditors()
{
    if (!baseStartDateEdit || !baseEndDateEdit)
        return;

    const BaseScheduleEntry *base = activeBaseSchedule();
    const bool editable = base && isSelectedBaseEditable();
    suppressDateEditSignal = true;
    if (!base)
    {
        baseStartDateEdit->setEnabled(false);
        baseEndDateEdit->setEnabled(false);
        QDate fallback = mondayOf(currentWeekStart);
        baseStartDateEdit->setDate(fallback);
        baseEndDateEdit->setDate(fallback.addDays(6));
    }
    else
    {
        baseStartDateEdit->setEnabled(editable);
        baseEndDateEdit->setEnabled(editable);
        baseStartDateEdit->setDate(base->startDate.isValid() ? base->startDate : mondayOf(currentWeekStart));
        QDate end = base->endDate.isValid() ? base->endDate : baseStartDateEdit->date().addDays(6);
        baseEndDateEdit->setDate(end);
    }
    suppressDateEditSignal = false;
}

void MainWindow::setForcedBaseScheduleIndex(int index)
{
    if (index >= 0 && index < baseSchedules.size())
        forcedBaseScheduleIndex = index;
    else
        forcedBaseScheduleIndex = -1;
}

void MainWindow::ensureConfigSelection()
{
    if (baseSchedules.isEmpty())
    {
        configSelectedBaseScheduleIndex = -1;
        setForcedBaseScheduleIndex(-1);
        return;
    }

    if (configSelectedBaseScheduleIndex < 0 || configSelectedBaseScheduleIndex >= baseSchedules.size())
    {
        const QDate weekEnd = currentWeekStart.addDays(6);
        configSelectedBaseScheduleIndex = 0;
        for (int i = 0; i < baseSchedules.size(); ++i)
        {
            const BaseScheduleEntry &entry = baseSchedules.at(i);
            if (!entry.startDate.isValid() || !entry.endDate.isValid())
                continue;
            if (entry.startDate <= currentWeekStart && entry.endDate >= weekEnd)
            {
                configSelectedBaseScheduleIndex = i;
                break;
            }
        }
    }
}

void MainWindow::refreshBaseScheduleList()
{
    ensureConfigSelection();
    if (!baseScheduleList)
        return;

    QVector<int> displayOrder;
    displayOrder.reserve(baseSchedules.size());
    for (int i = 0; i < baseSchedules.size(); ++i)
    {
        if (i == unsavedBaseScheduleIndex)
            continue;
        displayOrder.append(i);
    }
    if (unsavedBaseScheduleIndex >= 0 && unsavedBaseScheduleIndex < baseSchedules.size())
        displayOrder.append(unsavedBaseScheduleIndex);

    const int dirtyIndex = baseScheduleDirty ? currentEditableBaseIndex() : -1;
    auto createStatusIcon = [](const QColor &bgColor, const QString &symbol) {
        QPixmap pix(20, 20);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(bgColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(pix.rect().adjusted(1, 1, -1, -1));
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(9);
        painter.setFont(font);
        painter.drawText(pix.rect(), Qt::AlignCenter, symbol);
        return QIcon(pix);
    };
    const QIcon savedIcon = createStatusIcon(QColor("#22c55e"), QString::fromUtf8("\xe4\xbf\x9d"));
    const QIcon dirtyIcon = createStatusIcon(QColor("#f97316"), QString::fromUtf8("\xe6\x94\xb9"));

    QSignalBlocker blocker(baseScheduleList);
    baseScheduleList->clear();
    int selectedRow = -1;
    for (int row = 0; row < displayOrder.size(); ++row)
    {
        const int baseIndex = displayOrder.at(row);
        const BaseScheduleEntry &entry = baseSchedules.at(baseIndex);
        const QString startText = entry.startDate.isValid() ? entry.startDate.toString("yyyy-MM-dd") : QString::fromUtf8("\xe6\x9c\xaa\xe8\xae\xbe\xe7\xbd\xae");
        const QString endText = entry.endDate.isValid() ? entry.endDate.toString("yyyy-MM-dd") : QString::fromUtf8("\xe6\x9c\xaa\xe8\xae\xbe\xe7\xbd\xae");
        const bool isUnsavedNewEntry = unsavedBaseScheduleIndex >= 0 && baseIndex == unsavedBaseScheduleIndex;
        const bool isDirty = (dirtyIndex >= 0 && baseIndex == dirtyIndex) || isUnsavedNewEntry;
        QString label = QString::fromUtf8("\xe8\xaf\xbe\xe7\xa8\x8b\xe8\xa1\xa8%1: %2 - %3")
                            .arg(baseIndex + 1)
                            .arg(startText)
                            .arg(endText);
        const QString statusText = isDirty ? QString::fromUtf8("\xe7\x8a\xb6\xe6\x80\x81\xef\xbc\x9a\xe6\x9c\xaa\xe4\xbf\x9d\xe5\xad\x98")
                                           : QString::fromUtf8("\xe7\x8a\xb6\xe6\x80\x81\xef\xbc\x9a\xe5\xb7\xb2\xe4\xbf\x9d\xe5\xad\x98");
        label += QStringLiteral("\n%1").arg(statusText);
        QListWidgetItem *item = new QListWidgetItem(label, baseScheduleList);
        item->setData(Qt::UserRole, baseIndex);
        item->setIcon(isDirty ? dirtyIcon : savedIcon);
        item->setBackground(isDirty ? QColor("#fff7ed") : QColor("#ecfdf5"));
        QSize hint = item->sizeHint();
        if (hint.height() < 52)
            hint.setHeight(52);
        item->setSizeHint(hint);
        if (baseIndex == configSelectedBaseScheduleIndex)
            selectedRow = row;
    }

    if (selectedRow >= 0)
        baseScheduleList->setCurrentRow(selectedRow);
    else
        baseScheduleList->setCurrentRow(-1);

    if (isConfigModeActive())
        handleBaseScheduleSelectionChanged();
}

void MainWindow::handleBaseScheduleSelectionChanged()
{
    if (!baseScheduleList)
        return;

    QListWidgetItem *currentItem = baseScheduleList->currentItem();
    const int baseIndex = currentItem ? currentItem->data(Qt::UserRole).toInt() : -1;
    if (baseIndex < 0 || baseIndex >= baseSchedules.size())
    {
        configSelectedBaseScheduleIndex = -1;
        editingNewBaseSchedule = false;
        if (isConfigModeActive())
        {
            setForcedBaseScheduleIndex(-1);
            refreshTimeColumn();
            refreshScheduleCells();
            initializeQuickCoursePalette();
            updateEffectiveDateEditors();
        }
        updateToolbarInteractivity();
        return;
    }

    configSelectedBaseScheduleIndex = baseIndex;
    editingNewBaseSchedule = isBaseIndexEditable(baseIndex);
    if (!isConfigModeActive())
    {
        updateToolbarInteractivity();
        return;
    }

    setForcedBaseScheduleIndex(baseIndex);
    ensureCellWidgets();
    refreshTimeColumn();
    refreshScheduleCells();
    applyRowLayout();
    initializeQuickCoursePalette();
    updateEffectiveDateEditors();
    updateToolbarInteractivity();
}

void MainWindow::handleEffectiveDateChanged()
{
    if (suppressDateEditSignal)
        return;
    if (!baseStartDateEdit || !baseEndDateEdit)
        return;
    if (!isSelectedBaseEditable())
        return;

    QDate start = baseStartDateEdit->date();
    QDate end = baseEndDateEdit->date();
    if (!validateEffectiveDateRange(start, end, true))
    {
        suppressDateEditSignal = true;
        if (const BaseScheduleEntry *base = activeBaseSchedule())
        {
            if (base->startDate.isValid())
                baseStartDateEdit->setDate(base->startDate);
            if (base->endDate.isValid())
                baseEndDateEdit->setDate(base->endDate);
        }
        suppressDateEditSignal = false;
        return;
    }

    BaseScheduleEntry *base = activeBaseScheduleMutable();
    if (!base)
        return;

    if (base->startDate == start && base->endDate == end)
        return;

    base->startDate = start;
    base->endDate = end;
    markBaseScheduleDirty();
    updateWeekLabel();
    refreshDateHeader();
    refreshScheduleCells();
    refreshTimeColumn();
    refreshBaseScheduleList();
}

bool MainWindow::validateEffectiveDateRange(const QDate &start, const QDate &end, bool showWarning)
{
    QString message;
    if (!start.isValid())
        message = QString::fromUtf8("请选择有效的开始日期。");
    else if (!end.isValid())
        message = QString::fromUtf8("请选择有效的结束日期。");
    else if (start > end)
        message = QString::fromUtf8("开始日期不能晚于结束日期。");
    else if (start.dayOfWeek() != 1)
        message = QString::fromUtf8("开始日期必须为周一。");
    else if (end.dayOfWeek() != 7)
        message = QString::fromUtf8("结束日期必须为周日。");

    if (!message.isEmpty())
    {
        if (showWarning)
            QMessageBox::warning(this, QString::fromUtf8("提示"), message);
        return false;
    }
    return true;
}

QDate MainWindow::ensureStartMonday(const QDate &date) const
{
    if (date.isValid())
        return mondayOf(date);
    return mondayOf(QDate::currentDate());
}

QDate MainWindow::ensureEndSunday(const QDate &start, const QDate &candidate) const
{
    QDate normalizedStart = start.isValid() ? start : QDate::currentDate();
    normalizedStart = mondayOf(normalizedStart);
    QDate end = candidate.isValid() ? candidate : normalizedStart.addDays(6);
    const int day = end.dayOfWeek();
    if (day != 7)
        end = end.addDays(7 - day);
    if (end < normalizedStart)
        end = normalizedStart.addDays(6);
    return end;
}

void MainWindow::initializeScheduleData()
{
    baseSchedules.clear();
    actualWeekCourses.clear();
    reminderEntries.clear();

    loadBaseScheduleFromDisk();
    loadActualScheduleFromDisk();
    loadReminders();

    persistedBaseSchedules = baseSchedules;
    baseScheduleDirty = false;
    refreshBaseScheduleList();
    ensureConfigSelection();
}

void MainWindow::refreshScheduleCells()
{
    if (!scheduleTable)
        return;

    ensureCustomTemporaryCourseNameUnique();

    synchronizeScheduleRowCount();
    ensureCellWidgets();
    applyRowLayout();

    const MainWindow::BaseScheduleEntry *baseEntry = activeBaseSchedule();
    const bool useActualSchedule = !isConfigModeActive();
    const MainWindow::ActualWeekEntry *actualEntry = useActualSchedule ? findActualWeek(currentWeekStart) : nullptr;

    auto actualCellValue = [&](int rowIndex, int columnIndex) -> MainWindow::CourseCellData {
        if (!actualEntry)
            return MainWindow::CourseCellData::empty();
        const auto rowIt = actualEntry->cellOverrides.constFind(rowIndex);
        if (rowIt == actualEntry->cellOverrides.constEnd())
            return MainWindow::CourseCellData::empty();
        const auto colIt = rowIt->constFind(columnIndex);
        if (colIt == rowIt->constEnd())
            return MainWindow::CourseCellData::empty();
        return colIt.value();
    };

    auto firstActualMeaningful = [&](int rowIndex) -> MainWindow::CourseCellData {
        if (!actualEntry)
            return MainWindow::CourseCellData::empty();
        const auto rowIt = actualEntry->cellOverrides.constFind(rowIndex);
        if (rowIt == actualEntry->cellOverrides.constEnd())
            return MainWindow::CourseCellData::empty();
        for (auto it = rowIt->cbegin(); it != rowIt->cend(); ++it)
        {
            const MainWindow::CourseCellData &cell = it.value();
            if (cell.type != ScheduleCellLabel::CourseType::None || !cell.name.trimmed().isEmpty())
                return cell;
        }
        return MainWindow::CourseCellData::empty();
    };

    const int dataRows = qMax(0, scheduleTable->rowCount() - 1);
    const int dataColumns = qMax(0, scheduleTable->columnCount() - 1);

    for (int row = 0; row < dataRows; ++row)
    {
        const bool rowIsPlaceholder = baseEntry &&
                                      row < baseEntry->rowDefinitions.size() &&
                                      baseEntry->rowDefinitions.at(row).kind == MainWindow::RowDefinition::Kind::Placeholder;

        auto firstMeaningful = [](const QVector<MainWindow::CourseCellData> &rowData) -> MainWindow::CourseCellData {
            for (const MainWindow::CourseCellData &cell : rowData)
            {
                if (cell.type != ScheduleCellLabel::CourseType::None || !cell.name.trimmed().isEmpty())
                    return cell;
            }
            return MainWindow::CourseCellData::empty();
        };

        MainWindow::CourseCellData placeholderData = MainWindow::CourseCellData::empty();
        if (rowIsPlaceholder)
        {
            if (baseEntry && row < baseEntry->courseCells.size())
                placeholderData = firstMeaningful(baseEntry->courseCells.at(row));
            if (useActualSchedule)
            {
                MainWindow::CourseCellData candidate = firstActualMeaningful(row);
                if (candidate.type != ScheduleCellLabel::CourseType::None || !candidate.name.trimmed().isEmpty())
                {
                    if (candidate.type == ScheduleCellLabel::CourseType::Deleted && candidate.name.trimmed().isEmpty() && baseEntry &&
                        row < baseEntry->courseCells.size())
                    {
                        MainWindow::CourseCellData baseCell = baseEntry->courseCells.at(row).value(0, MainWindow::CourseCellData::empty());
                        candidate.name = baseCell.name;
                    }
                    placeholderData = candidate;
                }
            }
        }

        ScheduleCellLabel *placeholderLabel = nullptr;
        if (rowIsPlaceholder)
        {
            if (row < cellLabels.size() && !cellLabels.at(row).isEmpty())
                placeholderLabel = cellLabels.at(row).at(0);
            if (!placeholderLabel)
            {
                placeholderLabel = new ScheduleCellLabel(scheduleTable);
                if (row >= cellLabels.size())
                    cellLabels.resize(row + 1);
                if (cellLabels[row].size() < dataColumns)
                    cellLabels[row].resize(dataColumns);
                cellLabels[row][0] = placeholderLabel;
            connect(placeholderLabel, &ScheduleCellLabel::requestDropOperation, this, &MainWindow::handleTemporaryDrop);
            }
            scheduleTable->setCellWidget(row + 1, 1, placeholderLabel);
            placeholderLabel->setDragEnabled(false);
            placeholderLabel->setTodayHighlighted(false);
            for (int col = 1; col < dataColumns; ++col)
            {
                QWidget *existing = scheduleTable->cellWidget(row + 1, col + 1);
                if (existing && existing != placeholderLabel)
                {
                    scheduleTable->removeCellWidget(row + 1, col + 1);
                    existing->deleteLater();
                }
                if (row < cellLabels.size() && col < cellLabels[row].size())
                    cellLabels[row][col] = placeholderLabel;
            }
        }

        for (int col = 0; col < dataColumns; ++col)
        {
            if (rowIsPlaceholder && col > 0)
                continue;

            MainWindow::CourseCellData data = MainWindow::CourseCellData::empty();
            if (rowIsPlaceholder)
            {
                data = placeholderData;
            }
            else
            {
                if (baseEntry && row < baseEntry->courseCells.size() && col < baseEntry->courseCells.at(row).size())
                    data = baseEntry->courseCells.at(row).at(col);
                if (useActualSchedule)
                {
                    MainWindow::CourseCellData candidate = actualCellValue(row, col);
                    if (candidate.type != ScheduleCellLabel::CourseType::None || !candidate.name.trimmed().isEmpty())
                    {
                        if (candidate.type == ScheduleCellLabel::CourseType::Deleted && candidate.name.trimmed().isEmpty() && baseEntry &&
                            row < baseEntry->courseCells.size() && col < baseEntry->courseCells.at(row).size())
                        {
                            candidate.name = baseEntry->courseCells.at(row).at(col).name;
                        }
                        data = candidate;
                    }
                }
            }

            if (row < cellLabels.size() && col < cellLabels.at(row).size())
            {
                ScheduleCellLabel *label = cellLabels.at(row).at(col);
                if (label)
                {
                    if (!rowIsPlaceholder)
                    {
                        label->setCellCoordinates(row, col);
                        label->setDragEnabled(isTemporaryAdjustModeActive());
                    }
                    else
                    {
                        label->setDragEnabled(false);
                    }

                    ScheduleCellLabel::CourseInfo info;
                    // 两行显示：上面科目名称，下面班级名称（可为空）
                    if (!data.className.trimmed().isEmpty())
                        info.name = data.name + "\n" + data.className;
                    else
                        info.name = data.name;
                    info.background = data.background;
                    info.type = data.type;
                    label->setCourseInfo(info);
                    label->setTodayHighlighted(false);
                    if (data.name.trimmed().isEmpty() && data.type == ScheduleCellLabel::CourseType::None)
                        label->setText(QString());
                }
            }
        }
    }

    updateWeeklyStatistics();
    updateReminderCards();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateReminderOverlayGeometry();
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    QMainWindow::moveEvent(event);
    updateReminderOverlayGeometry();
}

void MainWindow::enterEvent(QEvent *event)
{
    QMainWindow::enterEvent(event);
    cancelChromeAutoHide();
    if (!chromeHiddenInDisplay && mainToolbarWidget)
    {
        mainToolbarWidget->setVisible(true);
        applyChromeVisibility(true); // 仅在未隐藏状态下保持可见
    }
}

void MainWindow::leaveEvent(QEvent *event)
{
    QMainWindow::leaveEvent(event);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (isDisplayViewActive() && event->button() == Qt::LeftButton)
    {
        if (!resizeInProgress && resizeArea == ResizeArea::None)
        {
            displayDragging = true;
            displayDragOffset = event->globalPos() - frameGeometry().topLeft();
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (isDisplayViewActive() && displayDragging && (event->buttons() & Qt::LeftButton))
        move(event->globalPos() - displayDragOffset);
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    displayDragging = false;
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::ensureCellWidgets()
{
    if (!scheduleTable)
        return;

    scheduleTable->clearSpans();

    const int dataRows = scheduleTable->rowCount() - 1;
    const int dataColumns = scheduleTable->columnCount() - 1;

    if (cellLabels.size() == dataRows)
    {
        bool complete = true;
        for (const auto &row : cellLabels)
        {
            if (row.size() != dataColumns)
            {
                complete = false;
                break;
            }
        }
        if (complete)
            return;
    }

    cellLabels.clear();
    cellLabels.resize(dataRows);

    for (int row = 0; row < dataRows; ++row)
    {
        cellLabels[row].resize(dataColumns);
        for (int col = 0; col < dataColumns; ++col)
        {
            auto *label = new ScheduleCellLabel(scheduleTable);
            connect(label, &ScheduleCellLabel::requestDropOperation, this, &MainWindow::handleTemporaryDrop);
            connect(label, &ScheduleCellLabel::requestWeeklyCourseDrop, this, &MainWindow::handleWeeklyCourseDrop);
            cellLabels[row][col] = label;
            scheduleTable->setCellWidget(row + 1, col + 1, label);
            label->setDragEnabled(false);
        }
    }

    updateWeeklyStatistics();
}

void MainWindow::synchronizeScheduleRowCount()
{
    if (!scheduleTable)
        return;

    const MainWindow::BaseScheduleEntry *base = activeBaseSchedule();
    int desiredDataRows = 0;
    if (base)
    {
        desiredDataRows = base->courseCells.size();
        if (desiredDataRows <= 0)
            desiredDataRows = base->rowDefinitions.size();
        if (desiredDataRows <= 0)
            desiredDataRows = base->rows;
    }

    if (desiredDataRows <= 0)
        desiredDataRows = lessonTimes.size();
    if (desiredDataRows <= 0)
        desiredDataRows = 1;

    const int desiredTotalRows = desiredDataRows + 1;
    const int currentRows = scheduleTable->rowCount();
    if (currentRows == desiredTotalRows)
        return;

    if (currentRows < desiredTotalRows)
    {
        while (scheduleTable->rowCount() < desiredTotalRows)
        {
            const int insertRow = scheduleTable->rowCount();
            scheduleTable->insertRow(insertRow);
            for (int col = 0; col < scheduleTable->columnCount(); ++col)
            {
                QTableWidgetItem *item = new QTableWidgetItem();
                item->setTextAlignment(Qt::AlignCenter);
                scheduleTable->setItem(insertRow, col, item);
            }
        }
    }
    else
    {
        while (scheduleTable->rowCount() > desiredTotalRows && scheduleTable->rowCount() > 1)
            scheduleTable->removeRow(scheduleTable->rowCount() - 1);
    }

    cellLabels.clear();
    selectedTimeRow = -1;

    updateAdaptiveCellFonts();
}

void MainWindow::applyRowLayout()
{
    if (!scheduleTable)
        return;

    scheduleTable->clearSpans();

    const MainWindow::BaseScheduleEntry *base = activeBaseSchedule();
    if (!base)
        return;

    const int dataRows = scheduleTable->rowCount() - 1;
    const int spanColumns = scheduleTable->columnCount() - 1;
    if (spanColumns <= 0)
        return;

    for (int row = 0; row < dataRows; ++row)
    {
        if (row < base->rowDefinitions.size() &&
            base->rowDefinitions.at(row).kind == MainWindow::RowDefinition::Kind::Placeholder)
        {
            scheduleTable->setSpan(row + 1, 1, 1, spanColumns);
        }
    }
}

void MainWindow::updateAdaptiveCellFonts()
{
    if (!scheduleTable)
        return;

    if (scheduleHeaderBaseFont.pointSizeF() <= 0)
    {
        scheduleHeaderBaseFont = scheduleTable->font();
        if (scheduleHeaderBaseFont.pointSizeF() <= 0)
        {
            const int pixelSize = scheduleHeaderBaseFont.pixelSize();
            scheduleHeaderBaseFont.setPointSizeF(pixelSize > 0 ? pixelSize : 12.0);
        }
    }

    const int columns = scheduleTable->columnCount();
    const int rows = scheduleTable->rowCount();
    if (rows <= 0 || columns <= 0)
        return;

    for (int col = 0; col < columns; ++col)
        applyAdaptiveFontToItem(scheduleTable->item(0, col), 0, col);

    for (int row = 1; row < rows; ++row)
        applyAdaptiveFontToItem(scheduleTable->item(row, 0), row, 0);
}

void MainWindow::applyAdaptiveFontToItem(QTableWidgetItem *item, int row, int column)
{
    if (!scheduleTable || !item || row < 0 || column < 0)
        return;

    const int columnWidth = qMax(0, scheduleTable->columnWidth(column) - 8);
    const int rowHeight = qMax(0, scheduleTable->rowHeight(row) - 6);
    if (columnWidth <= 0 || rowHeight <= 0)
        return;

    QFont workingFont = scheduleHeaderBaseFont;
    const qreal baseSize = scheduleHeaderBaseFont.pointSizeF();
    if (baseSize <= 0)
        return;

    const QString textValue = item->text();
    if (textValue.isEmpty())
    {
        item->setFont(workingFont);
        return;
    }

    const QStringList lines = textValue.split('\n', Qt::KeepEmptyParts);
    QFontMetricsF metrics(workingFont);
    qreal maxWidth = 0.0;
    for (const QString &line : lines)
        maxWidth = qMax(maxWidth, metrics.horizontalAdvance(line));
    if (maxWidth <= 0)
        maxWidth = 1;

    const int lineCount = qMax(1, lines.size());
    const qreal totalHeight = metrics.lineSpacing() * lineCount;
    if (totalHeight <= 0)
        return;

    const qreal widthRatio = static_cast<qreal>(columnWidth) / maxWidth;
    const qreal heightRatio = static_cast<qreal>(rowHeight) / totalHeight;
    qreal ratio = qMin(widthRatio, heightRatio);
    if (ratio <= 0)
        ratio = 0.1;

    workingFont.setPointSizeF(baseSize * ratio);
    item->setFont(workingFont);
}

void MainWindow::initializeQuickCoursePalette()
{
    courseColorMap.clear();
    courseColorMap.insert(QString::fromUtf8("语文"), QColor("#f87171"));
    courseColorMap.insert(QString::fromUtf8("数学"), QColor("#60a5fa"));
    courseColorMap.insert(QString::fromUtf8("英语"), QColor("#34d399"));
    courseColorMap.insert(QString::fromUtf8("物理"), QColor("#fbbf24"));
    courseColorMap.insert(QString::fromUtf8("化学"), QColor("#a78bfa"));
    courseColorMap.insert(QString::fromUtf8("生物"), QColor("#4ade80"));
    courseColorMap.insert(QString::fromUtf8("历史"), QColor("#f472b6"));
    courseColorMap.insert(QString::fromUtf8("地理"), QColor("#38bdf8"));
    courseColorMap.insert(QString::fromUtf8("政治"), QColor("#facc15"));
    courseColorMap.insert(QString::fromUtf8("体育"), QColor("#fb923c"));

    const MainWindow::BaseScheduleEntry *base = activeBaseSchedule();
    if (base)
    {
        for (const QVector<MainWindow::CourseCellData> &row : base->courseCells)
        {
            for (const MainWindow::CourseCellData &cell : row)
            {
                if (!cell.name.trimmed().isEmpty() && cell.background.isValid())
                {
                    courseColorMap.insert(cell.name, cell.background);
                }
            }
        }
    }

    updateQuickCourseButtons();
}

void MainWindow::updateQuickCourseButtons()
{
    if (!quickCourseLayout)
        return;

    activePaletteButton = nullptr;

    QLayoutItem *child;
    while ((child = quickCourseLayout->takeAt(0)) != nullptr)
    {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }

    int index = 0;
    QPushButton *buttonToSelect = nullptr;
    for (auto it = courseColorMap.constBegin(); it != courseColorMap.constEnd() && index < 8; ++it, ++index)
    {
        QPushButton *btn = new QPushButton(it.key(), quickCourseWidget);
        const QString baseStyle = QString("QPushButton{background:%1; border-radius:6px; padding:6px;}")
                                      .arg(it.value().isValid() ? it.value().name(QColor::HexRgb) : QString("#dff1f1"));
        btn->setCheckable(true);
        btn->setProperty("baseStyle", baseStyle);
        btn->setStyleSheet(baseStyle);
        btn->setProperty("courseName", it.key());
        btn->setProperty("courseColor", it.value());
        btn->setToolTip(QString::fromUtf8("\xe7\x82\xb9\xe5\x87\xbb\xe5\x8d\xb3\xe5\x8f\xaf\xe5\xbf\xab\xe9\x80\x9f\xe6\x8f\x92\xe5\x85\xa5\xe5\x90\x8c\xe5\x90\x8d\xe8\xaf\xbe\xe7\xa8\x8b"));
        connect(btn, &QPushButton::clicked, this, [this, btn]() {
            selectQuickCourseButton(btn);
        });
        btn->installEventFilter(this);
        quickCourseLayout->addWidget(btn, index / 3, index % 3);

        if (!activePaletteCourse.isEmpty() && activePaletteCourse == it.key())
            buttonToSelect = btn;
    }

    clearPaletteButton = new QPushButton(QString::fromUtf8("\xe6\xb8\x85\xe9\x99\xa4"), quickCourseWidget);
    clearPaletteButton->setToolTip(QString::fromUtf8("\xe7\x82\xb9\xe5\x87\xbb\xe6\xb8\x85\xe9\x99\xa4\xe5\xbd\x93\xe5\x89\x8d\xe8\xaf\xbe\xe7\xa8\x8b\xe6\xa0\xbc"));
    clearPaletteButton->setAccessibleName(QString::fromUtf8("\xe6\xb8\x85\xe9\x99\xa4\xe5\xbd\x93\xe5\x89\x8d\xe8\xaf\xbe\xe7\xa8\x8b"));
    const QString clearStyle = QStringLiteral("QPushButton{background:#fee2e2;border:1px solid #fca5a5;border-radius:6px;padding:6px;color:#b91c1c;font-weight:600;} QPushButton:hover{background:#fecaca;}");
    clearPaletteButton->setProperty("baseStyle", clearStyle);
    clearPaletteButton->setStyleSheet(clearStyle);
    connect(clearPaletteButton, &QPushButton::clicked, this, [this]() {
        if (!scheduleTable)
            return;
        const int row = scheduleTable->currentRow();
        const int column = scheduleTable->currentColumn();
        if (row <= 0 || column <= 0)
            return;
        if (applyCourseData(row - 1, column - 1, MainWindow::CourseCellData::empty()))
        {
            refreshScheduleCells();
            scheduleTable->setCurrentCell(row, column);
        }
    });
    clearPaletteButton->installEventFilter(this);
    clearPaletteButton->setToolTip(QString::fromUtf8("\xe7\x82\xb9\xe5\x87\xbb\xe6\xb8\x85\xe9\x99\xa4\xe5\xbd\x93\xe5\x89\x8d\xe8\xaf\xbe\xe7\xa8\x8b\xe6\xa0\xbc"));
    quickCourseLayout->addWidget(clearPaletteButton, 2, 2);

    quickCourseLayout->setColumnStretch(3, 1);

    if (buttonToSelect)
        selectQuickCourseButton(buttonToSelect);
    else if (!activePaletteCourse.isEmpty())
        selectQuickCourseButton(nullptr);
}

const MainWindow::BaseScheduleEntry *MainWindow::activeBaseSchedule() const
{
    if (forcedBaseScheduleIndex >= 0 && forcedBaseScheduleIndex < baseSchedules.size())
        return &baseSchedules.at(forcedBaseScheduleIndex);
    const QDate weekEnd = currentWeekStart.addDays(6);
    for (const MainWindow::BaseScheduleEntry &entry : baseSchedules)
    {
        if (entry.startDate <= currentWeekStart && entry.endDate >= weekEnd)
            return &entry;
    }
    return nullptr;
}

const MainWindow::ActualWeekEntry *MainWindow::findActualWeek(const QDate &weekStart) const
{
    for (const MainWindow::ActualWeekEntry &entry : actualWeekCourses)
    {
        if (entry.weekStart == weekStart)
            return &entry;
    }
    return nullptr;
}

MainWindow::BaseScheduleEntry *MainWindow::activeBaseScheduleMutable()
{
    if (forcedBaseScheduleIndex >= 0 && forcedBaseScheduleIndex < baseSchedules.size())
        return &baseSchedules[forcedBaseScheduleIndex];
    const QDate weekEnd = currentWeekStart.addDays(6);
    for (MainWindow::BaseScheduleEntry &entry : baseSchedules)
    {
        if (entry.startDate <= currentWeekStart && entry.endDate >= weekEnd)
            return &entry;
    }
    return nullptr;
}

MainWindow::ActualWeekEntry *MainWindow::findActualWeekMutable(const QDate &weekStart)
{
    for (MainWindow::ActualWeekEntry &entry : actualWeekCourses)
    {
        if (entry.weekStart == weekStart)
            return &entry;
    }
    return nullptr;
}

void MainWindow::ensureActualWeekExists(const QDate &weekStart, bool createIfMissing)
{
    if (!createIfMissing)
        return;
    QDate normalized = weekStart.isValid() ? mondayOf(weekStart) : mondayOf(QDate::currentDate());
    if (findActualWeekMutable(normalized))
        return;
    MainWindow::ActualWeekEntry created;
    created.weekStart = normalized;
    actualWeekCourses.append(created);
}

void MainWindow::adjustSecondaryContainerHeight()
{
    if (!secondaryContainer)
        return;

    if (!secondaryContainer->isVisible())
    {
        secondaryContainer->setMinimumHeight(0);
        secondaryContainer->setMaximumHeight(QWIDGETSIZE_MAX);
        return;
    }

    QWidget *currentPanel = secondaryStack ? secondaryStack->currentWidget() : nullptr;
    if (!currentPanel)
        return;

    int panelHeight = currentPanel->sizeHint().height();
    int margins = 0;
    if (secondaryContainer->layout())
    {
        const QMargins containerMargins = secondaryContainer->layout()->contentsMargins();
        margins = containerMargins.top() + containerMargins.bottom();
    }

    const int totalHeight = panelHeight + margins;
    secondaryContainer->setMinimumHeight(totalHeight);
    secondaryContainer->setMaximumHeight(totalHeight);
}

bool MainWindow::isConfigModeActive() const
{
    return activePanel == SecondaryPanel::Config && secondaryContainer && secondaryContainer->isVisible();
}

bool MainWindow::isTemporaryAdjustModeActive() const
{
    return activePanel == SecondaryPanel::TemporaryAdjust && secondaryContainer && secondaryContainer->isVisible();
}

void MainWindow::updateWeeklyStatistics()
{
    if (!weeklyStatsLayout || !weeklyStatsPlaceholder)
        return;

    if (!isTemporaryAdjustModeActive())
    {
        weeklyStatsPlaceholder->setVisible(true);
        return;
    }

    while (QLayoutItem *item = weeklyStatsLayout->takeAt(0))
    {
        if (QWidget *widget = item->widget())
            widget->deleteLater();
        delete item;
    }

    struct CourseStat
    {
        int standard = 0;
        int actual = 0;
        QColor color;
    };

    QMap<QString, CourseStat> stats;

    const BaseScheduleEntry *base = activeBaseSchedule();
    if (base)
    {
        for (int row = 0; row < base->courseCells.size(); ++row)
        {
            const bool isPlaceholderRow = row < base->rowDefinitions.size() &&
                                          base->rowDefinitions.at(row).kind == RowDefinition::Kind::Placeholder;
            if (isPlaceholderRow)
                continue;
            const QVector<CourseCellData> &rowData = base->courseCells.at(row);
            for (int col = 0; col < rowData.size(); ++col)
            {
                const CourseCellData &cell = rowData.at(col);
                const QString courseName = cell.name.trimmed();
                if (courseName.isEmpty())
                    continue;
                CourseStat &stat = stats[courseName];
                stat.standard++;
                stat.actual++;
                if (!stat.color.isValid() && cell.background.isValid())
                    stat.color = cell.background;
            }
        }
    }

    auto decrementActual = [&](const QString &name) {
        const QString trimmed = name.trimmed();
        if (trimmed.isEmpty())
            return;
        CourseStat &stat = stats[trimmed];
        stat.actual = qMax(0, stat.actual - 1);
    };

    auto incrementActual = [&](const CourseCellData &cell) {
        const QString trimmed = cell.name.trimmed();
        if (trimmed.isEmpty())
            return;
        CourseStat &stat = stats[trimmed];
        stat.actual++;
        if (!stat.color.isValid() && cell.background.isValid())
            stat.color = cell.background;
    };

    if (const ActualWeekEntry *entry = findActualWeek(currentWeekStart))
    {
        for (auto rowIt = entry->cellOverrides.constBegin(); rowIt != entry->cellOverrides.constEnd(); ++rowIt)
        {
            const int row = rowIt.key();
            if (base && row < base->rowDefinitions.size() &&
                base->rowDefinitions.at(row).kind == RowDefinition::Kind::Placeholder)
                continue;
            for (auto colIt = rowIt.value().constBegin(); colIt != rowIt.value().constEnd(); ++colIt)
            {
                const int column = colIt.key();
                const CourseCellData &overrideCell = colIt.value();
                const QString baseName = baseCellDataAt(row, column).name;

                switch (overrideCell.type)
                {
                case ScheduleCellLabel::CourseType::Deleted:
                    decrementActual(baseName);
                    break;
                case ScheduleCellLabel::CourseType::Temporary:
                    incrementActual(overrideCell);
                    break;
                case ScheduleCellLabel::CourseType::DeletedTemporary:
                    decrementActual(baseName);
                    incrementActual(overrideCell);
                    break;
                default:
                    break;
                }
            }
        }
    }

    bool hasStats = false;
    for (auto it = stats.constBegin(); it != stats.constEnd(); ++it)
    {
        if (it.value().standard > 0 || it.value().actual > 0)
        {
            hasStats = true;
            break;
        }
    }

    weeklyStatsPlaceholder->setVisible(!hasStats);
    if (!hasStats)
        return;

    int index = 0;
    for (auto it = stats.constBegin(); it != stats.constEnd(); ++it)
    {
        const CourseStat &stat = it.value();
        if (stat.standard <= 0 && stat.actual <= 0)
            continue;

        WeeklyStatsCard *card = new WeeklyStatsCard(it.key(), stat.color, stat.standard, stat.actual, weeklyStatsContainer);
        weeklyStatsLayout->insertWidget(index, card, 0, Qt::AlignLeft | Qt::AlignTop);
        ++index;
    }

    weeklyStatsPlaceholder->setVisible(index == 0);
}

bool MainWindow::isBaseIndexEditable(int index) const
{
    return index >= 0 && index == unsavedBaseScheduleIndex && index < baseSchedules.size();
}

bool MainWindow::isSelectedBaseEditable() const
{
    return isBaseIndexEditable(configSelectedBaseScheduleIndex);
}

bool MainWindow::canEditSelectedBaseSchedule() const
{
    return isConfigModeActive() && isSelectedBaseEditable();
}

QVector<int> MainWindow::overlappingBaseScheduleIndexes(int candidateIndex) const
{
    QVector<int> indexes;
    if (candidateIndex < 0 || candidateIndex >= baseSchedules.size())
        return indexes;

    const BaseScheduleEntry &candidate = baseSchedules.at(candidateIndex);
    if (!candidate.startDate.isValid() || !candidate.endDate.isValid())
        return indexes;

    for (int i = 0; i < baseSchedules.size(); ++i)
    {
        if (i == candidateIndex)
            continue;
        const BaseScheduleEntry &other = baseSchedules.at(i);
        if (!other.startDate.isValid() || !other.endDate.isValid())
            continue;
        if (candidate.endDate < other.startDate || other.endDate < candidate.startDate)
            continue;
        indexes.append(i);
    }
    std::sort(indexes.begin(), indexes.end());
    return indexes;
}

QString MainWindow::buildOverlapWarning(const BaseScheduleEntry &candidate, const QVector<int> &overlaps) const
{
    QString baseText = QString::fromUtf8("新课程表 (%1 至 %2) 与以下课程表的适用日期重叠，将自动截断旧课程表的重叠部分：\n")
                           .arg(candidate.startDate.toString("yyyy-MM-dd"),
                                candidate.endDate.toString("yyyy-MM-dd"));
    QStringList rows;
    for (int index : overlaps)
    {
        if (index < 0 || index >= baseSchedules.size())
            continue;
        const BaseScheduleEntry &entry = baseSchedules.at(index);
        rows << QString::fromUtf8("• 课程表%1：%2 至 %3")
                    .arg(index + 1)
                    .arg(entry.startDate.toString("yyyy-MM-dd"),
                         entry.endDate.toString("yyyy-MM-dd"));
    }
    if (!rows.isEmpty())
        baseText += rows.join('\n');
    baseText += QString::fromUtf8("\n\n点击“确定”后，将调整旧课程表并保存新课程表，是否继续？");
    return baseText;
}

void MainWindow::adjustScheduleListForOverlap(QVector<BaseScheduleEntry> &list, int &newIndexRef, const BaseScheduleEntry &candidate)
{
    if (list.isEmpty())
        return;

    for (int i = list.size() - 1; i >= 0; --i)
    {
        if (newIndexRef >= 0 && i == newIndexRef)
            continue;
        BaseScheduleEntry &existing = list[i];
        if (!existing.startDate.isValid() || !existing.endDate.isValid())
            continue;
        if (existing.endDate < candidate.startDate || candidate.endDate < existing.startDate)
            continue;

        const bool coversStart = candidate.startDate <= existing.startDate;
        const bool coversEnd = candidate.endDate >= existing.endDate;

        if (coversStart && coversEnd)
        {
            list.removeAt(i);
            if (newIndexRef >= 0 && i < newIndexRef)
                --newIndexRef;
            continue;
        }

        if (coversStart)
        {
            existing.startDate = candidate.endDate.addDays(1);
            if (existing.startDate > existing.endDate)
            {
                list.removeAt(i);
                if (newIndexRef >= 0 && i < newIndexRef)
                    --newIndexRef;
            }
            continue;
        }

        if (coversEnd)
        {
            existing.endDate = candidate.startDate.addDays(-1);
            if (existing.startDate > existing.endDate)
            {
                list.removeAt(i);
                if (newIndexRef >= 0 && i < newIndexRef)
                    --newIndexRef;
            }
            continue;
        }

        BaseScheduleEntry tail = existing;
        existing.endDate = candidate.startDate.addDays(-1);
        tail.startDate = candidate.endDate.addDays(1);
        list.insert(i + 1, tail);
        if (newIndexRef >= 0 && i + 1 <= newIndexRef)
            ++newIndexRef;
    }
}

void MainWindow::resolveOverlapsForNewSchedule(int &newIndex)
{
    if (newIndex < 0 || newIndex >= baseSchedules.size())
        return;

    const BaseScheduleEntry candidate = baseSchedules.at(newIndex);
    adjustScheduleListForOverlap(baseSchedules, newIndex, candidate);
    unsavedBaseScheduleIndex = newIndex;
    configSelectedBaseScheduleIndex = newIndex;
    forcedBaseScheduleIndex = newIndex;
}

bool MainWindow::baseCellHasCourse(int row, int column) const
{
    CourseCellData cell = baseCellDataAt(row, column);
    return cell.type != ScheduleCellLabel::CourseType::None || !cell.name.trimmed().isEmpty();
}

MainWindow::CourseCellData MainWindow::baseCellDataAt(int row, int column) const
{
    const BaseScheduleEntry *base = activeBaseSchedule();
    if (!base)
        return CourseCellData::empty();
    if (row < 0 || row >= base->courseCells.size())
        return CourseCellData::empty();
    if (column < 0 || column >= base->courseCells.at(row).size())
        return CourseCellData::empty();
    return base->courseCells.at(row).at(column);
}

void MainWindow::setActualOverride(int row, int column, const CourseCellData &data)
{
    if (row < 0 || column < 0)
        return;
    const QDate weekKey = mondayOf(currentWeekStart);
    int entryIndex = -1;
    for (int i = 0; i < actualWeekCourses.size(); ++i)
    {
        if (actualWeekCourses.at(i).weekStart == weekKey)
        {
            entryIndex = i;
            break;
        }
    }

    if (entryIndex < 0)
    {
        ensureActualWeekExists(weekKey, true);
        entryIndex = actualWeekCourses.size() - 1;
    }

    if (entryIndex < 0 || entryIndex >= actualWeekCourses.size())
        return;

    MainWindow::ActualWeekEntry &entry = actualWeekCourses[entryIndex];

    auto removeOverride = [&]() {
        if (entry.cellOverrides.contains(row))
        {
            entry.cellOverrides[row].remove(column);
            if (entry.cellOverrides[row].isEmpty())
                entry.cellOverrides.remove(row);
        }
    };

    if (data.type == ScheduleCellLabel::CourseType::None && data.name.trimmed().isEmpty())
    {
        removeOverride();
    }
    else
    {
        entry.cellOverrides[row][column] = data;
    }

    if (entry.cellOverrides.isEmpty())
    {
        actualWeekCourses.removeAt(entryIndex);
    }

    saveActualScheduleToDisk();
}

MainWindow::CourseCellData MainWindow::actualOverrideAt(int row, int column) const
{
    const ActualWeekEntry *entry = findActualWeek(currentWeekStart);
    if (!entry)
        return CourseCellData::empty();
    auto rowIt = entry->cellOverrides.constFind(row);
    if (rowIt == entry->cellOverrides.constEnd())
        return CourseCellData::empty();
    auto colIt = rowIt->constFind(column);
    if (colIt == rowIt->constEnd())
        return CourseCellData::empty();
    return colIt.value();
}

bool MainWindow::activeBaseContainsCourseName(const QString &name) const
{
    QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
        return false;
    const BaseScheduleEntry *base = activeBaseSchedule();
    if (!base)
        return false;
    for (const QVector<CourseCellData> &row : base->courseCells)
    {
        for (const CourseCellData &cell : row)
        {
            if (cell.name.trimmed() == trimmed)
                return true;
        }
    }
    return false;
}

QString MainWindow::generateUniqueCustomCourseName(const QString &seed) const
{
    QString desired = seed.trimmed();
    if (desired.isEmpty())
        desired = QString::fromUtf8("临时课程");
    if (!activeBaseContainsCourseName(desired))
        return desired;

    QString baseName = desired;
    int index = 1;
    QString candidate;
    do
    {
        ++index;
        candidate = QStringLiteral("%1(%2)").arg(baseName).arg(index);
    } while (activeBaseContainsCourseName(candidate));
    return candidate;
}

void MainWindow::ensureCustomTemporaryCourseNameUnique()
{
    QString unique = generateUniqueCustomCourseName(customTemporaryCourseName);
    if (unique == customTemporaryCourseName)
        return;
    customTemporaryCourseName = unique;
    if (customTemporaryCourseLabel)
        updateCustomTemporaryCourseLabel();
    saveActualScheduleToDisk();
}


void MainWindow::handleAddLessonRow()
{
    if (!canEditSelectedBaseSchedule() || !scheduleTable)
        return;

    if (selectedTimeRow < 0 || selectedTimeRow >= scheduleTable->rowCount())
        return;

    MainWindow::BaseScheduleEntry *baseEntry = activeBaseScheduleMutable();
    if (!baseEntry)
        return;

    const int insertRow = selectedTimeRow + 1;
    scheduleTable->insertRow(insertRow);

    for (int col = 0; col < scheduleTable->columnCount(); ++col)
    {
        auto *item = new QTableWidgetItem();
        item->setTextAlignment(Qt::AlignCenter);
        scheduleTable->setItem(insertRow, col, item);
    }

    const QString referenceText = scheduleTable->item(selectedTimeRow, 0) ? scheduleTable->item(selectedTimeRow, 0)->text() : QString();
    scheduleTable->item(insertRow, 0)->setText(referenceText);

    const int dataIndex = insertRow - 1;
    if (dataIndex >= 0 && dataIndex <= baseEntry->timeColumnTexts.size())
        baseEntry->timeColumnTexts.insert(dataIndex, referenceText);
    if (dataIndex >= 0 && dataIndex <= baseEntry->rowDefinitions.size())
        baseEntry->rowDefinitions.insert(dataIndex, MainWindow::RowDefinition{MainWindow::RowDefinition::Kind::Lesson});

    QVector<MainWindow::CourseCellData> blankRow(baseEntry->columns, MainWindow::CourseCellData::empty());
    if (dataIndex >= 0 && dataIndex <= baseEntry->courseCells.size())
        baseEntry->courseCells.insert(dataIndex, blankRow);
    baseEntry->rows = baseEntry->courseCells.size();

    ensureCellWidgets();
    refreshScheduleCells();
    refreshTimeColumn();

    int rowHeight = scheduleTable->verticalHeader()->defaultSectionSize();
    if (rowHeight <= 0)
        rowHeight = scheduleTable->rowHeight(insertRow);
    if (rowHeight > 0)
        resize(width(), height() + rowHeight);

    selectedTimeRow = insertRow;
    markBaseScheduleDirty();
    refreshBaseScheduleList();
    scheduleTable->setCurrentCell(insertRow, 0);
}

void MainWindow::handleAddMergedRow()
{
    if (!canEditSelectedBaseSchedule() || !scheduleTable)
        return;

    if (selectedTimeRow < 0 || selectedTimeRow >= scheduleTable->rowCount())
        return;

    MainWindow::BaseScheduleEntry *baseEntry = activeBaseScheduleMutable();
    if (!baseEntry)
        return;

    const int insertRow = selectedTimeRow + 1;
    scheduleTable->insertRow(insertRow);

    for (int col = 0; col < scheduleTable->columnCount(); ++col)
    {
        auto *item = new QTableWidgetItem();
        item->setTextAlignment(Qt::AlignCenter);
        scheduleTable->setItem(insertRow, col, item);
    }

    const QString referenceText = scheduleTable->item(selectedTimeRow, 0) ? scheduleTable->item(selectedTimeRow, 0)->text() : QString();
    if (auto *timeItem = scheduleTable->item(insertRow, 0))
        timeItem->setText(referenceText);

    for (int col = 1; col < scheduleTable->columnCount(); ++col)
    {
        if (auto *cellItem = scheduleTable->item(insertRow, col))
            cellItem->setText("");
    }

    const int dataIndex = insertRow - 1;
    if (dataIndex >= 0 && dataIndex <= baseEntry->timeColumnTexts.size())
        baseEntry->timeColumnTexts.insert(dataIndex, referenceText);
    else
        baseEntry->timeColumnTexts.append(referenceText);

    MainWindow::RowDefinition placeholderDef;
    placeholderDef.kind = MainWindow::RowDefinition::Kind::Placeholder;
    if (dataIndex >= 0 && dataIndex <= baseEntry->rowDefinitions.size())
        baseEntry->rowDefinitions.insert(dataIndex, placeholderDef);
    else
        baseEntry->rowDefinitions.append(placeholderDef);

    QVector<MainWindow::CourseCellData> blankRow(baseEntry->columns, MainWindow::CourseCellData::empty());
    if (dataIndex >= 0 && dataIndex <= baseEntry->courseCells.size())
        baseEntry->courseCells.insert(dataIndex, blankRow);
    else
        baseEntry->courseCells.append(blankRow);
    baseEntry->rows = baseEntry->courseCells.size();

    ensureCellWidgets();
    refreshScheduleCells();
    refreshTimeColumn();

    int rowHeight = scheduleTable->verticalHeader()->defaultSectionSize();
    if (rowHeight <= 0)
        rowHeight = scheduleTable->rowHeight(insertRow);
    if (rowHeight > 0)
        resize(width(), height() + rowHeight);

    selectedTimeRow = insertRow;
    markBaseScheduleDirty();
    refreshBaseScheduleList();
    scheduleTable->setCurrentCell(insertRow, 0);
}

void MainWindow::handleDeleteRow()
{
    if (!canEditSelectedBaseSchedule() || !scheduleTable)
        return;

    if (selectedTimeRow <= 0 || selectedTimeRow >= scheduleTable->rowCount())
        return;

    MainWindow::BaseScheduleEntry *baseEntry = activeBaseScheduleMutable();
    if (!baseEntry)
        return;

    const int removeRow = selectedTimeRow;
    const int dataIndex = removeRow - 1;

    int rowHeight = scheduleTable->rowHeight(removeRow);
    if (rowHeight <= 0)
        rowHeight = scheduleTable->verticalHeader()->defaultSectionSize();

    scheduleTable->removeRow(removeRow);

    if (dataIndex >= 0 && dataIndex < baseEntry->timeColumnTexts.size())
        baseEntry->timeColumnTexts.removeAt(dataIndex);
    if (dataIndex >= 0 && dataIndex < baseEntry->rowDefinitions.size())
        baseEntry->rowDefinitions.removeAt(dataIndex);
    if (dataIndex >= 0 && dataIndex < baseEntry->courseCells.size())
        baseEntry->courseCells.removeAt(dataIndex);
    baseEntry->rows = baseEntry->courseCells.size();

    ensureCellWidgets();
    refreshScheduleCells();
    refreshTimeColumn();

    if (rowHeight <= 0 && removeRow < scheduleTable->rowCount())
        rowHeight = scheduleTable->rowHeight(removeRow);
    if (rowHeight > 0)
        resize(width(), height() - rowHeight);

    selectedTimeRow = -1;
    markBaseScheduleDirty();
    refreshBaseScheduleList();
    scheduleTable->clearSelection();
}

void MainWindow::handleCreateBaseSchedule()
{
    if (unsavedBaseScheduleIndex >= 0)
    {
        QMessageBox::information(this,
                                 QString::fromUtf8("\xe6\x8f\x90\xe7\xa4\xba"),
                                 QString::fromUtf8("\xe5\xbd\x93\xe5\x89\x8d\xe5\xb7\xb2\xe6\x9c\x89\xe6\x9c\xaa\xe4\xbf\x9d\xe5\xad\x98\xe7\x9a\x84\xe5\x9f\xba\xe7\xa1\x80\xe8\xaf\xbe\xe8\xa1\xa8\xef\xbc\x8c\xe8\xaf\xb7\xe4\xbb\x85\xe4\xbf\x9d\xe5\xad\x98\xe6\x88\x96\xe6\x94\xbe\xe5\xbc\x83\xe5\x90\x8e\xe5\x86\x8d\xe6\x96\xb0\xe5\xa2\x9e"));
        return;
    }

    BaseScheduleEntry templateEntry;
    if (const BaseScheduleEntry *current = activeBaseSchedule())
    {
        templateEntry = *current;
    }
    else
    {
        templateEntry.rows = lessonTimes.size();
        templateEntry.columns = scheduleTable ? scheduleTable->columnCount() - 1 : 7;
        templateEntry.rowDefinitions = QVector<RowDefinition>(templateEntry.rows, RowDefinition{RowDefinition::Kind::Lesson});
        templateEntry.courseCells = QVector<QVector<CourseCellData>>(templateEntry.rows, QVector<CourseCellData>(templateEntry.columns, CourseCellData::empty()));
        templateEntry.timeColumnTexts.clear();
        for (int i = 0; i < templateEntry.rows && i < lessonTimes.size(); ++i)
        {
            QString periodText = QString::fromUtf8("\xe7\xac\xac") + QString::number(i + 1) + QString::fromUtf8("\xe8\x8a\x82");
            templateEntry.timeColumnTexts << QString("%1\n%2").arg(periodText, lessonTimes.at(i));
        }
    }

    templateEntry.startDate = ensureStartMonday(currentWeekStart);
    templateEntry.endDate = ensureEndSunday(templateEntry.startDate, templateEntry.startDate.addDays(6));

    baseSchedules.append(templateEntry);
    configSelectedBaseScheduleIndex = baseSchedules.size() - 1;
    setForcedBaseScheduleIndex(configSelectedBaseScheduleIndex);
    editingNewBaseSchedule = true;
    unsavedBaseScheduleIndex = configSelectedBaseScheduleIndex;
    markBaseScheduleDirty();
    refreshBaseScheduleList();
    ensureCellWidgets();
    refreshTimeColumn();
    refreshScheduleCells();
    applyRowLayout();
    initializeQuickCoursePalette();
    updateEffectiveDateEditors();
    updateToolbarInteractivity();
}

void MainWindow::editCourseCell(int row, int column)
{
    if (!canEditSelectedBaseSchedule() || !scheduleTable)
        return;

    MainWindow::BaseScheduleEntry *baseEntry = activeBaseScheduleMutable();
    if (!baseEntry)
        return;

    const int dataRow = row - 1;
    const int dataColumn = column - 1;
    if (dataRow < 0 || dataRow >= baseEntry->courseCells.size() ||
        dataColumn < 0 || dataColumn >= baseEntry->columns)
        return;

    bool isPlaceholderRow = dataRow < baseEntry->rowDefinitions.size() &&
                            baseEntry->rowDefinitions.at(dataRow).kind == MainWindow::RowDefinition::Kind::Placeholder;
    int effectiveColumn = dataColumn;
    if (isPlaceholderRow)
        effectiveColumn = 0;
    if (effectiveColumn < 0)
        effectiveColumn = 0;
    QVector<CourseCellData> &editableRow = baseEntry->courseCells[dataRow];
    if (editableRow.size() < baseEntry->columns)
        editableRow.resize(baseEntry->columns);
    const MainWindow::CourseCellData currentData = editableRow.value(effectiveColumn, MainWindow::CourseCellData::empty());

    CourseEditDialog dialog(this);
    dialog.setCourseName(currentData.name);
    dialog.setClassName(currentData.className);
    QColor initialColor = currentData.background;
    if (!initialColor.isValid() && courseColorMap.contains(currentData.name))
        initialColor = courseColorMap.value(currentData.name);
    dialog.setCourseColor(initialColor);

    if (dialog.exec() != QDialog::Accepted)
        return;

    if (dialog.removeRequested())
    {
        if (applyCourseData(dataRow, dataColumn, MainWindow::CourseCellData::empty()))
            refreshScheduleCells();
        return;
    }

    QString finalName = dialog.courseName();
    QString finalClassName = dialog.className();
    QColor finalColor = dialog.courseColor();
    if (!finalColor.isValid() && courseColorMap.contains(finalName))
        finalColor = courseColorMap.value(finalName);
    if (!finalColor.isValid())
        finalColor = QColor("#dff1f1");

    MainWindow::CourseCellData updated = MainWindow::CourseCellData::create(finalName, finalColor, ScheduleCellLabel::CourseType::Normal, finalClassName);
    if (applyCourseData(dataRow, dataColumn, updated))
    {
        recordCourseTemplate(finalName, finalColor);
        refreshScheduleCells();
    }
}

bool MainWindow::applyCourseData(int dataRow, int dataColumn, const MainWindow::CourseCellData &data)
{
    if (!canEditSelectedBaseSchedule())
        return false;
    MainWindow::BaseScheduleEntry *baseEntry = activeBaseScheduleMutable();
    if (!baseEntry)
        return false;
    if (dataRow < 0 || dataRow >= baseEntry->courseCells.size())
        return false;
    if (baseEntry->columns <= 0)
        return false;
    const bool isPlaceholderRow = dataRow < baseEntry->rowDefinitions.size() &&
                                  baseEntry->rowDefinitions.at(dataRow).kind == MainWindow::RowDefinition::Kind::Placeholder;
    if (isPlaceholderRow)
        dataColumn = 0;
    if (dataColumn < 0)
        dataColumn = 0;
    if (dataColumn >= baseEntry->columns)
        dataColumn = baseEntry->columns - 1;

    QVector<CourseCellData> &rowVec = baseEntry->courseCells[dataRow];
    if (rowVec.size() < baseEntry->columns)
        rowVec.resize(baseEntry->columns);

    auto assignCell = [&](QVector<CourseCellData> &targetRow, int column) -> bool {
        if (column < 0 || column >= targetRow.size())
            return false;
        CourseCellData &cell = targetRow[column];
        const bool diff = cell.name != data.name || cell.className != data.className || cell.background != data.background || cell.type != data.type;
        if (diff)
            cell = data;
        return diff;
    };

    bool changed = false;
    if (isPlaceholderRow)
    {
        for (int col = 0; col < baseEntry->columns; ++col)
        {
            if (assignCell(rowVec, col))
                changed = true;
        }
    }
    else
    {
        changed = assignCell(rowVec, dataColumn);
    }

    if (!changed)
        return false;

    markBaseScheduleDirty();
    return true;
}

void MainWindow::recordCourseTemplate(const QString &name, const QColor &color)
{
    if (name.trimmed().isEmpty() || !color.isValid())
        return;
    if (courseColorMap.value(name) == color)
        return;
    courseColorMap.insert(name, color);
    updateQuickCourseButtons();
}

void MainWindow::applyActiveCourseToCell(int row, int column)
{
    if (!canEditSelectedBaseSchedule() || !scheduleTable || activePaletteCourse.trimmed().isEmpty())
        return;

    if (row <= 0 || column <= 0)
        return;

    QColor color = activePaletteColor.isValid() ? activePaletteColor : courseColorMap.value(activePaletteCourse, QColor("#dff1f1"));
    MainWindow::CourseCellData data = MainWindow::CourseCellData::create(activePaletteCourse, color, ScheduleCellLabel::CourseType::Normal);
    if (applyCourseData(row - 1, column - 1, data))
        refreshScheduleCells();
}

void MainWindow::selectQuickCourseButton(QPushButton *button)
{
    auto applyStyle = [](QPushButton *btn, bool selected) {
        if (!btn)
            return;
        QString base = btn->property("baseStyle").toString();
        if (base.isEmpty())
            base = btn->styleSheet();
        QString style;
        if (selected)
        {
            style = base + "border:2px solid rgba(37,99,235,0.8); padding:12px; font-weight:700; background-color:#eef2ff;";
        }
        else
        {
            style = base + "padding:6px;";
        }
        btn->setStyleSheet(style);
        btn->setChecked(selected);

        QGraphicsDropShadowEffect *shadow = qobject_cast<QGraphicsDropShadowEffect *>(btn->graphicsEffect());
        if (!shadow)
        {
            shadow = new QGraphicsDropShadowEffect(btn);
            btn->setGraphicsEffect(shadow);
        }
        if (selected)
        {
            shadow->setEnabled(true);
            shadow->setBlurRadius(28);
            shadow->setOffset(0, 10);
            shadow->setColor(QColor(37, 99, 235, 120));
        }
        else
        {
            shadow->setEnabled(false);
        }
    };

    if (activePaletteButton && activePaletteButton != button)
        applyStyle(activePaletteButton, false);

    if (activePaletteButton == button)
    {
        applyStyle(activePaletteButton, false);
        activePaletteButton = nullptr;
        activePaletteCourse.clear();
        activePaletteColor = QColor();
        return;
    }

    if (!button)
    {
        activePaletteButton = nullptr;
        activePaletteCourse.clear();
        activePaletteColor = QColor();
        return;
    }

    activePaletteButton = button;
    activePaletteCourse = button->property("courseName").toString();
    activePaletteColor = button->property("courseColor").value<QColor>();
    applyStyle(button, true);
}
void MainWindow::handleTableCellClicked(int row, int column)
{
    const bool editable = canEditSelectedBaseSchedule();

    if (column == 0)
    {
        selectedTimeRow = editable ? row : -1;
        clearConflictHighlights();
    }
    else
    {
        selectedTimeRow = -1;
        scheduleTable->setCurrentCell(row, column);
        if (editable && !activePaletteCourse.isEmpty())
            applyActiveCourseToCell(row, column);
        if (conflictDetectionEnabled)
            checkTeacherConflicts(row, column);
    }
}

void MainWindow::handleCourseCellDoubleClicked(int row, int column)
{
    if (!canEditSelectedBaseSchedule())
        return;
    if (row <= 0)
        return;
    if (column == 0)
    {
        editTimeSlot(row);
        return;
    }
    if (column <= 0)
        return;
    editCourseCell(row, column);
}

void MainWindow::handleTemporaryDrop(int sourceRow, int sourceColumn, int targetRow, int targetColumn, const QString &name, const QColor &color, int sourceType)
{
    ScheduleCellLabel::CourseType dragType = static_cast<ScheduleCellLabel::CourseType>(sourceType);
    if (!isTemporaryAdjustModeActive())
        return;
    if (sourceRow < 0 || sourceColumn < 0 || targetRow < 0 || targetColumn < 0)
        return;
    if (sourceRow == targetRow && sourceColumn == targetColumn)
        return;
    if (name.trimmed().isEmpty())
        return;

    CourseCellData targetOverride = actualOverrideAt(targetRow, targetColumn);
    bool targetOverrideExists = targetOverride.type != ScheduleCellLabel::CourseType::None || !targetOverride.name.trimmed().isEmpty();
    bool targetBaseHasCourse = baseCellHasCourse(targetRow, targetColumn);
    if (targetOverrideExists && targetOverride.type == ScheduleCellLabel::CourseType::Deleted)
        targetBaseHasCourse = false;

    bool targetOccupied = false;
    if (targetOverrideExists)
    {
        if (targetOverride.type == ScheduleCellLabel::CourseType::Temporary ||
            targetOverride.type == ScheduleCellLabel::CourseType::DeletedTemporary ||
            targetOverride.type == ScheduleCellLabel::CourseType::Normal)
            targetOccupied = true;
    }
    if (!targetOccupied && targetBaseHasCourse)
        targetOccupied = true;

    const bool requiresEmptyTarget = (dragType == ScheduleCellLabel::CourseType::Normal ||
                                      dragType == ScheduleCellLabel::CourseType::Temporary ||
                                      dragType == ScheduleCellLabel::CourseType::DeletedTemporary);
    if (requiresEmptyTarget && targetOccupied)
    {
        QMessageBox::information(this,
                                 QString::fromUtf8("调课失败"),
                                 QString::fromUtf8("该单元格已有课程，请先删除目标课程后再拖动。"));
        return;
    }

    CourseCellData movedData = CourseCellData::create(name, color.isValid() ? color : QColor("#dff1f1"));
    movedData.type = baseCellHasCourse(targetRow, targetColumn) ? ScheduleCellLabel::CourseType::DeletedTemporary : ScheduleCellLabel::CourseType::Temporary;
    bool restoredTarget = false;
    if (targetOverride.type == ScheduleCellLabel::CourseType::Deleted)
    {
        QString baseName = baseCellDataAt(targetRow, targetColumn).name.trimmed();
        if (!baseName.isEmpty() && baseName == name.trimmed())
        {
            setActualOverride(targetRow, targetColumn, CourseCellData::empty());
            restoredTarget = true;
        }
    }

    if (baseCellHasCourse(sourceRow, sourceColumn))
    {
        CourseCellData deletedMarker = baseCellDataAt(sourceRow, sourceColumn);
        if (deletedMarker.name.trimmed().isEmpty())
            deletedMarker.name = name;
        if (!deletedMarker.background.isValid())
            deletedMarker.background = color;
        deletedMarker.type = ScheduleCellLabel::CourseType::Deleted;
        setActualOverride(sourceRow, sourceColumn, deletedMarker);
    }
    else
    {
        setActualOverride(sourceRow, sourceColumn, CourseCellData::empty());
    }

    if (!restoredTarget)
        setActualOverride(targetRow, targetColumn, movedData);

    refreshScheduleCells();
}

void MainWindow::handleWeeklyCourseDrop(int targetRow, int targetColumn, const QString &name, const QColor &color)
{
    if (!isTemporaryAdjustModeActive())
        return;
    if (targetRow < 0 || targetColumn < 0)
        return;
    if (name.trimmed().isEmpty())
        return;

    CourseCellData overrideData = actualOverrideAt(targetRow, targetColumn);
    const bool targetIsTemporary = overrideData.type == ScheduleCellLabel::CourseType::Temporary ||
                                   overrideData.type == ScheduleCellLabel::CourseType::DeletedTemporary ||
                                   overrideData.type == ScheduleCellLabel::CourseType::Normal;
    const bool targetHasBaseCourse = baseCellHasCourse(targetRow, targetColumn);

    if (targetIsTemporary || (!overrideData.name.trimmed().isEmpty() && overrideData.type == ScheduleCellLabel::CourseType::DeletedTemporary) ||
        (!overrideData.name.trimmed().isEmpty() && overrideData.type == ScheduleCellLabel::CourseType::Temporary) ||
        (overrideData.type == ScheduleCellLabel::CourseType::None && targetHasBaseCourse))
    {
        QMessageBox::information(this,
                                 QString::fromUtf8("添加失败"),
                                 QString::fromUtf8("该单元格已有课程，请先删除该节课程后再添加。"));
        return;
    }

    if (overrideData.type == ScheduleCellLabel::CourseType::Deleted)
    {
        QString deletedName = overrideData.name.trimmed();
        if (deletedName.isEmpty())
            deletedName = baseCellDataAt(targetRow, targetColumn).name.trimmed();

        if (deletedName == name.trimmed())
        {
            setActualOverride(targetRow, targetColumn, CourseCellData::empty());
        }
        else
        {
            CourseCellData data = CourseCellData::create(name, color.isValid() ? color : QColor("#dff1f1"));
            data.type = ScheduleCellLabel::CourseType::DeletedTemporary;
            setActualOverride(targetRow, targetColumn, data);
        }
    }
    else
    {
        CourseCellData data = CourseCellData::create(name, color.isValid() ? color : QColor("#dff1f1"));
        data.type = ScheduleCellLabel::CourseType::Temporary;
        setActualOverride(targetRow, targetColumn, data);
    }

    refreshScheduleCells();
}

void MainWindow::handleWeeklyStatsDeletionDrop(int sourceRow, int sourceColumn, const QString &name, const QColor &color, ScheduleCellLabel::CourseType sourceType, const QPoint &globalPos)
{
    if (!isTemporaryAdjustModeActive())
        return;
    if (sourceRow < 0 || sourceColumn < 0)
        return;

    auto buildDeletedData = [&](const QString &fallbackName, const QColor &fallbackColor) -> CourseCellData {
        CourseCellData data = baseCellDataAt(sourceRow, sourceColumn);
        if (data.name.trimmed().isEmpty())
            data.name = fallbackName;
        if (!data.background.isValid())
            data.background = fallbackColor.isValid() ? fallbackColor : QColor("#f2f2f2");
        data.type = ScheduleCellLabel::CourseType::Deleted;
        return data;
    };

    bool changed = false;
    switch (sourceType)
    {
    case ScheduleCellLabel::CourseType::Normal:
    {
        CourseCellData deletedData = buildDeletedData(name, color);
        setActualOverride(sourceRow, sourceColumn, deletedData);
        changed = true;
        break;
    }
    case ScheduleCellLabel::CourseType::Temporary:
        setActualOverride(sourceRow, sourceColumn, CourseCellData::empty());
        changed = true;
        break;
    case ScheduleCellLabel::CourseType::DeletedTemporary:
    {
        CourseCellData deletedData = buildDeletedData(name, color);
        setActualOverride(sourceRow, sourceColumn, deletedData);
        changed = true;
        break;
    }
    default:
        break;
    }

    if (!changed)
        return;

    playWeeklyStatsDeletionEffect(globalPos);
    refreshScheduleCells();
}

void MainWindow::playWeeklyStatsDeletionEffect(const QPoint &globalPos)
{
    if (!weeklyStatsContainer)
        return;

    QPoint localPos = weeklyStatsContainer->mapFromGlobal(globalPos);
    localPos.setX(qBound(0, localPos.x(), weeklyStatsContainer->width()));
    localPos.setY(qBound(0, localPos.y(), weeklyStatsContainer->height()));

    auto *burst = new QLabel(weeklyStatsContainer);
    burst->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    burst->raise();
    auto *opacityEffect = new QGraphicsOpacityEffect(burst);
    burst->setGraphicsEffect(opacityEffect);
    burst->resize(0, 0);
    burst->show();

    auto *animation = new QVariantAnimation(burst);
    animation->setDuration(420);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    QObject::connect(animation, &QVariantAnimation::valueChanged, burst, [burst, opacityEffect, localPos](const QVariant &value) {
        const qreal progress = value.toReal();
        const int minSize = 32;
        const int maxSize = 90;
        const int size = minSize + static_cast<int>((maxSize - minSize) * progress);
        burst->resize(size, size);
        burst->move(localPos - QPoint(size / 2, size / 2));
        const qreal fillAlpha = qBound(0.0, 0.25 * (1.0 - progress), 0.25);
        const qreal borderAlpha = qBound(0.0, 0.9 * (1.0 - progress), 0.9);
        burst->setStyleSheet(QStringLiteral("background:rgba(248,113,113,%1); border:2px solid rgba(248,113,113,%2); border-radius:%3px;")
                                 .arg(QString::number(fillAlpha, 'f', 3))
                                 .arg(QString::number(borderAlpha, 'f', 3))
                                 .arg(size / 2));
        opacityEffect->setOpacity(qMax(0.0, 1.0 - progress));
    });
    QObject::connect(animation, &QVariantAnimation::finished, burst, [burst]() {
        burst->deleteLater();
    });
    animation->start();
}

void MainWindow::editCustomTemporaryCourse()
{
    while (true)
    {
        CourseEditDialog dialog(this);
        dialog.setCourseName(customTemporaryCourseName);
        dialog.setCourseColor(customTemporaryCourseColor);
        if (dialog.exec() != QDialog::Accepted)
            break;

        if (dialog.removeRequested())
        {
            customTemporaryCourseName = generateUniqueCustomCourseName(QString::fromUtf8("临时课程"));
            customTemporaryCourseColor = QColor("#f97316");
            updateCustomTemporaryCourseLabel();
            saveActualScheduleToDisk();
            break;
        }

        QString name = dialog.courseName().trimmed();
        if (name.isEmpty())
            name = QString::fromUtf8("临时课程");
        if (activeBaseContainsCourseName(name))
        {
            QMessageBox::warning(this,
                                 QString::fromUtf8("名称冲突"),
                                 QString::fromUtf8("该名称已在本周基础课表中使用，请输入不同的临时课程名称。"));
            continue;
        }
        customTemporaryCourseName = name;
        customTemporaryCourseColor = dialog.courseColor().isValid() ? dialog.courseColor() : QColor("#f97316");
        updateCustomTemporaryCourseLabel();
        saveActualScheduleToDisk();
        break;
    }
}

void MainWindow::updateCustomTemporaryCourseLabel()
{
    if (!customTemporaryCourseLabel)
        return;
    QString displayName = customTemporaryCourseName.trimmed();
    if (displayName.isEmpty())
        displayName = QString::fromUtf8("临时课程");
    QColor fill = customTemporaryCourseColor.isValid() ? customTemporaryCourseColor : QColor("#fcd34d");
    QColor darker = fill.darker(140);
    QString style = QStringLiteral("QLabel{background:%1; color:%2; border:1px solid rgba(15,23,42,0.15); border-radius:10px; font-weight:600; padding:8px 12px;}")
                        .arg(fill.name(QColor::HexRgb))
                        .arg(darker.value() < 150 ? QStringLiteral("#ffffff") : QStringLiteral("#0f172a"));
    customTemporaryCourseLabel->setStyleSheet(style);
    customTemporaryCourseLabel->setText(displayName);
}

void MainWindow::startCustomTemporaryCourseDrag()
{
    if (!customTemporaryCourseLabel)
        return;
    ensureCustomTemporaryCourseNameUnique();
    QString name = customTemporaryCourseName.trimmed();
    if (name.isEmpty())
        name = QString::fromUtf8("临时课程");
    QColor color = customTemporaryCourseColor.isValid() ? customTemporaryCourseColor : QColor("#f97316");

    auto *drag = new QDrag(customTemporaryCourseLabel);
    auto *mime = new QMimeData();
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << name << color;
    mime->setData("application/x-weekly-course", payload);
    drag->setMimeData(mime);

    QPixmap pixmap(customTemporaryCourseLabel->size());
    customTemporaryCourseLabel->render(&pixmap);
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
    drag->exec(Qt::CopyAction);
    customTemporaryCourseLabel->setCursor(Qt::OpenHandCursor);
}

void MainWindow::handleResetTemporaryAdjustments()
{
    if (!isTemporaryAdjustModeActive())
        return;
    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              QString::fromUtf8("恢复基础课表"),
                                                              QString::fromUtf8("确定要清除本周的所有临时调课记录，并恢复为基础课表吗？"));
    if (reply != QMessageBox::Yes)
        return;

    const QDate weekKey = mondayOf(currentWeekStart);
    bool removed = false;
    for (int i = actualWeekCourses.size() - 1; i >= 0; --i)
    {
        if (actualWeekCourses.at(i).weekStart == weekKey)
        {
            actualWeekCourses.removeAt(i);
            removed = true;
            break;
        }
    }

    if (!removed)
    {
        QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("本周暂无临时调课记录。"));
        return;
    }

    saveActualScheduleToDisk();
    refreshScheduleCells();
    updateWeeklyStatistics();
}

bool MainWindow::buildReminderEntryFromForm(ReminderEntry &entry, QString &error) const
{
    if (!reminderTitleEdit || !reminderTypeCombo)
    {
        error = QString::fromUtf8("提醒控件未初始化。");
        return false;
    }

    entry.title = reminderTitleEdit->text().trimmed();
    if (entry.title.isEmpty())
    {
        error = QString::fromUtf8("提醒内容不能为空。");
        return false;
    }

    entry.type = static_cast<ReminderEntry::Type>(reminderTypeCombo->currentData().toInt());

    if (entry.type == ReminderEntry::Type::Once)
    {
        if (!reminderOnceDateTimeEdit || !reminderOnceDateTimeEdit->dateTime().isValid())
        {
            error = QString::fromUtf8("请设置有效的一次性提醒时间。");
            return false;
        }
        entry.onceDateTime = reminderOnceDateTimeEdit->dateTime();
    }
    else
    {
        if (!reminderStartDateEdit || !reminderEndDateEdit || !reminderTimeEdit)
        {
            error = QString::fromUtf8("请设置有效的重复提醒时间。");
            return false;
        }
        entry.startDate = reminderStartDateEdit->date();
        entry.endDate = reminderEndDateEdit->date();
        if (!entry.startDate.isValid() || !entry.endDate.isValid())
        {
            error = QString::fromUtf8("开始和结束日期必须有效。");
            return false;
        }
        if (entry.endDate < entry.startDate)
        {
            error = QString::fromUtf8("结束日期必须不早于开始日期。");
            return false;
        }
        entry.time = reminderTimeEdit->time();
        if (entry.type == ReminderEntry::Type::Weekly && reminderWeekdayCombo)
            entry.weekday = reminderWeekdayCombo->currentIndex() + 1;
    }
    entry.visible = true;
    return true;
}

void MainWindow::handleReminderTypeChanged(int index)
{
    ReminderEntry::Type type = ReminderEntry::Type::Once;
    if (reminderTypeCombo && index >= 0)
        type = static_cast<ReminderEntry::Type>(reminderTypeCombo->itemData(index).toInt());

    if (reminderOnceFields)
        reminderOnceFields->setVisible(type == ReminderEntry::Type::Once);
    if (reminderRepeatFields)
        reminderRepeatFields->setVisible(type != ReminderEntry::Type::Once);
    if (reminderWeeklyExtraFields)
        reminderWeeklyExtraFields->setVisible(type == ReminderEntry::Type::Weekly);
    adjustSecondaryContainerHeight();
}

void MainWindow::handleAddReminder()
{
    ReminderEntry entry;
    QString error;
    if (!buildReminderEntryFromForm(entry, error))
    {
        QMessageBox::warning(this, QString::fromUtf8("添加提醒失败"), error);
        return;
    }

    reminderEntries.append(entry);
    saveReminders();
    refreshReminderList();
    reminderTitleEdit->clear();
    if (reminderListWidget)
        reminderListWidget->clearSelection();
    clearReminderForm();
}

void MainWindow::handleRemoveReminder()
{
    if (!reminderListWidget)
        return;
    const int row = reminderListWidget->currentRow();
    if (row < 0 || row >= reminderEntries.size())
        return;
    reminderEntries.removeAt(row);
    saveReminders();
    refreshReminderList();
}

void MainWindow::refreshReminderList()
{
    if (!reminderListWidget)
        return;
    const int previousRow = reminderListWidget->currentRow();
    reminderListWidget->clear();

    auto weekdayText = [](int weekday) {
        static const QStringList names = {QString::fromUtf8("周一"), QString::fromUtf8("周二"), QString::fromUtf8("周三"),
                                          QString::fromUtf8("周四"), QString::fromUtf8("周五"), QString::fromUtf8("周六"), QString::fromUtf8("周日")};
        if (weekday >= 1 && weekday <= names.size())
            return names.at(weekday - 1);
        return QString::fromUtf8("周一");
    };

    for (int i = 0; i < reminderEntries.size(); ++i)
    {
        const ReminderEntry &entry = reminderEntries.at(i);
        QString typeText;
        switch (entry.type)
        {
        case ReminderEntry::Type::Once:
            typeText = QString::fromUtf8("一次");
            break;
        case ReminderEntry::Type::Daily:
            typeText = QString::fromUtf8("每天");
            break;
        case ReminderEntry::Type::Weekly:
            typeText = QString::fromUtf8("每周");
            break;
        }

        QString text = QStringLiteral("%1 | %2").arg(typeText, entry.title);
        QListWidgetItem *item = new QListWidgetItem(text);
        QColor baseColor;
        QColor textColor = QColor("#0f172a");
        switch (entry.type)
        {
        case ReminderEntry::Type::Once:
            baseColor = QColor("#fef3c7");
            break;
        case ReminderEntry::Type::Daily:
            baseColor = QColor("#dcfce7");
            break;
        case ReminderEntry::Type::Weekly:
            baseColor = QColor("#dbeafe");
            textColor = QColor("#0f172a");
            break;
        }
        item->setBackground(baseColor);
        item->setForeground(textColor);
        if (!entry.visible)
        {
            item->setForeground(QColor("#94a3b8"));
            item->setText(text + QString::fromUtf8(" (已隐藏)"));
        }
        reminderListWidget->addItem(item);
    }
    int rowToSelect = previousRow;
    if (rowToSelect >= reminderEntries.size())
        rowToSelect = reminderEntries.size() - 1;
    QSignalBlocker blocker(reminderListWidget);
    if (rowToSelect >= 0)
        reminderListWidget->setCurrentRow(rowToSelect);
    else
        reminderListWidget->setCurrentRow(-1);
    blocker.unblock();
    updateReminderActionStates();
}

void MainWindow::populateReminderForm(int index)
{
    if (!reminderTitleEdit || !reminderTypeCombo)
        return;

    if (index < 0 || index >= reminderEntries.size())
    {
        clearReminderForm();
        return;
    }

    const ReminderEntry &entry = reminderEntries.at(index);
    {
        QSignalBlocker blockTitle(reminderTitleEdit);
        reminderTitleEdit->setText(entry.title);
    }

    int typeIndex = reminderTypeCombo->findData(static_cast<int>(entry.type));
    if (typeIndex < 0)
        typeIndex = 0;
    {
        QSignalBlocker blockType(reminderTypeCombo);
        reminderTypeCombo->setCurrentIndex(typeIndex);
    }
    handleReminderTypeChanged(typeIndex);

    if (entry.type == ReminderEntry::Type::Once && reminderOnceDateTimeEdit)
        reminderOnceDateTimeEdit->setDateTime(entry.onceDateTime.isValid() ? entry.onceDateTime : QDateTime::currentDateTime());
    if (entry.type != ReminderEntry::Type::Once)
    {
        if (reminderStartDateEdit)
            reminderStartDateEdit->setDate(entry.startDate.isValid() ? entry.startDate : QDate::currentDate());
        if (reminderEndDateEdit)
            reminderEndDateEdit->setDate(entry.endDate.isValid() ? entry.endDate : QDate::currentDate());
        if (reminderTimeEdit)
            reminderTimeEdit->setTime(entry.time.isValid() ? entry.time : QTime::currentTime());
        if (entry.type == ReminderEntry::Type::Weekly && reminderWeekdayCombo)
            reminderWeekdayCombo->setCurrentIndex(qBound(0, entry.weekday - 1, 6));
    }
    updateReminderActionStates();
}

void MainWindow::clearReminderForm()
{
    if (reminderTitleEdit)
        reminderTitleEdit->clear();
    if (reminderTypeCombo)
    {
        QSignalBlocker block(reminderTypeCombo);
        reminderTypeCombo->setCurrentIndex(0);
    }
    handleReminderTypeChanged(reminderTypeCombo ? reminderTypeCombo->currentIndex() : 0);
    if (reminderOnceDateTimeEdit)
        reminderOnceDateTimeEdit->setDateTime(QDateTime::currentDateTime());
    if (reminderStartDateEdit)
        reminderStartDateEdit->setDate(QDate::currentDate());
    if (reminderEndDateEdit)
        reminderEndDateEdit->setDate(QDate::currentDate().addDays(7));
    if (reminderTimeEdit)
        reminderTimeEdit->setTime(QTime::currentTime());
    if (reminderWeekdayCombo)
        reminderWeekdayCombo->setCurrentIndex(0);
    if (reminderListWidget && reminderListWidget->currentRow() == -1)
        updateReminderActionStates();
}

void MainWindow::updateReminderActionStates()
{
    const bool hasList = reminderListWidget && reminderListWidget->count() > 0;
    const bool hasSelection = reminderListWidget && !reminderListWidget->selectedItems().isEmpty();
    const bool hasCurrent = reminderListWidget && reminderListWidget->currentRow() >= 0 && reminderListWidget->currentRow() < reminderEntries.size();

    if (removeReminderItemButton)
        removeReminderItemButton->setEnabled(hasCurrent);
    if (saveReminderItemButton)
        saveReminderItemButton->setEnabled(hasCurrent);
    if (showSelectedRemindersButton)
        showSelectedRemindersButton->setEnabled(hasSelection);
    if (hideSelectedRemindersButton)
        hideSelectedRemindersButton->setEnabled(hasSelection);
    if (showAllRemindersButton)
        showAllRemindersButton->setEnabled(hasList);
    if (hideAllRemindersButton)
        hideAllRemindersButton->setEnabled(hasList);
}

void MainWindow::handleSaveReminderChanges()
{
    if (!reminderListWidget)
        return;
    int row = reminderListWidget->currentRow();
    if (row < 0 || row >= reminderEntries.size())
        return;
    ReminderEntry entry = reminderEntries.at(row);
    QString error;
    if (!buildReminderEntryFromForm(entry, error))
    {
        QMessageBox::warning(this, QString::fromUtf8("保存失败"), error);
        return;
    }
    reminderEntries[row] = entry;
    saveReminders();
    refreshReminderList();
}

void MainWindow::handleShowSelectedReminders()
{
    if (!reminderListWidget)
        return;
    const QList<QListWidgetItem *> items = reminderListWidget->selectedItems();
    if (items.isEmpty())
        return;
    bool changed = false;
    for (QListWidgetItem *item : items)
    {
        int row = reminderListWidget->row(item);
        if (row >= 0 && row < reminderEntries.size() && !reminderEntries[row].visible)
        {
            reminderEntries[row].visible = true;
            changed = true;
        }
    }
    if (changed)
    {
        saveReminders();
        refreshReminderList();
    }
    updateReminderCards();
}

void MainWindow::handleHideSelectedReminders()
{
    if (!reminderListWidget)
        return;
    const QList<QListWidgetItem *> items = reminderListWidget->selectedItems();
    if (items.isEmpty())
        return;
    bool changed = false;
    for (QListWidgetItem *item : items)
    {
        int row = reminderListWidget->row(item);
        if (row >= 0 && row < reminderEntries.size() && reminderEntries[row].visible)
        {
            reminderEntries[row].visible = false;
            changed = true;
        }
    }
    if (changed)
    {
        saveReminders();
        refreshReminderList();
    }
    updateReminderCards();
}

void MainWindow::handleShowAllReminders()
{
    bool changed = false;
    for (ReminderEntry &entry : reminderEntries)
    {
        if (!entry.visible)
        {
            entry.visible = true;
            changed = true;
        }
    }
    if (changed)
    {
        saveReminders();
        refreshReminderList();
    }
    updateReminderCards();
}

void MainWindow::handleHideAllReminders()
{
    bool changed = false;
    for (ReminderEntry &entry : reminderEntries)
    {
        if (entry.visible)
        {
            entry.visible = false;
            changed = true;
        }
    }
    if (changed)
    {
        saveReminders();
        refreshReminderList();
    }
    updateReminderCards();
}

void MainWindow::showLargeScheduleDialog()
{
    ensureLargeScheduleDialog();
    if (largeScheduleDialog)
        largeScheduleDialog->show();
}

void MainWindow::ensureLargeScheduleDialog()
{
    if (largeScheduleDialog)
        return;
    largeScheduleDialog = new QMainWindow();
    largeScheduleDialog->setAttribute(Qt::WA_DeleteOnClose, false);
    largeScheduleDialog->installEventFilter(this);
    largeScheduleDialog->setWindowTitle(QString::fromUtf8("参考课表"));
    largeScheduleDialog->resize(900, 600);
    QWidget *central = new QWidget(largeScheduleDialog);
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    QHBoxLayout *barLayout = new QHBoxLayout();
    barLayout->setContentsMargins(0, 0, 0, 0);
    barLayout->setSpacing(8);
    QLabel *titleLabel = new QLabel(QString::fromUtf8("参考课表"), largeScheduleDialog);
    titleLabel->setStyleSheet("font-weight:600;");
    barLayout->addWidget(titleLabel);
    barLayout->addStretch();
    QPushButton *importButton = new QPushButton(QString::fromUtf8("导入"), largeScheduleDialog);
    importButton->setCursor(Qt::PointingHandCursor);
    importButton->setStyleSheet("QPushButton{padding:6px 16px; border-radius:6px; background:#2563eb; color:white;} QPushButton:hover{background:#1d4ed8;}");
    QPushButton *deleteColumnButton = new QPushButton(QString::fromUtf8("删除"), largeScheduleDialog);
    deleteColumnButton->setCursor(Qt::PointingHandCursor);
    deleteColumnButton->setStyleSheet("QPushButton{padding:6px 16px; border-radius:6px; background:#ef4444; color:white;} QPushButton:hover{background:#dc2626;}");
    toggleLargeScheduleSelectorsButton = new QPushButton(QString::fromUtf8("隐藏/显示"), largeScheduleDialog);
    toggleLargeScheduleSelectorsButton->setCursor(Qt::PointingHandCursor);
    toggleLargeScheduleSelectorsButton->setStyleSheet("QPushButton{padding:6px 12px; border-radius:6px; background:#0ea5e9; color:white;} QPushButton:hover{background:#0284c7;}");
    configureLargeScheduleColorsButton = new QPushButton(QString::fromUtf8("配置颜色"), largeScheduleDialog);
    configureLargeScheduleColorsButton->setCursor(Qt::PointingHandCursor);
    configureLargeScheduleColorsButton->setStyleSheet("QPushButton{padding:6px 12px; border-radius:6px; background:#22c55e; color:white;} QPushButton:hover{background:#16a34a;}");
    conflictDetectionButton = new QPushButton(QString::fromUtf8("冲突检测"), largeScheduleDialog);
    conflictDetectionButton->setCheckable(true);
    conflictDetectionButton->setCursor(Qt::PointingHandCursor);
    conflictDetectionButton->setStyleSheet("QPushButton{padding:6px 12px; border-radius:6px; background:#94a3b8; color:white;} QPushButton:checked{background:#ea580c;} QPushButton:hover{background:#a1a1aa;} QPushButton:checked:hover{background:#c2410c;}");
    teacherRosterButton = new QPushButton(QString::fromUtf8("任课教师一览表"), largeScheduleDialog);
    teacherRosterButton->setCheckable(true);
    teacherRosterButton->setCursor(Qt::PointingHandCursor);
    teacherRosterButton->setStyleSheet("QPushButton{padding:6px 12px; border-radius:6px; background:#f97316; color:white;} QPushButton:checked{background:#c2410c;} QPushButton:hover{background:#fb923c;} QPushButton:checked:hover{background:#ea580c;}");
    barLayout->addWidget(importButton);
    barLayout->addWidget(deleteColumnButton);
    barLayout->addWidget(toggleLargeScheduleSelectorsButton);
    barLayout->addWidget(configureLargeScheduleColorsButton);
    barLayout->addWidget(conflictDetectionButton);
    barLayout->addWidget(teacherRosterButton);
    layout->addLayout(barLayout);

    largeScheduleTable = new QTableWidget(central);
    largeScheduleTable->setColumnCount(0);
    largeScheduleTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    largeScheduleTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    largeScheduleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    largeScheduleTable->horizontalHeader()->setMinimumSectionSize(kLargeScheduleMinColumnWidth);
    largeScheduleTable->verticalHeader()->setVisible(false);
    largeScheduleTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    largeScheduleTable->verticalHeader()->setMinimumSectionSize(kLargeScheduleMinRowHeight);
    largeScheduleTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    largeScheduleTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    largeScheduleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    largeScheduleTable->setWordWrap(true);
    largeScheduleTable->setStyleSheet("QTableWidget{background:white;} QTableWidget::item{padding:6px;}");
    if (largeScheduleTable->viewport())
        largeScheduleTable->viewport()->installEventFilter(this);
    layout->addWidget(largeScheduleTable, 1);

    largeScheduleDialog->setCentralWidget(central);

    connect(importButton, &QPushButton::clicked, this, &MainWindow::handleImportLargeSchedule);
    connect(deleteColumnButton, &QPushButton::clicked, this, &MainWindow::handleDeleteLargeScheduleColumns);
    connect(toggleLargeScheduleSelectorsButton, &QPushButton::clicked, this, &MainWindow::toggleLargeScheduleSelectorsVisibility);
    connect(configureLargeScheduleColorsButton, &QPushButton::clicked, this, &MainWindow::configureLargeScheduleColors);
    connect(conflictDetectionButton, &QPushButton::toggled, this, &MainWindow::setConflictDetectionEnabled);
    connect(teacherRosterButton, &QPushButton::toggled, this, &MainWindow::toggleTeacherRosterVisibility);
    connect(largeScheduleTable, &QTableWidget::cellClicked, this, &MainWindow::handleLargeScheduleCellClicked);
}

void MainWindow::buildTeacherRosterDock()
{
    if (!largeScheduleDialog || teacherRosterDock)
        return;
    teacherRosterDock = new QDockWidget(QString::fromUtf8("任课教师一览表"), largeScheduleDialog);
    teacherRosterDock->setObjectName(QStringLiteral("teacherRosterDock"));
    teacherRosterDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::TopDockWidgetArea);
    teacherRosterDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    QWidget *dockContent = new QWidget(teacherRosterDock);
    QVBoxLayout *dockLayout = new QVBoxLayout(dockContent);
    dockLayout->setContentsMargins(8, 8, 8, 8);
    dockLayout->setSpacing(6);

    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *hintLabel = new QLabel(QString::fromUtf8("首行科目／首列班级"), dockContent);
    hintLabel->setStyleSheet("color:#475569;");
    toolbarLayout->addWidget(hintLabel);
    toolbarLayout->addStretch();
    QPushButton *importRosterButton = new QPushButton(QString::fromUtf8("导入任课教师"), dockContent);
    importRosterButton->setCursor(Qt::PointingHandCursor);
    importRosterButton->setStyleSheet("QPushButton{padding:4px 12px; border-radius:6px; background:#059669; color:white;} QPushButton:hover{background:#047857;}");
    toolbarLayout->addWidget(importRosterButton);
    associateRosterButton = new QPushButton(QString::fromUtf8("关联"), dockContent);
    associateRosterButton->setCursor(Qt::PointingHandCursor);
    associateRosterButton->setStyleSheet("QPushButton{padding:4px 12px; border-radius:6px; background:#ea580c; color:white;} QPushButton:hover{background:#c2410c;}");
    toolbarLayout->addWidget(associateRosterButton);
    dockLayout->addLayout(toolbarLayout);

    teacherRosterTable = new QTableWidget(dockContent);
    teacherRosterTable->setAlternatingRowColors(true);
    teacherRosterTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    teacherRosterTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    teacherRosterTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    teacherRosterTable->setStyleSheet("QTableWidget{background:white;} QTableWidget::item{padding:4px;}");
    teacherRosterTable->setWordWrap(true);
    teacherRosterTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    teacherRosterTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    teacherRosterTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    teacherRosterTable->horizontalHeader()->setMinimumSectionSize(kTeacherRosterMinColumnWidth);
    teacherRosterTable->verticalHeader()->setVisible(true);
    teacherRosterTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    teacherRosterTable->verticalHeader()->setMinimumSectionSize(kTeacherRosterMinRowHeight);
    if (teacherRosterTable->viewport())
        teacherRosterTable->viewport()->installEventFilter(this);
    dockLayout->addWidget(teacherRosterTable, 1);

    teacherRosterDock->setWidget(dockContent);
    largeScheduleDialog->addDockWidget(Qt::LeftDockWidgetArea, teacherRosterDock);

    connect(importRosterButton, &QPushButton::clicked, this, &MainWindow::handleImportTeacherRoster);
    connect(associateRosterButton, &QPushButton::clicked, this, &MainWindow::handleAssociateTeacherRoster);
    connect(teacherRosterDock, &QDockWidget::topLevelChanged, this, [this](bool floating) {
        if (floating)
            positionTeacherRosterDock();
    });
    connect(teacherRosterDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (teacherRosterButton)
        {
            QSignalBlocker blocker(teacherRosterButton);
            teacherRosterButton->setChecked(visible);
        }
        if (visible && teacherRosterDock->isFloating())
            positionTeacherRosterDock();
    });
    teacherRosterDock->setFloating(true);
    teacherRosterDock->hide();
    adjustTeacherRosterCellSizes();
}

void MainWindow::toggleTeacherRosterVisibility(bool visible)
{
    if (visible)
    {
        ensureLargeScheduleDialog();
        if (!teacherRosterDock)
            buildTeacherRosterDock();
    }
    if (!teacherRosterDock)
        return;

    if (visible)
    {
        teacherRosterDock->show();
        if (teacherRosterDock->isFloating())
            positionTeacherRosterDock();
        teacherRosterDock->raise();
        teacherRosterDock->activateWindow();
    }
    else
    {
        teacherRosterDock->hide();
    }
}

void MainWindow::handleImportLargeSchedule()
{
    ensureLargeScheduleDialog();
    if (!largeScheduleTable)
        return;
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                     QString::fromUtf8("导入参考课表"),
                                                         QString(),
                                                         QString::fromUtf8("Excel 文件 (*.xlsx)"));
    if (filePath.isEmpty())
        return;

    QXlsx::Document xlsx(filePath);
    if (!xlsx.load())
    {
        QMessageBox::warning(this, QString::fromUtf8("导入失败"), QString::fromUtf8("无法打开所选文件。"));
        return;
    }
    Worksheet *sheet = dynamic_cast<Worksheet *>(xlsx.currentWorksheet());
    if (!sheet)
    {
        QMessageBox::warning(this, QString::fromUtf8("导入失败"), QString::fromUtf8("未找到有效的工作表。"));
        return;
    }
    QXlsx::CellRange range = sheet->dimension();
    if (!range.isValid())
    {
        QMessageBox::information(this, QString::fromUtf8("导入完成"), QString::fromUtf8("工作表为空。"));
        largeScheduleTable->clear();
        largeScheduleTable->setRowCount(1);
        largeScheduleTable->setColumnCount(1);
        clearLargeScheduleColumnSelections();
        clearLargeScheduleRowSelections();
        updateLargeScheduleColumnLabels();
        updateLargeScheduleRowLabels();
        adjustLargeScheduleCellSizes();
        return;
    }

    const int firstRow = range.firstRow();
    const int firstColumn = range.firstColumn();
    const int rowCount = range.rowCount();
    const int columnCount = range.columnCount();

    // 先读取数据到临时矩阵，便于去除空行/空列
    QVector<QVector<QString>> rawTexts(rowCount, QVector<QString>(columnCount));
    QVector<QVector<QColor>> rawColors(rowCount, QVector<QColor>(columnCount));
    for (int r = 0; r < rowCount; ++r)
    {
        for (int c = 0; c < columnCount; ++c)
        {
            const int sheetRow = firstRow + r;
            const int sheetCol = firstColumn + c;
            auto cell = sheet->cellAt(sheetRow, sheetCol);
            QString text = cell ? cell->value().toString() : QString();
            rawTexts[r][c] = text;
            if (cell)
            {
                QXlsx::Format fmt = cell->format();
                QColor fill = fmt.patternBackgroundColor();
                if (!fill.isValid())
                    fill = fmt.patternForegroundColor();
                rawColors[r][c] = fill;
            }
        }
    }

    // 计算非空行/列
    QVector<int> validRows;
    QVector<int> validCols;
    for (int r = 0; r < rowCount; ++r)
    {
        bool rowHasContent = false;
        for (int c = 0; c < columnCount; ++c)
        {
            if (!rawTexts[r][c].trimmed().isEmpty())
            {
                rowHasContent = true;
                break;
            }
        }
        if (rowHasContent)
            validRows.append(r);
    }
    for (int c = 0; c < columnCount; ++c)
    {
        bool colHasContent = false;
        for (int r = 0; r < rowCount; ++r)
        {
            if (!rawTexts[r][c].trimmed().isEmpty())
            {
                colHasContent = true;
                break;
            }
        }
        if (colHasContent)
            validCols.append(c);
    }
    if (validRows.isEmpty() || validCols.isEmpty())
    {
        largeScheduleTable->clear();
        largeScheduleTable->setRowCount(1);
        largeScheduleTable->setColumnCount(1);
        clearLargeScheduleColumnSelections();
        clearLargeScheduleRowSelections();
        updateLargeScheduleColumnLabels();
        updateLargeScheduleRowLabels();
        adjustLargeScheduleCellSizes();
        return;
    }

    const int filteredRowCount = validRows.size();
    const int filteredColCount = validCols.size();

    largeScheduleTable->clear();
    largeScheduleTable->setRowCount(filteredRowCount + 1);
    largeScheduleTable->setColumnCount(filteredColCount + 1);
    largeScheduleTable->horizontalHeader()->setVisible(false);
    largeScheduleTable->verticalHeader()->setVisible(false);
    updateLargeScheduleColumnLabels();
    updateLargeScheduleRowLabels();

    for (int rIdx = 0; rIdx < filteredRowCount; ++rIdx)
    {
        int r = validRows.at(rIdx);
        for (int cIdx = 0; cIdx < filteredColCount; ++cIdx)
        {
            int c = validCols.at(cIdx);
            QString text = rawTexts[r][c];
            auto *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            QColor fill = rawColors[r][c];
            if (fill.isValid())
                item->setBackground(fill);
            largeScheduleTable->setItem(rIdx + 1, cIdx + 1, item);
            const QString courseName = text.trimmed();
            if (!courseName.isEmpty() && courseColorMap.contains(courseName))
                item->setBackground(courseColorMap.value(courseName));
        }
    }

    const QList<CellRange> merges = sheet->mergedCells();
    for (const CellRange &merged : merges)
    {
        const int top = merged.firstRow() - firstRow;
        const int left = merged.firstColumn() - firstColumn;
        if (top < 0 || left < 0)
            continue;
        largeScheduleTable->setSpan(top + 1,
                                    left + 1,
                                    merged.rowCount(),
                                    merged.columnCount());
    }

    removeEmptyLargeScheduleRows();
    removeEmptyLargeScheduleColumns();
    updateLargeScheduleColumnLabels();
    updateLargeScheduleRowLabels();
    clearLargeScheduleColumnSelections();
    clearLargeScheduleRowSelections();
    clearConflictHighlights();
    applyCourseColorsToLargeSchedule();
    adjustLargeScheduleCellSizes();
}

void MainWindow::handleImportTeacherRoster()
{
    ensureLargeScheduleDialog();
    if (!teacherRosterTable)
        return;
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                         QString::fromUtf8("导入任课教师"),
                                                         QString(),
                                                         QString::fromUtf8("Excel 文件 (*.xlsx)"));
    if (filePath.isEmpty())
        return;

    QXlsx::Document xlsx(filePath);
    if (!xlsx.load())
    {
        QMessageBox::warning(this, QString::fromUtf8("导入失败"), QString::fromUtf8("无法打开所选文件。"));
        return;
    }
    Worksheet *sheet = dynamic_cast<Worksheet *>(xlsx.currentWorksheet());
    if (!sheet)
    {
        QMessageBox::warning(this, QString::fromUtf8("导入失败"), QString::fromUtf8("未找到有效的工作表。"));
        return;
    }
    const CellRange range = sheet->dimension();
    if (!range.isValid())
    {
        QMessageBox::warning(this, QString::fromUtf8("导入失败"), QString::fromUtf8("工作表为空。"));
        return;
    }

    const int firstRow = range.firstRow();
    const int firstColumn = range.firstColumn();
    const int totalRows = range.rowCount();
    const int totalColumns = range.columnCount();

    // 读取所有文本，忽略颜色
    QVector<QVector<QString>> rawTexts(totalRows, QVector<QString>(totalColumns));
    for (int r = 0; r < totalRows; ++r)
    {
        for (int c = 0; c < totalColumns; ++c)
        {
            auto cell = sheet->cellAt(firstRow + r, firstColumn + c);
            if (cell)
                rawTexts[r][c] = cell->value().toString();
        }
    }

    // 去除全空行/列
    QVector<int> validRows;
    QVector<int> validCols;
    for (int r = 0; r < totalRows; ++r)
    {
        bool hasText = false;
        for (int c = 0; c < totalColumns; ++c)
        {
            if (!rawTexts[r][c].trimmed().isEmpty())
            {
                hasText = true;
                break;
            }
        }
        if (hasText)
            validRows.append(r);
    }
    for (int c = 0; c < totalColumns; ++c)
    {
        bool hasText = false;
        for (int r = 0; r < totalRows; ++r)
        {
            if (!rawTexts[r][c].trimmed().isEmpty())
            {
                hasText = true;
                break;
            }
        }
        if (hasText)
            validCols.append(c);
    }

    if (validRows.size() < 2 || validCols.size() < 2)
    {
        teacherRosterTable->clear();
        teacherRosterTable->setRowCount(0);
        teacherRosterTable->setColumnCount(0);
        adjustTeacherRosterCellSizes();
        QMessageBox::warning(this, QString::fromUtf8("导入失败"), QString::fromUtf8("工作表内容不足，无法生成任课教师一览表。"));
        return;
    }

    const int dataRows = validRows.size() - 1;
    const int dataCols = validCols.size() - 1;

    teacherRosterTable->clear();
    teacherRosterTable->setRowCount(dataRows);
    teacherRosterTable->setColumnCount(dataCols);

    QStringList subjectHeaders;
    subjectHeaders.reserve(dataCols);
    for (int ci = 1; ci < validCols.size(); ++ci)
    {
        QString text = rawTexts[validRows.first()][validCols.at(ci)].trimmed();
        if (text.isEmpty())
            text = QString::fromUtf8("科目%1").arg(ci);
        subjectHeaders << text;
    }
    teacherRosterTable->setHorizontalHeaderLabels(subjectHeaders);

    QStringList rowHeaders;
    rowHeaders.reserve(dataRows);
    for (int ri = 1; ri < validRows.size(); ++ri)
    {
        QString text = rawTexts[validRows.at(ri)][validCols.first()].trimmed();
        if (text.isEmpty())
            text = QString::fromUtf8("班级%1").arg(ri);
        rowHeaders << text;
    }
    teacherRosterTable->setVerticalHeaderLabels(rowHeaders);

    for (int ri = 1; ri < validRows.size(); ++ri)
    {
        const int tableRow = ri - 1;
        for (int ci = 1; ci < validCols.size(); ++ci)
        {
            QString text = rawTexts[validRows.at(ri)][validCols.at(ci)];
            auto *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            teacherRosterTable->setItem(tableRow, ci - 1, item);
        }
    }

    adjustTeacherRosterCellSizes();
    if (teacherRosterDock)
        teacherRosterDock->raise();
}

void MainWindow::handleLargeScheduleCellClicked(int row, int column)
{
    if (!largeScheduleTable)
        return;
    QTableWidgetItem *clickedItem = largeScheduleTable->item(row, column);
    const QString cellText = clickedItem ? clickedItem->text() : QString();
    qDebug() << "[LargeSchedule] clicked" << "(" << row << "," << column << ")" << cellText;
    if (row <= 0 || column <= 0)
        return;
    if (conflictDetectionEnabled)
        checkTeacherConflicts(row, column);
}

void MainWindow::applyLargeScheduleSelectorClick(int column, Qt::KeyboardModifiers modifiers)
{
    if (!largeScheduleTable)
        return;
    const int columnCount = largeScheduleTable->columnCount();
    if (columnCount <= 1)
        return;
    if (column < 1 || column >= columnCount)
        return;

    if ((modifiers & Qt::ShiftModifier) && largeScheduleSelectorAnchorColumn >= 0)
    {
        const bool additive = modifiers.testFlag(Qt::ControlModifier);
        applyLargeScheduleSelectorRange(largeScheduleSelectorAnchorColumn, column, additive);
    }
    else if (modifiers & Qt::ControlModifier)
    {
        if (largeScheduleSelectedColumns.contains(column))
            largeScheduleSelectedColumns.remove(column);
        else
            largeScheduleSelectedColumns.insert(column);
        largeScheduleSelectorAnchorColumn = column;
        largeScheduleSelectorLastColumn = column;
        updateLargeScheduleSelectorVisuals();
    }
    else
    {
        largeScheduleSelectedColumns.clear();
        largeScheduleSelectedColumns.insert(column);
        largeScheduleSelectorAnchorColumn = column;
        largeScheduleSelectorLastColumn = column;
        updateLargeScheduleSelectorVisuals();
    }
}

void MainWindow::applyLargeScheduleSelectorDrag(int column)
{
    if (!largeScheduleTable)
        return;
    const int columnCount = largeScheduleTable->columnCount();
    if (columnCount <= 1)
        return;
    if (column < 1 || column >= columnCount)
        return;
    if (largeScheduleSelectorAnchorColumn < 0)
        largeScheduleSelectorAnchorColumn = column;
    if (column == largeScheduleSelectorLastColumn)
        return;
    applyLargeScheduleSelectorRange(largeScheduleSelectorAnchorColumn, column, false);
    largeScheduleSelectorLastColumn = column;
}

void MainWindow::applyLargeScheduleSelectorRange(int startColumn, int endColumn, bool additive)
{
    if (!largeScheduleTable)
        return;
    const int columnCount = largeScheduleTable->columnCount();
    if (columnCount <= 1)
        return;
    int start = qBound(1, qMin(startColumn, endColumn), columnCount - 1);
    int end = qBound(1, qMax(startColumn, endColumn), columnCount - 1);

    QSet<int> newSelection;
    for (int c = start; c <= end; ++c)
        newSelection.insert(c);
    if (additive)
        largeScheduleSelectedColumns.unite(newSelection);
    else
        largeScheduleSelectedColumns = newSelection;
    updateLargeScheduleSelectorVisuals();
}

void MainWindow::applyLargeScheduleRowSelectorClick(int row, Qt::KeyboardModifiers modifiers)
{
    if (!largeScheduleTable)
        return;
    const int rowCount = largeScheduleTable->rowCount();
    if (rowCount <= 1)
        return;
    if (row < 1 || row >= rowCount)
        return;

    if ((modifiers & Qt::ShiftModifier) && largeScheduleRowAnchor >= 0)
    {
        const bool additive = modifiers.testFlag(Qt::ControlModifier);
        applyLargeScheduleRowSelectorRange(largeScheduleRowAnchor, row, additive);
    }
    else if (modifiers & Qt::ControlModifier)
    {
        if (largeScheduleSelectedRows.contains(row))
            largeScheduleSelectedRows.remove(row);
        else
            largeScheduleSelectedRows.insert(row);
        largeScheduleRowAnchor = row;
        largeScheduleRowLast = row;
        updateLargeScheduleSelectorVisuals();
    }
    else
    {
        largeScheduleSelectedRows.clear();
        largeScheduleSelectedRows.insert(row);
        largeScheduleRowAnchor = row;
        largeScheduleRowLast = row;
        updateLargeScheduleSelectorVisuals();
    }
}

void MainWindow::applyLargeScheduleRowSelectorDrag(int row)
{
    if (!largeScheduleTable)
        return;
    const int rowCount = largeScheduleTable->rowCount();
    if (rowCount <= 1)
        return;
    if (row < 1 || row >= rowCount)
        return;
    if (largeScheduleRowAnchor < 0)
        largeScheduleRowAnchor = row;
    if (row == largeScheduleRowLast)
        return;
    applyLargeScheduleRowSelectorRange(largeScheduleRowAnchor, row, false);
    largeScheduleRowLast = row;
}

void MainWindow::applyLargeScheduleRowSelectorRange(int startRow, int endRow, bool additive)
{
    if (!largeScheduleTable)
        return;
    const int rowCount = largeScheduleTable->rowCount();
    if (rowCount <= 1)
        return;
    int start = qBound(1, qMin(startRow, endRow), rowCount - 1);
    int end = qBound(1, qMax(startRow, endRow), rowCount - 1);

    QSet<int> newSelection;
    for (int r = start; r <= end; ++r)
        newSelection.insert(r);
    if (additive)
        largeScheduleSelectedRows.unite(newSelection);
    else
        largeScheduleSelectedRows = newSelection;
    updateLargeScheduleSelectorVisuals();
}

void MainWindow::clearLargeScheduleColumnSelections()
{
    largeScheduleSelectedColumns.clear();
    largeScheduleSelectorAnchorColumn = -1;
    largeScheduleSelectorLastColumn = -1;
    largeScheduleSelectorDragging = false;
    updateLargeScheduleSelectorVisuals();
}

void MainWindow::clearLargeScheduleRowSelections()
{
    largeScheduleSelectedRows.clear();
    largeScheduleRowAnchor = -1;
    largeScheduleRowLast = -1;
    largeScheduleRowDragging = false;
    updateLargeScheduleSelectorVisuals();
}

void MainWindow::updateLargeScheduleSelectorVisuals()
{
    if (!largeScheduleTable || largeScheduleTable->rowCount() == 0)
        return;
    const int columnCount = largeScheduleTable->columnCount();
    for (int c = 1; c < columnCount; ++c)
    {
        if (QTableWidgetItem *selector = largeScheduleTable->item(0, c))
        {
            if (largeScheduleSelectedColumns.contains(c))
                selector->setBackground(QColor("#bfdbfe"));
            else
                selector->setBackground(Qt::white);
        }
    }
    const int rowCount = largeScheduleTable->rowCount();
    for (int r = 1; r < rowCount; ++r)
    {
        if (QTableWidgetItem *selector = largeScheduleTable->item(r, 0))
        {
            if (largeScheduleSelectedRows.contains(r))
                selector->setBackground(QColor("#fde68a"));
            else
                selector->setBackground(Qt::white);
        }
    }
}

void MainWindow::updateLargeScheduleColumnLabels()
{
    if (!largeScheduleTable)
        return;
    if (largeScheduleTable->rowCount() == 0)
        largeScheduleTable->setRowCount(1);
    if (largeScheduleTable->columnCount() == 0)
        largeScheduleTable->setColumnCount(1);
    if (!largeScheduleTable->item(0, 0))
    {
        auto *corner = new QTableWidgetItem();
        corner->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        corner->setTextAlignment(Qt::AlignCenter);
        largeScheduleTable->setItem(0, 0, corner);
    }
    const int columnCount = largeScheduleTable->columnCount();
    for (int c = 1; c < columnCount; ++c)
    {
        QTableWidgetItem *selector = largeScheduleTable->item(0, c);
        if (!selector)
        {
            selector = new QTableWidgetItem();
            selector->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            selector->setTextAlignment(Qt::AlignCenter);
            largeScheduleTable->setItem(0, c, selector);
        }
        selector->setText(largeScheduleColumnLabel(c - 1));
    }
    updateLargeScheduleSelectorVisuals();
}

void MainWindow::updateLargeScheduleRowLabels()
{
    if (!largeScheduleTable)
        return;
    if (largeScheduleTable->rowCount() == 0)
        largeScheduleTable->setRowCount(1);
    if (largeScheduleTable->columnCount() == 0)
        largeScheduleTable->setColumnCount(1);
    for (int r = 1; r < largeScheduleTable->rowCount(); ++r)
    {
        QTableWidgetItem *selector = largeScheduleTable->item(r, 0);
        if (!selector)
        {
            selector = new QTableWidgetItem();
            selector->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            selector->setTextAlignment(Qt::AlignCenter);
            largeScheduleTable->setItem(r, 0, selector);
        }
        selector->setText(QString::number(r));
    }
    if (QTableWidgetItem *corner = largeScheduleTable->item(0, 0))
        corner->setText(QString());
    updateLargeScheduleSelectorVisuals();
}

QString MainWindow::largeScheduleColumnLabel(int columnIndex) const
{
    if (columnIndex < 0)
        return QString();
    QString label;
    int index = columnIndex;
    do
    {
        int remainder = index % 26;
        label.prepend(QChar('A' + remainder));
        index = index / 26 - 1;
    } while (index >= 0);
    return label;
}

void MainWindow::removeEmptyLargeScheduleRows()
{
    if (!largeScheduleTable)
        return;
    int rowCount = largeScheduleTable->rowCount();
    const int columnCount = largeScheduleTable->columnCount();
    if (rowCount <= 1 || columnCount <= 1)
        return;

    QVector<bool> rowHasData(rowCount, false);
    for (int r = 1; r < rowCount; ++r)
    {
        for (int c = 1; c < columnCount; ++c)
        {
            QTableWidgetItem *item = largeScheduleTable->item(r, c);
            if (!item)
                continue;
            const QString text = item->text().trimmed();
            const QColor bg = item->background().color();
            if (text.isEmpty() && (!bg.isValid() || bg.alpha() == 0))
                continue;
            const int spanRows = qMax(1, largeScheduleTable->rowSpan(r, c));
            for (int offset = 0; offset < spanRows && r + offset < rowCount; ++offset)
                rowHasData[r + offset] = true;
        }
    }

    for (int r = rowCount - 1; r >= 1; --r)
    {
        if (!rowHasData.value(r))
            largeScheduleTable->removeRow(r);
    }
    if (largeScheduleTable->rowCount() == 0)
        largeScheduleTable->setRowCount(1);
    updateLargeScheduleRowLabels();
    clearLargeScheduleRowSelections();
}

void MainWindow::removeEmptyLargeScheduleColumns()
{
    if (!largeScheduleTable)
        return;
    const int columnCount = largeScheduleTable->columnCount();
    const int rowCount = largeScheduleTable->rowCount();
    if (columnCount <= 1 || rowCount <= 1)
        return;

    QVector<bool> columnHasData(columnCount, false);
    for (int r = 1; r < rowCount; ++r)
    {
        for (int c = 1; c < columnCount; ++c)
        {
            QTableWidgetItem *item = largeScheduleTable->item(r, c);
            if (!item)
                continue;
            const QString text = item->text().trimmed();
            const QColor bg = item->background().color();
            if (text.isEmpty() && (!bg.isValid() || bg.alpha() == 0))
                continue;
            const int spanCols = qMax(1, largeScheduleTable->columnSpan(r, c));
            for (int offset = 0; offset < spanCols && c + offset < columnCount; ++offset)
                columnHasData[c + offset] = true;
        }
    }

    for (int c = columnCount - 1; c >= 1; --c)
    {
        if (!columnHasData.value(c))
        {
            largeScheduleTable->removeColumn(c);
            QSet<int> updated;
            for (int existing : largeScheduleSelectedColumns)
            {
                if (existing == c)
                    continue;
                updated.insert(existing > c ? existing - 1 : existing);
            }
            largeScheduleSelectedColumns = updated;
        }
    }
    if (largeScheduleTable->columnCount() == 0)
        largeScheduleTable->setColumnCount(1);
    largeScheduleSelectorAnchorColumn = -1;
    largeScheduleSelectorLastColumn = -1;
    updateLargeScheduleColumnLabels();
    clearLargeScheduleColumnSelections();
}

void MainWindow::adjustLargeScheduleCellSizes()
{
    if (!largeScheduleTable)
        return;
    QWidget *viewport = largeScheduleTable->viewport();
    if (!viewport)
        return;

    const int columnCount = largeScheduleTable->columnCount();
    const int rowCount = largeScheduleTable->rowCount();
    QVector<int> visibleColumns;
    visibleColumns.reserve(columnCount);
    for (int c = 0; c < columnCount; ++c)
    {
        if (!largeScheduleTable->isColumnHidden(c))
            visibleColumns.append(c);
    }
    QVector<int> visibleRows;
    visibleRows.reserve(rowCount);
    for (int r = 0; r < rowCount; ++r)
    {
        if (!largeScheduleTable->isRowHidden(r))
            visibleRows.append(r);
    }
    const int viewportWidth = viewport->width();
    const int viewportHeight = viewport->height();

    const int visibleColumnCount = visibleColumns.size();
    if (visibleColumnCount > 0 && viewportWidth > 0)
    {
        const int totalMinWidth = kLargeScheduleMinColumnWidth * visibleColumnCount;
        if (viewportWidth >= totalMinWidth)
        {
            int baseWidth = viewportWidth / visibleColumnCount;
            baseWidth = qMax(baseWidth, kLargeScheduleMinColumnWidth);
            int usedWidth = baseWidth * visibleColumnCount;
            int remainder = viewportWidth - usedWidth;
            for (int idx = 0; idx < visibleColumnCount; ++idx)
            {
                int c = visibleColumns.at(idx);
                int width = baseWidth;
                if (largeScheduleTable->horizontalHeader())
                    width = qMax(width, largeScheduleTable->horizontalHeader()->sectionSizeHint(c));
                if (remainder > 0)
                {
                    ++width;
                    --remainder;
                }
                largeScheduleTable->setColumnWidth(c, width);
            }
        }
        else
        {
            for (int idx = 0; idx < visibleColumnCount; ++idx)
            {
                int c = visibleColumns.at(idx);
                int width = kLargeScheduleMinColumnWidth;
                if (largeScheduleTable->horizontalHeader())
                    width = qMax(width, largeScheduleTable->horizontalHeader()->sectionSizeHint(c));
                largeScheduleTable->setColumnWidth(c, width);
            }
        }
    }

    const int visibleRowCount = visibleRows.size();
    if (visibleRowCount > 0 && viewportHeight > 0)
    {
        const int totalMinHeight = kLargeScheduleMinRowHeight * visibleRowCount;
        if (viewportHeight >= totalMinHeight)
        {
            int baseHeight = viewportHeight / visibleRowCount;
            baseHeight = qMax(baseHeight, kLargeScheduleMinRowHeight);
            int usedHeight = baseHeight * visibleRowCount;
            int remainder = viewportHeight - usedHeight;
            for (int idx = 0; idx < visibleRowCount; ++idx)
            {
                int r = visibleRows.at(idx);
                int height = baseHeight;
                if (largeScheduleTable->verticalHeader())
                    height = qMax(height, largeScheduleTable->verticalHeader()->sectionSizeHint(r));
                if (remainder > 0)
                {
                    ++height;
                    --remainder;
                }
                largeScheduleTable->setRowHeight(r, height);
            }
        }
        else
        {
            for (int idx = 0; idx < visibleRowCount; ++idx)
            {
                int r = visibleRows.at(idx);
                int height = kLargeScheduleMinRowHeight;
                if (largeScheduleTable->verticalHeader())
                    height = qMax(height, largeScheduleTable->verticalHeader()->sectionSizeHint(r));
                largeScheduleTable->setRowHeight(r, height);
            }
        }
    }
}

void MainWindow::adjustTeacherRosterCellSizes()
{
    if (!teacherRosterTable)
        return;
    QWidget *viewport = teacherRosterTable->viewport();
    if (!viewport)
        return;

    const int columnCount = teacherRosterTable->columnCount();
    const int totalRows = teacherRosterTable->rowCount();
    QVector<int> visibleRows;
    visibleRows.reserve(totalRows);
    for (int r = 0; r < totalRows; ++r)
    {
        if (!teacherRosterTable->isRowHidden(r))
            visibleRows.append(r);
    }
    const int visibleRowCount = visibleRows.size();
    const int viewportWidth = viewport->width();
    const int viewportHeight = viewport->height();

    if (columnCount > 0 && viewportWidth > 0)
    {
        const int totalMinWidth = kTeacherRosterMinColumnWidth * columnCount;
        if (viewportWidth >= totalMinWidth)
        {
            int baseWidth = viewportWidth / columnCount;
            baseWidth = qMax(baseWidth, kTeacherRosterMinColumnWidth);
            int usedWidth = baseWidth * columnCount;
            int remainder = viewportWidth - usedWidth;
            for (int c = 0; c < columnCount; ++c)
            {
                int width = baseWidth;
                if (teacherRosterTable->horizontalHeader())
                    width = qMax(width, teacherRosterTable->horizontalHeader()->sectionSizeHint(c));
                if (remainder > 0)
                {
                    ++width;
                    --remainder;
                }
                teacherRosterTable->setColumnWidth(c, width);
            }
        }
        else
        {
            for (int c = 0; c < columnCount; ++c)
            {
                int width = kTeacherRosterMinColumnWidth;
                if (teacherRosterTable->horizontalHeader())
                    width = qMax(width, teacherRosterTable->horizontalHeader()->sectionSizeHint(c));
                teacherRosterTable->setColumnWidth(c, width);
            }
        }
    }

    if (visibleRowCount > 0 && viewportHeight > 0)
    {
        const int totalMinHeight = kTeacherRosterMinRowHeight * visibleRowCount;
        if (viewportHeight >= totalMinHeight)
        {
            int baseHeight = viewportHeight / visibleRowCount;
            baseHeight = qMax(baseHeight, kTeacherRosterMinRowHeight);
            int usedHeight = baseHeight * visibleRowCount;
            int remainder = viewportHeight - usedHeight;
            for (int idx = 0; idx < visibleRowCount; ++idx)
            {
                int r = visibleRows.at(idx);
                int height = baseHeight;
                if (teacherRosterTable->verticalHeader())
                    height = qMax(height, teacherRosterTable->verticalHeader()->sectionSizeHint(r));
                if (remainder > 0)
                {
                    ++height;
                    --remainder;
                }
                teacherRosterTable->setRowHeight(r, height);
            }
        }
        else
        {
            for (int idx = 0; idx < visibleRowCount; ++idx)
            {
                int r = visibleRows.at(idx);
                int height = kTeacherRosterMinRowHeight;
                if (teacherRosterTable->verticalHeader())
                    height = qMax(height, teacherRosterTable->verticalHeader()->sectionSizeHint(r));
                teacherRosterTable->setRowHeight(r, height);
            }
        }
    }
}

void MainWindow::toggleLargeScheduleSelectorsVisibility()
{
    largeScheduleSelectorsVisible = !largeScheduleSelectorsVisible;
    if (!largeScheduleTable)
        return;
    largeScheduleTable->setRowHidden(0, !largeScheduleSelectorsVisible);
    largeScheduleTable->setColumnHidden(0, !largeScheduleSelectorsVisible);
    adjustLargeScheduleCellSizes();
}

void MainWindow::setConflictDetectionEnabled(bool enabled)
{
    if (conflictDetectionEnabled == enabled)
        return;
    conflictDetectionEnabled = enabled;
    if (conflictDetectionButton)
        conflictDetectionButton->setChecked(enabled);
    if (!enabled)
    {
        clearConflictHighlights();
    }
    else if (scheduleTable)
    {
        int row = scheduleTable->currentRow();
        int column = scheduleTable->currentColumn();
        if (row >= 0 && column >= 0)
            checkTeacherConflicts(row, column);
    }
}

void MainWindow::configureLargeScheduleColors()
{
    ensureLargeScheduleDialog();
    QDialog dialog(largeScheduleDialog ? largeScheduleDialog : this);
    dialog.setWindowTitle(QString::fromUtf8("配置颜色"));
    dialog.resize(360, 380);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(12);

    auto scrollArea = new QScrollArea(&dialog);
    scrollArea->setWidgetResizable(true);
    auto rowsContent = new QWidget(scrollArea);
    auto rowsLayout = new QVBoxLayout(rowsContent);
    rowsLayout->setContentsMargins(0, 0, 0, 0);
    rowsLayout->setSpacing(10);

    auto buildColorSwatch = [](const QColor &color) {
        QWidget *swatch = new QWidget();
        swatch->setFixedSize(32, 32);
        swatch->setStyleSheet(QString("border-radius:6px; border:1px solid #e5e7eb; background:%1;").arg(color.isValid() ? color.name(QColor::HexRgb) : QString("#f4f4f5")));
        return swatch;
    };

    auto createRow = [this, rowsLayout, rowsContent, buildColorSwatch](const QString &name, const QColor &color) {
        QHBoxLayout *rowLayout = new QHBoxLayout();
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(10);

        QLineEdit *nameEdit = new QLineEdit(name, rowsContent);
        nameEdit->setPlaceholderText(QString::fromUtf8("科目名称"));
        nameEdit->setMinimumWidth(140);

        QWidget *colorSwatch = buildColorSwatch(color);
        QPushButton *colorButton = new QPushButton(QString::fromUtf8("🎨"), rowsContent);
        colorButton->setFixedSize(40, 36);
        colorButton->setProperty("color", color);
        colorButton->setToolTip(QString::fromUtf8("点击以选择颜色"));
        colorButton->setStyleSheet("QPushButton{border:1px solid transparent; border-radius:6px;} QPushButton:hover{border-color:#94a3b8;}");

        auto applyColor = [this, colorButton, colorSwatch, nameEdit](const QColor &newColor) {
            colorButton->setProperty("color", newColor);
            colorSwatch->setStyleSheet(QString("border-radius:6px; border:1px solid #e5e7eb; background:%1;").arg(newColor.isValid() ? newColor.name(QColor::HexRgb) : QString("#f4f4f5")));
            QString newName = nameEdit->text().trimmed();
            if (!newName.isEmpty() && newColor.isValid())
            {
                courseColorMap[newName] = newColor;
                applyCourseColorsToLargeSchedule();
                updateQuickCourseButtons();
            }
        };

        connect(colorButton, &QPushButton::clicked, this, [colorButton, applyColor]() {
            QColor current = colorButton->property("color").value<QColor>();
            QColorDialog dialog(current.isValid() ? current : QColor("#ffffff"));
            dialog.setOption(QColorDialog::NoButtons);
            dialog.setWindowTitle(QString::fromUtf8("选择颜色"));
            connect(&dialog, &QColorDialog::currentColorChanged, applyColor);
            dialog.exec();
        });

        connect(nameEdit, &QLineEdit::editingFinished, this, [this, nameEdit, colorButton]() {
            QString newName = nameEdit->text().trimmed();
            QColor color = colorButton->property("color").value<QColor>();
            if (!newName.isEmpty() && color.isValid())
            {
                courseColorMap.remove(nameEdit->property("originalName").toString());
                nameEdit->setProperty("originalName", newName);
                courseColorMap[newName] = color;
                applyCourseColorsToLargeSchedule();
                updateQuickCourseButtons();
            }
        });
        nameEdit->setProperty("originalName", name);

        rowLayout->addWidget(nameEdit, 1);
        rowLayout->addWidget(colorSwatch);
        rowLayout->addWidget(colorButton);
        const int insertIndex = qMax(0, rowsLayout->count() - 1);
        rowsLayout->insertLayout(insertIndex, rowLayout);
    };

    for (auto it = courseColorMap.constBegin(); it != courseColorMap.constEnd(); ++it)
        createRow(it.key(), it.value());

    rowsLayout->addStretch();
    scrollArea->setWidget(rowsContent);
    layout->addWidget(scrollArea, 1);

    QPushButton *addButton = new QPushButton(QString::fromUtf8("添加科目"), &dialog);
    addButton->setCursor(Qt::PointingHandCursor);
    addButton->setStyleSheet("QPushButton{padding:6px 12px; border-radius:6px; background:#2563eb; color:white;} QPushButton:hover{background:#1d4ed8;}");
    connect(addButton, &QPushButton::clicked, this, [this, createRow]() {
        QString baseName = QString::fromUtf8("新科目");
        int suffix = 1;
        QString candidate = baseName;
        while (courseColorMap.contains(candidate))
        {
            ++suffix;
            candidate = baseName + QString::number(suffix);
        }
        QColor defaultColor("#94a3b8");
        courseColorMap.insert(candidate, defaultColor);
        createRow(candidate, defaultColor);
    });
    layout->addWidget(addButton, 0, Qt::AlignLeft);
    dialog.exec();
}

void MainWindow::applyCourseColorsToLargeSchedule()
{
    if (!largeScheduleTable)
        return;
    const int rowCount = largeScheduleTable->rowCount();
    const int columnCount = largeScheduleTable->columnCount();
    for (int r = 1; r < rowCount; ++r)
    {
        for (int c = 1; c < columnCount; ++c)
        {
            if (QTableWidgetItem *item = largeScheduleTable->item(r, c))
            {
                const QString courseName = item->text().trimmed();
                if (!courseName.isEmpty() && courseColorMap.contains(courseName))
                    item->setBackground(courseColorMap.value(courseName));
            }
        }
    }
}

void MainWindow::ensureTeacherAssociationsLoaded()
{
    if (associationDataLoaded)
        return;
    teacherAssignments.clear();
    QFile file(teacherAssociationStoragePath());
    if (!file.exists())
        return;
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonArray array;
    if (doc.isArray())
        array = doc.array();
    else if (doc.isObject())
    {
        QJsonObject root = doc.object();
        if (root.contains(QStringLiteral("teachers")))
            array = root.value(QStringLiteral("teachers")).toArray();
        else if (root.contains(QStringLiteral("associations")))
            array = root.value(QStringLiteral("associations")).toArray();
    }
    if (array.isEmpty())
        return;

    bool parsed = false;
    for (const QJsonValue &value : array)
    {
        QJsonObject obj = value.toObject();
        if (obj.contains(QStringLiteral("subjects")))
        {
            const QString teacher = obj.value(QStringLiteral("teacher")).toString();
            if (teacher.isEmpty())
                continue;
            QJsonArray subjectsArray = obj.value(QStringLiteral("subjects")).toArray();
            for (const QJsonValue &subjectValue : subjectsArray)
            {
                QJsonObject subjectObj = subjectValue.toObject();
                const QString subjectName = subjectObj.value(QStringLiteral("subject")).toString();
                if (subjectName.isEmpty())
                    continue;
                QJsonArray classArray = subjectObj.value(QStringLiteral("classes")).toArray();
                for (const QJsonValue &classValue : classArray)
                {
                    QString cls = normalizedClassIdentifier(classValue.toString());
                    if (!cls.isEmpty())
                        teacherAssignments[teacher].subjectToClasses[subjectName].insert(cls);
                }
            }
            parsed = true;
        }
        else
        {
            const QString teacher = obj.value(QStringLiteral("teacher")).toString();
            const QString subject = obj.value(QStringLiteral("subject")).toString();
            const QString cls = normalizedClassIdentifier(obj.value(QStringLiteral("class")).toString());
            if (teacher.isEmpty() || subject.isEmpty() || cls.isEmpty())
                continue;
            teacherAssignments[teacher].subjectToClasses[subject].insert(cls);
            parsed = true;
        }
    }
    associationDataLoaded = parsed;
}

void MainWindow::clearConflictHighlights()
{
    for (const ConflictHighlight &highlight : qAsConst(conflictHighlights))
    {
        if (highlight.item)
            highlight.item->setBackground(highlight.originalBrush);
    }
    conflictHighlights.clear();
    for (const TeacherRosterHighlight &highlight : qAsConst(teacherRosterHighlights))
    {
        if (highlight.item)
            highlight.item->setBackground(highlight.originalBrush);
    }
    teacherRosterHighlights.clear();
    for (const TeacherRosterIcon &icon : qAsConst(teacherRosterIcons))
    {
        if (icon.item)
            icon.item->setText(icon.originalText);
    }
    teacherRosterIcons.clear();
    if (statusBar())
        statusBar()->clearMessage();
}

void MainWindow::checkTeacherConflicts(int row, int column)
{
   qDebug()<<"checkTeacherConflicts:"<<row<<"     "<<column;
   QString week="";
   int i=column;
   QString claNum="";
   while(true){
       if(largeScheduleTable->item(1,i)->text().isEmpty()){
           i--;
       }else {
           break;
       }
   }
   week=largeScheduleTable->item(1,i)->text();
   claNum=largeScheduleTable->item(2,column)->text().trimmed();
   int rosterRowCount=teacherRosterTable->rowCount();
   int rowNum=0;

   // 清理上一次的覆盖层与背景色
   if (teacherRosterTable)
   {
       QWidget *viewport = teacherRosterTable->viewport();
       if (QWidget *old = viewport->findChild<QWidget *>("teacherRosterResultOverlay"))
       {
           bool okTarget = false;
           bool okSource = false;
           int prevTargetRow = old->property("targetRow").toInt(&okTarget);
           int prevSourceRow = old->property("sourceRow").toInt(&okSource);
           old->deleteLater();
           auto clearRowBrush = [this](int r) {
               if (!teacherRosterTable || r < 0 || r >= teacherRosterTable->rowCount())
                   return;
               for (int c = 0; c < teacherRosterTable->columnCount(); ++c)
               {
                   if (QTableWidgetItem *cell = teacherRosterTable->item(r, c))
                       cell->setBackground(QBrush());
               }
           };
           if (okTarget)
               clearRowBrush(prevTargetRow);
           if (okSource)
               clearRowBrush(prevSourceRow);
       }
       else
       {
           // 无覆盖层时，也先尝试清理之前可能残留的背景色
           const int rows = teacherRosterTable->rowCount();
           const int cols = teacherRosterTable->columnCount();
           for (int r = 0; r < rows; ++r)
           {
               for (int c = 0; c < cols; ++c)
                   if (QTableWidgetItem *cell = teacherRosterTable->item(r, c))
                       cell->setBackground(QBrush());
           }
       }
   }

   for (int i = 0; i < rosterRowCount; ++i) {
       auto cTempName= teacherRosterTable->verticalHeaderItem(i)->text().trimmed()+"班";
       qDebug()<<"cTempName "<<cTempName<<" claNum "<<claNum;
       if(cTempName==claNum){
           rowNum=i;
           break;
       }
   }
    qDebug()<<"week: "<<week<<"rosterRowCount "<<rosterRowCount<<"claNum: "<<claNum;
   int confState=0; //0代表不冲突，1代表冲突
   const int rosterColumns = teacherRosterTable->columnCount();
   QVector<QString> resultTexts(rosterColumns);
   QVector<QColor> resultColors(rosterColumns);
   for (int var = 0; var < rosterColumns; ++var) {
       QTableWidgetItem *teacherItem = teacherRosterTable->item(rowNum,var);
       if (!teacherItem)
           continue;
       auto teachName= teacherItem->text().trimmed();
       const QString subjectHeader = teacherRosterTable->horizontalHeaderItem(var) ? teacherRosterTable->horizontalHeaderItem(var)->text().trimmed() : QString();
       qDebug()<<"检测冲突teachName: "<<teachName;
       auto subjectToClasses= teacherAssignments[teachName].subjectToClasses;
        qDebug()<<"subjectToClasses: "<<subjectToClasses;
       for(auto subTt=subjectToClasses.constBegin(); subTt!=subjectToClasses.constEnd();++subTt){
           qDebug()<<"subTt: "<<subTt.key();
           const QSet<QString> &classes=subTt.value();
           for(const QString & className:classes){
               QString classNameban=className+"班";
               qDebug()<<"classNameban: "<<classNameban;
               int teaClaclumn= classClumnNums[week][classNameban];
               QString itemName= largeScheduleTable->item(row,teaClaclumn)->text().trimmed();
               if(itemName==subTt.key()){
                   //冲突
                   qDebug()<<"冲突："<<teachName<<"("<<rowNum<<","<<var<<")";
                   confState=1;
                   break;
               }
           }
           if(confState==1){
               break;
           }
       }
       qDebug()<<teachName<<" "<<confState;

       //在任课教师一览表的rowNum行的上面创建一个水平方向布局的列表，列表宽等于任课教师一览表的宽，
       //高等于任课教师一览表的行高，且位置刚好覆盖rowNum上面的一行。并且也是可以随着窗口大小自动缩放的。
       //该列表共有与任课教师一览表的列数相同的项，每一项的宽高等于当时的单元格的宽高。列表项是一个label，
       //用来展示rowNum行中教师的检测结果。
       //检测不冲突的教师这一天共有几节课
       if(confState==0){
           int dayLessons = 0;
           if (largeScheduleTable && classClumnNums.contains(week) && !subjectHeader.isEmpty())
           {
               const QHash<QString, QSet<QString>> &map = teacherAssignments.value(teachName).subjectToClasses;
               const QSet<QString> classesForSubject = map.value(subjectHeader);
               const int schedRows = largeScheduleTable->rowCount();
               const int startRow = 3; // 跳过日期/班级头部
               for (const QString &cls : classesForSubject)
               {
                   const QString classKey = cls + QStringLiteral("班");
                   const int classCol = classClumnNums[week].value(classKey, -1);
                   if (classCol < 0)
                       continue;
                   for (int r = startRow; r < schedRows; ++r)
                   {
                       QTableWidgetItem *cell = largeScheduleTable->item(r, classCol);
                       if (!cell)
                           continue;
                       if (cell->text().trimmed() == subjectHeader)
                           dayLessons++;
                   }
               }
           }
           resultTexts[var] = QString::fromUtf8("%1节").arg(dayLessons);
           resultColors[var] = QColor("#dcfce7");
       }
       else
       {
           resultTexts[var] = QString::fromUtf8("冲突");
           resultColors[var] = QColor("#fee2e2");
       }
       confState=0;
   }

   // 构造覆盖在目标行“上一行”（若目标行为首行则使用第二行）上的结果列表
   if (teacherRosterTable && !resultTexts.isEmpty())
   {
       QWidget *viewport = teacherRosterTable->viewport();
       if (QWidget *old = viewport->findChild<QWidget *>("teacherRosterResultOverlay"))
           old->deleteLater();

       int targetRow = rowNum;
       if (rowNum == 0 && teacherRosterTable->rowCount() > 1)
           targetRow = 1;
       else if (rowNum > 0)
           targetRow = rowNum - 1;

       if (targetRow >= 0 && targetRow < teacherRosterTable->rowCount())
       {
           const int lastCol = teacherRosterTable->columnCount() - 1;
           if (lastCol >= 0)
           {
               QRect firstRect = teacherRosterTable->visualRect(teacherRosterTable->model()->index(targetRow, 0));
               QRect lastRect = teacherRosterTable->visualRect(teacherRosterTable->model()->index(targetRow, lastCol));
               QRect rowRect = firstRect.united(lastRect);
               QWidget *overlay = new QWidget(viewport);
               overlay->setObjectName("teacherRosterResultOverlay");
               overlay->setProperty("targetRow", targetRow);
               overlay->setProperty("sourceRow", rowNum);
               overlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
               overlay->setGeometry(rowRect);
               QHBoxLayout *hLayout = new QHBoxLayout(overlay);
               hLayout->setContentsMargins(0, 0, 0, 0);
               hLayout->setSpacing(0);
               for (int c = 0; c < teacherRosterTable->columnCount(); ++c)
               {
                   QLabel *label = new QLabel(resultTexts.value(c), overlay);
                   label->setAlignment(Qt::AlignCenter);
                   const int colWidth = teacherRosterTable->columnWidth(c);
                   label->setMinimumWidth(colWidth);
                   label->setMaximumWidth(colWidth);
                   label->setFixedHeight(teacherRosterTable->rowHeight(targetRow));
                   label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                   const QColor bg = resultColors.value(c, QColor("#e5e7eb"));
                   label->setStyleSheet(QString("background:%1; color:#111827; border:1px solid #e5e7eb;").arg(bg.name()));
                   hLayout->addWidget(label);
                   hLayout->setStretch(c, teacherRosterTable->columnWidth(c));
               }
               overlay->show();
           }
       }
   }

   // 为目标行单元格添加与结果列表相同的背景色
   if (teacherRosterTable && rowNum >= 0 && rowNum < teacherRosterTable->rowCount())
   {
       for (int c = 0; c < teacherRosterTable->columnCount(); ++c)
       {
           if (QTableWidgetItem *cell = teacherRosterTable->item(rowNum, c))
           {
               const QColor bg = resultColors.value(c, QColor("#e5e7eb"));
               cell->setBackground(bg);
           }
       }
   }

}

void MainWindow::handleAssociateTeacherRoster()
{
    ensureLargeScheduleDialog();
    if (!teacherRosterTable)
    {
        QMessageBox::warning(this, QString::fromUtf8("关联失败"), QString::fromUtf8("请先导入任课教师数据。"));
        return;
    }

    //周几班级列号
    auto columnCount= largeScheduleTable->columnCount();
    QString weekday="";
    for (int var =2; var < columnCount; ++var) {
        auto item= largeScheduleTable->item(1,var);
        if(!item->text().isEmpty()){
            weekday=item->text();
        }
        QString claName=largeScheduleTable->item(2,var)->text();
        classClumnNums[weekday][claName]=var;
    }
    qDebug()<<"classClumnNums: "<<classClumnNums;
    // 持久化班级列号索引
    {
        QJsonObject root;
        for (auto wIt = classClumnNums.constBegin(); wIt != classClumnNums.constEnd(); ++wIt)
        {
            QJsonObject classObj;
            const auto &clsMap = wIt.value();
            for (auto cIt = clsMap.constBegin(); cIt != clsMap.constEnd(); ++cIt)
                classObj.insert(cIt.key(), cIt.value());
            root.insert(wIt.key(), classObj);
        }
        QFile f(classColumnIndexStoragePath());
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
            f.close();
        }
    }
    //

    const int rosterRowCount = teacherRosterTable->rowCount();
    const int rosterColumnCount = teacherRosterTable->columnCount();
    qDebug()<<"rosterRowCount "<<rosterRowCount<<"|| rosterColumnCount "<<rosterColumnCount;
    auto item= teacherRosterTable->item(1,2);
    qDebug()<<"teacherRosterTable : "<< item->text()<<"  "<<teacherRosterTable->horizontalHeaderItem(2)->text();
    if (rosterRowCount == 0 || rosterColumnCount == 0)
    {
        QMessageBox::information(this, QString::fromUtf8("关联失败"), QString::fromUtf8("任课教师数据为空。"));
        return;
    }

    teacherAssignments.clear();
    for (int row = 0; row < rosterRowCount; ++row)
    {
        if (teacherRosterTable->isRowHidden(row))
            continue;
        QTableWidgetItem *classItem = teacherRosterTable->verticalHeaderItem(row);
        const QString classLabel = classItem ? classItem->text().trimmed() : QString();
        const QString classId = normalizedClassIdentifier(classLabel);
        if (classId.isEmpty())
            continue;
        for (int col = 0; col < rosterColumnCount; ++col)
        {
            QTableWidgetItem *subjectHeader = teacherRosterTable->horizontalHeaderItem(col);
            const QString subject = subjectHeader ? subjectHeader->text().trimmed() : QString();
            if (subject.isEmpty())
                continue;
            QTableWidgetItem *teacherItem = teacherRosterTable->item(row, col);
            const QString teacherName = teacherItem ? teacherItem->text().trimmed() : QString();
            if (teacherName.isEmpty())
                continue;
            teacherAssignments[teacherName].subjectToClasses[subject].insert(classId);
        }
    }
    associationDataLoaded = true;

    QJsonArray teacherArray;
    for (auto teacherIt = teacherAssignments.constBegin(); teacherIt != teacherAssignments.constEnd(); ++teacherIt)
    {
        QJsonObject teacherObj;
        teacherObj.insert(QStringLiteral("teacher"), teacherIt.key());
        QJsonArray subjectArray;
        const auto &subjects = teacherIt.value().subjectToClasses;
        for (auto subjectIt = subjects.constBegin(); subjectIt != subjects.constEnd(); ++subjectIt)
        {
            QJsonObject subjectObj;
            subjectObj.insert(QStringLiteral("subject"), subjectIt.key());
            QJsonArray classArray;
            for (const QString &cls : subjectIt.value())
                classArray.append(cls);
            subjectObj.insert(QStringLiteral("classes"), classArray);
            subjectArray.append(subjectObj);
        }
        teacherObj.insert(QStringLiteral("subjects"), subjectArray);
        teacherArray.append(teacherObj);
    }

    QFile file(teacherAssociationStoragePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QJsonObject root;
        root.insert(QStringLiteral("teachers"), teacherArray);
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }

    QMessageBox::information(this,
                             QString::fromUtf8("关联完成"),
                             QString::fromUtf8("已生成 %1 名教师关联信息。").arg(teacherAssignments.size()));
}

QString MainWindow::teacherAssociationStoragePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::currentPath();
    QDir directory(dir);
    if (!directory.exists())
        directory.mkpath(".");
    return directory.filePath(QStringLiteral("teacher_associations.json"));
}

QString MainWindow::classColumnIndexStoragePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::currentPath();
    QDir directory(dir);
    if (!directory.exists())
        directory.mkpath(".");
    return directory.filePath(QStringLiteral("class_columns.json"));
}

void MainWindow::showStatisticsDialog()
{
    if (!statisticsDialog)
    {
        statisticsDialog = new QDialog(this);
        statisticsDialog->setWindowTitle(QString::fromUtf8("统计"));
        statisticsDialog->setFixedSize(900, 600);
        QVBoxLayout *layout = new QVBoxLayout(statisticsDialog);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(12);

        QLabel *title = new QLabel(QString::fromUtf8("统计信息"), statisticsDialog);
        title->setStyleSheet("font-weight:600; font-size:16px;");
        layout->addWidget(title);

        QHBoxLayout *rangeLayout = new QHBoxLayout();
        rangeLayout->setContentsMargins(0, 0, 0, 0);
        rangeLayout->setSpacing(8);
        rangeLayout->addWidget(new QLabel(QString::fromUtf8("开始日期"), statisticsDialog));
        QDateEdit *startEdit = new QDateEdit(QDate::currentDate(), statisticsDialog);
        startEdit->setCalendarPopup(true);
        startEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
        rangeLayout->addWidget(startEdit);
        rangeLayout->addWidget(new QLabel(QString::fromUtf8("结束日期"), statisticsDialog));
        QDateEdit *endEdit = new QDateEdit(QDate::currentDate().addDays(7), statisticsDialog);
        endEdit->setCalendarPopup(true);
        endEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
        rangeLayout->addWidget(endEdit);
        rangeLayout->addStretch();
        layout->addLayout(rangeLayout);

        QTabWidget *tabWidget = new QTabWidget(statisticsDialog);
        tabWidget->setDocumentMode(true);
        tabWidget->setTabPosition(QTabWidget::North);

        auto buildTab = [this](const QString &title, QtCharts::QChartView *&chartView, QVBoxLayout *&listLayout, QLabel *&evalLabel, QLabel *&sumLabel) -> QWidget * {
            Q_UNUSED(title);
            QWidget *tab = new QWidget;
            QHBoxLayout *metricsLayout = new QHBoxLayout(tab);
            metricsLayout->setContentsMargins(0, 8, 0, 8);
            metricsLayout->setSpacing(12);

            QWidget *leftCard = new QWidget(tab);
            leftCard->setStyleSheet("QWidget{border:1px solid #e5e7eb; border-radius:10px; background:#f8fafc;}");
            QVBoxLayout *leftLayout = new QVBoxLayout(leftCard);
            leftLayout->setContentsMargins(12, 12, 12, 12);
            leftLayout->setSpacing(6);
            QHBoxLayout *leftHeader = new QHBoxLayout();
            leftHeader->setContentsMargins(0, 0, 0, 0);
            leftHeader->setSpacing(6);
            QLabel *leftTitle = new QLabel(QString::fromUtf8("节次统计"), leftCard);
            leftTitle->setStyleSheet("font-weight:600; color:#0f172a;");
            sumLabel = new QLabel(QString::fromUtf8("共计：0节"), leftCard);
            sumLabel->setObjectName("lessonSumLabel");
            sumLabel->setStyleSheet("color:#475569; font-weight:600;");
            leftHeader->addWidget(leftTitle);
            leftHeader->addStretch();
            leftHeader->addWidget(sumLabel);
            leftLayout->addLayout(leftHeader);
            listLayout = new QVBoxLayout();
            listLayout->setContentsMargins(0, 4, 0, 4);
            listLayout->setSpacing(4);
            leftLayout->addLayout(listLayout);
            leftLayout->addStretch();

            QWidget *rightCard = new QWidget(tab);
            rightCard->setStyleSheet("QWidget{border:1px solid #e5e7eb; border-radius:10px; background:#f8fafc;}");
            QVBoxLayout *rightLayout = new QVBoxLayout(rightCard);
            rightLayout->setContentsMargins(12, 12, 12, 12);
            rightLayout->setSpacing(6);
            QHBoxLayout *titleRow = new QHBoxLayout();
            titleRow->setContentsMargins(0, 0, 0, 0);
            titleRow->setSpacing(8);
            QLabel *rightTitle = new QLabel(QString::fromUtf8("课程分布"), rightCard);
            rightTitle->setStyleSheet("font-weight:600; color:#0f172a;");
            titleRow->addWidget(rightTitle);
            titleRow->addStretch();
            evalLabel = new QLabel(rightCard);
            evalLabel->setObjectName("chartEvalLabel");
            evalLabel->setStyleSheet("color:#ef4444; font-weight:600;");
            titleRow->addWidget(evalLabel, 0, Qt::AlignVCenter | Qt::AlignRight);
            rightLayout->addLayout(titleRow);

            chartView = new QtCharts::QChartView(rightCard);
            chartView->setMinimumHeight(380);
            chartView->setMinimumWidth(520);
            chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            rightLayout->addWidget(chartView, 1);

            metricsLayout->addWidget(leftCard, 1);
            metricsLayout->addWidget(rightCard, 4);
            metricsLayout->setStretch(0, 1);
            metricsLayout->setStretch(1, 4);
            return tab;
        };

        QWidget *actualTab = buildTab(QString::fromUtf8("实际课程"), statisticsChartViewActual, statisticsListLayoutActual, statisticsEvaluationLabelActual, statisticsLessonSumLabelActual);
        QWidget *baseTab = buildTab(QString::fromUtf8("原课程"), statisticsChartViewBase, statisticsListLayoutBase, statisticsEvaluationLabelBase, statisticsLessonSumLabelBase);
        tabWidget->addTab(actualTab, QString::fromUtf8("实际课程"));
        tabWidget->addTab(baseTab, QString::fromUtf8("原课程"));
        layout->addWidget(tabWidget, 1);

        auto countCoursesInRange = [this](const QDate &start, const QDate &end, bool actual, QVector<int> *lessonCountsOut, QMap<QString, int> *courseCountsOut, int *preferredOut, int *otherOut) -> int {
            const BaseScheduleEntry *base = activeBaseSchedule();
            if (!base)
                return 0;
            const int colCount = base->columns > 0 ? base->columns : base->courseCells.isEmpty() ? 0 : base->courseCells.first().size();
            const int rowCount = base->courseCells.size();
            if (colCount <= 0 || rowCount <= 0)
                return 0;

            QVector<int> lessonCounts(rowCount, 0);
            QMap<QString, int> courseCounts;
            int preferred = 0;
            int other = 0;

            auto resolveName = [this, base, actual, colCount](int r, int c, const QDate &weekStart) -> QString {
                QString baseName;
                if (r < base->courseCells.size() && c < base->courseCells.at(r).size())
                    baseName = base->courseCells.at(r).at(c).name.trimmed();
                if (!actual)
                    return baseName;

                const ActualWeekEntry *entry = findActualWeek(weekStart);
                if (entry)
                {
                    auto rowIt = entry->cellOverrides.constFind(r);
                    if (rowIt != entry->cellOverrides.constEnd())
                    {
                        auto colIt = rowIt->constFind(c);
                        if (colIt != rowIt->constEnd())
                        {
                            const CourseCellData &data = colIt.value();
                            if (data.type == ScheduleCellLabel::CourseType::Deleted)
                                return QString();
                            if (!data.name.trimmed().isEmpty())
                                return data.name.trimmed();
                        }
                    }
                }
                return baseName;
            };

            auto isPlaceholderRow = [base](int r) -> bool {
                return r < base->rowDefinitions.size() &&
                       base->rowDefinitions.at(r).kind == MainWindow::RowDefinition::Kind::Placeholder;
            };

            int count = 0;
            QDate current = start;
            while (current <= end)
            {
                const QDate weekStart = mondayOf(current);
                const int dayIdx = current.dayOfWeek() - 1; // 0-based
                if (dayIdx < 0 || dayIdx >= colCount)
                {
                    current = current.addDays(1);
                    continue;
                }
                for (int r = 0; r < rowCount; ++r)
                {
                    if (isPlaceholderRow(r))
                        continue;
                    QString name = resolveName(r, dayIdx, weekStart);
                    if (!name.trimmed().isEmpty())
                    {
                        ++count;
                        lessonCounts[r]++;
                        courseCounts[name]++;
                        const QString timeLabel = (r < base->timeColumnTexts.size()) ? base->timeColumnTexts.at(r) : QString();
                        QString startTimeText = timeLabel.split('\n').value(1);
                        if (startTimeText.isEmpty())
                            startTimeText = lessonTimes.value(r);
                        const QTime startTime = QTime::fromString(startTimeText.left(5), "HH:mm");
                        if (startTime.isValid() && ((startTime.hour() >= 8 && startTime.hour() < 11) || (startTime.hour() >= 15 && startTime.hour() < 16)))
                            preferred++;
                        else
                            other++;
                    }
                }
                current = current.addDays(1);
            }

            if (lessonCountsOut)
                *lessonCountsOut = lessonCounts;
            if (courseCountsOut)
                *courseCountsOut = courseCounts;
            if (preferredOut)
                *preferredOut = preferred;
            if (otherOut)
                *otherOut = other;
            return count;
        };

        std::function<void()> updateStats = [this, startEdit, endEdit, countCoursesInRange]() {
            QDate s = startEdit->date();
            QDate e = endEdit->date();
            if (e < s)
                std::swap(s, e);
            QVector<int> lessonCounts;
            QMap<QString, int> courseCounts;
            int preferred = 0;
            int other = 0;
            QVector<int> lessonCountsBase;
            QMap<QString, int> courseCountsBase;
            int preferredBase = 0;
            int otherBase = 0;
            int actualCount = countCoursesInRange(s, e, true, &lessonCounts, &courseCounts, &preferred, &other);
            int baseCount = countCoursesInRange(s, e, false, &lessonCountsBase, &courseCountsBase, &preferredBase, &otherBase);
            QDialog *dlg = statisticsDialog;
            // 更新节次列表
            auto updateLessonList = [this, dlg](QVBoxLayout *layout, const QVector<int> &counts) {
                if (!layout)
                    return;
                while (QLayoutItem *item = layout->takeAt(0))
                {
                    if (item->widget())
                        item->widget()->deleteLater();
                    delete item;
                }
                const BaseScheduleEntry *base = activeBaseSchedule();
                auto updateSum = [layout](QLabel *sumLabel) {
                    if (!layout || !sumLabel)
                        return;
                    int total = 0;
                    for (int i = 0; i < layout->count(); ++i)
                    {
                        QWidget *rowWidget = layout->itemAt(i)->widget();
                        if (!rowWidget)
                            continue;
                        if (QCheckBox *cb = rowWidget->findChild<QCheckBox *>("lessonToggle"))
                        {
                            bool ok = false;
                            int count = cb->property("lessonCount").toInt(&ok);
                            if (ok && cb->isChecked())
                                total += count;
                        }
                    }
                    sumLabel->setText(QString::fromUtf8("共计：%1节").arg(total));
                };

                QLabel *sumLabel = nullptr;
                // sum label 在左卡头部的布局中，已提前创建；从布局父级找到
                QWidget *parentWidget = layout->parentWidget();
                if (parentWidget)
                    sumLabel = parentWidget->findChild<QLabel *>("lessonSumLabel");

                for (int i = 0; i < counts.size(); ++i)
                {
                    QString title;
                    if (base && i < base->timeColumnTexts.size() && !base->timeColumnTexts.at(i).isEmpty())
                        title = base->timeColumnTexts.at(i).split('\n').first();
                    if (title.isEmpty())
                        title = QString::fromUtf8("第%1节").arg(i + 1);

                    QWidget *rowWidget = new QWidget(dlg);
                    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
                    rowLayout->setContentsMargins(0, 0, 0, 0);
                    rowLayout->setSpacing(6);

                    QCheckBox *toggle = new QCheckBox(rowWidget);
                    toggle->setObjectName("lessonToggle");
                    toggle->setChecked(true);
                    toggle->setProperty("lessonCount", counts.at(i));
                    QLabel *rowLabel = new QLabel(QString("%1：%2").arg(title).arg(counts.at(i)), dlg);
                    rowLabel->setStyleSheet("color:#0f172a;");

                    rowLayout->addWidget(toggle);
                    rowLayout->addWidget(rowLabel);
                    rowLayout->addStretch();
                    layout->addWidget(rowWidget);

                    QObject::connect(toggle, &QCheckBox::toggled, rowWidget, [updateSum, sumLabel]() {
                        updateSum(sumLabel);
                    });
                }
                updateSum(sumLabel);
            };
            updateLessonList(statisticsListLayoutActual, lessonCounts);
            updateLessonList(statisticsListLayoutBase, lessonCountsBase);

            // 更新柱状图
            auto updateChart = [this](QtCharts::QChartView *view, const QVector<int> &counts) {
                if (!view)
                    return;
                auto *series = new QtCharts::QBarSeries();
                QStringList categories;
                const QStringList colorPalette = {
                    "#3b82f6", "#f97316", "#10b981", "#f59e0b", "#8b5cf6", "#ef4444", "#0ea5e9", "#22c55e", "#ec4899"
                };
                const int lessonCountSize = counts.size();
                for (int i = 0; i < lessonCountSize; ++i)
                {
                    QString title;
                    const BaseScheduleEntry *base = activeBaseSchedule();
                    if (base && i < base->timeColumnTexts.size() && !base->timeColumnTexts.at(i).isEmpty())
                        title = base->timeColumnTexts.at(i).split('\n').first();
                    if (title.isEmpty())
                        title = QString::fromUtf8("第%1节").arg(i + 1);
                    categories << title;

                    auto *set = new QtCharts::QBarSet(title);
                    QVector<qreal> values(lessonCountSize, 0);
                    values[i] = counts.value(i);
                    for (qreal v : values)
                        set->append(v);
                    set->setColor(QColor(colorPalette.at(i % colorPalette.size())));
                    series->append(set);
                }

                auto *chart = new QtCharts::QChart();
                chart->addSeries(series);
                chart->setTitle(QString::fromUtf8("课程分布"));
                auto *axisX = new QtCharts::QBarCategoryAxis();
                axisX->append(categories);
                axisX->setLabelsAngle(-70);
                QFont axisFont = axisX->labelsFont();
                axisFont.setPointSize(9);
                axisFont.setBold(false);
                axisX->setLabelsFont(axisFont);
                axisX->setLabelsBrush(QBrush(QColor("#111827")));
                axisX->setGridLineVisible(false);
                chart->addAxis(axisX, Qt::AlignBottom);
                series->attachAxis(axisX);
                auto *axisY = new QtCharts::QValueAxis();
                axisY->setTitleText(QString::fromUtf8("课程数"));
                QFont axisYFont = axisFont;
                axisYFont.setPointSize(9);
                axisY->setLabelsFont(axisYFont);
                chart->addAxis(axisY, Qt::AlignLeft);
                series->attachAxis(axisY);
                chart->legend()->setVisible(false);
                chart->setMargins(QMargins(6, 8, 6, 18));
                view->setChart(chart);
                if (QLabel *evalLabel = qobject_cast<QLabel *>(view->parentWidget()->findChild<QLabel *>("chartEvalLabel")))
                {
                    evalLabel->raise();
                }
            };
            updateChart(statisticsChartViewActual, lessonCounts);
            updateChart(statisticsChartViewBase, lessonCountsBase);

            // 分布评价
            auto updateEval = [](QLabel *label, int preferredVal, int otherVal) {
                if (!label)
                    return;
                const bool coreMore = preferredVal >= otherVal;
                QString eval = coreMore ? QString::fromUtf8("课程主要分布在核心时段") : QString::fromUtf8("非核心时段课程较多");
                QString color = coreMore ? "#16a34a" : "#ef4444";
                label->setStyleSheet(QString("color:%1; font-weight:600;").arg(color));
                label->setText(eval);
            };
            updateEval(statisticsEvaluationLabelActual, preferred, other);
            updateEval(statisticsEvaluationLabelBase, preferredBase, otherBase);
        };
        connect(startEdit, &QDateEdit::dateChanged, statisticsDialog, [updateStats](const QDate &) { updateStats(); });
        connect(endEdit, &QDateEdit::dateChanged, statisticsDialog, [updateStats](const QDate &) { updateStats(); });
        updateStats();
    }
    statisticsDialog->show();
    statisticsDialog->raise();
    statisticsDialog->activateWindow();
}

void MainWindow::positionTeacherRosterDock()
{
    if (!largeScheduleDialog || !teacherRosterDock)
        return;
    if (!teacherRosterDock->isFloating())
        return;
    QSize dockSize = teacherRosterDock->size();
    if (dockSize.width() <= 0 || dockSize.height() <= 0)
        dockSize = QSize(260, largeScheduleDialog->height() * 0.8);
    const QRect hostGeom = largeScheduleDialog->frameGeometry();
    QPoint targetPos(hostGeom.left() - dockSize.width() - 16, hostGeom.top());
    teacherRosterDock->move(targetPos);
    teacherRosterDock->resize(dockSize);
}

void MainWindow::handleDeleteLargeScheduleColumns()
{
    if (!largeScheduleTable)
        return;
    QList<int> cols = largeScheduleSelectedColumns.values();
    QList<int> rows = largeScheduleSelectedRows.values();
    std::sort(cols.begin(), cols.end(), std::greater<int>());
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    if (cols.isEmpty() && rows.isEmpty())
    {
        QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("请先选择需要删除的行或列。"));
        return;
    }
    bool removed = false;
    for (int row : rows)
    {
        if (row >= 1 && row < largeScheduleTable->rowCount())
        {
            largeScheduleTable->removeRow(row);
            removed = true;
        }
    }
    for (int col : cols)
    {
        if (col >= 1 && col < largeScheduleTable->columnCount())
        {
            largeScheduleTable->removeColumn(col);
            removed = true;
        }
    }
    if (largeScheduleTable->rowCount() == 0)
        largeScheduleTable->setRowCount(1);
    if (largeScheduleTable->columnCount() == 0)
        largeScheduleTable->setColumnCount(1);
    if (removed)
    {
        updateLargeScheduleRowLabels();
        updateLargeScheduleColumnLabels();
        clearLargeScheduleRowSelections();
        clearLargeScheduleColumnSelections();
        adjustLargeScheduleCellSizes();
    }
}




void MainWindow::editTimeSlot(int row)
{
    if (!canEditSelectedBaseSchedule() || !scheduleTable || row <= 0)
        return;

    BaseScheduleEntry *baseEntry = activeBaseScheduleMutable();
    if (!baseEntry)
        return;

    const int dataRow = row - 1;
    while (baseEntry->timeColumnTexts.size() <= dataRow)
        baseEntry->timeColumnTexts.append(QString());

    QString currentText = baseEntry->timeColumnTexts.at(dataRow);
    if (currentText.trimmed().isEmpty())
    {
        if (QTableWidgetItem *item = scheduleTable->item(row, 0))
            currentText = item->text();
    }

    QStringList parts = currentText.split('\n');
    QString title = parts.value(0).trimmed();
    QString timeRange = parts.mid(1).join(QStringLiteral(" ")).trimmed();
    if (title.isEmpty())
        title = QString::fromUtf8("\xe7\xac\xac%1\xe8\x8a\x82").arg(row);

    if (!promptTimeSlotTexts(title, timeRange))
        return;

    QString normalizedTitle = title.trimmed();
    if (normalizedTitle.isEmpty())
        normalizedTitle = QString::fromUtf8("\xe7\xac\xac%1\xe8\x8a\x82").arg(row);
    QString normalizedTime = timeRange.trimmed();
    QString combined = normalizedTime.isEmpty() ? normalizedTitle : normalizedTitle + QStringLiteral("\n") + normalizedTime;

    baseEntry->timeColumnTexts[dataRow] = combined;
    if (QTableWidgetItem *item = scheduleTable->item(row, 0))
        item->setText(combined);

    markBaseScheduleDirty();
    refreshTimeColumn();
    refreshBaseScheduleList();
}

bool MainWindow::promptTimeSlotTexts(QString &title, QString &timeRange)
{
    QDialog dialog(this);
    dialog.setWindowTitle(QString::fromUtf8("\xe7\xbc\x96\xe8\xbe\x91\xe6\x97\xb6\xe9\x97\xb4\xe5\x8d\x95\xe5\x85\x83"));
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();
    QLineEdit *titleEdit = new QLineEdit(title, &dialog);
    QLineEdit *timeEdit = new QLineEdit(timeRange, &dialog);
    formLayout->addRow(QString::fromUtf8("\xe8\x8a\x82\xe6\xac\xa1\xe5\x90\x8d\xe7\xa7\xb0"), titleEdit);
    formLayout->addRow(QString::fromUtf8("\xe6\x97\xb6\xe9\x97\xb4\xe6\xae\xb5"), timeEdit);
    mainLayout->addLayout(formLayout);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttons);

    if (dialog.exec() == QDialog::Accepted)
    {
        title = titleEdit->text();
        timeRange = timeEdit->text();
        return true;
    }
    return false;
}

bool MainWindow::hasOverlapWithOthers(int candidateIndex, QString *detailMessage) const
{
    if (candidateIndex < 0 || candidateIndex >= baseSchedules.size())
        return false;

    const BaseScheduleEntry &candidate = baseSchedules.at(candidateIndex);
    if (!candidate.startDate.isValid() || !candidate.endDate.isValid())
        return false;

    for (int i = 0; i < baseSchedules.size(); ++i)
    {
        if (i == candidateIndex)
            continue;

        const BaseScheduleEntry &other = baseSchedules.at(i);
        if (!other.startDate.isValid() || !other.endDate.isValid())
            continue;

        const bool overlap = !(candidate.endDate < other.startDate || other.endDate < candidate.startDate);
        if (overlap)
        {
            if (detailMessage)
            {
                const QString candidateRange = QString("%1 - %2").arg(candidate.startDate.toString("yyyy-MM-dd"), candidate.endDate.toString("yyyy-MM-dd"));
                const QString otherRange = QString("%1 - %2").arg(other.startDate.toString("yyyy-MM-dd"), other.endDate.toString("yyyy-MM-dd"));
                *detailMessage = QString::fromUtf8("\xe5\xbd\x93\xe5\x89\x8d\xe5\x9f\xba\xe7\xa1\x80\xe8\xaf\xbe\xe8\xa1\xa8(%1)\xe4\xb8\x8e\xe5\x9f\xba\xe7\xa1\x80\xe8\xaf\xbe\xe8\xa1\xa8%2(%3)\xe6\x97\xb6\xe9\x97\xb4\xe9\x87\x8d\xe5\x8f\xa0\xef\xbc\x8c\xe8\xaf\xb7\xe8\xb0\x83\xe6\x95\xb4\xe6\x97\xa5\xe6\x9c\x9f\xe5\x90\x8e\xe5\x86\x8d\xe4\xbf\x9d\xe5\xad\x98").arg(candidateRange).arg(i + 1).arg(otherRange);
            }
            return true;
        }
    }

    return false;
}

int MainWindow::currentEditableBaseIndex() const
{
    if (unsavedBaseScheduleIndex >= 0 && unsavedBaseScheduleIndex < baseSchedules.size())
        return unsavedBaseScheduleIndex;
    return -1;
}

void MainWindow::updateDirtyStateAfterSave(int savedIndex)
{
    if (unsavedBaseScheduleIndex >= 0 && unsavedBaseScheduleIndex == savedIndex)
        unsavedBaseScheduleIndex = -1;

    editingNewBaseSchedule = false;

    if (unsavedBaseScheduleIndex >= 0)
        markBaseScheduleDirty(true);
    else
        markBaseScheduleDirty(false);
}

void MainWindow::updateToolbarInteractivity()
{
    const bool inConfigMode = isConfigModeActive();
    const bool inTemporaryMode = isTemporaryAdjustModeActive();
    const bool inReminderMode = (activePanel == SecondaryPanel::Reminder && secondaryContainer && secondaryContainer->isVisible());
    const bool editingAllowed = canEditSelectedBaseSchedule();

    if (previousWeekButton)
        previousWeekButton->setEnabled(!inConfigMode);
    if (nextWeekButton)
        nextWeekButton->setEnabled(!inConfigMode);
    if (currentWeekButton)
        currentWeekButton->setEnabled(!inConfigMode);
    const bool disableAdvanced = inTemporaryMode || inReminderMode || inConfigMode;
    if (temporaryAdjustButton)
        temporaryAdjustButton->setEnabled(!inConfigMode && !inReminderMode);
    if (addReminderButton)
        addReminderButton->setEnabled(!inConfigMode && !inTemporaryMode);
    if (configButton)
        configButton->setEnabled(!inTemporaryMode && !inReminderMode);
    if (statisticsButton)
        statisticsButton->setEnabled(!disableAdvanced);
    if (minimalToggleButton)
        minimalToggleButton->setEnabled(!disableAdvanced);
    if (displayModeButton)
        displayModeButton->setEnabled(!disableAdvanced);

    if (addMergedRowButton)
        addMergedRowButton->setEnabled(editingAllowed);
    if (addLessonRowButton)
        addLessonRowButton->setEnabled(editingAllowed);
    if (deleteRowButton)
        deleteRowButton->setEnabled(editingAllowed);

    if (scheduleTable)
    {
        scheduleTable->setSelectionMode(editingAllowed ? QAbstractItemView::SingleSelection : QAbstractItemView::NoSelection);
        if (!editingAllowed)
        {
            selectedTimeRow = -1;
            scheduleTable->clearSelection();
        }
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (handleFramelessResizeEvent(watched, event))
        return true;

    if (manualMinimalMode && isDisplayViewActive()) // 极简模式下允许拖动与无边框缩放
    {
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            resizeArea = hitTestResizeArea(mapFromGlobal(mouseEvent->globalPos()));
            if (!resizeInProgress && resizeArea == ResizeArea::None)
            {
                displayDragging = true;
                displayDragOffset = mouseEvent->globalPos() - frameGeometry().topLeft();
            }
            else
            {
                resizeInProgress = true;
                resizeStartPos = mouseEvent->globalPos();
                resizeStartGeometry = geometry();
            }
        }
    }
    else if (event->type() == QEvent::MouseMove)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (resizeInProgress && (mouseEvent->buttons() & Qt::LeftButton))
        {
            handleFramelessResizeEvent(this, event);
            return true;
        }
        if (displayDragging && (mouseEvent->buttons() & Qt::LeftButton))
            move(mouseEvent->globalPos() - displayDragOffset);
    }
    else if (event->type() == QEvent::MouseButtonDblClick)
    {
        applyManualMinimalMode(false);
        return true;
    }
    else if (event->type() == QEvent::MouseButtonRelease)
    {
        displayDragging = false;
        resizeInProgress = false;
        resizeArea = ResizeArea::None;
    }
    }

    if (largeScheduleDialog && watched == largeScheduleDialog)
    {
        if (event->type() == QEvent::Move || event->type() == QEvent::Resize)
            positionTeacherRosterDock();
        return false;
    }
    if (weeklyStatsContainer)
    {
        QWidget *dropWidget = qobject_cast<QWidget *>(watched);
        if (dropWidget && (dropWidget == weeklyStatsContainer || (weeklyStatsViewport && dropWidget == weeklyStatsViewport)))
        {
            if (!isTemporaryAdjustModeActive())
                return false;

            auto acceptsScheduleMime = [](const QMimeData *mime) -> bool {
                return mime && mime->hasFormat("application/x-schedule-cell");
            };

            if (event->type() == QEvent::DragEnter)
            {
                auto *dragEvent = static_cast<QDragEnterEvent *>(event);
                if (acceptsScheduleMime(dragEvent->mimeData()))
                {
                    dragEvent->acceptProposedAction();
                    return true;
                }
            }
            else if (event->type() == QEvent::DragMove)
            {
                auto *moveEvent = static_cast<QDragMoveEvent *>(event);
                if (acceptsScheduleMime(moveEvent->mimeData()))
                {
                    moveEvent->acceptProposedAction();
                    return true;
                }
            }
            else if (event->type() == QEvent::Drop)
            {
                auto *dropEvent = static_cast<QDropEvent *>(event);
                if (acceptsScheduleMime(dropEvent->mimeData()))
                {
                    QByteArray encoded = dropEvent->mimeData()->data("application/x-schedule-cell");
                    QDataStream stream(&encoded, QIODevice::ReadOnly);
                    int sourceRow = -1;
                    int sourceColumn = -1;
                    QString name;
                    QColor color;
                    int typeValue = 0;
                    stream >> sourceRow >> sourceColumn >> name >> color >> typeValue;
                    dropEvent->acceptProposedAction();
                    const QPoint globalPoint = dropWidget->mapToGlobal(dropEvent->pos());
                    handleWeeklyStatsDeletionDrop(sourceRow, sourceColumn, name, color, static_cast<ScheduleCellLabel::CourseType>(typeValue), globalPoint);
                    return true;
                }
            }
        }
    }

    if (largeScheduleTable && watched == largeScheduleTable->viewport())
    {
        if (event->type() == QEvent::Resize)
        {
            adjustLargeScheduleCellSizes();
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                QModelIndex index = largeScheduleTable->indexAt(mouseEvent->pos());
                if (index.isValid() && index.row() == 0 && index.column() > 0)
                {
                    if (!(mouseEvent->modifiers() & Qt::ShiftModifier) || largeScheduleSelectorAnchorColumn < 0)
                        largeScheduleSelectorAnchorColumn = index.column();
                    largeScheduleSelectorDragging = true;
                    largeScheduleSelectorLastColumn = index.column();
                    applyLargeScheduleSelectorClick(index.column(), mouseEvent->modifiers());
                    return true;
                }
                if (index.isValid() && index.column() == 0 && index.row() > 0)
                {
                    if (!(mouseEvent->modifiers() & Qt::ShiftModifier) || largeScheduleRowAnchor < 0)
                        largeScheduleRowAnchor = index.row();
                    largeScheduleRowDragging = true;
                    largeScheduleRowLast = index.row();
                    applyLargeScheduleRowSelectorClick(index.row(), mouseEvent->modifiers());
                    return true;
                }
                largeScheduleSelectorDragging = false;
                largeScheduleRowDragging = false;
            }
        }
        else if (event->type() == QEvent::MouseMove)
        {
            if (largeScheduleSelectorDragging)
            {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
                QModelIndex index = largeScheduleTable->indexAt(mouseEvent->pos());
                if (index.isValid() && index.row() == 0 && index.column() > 0)
                    applyLargeScheduleSelectorDrag(index.column());
                return true;
            }
            if (largeScheduleRowDragging)
            {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
                QModelIndex index = largeScheduleTable->indexAt(mouseEvent->pos());
                if (index.isValid() && index.column() == 0 && index.row() > 0)
                    applyLargeScheduleRowSelectorDrag(index.row());
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                bool handled = false;
                if (largeScheduleSelectorDragging)
                {
                    largeScheduleSelectorDragging = false;
                    handled = true;
                }
                if (largeScheduleRowDragging)
                {
                    largeScheduleRowDragging = false;
                    handled = true;
                }
                if (handled)
                    return true;
            }
        }
    }

    if (teacherRosterTable && watched == teacherRosterTable->viewport())
    {
        if (event->type() == QEvent::Resize)
        {
            adjustTeacherRosterCellSizes();
            if (QWidget *overlay = teacherRosterTable->viewport()->findChild<QWidget *>("teacherRosterResultOverlay"))
            {
                bool ok = false;
                const int targetRow = overlay->property("targetRow").toInt(&ok);
                if (ok && targetRow >= 0 && targetRow < teacherRosterTable->rowCount() && teacherRosterTable->columnCount() > 0)
                {
                    const int lastCol = teacherRosterTable->columnCount() - 1;
                    QRect firstRect = teacherRosterTable->visualRect(teacherRosterTable->model()->index(targetRow, 0));
                    QRect lastRect = teacherRosterTable->visualRect(teacherRosterTable->model()->index(targetRow, lastCol));
                    QRect rowRect = firstRect.united(lastRect);
                    overlay->setGeometry(rowRect);
                    if (QHBoxLayout *layout = qobject_cast<QHBoxLayout *>(overlay->layout()))
                    {
                        for (int c = 0; c < layout->count() && c < teacherRosterTable->columnCount(); ++c)
                        {
                            if (QLabel *lbl = qobject_cast<QLabel *>(layout->itemAt(c)->widget()))
                            {
                                const int colWidth = teacherRosterTable->columnWidth(c);
                                lbl->setMinimumWidth(colWidth);
                                lbl->setMaximumWidth(colWidth);
                                lbl->setFixedHeight(teacherRosterTable->rowHeight(targetRow));
                                layout->setStretch(c, colWidth);
                            }
                        }
                    }
                }
            }
        }
    }

    if (scheduleTable && watched == scheduleTable->viewport())
    {
        if (event->type() == QEvent::Resize)
        {
            updateAdaptiveCellFonts();
            updateReminderCards();
        }
        else if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (canEditSelectedBaseSchedule() && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter))
            {
                const int row = scheduleTable->currentRow();
                const int column = scheduleTable->currentColumn();
                if (row > 0)
                {
                    if (column == 0)
                    {
                        editTimeSlot(row);
                        return true;
                    }
                    if (column > 0)
                    {
                        editCourseCell(row, column);
                        return true;
                    }
                }
            }
        }
    }

    if (customTemporaryCourseLabel && watched == customTemporaryCourseLabel)
    {
        if (event->type() == QEvent::MouseButtonDblClick)
        {
            editCustomTemporaryCourse();
            return true;
        }
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                customTempDragStartPos = mouseEvent->pos();
                customTemporaryCourseLabel->setCursor(Qt::ClosedHandCursor);
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton)
                customTemporaryCourseLabel->setCursor(Qt::OpenHandCursor);
        }
        else if (event->type() == QEvent::MouseMove)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->buttons() & Qt::LeftButton)
            {
                if ((mouseEvent->pos() - customTempDragStartPos).manhattanLength() >= QApplication::startDragDistance())
                {
                    startCustomTemporaryCourseDrag();
                    return true;
                }
            }
        }
    }

    if (quickCourseWidget)
    {
        QPushButton *paletteBtn = qobject_cast<QPushButton *>(watched);
        if (paletteBtn && quickCourseWidget->isAncestorOf(paletteBtn))
        {
            if (event->type() == QEvent::MouseButtonDblClick)
            {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
                if (mouseEvent->button() != Qt::LeftButton)
                    return true;

                QString originalName = paletteBtn->property("courseName").toString();
                QColor originalColor = paletteBtn->property("courseColor").value<QColor>();
                CourseEditDialog dialog(this);
                dialog.setCourseName(originalName);
                dialog.setCourseColor(originalColor);
                if (dialog.exec() == QDialog::Accepted)
                {
                    if (dialog.removeRequested())
                    {
                        courseColorMap.remove(originalName);
                        if (activePaletteCourse == originalName)
                            selectQuickCourseButton(nullptr);
                    }
                    else
                    {
                        courseColorMap.remove(originalName);
                        courseColorMap.insert(dialog.courseName(), dialog.courseColor());
                        activePaletteCourse = dialog.courseName();
                        activePaletteColor = dialog.courseColor();
                    }
                    updateQuickCourseButtons();
                }
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::showSecondaryPanel(SecondaryPanel panel)
{
    const bool wasTemporaryAdjust = (activePanel == SecondaryPanel::TemporaryAdjust);
    const bool wasHiddenDisplay = chromeHiddenInDisplay;

    if (panel == activePanel)
    {
        if (panel == SecondaryPanel::Config && !ensureBaseScheduleCanClose())
            return;
        secondaryContainer->setVisible(false);
        activePanel = SecondaryPanel::None;
        if (panel == SecondaryPanel::Config)
        {
            setForcedBaseScheduleIndex(-1);
            editingNewBaseSchedule = false;
            ensureCellWidgets();
            refreshTimeColumn();
            refreshScheduleCells();
            applyRowLayout();
            initializeQuickCoursePalette();
        }
        adjustSecondaryContainerHeight();
        updateToolbarInteractivity();
        applyDisplayPreferences();
        if (wasTemporaryAdjust && panel == SecondaryPanel::TemporaryAdjust)
            saveActualScheduleToDisk();
        return;
    }

    if (activePanel == SecondaryPanel::Config && panel != SecondaryPanel::Config)
    {
        if (!ensureBaseScheduleCanClose())
            return;
    }

    activePanel = panel;
    QWidget *target = nullptr;
    switch (panel)
    {
    case SecondaryPanel::Config:
        target = configPanel;
        break;
    case SecondaryPanel::TemporaryAdjust:
        target = temporaryAdjustPanel;
        break;
    case SecondaryPanel::Reminder:
        target = reminderPanel;
        break;
    case SecondaryPanel::DisplayMode:
        target = displayModePanel;
        break;
    case SecondaryPanel::None:
    default:
        break;
    }

    if (target)
    {
        secondaryStack->setCurrentWidget(target);
        secondaryContainer->setVisible(true);
        if (panel == SecondaryPanel::Config)
            refreshBaseScheduleList();
        if (panel == SecondaryPanel::TemporaryAdjust)
            updateWeeklyStatistics();
    }
    else
    {
        secondaryContainer->setVisible(false);
    }

    if (panel != SecondaryPanel::Config)
    {
        setForcedBaseScheduleIndex(-1);
        editingNewBaseSchedule = false;
        ensureCellWidgets();
        refreshTimeColumn();
        refreshScheduleCells();
        applyRowLayout();
        initializeQuickCoursePalette();
    }

    adjustSecondaryContainerHeight();
    updateToolbarInteractivity();
    updateReminderCards();
    applyDisplayPreferences();

    if (wasTemporaryAdjust && activePanel != SecondaryPanel::TemporaryAdjust)
        saveActualScheduleToDisk();

    if (activePanel != SecondaryPanel::None && wasHiddenDisplay)
    {
        chromeHiddenInDisplay = false;
        applyDisplayPreferences();
    }
}

QDate MainWindow::mondayOf(const QDate &date) const
{
    const int dayOfWeek = date.dayOfWeek(); // 1=Mon
    return date.addDays(1 - dayOfWeek);
}

void MainWindow::loadTermSettings()
{
    QSettings settings;
    const QDate start = settings.value("term/startDate").toDate();
    const QDate end = settings.value("term/endDate").toDate();
    if (start.isValid() && end.isValid() && start <= end)
    {
        termStartDate = start;
        termEndDate = end;
        hasTermRange = true;
    }
    else
    {
        termStartDate = QDate();
        termEndDate = QDate();
        hasTermRange = false;
    }
}

void MainWindow::saveTermSettings()
{
    QSettings settings;
    if (hasTermRange)
    {
        settings.setValue("term/startDate", termStartDate);
        settings.setValue("term/endDate", termEndDate);
    }
    else
    {
        settings.remove("term/startDate");
        settings.remove("term/endDate");
    }
}

void MainWindow::loadActualScheduleFromDisk()
{
    actualWeekCourses.clear();
    QFile file(actualScheduleFilePath());
    if (!file.exists())
    {
        ensureCustomTemporaryCourseNameUnique();
        return;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        ensureCustomTemporaryCourseNameUnique();
        return;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
    {
        ensureCustomTemporaryCourseNameUnique();
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject customObj = root.value("customTemporaryCourse").toObject();
    QString loadedName = customObj.value("name").toString();
    if (!loadedName.trimmed().isEmpty())
        customTemporaryCourseName = loadedName.trimmed();
    QColor loadedColor(customObj.value("color").toString());
    if (loadedColor.isValid())
        customTemporaryCourseColor = loadedColor;

    QJsonArray weeksArray = root.value("weeks").toArray();
    auto ensureEntryForWeek = [this](const QDate &weekStart) -> ActualWeekEntry & {
        for (ActualWeekEntry &entry : actualWeekCourses)
        {
            if (entry.weekStart == weekStart)
                return entry;
        }
        ActualWeekEntry entry;
        entry.weekStart = weekStart;
        actualWeekCourses.append(entry);
        return actualWeekCourses.last();
    };

    for (const QJsonValue &weekValue : weeksArray)
    {
        if (!weekValue.isObject())
            continue;
        QJsonObject weekObj = weekValue.toObject();
        QDate weekStart = QDate::fromString(weekObj.value("weekStart").toString(), Qt::ISODate);
        if (!weekStart.isValid())
            continue;
        weekStart = mondayOf(weekStart);
        QJsonArray cellsArray = weekObj.value("cells").toArray();
        if (cellsArray.isEmpty())
            continue;

        ActualWeekEntry &entry = ensureEntryForWeek(weekStart);
        for (const QJsonValue &cellValue : cellsArray)
        {
            if (!cellValue.isObject())
                continue;
            QJsonObject cellObj = cellValue.toObject();
            int row = cellObj.value("row").toInt(-1);
            int column = cellObj.value("column").toInt(-1);
            if (row < 0 || column < 0)
                continue;

            CourseCellData cell = CourseCellData::empty();
            cell.name = cellObj.value("name").toString();
            QColor cellColor(cellObj.value("color").toString());
            if (cellColor.isValid())
                cell.background = cellColor;
            cell.type = static_cast<ScheduleCellLabel::CourseType>(cellObj.value("type").toInt(static_cast<int>(ScheduleCellLabel::CourseType::None)));
            if (cell.type == ScheduleCellLabel::CourseType::None && cell.name.trimmed().isEmpty())
                continue;

            entry.cellOverrides[row][column] = cell;
        }
    }

    for (int i = actualWeekCourses.size() - 1; i >= 0; --i)
    {
        if (actualWeekCourses.at(i).cellOverrides.isEmpty())
            actualWeekCourses.removeAt(i);
    }

    ensureCustomTemporaryCourseNameUnique();
    if (customTemporaryCourseLabel)
        updateCustomTemporaryCourseLabel();
}

void MainWindow::saveActualScheduleToDisk() const
{
    QJsonObject root;
    QJsonObject customObj;
    customObj.insert("name", customTemporaryCourseName);
    if (customTemporaryCourseColor.isValid())
        customObj.insert("color", customTemporaryCourseColor.name(QColor::HexRgb));
    root.insert("customTemporaryCourse", customObj);

    QJsonArray weeksArray;
    for (const ActualWeekEntry &entry : actualWeekCourses)
    {
        if (entry.cellOverrides.isEmpty())
            continue;

        QJsonObject weekObj;
        weekObj.insert("weekStart", entry.weekStart.toString(Qt::ISODate));
        QJsonArray cellsArray;
        for (auto rowIt = entry.cellOverrides.constBegin(); rowIt != entry.cellOverrides.constEnd(); ++rowIt)
        {
            const int row = rowIt.key();
            const QMap<int, CourseCellData> &columns = rowIt.value();
            for (auto colIt = columns.constBegin(); colIt != columns.constEnd(); ++colIt)
            {
                const CourseCellData &cell = colIt.value();
                if (cell.type == ScheduleCellLabel::CourseType::None && cell.name.trimmed().isEmpty())
                    continue;
                QJsonObject cellObj;
                cellObj.insert("row", row);
                cellObj.insert("column", colIt.key());
                cellObj.insert("name", cell.name);
                if (cell.background.isValid())
                    cellObj.insert("color", cell.background.name(QColor::HexRgb));
                cellObj.insert("type", static_cast<int>(cell.type));
                cellsArray.append(cellObj);
            }
        }

        if (cellsArray.isEmpty())
            continue;
        weekObj.insert("cells", cellsArray);
        weeksArray.append(weekObj);
    }
    root.insert("weeks", weeksArray);

    QJsonDocument doc(root);
    const QString path = actualScheduleFilePath();
    QFile file(path);
    QDir dir(QFileInfo(path).absolutePath());
    if (!dir.exists())
        dir.mkpath(".");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    file.write(doc.toJson(QJsonDocument::Indented));
}

void MainWindow::loadReminders()
{
    reminderEntries.clear();
    QFile file(reminderStoragePath());
    if (!file.exists())
        return;
    if (!file.open(QIODevice::ReadOnly))
        return;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError)
        return;
    const QJsonArray array = doc.isArray() ? doc.array() : doc.object().value("reminders").toArray();
    for (const QJsonValue &value : array)
    {
        if (!value.isObject())
            continue;
        QJsonObject obj = value.toObject();
        ReminderEntry entry;
        entry.title = obj.value("title").toString();
        entry.type = static_cast<ReminderEntry::Type>(obj.value("type").toInt(static_cast<int>(ReminderEntry::Type::Once)));
        entry.onceDateTime = QDateTime::fromString(obj.value("onceDateTime").toString(), Qt::ISODate);
        entry.startDate = QDate::fromString(obj.value("startDate").toString(), Qt::ISODate);
        entry.endDate = QDate::fromString(obj.value("endDate").toString(), Qt::ISODate);
        entry.time = QTime::fromString(obj.value("time").toString(), QStringLiteral("HH:mm:ss"));
        entry.weekday = obj.value("weekday").toInt(1);
        if (entry.title.trimmed().isEmpty())
            continue;
        reminderEntries.append(entry);
    }
}

void MainWindow::saveReminders() const
{
    QJsonArray array;
    for (const ReminderEntry &entry : reminderEntries)
    {
        QJsonObject obj;
        obj.insert("title", entry.title);
        obj.insert("type", static_cast<int>(entry.type));
        if (entry.onceDateTime.isValid())
            obj.insert("onceDateTime", entry.onceDateTime.toString(Qt::ISODate));
        if (entry.startDate.isValid())
            obj.insert("startDate", entry.startDate.toString(Qt::ISODate));
        if (entry.endDate.isValid())
            obj.insert("endDate", entry.endDate.toString(Qt::ISODate));
        if (entry.time.isValid())
            obj.insert("time", entry.time.toString(QStringLiteral("HH:mm:ss")));
        obj.insert("weekday", entry.weekday);
        array.append(obj);
    }
    QJsonDocument doc(array);
    QFile file(reminderStoragePath());
    QDir dir(QFileInfo(file).absolutePath());
    if (!dir.exists())
        dir.mkpath(".");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    file.write(doc.toJson(QJsonDocument::Indented));
}

QList<MainWindow::ReminderOccurrence> MainWindow::remindersForCurrentWeek() const
{
    QList<ReminderOccurrence> occurrences;
    const QDate weekStart = mondayOf(currentWeekStart);
    if (!weekStart.isValid())
        return occurrences;
    const QDate weekEnd = weekStart.addDays(6);
    for (const ReminderEntry &entry : reminderEntries)
    {
        if (!entry.visible)
            continue;
        if (entry.type == ReminderEntry::Type::Once)
        {
            const QDate date = entry.onceDateTime.date();
            if (entry.onceDateTime.isValid() && date >= weekStart && date <= weekEnd)
            {
                ReminderOccurrence occ;
                occ.date = date;
                occ.time = entry.onceDateTime.time();
                occ.title = entry.title;
                occ.detail = occ.time.toString(QStringLiteral("HH:mm"));
                occurrences.append(occ);
            }
            continue;
        }
        if (!entry.startDate.isValid() || !entry.endDate.isValid())
            continue;
        QDate iterDate = qMax(entry.startDate, weekStart);
        const QDate iterEnd = qMin(entry.endDate, weekEnd);
        while (iterDate <= iterEnd)
        {
            bool match = (entry.type == ReminderEntry::Type::Daily) ||
                         (entry.type == ReminderEntry::Type::Weekly && iterDate.dayOfWeek() == entry.weekday);
            if (match)
            {
                ReminderOccurrence occ;
                occ.date = iterDate;
                occ.time = entry.time.isValid() ? entry.time : QTime(8, 0);
                occ.title = entry.title;
                occ.detail = occ.time.toString(QStringLiteral("HH:mm"));
                occurrences.append(occ);
            }
            iterDate = iterDate.addDays(1);
        }
    }
    std::sort(occurrences.begin(), occurrences.end(), [](const ReminderOccurrence &a, const ReminderOccurrence &b) {
        if (a.date == b.date)
            return a.time < b.time;
        return a.date < b.date;
    });
    return occurrences;
}

void MainWindow::updateReminderCards()
{
    if (!scheduleTable || !reminderOverlay)
        return;

    const bool shouldShow = (activePanel == SecondaryPanel::None || activePanel == SecondaryPanel::Reminder || activePanel == SecondaryPanel::DisplayMode);
    qDeleteAll(reminderCardWidgets);
    reminderCardWidgets.clear();

    if (!shouldShow)
    {
        reminderOverlay->hide();
        return;
    }

    QList<ReminderOccurrence> occurrences = remindersForCurrentWeek();
    if (occurrences.isEmpty())
    {
        reminderOverlay->hide();
        return;
    }

    const int totalRows = scheduleTable->rowCount();
    QVector<int> rowHeights(totalRows);
    for (int row = 0; row < totalRows; ++row)
        rowHeights[row] = scheduleTable->rowHeight(row);

    auto columnViewportX = [&](int column) -> int {
        return scheduleTable->columnViewportPosition(column);
    };

    for (const ReminderOccurrence &occ : occurrences)
    {
        int weekdayIndex = occ.date.dayOfWeek(); // 1-7
        int column = weekdayIndex;              // column 1..7
        int rowIndex = 1;
        qreal ratio = 0.0;
        if (!reminderPositionForTime(occ.time, rowIndex, ratio))
            continue;
        if (column < 1 || column >= scheduleTable->columnCount())
            continue;
        const int rowTop = scheduleTable->rowViewportPosition(rowIndex);
        const int rowHeight = scheduleTable->rowHeight(rowIndex);
        const int yPos = rowTop + static_cast<int>(ratio * rowHeight) - 20;
        ReminderCardWidget *card = new ReminderCardWidget(occ.title, occ.detail, reminderOverlay);
        const int columnWidth = scheduleTable->columnWidth(column);
        const int colStart = columnViewportX(column);
        int xPos = colStart + columnWidth - card->width() - 8;
        if (xPos < colStart)
            xPos = colStart + 2;
        card->move(xPos, qMax(0, yPos));
        card->show();
        reminderCardWidgets.append(card);
    }
    reminderOverlay->raise();
    reminderOverlay->show();
    updateReminderOverlayGeometry();
}

void MainWindow::updateReminderOverlayGeometry() const
{
    if (!scheduleTable || !reminderOverlay)
        return;
    QRect rect = scheduleTable->viewport()->rect();
    reminderOverlay->setGeometry(rect);
}

bool MainWindow::reminderPositionForTime(const QTime &time, int &rowIndex, qreal &rowRatio) const
{
    QTime target = time.isValid() ? time : QTime(8, 0);
    rowIndex = 1;
    rowRatio = 0.0;
    qreal targetMinutes = target.hour() * 60 + target.minute();
    qreal previousEnd = -1;
    for (int dataRow = 0; dataRow < lessonTimes.size(); ++dataRow)
    {
        QTime start;
        QTime end;
        if (!extractRowTimeRange(dataRow, start, end))
            continue;
        qreal startMin = start.hour() * 60 + start.minute();
        qreal endMin = end.hour() * 60 + end.minute();
        if (previousEnd >= 0 && targetMinutes < startMin)
        {
            rowIndex = dataRow + 1;
            rowRatio = 0.0;
            return true;
        }
        if (targetMinutes <= endMin)
        {
            rowIndex = dataRow + 1;
            if (endMin > startMin)
                rowRatio = qBound(0.0, (targetMinutes - startMin) / (endMin - startMin), 1.0);
            else
                rowRatio = 0.0;
            return true;
        }
        previousEnd = endMin;
    }
    rowIndex = lessonTimes.size();
    rowRatio = 1.0;
    return true;
}

bool MainWindow::extractRowTimeRange(int lessonIndex, QTime &start, QTime &end) const
{
    if (lessonIndex < 0 || lessonIndex >= lessonTimes.size())
        return false;
    const QString slot = lessonTimes.at(lessonIndex);
    const QStringList times = slot.split('-', Qt::SkipEmptyParts);
    if (times.size() != 2)
        return false;
    start = QTime::fromString(times.at(0), QStringLiteral("HH:mm"));
    end = QTime::fromString(times.at(1), QStringLiteral("HH:mm"));
    return start.isValid() && end.isValid();
}


bool MainWindow::ensureTermConfigured()
{
    if (hasTermRange)
        return true;
    return openTermSettingsDialog();
}

bool MainWindow::openTermSettingsDialog()
{
    TermSettingsDialog dialog(this);
    if (termStartDate.isValid())
        dialog.setStartDate(termStartDate);
    if (termEndDate.isValid())
        dialog.setEndDate(termEndDate);

    if (dialog.exec() == QDialog::Accepted)
    {
        termStartDate = dialog.startDate();
        termEndDate = dialog.endDate();
        hasTermRange = true;
        saveTermSettings();
        updateWeekLabel();
        return true;
    }

    return false;
}

QString MainWindow::baseScheduleFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::currentPath();
    QDir directory(dir);
    if (!directory.exists())
        directory.mkpath(".");
    return directory.filePath("base_schedule.json");
}

QString MainWindow::actualScheduleFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::currentPath();
    QDir directory(dir);
    if (!directory.exists())
        directory.mkpath(".");
    return directory.filePath("actual_schedule.json");
}

QString MainWindow::reminderStoragePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::currentPath();
    QDir directory(dir);
    if (!directory.exists())
        directory.mkpath(".");
    return directory.filePath("reminders.json");
}

void MainWindow::markBaseScheduleDirty(bool dirty)
{
    if (baseScheduleDirty == dirty)
        return;
    baseScheduleDirty = dirty;
    updateSaveButtons();
}

void MainWindow::updateSaveButtons()
{
    const QString saveText = QString::fromUtf8("\xe4\xbf\x9d\xe5\xad\x98\xe8\xaf\xbe\xe8\xa1\xa8");
    if (saveScheduleButton)
    {
        saveScheduleButton->setEnabled(baseScheduleDirty);
        saveScheduleButton->setText(baseScheduleDirty ? saveText + QStringLiteral(" *") : saveText);
    }
    if (discardScheduleButton)
        discardScheduleButton->setEnabled(baseScheduleDirty);
}

bool MainWindow::saveBaseScheduleToDisk(int targetIndex)
{
    Q_UNUSED(targetIndex);

    QVector<BaseScheduleEntry> entriesToSave = baseSchedules;

    QJsonArray entriesArray;
    for (const BaseScheduleEntry &entry : entriesToSave)
    {
        QJsonObject entryObj;
        entryObj["startDate"] = entry.startDate.toString(Qt::ISODate);
        entryObj["endDate"] = entry.endDate.toString(Qt::ISODate);
        entryObj["rows"] = entry.rowDefinitions.size();
        entryObj["columns"] = entry.columns;

        QJsonArray rowDefs;
        for (const RowDefinition &def : entry.rowDefinitions)
        {
            QJsonObject defObj;
            defObj["kind"] = def.kind == RowDefinition::Kind::Placeholder ? "placeholder" : "lesson";
            rowDefs.append(defObj);
        }
        entryObj["rowDefinitions"] = rowDefs;

        QJsonArray timeArray;
        for (const QString &text : entry.timeColumnTexts)
            timeArray.append(text);
        entryObj["timeColumnTexts"] = timeArray;

        QJsonArray rowsArray;
        for (const QVector<CourseCellData> &row : entry.courseCells)
        {
            QJsonArray rowArray;
            for (const CourseCellData &cell : row)
            {
                QJsonObject cellObj;
                cellObj["name"] = cell.name;
                if (!cell.className.trimmed().isEmpty())
                    cellObj["class_name"] = cell.className;
                if (cell.background.isValid())
                    cellObj["color"] = cell.background.name(QColor::HexRgb);
                cellObj["type"] = static_cast<int>(cell.type);
                rowArray.append(cellObj);
            }
            rowsArray.append(rowArray);
        }
        entryObj["courseCells"] = rowsArray;
        entriesArray.append(entryObj);
    }

    QJsonObject root;
    root["entries"] = entriesArray;
    QJsonDocument doc(root);

    const QString path = baseScheduleFilePath();
    QFile file(path);
    QDir dir(QFileInfo(path).absolutePath());
    if (!dir.exists() && !dir.mkpath("."))
    {
        QMessageBox::warning(this,
                             QString::fromUtf8("\xe4\xbf\x9d\xe5\xad\x98\xe5\xa4\xb1\xe8\xb4\xa5"),
                             QString::fromUtf8("\xe6\x97\xa0\xe6\xb3\x95\xe5\x88\x9b\xe5\xbb\xba\xe5\x9f\xba\xe7\xa1\x80\xe8\xaf\xbe\xe8\xa1\xa8\xe5\xad\x98\xe5\x82\xa8\xe7\x9b\xae\xe5\xbd\x95"));
        return false;
    }
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QMessageBox::warning(this,
                             QString::fromUtf8("\xe4\xbf\x9d\xe5\xad\x98\xe5\xa4\xb1\xe8\xb4\xa5"),
                             file.errorString());
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    persistedBaseSchedules = entriesToSave;
    refreshBaseScheduleList();
    return true;
}

bool MainWindow::loadBaseScheduleFromDisk()
{
    QFile file(baseScheduleFilePath());
    if (!file.exists())
        return false;
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    QJsonArray entriesArray = doc.object().value("entries").toArray();
    if (entriesArray.isEmpty())
        return false;

    QVector<BaseScheduleEntry> loadedEntries;
    for (const QJsonValue &value : entriesArray)
    {
        if (!value.isObject())
            continue;
        QJsonObject obj = value.toObject();

        BaseScheduleEntry entry;
        entry.startDate = QDate::fromString(obj.value("startDate").toString(), Qt::ISODate);
        entry.endDate = QDate::fromString(obj.value("endDate").toString(), Qt::ISODate);
        entry.startDate = ensureStartMonday(entry.startDate);
        entry.endDate = ensureEndSunday(entry.startDate, entry.endDate);
        entry.columns = obj.value("columns").toInt(7);

        QJsonArray rowDefs = obj.value("rowDefinitions").toArray();
        entry.rowDefinitions.clear();
        for (const QJsonValue &defVal : rowDefs)
        {
            RowDefinition def;
            QString kind = defVal.toObject().value("kind").toString();
            def.kind = kind == "placeholder" ? RowDefinition::Kind::Placeholder : RowDefinition::Kind::Lesson;
            entry.rowDefinitions.append(def);
        }

        QJsonArray timeArray = obj.value("timeColumnTexts").toArray();
        entry.timeColumnTexts.clear();
        for (const QJsonValue &timeVal : timeArray)
            entry.timeColumnTexts.append(timeVal.toString());

        QJsonArray rowsArray = obj.value("courseCells").toArray();
        entry.courseCells.clear();
        for (const QJsonValue &rowVal : rowsArray)
        {
            QVector<CourseCellData> rowData;
            const QJsonArray rowArray = rowVal.toArray();
            for (const QJsonValue &cellVal : rowArray)
            {
                CourseCellData cell = CourseCellData::empty();
                if (cellVal.isObject())
                {
                    const QJsonObject cellObj = cellVal.toObject();
                    cell.name = cellObj.value("name").toString();
                    cell.className = cellObj.value("class_name").toString();
                    cell.background = QColor(cellObj.value("color").toString());
                    cell.type = static_cast<ScheduleCellLabel::CourseType>(cellObj.value("type").toInt(static_cast<int>(ScheduleCellLabel::CourseType::None)));
                }
                rowData.append(cell);
            }
            entry.courseCells.append(rowData);
        }

        if (entry.rowDefinitions.isEmpty())
            entry.rowDefinitions = QVector<RowDefinition>(entry.courseCells.size(), RowDefinition{RowDefinition::Kind::Lesson});
        if (entry.timeColumnTexts.size() < entry.courseCells.size())
        {
            while (entry.timeColumnTexts.size() < entry.courseCells.size())
                entry.timeColumnTexts.append(QString());
        }
        if (entry.columns <= 0 && !entry.courseCells.isEmpty())
            entry.columns = entry.courseCells.first().size();
        entry.rows = entry.courseCells.size();
        loadedEntries.append(entry);
    }

    if (loadedEntries.isEmpty())
        return false;

    baseSchedules = loadedEntries;
    editingNewBaseSchedule = false;
    unsavedBaseScheduleIndex = -1;
    refreshBaseScheduleList();
    return true;
}

void MainWindow::discardBaseScheduleChanges()
{
    baseSchedules = persistedBaseSchedules;
    markBaseScheduleDirty(false);
    editingNewBaseSchedule = false;
    unsavedBaseScheduleIndex = -1;
    ensureCellWidgets();
    refreshTimeColumn();
    refreshScheduleCells();
    applyRowLayout();
    initializeQuickCoursePalette();
    updateEffectiveDateEditors();
    refreshBaseScheduleList();
    updateToolbarInteractivity();
}

void MainWindow::handleSaveBaseSchedule()
{
    int targetIndex = currentEditableBaseIndex();
    if (targetIndex < 0 || targetIndex >= baseSchedules.size())
        return;

    QVector<int> overlapIndexes = overlappingBaseScheduleIndexes(targetIndex);
    if (!overlapIndexes.isEmpty())
    {
        const QString message = buildOverlapWarning(baseSchedules.at(targetIndex), overlapIndexes);
        QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                                  QString::fromUtf8("\xe6\x97\xa5\xe6\x9c\x9f\xe9\x87\x8d\xe5\x8f\xa0"),
                                                                  message,
                                                                  QMessageBox::Ok | QMessageBox::Cancel,
                                                                  QMessageBox::Cancel);
        if (reply != QMessageBox::Ok)
            return;
        resolveOverlapsForNewSchedule(targetIndex);
    }

    QString overlapDetail;
    if (hasOverlapWithOthers(targetIndex, &overlapDetail))
    {
        QMessageBox::warning(this,
                             QString::fromUtf8("\xe4\xbf\x9d\xe5\xad\x98\xe5\xa4\xb1\xe8\xb4\xa5"),
                             overlapDetail);
        return;
    }

    if (!saveBaseScheduleToDisk(targetIndex))
        return;

    updateDirtyStateAfterSave(targetIndex);
    const BaseScheduleEntry &savedEntry = baseSchedules.at(targetIndex);
    QDate start = savedEntry.startDate.isValid() ? savedEntry.startDate : (baseStartDateEdit ? baseStartDateEdit->date() : QDate());
    QDate end = savedEntry.endDate.isValid() ? savedEntry.endDate : (baseEndDateEdit ? baseEndDateEdit->date() : QDate());
    QString startText = start.isValid() ? start.toString(QString::fromUtf8("yyyy年M月d日")) : QString::fromUtf8("当前周");
    QString endText = end.isValid() ? end.toString(QString::fromUtf8("yyyy年M月d日")) : QString::fromUtf8("当前周");
    QString message = QString::fromUtf8("已将适用于 %1 至 %2 的课程表保存并启用。").arg(startText, endText);
    QMessageBox::information(this, QString::fromUtf8("保存成功"), message);
    refreshBaseScheduleList();
    refreshScheduleCells();
    refreshTimeColumn();
    updateToolbarInteractivity();
}

void MainWindow::handleDiscardBaseSchedule()
{
    if (!baseScheduleDirty)
        return;
    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                             QString::fromUtf8("\xe6\x94\xbe\xe5\xbc\x83\xe6\x9b\xb4\xe6\x94\xb9"),
                                                             QString::fromUtf8("\xe7\xa1\xae\xe5\xae\x9a\xe8\xa6\x81\xe6\x94\xbe\xe5\xbc\x83\xe5\xbd\x93\xe5\x89\x8d\xe7\x9a\x84\xe5\x9f\xba\xe7\xa1\x80\xe8\xaf\xbe\xe8\xa1\xa8\xe8\xb0\x83\xe6\x95\xb4\xe5\x90\x97\xef\xbc\x9f"),
                                                             QMessageBox::Yes | QMessageBox::No,
                                                             QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        discardBaseScheduleChanges();
        updateToolbarInteractivity();
    }
}

void MainWindow::handleResetAllData()
{
    QMessageBox::StandardButton reply = QMessageBox::warning(this,
                                                             QString::fromUtf8("\xe7\xa1\xae\xe8\xae\xa4\xe9\x87\x8d\xe7\xbd\xae"),
                                                             QString::fromUtf8("\xe6\xad\xa4\xe6\x93\x8d\xe4\xbd\x9c\xe4\xbc\x9a\xe6\xb8\x85\xe9\x99\xa4\xe5\x9f\xba\xe7\xa1\x80\xe8\xaf\xbe\xe8\xa1\xa8\xe3\x80\x81\xe5\xae\x9e\xe9\x99\x85\xe8\xaf\xbe\xe8\xa1\xa8\xe3\x80\x81\xe6\x8f\x90\xe9\x86\x92\xe4\xbb\xa5\xe5\x8f\x8a\xe4\xbb\xbb\xe8\xaf\xbe\xe6\x95\x99\xe5\xb8\x88\xe7\xad\x89\xe6\x89\x80\xe6\x9c\x89\xe6\x9c\xac\xe5\x9c\xb0\xe6\x95\xb0\xe6\x8d\xae\xe4\xb8\x94\xe6\x97\xa0\xe6\xb3\x95\xe6\x92\xa4\xe9\x94\x80\xef\xbc\x8c\xe6\x98\xaf\xe5\x90\xa6\xe7\xbb\xa7\xe7\xbb\xad\xef\xbc\x9f"),
                                                             QMessageBox::Yes | QMessageBox::No,
                                                             QMessageBox::No);
    if (reply != QMessageBox::Yes)
        return;

    auto removeFileIfExists = [](const QString &path) {
        if (path.isEmpty())
            return;
        QFile file(path);
        if (file.exists())
            file.remove();
    };

    removeFileIfExists(baseScheduleFilePath());
    removeFileIfExists(actualScheduleFilePath());
    removeFileIfExists(reminderStoragePath());
    removeFileIfExists(teacherAssociationStoragePath());

    persistedBaseSchedules.clear();
    baseSchedules.clear();
    actualWeekCourses.clear();
    reminderEntries.clear();
    teacherAssignments.clear();
    teacherRosterHighlights.clear();
    teacherRosterIcons.clear();
    associationDataLoaded = false;
    baseScheduleDirty = false;
    editingNewBaseSchedule = false;
    unsavedBaseScheduleIndex = -1;
    forcedBaseScheduleIndex = -1;
    configSelectedBaseScheduleIndex = -1;

    if (baseScheduleList)
    {
        baseScheduleList->clear();
        baseScheduleList->setCurrentRow(-1);
    }
    refreshBaseScheduleList();
    updateSaveButtons();

    customTemporaryCourseName = QString::fromUtf8("\xe4\xb8\xb4\xe6\x97\xb6\xe8\xaf\xbe\xe7\xa8\x8b");
    customTemporaryCourseColor = QColor("#f97316");
    ensureCustomTemporaryCourseNameUnique();
    updateCustomTemporaryCourseLabel();

    refreshScheduleCells();
    refreshTimeColumn();
    updateWeeklyStatistics();

    if (reminderListWidget)
        reminderListWidget->clear();
    updateReminderActionStates();
    updateReminderCards();

    if (teacherRosterTable)
    {
        teacherRosterTable->clear();
        teacherRosterTable->setRowCount(0);
        teacherRosterTable->setColumnCount(0);
        adjustTeacherRosterCellSizes();
    }

    if (largeScheduleTable)
    {
        largeScheduleTable->clear();
        largeScheduleTable->setRowCount(1);
        largeScheduleTable->setColumnCount(1);
        clearLargeScheduleColumnSelections();
        clearLargeScheduleRowSelections();
        updateLargeScheduleColumnLabels();
        updateLargeScheduleRowLabels();
        adjustLargeScheduleCellSizes();
    }

    QMessageBox::information(this,
                             QString::fromUtf8("\xe9\x87\x8d\xe7\xbd\xae\xe5\xae\x8c\xe6\x88\x90"),
                             QString::fromUtf8("\xe6\x89\x80\xe6\x9c\x89\xe6\x9c\xac\xe5\x9c\xb0\xe6\x95\xb0\xe6\x8d\xae\xe5\xb7\xb2\xe6\xb8\x85\xe9\x99\xa4\xe3\x80\x82"));
}

bool MainWindow::ensureBaseScheduleCanClose()
{
    if (!baseScheduleDirty)
        return true;

    QMessageBox prompt(this);
    prompt.setIcon(QMessageBox::Question);
    prompt.setWindowTitle(QString::fromUtf8("\xe5\x9f\xba\xe7\xa1\x80\xe8\xaf\xbe\xe8\xa1\xa8\xe6\x9c\xaa\xe4\xbf\x9d\xe5\xad\x98"));
    prompt.setText(QString::fromUtf8("\xe5\xad\x98\xe5\x9c\xa8\xe5\xb0\x9a\xe6\x9c\xaa\xe4\xbf\x9d\xe5\xad\x98\xe7\x9a\x84\xe4\xbf\xae\xe6\x94\xb9\xef\xbc\x8c\xe6\x98\xaf\xe5\x90\xa6\xe7\xab\x8b\xe5\x8d\xb3\xe4\xbf\x9d\xe5\xad\x98\xef\xbc\x9f"));
    QPushButton *saveBtn = prompt.addButton(QString::fromUtf8("\xe4\xbf\x9d\xe5\xad\x98"), QMessageBox::AcceptRole);
    QPushButton *discardBtn = prompt.addButton(QString::fromUtf8("\xe6\x94\xbe\xe5\xbc\x83"), QMessageBox::DestructiveRole);
    QPushButton *cancelBtn = prompt.addButton(QString::fromUtf8("\xe5\x8f\x96\xe6\xb6\x88"), QMessageBox::RejectRole);
    prompt.exec();

    if (prompt.clickedButton() == saveBtn)
    {
        const int targetIndex = currentEditableBaseIndex();
        if (targetIndex < 0 || targetIndex >= baseSchedules.size())
            return true;
        if (!saveBaseScheduleToDisk(targetIndex))
            return false;
        updateDirtyStateAfterSave(targetIndex);
        return !baseScheduleDirty;
    }
    if (prompt.clickedButton() == discardBtn)
    {
        discardBaseScheduleChanges();
        return true;
    }
    Q_UNUSED(cancelBtn);
    return false;
}






