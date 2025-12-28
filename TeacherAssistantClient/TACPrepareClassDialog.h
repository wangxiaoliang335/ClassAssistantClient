#pragma once

#include "TABaseDialog.h"
#include <QPointer>

class QTextEdit;
class QWidget;
class QLabel;

class TACPrepareClassDialog : public TABaseDialog
{
	Q_OBJECT

public:
	TACPrepareClassDialog(QWidget *parent);
	~TACPrepareClassDialog();

protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QLabel* m_headerLabel = nullptr;
    QTextEdit* m_contentEdit = nullptr;
};
