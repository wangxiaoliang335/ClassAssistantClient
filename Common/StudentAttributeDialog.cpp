#include "StudentAttributeDialog.h"
#include "ScheduleDialog.h"
#include "ScoreHeaderIdStorage.h"
#include "CommentStorage.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QMap>
#include <QApplication>
#include <QLayoutItem>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>

static void clearLayoutRecursive(QLayout* layout)
{
    if (!layout) return;
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            // 这里必须立即销毁，否则在重建UI时控件可能短暂“脱离布局”漂到顶部
            delete w;
        }
        if (QLayout* childLayout = item->layout()) {
            // 注意：不要在这里 delete childLayout。
            // childLayout 的生命周期由其父对象/父layout item管理，手动 delete 容易导致 double free。
            clearLayoutRecursive(childLayout);
        }
        delete item;
    }
}

StudentAttributeDialog::StudentAttributeDialog(QWidget* parent)
    : QDialog(parent)
    , m_selectedAttribute("")
    , m_currentEditingAttribute("")
    , m_selectedExcelTable("")
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setStyleSheet("background-color: #808080;"); // 灰色背景
    resize(400, 500);
    
    // 启用鼠标跟踪以检测鼠标进入/离开
    setMouseTracking(true);

    // 创建关闭按钮
    m_btnClose = new QPushButton("X", this);
    m_btnClose->setFixedSize(30, 30);
    m_btnClose->setStyleSheet(
        "QPushButton { background-color: #666666; color: white; font-weight: bold; font-size: 14px; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #777777; }"
    );
    m_btnClose->hide(); // 初始隐藏
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::close);
    
    // 为关闭按钮安装事件过滤器，确保鼠标在按钮上时不会隐藏
    m_btnClose->installEventFilter(this);

    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 40, 20, 20); // 增加顶部边距，为关闭按钮留出空间
    outerLayout->setSpacing(15);

    // 标题："学生统计信息"
    m_titleLabel = new QLabel("学生统计信息");
    m_titleLabel->setStyleSheet(
        "background-color: #d0d0d0;"
        "color: black;"
        "font-size: 14px;"
        "padding: 8px;"
        "border-radius: 4px;"
    );
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    outerLayout->addWidget(m_titleLabel);

    // Excel表格选择（仅显示下拉框；不显示“表格：”label）
    QHBoxLayout* excelLayout = new QHBoxLayout;
    excelLayout->setSpacing(10);
    m_excelComboBox = new QComboBox(this);
    m_excelComboBox->setStyleSheet(
        "QComboBox { color: white; background-color: #666666; padding: 6px 10px; border-radius: 4px; border: 1px solid #555; }"
        "QComboBox QAbstractItemView { color: white; background-color: #666666; selection-background-color: #00cc00; }"
    );
    m_excelComboBox->hide();
    connect(m_excelComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StudentAttributeDialog::onExcelTableChanged);

    excelLayout->addWidget(m_excelComboBox, 1);
    outerLayout->addLayout(excelLayout);

    // 主内容区域
    m_mainLayout = new QVBoxLayout;
    m_mainLayout->setSpacing(10);
    outerLayout->addLayout(m_mainLayout);

    outerLayout->addStretch();
}

void StudentAttributeDialog::setScoreContext(const QString& classId, const QString& examName, const QString& term)
{
    m_classId = classId;
    m_examName = examName;
    m_term = term;
    m_scoreHeaderId = ScoreHeaderIdStorage::getScoreHeaderId(m_classId, m_examName, m_term);
    qDebug() << "StudentAttributeDialog score context:" << m_classId << m_examName << m_term
             << "score_header_id=" << m_scoreHeaderId;

    // 如果还没有缓存到 score_header_id，后台尝试拉取一次（不打断用户）
    if (m_scoreHeaderId <= 0 && !m_classId.isEmpty() && !m_examName.isEmpty() && !m_term.isEmpty()) {
        fetchScoreHeaderIdFromServer(nullptr);
    }
}

void StudentAttributeDialog::fetchScoreHeaderIdFromServer(std::function<void(bool)> onDone)
{
    if (m_scoreHeaderId > 0) {
        if (onDone) onDone(true);
        return;
    }
    if (m_classId.isEmpty() || m_examName.isEmpty() || m_term.isEmpty()) {
        qDebug() << "fetchScoreHeaderIdFromServer: context missing";
        if (onDone) onDone(false);
        return;
    }

    QUrl url("http://47.100.126.194:5000/student-scores");
    QUrlQuery q;
    q.addQueryItem("class_id", m_classId);
    q.addQueryItem("exam_name", m_examName);
    q.addQueryItem("term", m_term);
    url.setQuery(q);

    QNetworkAccessManager* networkManager = new QNetworkAccessManager(this);
    QNetworkRequest req(url);
    QNetworkReply* reply = networkManager->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, networkManager, onDone]() {
        bool ok = false;
        if (reply->error() == QNetworkReply::NoError) {
            const QByteArray bytes = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(bytes);
            if (doc.isObject()) {
                QJsonObject root = doc.object();
                QJsonObject dataObj = root.value("data").toObject();

                // 尽可能兼容不同返回：data.headers[] / data.header / data.id / data.score_header_id
                int scoreHeaderId = -1;
                if (dataObj.contains("headers") && dataObj.value("headers").isArray()) {
                    QJsonArray headers = dataObj.value("headers").toArray();
                    if (!headers.isEmpty() && headers.first().isObject()) {
                        QJsonObject headerObj = headers.first().toObject();
                        if (headerObj.contains("score_header_id")) scoreHeaderId = headerObj.value("score_header_id").toInt(-1);
                        if (scoreHeaderId <= 0 && headerObj.contains("id")) scoreHeaderId = headerObj.value("id").toInt(-1);
                    }
                }
                if (scoreHeaderId <= 0 && dataObj.contains("header") && dataObj.value("header").isObject()) {
                    QJsonObject headerObj = dataObj.value("header").toObject();
                    if (headerObj.contains("score_header_id")) scoreHeaderId = headerObj.value("score_header_id").toInt(-1);
                    if (scoreHeaderId <= 0 && headerObj.contains("id")) scoreHeaderId = headerObj.value("id").toInt(-1);
                }
                if (scoreHeaderId <= 0 && dataObj.contains("score_header_id")) {
                    scoreHeaderId = dataObj.value("score_header_id").toInt(-1);
                }
                if (scoreHeaderId <= 0 && dataObj.contains("id")) {
                    scoreHeaderId = dataObj.value("id").toInt(-1);
                }

                if (scoreHeaderId > 0) {
                    m_scoreHeaderId = scoreHeaderId;
                    ScoreHeaderIdStorage::saveScoreHeaderId(m_classId, m_examName, m_term, m_scoreHeaderId);
                    ok = true;
                    qDebug() << "fetchScoreHeaderIdFromServer ok, id=" << m_scoreHeaderId;
                } else {
                    qDebug() << "fetchScoreHeaderIdFromServer: no score_header_id in response";
                }
            } else {
                qDebug() << "fetchScoreHeaderIdFromServer: response is not JSON object";
            }
        } else {
            qDebug() << "fetchScoreHeaderIdFromServer network error:" << reply->errorString();
        }

        reply->deleteLater();
        networkManager->deleteLater();
        if (onDone) onDone(ok);
    });
}

void StudentAttributeDialog::setStudentInfo(const struct StudentInfo& student)
{
    m_student = student;
    rebuildAttributeRows();
}

void StudentAttributeDialog::setAvailableAttributes(const QList<QString>& attributes)
{
    m_availableAttributes = attributes;
}

void StudentAttributeDialog::setExcelTables(const QStringList& tables)
{
    m_excelTables = tables;
    if (!m_excelComboBox) return;

    m_excelComboBox->blockSignals(true);
    m_excelComboBox->clear();
    for (const QString& t : m_excelTables) {
        if (t.trimmed().isEmpty()) continue;
        m_excelComboBox->addItem(t, t);
    }
    if (m_excelComboBox->count() == 0) {
        m_excelComboBox->addItem(QString::fromUtf8(u8"暂无表格"), QString());
        m_excelComboBox->setEnabled(false);
    } else {
        m_excelComboBox->setEnabled(true);
    }
    m_excelComboBox->blockSignals(false);

    // 始终显示下拉框（即使暂无数据也显示占位）
    m_excelComboBox->setVisible(true);

    // 选中预设表格（若存在）；否则选中第一个有效表格
    if (!m_selectedExcelTable.isEmpty()) {
        int idx = m_excelComboBox->findData(m_selectedExcelTable);
        if (idx >= 0) {
            m_excelComboBox->setCurrentIndex(idx);
        } else {
            m_selectedExcelTable.clear();
            m_excelComboBox->setCurrentIndex(0);
            m_selectedExcelTable = m_excelComboBox->itemData(0).toString();
        }
    } else {
        m_excelComboBox->setCurrentIndex(0);
        m_selectedExcelTable = m_excelComboBox->itemData(0).toString();
    }

    rebuildAttributeRows();
}

void StudentAttributeDialog::setSelectedExcelTable(const QString& tableName)
{
    m_selectedExcelTable = tableName;
    if (m_excelComboBox) {
        int idx = m_excelComboBox->findData(m_selectedExcelTable);
        if (idx >= 0) {
            m_excelComboBox->setCurrentIndex(idx);
        } else {
            // 找不到就回退到第一个
            m_excelComboBox->setCurrentIndex(0);
            m_selectedExcelTable = m_excelComboBox->itemData(0).toString();
        }
    }
}

void StudentAttributeDialog::onExcelTableChanged(int index)
{
    if (!m_excelComboBox || index < 0) return;
    m_selectedExcelTable = m_excelComboBox->itemData(index).toString();
    if (m_selectedExcelTable.isEmpty()) {
        // “暂无表格”等占位项
        clearLayoutRecursive(m_mainLayout);
        m_attributeValueButtons.clear();
        m_annotationButtons.clear();
        m_valueEdits.clear();
        m_selectedAttribute.clear();
        return;
    }
    rebuildAttributeRows();
}

QStringList StudentAttributeDialog::getDisplayAttributes() const
{
    // 需求：只显示当前选中表格的属性
    QStringList attrs;
    if (m_selectedExcelTable.isEmpty()) return attrs;

    // 优先从父窗口（ScheduleDialog）按表头读取字段，保证字段齐全
    if (auto* scheduleDlg = qobject_cast<ScheduleDialog*>(parent())) {
        attrs = scheduleDlg->getAttributesForTable(m_selectedExcelTable);
    }

    // 回退：从学生数据里提取（可能不完整）
    if (attrs.isEmpty() && m_student.attributesByExcel.contains(m_selectedExcelTable)) {
        attrs = m_student.attributesByExcel.value(m_selectedExcelTable).keys();
    }
    if (attrs.isEmpty()) {
        for (auto it = m_student.attributesFull.begin(); it != m_student.attributesFull.end(); ++it) {
            const QString key = it.key();
            const int underscorePos = key.lastIndexOf('_');
            if (underscorePos <= 0) continue;
            const QString fieldName = key.left(underscorePos).trimmed();
            const QString tableName = key.mid(underscorePos + 1).trimmed();
            if (tableName == m_selectedExcelTable && !fieldName.isEmpty()) {
                if (!attrs.contains(fieldName)) attrs.append(fieldName);
            }
        }
    }

    return attrs;
}

void StudentAttributeDialog::rebuildAttributeRows()
{
    if (!m_mainLayout) return;

    clearLayoutRecursive(m_mainLayout);
    m_attributeValueButtons.clear();
    m_annotationButtons.clear();
    m_valueEdits.clear();
    m_selectedAttribute.clear();

    const QStringList attrs = getDisplayAttributes();
    for (const QString& attrName : attrs) {
        const QString trimmed = attrName.trimmed();
        if (trimmed.isEmpty()) continue;

        const double value = m_student.getAttributeValue(trimmed, m_selectedExcelTable);
        updateAttributeRow(trimmed, value, false);
    }
}

void StudentAttributeDialog::updateAttributeRow(const QString& attributeName, double value, bool isHighlighted)
{
    QHBoxLayout* rowLayout = new QHBoxLayout;
    rowLayout->setSpacing(10);
    
    // 左侧绿色小方块（带"-"）
    QPushButton* leftSquare = new QPushButton("-");
    leftSquare->setFixedSize(30, 30);
    leftSquare->setStyleSheet(
        "QPushButton {"
        "background-color: green;"
        "color: white;"
        "border: none;"
        "border-radius: 4px;"
        "font-size: 16px;"
        "font-weight: bold;"
        "}"
    );
    leftSquare->setEnabled(false);
    rowLayout->addWidget(leftSquare);
    
    // 属性标签（浅蓝色按钮）
    QPushButton* attrLabel = new QPushButton(attributeName);
    attrLabel->setStyleSheet(
        "QPushButton {"
        "background-color: #87ceeb;"
        "color: black;"
        "font-size: 14px;"
        "padding: 8px 16px;"
        "border: none;"
        "border-radius: 4px;"
        "text-align: left;"
        "}"
        "QPushButton:hover {"
        "background-color: #6bb6ff;"
        "}"
    );
    attrLabel->setFixedHeight(35);
    connect(attrLabel, &QPushButton::clicked, this, [this, attributeName, attrLabel]() {
        // 点击属性标签时，选中该行
        clearAllHighlights();
        m_selectedAttribute = attributeName;
        // 使用当前选择的Excel表格获取值
        double value = m_student.getAttributeValue(attributeName, m_selectedExcelTable);
        // 找到该行并高亮
        if (m_attributeValueButtons.contains(attributeName)) {
            QPushButton* valueBtn = m_attributeValueButtons[attributeName];
            if (attributeName == QString::fromUtf8(u8"总分")) {
                // “总分”只读：保持灰色，只加高亮边框
                valueBtn->setStyleSheet(
                    "QPushButton {"
                    "background-color: #4a4a4a;"
                    "color: #dddddd;"
                    "font-size: 14px;"
                    "padding: 8px 16px;"
                    "border: 3px solid yellow;"
                    "border-radius: 4px;"
                    "}"
                );
            } else {
                valueBtn->setStyleSheet(
                    "QPushButton {"
                    "background-color: green;"
                    "color: white;"
                    "font-size: 14px;"
                    "padding: 8px 16px;"
                    "border: 3px solid yellow;"
                    "border-radius: 4px;"
                    "}"
                );
            }
        }
        if (attrLabel) {
            attrLabel->setStyleSheet(
                "QPushButton {"
                "background-color: #87ceeb;"
                "color: black;"
                "font-size: 14px;"
                "padding: 8px 16px;"
                "border: 3px solid yellow;"
                "border-radius: 4px;"
                "text-align: left;"
                "}"
            );
        }
    });
    rowLayout->addWidget(attrLabel);
    
    // 属性值（绿色方块，可点击编辑）
    QPushButton* valueBtn = new QPushButton(QString::number(value));

    const bool isTotalScore = (attributeName == QString::fromUtf8(u8"总分"));
    if (isTotalScore) {
        // “总分”不可编辑
        valueBtn->setEnabled(false);
        valueBtn->setStyleSheet(
            "QPushButton {"
            "background-color: #4a4a4a;"
            "color: #dddddd;"
            "font-size: 14px;"
            "padding: 8px 16px;"
            "border: none;"
            "border-radius: 4px;"
            "}"
        );
    } else {
        valueBtn->setStyleSheet(
            "QPushButton {"
            "background-color: green;"
            "color: white;"
            "font-size: 14px;"
            "padding: 8px 16px;"
            "border: none;"
            "border-radius: 4px;"
            "}"
            "QPushButton:hover {"
            "background-color: #00cc00;"
            "}"
        );
    }
    valueBtn->setFixedHeight(35);
    valueBtn->setProperty("attributeName", attributeName);
    if (!isTotalScore) {
        connect(valueBtn, &QPushButton::clicked, this, &StudentAttributeDialog::onAttributeValueClicked);
    }
    m_attributeValueButtons[attributeName] = valueBtn;
    rowLayout->addWidget(valueBtn);
    
    // 注释按钮（绿色方块）
    QPushButton* annotationBtn = new QPushButton("注释");
    annotationBtn->setStyleSheet(
        "QPushButton {"
        "background-color: green;"
        "color: white;"
        "font-size: 14px;"
        "padding: 8px 16px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #00cc00;"
        "}"
    );
    annotationBtn->setFixedHeight(35);
    annotationBtn->setProperty("attributeName", attributeName);
    connect(annotationBtn, &QPushButton::clicked, this, &StudentAttributeDialog::onAnnotationClicked);
    m_annotationButtons[attributeName] = annotationBtn;
    rowLayout->addWidget(annotationBtn);
    
    m_mainLayout->addLayout(rowLayout);
}

void StudentAttributeDialog::onAttributeValueClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    QString attributeName = btn->property("attributeName").toString();
    if (attributeName.isEmpty()) return;
    if (attributeName == QString::fromUtf8(u8"总分")) {
        // 防御：总分不允许修改（正常情况下该按钮已禁用，不会进来）
        QMessageBox::information(this, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"“总分”不可修改"));
        return;
    }
    
    // 使用当前选择的Excel表格获取当前值
    double currentValue = m_student.getAttributeValue(attributeName, m_selectedExcelTable);
    
    // 弹出输入对话框
    bool ok;
    double newValue = QInputDialog::getDouble(this, "修改属性值", 
        QString("请输入 %1 的新值:").arg(attributeName),
        currentValue, 0, 1000, 2, &ok);
    
    if (ok) {
        // 更新学生属性（按选择的表格写回；“全部”则写入向后兼容的 attributes）
        if (!m_selectedExcelTable.isEmpty()) {
            m_student.attributesByExcel[m_selectedExcelTable][attributeName] = newValue;
            m_student.attributesFull[QString("%1_%2").arg(attributeName).arg(m_selectedExcelTable)] = newValue;
        } else {
            m_student.attributes[attributeName] = newValue;
        }
        
        // 如果是"总分"，也更新score
        if (attributeName == "总分") {
            m_student.score = newValue;
        }
        
        // 更新按钮文本
        btn->setText(QString::number(newValue));
        
        // 发送更新信号（携带当前选择的 Excel 表格维度）
        emit attributeUpdated(m_student.id, attributeName, newValue, m_selectedExcelTable);

        // 同步到服务器：/student-scores/set-score
        sendScoreToServer(attributeName, newValue);
    }
}

void StudentAttributeDialog::sendScoreToServer(const QString& attributeName, double scoreValue)
{
    if (attributeName.isEmpty()) return;
    if (attributeName == QString::fromUtf8(u8"总分")) {
        // 总分不允许客户端修改，不上传
        return;
    }
    if (m_student.name.isEmpty()) return;

    // 不传 score_header_id：改用 class_id + term (+ excel_filename) 自动定位
    if (m_classId.trimmed().isEmpty() || m_term.trimmed().isEmpty()) {
        qDebug() << "set-score: context missing, skip upload. class_id=" << m_classId << "term=" << m_term;
        return;
    }

    QJsonObject requestObj;
    requestObj["class_id"] = m_classId.trimmed();
    requestObj["term"] = m_term.trimmed();
    requestObj["student_name"] = m_student.name;
    if (!m_student.id.isEmpty()) {
        requestObj["student_id"] = m_student.id;
    }
    requestObj["field_name"] = attributeName;
    if (!m_selectedExcelTable.isEmpty()) {
        requestObj["excel_filename"] = m_selectedExcelTable;
    }
    requestObj["score"] = scoreValue;

    QJsonDocument doc(requestObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    QNetworkAccessManager* networkManager = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setUrl(QUrl("http://47.100.126.194:5000/student-scores/set-score"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = networkManager->post(request, jsonData);
    connect(reply, &QNetworkReply::finished, this, [this, reply, networkManager, attributeName, scoreValue]() {
        if (reply->error() == QNetworkReply::NoError) {
            const QByteArray responseData = reply->readAll();
            QJsonDocument respDoc = QJsonDocument::fromJson(responseData);
            if (respDoc.isObject()) {
                QJsonObject obj = respDoc.object();
                int code = obj.value("code").toInt(-1);
                QString msg = obj.value("message").toString();
                qDebug() << "set-score resp code=" << code << "msg=" << msg << "attr=" << attributeName << "score=" << scoreValue;

                // 若服务端返回 total_score，可顺便刷新本地“总分”显示（只读）
                if (code == 200) {
                    QJsonObject dataObj = obj.value("data").toObject();
                    const QString totalName = QString::fromUtf8(u8"总分");

                    // 1) 当前 excel 的总分（总分_<excel>），优先刷新当前选择表格的“总分”行
                    QString excelFileName = dataObj.value("excel_filename").toString();
                    if (excelFileName.isEmpty()) {
                        // 如果服务端没返回 excel_filename，则用当前选择值（如果有）
                        excelFileName = m_selectedExcelTable;
                    }
                    // 当前 Excel 表维度总分：兼容新字段 excel_total 与旧字段 excel_total_score
                    const bool hasExcelTotal =
                        (dataObj.contains("excel_total") && !dataObj.value("excel_total").isNull())
                        || (dataObj.contains("excel_total_score") && !dataObj.value("excel_total_score").isNull());
                    if (hasExcelTotal && !excelFileName.isEmpty()) {
                        const double excelTotal = dataObj.contains("excel_total")
                            ? dataObj.value("excel_total").toDouble()
                            : dataObj.value("excel_total_score").toDouble();
                        // 写回本地（按表格维度）
                        m_student.attributesByExcel[excelFileName][totalName] = excelTotal;
                        m_student.attributesFull[QString("%1_%2").arg(totalName).arg(excelFileName)] = excelTotal;

                        // 如果当前弹窗正展示该表格，则刷新“总分”按钮显示
                        if (m_selectedExcelTable == excelFileName && m_attributeValueButtons.contains(totalName)) {
                            m_attributeValueButtons[totalName]->setText(QString::number(excelTotal));
                        }

                        // 同步通知外层（ScheduleDialog 等）更新缓存：总分_<excel>
                        if (!m_student.id.isEmpty()) {
                            emit attributeUpdated(m_student.id, totalName, excelTotal, excelFileName);
                        }
                    }

                    // 2) 整体汇总总分（用于排序），写入 student.score；若当前是“全部”视图，也刷新“总分”行
                    if (dataObj.contains("total_score") && !dataObj.value("total_score").isNull()) {
                        double total = dataObj.value("total_score").toDouble();
                        m_student.score = total;
                        // 兼容：全部视图用 attributes["总分"] 展示
                        if (m_selectedExcelTable.isEmpty()) {
                            m_student.attributes[totalName] = total;
                            if (m_attributeValueButtons.contains(totalName)) {
                                m_attributeValueButtons[totalName]->setText(QString::number(total));
                            }
                        }

                        // 同步通知外层（ScheduleDialog 等）更新缓存：整体总分
                        if (!m_student.id.isEmpty()) {
                            emit attributeUpdated(m_student.id, totalName, total, QString());
                        }
                    }
                }
            } else {
                qDebug() << "set-score resp:" << responseData;
            }
        } else {
            qDebug() << "set-score network error:" << reply->errorString();
        }
        reply->deleteLater();
        networkManager->deleteLater();
    });
}

void StudentAttributeDialog::onAnnotationClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    QString attributeName = btn->property("attributeName").toString();
    if (attributeName.isEmpty()) return;

    // 当前注释（优先按选中表格取注释）
    QString currentComment = m_student.getComment(attributeName, m_selectedExcelTable);
    // 若没有，尝试从全局注释缓存取（key 统一用“字段_Excel文件名”）
    if (currentComment.isEmpty() && !m_classId.isEmpty() && !m_examName.isEmpty() && !m_term.isEmpty() && !m_student.id.isEmpty()) {
        const QString fieldKey = m_selectedExcelTable.isEmpty()
            ? attributeName
            : QString("%1_%2").arg(attributeName).arg(m_selectedExcelTable);
        currentComment = CommentStorage::getComment(m_classId, m_term, m_student.id, attributeName, m_selectedExcelTable);
        if (currentComment.isEmpty() && !m_selectedExcelTable.isEmpty()) {
            const QString compositeKey = QString("%1_%2").arg(attributeName).arg(m_selectedExcelTable);
            const QString inferredTable = CommentStorage::inferTableNameFromFieldKey(compositeKey);
            currentComment = CommentStorage::getComment(m_classId, m_term, m_student.id, compositeKey, inferredTable);
        }
    }

    bool ok = false;
    QString newComment = QInputDialog::getText(this, QString::fromUtf8(u8"设置注释"),
                                               QString::fromUtf8(u8"请输入 %1 的注释：").arg(attributeName),
                                               QLineEdit::Normal, currentComment, &ok);
    if (!ok) return;
    newComment = newComment.trimmed();

    // 更新本地学生注释（按表格维度存储）
    if (!m_selectedExcelTable.isEmpty()) {
        m_student.commentsByExcel[m_selectedExcelTable][attributeName] = newComment;
        m_student.commentsFull[QString("%1_%2").arg(attributeName).arg(m_selectedExcelTable)] = newComment;
    } else {
        m_student.comments[attributeName] = newComment;
    }

    // 更新全局注释缓存（便于其它页面复用）
    if (!m_classId.isEmpty() && !m_examName.isEmpty() && !m_term.isEmpty() && !m_student.id.isEmpty()) {
        const QString fieldKey = m_selectedExcelTable.isEmpty()
            ? attributeName
            : QString("%1_%2").arg(attributeName).arg(m_selectedExcelTable);
        CommentStorage::saveComment(m_classId, m_term, m_student.id, attributeName, m_selectedExcelTable, newComment);
        if (!m_selectedExcelTable.isEmpty()) {
            const QString compositeKey = QString("%1_%2").arg(attributeName).arg(m_selectedExcelTable);
            const QString inferredTable = CommentStorage::inferTableNameFromFieldKey(compositeKey);
            CommentStorage::saveComment(m_classId, m_term, m_student.id, compositeKey, inferredTable, newComment);
        }
    }

    // 发送到服务器：/student-scores/set-comment
    QJsonObject requestObj;
    // 不传 score_header_id：改用 class_id + term (+ excel_filename) 自动定位
    if (!m_classId.trimmed().isEmpty()) requestObj["class_id"] = m_classId.trimmed();
    if (!m_term.trimmed().isEmpty()) requestObj["term"] = m_term.trimmed();
    requestObj["student_name"] = m_student.name;
    if (!m_student.id.isEmpty()) {
        requestObj["student_id"] = m_student.id;
    }
    // 服务器的字段名：如果按表格维度注释，使用复合键名（与后端返回 comments/scores_json_full 一致）
    const QString serverFieldName = m_selectedExcelTable.isEmpty()
        ? attributeName
        : QString("%1_%2").arg(attributeName).arg(m_selectedExcelTable);
    requestObj["field_name"] = serverFieldName;
    if (!m_selectedExcelTable.isEmpty()) {
        requestObj["excel_filename"] = m_selectedExcelTable;
    }
    requestObj["comment"] = newComment;

    QJsonDocument doc(requestObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    QNetworkAccessManager* networkManager = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setUrl(QUrl("http://47.100.126.194:5000/student-scores/set-comment"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = networkManager->post(request, jsonData);
    connect(reply, &QNetworkReply::finished, this, [reply, networkManager]() {
        if (reply->error() == QNetworkReply::NoError) {
            // 成功/失败都不强打断用户，只打日志
            const QByteArray responseData = reply->readAll();
            QJsonDocument respDoc = QJsonDocument::fromJson(responseData);
            if (respDoc.isObject()) {
                QJsonObject obj = respDoc.object();
                int code = obj.value("code").toInt(-1);
                QString msg = obj.value("message").toString();
                qDebug() << "set-comment resp code=" << code << "msg=" << msg;
            } else {
                qDebug() << "set-comment resp:" << responseData;
            }
        } else {
            qDebug() << "set-comment network error:" << reply->errorString();
        }
        reply->deleteLater();
        networkManager->deleteLater();
    });
}

void StudentAttributeDialog::onValueEditingFinished()
{
    // 处理编辑完成
}

void StudentAttributeDialog::clearAllHighlights()
{
    // 清除所有高亮
    for (auto it = m_attributeValueButtons.begin(); it != m_attributeValueButtons.end(); ++it) {
        const QString attrName = it.key();
        if (attrName == QString::fromUtf8(u8"总分")) {
            it.value()->setStyleSheet(
                "QPushButton {"
                "background-color: #4a4a4a;"
                "color: #dddddd;"
                "font-size: 14px;"
                "padding: 8px 16px;"
                "border: none;"
                "border-radius: 4px;"
                "}"
            );
        } else {
            it.value()->setStyleSheet(
                "QPushButton {"
                "background-color: green;"
                "color: white;"
                "font-size: 14px;"
                "padding: 8px 16px;"
                "border: none;"
                "border-radius: 4px;"
                "}"
                "QPushButton:hover {"
                "background-color: #00cc00;"
                "}"
            );
        }
    }
    m_selectedAttribute = "";
}

// 鼠标进入窗口时显示关闭按钮
void StudentAttributeDialog::enterEvent(QEvent* event)
{
    if (m_btnClose) {
        m_btnClose->show();
    }
    QDialog::enterEvent(event);
}

// 鼠标离开窗口时隐藏关闭按钮
void StudentAttributeDialog::leaveEvent(QEvent* event)
{
    // 检查鼠标是否真的离开了窗口（包括关闭按钮）
    QPoint globalPos = QCursor::pos();
    QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
    if (!widgetRect.contains(globalPos) && m_btnClose) {
        // 如果鼠标不在窗口内，检查是否在关闭按钮上
        QRect btnRect = QRect(m_btnClose->mapToGlobal(QPoint(0, 0)), m_btnClose->size());
        if (!btnRect.contains(globalPos)) {
            m_btnClose->hide();
        }
    }
    QDialog::leaveEvent(event);
}

// 窗口大小改变时更新关闭按钮位置
void StudentAttributeDialog::resizeEvent(QResizeEvent* event)
{
    if (m_btnClose) {
        m_btnClose->move(width() - 35, 5);
    }
    QDialog::resizeEvent(event);
}

// 窗口显示时更新关闭按钮位置
void StudentAttributeDialog::showEvent(QShowEvent* event)
{
    if (m_btnClose) {
        m_btnClose->move(width() - 35, 5);
        // 窗口显示时也显示关闭按钮
        m_btnClose->show();
    }
    
    // 确保窗口位置在屏幕可见区域内
    QRect screenGeometry = QApplication::desktop()->availableGeometry();
    QRect windowGeometry = geometry();
    
    // 如果窗口完全在屏幕外，移动到屏幕中央
    if (!screenGeometry.intersects(windowGeometry)) {
        move(screenGeometry.center() - QPoint(windowGeometry.width() / 2, windowGeometry.height() / 2));
    }
    
    // 确保窗口显示在最前面
    raise();
    activateWindow();
    QDialog::showEvent(event);
}

// 事件过滤器，处理关闭按钮的鼠标事件
bool StudentAttributeDialog::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_btnClose) {
        if (event->type() == QEvent::Enter) {
            // 鼠标进入关闭按钮时确保显示
            m_btnClose->show();
        } else if (event->type() == QEvent::Leave) {
            // 鼠标离开关闭按钮时，检查是否还在窗口内
            QPoint globalPos = QCursor::pos();
            QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
            if (!widgetRect.contains(globalPos)) {
                m_btnClose->hide();
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

// 重写鼠标事件以实现窗口拖动
void StudentAttributeDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void StudentAttributeDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton && !m_dragPosition.isNull()) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void StudentAttributeDialog::setTitle(const QString& title)
{
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

