#pragma once

#include "TABaseDialog.h"
#include <QStackedWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QFileInfo>
#include <QDir>
#include <QList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include "../Common/TAHttpHandler.h"

// 壁纸信息结构
struct WallpaperInfo {
	QString imagePath;  // 图片路径
	QString name;       // 壁纸名称
	bool isLocal;       // 是否为本地壁纸
	QString serverId;   // 服务器ID（如果是服务器壁纸）
	
	WallpaperInfo() : isLocal(true) {}
	WallpaperInfo(const QString& path, const QString& n, bool local = true) 
		: imagePath(path), name(n), isLocal(local) {}
};

// 自定义消息框（继承自TABaseDialog以访问protected成员）
class TACStyledMessageDialog : public TABaseDialog
{
	Q_OBJECT
public:
	TACStyledMessageDialog(const QString& title, const QString& message, QWidget* parent = nullptr);
};

class TACWallpaperItemWidget : public QWidget
{
	Q_OBJECT
public:
	explicit TACWallpaperItemWidget(QWidget* parent = nullptr, bool isAddButton = false);
	void setWallpaperInfo(const WallpaperInfo& info);
	void setBackgroundImage(const QString& path);
	void setWallpaperName(const QString& name);
	void setSelected(bool selected);
	bool isSelected() const { return m_selected; }
	WallpaperInfo getWallpaperInfo() const { return m_wallpaperInfo; }
	
signals:
	void clicked(); // 点击信号
	void addButtonClicked(); // 添加按钮点击信号（仅当isAddButton为true时）
	
protected:
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent* event) override;
	
private:
	bool hover;
	bool m_selected;
	bool m_isAddButton; // 是否为"添加"按钮
	QPixmap backgroundImage;
	WallpaperInfo m_wallpaperInfo;
	QLabel* m_nameLabel; // 名称标签
};

class TACWallpaperLibraryDialog  : public TABaseDialog
{
	Q_OBJECT

public:
	TACWallpaperLibraryDialog(QWidget *parent);
	~TACWallpaperLibraryDialog();
	void init();
	void initMyWallpapers(); // 初始化"我的壁纸"页面
	void initWallpaperLibrary(); // 初始化"壁纸库"页面
	void loadLocalWallpapers(); // 加载本地保存的壁纸
	void saveLocalWallpapers(); // 保存壁纸列表到本地
	void fetchWallpaperLibraryFromServer(); // 从服务器获取壁纸库
	
private slots:
	void onMyWallpaperButtonClicked(); // 切换到"我的壁纸"页面
	void onWallpaperLibraryButtonClicked(); // 切换到"壁纸库"页面
	void onWallpaperItemClicked(); // 壁纸项点击
	void onAddButtonClicked(); // 添加本地壁纸按钮点击
	void onSetAsWallpaperClicked(); // "设为壁纸"按钮点击
	void onDownloadAndSetClicked(); // "下载并设为壁纸"按钮点击
	void onDownloadClicked(); // "下载"按钮点击
	void updateButtonVisibility(); // 根据当前页面更新按钮可见性
	void onWallpaperDownloadFinished(); // 壁纸下载完成
	void downloadWallpaper(const WallpaperInfo& info, bool setAsWallpaper); // 下载壁纸
	void onWallpaperListReceived(const QString& content); // 壁纸列表获取成功
	void onWallpaperListFailed(const QString& content); // 壁纸列表获取失败
	void loadWallpaperImageFromUrl(TACWallpaperItemWidget* item, const QString& url); // 从URL加载壁纸图片
	void showStyledMessage(const QString& title, const QString& message, bool isError = false); // 显示样式一致的消息框
	
private:
	QStackedWidget* m_stackedWidget; // 页面堆叠组件
	QPushButton* m_myWallpaperButton; // "我的壁纸"按钮
	QPushButton* m_wallpaperLibraryButton; // "壁纸库"按钮
	QPushButton* m_setAsWallpaperButton; // "设为壁纸"按钮（我的壁纸页面）
	QPushButton* m_downloadAndSetButton; // "下载并设为壁纸"按钮（壁纸库页面）
	QPushButton* m_downloadButton; // "下载"按钮（壁纸库页面）
	QWidget* m_myWallpaperPage; // "我的壁纸"页面
	QWidget* m_wallpaperLibraryPage; // "壁纸库"页面
	QGridLayout* m_myWallpaperGridLayout; // "我的壁纸"网格布局
	QGridLayout* m_wallpaperLibraryGridLayout; // "壁纸库"网格布局
	TACWallpaperItemWidget* m_selectedMyWallpaperItem; // 选中的"我的壁纸"项
	TACWallpaperItemWidget* m_selectedLibraryItem; // 选中的"壁纸库"项
	QList<WallpaperInfo> m_myWallpapers; // 我的壁纸列表
	QList<WallpaperInfo> m_wallpaperLibrary; // 壁纸库列表
	QString m_wallpaperStorageDir; // 壁纸存储目录
	QNetworkAccessManager* m_networkManager; // 网络管理器（用于下载壁纸）
	QNetworkReply* m_currentDownloadReply; // 当前下载的回复
	WallpaperInfo m_downloadingWallpaper; // 正在下载的壁纸信息
	bool m_downloadAndSetFlag; // 下载后是否设置为壁纸的标志
	TAHttpHandler* m_httpHandler; // HTTP处理器（用于获取壁纸库列表）
	bool m_wallpaperLibraryFetchInFlight = false; // 是否正在请求壁纸库
	bool m_wallpaperLibraryFetchedOnce = false;   // 是否至少成功请求过一次（哪怕返回空）
};
