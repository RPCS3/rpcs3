#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>

#include "sendmessage_dialog_frame.h"
#include "Emu/IdManager.h"
#include "Emu/NP/rpcn_client.h"
#include "Emu/System.h"

#include "util/logs.hpp"

LOG_CHANNEL(sendmessage_dlg_log, "sendmessage dlg");

void sendmessage_friend_callback(void* param, rpcn::NotificationType ntype, const std::string& username, bool status)
{
	auto* dlg = static_cast<sendmessage_dialog_frame*>(param);
	dlg->callback_handler(ntype, username, status);
}

sendmessage_dialog_frame::~sendmessage_dialog_frame()
{
	if (m_dialog)
	{
		m_dialog->deleteLater();
	}
}

error_code sendmessage_dialog_frame::Exec(message_data& msg_data, std::set<std::string>& npids)
{
	if (m_dialog)
	{
		m_dialog->close();
		delete m_dialog;
	}

	m_dialog = new custom_dialog(false);
	m_dialog->setModal(true);

	m_dialog->setWindowTitle(tr("Choose friend to message:"));

	m_rpcn = rpcn::rpcn_client::get_instance(0, true);

	QVBoxLayout* vbox_global = new QVBoxLayout();

	m_lst_friends = new QListWidget();
	vbox_global->addWidget(m_lst_friends);

	QHBoxLayout* hbox_btns = new QHBoxLayout();
	hbox_btns->addStretch();
	QPushButton* btn_ok     = new QPushButton(tr("Ok"));
	QPushButton* btn_cancel = new QPushButton(tr("Cancel"));
	hbox_btns->addWidget(btn_ok);
	hbox_btns->addWidget(btn_cancel);
	vbox_global->addLayout(hbox_btns);

	m_dialog->setLayout(vbox_global);

	connect(this, &sendmessage_dialog_frame::signal_add_friend, this, &sendmessage_dialog_frame::slot_add_friend);
	connect(this, &sendmessage_dialog_frame::signal_remove_friend, this, &sendmessage_dialog_frame::slot_remove_friend);

	error_code result = CELL_CANCEL;

	connect(btn_ok, &QAbstractButton::clicked, this, [this, &msg_data, &npids, &result]()
		{
			// Check one target is selected
			auto selected = m_lst_friends->selectedItems();
			if (selected.empty())
			{
				QMessageBox::critical(m_dialog, tr("Error sending a message!"), tr("You must select a friend!"), QMessageBox::Ok);
				return;
			}

			npids.insert(selected[0]->text().toStdString());

			// Send the message
			if (m_rpcn->send_message(msg_data, npids))
			{
				result = CELL_OK;
			}
			m_dialog->close();
		});

	connect(btn_cancel, &QAbstractButton::clicked, m_dialog, &custom_dialog::close);

	rpcn::friend_data data;
	m_rpcn->get_friends_and_register_cb(data, sendmessage_friend_callback, this);

	for (const auto& fr : data.friends)
	{
		// Only add online friends to the list
		if (fr.second.online)
		{
			add_friend(m_lst_friends, QString::fromStdString(fr.first));
		}
	}

	auto& nps = g_fxo->get<np_state>();

	QTimer timer;
	connect(&timer, &QTimer::timeout, this, [this, &nps, &timer]()
	{
		bool abort = Emu.IsStopped();

		if (!abort && nps.abort_gui_flag.exchange(false))
		{
			sendmessage_dlg_log.warning("Aborted by sceNp!");
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

	m_rpcn->remove_friend_cb(sendmessage_friend_callback, this);

	return result;
}

void sendmessage_dialog_frame::add_friend(QListWidget* list, const QString& name)
{
	if (auto found = list->findItems(name, Qt::MatchExactly); !found.empty())
	{
		return;
	}

	list->addItem(new QListWidgetItem(name));
}

void sendmessage_dialog_frame::remove_friend(QListWidget* list, const QString& name)
{
	if (auto found = list->findItems(name, Qt::MatchExactly); !found.empty())
	{
		delete list->takeItem(list->row(found[0]));
	}
}

void sendmessage_dialog_frame::slot_add_friend(QString name)
{
	add_friend(m_lst_friends, name);
}

void sendmessage_dialog_frame::slot_remove_friend(QString name)
{
	remove_friend(m_lst_friends, name);
}

void sendmessage_dialog_frame::callback_handler(rpcn::NotificationType ntype, const std::string& username, bool status)
{
	QString qtr_username = QString::fromStdString(username);
	switch (ntype)
	{
	case rpcn::NotificationType::FriendQuery: // Other user sent a friend request
	case rpcn::NotificationType::FriendPresenceChanged:
		break;
	case rpcn::NotificationType::FriendNew: // Add a friend to the friendlist(either accepted a friend request or friend accepted it)
	{
		if (status)
		{
			Q_EMIT signal_add_friend(qtr_username);
		}
		break;
	}
	case rpcn::NotificationType::FriendLost: // Remove friend from the friendlist(user removed friend or friend removed friend)
	{
		Q_EMIT signal_remove_friend(qtr_username);
		break;
	}
	case rpcn::NotificationType::FriendStatus: // Set status of friend to Offline or Online
	{
		if (status)
		{
			Q_EMIT signal_add_friend(qtr_username);
		}
		else
		{
			Q_EMIT signal_remove_friend(qtr_username);
		}
		break;
	}
	default:
	{
		sendmessage_dlg_log.fatal("An unhandled notification type was received by the sendmessage dialog callback!");
		break;
	}
	}
}
