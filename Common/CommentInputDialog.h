#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QMouseEvent>
#include <QEvent>
#include <QResizeEvent>
#include <QPoint>
#include <QString>

// 自定义注释输入对话框
class CommentInputDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CommentInputDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        // 去掉标题栏
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setModal(true);
        resize(500, 300);
        
        // 设置窗口背景色和圆角
        setStyleSheet(
            "QDialog { background-color: #565656; border-radius: 20px; } "
            "QLabel { background-color: #565656; color: #f5f5f5; } "
            "QTextEdit { background-color: #454545; color: #f5f5f5; border-radius: 15px; padding: 6px; } "
            "QPushButton { background-color: #454545; color: #f5f5f5; border-radius: 15px; } "
        );
        
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);
        
        // 标题标签
        m_titleLabel = new QLabel(QStringLiteral("单元格注释"), this);
        m_titleLabel->setStyleSheet("background-color: #565656; color: #f5f5f5; font-size: 18px; font-weight: bold;");
        m_titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(m_titleLabel);
        
        // 提示标签
        m_promptLabel = new QLabel("", this);
        m_promptLabel->setStyleSheet("background-color: #565656; color: #f5f5f5; font-size: 14px;");
        m_promptLabel->setWordWrap(true);
        mainLayout->addWidget(m_promptLabel);
        
        // 多行文本输入框
        m_textEdit = new QTextEdit(this);
        m_textEdit->setStyleSheet("background-color: #454545; color: #f5f5f5; border-radius: 15px; padding: 6px;");
        m_textEdit->setMinimumHeight(120);
        mainLayout->addWidget(m_textEdit);
        
        // 按钮布局
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        
        m_okButton = new QPushButton(QStringLiteral("确定"), this);
        m_okButton->setFixedSize(80, 35);
        m_okButton->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 6px 12px; border-radius: 15px; font-weight: bold;");
        connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
        buttonLayout->addWidget(m_okButton);
        
        m_cancelButton = new QPushButton(QStringLiteral("取消"), this);
        m_cancelButton->setFixedSize(80, 35);
        m_cancelButton->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 6px 12px; border-radius: 15px; font-weight: bold;");
        connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        buttonLayout->addWidget(m_cancelButton);
        
        buttonLayout->addStretch();
        mainLayout->addLayout(buttonLayout);
        
        // 关闭按钮（鼠标移入显示）
        m_closeButton = new QPushButton(QStringLiteral("×"), this);
        m_closeButton->setFixedSize(30, 30);
        m_closeButton->setStyleSheet(
            "QPushButton { "
            "background-color: #707070; "
            "color: #f5f5f5; "
            "border: none; "
            "border-radius: 15px; "
            "font-size: 20px; "
            "font-weight: bold; "
            "} "
            "QPushButton:hover { "
            "background-color: #ff4444; "
            "}"
        );
        m_closeButton->move(width() - 35, 5);
        m_closeButton->hide();
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
        
        // 初始化拖动
        m_dragging = false;
        m_dragStartPos = QPoint();
    }
    
    void setTitle(const QString& title) {
        if (m_titleLabel) {
            m_titleLabel->setText(title);
        }
    }
    
    void setPrompt(const QString& prompt) {
        if (m_promptLabel) {
            m_promptLabel->setText(prompt);
        }
    }
    
    void setText(const QString& text) {
        if (m_textEdit) {
            m_textEdit->setPlainText(text);
        }
    }
    
    QString text() const {
        return m_textEdit ? m_textEdit->toPlainText() : QString();
    }
    
    static QString getMultiLineText(QWidget* parent, const QString& title, const QString& prompt, 
                                   const QString& defaultValue = "", bool* ok = nullptr)
    {
        CommentInputDialog dlg(parent);
        dlg.setTitle(title);
        dlg.setPrompt(prompt);
        dlg.setText(defaultValue);
        
        int result = dlg.exec();
        if (ok) {
            *ok = (result == QDialog::Accepted);
        }
        
        return result == QDialog::Accepted ? dlg.text() : QString();
    }
    
protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    
    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
            event->accept();
        }
    }
    
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            event->accept();
        }
    }
    
    void enterEvent(QEvent* event) override {
        if (m_closeButton) {
            m_closeButton->show();
        }
        QDialog::enterEvent(event);
    }
    
    void leaveEvent(QEvent* event) override {
        if (m_closeButton) {
            m_closeButton->hide();
        }
        QDialog::leaveEvent(event);
    }
    
    void resizeEvent(QResizeEvent* event) override {
        QDialog::resizeEvent(event);
        // 更新关闭按钮位置
        if (m_closeButton) {
            m_closeButton->move(width() - 35, 5);
        }
    }
    
private:
    QLabel* m_titleLabel = nullptr;
    QLabel* m_promptLabel = nullptr;
    QTextEdit* m_textEdit = nullptr;
    QPushButton* m_okButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

