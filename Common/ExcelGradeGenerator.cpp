#include "ExcelGradeGenerator.h"
#include "QXlsx/header/xlsxdocument.h"
#include "QXlsx/header/xlsxformat.h"
#include "QXlsx/header/xlsxcellrange.h"
#include <QRegularExpression>
#include <QDebug>
#include <QRandomGenerator>
#include <QDateTime>

using namespace QXlsx;

ExcelGradeGenerator::ExcelGradeGenerator()
{
}

bool ExcelGradeGenerator::readSeatingChart(const QString& excelPath, QList<StudentInfo>& students)
{
    Document doc(excelPath);
    if (!doc.load()) {
        qWarning() << "无法打开Excel文件:" << excelPath;
        return false;
    }
    
    // 确保文件已加载
    if (!doc.isLoadPackage()) {
        qWarning() << "Excel文件加载失败:" << excelPath;
        return false;
    }
    
    students.clear();
    
    // 获取工作表
    QStringList sheetNames = doc.sheetNames();
    if (sheetNames.isEmpty()) {
        qWarning() << "Excel文件没有工作表";
        return false;
    }
    
    // 读取第一个工作表
    doc.selectSheet(sheetNames.first());
    
    // 获取数据范围
    CellRange range = doc.dimension();
    if (range.rowCount() == 0 || range.columnCount() == 0) {
        qWarning() << "Excel文件为空";
        return false;
    }
    
    // 遍历所有单元格，查找学生信息
    for (int row = 1; row <= range.rowCount(); ++row) {
        for (int col = 1; col <= range.columnCount(); ++col) {
            QVariant cellValue = doc.read(row, col);
            if (cellValue.isNull() || !cellValue.isValid()) {
                continue;
            }
            
            QString cellStr = cellValue.toString().trimmed();
            if (cellStr.isEmpty()) {
                continue;
            }
            
            // 尝试解析学生信息
            StudentInfo info;
            if (parseStudentString(cellStr, info)) {
                // 检查是否已存在该学号
                bool exists = false;
                for (const StudentInfo& existing : students) {
                    if (existing.studentId == info.studentId) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    students.append(info);
                }
            }
        }
    }
    
    // 按学号排序
    std::sort(students.begin(), students.end(), 
              [](const StudentInfo& a, const StudentInfo& b) {
                  return a.studentId < b.studentId;
              });
    
    qDebug() << "成功读取" << students.size() << "个学生信息";
    return true;
}

bool ExcelGradeGenerator::parseStudentString(const QString& str, StudentInfo& info)
{
    // 格式示例："姜凯文13-5/14"
    // 学号在最后一个"/"之后
    // 姓名在学号之前
    
    // 查找最后一个"/"
    int lastSlashIndex = str.lastIndexOf('/');
    if (lastSlashIndex == -1 || lastSlashIndex >= str.length() - 1) {
        return false;
    }
    
    // 提取学号（最后一个"/"之后的数字）
    QString idStr = str.mid(lastSlashIndex + 1).trimmed();
    bool ok = false;
    int studentId = idStr.toInt(&ok);
    if (!ok || studentId <= 0) {
        return false;
    }
    
    // 提取姓名（最后一个"/"之前的部分，去除末尾的数字和"-"）
    QString namePart = str.left(lastSlashIndex).trimmed();
    
    // 使用正则表达式提取姓名（去除末尾的数字和"-"等）
    // 例如："姜凯文13-5" -> "姜凯文"
    QRegularExpression nameRegex(R"(^(.+?)\d)");
    QRegularExpressionMatch match = nameRegex.match(namePart);
    if (match.hasMatch()) {
        info.name = match.captured(1).trimmed();
    } else {
        // 如果没有匹配到，尝试直接使用整个字符串（去除末尾的数字和符号）
        QRegularExpression cleanRegex(R"(^(.+?)[\d\-]+$)");
        QRegularExpressionMatch cleanMatch = cleanRegex.match(namePart);
        if (cleanMatch.hasMatch()) {
            info.name = cleanMatch.captured(1).trimmed();
        } else {
            info.name = namePart;
        }
    }
    
    // 如果姓名仍然包含数字或特殊字符，尝试更精确的提取
    if (info.name.contains(QRegularExpression(R"(\d)"))) {
        // 再次尝试：找到第一个数字的位置，之前的部分就是姓名
        QRegularExpression firstDigitRegex(R"((.+?)\d)");
        QRegularExpressionMatch firstDigitMatch = firstDigitRegex.match(namePart);
        if (firstDigitMatch.hasMatch()) {
            info.name = firstDigitMatch.captured(1).trimmed();
        }
    }
    
    info.name = info.name.trimmed();
    if (info.name.isEmpty()) {
        return false;
    }
    
    info.studentId = studentId;
    return true;
}

bool ExcelGradeGenerator::generateMidtermGradeSheet(const QString& outputPath, const QList<StudentInfo>& students)
{
    if (students.isEmpty()) {
        qWarning() << "学生列表为空";
        return false;
    }
    
    Document doc;
    
    // 设置随机数种子
    QRandomGenerator::global()->seed(QDateTime::currentMSecsSinceEpoch());
    
    // 写入表头
    Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setFontColor(QColor(Qt::white));
    headerFormat.setPatternBackgroundColor(QColor(128, 128, 128)); // #808080
    headerFormat.setHorizontalAlignment(Format::AlignHCenter);
    headerFormat.setVerticalAlignment(Format::AlignVCenter);
    headerFormat.setBorderStyle(Format::BorderThin);
    headerFormat.setBorderColor(QColor(221, 221, 221)); // #ddd
    
    doc.write(1, 1, "学号", headerFormat);
    doc.write(1, 2, "姓名", headerFormat);
    doc.write(1, 3, "总分", headerFormat);
    doc.write(1, 4, "语文", headerFormat);
    doc.write(1, 5, "数学", headerFormat);
    doc.write(1, 6, "英语", headerFormat);
    
    // 设置列宽
    doc.setColumnWidth(1, 12); // 学号
    doc.setColumnWidth(2, 15); // 姓名
    doc.setColumnWidth(3, 12); // 总分
    doc.setColumnWidth(4, 12); // 语文
    doc.setColumnWidth(5, 12); // 数学
    doc.setColumnWidth(6, 12); // 英语
    
    // 数据格式
    Format dataFormat;
    dataFormat.setHorizontalAlignment(Format::AlignHCenter);
    dataFormat.setVerticalAlignment(Format::AlignVCenter);
    dataFormat.setBorderStyle(Format::BorderThin);
    dataFormat.setBorderColor(QColor(221, 221, 221));
    
    Format altDataFormat = dataFormat;
    altDataFormat.setPatternBackgroundColor(QColor(230, 243, 255)); // #e6f3ff
    
    // 写入学生数据
    for (int i = 0; i < students.size(); ++i) {
        const StudentInfo& student = students[i];
        int row = i + 2; // 从第2行开始（第1行是表头）
        
        // 随机生成成绩（60-150之间，保留1位小数）
        double chinese = 60.0 + (QRandomGenerator::global()->bounded(900)) / 10.0; // 60.0-150.0
        double math = 60.0 + (QRandomGenerator::global()->bounded(900)) / 10.0;
        double english = 60.0 + (QRandomGenerator::global()->bounded(900)) / 10.0;
        double total = chinese + math + english;
        
        // 使用交替行颜色
        Format rowFormat = (row % 2 == 0) ? dataFormat : altDataFormat;
        
        doc.write(row, 1, student.studentId, rowFormat);
        doc.write(row, 2, student.name, rowFormat);
        doc.write(row, 3, QString::number(total, 'f', 1), rowFormat);
        doc.write(row, 4, QString::number(chinese, 'f', 1), rowFormat);
        doc.write(row, 5, QString::number(math, 'f', 1), rowFormat);
        doc.write(row, 6, QString::number(english, 'f', 1), rowFormat);
    }
    
    // 设置行高
    doc.setRowHeight(1, 25); // 表头行高
    for (int i = 2; i <= students.size() + 1; ++i) {
        doc.setRowHeight(i, 20);
    }
    
    // 保存文件
    if (!doc.saveAs(outputPath)) {
        qWarning() << "无法保存Excel文件:" << outputPath;
        return false;
    }
    
    qDebug() << "成功生成期中成绩表:" << outputPath;
    return true;
}

double ExcelGradeGenerator::generateRandomScore() const
{
    // 生成60-150之间的随机分数，保留1位小数
    return 60.0 + (QRandomGenerator::global()->bounded(900)) / 10.0;
}

void ExcelGradeGenerator::applyTableStyle(Document& doc, int rowCount) const
{
    // 样式已在generateMidtermGradeSheet中应用
    Q_UNUSED(doc);
    Q_UNUSED(rowCount);
}

