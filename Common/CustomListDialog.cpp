#include "CustomListDialog.h"
#include "ScheduleDialog.h"
#include <QRegularExpression>
#include <QDebug>

// 比对并更新姓名（以座位表为准）
void CustomListDialog::updateNamesFromSeatInfo(const QStringList& headers, QList<QStringList>& dataRows)
{
    // 获取父窗口（ScheduleDialog）的座位信息列表
    ScheduleDialog* scheduleDlg = qobject_cast<ScheduleDialog*>(parent());
    if (!scheduleDlg) {
        return; // 如果无法获取父窗口，直接返回
    }
    
    QList<SeatInfo> seatInfoList = scheduleDlg->getSeatInfoList();
    if (seatInfoList.isEmpty()) {
        return; // 如果座位信息列表为空，直接返回
    }
    
    // 查找"学号"和"姓名"列的索引
    int studentIdCol = headers.indexOf("学号");
    int nameCol = headers.indexOf("姓名");
    
    if (studentIdCol == -1 || nameCol == -1) {
        return; // 如果找不到"学号"或"姓名"列，直接返回
    }
    
    // 创建学号到姓名的映射（从座位信息列表中）
    QMap<QString, QString> seatIdToNameMap;
    for (const SeatInfo& seatInfo : seatInfoList) {
        if (seatInfo.studentId.isEmpty()) {
            continue;
        }
        
        // 从 seatLabel 或 studentName 中提取纯姓名（去掉数字部分）
        QString pureName = extractPureName(seatInfo.seatLabel.isEmpty() ? seatInfo.studentName : seatInfo.seatLabel);
        if (!pureName.isEmpty()) {
            seatIdToNameMap[seatInfo.studentId] = pureName;
        }
    }
    
    // 遍历 dataRows，比对并更新姓名
    int updatedCount = 0;
    for (QStringList& row : dataRows) {
        if (row.size() <= qMax(studentIdCol, nameCol)) {
            continue; // 行数据不完整，跳过
        }
        
        QString studentId = row[studentIdCol].trimmed();
        QString currentName = row[nameCol].trimmed();
        
        if (studentId.isEmpty()) {
            continue; // 学号为空，跳过
        }
        
        // 查找座位信息中对应的姓名
        if (seatIdToNameMap.contains(studentId)) {
            QString seatName = seatIdToNameMap[studentId];
            // 以导入的 Excel 姓名为准，仅当 Excel 里为空时才用座位表补全
            if (currentName.isEmpty() && !seatName.isEmpty()) {
                row[nameCol] = seatName;
                updatedCount++;
            }
        }
    }
    
    if (updatedCount > 0) {
        qDebug() << "已根据座位表更新" << updatedCount << "个学生的姓名";
    }
}

// 从字符串中提取纯姓名（去掉数字部分）
// 例如："姜凯文13-5/14" -> "姜凯文"
QString CustomListDialog::extractPureName(const QString& nameStr)
{
    if (nameStr.isEmpty()) {
        return QString();
    }
    
    QString trimmed = nameStr.trimmed();
    
    // 如果包含"/"，提取"/"之前的部分
    int slashIndex = trimmed.lastIndexOf('/');
    if (slashIndex != -1) {
        trimmed = trimmed.left(slashIndex).trimmed();
    }
    
    // 使用正则表达式提取姓名（去除末尾的数字和"-"等）
    // 例如："姜凯文13-5" -> "姜凯文"
    QRegularExpression nameRegex(R"(^(.+?)\d)");
    QRegularExpressionMatch match = nameRegex.match(trimmed);
    if (match.hasMatch()) {
        QString pureName = match.captured(1).trimmed();
        // 去除末尾的连字符和空格
        while (pureName.endsWith('-') || pureName.endsWith(' ')) {
            pureName = pureName.left(pureName.length() - 1).trimmed();
        }
        return pureName;
    }
    
    // 如果没有匹配到数字，尝试匹配其他模式
    QRegularExpression altRegex(R"(^(.+?)[\d\-]+$)");
    QRegularExpressionMatch altMatch = altRegex.match(trimmed);
    if (altMatch.hasMatch()) {
        QString pureName = altMatch.captured(1).trimmed();
        while (pureName.endsWith('-') || pureName.endsWith(' ')) {
            pureName = pureName.left(pureName.length() - 1).trimmed();
        }
        return pureName;
    }
    
    // 如果都不匹配，直接返回原字符串（去除末尾的数字和符号）
    QString result = trimmed;
    while (result.length() > 0 && (result.endsWith('-') || result.endsWith(' ') || result.at(result.length() - 1).isDigit())) {
        result = result.left(result.length() - 1).trimmed();
    }
    return result;
}

