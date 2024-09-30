#pragma once

#include <optional>

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>

#include "Emu/NP/rpcn_client.h"

class rpcn_settings_dialog : public QDialog
{
	Q_OBJECT
public:
	rpcn_settings_dialog(QWidget* parent = nullptr);
};

class rpcn_account_dialog : public QDialog
{
	Q_OBJECT
public:
	rpcn_account_dialog(QWidget* parent = nullptr);

private:
	void refresh_combobox();

private:
	QComboBox* cbx_servers = nullptr;
};

class rpcn_add_server_dialog : public QDialog
{
	Q_OBJECT
public:
	rpcn_add_server_dialog(QWidget* parent = nullptr);
	const std::optional<std::pair<std::string, std::string>>& get_new_server() const;

private:
	std::optional<std::pair<std::string, std::string>> m_new_server;
};

class rpcn_ask_username_dialog : public QDialog
{
	Q_OBJECT
public:
	rpcn_ask_username_dialog(QWidget* parent, const QString& description);
	const std::optional<std::string>& get_username() const;

private:
	std::optional<std::string> m_username;
};

class rpcn_ask_password_dialog : public QDialog
{
	Q_OBJECT
public:
	rpcn_ask_password_dialog(QWidget* parent, const QString& description);
	const std::optional<std::string>& get_password() const;

private:
	std::optional<std::string> m_password;
};

class rpcn_ask_email_dialog : public QDialog
{
	Q_OBJECT
public:
	rpcn_ask_email_dialog(QWidget* parent, const QString& description);
	const std::optional<std::string>& get_email() const;

private:
	std::optional<std::string> m_email;
};

class rpcn_ask_token_dialog : public QDialog
{
	Q_OBJECT
public:
	rpcn_ask_token_dialog(QWidget* parent, const QString& description);
	const std::optional<std::string>& get_token() const;

private:
	std::optional<std::string> m_token;
};

class rpcn_account_edit_dialog : public QDialog
{
	Q_OBJECT
public:
	rpcn_account_edit_dialog(QWidget* parent = nullptr);

private:
	bool save_config();

private Q_SLOTS:
	void resend_token();
	void change_password();

protected:
	QLineEdit *m_edit_username, *m_edit_token;
};

class rpcn_friends_dialog : public QDialog
{
	Q_OBJECT
public:
	rpcn_friends_dialog(QWidget* parent = nullptr);
	~rpcn_friends_dialog();
	void callback_handler(rpcn::NotificationType ntype, const std::string& username, bool status);
	bool is_ok() const;

private:
	void add_update_list(QListWidget* list, const QString& name, const QIcon& icon, const QVariant& data);
	void remove_list(QListWidget* list, const QString& name);

private Q_SLOTS:
	void add_update_friend(const QString& name, bool status);
	void remove_friend(const QString& name);
	void add_query(const QString& name);

Q_SIGNALS:
	void signal_add_update_friend(const QString& name, bool status);
	void signal_remove_friend(const QString& name);
	void signal_add_query(const QString& name);

private:
	QIcon m_icon_online;
	QIcon m_icon_offline;
	QIcon m_icon_blocked;
	QIcon m_icon_request_received;
	QIcon m_icon_request_sent;

	// list of friends
	QListWidget* m_lst_friends = nullptr;
	// list of friend requests sent by/to the current user
	QListWidget* m_lst_requests = nullptr;
	// list of people blocked by the user
	QListWidget* m_lst_blocks = nullptr;
	// list of players in history
	QListWidget* m_lst_history = nullptr;

	std::shared_ptr<rpcn::rpcn_client> m_rpcn;
	bool m_rpcn_ok = false;
};
