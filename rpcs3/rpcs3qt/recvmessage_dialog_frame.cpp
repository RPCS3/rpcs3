#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>

#include "recvmessage_dialog_frame.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/NP/rpcn_client.h"

#include "util/logs.hpp"

LOG_CHANNEL(recvmessage_dlg_log, "recvmessage dlg");

void recvmessage_callback(void* param, shared_ptr<std::pair<std::string, message_data>> new_msg, u64 msg_id)
{
	auto* dlg = static_cast<recvmessage_dialog_frame*>(param);
	dlg->callback_handler(std::move(new_msg), msg_id);
}

recvmessage_dialog_frame::~recvmessage_dialog_frame()
{
	if (m_dialog)
	{
		m_dialog->deleteLater();
	}
}

error_code recvmessage_dialog_frame::Exec(SceNpBasicMessageMainType type, SceNpBasicMessageRecvOptions options, SceNpBasicMessageRecvAction& recv_result, u64& chosen_msg_id)
{
	qRegisterMetaType<recvmessage_signal_struct>();

	if (m_dialog)
	{
		m_dialog->close();
		delete m_dialog;
	}

	m_dialog = new custom_dialog(false);
	m_dialog->setModal(true);

	m_dialog->setWindowTitle(tr("Choose message:"));

	m_rpcn = rpcn::rpcn_client::get_instance(0, true);

	QVBoxLayout* vbox_global = new QVBoxLayout();

	m_lst_messages = new QListWidget();
	vbox_global->addWidget(m_lst_messages);

	QHBoxLayout* hbox_btns = new QHBoxLayout();
	hbox_btns->addStretch();
	QPushButton* btn_accept = new QPushButton(tr("Accept"));
	QPushButton* btn_deny   = new QPushButton(tr("Deny"));
	QPushButton* btn_cancel = new QPushButton(tr("Cancel"));
	hbox_btns->addWidget(btn_accept);
	hbox_btns->addWidget(btn_deny);
	hbox_btns->addWidget(btn_cancel);
	vbox_global->addLayout(hbox_btns);

	m_dialog->setLayout(vbox_global);

	error_code result           = CELL_CANCEL;
	const bool preserve         = options & SCE_NP_BASIC_RECV_MESSAGE_OPTIONS_PRESERVE;
	const bool include_bootable = options & SCE_NP_BASIC_RECV_MESSAGE_OPTIONS_INCLUDE_BOOTABLE;

	auto accept_or_deny = [preserve, this, &result, &recv_result, &chosen_msg_id](SceNpBasicMessageRecvAction result_from_action)
	{
		auto selected = m_lst_messages->selectedItems();
		if (selected.empty())
		{
			QMessageBox::critical(m_dialog, tr("Error receiving a message!"), tr("You must select a message!"), QMessageBox::Ok);
			return;
		}

		chosen_msg_id = selected[0]->data(Qt::UserRole).toULongLong();
		recv_result   = result_from_action;
		result        = CELL_OK;

		if (!preserve)
		{
			m_rpcn->mark_message_used(chosen_msg_id);
		}

		m_dialog->close();
	};

	connect(btn_accept, &QAbstractButton::clicked, this, [&accept_or_deny]()
		{ accept_or_deny(SCE_NP_BASIC_MESSAGE_ACTION_ACCEPT); });
	connect(btn_deny, &QAbstractButton::clicked, this, [&accept_or_deny]()
		{ accept_or_deny(SCE_NP_BASIC_MESSAGE_ACTION_DENY); });
	connect(btn_cancel, &QAbstractButton::clicked, this, [this]()
		{ m_dialog->close(); });
	connect(this, &recvmessage_dialog_frame::signal_new_message, this, &recvmessage_dialog_frame::slot_new_message);

	// Get list of messages
	const auto messages = m_rpcn->get_messages_and_register_cb(type, include_bootable, recvmessage_callback, this);
	for (const auto& [id, message] : messages)
	{
		add_message(message, id);
	}

	auto& nps = g_fxo->get<np_state>();

	QTimer timer;
	connect(&timer, &QTimer::timeout, this, [this, &nps, &timer]()
	{
		bool abort = Emu.IsStopped();

		if (!abort && nps.abort_gui_flag.exchange(false))
		{
			recvmessage_dlg_log.warning("Aborted by sceNp!");
			abort = true;
		}

		if (abort)
		{
			if (m_dialog)
			{
				m_dialog->close();
			}

			timer.stop();
		}
	});
	timer.start(10ms);

	m_dialog->exec();

	m_rpcn->remove_message_cb(recvmessage_callback, this);

	return result;
}

void recvmessage_dialog_frame::add_message(const shared_ptr<std::pair<std::string, message_data>>& msg, u64 msg_id)
{
	ensure(msg);
	auto new_item = new QListWidgetItem(QString::fromStdString(msg->first));
	new_item->setData(Qt::UserRole, static_cast<qulonglong>(msg_id));
	m_lst_messages->addItem(new_item);
}

void recvmessage_dialog_frame::slot_new_message(recvmessage_signal_struct msg_and_id)
{
	add_message(msg_and_id.msg, msg_and_id.msg_id);
}

void recvmessage_dialog_frame::callback_handler(shared_ptr<std::pair<std::string, message_data>> new_msg, u64 msg_id)
{
	recvmessage_signal_struct signal_struct = {
		.msg    = new_msg,
		.msg_id = msg_id,
	};

	Q_EMIT signal_new_message(signal_struct);
}
