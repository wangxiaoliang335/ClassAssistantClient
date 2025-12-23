#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QPair>

struct StudentInfo {
    int studentId;      // 学号
    QString name;       // 姓名
};

struct GradeInfo {
    int studentId;      // 学号
    QString name;       // 姓名
    double chinese;     // 语文
    double math;        // 数学
    double english;     // 英语
    double total;       // 总分
};

class ExcelGradeGenerator
{
public:
    ExcelGradeGenerator();
    
    // 从座位表Excel读取学生信息
    bool readSeatingChart(const QString& excelPath, QList<StudentInfo>& students);
    
    // 生成期中成绩表Excel
    bool generateMidtermGradeSheet(const QString& outputPath, const QList<StudentInfo>& students);
    
    // 解析学生信息字符串，例如"姜凯文13-5/14" -> 学号14, 姓名"姜凯文"
    static bool parseStudentString(const QString& str, StudentInfo& info);

private:
    // 随机生成成绩（0-150之间的随机数，保留1位小数）
    double generateRandomScore() const;
    
    // 设置表格样式
    void applyTableStyle(class QXlsx::Document& doc, int rowCount) const;
};

