#include <QMessageBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QIntValidator>

#include "ipc_settings_dialog.h"
#include "Emu/IPC_config.h"
#include "Emu/IPC_socket.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"

ipc_settings_dialog::ipc_settings_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("IPC Settings"));
	setObjectName("ipc_settings_dialog");
	setMinimumSize(QSize(200, 100));

	QVBoxLayout* vbox_global = new QVBoxLayout();

	QCheckBox* checkbox_server_enabled = new QCheckBox(tr("Enable IPC Server"));

	QGroupBox* group_server_port = new QGroupBox(tr("IPC Server Port"));
	QHBoxLayout* hbox_group_port = new QHBoxLayout();
	QLineEdit* line_edit_server_port = new QLineEdit();
	line_edit_server_port->setValidator(new QIntValidator(1025, 65535, this));

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
	buttons->button(QDialogButtonBox::Save)->setDefault(true);

	hbox_group_port->addWidget(line_edit_server_port);
	group_server_port->setLayout(hbox_group_port);

	vbox_global->addWidget(checkbox_server_enabled);
	vbox_global->addWidget(group_server_port);
	vbox_global->addWidget(buttons);

	setLayout(vbox_global);

	connect(buttons, &QDialogButtonBox::accepted, this, [this, checkbox_server_enabled, line_edit_server_port]()
		{
			bool ok = true;
			const bool server_enabled = checkbox_server_enabled->isChecked();
			const int server_port = line_edit_server_port->text().toInt(&ok);

			if (!ok || server_port < 1025 || server_port > 65535)
			{
				QMessageBox::critical(this, tr("Invalid port"), tr("The server port must be an integer in the range 1025 - 65535!"), QMessageBox::Ok);
				return;
			}

			g_cfg_ipc.set_server_enabled(server_enabled);
			g_cfg_ipc.set_port(server_port);
			g_cfg_ipc.save();

			if (auto manager = g_fxo->try_get<IPC_socket::IPC_server_manager>())
			{
				manager->set_server_enabled(server_enabled);
			}
			else if (server_enabled && Emu.IsRunning())
			{
				g_fxo->init<IPC_socket::IPC_server_manager>(true);
			}

			accept();
		});

	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	g_cfg_ipc.load();

	checkbox_server_enabled->setChecked(g_cfg_ipc.get_server_enabled());
	line_edit_server_port->setText(QString::number(g_cfg_ipc.get_port()));
}
