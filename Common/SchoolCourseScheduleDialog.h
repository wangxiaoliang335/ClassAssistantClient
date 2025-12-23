#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QDate>
#include <QMap>
#include <QSet>
#include <QRegExp>
#include <QTimer>
#include <QSignalBlocker>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QShowEvent>
#include <QAbstractButton>
#include <QScrollBar>
#include <QApplication>
#include <QCursor>
#include <QFontMetrics>
#include <QPainterPath>

#include "CommonInfo.h"

// 自定义表格类：重绘背景
class CustomCourseScheduleTable : public QTableWidget
{
public:
    explicit CustomCourseScheduleTable(QWidget* parent = nullptr)
        : QTableWidget(parent)
    {
        // 设置表格背景为浅灰色
        setStyleSheet(
            "QTableWidget { background-color: #5C5C5C; }"
            "QTableWidget::viewport { background-color: #5C5C5C; }"
        );
    }
    
    void showEvent(QShowEvent* event) override
    {
        QTableWidget::showEvent(event);
        // 在显示后设置表头角落按钮的样式
        updateCornerButtonStyle();
    }
    
    void updateCornerButtonStyle()
    {
        // 查找并设置表头角落按钮的样式
        // 角落按钮通常在表格的所有子控件中
        QList<QAbstractButton*> buttons = findChildren<QAbstractButton*>();
        for (QAbstractButton* btn : buttons) {
            // 检查是否是角落按钮（通常位置在左上角）
            QRect btnRect = btn->geometry();
            if (btnRect.x() < 50 && btnRect.y() < 50) { // 大概在左上角区域
                btn->setStyleSheet(
                    "QAbstractButton { "
                    "background-color: #5C5C5C !important; "
                    "border: 1px solid #5C5C5C !important; "
                    "}"
                );
            }
        }
        
        // 也通过样式表设置
        if (horizontalHeader() && verticalHeader()) {
            horizontalHeader()->setStyleSheet(
                "QHeaderView::section { background-color: #5C5C5C; color: white; }"
            );
            verticalHeader()->setStyleSheet(
                "QHeaderView::section { background-color: #5C5C5C; color: white; }"
            );
        }
        
        // 延迟更新，确保控件已完全创建
        QApplication::processEvents();
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        
        // 绘制整个表格区域的背景（浅灰色）
        painter.fillRect(rect(), QColor("#5C5C5C"));
        
        // 绘制 viewport 背景
        if (viewport()) {
            QRect viewportRect = viewport()->geometry();
            painter.fillRect(viewportRect, QColor("#5C5C5C"));
        }
        
        // 绘制水平表头背景
        if (horizontalHeader() && horizontalHeader()->isVisible()) {
            QRect headerRect = horizontalHeader()->geometry();
            painter.fillRect(headerRect, QColor("#5C5C5C"));
        }
        
        // 绘制垂直表头背景
        if (verticalHeader() && verticalHeader()->isVisible()) {
            QRect headerRect = verticalHeader()->geometry();
            painter.fillRect(headerRect, QColor("#5C5C5C"));
        }
        
        // 调用基类的绘制方法
        QTableWidget::paintEvent(event);
        
        // 在基类绘制后，再次绘制表头角落区域，确保覆盖白色背景
        if (horizontalHeader() && verticalHeader() && 
            horizontalHeader()->isVisible() && verticalHeader()->isVisible()) {
            QRect hHeaderRect = horizontalHeader()->geometry();
            QRect vHeaderRect = verticalHeader()->geometry();
            
            // 查找角落按钮并绘制其背景
            QAbstractButton* cornerButton = findChild<QAbstractButton*>();
            if (cornerButton && cornerButton->isVisible()) {
                QRect cornerBtnRect = cornerButton->geometry();
                painter.fillRect(cornerBtnRect, QColor("#5C5C5C"));
            }
            
            // 绘制角落区域（行头和列头交叉处）- 使用表头的实际位置
            QRect cornerRect(vHeaderRect.x(), hHeaderRect.y(), 
                           vHeaderRect.width(), hHeaderRect.height());
            painter.fillRect(cornerRect, QColor("#5C5C5C"));
            
            // 也绘制在 (0,0) 位置，以防表头位置计算有误
            QRect cornerRect2(0, 0, vHeaderRect.width(), hHeaderRect.height());
            painter.fillRect(cornerRect2, QColor("#5C5C5C"));
            
            // 如果表头有偏移，也需要在偏移位置绘制
            if (hHeaderRect.x() > 0 || vHeaderRect.y() > 0) {
                QRect offsetCornerRect(vHeaderRect.x(), hHeaderRect.y(), 
                                      vHeaderRect.width(), hHeaderRect.height());
                painter.fillRect(offsetCornerRect, QColor("#5C5C5C"));
            }
        }
    }
};

// 学校课程表总览：按年级查看该年级所有班级的课程表（数据源：/getClassesByPrefix + /course-schedule）
class SchoolCourseScheduleDialog : public QDialog
{
public:
    explicit SchoolCourseScheduleDialog(QWidget* parent = nullptr)
        : QDialog(parent)
        , m_dragging(false)
    {
        // 去掉标题栏
        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        // 设置窗口透明背景，以便绘制圆角
        setAttribute(Qt::WA_TranslucentBackground);
        setWindowTitle(QString::fromUtf8(u8"年级课程表"));
        resize(1280, 800);
        setStyleSheet(
            "QDialog{background-color:#2f2f2f;color:#eaeaea;}"
            "QWidget{background-color:#2f2f2f;}"
            "QLabel{color:#eaeaea;background-color:transparent;}"
            "QComboBox{background-color:#3b3b3b;color:#eaeaea;border:1px solid #555;padding:6px 10px;border-radius:6px;}"
            "QPushButton{background-color:#3b3b3b;color:#eaeaea;border:1px solid #555;padding:6px 10px;border-radius:6px;}"
            "QPushButton:hover{background-color:#454545;}"
            "QHBoxLayout{background-color:transparent;}"
            "QVBoxLayout{background-color:transparent;}"
            "QTableWidget{background-color:#2b2b2b;color:#eaeaea;gridline-color:#444;}"
            "QHeaderView::section{background-color:#3a3a3a;color:#eaeaea;border:1px solid #444;padding:6px;}"
        );

        m_manager = new QNetworkAccessManager(this);

        QVBoxLayout* root = new QVBoxLayout(this);
        root->setContentsMargins(12, 12, 12, 12);
        root->setSpacing(10);

        // 顶部栏：标题 + 年级下拉 + 刷新 + 关闭
        QHBoxLayout* top = new QHBoxLayout;
        top->setSpacing(10);
        QLabel* title = new QLabel(QString::fromUtf8(u8"课程表"), this);
        title->setStyleSheet("font-size:16px;font-weight:700;");
        top->addWidget(title);
        top->addStretch();

        m_gradeCombo = new QComboBox(this);
        m_gradeCombo->setMinimumWidth(180);
        m_gradeCombo->setStyleSheet(
            "QComboBox { "
            "background-color: #3b3b3b; "
            "color: white; "
            "border: 1px solid #555; "
            "padding: 6px 10px; "
            "padding-right: 30px; "
            "border-radius: 6px; "
            "font-weight: bold; "
            "}"
            "QComboBox::drop-down { "
            "border: none; "
            "background-color: transparent; "
            "width: 20px; "
            "}"
            "QComboBox::down-arrow { "
            "image: none; "
            "width: 0; "
            "height: 0; "
            "border-left: 5px solid transparent; "
            "border-right: 5px solid transparent; "
            "border-top: 6px solid #5C5C5C; "
            "margin-right: 5px; "
            "}"
            "QComboBox QAbstractItemView { "
            "background-color: #3b3b3b; "
            "color: white; "
            "selection-background-color: #555; "
            "selection-color: white; "
            "border: 1px solid #555; "
            "}"
        );
        top->addWidget(m_gradeCombo);

        // 预留：上传/下载（暂不实现）
        QPushButton* downloadBtn = new QPushButton(QString::fromUtf8(u8"下载"), this);
        QPushButton* uploadBtn = new QPushButton(QString::fromUtf8(u8"上传"), this);
        top->addWidget(downloadBtn);
        top->addWidget(uploadBtn);

        m_refreshBtn = new QPushButton(QString::fromUtf8(u8"刷新"), this);
        top->addWidget(m_refreshBtn);

        // 关闭按钮（初始隐藏，鼠标移入窗口时显示）
        m_closeBtn = new QPushButton("×", this);
        m_closeBtn->setFixedSize(30, 30);
        m_closeBtn->setStyleSheet(
            "QPushButton{background-color:#3a3a3a;color:white;border:none;border-radius:15px;font-weight:bold;font-size:20px;}"
            "QPushButton:hover{background-color:#4a4a4a;}"
        );
        m_closeBtn->setAttribute(Qt::WA_OpaquePaintEvent, false); // 允许透明绘制
        m_closeBtn->hide(); // 初始隐藏
        top->addWidget(m_closeBtn);
        root->addLayout(top);

        // 主体：表格（使用自定义表格类）
        m_table = new CustomCourseScheduleTable(this);
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_table->setWordWrap(true);
        // 表格样式
        m_table->setStyleSheet(
            "QTableWidget { "
            "background-color: #5C5C5C; "
            "color: white; "
            "gridline-color: #777777; "
            "border: 1px solid #777777; "
            "}"
            "QTableWidget::item { "
            "background-color: #5C5C5C; "
            "color: white; "
            "padding: 5px; "
            "}"
            "QHeaderView::section { "
            "background-color: #5C5C5C; "
            "color: white; "
            "border: 1px solid #777777; "
            "padding: 5px; "
            "}"
            "QTableCornerButton::section { "
            "background-color: #5C5C5C; "
            "border: 1px solid #777777; "
            "}"
            "QAbstractScrollArea::corner { "
            "background-color: #5C5C5C; "
            "}"
        );
        root->addWidget(m_table, 1);

        // 状态栏
        m_status = new QLabel(QString::fromUtf8(u8"就绪"), this);
        m_status->setStyleSheet("color:#cfcfcf;");
        root->addWidget(m_status);

        connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);
        connect(m_refreshBtn, &QPushButton::clicked, this, [this]() { refresh(); });
        connect(downloadBtn, &QPushButton::clicked, this, [this]() {
            QMessageBox::information(this, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"下载功能暂未实现"));
        });
        connect(uploadBtn, &QPushButton::clicked, this, [this]() {
            QMessageBox::information(this, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"上传功能暂未实现"));
        });
        connect(m_gradeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            const QString grade = m_gradeCombo->currentData().toString();
            if (!grade.isEmpty()) {
                fetchSchedulesForGrade(grade);
            }
        });
    }

    void refresh()
    {
        m_status->setText(QString::fromUtf8(u8"正在获取班级列表..."));
        fetchClassesForSchool();
    }

protected:
    // 重写 paintEvent 绘制圆角背景
    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing); // 启用抗锯齿，使圆角更平滑
        
        // 创建圆角矩形路径
        QPainterPath path;
        int radius = 15; // 圆角半径
        QRect rect = this->rect();
        path.addRoundedRect(rect, radius, radius);
        
        // 绘制圆角背景
        painter.fillPath(path, QColor("#2f2f2f"));
        
        QDialog::paintEvent(event);
    }

    // 鼠标事件处理：实现窗口拖拽
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            // 检查点击的是否是按钮或其他控件，如果是则不拖拽
            QWidget* child = childAt(event->pos());
            if (child && (qobject_cast<QPushButton*>(child) || 
                          qobject_cast<QComboBox*>(child) ||
                          qobject_cast<QTableWidget*>(child) ||
                          child->parent() == m_table)) {
                QDialog::mousePressEvent(event);
                return;
            }
            // 允许拖拽的区域（主要是标题栏区域）
            m_dragging = true;
            m_dragStartPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
        QDialog::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPosition);
            event->accept();
        }
        QDialog::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            event->accept();
        }
        QDialog::mouseReleaseEvent(event);
    }

    // 鼠标进入/离开事件：控制关闭按钮显示/隐藏
    void enterEvent(QEvent* event) override
    {
        if (m_closeBtn) {
            m_closeBtn->show();
        }
        QDialog::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        // 只有当鼠标真正移出窗口时，才隐藏关闭按钮
        // 使用定时器延迟检查，避免点击 ComboBox 等控件时误触发
        QTimer::singleShot(100, this, [this]() {
            QPoint globalPos = QCursor::pos();
            QPoint windowTopLeft = this->mapToGlobal(QPoint(0, 0));
            QRect globalWindowRect(windowTopLeft, this->size());
            
            // 检查鼠标是否真的在窗口外
            if (!globalWindowRect.contains(globalPos)) {
                // 还要检查是否在 ComboBox 的下拉菜单上
                bool onComboBoxMenu = false;
                if (m_gradeCombo && m_gradeCombo->view() && m_gradeCombo->view()->isVisible()) {
                    QPoint menuTopLeft = m_gradeCombo->view()->mapToGlobal(QPoint(0, 0));
                    QRect menuRect(menuTopLeft, m_gradeCombo->view()->size());
                    if (menuRect.contains(globalPos)) {
                        onComboBoxMenu = true;
                    }
                }
                
                if (!onComboBoxMenu && m_closeBtn) {
                    m_closeBtn->hide();
                }
            }
        });
        QDialog::leaveEvent(event);
    }

private:
    struct ClassInfo {
        QString classId;     // class_code
        QString stage;       // school_stage
        QString grade;       // grade
        QString className;   // class_name
        QString displayName; // 用于表头展示（比如：3班/七(3)班等）
    };

    struct ScheduleData {
        QStringList days;
        QStringList times;
        // row -> col -> course
        QMap<int, QMap<int, QString>> cellText;
        QSet<QPair<int,int>> highlightCells;
        bool filteredFirstColumn = false;
    };

    static QString currentTermString()
    {
        // 与 CourseDialog / ScheduleDialog 逻辑对齐
        const QDate currentDate = QDate::currentDate();
        const int year = currentDate.year();
        const int month = currentDate.month();

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
            // 1-2月属于上一学年第一学期
            startYear = year - 1;
            endYear = year;
            termIndex = 1;
        }

        return QString("%1-%2-%3").arg(startYear).arg(endYear).arg(termIndex);
    }

    static bool looksLikeTimeText(const QString& text)
    {
        static QRegExp preciseTimePattern(QStringLiteral("^\\d{1,2}:\\d{2}(?::\\d{2}(?:\\.\\d{1,3})?)?$"));
        if (preciseTimePattern.exactMatch(text)) return true;
        QRegExp rangePattern(QStringLiteral("^(\\d{1,2}:\\d{2})\\s*-\\s*(\\d{1,2}:\\d{2})$"));
        if (rangePattern.exactMatch(text)) return true;
        return false;
    }

    void fetchClassesForSchool()
    {
        const QString schoolId = CommonInfo::GetData().schoolId;
        if (schoolId.trimmed().isEmpty()) {
            m_status->setText(QString::fromUtf8(u8"schoolId 为空，无法获取班级列表"));
            return;
        }
        QString schoolIdTrimmed = schoolId.trimmed();
        if (schoolIdTrimmed.length() != 6 || !schoolIdTrimmed.toInt()) {
            m_status->setText(QString::fromUtf8(u8"schoolId 格式错误，应为6位数字"));
            return;
        }

        // 获取最新一学期的课程表
        const QString term = currentTermString();
        m_status->setText(QString::fromUtf8(u8"正在获取课程表（") + term + QString::fromUtf8(u8"）..."));

        // 使用新接口 GET /course-schedule/school?school_id=xxx&term=xxx
        QUrl url(QStringLiteral("http://47.100.126.194:5000/course-schedule/school"));
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("school_id"), schoolIdTrimmed);
        q.addQueryItem(QStringLiteral("term"), term);
        url.setQuery(q);

        QNetworkRequest request(url);
        QNetworkReply* reply = m_manager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply, term]() {
            const QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> guard(reply);
            if (reply->error() != QNetworkReply::NoError) {
                m_status->setText(QString::fromUtf8(u8"获取课程表失败：") + reply->errorString());
                return;
            }
            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (!doc.isObject()) {
                m_status->setText(QString::fromUtf8(u8"课程表响应解析失败"));
                return;
            }
            const QJsonObject root = doc.object();
            const int code = root.value("code").toInt(-1);
            if (code != 200) {
                m_status->setText(QString::fromUtf8(u8"获取课程表失败：") + root.value("message").toString());
                return;
            }

            const QJsonArray dataArray = root.value("data").toArray();
            if (dataArray.isEmpty()) {
                m_status->setText(QString::fromUtf8(u8"未获取到任何班级课程表"));
                return;
            }

            m_classesByGrade.clear();
            m_pendingSchedules.clear();
            m_pendingTerm = term;

            // 解析返回的数据：每个元素包含班级信息和课程表
            for (const QJsonValue& v : dataArray) {
                if (!v.isObject()) continue;
                const QJsonObject classObj = v.toObject();

                ClassInfo info;
                info.classId = classObj.value("class_id").toString().trimmed();
                info.className = classObj.value("class_name").toString().trimmed();
                info.stage = classObj.value("school_stage").toString().trimmed();
                info.grade = classObj.value("grade").toString().trimmed();

                if (info.classId.isEmpty()) continue;

                // displayName 尽量只显示"X班"；为空则回退 classId
                info.displayName = info.className;
                if (info.displayName.isEmpty()) info.displayName = info.classId;

                const QString gradeKey = !info.grade.isEmpty() ? info.grade : info.stage;
                if (gradeKey.isEmpty()) continue;
                m_classesByGrade[gradeKey].append(info);

                // 解析课程表数据
                const QJsonObject schedulesObj = classObj.value("schedules").toObject();
                if (!schedulesObj.isEmpty()) {
                    const QJsonObject scheduleObj = schedulesObj.value("schedule").toObject();
                    const QJsonArray cells = schedulesObj.value("cells").toArray();

                    ScheduleData sd;
                    sd.days = jsonArrayToStringList(scheduleObj.value("days").toArray());
                    sd.times = jsonArrayToStringList(scheduleObj.value("times").toArray());
                    parseCellsInto(sd, cells);
                    m_pendingSchedules[info.classId] = sd;
                }
            }

            updateGradeCombo();
        });
    }

    void updateGradeCombo()
    {
        m_gradeCombo->blockSignals(true);
        m_gradeCombo->clear();
        QStringList grades = m_classesByGrade.keys();
        grades.sort();
        for (const QString& g : grades) {
            // 显示格式：一年级课程表、二年级课程表等
            QString displayText = g + QString::fromUtf8(u8"课程表");
            m_gradeCombo->addItem(displayText, g);
        }
        m_gradeCombo->blockSignals(false);

        if (m_gradeCombo->count() == 0) {
            m_status->setText(QString::fromUtf8(u8"未获取到任何班级"));
            return;
        }

        const QString grade = m_gradeCombo->currentData().toString();
        // 如果已经获取了当前学期的课程表数据，直接构建表格；否则需要重新获取
        const QString currentTerm = currentTermString();
        if (!m_pendingTerm.isEmpty() && m_pendingTerm == currentTerm && !m_pendingSchedules.isEmpty()) {
            // 检查该年级的所有班级是否都有数据
            const QList<ClassInfo> classes = m_classesByGrade.value(grade);
            bool hasAllData = true;
            for (const ClassInfo& cls : classes) {
                if (!m_pendingSchedules.contains(cls.classId)) {
                    hasAllData = false;
                    break;
                }
            }
            if (hasAllData) {
                m_pendingGrade = grade;
                m_status->setText(QString::fromUtf8(u8"班级列表已更新，正在显示课程表"));
                buildGradeTable(grade, m_pendingTerm);
            } else {
                m_status->setText(QString::fromUtf8(u8"班级列表已更新，选择年级后将拉取课程表"));
                fetchSchedulesForGrade(grade);
            }
        } else {
            m_status->setText(QString::fromUtf8(u8"班级列表已更新，选择年级后将拉取课程表"));
            fetchSchedulesForGrade(grade);
        }
    }

    void fetchSchedulesForGrade(const QString& grade)
    {
        if (!m_classesByGrade.contains(grade)) {
            m_status->setText(QString::fromUtf8(u8"年级不存在：") + grade);
            return;
        }

        const QString term = currentTermString();
        const QList<ClassInfo> classes = m_classesByGrade.value(grade);
        if (classes.isEmpty()) {
            m_status->setText(QString::fromUtf8(u8"该年级无班级"));
            return;
        }

        // 检查是否已经有当前学期的数据（从 fetchClassesForSchool 获取的）
        bool hasAllData = true;
        if (m_pendingTerm != term) {
            hasAllData = false;
        } else {
            for (const ClassInfo& cls : classes) {
                if (!m_pendingSchedules.contains(cls.classId)) {
                    hasAllData = false;
                    break;
                }
            }
        }

        // 如果已有数据，直接使用
        if (hasAllData) {
            m_pendingGrade = grade;
            m_status->setText(QString::fromUtf8(u8"正在显示课程表：") + grade + QString::fromUtf8(u8"（") + term + QString::fromUtf8(u8"）"));
            buildGradeTable(grade, term);
            return;
        }

        // 否则重新获取该年级的课程表数据
        m_status->setText(QString::fromUtf8(u8"正在获取课程表：") + grade + QString::fromUtf8(u8"（") + term + QString::fromUtf8(u8"）"));

        m_pendingGrade = grade;
        m_pendingTerm = term;
        
        // 只清空该年级相关的数据，保留其他年级的数据
        QMap<QString, ScheduleData> tempSchedules = m_pendingSchedules;
        m_pendingSchedules.clear();
        // 保留非当前年级班级的数据
        for (auto it = tempSchedules.begin(); it != tempSchedules.end(); ++it) {
            bool isCurrentGrade = false;
            for (const ClassInfo& cls : classes) {
                if (cls.classId == it.key()) {
                    isCurrentGrade = true;
                    break;
                }
            }
            if (!isCurrentGrade) {
                m_pendingSchedules[it.key()] = it.value();
            }
        }

        int* pending = new int(classes.size());
        for (const ClassInfo& cls : classes) {
            QUrl url(QStringLiteral("http://47.100.126.194:5000/course-schedule"));
            QUrlQuery q;
            q.addQueryItem(QStringLiteral("class_id"), cls.classId);
            q.addQueryItem(QStringLiteral("term"), term);
            url.setQuery(q);

            QNetworkRequest req(url);
            QNetworkReply* reply = m_manager->get(req);

            connect(reply, &QNetworkReply::finished, this, [this, reply, cls, pending]() {
                const QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> guard(reply);
                if (reply->error() == QNetworkReply::NoError) {
                    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                    if (doc.isObject()) {
                        const QJsonObject root = doc.object();
                        if (root.value("code").toInt(-1) == 200) {
                            const QJsonObject dataObj = root.value("data").toObject();
                            const QJsonObject scheduleObj = dataObj.value(QStringLiteral("schedule")).toObject();
                            ScheduleData sd;
                            sd.days = jsonArrayToStringList(scheduleObj.value(QStringLiteral("days")).toArray());
                            sd.times = jsonArrayToStringList(scheduleObj.value(QStringLiteral("times")).toArray());
                            const QJsonArray cells = dataObj.value(QStringLiteral("cells")).toArray();
                            parseCellsInto(sd, cells);
                            m_pendingSchedules[cls.classId] = sd;
                        }
                    }
                }

                (*pending)--;
                if (*pending <= 0) {
                    delete pending;
                    buildGradeTable(m_pendingGrade, m_pendingTerm);
                }
            });
        }
    }

    static QStringList jsonArrayToStringList(const QJsonArray& arr)
    {
        QStringList list;
        for (const QJsonValue& v : arr) list << v.toString();
        return list;
    }

    void parseCellsInto(ScheduleData& sd, const QJsonArray& cells)
    {
        // 检测是否存在第一列时间文本
        sd.filteredFirstColumn = false;
        for (const QJsonValue& v : cells) {
            if (!v.isObject()) continue;
            const QJsonObject o = v.toObject();
            const int colIndex = o.value(QStringLiteral("col_index")).toInt(-1);
            const QString courseName = o.value(QStringLiteral("course_name")).toString();
            if (colIndex == 0 && looksLikeTimeText(courseName)) {
                sd.filteredFirstColumn = true;
                break;
            }
        }

        for (const QJsonValue& v : cells) {
            if (!v.isObject()) continue;
            const QJsonObject o = v.toObject();
            const int rowIndex = o.value(QStringLiteral("row_index")).toInt(-1);
            const int colIndex = o.value(QStringLiteral("col_index")).toInt(-1);
            const QString courseName = o.value(QStringLiteral("course_name")).toString();
            const bool isHighlight = o.value(QStringLiteral("is_highlight")).toInt(0) != 0;
            if (rowIndex < 0 || colIndex < 0) continue;
            if (courseName.trimmed().isEmpty()) continue;
            if (colIndex == 0 && looksLikeTimeText(courseName)) continue;

            int targetCol = colIndex;
            if (sd.filteredFirstColumn) {
                targetCol = colIndex - 1;
                if (targetCol < 0) continue;
            }
            sd.cellText[rowIndex][targetCol] = courseName;
            if (isHighlight) sd.highlightCells.insert(qMakePair(rowIndex, targetCol));
        }
    }

    void buildGradeTable(const QString& grade, const QString& term)
    {
        const QList<ClassInfo> classes = m_classesByGrade.value(grade);
        if (classes.isEmpty()) {
            m_status->setText(QString::fromUtf8(u8"该年级无班级"));
            return;
        }

        // 以第一个成功返回的 schedule 作为基准 days/times
        QStringList baseDays;
        QStringList baseTimes;
        for (const ClassInfo& cls : classes) {
            if (m_pendingSchedules.contains(cls.classId)) {
                baseDays = m_pendingSchedules[cls.classId].days;
                baseTimes = m_pendingSchedules[cls.classId].times;
                break;
            }
        }
        if (baseDays.isEmpty()) {
            // Qt 5.15 下直接用 initializer_list 赋值可能触发 operator= 歧义（MSVC C2593）
            baseDays = QStringList()
                << QString::fromUtf8(u8"周一")
                << QString::fromUtf8(u8"周二")
                << QString::fromUtf8(u8"周三")
                << QString::fromUtf8(u8"周四")
                << QString::fromUtf8(u8"周五");
        }
        const int dayCount = baseDays.size();
        const int classCount = classes.size();
        const int rows = qMax(baseTimes.size(), 15);
        const int cols = dayCount * classCount;

        m_table->clear();
        m_table->setRowCount(rows + 2); // +2 用于第一行显示周几，第二行显示班级
        m_table->setColumnCount(cols);
        
        // 清除所有合并状态，确保表格是干净的状态（在设置行列数之后）
        m_table->clearSpans();

        // 隐藏水平表头，因为周几和班级都在表格内容区域显示
        m_table->horizontalHeader()->hide();
        
        // 在表格第一行（第0行）合并单元格显示周几（在班级上面）
        m_table->setVerticalHeaderItem(0, new QTableWidgetItem("")); // 第一行行头为空
        
        // 设置第一行（周几）的最小行高，确保文字完整显示
        m_table->setRowHeight(0, 40); // 设置第一行高度为40像素，确保文字完整显示
        
        // 设置列的最小宽度，确保周几文字能完整显示（合并单元格后需要足够的宽度）
        for (int c = 0; c < cols; ++c) {
            m_table->setColumnWidth(c, qMax(100, m_table->columnWidth(c))); // 设置最小列宽为100像素
        }
        
        // 在第一行合并单元格显示周几
        // 先清除第一行的所有项，确保干净的状态
        for (int c = 0; c < cols; ++c) {
            QTableWidgetItem* item = m_table->item(0, c);
            if (item) {
                delete m_table->takeItem(0, c);
            }
        }
        
        // 现在按顺序设置每个周几的单元格
        for (int d = 0; d < dayCount; ++d) {
            int startCol = d * classCount;
            int spanCols = classCount;
            
            // 确保起始列在有效范围内，且不会超出列数
            if (startCol >= 0 && startCol < cols && startCol + spanCols <= cols) {
                // 创建周几的单元格项
                QTableWidgetItem* dayItem = new QTableWidgetItem(baseDays[d]);
                dayItem->setTextAlignment(Qt::AlignCenter);
                dayItem->setFlags(Qt::NoItemFlags); // 不可编辑
                // 使用与行头相同的背景色（从样式表获取，通常是 #3a3a3a 或 #5C5C5C）
                dayItem->setBackground(QBrush(QColor("#5C5C5C"))); // 设置背景色为浅灰色
                dayItem->setForeground(QBrush(QColor("white"))); // 设置文字颜色为白色
                dayItem->setFont(QFont(dayItem->font().family(), dayItem->font().pointSize(), QFont::Bold)); // 粗体
                
                // 设置单元格项（必须在合并之前设置）
                m_table->setItem(0, startCol, dayItem);
                
                // 如果需要合并（多个班级），则合并单元格
                // 注意：合并必须在设置item之后进行
                if (spanCols > 1) {
                    m_table->setSpan(0, startCol, 1, spanCols);
                }
            }
        }
        
        // 合并单元格后，重新调整列宽，确保周几文字能完整显示
        // 使用QFontMetrics计算"周几"文字的实际宽度，确保有足够空间
        QFont dayFont = m_table->font();
        dayFont.setBold(true); // 周几使用粗体
        QFontMetrics fm(dayFont);
        int maxDayTextWidth = 0;
        for (const QString& day : baseDays) {
            int textWidth = fm.width(day);
            if (textWidth > maxDayTextWidth) {
                maxDayTextWidth = textWidth;
            }
        }
        // 最小宽度 = 文字宽度 + 左右padding（各15像素）+ 安全边距（30像素）
        int minDayWidth = maxDayTextWidth + 60;
        
        // 计算每个合并区域的最小宽度
        for (int d = 0; d < dayCount; ++d) {
            int startCol = d * classCount;
            int spanCols = classCount;
            int totalWidth = 0;
            for (int c = startCol; c < startCol + spanCols; ++c) {
                totalWidth += m_table->columnWidth(c);
            }
            // 如果总宽度小于所需最小宽度，按比例增加各列宽度
            if (totalWidth < minDayWidth && spanCols > 0) {
                int extraWidth = (minDayWidth - totalWidth) / spanCols;
                int remainder = (minDayWidth - totalWidth) % spanCols;
                for (int c = startCol; c < startCol + spanCols; ++c) {
                    int addWidth = extraWidth;
                    // 将余数分配给前面的列
                    if (c - startCol < remainder) {
                        addWidth++;
                    }
                    m_table->setColumnWidth(c, m_table->columnWidth(c) + addWidth);
                }
            }
        }
        
        // 在表格第二行（第1行）显示班级名称（在周几下面）
        for (int d = 0; d < dayCount; ++d) {
            for (int ci = 0; ci < classCount; ++ci) {
                int col = d * classCount + ci;
                QTableWidgetItem* classItem = new QTableWidgetItem(classes[ci].displayName);
                classItem->setTextAlignment(Qt::AlignCenter);
                classItem->setFlags(Qt::NoItemFlags); // 不可编辑
                classItem->setBackground(QBrush(QColor("#5C5C5C"))); // 设置背景色为浅灰色
                classItem->setForeground(QBrush(QColor("white"))); // 设置文字颜色为白色
                m_table->setItem(1, col, classItem);
            }
        }

        // 行头：times（不足填空，注意前两行已被占用）
        QStringList vheaders;
        vheaders.append(""); // 第0行用于显示周几，行头为空
        vheaders.append(""); // 第1行用于显示班级，行头为空
        vheaders.append(baseTimes);
        while (vheaders.size() < rows + 2) vheaders.append(""); // +2 因为前两行被占用
        m_table->setVerticalHeaderLabels(vheaders);

        // 清空单元格（从第2行开始，第0行显示周几，第1行显示班级）
        {
            const QSignalBlocker blocker(m_table);
            for (int r = 2; r <= rows + 1; ++r) { // 从第2行开始
                for (int c = 0; c < cols; ++c) {
                    QTableWidgetItem* item = new QTableWidgetItem("");
                    item->setTextAlignment(Qt::AlignCenter);
                    item->setBackground(QBrush(QColor("#5C5C5C"))); // 设置背景色为浅灰色
                    item->setForeground(QBrush(QColor("white"))); // 设置文字颜色为白色
                    m_table->setItem(r, c, item);
                }
            }
        }

        // 填充：按 class、day、row
        const QSignalBlocker blocker(m_table);
        for (int ci = 0; ci < classCount; ++ci) {
            const QString classId = classes[ci].classId;
            if (!m_pendingSchedules.contains(classId)) continue;
            const ScheduleData& sd = m_pendingSchedules[classId];

            for (auto rowIt = sd.cellText.begin(); rowIt != sd.cellText.end(); ++rowIt) {
                const int r = rowIt.key();
                if (r < 0 || r >= rows) continue;
                const int tableRow = r + 2; // +2 因为第0行显示周几，第1行显示班级
                for (auto colIt = rowIt.value().begin(); colIt != rowIt.value().end(); ++colIt) {
                    const int dayIdx = colIt.key(); // 0..dayCount-1
                    if (dayIdx < 0 || dayIdx >= dayCount) continue;
                    const int tableCol = dayIdx * classCount + ci;
                    QTableWidgetItem* item = m_table->item(tableRow, tableCol);
                    if (!item) {
                        item = new QTableWidgetItem("");
                        item->setTextAlignment(Qt::AlignCenter);
                        item->setBackground(QBrush(QColor("#5C5C5C"))); // 设置背景色为浅灰色
                        item->setForeground(QBrush(QColor("white"))); // 设置文字颜色为白色
                        m_table->setItem(tableRow, tableCol, item);
                    }
                    item->setText(colIt.value());
                    // 确保背景色为浅灰色（除非是高亮单元格）
                    if (sd.highlightCells.contains(qMakePair(r, dayIdx))) {
                        item->setBackground(QBrush(QColor("#3d4f2b")));
                        item->setForeground(QBrush(QColor("white"))); // 高亮单元格文字也为白色
                    } else {
                        item->setBackground(QBrush(QColor("#5C5C5C"))); // 浅灰色
                        item->setForeground(QBrush(QColor("white"))); // 设置文字颜色为白色
                    }
                }
            }
        }

        m_status->setText(QString::fromUtf8(u8"已加载：") + grade + QString::fromUtf8(u8"（") + term + QString::fromUtf8(u8"）")
                          + QString::fromUtf8(u8" 班级数=") + QString::number(classCount));
        
        // 更新表头角落按钮样式
        if (m_table) {
            // 直接调用，因为 m_table 就是 CustomCourseScheduleTable* 类型
            CustomCourseScheduleTable* customTable = static_cast<CustomCourseScheduleTable*>(m_table);
            if (customTable) {
                QTimer::singleShot(100, customTable, [customTable]() {
                    customTable->updateCornerButtonStyle();
                });
            }
        }
    }

private:
    QNetworkAccessManager* m_manager = nullptr;
    QComboBox* m_gradeCombo = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_closeBtn = nullptr; // 关闭按钮
    CustomCourseScheduleTable* m_table = nullptr; // 自定义表格
    QLabel* m_status = nullptr;

    // 窗口拖拽相关
    bool m_dragging = false;
    QPoint m_dragStartPosition;

    QMap<QString, QList<ClassInfo>> m_classesByGrade; // grade -> classes

    // 当前请求中的临时结果
    QString m_pendingGrade;
    QString m_pendingTerm;
    QMap<QString, ScheduleData> m_pendingSchedules; // classId -> schedule
};


