#include <QMessageBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QInputDialog>
#include <QGroupBox>
#include <QMenu>
#include <QDialogButtonBox>
#include <QCheckBox>

#include "qt_utils.h"

#include "rpcn_settings_dialog.h"
#include "Emu/System.h"
#include "Emu/NP/rpcn_config.h"
#include "Emu/NP/ip_address.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wextern-c-compat"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif
#include <wolfssl/ssl.h>
#include <wolfssl/openssl/evp.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

LOG_CHANNEL(rpcn_settings_log, "rpcn settings dlg");

bool validate_rpcn_username(std::string_view username)
{
	if (username.length() < 3 || username.length() > 16)
		return false;

	return std::all_of(username.cbegin(), username.cend(), [](const char c)
		{
			return std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_';
		});
}

bool validate_email(std::string_view email)
{
	const QRegularExpressionValidator simple_email_validator(QRegularExpression("^[a-zA-Z0-9.!#$%&’*+/=?^_`{|}~-]+@[a-zA-Z0-9-]+(?:\\.[a-zA-Z0-9-]+)*$"));
	QString qstr_email = QString::fromStdString(std::string(email));
	int pos            = 0;

	if (qstr_email.isEmpty() || qstr_email.contains(' ') || qstr_email.contains('\t') || simple_email_validator.validate(qstr_email, pos) != QValidator::Acceptable)
		return false;

	return true;
}

bool validate_token(std::string_view token)
{
	return token.size() == 16 && std::all_of(token.cbegin(), token.cend(), [](const char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z'); });
}

std::string derive_password(std::string_view user_password)
{
	std::string_view salt_str = "No matter where you go, everybody's connected.";

	u8 derived_password_digest[SHA3_256_DIGEST_LENGTH];
	ensure(!wc_PBKDF2(derived_password_digest, reinterpret_cast<const u8*>(user_password.data()), ::narrow<s32>(user_password.size()), reinterpret_cast<const u8*>(salt_str.data()), ::narrow<s32>(salt_str.size()), 200'000, SHA3_256_DIGEST_LENGTH, WC_SHA3_256));

	std::string derived_password("0000000000000000000000000000000000000000000000000000000000000000");
	for (u32 i = 0; i < SHA3_256_DIGEST_LENGTH; i++)
	{
		constexpr auto pal            = "0123456789ABCDEF";
		derived_password[i * 2]       = pal[derived_password_digest[i] >> 4];
		derived_password[(i * 2) + 1] = pal[derived_password_digest[i] & 15];
	}

	return derived_password;
}

rpcn_settings_dialog::rpcn_settings_dialog(QWidget* parent)
	: QDialog(parent)
{
	g_cfg_rpcn.load();

	const auto set_title = [this]()
	{
		if (const std::string npid = g_cfg_rpcn.get_npid(); !npid.empty())
		{
			setWindowTitle(tr("RPCN - %0").arg(QString::fromStdString(npid)));
			return;
		}

		setWindowTitle(tr("RPCN"));
	};

	set_title();
	setObjectName("rpcn_settings_dialog");
	setMinimumSize(QSize(400, 100));

	QVBoxLayout* vbox_global = new QVBoxLayout();

	QGroupBox* group_btns   = new QGroupBox(tr("RPCN"));
	QHBoxLayout* hbox_group = new QHBoxLayout();

	QPushButton* btn_account = new QPushButton(tr("Account"));
	QPushButton* btn_friends = new QPushButton(tr("Friends"));

	hbox_group->addWidget(btn_account);
	hbox_group->addWidget(btn_friends);

	group_btns->setLayout(hbox_group);
	vbox_global->addWidget(group_btns);
	setLayout(vbox_global);

	connect(btn_account, &QPushButton::clicked, this, [this, set_title]()
		{
			if (!Emu.IsStopped())
			{
				QMessageBox::critical(this, tr("Error: Emulation Running"), tr("You need to stop the emulator before editing RPCN account information!"), QMessageBox::Ok);
				return;
			}
			rpcn_account_dialog dlg(this);
			dlg.exec();
			set_title();
		});

	connect(btn_friends, &QPushButton::clicked, this, [this]()
		{
			rpcn_friends_dialog dlg(this);
			if (dlg.is_ok())
				dlg.exec();
		});
}

rpcn_account_dialog::rpcn_account_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("RPCN: Account"));
	setObjectName("rpcn_account_dialog");

	QVBoxLayout* vbox_global = new QVBoxLayout();

	QGroupBox* grp_server    = new QGroupBox(tr("Server:"));
	QVBoxLayout* vbox_server = new QVBoxLayout();

	QHBoxLayout* hbox_lbl_combo = new QHBoxLayout();
	QLabel* lbl_server          = new QLabel(tr("Server:"));
	cbx_servers                 = new QComboBox();

	refresh_combobox();

	hbox_lbl_combo->addWidget(lbl_server);
	hbox_lbl_combo->addWidget(cbx_servers);

	QHBoxLayout* hbox_buttons   = new QHBoxLayout();
	QPushButton* btn_add_server = new QPushButton(tr("Add"));
	QPushButton* btn_del_server = new QPushButton(tr("Del"));
	hbox_buttons->addStretch();
	hbox_buttons->addWidget(btn_add_server);
	hbox_buttons->addWidget(btn_del_server);

	vbox_server->addLayout(hbox_lbl_combo);
	vbox_server->addLayout(hbox_buttons);

	grp_server->setLayout(vbox_server);
	vbox_global->addWidget(grp_server);

	QGroupBox* grp_buttons    = new QGroupBox(tr("Account:"));
	QVBoxLayout* vbox_buttons = new QVBoxLayout();
	QPushButton* btn_create   = new QPushButton(tr("Create Account"));
	QPushButton* btn_edit     = new QPushButton(tr("Edit Account"));
	QPushButton* btn_test     = new QPushButton(tr("Test Account"));
	QLabel* label_npid        = new QLabel();

	QCheckBox* checkbox_disable_ipv6 = new QCheckBox(tr("Disable IPv6"));
	checkbox_disable_ipv6->setCheckState(g_cfg_rpcn.get_ipv6_support() ? Qt::Unchecked : Qt::Checked);

	const auto update_npid_label = [label_npid]()
	{
		const std::string npid = g_cfg_rpcn.get_npid();
		label_npid->setText(tr("Current ID: %0").arg(npid.empty() ? "-" : QString::fromStdString(npid)));
	};
	update_npid_label();

	vbox_buttons->addWidget(label_npid);
	vbox_buttons->addSpacing(10);
	vbox_buttons->addWidget(btn_create);
	vbox_buttons->addSpacing(10);
	vbox_buttons->addWidget(btn_edit);
	vbox_buttons->addSpacing(10);
	vbox_buttons->addWidget(btn_test);
	vbox_buttons->addSpacing(10);
	grp_buttons->setLayout(vbox_buttons);

	vbox_global->addWidget(grp_buttons);
	vbox_global->addWidget(checkbox_disable_ipv6);

	setLayout(vbox_global);

	connect(cbx_servers, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index)
		{
			if (index < 0)
				return;

			QVariant host = cbx_servers->itemData(index);

			if (!host.isValid() || !host.canConvert<QString>())
				return;

			g_cfg_rpcn.set_host(host.toString().toStdString());
			g_cfg_rpcn.save();

			// Resets the ipv6 support as it depends on availability of the feature on the server
			np::is_ipv6_supported(np::IPV6_SUPPORT::IPV6_UNKNOWN);
		});

	connect(btn_add_server, &QAbstractButton::clicked, this, [this]()
		{
			rpcn_add_server_dialog dlg(this);
			dlg.exec();
			const auto& new_server = dlg.get_new_server();
			if (new_server)
			{
				if (!g_cfg_rpcn.add_host(new_server->first, new_server->second))
				{
					QMessageBox::critical(this, tr("Existing Server"), tr("You already have a server with this description & hostname in the list."), QMessageBox::Ok);
					return;
				}

				g_cfg_rpcn.save();
				refresh_combobox();
			}
		});

	connect(btn_del_server, &QAbstractButton::clicked, this, [this]()
		{
			const int index = cbx_servers->currentIndex();

			if (index < 0)
				return;

			const std::string desc = cbx_servers->itemText(index).toStdString();
			const std::string host = cbx_servers->itemData(index).toString().toStdString();

			ensure(g_cfg_rpcn.del_host(desc, host));
			g_cfg_rpcn.save();
			refresh_combobox();
		});

	connect(btn_create, &QAbstractButton::clicked, this, [this, update_npid_label]()
		{
			rpcn_ask_username_dialog dlg_username(this, tr("Please enter your username.\n\n"
			                                               "Note that these restrictions apply:\n"
			                                               "- Username must be between 3 and 16 characters\n"
			                                               "- Username can only contain a-z A-Z 0-9 '-' '_'\n"
			                                               "- Username is case sensitive\n"));
			dlg_username.exec();
			const auto& username = dlg_username.get_username();

			if (!username)
				return;

			rpcn_ask_password_dialog dlg_password(this, tr("Please choose your password:\n\n"));
			dlg_password.exec();
			const auto& password = dlg_password.get_password();

			if (!password)
				return;

			rpcn_ask_email_dialog dlg_email(this, tr("An email address is required, please note:\n"
			                                         "- A valid email is needed to receive the token that validates your account.\n"
			                                         "- Your email won't be used for anything beyond sending you this token or the password reset token.\n\n"));
			dlg_email.exec();
			const auto& email = dlg_email.get_email();

			if (!email)
				return;

			if (QMessageBox::question(this, tr("RPCN: Account Creation"), tr("You are about to create an account with:\n-Username:%0\n-Email:%1\n\nIs this correct?").arg(QString::fromStdString(*username)).arg(QString::fromStdString(*email))) != QMessageBox::Yes)
				return;

			{
				const auto rpcn = rpcn::rpcn_client::get_instance(0);
				const auto avatar_url = "https://rpcs3.net/cdn/netplay/DefaultAvatar.png";

				if (auto result = rpcn->wait_for_connection(); result != rpcn::rpcn_state::failure_no_failure)
				{
					const QString error_message = tr("Failed to connect to RPCN server:\n%0").arg(QString::fromStdString(rpcn::rpcn_state_to_string(result)));
					QMessageBox::critical(this, tr("Error Connecting"), error_message, QMessageBox::Ok);
					return;
				}

				if (auto error = rpcn->create_user(*username, *password, *username, avatar_url, *email); error != rpcn::ErrorType::NoError)
				{
					QString error_message;
					switch (error)
					{
					case rpcn::ErrorType::CreationExistingUsername: error_message = tr("An account with that username already exists!"); break;
					case rpcn::ErrorType::CreationBannedEmailProvider: error_message = tr("This email provider is banned!"); break;
					case rpcn::ErrorType::CreationExistingEmail: error_message = tr("An account with that email already exists!"); break;
					case rpcn::ErrorType::CreationError: error_message = tr("Unknown creation error!"); break;
					default: error_message = tr("Unknown error"); break;
					}
					QMessageBox::critical(this, tr("Error Creating Account!"), tr("Failed to create the account:\n%0").arg(error_message), QMessageBox::Ok);
					return;
				}
			}

			g_cfg_rpcn.set_npid(*username);
			g_cfg_rpcn.set_password(*password);
			g_cfg_rpcn.save();

			rpcn_ask_token_dialog token_dlg(this, tr("Your account has been created successfully!\n"
			                                         "Your account authentification was saved.\n"
			                                         "Now all you need is to enter the token that was sent to your email.\n"
			                                         "You can skip this step by leaving it empty and entering it later in the Edit Account section too.\n"));
			token_dlg.exec();
			const auto& token = token_dlg.get_token();

			if (!token)
				return;

			g_cfg_rpcn.set_token(*token);
			g_cfg_rpcn.save();

			update_npid_label();
		});

	connect(btn_edit, &QAbstractButton::clicked, this, [this, update_npid_label]()
		{
			rpcn_account_edit_dialog dlg_edit(this);
			dlg_edit.exec();
			update_npid_label();
		});

	connect(btn_test, &QAbstractButton::clicked, this, [this]()
		{
			auto rpcn = rpcn::rpcn_client::get_instance(0);

			if (auto res = rpcn->wait_for_connection(); res != rpcn::rpcn_state::failure_no_failure)
			{
				const QString error_msg = tr("Failed to connect to RPCN:\n%0").arg(QString::fromStdString(rpcn::rpcn_state_to_string(res)));
				QMessageBox::warning(this, tr("Error connecting to RPCN!"), error_msg, QMessageBox::Ok);
				return;
			}
			if (auto res = rpcn->wait_for_authentified(); res != rpcn::rpcn_state::failure_no_failure)
			{
				const QString error_msg = tr("Failed to authentify to RPCN:\n%0").arg(QString::fromStdString(rpcn::rpcn_state_to_string(res)));
				QMessageBox::warning(this, tr("Error authentifying to RPCN!"), error_msg, QMessageBox::Ok);
				return;
			}

			QMessageBox::information(this, tr("RPCN Account Valid!"), tr("Your account is valid!"), QMessageBox::Ok);
		});

	connect(checkbox_disable_ipv6, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state)
	{
		g_cfg_rpcn.set_ipv6_support(state == Qt::Unchecked);
		g_cfg_rpcn.save();
	});
}

void rpcn_account_dialog::refresh_combobox()
{
	g_cfg_rpcn.load();
	const auto vec_hosts = g_cfg_rpcn.get_hosts();
	const auto cur_host  = g_cfg_rpcn.get_host();
	int i = 0, index = 0;

	cbx_servers->clear();

	for (const auto& [desc, host] : vec_hosts)
	{
		cbx_servers->addItem(QString::fromStdString(desc), QString::fromStdString(host));
		if (cur_host == host)
			index = i;

		i++;
	}

	cbx_servers->setCurrentIndex(index);
}

rpcn_add_server_dialog::rpcn_add_server_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("RPCN: Add Server"));
	setObjectName("rpcn_add_server_dialog");
	setMinimumSize(QSize(400, 200));

	QVBoxLayout* vbox_global = new QVBoxLayout();

	QLabel* lbl_description    = new QLabel(tr("Description:"));
	QLineEdit* edt_description = new QLineEdit();
	QLabel* lbl_host           = new QLabel(tr("Host:"));
	QLineEdit* edt_host        = new QLineEdit();
	QDialogButtonBox* btn_box  = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	vbox_global->addWidget(lbl_description);
	vbox_global->addWidget(edt_description);
	vbox_global->addWidget(lbl_host);
	vbox_global->addWidget(edt_host);
	vbox_global->addWidget(btn_box);

	setLayout(vbox_global);

	connect(btn_box, &QDialogButtonBox::accepted, this, [this, edt_description, edt_host]()
		{
			const QString description = edt_description->text();
			const QString host        = edt_host->text();

			if (description.isEmpty())
			{
				QMessageBox::critical(this, tr("Missing Description!"), tr("You must enter a description!"), QMessageBox::Ok);
				return;
			}
			if (host.isEmpty())
			{
				QMessageBox::critical(this, tr("Missing Hostname!"), tr("You must enter a hostname for the server!"), QMessageBox::Ok);
				return;
			}

			m_new_server = std::make_pair(description.toStdString(), host.toStdString());
			QDialog::accept();
		});
	connect(btn_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

const std::optional<std::pair<std::string, std::string>>& rpcn_add_server_dialog::get_new_server() const
{
	return m_new_server;
}

rpcn_ask_username_dialog::rpcn_ask_username_dialog(QWidget* parent, const QString& description)
	: QDialog(parent)
{
	setWindowTitle(tr("RPCN: Username"));
	setObjectName("rpcn_ask_username_dialog");

	QVBoxLayout* vbox_global = new QVBoxLayout();

	QLabel* lbl_username = new QLabel(description);

	QGroupBox* grp_username        = new QGroupBox(tr("Username:"));
	QHBoxLayout* hbox_grp_username = new QHBoxLayout();
	QLineEdit* edt_username        = new QLineEdit(QString::fromStdString(g_cfg_rpcn.get_npid()));
	edt_username->setMaxLength(16);
	edt_username->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-zA-Z0-9_\\-]*$"), this));
	hbox_grp_username->addWidget(edt_username);
	grp_username->setLayout(hbox_grp_username);

	QDialogButtonBox* btn_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	vbox_global->addWidget(lbl_username);
	vbox_global->addWidget(grp_username);
	vbox_global->addWidget(btn_box);

	setLayout(vbox_global);

	connect(btn_box, &QDialogButtonBox::accepted, this, [this, edt_username]()
		{
			const auto username = edt_username->text().toStdString();

			if (username.empty())
			{
				QMessageBox::critical(this, tr("Missing Username!"), tr("You must enter a username!"), QMessageBox::Ok);
				return;
			}
			if (!validate_rpcn_username(username))
			{
				QMessageBox::critical(this, tr("Invalid Username!"), tr("Please enter a valid username!"), QMessageBox::Ok);
			}

			m_username = username;
			QDialog::accept();
		});
	connect(btn_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

const std::optional<std::string>& rpcn_ask_username_dialog::get_username() const
{
	return m_username;
}

rpcn_ask_password_dialog::rpcn_ask_password_dialog(QWidget* parent, const QString& description)
	: QDialog(parent)
{
	setWindowTitle(tr("RPCN: Password"));
	setObjectName("rpcn_ask_password_dialog");

	QVBoxLayout* vbox_global = new QVBoxLayout();

	QLabel* lbl_description = new QLabel(description);

	QGroupBox* gbox_password = new QGroupBox();
	QVBoxLayout* vbox_gbox   = new QVBoxLayout();

	QLabel* lbl_pass1       = new QLabel(tr("Enter your password:"));
	QLineEdit* m_edit_pass1 = new QLineEdit();
	m_edit_pass1->setEchoMode(QLineEdit::Password);
	QLabel* lbl_pass2       = new QLabel(tr("Enter your password a second time:"));
	QLineEdit* m_edit_pass2 = new QLineEdit();
	m_edit_pass2->setEchoMode(QLineEdit::Password);

	vbox_gbox->addWidget(lbl_pass1);
	vbox_gbox->addWidget(m_edit_pass1);
	vbox_gbox->addWidget(lbl_pass2);
	vbox_gbox->addWidget(m_edit_pass2);
	gbox_password->setLayout(vbox_gbox);

	QDialogButtonBox* btn_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	vbox_global->addWidget(lbl_description);
	vbox_global->addWidget(gbox_password);
	vbox_global->addWidget(btn_box);

	setLayout(vbox_global);

	connect(btn_box, &QDialogButtonBox::accepted, this, [this, m_edit_pass1, m_edit_pass2]()
		{
			if (m_edit_pass1->text() != m_edit_pass2->text())
			{
				QMessageBox::critical(this, tr("Wrong Input"), tr("The two passwords you entered don't match!"), QMessageBox::Ok);
				return;
			}

			if (m_edit_pass1->text().isEmpty())
			{
				QMessageBox::critical(this, tr("Missing Password"), tr("You need to enter a password!"), QMessageBox::Ok);
				return;
			}

			m_password = derive_password(m_edit_pass1->text().toStdString());
			QDialog::accept();
		});
	connect(btn_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

const std::optional<std::string>& rpcn_ask_password_dialog::get_password() const
{
	return m_password;
}

rpcn_ask_email_dialog::rpcn_ask_email_dialog(QWidget* parent, const QString& description)
	: QDialog(parent)
{
	setWindowTitle(tr("RPCN: Email"));
	setObjectName("rpcn_ask_email_dialog");

	QVBoxLayout* vbox_global = new QVBoxLayout();

	QLabel* lbl_emailinfo = new QLabel(description);

	QGroupBox* gbox_password = new QGroupBox();
	QVBoxLayout* vbox_gbox   = new QVBoxLayout();

	QLabel* lbl_pass1       = new QLabel(tr("Enter your email:"));
	QLineEdit* m_edit_pass1 = new QLineEdit();
	QLabel* lbl_pass2       = new QLabel(tr("Enter your email a second time:"));
	QLineEdit* m_edit_pass2 = new QLineEdit();

	vbox_gbox->addWidget(lbl_pass1);
	vbox_gbox->addWidget(m_edit_pass1);
	vbox_gbox->addWidget(lbl_pass2);
	vbox_gbox->addWidget(m_edit_pass2);
	gbox_password->setLayout(vbox_gbox);

	QDialogButtonBox* btn_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	vbox_global->addWidget(lbl_emailinfo);
	vbox_global->addWidget(gbox_password);
	vbox_global->addWidget(btn_box);

	setLayout(vbox_global);

	connect(btn_box, &QDialogButtonBox::accepted, this, [this, m_edit_pass1, m_edit_pass2]()
		{
			if (m_edit_pass1->text() != m_edit_pass2->text())
			{
				QMessageBox::critical(this, tr("Wrong Input"), tr("The two emails you entered don't match!"), QMessageBox::Ok);
				return;
			}

			if (m_edit_pass1->text().isEmpty())
			{
				QMessageBox::critical(this, tr("Missing Email"), tr("You need to enter an email!"), QMessageBox::Ok);
				return;
			}

			std::string email = m_edit_pass1->text().toStdString();
			if (!validate_email(email))
			{
				QMessageBox::critical(this, tr("Invalid Email"), tr("You need to enter a valid email!"), QMessageBox::Ok);
				return;
			}

			m_email = std::move(email);
			QDialog::accept();
		});
	connect(btn_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

const std::optional<std::string>& rpcn_ask_email_dialog::get_email() const
{
	return m_email;
}

rpcn_ask_token_dialog::rpcn_ask_token_dialog(QWidget* parent, const QString& description)
	: QDialog(parent)
{
	setWindowTitle(tr("RPCN: Username"));
	setObjectName("rpcn_ask_token_dialog");

	QVBoxLayout* vbox_global = new QVBoxLayout();

	QLabel* lbl_token = new QLabel(description);

	QGroupBox* grp_token        = new QGroupBox(tr("Token:"));
	QHBoxLayout* hbox_grp_token = new QHBoxLayout();
	QLineEdit* edt_token        = new QLineEdit();
	edt_token->setMaxLength(16);
	hbox_grp_token->addWidget(edt_token);
	grp_token->setLayout(hbox_grp_token);

	QDialogButtonBox* btn_box = new QDialogButtonBox(QDialogButtonBox::Ok);

	vbox_global->addWidget(lbl_token);
	vbox_global->addWidget(grp_token);
	vbox_global->addWidget(btn_box);

	setLayout(vbox_global);

	connect(btn_box, &QDialogButtonBox::accepted, this, [this, edt_token]()
		{
			std::string token = edt_token->text().toStdString();

			if (!token.empty())
			{
				if (!validate_token(token))
				{
					QMessageBox::critical(this, tr("Invalid Token"), tr("The token appears to be invalid:\n"
					                                                    "-Token should be 16 characters long\n"
					                                                    "-Token should only contain 0-9 and A-F"),
						QMessageBox::Ok);
					return;
				}

				m_token = std::move(token);
			}

			QDialog::accept();
			return;
		});
}

const std::optional<std::string>& rpcn_ask_token_dialog::get_token() const
{
	return m_token;
}

rpcn_account_edit_dialog::rpcn_account_edit_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("RPCN: Edit Account"));
	setObjectName("rpcn_account_edit_dialog");
	setMinimumSize(QSize(400, 200));

	QVBoxLayout* vbox_global           = new QVBoxLayout();
	QHBoxLayout* hbox_labels_and_edits = new QHBoxLayout();
	QVBoxLayout* vbox_labels           = new QVBoxLayout();
	QVBoxLayout* vbox_edits            = new QVBoxLayout();
	QHBoxLayout* hbox_buttons          = new QHBoxLayout();

	QLabel* lbl_username = new QLabel(tr("Username:"));
	m_edit_username      = new QLineEdit();
	m_edit_username->setMaxLength(16);
	m_edit_username->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-zA-Z0-9_\\-]*$"), this));
	QLabel* lbl_pass          = new QLabel(tr("Password:"));
	QPushButton* btn_chg_pass = new QPushButton(tr("Set Password"));
	QLabel* lbl_token         = new QLabel(tr("Token:"));
	m_edit_token              = new QLineEdit();
	m_edit_token->setMaxLength(16);

	QPushButton* btn_resendtoken     = new QPushButton(tr("Resend Token"), this);
	QPushButton* btn_change_password = new QPushButton(tr("Change Password"), this);
	QPushButton* btn_save            = new QPushButton(tr("Save"), this);

	vbox_labels->addWidget(lbl_username);
	vbox_labels->addWidget(lbl_pass);
	vbox_labels->addWidget(lbl_token);

	vbox_edits->addWidget(m_edit_username);
	vbox_edits->addWidget(btn_chg_pass);
	vbox_edits->addWidget(m_edit_token);

	hbox_buttons->addWidget(btn_resendtoken);
	hbox_buttons->addWidget(btn_change_password);
	hbox_buttons->addStretch();
	hbox_buttons->addWidget(btn_save);

	hbox_labels_and_edits->addLayout(vbox_labels);
	hbox_labels_and_edits->addLayout(vbox_edits);

	vbox_global->addLayout(hbox_labels_and_edits);
	vbox_global->addLayout(hbox_buttons);

	setLayout(vbox_global);

	connect(btn_chg_pass, &QAbstractButton::clicked, this, [this]()
		{
			rpcn_ask_password_dialog ask_pass(this, tr("Please enter your password:"));
			ask_pass.exec();

			const auto& password = ask_pass.get_password();
			if (!password)
				return;

			g_cfg_rpcn.set_password(*password);
			g_cfg_rpcn.save();

			QMessageBox::information(this, tr("RPCN Password Saved"), tr("Your password was saved successfully!"), QMessageBox::Ok);
		});

	connect(btn_save, &QAbstractButton::clicked, this, [this]()
		{
			if (save_config())
				close();
		});
	connect(btn_resendtoken, &QAbstractButton::clicked, this, &rpcn_account_edit_dialog::resend_token);
	connect(btn_change_password, &QAbstractButton::clicked, this, &rpcn_account_edit_dialog::change_password);

	g_cfg_rpcn.load();

	m_edit_username->setText(QString::fromStdString(g_cfg_rpcn.get_npid()));
	m_edit_token->setText(QString::fromStdString(g_cfg_rpcn.get_token()));
}

bool rpcn_account_edit_dialog::save_config()
{
	const std::string username = m_edit_username->text().toStdString();
	const std::string token    = m_edit_token->text().toStdString();

	if (username.empty() || g_cfg_rpcn.get_password().empty())
	{
		QMessageBox::critical(this, tr("Missing Input"), tr("You need to enter a username and a password!"), QMessageBox::Ok);
		return false;
	}

	if (!validate_rpcn_username(username))
	{
		QMessageBox::critical(this, tr("Invalid Username"), tr("Username must be between 3 and 16 characters and can only contain '-', '_' or alphanumeric characters."), QMessageBox::Ok);
		return false;
	}

	if (!token.empty() && !validate_token(token))
	{
		QMessageBox::critical(this, tr("Invalid Token"), tr("The token you have received should be 16 characters long and contain only 0-9 A-F."), QMessageBox::Ok);
		return false;
	}

	g_cfg_rpcn.set_npid(username);
	g_cfg_rpcn.set_token(token);

	g_cfg_rpcn.save();

	return true;
}

void rpcn_account_edit_dialog::resend_token()
{
	if (!save_config())
		return;

	const auto rpcn = rpcn::rpcn_client::get_instance(0);

	const std::string npid     = g_cfg_rpcn.get_npid();
	const std::string password = g_cfg_rpcn.get_password();

	if (auto result = rpcn->wait_for_connection(); result != rpcn::rpcn_state::failure_no_failure)
	{
		const QString error_message = tr("Failed to connect to RPCN server:\n%0").arg(QString::fromStdString(rpcn::rpcn_state_to_string(result)));
		QMessageBox::critical(this, tr("Error Connecting!"), error_message, QMessageBox::Ok);
		return;
	}

	if (auto error = rpcn->resend_token(npid, password); error != rpcn::ErrorType::NoError)
	{
		QString error_message;
		switch (error)
		{
		case rpcn::ErrorType::Invalid: error_message = tr("The server has no email verification and doesn't need a token!"); break;
		case rpcn::ErrorType::DbFail: error_message = tr("A database related error happened on the server!"); break;
		case rpcn::ErrorType::TooSoon: error_message = tr("You can only ask for a token mail once every 24 hours!"); break;
		case rpcn::ErrorType::EmailFail: error_message = tr("The mail couldn't be sent successfully!"); break;
		case rpcn::ErrorType::LoginError: error_message = tr("The username/password pair is invalid!"); break;
		default: error_message = tr("Unknown error"); break;
		}
		QMessageBox::critical(this, tr("Error Sending Token!"), tr("Failed to send the token:\n%0").arg(error_message), QMessageBox::Ok);
		return;
	}

	QMessageBox::information(this, tr("Token Sent!"), tr("Your token was successfully resent to the email associated with your account!"), QMessageBox::Ok);
}

void rpcn_account_edit_dialog::change_password()
{
	rpcn_ask_username_dialog dlg_username(this, tr("Please confirm your username:"));
	dlg_username.exec();
	const auto& username = dlg_username.get_username();

	if (!username)
		return;

	switch (QMessageBox::question(this, tr("RPCN: Change Password"), tr("Do you already have a reset password token?\n"
	                                                                    "Note that the reset password token is different from the email verification token.")))
	{
	case QMessageBox::No:
	{
		rpcn_ask_email_dialog dlg_email(this, tr("Please enter the email you used to create the account:"));
		dlg_email.exec();
		const auto& email = dlg_email.get_email();

		if (!email)
			return;

		{
			const auto rpcn = rpcn::rpcn_client::get_instance(0);
			if (auto result = rpcn->wait_for_connection(); result != rpcn::rpcn_state::failure_no_failure)
			{
				const QString error_message = tr("Failed to connect to RPCN server:\n%0").arg(QString::fromStdString(rpcn::rpcn_state_to_string(result)));
				QMessageBox::critical(this, tr("Error Connecting!"), error_message, QMessageBox::Ok);
				return;
			}

			if (auto error = rpcn->send_reset_token(*username, *email); error != rpcn::ErrorType::NoError)
			{
				QString error_message;
				switch (error)
				{
				case rpcn::ErrorType::Invalid: error_message = tr("The server has no email verification and doesn't support password changes!"); break;
				case rpcn::ErrorType::DbFail: error_message = tr("A database related error happened on the server!"); break;
				case rpcn::ErrorType::TooSoon: error_message = tr("You can only ask for a reset password token once every 24 hours!"); break;
				case rpcn::ErrorType::EmailFail: error_message = tr("The mail couldn't be sent successfully!"); break;
				case rpcn::ErrorType::LoginError: error_message = tr("The username/email pair is invalid!"); break;
				default: error_message = tr("Unknown error!"); break;
				}
				QMessageBox::critical(this, tr("Error Sending Password Reset Token!"), tr("Failed to send the password reset token:\n%0").arg(error_message), QMessageBox::Ok);
				return;
			}

			QMessageBox::information(this, tr("Password Reset Token Sent!"), tr("The reset password token has successfully been sent!"), QMessageBox::Ok);
		}
		[[fallthrough]];
	}
	case QMessageBox::Yes:
	{
		rpcn_ask_token_dialog dlg_token(this, tr("Please enter the password reset token you received:"));
		dlg_token.exec();
		const auto& token = dlg_token.get_token();

		if (!token)
			return;

		rpcn_ask_password_dialog dlg_password(this, tr("Please enter your new password:"));
		dlg_password.exec();
		const auto& password = dlg_password.get_password();

		if (!password)
			return;

		{
			const auto rpcn = rpcn::rpcn_client::get_instance(0);
			if (auto result = rpcn->wait_for_connection(); result != rpcn::rpcn_state::failure_no_failure)
			{
				const QString error_message = tr("Failed to connect to RPCN server:\n%0").arg(QString::fromStdString(rpcn::rpcn_state_to_string(result)));
				QMessageBox::critical(this, tr("Error Connecting!"), error_message, QMessageBox::Ok);
				return;
			}

			if (auto error = rpcn->reset_password(*username, *token, *password); error != rpcn::ErrorType::NoError)
			{
				QString error_message;
				switch (error)
				{
				case rpcn::ErrorType::Invalid: error_message = tr("The server has no email verification and doesn't support password changes!"); break;
				case rpcn::ErrorType::DbFail: error_message = tr("A database related error happened on the server!"); break;
				case rpcn::ErrorType::TooSoon: error_message = tr("You can only ask for a reset password token once every 24 hours!"); break;
				case rpcn::ErrorType::EmailFail: error_message = tr("The mail couldn't be sent successfully!"); break;
				case rpcn::ErrorType::LoginError: error_message = tr("The username/token pair is invalid!"); break;
				default: error_message = tr("Unknown error"); break;
				}
				QMessageBox::critical(this, tr("Error Sending Password Reset Token"), tr("Failed to change the password:\n%0").arg(error_message), QMessageBox::Ok);
				return;
			}

			g_cfg_rpcn.set_password(*password);
			g_cfg_rpcn.save();
			QMessageBox::information(this, tr("Password Successfully Changed!"), tr("Your password has been successfully changed!"), QMessageBox::Ok);
		}
		break;
	}
	default:
		return;
	}
}

void friend_callback(void* param, rpcn::NotificationType ntype, const std::string& username, bool status)
{
	auto* dlg = static_cast<rpcn_friends_dialog*>(param);
	dlg->callback_handler(ntype, username, status);
}

// Avoid including np_handler.h
namespace np
{
	struct player_history
	{
		u64 timestamp{};
		std::set<std::string> communication_ids;
		std::string description;
	};

	std::map<std::string, player_history> load_players_history();
}

rpcn_friends_dialog::rpcn_friends_dialog(QWidget* parent)
	: QDialog(parent)
{
	const qreal pixel_ratio = devicePixelRatioF();

	// Create colored circle pixmaps
	gui::utils::circle_pixmap online_pixmap(QColorConstants::Svg::green, pixel_ratio * 2);
	gui::utils::circle_pixmap offline_pixmap(QColorConstants::Svg::red, pixel_ratio * 2);
	gui::utils::circle_pixmap blocked_pixmap(QColorConstants::Svg::red, pixel_ratio * 2);
	gui::utils::circle_pixmap req_rcvd_pixmap(QColorConstants::Svg::yellow, pixel_ratio * 2);
	gui::utils::circle_pixmap req_sent_pixmap(QColorConstants::Svg::orange, pixel_ratio * 2);

	// Reset device pixel ratios for further painting
	online_pixmap.setDevicePixelRatio(1.0);
	offline_pixmap.setDevicePixelRatio(1.0);
	blocked_pixmap.setDevicePixelRatio(1.0);
	req_rcvd_pixmap.setDevicePixelRatio(1.0);
	req_sent_pixmap.setDevicePixelRatio(1.0);

	// The width and height are identical for all pixmaps
	const int w = online_pixmap.width();
	const int h = online_pixmap.height();

	const QPen pen1(QBrush(Qt::black), w * 0.1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);
	const QPen pen2(QBrush(Qt::black), w * 0.15, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);

	QPainter painter;
	painter.setRenderHint(QPainter::Antialiasing);

	// Render a bar into the offline pixmap
	{
		painter.begin(&offline_pixmap);
		painter.setPen(pen2);
		painter.drawLine(QPointF(w * 0.25, h * 0.5), QPointF(w * 0.75, h * 0.5));
		painter.end();
	}

	// Render a cross into the blocked pixmap
	{
		painter.begin(&blocked_pixmap);
		painter.setPen(pen1);
		painter.drawLine(QPointF(w * 0.25, h * 0.25), QPointF(w * 0.75, h * 0.75));
		painter.drawLine(QPointF(w * 0.25, h * 0.75), QPointF(w * 0.75, h * 0.25));
		painter.end();
	}

	// Render a downward arrow into the request received pixmap
	{
		painter.begin(&req_rcvd_pixmap);
		painter.setPen(pen1);
		painter.drawLine(QPointF(w * 0.5, h * 0.25), QPointF(w * 0.5, h * 0.75));
		painter.drawLines({ QLineF(w * 0.5, h * 0.75, w * 0.25, h * 0.5), QLineF(w * 0.5, h * 0.75, w * 0.75, h * 0.5) });
		painter.end();
	}

	// Render an upward arrow into the request sent pixmap
	{
		painter.begin(&req_sent_pixmap);
		painter.setPen(pen1);
		painter.drawLine(QPointF(w * 0.5, h * 0.25), QPointF(w * 0.5, h * 0.75));
		painter.drawLines({ QLineF(w * 0.5, h * 0.25, w * 0.25, h * 0.5), QLineF(w * 0.5, h * 0.25, w * 0.75, h * 0.5) });
		painter.end();
	}

	m_icon_online = online_pixmap;
	m_icon_offline = offline_pixmap;
	m_icon_blocked = blocked_pixmap;
	m_icon_request_received = req_rcvd_pixmap;
	m_icon_request_sent = req_sent_pixmap;

	const auto set_title = [this]()
	{
		if (const std::string npid = g_cfg_rpcn.get_npid(); !npid.empty())
		{
			if (m_rpcn && m_rpcn->is_connected() && m_rpcn->is_authentified())
			{
				setWindowTitle(tr("RPCN: Friends - Logged in as %0").arg(QString::fromStdString(npid)));
				return;
			}

			setWindowTitle(tr("RPCN: Friends - %0 (Not logged in)").arg(QString::fromStdString(npid)));
			return;
		}

		setWindowTitle(tr("RPCN: Friends"));
	};

	set_title();
	setObjectName("rpcn_friends_dialog");
	setMinimumSize(QSize(400, 100));

	QVBoxLayout* vbox_global = new QVBoxLayout();

	QHBoxLayout* hbox_groupboxes = new QHBoxLayout();

	QGroupBox* grp_list_friends   = new QGroupBox(tr("Friends"));
	QVBoxLayout* vbox_lst_friends = new QVBoxLayout();
	m_lst_friends                 = new QListWidget(this);
	m_lst_friends->setContextMenuPolicy(Qt::CustomContextMenu);
	vbox_lst_friends->addWidget(m_lst_friends);
	QPushButton* btn_addfriend = new QPushButton(tr("Add Friend"));
	vbox_lst_friends->addWidget(btn_addfriend);
	grp_list_friends->setLayout(vbox_lst_friends);
	hbox_groupboxes->addWidget(grp_list_friends);

	QGroupBox* grp_list_requests   = new QGroupBox(tr("Friend Requests"));
	QVBoxLayout* vbox_lst_requests = new QVBoxLayout();
	m_lst_requests                 = new QListWidget(this);
	m_lst_requests->setContextMenuPolicy(Qt::CustomContextMenu);
	vbox_lst_requests->addWidget(m_lst_requests);
	QHBoxLayout* hbox_request_btns = new QHBoxLayout();
	vbox_lst_requests->addLayout(hbox_request_btns);
	grp_list_requests->setLayout(vbox_lst_requests);
	hbox_groupboxes->addWidget(grp_list_requests);

	QGroupBox* grp_list_blocks   = new QGroupBox(tr("Blocked Users"));
	QVBoxLayout* vbox_lst_blocks = new QVBoxLayout();
	m_lst_blocks                 = new QListWidget(this);
	vbox_lst_blocks->addWidget(m_lst_blocks);
	grp_list_blocks->setLayout(vbox_lst_blocks);
	hbox_groupboxes->addWidget(grp_list_blocks);

	QGroupBox* grp_list_history   = new QGroupBox(tr("Recent Players"));
	QVBoxLayout* vbox_lst_history = new QVBoxLayout();
	m_lst_history                 = new QListWidget(this);
	m_lst_history->setContextMenuPolicy(Qt::CustomContextMenu);
	vbox_lst_history->addWidget(m_lst_history);
	grp_list_history->setLayout(vbox_lst_history);
	hbox_groupboxes->addWidget(grp_list_history);

	vbox_global->addLayout(hbox_groupboxes);

	setLayout(vbox_global);

	// Tries to connect to RPCN
	m_rpcn = rpcn::rpcn_client::get_instance(0);

	if (auto res = m_rpcn->wait_for_connection(); res != rpcn::rpcn_state::failure_no_failure)
	{
		const QString error_msg = tr("Failed to connect to RPCN:\n%0").arg(QString::fromStdString(rpcn::rpcn_state_to_string(res)));
		QMessageBox::warning(parent, tr("Error connecting to RPCN!"), error_msg, QMessageBox::Ok);
		return;
	}
	if (auto res = m_rpcn->wait_for_authentified(); res != rpcn::rpcn_state::failure_no_failure)
	{
		const QString error_msg = tr("Failed to authentify to RPCN:\n%0").arg(QString::fromStdString(rpcn::rpcn_state_to_string(res)));
		QMessageBox::warning(parent, tr("Error authentifying to RPCN!"), error_msg, QMessageBox::Ok);
		return;
	}

	set_title();

	// Get friends, setup callback and setup comboboxes
	rpcn::friend_data data;
	m_rpcn->get_friends_and_register_cb(data, friend_callback, this);

	for (const auto& [username, data] : data.friends)
	{
		add_update_list(m_lst_friends, QString::fromStdString(username), data.online ? m_icon_online : m_icon_offline, data.online);
	}

	for (const auto& fr_req : data.requests_sent)
	{
		add_update_list(m_lst_requests, QString::fromStdString(fr_req), m_icon_request_sent, QVariant(false));
	}

	for (const auto& fr_req : data.requests_received)
	{
		add_update_list(m_lst_requests, QString::fromStdString(fr_req), m_icon_request_received, QVariant(true));
	}

	for (const auto& blck : data.blocked)
	{
		add_update_list(m_lst_blocks, QString::fromStdString(blck), m_icon_blocked, QVariant(false));
	}

	auto history = np::load_players_history();
	std::map<u64, std::pair<std::string, std::string>, std::greater<u64>> sorted_history;

	for (auto& [username, user_info] : history)
	{
		if (!data.friends.contains(username) && !data.requests_sent.contains(username) && !data.requests_received.contains(username))
			sorted_history.insert(std::make_pair(user_info.timestamp, std::make_pair(username, std::move(user_info.description))));
	}

	for (const auto& [_, username_and_description] : sorted_history)
	{
		const auto& [username, description] = username_and_description;
		QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(username));

		if (!description.empty())
			item->setToolTip(QString::fromStdString(description));

		m_lst_history->addItem(item);
	}

	connect(this, &rpcn_friends_dialog::signal_add_update_friend, this, &rpcn_friends_dialog::add_update_friend);
	connect(this, &rpcn_friends_dialog::signal_remove_friend, this, &rpcn_friends_dialog::remove_friend);
	connect(this, &rpcn_friends_dialog::signal_add_query, this, &rpcn_friends_dialog::add_query);

	connect(m_lst_friends, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos)
		{
			if (!m_lst_friends->itemAt(pos) || m_lst_friends->selectedItems().count() != 1)
			{
				return;
			}

			QListWidgetItem* selected_item = m_lst_friends->selectedItems().first();
			std::string str_sel_friend     = selected_item->text().toStdString();

			QMenu* context_menu           = new QMenu();
			QAction* remove_friend_action = context_menu->addAction(tr("&Remove Friend"));

			connect(remove_friend_action, &QAction::triggered, this, [this, str_sel_friend]()
				{
					if (!m_rpcn->remove_friend(str_sel_friend))
					{
						QMessageBox::critical(this, tr("Error removing a friend!"), tr("An error occurred while trying to remove a friend!"), QMessageBox::Ok);
					}
					else
					{
						QMessageBox::information(this, tr("Friend removed!"), tr("You've successfully removed a friend!"), QMessageBox::Ok);
					}
				});

			context_menu->exec(m_lst_friends->viewport()->mapToGlobal(pos));
			context_menu->deleteLater();
		});

	connect(m_lst_requests, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos)
		{
			if (!m_lst_requests->itemAt(pos) || m_lst_requests->selectedItems().count() != 1)
			{
				return;
			}

			QListWidgetItem* selected_item = m_lst_requests->selectedItems().first();
			std::string str_sel_friend = selected_item->text().toStdString();

			QMenu* context_menu = new QMenu();

			// Presents different context based on role
			if (selected_item->data(Qt::UserRole) == false)
			{
				QAction* cancel_friend_request = context_menu->addAction(tr("&Cancel Request"));

				connect(cancel_friend_request, &QAction::triggered, this, [this, str_sel_friend]()
					{
						if (!m_rpcn->remove_friend(str_sel_friend))
						{
							QMessageBox::critical(this, tr("Error cancelling friend request!"), tr("An error occurred while trying to cancel friend request!"), QMessageBox::Ok);
						}
						else
						{
							QMessageBox::information(this, tr("Friend request cancelled!"), tr("You've successfully cancelled the friend request!"), QMessageBox::Ok);
						}
					});

				context_menu->exec(m_lst_requests->viewport()->mapToGlobal(pos));
				context_menu->deleteLater();

				return;
			}


			QAction* accept_request_action = context_menu->addAction(tr("&Accept Request"));
			QAction* reject_friend_request = context_menu->addAction(tr("&Reject Request"));

			connect(accept_request_action, &QAction::triggered, this, [this, str_sel_friend]()
				{
					if (!m_rpcn->add_friend(str_sel_friend))
					{
						QMessageBox::critical(this, tr("Error adding a friend!"), tr("An error occurred while trying to add a friend!"), QMessageBox::Ok);
					}
					else
					{
						QMessageBox::information(this, tr("Friend added!"), tr("You've successfully added a friend!"), QMessageBox::Ok);
					}
				});

			connect(reject_friend_request, &QAction::triggered, this, [this, str_sel_friend]()
			{
				if (!m_rpcn->remove_friend(str_sel_friend))
				{
					QMessageBox::critical(this, tr("Error rejecting friend request!"), tr("An error occurred while trying to reject the friend request!"), QMessageBox::Ok);
				}
				else
				{
					QMessageBox::information(this, tr("Friend request cancelled!"), tr("You've successfully rejected the friend request!"), QMessageBox::Ok);
				}
			});

			context_menu->exec(m_lst_requests->viewport()->mapToGlobal(pos));
			context_menu->deleteLater();
		});

	connect(m_lst_history, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos)
		{
			if (!m_lst_history->itemAt(pos) || m_lst_history->selectedItems().count() != 1)
			{
				return;
			}

			QListWidgetItem* selected_item = m_lst_history->selectedItems().first();

			std::string str_sel_friend = selected_item->text().toStdString();

			QMenu* context_menu = new QMenu();
			QAction* send_friend_request_action = context_menu->addAction(tr("&Send Friend Request"));

			connect(send_friend_request_action, &QAction::triggered, this, [this, str_sel_friend]()
				{
					if (!m_rpcn->add_friend(str_sel_friend))
					{
						QMessageBox::critical(this, tr("Error sending a friend request!"), tr("An error occurred while trying to send a friend request!"), QMessageBox::Ok);
						return;
					}

					QString qstr_friend = QString::fromStdString(str_sel_friend);
					add_update_list(m_lst_requests, qstr_friend, m_icon_request_sent, QVariant(false));
					remove_list(m_lst_history, qstr_friend);
				});

			context_menu->exec(m_lst_history->viewport()->mapToGlobal(pos));
			context_menu->deleteLater();
		});

	connect(btn_addfriend, &QAbstractButton::clicked, this, [this]()
		{
			std::string str_friend_username;
			while (true)
			{
				bool ok                 = false;
				QString friend_username = QInputDialog::getText(this, tr("Add a friend"), tr("Friend's username:"), QLineEdit::Normal, "", &ok);
				if (!ok)
				{
					return;
				}

				str_friend_username = friend_username.toStdString();

				if (validate_rpcn_username(str_friend_username))
				{
					break;
				}

				QMessageBox::critical(this, tr("Error validating username!"), tr("The username you entered is invalid!"), QMessageBox::Ok);
			}

			if (!m_rpcn->add_friend(str_friend_username))
			{
				QMessageBox::critical(this, tr("Error adding friend!"), tr("An error occurred while adding a friend!"), QMessageBox::Ok);
			}
			else
			{
				add_update_list(m_lst_requests, QString::fromStdString(str_friend_username), m_icon_request_sent, QVariant(false));
				QMessageBox::information(this, tr("Friend added!"), tr("Friend was successfully added!"), QMessageBox::Ok);
			}
		});

	m_rpcn_ok = true;
}

rpcn_friends_dialog::~rpcn_friends_dialog()
{
	m_rpcn->remove_friend_cb(friend_callback, this);
}

bool rpcn_friends_dialog::is_ok() const
{
	return m_rpcn_ok;
}

void rpcn_friends_dialog::add_update_list(QListWidget* list, const QString& name, const QIcon& icon, const QVariant& data)
{
	QListWidgetItem* item = nullptr;
	if (auto found = list->findItems(name, Qt::MatchExactly); !found.empty())
	{
		item = found[0];
		item->setData(Qt::UserRole, data);
		item->setIcon(icon);
	}
	else
	{
		item = new QListWidgetItem(name);
		item->setIcon(icon);
		item->setData(Qt::UserRole, data);
		list->addItem(item);
	}
}

void rpcn_friends_dialog::remove_list(QListWidget* list, const QString& name)
{
	if (auto found = list->findItems(name, Qt::MatchExactly); !found.empty())
	{
		delete list->takeItem(list->row(found[0]));
	}
}

void rpcn_friends_dialog::add_update_friend(const QString& name, bool status)
{
	add_update_list(m_lst_friends, name, status ? m_icon_online : m_icon_offline, status);
	remove_list(m_lst_requests, name);
}

void rpcn_friends_dialog::remove_friend(const QString& name)
{
	remove_list(m_lst_friends, name);
	remove_list(m_lst_requests, name);
}

void rpcn_friends_dialog::add_query(const QString& name)
{
	add_update_list(m_lst_requests, name, m_icon_request_received, QVariant(true));
	remove_list(m_lst_history, name);
}

void rpcn_friends_dialog::callback_handler(rpcn::NotificationType ntype, const std::string& username, bool status)
{
	const QString qtr_username = QString::fromStdString(username);
	switch (ntype)
	{
	case rpcn::NotificationType::FriendQuery: // Other user sent a friend request
	{
		Q_EMIT signal_add_query(qtr_username);
		break;
	}
	case rpcn::NotificationType::FriendNew: // Add a friend to the friendlist(either accepted a friend request or friend accepted it)
	{
		Q_EMIT signal_add_update_friend(qtr_username, status);
		break;
	}
	case rpcn::NotificationType::FriendLost: // Remove friend from the friendlist(user removed friend or friend removed friend)
	{
		Q_EMIT signal_remove_friend(qtr_username);
		break;
	}
	case rpcn::NotificationType::FriendStatus: // Set status of friend to Offline or Online
	{
		Q_EMIT signal_add_update_friend(qtr_username, status);
		break;
	}
	case rpcn::NotificationType::FriendPresenceChanged:
	{
		break;
	}
	default:
	{
		rpcn_settings_log.fatal("An unhandled notification type was received by the RPCN friends dialog callback!");
		break;
	}
	}
}
