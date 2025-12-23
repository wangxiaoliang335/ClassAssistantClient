// 测试程序：生成期中成绩表
// 可以直接在main函数中调用，或创建一个对话框按钮来调用

#include "ExcelGradeGenerator.h"
#include "GenerateGradeSheet.cpp"
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>

// 测试函数：生成期中成绩表
void testGenerateGradeSheet()
{
    // 输入文件路径
    QString seatingChartPath = "E:\\Agreement\\座次表9.20(1).xlsx";
    
    // 输出文件路径
    QString outputPath = "E:\\Agreement\\期中成绩表.xlsx";
    
    // 生成期中成绩表
    if (generateMidtermGradeSheetFromSeatingChart(seatingChartPath, outputPath)) {
        QMessageBox::information(nullptr, "成功", 
            QString("期中成绩表已生成！\n输出路径：%1").arg(outputPath));
    } else {
        QMessageBox::warning(nullptr, "失败", 
            "生成期中成绩表失败，请检查日志。\n确保座位表文件存在且格式正确。");
    }
}

// 如果需要交互式选择文件，可以使用这个函数
void testGenerateGradeSheetWithDialog()
{
    QApplication* app = qApp;
    if (!app) {
        qWarning() << "QApplication未初始化";
        return;
    }
    
    // 选择输入文件
    QString seatingChartPath = QFileDialog::getOpenFileName(
        nullptr,
        "选择座位表Excel文件",
        "E:\\Agreement",
        "Excel Files (*.xlsx *.xls)"
    );
    
    if (seatingChartPath.isEmpty()) {
        return;
    }
    
    // 选择输出文件
    QString outputPath = QFileDialog::getSaveFileName(
        nullptr,
        "保存期中成绩表",
        "E:\\Agreement\\期中成绩表.xlsx",
        "Excel Files (*.xlsx)"
    );
    
    if (outputPath.isEmpty()) {
        return;
    }
    
    // 生成期中成绩表
    if (generateMidtermGradeSheetFromSeatingChart(seatingChartPath, outputPath)) {
        QMessageBox::information(nullptr, "成功", 
            QString("期中成绩表已生成！\n输出路径：%1").arg(outputPath));
    } else {
        QMessageBox::warning(nullptr, "失败", 
            "生成期中成绩表失败，请检查日志。");
    }
}

