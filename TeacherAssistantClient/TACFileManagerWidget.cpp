#pragma execution_character_set("utf-8")
#include "TACFileManagerWidget.h"
#include "TACWallpaperLibraryDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFileDialog>
#include <QDebug>
#include <QDataStream>
#include <QStandardPaths>
#include <QFile>
#include <QCloseEvent>
#include <QDrag>
#include <QPixmap>
#include <QScrollArea>
#include <QGridLayout>
#include <QToolButton>
#include <QMap>
#include <QFileIconProvider>
#include <QMouseEvent>
#include <QEventLoop>
#include "common.h"

TACFileManagerWidget::TACFileManagerWidget(QWidget* parent)
    : TAFloatingWidget(parent)
{
    this->setObjectName("TACFileManagerWidget");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(15);
    this->visibleCloseButton(true); // 显示关闭按钮
    this->setTitleName(QString::fromUtf8("文件管理窗口"));
    this->setWindowTitle(QString::fromUtf8("文件管理窗口"));
    
    // 初始化文件图标提供程序
    m_iconProvider = new QFileIconProvider();
    
    setupUI();
    setupContextMenu();
    
    // 启用拖放
    setAcceptDrops(true);
    
    resize(600, 600);
}

TACFileManagerWidget::~TACFileManagerWidget()
{
    // 清理资源
    clearGridLayout();
    if (m_iconProvider) {
        delete m_iconProvider;
        m_iconProvider = nullptr;
    }
}

void TACFileManagerWidget::initShow()
{
    // 初始化显示
}

void TACFileManagerWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 40, 10, 10); // 顶部留空间给标题和关闭按钮
    mainLayout->setSpacing(10);
    
    // 创建滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(
        "QScrollArea {"
        "background-color: transparent;"
        "border: none;"
        "}"
    );
    m_scrollArea->setAutoFillBackground(false);
    
    // 创建滚动内容容器
    m_scrollContentWidget = new QWidget();
    m_scrollContentWidget->setStyleSheet(
        "QWidget {"
        "background-color: transparent;"
        "}"
    );
    m_scrollContentWidget->setAutoFillBackground(false);
    m_gridLayout = new QGridLayout(m_scrollContentWidget);
    m_gridLayout->setSpacing(15);
    m_gridLayout->setContentsMargins(15, 15, 15, 15);
    
    m_scrollArea->setWidget(m_scrollContentWidget);
    mainLayout->addWidget(m_scrollArea, 1); // 占据主要空间
    
    // 创建底部按钮区域
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(10);
    bottomLayout->addStretch();
    
    // 新文件夹按钮（透明背景，与其他控件风格一致）
    m_newFolderButton = new QPushButton(QString::fromUtf8("新文件夹"), this);
    m_newFolderButton->setFixedSize(120, 50);
    m_newFolderButton->setAutoFillBackground(false); // 确保背景透明
    m_newFolderButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(80, 80, 80, 150);"
        "color: white;"
        "border: none;"
        "border-radius: 5px;"
        "font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(100, 100, 100, 180);"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(60, 60, 60, 200);"
        "}"
    );
    connect(m_newFolderButton, &QPushButton::clicked, this, &TACFileManagerWidget::onCreateFolder);
    bottomLayout->addWidget(m_newFolderButton);
    
    // 我的文件夹按钮（透明背景，与其他控件风格一致）
    m_myFolderButton = new QPushButton(QString::fromUtf8("我的文件夹"), this);
    m_myFolderButton->setFixedSize(100, 40);
    m_myFolderButton->setAutoFillBackground(false); // 确保背景透明
    m_myFolderButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(80, 80, 80, 150);"
        "color: white;"
        "border: none;"
        "border-radius: 5px;"
        "font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(100, 100, 100, 180);"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(60, 60, 60, 200);"
        "}"
    );
    connect(m_myFolderButton, &QPushButton::clicked, this, &TACFileManagerWidget::onMyFolderButtonClicked);
    bottomLayout->addWidget(m_myFolderButton);
    
    mainLayout->addLayout(bottomLayout);
}

void TACFileManagerWidget::setupContextMenu()
{
    m_contextMenu = new QMenu(this);
    
    m_createFolderAction = new QAction(QString::fromUtf8("新建文件夹"), this);
    connect(m_createFolderAction, &QAction::triggered, this, &TACFileManagerWidget::onCreateFolder);
    m_contextMenu->addAction(m_createFolderAction);
    
    m_renameFolderAction = new QAction(QString::fromUtf8("重命名"), this);
    connect(m_renameFolderAction, &QAction::triggered, this, &TACFileManagerWidget::onRenameFolder);
    m_contextMenu->addAction(m_renameFolderAction);
    
    m_contextMenu->addSeparator();
    
    m_closeAction = new QAction(QString::fromUtf8("关闭并释放文件"), this);
    connect(m_closeAction, &QAction::triggered, this, &TACFileManagerWidget::onCloseAndRelease);
    m_contextMenu->addAction(m_closeAction);
}

void TACFileManagerWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasFormat("application/x-filemanager-item")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void TACFileManagerWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasFormat("application/x-filemanager-item")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void TACFileManagerWidget::dropEvent(QDropEvent* event)
{
    // 处理从桌面拖入的文件
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl& url : urls) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                if (isValidFileOrFolder(filePath)) {
                    QFileInfo fileInfo(filePath);
                    if (fileInfo.isDir()) {
                        // 保存文件夹的完整路径，以便正确显示图标
                        addFileItem(filePath, true);
                    } else {
                        addFileItem(filePath, false);
                    }
                }
            }
        }
        event->acceptProposedAction();
        updateFileListDisplay();
    }
    // 处理从其他文件管理组件拖入的文件
    else if (event->mimeData()->hasFormat("application/x-filemanager-item")) {
        QByteArray itemData = event->mimeData()->data("application/x-filemanager-item");
        QDataStream stream(&itemData, QIODevice::ReadOnly);
        QString filePath;
        bool isFolder;
        QString displayName;
        stream >> filePath >> isFolder >> displayName;
        
        // 检查拖放源是否是另一个 TACFileManagerWidget
        TACFileManagerWidget* sourceFileManager = nullptr;
        QWidget* sourceWidget = qobject_cast<QWidget*>(event->source());
        if (sourceWidget) {
            // 尝试直接转换
            sourceFileManager = qobject_cast<TACFileManagerWidget*>(sourceWidget);
            // 如果是从 FileToolButton 拖出的，向上查找父窗口
            if (!sourceFileManager) {
                QWidget* parent = sourceWidget;
                while (parent) {
                    sourceFileManager = qobject_cast<TACFileManagerWidget*>(parent);
                    if (sourceFileManager) {
                        break;
                    }
                    parent = parent->parentWidget();
                }
            }
        }
        
        // 如果是文件夹（虚拟文件夹，没有实际路径）
        if (isFolder && filePath.isEmpty()) {
            addFolderItem(displayName);
        } else if (isValidFileOrFolder(filePath)) {
            // 保存完整路径，以便正确显示图标和后续操作
            if (isFolder) {
                addFileItem(filePath, true);
            } else {
                addFileItem(filePath, false);
            }
        }
        
        // 如果是从另一个文件管理窗口拖入的，使用移动操作（会在源窗口移除）
        if (sourceFileManager && sourceFileManager != this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
        updateFileListDisplay();
    } else {
        event->ignore();
    }
}

void TACFileManagerWidget::contextMenuEvent(QContextMenuEvent* event)
{
    // 检查是否点击在文件夹按钮上
    QWidget* widget = childAt(event->pos());
    QToolButton* button = qobject_cast<QToolButton*>(widget);
    if (button && m_buttonToIndexMap.contains(button)) {
        int index = m_buttonToIndexMap[button];
        if (index >= 0 && index < m_fileItems.size() && m_fileItems[index].isFolder) {
            m_renameFolderAction->setEnabled(true);
            // 可以存储当前选中的索引用于重命名
        } else {
            m_renameFolderAction->setEnabled(false);
        }
    } else {
        m_renameFolderAction->setEnabled(false);
    }
    m_contextMenu->exec(event->globalPos());
}

void TACFileManagerWidget::onCreateFolder()
{
    QString folderName = getUniqueFolderName(QString::fromUtf8("新建文件夹"));
    
    // 弹出输入对话框让用户输入文件夹名称
    bool ok;
    QString newFolderName = TACStyledInputDialog::getText(this, QString::fromUtf8("新建文件夹"), QString::fromUtf8("文件夹名称:"), folderName, &ok);
    
    if (ok && !newFolderName.isEmpty()) {
        // 确保名称唯一
        QString uniqueName = getUniqueFolderName(newFolderName);
        addFolderItem(uniqueName);
        updateFileListDisplay();
    }
}

void TACFileManagerWidget::onRenameFolder()
{
    // 从右键菜单获取选中的文件夹（通过上下文菜单事件）
    // 这里简化处理，重命名第一个文件夹项
    // 实际应该从右键菜单的上下文获取
    int folderIndex = -1;
    for (int i = 0; i < m_fileItems.size(); ++i) {
        if (m_fileItems[i].isFolder) {
            folderIndex = i;
            break;
        }
    }
    
    if (folderIndex < 0) {
        TACFileManagerDialogHelper::showInformation(this, QString::fromUtf8("提示"), QString::fromUtf8("没有可重命名的文件夹"));
        return;
    }
    
    FileItem& fileItem = m_fileItems[folderIndex];
    QString oldName = fileItem.displayName;
    bool ok;
    QString newName = TACStyledInputDialog::getText(this, QString::fromUtf8("重命名文件夹"), QString::fromUtf8("新名称:"), oldName, &ok);
    
    if (ok && !newName.isEmpty() && newName != oldName) {
        // 检查名称是否已存在
        bool nameExists = false;
        for (int i = 0; i < m_fileItems.size(); ++i) {
            if (i != folderIndex && m_fileItems[i].displayName == newName && m_fileItems[i].isFolder) {
                nameExists = true;
                break;
            }
        }
        
        if (nameExists) {
            TACFileManagerDialogHelper::showWarning(this, QString::fromUtf8("提示"), QString::fromUtf8("该名称已存在"));
            return;
        }
        
        fileItem.displayName = newName;
        updateFileListDisplay();
    }
}

void TACFileManagerWidget::onFolderButtonClicked()
{
    QToolButton* button = qobject_cast<QToolButton*>(sender());
    if (!button || !m_buttonToIndexMap.contains(button)) {
        return;
    }
    
    int index = m_buttonToIndexMap[button];
    if (index >= 0 && index < m_fileItems.size()) {
        FileItem& fileItem = m_fileItems[index];
        if (fileItem.isFolder) {
            // 如果是文件夹，可以打开（这里只是示例，可以根据需求实现）
            TACFileManagerDialogHelper::showInformation(this, QString::fromUtf8("提示"), QString::fromUtf8("文件夹: %1").arg(fileItem.displayName));
        } else if (!fileItem.filePath.isEmpty()) {
            // 打开文件
            QDesktopServices::openUrl(QUrl::fromLocalFile(fileItem.filePath));
        }
    }
}

void TACFileManagerWidget::onMyFolderButtonClicked()
{
    // 显示我的文件夹（可以打开系统文件夹或自定义文件夹）
    QString myFolderPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QDesktopServices::openUrl(QUrl::fromLocalFile(myFolderPath));
}

void TACFileManagerWidget::onCloseAndRelease()
{
    releaseFilesToDesktop();
    this->close();
}

void TACFileManagerWidget::closeEvent(QCloseEvent* event)
{
    releaseFilesToDesktop();
    event->accept();
}

void TACFileManagerWidget::addFileItem(const QString& filePath, bool isFolder)
{
    // 对于文件夹，如果没有路径也是允许的（虚拟文件夹）
    if (!isFolder && !isValidFileOrFolder(filePath)) {
        return;
    }
    
    FileItem item;
    item.filePath = filePath;
    item.isFolder = isFolder;
    
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        item.displayName = fileInfo.fileName();
    }
    
    // 检查是否已存在（通过文件路径判断，如果是文件夹且路径为空，则通过名称判断）
    for (const FileItem& existingItem : m_fileItems) {
        if (!filePath.isEmpty() && existingItem.filePath == filePath) {
            return; // 已存在，不重复添加
        }
        if (isFolder && filePath.isEmpty() && existingItem.isFolder && 
            existingItem.filePath.isEmpty()) {
            // 虚拟文件夹通过名称判断，但这里暂时不检查名称，允许重复
        }
    }
    
    m_fileItems.append(item);
}

void TACFileManagerWidget::addFolderItem(const QString& folderName)
{
    // 确保名称唯一
    QString uniqueName = getUniqueFolderName(folderName);
    
    FileItem item;
    item.filePath = ""; // 虚拟文件夹没有实际路径
    item.isFolder = true;
    item.displayName = uniqueName;
    
    m_fileItems.append(item);
}

void TACFileManagerWidget::releaseFilesToDesktop()
{
    if (m_fileItems.isEmpty()) {
        return;
    }
    
        // 获取桌面路径
        QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        if (desktopPath.isEmpty()) {
            TACFileManagerDialogHelper::showError(this, QString::fromUtf8("错误"), QString::fromUtf8("无法获取桌面路径"));
            return;
        }
    
    QDir desktopDir(desktopPath);
    
    // 释放所有文件到桌面
    for (const FileItem& item : m_fileItems) {
        if (item.isFolder) {
            // 创建文件夹
            QString folderPath = desktopDir.filePath(item.displayName);
            if (!QDir(folderPath).exists()) {
                desktopDir.mkdir(item.displayName);
            }
        } else if (!item.filePath.isEmpty()) {
            // 复制文件到桌面
            QString fileName = QFileInfo(item.filePath).fileName();
            QString destPath = desktopDir.filePath(fileName);
            
            // 如果文件已存在，生成新名称
            if (QFile::exists(destPath)) {
                destPath = getUniqueFileName(destPath);
            }
            
            QFile::copy(item.filePath, destPath);
        }
    }
}

QString TACFileManagerWidget::getUniqueFolderName(const QString& baseName)
{
    QString newName = baseName;
    int counter = 1;
    
    while (true) {
        bool exists = false;
        for (const FileItem& item : m_fileItems) {
            if (item.isFolder && item.displayName == newName) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            return newName;
        }
        
        newName = QString("%1 (%2)").arg(baseName).arg(counter);
        counter++;
    }
}

QString TACFileManagerWidget::getUniqueFileName(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.baseName();
    QString suffix = fileInfo.suffix();
    QString dir = fileInfo.absolutePath();
    
    QString newPath = filePath;
    int counter = 1;
    
    while (QFile::exists(newPath)) {
        if (suffix.isEmpty()) {
            newPath = QDir(dir).filePath(QString("%1 (%2)").arg(baseName).arg(counter));
        } else {
            newPath = QDir(dir).filePath(QString("%1 (%2).%3").arg(baseName).arg(counter).arg(suffix));
        }
        counter++;
    }
    
    return newPath;
}

bool TACFileManagerWidget::isValidFileOrFolder(const QString& path)
{
    if (path.isEmpty()) {
        return false;
    }
    
    QFileInfo fileInfo(path);
    return fileInfo.exists() && (fileInfo.isFile() || fileInfo.isDir());
}

void TACFileManagerWidget::updateFileListDisplay()
{
    clearGridLayout();
    
    // 计算网格布局（每行4个）
    const int columnsPerRow = 4;
    int row = 0;
    int col = 0;
    
    for (int i = 0; i < m_fileItems.size(); ++i) {
        const FileItem& item = m_fileItems[i];
        QToolButton* button = createFolderButton(item, i);
        m_gridLayout->addWidget(button, row, col, Qt::AlignLeft | Qt::AlignTop);
        
        col++;
        if (col >= columnsPerRow) {
            col = 0;
            row++;
        }
    }
}

QToolButton* TACFileManagerWidget::createFolderButton(const FileItem& item, int index)
{
    FileToolButton* button = new FileToolButton(m_scrollContentWidget);
    
    // 设置文件项信息，用于拖放
    button->setFileItem(item.filePath, item.isFolder, item.displayName, index);
    
    // 连接拖出信号，当文件被拖到其他窗口时移除
    connect(button, &FileToolButton::itemDraggedOut, this, [this](int itemIndex) {
        if (itemIndex >= 0 && itemIndex < m_fileItems.size()) {
            m_fileItems.removeAt(itemIndex);
            updateFileListDisplay();
        }
    });
    
    // 根据文件类型设置图标
    QIcon icon;
    if (item.isFolder) {
        // 如果是文件夹
        if (item.filePath.isEmpty()) {
            // 虚拟文件夹，使用默认文件夹图标
            icon = QIcon(":/res/img/home_ic_file_nor.png");
        } else {
            // 实际文件夹，使用系统图标
            QFileInfo fileInfo(item.filePath);
            icon = m_iconProvider->icon(QFileIconProvider::Folder);
        }
    } else {
        // 如果是文件，使用系统文件图标
        if (!item.filePath.isEmpty()) {
            QFileInfo fileInfo(item.filePath);
            icon = m_iconProvider->icon(fileInfo);
        } else {
            // 如果没有路径，使用默认文件图标
            icon = m_iconProvider->icon(QFileIconProvider::File);
        }
    }
    
    button->setIcon(icon);
    // 设置图标大小为 48x48，确保图标不会太大
    button->setIconSize(QSize(48, 48));
    button->setText(item.displayName);
    button->setFixedSize(100, 100);
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setStyleSheet(
        "QToolButton {"
        "background-color: transparent;"
        "color: white;"
        "border: none;"
        "font-size: 12px;"
        "}"
        "QToolButton:hover {"
        "background-color: rgba(100, 100, 100, 100);"
        "border-radius: 5px;"
        "}"
    );
    // 确保按钮背景透明
    button->setAutoFillBackground(false);
    
    // 存储索引到按钮
    m_buttonToIndexMap[button] = index;
    
    // 连接点击事件
    connect(button, &QToolButton::clicked, this, &TACFileManagerWidget::onFolderButtonClicked);
    
    // 启用拖放
    button->setAcceptDrops(true);
    
    return button;
}

void TACFileManagerWidget::clearGridLayout()
{
    // 清除所有按钮
    QLayoutItem* item;
    while ((item = m_gridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            QToolButton* button = qobject_cast<QToolButton*>(item->widget());
            if (button) {
                m_buttonToIndexMap.remove(button);
            }
            item->widget()->deleteLater();
        }
        delete item;
    }
}

// FileToolButton 实现
void FileToolButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
    }
    QToolButton::mousePressEvent(event);
}

void FileToolButton::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        QToolButton::mouseMoveEvent(event);
        return;
    }
    
    if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
        QToolButton::mouseMoveEvent(event);
        return;
    }
    
    // 开始拖放
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData();
    
    QByteArray itemData;
    QDataStream stream(&itemData, QIODevice::WriteOnly);
    stream << m_filePath << m_isFolder << m_displayName;
    
    mimeData->setData("application/x-filemanager-item", itemData);
    
    // 如果是实际文件，也添加到 URLs
    if (!m_filePath.isEmpty() && !m_isFolder && QFile::exists(m_filePath)) {
        QList<QUrl> urls;
        urls.append(QUrl::fromLocalFile(m_filePath));
        mimeData->setUrls(urls);
    }
    
    drag->setMimeData(mimeData);
    
    // 设置拖放图标
    QPixmap pixmap = icon().pixmap(iconSize());
    if (!pixmap.isNull()) {
        drag->setPixmap(pixmap);
    }
    
    // 执行拖放，优先使用移动操作
    Qt::DropAction dropAction = drag->exec(Qt::MoveAction | Qt::CopyAction);
    
    // 如果拖放成功且是移动操作（通常是移动到另一个文件管理窗口），发出信号移除源项
    if (dropAction == Qt::MoveAction) {
        emit itemDraggedOut(m_itemIndex);
    }
    
    QToolButton::mouseMoveEvent(event);
}

// TACFileManagerDialogHelper 实现
namespace TACFileManagerDialogHelper {
    void showInformation(QWidget* parent, const QString& title, const QString& message)
    {
        TACStyledMessageDialog* dialog = new TACStyledMessageDialog(title, message, parent);
        if (parent) {
            QRect parentRect = parent->geometry();
            int x = parentRect.x() + (parentRect.width() - dialog->width()) / 2;
            int y = parentRect.y() + (parentRect.height() - dialog->height()) / 2;
            dialog->move(x, y);
        }
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
    }

    void showWarning(QWidget* parent, const QString& title, const QString& message)
    {
        showInformation(parent, title, message); // 使用相同的实现
    }

    void showError(QWidget* parent, const QString& title, const QString& message)
    {
        showInformation(parent, title, message); // 使用相同的实现
    }
}

// TACStyledInputDialog 实现
TACStyledInputDialog::TACStyledInputDialog(const QString& title, const QString& label, const QString& defaultValue, QWidget* parent)
    : TABaseDialog(parent), m_ok(false), m_lineEdit(nullptr), m_eventLoop(nullptr)
{
    setObjectName("TACFileManagerInputDialog");
    setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    setBorderColor(WIDGET_BORDER_COLOR);
    setBorderWidth(WIDGET_BORDER_WIDTH);
    setRadius(15);
    setFixedSize(QSize(400, 240)); // 增加窗口高度以适应更高的输入框和按钮间距
    setTitle(title);
    
    // 创建标签
    QLabel* labelWidget = new QLabel(label, this);
    labelWidget->setStyleSheet(
        "QLabel { "
        "color: white; "
        "font-size: 14px; "
        "background: transparent; "
        "}"
    );
    
    // 创建输入框
    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setText(defaultValue);
    m_lineEdit->setFixedHeight(35); // 设置固定高度，确保文本可见
    m_lineEdit->setStyleSheet(
        "QLineEdit { "
        "background-color: rgba(255, 255, 255, 30); "
        "border: 1px solid #5C5C5C; "
        "border-radius: 5px; "
        "color: white; "
        "font-size: 14px; "
        "padding: 8px; "
        "}"
        "QLineEdit:focus { "
        "border: 1px solid #4169E1; "
        "background-color: rgba(255, 255, 255, 40); "
        "}"
    );
    m_lineEdit->selectAll();
    
    // 创建确定和取消按钮
    QPushButton* okButton = new QPushButton(QString::fromUtf8("确定"), this);
    okButton->setFixedHeight(35);
    okButton->setStyleSheet(
        "QPushButton { "
        "padding: 8px 25px; "
        "border-radius: 5px; "
        "background-color: #4169E1; "
        "color: white; "
        "font-size: 14px; "
        "min-width: 80px; "
        "}"
        "QPushButton:hover { "
        "background-color: #5A7AE8; "
        "}"
        "QPushButton:pressed { "
        "background-color: #3651C4; "
        "}"
    );
    
    QPushButton* cancelButton = new QPushButton(QString::fromUtf8("取消"), this);
    cancelButton->setFixedHeight(35);
    cancelButton->setStyleSheet(
        "QPushButton { "
        "padding: 8px 25px; "
        "border-radius: 5px; "
        "background-color: #5C5C5C; "
        "color: white; "
        "font-size: 14px; "
        "min-width: 80px; "
        "}"
        "QPushButton:hover { "
        "background-color: #6B6B6B; "
        "}"
        "QPushButton:pressed { "
        "background-color: #4A4A4A; "
        "}"
    );
    
    // 创建布局
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(20); // 增加间距
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    mainLayout->addWidget(labelWidget);
    mainLayout->addWidget(m_lineEdit);
    mainLayout->addSpacing(10); // 在输入框和按钮之间添加额外间距
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10); // 按钮之间的间距
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
    
    contentLayout->addLayout(mainLayout);
    
    // 连接信号
    connect(okButton, &QPushButton::clicked, this, &TACStyledInputDialog::onOkClicked);
    connect(cancelButton, &QPushButton::clicked, this, &TACStyledInputDialog::onCancelClicked);
    connect(m_lineEdit, &QLineEdit::returnPressed, this, &TACStyledInputDialog::onOkClicked);
    
    // 设置焦点到输入框
    m_lineEdit->setFocus();
}

QString TACStyledInputDialog::getText(QWidget* parent, const QString& title, const QString& label, const QString& defaultValue, bool* ok)
{
    TACStyledInputDialog* dialog = new TACStyledInputDialog(title, label, defaultValue, parent);
    if (parent) {
        QRect parentRect = parent->geometry();
        int x = parentRect.x() + (parentRect.width() - dialog->width()) / 2;
        int y = parentRect.y() + (parentRect.height() - dialog->height()) / 2;
        dialog->move(x, y);
    }
    
    // 创建事件循环
    QEventLoop loop;
    dialog->m_eventLoop = &loop;
    
    // 连接完成信号以退出循环
    QObject::connect(dialog, &TACStyledInputDialog::finished, [&loop, dialog](int result) {
        dialog->m_ok = (result == 1); // 1 = Accepted
        loop.quit();
    });
    
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
    
    // 进入事件循环等待用户操作
    loop.exec();
    
    QString result = dialog->m_ok ? dialog->text() : QString();
    if (ok) {
        *ok = dialog->m_ok;
    }
    
    dialog->deleteLater();
    
    return result;
}

QString TACStyledInputDialog::text() const
{
    return m_lineEdit->text();
}

void TACStyledInputDialog::setText(const QString& text)
{
    m_lineEdit->setText(text);
}

void TACStyledInputDialog::onOkClicked()
{
    m_ok = true;
    acceptDialog();
}

void TACStyledInputDialog::onCancelClicked()
{
    m_ok = false;
    rejectDialog();
}

void TACStyledInputDialog::acceptDialog()
{
    emit finished(1); // 1 = Accepted
    QWidget::close();
}

void TACStyledInputDialog::rejectDialog()
{
    emit finished(0); // 0 = Rejected
    QWidget::close();
}
