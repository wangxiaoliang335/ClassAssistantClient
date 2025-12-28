#pragma execution_character_set("utf-8")
#include "TACClassWeekCourseScheduleDialog.h"
#include "TACSubjectSelectDialog.h"
#include "common.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QDate>
#include <QPointer>

namespace {
struct SlotRow {
    QString name;   // 行名（节次/活动）
    QString time;   // 时间段
    bool isSpanRow; // 是否跨列显示（如：大课间/午休/眼保健操）
};

static QVector<SlotRow> buildDefaultRows()
{
    return {
        { QString::fromUtf8(u8"早读"),     QString::fromUtf8(u8"7:00-7:40"),  false },
        { QString::fromUtf8(u8"第一节"),   QString::fromUtf8(u8"7:50-8:30"),  false },
        { QString::fromUtf8(u8"第二节"),   QString::fromUtf8(u8"8:40-9:20"),  false },
        { QString::fromUtf8(u8"大课间"),   QString::fromUtf8(u8"9:20-9:40"),  true  },
        { QString::fromUtf8(u8"第三节"),   QString::fromUtf8(u8"9:40-10:20"), false },
        { QString::fromUtf8(u8"第四节"),   QString::fromUtf8(u8"10:30-11:10"),false },
        { QString::fromUtf8(u8"午休"),     QString::fromUtf8(u8"11:10-13:30"),true  },
        { QString::fromUtf8(u8"第五节"),   QString::fromUtf8(u8"13:30-14:10"),false },
        { QString::fromUtf8(u8"第六节"),   QString::fromUtf8(u8"14:20-15:00"),false },
        { QString::fromUtf8(u8"眼保健操"), QString::fromUtf8(u8"15:00-15:20"),true  },
        { QString::fromUtf8(u8"第七节"),   QString::fromUtf8(u8"15:20-16:00"),false },
        { QString::fromUtf8(u8"课服1"),    QString::fromUtf8(u8"16:10-16:50"),false },
        { QString::fromUtf8(u8"课服2"),    QString::fromUtf8(u8"17:00-17:40"),false },
        { QString::fromUtf8(u8"晚自习1"),  QString::fromUtf8(u8"19:00-19:40"),false },
        { QString::fromUtf8(u8"晚自习2"),  QString::fromUtf8(u8"19:50-20:30"),false },
    };
}

static QLabel* makeRowHeaderLabel(const QString& name, const QString& time, QWidget* parent)
{
    QLabel* lbl = new QLabel(parent);
    // 行头文字整体往上靠，避免行高变大后“时间”跑到很下面
    lbl->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    lbl->setText(QStringLiteral(
        "<div style='line-height:1.1; padding-top:6px;'>"
        "%1<br><span style='font-size:11px;'>%2</span>"
        "</div>"
    ).arg(name, time));
    lbl->setTextFormat(Qt::RichText);
    lbl->setMinimumWidth(96);
    lbl->setMinimumHeight(96);
    lbl->setStyleSheet("border: 1px solid rgba(255,255,255,0.18); padding-bottom: 6px;");
    return lbl;
}
} // namespace
TACClassWeekCourseScheduleDialog::TACClassWeekCourseScheduleDialog(QWidget *parent)
	: TABaseDialog(parent), currentClassButton(nullptr), m_httpHandler(nullptr)
{
    this->setObjectName("TACClassWeekCourseScheduleDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(720, 980));
    this->setTitle(QString::fromUtf8(u8"教室周课表"));
    
    // 重新排列标题栏布局，将关闭按钮移到右边
    if (titleLayout && closeButton && titleLabel) {
        // 移除所有组件
        titleLayout->removeWidget(closeButton);
        titleLayout->removeWidget(titleLabel);
        
        // 重新添加：标题在左，关闭按钮在右
        titleLayout->addWidget(titleLabel);
        titleLayout->addStretch(); // 添加弹性空间
        titleLayout->addWidget(closeButton);
    }
    
    m_httpHandler = new TAHttpHandler(this);


	TACToolWidget* toolWidget = new TACToolWidget(this);
    toolWidget->setObjectName("toolWidget");
	this->contentLayout->addWidget(toolWidget);

	QWidget* container = new QWidget(this);
    container->setObjectName("container");
	QVBoxLayout* containerLayout = new QVBoxLayout();
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    container->setLayout(containerLayout);

    // 网格：左列为“节次+时间”，上行为“周一~周五”
    QWidget* gridWidget = new QWidget(this);
    gridWidget->setObjectName("gridWidget");
    gridLayout = new QGridLayout(gridWidget);
    gridLayout->setSpacing(0);
    gridLayout->setContentsMargins(0, 0, 0, 0);

    // 顶部表头
    QLabel* topLeft = new QLabel(QString::fromUtf8(u8"节次"), gridWidget);
    topLeft->setAlignment(Qt::AlignCenter);
    topLeft->setMinimumWidth(96);
    topLeft->setStyleSheet("border: 1px solid rgba(255,255,255,0.18); font-weight: bold;");
    gridLayout->addWidget(topLeft, 0, 0);

    QStringList days = {
        QString::fromUtf8(u8"周一"),
        QString::fromUtf8(u8"周二"),
        QString::fromUtf8(u8"周三"),
        QString::fromUtf8(u8"周四"),
        QString::fromUtf8(u8"周五")
    };
    for (int col = 0; col < 5; ++col) {
        QLabel* label = new QLabel(days[col], gridWidget);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("border: 1px solid rgba(255,255,255,0.18); font-weight: bold;");
        gridLayout->addWidget(label, 0, col + 1);
    }

    containerLayout->addWidget(gridWidget);
	this->contentLayout->addWidget(container);
    
    updateClassList();
}

TACClassWeekCourseScheduleDialog::~TACClassWeekCourseScheduleDialog()
{}

void TACClassWeekCourseScheduleDialog::updateClassList()
{
    if (!gridLayout) return;

    // 清空旧单元格（保留第0行表头）
    while (gridLayout->count() > 0) {
        QLayoutItem* item = gridLayout->takeAt(0);
        if (!item) break;
        QWidget* w = item->widget();
        if (w) w->deleteLater();
        delete item;
    }

    // 重建表头
    QLabel* topLeft = new QLabel(QString::fromUtf8(u8"节次"), this);
    topLeft->setAlignment(Qt::AlignCenter);
    topLeft->setMinimumWidth(96);
    topLeft->setStyleSheet("border: 1px solid rgba(255,255,255,0.18); font-weight: bold;");
    gridLayout->addWidget(topLeft, 0, 0);

    QStringList days = {
        QString::fromUtf8(u8"周一"),
        QString::fromUtf8(u8"周二"),
        QString::fromUtf8(u8"周三"),
        QString::fromUtf8(u8"周四"),
        QString::fromUtf8(u8"周五")
    };
    for (int col = 0; col < 5; ++col) {
        QLabel* label = new QLabel(days[col], this);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("border: 1px solid rgba(255,255,255,0.18); font-weight: bold;");
        gridLayout->addWidget(label, 0, col + 1);
    }

    const QVector<SlotRow> rows = buildDefaultRows();
    classVecotr.clear();
    classVecotr.resize(rows.size());

    for (int r = 0; r < rows.size(); ++r) {
        const SlotRow& sr = rows[r];
        const int gridRow = r + 1;

        QLabel* rowHead = makeRowHeaderLabel(sr.name, sr.time, this);
        gridLayout->addWidget(rowHead, gridRow, 0);

        classVecotr[r].resize(5);
        if (sr.isSpanRow) {
            QLabel* span = new QLabel(sr.name, this);
            span->setAlignment(Qt::AlignCenter);
            span->setStyleSheet("border: 1px solid rgba(255,255,255,0.18); color: rgba(255,255,255,0.9);");
            gridLayout->addWidget(span, gridRow, 1, 1, 5);
            for (int c = 0; c < 5; ++c) classVecotr[r][c] = nullptr;
        } else {
            for (int c = 0; c < 5; ++c) {
                QPushButton* btn = new QPushButton(QString(), this);
                btn->setProperty("row", r);
                btn->setProperty("col", c);
                btn->setCheckable(true);
                btn->setStyleSheet("border: 1px solid rgba(255,255,255,0.18);");
                connect(btn, &QPushButton::clicked, this, &TACClassWeekCourseScheduleDialog::classClick);
                gridLayout->addWidget(btn, gridRow, c + 1);
                classVecotr[r][c] = btn;
            }
        }

        // 行高接近截图
        if (sr.isSpanRow) {
            gridLayout->setRowMinimumHeight(gridRow, 72);
        } else {
            gridLayout->setRowMinimumHeight(gridRow, 110);
        }
    }

}
void TACClassWeekCourseScheduleDialog::init()
{
    

}
void TACClassWeekCourseScheduleDialog::classClick()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    // 取消之前选中的按钮
    if (currentClassButton && currentClassButton != btn) {
        currentClassButton->setChecked(false);
    }
    
    // 设置当前选中的按钮
    currentClassButton = btn;
    btn->setChecked(true);
    
    // 显示科目选择对话框
    if (!m_subjectSelectDialog) {
        m_subjectSelectDialog = new TACSubjectSelectDialog(this);
        // 捕获当前按钮的指针，确保在 lambda 中能正确更新
        connect(m_subjectSelectDialog, &TACSubjectSelectDialog::subjectSelected, this, [this](const QString& subject) {
            // 更新当前选中的按钮
            if (currentClassButton) {
                currentClassButton->setText(subject);
                currentClassButton->setStyleSheet(
                    "QPushButton {"
                    "border: 1px solid rgba(255,255,255,0.18);"
                    "color: white;"
                    "background-color: #4CAF50;"
                    "font-size: 14px;"
                    "}"
                    "QPushButton:hover {"
                    "background-color: rgba(255,255,255,0.1);"
                    "}"
                    "QPushButton:checked {"
                    "background-color: rgba(0,120,212,255);"
                    "}"
                );
                // 保存到服务器
                saveCourseScheduleToServer();
            }
        });
        // 监听科目编辑信号
        connect(m_subjectSelectDialog, &TACSubjectSelectDialog::subjectEdited, this, [this](const QString& oldSubject, const QString& newSubject) {
            // 科目被编辑后，可以更新列表中的科目名称
            // 这里可以根据需要更新科目列表
            qDebug() << "Subject edited:" << oldSubject << "->" << newSubject;
        });
    }
    
    // 定位对话框到按钮右侧或下方（避免超出屏幕）
    QPoint globalPos = btn->mapToGlobal(QPoint(btn->width() + 5, 0));
    QRect screenGeometry = QApplication::desktop()->availableGeometry();
    
    // 如果右侧空间不足，则显示在按钮下方
    if (globalPos.x() + m_subjectSelectDialog->width() > screenGeometry.right()) {
        globalPos = btn->mapToGlobal(QPoint(0, btn->height() + 5));
    }
    
    // 确保对话框不会超出屏幕
    if (globalPos.y() + m_subjectSelectDialog->height() > screenGeometry.bottom()) {
        globalPos.setY(screenGeometry.bottom() - m_subjectSelectDialog->height() - 10);
    }
    if (globalPos.x() + m_subjectSelectDialog->width() > screenGeometry.right()) {
        globalPos.setX(screenGeometry.right() - m_subjectSelectDialog->width() - 10);
    }
    
    m_subjectSelectDialog->move(globalPos);
    m_subjectSelectDialog->show();
    m_subjectSelectDialog->raise();
    m_subjectSelectDialog->activateWindow();
}

QString TACClassWeekCourseScheduleDialog::currentTermString() const
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

QStringList TACClassWeekCourseScheduleDialog::jsonArrayToStringList(const QJsonArray& arr) const
{
    QStringList list;
    for (const QJsonValue& value : arr) {
        list << value.toString();
    }
    return list;
}

void TACClassWeekCourseScheduleDialog::fetchCourseScheduleFromServer(const QString& classId)
{
    if (classId.isEmpty()) {
        qWarning() << "班级ID为空，无法获取课程表";
        return;
    }

    m_currentClassId = classId; // 保存当前班级ID

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
        qWarning() << "课程表接口请求失败:" << error;
    });

    m_httpHandler->get(url.toString());
    qDebug() << "正在请求课程表:" << url.toString();
}

void TACClassWeekCourseScheduleDialog::handleScheduleResponse(const QString& resp)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        qWarning() << "课程表数据解析失败:" << parseError.errorString();
        return;
    }

    QJsonObject obj = doc.object();
    const int code = obj.value(QStringLiteral("code")).toInt(-1);
    const QString message = obj.value(QStringLiteral("message")).toString(QStringLiteral("查询失败"));
    if (code != 200)
    {
        qWarning() << "课程表接口返回错误:" << code << message;
        return;
    }

    QJsonObject dataObj = obj.value(QStringLiteral("data")).toObject();
    QJsonObject scheduleObj = dataObj.value(QStringLiteral("schedule")).toObject();
    QJsonArray daysArray = scheduleObj.value(QStringLiteral("days")).toArray();
    QJsonArray timesArray = scheduleObj.value(QStringLiteral("times")).toArray();
    QJsonArray cellsArray = dataObj.value(QStringLiteral("cells")).toArray();

    QStringList days = jsonArrayToStringList(daysArray);
    QStringList times = jsonArrayToStringList(timesArray);
    applySchedule(days, times, cellsArray);
}

void TACClassWeekCourseScheduleDialog::applySchedule(const QStringList& days, const QStringList& times, const QJsonArray& cells)
{
    if (!gridLayout || classVecotr.isEmpty()) {
        qWarning() << "网格布局或课程按钮数组未初始化";
        return;
    }

    // 先清空所有按钮文本
    const QVector<SlotRow> rows = buildDefaultRows();
    for (int r = 0; r < rows.size() && r < classVecotr.size(); ++r) {
        if (rows[r].isSpanRow) continue; // 跳过跨行项
        for (int c = 0; c < 5 && c < classVecotr[r].size(); ++c) {
            QPushButton* btn = classVecotr[r][c];
            if (btn) {
                btn->setText("");
                btn->setChecked(false);
            }
        }
    }

    // 检查是否需要过滤第一列（第一列可能是时间）
    bool filteredFirstColumn = false;
    for (const QJsonValue& value : cells) {
        if (!value.isObject())
            continue;

        QJsonObject cellObj = value.toObject();
        int colIndex = cellObj.value(QStringLiteral("col_index")).toInt(-1);
        QString courseName = cellObj.value(QStringLiteral("course_name")).toString();
        if (colIndex == 0 && looksLikeTimeText(courseName)) {
            filteredFirstColumn = true;
            break;
        }
    }

    // 填充课程表数据
    for (const QJsonValue& value : cells) {
        if (!value.isObject())
            continue;

        QJsonObject cellObj = value.toObject();
        int rowIndex = cellObj.value(QStringLiteral("row_index")).toInt(-1);
        int colIndex = cellObj.value(QStringLiteral("col_index")).toInt(-1);
        QString courseName = cellObj.value(QStringLiteral("course_name")).toString();
        bool isHighlight = cellObj.value(QStringLiteral("is_highlight")).toInt(0) != 0;

        if (rowIndex < 0 || colIndex < 0)
            continue;
        if (courseName.isEmpty())
            continue;
        if (colIndex == 0 && looksLikeTimeText(courseName))
            continue; // 跳过第一列的时间文本

        // 计算目标列索引（如果需要过滤第一列，则减1）
        int targetCol = colIndex;
        if (filteredFirstColumn) {
            targetCol = colIndex - 1;
            if (targetCol < 0)
                continue;
        }

        // 只处理周一到周五（列索引0-4）
        if (targetCol >= 5)
            continue;

        // 行索引应该对应 classVecotr 的索引
        // 需要跳过跨行项（大课间、午休等）
        int actualRowIndex = -1;
        int validRowCount = 0;
        for (int r = 0; r < rows.size() && r < classVecotr.size(); ++r) {
            if (!rows[r].isSpanRow) {
                if (validRowCount == rowIndex) {
                    actualRowIndex = r;
                    break;
                }
                validRowCount++;
            }
        }

        if (actualRowIndex < 0 || actualRowIndex >= classVecotr.size())
            continue;
        if (targetCol >= classVecotr[actualRowIndex].size())
            continue;

        QPushButton* btn = classVecotr[actualRowIndex][targetCol];
        if (!btn)
            continue;

        btn->setText(courseName);
        QString styleSheet = QStringLiteral(
            "QPushButton {"
            "border: 1px solid rgba(255,255,255,0.18);"
            "color: white;"
            "background-color: transparent;"
            "font-size: 14px;"
            "}"
            "QPushButton:hover {"
            "background-color: rgba(255,255,255,0.1);"
            "}"
            "QPushButton:checked {"
            "background-color: rgba(0,120,212,255);"
            "}"
        );
        btn->setStyleSheet(styleSheet);
    }

    qDebug() << "课程表数据已应用，共" << cells.size() << "个单元格";
}

bool TACClassWeekCourseScheduleDialog::looksLikeTimeText(const QString& text) const
{
    // 支持格式：HH:MM、HH:MM:SS、HH:MM:SS.mmm
    static QRegExp preciseTimePattern(QStringLiteral("^\\d{1,2}:\\d{2}(?::\\d{2}(?:\\.\\d{1,3})?)?$"));
    if (preciseTimePattern.exactMatch(text))
        return true;

    // 兼容形如 "08:00-08:45" 的区间
    QRegExp rangePattern(QStringLiteral("^(\\d{1,2}:\\d{2})\\s*-\\s*(\\d{1,2}:\\d{2})$"));
    if (rangePattern.exactMatch(text))
        return true;

    return false;
}

void TACClassWeekCourseScheduleDialog::saveCourseScheduleToServer()
{
    if (m_currentClassId.isEmpty()) {
        qWarning() << "班级ID为空，无法保存课程表";
        return;
    }

    if (!m_httpHandler) {
        m_httpHandler = new TAHttpHandler(this);
    }

    if (!gridLayout || classVecotr.isEmpty()) {
        qWarning() << "网格布局或课程按钮数组未初始化";
        return;
    }

    // 1) 收集表头 days 与 times
    QStringList days = {
        QString::fromUtf8(u8"周一"),
        QString::fromUtf8(u8"周二"),
        QString::fromUtf8(u8"周三"),
        QString::fromUtf8(u8"周四"),
        QString::fromUtf8(u8"周五")
    };

    QStringList times;
    const QVector<SlotRow> rows = buildDefaultRows();
    for (const SlotRow& sr : rows) {
        if (!sr.isSpanRow) {
            times.append(sr.time);
        }
    }

    // 2) 收集单元格（包括空单元格，以便服务器清空旧数据）
    QJsonArray cells;
    int validRowIndex = 0; // 有效的行索引（跳过跨行项）
    
    for (int r = 0; r < rows.size() && r < classVecotr.size(); ++r) {
        const SlotRow& sr = rows[r];
        if (sr.isSpanRow) {
            continue; // 跳过跨行项
        }
        
        for (int c = 0; c < 5 && c < classVecotr[r].size(); ++c) {
            QPushButton* btn = classVecotr[r][c];
            if (!btn) continue;
            
            QString courseName = btn->text().trimmed();
            
            QJsonObject cell;
            cell[QStringLiteral("row_index")] = validRowIndex;
            cell[QStringLiteral("col_index")] = c;
            cell[QStringLiteral("course_name")] = courseName; // 空单元格发送空字符串
            cell[QStringLiteral("is_highlight")] = 0; // 默认不高亮
            cells.append(cell);
        }
        
        validRowIndex++;
    }

    // 3) 组装 JSON（契合后端 /course-schedule/save 接口）
    QJsonObject payload;
    payload[QStringLiteral("class_id")] = m_currentClassId;
    payload[QStringLiteral("term")] = currentTermString();
    payload[QStringLiteral("days")] = QJsonArray::fromStringList(days);
    payload[QStringLiteral("times")] = QJsonArray::fromStringList(times);
    payload[QStringLiteral("cells")] = cells;
    payload[QStringLiteral("remark")] = QString(); // 备注可按需填写

    QJsonDocument doc(payload);
    const QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // 4) 发送到服务器（使用提供的保存接口）
    const QString url = QStringLiteral("http://47.100.126.194:5000/course-schedule/save");
    
    connect(m_httpHandler, &TAHttpHandler::success, this, [this](const QString& responseString) {
        m_httpHandler->disconnect(this);
        
        QJsonParseError parseError;
        QJsonDocument respDoc = QJsonDocument::fromJson(responseString.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !respDoc.isObject()) {
            qWarning() << "课程表保存响应解析失败:" << parseError.errorString();
            return;
        }
        
        QJsonObject obj = respDoc.object();
        int code = obj.value(QStringLiteral("code")).toInt(-1);
        QString message = obj.value(QStringLiteral("message")).toString();
        
        if (code == 200 || code == 0) {
            qDebug() << "课程表已成功保存到服务器";
        } else {
            qWarning() << "课程表保存失败:" << code << message;
        }
    });
    
    connect(m_httpHandler, &TAHttpHandler::failed, this, [](const QString& error) {
        qWarning() << "课程表保存接口请求失败:" << error;
    });
    
    m_httpHandler->post(url, jsonData);
    qDebug() << "正在保存课程表到服务器:" << url;
}
