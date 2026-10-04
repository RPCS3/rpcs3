#pragma once
#include <QDialog>

class QCheckBox;
class QFrame;
class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;

class ra_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit ra_settings_dialog(QWidget* parent = nullptr);

private Q_SLOTS:
	void on_login();
	void on_logout();
	void update_ui_state();
	void on_avatar_downloaded(QNetworkReply* reply);

private:
	void fetch_avatar(const QString& username);

	// User card (logged-in only)
	QFrame*      m_user_card       = nullptr;
	QLabel*      m_avatar_label    = nullptr;
	QLabel*      m_user_label      = nullptr;
	QLabel*      m_score_label     = nullptr;

	// Login form (logged-out only)
	QLabel*      m_username_label  = nullptr;
	QLabel*      m_password_label  = nullptr;
	QLineEdit*   m_username        = nullptr;
	QLineEdit*   m_password        = nullptr;
	QPushButton* m_login_button    = nullptr;
	QPushButton* m_logout_button   = nullptr;

	// Behavior Settings
	QCheckBox*   m_hardcore              = nullptr;
	QCheckBox*   m_unofficial            = nullptr;
	QCheckBox*   m_encore                = nullptr;
	QCheckBox*   m_spectator             = nullptr;
	QCheckBox*   m_native_trophies       = nullptr;

	// Display Settings
	QCheckBox*   m_discord               = nullptr;
	QCheckBox*   m_leaderboard_trackers  = nullptr;
	QCheckBox*   m_challenge_indicators  = nullptr;
	QCheckBox*   m_progress_notifications = nullptr;

	QNetworkAccessManager* m_network = nullptr;
};
