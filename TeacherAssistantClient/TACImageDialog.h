#pragma once

#include "TADialog.h"
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QFileDialog>
#include <QPainter>
#include <QPaintEvent>

// 图片显示组件（自定义Widget用于绘制图片）
class ImageDisplayWidget : public QWidget
{
    // 不需要 Q_OBJECT，因为没有信号和槽

public:
    ImageDisplayWidget(QWidget* parent = nullptr);
    void setImage(const QPixmap& pixmap);
    void setStretchMode(int mode); // 0-原始，1-左右拉伸，2-上下拉伸

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap m_pixmap;
    int m_stretchMode;
};

class TACImageDialog : public TADialog
{
    Q_OBJECT

public:
    TACImageDialog(QWidget *parent);
    ~TACImageDialog();

private slots:
    void onSelectImage(); // 选择图片
    void onStretchHorizontal(); // 左右拉伸
    void onStretchVertical(); // 上下拉伸
    void onSelectBackgroundImage(); // 选择背景图片

private:
    // 图片组件部分
    ImageDisplayWidget* imageWidget; // 图片显示区域
    QPushButton* selectImageButton; // 选择图片按钮
    QPushButton* stretchHorizontalButton; // 左右拉伸按钮
    QPushButton* stretchVerticalButton; // 上下拉伸按钮
    QPixmap originalImage; // 原始图片
    QString imageFilePath; // 图片文件路径
    int stretchMode; // 拉伸模式：0-原始，1-左右拉伸，2-上下拉伸
    
    // 班规班训模版部分
    QWidget* templateWidget; // 模版容器
    QLabel* backgroundLabel; // 背景图显示
    QLineEdit* singleLineTextEdit; // 单行文本
    QTextEdit* multiLineTextEdit; // 多行文本
    QPushButton* backgroundImageButton; // 背景图片按钮
    QString backgroundImagePath; // 背景图片路径
};

