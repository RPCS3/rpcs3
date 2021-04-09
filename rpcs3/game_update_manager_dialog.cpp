#include <QMessageBox>
#include <rpcs3qt/qt_utils.h>
#include "rpcs3qt/pkg_install_dialog.h"
#include "rpcs3qt/main_window.h"
#include <rpcs3qt/game_list_frame.h>
#include "rpcs3qt/downloader.h"
#include <pugixml.hpp>
#include "util/logs.hpp"
#include <Crypto/unpkg.h>
#include <Utilities/Thread.h>
#include "Utilities/File.h"
#include "game_update_manager_dialog.h"
#include "ui_game_update_manager_dialog.h"
LOG_CHANNEL(game_update_manger_log, "Game Update Manger");

enum patch_column : int
{
	enabled,
	serials,
	title,
	game_ver,
	size,
	hash,
	sysver,
	patch_ver
};

enum patch_role : int
{
	hash_role = Qt::UserRole,
	serial_role,
	title_role,
	app_version_role,
	size_role,
	sysver_role,
	patch_ver_role,
	url_role,
	persistance_role,
	node_level_role
};

enum node_level : int
{
	title_level,
	serial_level,
	patch_level
};

game_update_manager_dialog::game_update_manager_dialog(GameInfo G, std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<persistent_settings> persistent_settings, game_compatibility* compat, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::game_update_manager_dialog)
	, m_gui_settings(gui_settings)
	, m_persistent_settings(persistent_settings)
	, m_game_compatibillty(compat)
{
	ui->setupUi(this);
	setModal(true);
	m_downloader = new downloader(this);
	P_m_downloader = new downloader(this);
	connect(ui->patches, &QTreeWidget::currentItemChanged, this, &game_update_manager_dialog::handle_item_selected);
	connect(ui->DLI, &QPushButton::clicked, this, &game_update_manager_dialog::download);
	connect(m_downloader, &downloader::signal_download_error, this, [this](const QString& /*error*/) {
		QMessageBox::warning(this, tr("Patch downloader"), tr("An error occurred during the download process.\nCheck the log for more information."));
	});
	connect(P_m_downloader, &downloader::signal_download_error, this, [this](const QString& /*error*/) {
		QMessageBox::warning(this, tr("Patch downloader"), tr("An error occurred during the download process.\nCheck the log for more information."));
	});
	connect(P_m_downloader, &downloader::signal_download_finished, this, [this](const QByteArray& data) {
		game_update_manger_log.warning("Downloaded Game Update");
		install(data);
	});
	connect(m_downloader, &downloader::signal_download_finished, this, [this](const QByteArray& data) {
		const bool result_XML = handle_XML(data, m_Gameinfo.name, m_Gameinfo.app_ver);
		if (!result_XML)
		{
			QMessageBox::warning(this, tr("Patch downloader"), tr("An error occurred during the download process.\nCheck the log for more information."));
		}
	});
	m_Gameinfo = G;
	// https://a0.ww.np.dl.playstation.net/tpl/np/{game_id}/{game_id}-ver.xml
	game_update_manger_log.warning("Atempting to download Update XML");
	std::string url        = "https://a0.ww.np.dl.playstation.net/tpl/np/" + m_Gameinfo.serial + "/" + m_Gameinfo.serial + "-ver.xml";
	const std::string path = "update/" + m_Gameinfo.serial + "-ver.xml";
	download_update(url);
	game_update_manger_log.warning("Downloaded Update XML");
}

inline std::string sstr(const QString& _in)
{
	return _in.toStdString();
}

void game_update_manager_dialog::install(const QByteArray& data)
{
	game_update_manger_log.warning("Atempting to write and install game update");
	const QString pkgpath = QCoreApplication::applicationDirPath() + "/temp/temp.pkg";
	std::vector<compat::package_info> packages;
	QStringList QSLPKGPATH = QStringList();
	QSLPKGPATH.push_back(pkgpath);
	QFile F(pkgpath);
	F.open(QIODevice::ReadWrite);
	F.write(data);
	F.flush();
	F.close();
	F.waitForBytesWritten(data.size());
	packages.push_back(game_compatibility::GetPkgInfo(pkgpath, m_game_compatibillty));
	pkg_install_dialog dlg = pkg_install_dialog(QSLPKGPATH, m_game_compatibillty);
	dlg.exec();
	game_update_manger_log.warning("Waiting for user to click continue..");
	// Synchronization variable
	atomic_t<double> progress(0.);
	package_error error = package_error::no_error;
	bool cancelled = false;
	for (size_t i = 0, count = QSLPKGPATH.size(); i < count; i++)
	{
		progress = 0.;
	    const compat::package_info& package = packages.at(i);
		QString app_info                    = package.title; // This should always be non-empty

		if (!package.title_id.isEmpty() || !package.version.isEmpty())
		{
			app_info += QStringLiteral("\n");

			if (!package.title_id.isEmpty())
			{
				app_info += package.title_id;
			}

			if (!package.version.isEmpty())
			{
				if (!package.title_id.isEmpty())
				{
					app_info += " ";
				}

				app_info += tr("v.%0", "Package version for install progress dialog").arg(package.version);
			}
		}
		const QFileInfo file_info(package.path);
		const std::string path      = sstr(package.path);
		const std::string file_name = sstr(file_info.fileName());

		// Run PKG unpacking asynchronously
		named_thread worker("PKG Installer", [path, &progress, &error] {
			package_reader reader(path);
			error = reader.check_target_app_version();

			if (error == package_error::no_error)
			{
				return reader.extract_data(progress);
			}
			return false;
		});

		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), worker <= thread_state::aborting)
		{
		}
	}
	game_update_manger_log.warning("Update installed.. cleaning..");
	fs::remove_file(pkgpath.toStdString());
}

void game_update_manager_dialog::download()
{
	if (ui->patches->currentItem()->isSelected())
	{
		game_update_manger_log.warning("Atempting to download game update");
		std::string url = ui->patches->currentItem()->data(0, url_role).toString().toStdString();
		P_m_downloader->startU(url, true, false, tr("Downloading selected patches"));
	}
	else
	{
		QMessageBox::warning(this, tr("Error"), "Invailded item or selected item is not checked");
	}
}

void game_update_manager_dialog::download_update(std::string Url)
{
	m_downloader->startU(Url, true, false, tr("Downloading latest patches"));
}

void game_update_manager_dialog::handle_item_selected(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/)
{
	if (!current)
	{
		// Clear patch info if no item is selected
		update_patch_info({});
		return;
	}
	game_update_manager_dialog::gui_patch_info info{};
	info.size = current->data(0, size_role).toString();
	info.hash = current->data(0, hash_role).toString();
	info.rsysv = current->data(0, sysver_role).toString();
	info.pv = current->data(0, patch_ver_role).toString();
	info.serial = current->data(0, serial_role).toString();
	info.Title  = current->data(0, title_role).toString();
	info.currentver = current->data(0, app_version_role).toString();
	update_patch_info(info);
}

bool game_update_manager_dialog::handle_XML(const QByteArray& data, std::string name, std::string app_ver)
{
	game_update_manger_log.warning("Atempting to parse downloaded XML");
	std::string xml = data.toStdString();
	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load(xml.c_str());
	if (!res)
	{
		std::string des(res.description());
		std::string pos((char*)res.offset);
		std::string de = "Parse error: " + des + ", character pos= " + pos;
		QMessageBox::warning(this, tr("XML Parser"), tr("An error occurred during the XML Parseing process.\nCheck the log for more information."));
		return false;
	}
	pugi::xml_node root = doc.document_element();
	QTreeWidgetItem* title_level_item = nullptr;
	if (!title_level_item)
	{
		title_level_item = new QTreeWidgetItem();
		title_level_item->setText(0, QString::fromStdString(name));
		ui->patches->addTopLevelItem(title_level_item);
	}
	ensure(title_level_item);
	for (auto s : root.select_nodes("tag/package"))
	{
		// Add a checkable leaf item for this patch
		const QString q_description       = QString::fromStdString(s.node().attribute("version").value());
		QString visible_description       = q_description;
		QTreeWidgetItem* patch_level_item = new QTreeWidgetItem();
		patch_level_item->setText(0, visible_description);
		patch_level_item->setCheckState(0, false ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
		patch_level_item->setData(0, size_role, s.node().attribute("size").value());
		patch_level_item->setData(0, hash_role, s.node().attribute("sha1sum").value());
		patch_level_item->setData(0, sysver_role, s.node().attribute("ps3_system_ver").value());
		patch_level_item->setData(0, patch_ver_role, s.node().attribute("version").value());
		patch_level_item->setData(0, url_role, s.node().attribute("url").value());
		patch_level_item->setData(0, title_role, QString::fromStdString(name));
		patch_level_item->setData(0, app_version_role, QString::fromStdString(app_ver));
		patch_level_item->setData(0, serial_role, root.attribute("titleid").value());
		patch_level_item->setData(0, node_level_role, node_level::patch_level);
		patch_level_item->setData(0, persistance_role, true);
		title_level_item->addChild(patch_level_item);
	}
	game_update_manger_log.warning("Parsed downloaded XML");
	return true;
}

void game_update_manager_dialog::update_patch_info(const game_update_manager_dialog::gui_patch_info& info)
{
	ui->label_Size->setText(info.size);
	ui->label_hash->setText(info.hash);
	ui->label_RSysVer->setText(info.rsysv);
	ui->label_patch_version->setText(info.pv);
	ui->Label_GameID->setText(info.serial);
	ui->Label_Game_Title->setText(info.Title);
	ui->label_app_version->setText(info.currentver);
}

game_update_manager_dialog::~game_update_manager_dialog()
{
	delete ui;
}

int game_update_manager_dialog::exec()
{
	show();
	return QDialog::exec();
}