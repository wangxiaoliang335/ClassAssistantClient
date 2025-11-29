#pragma once
#include "TABaseDialog.h"

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
};

// StudentInfo 结构体在 ScheduleDialog.h 中定义，这里使用条件编译避免重复定义
//#ifndef STUDENT_INFO_DEFINED
//#define STUDENT_INFO_DEFINED
struct StudentInfo {
    QString id;      // 学号
    QString name;    // 姓名
    double score;    // 成绩（用于排序）
    int originalIndex; // 原始索引
    QMap<QString, double> attributes; // 多个属性值（如"背诵"、"语文"等）
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
