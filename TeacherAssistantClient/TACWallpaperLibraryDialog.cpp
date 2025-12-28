#pragma execution_character_set("utf-8")
#include "TACWallpaperLibraryDialog.h"
#include <QScrollArea>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QApplication>
#include <QDateTime>
#include <windows.h>
#include <QUrlQuery>
#include <QStandardPaths>
#include "common.h"
#include "../TeacherAssistantClient/TACMainDialog.h"

TACWallpaperLibraryDialog::TACWallpaperLibraryDialog(QWidget *parent)
	: TABaseDialog(parent),
	m_selectedMyWallpaperItem(nullptr),
	m_selectedLibraryItem(nullptr),
	m_networkManager(nullptr),
	m_currentDownloadReply(nullptr),
	m_downloadAndSetFlag(false),
	m_httpHandler(nullptr)
{
    this->setObjectName("TACWallpaperLibraryDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(890, 550));
    //this->setTitle(QString::fromUtf8(u8"我的壁纸"));

    // 初始化壁纸存储目录
    m_wallpaperStorageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/wallpapers";
    QDir dir;
    if (!dir.exists(m_wallpaperStorageDir)) {
        dir.mkpath(m_wallpaperStorageDir);
    }

    // 创建网络管理器
    m_networkManager = new QNetworkAccessManager(this);
    
    // 创建HTTP处理器
    m_httpHandler = new TAHttpHandler(this);
    connect(m_httpHandler, &TAHttpHandler::success, this, &TACWallpaperLibraryDialog::onWallpaperListReceived);
    connect(m_httpHandler, &TAHttpHandler::failed, this, &TACWallpaperLibraryDialog::onWallpaperListFailed);

    // 创建页面切换按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(10, 0, 10, 0);
    
    m_myWallpaperButton = new QPushButton(QString::fromUtf8(u8"我的壁纸"), this);
    m_myWallpaperButton->setCheckable(true);
    m_myWallpaperButton->setChecked(true); // 默认选中"我的壁纸"
    m_myWallpaperButton->setStyleSheet(
        "QPushButton { padding: 8px 20px; border-radius: 5px; background-color: #5C5C5C; color: white; }"
        "QPushButton:checked { background-color: #4169E1; }"
        "QPushButton:hover { background-color: #6B6B6B; }"
    );
    connect(m_myWallpaperButton, &QPushButton::clicked, this, &TACWallpaperLibraryDialog::onMyWallpaperButtonClicked);
    
    m_wallpaperLibraryButton = new QPushButton(QString::fromUtf8(u8"壁纸库"), this);
    m_wallpaperLibraryButton->setCheckable(true);
    m_wallpaperLibraryButton->setStyleSheet(
        "QPushButton { padding: 8px 20px; border-radius: 5px; background-color: #5C5C5C; color: white; }"
        "QPushButton:checked { background-color: #4169E1; }"
        "QPushButton:hover { background-color: #6B6B6B; }"
    );
    connect(m_wallpaperLibraryButton, &QPushButton::clicked, this, &TACWallpaperLibraryDialog::onWallpaperLibraryButtonClicked);
    
    buttonLayout->addWidget(m_myWallpaperButton);
    buttonLayout->addWidget(m_wallpaperLibraryButton);
    buttonLayout->addStretch();
    
    // 创建"设为壁纸"按钮（我的壁纸页面）
    m_setAsWallpaperButton = new QPushButton(QString::fromUtf8(u8"设为壁纸"), this);
    m_setAsWallpaperButton->setStyleSheet(
        "QPushButton { padding: 8px 20px; border-radius: 5px; background-color: #4169E1; color: white; }"
        "QPushButton:hover { background-color: #5A7AE8; }"
        "QPushButton:disabled { background-color: #5C5C5C; color: #888888; }"
    );
    m_setAsWallpaperButton->setEnabled(false);
    connect(m_setAsWallpaperButton, &QPushButton::clicked, this, &TACWallpaperLibraryDialog::onSetAsWallpaperClicked);
    
    // 创建"下载并设为壁纸"按钮（壁纸库页面）
    m_downloadAndSetButton = new QPushButton(QString::fromUtf8(u8"下载并设为壁纸"), this);
    m_downloadAndSetButton->setStyleSheet(
        "QPushButton { padding: 8px 20px; border-radius: 5px; background-color: #4169E1; color: white; }"
        "QPushButton:hover { background-color: #5A7AE8; }"
        "QPushButton:disabled { background-color: #5C5C5C; color: #888888; }"
    );
    //m_downloadAndSetButton->setEnabled(false);
    connect(m_downloadAndSetButton, &QPushButton::clicked, this, &TACWallpaperLibraryDialog::onDownloadAndSetClicked);
    
    // 创建"下载"按钮（壁纸库页面）
    m_downloadButton = new QPushButton(QString::fromUtf8(u8"下载"), this);
    m_downloadButton->setStyleSheet(
        "QPushButton { padding: 8px 20px; border-radius: 5px; background-color: #5C5C5C; color: white; }"
        "QPushButton:hover { background-color: #6B6B6B; }"
        "QPushButton:disabled { background-color: #5C5C5C; color: #888888; }"
    );
    //m_downloadButton->setEnabled(false);
    connect(m_downloadButton, &QPushButton::clicked, this, &TACWallpaperLibraryDialog::onDownloadClicked);
    
    buttonLayout->addWidget(m_setAsWallpaperButton);
    buttonLayout->addWidget(m_downloadAndSetButton);
    buttonLayout->addWidget(m_downloadButton);
    
    // 将按钮布局添加到标题布局
    this->titleLayout->addLayout(buttonLayout);

    // 创建堆叠组件
    m_stackedWidget = new QStackedWidget(this);
    
    // 创建"我的壁纸"页面
    m_myWallpaperPage = new QWidget();
    QScrollArea* myWallpaperScrollArea = new QScrollArea();
    myWallpaperScrollArea->setWidgetResizable(true);
    myWallpaperScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    myWallpaperScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* myWallpaperContainer = new QWidget();
    m_myWallpaperGridLayout = new QGridLayout(myWallpaperContainer);
    m_myWallpaperGridLayout->setSpacing(10);
    m_myWallpaperGridLayout->setContentsMargins(10, 10, 10, 10);
    
    myWallpaperScrollArea->setWidget(myWallpaperContainer);
    QVBoxLayout* myWallpaperPageLayout = new QVBoxLayout(m_myWallpaperPage);
    myWallpaperPageLayout->setContentsMargins(0, 0, 0, 0);
    myWallpaperPageLayout->addWidget(myWallpaperScrollArea);
    
    // 创建"壁纸库"页面
    m_wallpaperLibraryPage = new QWidget();
    QScrollArea* wallpaperLibraryScrollArea = new QScrollArea();
    wallpaperLibraryScrollArea->setWidgetResizable(true);
    wallpaperLibraryScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    wallpaperLibraryScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* wallpaperLibraryContainer = new QWidget();
    m_wallpaperLibraryGridLayout = new QGridLayout(wallpaperLibraryContainer);
    m_wallpaperLibraryGridLayout->setSpacing(10);
    m_wallpaperLibraryGridLayout->setContentsMargins(10, 10, 10, 10);
    
    wallpaperLibraryScrollArea->setWidget(wallpaperLibraryContainer);
    QVBoxLayout* wallpaperLibraryPageLayout = new QVBoxLayout(m_wallpaperLibraryPage);
    wallpaperLibraryPageLayout->setContentsMargins(0, 0, 0, 0);
    wallpaperLibraryPageLayout->addWidget(wallpaperLibraryScrollArea);
    
    // 添加页面到堆叠组件
    m_stackedWidget->addWidget(m_myWallpaperPage); // 索引0：我的壁纸
    m_stackedWidget->addWidget(m_wallpaperLibraryPage); // 索引1：壁纸库
    
    // 默认显示"我的壁纸"页面
    m_stackedWidget->setCurrentIndex(0);
    
    this->contentLayout->addWidget(m_stackedWidget);
    
    // 初始化两个页面
    loadLocalWallpapers(); // 先加载本地壁纸
    initMyWallpapers();
    fetchWallpaperLibraryFromServer(); // 从服务器获取壁纸库
    initWallpaperLibrary();
    
    // 更新按钮可见性
    updateButtonVisibility();
}

TACWallpaperLibraryDialog::~TACWallpaperLibraryDialog()
{}

void TACWallpaperLibraryDialog::init()
{
    // 这个方法保留用于兼容
}

void TACWallpaperLibraryDialog::loadLocalWallpapers()
{
    // 从本地文件加载壁纸列表
    QString configPath = m_wallpaperStorageDir + "/wallpapers.json";
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            QJsonArray array = doc.array();
            m_myWallpapers.clear();
            for (const QJsonValue& value : array) {
                if (value.isObject()) {
                    QJsonObject obj = value.toObject();
                    WallpaperInfo info;
                    info.imagePath = obj["imagePath"].toString();
                    info.name = obj["name"].toString();
                    info.isLocal = obj["isLocal"].toBool(true);
                    info.serverId = obj["serverId"].toString();
                    
                    // 检查文件是否存在
                    if (QFile::exists(info.imagePath)) {
                        m_myWallpapers.append(info);
                    }
                }
            }
        }
    }
}

void TACWallpaperLibraryDialog::saveLocalWallpapers()
{
    // 保存壁纸列表到本地文件
    QString configPath = m_wallpaperStorageDir + "/wallpapers.json";
    QJsonArray array;
    for (const WallpaperInfo& info : m_myWallpapers) {
        QJsonObject obj;
        obj["imagePath"] = info.imagePath;
        obj["name"] = info.name;
        obj["isLocal"] = info.isLocal;
        obj["serverId"] = info.serverId;
        array.append(obj);
    }
    
    QJsonDocument doc(array);
    QFile file(m_wallpaperStorageDir + "/wallpapers.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void TACWallpaperLibraryDialog::initMyWallpapers()
{
    // 清空现有布局
    QLayoutItem* item;
    while ((item = m_myWallpaperGridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    // 添加本地壁纸
    int row = 0, col = 0;
    const int colsPerRow = 4;
    
    for (const WallpaperInfo& info : m_myWallpapers) {
        TACWallpaperItemWidget* item = new TACWallpaperItemWidget(m_myWallpaperPage, false);
        item->setWallpaperInfo(info);
        item->setBackgroundImage(info.imagePath);
        item->setWallpaperName(info.name);
        connect(item, &TACWallpaperItemWidget::clicked, this, [this, item]() {
            // 取消之前选中的项
            if (m_selectedMyWallpaperItem && m_selectedMyWallpaperItem != item) {
                m_selectedMyWallpaperItem->setSelected(false);
            }
            // 切换当前项的选中状态
            item->setSelected(!item->isSelected());
            m_selectedMyWallpaperItem = item->isSelected() ? item : nullptr;
            m_setAsWallpaperButton->setEnabled(m_selectedMyWallpaperItem != nullptr);
        });
        
        m_myWallpaperGridLayout->addWidget(item, row, col, Qt::AlignLeft | Qt::AlignTop);
        col++;
        if (col >= colsPerRow) {
            col = 0;
            row++;
        }
    }
    
    // 添加"添加本地壁纸"按钮
    TACWallpaperItemWidget* addButton = new TACWallpaperItemWidget(m_myWallpaperPage, true);
    connect(addButton, &TACWallpaperItemWidget::addButtonClicked, this, &TACWallpaperLibraryDialog::onAddButtonClicked);
    m_myWallpaperGridLayout->addWidget(addButton, row, col, Qt::AlignLeft | Qt::AlignTop);
}

void TACWallpaperLibraryDialog::fetchWallpaperLibraryFromServer()
{
    // 从服务器获取壁纸库列表
    QString url = QString("http://47.100.126.194:5000/wallpaper-library");
    
    if (m_httpHandler) {
        m_httpHandler->get(url);
    }
}

void TACWallpaperLibraryDialog::onWallpaperListReceived(const QString& content)
{
    // 解析服务器返回的壁纸列表
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON response for wallpaper library";
        return;
    }
    
    QJsonObject rootObj = doc.object();
    if (!rootObj.contains("data") || !rootObj["data"].isObject()) {
        qWarning() << "Missing 'data' field in response";
        return;
    }
    
    QJsonObject dataObj = rootObj["data"].toObject();
    if (!dataObj.contains("wallpapers") || !dataObj["wallpapers"].isArray()) {
        qWarning() << "Missing 'wallpapers' array in response";
        return;
    }
    
    QJsonArray wallpapersArray = dataObj["wallpapers"].toArray();
    m_wallpaperLibrary.clear();
    
    for (const QJsonValue& value : wallpapersArray) {
        if (value.isObject()) {
            QJsonObject wallpaperObj = value.toObject();
            WallpaperInfo info;
            info.serverId = QString::number(wallpaperObj["id"].toInt());
            info.name = wallpaperObj["name"].toString();
            info.imagePath = wallpaperObj["image_url"].toString(); // 保存URL作为imagePath
            info.isLocal = false; // 服务器壁纸，不是本地
            
            m_wallpaperLibrary.append(info);
        }
    }
    
    qDebug() << "Loaded" << m_wallpaperLibrary.size() << "wallpapers from server";
    
    // 刷新壁纸库页面显示
    initWallpaperLibrary();
}

void TACWallpaperLibraryDialog::onWallpaperListFailed(const QString& content)
{
    qWarning() << "Failed to fetch wallpaper library:" << content;
    showStyledMessage(QString::fromUtf8(u8"错误"), 
        QString::fromUtf8(u8"获取壁纸库列表失败：%1").arg(content), true);
}

void TACWallpaperLibraryDialog::initWallpaperLibrary()
{
    // 清空现有布局
    QLayoutItem* item;
    while ((item = m_wallpaperLibraryGridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    // 如果壁纸库为空，从服务器获取
    if (m_wallpaperLibrary.isEmpty()) {
        fetchWallpaperLibraryFromServer();
        return; // 等待服务器响应后再显示
    }
    
    // 添加壁纸库中的壁纸
    int row = 0, col = 0;
    const int colsPerRow = 4;
    
    for (const WallpaperInfo& info : m_wallpaperLibrary) {
        TACWallpaperItemWidget* item = new TACWallpaperItemWidget(m_wallpaperLibraryPage, false);
        item->setWallpaperInfo(info);
        item->setWallpaperName(info.name);
        
        // 如果imagePath是URL，需要异步加载图片
        if (info.imagePath.startsWith("http://") || info.imagePath.startsWith("https://")) {
            // 异步加载网络图片
            loadWallpaperImageFromUrl(item, info.imagePath);
        } else {
            // 本地路径直接加载
            item->setBackgroundImage(info.imagePath);
        }
        
        connect(item, &TACWallpaperItemWidget::clicked, this, [this, item]() {
            // 取消之前选中的项
            if (m_selectedLibraryItem && m_selectedLibraryItem != item) {
                m_selectedLibraryItem->setSelected(false);
            }
            // 切换当前项的选中状态
            item->setSelected(!item->isSelected());
            m_selectedLibraryItem = item->isSelected() ? item : nullptr;
            bool hasSelection = m_selectedLibraryItem != nullptr;
            m_downloadAndSetButton->setEnabled(hasSelection);
            m_downloadButton->setEnabled(hasSelection);
        });
        
        m_wallpaperLibraryGridLayout->addWidget(item, row, col, Qt::AlignLeft | Qt::AlignTop);
        col++;
        if (col >= colsPerRow) {
            col = 0;
            row++;
        }
    }
}

void TACWallpaperLibraryDialog::loadWallpaperImageFromUrl(TACWallpaperItemWidget* item, const QString& url)
{
    // 异步加载网络图片
    QUrl imageUrl(url);
    QNetworkRequest request(imageUrl);
    QNetworkReply* reply = m_networkManager->get(request);
    
    // 使用lambda捕获item，在下载完成后设置图片
    connect(reply, &QNetworkReply::finished, [item, reply]() {
        if (reply && reply->error() == QNetworkReply::NoError) {
            QByteArray imageData = reply->readAll();
            QPixmap pixmap;
            if (pixmap.loadFromData(imageData)) {
                // 将图片保存到临时文件或直接使用QPixmap
                // 为了简化，我们直接使用QPixmap，但TACWallpaperItemWidget需要支持QPixmap
                // 暂时先尝试保存到临时文件
                QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) 
                    + "/wallpaper_" + QString::number(reinterpret_cast<quintptr>(item)) + ".jpg";
                QFile file(tempPath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(imageData);
                    file.close();
                    item->setBackgroundImage(tempPath);
                }
            }
        }
        if (reply) {
            reply->deleteLater();
        }
    });
}

void TACWallpaperLibraryDialog::onMyWallpaperButtonClicked()
{
    m_myWallpaperButton->setChecked(true);
    m_wallpaperLibraryButton->setChecked(false);
    m_stackedWidget->setCurrentIndex(0); // 切换到"我的壁纸"页面
    //this->setTitle(QString::fromUtf8(u8"我的壁纸"));
    
    // 取消壁纸库的选中
    if (m_selectedLibraryItem) {
        m_selectedLibraryItem->setSelected(false);
        m_selectedLibraryItem = nullptr;
    }
    
    updateButtonVisibility();
}

void TACWallpaperLibraryDialog::onWallpaperLibraryButtonClicked()
{
    m_myWallpaperButton->setChecked(false);
    m_wallpaperLibraryButton->setChecked(true);
    m_stackedWidget->setCurrentIndex(1); // 切换到"壁纸库"页面
    //this->setTitle(QString::fromUtf8(u8"壁纸库"));
    
    // 取消"我的壁纸"的选中
    if (m_selectedMyWallpaperItem) {
        m_selectedMyWallpaperItem->setSelected(false);
        m_selectedMyWallpaperItem = nullptr;
    }
    
    updateButtonVisibility();
}

void TACWallpaperLibraryDialog::onWallpaperItemClicked()
{
    // 这个方法可以通过信号连接调用，但实际处理在 initMyWallpapers 和 initWallpaperLibrary 的 lambda 中
}

void TACWallpaperLibraryDialog::onAddButtonClicked()
{
    // 打开文件选择对话框
    QString imagePath = QFileDialog::getOpenFileName(this, 
        QString::fromUtf8(u8"选择图片"), 
        "", 
        QString::fromUtf8(u8"图片文件 (*.png *.jpg *.jpeg *.bmp)"));
    
    if (!imagePath.isEmpty()) {
        QFileInfo fileInfo(imagePath);
        QString fileName = fileInfo.baseName();
        
        // 复制文件到壁纸存储目录
        QString destPath = m_wallpaperStorageDir + "/" + fileInfo.fileName();
        if (QFile::copy(imagePath, destPath)) {
            // 添加到"我的壁纸"列表
            WallpaperInfo info(destPath, fileName, true);
            m_myWallpapers.append(info);
            saveLocalWallpapers();
            
            // 刷新"我的壁纸"页面
            initMyWallpapers();
        } else {
            showStyledMessage(QString::fromUtf8(u8"错误"), 
                QString::fromUtf8(u8"无法复制文件到壁纸目录"), true);
        }
    }
}

void TACWallpaperLibraryDialog::onSetAsWallpaperClicked()
{
    if (!m_selectedMyWallpaperItem) {
        return;
    }
    
    WallpaperInfo info = m_selectedMyWallpaperItem->getWallpaperInfo();
    if (info.imagePath.isEmpty() || !QFile::exists(info.imagePath)) {
        showStyledMessage(QString::fromUtf8(u8"错误"), 
            QString::fromUtf8(u8"壁纸文件不存在"), true);
        return;
    }
    
    // 设置系统桌面壁纸
    QString wallpaperPath = QDir::toNativeSeparators(QFileInfo(info.imagePath).absoluteFilePath());
    
    // 使用 Windows API 设置桌面壁纸
    // 需要将路径转换为宽字符（UTF-16）
    std::wstring wpath = wallpaperPath.toStdWString();
    int result = SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (void*)wpath.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    
    if (result) {
        showStyledMessage(QString::fromUtf8(u8"成功"), 
            QString::fromUtf8(u8"桌面壁纸已设置"), false);
    } else {
        showStyledMessage(QString::fromUtf8(u8"错误"), 
            QString::fromUtf8(u8"设置桌面壁纸失败"), true);
    }
}

void TACWallpaperLibraryDialog::onDownloadAndSetClicked()
{
    if (!m_selectedLibraryItem) {
        return;
    }
    
    WallpaperInfo info = m_selectedLibraryItem->getWallpaperInfo();
    m_downloadingWallpaper = info;
    m_downloadAndSetFlag = true; // 标记为下载后设置为壁纸
    
    // 下载壁纸
    downloadWallpaper(info, true); // true 表示下载后设置为壁纸
}

void TACWallpaperLibraryDialog::onDownloadClicked()
{
    if (!m_selectedLibraryItem) {
        return;
    }
    
    WallpaperInfo info = m_selectedLibraryItem->getWallpaperInfo();
    m_downloadingWallpaper = info;
    m_downloadAndSetFlag = false; // 只下载，不设置
    
    // 下载壁纸
    downloadWallpaper(info, false); // false 表示只下载，不设置
}

void TACWallpaperLibraryDialog::downloadWallpaper(const WallpaperInfo& info, bool setAsWallpaper)
{
    // 如果壁纸已经有本地路径（已下载），直接使用
    if (info.isLocal && QFile::exists(info.imagePath)) {
        if (setAsWallpaper) {
            // 设置系统桌面壁纸
            QString wallpaperPath = QDir::toNativeSeparators(QFileInfo(info.imagePath).absoluteFilePath());
            std::wstring wpath = wallpaperPath.toStdWString();
            SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (void*)wpath.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
        }
        return;
    }
    
    // 从服务器下载壁纸（info.imagePath应该是image_url）
    QUrl url(info.imagePath);
    if (!url.isValid()) {
        showStyledMessage(QString::fromUtf8(u8"错误"), 
            QString::fromUtf8(u8"无效的壁纸URL"), true);
        return;
    }
    
    QNetworkRequest request(url);
    m_currentDownloadReply = m_networkManager->get(request);
    
    // 保存下载后是否设置为壁纸的标志和壁纸信息
    m_downloadAndSetFlag = setAsWallpaper;
    m_downloadingWallpaper = info;
    
    connect(m_currentDownloadReply, &QNetworkReply::finished, this, &TACWallpaperLibraryDialog::onWallpaperDownloadFinished);
}

void TACWallpaperLibraryDialog::onWallpaperDownloadFinished()
{
    if (!m_currentDownloadReply) {
        return;
    }
    
    if (m_currentDownloadReply->error() == QNetworkReply::NoError) {
        QByteArray imageData = m_currentDownloadReply->readAll();
        
        // 保存到本地
        QString fileName = m_downloadingWallpaper.name.isEmpty() 
            ? QString("wallpaper_%1.jpg").arg(QDateTime::currentMSecsSinceEpoch())
            : m_downloadingWallpaper.name + ".jpg";
        QString savePath = m_wallpaperStorageDir + "/" + fileName;
        
        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(imageData);
            file.close();
            
            // 添加到"我的壁纸"列表
            WallpaperInfo localInfo(savePath, m_downloadingWallpaper.name, true);
            localInfo.serverId = m_downloadingWallpaper.serverId;
            m_myWallpapers.append(localInfo);
            saveLocalWallpapers();
            
            // 刷新"我的壁纸"页面
            initMyWallpapers();
            
            // 如果设置了下载后设为壁纸，则设置系统桌面壁纸
            if (m_downloadAndSetFlag) {
                QString wallpaperPath = QDir::toNativeSeparators(QFileInfo(savePath).absoluteFilePath());
                std::wstring wpath = wallpaperPath.toStdWString();
                int result = SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (void*)wpath.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
                
                if (result) {
                    showStyledMessage(QString::fromUtf8(u8"成功"), 
                        QString::fromUtf8(u8"壁纸已下载并设置为桌面壁纸"), false);
                } else {
                    showStyledMessage(QString::fromUtf8(u8"警告"), 
                        QString::fromUtf8(u8"壁纸已下载，但设置桌面壁纸失败"), true);
                }
            } else {
                showStyledMessage(QString::fromUtf8(u8"成功"), 
                    QString::fromUtf8(u8"壁纸已下载到'我的壁纸'"), false);
            }
        } else {
            showStyledMessage(QString::fromUtf8(u8"错误"), 
                QString::fromUtf8(u8"无法保存壁纸文件"), true);
        }
    } else {
        showStyledMessage(QString::fromUtf8(u8"错误"), 
            QString::fromUtf8(u8"下载失败：%1").arg(m_currentDownloadReply->errorString()), true);
    }
    
    m_currentDownloadReply->deleteLater();
    m_currentDownloadReply = nullptr;
}

void TACWallpaperLibraryDialog::updateButtonVisibility()
{
    int currentIndex = m_stackedWidget->currentIndex();
    
    // "设为壁纸"按钮只在"我的壁纸"页面显示
    m_setAsWallpaperButton->setVisible(currentIndex == 0);
    
    // "下载并设为壁纸"和"下载"按钮只在"壁纸库"页面显示
    bool isLibraryPage = (currentIndex == 1);
    m_downloadAndSetButton->setVisible(isLibraryPage);
    m_downloadButton->setVisible(isLibraryPage);
}

// TACStyledMessageDialog 实现
TACStyledMessageDialog::TACStyledMessageDialog(const QString& title, const QString& message, QWidget* parent)
	: TABaseDialog(parent)
{
    setObjectName("TACWallpaperMessageDialog");
    setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    setBorderColor(WIDGET_BORDER_COLOR);
    setBorderWidth(WIDGET_BORDER_WIDTH);
    setRadius(40);
    setFixedSize(QSize(400, 300)); // 增加窗口高度从240到300，确保文本和按钮都有足够空间
    setTitle(title);
    
    // 创建消息内容标签
    QLabel* messageLabel = new QLabel(message, this);
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setMinimumHeight(120); // 设置最小高度，确保文本有足够显示空间
    messageLabel->setStyleSheet(
        "QLabel { "
        "color: white; "
        "font-size: 14px; "
        "padding: 20px; "
        "background: transparent; "
        "}"
    );
    
    // 创建确定按钮
    QPushButton* okButton = new QPushButton(QString::fromUtf8(u8"确定"), this);
    okButton->setFixedHeight(40); // 设置按钮固定高度
    okButton->setStyleSheet(
        "QPushButton { "
        "padding: 10px 30px; "
        "border-radius: 5px; "
        "background-color: #4169E1; "
        "color: white; "
        "font-size: 14px; "
        "min-width: 100px; "
        "}"
        "QPushButton:hover { "
        "background-color: #5A7AE8; "
        "}"
        "QPushButton:pressed { "
        "background-color: #3651C4; "
        "}"
    );
    
    // 创建布局并添加到contentLayout
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setSpacing(20);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->addWidget(messageLabel, 1, Qt::AlignCenter); // 使用stretch factor 1，让文本区域可以扩展
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    
    // 由于是继承自TABaseDialog，可以访问protected成员contentLayout
    contentLayout->addLayout(layout);
    
    // 连接确定按钮
    connect(okButton, &QPushButton::clicked, this, &TABaseDialog::close);
}

void TACWallpaperLibraryDialog::showStyledMessage(const QString& title, const QString& message, bool isError)
{
    // 创建与主窗口样式一致的消息框
    TACStyledMessageDialog* msgDialog = new TACStyledMessageDialog(title, message, this);
    
    // 居中显示
    QRect parentRect = this->geometry();
    int x = parentRect.x() + (parentRect.width() - msgDialog->width()) / 2;
    int y = parentRect.y() + (parentRect.height() - msgDialog->height()) / 2;
    msgDialog->move(x, y);
    
    msgDialog->show();
    msgDialog->raise();
    msgDialog->activateWindow();
}

// TACWallpaperItemWidget 实现
TACWallpaperItemWidget::TACWallpaperItemWidget(QWidget* parent, bool isAddButton) 
	: QWidget(parent), 
	hover(false), 
	m_selected(false), 
	m_isAddButton(isAddButton),
	m_nameLabel(nullptr)
{
    setMinimumSize(200, 150);
    setMaximumSize(200, 150);
    
    if (m_isAddButton) {
        // 添加按钮：显示+号和文字
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(5);
        
        QLabel* plusLabel = new QLabel("+", this);
        plusLabel->setAlignment(Qt::AlignCenter);
        plusLabel->setStyleSheet("font-size: 48px; color: white; background: transparent;");
        layout->addWidget(plusLabel);
        
        QLabel* textLabel = new QLabel(QString::fromUtf8(u8"添加本地壁纸"), this);
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setStyleSheet("font-size: 12px; color: white; background: transparent;");
        layout->addWidget(textLabel);
        
        setStyleSheet("background-color: #3C3C3C; border-radius: 15px;");
    } else {
        // 普通壁纸项：显示图片和名称
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        
        m_nameLabel = new QLabel(this);
        m_nameLabel->setAlignment(Qt::AlignCenter);
        m_nameLabel->setStyleSheet("font-size: 12px; color: white; background: rgba(0,0,0,128); padding: 2px;");
        m_nameLabel->setWordWrap(true);
        layout->addWidget(m_nameLabel);
    }
}

void TACWallpaperItemWidget::setWallpaperInfo(const WallpaperInfo& info)
{
    m_wallpaperInfo = info;
    if (m_nameLabel && !info.name.isEmpty()) {
        m_nameLabel->setText(info.name);
    }
}

void TACWallpaperItemWidget::setBackgroundImage(const QString& path)
{
    if (backgroundImage.load(path)) {
        update();
    }
}

void TACWallpaperItemWidget::setWallpaperName(const QString& name)
{
    m_wallpaperInfo.name = name;
    if (m_nameLabel) {
        m_nameLabel->setText(name);
    }
}

void TACWallpaperItemWidget::setSelected(bool selected)
{
    m_selected = selected;
        update();
    }
    
void TACWallpaperItemWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_isAddButton) {
            emit addButtonClicked();
        } else {
            emit clicked();
        }
    }
    QWidget::mousePressEvent(event);
}

void TACWallpaperItemWidget::paintEvent(QPaintEvent*)
{
    if (m_isAddButton) {
        // 添加按钮不需要绘制图片
        return;
    }
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    QRect rectArea = rect().adjusted(0, 0, -1, -1);
    path.addRoundedRect(rectArea, 15, 15);
    painter.setClipPath(path);
    
    // 绘制背景图片
    if (!backgroundImage.isNull()) {
        painter.drawPixmap(rect(), backgroundImage.scaled(rect().size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        painter.fillRect(rect(), QColor(60, 60, 60));
    }
    
    // 绘制边框
    if (m_selected) {
        painter.setPen(QPen(QColor(65, 105, 225), 3)); // 蓝色边框，选中状态
    } else if (hover) {
        painter.setPen(QPen(QColor(255, 255, 255), 2)); // 白色边框，悬停状态
    } else {
        painter.setPen(QPen(Qt::NoPen));
    }
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 15, 15);
}

void TACWallpaperItemWidget::enterEvent(QEvent* event)
{
    hover = true;
    update();
    QWidget::enterEvent(event);
}

void TACWallpaperItemWidget::leaveEvent(QEvent* event)
{
    hover = false;
    update();
    QWidget::leaveEvent(event);
}
