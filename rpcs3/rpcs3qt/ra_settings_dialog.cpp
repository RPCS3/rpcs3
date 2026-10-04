#include "stdafx.h"
#include "ra_settings_dialog.h"
#include "Emu/RetroAchievements.h"
#include "Emu/ra_config.h"
#include "Emu/System.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

static QLabel* make_section_header(const QString& text, QWidget* parent)
{
	QLabel* label = new QLabel(text, parent);
	label->setForegroundRole(QPalette::Link);
	QFont font = label->font();
	font.setBold(true);
	label->setFont(font);
	return label;
}

static QFrame* make_separator(QWidget* parent)
{
	QFrame* line = new QFrame(parent);
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	return line;
}

ra_settings_dialog::ra_settings_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("RetroAchievements"));
	setMinimumWidth(380);

	g_cfg_ra.load();

	m_network = new QNetworkAccessManager(this);
	connect(m_network, &QNetworkAccessManager::finished, this, &ra_settings_dialog::on_avatar_downloaded);

	QVBoxLayout* main_layout = new QVBoxLayout(this);
	main_layout->setSpacing(6);

	// ── User card (visible only when logged in) ──────────────────────────────
	m_user_card = new QFrame(this);
	m_user_card->setFrameShape(QFrame::StyledPanel);
	m_user_card->setFrameShadow(QFrame::Raised);
	QHBoxLayout* card_layout = new QHBoxLayout(m_user_card);
	card_layout->setContentsMargins(8, 8, 8, 8);

	m_avatar_label = new QLabel(this);
	m_avatar_label->setFixedSize(48, 48);
	m_avatar_label->setAlignment(Qt::AlignCenter);
	m_avatar_label->setFrameShape(QFrame::Box);
	m_avatar_label->setText("...");

	m_user_label = new QLabel(this);
	m_user_label->setWordWrap(false);
	QFont user_font = m_user_label->font();
	user_font.setBold(true);
	user_font.setPointSize(user_font.pointSize() + 1);
	m_user_label->setFont(user_font);

	m_score_label = new QLabel(this);
	m_score_label->setWordWrap(false);
	m_score_label->setForegroundRole(QPalette::Mid);

	QVBoxLayout* user_info_layout = new QVBoxLayout();
	user_info_layout->setSpacing(2);
	user_info_layout->addWidget(m_user_label);
	user_info_layout->addWidget(m_score_label);

	card_layout->addWidget(m_avatar_label);
	card_layout->addSpacing(8);
	card_layout->addLayout(user_info_layout);
	card_layout->addStretch();
	main_layout->addWidget(m_user_card);

	main_layout->addWidget(make_separator(this));

	// ── Account ──────────────────────────────────────────────────────────────
	main_layout->addWidget(make_section_header(tr("Account"), this));

	m_username_label = new QLabel(tr("Username:"), this);
	m_username = new QLineEdit(this);
	m_username->setText(QString::fromStdString(g_cfg_ra.username.get()));
	m_username->setPlaceholderText(tr("Username"));

	m_password_label = new QLabel(tr("Password:"), this);
	m_password = new QLineEdit(this);
	m_password->setEchoMode(QLineEdit::Password);
	m_password->setPlaceholderText(tr("Password"));

	m_login_button  = new QPushButton(tr("Login"), this);
	m_logout_button = new QPushButton(tr("Logout"), this);

	main_layout->addWidget(m_username_label);
	main_layout->addWidget(m_username);
	main_layout->addWidget(m_password_label);
	main_layout->addWidget(m_password);
	main_layout->addWidget(m_login_button);
	main_layout->addWidget(m_logout_button);

	main_layout->addWidget(make_separator(this));

	// ── Behavior Settings ────────────────────────────────────────────────────
	main_layout->addWidget(make_section_header(tr("Behavior Settings"), this));

	m_hardcore        = new QCheckBox(tr("Hardcore Mode"), this);
	m_unofficial      = new QCheckBox(tr("Unofficial Achievements"), this);
	m_encore          = new QCheckBox(tr("Encore Mode"), this);
	m_spectator       = new QCheckBox(tr("Spectator Mode"), this);
	m_native_trophies = new QCheckBox(tr("Enable Native PS3 Trophies"), this);

	m_hardcore->setChecked(g_cfg_ra.hardcore.get());
	m_unofficial->setChecked(g_cfg_ra.unofficial.get());
	m_encore->setChecked(g_cfg_ra.encore.get());
	m_spectator->setChecked(g_cfg_ra.spectator.get());
	m_native_trophies->setChecked(g_cfg_ra.native_trophies.get());

	m_hardcore->setToolTip(tr(
		"Enables Hardcore Mode.\n\n"
		"In Hardcore Mode, features that give an advantage over console players are disabled:\n"
		"  - Loading save states\n"
		"  - Emulator speeds below 100%\n"
		"  - Cheats and memory patches\n\n"
		"Leaderboards require Hardcore Mode. RA rankings also emphasize Hardcore points.\n\n"
		"Enabling while a game is running will restart the game.\n"
		"Disabling takes effect immediately. No restart required."));
	m_unofficial->setToolTip(tr(
		"Enables Unofficial Achievements.\n\n"
		"Unofficial achievements are optional or unfinished achievements not yet approved by RetroAchievements.\n"
		"Useful for testing or simply for fun.\n\n"
		"Changing this setting while a game is running will restart the game."));
	m_encore->setToolTip(tr(
		"Enables Encore Mode.\n\n"
		"Re-enables achievements you have already unlocked, so you will be notified again\n"
		"if you meet the unlock conditions. Useful for custom speedrun criteria or practice.\n\n"
		"Changing this setting while a game is running will restart the game."));
	m_spectator->setToolTip(tr(
		"Enables Spectator Mode.\n\n"
		"Achievements and leaderboards are processed and displayed on screen,\n"
		"but no results are submitted to the server.\n\n"
		"Takes effect immediately. No restart required."));
	m_native_trophies->setToolTip(tr(
		"Enables native PS3 trophy unlocking alongside RetroAchievements.\n\n"
		"When enabled, the game will unlock PS3 trophies and show trophy popups\n"
		"as it normally would.\n\n"
		"Disable this if you want RetroAchievements to be your only achievement\n"
		"system: no trophy data will be written and no trophy popups will appear.\n\n"
		"Takes effect immediately. No restart required."));

	main_layout->addWidget(m_hardcore);
	main_layout->addWidget(m_unofficial);
	main_layout->addWidget(m_encore);
	main_layout->addWidget(m_spectator);
	main_layout->addWidget(m_native_trophies);

	main_layout->addWidget(make_separator(this));

	// ── Display Settings ─────────────────────────────────────────────────────
	main_layout->addWidget(make_section_header(tr("Display Settings"), this));

	m_discord                = new QCheckBox(tr("Discord Presence"), this);
	m_leaderboard_trackers   = new QCheckBox(tr("Leaderboard Trackers"), this);
	m_challenge_indicators   = new QCheckBox(tr("Challenge Indicators"), this);
	m_progress_notifications = new QCheckBox(tr("Progress Notifications"), this);

	m_discord->setChecked(g_cfg_ra.discord.get());
	m_leaderboard_trackers->setChecked(g_cfg_ra.leaderboard_trackers.get());
	m_challenge_indicators->setChecked(g_cfg_ra.challenge_indicators.get());
	m_progress_notifications->setChecked(g_cfg_ra.progress_notifications.get());

	m_discord->setToolTip(tr(
		"Shows RetroAchievements rich presence in your Discord status.\n\n"
		"Requires 'Use Discord Rich Presence' enabled in Configuration → GUI → Discord.\n"
		"Updates every 10 seconds while a game is running.\n\n"
		"Takes effect immediately. No restart required."));
	m_leaderboard_trackers->setToolTip(tr(
		"Shows on-screen leaderboard trackers.\n\n"
		"A tracker appears in the corner while you are competing in a leaderboard,\n"
		"displaying your current score in real time.\n\n"
		"Takes effect immediately. No restart required."));
	m_challenge_indicators->setToolTip(tr(
		"Shows on-screen challenge indicators.\n\n"
		"Challenge badges appear while you are in the process of meeting\n"
		"the conditions for a challenge achievement.\n\n"
		"Takes effect immediately. No restart required."));
	m_progress_notifications->setToolTip(tr(
		"Enables progress notifications.\n\n"
		"Displays a brief popup whenever you make progress on an achievement\n"
		"that tracks an accumulated value (e.g. 60 out of 120).\n\n"
		"Takes effect immediately. No restart required."));

	main_layout->addWidget(m_discord);
	main_layout->addWidget(m_leaderboard_trackers);
	main_layout->addWidget(m_challenge_indicators);
	main_layout->addWidget(m_progress_notifications);

	// ── Close button ─────────────────────────────────────────────────────────
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Close, this);
	main_layout->addWidget(button_box);

	setLayout(main_layout);

	connect(m_login_button, &QPushButton::clicked, this, &ra_settings_dialog::on_login);
	connect(m_logout_button, &QPushButton::clicked, this, &ra_settings_dialog::on_logout);
	connect(button_box, &QDialogButtonBox::rejected, this, &ra_settings_dialog::close);

	connect(m_hardcore, &QCheckBox::toggled, this, [this](bool checked)
	{
		if (checked && !Emu.IsStopped())
		{
			const auto result = QMessageBox::question(this,
				tr("Hardcore Mode"),
				tr("Enabling Hardcore Mode will restart the game.\nUnsaved progress will be lost.\n\nContinue?"),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No);
			if (result == QMessageBox::No)
			{
				QSignalBlocker blocker(m_hardcore);
				m_hardcore->setChecked(false);
				return;
			}
		}
		g_cfg_ra.hardcore.set(checked);
		g_cfg_ra.save();
		rpcs3::ra::set_hardcore(checked);
		if (checked && rpcs3::ra::consume_pending_hc_restart())
			Emu.Restart(false);
	});
	connect(m_unofficial, &QCheckBox::toggled, this, [this](bool checked)
	{
		if (!Emu.IsStopped())
		{
			const auto result = QMessageBox::question(this,
				tr("Unofficial Achievements"),
				tr("Changing this setting requires reloading the game.\nUnsaved progress will be lost.\n\nContinue?"),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No);
			if (result == QMessageBox::No)
			{
				QSignalBlocker blocker(m_unofficial);
				m_unofficial->setChecked(!checked);
				return;
			}
		}
		g_cfg_ra.unofficial.set(checked);
		g_cfg_ra.save();
		rpcs3::ra::set_unofficial(checked);
		if (!Emu.IsStopped())
			Emu.Restart(false);
	});
	connect(m_encore, &QCheckBox::toggled, this, [this](bool checked)
	{
		if (!Emu.IsStopped())
		{
			const auto result = QMessageBox::question(this,
				tr("Encore Mode"),
				tr("Changing this setting requires reloading the game.\nUnsaved progress will be lost.\n\nContinue?"),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No);
			if (result == QMessageBox::No)
			{
				QSignalBlocker blocker(m_encore);
				m_encore->setChecked(!checked);
				return;
			}
		}
		g_cfg_ra.encore.set(checked);
		g_cfg_ra.save();
		rpcs3::ra::set_encore(checked);
		if (!Emu.IsStopped())
			Emu.Restart(false);
	});
	connect(m_spectator, &QCheckBox::toggled, this, [this](bool checked)
	{
		g_cfg_ra.spectator.set(checked);
		g_cfg_ra.save();
		rpcs3::ra::set_spectator(checked);
		if (checked && !Emu.IsStopped())
			QMessageBox::information(this,
				tr("Spectator Mode"),
				tr("Spectator Mode is now active.\nAchievements and leaderboards will be processed but not submitted to the server."));
	});
	connect(m_native_trophies, &QCheckBox::toggled, this, [this](bool checked)
	{
		g_cfg_ra.native_trophies.set(checked);
		g_cfg_ra.save();
		if (!Emu.IsStopped())
			QMessageBox::information(this,
				tr("Native PS3 Trophies"),
				checked ? tr("Native PS3 trophies are now enabled.\nThe game will write trophy data and show trophy popups.")
				        : tr("Native PS3 trophies are now disabled.\nTrophy unlocks will be silently accepted without writing data or showing popups."));
	});
	connect(m_discord, &QCheckBox::toggled, this, [this](bool checked)
	{
		g_cfg_ra.discord.set(checked);
		g_cfg_ra.save();
		if (!Emu.IsStopped())
			QMessageBox::information(this,
				tr("Discord Rich Presence"),
				checked ? tr("Discord Rich Presence is now enabled.\nYour status will be updated with RetroAchievements rich presence every 10 seconds.")
				        : tr("Discord Rich Presence is now disabled."));
	});
	connect(m_leaderboard_trackers, &QCheckBox::toggled, this, [this](bool checked)
	{
		g_cfg_ra.leaderboard_trackers.set(checked);
		g_cfg_ra.save();
		if (!Emu.IsStopped())
			QMessageBox::information(this,
				tr("Leaderboard Trackers"),
				checked ? tr("Leaderboard trackers are now enabled.")
				        : tr("Leaderboard trackers are now disabled."));
	});
	connect(m_challenge_indicators, &QCheckBox::toggled, this, [this](bool checked)
	{
		g_cfg_ra.challenge_indicators.set(checked);
		g_cfg_ra.save();
		if (!Emu.IsStopped())
			QMessageBox::information(this,
				tr("Challenge Indicators"),
				checked ? tr("Challenge indicators are now enabled.")
				        : tr("Challenge indicators are now disabled."));
	});
	connect(m_progress_notifications, &QCheckBox::toggled, this, [this](bool checked)
	{
		g_cfg_ra.progress_notifications.set(checked);
		g_cfg_ra.save();
		if (!Emu.IsStopped())
			QMessageBox::information(this,
				tr("Progress Notifications"),
				checked ? tr("Progress notifications are now enabled.")
				        : tr("Progress notifications are now disabled."));
	});

	update_ui_state();
}

void ra_settings_dialog::on_login()
{
	const std::string username = m_username->text().toStdString();
	const std::string password = m_password->text().toStdString();

	if (username.empty() || password.empty())
		return;

	m_login_button->setEnabled(false);

	rpcs3::ra::login(username, password, [this, username](bool success)
	{
		QMetaObject::invokeMethod(this, [this, username, success]()
		{
			if (success)
			{
				g_cfg_ra.username.set(username);
				g_cfg_ra.token.set(rpcs3::ra::get_token());
				g_cfg_ra.save();
			}
			update_ui_state();
		}, Qt::QueuedConnection);
	});
}

void ra_settings_dialog::on_logout()
{
	rpcs3::ra::logout();
	g_cfg_ra.token.set("");
	g_cfg_ra.save();
	m_password->clear();
	update_ui_state();
}

void ra_settings_dialog::fetch_avatar(const QString& username)
{
	const QString url = QString("https://media.retroachievements.org/UserPic/%1.png").arg(username);
	m_network->get(QNetworkRequest{QUrl(url)});
}

void ra_settings_dialog::on_avatar_downloaded(QNetworkReply* reply)
{
	reply->deleteLater();
	if (reply->error() != QNetworkReply::NoError)
		return;

	QPixmap pixmap;
	if (pixmap.loadFromData(reply->readAll()))
		m_avatar_label->setPixmap(pixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void ra_settings_dialog::update_ui_state()
{
	const bool logged_in = rpcs3::ra::is_active();

	m_user_card->setVisible(logged_in);
	if (logged_in)
	{
		const QString username = QString::fromStdString(g_cfg_ra.username.get());
		m_user_label->setText(username);
		m_score_label->setText(tr("%1 points").arg(rpcs3::ra::get_user_score()));
		m_avatar_label->setText("...");
		fetch_avatar(username);
	}

	m_username_label->setVisible(!logged_in);
	m_username->setVisible(!logged_in);

	m_password_label->setVisible(!logged_in);
	m_password->setVisible(!logged_in);

	m_login_button->setVisible(!logged_in);
	m_login_button->setEnabled(true);

	m_logout_button->setVisible(logged_in);
}
