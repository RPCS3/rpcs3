#pragma once
#include <QDialog>

class QLineEdit;
class QCheckBox;
class QPushButton;
class QLabel;

class ra_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit ra_settings_dialog(QWidget* parent = nullptr);

private Q_SLOTS:
	void on_login();
	void on_logout();
	void update_ui_state();

private:
	QCheckBox* m_enabled = nullptr;
	QCheckBox* m_hardcore = nullptr;
	QLineEdit* m_username = nullptr;
	QLineEdit* m_password = nullptr;
	QPushButton* m_login_button = nullptr;
	QPushButton* m_logout_button = nullptr;
	QLabel* m_status_label = nullptr;
};
