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
	bool save_config();

private Q_SLOTS:	
	void create_account();
	void resend_token();

protected:
	QLineEdit *m_edit_host, *m_edit_npid, *m_edit_token;
};

class rpcn_ask_password_dialog : public QDialog
{
	Q_OBJECT
public:
	rpcn_ask_password_dialog(QWidget* parent = nullptr);
	std::optional<std::string> get_password();

private:
	QLineEdit *m_edit_pass1, *m_edit_pass2;
	std::optional<std::string> m_password;
};

class rpcn_friends_dialog : public QDialog
{
	Q_OBJECT
public:
	rpcn_friends_dialog(QWidget* parent = nullptr);
	~rpcn_friends_dialog();
	void callback_handler(rpcn::NotificationType ntype, std::string username, bool status);
	bool is_ok() const;

private:
	void add_update_list(QListWidget* list, const QString& name, const QIcon& icon, const QVariant& data);
	void remove_list(QListWidget* list, const QString& name);

private Q_SLOTS:
	void add_update_friend(QString name, bool status);
	void remove_friend(QString name);
	void add_query(QString name);

Q_SIGNALS:
	void signal_add_update_friend(QString name, bool status);
	void signal_remove_friend(QString name);
	void signal_add_query(QString name);

private:
	const QIcon m_green_icon, m_red_icon, m_yellow_icon, m_orange_icon, m_black_icon;
	// list of friends
	QListWidget* m_lst_friends = nullptr;
	// list of friend requests sent by/to the current user
	QListWidget* m_lst_requests = nullptr;
	// list of people blocked by the user
	QListWidget* m_lst_blocks = nullptr;

	std::shared_ptr<rpcn::rpcn_client> m_rpcn;
	bool m_rpcn_ok = false;
};
