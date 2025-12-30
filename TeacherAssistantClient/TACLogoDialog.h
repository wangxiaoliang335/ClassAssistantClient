#pragma once

#include "TADialog.h"
#include <QLineEdit>
#include <QWidget>
#include <QToolButton>
#include <QHBoxLayout>
#include <QPixmap>
#include <QLabel>
#include <QStringList>

class TACLogoDialog  : public TADialog
{
	Q_OBJECT

public:
	TACLogoDialog(QWidget *parent);
	~TACLogoDialog();
	QString getSchoolName() const; // 保留以兼容旧代码，但返回空字符串
	QString getClassName();
	QString getLogoFileName();
	QString getOtherFileName(); // 保留以兼容旧代码，但返回空字符串（已废弃，使用getHonorIconFileNames）
	QStringList getHonorIconFileNames(); // 返回荣誉图标文件列表（最多3个）
	
	// 设置方法：用于在打开对话框时设置当前值
	void setLogoFileName(const QString& fileName); // 设置学校logo文件名并更新显示
	void setClassName(const QString& className); // 设置班级名称并更新显示
	void setHonorIconFileNames(const QStringList& fileNames); // 设置荣誉图标文件列表并更新显示
	
private slots:
	void onSchoolLogoButtonClicked(); // 学校logo按钮点击
	void onSchoolLogoRemoveClicked(); // 删除学校logo
	void onAddHonorIconClicked(); // 添加荣誉图标
	void onRemoveHonorIcon(int index); // 删除荣誉图标

private:
	void updateSchoolLogoDisplay(); // 更新学校logo显示
	void updateHonorIconsDisplay(); // 更新荣誉图标显示
	void addHonorIconButton(const QString& fileName, int index); // 添加荣誉图标按钮
	
private:
	QLabel* schoolLogoLabel; // 学校logo标签文本
	QWidget* schoolLogoContainer; // 学校logo容器
	QToolButton* schoolLogoButton; // 学校logo按钮（添加或显示）
	QToolButton* schoolLogoRemoveButton; // 删除学校logo按钮
	QLineEdit* classNameEdit; // 班级名称输入框
	QLabel* honorIconLabel; // 荣誉图标标签
	QHBoxLayout* honorIconsLayout; // 荣誉图标布局
	QWidget* honorIconsContainer; // 荣誉图标容器
	
	QString logoFileName; // 学校logo文件名
	QStringList honorIconFileNames; // 荣誉图标文件列表（最多3个）
};
