#pragma once
#pragma execution_character_set("utf-8")
#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDate>
#include <QDebug>
#include <QLabel>
#include <QScrollArea>
#include <QEvent>
#include "QXlsx/header/xlsxdocument.h"
#include "QXlsx/header/xlsxworksheet.h"
#include "QXlsx/header/xlsxcell.h"
#include "QXlsx/header/xlsxglobal.h"
#include "TAHttpHandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
QT_BEGIN_NAMESPACE_XLSX
QT_END_NAMESPACE_XLSX

class DutyRosterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DutyRosterDialog(QWidget* parent = nullptr);
    ~DutyRosterDialog();

    // 设置群组ID
    void setGroupId(const QString& groupId) {
        m_groupId = groupId;
        // 设置群组ID后，从服务器加载值日表数据
        if (m_httpHandler) {
            loadDutyRosterFromServer();
        }
    }

    // 设置班级ID
    void setClassId(const QString& classId) {
        m_classId = classId;
    }

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onCloseClicked();
    void onImportClicked();
    void onMinimalistClicked();
    void onCellDoubleClicked(int row, int column);
    void onCellChanged(int row, int column);
    void onLoadDutyRosterSuccess(const QString& resp);
    void onLoadDutyRosterFailed(const QString& err);
    void onSaveDutyRosterSuccess(const QString& resp);
    void onSaveDutyRosterFailed(const QString& err);

private:
    void setupUI();
    void importExcelFile(const QString& filePath);
    void parseExcelData(const QList<QList<QString>>& data);
    void updateTableDisplay();
    void switchToMinimalistMode();
    void switchToFullMode();
    void loadDutyRosterFromServer();
    void saveDutyRosterToServer();
    bool isRequirementRow(int row);
    int getTodayColumn();  // 获取今天对应的列索引（0=周一，4=周五）

private:
    QPushButton* m_closeButton = nullptr;
    QPushButton* m_importButton = nullptr;
    QPushButton* m_minimalistButton = nullptr;
    QTableWidget* m_tableWidget = nullptr;
    
    QVBoxLayout* m_mainLayout = nullptr;
    QHBoxLayout* m_topButtonLayout = nullptr;
    QWidget* m_minimalistWidget = nullptr;  // 极简模式的自定义widget
    QVBoxLayout* m_minimalistLayout = nullptr;  // 极简模式的布局
    QScrollArea* m_minimalistScrollArea = nullptr;  // 极简模式的滚动区域
    
    QString m_groupId;
    QString m_classId;
    
    bool m_isMinimalistMode = false;  // 是否为极简模式
    QList<QList<QString>> m_dutyData;  // 存储值日表数据（行，列）
    int m_requirementRowIndex = -1;  // 要求行的索引
    
    TAHttpHandler* m_httpHandler = nullptr;  // HTTP请求处理器
    
    QPoint m_dragPosition;  // 窗口拖动位置
};

