#pragma execution_character_set("utf-8")
#include "TACClassWeekCourseScheduleDialog.h"
#include "common.h"

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
	: TABaseDialog(parent)
{
    this->setObjectName("TACClassWeekCourseScheduleDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(720, 980));
    this->setTitle(QString::fromUtf8(u8"我的课表"));


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

}
