#pragma once
#include <QString>
#include <QMap>
#include <QDebug>
#include <QList>

// 全局存储注释信息（按 classId_examName_term_studentId_fieldName 作为键）
class CommentStorage {
public:
    // 统一的 key 分隔符（避免 fieldName/tableName 内含 "_" 时解析困难）
    static QChar keySep() { return QChar(0x1F); } // Unit Separator

    static QString buildKey(const QString& classId, const QString& term,
                            const QString& studentId, const QString& tableName, const QString& fieldName) {
        const QChar sep = keySep();
        return classId + sep + term + sep + studentId + sep + tableName + sep + fieldName;
    }

    // 若 fieldName 是复合键（字段_Excel文件名.xlsx），尝试从中推断 tableName（Excel 文件名）
    static QString inferTableNameFromFieldKey(const QString& fieldName) {
        const int underscorePos = fieldName.lastIndexOf('_');
        if (underscorePos <= 0 || underscorePos >= fieldName.length() - 1) return QString();
        const QString tail = fieldName.mid(underscorePos + 1);
        if (tail.endsWith(".xlsx", Qt::CaseInsensitive) ||
            tail.endsWith(".xls", Qt::CaseInsensitive) ||
            tail.endsWith(".csv", Qt::CaseInsensitive)) {
            return tail;
        }
        return QString();
    }

    // 新接口：按 classId + term + studentId + fieldName 存储（不区分 examName）
    // 兼容：若 fieldName 为复合键，会自动推断 tableName 参与去重
    static void saveComment(const QString& classId, const QString& term,
                            const QString& studentId, const QString& fieldName, const QString& comment) {
        const QString inferredTable = inferTableNameFromFieldKey(fieldName);
        saveComment(classId, term, studentId, fieldName, inferredTable, comment);
    }

    // 新接口（推荐）：显式带 tableName/excelFilename，避免同名字段在不同表互相覆盖
    static void saveComment(const QString& classId, const QString& term,
                            const QString& studentId, const QString& fieldName, const QString& tableName,
                            const QString& comment) {
        const QString key = buildKey(classId, term, studentId, tableName, fieldName);
        if (!comment.isEmpty()) {
            s_comments[key] = comment;
        } else {
            s_comments.remove(key);
        }
    }

    static QString getComment(const QString& classId, const QString& term,
                              const QString& studentId, const QString& fieldName) {
        const QString inferredTable = inferTableNameFromFieldKey(fieldName);
        const QString v = getComment(classId, term, studentId, fieldName, inferredTable);
        if (!v.isEmpty()) return v;
        // 兼容老 key（仅用于迁移期间/未重启时）
        const QString legacyKey = QString("%1_%2_%3_%4").arg(classId).arg(term).arg(studentId).arg(fieldName);
        return s_comments.value(legacyKey, "");
    }

    static QString getComment(const QString& classId, const QString& term,
                              const QString& studentId, const QString& fieldName, const QString& tableName) {
        const QString key = buildKey(classId, term, studentId, tableName, fieldName);
        const QString v = s_comments.value(key, "");
        if (!v.isEmpty()) return v;
        // 若 fieldName 是复合键，且 tableName 没传/不匹配，尝试推断一次
        const QString inferredTable = inferTableNameFromFieldKey(fieldName);
        if (!inferredTable.isEmpty() && inferredTable != tableName) {
            return s_comments.value(buildKey(classId, term, studentId, inferredTable, fieldName), "");
        }
        return "";
    }

    static void clearComments(const QString& classId, const QString& term) {
        const QChar sep = keySep();
        const QString prefix = classId + sep + term + sep;
        QList<QString> keysToRemove;
        for (auto it = s_comments.begin(); it != s_comments.end(); ++it) {
            if (it.key().startsWith(prefix)) {
                keysToRemove.append(it.key());
            }
        }
        for (const QString& key : keysToRemove) {
            s_comments.remove(key);
        }
        // 兼容老 key 清理
        const QString legacyPrefix = QString("%1_%2_").arg(classId).arg(term);
        keysToRemove.clear();
        for (auto it = s_comments.begin(); it != s_comments.end(); ++it) {
            if (it.key().startsWith(legacyPrefix)) keysToRemove.append(it.key());
        }
        for (const QString& key : keysToRemove) s_comments.remove(key);
    }

    // 清除指定班级+学期+学生的所有注释（用于用服务端返回的整段 comments_json 覆盖本地缓存）
    static void clearStudentComments(const QString& classId, const QString& term, const QString& studentId) {
        const QChar sep = keySep();
        const QString prefix = classId + sep + term + sep + studentId + sep;
        QList<QString> keysToRemove;
        for (auto it = s_comments.begin(); it != s_comments.end(); ++it) {
            if (it.key().startsWith(prefix)) {
                keysToRemove.append(it.key());
            }
        }
        for (const QString& key : keysToRemove) {
            s_comments.remove(key);
        }
        // 兼容老 key 清理
        const QString legacyPrefix = QString("%1_%2_%3_").arg(classId).arg(term).arg(studentId);
        keysToRemove.clear();
        for (auto it = s_comments.begin(); it != s_comments.end(); ++it) {
            if (it.key().startsWith(legacyPrefix)) keysToRemove.append(it.key());
        }
        for (const QString& key : keysToRemove) s_comments.remove(key);
    }

    // 清除指定班级+学期的注释，但保留 fieldName 以指定前缀开头的注释（用于同时保留多套来源数据）
    // key 格式：classId␟term␟studentId␟tableName␟fieldName
    static void clearCommentsExceptFieldPrefix(const QString& classId, const QString& term, const QString& fieldPrefixToKeep) {
        const QChar sep = keySep();
        const QString prefix = classId + sep + term + sep;
        QList<QString> keysToRemove;
        for (auto it = s_comments.begin(); it != s_comments.end(); ++it) {
            const QString& k = it.key();
            if (!k.startsWith(prefix)) continue;

            // fieldName 在最后一个分隔符之后
            const int lastSep = k.lastIndexOf(sep);
            if (lastSep < 0 || lastSep + 1 >= k.length()) {
                keysToRemove.append(k);
                continue;
            }
            const QString fieldName = k.mid(lastSep + 1);
            if (!fieldName.startsWith(fieldPrefixToKeep)) {
                keysToRemove.append(k);
            }
        }
        for (const QString& key : keysToRemove) {
            s_comments.remove(key);
        }

        // 兼容老 key 清理逻辑：classId_term_studentId_fieldName
        const QString legacyPrefix = QString("%1_%2_").arg(classId).arg(term);
        keysToRemove.clear();
        for (auto it = s_comments.begin(); it != s_comments.end(); ++it) {
            const QString& k = it.key();
            if (!k.startsWith(legacyPrefix)) continue;
            const int start = legacyPrefix.length();
            const int sepPos = k.indexOf('_', start);
            if (sepPos < 0) { keysToRemove.append(k); continue; }
            const QString fieldName = k.mid(sepPos + 1);
            if (!fieldName.startsWith(fieldPrefixToKeep)) keysToRemove.append(k);
        }
        for (const QString& key : keysToRemove) s_comments.remove(key);
    }

private:
	static QMap<QString, QString> s_comments;
};

