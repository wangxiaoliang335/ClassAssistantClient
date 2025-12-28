#pragma once

#include <QDialog>

class QLabel;
class QTextEdit;
class QPushButton;

// 课前准备编辑对话框（无系统标题栏、圆角半透明背景）
class PrepareClassEditDialog final : public QDialog
{
public:
	explicit PrepareClassEditDialog(QWidget* parent = nullptr);

	void setHeaderText(const QString& text);
	void setInitialContent(const QString& content);
	QString content() const;

protected:
	void paintEvent(QPaintEvent* event) override;
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	void applyUi();
	void onCloseClicked();
	void updateCloseButtonPos();

private:
	QPushButton* m_closeButton = nullptr;
	QLabel* m_headerLabel = nullptr;
	QTextEdit* m_textEdit = nullptr;

	QString m_content;

	bool m_dragging = false;
	QPoint m_dragStartPos;
};


