#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QFrame>
#include <QBrush>
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QCoreApplication>
#include <QMap>
#include <QDebug>
#include <QShowEvent>
#include <QFile>
#include <QTextStream>
#include <QCursor>
#include <QEvent>
#include <QPoint>
#include <QRect>
#include <QMouseEvent>
#include "QXlsx/header/xlsxdocument.h"
#include "CommonInfo.h"

class ArrangeSeatDialog : public QDialog
{
    Q_OBJECT

public:
    ArrangeSeatDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        setWindowTitle("排座");
        resize(400, 350);
        setStyleSheet(
            "QDialog { background-color: #2d2f32; font-size:14px; color: white; }"
        );

        // 关闭按钮
        m_btnClose = new QPushButton("X", this);
        m_btnClose->setFixedSize(30, 30);
        m_btnClose->setStyleSheet(
            "QPushButton { background-color: #666666; color: white; font-weight: bold; font-size: 14px; border: none; border-radius: 4px; }"
            "QPushButton:hover { background-color: #777777; }"
        );
        m_btnClose->hide();
        m_btnClose->installEventFilter(this);
        connect(m_btnClose, &QPushButton::clicked, this, &QDialog::close);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(15);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        // 顶部：两个列表框并排
        QHBoxLayout* listLayout = new QHBoxLayout;
        listLayout->setSpacing(20);

        // 左侧下拉框：小组/不分组
        QFrame* leftFrame = new QFrame;
        leftFrame->setFrameShape(QFrame::StyledPanel);
        leftFrame->setStyleSheet("background-color: #1f1f1f; border: 1px solid #3a3a3a;");
        QVBoxLayout* leftLayout = new QVBoxLayout(leftFrame);
        leftLayout->setContentsMargins(5, 5, 5, 5);

        leftComboBox = new QComboBox;
        leftComboBox->setEditable(false); // 设置为不可编辑，确保显示选中的项
        leftComboBox->setStyleSheet(
            "QComboBox { background-color: #1f1f1f; border: 1px solid #555; padding: 8px; min-width: 120px; color: white; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView { background-color: #1f1f1f; selection-background-color: #ff8c00; color: white; }"
            "QComboBox QAbstractItemView::item { padding: 8px; color: white; }"
            "QComboBox QAbstractItemView::item:selected { background-color: #ff8c00; color: white; }"
        );

        // 添加左侧选项
        leftComboBox->addItem("小组");
        leftComboBox->addItem("不分组");

        // 默认选择"不分组"（索引1）
        leftComboBox->setCurrentIndex(1);
        leftComboBox->update(); // 强制更新显示

        leftLayout->addWidget(leftComboBox);
        listLayout->addWidget(leftFrame);

        // 右侧下拉框：根据左侧选择动态变化
        QFrame* rightFrame = new QFrame;
        rightFrame->setFrameShape(QFrame::StyledPanel);
        rightFrame->setStyleSheet("background-color: #1f1f1f; border: 1px solid #3a3a3a;");
        QVBoxLayout* rightLayout = new QVBoxLayout(rightFrame);
        rightLayout->setContentsMargins(5, 5, 5, 5);

        rightComboBox = new QComboBox;
        rightComboBox->setEditable(false); // 设置为不可编辑，确保显示选中的项
        rightComboBox->setStyleSheet(
            "QComboBox { background-color: #1f1f1f; border: 1px solid #555; padding: 8px; min-width: 120px; color: white; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView { background-color: #1f1f1f; selection-background-color: #ff8c00; color: white; }"
            "QComboBox QAbstractItemView::item { padding: 8px; color: white; }"
            "QComboBox QAbstractItemView::item:selected { background-color: #ff8c00; color: white; }"
        );

        // 初始化右侧下拉框（默认显示"不分组"的选项）
        updateRightList(false);
        rightComboBox->update(); // 强制更新显示

        rightLayout->addWidget(rightComboBox);
        listLayout->addWidget(rightFrame);

        mainLayout->addLayout(listLayout);

        // 连接左侧下拉框选择变化事件
        connect(leftComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
            QString text = leftComboBox->itemText(index);
            bool isGroup = (text == "小组");

            // 更新右侧下拉框
            updateRightList(isGroup);
        });

        // 底部按钮
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->setSpacing(10);

        QString grayStyle = "background-color: #1f1f1f; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px; border: 1px solid #555;";
        QString buttonStyle = "background-color: #666666; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px; border: 1px solid #555555;";

        midtermComboBox = new QComboBox(this);
        midtermComboBox->setStyleSheet(
            "QComboBox { color: white; background-color: #1f1f1f; padding: 6px 12px; border-radius: 4px; font-size: 14px; border: 1px solid #555; }"
            "QComboBox QAbstractItemView { color: white; background-color: #1f1f1f; selection-background-color: #ff8c00; }"
        );
        subjectComboBox = new QComboBox(this);
        subjectComboBox->setStyleSheet(
            "QComboBox { color: white; background-color: #1f1f1f; padding: 6px 12px; border-radius: 4px; font-size: 14px; border: 1px solid #555; }"
            "QComboBox QAbstractItemView { color: white; background-color: #1f1f1f; selection-background-color: #ff8c00; }"
        );
        btnConfirm = new QPushButton("确定");

        btnConfirm->setStyleSheet(buttonStyle);

        buttonLayout->addWidget(midtermComboBox);
        buttonLayout->addWidget(subjectComboBox);
        buttonLayout->addStretch();
        buttonLayout->addWidget(btnConfirm);

        mainLayout->addLayout(buttonLayout);

        // 连接确定按钮
        connect(btnConfirm, &QPushButton::clicked, this, [this]() {
            emit arrangeRequested(midtermComboBox->currentText(),
                                  subjectComboBox->currentText(),
                                  rightComboBox->currentText(),
                                  isGroupMode());
            accept();
        });

        // 下拉联动：选择文件后更新属性字段
        connect(midtermComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            updateAttributesFromSelectedFile();
        });
    }

    // 加载已下载的Excel文件并更新科目下拉框
    void loadExcelFiles(const QString& classId)
    {
        m_classId = classId;

        // 获取学校ID和班级ID
        UserInfo userInfo = CommonInfo::GetData();
        QString schoolId = userInfo.schoolId;

        if (schoolId.isEmpty() || classId.isEmpty()) {
            qDebug() << "学校ID或班级ID为空，无法加载Excel文件";
            return;
        }

        // 构建Excel文件目录路径
        QString baseDir = QCoreApplication::applicationDirPath() + "/excel_files";
        QString schoolDir = baseDir + "/" + schoolId;
        QString classDir = schoolDir + "/" + classId;

        QStringList filters;
        filters << "*.xlsx" << "*.xls" << "*.csv";
        QFileInfoList fileList;
        
        // 扫描主目录（向后兼容）
        QDir dir(classDir);
        if (dir.exists()) {
            QFileInfoList mainFiles = dir.entryInfoList(filters, QDir::Files);
            fileList.append(mainFiles);
        }
        
        // 扫描 group/ 子目录
        QDir groupDir(classDir + "/group");
        if (groupDir.exists()) {
            QFileInfoList groupFiles = groupDir.entryInfoList(filters, QDir::Files);
            fileList.append(groupFiles);
        }
        
        // 扫描 student/ 子目录
        QDir studentDir(classDir + "/student");
        if (studentDir.exists()) {
            QFileInfoList studentFiles = studentDir.entryInfoList(filters, QDir::Files);
            fileList.append(studentFiles);
        }
        
        if (fileList.isEmpty()) {
            qDebug() << "Excel文件目录不存在或没有文件:" << classDir;
            return;
        }

        qDebug() << "找到" << fileList.size() << "个Excel文件";

        // 收集所有Excel文件
        midtermComboBox->clear();
        subjectComboBox->clear();
        m_excelFileMap.clear();

        for (const QFileInfo& fileInfo : fileList) {
            QString filePath = fileInfo.absoluteFilePath();
            QString fileName = fileInfo.baseName(); // 去掉扩展名的文件名

            midtermComboBox->addItem(fileName);
            subjectComboBox->addItem(fileName);
            m_excelFileMap[fileName] = filePath;
        }

        // 如果有Excel文件，默认选中第一个
        if (midtermComboBox->count() > 0) {
            midtermComboBox->setCurrentIndex(0);
        }
        if (subjectComboBox->count() > 0) {
            subjectComboBox->setCurrentIndex(0);
        }

        // 加载首个文件的字段
        updateAttributesFromSelectedFile();
    }

    void setClassId(const QString& classId) { m_classId = classId; }

    // 获取选中的左侧选项（true=小组, false=不分组）
    bool isGroupMode() const
    {
        return leftComboBox->currentText() == "小组";
    }

    // 获取选中的右侧选项文本
    QString getSelectedMethod() const
    {
        return rightComboBox->currentText();
    }

protected:
    void showEvent(QShowEvent* event) override
    {
        QDialog::showEvent(event);
        if (!m_classId.isEmpty()) {
            loadExcelFiles(m_classId);
        }
        // 调整关闭按钮位置并显示
        if (m_btnClose) {
            m_btnClose->move(width() - 35, 5);
            m_btnClose->show();
        }
    }
    void enterEvent(QEvent* event) override
    {
        if (m_btnClose) m_btnClose->show();
        QDialog::enterEvent(event);
    }
    void leaveEvent(QEvent* event) override
    {
        QPoint globalPos = QCursor::pos();
        QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
        if (!widgetRect.contains(globalPos) && m_btnClose) {
            QRect btnRect = QRect(m_btnClose->mapToGlobal(QPoint(0, 0)), m_btnClose->size());
            if (!btnRect.contains(globalPos)) {
                m_btnClose->hide();
            }
        }
        QDialog::leaveEvent(event);
    }
    void resizeEvent(QResizeEvent* event) override
    {
        if (m_btnClose) {
            m_btnClose->move(width() - 35, 5);
        }
        QDialog::resizeEvent(event);
    }
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (obj == m_btnClose) {
            if (event->type() == QEvent::Enter) {
                m_btnClose->show();
            } else if (event->type() == QEvent::Leave) {
                QPoint globalPos = QCursor::pos();
                QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
                if (!widgetRect.contains(globalPos)) {
                    m_btnClose->hide();
                }
            }
        }
        return QDialog::eventFilter(obj, event);
    }
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragPos = event->globalPos() - frameGeometry().topLeft();
        }
        QDialog::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragPos);
        }
        QDialog::mouseMoveEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
        }
        QDialog::mouseReleaseEvent(event);
    }

private:
    // 读取指定文件的表头并更新右侧下拉框
    void updateAttributesFromSelectedFile()
    {
        subjectComboBox->clear();
        QString fileName = midtermComboBox->currentText();
        if (fileName.isEmpty() || !m_excelFileMap.contains(fileName)) return;

        QString filePath = m_excelFileMap[fileName];
        QStringList headers;
        if (!readHeaders(filePath, headers)) return;

        for (const QString& h : headers) {
            QString trimmed = h.trimmed();
            if (trimmed.isEmpty()) continue;
            // 排除常规标识列
            if (trimmed == "学号" || trimmed == "姓名" || trimmed == "小组" || trimmed == "组号") continue;
            subjectComboBox->addItem(trimmed);
        }
        if (subjectComboBox->count() > 0) {
            subjectComboBox->setCurrentIndex(0);
        }
    }

    // 读取表头（首行），支持 xlsx/xls/csv
    bool readHeaders(const QString& filePath, QStringList& headers)
    {
        QFileInfo info(filePath);
        QString suffix = info.suffix().toLower();
        if (suffix == "xlsx" || suffix == "xls") {
            QXlsx::Document xlsx(filePath);
            int col = 1;
            while (true) {
                QString val = xlsx.read(1, col).toString();
                if (val.isEmpty()) break;
                headers << val;
                ++col;
            }
            return !headers.isEmpty();
        } else if (suffix == "csv") {
            QFile f(filePath);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
            QTextStream ts(&f);
            ts.setCodec("UTF-8");
            if (!ts.atEnd()) {
                QString line = ts.readLine();
                headers = line.split(',', Qt::KeepEmptyParts);
            }
            f.close();
            return !headers.isEmpty();
        }
        return false;
    }

    void updateRightList(bool isGroup)
    {
        rightComboBox->clear();

        if (isGroup) {
            // 小组模式：2人组、4人组、6人组
            rightComboBox->addItem("2人组排座");
            rightComboBox->addItem("4人组排座");
            rightComboBox->addItem("6人组排座");
            // 默认选择第一个（2人组排座）
            rightComboBox->setCurrentIndex(0);
        } else {
            // 不分组模式：随机排座、倒序、正序
            rightComboBox->addItem("随机排座");
            rightComboBox->addItem("倒序");
            rightComboBox->addItem("正序");
            // 默认选择最后一个（正序）
            rightComboBox->setCurrentIndex(2);
        }
    }

private:
    QComboBox* leftComboBox;
    QComboBox* rightComboBox;
    QComboBox* midtermComboBox;
    QComboBox* subjectComboBox;
    QPushButton* btnConfirm;
    QPushButton* m_btnClose = nullptr;
    bool m_dragging = false;
    QPoint m_dragPos;
    QString m_classId;
    QMap<QString, QString> m_excelFileMap;

signals:
    void arrangeRequested(const QString& fileName, const QString& fieldName, const QString& mode, bool isGroupMode);
};

