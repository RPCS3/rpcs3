#include "stdafx.h"
#include "ra_settings_dialog.h"
#include "Emu/RetroAchievements.h"
#include "Emu/ra_config.h"

#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMetaObject>

ra_settings_dialog::ra_settings_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("RetroAchievements"));
	setMinimumWidth(400);

	g_cfg_ra.load();

	QGroupBox* settings_group = new QGroupBox(tr("Settings"), this);
	QVBoxLayout* settings_layout = new QVBoxLayout(settings_group);

	m_enabled = new QCheckBox(tr("Enable RetroAchievements"), this);
	m_enabled->setChecked(g_cfg_ra.enabled.get());
	settings_layout->addWidget(m_enabled);

	m_hardcore = new QCheckBox(tr("Hardcore Mode"), this);
	m_hardcore->setChecked(g_cfg_ra.hardcore.get());
	settings_layout->addWidget(m_hardcore);

	QGroupBox* login_group = new QGroupBox(tr("Account"), this);
	QFormLayout* login_layout = new QFormLayout(login_group);

	m_username = new QLineEdit(this);
	m_username->setText(QString::fromStdString(g_cfg_ra.username.get()));
	m_username->setPlaceholderText(tr("Username"));
	login_layout->addRow(tr("Username:"), m_username);

	m_password = new QLineEdit(this);
	m_password->setEchoMode(QLineEdit::Password);
	m_password->setPlaceholderText(tr("Password"));
	login_layout->addRow(tr("Password:"), m_password);

	m_status_label = new QLabel(tr("Not logged in."), this);
	m_status_label->setWordWrap(true);
	login_layout->addRow(m_status_label);

	QHBoxLayout* button_layout = new QHBoxLayout();
	m_login_button = new QPushButton(tr("Login"), this);
	m_logout_button = new QPushButton(tr("Logout"), this);
	button_layout->addWidget(m_login_button);
	button_layout->addWidget(m_logout_button);
	button_layout->addStretch();

	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Close, this);

	QVBoxLayout* main_layout = new QVBoxLayout(this);
	main_layout->addWidget(settings_group);
	main_layout->addWidget(login_group);
	main_layout->addLayout(button_layout);
	main_layout->addWidget(button_box);
	setLayout(main_layout);

	connect(m_login_button, &QPushButton::clicked, this, &ra_settings_dialog::on_login);
	connect(m_logout_button, &QPushButton::clicked, this, &ra_settings_dialog::on_logout);
	connect(button_box, &QDialogButtonBox::rejected, this, &ra_settings_dialog::close);
	connect(m_enabled, &QCheckBox::toggled, this, [this](bool checked)
	{
		g_cfg_ra.enabled.set(checked);
		g_cfg_ra.save();
	});
	connect(m_hardcore, &QCheckBox::toggled, this, [this](bool checked)
	{
		g_cfg_ra.hardcore.set(checked);
		g_cfg_ra.save();
	});

	update_ui_state();
}

void ra_settings_dialog::on_login()
{
	const std::string username = m_username->text().toStdString();
	const std::string password = m_password->text().toStdString();

	if (username.empty() || password.empty())
	{
		m_status_label->setText(tr("Please enter your username and password."));
		return;
	}

	m_login_button->setEnabled(false);
	m_status_label->setText(tr("Logging in..."));

	rpcs3::ra::login(username, password, [this, username](bool success)
	{
		QMetaObject::invokeMethod(this, [this, username, success]()
		{
			if (success)
			{
				g_cfg_ra.username.set(username);
				g_cfg_ra.token.set(rpcs3::ra::get_token());
				g_cfg_ra.enabled.set(true);
				m_enabled->setChecked(true);
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
	g_cfg_ra.enabled.set(false);
	m_enabled->setChecked(false);
	g_cfg_ra.save();
	m_password->clear();
	update_ui_state();
}

void ra_settings_dialog::update_ui_state()
{
	const bool logged_in = rpcs3::ra::is_active();
	m_username->setEnabled(!logged_in);
	m_password->setEnabled(!logged_in);
	m_login_button->setEnabled(!logged_in);
	m_logout_button->setEnabled(logged_in);

	if (logged_in)
		m_status_label->setText(tr("Logged in as: %1").arg(
			QString::fromStdString(g_cfg_ra.username.get())));
	else
		m_status_label->setText(tr("Not logged in."));
}
