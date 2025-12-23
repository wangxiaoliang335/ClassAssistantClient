// 生成期中成绩表的工具程序
// 使用方法：在main函数中调用，或创建一个对话框按钮来调用

#include "ExcelGradeGenerator.h"
#include <QDebug>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>

// 生成期中成绩表的函数
bool generateMidtermGradeSheetFromSeatingChart(const QString& seatingChartPath, const QString& outputPath)
{
    ExcelGradeGenerator generator;
    
    // 检查输入文件是否存在
    if (!QFileInfo::exists(seatingChartPath)) {
        qWarning() << "座位表文件不存在:" << seatingChartPath;
        return false;
    }
    
    // 读取座位表
    QList<StudentInfo> students;
    if (!generator.readSeatingChart(seatingChartPath, students)) {
        qWarning() << "读取座位表失败";
        return false;
    }
    
    if (students.isEmpty()) {
        qWarning() << "座位表中没有找到学生信息";
        return false;
    }
    
    qDebug() << "成功读取" << students.size() << "个学生信息";
    
    // 确保输出目录存在
    QFileInfo outputInfo(outputPath);
    QDir outputDir = outputInfo.absoluteDir();
    if (!outputDir.exists()) {
        if (!outputDir.mkpath(".")) {
            qWarning() << "无法创建输出目录:" << outputDir.absolutePath();
            return false;
        }
    }
    
    // 生成期中成绩表
    if (!generator.generateMidtermGradeSheet(outputPath, students)) {
        qWarning() << "生成期中成绩表失败";
        return false;
    }
    
    qDebug() << "成功生成期中成绩表:" << outputPath;
    return true;
}

