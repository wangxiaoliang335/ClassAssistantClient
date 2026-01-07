#include "HomeworkEditDialog.h"
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>
#include <QDate>

HomeworkEditDialog::HomeworkEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setStyleSheet("background-color: #2b2b2b; color: white;");
    resize(500, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 顶部栏：关闭按钮
    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->addStretch();
    
    QPushButton* btnCloseTop = new QPushButton("✕");
    btnCloseTop->setFixedSize(30, 30);
    btnCloseTop->setStyleSheet(
        "QPushButton {"
        "background-color: #666666;"
        "color: white;"
        "font-size: 18px;"
        "font-weight: bold;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #777777;"
        "}"
    );
    connect(btnCloseTop, &QPushButton::clicked, this, &HomeworkEditDialog::onCloseClicked);
    topLayout->addWidget(btnCloseTop);
    mainLayout->addLayout(topLayout);

    // 标题："家庭作业"
    QLabel* titleLabel = new QLabel("家庭作业");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 20px;"
        "font-weight: bold;"
        "color: white;"
        "padding: 10px;"
    );
    mainLayout->addWidget(titleLabel);

    // 日期显示
    dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEdit->setStyleSheet(
        "QDateEdit {"
        "background-color: #3b3b3b;"
        "color: white;"
        "border: 1px solid #555;"
        "padding: 8px;"
        "border-radius: 4px;"
        "font-size: 14px;"
        "}"
        "QDateEdit::drop-down {"
        "border: none;"
        "}"
    );
    mainLayout->addWidget(dateEdit);

    // 选择科目
    subjectCombo = new QComboBox(this);
    subjectCombo->setStyleSheet(
        "QComboBox {"
        "background-color: #3b3b3b;"
        "color: white;"
        "border: 1px solid #555;"
        "padding: 6px 10px;"
        "border-radius: 4px;"
        "font-size: 14px;"
        "}"
        "QComboBox QAbstractItemView {"
        "background-color: #3b3b3b;"
        "color: white;"
        "selection-background-color: #0052a3;"
        "}"
    );
    mainLayout->addWidget(subjectCombo);

    // 编辑区（根据科目切换）
    editorStack = new QStackedWidget(this);
    mainLayout->addWidget(editorStack, 1);

    connect(subjectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HomeworkEditDialog::onSubjectChanged);

    // 默认科目（若外部未传入）
    setAvailableSubjects(QStringList() << QString::fromUtf8(u8"语文") << QString::fromUtf8(u8"数学") << QString::fromUtf8(u8"英语"));
    mainLayout->addStretch();

    // 发布按钮
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    
    btnPublish = new QPushButton("发布");
    btnPublish->setStyleSheet(
        "QPushButton {"
        "background-color: #0066cc;"
        "color: white;"
        "font-size: 16px;"
        "padding: 10px 30px;"
        "border: none;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #0052a3;"
        "}"
    );
    btnPublish->setFixedHeight(40);
    connect(btnPublish, &QPushButton::clicked, this, &HomeworkEditDialog::onPublishClicked);
    btnLayout->addWidget(btnPublish);
    mainLayout->addLayout(btnLayout);
}

void HomeworkEditDialog::setAvailableSubjects(const QStringList& subjects)
{
    // 保留已有内容（按科目名）
    QMap<QString, QString> existing;
    for (auto it = subjectEdits.begin(); it != subjectEdits.end(); ++it) {
        existing[it.key()] = it.value()->toPlainText();
    }

    subjectEdits.clear();

    if (subjectCombo) {
        subjectCombo->blockSignals(true);
        subjectCombo->clear();
    }
    if (editorStack) {
        while (editorStack->count() > 0) {
            QWidget* w = editorStack->widget(0);
            editorStack->removeWidget(w);
            w->deleteLater();
        }
    }

    const QString editorStyle =
        "QTextEdit {"
        "background-color: #3b3b3b;"
        "color: white;"
        "border: 1px solid #555;"
        "border-radius: 4px;"
        "padding: 8px;"
        "font-size: 14px;"
        "min-height: 280px;"
        "}";

    QStringList cleaned;
    for (const auto& s : subjects) {
        const QString t = s.trimmed();
        if (!t.isEmpty() && !cleaned.contains(t)) cleaned.append(t);
    }
    if (cleaned.isEmpty()) {
        cleaned << QString::fromUtf8(u8"语文") << QString::fromUtf8(u8"数学") << QString::fromUtf8(u8"英语");
    }

    for (const auto& subject : cleaned) {
        if (subjectCombo) subjectCombo->addItem(subject);

        QTextEdit* edit = new QTextEdit(this);
        edit->setPlaceholderText(QString::fromUtf8(u8"请输入%1作业内容...").arg(subject));
        edit->setStyleSheet(editorStyle);
        edit->setPlainText(existing.value(subject));

        subjectEdits[subject] = edit;
        if (editorStack) editorStack->addWidget(edit);
    }

    if (subjectCombo) {
        subjectCombo->setCurrentIndex(0);
        subjectCombo->blockSignals(false);
    }
    if (editorStack) {
        editorStack->setCurrentIndex(0);
    }
}

void HomeworkEditDialog::setDate(const QDate& date)
{
    if (dateEdit) {
        dateEdit->setDate(date);
    }
}

void HomeworkEditDialog::setHomeworkContent(const QMap<QString, QString>& content)
{
    for (auto it = content.begin(); it != content.end(); ++it) {
        if (subjectEdits.contains(it.key())) {
            subjectEdits[it.key()]->setPlainText(it.value());
        }
    }
}

void HomeworkEditDialog::onSubjectChanged(int idx)
{
    if (editorStack && idx >= 0 && idx < editorStack->count()) {
        editorStack->setCurrentIndex(idx);
    }
}

void HomeworkEditDialog::onPublishClicked()
{
    const QString subject = subjectCombo ? subjectCombo->currentText().trimmed() : QString();
    if (subject.isEmpty() || !subjectEdits.contains(subject)) {
        QMessageBox::warning(this, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"请选择科目！"));
        return;
    }

    const QString text = subjectEdits[subject]->toPlainText().trimmed();
    if (text.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"请输入作业内容！"));
        return;
    }

    // 新信号：只发布当前选择的科目
    emit homeworkPublishedSingle(dateEdit->date(), subject, text);
    accept();
}

void HomeworkEditDialog::onCloseClicked()
{
    reject();
}

void HomeworkEditDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

void HomeworkEditDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void HomeworkEditDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
        return;
    }
    QDialog::mouseReleaseEvent(event);
}

