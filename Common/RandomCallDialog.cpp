#include "RandomCallDialog.h"
#include "StudentAttributeDialog.h"
#include "RandomCallMessageDialog.h"
#include <QInputDialog>
#include <QTimer>
#include <QGraphicsEffect>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QApplication>
#include <QThread>
#include <random>
#include <algorithm>
#include <QSet>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>

RandomCallDialog::RandomCallDialog(QWidget* parent)
    : QDialog(parent)
    , m_seatTable(nullptr)
    , animationTimer(nullptr)
    , animationStep(0)
    , isAnimating(false)
    , m_dragging(false)
{
    // 设置无边框窗口
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setStyleSheet("background-color: rgb(85, 85, 85);");
    resize(500, 350); // 还原为原始大小

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 50, 30, 30);

    // 顶部栏：关闭按钮和数字按钮
    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->addStretch();
    
    //// 黄色圆圈带红色数字2
    //QPushButton* btnNumber = new QPushButton("2");
    //btnNumber->setFixedSize(30, 30);
    //btnNumber->setStyleSheet(
    //    "QPushButton {"
    //    "background-color: yellow;"
    //    "color: red;"
    //    "border-radius: 15px;"
    //    "font-weight: bold;"
    //    "font-size: 16px;"
    //    "border: none;"
    //    "}"
    //);
    //topLayout->addWidget(btnNumber);
    
    // 关闭按钮
    QPushButton* btnClose = new QPushButton("✕");
    btnClose->setFixedSize(30, 30);
    btnClose->setStyleSheet(
        "QPushButton {"
        "background-color: #666666;"
        "color: white;"
        "border-radius: 4px;"
        "font-weight: bold;"
        "font-size: 18px;"
        "border: none;"
        "}"
        "QPushButton:hover {"
        "background-color: #777777;"
        "}"
    );
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);
    topLayout->addWidget(btnClose);
    
    mainLayout->addLayout(topLayout);

    // 标题框："指定参与者"
    QLabel* titleLabel = new QLabel("指定参与者");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "background-color: black;"
        "color: white;"
        "font-size: 18px;"
        "font-weight: bold;"
        "padding: 10px;"
        "border: none;"
    );
    titleLabel->setFixedHeight(50);
    mainLayout->addWidget(titleLabel);

    // 表格和属性选择（绿色按钮）
    QHBoxLayout* selectLayout = new QHBoxLayout;
    selectLayout->setSpacing(15);
    
    // 表格选择按钮
    QPushButton* btnTable = new QPushButton("期中成绩表");
    btnTable->setStyleSheet(
        "QPushButton {"
        "background-color: #666666;"
        "color: white;"
        "font-size: 14px;"
        "padding: 8px 16px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #777777;"
        "}"
    );
    btnTable->setFixedHeight(35);
    connect(btnTable, &QPushButton::clicked, this, [this]() {
        // 可以在这里添加下拉菜单或对话框来选择表格
        // 暂时保持简单，使用ComboBox
    });
    selectLayout->addWidget(btnTable);
    
    // 属性选择按钮
    QPushButton* btnAttr = new QPushButton("数学");
    btnAttr->setStyleSheet(
        "QPushButton {"
        "background-color: #666666;"
        "color: white;"
        "font-size: 14px;"
        "padding: 8px 16px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #777777;"
        "}"
    );
    btnAttr->setFixedHeight(35);
    connect(btnAttr, &QPushButton::clicked, this, [this]() {
        // 可以在这里添加下拉菜单或对话框来选择属性
    });
    selectLayout->addWidget(btnAttr);
    
    selectLayout->addStretch();
    mainLayout->addLayout(selectLayout);

    // 隐藏的ComboBox（用于实际功能）
    tableComboBox = new QComboBox(this);
    tableComboBox->setStyleSheet("QComboBox { color: white; } QAbstractItemView { color: white; }");
    tableComboBox->addItem("期中成绩表");
    tableComboBox->addItem("学生体质统计表");
    tableComboBox->hide();
    connect(tableComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, btnTable](int index) {
        btnTable->setText(tableComboBox->itemText(index));
        
        // 加载对应Excel文件的数据
        if (index >= 0 && index < tableComboBox->count()) {
            QString tableName = tableComboBox->itemText(index);
            QString filePath = m_excelFileMap[tableName];
            
            if (!filePath.isEmpty()) {
                QStringList headers;
                QList<QStringList> dataRows;
                
                QFileInfo fileInfo(filePath);
                QString suffix = fileInfo.suffix().toLower();
                
                bool readSuccess = false;
                if (suffix == "xlsx" || suffix == "xls") {
                    readSuccess = readExcelFile(filePath, headers, dataRows);
                } else if (suffix == "csv") {
                    readSuccess = readCSVFile(filePath, headers, dataRows);
                }
                
                if (readSuccess && !headers.isEmpty()) {
                    // 从Excel数据创建学生信息列表
                    createStudentsFromExcelData(headers, dataRows);
                    
                    // 更新属性下拉框
                    attributeComboBox->clear();
                    for (const QString& header : headers) {
                        if (header != "学号" && header != "姓名" && header != "小组" && !header.isEmpty()) {
                            attributeComboBox->addItem(header);
                        }
                    }
                    
                    if (attributeComboBox->count() > 0) {
                        attributeComboBox->setCurrentIndex(0);
                        currentAttribute = attributeComboBox->currentText();
                    }
                    
                    updateParticipants();
                }
            }
        } else {
            updateParticipants();
        }
    });
    connect(btnTable, &QPushButton::clicked, this, [this, btnTable]() {
        // 显示下拉菜单
        // 将按钮的顶部位置转换为相对于对话框的坐标，X坐标对齐按钮左边，向上偏移一些让下拉菜单更靠近按钮
        QPoint globalPos = btnTable->mapToGlobal(QPoint(0, 0));
        QPoint localPos = this->mapFromGlobal(globalPos);
        // 设置下拉菜单的宽度与按钮相同，左边对齐
        tableComboBox->setFixedWidth(btnTable->width());
        tableComboBox->move(localPos);
        tableComboBox->showPopup();
    });

    attributeComboBox = new QComboBox(this);
    attributeComboBox->setStyleSheet("QComboBox { color: white; } QAbstractItemView { color: white; }");
    // 属性下拉框将在加载Excel文件时动态填充
    attributeComboBox->hide();
    connect(attributeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, btnAttr](int index) {
        btnAttr->setText(attributeComboBox->itemText(index));
        currentAttribute = attributeComboBox->currentText();
        updateParticipants();
    });
    connect(btnAttr, &QPushButton::clicked, this, [this, btnAttr]() {
        // 显示下拉菜单
        // 将按钮的顶部位置转换为相对于对话框的坐标，X坐标对齐按钮左边，向上偏移一些让下拉菜单更靠近按钮
        QPoint globalPos = btnAttr->mapToGlobal(QPoint(0, 0));
        QPoint localPos = this->mapFromGlobal(globalPos);
        // 设置下拉菜单的宽度与按钮相同，左边对齐
        attributeComboBox->setFixedWidth(btnAttr->width());
        attributeComboBox->move(localPos);
        attributeComboBox->showPopup();
    });

    // 属性值范围（灰色输入框）
    QHBoxLayout* rangeLayout = new QHBoxLayout;
    rangeLayout->setSpacing(10);
    
    minValueEdit = new QLineEdit("0");
    minValueEdit->setStyleSheet(
        "QLineEdit {"
        "background-color: #d0d0d0;"
        "border: 1px solid #999;"
        "padding: 8px;"
        "font-size: 14px;"
        "border-radius: 4px;"
        "}"
    );
    minValueEdit->setFixedSize(80, 35);
    minValueEdit->setAlignment(Qt::AlignCenter);
    
    QLabel* dashLabel = new QLabel("-");
    dashLabel->setStyleSheet("font-size: 16px; color: black;");
    dashLabel->setAlignment(Qt::AlignCenter);
    
    maxValueEdit = new QLineEdit("100");
    maxValueEdit->setStyleSheet(
        "QLineEdit {"
        "background-color: #d0d0d0;"
        "border: 1px solid #999;"
        "padding: 8px;"
        "font-size: 14px;"
        "border-radius: 4px;"
        "}"
    );
    maxValueEdit->setFixedSize(80, 35);
    maxValueEdit->setAlignment(Qt::AlignCenter);
    
    rangeLayout->addStretch();
    rangeLayout->addWidget(minValueEdit);
    rangeLayout->addWidget(dashLabel);
    rangeLayout->addWidget(maxValueEdit);
    rangeLayout->addStretch();
    mainLayout->addLayout(rangeLayout);

    // 信息标签（隐藏，因为图片中没有显示）
    lblInfo = new QLabel("参与者: 0 人");
    lblInfo->setStyleSheet("color: blue; font-weight: bold; font-size: 12px;");
    lblInfo->hide(); // 隐藏，因为图片中没有
    mainLayout->addWidget(lblInfo);

    mainLayout->addStretch();

    // 确定按钮（底部右侧）
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    
    btnConfirm = new QPushButton("确定");
    btnConfirm->setStyleSheet(
        "QPushButton {"
        "background-color: #666666;"
        "color: white;"
        "font-size: 14px;"
        "padding: 8px 20px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #777777;"
        "}"
    );
    btnConfirm->setFixedHeight(35);
    btnLayout->addWidget(btnConfirm);
    mainLayout->addLayout(btnLayout);

    connect(minValueEdit, &QLineEdit::textChanged, this, &RandomCallDialog::onRangeChanged);
    connect(maxValueEdit, &QLineEdit::textChanged, this, &RandomCallDialog::onRangeChanged);
    connect(btnConfirm, &QPushButton::clicked, this, &RandomCallDialog::onConfirm);

    // 初始化当前属性（将在加载Excel文件时动态设置）
    currentAttribute = "";

    // 初始化动画定时器
    animationTimer = new QTimer(this);
    animationTimer->setInterval(100); // 每100ms切换一次
    connect(animationTimer, &QTimer::timeout, this, &RandomCallDialog::onAnimationFinished);
}

void RandomCallDialog::setStudentData(const QList<struct StudentInfo>& students)
{
    m_students = students;
    updateParticipants();
}

void RandomCallDialog::setSeatTable(QTableWidget* seatTable)
{
    m_seatTable = seatTable;
    
    // 保存原始样式并连接所有座位按钮的点击事件
    if (m_seatTable) {
        for (int row = 0; row < 6; ++row) {
            for (int col = 0; col < 10; ++col) {
                QPushButton* btn = qobject_cast<QPushButton*>(m_seatTable->cellWidget(row, col));
                if (btn && btn->property("isSeat").toBool()) {
                    // 保存原始样式
                    if (!originalStyles.contains(btn)) {
                        originalStyles[btn] = btn->styleSheet();
                    }
                    // 断开之前的连接（如果有）
                    btn->disconnect(this);
                    // 重新连接点击事件
                    connect(btn, &QPushButton::clicked, this, &RandomCallDialog::onSeatClicked, Qt::UniqueConnection);
                }
            }
        }
    }
}

QList<struct StudentInfo> RandomCallDialog::getParticipants() const
{
    return m_participants;
}

void RandomCallDialog::onTableChanged(int index)
{
    Q_UNUSED(index);
    updateParticipants();
}

void RandomCallDialog::onAttributeChanged(int index)
{
    Q_UNUSED(index);
    currentAttribute = attributeComboBox->currentText();
    updateParticipants();
}

void RandomCallDialog::onRangeChanged()
{
    updateParticipants();
}

void RandomCallDialog::onConfirm()
{
    if (m_participants.isEmpty()) {
        QMessageBox::warning(this, "提示", "没有符合条件的参与者！");
        return;
    }

    // 停止之前的动画
    if (animationTimer->isActive()) {
        animationTimer->stop();
    }
    clearAllHighlights();

    // 开始随机动画
    startRandomAnimation();
}

void RandomCallDialog::onAnimationFinished()
{
    animationStep++;
    
    // 3秒 = 30步（每100ms一步）
    if (animationStep >= 30) {
        animationTimer->stop();
        isAnimating = false;
        qDebug() << "动画结束，步数:" << animationStep;
        
        // 先清除所有高亮，确保没有残留的高亮效果
        clearAllHighlights();
        
        // 强制刷新界面，确保清除效果可见
        if (m_seatTable) {
            m_seatTable->update();
            m_seatTable->repaint();
            QApplication::processEvents();
        }
        
        // 随机选择一个参与者
        if (!m_participants.isEmpty()) {
            std::random_device rd;
            std::mt19937 g(rd());
            std::uniform_int_distribution<> dis(0, m_participants.size() - 1);
            int randomIndex = dis(g);
            selectedStudentId = m_participants[randomIndex].id;
            
            qDebug() << "最终选中的学生ID:" << selectedStudentId << "，姓名:" << m_participants[randomIndex].name;
            
            // 保存学生信息用于延迟显示
            QString studentName = m_participants[randomIndex].name;
            QString studentId = selectedStudentId;
            
            // 使用单次定时器延迟高亮，确保清除操作完成后再高亮
            QTimer::singleShot(50, this, [this, studentId]() {
                // 再次清除所有高亮（防止在延迟期间有其他高亮）
                clearAllHighlights();
                QApplication::processEvents();
                
                // 高亮选中的学生（确保只高亮一个）
                highlightStudent(studentId, true);
                
                // 强制刷新界面，确保高亮效果可见
                if (m_seatTable) {
                    m_seatTable->update();
                    m_seatTable->repaint();
                    QApplication::processEvents();
                }
                
                qDebug() << "已高亮选中的学生ID:" << studentId;
            });
            
            // 延迟显示消息对话框，确保高亮效果先显示
            QTimer::singleShot(150, this, [this, studentName, studentId]() {
                // 使用自定义消息对话框
                RandomCallMessageDialog* msgDlg = new RandomCallMessageDialog(this);
                msgDlg->setTitle("随机点名");
                msgDlg->setMessage(QString("选中学生: %1 (学号: %2)").arg(studentName).arg(studentId));
                msgDlg->setTitleBarColor(QColor(85, 85, 85));  // RGB(85, 85, 85)
                msgDlg->setBackgroundColor(QColor(85, 85, 85));  // RGB(85, 85, 85)
                msgDlg->exec();
            });
        }
        return;
    }
    
    // 随机高亮一个参与者（动画过程中）
    if (!m_participants.isEmpty()) {
        clearAllHighlights();
        // 强制刷新界面，确保清除效果可见
        if (m_seatTable) {
            m_seatTable->update();
            QApplication::processEvents(); // 处理事件，确保界面更新
        }
        
        std::random_device rd;
        std::mt19937 g(rd());
        std::uniform_int_distribution<> dis(0, m_participants.size() - 1);
        int randomIndex = dis(g);
        QString studentId = m_participants[randomIndex].id;
        qDebug() << "动画步骤" << animationStep << "，高亮学生ID:" << studentId;
        highlightStudent(studentId, true);
        
        // 再次强制刷新界面
        QApplication::processEvents();
    }
}

void RandomCallDialog::onSeatClicked()
{
    if (isAnimating) return; // 动画进行中不允许点击
    
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    QString studentId = btn->property("studentId").toString();
    if (studentId.isEmpty()) return;
    
    // 检查是否是选中的学生
    if (studentId != selectedStudentId) {
        QMessageBox::information(this, "提示", "请点击被选中的学生！");
        return;
    }
    
    // 查找学生信息
    struct StudentInfo* student = nullptr;
    for (auto& s : m_students) {
        if (s.id == studentId) {
            student = &s;
            break;
        }
    }
    
    if (!student) return;
    
    // 打开属性编辑对话框
    StudentAttributeDialog* attrDlg = new StudentAttributeDialog(this);
    
    // 设置标题为当前选中的表格名称
    if (tableComboBox && tableComboBox->count() > 0) {
        QString currentTable = tableComboBox->currentText();
        attrDlg->setTitle(currentTable);
    }
    
    // 动态获取可用属性列表（从学生数据中获取所有属性）
    QList<QString> attributes;
    QSet<QString> attributeSet; // 使用Set避免重复
    
    // 遍历所有学生，收集所有属性名称
    for (const auto& s : m_students) {
        for (auto it = s.attributes.begin(); it != s.attributes.end(); ++it) {
            attributeSet.insert(it.key());
        }
    }
    
    // 转换为列表并排序
    attributes = attributeSet.values();
    std::sort(attributes.begin(), attributes.end());
    
    attrDlg->setAvailableAttributes(attributes);
    
    // 设置学生信息
    attrDlg->setStudentInfo(*student);
    
    // 连接属性更新信号
    connect(attrDlg, &StudentAttributeDialog::attributeUpdated, this, 
        [this, studentId](const QString& id, const QString& attrName, double newValue, const QString& excelFileName) {
            // 更新学生数据
            for (auto& s : m_students) {
                if (s.id == id) {
                    if (excelFileName.isEmpty()) {
                        s.attributes[attrName] = newValue;
                    } else {
                        s.attributesByExcel[excelFileName][attrName] = newValue;
                        s.attributesFull[QString("%1_%2").arg(attrName, excelFileName)] = newValue;
                    }
                    // 如果是当前选择的属性，也更新score
                    if (attrName == currentAttribute || attrName == "总分") {
                        s.score = newValue;
                    }
                    break;
                }
            }
            // 更新参与者列表
            for (auto& p : m_participants) {
                if (p.id == id) {
                    if (excelFileName.isEmpty()) {
                        p.attributes[attrName] = newValue;
                    } else {
                        p.attributesByExcel[excelFileName][attrName] = newValue;
                        p.attributesFull[QString("%1_%2").arg(attrName, excelFileName)] = newValue;
                    }
                    if (attrName == currentAttribute || attrName == "总分") {
                        p.score = newValue;
                    }
                    break;
                }
            }
            // 发送信号通知父窗口
            emit studentScoreUpdated(id, newValue);
            // 重新计算参与者
            updateParticipants();
        });
    
    attrDlg->exec();
    attrDlg->deleteLater();
}

void RandomCallDialog::updateParticipants()
{
    m_participants.clear();
    
    if (m_students.isEmpty()) {
        lblInfo->setText("参与者: 0 人");
        return;
    }
    
    // 获取范围值
    bool ok1, ok2;
    double minValue = minValueEdit->text().toDouble(&ok1);
    double maxValue = maxValueEdit->text().toDouble(&ok2);
    
    if (!ok1 || !ok2) {
        lblInfo->setText("参与者: 0 人（范围值无效）");
        return;
    }
    
    // 根据当前选择的属性筛选参与者
    QString selectedAttr = currentAttribute;
    for (const auto& student : m_students) {
        // 使用新的辅助函数获取属性值（优先级：attributesByExcel → attributesFull → attributes）
        double value = student.getAttributeValue(selectedAttr);
        
        // 如果学生没有该属性，跳过该学生
        if (value == 0.0 && !student.attributes.contains(selectedAttr) && 
            !student.attributesByExcel.isEmpty() && !student.attributesFull.isEmpty()) {
            // 检查是否真的没有该属性（在所有Excel文件中都没有）
            bool hasAttribute = false;
            for (auto it = student.attributesByExcel.begin(); it != student.attributesByExcel.end(); ++it) {
                if (it.value().contains(selectedAttr)) {
                    hasAttribute = true;
                    break;
                }
            }
            if (!hasAttribute) {
                for (auto it = student.attributesFull.begin(); it != student.attributesFull.end(); ++it) {
                    QString key = it.key();
                    int underscorePos = key.lastIndexOf('_');
                    if (underscorePos > 0 && key.left(underscorePos) == selectedAttr) {
                        hasAttribute = true;
                        break;
                    }
                }
            }
            if (!hasAttribute) {
                continue;
            }
        }
        
        if (value >= minValue && value <= maxValue) {
            m_participants.append(student);
        }
    }
    
    lblInfo->setText(QString("参与者: %1 人").arg(m_participants.size()));
}

void RandomCallDialog::startRandomAnimation()
{
    if (m_participants.isEmpty()) {
        qDebug() << "startRandomAnimation: 参与者列表为空";
        return;
    }
    
    qDebug() << "开始随机动画，参与者数量:" << m_participants.size() << "，座位表格:" << (m_seatTable ? "存在" : "不存在");
    isAnimating = true;
    animationStep = 0;
    animationTimer->start();
    qDebug() << "动画定时器已启动，间隔:" << animationTimer->interval() << "ms";
}

void RandomCallDialog::highlightStudent(const QString& studentId, bool highlight)
{
    QPushButton* btn = findSeatButtonByStudentId(studentId);
    if (!btn) {
        qDebug() << "highlightStudent: 未找到学生ID为" << studentId << "的座位按钮";
        return;
    }
    
    if (highlight) {
        // 保存原始样式（如果还没有保存）
        if (!originalStyles.contains(btn)) {
            originalStyles[btn] = btn->styleSheet();
        }
        
        // 添加高亮效果：明显的蓝色边框和背景
        QString highlightStyle = 
            "QPushButton { "
            "border: 5px solid #0066FF !important; "
            "background-color: #3399FF !important; "
            "color: #FFFFFF !important; "
            "font-weight: bold !important; "
            "font-size: 14px !important; "
            "border-radius: 4px !important; "
            "}";
        btn->setStyleSheet(highlightStyle);
        
        // 添加明显的蓝色阴影效果
        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect;
        shadow->setBlurRadius(25);
        shadow->setColor(QColor(0, 102, 255, 255)); // 蓝色阴影
        shadow->setOffset(0, 0);
        shadow->setXOffset(0);
        shadow->setYOffset(0);
        btn->setGraphicsEffect(shadow);
        
        // 确保按钮可见（提升到最前）
        btn->raise();
        btn->show(); // 确保按钮可见
        btn->setVisible(true); // 再次确保可见
        
        // 如果按钮在表格中，尝试滚动到按钮位置
        if (m_seatTable) {
            // 查找按钮在表格中的位置
            for (int row = 0; row < 6; ++row) {
                for (int col = 0; col < 10; ++col) {
                    QPushButton* cellBtn = qobject_cast<QPushButton*>(m_seatTable->cellWidget(row, col));
                    if (cellBtn == btn) {
                        // 滚动到该单元格
                        m_seatTable->scrollToItem(m_seatTable->item(row, col), QAbstractItemView::EnsureVisible);
                        break;
                    }
                }
            }
        }
        
        // 强制更新按钮和表格
        btn->repaint(); // 强制重绘
        btn->update(); // 更新显示
        
        // 如果按钮在表格中，也需要更新表格
        if (m_seatTable) {
            m_seatTable->update();
            m_seatTable->repaint();
        }
        
        // 多次处理事件，确保界面更新
        QApplication::processEvents();
        QApplication::processEvents();
        
        // 再次强制更新，确保高亮效果显示
        QTimer::singleShot(10, this, [btn, m_seatTable = this->m_seatTable]() {
            if (btn) {
                btn->repaint();
                btn->update();
            }
            if (m_seatTable) {
                m_seatTable->update();
                m_seatTable->repaint();
            }
            QApplication::processEvents();
        });
        
        qDebug() << "高亮学生ID:" << studentId << "，按钮位置:" << btn->pos() << "，按钮文本:" << btn->text();
    } else {
        // 移除高亮效果
        btn->setGraphicsEffect(nullptr);
        // 恢复原始样式
        if (originalStyles.contains(btn)) {
            btn->setStyleSheet(originalStyles[btn]);
        }
    }
}

void RandomCallDialog::clearAllHighlights()
{
    if (!m_seatTable) return;
    
    // 遍历所有座位按钮，清除高亮效果
    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 10; ++col) {
            QPushButton* btn = qobject_cast<QPushButton*>(m_seatTable->cellWidget(row, col));
            if (btn && btn->property("isSeat").toBool()) {
                // 先移除图形效果（Qt会自动删除旧的效果对象）
                if (btn->graphicsEffect()) {
                    btn->setGraphicsEffect(nullptr);
                }
                
                // 恢复原始样式
                if (originalStyles.contains(btn)) {
                    btn->setStyleSheet(originalStyles[btn]);
                } else {
                    // 如果没有保存原始样式，使用默认样式
                    btn->setStyleSheet("");
                }
                
                // 强制更新按钮
                btn->update();
                btn->repaint();
            }
        }
    }
    
    // 强制更新表格
    if (m_seatTable) {
        m_seatTable->update();
        m_seatTable->repaint();
    }
}

QPushButton* RandomCallDialog::findSeatButtonByStudentId(const QString& studentId)
{
    if (!m_seatTable) return nullptr;
    
    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 10; ++col) {
            QPushButton* btn = qobject_cast<QPushButton*>(m_seatTable->cellWidget(row, col));
            if (btn && btn->property("isSeat").toBool()) {
                if (btn->property("studentId").toString() == studentId) {
                    return btn;
                }
            }
        }
    }
    return nullptr;
}

void RandomCallDialog::updateStudentScore(const QString& studentId, double newScore)
{
    // 更新学生数据中的成绩
    for (auto& student : m_students) {
        if (student.id == studentId) {
            student.score = newScore;
            break;
        }
    }
    
    // 更新参与者列表中的成绩
    for (auto& participant : m_participants) {
        if (participant.id == studentId) {
            participant.score = newScore;
            break;
        }
    }
    
    // 通知父窗口更新学生数据
    emit studentScoreUpdated(studentId, newScore);
}

double RandomCallDialog::getStudentScore(const QString& studentId)
{
    for (const auto& student : m_students) {
        if (student.id == studentId) {
            return student.score;
        }
    }
    return 0.0;
}

QString RandomCallDialog::getStudentName(const QString& studentId)
{
    for (const auto& student : m_students) {
        if (student.id == studentId) {
            return student.name;
        }
    }
    return "";
}

void RandomCallDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QDialog::mousePressEvent(event);
}

void RandomCallDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

void RandomCallDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
    QDialog::mouseReleaseEvent(event);
}

void RandomCallDialog::closeEvent(QCloseEvent* event)
{
    // 断开座位按钮的连接，恢复原来的行为
    restoreSeatButtonConnections();
    QDialog::closeEvent(event);
}

void RandomCallDialog::hideEvent(QHideEvent* event)
{
    // 断开座位按钮的连接，恢复原来的行为
    restoreSeatButtonConnections();
    QDialog::hideEvent(event);
}

// 恢复座位按钮的原始连接
void RandomCallDialog::restoreSeatButtonConnections()
{
    if (!m_seatTable) return;
    
    // 断开所有座位按钮与 onSeatClicked 的连接
    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 10; ++col) {
            QPushButton* btn = qobject_cast<QPushButton*>(m_seatTable->cellWidget(row, col));
            if (btn && btn->property("isSeat").toBool()) {
                // 断开与 onSeatClicked 的连接
                btn->disconnect(this);
                // 清除高亮效果
                btn->setGraphicsEffect(nullptr);
                if (originalStyles.contains(btn)) {
                    btn->setStyleSheet(originalStyles[btn]);
                }
            }
        }
    }
    
    // 清除选中的学生ID
    selectedStudentId.clear();
    isAnimating = false;
    
    qDebug() << "已恢复座位按钮的原始连接";
}

// 加载已下载的Excel文件并更新表格和属性选择
void RandomCallDialog::loadExcelFiles(const QString& classId)
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
    QStringList excelFiles;
    m_excelFileMap.clear();
    
    for (const QFileInfo& fileInfo : fileList) {
        QString filePath = fileInfo.absoluteFilePath();
        QString fileName = fileInfo.baseName(); // 去掉扩展名的文件名
        
        excelFiles.append(fileName);
        m_excelFileMap[fileName] = filePath;
    }
    
    // 更新表格和属性下拉框
    updateTableAndAttributeComboBoxes(excelFiles);
    
    // 如果有Excel文件，加载第一个文件的数据
    if (!excelFiles.isEmpty()) {
        QString firstTable = excelFiles.first();
        QString firstFilePath = m_excelFileMap[firstTable];
        
        // 读取Excel文件
        QStringList headers;
        QList<QStringList> dataRows;
        
        QFileInfo fileInfo(firstFilePath);
        QString suffix = fileInfo.suffix().toLower();
        
        bool readSuccess = false;
        if (suffix == "xlsx" || suffix == "xls") {
            readSuccess = readExcelFile(firstFilePath, headers, dataRows);
        } else if (suffix == "csv") {
            readSuccess = readCSVFile(firstFilePath, headers, dataRows);
        }
        
        if (readSuccess && !headers.isEmpty()) {
            // 从Excel数据创建学生信息列表
            createStudentsFromExcelData(headers, dataRows);
            
            // 更新参与者列表
            updateParticipants();
        }
    }
}

// 更新表格和属性下拉框
void RandomCallDialog::updateTableAndAttributeComboBoxes(const QStringList& excelFiles)
{
    // 清空并更新表格下拉框
    tableComboBox->clear();
    for (const QString& fileName : excelFiles) {
        tableComboBox->addItem(fileName);
    }
    
    // 如果当前选择的表格不在列表中，选择第一个
    if (tableComboBox->count() > 0) {
        tableComboBox->setCurrentIndex(0);
    }
    
    // 更新属性下拉框（需要读取当前选择的Excel文件）
    if (tableComboBox->count() > 0) {
        QString currentTable = tableComboBox->currentText();
        QString filePath = m_excelFileMap[currentTable];
        
        if (!filePath.isEmpty()) {
            QStringList headers;
            QList<QStringList> dataRows;
            
            QFileInfo fileInfo(filePath);
            QString suffix = fileInfo.suffix().toLower();
            
            bool readSuccess = false;
            if (suffix == "xlsx" || suffix == "xls") {
                readSuccess = readExcelFile(filePath, headers, dataRows);
            } else if (suffix == "csv") {
                readSuccess = readCSVFile(filePath, headers, dataRows);
            }
            
            if (readSuccess && !headers.isEmpty()) {
                // 更新属性下拉框（排除固定列）
                attributeComboBox->clear();
                for (const QString& header : headers) {
                    if (header != "学号" && header != "姓名" && header != "小组" && !header.isEmpty()) {
                        attributeComboBox->addItem(header);
                    }
                }
                
                // 如果当前属性不在列表中，选择第一个
                if (attributeComboBox->count() > 0) {
                    int index = attributeComboBox->findText(currentAttribute);
                    if (index >= 0) {
                        attributeComboBox->setCurrentIndex(index);
                    } else {
                        attributeComboBox->setCurrentIndex(0);
                        currentAttribute = attributeComboBox->currentText();
                    }
                }
            }
        }
    }
}

// 读取Excel文件
bool RandomCallDialog::readExcelFile(const QString& fileName, QStringList& headers, QList<QStringList>& dataRows)
{
    using namespace QXlsx;
    
    if (!QFile::exists(fileName)) {
        return false;
    }
    
    Document xlsx(fileName);
    
    // 读取第一行作为表头
    int col = 1;
    headers.clear();
    while (true) {
        QVariant cellValue = xlsx.read(1, col);
        if (cellValue.isNull()) {
            if (col == 1) {
                return false;
            }
            break;
        }
        QString cellText = cellValue.toString().trimmed();
        if (cellText.isEmpty() && col > 1) {
            break;
        }
        headers.append(cellText);
        ++col;
        if (col > 1000) {
            break;
        }
    }
    
    if (headers.isEmpty()) {
        return false;
    }
    
    // 读取数据行
    dataRows.clear();
    int row = 2;
    int maxRows = 10000;
    
    while (row <= maxRows) {
        QStringList rowData;
        bool hasData = false;
        
        for (int c = 1; c <= headers.size(); ++c) {
            QVariant cellValue = xlsx.read(row, c);
            QString cellText = cellValue.isNull() ? "" : cellValue.toString().trimmed();
            rowData.append(cellText);
            if (!cellText.isEmpty()) {
                hasData = true;
            }
        }
        
        if (!hasData) {
            bool allEmpty = true;
            for (int checkRow = row; checkRow < row + 3 && checkRow <= maxRows; ++checkRow) {
                for (int c = 1; c <= headers.size(); ++c) {
                    QVariant cellValue = xlsx.read(checkRow, c);
                    if (!cellValue.isNull() && !cellValue.toString().trimmed().isEmpty()) {
                        allEmpty = false;
                        break;
                    }
                }
                if (!allEmpty) break;
            }
            if (allEmpty) {
                break;
            }
        }
        
        dataRows.append(rowData);
        ++row;
    }
    
    return true;
}

// 读取CSV文件
bool RandomCallDialog::readCSVFile(const QString& fileName, QStringList& headers, QList<QStringList>& dataRows)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream in(&file);
    in.setCodec("UTF-8");
    
    QString content = in.readAll();
    if (content.startsWith("\xEF\xBB\xBF")) {
        content = content.mid(3);
    }
    
    // 解析CSV内容
    QStringList lines;
    QString currentLine;
    bool inQuotes = false;
    
    for (int i = 0; i < content.length(); ++i) {
        QChar c = content[i];
        if (c == '"') {
            inQuotes = !inQuotes;
            currentLine += c;
        } else if (c == '\n' && !inQuotes) {
            if (!currentLine.isEmpty()) {
                lines.append(currentLine);
            }
            currentLine.clear();
        } else {
            currentLine += c;
        }
    }
    if (!currentLine.isEmpty()) {
        lines.append(currentLine);
    }
    
    if (lines.isEmpty()) {
        file.close();
        return false;
    }
    
    // 解析CSV行的辅助函数
    auto parseCSVLine = [](const QString& line) -> QStringList {
        QStringList fields;
        QString currentField;
        bool inQuotes = false;
        
        for (int i = 0; i < line.length(); ++i) {
            QChar c = line[i];
            if (c == '"') {
                if (i + 1 < line.length() && line[i + 1] == '"') {
                    currentField += '"';
                    ++i;
                } else {
                    inQuotes = !inQuotes;
                }
            } else if (c == ',' && !inQuotes) {
                fields.append(currentField.trimmed());
                currentField.clear();
            } else {
                currentField += c;
            }
        }
        fields.append(currentField.trimmed());
        return fields;
    };
    
    // 读取表头
    headers = parseCSVLine(lines[0]);
    
    // 读取数据行
    dataRows.clear();
    for (int i = 1; i < lines.size(); ++i) {
        QStringList fields = parseCSVLine(lines[i]);
        if (!fields.isEmpty() && !fields[0].trimmed().isEmpty()) {
            dataRows.append(fields);
        }
    }
    
    file.close();
    return true;
}

// 从Excel数据创建学生信息列表
void RandomCallDialog::createStudentsFromExcelData(const QStringList& headers, const QList<QStringList>& dataRows)
{
    m_students.clear();
    
    // 找到学号、姓名、小组列的索引
    int colGroup = -1, colId = -1, colName = -1, colGroupTotal = -1;
    QMap<QString, int> attributeColumnMap;
    
    for (int i = 0; i < headers.size(); ++i) {
        QString header = headers[i];
        if (header == "小组") {
            colGroup = i;
        } else if (header == "学号") {
            colId = i;
        } else if (header == "姓名") {
            colName = i;
        } else if (header == "小组总分") {
            colGroupTotal = i;
        } else if (header != "小组" && header != "小组总分" && !header.isEmpty()) {
            attributeColumnMap[header] = i;
        }
    }
    
    if (colName < 0) {
        qWarning() << "Excel文件中缺少姓名列";
        return;
    }
    
    // 创建学生信息列表
    for (const QStringList& rowData : dataRows) {
        if (rowData.size() <= colName) continue;
        
        StudentInfo student;
        student.id = (colId >= 0 && colId < rowData.size()) ? rowData[colId].trimmed() : "";
        student.groupName = (colGroup >= 0 && colGroup < rowData.size()) ? rowData[colGroup].trimmed() : "";
        student.name = rowData[colName].trimmed();
        
        if (student.name.isEmpty()) continue;
        
        // 读取所有属性列
        for (auto it = attributeColumnMap.begin(); it != attributeColumnMap.end(); ++it) {
            QString columnName = it.key();
            int col = it.value();
            
            if (col < rowData.size() && !rowData[col].trimmed().isEmpty()) {
                bool ok;
                double value = rowData[col].toDouble(&ok);
                if (ok) {
                    student.attributes[columnName] = value;
                    
                    // 如果属性是"总分"，也设置到score
                    if (columnName == "总分") {
                        student.score = value;
                    }
                }
            }
        }
        
        // 如果没有总分，尝试计算（从所有Excel文件的属性中计算）
        double totalFromAttrs = student.getAttributeValue("总分");
        if (totalFromAttrs == 0.0 && student.score == 0) {
            double total = 0;
            // 从所有Excel文件的属性中计算总和
            QSet<QString> processedAttrs; // 避免重复计算
            for (auto excelIt = student.attributesByExcel.begin(); excelIt != student.attributesByExcel.end(); ++excelIt) {
                const QMap<QString, double>& excelAttrs = excelIt.value();
                for (auto it = excelAttrs.begin(); it != excelAttrs.end(); ++it) {
                    QString attrName = it.key();
                    // 只计算数值属性，排除"总分"和"小组总分"
                    if (attrName != "总分" && attrName != "小组总分" && !processedAttrs.contains(attrName)) {
                        total += it.value();
                        processedAttrs.insert(attrName);
                    }
                }
            }
            // 从 attributesFull 中计算
            for (auto it = student.attributesFull.begin(); it != student.attributesFull.end(); ++it) {
                QString compositeKey = it.key();
                int underscorePos = compositeKey.lastIndexOf('_');
                if (underscorePos > 0) {
                    QString attrName = compositeKey.left(underscorePos);
                    if (attrName != "总分" && attrName != "小组总分" && !processedAttrs.contains(attrName)) {
                        total += it.value();
                        processedAttrs.insert(attrName);
                    }
                }
            }
            // 向后兼容：从 attributes 中计算
            for (auto it = student.attributes.begin(); it != student.attributes.end(); ++it) {
                QString attrName = it.key();
                if (attrName != "总分" && attrName != "小组总分" && !processedAttrs.contains(attrName)) {
                    total += it.value();
                    processedAttrs.insert(attrName);
                }
            }
            if (total > 0) {
                student.attributes["总分"] = total;
                student.score = total;
            }
        }

        // 读取小组总分（如果存在）
        if (colGroupTotal >= 0 && colGroupTotal < rowData.size()) {
            bool ok = false;
            double groupTotal = rowData[colGroupTotal].trimmed().toDouble(&ok);
            if (ok) {
                student.groupTotalScore = groupTotal;
                student.attributes["小组总分"] = groupTotal;
            }
        }
        
        student.originalIndex = m_students.size();
        m_students.append(student);
    }
    
    qDebug() << "从Excel文件创建了" << m_students.size() << "个学生信息";
}

