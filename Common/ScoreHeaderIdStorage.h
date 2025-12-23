#pragma once
#include <QString>
#include <QMap>
#include <QDebug>

// 全局存储 score_header_id（按 class_id + exam_name + term 作为键）
class ScoreHeaderIdStorage {
public:
    // 新接口：按 class_id + term 存储（不区分 exam_name）
    static void saveScoreHeaderId(const QString& classId, const QString& term, int scoreHeaderId) {
        QString key = QString("%1_%2").arg(classId).arg(term);
        s_scoreHeaderIds[key] = scoreHeaderId;
        qDebug() << "已保存 score_header_id 到全局存储，键:" << key << "，ID:" << scoreHeaderId;
    }

    static int getScoreHeaderId(const QString& classId, const QString& term) {
        QString key = QString("%1_%2").arg(classId).arg(term);
        return s_scoreHeaderIds.value(key, -1);
    }

    static bool hasScoreHeaderId(const QString& classId, const QString& term) {
        QString key = QString("%1_%2").arg(classId).arg(term);
        return s_scoreHeaderIds.contains(key);
    }

    // 兼容旧接口：examName 参数将被忽略（保留签名避免大范围改动）
	static void saveScoreHeaderId(const QString& classId, const QString& examName, const QString& term, int scoreHeaderId) {
        Q_UNUSED(examName);
        saveScoreHeaderId(classId, term, scoreHeaderId);
	}
	
	static int getScoreHeaderId(const QString& classId, const QString& examName, const QString& term) {
        Q_UNUSED(examName);
        return getScoreHeaderId(classId, term);
	}
	
	static bool hasScoreHeaderId(const QString& classId, const QString& examName, const QString& term) {
        Q_UNUSED(examName);
        return hasScoreHeaderId(classId, term);
	}
	
private:
	static QMap<QString, int> s_scoreHeaderIds;
};

