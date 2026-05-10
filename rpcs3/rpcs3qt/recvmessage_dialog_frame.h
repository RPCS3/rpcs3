#pragma once

#include <QObject>
#include <QListWidget>

#include "util/types.hpp"
#include "util/shared_ptr.hpp"
#include "custom_dialog.h"
#include "Emu/Cell/Modules/sceNp.h"

struct recvmessage_signal_struct
{
	shared_ptr<std::pair<std::string, message_data>> msg;
	u64 msg_id = 0;
};

Q_DECLARE_METATYPE(recvmessage_signal_struct);

class recvmessage_dialog_frame : public QObject, public RecvMessageDialogBase
{
	Q_OBJECT

public:
	recvmessage_dialog_frame() = default;
	~recvmessage_dialog_frame();
	error_code Exec(SceNpBasicMessageMainType type, SceNpBasicMessageRecvOptions options, SceNpBasicMessageRecvAction& recv_result, u64& chosen_msg_id) override;
	void callback_handler(const shared_ptr<std::pair<std::string, message_data>> new_msg, u64 msg_id) override;

private:
	void add_message(const shared_ptr<std::pair<std::string, message_data>>& msg, u64 msg_id);

Q_SIGNALS:
	void signal_new_message(const recvmessage_signal_struct msg_and_id);

private Q_SLOTS:
	void slot_new_message(const recvmessage_signal_struct msg_and_id);

private:
	custom_dialog* m_dialog = nullptr;
	QListWidget* m_lst_messages = nullptr;
};
