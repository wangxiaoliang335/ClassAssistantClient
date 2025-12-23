#pragma once
#include "TABaseDialog.h"
#include <QStringList>

struct Notification {
    int id;
    int sender_id;
    QString sender_name;
    int receiver_id;
    QString unique_group_id;
    QString group_name;
    QString content;
    int content_text;
    int is_read;
    int is_agreed;
    QString remark;
    QString created_at;
    QString updated_at;
};

struct GroupMemberInfo {
    QString member_id;
    QString member_name;
    QString member_role;
    bool is_voice_enabled;  // 是否开启语音
    QStringList teach_subjects; // 任教科目（来自 /groups/members 的 teach_subjects 字段）
};

// StudentInfo 结构体在 ScheduleDialog.h 中定义，这里使用条件编译避免重复定义
//#ifndef STUDENT_INFO_DEFINED
//#define STUDENT_INFO_DEFINED
struct StudentInfo {
    QString id;      // 学号
    QString name;    // 姓名
    QString groupName; // 小组名称
    double score;    // 成绩（用于排序）
    double groupTotalScore = 0.0; // 小组总分
    int originalIndex; // 原始索引
    QMap<QString, double> attributes; // 多个属性值（如"背诵"、"语文"等）- 向后兼容，取第一个值
    QMap<QString, QString> comments; // 字段注释（如"数学" -> "需要加强练习"）- 向后兼容
    // 新增：按Excel文件名组织的字段映射（支持不同Excel文件的相同字段名）
    QMap<QString, QMap<QString, double>> attributesByExcel; // Excel文件名 -> (字段名 -> 值)
    QMap<QString, QMap<QString, QString>> commentsByExcel; // Excel文件名 -> (字段名 -> 注释)
    // 新增：完整的复合键名映射（字段名_Excel文件名 -> 值）
    QMap<QString, double> attributesFull; // 复合键名（如"语文_期中成绩单.xlsx" -> 值）
    QMap<QString, QString> commentsFull; // 复合键名注释（如"语文_期中成绩单.xlsx" -> 注释）
    
    // 辅助函数：获取属性值（优先级：attributesByExcel → attributesFull → attributes）
    // excelFileName: 如果指定，优先从该Excel文件中获取；如果为空，从所有Excel文件中获取第一个值
    double getAttributeValue(const QString& attributeName, const QString& excelFileName = QString()) const {
        // 1. 优先从 attributesByExcel 中获取（如果指定了Excel文件名）
        if (!excelFileName.isEmpty() && attributesByExcel.contains(excelFileName)) {
            const QMap<QString, double>& excelAttrs = attributesByExcel[excelFileName];
            if (excelAttrs.contains(attributeName)) {
                return excelAttrs[attributeName];
            }
        }
        
        // 2. 如果没有指定Excel文件名，遍历所有Excel文件找到第一个有该字段的值
        if (excelFileName.isEmpty()) {
            for (auto it = attributesByExcel.begin(); it != attributesByExcel.end(); ++it) {
                const QMap<QString, double>& excelAttrs = it.value();
                if (excelAttrs.contains(attributeName)) {
                    return excelAttrs[attributeName];
                }
            }
        }
        
        // 3. 从 attributesFull 中获取（复合键名）
        if (!excelFileName.isEmpty()) {
            QString compositeKey = QString("%1_%2").arg(attributeName).arg(excelFileName);
            if (attributesFull.contains(compositeKey)) {
                return attributesFull[compositeKey];
            }
        } else {
            // 遍历所有复合键名，找到第一个匹配的
            for (auto it = attributesFull.begin(); it != attributesFull.end(); ++it) {
                QString key = it.key();
                int underscorePos = key.lastIndexOf('_');
                if (underscorePos > 0) {
                    QString fieldName = key.left(underscorePos);
                    if (fieldName == attributeName) {
                        return it.value();
                    }
                }
            }
        }
        
        // 4. 向后兼容：从 attributes 中获取（第一个值）
        if (attributes.contains(attributeName)) {
            return attributes[attributeName];
        }
        
        return 0.0;
    }
    
    // 辅助函数：获取注释（优先级：commentsByExcel → commentsFull → comments）
    QString getComment(const QString& attributeName, const QString& excelFileName = QString()) const {
        // 1. 优先从 commentsByExcel 中获取（如果指定了Excel文件名）
        if (!excelFileName.isEmpty() && commentsByExcel.contains(excelFileName)) {
            const QMap<QString, QString>& excelComments = commentsByExcel[excelFileName];
            if (excelComments.contains(attributeName)) {
                return excelComments[attributeName];
            }
        }
        
        // 2. 如果没有指定Excel文件名，遍历所有Excel文件找到第一个有该字段的注释
        if (excelFileName.isEmpty()) {
            for (auto it = commentsByExcel.begin(); it != commentsByExcel.end(); ++it) {
                const QMap<QString, QString>& excelComments = it.value();
                if (excelComments.contains(attributeName)) {
                    return excelComments[attributeName];
                }
            }
        }
        
        // 3. 从 commentsFull 中获取（复合键名）
        if (!excelFileName.isEmpty()) {
            QString compositeKey = QString("%1_%2").arg(attributeName).arg(excelFileName);
            if (commentsFull.contains(compositeKey)) {
                return commentsFull[compositeKey];
            }
        } else {
            // 遍历所有复合键名，找到第一个匹配的
            for (auto it = commentsFull.begin(); it != commentsFull.end(); ++it) {
                QString key = it.key();
                int underscorePos = key.lastIndexOf('_');
                if (underscorePos > 0) {
                    QString fieldName = key.left(underscorePos);
                    if (fieldName == attributeName) {
                        return it.value();
                    }
                }
            }
        }
        
        // 4. 向后兼容：从 comments 中获取（第一个值）
        if (comments.contains(attributeName)) {
            return comments[attributeName];
        }
        
        return QString();
    }
};
//#endif


// 班级端登录信息结构体
struct ClassLoginInfo {
    QString class_id;       // 班级ID（使用班级编号class_code作为class_id）
    QString class_code;     // 班级唯一编号
    QString class_name;     // 班级名称
    QString school_stage;    // 学段（如"小学"、"初中"、"高中"）
    QString grade;          // 年级（如"一年级"、"二年级"）
    QString schoolid;       // 学校ID
    QString face_url;       // 班级头像URL（GET /api/class/info 的 face_url）
    QString access_token;   // JWT token
    QString token_type;     // token类型（通常是"bearer"）
    
    // 清空信息
    void clear() {
        class_id.clear();
        class_code.clear();
        class_name.clear();
        school_stage.clear();
        grade.clear();
        schoolid.clear();
        face_url.clear();
        access_token.clear();
        token_type.clear();
    }
    
    // 检查是否已登录
    bool isLoggedIn() const {
        return !class_id.isEmpty() && !access_token.isEmpty();
    }
    
    // 兼容旧代码：获取group_id（返回class_code）
    QString group_id() const {
        return class_code;
    }
    
    // 兼容旧代码：获取group_name（返回class_name）
    QString group_name() const {
        return class_name;
    }
};

class CommonInfo
{
public:
	CommonInfo()
	{

	}

    static void InitData(UserInfo userInfo)
    {
        m_userInfo = userInfo;
    }

    static UserInfo GetData()
    {
        return m_userInfo;
    }

    // 班级端登录信息相关方法
    static void InitClassLoginInfo(const ClassLoginInfo& loginInfo)
    {
        m_classLoginInfo = loginInfo;
    }

    static ClassLoginInfo GetClassLoginInfo()
    {
        return m_classLoginInfo;
    }

    static void ClearClassLoginInfo()
    {
        m_classLoginInfo.clear();
    }

private:
    static UserInfo m_userInfo;
    static ClassLoginInfo m_classLoginInfo;
};

// 这里必须定义静态成员变量一次，否则会链接错误
//UserInfo CommonInfo::m_userInfo;
