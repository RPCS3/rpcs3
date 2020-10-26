#include "main_window.h"
#include "qt_utils.h"
#include "vfs_dialog.h"
#include "save_manager_dialog.h"
#include "trophy_manager_dialog.h"
#include "user_manager_dialog.h"
#include "screenshot_manager_dialog.h"
#include "kernel_explorer.h"
#include "game_list_frame.h"
#include "debugger_frame.h"
#include "log_frame.h"
#include "settings_dialog.h"
#include "rpcn_settings_dialog.h"
#include "auto_pause_settings_dialog.h"
#include "cg_disasm_window.h"
#include "log_viewer.h"
#include "memory_string_searcher.h"
#include "memory_viewer_panel.h"
#include "rsx_debugger.h"
#include "about_dialog.h"
#include "pad_settings_dialog.h"
#include "progress_dialog.h"
#include "skylander_dialog.h"
#include "cheat_manager.h"
#include "patch_manager_dialog.h"
#include "pkg_install_dialog.h"
#include "category.h"
#include "gui_settings.h"
#include "input_dialog.h"

#include <thread>
#include <charconv>

#include <QScreen>
#include <QDirIterator>
#include <QMimeData>
#include <QMessageBox>
#include <QFileDialog>
#include <QFontDatabase>

#include "rpcs3_version.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/system_config.h"

#include "Crypto/unpkg.h"
#include "Crypto/unself.h"
#include "Crypto/unedat.h"

#include "Loader/PUP.h"
#include "Loader/TAR.h"

#include "Utilities/Thread.h"

#include "ui_main_window.h"

LOG_CHANNEL(gui_log, "GUI");

extern atomic_t<bool> g_user_asked_for_frame_capture;

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

main_window::main_window(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, std::shared_ptr<persistent_settings> persistent_settings, QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::main_window)
	, m_gui_settings(gui_settings)
	, m_emu_settings(emu_settings)
	, m_persistent_settings(persistent_settings)
{
	Q_INIT_RESOURCE(resources);

	// We have to setup the ui before using a translation
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
}

main_window::~main_window()
{
	SaveWindowState();
	delete ui;
}

/* An init method is used so that RPCS3App can create the necessary connects before calling init (specifically the stylesheet connect).
 * Simplifies logic a bit.
 */
bool main_window::Init()
{
	setAcceptDrops(true);

	// add toolbar widgets (crappy Qt designer is not able to)
	ui->toolBar->setObjectName("mw_toolbar");
	ui->sizeSlider->setRange(0, gui::gl_max_slider_pos);
	ui->toolBar->addWidget(ui->sizeSliderContainer);
	ui->toolBar->addWidget(ui->mw_searchbar);

	CreateActions();
	CreateDockWindows();
	CreateConnects();

	setMinimumSize(350, minimumSizeHint().height());    // seems fine on win 10
	setWindowTitle(QString::fromStdString("RPCS3 " + rpcs3::get_version().to_string()));

	Q_EMIT RequestGlobalStylesheetChange();
	ConfigureGuiFromSettings(true);

	if (const std::string_view branch_name = rpcs3::get_full_branch(); branch_name != "RPCS3/rpcs3/master" && branch_name != "local_build")
	{
		gui_log.warning("Experimental Build Warning! Build origin: %s", branch_name);

		QMessageBox msg;
		msg.setWindowTitle(tr("Experimental Build Warning"));
		msg.setIcon(QMessageBox::Critical);
		msg.setTextFormat(Qt::RichText);
		msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msg.setDefaultButton(QMessageBox::No);
		msg.setText(QString(tr(
			R"(
				<p style="white-space: nowrap;">
					Please understand that this build is not an official RPCS3 release.<br>
					This build contains changes that may break games, or even <b>damage</b> your data.<br>
					We recommend to download and use the official build from the <a href='https://rpcs3.net/download'>RPCS3 website</a>.<br><br>
					Build origin: %1<br>
					Do you wish to use this build anyway?
				</p>
			)"
		)).arg(Qt::convertFromPlainText(branch_name.data())));
		msg.layout()->setSizeConstraint(QLayout::SetFixedSize);

		if (msg.exec() == QMessageBox::No)
		{
			return false;
		}
	}

	show(); // needs to be done before creating the thumbnail toolbar

	// enable play options if a recent game exists
	const bool enable_play_last = !m_recent_game_acts.isEmpty() && m_recent_game_acts.first();

	const QString start_tooltip = enable_play_last ? tr("Play %0").arg(m_recent_game_acts.first()->text()) : tr("Play");

	if (enable_play_last)
	{
		ui->sysPauseAct->setText(tr("&Play last played game\tCtrl+E"));
		ui->sysPauseAct->setIcon(m_icon_play);
		ui->toolbar_start->setToolTip(start_tooltip);
	}

	ui->sysPauseAct->setEnabled(enable_play_last);
	ui->toolbar_start->setEnabled(enable_play_last);

	// create tool buttons for the taskbar thumbnail
#ifdef _WIN32
	m_thumb_bar = new QWinThumbnailToolBar(this);
	m_thumb_bar->setWindow(windowHandle());

	m_thumb_playPause = new QWinThumbnailToolButton(m_thumb_bar);
	m_thumb_playPause->setToolTip(start_tooltip);
	m_thumb_playPause->setIcon(m_icon_thumb_play);
	m_thumb_playPause->setEnabled(enable_play_last);

	m_thumb_stop = new QWinThumbnailToolButton(m_thumb_bar);
	m_thumb_stop->setToolTip(tr("Stop"));
	m_thumb_stop->setIcon(m_icon_thumb_stop);
	m_thumb_stop->setEnabled(false);

	m_thumb_restart = new QWinThumbnailToolButton(m_thumb_bar);
	m_thumb_restart->setToolTip(tr("Restart"));
	m_thumb_restart->setIcon(m_icon_thumb_restart);
	m_thumb_restart->setEnabled(false);

	m_thumb_bar->addButton(m_thumb_playPause);
	m_thumb_bar->addButton(m_thumb_stop);
	m_thumb_bar->addButton(m_thumb_restart);

	RepaintThumbnailIcons();

	connect(m_thumb_stop, &QWinThumbnailToolButton::clicked, [this]() { Emu.Stop(); });
	connect(m_thumb_restart, &QWinThumbnailToolButton::clicked, [this]() { Emu.Restart(); });
	connect(m_thumb_playPause, &QWinThumbnailToolButton::clicked, this, &main_window::OnPlayOrPause);
#endif

	// Fix possible hidden game list columns. The game list has to be visible already. Use this after show()
	m_game_list_frame->FixNarrowColumns();

	// RPCS3 Updater

	QMenu* download_menu = new QMenu(tr("Update Available!"));

	QAction *download_action = new QAction(tr("Download Update"), download_menu);
	connect(download_action, &QAction::triggered, this, [this]
	{
		m_updater.update();
	});

	download_menu->addAction(download_action);

#ifdef _WIN32
	// Use a menu at the top right corner to indicate the new version.
	QMenuBar *corner_bar = new QMenuBar(ui->menuBar);
	m_download_menu_action = corner_bar->addMenu(download_menu);
	ui->menuBar->setCornerWidget(corner_bar);
	ui->menuBar->cornerWidget()->setVisible(false);
#else
	// Append a menu to the right of the regular menus to indicate the new version.
	// Some distros just can't handle corner widgets at the moment.
	m_download_menu_action = ui->menuBar->addMenu(download_menu);
#endif

	ensure(m_download_menu_action);
	m_download_menu_action->setVisible(false);

	connect(&m_updater, &update_manager::signal_update_available, this, [this](bool update_available)
	{
		if (m_download_menu_action)
		{
			m_download_menu_action->setVisible(update_available);
		}
		if (ui->menuBar && ui->menuBar->cornerWidget())
		{
			ui->menuBar->cornerWidget()->setVisible(update_available);
		}
	});

#if defined(_WIN32) || defined(__linux__)
	if (const auto update_value = m_gui_settings->GetValue(gui::m_check_upd_start).toString(); update_value != "false")
	{
		m_updater.check_for_updates(true, update_value != "true", this);
	}
#endif

	return true;
}

QString main_window::GetCurrentTitle()
{
	QString title = qstr(Emu.GetTitleAndTitleID());
	if (title.isEmpty())
	{
		title = qstr(Emu.GetBoot());
	}
	return title;
}

// returns appIcon
QIcon main_window::GetAppIcon()
{
	return m_app_icon;
}

void main_window::ResizeIcons(int index)
{
	if (ui->sizeSlider->value() != index)
	{
		ui->sizeSlider->setSliderPosition(index);
		return; // ResizeIcons will be triggered again by setSliderPosition, so return here
	}

	if (m_save_slider_pos)
	{
		m_save_slider_pos = false;
		m_gui_settings->SetValue(m_is_list_mode ? gui::gl_iconSize : gui::gl_iconSizeGrid, index);

		// this will also fire when we used the actions, but i didn't want to add another boolean member
		SetIconSizeActions(index);
	}

	m_game_list_frame->ResizeIcons(index);
}

void main_window::OnPlayOrPause()
{
	if (Emu.IsReady())
	{
		Emu.Run(true);
	}
	else if (Emu.IsPaused())
	{
		Emu.Resume();
	}
	else if (Emu.IsRunning())
	{
		Emu.Pause();
	}
	else if (Emu.IsStopped())
	{
		if (m_selected_game)
		{
			Boot(m_selected_game->info.path, m_selected_game->info.serial, false, false, false);
		}
		else if (const auto path = Emu.GetBoot(); !path.empty())
		{
			if (const auto error = Emu.Load(); error != game_boot_result::no_errors)
			{
				gui_log.error("Boot failed: reason: %s, path: %s", error, path);
				show_boot_error(error);
			}
		}
		else if (!m_recent_game_acts.isEmpty())
		{
			BootRecentAction(m_recent_game_acts.first());
		}
	}
}

void main_window::show_boot_error(game_boot_result status)
{
	QString message;
	switch (status)
	{
	case game_boot_result::nothing_to_boot:
		message = tr("No bootable content was found.");
		break;
	case game_boot_result::wrong_disc_location:
		message = tr("Disc could not be mounted properly. Make sure the disc is not in the dev_hdd0/game folder.");
		break;
	case game_boot_result::invalid_file_or_folder:
		message = tr("The selected file or folder is invalid or corrupted.");
		break;
	case game_boot_result::install_failed:
		message = tr("Additional content could not be installed.");
		break;
	case game_boot_result::decryption_error:
		message = tr("Digital content could not be decrypted. This is usually caused by a missing or invalid license (RAP) file.");
		break;
	case game_boot_result::file_creation_error:
		message = tr("The emulator could not create files required for booting.");
		break;
	case game_boot_result::firmware_missing: // Handled elsewhere
	case game_boot_result::no_errors:
		return;
	case game_boot_result::generic_error:
	default:
		message = tr("Unknown error.");
		break;
	}
	const QString link = tr("<br /><br />For information on setting up the emulator and dumping your PS3 games, read the <a href=\"https://rpcs3.net/quickstart\">quickstart guide</a>.");

	QMessageBox msg;
	msg.setWindowTitle(tr("Boot Failed"));
	msg.setIcon(QMessageBox::Critical);
	msg.setTextFormat(Qt::RichText);
	msg.setStandardButtons(QMessageBox::Ok);
	msg.setText(tr("Booting failed: %1 %2").arg(message).arg(link));
	msg.exec();
}

void main_window::Boot(const std::string& path, const std::string& title_id, bool direct, bool add_only, bool force_global_config)
{
	if (!m_gui_settings->GetBootConfirmation(this, gui::ib_confirm_boot))
	{
		return;
	}

	m_app_icon = gui::utils::get_app_icon_from_path(path, title_id);

	Emu.SetForceBoot(true);
	Emu.Stop();

	if (const auto error = Emu.BootGame(path, title_id, direct, add_only, force_global_config); error != game_boot_result::no_errors)
	{
		gui_log.error("Boot failed: reason: %s, path: %s", error, path);
		show_boot_error(error);
	}
	else
	{
		gui_log.success("Boot successful.");
		if (!add_only)
		{
			AddRecentAction(gui::Recent_Game(qstr(Emu.GetBoot()), qstr(Emu.GetTitleAndTitleID())));
		}
	}

	m_game_list_frame->Refresh(true);
}

void main_window::BootElf()
{
	bool stopped = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	const QString path_last_elf = m_gui_settings->GetValue(gui::fd_boot_elf).toString();
	const QString file_path = QFileDialog::getOpenFileName(this, tr("Select (S)ELF To Boot"), path_last_elf, tr(
		"(S)ELF files (*BOOT.BIN *.elf *.self);;"
		"ELF files (BOOT.BIN *.elf);;"
		"SELF files (EBOOT.BIN *.self);;"
		"BOOT files (*BOOT.BIN);;"
		"BIN files (*.bin);;"
		"All files (*.*)"),
		Q_NULLPTR, QFileDialog::DontResolveSymlinks);

	if (file_path.isEmpty())
	{
		if (stopped)
		{
			Emu.Resume();
		}
		return;
	}

	// If we resolved the filepath earlier we would end up setting the last opened dir to the unwanted
	// game folder in case of having e.g. a Game Folder with collected links to elf files.
	// Don't set last path earlier in case of cancelled dialog
	m_gui_settings->SetValue(gui::fd_boot_elf, file_path);
	const std::string path = sstr(QFileInfo(file_path).absoluteFilePath());

	gui_log.notice("Booting from BootElf...");
	Boot(path, "", true);
}

void main_window::BootGame()
{
	bool stopped = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	const QString path_last_game = m_gui_settings->GetValue(gui::fd_boot_game).toString();
	const QString dir_path = QFileDialog::getExistingDirectory(this, tr("Select Game Folder"), path_last_game, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (dir_path.isEmpty())
	{
		if (stopped)
		{
			Emu.Resume();
		}
		return;
	}

	m_gui_settings->SetValue(gui::fd_boot_game, QFileInfo(dir_path).path());

	gui_log.notice("Booting from BootGame...");
	Boot(sstr(dir_path));
}

void main_window::BootRsxCapture(std::string path)
{
	if (path.empty())
	{
		bool is_stopped = false;

		if (Emu.IsRunning())
		{
			Emu.Pause();
			is_stopped = true;
		}

		const QString file_path = QFileDialog::getOpenFileName(this, tr("Select RSX Capture"), qstr(fs::get_config_dir() + "captures/"), tr("RRC files (*.rrc);;All files (*.*)"));

		if (file_path.isEmpty())
		{
			if (is_stopped)
			{
				Emu.Resume();
			}
			return;
		}
		path = sstr(file_path);
	}

	if (!m_gui_settings->GetBootConfirmation(this))
	{
		return;
	}

	Emu.SetForceBoot(true);
	Emu.Stop();

	if (!Emu.BootRsxCapture(path))
	{
		gui_log.error("Capture Boot Failed. path: %s", path);
	}
	else
	{
		gui_log.success("Capture Boot Success. path: %s", path);
	}
}

bool main_window::InstallRapFile(const QString& path, const std::string& filename)
{
	if (path.isEmpty() || filename.empty())
	{
		return false;
	}
	return fs::copy_file(sstr(path), Emulator::GetHddDir() + "/home/" + Emu.GetUsr() + "/exdata/" + filename, true);
}

void main_window::InstallPackages(QStringList file_paths)
{
	if (file_paths.isEmpty())
	{
		// If this function was called without a path, ask the user for files to install.
		const QString path_last_pkg = m_gui_settings->GetValue(gui::fd_install_pkg).toString();
		const QStringList paths = QFileDialog::getOpenFileNames(this, tr("Select packages and/or rap files to install"),
			path_last_pkg, tr("All relevant (*.pkg *.rap);;Package files (*.pkg);;Rap files (*.rap);;All files (*.*)"));

		if (paths.isEmpty())
		{
			return;
		}

		file_paths.append(paths);
		const QFileInfo file_info(file_paths[0]);
		m_gui_settings->SetValue(gui::fd_install_pkg, file_info.path());
	}
	else if (file_paths.count() == 1)
	{
		// This can currently only happen by drag and drop.
		const QString file_path = file_paths.front();
		const QFileInfo file_info(file_path);

		compat::package_info info = game_compatibility::GetPkgInfo(file_path, m_game_list_frame ? m_game_list_frame->GetGameCompatibility() : nullptr);

		if (info.type != compat::package_type::other)
		{
			if (info.type == compat::package_type::dlc)
			{
				info.local_cat = tr("\nDLC", "Block for package type (DLC)");
			}
			else
			{
				info.local_cat = tr("\nUpdate", "Block for package type (Update)");
			}
		}
		else if (!info.local_cat.isEmpty())
		{
			info.local_cat = tr("\n%0", "Block for package type").arg(info.local_cat);
		}

		if (!info.title_id.isEmpty())
		{
			info.title_id = tr("\n%0", "Block for Title ID").arg(info.title_id);
		}

		if (!info.version.isEmpty())
		{
			info.version = tr("\nVersion %0", "Block for Version").arg(info.version);
		}

		if (!info.changelog.isEmpty())
		{
			info.changelog = tr("\n\nChangelog:\n%0", "Block for Changelog").arg(info.changelog);
		}

		if (QMessageBox::question(this, tr("PKG Decrypter / Installer"), tr("Do you want to install this package?\n\n%0\n\n%1%2%3%4%5")
			.arg(file_info.fileName()).arg(info.title).arg(info.local_cat).arg(info.title_id).arg(info.version).arg(info.changelog),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		{
			gui_log.notice("PKG: Cancelled installation from drop. File: %s", sstr(file_paths.front()));
			return;
		}
	}

	// Install rap files if available
	for (const auto& rap : file_paths.filter(QRegExp(".*\\.rap")))
	{
		const QFileInfo file_info(rap);
		const std::string rapname = sstr(file_info.fileName());

		if (InstallRapFile(rap, rapname))
		{
			gui_log.success("Successfully copied rap file: %s", rapname);
		}
		else
		{
			gui_log.error("Could not copy rap file: %s", rapname);
		}
	}

	// Find remaining package files
	file_paths = file_paths.filter(QRegExp(".*\\.pkg", Qt::CaseInsensitive));

	if (!file_paths.isEmpty())
	{
		// Handle further installations with a timeout. Otherwise the source explorer instance is not usable during the following file processing.
		QTimer::singleShot(0, [this, paths = std::move(file_paths)]()
		{
			HandlePackageInstallation(paths);
		});
	}
}

void main_window::HandlePackageInstallation(QStringList file_paths)
{
	std::vector<compat::package_info> packages;

	game_compatibility* compat = m_game_list_frame ? m_game_list_frame->GetGameCompatibility() : nullptr;

	if (file_paths.size() > 1)
	{
		// Let the user choose the packages to install and select the order in which they shall be installed.
		pkg_install_dialog dlg(file_paths, compat, this);
		connect(&dlg, &QDialog::accepted, [&packages, &dlg]()
		{
			packages = dlg.GetPathsToInstall();
		});
		dlg.exec();
	}
	else
	{
		packages.push_back(game_compatibility::GetPkgInfo(file_paths.front(), compat));
	}

	if (packages.empty())
	{
		return;
	}

	if (!m_gui_settings->GetBootConfirmation(this))
	{
		return;
	}

	progress_dialog pdlg(tr("RPCS3 Package Installer"), tr("Installing package, please wait..."), tr("Cancel"), 0, 1000, false, this);
	pdlg.setAutoClose(false);
	pdlg.show();

	// Synchronization variable
	atomic_t<double> progress(0.);

	package_error error = package_error::no_error;

	bool cancelled = false;

	for (size_t i = 0, count = packages.size(); i < count; i++)
	{
		progress = 0.;

		const compat::package_info& package = packages.at(i);
		QString app_info = package.title; // This should always be non-empty

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

		pdlg.SetValue(0);
		pdlg.setLabelText(tr("Installing package (%0/%1), please wait...\n\n%2").arg(i + 1).arg(count).arg(app_info));
		pdlg.show();

		Emu.SetForceBoot(true);
		Emu.Stop();

		const QFileInfo file_info(package.path);
		const std::string path      = sstr(package.path);
		const std::string file_name = sstr(file_info.fileName());

		// Run PKG unpacking asynchronously
		named_thread worker("PKG Installer", [path, &progress, &error]
		{
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
			if (pdlg.wasCanceled())
			{
				cancelled = true;
				progress -= 1.;
				break;
			}

			// Update progress window
			double pval = progress;
			if (pval < 0.) pval += 1.;
			pdlg.SetValue(static_cast<int>(pval * pdlg.maximum()));
			QCoreApplication::processEvents();
		}

		if (worker())
		{
			pdlg.SetValue(pdlg.maximum());
			std::this_thread::sleep_for(100ms);
		}
		else
		{
			pdlg.setHidden(true);
			pdlg.SignalFailure();
		}

		if (worker())
		{
			m_game_list_frame->Refresh(true);
			gui_log.success("Successfully installed %s (title_id=%s, title=%s, version=%s).", file_name, sstr(package.title_id), sstr(package.title), sstr(package.version));

			if (i == (count - 1))
			{
				m_gui_settings->ShowInfoBox(tr("Success!"), tr("Successfully installed software from package(s)!"), gui::ib_pkg_success, this);
			}
		}
		else
		{
			if (!cancelled)
			{
				if (error == package_error::app_version)
				{
					gui_log.error("Cannot install %s.", file_name);
					QMessageBox::warning(this, tr("Warning!"), tr("The following package cannot be installed on top of the current data:\n%1!").arg(package.path));
				}
				else
				{
					gui_log.error("Failed to install %s.", file_name);
					QMessageBox::critical(this, tr("Failure!"), tr("Failed to install software from package:\n%1!").arg(package.path));
				}
			}
			return;
		}

		// return if the thread was still running after cancel
		if (cancelled)
		{
			return;
		}
	}
}

void main_window::InstallPup(QString file_path)
{
	if (file_path.isEmpty())
	{
		const QString path_last_pup = m_gui_settings->GetValue(gui::fd_install_pup).toString();
		file_path = QFileDialog::getOpenFileName(this, tr("Select PS3UPDAT.PUP To Install"), path_last_pup, tr("PS3 update file (PS3UPDAT.PUP);;All pup files (*.pup);;All files (*.*)"));
	}
	else
	{
		if (QMessageBox::question(this, tr("RPCS3 Firmware Installer"), tr("Install firmware: %1?").arg(file_path),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		{
			gui_log.notice("Firmware: Cancelled installation from drop. File: %s", sstr(file_path));
			return;
		}
	}

	if (!file_path.isEmpty())
	{
		// Handle the actual installation with a timeout. Otherwise the source explorer instance is not usable during the following file processing.
		QTimer::singleShot(0, [this, file_path]()
		{
			HandlePupInstallation(file_path);
		});
	}
}

void main_window::HandlePupInstallation(QString file_path)
{
	if (file_path.isEmpty())
	{
		return;
	}

	if (!m_gui_settings->GetBootConfirmation(this))
	{
		return;
	}

	Emu.SetForceBoot(true);
	Emu.Stop();

	m_gui_settings->SetValue(gui::fd_install_pup, QFileInfo(file_path).path());
	const std::string path = sstr(file_path);

	fs::file pup_f(path);
	if (!pup_f)
	{
		gui_log.error("Error opening PUP file %s", path);
		QMessageBox::critical(this, tr("Firmware Installation Failed"), tr("Firmware installation failed: The selected firmware file couldn't be opened."));
		return;
	}

	if (pup_f.size() < sizeof(PUPHeader))
	{
		gui_log.error("Too small PUP file: %llu", pup_f.size());
		QMessageBox::critical(this, tr("Firmware Installation Failed"), tr("Firmware installation failed: The provided file is empty."));
		return;
	}

	struct PUPHeader header = {};
	pup_f.seek(0);
	pup_f.read(header);

	if (header.header_length + header.data_length != pup_f.size())
	{
		gui_log.error("Firmware size mismatch, expected: %llu + %llu, actual: %llu", header.header_length, header.data_length, pup_f.size());
		QMessageBox::critical(this, tr("Firmware Installation Failed"), tr("Firmware installation failed: The provided file is incomplete. Try redownloading it."));
		return;
	}

	pup_object pup(pup_f);
	if (!pup)
	{
		gui_log.error("Error while installing firmware: PUP file is invalid.");
		QMessageBox::critical(this, tr("Firmware Installation Failed"), tr("Firmware installation failed: The provided file is corrupted."));
		return;
	}

	if (!pup.validate_hashes())
	{
		gui_log.error("Error while installing firmware: Hash check failed.");
		QMessageBox::critical(this, tr("Firmware Installation Failed"), tr("Firmware installation failed: The provided file's contents are corrupted."));
		return;
	}

	fs::file update_files_f = pup.get_file(0x300);
	tar_object update_files(update_files_f);
	auto update_filenames = update_files.get_filenames();

	update_filenames.erase(std::remove_if(
		update_filenames.begin(), update_filenames.end(), [](std::string s) { return s.find("dev_flash_") == umax; }),
		update_filenames.end());

	std::string version_string = pup.get_file(0x100).to_string();

	if (const usz version_pos = version_string.find('\n'); version_pos != umax)
	{
		version_string.erase(version_pos);
	}

	const std::string cur_version = "4.87";

	if (version_string < cur_version &&
		QMessageBox::question(this, tr("RPCS3 Firmware Installer"), tr("Old firmware detected.\nThe newest firmware version is %1 and you are trying to install version %2\nContinue installation?").arg(qstr(cur_version), qstr(version_string)),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
	{
		return;
	}

	// Remove possibly PS3 fonts from database
	QFontDatabase::removeAllApplicationFonts();

	progress_dialog pdlg(tr("RPCS3 Firmware Installer"), tr("Installing firmware version %1\nPlease wait...").arg(qstr(version_string)), tr("Cancel"), 0, static_cast<int>(update_filenames.size()), false, this);
	pdlg.show();

	// Synchronization variable
	atomic_t<int> progress(0);
	{
		// Run asynchronously
		named_thread worker("Firmware Installer", [&]
		{
			for (const auto& update_filename : update_filenames)
			{
				if (progress == -1) break;

				fs::file update_file = update_files.get_file(update_filename);

				SCEDecrypter self_dec(update_file);
				self_dec.LoadHeaders();
				self_dec.LoadMetadata(SCEPKG_ERK, SCEPKG_RIV);
				self_dec.DecryptData();

				auto dev_flash_tar_f = self_dec.MakeFile();
				if (dev_flash_tar_f.size() < 3)
				{
					gui_log.error("Error while installing firmware: PUP contents are invalid.");
					QMessageBox::critical(this, tr("Firmware Installation Failed"), tr("Firmware installation failed: Firmware could not be decompressed"));
					progress = -1;
				}

				tar_object dev_flash_tar(dev_flash_tar_f[2]);
				if (!dev_flash_tar.extract(g_cfg.vfs.get_dev_flash(), "dev_flash/"))
				{
					gui_log.error("Error while installing firmware: TAR contents are invalid.");
					QMessageBox::critical(this, tr("Firmware Installation Failed"), tr("Firmware installation failed: Firmware contents could not be extracted."));
					progress = -1;
				}

				if (progress >= 0)
					progress += 1;
			}
		});

		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), std::abs(progress) < pdlg.maximum())
		{
			if (pdlg.wasCanceled())
			{
				progress = -1;
				break;
			}
			// Update progress window
			pdlg.SetValue(static_cast<int>(progress));
			QCoreApplication::processEvents();
		}

		update_files_f.close();
		pup_f.close();

		if (progress > 0)
		{
			pdlg.SetValue(pdlg.maximum());
			std::this_thread::sleep_for(100ms);
		}
	}

	// Update with newly installed PS3 fonts
	Q_EMIT RequestGlobalStylesheetChange();

	if (progress > 0)
	{
		gui_log.success("Successfully installed PS3 firmware version %s.", version_string);
		m_gui_settings->ShowInfoBox(tr("Success!"), tr("Successfully installed PS3 firmware and LLE Modules!"), gui::ib_pup_success, this);

		CreateFirmwareCache();
	}
}

// This is ugly, but PS3 headers shall not be included there.
extern void sysutil_send_system_cmd(u64 status, u64 param);

void main_window::DecryptSPRXLibraries()
{
	const QString path_last_sprx = m_gui_settings->GetValue(gui::fd_decrypt_sprx).toString();
	const QStringList modules = QFileDialog::getOpenFileNames(this, tr("Select binary files"), path_last_sprx, tr("All Binaries (*.BIN *.self *.sprx);;BIN files (*.BIN);;SELF files (*.self);;SPRX files (*.sprx);;All files (*.*)"));

	if (modules.isEmpty())
	{
		return;
	}

	m_gui_settings->SetValue(gui::fd_decrypt_sprx, QFileInfo(modules.first()).path());

	gui_log.notice("Decrypting binaries...");

	// Always start with no KLIC
	std::vector<u128> klics{u128{}};

	if (const auto keys = g_fxo->get<loaded_npdrm_keys>())
	{
		// Second klic: get it from a running game
		if (const u128 klic = keys->devKlic)
		{
			klics.emplace_back(klic);
		}
	}

	for (const QString& _module : modules)
	{
		const std::string old_path = sstr(_module);

		fs::file elf_file;

		bool tried = false;
		bool invalid = false;
		usz key_it = 0;

		while (true)
		{
			for (; key_it < klics.size(); key_it++)
			{
				if (elf_file.open(old_path) && elf_file.size() >= 4 && elf_file.read<u32>() == "SCE\0"_u32)
				{
					// First KLIC is no KLIC
					elf_file = decrypt_self(std::move(elf_file), key_it != 0 ? reinterpret_cast<u8*>(&klics[key_it]) : nullptr);

					if (!elf_file)
					{
						// Try another key
						continue;
					}
				}
				else
				{
					elf_file = {};
					invalid = true;
				}

				break;
			}

			if (elf_file)
			{
				const std::string bin_ext  = _module.toLower().endsWith(".sprx") ? ".prx" : ".elf";
				const std::string new_path = old_path.substr(0, old_path.find_last_of('.')) + bin_ext;

				if (fs::file new_file{new_path, fs::rewrite})
				{
					new_file.write(elf_file.to_string());
					gui_log.success("Decrypted %s", old_path);
				}
				else
				{
					gui_log.error("Failed to create %s", new_path);
				}

				break;
			}
			else if (!invalid)
			{
				// Allow the user to manually type KLIC if decryption failed

				const std::string filename = old_path.substr(old_path.find_last_of(fs::delim) + 1);

				const QString hint = tr("Hint: KLIC (KLicense key) is a 16-byte long string. (32 hexadecimal characters)"
							"\nAnd is logged with some sceNpDrm* functions when the game/application which owns \"%0\" is running.").arg(qstr(filename));

				if (tried)
				{
					gui_log.error("Failed to decrypt %s with specfied KLIC, retrying.\n%s", old_path, sstr(hint));
				}

				input_dialog dlg(32, "", tr("Enter KLIC of %0").arg(qstr(filename)),
					tried ? tr("Decryption failed with provided KLIC.\n%0").arg(hint) : tr("Hexadecimal only."), "00000000000000000000000000000000", this);

				QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
				mono.setPointSize(8);
				dlg.set_input_font(mono, true, '0');
				dlg.set_clear_button_enabled(false);
				dlg.set_button_enabled(QDialogButtonBox::StandardButton::Ok, false);
				dlg.set_validator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]*$"))); // HEX only

				connect(&dlg, &input_dialog::text_changed, [&](const QString& text)
				{
					dlg.set_button_enabled(QDialogButtonBox::StandardButton::Ok, text.size() == 32);
				});

				if (dlg.exec() == QDialog::Accepted)
				{
					const std::string text = sstr(dlg.get_input_text());

					auto& klic = (tried ? klics.back() : klics.emplace_back());

					ensure(text.size() == 32);

					// It must succeed (only hex characters are present)
					u64 lo_ = 0;
					u64 hi_ = 0;
					std::from_chars(&text[0], &text[16], lo_, 16);
					std::from_chars(&text[16], &text[32], hi_, 16);

					be_t<u64> lo = std::bit_cast<be_t<u64>>(lo_);
					be_t<u64> hi = std::bit_cast<be_t<u64>>(hi_);

					klic = (u128{+hi} << 64) | +lo;

					// Retry with specified KLIC
					key_it -= +std::exchange(tried, true); // Rewind on second and above attempt
					gui_log.notice("KLIC entered for %s: %s", filename, klic);
					continue;
				}
				else
				{
					gui_log.notice("User has cancelled entering KLIC.");
				}
			}

			gui_log.error("Failed to decrypt \"%s\".", old_path);
			break;
		}
	}

	gui_log.notice("Finished decrypting all binaries.");
}

/** Needed so that when a backup occurs of window state in gui_settings, the state is current.
* Also, so that on close, the window state is preserved.
*/
void main_window::SaveWindowState()
{
	// Save gui settings
	m_gui_settings->SetValue(gui::mw_geometry, saveGeometry());
	m_gui_settings->SetValue(gui::mw_windowState, saveState());
	m_gui_settings->SetValue(gui::mw_mwState, m_mw->saveState());

	// Save column settings
	m_game_list_frame->SaveSettings();
	// Save splitter state
	m_debugger_frame->SaveSettings();
}

void main_window::RepaintThumbnailIcons()
{
	[[maybe_unused]] const QColor new_color = gui::utils::get_label_color("thumbnail_icon_color");

	[[maybe_unused]] const auto icon = [&new_color](const QString& path)
	{
		return gui::utils::get_colorized_icon(QPixmap::fromImage(gui::utils::get_opaque_image_area(path)), Qt::black, new_color);
	};

#ifdef _WIN32
	if (!m_thumb_bar) return;

	m_icon_thumb_play = icon(":/Icons/play.png");
	m_icon_thumb_pause = icon(":/Icons/pause.png");
	m_icon_thumb_stop = icon(":/Icons/stop.png");
	m_icon_thumb_restart = icon(":/Icons/restart.png");

	m_thumb_playPause->setIcon(Emu.IsRunning() ? m_icon_thumb_pause : m_icon_thumb_play);
	m_thumb_stop->setIcon(m_icon_thumb_stop);
	m_thumb_restart->setIcon(m_icon_thumb_restart);
#endif
}

void main_window::RepaintToolBarIcons()
{
	const QColor new_color = gui::utils::get_label_color("toolbar_icon_color");

	const auto icon = [&new_color](const QString& path)
	{
		return gui::utils::get_colorized_icon(QIcon(path), Qt::black, new_color);
	};

	m_icon_play           = icon(":/Icons/play.png");
	m_icon_pause          = icon(":/Icons/pause.png");
	m_icon_stop           = icon(":/Icons/stop.png");
	m_icon_restart        = icon(":/Icons/restart.png");
	m_icon_fullscreen_on  = icon(":/Icons/fullscreen.png");
	m_icon_fullscreen_off = icon(":/Icons/exit_fullscreen.png");

	ui->toolbar_config  ->setIcon(icon(":/Icons/configure.png"));
	ui->toolbar_controls->setIcon(icon(":/Icons/controllers.png"));
	ui->toolbar_open    ->setIcon(icon(":/Icons/open.png"));
	ui->toolbar_grid    ->setIcon(icon(":/Icons/grid.png"));
	ui->toolbar_list    ->setIcon(icon(":/Icons/list.png"));
	ui->toolbar_refresh ->setIcon(icon(":/Icons/refresh.png"));
	ui->toolbar_stop    ->setIcon(icon(":/Icons/stop.png"));

	if (Emu.IsRunning())
	{
		ui->toolbar_start->setIcon(m_icon_pause);
	}
	else if (Emu.IsStopped() && !Emu.GetBoot().empty())
	{
		ui->toolbar_start->setIcon(m_icon_restart);
	}
	else
	{
		ui->toolbar_start->setIcon(m_icon_play);
	}

	if (isFullScreen())
	{
		ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_off);
	}
	else
	{
		ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_on);
	}

	ui->sizeSlider->setStyleSheet(ui->sizeSlider->styleSheet().append("QSlider::handle:horizontal{ background: rgba(%1, %2, %3, %4); }")
		.arg(new_color.red()).arg(new_color.green()).arg(new_color.blue()).arg(new_color.alpha()));

	// resize toolbar elements

	// for highdpi resize toolbar icons and height dynamically
	// choose factors to mimic Gui-Design in main_window.ui
	// TODO: delete this in case Qt::AA_EnableHighDpiScaling is enabled in main.cpp
#ifdef _WIN32
	const int tool_icon_height = menuBar()->sizeHint().height() * 1.5;
	ui->toolBar->setIconSize(QSize(tool_icon_height, tool_icon_height));
#endif

	const int tool_bar_height = ui->toolBar->sizeHint().height();

	for (const auto& act : ui->toolBar->actions())
	{
		if (act->isSeparator())
		{
			continue;
		}

		ui->toolBar->widgetForAction(act)->setMinimumWidth(tool_bar_height);
	}

	ui->sizeSliderContainer->setFixedWidth(tool_bar_height * 4);
	ui->mw_searchbar->setFixedWidth(tool_bar_height * 5);
}

void main_window::OnEmuRun(bool /*start_playtime*/)
{
	const QString title = GetCurrentTitle();
	const QString restart_tooltip = tr("Restart %0").arg(title);
	const QString pause_tooltip = tr("Pause %0").arg(title);
	const QString stop_tooltip = tr("Stop %0").arg(title);

	m_debugger_frame->EnableButtons(true);

#ifdef _WIN32
	m_thumb_stop->setToolTip(stop_tooltip);
	m_thumb_restart->setToolTip(restart_tooltip);
	m_thumb_playPause->setToolTip(pause_tooltip);
	m_thumb_playPause->setIcon(m_icon_thumb_pause);
#endif
	ui->sysPauseAct->setText(tr("&Pause\tCtrl+P"));
	ui->sysPauseAct->setIcon(m_icon_pause);
	ui->toolbar_start->setIcon(m_icon_pause);
	ui->toolbar_start->setText(tr("Pause"));
	ui->toolbar_start->setToolTip(pause_tooltip);
	ui->toolbar_stop->setToolTip(stop_tooltip);

	EnableMenus(true);
}

void main_window::OnEmuResume()
{
	const QString title = GetCurrentTitle();
	const QString restart_tooltip = tr("Restart %0").arg(title);
	const QString pause_tooltip = tr("Pause %0").arg(title);
	const QString stop_tooltip = tr("Stop %0").arg(title);

#ifdef _WIN32
	m_thumb_stop->setToolTip(stop_tooltip);
	m_thumb_restart->setToolTip(restart_tooltip);
	m_thumb_playPause->setToolTip(pause_tooltip);
	m_thumb_playPause->setIcon(m_icon_thumb_pause);
#endif
	ui->sysPauseAct->setText(tr("&Pause\tCtrl+P"));
	ui->sysPauseAct->setIcon(m_icon_pause);
	ui->toolbar_start->setIcon(m_icon_pause);
	ui->toolbar_start->setText(tr("Pause"));
	ui->toolbar_start->setToolTip(pause_tooltip);
	ui->toolbar_stop->setToolTip(stop_tooltip);
}

void main_window::OnEmuPause()
{
	const QString title = GetCurrentTitle();
	const QString resume_tooltip = tr("Resume %0").arg(title);

#ifdef _WIN32
	m_thumb_playPause->setToolTip(resume_tooltip);
	m_thumb_playPause->setIcon(m_icon_thumb_play);
#endif
	ui->sysPauseAct->setText(tr("&Resume\tCtrl+E"));
	ui->sysPauseAct->setIcon(m_icon_play);
	ui->toolbar_start->setIcon(m_icon_play);
	ui->toolbar_start->setText(tr("Play"));
	ui->toolbar_start->setToolTip(resume_tooltip);

	// Refresh game list in order to update time played
	if (m_game_list_frame)
	{
		m_game_list_frame->Refresh();
	}
}

void main_window::OnEmuStop()
{
	const QString title = GetCurrentTitle();
	const QString play_tooltip = Emu.IsReady() ? tr("Play %0").arg(title) : tr("Resume %0").arg(title);

	m_debugger_frame->UpdateUI();

	ui->sysPauseAct->setText(Emu.IsReady() ? tr("&Play\tCtrl+E") : tr("&Resume\tCtrl+E"));
	ui->sysPauseAct->setIcon(m_icon_play);
#ifdef _WIN32
	m_thumb_playPause->setToolTip(play_tooltip);
	m_thumb_playPause->setIcon(m_icon_thumb_play);
#endif

	EnableMenus(false);

	if (Emu.GetBoot().empty())
	{
		ui->toolbar_start->setIcon(m_icon_play);
		ui->toolbar_start->setText(tr("Play"));
		ui->toolbar_start->setToolTip(play_tooltip);
	}
	else
	{
		const QString restart_tooltip = tr("Restart %0").arg(title);

		ui->toolbar_start->setEnabled(true);
		ui->toolbar_start->setIcon(m_icon_restart);
		ui->toolbar_start->setText(tr("Restart"));
		ui->toolbar_start->setToolTip(restart_tooltip);
		ui->sysRebootAct->setEnabled(true);
#ifdef _WIN32
		m_thumb_restart->setToolTip(restart_tooltip);
		m_thumb_restart->setEnabled(true);
#endif
	}
	ui->actionManage_Users->setEnabled(true);

	// Refresh game list in order to update time played
	if (m_game_list_frame)
	{
		m_game_list_frame->Refresh();
	}

	// Close kernel explorer if running
	if (m_kernel_explorer)
	{
		m_kernel_explorer->close();
	}
}

void main_window::OnEmuReady()
{
	const QString title = GetCurrentTitle();
	const QString play_tooltip = Emu.IsReady() ? tr("Play %0").arg(title) : tr("Resume %0").arg(title);

	m_debugger_frame->EnableButtons(true);
#ifdef _WIN32
	m_thumb_playPause->setToolTip(play_tooltip);
	m_thumb_playPause->setIcon(m_icon_thumb_play);
#endif
	ui->sysPauseAct->setText(Emu.IsReady() ? tr("&Play\tCtrl+E") : tr("&Resume\tCtrl+E"));
	ui->sysPauseAct->setIcon(m_icon_play);
	ui->toolbar_start->setIcon(m_icon_play);
	ui->toolbar_start->setText(tr("Play"));
	ui->toolbar_start->setToolTip(play_tooltip);

	EnableMenus(true);

	ui->actionManage_Users->setEnabled(false);
}

void main_window::EnableMenus(bool enabled)
{
	// Thumbnail Buttons
#ifdef _WIN32
	m_thumb_playPause->setEnabled(enabled);
	m_thumb_stop->setEnabled(enabled);
	m_thumb_restart->setEnabled(enabled);
#endif

	// Toolbar
	ui->toolbar_start->setEnabled(enabled);
	ui->toolbar_stop->setEnabled(enabled);

	// Emulation
	ui->sysPauseAct->setEnabled(enabled);
	ui->sysStopAct->setEnabled(enabled);
	ui->sysRebootAct->setEnabled(enabled);

	// PS3 Commands
	ui->sysSendOpenMenuAct->setEnabled(enabled);
	ui->sysSendExitAct->setEnabled(enabled);

	// Tools
	ui->toolskernel_explorerAct->setEnabled(enabled);
	ui->toolsmemory_viewerAct->setEnabled(enabled);
	ui->toolsRsxDebuggerAct->setEnabled(enabled);
	ui->toolsStringSearchAct->setEnabled(enabled);
	ui->actionCreate_RSX_Capture->setEnabled(enabled);
}

void main_window::BootRecentAction(const QAction* act)
{
	if (Emu.IsRunning())
	{
		return;
	}

	const QString pth = act->data().toString();
	const std::string path = sstr(pth);
	QString name;
	bool contains_path = false;

	int idx = -1;
	for (int i = 0; i < m_rg_entries.count(); i++)
	{
		if (m_rg_entries.at(i).first == pth)
		{
			idx = i;
			contains_path = true;
			name = m_rg_entries.at(idx).second;
			break;
		}
	}

	// path is invalid: remove action from list return
	if ((contains_path && name.isEmpty()) || (!QFileInfo(pth).isDir() && !QFileInfo(pth).isFile()))
	{
		if (contains_path)
		{
			// clear menu of actions
			for (auto act : m_recent_game_acts)
			{
				ui->bootRecentMenu->removeAction(act);
			}

			// remove action from list
			m_rg_entries.removeAt(idx);
			m_recent_game_acts.removeAt(idx);

			m_gui_settings->SetValue(gui::rg_entries, m_gui_settings->List2Var(m_rg_entries));

			gui_log.error("Recent Game not valid, removed from Boot Recent list: %s", path);

			// refill menu with actions
			for (int i = 0; i < m_recent_game_acts.count(); i++)
			{
				m_recent_game_acts[i]->setShortcut(tr("Ctrl+%1").arg(i + 1));
				m_recent_game_acts[i]->setToolTip(m_rg_entries.at(i).second);
				ui->bootRecentMenu->addAction(m_recent_game_acts[i]);
			}

			gui_log.warning("Boot Recent list refreshed");
			return;
		}

		gui_log.error("Path invalid and not in m_rg_paths: %s", path);
		return;
	}

	gui_log.notice("Booting from recent games list...");
	Boot(path, "", true);
}

QAction* main_window::CreateRecentAction(const q_string_pair& entry, const uint& sc_idx)
{
	// if path is not valid remove from list
	if (entry.second.isEmpty() || (!QFileInfo(entry.first).isDir() && !QFileInfo(entry.first).isFile()))
	{
		if (m_rg_entries.contains(entry))
		{
			gui_log.warning("Recent Game not valid, removing from Boot Recent list: %s", sstr(entry.first));

			const int idx = m_rg_entries.indexOf(entry);
			m_rg_entries.removeAt(idx);

			m_gui_settings->SetValue(gui::rg_entries, m_gui_settings->List2Var(m_rg_entries));
		}
		return nullptr;
	}

	// if name is a path get filename
	QString shown_name = entry.second;
	if (QFileInfo(entry.second).isFile())
	{
		shown_name = entry.second.section('/', -1);
	}

	// create new action
	QAction* act = new QAction(shown_name, this);
	act->setData(entry.first);
	act->setToolTip(entry.second);
	act->setShortcut(tr("Ctrl+%1").arg(sc_idx));

	// truncate if too long
	if (shown_name.length() > 60)
	{
		act->setText(shown_name.left(27) + "(....)" + shown_name.right(27));
	}

	// connect boot
	connect(act, &QAction::triggered, [act, this]() {BootRecentAction(act); });

	return act;
}

void main_window::AddRecentAction(const q_string_pair& entry)
{
	// don't change list on freeze
	if (ui->freezeRecentAct->isChecked())
	{
		return;
	}

	// create new action, return if not valid
	QAction* act = CreateRecentAction(entry, 1);
	if (!act)
	{
		return;
	}

	// clear menu of actions
	for (auto act : m_recent_game_acts)
	{
		ui->bootRecentMenu->removeAction(act);
	}

	// If path already exists, remove it in order to get it to beginning. Also remove duplicates.
	for (int i = m_rg_entries.count() - 1; i >= 0; --i)
	{
		if (m_rg_entries[i].first == entry.first)
		{
			m_rg_entries.removeAt(i);
			m_recent_game_acts.removeAt(i);
		}
	}

	// remove oldest action at the end if needed
	if (m_rg_entries.count() == 9)
	{
		m_rg_entries.removeLast();
		m_recent_game_acts.removeLast();
	}
	else if (m_rg_entries.count() > 9)
	{
		gui_log.error("Recent games entrylist too big");
	}

	if (m_rg_entries.count() < 9)
	{
		// add new action at the beginning
		m_rg_entries.prepend(entry);
		m_recent_game_acts.prepend(act);
	}

	// refill menu with actions
	for (int i = 0; i < m_recent_game_acts.count(); i++)
	{
		m_recent_game_acts[i]->setShortcut(tr("Ctrl+%1").arg(i+1));
		m_recent_game_acts[i]->setToolTip(m_rg_entries.at(i).second);
		ui->bootRecentMenu->addAction(m_recent_game_acts[i]);
	}

	m_gui_settings->SetValue(gui::rg_entries, m_gui_settings->List2Var(m_rg_entries));
}

void main_window::UpdateLanguageActions(const QStringList& language_codes, const QString& language_code)
{
	ui->languageMenu->clear();

	for (const auto& code : language_codes)
	{
		const QLocale locale      = QLocale(code);
		const QString locale_name = QLocale::languageToString(locale.language());

		// create new action
		QAction* act = new QAction(locale_name, this);
		act->setData(code);
		act->setToolTip(locale_name);
		act->setCheckable(true);
		act->setChecked(code == language_code);

		// connect to language changer
		connect(act, &QAction::triggered, [this, code]()
		{
			RequestLanguageChange(code);
		});

		ui->languageMenu->addAction(act);
	}
}

void main_window::RepaintGui()
{
	if (m_game_list_frame)
	{
		m_game_list_frame->RepaintIcons(true);
	}

	if (m_log_frame)
	{
		m_log_frame->RepaintTextColors();
	}

	if (m_debugger_frame)
	{
		m_debugger_frame->ChangeColors();
	}

	RepaintToolBarIcons();
	RepaintThumbnailIcons();

	Q_EMIT RequestTrophyManagerRepaint();
}

void main_window::RetranslateUI(const QStringList& language_codes, const QString& language)
{
	UpdateLanguageActions(language_codes, language);

	ui->retranslateUi(this);

	if (m_game_list_frame)
	{
		m_game_list_frame->Refresh(true);
	}
}

void main_window::ShowTitleBars(bool show)
{
	m_game_list_frame->SetTitleBarVisible(show);
	m_debugger_frame->SetTitleBarVisible(show);
	m_log_frame->SetTitleBarVisible(show);
}

void main_window::CreateActions()
{
	ui->exitAct->setShortcuts(QKeySequence::Quit);

	ui->toolbar_start->setEnabled(false);
	ui->toolbar_stop->setEnabled(false);

	m_category_visible_act_group = new QActionGroup(this);
	m_category_visible_act_group->addAction(ui->showCatHDDGameAct);
	m_category_visible_act_group->addAction(ui->showCatDiscGameAct);
	m_category_visible_act_group->addAction(ui->showCatPS1GamesAct);
	m_category_visible_act_group->addAction(ui->showCatPS2GamesAct);
	m_category_visible_act_group->addAction(ui->showCatPSPGamesAct);
	m_category_visible_act_group->addAction(ui->showCatHomeAct);
	m_category_visible_act_group->addAction(ui->showCatAudioVideoAct);
	m_category_visible_act_group->addAction(ui->showCatGameDataAct);
	m_category_visible_act_group->addAction(ui->showCatUnknownAct);
	m_category_visible_act_group->addAction(ui->showCatOtherAct);
	m_category_visible_act_group->setExclusive(false);

	m_icon_size_act_group = new QActionGroup(this);
	m_icon_size_act_group->addAction(ui->setIconSizeTinyAct);
	m_icon_size_act_group->addAction(ui->setIconSizeSmallAct);
	m_icon_size_act_group->addAction(ui->setIconSizeMediumAct);
	m_icon_size_act_group->addAction(ui->setIconSizeLargeAct);

	m_list_mode_act_group = new QActionGroup(this);
	m_list_mode_act_group->addAction(ui->setlistModeListAct);
	m_list_mode_act_group->addAction(ui->setlistModeGridAct);
}

void main_window::CreateConnects()
{
	connect(ui->bootElfAct, &QAction::triggered, this, &main_window::BootElf);
	connect(ui->bootGameAct, &QAction::triggered, this, &main_window::BootGame);
	connect(ui->actionopen_rsx_capture, &QAction::triggered, [this](){ BootRsxCapture(); });
	connect(ui->actionCreate_RSX_Capture, &QAction::triggered, []()
	{
		g_user_asked_for_frame_capture = true;
	});

	connect(ui->addGamesAct, &QAction::triggered, [this]()
	{
		QStringList paths;

		// Only select one folder for now
		paths << QFileDialog::getExistingDirectory(this, tr("Select a folder containing one or more games"), qstr(fs::get_config_dir()), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

		if (!paths.isEmpty())
		{
			for (const QString& path : paths)
			{
				AddGamesFromDir(path);
			}
			m_game_list_frame->Refresh(true);
		}
	});

	connect(ui->bootRecentMenu, &QMenu::aboutToShow, [this]()
	{
		// Enable/Disable Recent Games List
		const bool stopped = Emu.IsStopped();
		for (auto act : ui->bootRecentMenu->actions())
		{
			if (act != ui->freezeRecentAct && act != ui->clearRecentAct)
			{
				act->setEnabled(stopped);
			}
		}
	});

	connect(ui->clearRecentAct, &QAction::triggered, [this]()
	{
		if (ui->freezeRecentAct->isChecked())
		{
			return;
		}
		m_rg_entries.clear();
		for (auto act : m_recent_game_acts)
		{
			ui->bootRecentMenu->removeAction(act);
		}
		m_recent_game_acts.clear();
		m_gui_settings->SetValue(gui::rg_entries, m_gui_settings->List2Var(q_pair_list()));
	});

	connect(ui->freezeRecentAct, &QAction::triggered, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::rg_freeze, checked);
	});

	connect(ui->bootInstallPkgAct, &QAction::triggered, [this] {InstallPackages(); });
	connect(ui->bootInstallPupAct, &QAction::triggered, [this] {InstallPup(); });
	connect(ui->exitAct, &QAction::triggered, this, &QWidget::close);

	connect(ui->batchCreatePPUCachesAct, &QAction::triggered, m_game_list_frame, &game_list_frame::BatchCreatePPUCaches);
	connect(ui->batchRemovePPUCachesAct, &QAction::triggered, m_game_list_frame, &game_list_frame::BatchRemovePPUCaches);
	connect(ui->batchRemoveSPUCachesAct, &QAction::triggered, m_game_list_frame, &game_list_frame::BatchRemoveSPUCaches);
	connect(ui->batchRemoveShaderCachesAct, &QAction::triggered, m_game_list_frame, &game_list_frame::BatchRemoveShaderCaches);
	connect(ui->batchRemoveCustomConfigurationsAct, &QAction::triggered, m_game_list_frame, &game_list_frame::BatchRemoveCustomConfigurations);
	connect(ui->batchRemoveCustomPadConfigurationsAct, &QAction::triggered, m_game_list_frame, &game_list_frame::BatchRemoveCustomPadConfigurations);

	connect(ui->removeDiskCacheAct, &QAction::triggered, this, &main_window::RemoveDiskCache);

	connect(ui->removeFirmwareCacheAct, &QAction::triggered, this, &main_window::RemoveFirmwareCache);
	connect(ui->createFirmwareCacheAct, &QAction::triggered, this, &main_window::CreateFirmwareCache);

	connect(ui->sysPauseAct, &QAction::triggered, this, &main_window::OnPlayOrPause);
	connect(ui->sysStopAct, &QAction::triggered, [this]() { Emu.Stop(); });
	connect(ui->sysRebootAct, &QAction::triggered, [this]() { Emu.Restart(); });

	connect(ui->sysSendOpenMenuAct, &QAction::triggered, [this]()
	{
		sysutil_send_system_cmd(m_sys_menu_opened ? 0x0132 /* CELL_SYSUTIL_SYSTEM_MENU_CLOSE */ : 0x0131 /* CELL_SYSUTIL_SYSTEM_MENU_OPEN */, 0);
		m_sys_menu_opened = !m_sys_menu_opened;
		ui->sysSendOpenMenuAct->setText(tr("Send &%0 system menu cmd").arg(m_sys_menu_opened ? tr("close") : tr("open")));
	});

	connect(ui->sysSendExitAct, &QAction::triggered, [this]()
	{
		sysutil_send_system_cmd(0x0101 /* CELL_SYSUTIL_REQUEST_EXITGAME */, 0);
	});

	auto open_settings = [this](int tabIndex)
	{
		settings_dialog dlg(m_gui_settings, m_emu_settings, tabIndex, this);
		connect(&dlg, &settings_dialog::GuiSettingsSaveRequest, this, &main_window::SaveWindowState);
		connect(&dlg, &settings_dialog::GuiSettingsSyncRequest, this, &main_window::ConfigureGuiFromSettings);
		connect(&dlg, &settings_dialog::GuiStylesheetRequest, this, &main_window::RequestGlobalStylesheetChange);
		connect(&dlg, &settings_dialog::GuiRepaintRequest, this, &main_window::RepaintGui);
		connect(&dlg, &settings_dialog::EmuSettingsApplied, this, &main_window::NotifyEmuSettingsChange);
		connect(&dlg, &settings_dialog::EmuSettingsApplied, m_log_frame, &log_frame::LoadSettings);
		dlg.exec();
	};

	connect(ui->confCPUAct,    &QAction::triggered, [=, this]() { open_settings(0); });
	connect(ui->confGPUAct,    &QAction::triggered, [=, this]() { open_settings(1); });
	connect(ui->confAudioAct,  &QAction::triggered, [=, this]() { open_settings(2); });
	connect(ui->confIOAct,     &QAction::triggered, [=, this]() { open_settings(3); });
	connect(ui->confSystemAct, &QAction::triggered, [=, this]() { open_settings(4); });
	connect(ui->confAdvAct,    &QAction::triggered, [=, this]() { open_settings(6); });
	connect(ui->confEmuAct,    &QAction::triggered, [=, this]() { open_settings(7); });
	connect(ui->confGuiAct,    &QAction::triggered, [=, this]() { open_settings(8); });

	auto open_pad_settings = [this]
	{
		pad_settings_dialog dlg(m_gui_settings, this);
		dlg.exec();
	};

	connect(ui->confPadsAct, &QAction::triggered, open_pad_settings);

	connect(ui->confRPCNAct, &QAction::triggered, [this]()
	{
		rpcn_settings_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->confAutopauseManagerAct, &QAction::triggered, [this]()
	{
		auto_pause_settings_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->confVFSDialogAct, &QAction::triggered, [this]()
	{
		vfs_dialog dlg(m_gui_settings, m_emu_settings, this);
		dlg.exec();
		m_game_list_frame->Refresh(true); // dev-hdd0 may have changed. Refresh just in case.
	});

	connect(ui->confSavedataManagerAct, &QAction::triggered, [this]
	{
		save_manager_dialog* save_manager = new save_manager_dialog(m_gui_settings, m_persistent_settings);
		connect(this, &main_window::RequestTrophyManagerRepaint, save_manager, &save_manager_dialog::HandleRepaintUiRequest);
		save_manager->show();
	});

	connect(ui->actionManage_Trophy_Data, &QAction::triggered, [this]
	{
		trophy_manager_dialog* trop_manager = new trophy_manager_dialog(m_gui_settings);
		connect(this, &main_window::RequestTrophyManagerRepaint, trop_manager, &trophy_manager_dialog::HandleRepaintUiRequest);
		trop_manager->show();
	});

	connect(ui->actionManage_Skylanders_Portal, &QAction::triggered, [this]
	{
		skylander_dialog* sky_diag = skylander_dialog::get_dlg(this);
		sky_diag->show();
	});

	connect(ui->actionManage_Cheats, &QAction::triggered, [this]
	{
		cheat_manager_dialog* cheat_manager = cheat_manager_dialog::get_dlg(this);
		cheat_manager->show();
 	});

	connect(ui->actionManage_Game_Patches, &QAction::triggered, [this]
	{
		std::unordered_map<std::string, std::set<std::string>> games;
		if (m_game_list_frame)
		{
			for (const auto& game : m_game_list_frame->GetGameInfo())
			{
				if (game)
				{
					games[game->info.serial].insert(game_list_frame::GetGameVersion(game));
				}
			}
		}
		patch_manager_dialog patch_manager(m_gui_settings, games, "", this);
		patch_manager.exec();
 	});

	connect(ui->actionManage_Users, &QAction::triggered, [this]
	{
		user_manager_dialog user_manager(m_gui_settings, m_persistent_settings, this);
		user_manager.exec();
		m_game_list_frame->Refresh(true); // New user may have different games unlocked.
	});

	connect(ui->actionManage_Screenshots, &QAction::triggered, [this]
	{
		screenshot_manager_dialog* screenshot_manager = new screenshot_manager_dialog();
		screenshot_manager->show();
	});

	connect(ui->toolsCgDisasmAct, &QAction::triggered, [this]
	{
		cg_disasm_window* cgdw = new cg_disasm_window(m_gui_settings);
		cgdw->show();
	});

	connect(ui->actionLog_Viewer, &QAction::triggered, [this]
	{
		log_viewer* viewer = new log_viewer(m_gui_settings);
		viewer->show();
	});

	connect(ui->toolskernel_explorerAct, &QAction::triggered, [this]
	{
		if (!m_kernel_explorer)
		{
			m_kernel_explorer = new kernel_explorer(this);
			connect(m_kernel_explorer, &QDialog::finished, this, [this]() { m_kernel_explorer = nullptr; });
		}

		m_kernel_explorer->show();
	});

	connect(ui->toolsmemory_viewerAct, &QAction::triggered, [this]
	{
		if (!Emu.IsStopped())
			idm::make<memory_viewer_handle>(this);
	});

	connect(ui->toolsRsxDebuggerAct, &QAction::triggered, [this]
	{
		rsx_debugger* rsx = new rsx_debugger(m_gui_settings);
		rsx->show();
	});

	connect(ui->toolsStringSearchAct, &QAction::triggered, [this]
	{
		memory_string_searcher* mss = new memory_string_searcher(this);
		mss->show();
	});

	connect(ui->toolsDecryptSprxLibsAct, &QAction::triggered, this, &main_window::DecryptSPRXLibraries);

	connect(ui->showDebuggerAct, &QAction::triggered, [this](bool checked)
	{
		checked ? m_debugger_frame->show() : m_debugger_frame->hide();
		m_gui_settings->SetValue(gui::mw_debugger, checked);
	});

	connect(ui->showLogAct, &QAction::triggered, [this](bool checked)
	{
		checked ? m_log_frame->show() : m_log_frame->hide();
		m_gui_settings->SetValue(gui::mw_logger, checked);
	});

	connect(ui->showGameListAct, &QAction::triggered, [this](bool checked)
	{
		checked ? m_game_list_frame->show() : m_game_list_frame->hide();
		m_gui_settings->SetValue(gui::mw_gamelist, checked);
	});

	connect(ui->showTitleBarsAct, &QAction::triggered, [this](bool checked)
	{
		ShowTitleBars(checked);
		m_gui_settings->SetValue(gui::mw_titleBarsVisible, checked);
	});

	connect(ui->showToolBarAct, &QAction::triggered, [this](bool checked)
	{
		ui->toolBar->setVisible(checked);
		m_gui_settings->SetValue(gui::mw_toolBarVisible, checked);
	});

	connect(ui->showHiddenEntriesAct, &QAction::triggered, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::gl_show_hidden, checked);
		m_game_list_frame->SetShowHidden(checked);
		m_game_list_frame->Refresh();
	});

	connect(ui->showCompatibilityInGridAct, &QAction::triggered, m_game_list_frame, &game_list_frame::SetShowCompatibilityInGrid);

	connect(ui->refreshGameListAct, &QAction::triggered, [this]
	{
		m_game_list_frame->Refresh(true);
	});

	connect(m_category_visible_act_group, &QActionGroup::triggered, [this](QAction* act)
	{
		QStringList categories;
		int id = 0;
		const bool checked = act->isChecked();

		if      (act == ui->showCatHDDGameAct)    categories += category::cat_hdd_game, id = Category::HDD_Game;
		else if (act == ui->showCatDiscGameAct)   categories += category::cat_disc_game, id = Category::Disc_Game;
		else if (act == ui->showCatPS1GamesAct)   categories += category::cat_ps1_game, id = Category::PS1_Game;
		else if (act == ui->showCatPS2GamesAct)   categories += category::ps2_games, id = Category::PS2_Game;
		else if (act == ui->showCatPSPGamesAct)   categories += category::psp_games, id = Category::PSP_Game;
		else if (act == ui->showCatHomeAct)       categories += category::cat_home, id = Category::Home;
		else if (act == ui->showCatAudioVideoAct) categories += category::media, id = Category::Media;
		else if (act == ui->showCatGameDataAct)   categories += category::data, id = Category::Data;
		else if (act == ui->showCatUnknownAct)    categories += category::cat_unknown, id = Category::Unknown_Cat;
		else if (act == ui->showCatOtherAct)      categories += category::others, id = Category::Others;
		else gui_log.warning("categoryVisibleActGroup: category action not found");

		if (!categories.isEmpty())
		{
			m_game_list_frame->ToggleCategoryFilter(categories, checked);
			m_gui_settings->SetCategoryVisibility(id, checked);
		}
	});

	connect(ui->updateAct, &QAction::triggered, [this]()
	{
#if !defined(_WIN32) && !defined(__linux__)
		QMessageBox::warning(this, tr("Auto-updater"), tr("The auto-updater currently isn't available for your os."));
		return;
#endif
		if(!Emu.IsStopped())
		{
			QMessageBox::warning(this, tr("Auto-updater"), tr("Please stop the emulation before trying to update."));
			return;
		}
		m_updater.check_for_updates(false, false, this);
	});

	connect(ui->aboutAct, &QAction::triggered, [this]
	{
		about_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

	connect(m_icon_size_act_group, &QActionGroup::triggered, [this](QAction* act)
	{
		static const int index_small  = gui::get_Index(gui::gl_icon_size_small);
		static const int index_medium = gui::get_Index(gui::gl_icon_size_medium);

		int index;

		if (act == ui->setIconSizeTinyAct)
			index = 0;
		else if (act == ui->setIconSizeSmallAct)
			index = index_small;
		else if (act == ui->setIconSizeMediumAct)
			index = index_medium;
		else
			index = gui::gl_max_slider_pos;

		m_save_slider_pos = true;
		ResizeIcons(index);
	});

	connect(m_game_list_frame, &game_list_frame::RequestIconSizeChange, [this](const int& val)
	{
		const int idx = ui->sizeSlider->value() + val;
		m_save_slider_pos = true;
		ResizeIcons(idx);
	});

	connect(m_list_mode_act_group, &QActionGroup::triggered, [this](QAction* act)
	{
		const bool is_list_act = act == ui->setlistModeListAct;
		if (is_list_act == m_is_list_mode)
			return;

		const int slider_pos = ui->sizeSlider->sliderPosition();
		ui->sizeSlider->setSliderPosition(m_other_slider_pos);
		SetIconSizeActions(m_other_slider_pos);
		m_other_slider_pos = slider_pos;

		m_is_list_mode = is_list_act;
		m_game_list_frame->SetListMode(m_is_list_mode);
		m_category_visible_act_group->setEnabled(m_is_list_mode);
	});

	connect(ui->toolbar_open, &QAction::triggered, this, &main_window::BootGame);
	connect(ui->toolbar_refresh, &QAction::triggered, [this]() { m_game_list_frame->Refresh(true); });
	connect(ui->toolbar_stop, &QAction::triggered, [this]() { Emu.Stop(); });
	connect(ui->toolbar_start, &QAction::triggered, this, &main_window::OnPlayOrPause);

	connect(ui->toolbar_fullscreen, &QAction::triggered, [this]
	{
		if (isFullScreen())
		{
			showNormal();
			ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_on);
		}
		else
		{
			showFullScreen();
			ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_off);
		}
	});

	connect(ui->toolbar_controls, &QAction::triggered, open_pad_settings);
	connect(ui->toolbar_config, &QAction::triggered, [=, this]() { open_settings(0); });
	connect(ui->toolbar_list, &QAction::triggered, [this]() { ui->setlistModeListAct->trigger(); });
	connect(ui->toolbar_grid, &QAction::triggered, [this]() { ui->setlistModeGridAct->trigger(); });

	connect(ui->sizeSlider, &QSlider::valueChanged, this, &main_window::ResizeIcons);
	connect(ui->sizeSlider, &QSlider::sliderReleased, this, [&]
	{
		const int index = ui->sizeSlider->value();
		m_gui_settings->SetValue(m_is_list_mode ? gui::gl_iconSize : gui::gl_iconSizeGrid, index);
		SetIconSizeActions(index);
	});
	connect(ui->sizeSlider, &QSlider::actionTriggered, [&](int action)
	{
		if (action != QAbstractSlider::SliderNoAction && action != QAbstractSlider::SliderMove)
		{	// we only want to save on mouseclicks or slider release (the other connect handles this)
			m_save_slider_pos = true; // actionTriggered happens before the value was changed
		}
	});

	connect(ui->mw_searchbar, &QLineEdit::textChanged, m_game_list_frame, &game_list_frame::SetSearchText);
}

void main_window::CreateDockWindows()
{
	// new mainwindow widget because existing seems to be bugged for now
	m_mw = new QMainWindow();
	m_mw->setContextMenuPolicy(Qt::PreventContextMenu);

	m_game_list_frame = new game_list_frame(m_gui_settings, m_emu_settings, m_persistent_settings, m_mw);
	m_game_list_frame->setObjectName("gamelist");
	m_debugger_frame = new debugger_frame(m_gui_settings, m_mw);
	m_debugger_frame->setObjectName("debugger");
	m_log_frame = new log_frame(m_gui_settings, m_mw);
	m_log_frame->setObjectName("logger");

	m_mw->addDockWidget(Qt::LeftDockWidgetArea, m_game_list_frame);
	m_mw->addDockWidget(Qt::LeftDockWidgetArea, m_log_frame);
	m_mw->addDockWidget(Qt::RightDockWidgetArea, m_debugger_frame);
	m_mw->setDockNestingEnabled(true);
	m_mw->resizeDocks({ m_log_frame }, { m_mw->sizeHint().height() / 10 }, Qt::Orientation::Vertical);
	setCentralWidget(m_mw);

	connect(m_log_frame, &log_frame::LogFrameClosed, [this]()
	{
		if (ui->showLogAct->isChecked())
		{
			ui->showLogAct->setChecked(false);
			m_gui_settings->SetValue(gui::mw_logger, false);
		}
	});

	connect(m_debugger_frame, &debugger_frame::DebugFrameClosed, [this]()
	{
		if (ui->showDebuggerAct->isChecked())
		{
			ui->showDebuggerAct->setChecked(false);
			m_gui_settings->SetValue(gui::mw_debugger, false);
		}
	});

	connect(m_game_list_frame, &game_list_frame::GameListFrameClosed, [this]()
	{
		if (ui->showGameListAct->isChecked())
		{
			ui->showGameListAct->setChecked(false);
			m_gui_settings->SetValue(gui::mw_gamelist, false);
		}
	});

	connect(m_game_list_frame, &game_list_frame::NotifyGameSelection, [this](const game_info& game)
	{
		// Only change the button logic while the emulator is stopped.
		if (Emu.IsStopped())
		{
			QString tooltip;

			bool enable_play_buttons = true;

			if (game) // A game was selected
			{
				const std::string title_and_title_id = game->info.name + " [" + game->info.serial + "]";

				if (title_and_title_id == Emu.GetTitleAndTitleID()) // This should usually not cause trouble, but feel free to improve.
				{
					tooltip = tr("Restart %0").arg(qstr(title_and_title_id));

					ui->toolbar_start->setIcon(m_icon_restart);
					ui->toolbar_start->setText(tr("Restart"));
				}
				else
				{
					tooltip = tr("Play %0").arg(qstr(title_and_title_id));

					ui->toolbar_start->setIcon(m_icon_play);
					ui->toolbar_start->setText(tr("Play"));
				}
			}
			else if (m_selected_game) // No game was selected. Check if a game was selected before.
			{
				if (Emu.IsReady()) // Prefer games that are about to be booted ("Automatically start games" was set to off)
				{
					tooltip = tr("Play %0").arg(GetCurrentTitle());

					ui->toolbar_start->setIcon(m_icon_play);
				}
				else if (const auto path = Emu.GetBoot(); !path.empty()) // Restartable games
				{
					tooltip = tr("Restart %0").arg(GetCurrentTitle());

					ui->toolbar_start->setIcon(m_icon_restart);
					ui->toolbar_start->setText(tr("Restart"));
				}
				else if (!m_recent_game_acts.isEmpty()) // Get last played game
				{
					tooltip = tr("Play %0").arg(m_recent_game_acts.first()->text());
				}
				else
				{
					enable_play_buttons = false;
				}
			}
			else
			{
				enable_play_buttons = false;
			}

			ui->toolbar_start->setEnabled(enable_play_buttons);
			ui->sysPauseAct->setEnabled(enable_play_buttons);
#ifdef _WIN32
			m_thumb_playPause->setEnabled(enable_play_buttons);
#endif

			if (!tooltip.isEmpty())
			{
				ui->toolbar_start->setToolTip(tooltip);
#ifdef _WIN32
				m_thumb_playPause->setToolTip(tooltip);
#endif
			}
		}

		m_selected_game = game;
	});

	connect(m_game_list_frame, &game_list_frame::RequestBoot, [this](const game_info& game, bool force_global_config)
	{
		Boot(game->info.path, game->info.serial, false, false, force_global_config);
	});

	connect(m_game_list_frame, &game_list_frame::NotifyEmuSettingsChange, this, &main_window::NotifyEmuSettingsChange);
}

void main_window::ConfigureGuiFromSettings(bool configure_all)
{
	// Restore GUI state if needed. We need to if they exist.
	if (!restoreGeometry(m_gui_settings->GetValue(gui::mw_geometry).toByteArray()))
	{
		// By default, set the window to 70% of the screen and the debugger frame is hidden.
		m_debugger_frame->hide();
		resize(QGuiApplication::primaryScreen()->availableSize() * 0.7);
	}

	restoreState(m_gui_settings->GetValue(gui::mw_windowState).toByteArray());
	m_mw->restoreState(m_gui_settings->GetValue(gui::mw_mwState).toByteArray());

	ui->freezeRecentAct->setChecked(m_gui_settings->GetValue(gui::rg_freeze).toBool());
	m_rg_entries = m_gui_settings->Var2List(m_gui_settings->GetValue(gui::rg_entries));

	// clear recent games menu of actions
	for (auto act : m_recent_game_acts)
	{
		ui->bootRecentMenu->removeAction(act);
	}
	m_recent_game_acts.clear();

	// Fill the recent games menu
	for (int i = 0; i < m_rg_entries.count(); i++)
	{
		// adjust old unformatted entries (avoid duplication)
		m_rg_entries[i] = gui::Recent_Game(m_rg_entries[i].first, m_rg_entries[i].second);

		// create new action
		QAction* act = CreateRecentAction(m_rg_entries[i], i + 1);

		// add action to menu
		if (act)
		{
			m_recent_game_acts.append(act);
			ui->bootRecentMenu->addAction(act);
		}
		else
		{
			i--; // list count is now an entry shorter so we have to repeat the same index in order to load all other entries
		}
	}

	ui->showLogAct->setChecked(m_gui_settings->GetValue(gui::mw_logger).toBool());
	ui->showGameListAct->setChecked(m_gui_settings->GetValue(gui::mw_gamelist).toBool());
	ui->showDebuggerAct->setChecked(m_gui_settings->GetValue(gui::mw_debugger).toBool());
	ui->showToolBarAct->setChecked(m_gui_settings->GetValue(gui::mw_toolBarVisible).toBool());
	ui->showTitleBarsAct->setChecked(m_gui_settings->GetValue(gui::mw_titleBarsVisible).toBool());

	m_debugger_frame->setVisible(ui->showDebuggerAct->isChecked());
	m_log_frame->setVisible(ui->showLogAct->isChecked());
	m_game_list_frame->setVisible(ui->showGameListAct->isChecked());
	ui->toolBar->setVisible(ui->showToolBarAct->isChecked());

	ShowTitleBars(ui->showTitleBarsAct->isChecked());

	ui->showHiddenEntriesAct->setChecked(m_gui_settings->GetValue(gui::gl_show_hidden).toBool());
	m_game_list_frame->SetShowHidden(ui->showHiddenEntriesAct->isChecked()); // prevent GetValue in m_game_list_frame->LoadSettings

	ui->showCompatibilityInGridAct->setChecked(m_gui_settings->GetValue(gui::gl_draw_compat).toBool());

	ui->showCatHDDGameAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::HDD_Game));
	ui->showCatDiscGameAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::Disc_Game));
	ui->showCatPS1GamesAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::PS1_Game));
	ui->showCatPS2GamesAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::PS2_Game));
	ui->showCatPSPGamesAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::PSP_Game));
	ui->showCatHomeAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::Home));
	ui->showCatAudioVideoAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::Media));
	ui->showCatGameDataAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::Data));
	ui->showCatUnknownAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::Unknown_Cat));
	ui->showCatOtherAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::Others));

	// handle icon size options
	m_is_list_mode = m_gui_settings->GetValue(gui::gl_listMode).toBool();
	if (m_is_list_mode)
		ui->setlistModeListAct->setChecked(true);
	else
		ui->setlistModeGridAct->setChecked(true);
	m_category_visible_act_group->setEnabled(m_is_list_mode);

	const int icon_size_index = m_gui_settings->GetValue(m_is_list_mode ? gui::gl_iconSize : gui::gl_iconSizeGrid).toInt();
	m_other_slider_pos = m_gui_settings->GetValue(!m_is_list_mode ? gui::gl_iconSize : gui::gl_iconSizeGrid).toInt();
	ui->sizeSlider->setSliderPosition(icon_size_index);
	SetIconSizeActions(icon_size_index);

	if (configure_all)
	{
		// Handle log settings
		m_log_frame->LoadSettings();

		// Gamelist
		m_game_list_frame->LoadSettings();
	}
}

void main_window::SetIconSizeActions(int idx)
{
	static const int threshold_tiny = gui::get_Index((gui::gl_icon_size_small + gui::gl_icon_size_min) / 2);
	static const int threshold_small = gui::get_Index((gui::gl_icon_size_medium + gui::gl_icon_size_small) / 2);
	static const int threshold_medium = gui::get_Index((gui::gl_icon_size_max + gui::gl_icon_size_medium) / 2);

	if (idx < threshold_tiny)
		ui->setIconSizeTinyAct->setChecked(true);
	else if (idx < threshold_small)
		ui->setIconSizeSmallAct->setChecked(true);
	else if (idx < threshold_medium)
		ui->setIconSizeMediumAct->setChecked(true);
	else
		ui->setIconSizeLargeAct->setChecked(true);
}

void main_window::RemoveDiskCache()
{
	const std::string cache_dir = Emulator::GetHdd1Dir() + "/caches";

	if (fs::is_dir(cache_dir) && fs::remove_all(cache_dir, false))
	{
		QMessageBox::information(this, tr("Cache Cleared"), tr("Disk cache was cleared successfully"));
	}
	else
	{
		QMessageBox::warning(this, tr("Error"), tr("Could not remove disk cache"));
	}
}

void main_window::RemoveFirmwareCache()
{
	const std::string cache_dir = Emu.GetCacheDir();

	if (!fs::is_dir(cache_dir))
		return;

	if (QMessageBox::question(this, tr("Confirm Removal"), tr("Remove firmware cache?")) != QMessageBox::Yes)
		return;

	u32 caches_removed = 0;
	u32 caches_total = 0;

	const QStringList filter{ QStringLiteral("ppu-*-lib*.sprx")};

	QDirIterator dir_iter(qstr(cache_dir), filter, QDir::Dirs | QDir::NoDotAndDotDot);

	while (dir_iter.hasNext())
	{
		const QString path = dir_iter.next();

		if (QDir(path).removeRecursively())
		{
			++caches_removed;
			gui_log.notice("Removed firmware cache: %s", sstr(path));
		}
		else
		{
			gui_log.warning("Could not remove firmware cache: %s", sstr(path));
		}

		++caches_total;
	}

	const bool success = caches_total == caches_removed;

	if (success)
		gui_log.success("Removed firmware cache in %s", cache_dir);
	else
		gui_log.fatal("Only %d/%d firmware caches could be removed in %s", caches_removed, caches_total, cache_dir);

	return;
}

void main_window::CreateFirmwareCache()
{
	if (!m_gui_settings->GetBootConfirmation(this))
	{
		return;
	}

	Emu.SetForceBoot(true);

	if (const game_boot_result error = Emu.BootGame(g_cfg.vfs.get_dev_flash() + "sys/external/", "", true);
		error != game_boot_result::no_errors)
	{
		gui_log.error("Creating firmware cache failed: reason: %s", error);
	}
}

void main_window::keyPressEvent(QKeyEvent *keyEvent)
{
	if (((keyEvent->modifiers() & Qt::AltModifier) && keyEvent->key() == Qt::Key_Return) || (isFullScreen() && keyEvent->key() == Qt::Key_Escape))
	{
		ui->toolbar_fullscreen->trigger();
	}

	if (keyEvent->modifiers() & Qt::ControlModifier)
	{
		switch (keyEvent->key())
		{
		case Qt::Key_E: if (Emu.IsPaused()) Emu.Resume(); else if (Emu.IsReady()) Emu.Run(true); return;
		case Qt::Key_P: if (Emu.IsRunning()) Emu.Pause(); return;
		case Qt::Key_S: if (!Emu.IsStopped()) Emu.Stop(); return;
		case Qt::Key_R: if (!Emu.GetBoot().empty()) Emu.Restart(); return;
		}
	}
}

void main_window::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (isFullScreen())
	{
		if (event->button() == Qt::LeftButton)
		{
			showNormal();
			ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_on);
		}
	}
}

/** Override the Qt close event to have the emulator stop and the application die.
*/
void main_window::closeEvent(QCloseEvent* closeEvent)
{
	if (!m_gui_settings->GetBootConfirmation(this, gui::ib_confirm_exit))
	{
		closeEvent->ignore();
		return;
	}

	// Cleanly stop and quit the emulator.
	if (!Emu.IsStopped())
	{
		Emu.Stop();
	}
	Emu.Quit(true);
}

/**
Add valid disc games to gamelist (games.yml)
@param path = dir path to scan for game
*/
void main_window::AddGamesFromDir(const QString& path)
{
	if (!QFileInfo(path).isDir())
		return;

	const std::string s_path = sstr(path);

	// search dropped path first or else the direct parent to an elf is wrongly skipped
	if (const auto error = Emu.BootGame(s_path, "", false, true); error == game_boot_result::no_errors)
	{
		gui_log.notice("Returned from game addition by drag and drop: %s", s_path);
	}

	// search direct subdirectories, that way we can drop one folder containing all games
	QDirIterator dir_iter(path, QDir::Dirs | QDir::NoDotAndDotDot);
	while (dir_iter.hasNext())
	{
		const std::string path = sstr(dir_iter.next());

		if (const auto error = Emu.BootGame(path, "", false, true); error == game_boot_result::no_errors)
		{
			gui_log.notice("Returned from game addition by drag and drop: %s", path);
		}
	}
}

/**
Check data for valid file types and cache their paths if necessary
@param md = data containing file urls
@param savePaths = flag for path caching
@returns validity of file type
*/
main_window::drop_type main_window::IsValidFile(const QMimeData& md, QStringList* drop_paths)
{
	auto drop_type = drop_type::drop_error;

	const QList<QUrl> list = md.urls(); // get list of all the dropped file urls

	for (auto&& url : list) // check each file in url list for valid type
	{
		const QString path = url.toLocalFile(); // convert url to filepath

		const QFileInfo info = path;

		// check for directories first, only valid if all other paths led to directories until now.
		if (info.isDir())
		{
			if (drop_type != drop_type::drop_dir && drop_type != drop_type::drop_error)
			{
				return drop_type::drop_error;
			}

			drop_type = drop_type::drop_dir;
		}
		else if (info.fileName() == "PS3UPDAT.PUP")
		{
			if (list.size() != 1)
			{
				return drop_type::drop_error;
			}

			drop_type = drop_type::drop_pup;
		}
		else if (info.suffix().toLower() == "pkg")
		{
			if (drop_type != drop_type::drop_pkg && drop_type != drop_type::drop_error)
			{
				return drop_type::drop_error;
			}

			drop_type = drop_type::drop_pkg;
		}
		else if (info.suffix() == "rap")
		{
			if (drop_type != drop_type::drop_rap && drop_type != drop_type::drop_error)
			{
				return drop_type::drop_error;
			}

			drop_type = drop_type::drop_rap;
		}
		else if (list.size() == 1)
		{
			if (info.suffix() == "rrc")
			{
				drop_type = drop_type::drop_rrc;
			}
			else
			{
				drop_type = drop_type::drop_game;
			}
		}
		else
		{
			return drop_type::drop_error;
		}

		if (drop_paths) // we only need to know the paths on drop
		{
			drop_paths->append(path);
		}
	}

	return drop_type;
}

void main_window::dropEvent(QDropEvent* event)
{
	QStringList drop_paths;

	switch (IsValidFile(*event->mimeData(), &drop_paths)) // get valid file paths and drop type
	{
	case drop_type::drop_error:
	{
		break;
	}
	case drop_type::drop_pkg: // install the packages
	{
		InstallPackages(drop_paths);
		break;
	}
	case drop_type::drop_pup: // install the firmware
	{
		InstallPup(drop_paths.first());
		break;
	}
	case drop_type::drop_rap: // import rap files to exdata dir
	{
		int installed_rap_count = 0;

		for (const auto& rap : drop_paths)
		{
			const std::string rapname = sstr(QFileInfo(rap).fileName());

			if (InstallRapFile(rap, rapname))
			{
				gui_log.success("Successfully copied rap file by drop: %s", rapname);
				installed_rap_count++;
			}
			else
			{
				gui_log.error("Could not copy rap file by drop: %s", rapname);
			}
		}

		if (installed_rap_count > 0)
		{
			// Refresh game list since we probably unlocked some games now.
			m_game_list_frame->Refresh(true);
		}
		break;
	}
	case drop_type::drop_dir: // import valid games to gamelist (games.yaml)
	{
		if (!m_gui_settings->GetBootConfirmation(this))
		{
			return;
		}
		for (const auto& path : drop_paths)
		{
			AddGamesFromDir(path);
		}
		m_game_list_frame->Refresh(true);
		break;
	}
	case drop_type::drop_game: // import valid games to gamelist (games.yaml)
	{
		if (!m_gui_settings->GetBootConfirmation(this))
		{
			return;
		}
		if (const auto error = Emu.BootGame(sstr(drop_paths.first()), "", true); error != game_boot_result::no_errors)
		{
			gui_log.error("Boot failed: reason: %s, path: %s", error, sstr(drop_paths.first()));
			show_boot_error(error);
		}
		else
		{
			gui_log.success("Elf Boot from drag and drop done: %s", sstr(drop_paths.first()));
			m_game_list_frame->Refresh(true);
		}
		break;
	}
	case drop_type::drop_rrc: // replay a rsx capture file
	{
		BootRsxCapture(sstr(drop_paths.first()));
		break;
	}
	default:
	{
		gui_log.warning("Invalid dropType in gamelist dropEvent");
		break;
	}
	}
}

void main_window::dragEnterEvent(QDragEnterEvent* event)
{
	if (IsValidFile(*event->mimeData()) != drop_type::drop_error)
	{
		event->accept();
	}
}

void main_window::dragMoveEvent(QDragMoveEvent* event)
{
	if (IsValidFile(*event->mimeData()) != drop_type::drop_error)
	{
		event->accept();
	}
}

void main_window::dragLeaveEvent(QDragLeaveEvent* event)
{
	event->accept();
}
