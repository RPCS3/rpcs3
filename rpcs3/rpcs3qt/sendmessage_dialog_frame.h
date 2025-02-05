#pragma once

#include <QObject>
#include <QListWidget>

#include "util/types.hpp"
#include "custom_dialog.h"

#include "Emu/Cell/Modules/sceNp.h"

class sendmessage_dialog_frame : public QObject, public SendMessageDialogBase
{
	Q_OBJECT

public:
	sendmessage_dialog_frame() = default;
	~sendmessage_dialog_frame();
	error_code Exec(message_data& msg_data, std::set<std::string>& npids) override;
	void callback_handler(rpcn::NotificationType ntype, const std::string& username, bool status) override;

private:
	void add_friend(QListWidget* list, const QString& name);
	void remove_friend(QListWidget* list, const QString& name);

private:
	custom_dialog* m_dialog = nullptr;
	QListWidget* m_lst_friends = nullptr;

Q_SIGNALS:
	void signal_add_friend(QString name);
	void signal_remove_friend(QString name);

private Q_SLOTS:
	void slot_add_friend(QString name);
	void slot_remove_friend(QString name);
};
