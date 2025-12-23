#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDate>
#include <QTime>
#include <QVector>
#include <QColor>
#include <QFont>
#include <QMap>
#include <QFrame>
#include <QPoint>
#include <QDateTime>
#include <QSet>
#include <QHash>
#include <QTimer>

#include "schedulecelllabel.h"

#pragma execution_character_set("utf-8")

class QFrame;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QTableWidgetItem;
class QGridLayout;
class QDateEdit;
class QDateTimeEdit;
class QTimeEdit;
class QComboBox;
class QListWidget;
class QLineEdit;
class QHBoxLayout;
class QVBoxLayout;
class QDockWidget;
class QCheckBox;
class QMouseEvent;
namespace QtCharts {
class QChartView;
}

class TermSettingsDialog;
class CourseEditDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    struct CourseCellData;
    struct RowDefinition;
    struct BaseScheduleEntry;
    struct ActualWeekEntry;
    struct ReminderEntry;
    struct ReminderOccurrence;

    enum class SecondaryPanel
    {
        None,
        Config,
        TemporaryAdjust,
        Reminder,
        DisplayMode
    };

    void setupUi();
    QWidget *buildMainToolbar();
    QWidget *buildTemporaryPanel();
    QWidget *buildReminderPanel();
    QWidget *buildConfigPanel();
    QWidget *buildDisplayModePanel();
    void setupScheduleTable();
    void connectSignals();
    void applyDisplayPreferences();
    void applyChromeVisibility(bool visible);
    void applyManualMinimalMode(bool minimal);
    void adjustWindowToKeepTable(const QPoint &targetTopLeftGlobal, const QSize &targetSize);
    void setCaptionVisible(bool visible);
    QRect clientRectFromFrame(const QRect &frame) const;
    QHash<QString, QHash<QString, QList<int>>> buildReferenceScheduleIndex() const;
    bool handleFramelessResizeEvent(QObject *watched, QEvent *event);
    enum class ResizeArea
    {
        None,
        Left,
        Right,
        Top,
        Bottom,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    };
    ResizeArea hitTestResizeArea(const QPoint &pos) const;
    bool canUseFramelessResize() const;
    bool isDisplayViewActive() const;
    bool shouldAutoHideChrome() const;
    void scheduleChromeAutoHide();
    void cancelChromeAutoHide();
    void updateEffectiveDateEditors();
    void handleEffectiveDateChanged();
    bool validateEffectiveDateRange(const QDate &start, const QDate &end, bool showWarning = true);
    QDate ensureStartMonday(const QDate &date) const;
    QDate ensureEndSunday(const QDate &start, const QDate &candidate) const;
    void refreshBaseScheduleList();
    void ensureConfigSelection();
    void handleBaseScheduleSelectionChanged();
    void setForcedBaseScheduleIndex(int index);
    void handleWeekNavigation(int offsetDays);
    void updateWeekLabel();
    void refreshDateHeader();
    void refreshTimeColumn();
    void initializeScheduleData();
    void refreshScheduleCells();
    void ensureCellWidgets();
    void synchronizeScheduleRowCount();
    void applyRowLayout();
    void initializeQuickCoursePalette();
    void updateQuickCourseButtons();
    void adjustSecondaryContainerHeight();
    void updateToolbarInteractivity();
    void updateAdaptiveCellFonts();
    void applyAdaptiveFontToItem(QTableWidgetItem *item, int row, int column);
    void showSecondaryPanel(SecondaryPanel panel);
    QDate mondayOf(const QDate &date) const;
    void loadTermSettings();
    void saveTermSettings();
    bool ensureTermConfigured();
    bool openTermSettingsDialog();
    bool ensureBaseScheduleCanClose();
    QString baseScheduleFilePath() const;
    bool loadBaseScheduleFromDisk();
    bool saveBaseScheduleToDisk(int targetIndex);
    void discardBaseScheduleChanges();
    void markBaseScheduleDirty(bool dirty = true);
    void updateSaveButtons();
    void handleSaveBaseSchedule();
    void handleDiscardBaseSchedule();
    void handleResetAllData();
    const BaseScheduleEntry *activeBaseSchedule() const;
    BaseScheduleEntry *activeBaseScheduleMutable();
    const ActualWeekEntry *findActualWeek(const QDate &weekStart) const;
    ActualWeekEntry *findActualWeekMutable(const QDate &weekStart);
    void ensureActualWeekExists(const QDate &weekStart, bool createIfMissing = false);
    bool isConfigModeActive() const;
    bool isBaseIndexEditable(int index) const;
    bool isSelectedBaseEditable() const;
    bool canEditSelectedBaseSchedule() const;
    QVector<int> overlappingBaseScheduleIndexes(int candidateIndex) const;
    QString buildOverlapWarning(const BaseScheduleEntry &candidate, const QVector<int> &overlaps) const;
    void resolveOverlapsForNewSchedule(int &newIndex);
    void adjustScheduleListForOverlap(QVector<BaseScheduleEntry> &list, int &newIndexRef, const BaseScheduleEntry &candidate);
    bool isTemporaryAdjustModeActive() const;
    void updateWeeklyStatistics();
    void handleTemporaryDrop(int sourceRow, int sourceColumn, int targetRow, int targetColumn, const QString &name, const QColor &color, int sourceType);
    void handleWeeklyCourseDrop(int targetRow, int targetColumn, const QString &name, const QColor &color);
    void handleWeeklyStatsDeletionDrop(int sourceRow, int sourceColumn, const QString &name, const QColor &color, ScheduleCellLabel::CourseType type, const QPoint &globalPos);
    void playWeeklyStatsDeletionEffect(const QPoint &globalPos);
    void editCustomTemporaryCourse();
    void updateCustomTemporaryCourseLabel();
    void startCustomTemporaryCourseDrag();
    void handleResetTemporaryAdjustments();
    void handleAddReminder();
    void handleRemoveReminder();
    void handleReminderTypeChanged(int index);
    void handleSaveReminderChanges();
    void updateReminderActionStates();
    void handleShowSelectedReminders();
    void handleHideSelectedReminders();
    void handleShowAllReminders();
    void handleHideAllReminders();
    void populateReminderForm(int index);
    void clearReminderForm();
    bool buildReminderEntryFromForm(ReminderEntry &entry, QString &error) const;
    void refreshReminderList();
    void loadReminders();
    void saveReminders() const;
    QString reminderStoragePath() const;
    QList<ReminderOccurrence> remindersForCurrentWeek() const;
    void updateReminderCards();
    void updateReminderOverlayGeometry() const;
    bool reminderPositionForTime(const QTime &time, int &rowIndex, qreal &rowRatio) const;
    bool extractRowTimeRange(int lessonIndex, QTime &start, QTime &end) const;
    bool baseCellHasCourse(int row, int column) const;
    CourseCellData baseCellDataAt(int row, int column) const;
    CourseCellData actualOverrideAt(int row, int column) const;
    void setActualOverride(int row, int column, const CourseCellData &data);
    void loadActualScheduleFromDisk();
    void saveActualScheduleToDisk() const;
    QString actualScheduleFilePath() const;
    bool activeBaseContainsCourseName(const QString &name) const;
    QString generateUniqueCustomCourseName(const QString &seed) const;
    void ensureCustomTemporaryCourseNameUnique();
    void showLargeScheduleDialog();
    void ensureLargeScheduleDialog();
    void handleDeleteLargeScheduleColumns();
    void handleImportLargeSchedule();
    void buildTeacherRosterDock();
    void handleImportTeacherRoster();
    void handleLargeScheduleCellClicked(int row, int column);
    void adjustLargeScheduleCellSizes();
    void adjustTeacherRosterCellSizes();
    void removeEmptyLargeScheduleRows();
    void removeEmptyLargeScheduleColumns();
    void applyLargeScheduleSelectorClick(int column, Qt::KeyboardModifiers modifiers);
    void applyLargeScheduleSelectorDrag(int column);
    void applyLargeScheduleSelectorRange(int startColumn, int endColumn, bool additive);
    void applyLargeScheduleRowSelectorClick(int row, Qt::KeyboardModifiers modifiers);
    void applyLargeScheduleRowSelectorDrag(int row);
    void applyLargeScheduleRowSelectorRange(int startRow, int endRow, bool additive);
    void clearLargeScheduleColumnSelections();
    void clearLargeScheduleRowSelections();
    void updateLargeScheduleSelectorVisuals();
    void updateLargeScheduleColumnLabels();
    void updateLargeScheduleRowLabels();
    QString largeScheduleColumnLabel(int columnIndex) const;
    void toggleLargeScheduleSelectorsVisibility();
    void configureLargeScheduleColors();
    void applyCourseColorsToLargeSchedule();
    void positionTeacherRosterDock();
    void handleAssociateTeacherRoster();
    QString classColumnIndexStoragePath() const;
    QString teacherAssociationStoragePath() const;
    void showStatisticsDialog();
    void ensureTeacherAssociationsLoaded();
    void checkTeacherConflicts(int row, int column);
    void clearConflictHighlights();
    void setConflictDetectionEnabled(bool enabled);
    void toggleTeacherRosterVisibility(bool visible);
    void handleAddLessonRow();
    void handleAddMergedRow();
    void handleDeleteRow();
    void handleCreateBaseSchedule();
    void handleTableCellClicked(int row, int column);
    void handleCourseCellDoubleClicked(int row, int column);
    void editTimeSlot(int row);
    void editCourseCell(int row, int column);
    bool applyCourseData(int dataRow, int dataColumn, const CourseCellData &data);
    void recordCourseTemplate(const QString &name, const QColor &color);
    void applyActiveCourseToCell(int row, int column);
    void selectQuickCourseButton(QPushButton *button);
    bool promptTimeSlotTexts(QString &title, QString &timeRange);
    bool hasOverlapWithOthers(int candidateIndex, QString *detailMessage = nullptr) const;
    int currentEditableBaseIndex() const;
    void updateDirtyStateAfterSave(int savedIndex);

    QWidget *mainToolbarWidget;
    QPushButton *previousWeekButton;
    QPushButton *nextWeekButton;
    QPushButton *currentWeekButton;
    QPushButton *temporaryAdjustButton;
    QPushButton *addReminderButton;
    QPushButton *configButton;
    QPushButton *displayModeButton;
    QPushButton *minimalToggleButton;
    QPushButton *statisticsButton;
    QPushButton *addMergedRowButton;
    QPushButton *addLessonRowButton;
    QPushButton *deleteRowButton;
    QPushButton *saveScheduleButton;
    QPushButton *discardScheduleButton;
    QPushButton *resetTemporaryAdjustButton;
    QPushButton *weekLabel;
    QPushButton *teacherRosterButton;
    QWidget *quickCourseWidget;
    QGridLayout *quickCourseLayout;
    QPushButton *activePaletteButton;
    QString activePaletteCourse;
    QColor activePaletteColor;
    QPushButton *clearPaletteButton;
    QPushButton *addBaseScheduleButton;
    QPushButton *resetAllDataButton;
    QDateEdit *baseStartDateEdit;
    QDateEdit *baseEndDateEdit;
    QListWidget *baseScheduleList;
    QWidget *centralContainer;
    QVBoxLayout *mainLayoutRoot;

    QFrame *secondaryContainer;
    QStackedWidget *secondaryStack;
    QWidget *configPanel;
    QWidget *temporaryAdjustPanel;
    QWidget *reminderPanel;
    QWidget *displayModePanel;
    QListWidget *reminderListWidget;
    QLineEdit *reminderTitleEdit;
    QComboBox *reminderTypeCombo;
    QDateTimeEdit *reminderOnceDateTimeEdit;
    QDateEdit *reminderStartDateEdit;
    QDateEdit *reminderEndDateEdit;
    QTimeEdit *reminderTimeEdit;
    QComboBox *reminderWeekdayCombo;
    QPushButton *addReminderItemButton;
    QPushButton *removeReminderItemButton;
    QPushButton *saveReminderItemButton;
    QPushButton *showSelectedRemindersButton;
    QPushButton *hideSelectedRemindersButton;
    QPushButton *showAllRemindersButton;
    QPushButton *hideAllRemindersButton;
    QWidget *reminderOnceFields;
    QWidget *reminderRepeatFields;
    QWidget *reminderWeeklyExtraFields;
    QWidget *reminderOverlay;
    QVector<QWidget *> reminderCardWidgets;
    QMainWindow *largeScheduleDialog;
    QDialog *statisticsDialog;
    QTableWidget *largeScheduleTable;
    QPushButton *toggleLargeScheduleSelectorsButton;
    QPushButton *configureLargeScheduleColorsButton;
    QPushButton *conflictDetectionButton;
    QPushButton *associateRosterButton;
    QDockWidget *teacherRosterDock;
    QTableWidget *teacherRosterTable;
    QtCharts::QChartView *statisticsChartViewActual;
    QtCharts::QChartView *statisticsChartViewBase;
    QVBoxLayout *statisticsListLayoutActual;
    QVBoxLayout *statisticsListLayoutBase;
    QLabel *statisticsEvaluationLabelActual;
    QLabel *statisticsEvaluationLabelBase;
    QLabel *statisticsLessonSumLabelActual;
    QLabel *statisticsLessonSumLabelBase;
    QSet<int> largeScheduleSelectedColumns;
    QSet<int> largeScheduleSelectedRows;
    bool largeScheduleSelectorDragging;
    bool largeScheduleRowDragging;
    int largeScheduleSelectorAnchorColumn;
    int largeScheduleSelectorLastColumn;
    int largeScheduleRowAnchor;
    int largeScheduleRowLast;
    bool largeScheduleSelectorsVisible;
    QHBoxLayout *weeklyStatsLayout;
    QWidget *weeklyStatsContainer;
    QLabel *weeklyStatsPlaceholder;
    QWidget *weeklyStatsViewport;
    QLabel *customTemporaryCourseLabel;
    QCheckBox *showWeekendToggle;
    QCheckBox *showChromeToggle;
    QTimer *chromeAutoHideTimer;
    QTimer *toolbarHideTimer;
    bool showWeekendInDisplay;
    bool showChromeInDisplay;
    bool chromeHiddenInDisplay;
    bool manualMinimalMode;
    bool savedFrameGeometryValid;
    QRect savedFrameGeometry;
    bool displayDragging;
    bool resizeInProgress;
    ResizeArea resizeArea;
    QPoint resizeStartPos;
    QRect resizeStartGeometry;
    QPoint displayDragOffset;
    Qt::WindowFlags normalWindowFlags;
    SecondaryPanel activePanel;

    QTableWidget *scheduleTable;
    QStringList lessonTimes;
    QDate currentWeekStart;
    QDate termStartDate;
    QDate termEndDate;
    bool hasTermRange;
    int selectedTimeRow;
    QMap<QString, QColor> courseColorMap;
    bool baseScheduleDirty;
    bool suppressDateEditSignal;
    QVector<BaseScheduleEntry> persistedBaseSchedules;
    int forcedBaseScheduleIndex;
    int configSelectedBaseScheduleIndex;
    bool editingNewBaseSchedule;
    int unsavedBaseScheduleIndex;

    QHash<QString, QHash<QString, int>> classClumnNums; //班级列号

    struct CourseCellData
    {
        QString name;
        QString className;
        QColor background;
        ScheduleCellLabel::CourseType type = ScheduleCellLabel::CourseType::None;

        static CourseCellData empty()
        {
            return {};
        }

        static CourseCellData create(const QString &n, const QColor &c, ScheduleCellLabel::CourseType t = ScheduleCellLabel::CourseType::Normal, const QString &cls = QString())
        {
            CourseCellData data;
            data.name = n;
            data.className = cls;
            data.background = c;
            data.type = t;
            return data;
        }
    };

    struct RowDefinition
    {
        enum class Kind
        {
            Placeholder,
            Lesson
        };

        Kind kind = Kind::Lesson;
    };

    struct BaseScheduleEntry
    {
        QDate startDate;
        QDate endDate;
        int rows = 0;
        int columns = 0;
        QVector<RowDefinition> rowDefinitions;
        QStringList timeColumnTexts;
        QVector<QVector<CourseCellData>> courseCells;
    };

    struct ActualWeekEntry
    {
        QDate weekStart;
        QMap<int, QMap<int, CourseCellData>> cellOverrides;
    };

    struct ReminderEntry
    {
        enum class Type
        {
            Once,
            Daily,
            Weekly
        };

        QString title;
        Type type = Type::Once;
        QDateTime onceDateTime;
        QDate startDate;
        QDate endDate;
        QTime time;
        int weekday = 1; // 1=Mon..7
        bool visible = true;
    };

    struct ReminderOccurrence
    {
        QDate date;
        QTime time;
        QString title;
        QString detail;
    };

    QVector<BaseScheduleEntry> baseSchedules;
    QVector<ActualWeekEntry> actualWeekCourses;
    QVector<ReminderEntry> reminderEntries;
    QVector<QVector<ScheduleCellLabel *>> cellLabels;
    bool conflictDetectionEnabled;
    bool associationDataLoaded;
    QFont scheduleHeaderBaseFont;
    QString customTemporaryCourseName;
    QColor customTemporaryCourseColor;
    QPoint customTempDragStartPos;

    struct ConflictHighlight
    {
        QTableWidgetItem *item = nullptr;
        QBrush originalBrush;
    };
    QVector<ConflictHighlight> conflictHighlights;

    struct TeacherRosterHighlight
    {
        QTableWidgetItem *item = nullptr;
        QBrush originalBrush;
    };
    QVector<TeacherRosterHighlight> teacherRosterHighlights;

    struct TeacherRosterIcon
    {
        QTableWidgetItem *item = nullptr;
        QString originalText;
    };
    QVector<TeacherRosterIcon> teacherRosterIcons;

    struct TeacherAssignment
    {
        QHash<QString, QSet<QString>> subjectToClasses;
    };
    QHash<QString, TeacherAssignment> teacherAssignments;
};

#endif // MAINWINDOW_H
