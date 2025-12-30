#include <QMessageBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLineEdit>

#include "clans_settings_dialog.h"
#include "Emu/NP/clans_config.h"

clans_settings_dialog::clans_settings_dialog(QWidget* parent)
	: QDialog(parent)
{
	g_cfg_clans.load();

	setWindowTitle(tr("Clans Configuration"));
	setObjectName("clans_settings_dialog");

	QVBoxLayout* vbox_global = new QVBoxLayout();

	QGroupBox* grp_server    = new QGroupBox(tr("Server:"));
	QVBoxLayout* vbox_server = new QVBoxLayout();

	QHBoxLayout* hbox_lbl_combo = new QHBoxLayout();
	QLabel* lbl_server          = new QLabel(tr("Server:"));
	m_cbx_servers                 = new QComboBox();
	m_cbx_protocol                = new QComboBox();

	m_cbx_protocol->addItem("HTTPS");
	m_cbx_protocol->addItem("HTTP");
	m_cbx_protocol->setCurrentIndex(g_cfg_clans.get_use_https() ? 0 : 1);

	refresh_combobox();

	hbox_lbl_combo->addWidget(lbl_server);
	hbox_lbl_combo->addWidget(m_cbx_servers);
	hbox_lbl_combo->addWidget(m_cbx_protocol);

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

	setLayout(vbox_global);

	connect(m_cbx_servers, &QComboBox::currentIndexChanged, this, [this](int index)
		{
			if (index < 0)
				return;

			QVariant host = m_cbx_servers->itemData(index);

			if (!host.isValid() || !host.canConvert<QString>())
				return;

			g_cfg_clans.set_host(host.toString().toStdString());
			g_cfg_clans.save();
		});

	connect(m_cbx_protocol, &QComboBox::currentIndexChanged, this, [this](int index)
		{
			if (index < 0)
				return;

			g_cfg_clans.set_use_https(index == 0);
			g_cfg_clans.save();
		});

	connect(btn_add_server, &QAbstractButton::clicked, this, [this]()
		{
			clans_add_server_dialog dlg(this);
			dlg.exec();
			const auto& new_server = dlg.get_new_server();
			if (new_server)
			{
				if (!g_cfg_clans.add_host(new_server->first, new_server->second))
				{
					QMessageBox::critical(this, tr("Existing Server"), tr("You already have a server with this description & hostname in the list."), QMessageBox::Ok);
					return;
				}

				g_cfg_clans.save();
				refresh_combobox();
			}
		});

	connect(btn_del_server, &QAbstractButton::clicked, this, [this]()
		{
			const int index = m_cbx_servers->currentIndex();

			if (index < 0)
				return;

			const std::string desc = m_cbx_servers->itemText(index).toStdString();
			const std::string host = m_cbx_servers->itemData(index).toString().toStdString();

			if (g_cfg_clans.del_host(desc, host))
			{
				g_cfg_clans.save();
				refresh_combobox();
			}
			else
			{
				QMessageBox::warning(this, tr("Cannot Delete"), tr("This server cannot be deleted."), QMessageBox::Ok);
			}
		});
}

void clans_settings_dialog::refresh_combobox()
{
	g_cfg_clans.load();
	const auto vec_hosts = g_cfg_clans.get_hosts();
	const auto cur_host  = g_cfg_clans.get_host();
	int i = 0, index = 0;

	m_cbx_servers->clear();

	for (const auto& [desc, host] : vec_hosts)
	{
		m_cbx_servers->addItem(QString::fromStdString(desc), QString::fromStdString(host));
		if (cur_host == host)
			index = i;

		i++;
	}

	m_cbx_servers->setCurrentIndex(index);
}

clans_add_server_dialog::clans_add_server_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Clans: Add Server"));
	setObjectName("clans_add_server_dialog");
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

const std::optional<std::pair<std::string, std::string>>& clans_add_server_dialog::get_new_server() const
{
	return m_new_server;
}
