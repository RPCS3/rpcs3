#pragma once

#include <QDialog>
#include <QKeyEvent>

class custom_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit custom_dialog(bool disableCancel, QWidget* parent = nullptr);
	bool m_disable_cancel;

private:
	void keyPressEvent(QKeyEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
};
