#pragma once

#include "TAFloatingWidget.h"
#include "TABaseDialog.h"
#include <QGridLayout>
#include <QScrollArea>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QContextMenuEvent>
#include <QLineEdit>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QFileIconProvider>
#include <QMouseEvent>
#include <QPoint>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDrag>
#include <QMimeData>
#include <QEventLoop>

// 前向声明
class TACFileManagerWidget;
class TACStyledMessageDialog; // 在 TACWallpaperLibraryDialog.h 中定义

// 使用 TACWallpaperLibraryDialog.h 中已定义的 TACStyledMessageDialog
// 只需要添加静态方法
namespace TACFileManagerDialogHelper {
    void showInformation(QWidget* parent, const QString& title, const QString& message);
    void showWarning(QWidget* parent, const QString& title, const QString& message);
    void showError(QWidget* parent, const QString& title, const QString& message);
}

// 自定义输入对话框（风格与 TACFileManagerWidget 一致）
class TACStyledInputDialog : public TABaseDialog
{
    Q_OBJECT
public:
    TACStyledInputDialog(const QString& title, const QString& label, const QString& defaultValue = "", QWidget* parent = nullptr);
    static QString getText(QWidget* parent, const QString& title, const QString& label, const QString& defaultValue = "", bool* ok = nullptr);
    
    QString text() const;
    void setText(const QString& text);
    
public slots:
    void acceptDialog(); // 接受对话框
    void rejectDialog(); // 拒绝对话框
    
private slots:
    void onOkClicked();
    void onCancelClicked();
    
signals:
    void finished(int result); // 对话框完成信号 (1 = Accepted, 0 = Rejected)
    
private:
    QLineEdit* m_lineEdit;
    bool m_ok;
    QEventLoop* m_eventLoop;
};

// 自定义 QToolButton 以支持拖出文件
class FileToolButton : public QToolButton {
    Q_OBJECT
public:
    explicit FileToolButton(QWidget* parent = nullptr) : QToolButton(parent), m_dragStartPos(), m_itemIndex(-1) {}
    
    void setFileItem(const QString& filePath, bool isFolder, const QString& displayName, int itemIndex) {
        m_filePath = filePath;
        m_isFolder = isFolder;
        m_displayName = displayName;
        m_itemIndex = itemIndex;
    }
    
signals:
    void itemDraggedOut(int itemIndex); // 当项目被拖出时发出信号
    
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    
private:
    QPoint m_dragStartPos;
    QString m_filePath;
    bool m_isFolder;
    QString m_displayName;
    int m_itemIndex; // 项目在列表中的索引
};

class TACFileManagerWidget : public TAFloatingWidget
{
    Q_OBJECT

public:
    explicit TACFileManagerWidget(QWidget* parent = nullptr);
    ~TACFileManagerWidget();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void initShow() override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onCreateFolder();
    void onRenameFolder();
    void onCloseAndRelease();
    void onFolderButtonClicked();
    void onMyFolderButtonClicked();

private:
    QScrollArea* m_scrollArea;
    QWidget* m_scrollContentWidget;
    QGridLayout* m_gridLayout;
    QPushButton* m_newFolderButton;
    QPushButton* m_myFolderButton;
    QMenu* m_contextMenu;
    QAction* m_createFolderAction;
    QAction* m_renameFolderAction;
    QAction* m_closeAction;
    
    QMap<QToolButton*, int> m_buttonToIndexMap; // 按钮到文件项索引的映射
    QFileIconProvider* m_iconProvider; // 文件图标提供程序
    
    // 文件列表数据（存储文件路径和类型）
    struct FileItem {
        QString filePath;  // 完整文件路径
        bool isFolder;     // 是否为文件夹
        QString displayName; // 显示名称
    };
    
    QList<FileItem> m_fileItems; // 存储所有文件项
    
    void setupUI();
    void setupContextMenu();
    void addFileItem(const QString& filePath, bool isFolder = false);
    void addFolderItem(const QString& folderName);
    void releaseFilesToDesktop();
    QToolButton* createFolderButton(const FileItem& item, int index);
    QString getUniqueFolderName(const QString& baseName);
    QString getUniqueFileName(const QString& filePath);
    bool isValidFileOrFolder(const QString& path);
    void updateFileListDisplay();
    void clearGridLayout();
};

