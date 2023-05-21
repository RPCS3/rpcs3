#pragma once

#include <QObject>
#include <QListWidget>

#include "util/types.hpp"
#include "custom_dialog.h"
#include "Emu/NP/rpcn_client.h"

class sendmessage_dialog_frame : public QObject, public SendMessageDialogBase
{
	Q_OBJECT

public:
	sendmessage_dialog_frame() = default;
	~sendmessage_dialog_frame();
	bool Exec(message_data& msg_data, std::set<std::string>& npids) override;
	void callback_handler(rpcn::NotificationType ntype, const std::string& username, bool status);

private:
	void add_friend(QListWidget* list, const QString& name);
	void remove_friend(QListWidget* list, const QString& name);

private:
	custom_dialog* m_dialog  = nullptr;
	QListWidget* m_lst_friends = nullptr;

	std::shared_ptr<rpcn::rpcn_client> m_rpcn;

Q_SIGNALS:
	void signal_add_friend(QString name);
	void signal_remove_friend(QString name);

private Q_SLOTS:
	void slot_add_friend(QString name);
	void slot_remove_friend(QString name);
};
