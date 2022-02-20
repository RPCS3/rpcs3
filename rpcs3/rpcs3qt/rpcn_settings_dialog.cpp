#include <QMessageBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegExpValidator>
#include <QInputDialog>
#include <QGroupBox>
#include <QMenu>
#include <thread>

#include "qt_utils.h"

#include "rpcn_settings_dialog.h"
#include "Emu/NP/rpcn_config.h"

#include <wolfssl/ssl.h>
#include <wolfssl/openssl/evp.h>

LOG_CHANNEL(rpcn_settings_log, "rpcn settings dlg");

const QString rpcn_state_to_qstr(rpcn::rpcn_state state)
{
	switch (state)
	{
	case rpcn::rpcn_state::failure_no_failure: return QObject::tr("No Error");
	case rpcn::rpcn_state::failure_input: return QObject::tr("Invalid Input");
	case rpcn::rpcn_state::failure_wolfssl: return QObject::tr("WolfSSL Error");
	case rpcn::rpcn_state::failure_resolve: return QObject::tr("Resolve Error");
	case rpcn::rpcn_state::failure_connect: return QObject::tr("Connect Error");
	case rpcn::rpcn_state::failure_id: return QObject::tr("Identification Error");
	case rpcn::rpcn_state::failure_id_already_logged_in: return QObject::tr("Identification Error: User Already Logged In");
	case rpcn::rpcn_state::failure_id_username: return QObject::tr("Identification Error: Invalid Username");
	case rpcn::rpcn_state::failure_id_password: return QObject::tr("Identification Error: Invalid Password");
	case rpcn::rpcn_state::failure_id_token: return QObject::tr("Identification Error: Invalid Token");
	case rpcn::rpcn_state::failure_protocol: return QObject::tr("Protocol Version Error");
	case rpcn::rpcn_state::failure_other: return QObject::tr("Unknown Error");
	default: return QObject::tr("Unhandled rpcn state!");
	}
}

bool validate_rpcn_username(const std::string& input)
{
	if (input.length() < 3 || input.length() > 16)
		return false;

	return std::all_of(input.cbegin(), input.cend(), [](const char c)
		{ return std::isalnum(c) || c == '-' || c == '_'; });
}

rpcn_settings_dialog::rpcn_settings_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("RPCN"));
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

	connect(btn_account, &QPushButton::clicked, this, [this]()
		{
			rpcn_account_dialog dlg(this);
			dlg.exec();
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
	setWindowTitle(tr("RPCN: Configuration"));
	setObjectName("rpcn_account_dialog");
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
	QLabel* label_token       = new QLabel(tr("Token:"));
	m_edit_token              = new QLineEdit();
	m_edit_token->setMaxLength(16);

	QPushButton* btn_create      = new QPushButton(tr("Create Account"), this);
	QPushButton* btn_resendtoken = new QPushButton(tr("Resend Token"), this);
	QPushButton* btn_changepass  = new QPushButton(tr("Change Password"), this);
	btn_changepass->setEnabled(false);
	QPushButton* btn_save = new QPushButton(tr("Save"), this);

	vbox_labels->addWidget(label_host);
	vbox_labels->addWidget(label_npid);
	vbox_labels->addWidget(label_pass);
	vbox_labels->addWidget(label_token);

	vbox_edits->addWidget(m_edit_host);
	vbox_edits->addWidget(m_edit_npid);
	vbox_edits->addWidget(btn_chg_pass);
	vbox_edits->addWidget(m_edit_token);

	hbox_buttons->addWidget(btn_create);
	hbox_buttons->addWidget(btn_resendtoken);
	hbox_buttons->addWidget(btn_changepass);
	hbox_buttons->addStretch();
	hbox_buttons->addWidget(btn_save);

	hbox_labels_and_edits->addLayout(vbox_labels);
	hbox_labels_and_edits->addLayout(vbox_edits);

	vbox_global->addLayout(hbox_labels_and_edits);
	vbox_global->addLayout(hbox_buttons);

	setLayout(vbox_global);

	connect(btn_chg_pass, &QAbstractButton::clicked, this, [this]()
		{
			rpcn_ask_password_dialog ask_pass;
			ask_pass.exec();

			auto password = ask_pass.get_password();
			if (!password)
				return;

			const std::string pass_str = password.value();
			const std::string salt_str = "No matter where you go, everybody's connected.";

			u8 salted_pass[SHA_DIGEST_SIZE];

			wolfSSL_PKCS5_PBKDF2_HMAC_SHA1(pass_str.c_str(), pass_str.size(), reinterpret_cast<const u8*>(salt_str.c_str()), salt_str.size(), 1000, SHA_DIGEST_SIZE, salted_pass);

			std::string hash("0000000000000000000000000000000000000000");
			for (u32 i = 0; i < 20; i++)
			{
				constexpr auto pal = "0123456789abcdef";
				hash[i * 2]        = pal[salted_pass[i] >> 4];
				hash[1 + i * 2]    = pal[salted_pass[i] & 15];
			}

			g_cfg_rpcn.set_password(hash);
			g_cfg_rpcn.save();

			QMessageBox::information(this, tr("RPCN Password Saved"), tr("Your password was saved successfully!"), QMessageBox::Ok);
		});

	connect(btn_save, &QAbstractButton::clicked, this, [this]()
		{
			if (save_config())
				close();
		});
	connect(btn_create, &QAbstractButton::clicked, this, &rpcn_account_dialog::create_account);
	connect(btn_resendtoken, &QAbstractButton::clicked, this, &rpcn_account_dialog::resend_token);

	g_cfg_rpcn.load();

	m_edit_host->setText(QString::fromStdString(g_cfg_rpcn.get_host()));
	m_edit_npid->setText(QString::fromStdString(g_cfg_rpcn.get_npid()));
	m_edit_token->setText(QString::fromStdString(g_cfg_rpcn.get_token()));
}

bool rpcn_account_dialog::save_config()
{
	const auto host  = m_edit_host->text().toStdString();
	const auto npid  = m_edit_npid->text().toStdString();
	const auto token = m_edit_token->text().toStdString();

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

	if (!validate_rpcn_username(npid))
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

void rpcn_account_dialog::create_account()
{
	// Validate and save
	if (!save_config())
		return;

	QString email;
	const QRegExpValidator simple_email_validator(QRegExp("^[a-zA-Z0-9.!#$%&’*+/=?^_`{|}~-]+@[a-zA-Z0-9-]+(?:\\.[a-zA-Z0-9-]+)*$"));

	while (true)
	{
		bool clicked_ok = false;
		email           = QInputDialog::getText(this, tr("Email address"), tr("An email address is required, please note:\n*A valid email is needed to validate your account.\n*Your email won't be used for anything beyond sending you the token.\n*Upon successful creation a token will be sent to your email which you'll need to login.\n\n"), QLineEdit::Normal, "", &clicked_ok);
		if (!clicked_ok)
			return;

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

	const auto rpcn = rpcn::rpcn_client::get_instance();

	const auto npid        = g_cfg_rpcn.get_npid();
	const auto online_name = npid;
	const auto avatar_url  = "https://rpcs3.net/cdn/netplay/DefaultAvatar.png";
	const auto password    = g_cfg_rpcn.get_password();

	if (auto result = rpcn->wait_for_connection(); result != rpcn::rpcn_state::failure_no_failure)
	{
		const QString error_message = tr("Failed to connect to RPCN server:\n%0").arg(rpcn_state_to_qstr(result));
		QMessageBox::critical(this, tr("Error Connecting"), error_message, QMessageBox::Ok);
		return;
	}

	if (auto error = rpcn->create_user(npid, password, online_name, avatar_url, email.toStdString()); error != rpcn::ErrorType::NoError)
	{
		QString error_message;
		switch (error)
		{
		case rpcn::ErrorType::CreationExistingUsername: error_message = tr("An account with that username already exists!"); break;
		case rpcn::ErrorType::CreationBannedEmailProvider: error_message = tr("This email provider is banned!"); break;
		case rpcn::ErrorType::CreationExistingEmail: error_message = tr("An account with that email already exists!"); break;
		case rpcn::ErrorType::CreationError: error_message = tr("Unknown creation error"); break;
		default: error_message = tr("Unknown error"); break;
		}
		QMessageBox::critical(this, tr("Error Creating Account"), tr("Failed to create the account:\n%0").arg(error_message), QMessageBox::Ok);
		return;
	}

	QMessageBox::information(this, tr("Account created!"), tr("Your account has been created successfully!\nCheck your email for your token!"), QMessageBox::Ok);
}

void rpcn_account_dialog::resend_token()
{
	if (!save_config())
		return;

	const auto rpcn = rpcn::rpcn_client::get_instance();

	const auto npid     = g_cfg_rpcn.get_npid();
	const auto password = g_cfg_rpcn.get_password();

	if (auto result = rpcn->wait_for_connection(); result != rpcn::rpcn_state::failure_no_failure)
	{
		const QString error_message = tr("Failed to connect to RPCN server:\n%0").arg(rpcn_state_to_qstr(result));
		QMessageBox::critical(this, tr("Error Connecting"), error_message, QMessageBox::Ok);
		return;
	}

	if (auto error = rpcn->resend_token(npid, password); error != rpcn::ErrorType::NoError)
	{
		QString error_message;
		switch (error)
		{
		case rpcn::ErrorType::Invalid: error_message = tr("The server has no email verification and doesn't need a token!"); break;
		case rpcn::ErrorType::LoginAlreadyLoggedIn: error_message = tr("You can't ask for your token while authentified!"); break;
		case rpcn::ErrorType::DbFail: error_message = tr("A database related error happened on the server!"); break;
		case rpcn::ErrorType::TooSoon: error_message = tr("You can only ask for a token mail once every 24 hours!"); break;
		case rpcn::ErrorType::EmailFail: error_message = tr("The mail couldn't be sent successfully!"); break;
		case rpcn::ErrorType::LoginError: error_message = tr("The login/password pair is invalid!"); break;
		default: error_message = tr("Unknown error"); break;
		}
		QMessageBox::critical(this, tr("Error Sending Token"), tr("Failed to send the token:\n%0").arg(error_message), QMessageBox::Ok);
		return;
	}

	QMessageBox::information(this, tr("Token Sent!"), tr("Your token was successfully resent to the email associated with your account!"), QMessageBox::Ok);
}

rpcn_ask_password_dialog::rpcn_ask_password_dialog(QWidget* parent)
	: QDialog(parent)
{
	QVBoxLayout* vbox_global  = new QVBoxLayout();
	QHBoxLayout* hbox_buttons = new QHBoxLayout();

	QGroupBox* gbox_password = new QGroupBox();
	QVBoxLayout* vbox_gbox = new QVBoxLayout();

	QLabel* label_pass1 = new QLabel(tr("Enter your password:"));
	m_edit_pass1        = new QLineEdit();
	m_edit_pass1->setEchoMode(QLineEdit::Password);
	QLabel* label_pass2 = new QLabel(tr("Enter your password a second time:"));
	m_edit_pass2        = new QLineEdit();
	m_edit_pass2->setEchoMode(QLineEdit::Password);

	vbox_gbox->addWidget(label_pass1);
	vbox_gbox->addWidget(m_edit_pass1);
	vbox_gbox->addWidget(label_pass2);
	vbox_gbox->addWidget(m_edit_pass2);
	gbox_password->setLayout(vbox_gbox);

	QPushButton* btn_ok     = new QPushButton(tr("Ok"));
	QPushButton* btn_cancel = new QPushButton(tr("Cancel"));

	hbox_buttons->addStretch();
	hbox_buttons->addWidget(btn_ok);
	hbox_buttons->addWidget(btn_cancel);

	vbox_global->addWidget(gbox_password);
	vbox_global->addLayout(hbox_buttons);

	setLayout(vbox_global);

	connect(btn_ok, &QAbstractButton::clicked, this, [this]()
		{
			if (m_edit_pass1->text() != m_edit_pass2->text())
			{
				QMessageBox::critical(this, tr("Wrong input"), tr("The two passwords you entered don't match!"), QMessageBox::Ok);
				return;
			}

			if (m_edit_pass1->text().isEmpty())
			{
				QMessageBox::critical(this, tr("Wrong input"), tr("You need to enter a password!"), QMessageBox::Ok);
				return;
			}

			m_password = m_edit_pass1->text().toStdString();
			close();
		});
	connect(btn_cancel, &QAbstractButton::clicked, this, [this]()
		{ this->close(); });
}

std::optional<std::string> rpcn_ask_password_dialog::get_password()
{
	return m_password;
}

void friend_callback(void* param, rpcn::NotificationType ntype, const std::string& username, bool status)
{
	auto* dlg = static_cast<rpcn_friends_dialog*>(param);
	dlg->callback_handler(ntype, username, status);
}

rpcn_friends_dialog::rpcn_friends_dialog(QWidget* parent)
	: QDialog(parent),
	  m_green_icon(gui::utils::circle_pixmap(QColorConstants::Svg::green, devicePixelRatioF() * 2)),
	  m_red_icon(gui::utils::circle_pixmap(QColorConstants::Svg::red, devicePixelRatioF() * 2)),
	  m_yellow_icon(gui::utils::circle_pixmap(QColorConstants::Svg::yellow, devicePixelRatioF() * 2)),
	  m_orange_icon(gui::utils::circle_pixmap(QColorConstants::Svg::orange, devicePixelRatioF() * 2)),
	  m_black_icon(gui::utils::circle_pixmap(QColorConstants::Svg::black, devicePixelRatioF() * 2))
{
	setWindowTitle(tr("RPCN: Friends"));
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

	vbox_global->addLayout(hbox_groupboxes);

	setLayout(vbox_global);

	// Tries to connect to RPCN
	m_rpcn = rpcn::rpcn_client::get_instance();

	if (auto res = m_rpcn->wait_for_connection(); res != rpcn::rpcn_state::failure_no_failure)
	{
		const QString error_msg = tr("Failed to connect to RPCN:\n%0").arg(rpcn_state_to_qstr(res));
		QMessageBox::warning(parent, tr("Error connecting to RPCN!"), error_msg, QMessageBox::Ok);
		return;
	}
	if (auto res = m_rpcn->wait_for_authentified(); res != rpcn::rpcn_state::failure_no_failure)
	{
		const QString error_msg = tr("Failed to authentify to RPCN:\n%0").arg(rpcn_state_to_qstr(res));
		QMessageBox::warning(parent, tr("Error authentifying to RPCN!"), error_msg, QMessageBox::Ok);
		return;
	}

	// Get friends, setup callback and setup comboboxes
	rpcn::friend_data data;
	m_rpcn->get_friends_and_register_cb(data, friend_callback, this);

	for (const auto& fr : data.friends)
	{
		add_update_list(m_lst_friends, QString::fromStdString(fr.first), fr.second.first ? m_green_icon : m_red_icon, fr.second.first);
	}

	for (const auto& fr_req : data.requests_sent)
	{
		add_update_list(m_lst_requests, QString::fromStdString(fr_req), m_orange_icon, QVariant(false));
	}

	for (const auto& fr_req : data.requests_received)
	{
		add_update_list(m_lst_requests, QString::fromStdString(fr_req), m_yellow_icon, QVariant(true));
	}

	for (const auto& blck : data.blocked)
	{
		add_update_list(m_lst_blocks, QString::fromStdString(blck), m_red_icon, QVariant(false));
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
						QMessageBox::critical(this, tr("Error removing a friend!"), tr("An error occured trying to remove a friend!"), QMessageBox::Ok);
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

			// Only create context menu for incoming requests
			if (selected_item->data(Qt::UserRole) == false)
			{
				return;
			}

			std::string str_sel_friend = selected_item->text().toStdString();

			QMenu* context_menu           = new QMenu();
			QAction* remove_friend_action = context_menu->addAction(tr("&Accept Request"));

			connect(remove_friend_action, &QAction::triggered, this, [this, str_sel_friend]()
				{
					if (!m_rpcn->add_friend(str_sel_friend))
					{
						QMessageBox::critical(this, tr("Error adding a friend!"), tr("An error occured trying to add a friend!"), QMessageBox::Ok);
					}
					else
					{
						QMessageBox::information(this, tr("Friend added!"), tr("You've successfully added a friend!"), QMessageBox::Ok);
					}
				});

			context_menu->exec(m_lst_requests->viewport()->mapToGlobal(pos));
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

				QMessageBox::critical(this, tr("Error validating username"), tr("The username you entered is invalid"), QMessageBox::Ok);
			}

			if (!m_rpcn->add_friend(str_friend_username))
			{
				QMessageBox::critical(this, tr("Error adding friend"), tr("An error occured adding friend"), QMessageBox::Ok);
			}
			else
			{
				add_update_list(m_lst_requests, QString::fromStdString(str_friend_username), m_orange_icon, QVariant(false));
				QMessageBox::information(this, tr("Friend added"), tr("Friend was successfully added!"), QMessageBox::Ok);
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

void rpcn_friends_dialog::add_update_friend(QString name, bool status)
{
	add_update_list(m_lst_friends, name, status ? m_green_icon : m_red_icon, status);
	remove_list(m_lst_requests, name);
}

void rpcn_friends_dialog::remove_friend(QString name)
{
	remove_list(m_lst_friends, name);
	remove_list(m_lst_requests, name);
}

void rpcn_friends_dialog::add_query(QString name)
{
	add_update_list(m_lst_requests, name, m_yellow_icon, QVariant(true));
}

void rpcn_friends_dialog::callback_handler(rpcn::NotificationType ntype, std::string username, bool status)
{
	QString qtr_username = QString::fromStdString(username);
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
	default:
	{
		rpcn_settings_log.fatal("An unhandled notification type was received by the rpcn friends dialog callback!");
		break;
	}
	}
}
