#include <QMessageBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegExpValidator>
#include <QInputDialog>
#include <thread>

#include "rpcn_settings_dialog.h"
#include "Emu/NP/rpcn_config.h"
#include "Emu/NP/rpcn_client.h"

#include <wolfssl/ssl.h>
#include <wolfssl/openssl/evp.h>

rpcn_settings_dialog::rpcn_settings_dialog(QWidget* parent)
    : QDialog(parent)
{
	setWindowTitle(tr("RPCN Configuration"));
	setObjectName("rpcn_settings_dialog");
	setMinimumSize(QSize(400, 200));

	QVBoxLayout* vbox_global           = new QVBoxLayout();
	QHBoxLayout* hbox_labels_and_edits = new QHBoxLayout();
	QVBoxLayout* vbox_labels           = new QVBoxLayout();
	QVBoxLayout* vbox_edits            = new QVBoxLayout();
	QHBoxLayout* hbox_buttons          = new QHBoxLayout();

	QLabel* label_host = new QLabel(tr("Host:"));
	m_edit_host        = new QLineEdit();
	QLabel* label_npid = new QLabel(tr("NPID (username):"));
	m_edit_npid        = new QLineEdit();
	m_edit_npid->setMaxLength(16);
	m_edit_npid->setValidator(new QRegExpValidator(QRegExp("^[a-zA-Z0-9_\\-]*$"), this));
	QLabel* label_pass        = new QLabel(tr("Password:"));
	QPushButton* btn_chg_pass = new QPushButton(tr("Set Password"));
	QLabel *label_token       = new QLabel(tr("Token:"));
	m_edit_token              = new QLineEdit();
	m_edit_token->setMaxLength(16);

	QPushButton* btn_create = new QPushButton(tr("Create Account"), this);
	QPushButton* btn_save     = new QPushButton(tr("Save"), this);

	vbox_labels->addWidget(label_host);
	vbox_labels->addWidget(label_npid);
	vbox_labels->addWidget(label_pass);
	vbox_labels->addWidget(label_token);

	vbox_edits->addWidget(m_edit_host);
	vbox_edits->addWidget(m_edit_npid);
	vbox_edits->addWidget(btn_chg_pass);
	vbox_edits->addWidget(m_edit_token);

	hbox_buttons->addWidget(btn_create);
	hbox_buttons->addStretch();
	hbox_buttons->addWidget(btn_save);

	hbox_labels_and_edits->addLayout(vbox_labels);
	hbox_labels_and_edits->addLayout(vbox_edits);

	vbox_global->addLayout(hbox_labels_and_edits);
	vbox_global->addLayout(hbox_buttons);

	setLayout(vbox_global);

	connect(btn_chg_pass, &QAbstractButton::clicked, [this]()
	{
		QString password;

		while (true)
		{
			bool clicked_ok = false;
			password = QInputDialog::getText(this, "Set/Change Password", "Set your password", QLineEdit::Password, "", &clicked_ok);
			if (!clicked_ok)
				return;

			if (password.isEmpty())
			{
				QMessageBox::critical(this, tr("Wrong input"), tr("You need to enter a password!"), QMessageBox::Ok);
			}
			else
			{
				break;
			}
		}

		const std::string pass_str = password.toStdString();
		const std::string salt_str = "No matter where you go, everybody's connected.";

		u8 salted_pass[SHA_DIGEST_SIZE];

		wolfSSL_PKCS5_PBKDF2_HMAC_SHA1(pass_str.c_str(), pass_str.size(), reinterpret_cast<const u8*>(salt_str.c_str()), salt_str.size(), 1000, SHA_DIGEST_SIZE, salted_pass);

		std::string hash("0000000000000000000000000000000000000000");
		for (u32 i = 0; i < 20; i++)
		{
			constexpr auto pal = "0123456789abcdef";
			hash[i * 2] = pal[salted_pass[i] >> 4];
			hash[1 + i * 2] = pal[salted_pass[i] & 15];
		}

		g_cfg_rpcn.set_password(hash);
		g_cfg_rpcn.save();
	});

	connect(btn_save, &QAbstractButton::clicked, [this]()
	{
		if (this->save_config())
			this->close();
	});
	connect(btn_create, &QAbstractButton::clicked, [this]() { this->create_account(); });

	g_cfg_rpcn.load();

	m_edit_host->setText(QString::fromStdString(g_cfg_rpcn.get_host()));
	m_edit_npid->setText(QString::fromStdString(g_cfg_rpcn.get_npid()));
	m_edit_token->setText(QString::fromStdString(g_cfg_rpcn.get_token()));
}

bool rpcn_settings_dialog::save_config()
{
	const auto host  = m_edit_host->text().toStdString();
	const auto npid  = m_edit_npid->text().toStdString();
	const auto token = m_edit_token->text().toStdString();

	auto validate = [](const std::string& input) -> bool
	{
		if (input.length() < 3 || input.length() > 16)
			return false;

		return std::all_of(input.cbegin(), input.cend(), [](const char c) { return std::isalnum(c) || c == '-' || c == '_'; });
	};

	if (host.empty())
	{
		QMessageBox::critical(this, tr("Missing host"), tr("You need to enter a host for rpcn!"), QMessageBox::Ok);
		return false;
	}

	if (npid.empty() || g_cfg_rpcn.get_password().empty())
	{
		QMessageBox::critical(this, tr("Wrong input"), tr("You need to enter a username and a password!"), QMessageBox::Ok);
		return false;
	}

	if (!validate(npid))
	{
		QMessageBox::critical(this, tr("Invalid character"), tr("NPID must be between 3 and 16 characters and can only contain '-', '_' or alphanumeric characters."), QMessageBox::Ok);
		return false;
	}

	if (!token.empty() && token.size() != 16)
	{
		QMessageBox::critical(this, tr("Invalid token"), tr("The token you have received should be 16 characters long."), QMessageBox::Ok);
		return false;
	}

	g_cfg_rpcn.set_host(host);
	g_cfg_rpcn.set_npid(npid);
	g_cfg_rpcn.set_token(token);

	g_cfg_rpcn.save();

	return true;
}

bool rpcn_settings_dialog::create_account()
{
	// Validate and save
	if (!save_config())
		return false;

	QString email;
	const QRegExpValidator simple_email_validator(QRegExp("^[a-zA-Z0-9.!#$%&’*+/=?^_`{|}~-]+@[a-zA-Z0-9-]+(?:\\.[a-zA-Z0-9-]+)*$"));

	while (true)
	{
		bool clicked_ok = false;
		email = QInputDialog::getText(this, "Email address", "An email address is required, please note:\n*A valid email is needed to validate your account.\n*Your email won't be used for anything beyond sending you the token.\n*Upon successful creation a token will be sent to your email which you'll need to login.\n\n", QLineEdit::Normal, "", &clicked_ok);
		if (!clicked_ok)
			return false;

		int pos = 0;
		if (email.isEmpty() || simple_email_validator.validate(email, pos) != QValidator::Acceptable)
		{
			QMessageBox::critical(this, tr("Wrong input"), tr("You need to enter a valid email!"), QMessageBox::Ok);
		}
		else
		{
			break;
		}
	}

	const auto rpcn = std::make_shared<rpcn_client>(true);

	const auto host        = g_cfg_rpcn.get_host();
	const auto npid        = g_cfg_rpcn.get_npid();
	const auto online_name = npid;
	const auto avatar_url  = "https://rpcs3.net/cdn/netplay/DefaultAvatar.png";
	const auto password    = g_cfg_rpcn.get_password();

	std::thread(
		[](const std::shared_ptr<rpcn_client> rpcn)
		{
			while (rpcn.use_count() != 1)
				rpcn->manage_connection();

			rpcn->disconnect();
		},
		rpcn)
		.detach();

	if (!rpcn->connect(host))
	{
		QMessageBox::critical(this, tr("Error Connecting"), tr("Failed to connect to RPCN server"), QMessageBox::Ok);
		rpcn->abort();
		return false;
	}

	if (!rpcn->create_user(npid, password, online_name, avatar_url, email.toStdString()))
	{
		QMessageBox::critical(this, tr("Error Creating Account"), tr("Failed to create the account"), QMessageBox::Ok);
		rpcn->abort();
		return false;
	}

	QMessageBox::information(this, tr("Account created!"), tr("Your account has been created successfully!\nCheck your email for your token!"), QMessageBox::Ok);
	rpcn->abort();
	return true;
}
