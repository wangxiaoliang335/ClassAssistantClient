#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QColorDialog>
#include <QMessageBox>
#include <QToolTip>
#include <QHeaderView>
#include <QMenu>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QSet>
#include <QTextStream>
#include <QFile>
#include <QIODevice>
#include <QLineEdit>
#include <QCursor>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>
#include <QTimer>
#include <QApplication>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QPoint>
#include <QCursor>
#include <QRect>
#include <QDesktopWidget>
#include <QScreen>
#include <QWidget>
#include "GroupScoreTableWidget.h"
#include "CellCommentWidget.h"
#include "CommentInputDialog.h"

/**
 * 小组管理表对话框
 * 格式：第一列为组号，倒数第二列为个人总分，最后一列为小组总分
 */
class GroupScoreDialog : public QDialog
{
    Q_OBJECT

public:
    GroupScoreDialog(QString classid, QWidget* parent = nullptr);
    
    // 导入Excel数据
    void importData(const QStringList& headers, const QList<QStringList>& dataRows, const QString& excelFilePath = QString());

    // 设置学期（用于 /group-scores/set-score 与 /group-scores/set-comment 定位到正确表头）
    void setTerm(const QString& term) { m_term = term; }

    // 在“无本地 Excel 文件路径”的刷新模式下（从服务器缓存构建表格），也需要显式设置表格名
    void setExcelFileName(const QString& excelFileName) { m_excelFileName = excelFileName; }
    
    // 设置成绩表头ID
    void setScoreHeaderId(int scoreHeaderId) { m_scoreHeaderId = scoreHeaderId; }

protected:
    // 重写鼠标事件以实现窗口拖动
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    
    // 鼠标进入窗口时显示关闭按钮
    void enterEvent(QEvent *event) override;
    
    // 鼠标离开窗口时隐藏关闭按钮
    void leaveEvent(QEvent *event) override;
    
    // 事件过滤器，处理关闭按钮的鼠标事件
    bool eventFilter(QObject *obj, QEvent *event) override;
    
    // 窗口大小改变时更新关闭按钮位置
    void resizeEvent(QResizeEvent *event) override;
    
    // 窗口显示时更新关闭按钮位置
    void showEvent(QShowEvent *event) override;

private slots:
    void onAddRow();
    void onDeleteRow();
    void onDeleteColumn();
    void onAddColumn();
    void onFontColor();
    void onBgColor();
    void onDescOrder();
    void onAscOrder();
    void onExport();
    void onUpload();
    void onCellClicked(int row, int column);
    void onCellEntered(int row, int column);
    void onTableContextMenu(const QPoint& pos);
    void onItemChanged(QTableWidgetItem* item);

private:
    void sortTable(bool ascending);
    void showCellComment(int row, int column);
    void setScoreToServer(int row, const QString& studentName, const QString& studentId,
                          const QString& fieldName, const QString& cellText);
    void setCommentToServer(const QString& studentName, const QString& studentId, 
                           const QString& fieldName, const QString& comment);
    void updateRowTotal(int row); // 更新指定行的个人总分
    void updateGroupTotal(); // 更新所有小组的小组总分

private:
    GroupScoreTableWidget* table;
    QTextEdit* textDescription;
    QLabel* m_lblTitle; // 标题标签
    QPushButton* btnAddRow;
    QPushButton* btnDeleteRow;
    QPushButton* btnDeleteColumn;
    QPushButton* btnAddColumn;
    QPushButton* btnFontColor;
    QPushButton* btnBgColor;
    QPushButton* btnDescOrder;
    QPushButton* btnAscOrder;
    QPushButton* btnExport;
    QPushButton* btnUpload;
    QNetworkAccessManager* networkManager;
    
    QSet<int> fixedColumns; // 固定列索引集合（不能删除的列：组号、学号、姓名、总分、小组总分）
    int nameColumnIndex; // 姓名列索引
    int groupColumnIndex; // 组号列索引（第一列）
    int totalColumnIndex; // 个人总分列索引（倒数第二列）
    int groupTotalColumnIndex; // 小组总分列索引（最后一列）
    QString m_classid;
    QString m_term; // 学期（优先使用外部传入；为空则回退当前学期计算）
    QPushButton* m_btnClose = nullptr; // 关闭按钮
    QPoint m_dragPosition; // 用于窗口拖动
    QString m_excelFilePath; // Excel文件路径
    QString m_excelFileName; // Excel文件名
    CellCommentWidget* commentWidget = nullptr; // 注释窗口
    int m_scoreHeaderId = -1; // 成绩表头ID，用于设置注释

    // 批量更新标记（导入Excel等场景），用于避免 itemChanged 触发大量计算/网络
    bool m_isBulkUpdating = false;
};

