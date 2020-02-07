
#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QDesktopWidget>
#include <QMimeData>

#include "qt_utils.h"
#include "vfs_dialog.h"
#include "save_manager_dialog.h"
#include "trophy_manager_dialog.h"
#include "user_manager_dialog.h"
#include "kernel_explorer.h"
#include "game_list_frame.h"
#include "debugger_frame.h"
#include "log_frame.h"
#include "settings_dialog.h"
#include "auto_pause_settings_dialog.h"
#include "cg_disasm_window.h"
#include "memory_string_searcher.h"
#include "memory_viewer_panel.h"
#include "rsx_debugger.h"
#include "main_window.h"
#include "emu_settings.h"
#include "about_dialog.h"
#include "pad_settings_dialog.h"
#include "progress_dialog.h"
#include "skylander_dialog.h"
#include "cheat_manager.h"
#include "pkg_install_dialog.h"

#include <thread>

#include <QScreen>

#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/vm.h"

#include "Crypto/unpkg.h"
#include "Crypto/unself.h"

#include "Loader/PUP.h"
#include "Loader/TAR.h"
#include "Loader/PSF.h"

#include "Utilities/Thread.h"
#include "Utilities/StrUtil.h"

#include "rpcs3_version.h"
#include "Utilities/sysinfo.h"

#include "ui_main_window.h"

LOG_CHANNEL(gui_log, "GUI");

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

main_window::main_window(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, std::shared_ptr<persistent_settings> persistent_settings, QWidget *parent)
	: QMainWindow(parent)
	, guiSettings(guiSettings)
	, emuSettings(emuSettings)
	, m_persistent_settings(persistent_settings)
	, m_sys_menu_opened(false)
	, ui(new Ui::main_window)
{
	Q_INIT_RESOURCE(resources);

	// We have to setup the ui before using a translation
	ui->setupUi(this);
}

main_window::~main_window()
{
	delete ui;
}

/* An init method is used so that RPCS3App can create the necessary connects before calling init (specifically the stylesheet connect).
 * Simplifies logic a bit.
 */
void main_window::Init()
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

	Q_EMIT RequestGlobalStylesheetChange(guiSettings->GetCurrentStylesheetPath());
	ConfigureGuiFromSettings(true);

#ifdef BRANCH
	if ("RPCS3/rpcs3/master"s != STRINGIZE(BRANCH) && ""s != STRINGIZE(BRANCH))
#else
	if (false)
#endif
	{
		gui_log.warning("Experimental Build Warning! Build origin: " STRINGIZE(BRANCH));

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
		)).arg(Qt::convertFromPlainText(STRINGIZE(BRANCH))));
		msg.layout()->setSizeConstraint(QLayout::SetFixedSize);

		if (msg.exec() == QMessageBox::No)
		{
			std::exit(EXIT_SUCCESS);
		}
	}

	show(); // needs to be done before creating the thumbnail toolbar

	// enable play options if a recent game exists
	const bool enable_play_last = !m_recentGameActs.isEmpty();

	if (enable_play_last)
	{
		ui->sysPauseAct->setEnabled(true);
		ui->sysPauseAct->setText(tr("&Start last played game\tCtrl+E"));
		ui->sysPauseAct->setIcon(m_icon_play);
		ui->toolbar_start->setToolTip(tr("Start last played game"));
		ui->toolbar_start->setEnabled(true);
	}

	// create tool buttons for the taskbar thumbnail
#ifdef _WIN32
	m_thumb_bar = new QWinThumbnailToolBar(this);
	m_thumb_bar->setWindow(windowHandle());

	m_thumb_playPause = new QWinThumbnailToolButton(m_thumb_bar);
	m_thumb_playPause->setToolTip(enable_play_last ? tr("Start last played game") : tr("Start emulation"));
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

	connect(m_thumb_stop, &QWinThumbnailToolButton::clicked, [=]() { Emu.Stop(); });
	connect(m_thumb_restart, &QWinThumbnailToolButton::clicked, [=]() { Emu.Restart(); });
	connect(m_thumb_playPause, &QWinThumbnailToolButton::clicked, this, &main_window::OnPlayOrPause);
#endif

	// Fix possible hidden game list columns. The game list has to be visible already. Use this after show()
	m_gameListFrame->FixNarrowColumns();

#if defined(_WIN32) || defined(__linux__)
	if (guiSettings->GetValue(gui::m_check_upd_start).toBool())
	{
		m_updater.check_for_updates(true, this);
	}
#endif
}

// returns appIcon
QIcon main_window::GetAppIcon()
{
	return m_appIcon;
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
		guiSettings->SetValue(m_is_list_mode ? gui::gl_iconSize : gui::gl_iconSizeGrid, index);

		// this will also fire when we used the actions, but i didn't want to add another boolean member
		SetIconSizeActions(index);
	}

	m_gameListFrame->ResizeIcons(index);
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
	else if (!Emu.GetBoot().empty())
	{
		Emu.Load();
	}
	else if (Emu.IsStopped() && !m_recentGameActs.isEmpty())
	{
		BootRecentAction(m_recentGameActs.first());
	}
}

void main_window::Boot(const std::string& path, const std::string& title_id, bool direct, bool add_only, bool force_global_config)
{
	if (!Emu.IsStopped())
	{
		int result = QMessageBox::Yes;
		guiSettings->ShowConfirmationBox(tr("Close Running Game?"),
			tr("Booting another game will close the current game.\nDo you really want to boot another game?\n\nAny unsaved progress will be lost!\n"),
			gui::ib_confirm_boot, &result, this);

		if (result != QMessageBox::Yes)
		{
			return;
		}
	}

	m_appIcon = gui::utils::get_app_icon_from_path(path, title_id);

	Emu.SetForceBoot(true);
	Emu.Stop();

	if (Emu.BootGame(path, title_id, direct, add_only, force_global_config))
	{
		gui_log.success("Boot successful.");
		const std::string serial = Emu.GetTitleID().empty() ? "" : "[" + Emu.GetTitleID() + "] ";
		if (!add_only)
		{
			AddRecentAction(gui::Recent_Game(qstr(Emu.GetBoot()), qstr(serial + Emu.GetTitle())));
		}
	}
	else
	{
		gui_log.error("Boot failed: %s", path);
	}

	m_gameListFrame->Refresh(true);
}

void main_window::BootElf()
{
	bool stopped = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	QString path_last_ELF = guiSettings->GetValue(gui::fd_boot_elf).toString();
	QString filePath = QFileDialog::getOpenFileName(this, tr("Select (S)ELF To Boot"), path_last_ELF, tr(
		"(S)ELF files (*BOOT.BIN *.elf *.self);;"
		"ELF files (BOOT.BIN *.elf);;"
		"SELF files (EBOOT.BIN *.self);;"
		"BOOT files (*BOOT.BIN);;"
		"BIN files (*.bin);;"
		"All files (*.*)"),
		Q_NULLPTR, QFileDialog::DontResolveSymlinks);

	if (filePath == NULL)
	{
		if (stopped) Emu.Resume();
		return;
	}

	// If we resolved the filepath earlier we would end up setting the last opened dir to the unwanted
	// game folder in case of having e.g. a Game Folder with collected links to elf files.
	// Don't set last path earlier in case of cancelled dialog
	guiSettings->SetValue(gui::fd_boot_elf, filePath);
	const std::string path = sstr(QFileInfo(filePath).canonicalFilePath());

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

	QString path_last_Game = guiSettings->GetValue(gui::fd_boot_game).toString();
	QString dirPath = QFileDialog::getExistingDirectory(this, tr("Select Game Folder"), path_last_Game, QFileDialog::ShowDirsOnly);

	if (dirPath == NULL)
	{
		if (stopped) Emu.Resume();
		return;
	}

	guiSettings->SetValue(gui::fd_boot_game, QFileInfo(dirPath).path());
	const std::string path = sstr(dirPath);

	gui_log.notice("Booting from BootGame...");
	Boot(path);
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

		QString filePath = QFileDialog::getOpenFileName(this, tr("Select RSX Capture"), qstr(fs::get_config_dir() + "captures/"), tr("RRC files (*.rrc);;All files (*.*)"));

		if (filePath.isEmpty())
		{
			if (is_stopped)
			{
				Emu.Resume();
			}
			return;
		}
		path = sstr(filePath);
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

void main_window::InstallPackages(QStringList file_paths, bool show_confirm)
{
	if (file_paths.isEmpty())
	{
		QString path_last_PKG = guiSettings->GetValue(gui::fd_install_pkg).toString();
		const QString file_path = QFileDialog::getOpenFileName(this, tr("Select PKG To Install"), path_last_PKG, tr("PKG files (*.pkg);;All files (*.*)"));

		if (!file_path.isEmpty())
		{
			file_paths.append(file_path);
		}
	}
	else if (show_confirm)
	{
		if (QMessageBox::question(this, tr("PKG Decrypter / Installer"), tr("Install package: %1?").arg(file_paths.front()),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		{
			gui_log.notice("PKG: Cancelled installation from drop. File: %s", sstr(file_paths.front()));
			return;
		}
	}

	if (!file_paths.isEmpty())
	{
		// Handle the actual installations with a timeout. Otherwise the source explorer instance is not usable during the following file processing.
		QTimer::singleShot(0, [this, file_paths]()
		{
			HandlePackageInstallation(file_paths);
		});
	}
}

void main_window::HandlePackageInstallation(QStringList file_paths)
{
	if (file_paths.isEmpty())
	{
		return;
	}

	progress_dialog pdlg(tr("RPCS3 Package Installer"), tr("Installing package, please wait..."), tr("Cancel"), 0, 1000, false, this);
	pdlg.show();

	// Synchronization variable
	atomic_t<double> progress(0.);

	bool cancelled = false;

	for (int i = 0, count = file_paths.count(); i < count; i++)
	{
		progress = 0.;

		pdlg.SetValue(0);
		pdlg.setLabelText(tr("Installing package (%0/%1), please wait...").arg(i + 1).arg(count));
		pdlg.show();

		Emu.SetForceBoot(true);
		Emu.Stop();

		const QString file_path = file_paths.at(i);
		const QFileInfo file_info(file_path);
		const std::string path = sstr(file_path);
		const std::string file_name = sstr(file_info.fileName());

		guiSettings->SetValue(gui::fd_install_pkg, file_info.path());

		// Run PKG unpacking asynchronously
		named_thread worker("PKG Installer", [path, &progress]
		{
			return pkg_install(path, progress);
		});

		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), worker != thread_state::finished)
		{
			if (pdlg.wasCanceled())
			{
				cancelled = true;
				progress -= 1.;
				break;
			}

			// Update progress window
			double pval = progress;
			pval < 0 ? pval += 1. : pval;
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
			m_gameListFrame->Refresh(true);
			gui_log.success("Successfully installed %s.", file_name);

			if (i == (count - 1))
			{
				guiSettings->ShowInfoBox(tr("Success!"), tr("Successfully installed software from package(s)!"), gui::ib_pkg_success, this);
			}
		}
		else
		{
			if (!cancelled)
			{
				gui_log.error("Failed to install %s.", file_name);
				QMessageBox::critical(this, tr("Failure!"), tr("Failed to install software from package %1!").arg(file_path));
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
		QString path_last_PUP = guiSettings->GetValue(gui::fd_install_pup).toString();
		file_path = QFileDialog::getOpenFileName(this, tr("Select PS3UPDAT.PUP To Install"), path_last_PUP, tr("PS3 update file (PS3UPDAT.PUP);;All pup files (*.pup);;All files (*.*)"));
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

	Emu.SetForceBoot(true);
	Emu.Stop();

	guiSettings->SetValue(gui::fd_install_pup, QFileInfo(file_path).path());
	const std::string path = sstr(file_path);

	fs::file pup_f(path);
	if (!pup_f)
	{
		gui_log.error("Error opening PUP file %s", path);
		QMessageBox::critical(this, tr("Failure!"), tr("The selected firmware file couldn't be opened."));
		return;
	}

	if (pup_f.size() < sizeof(PUPHeader))
	{
		gui_log.error("Too small PUP file: %llu", pup_f.size());
		QMessageBox::critical(this, tr("Failure!"), tr("Error while installing firmware: PUP file size is invalid."));
		return;
	}

	struct PUPHeader header = {};
	pup_f.seek(0);
	pup_f.read(header);

	if (header.header_length + header.data_length != pup_f.size())
	{
		gui_log.error("Firmware size mismatch, expected: %llu, actual: %llu + %llu", pup_f.size(), header.header_length, header.data_length);
		QMessageBox::critical(this, tr("Failure!"), tr("Error while installing firmware: PUP file is corrupted."));
		return;
	}

	pup_object pup(pup_f);
	if (!pup)
	{
		gui_log.error("Error while installing firmware: PUP file is invalid.");
		QMessageBox::critical(this, tr("Failure!"), tr("Error while installing firmware: PUP file is invalid."));
		return;
	}

	if (!pup.validate_hashes())
	{
		gui_log.error("Error while installing firmware: Hash check failed. ");
		QMessageBox::critical(this, tr("Failure!"), tr("Error while installing firmware: PUP file contents are invalid."));
		return;
	}

	fs::file update_files_f = pup.get_file(0x300);
	tar_object update_files(update_files_f);
	auto updatefilenames = update_files.get_filenames();

	updatefilenames.erase(std::remove_if(
		updatefilenames.begin(), updatefilenames.end(), [](std::string s) { return s.find("dev_flash_") == std::string::npos; }),
		updatefilenames.end());

	std::string version_string = pup.get_file(0x100).to_string();
	size_t version_pos = version_string.find('\n');
	if (version_pos != std::string::npos)
	{
		version_string.erase(version_pos);
	}

	const std::string cur_version = "4.85";

	if (version_string < cur_version &&
		QMessageBox::question(this, tr("RPCS3 Firmware Installer"), tr("Old firmware detected.\nThe newest firmware version is %1 and you are trying to install version %2\nContinue installation?").arg(qstr(cur_version), qstr(version_string)),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
	{
		return;
	}

	progress_dialog pdlg(tr("RPCS3 Firmware Installer"), tr("Installing firmware version %1\nPlease wait...").arg(qstr(version_string)), tr("Cancel"), 0, static_cast<int>(updatefilenames.size()), false, this);
	pdlg.show();

	// Synchronization variable
	atomic_t<int> progress(0);
	{
		// Run asynchronously
		named_thread worker("Firmware Installer", [&]
		{
			for (const auto& updatefilename : updatefilenames)
			{
				if (progress == -1) break;

				fs::file updatefile = update_files.get_file(updatefilename);

				SCEDecrypter self_dec(updatefile);
				self_dec.LoadHeaders();
				self_dec.LoadMetadata(SCEPKG_ERK, SCEPKG_RIV);
				self_dec.DecryptData();

				auto dev_flash_tar_f = self_dec.MakeFile();
				if (dev_flash_tar_f.size() < 3)
				{
					gui_log.error("Error while installing firmware: PUP contents are invalid.");
					QMessageBox::critical(this, tr("Failure!"), tr("Error while installing firmware: PUP contents are invalid."));
					progress = -1;
				}

				tar_object dev_flash_tar(dev_flash_tar_f[2]);
				if (!dev_flash_tar.extract(g_cfg.vfs.get_dev_flash(), "dev_flash/"))
				{
					gui_log.error("Error while installing firmware: TAR contents are invalid.");
					QMessageBox::critical(this, tr("Failure!"), tr("Error while installing firmware: TAR contents are invalid."));
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

	if (progress > 0)
	{
		gui_log.success("Successfully installed PS3 firmware version %s.", version_string);
		guiSettings->ShowInfoBox(tr("Success!"), tr("Successfully installed PS3 firmware and LLE Modules!"), gui::ib_pup_success, this);

		Emu.SetForceBoot(true);
		Emu.BootGame(g_cfg.vfs.get_dev_flash() + "sys/external/", "", true);
	}
}

// This is ugly, but PS3 headers shall not be included there.
extern void sysutil_send_system_cmd(u64 status, u64 param);

void main_window::DecryptSPRXLibraries()
{
	QString path_last_SPRX = guiSettings->GetValue(gui::fd_decrypt_sprx).toString();
	QStringList modules = QFileDialog::getOpenFileNames(this, tr("Select binary files"), path_last_SPRX, tr("BIN files (*.BIN);;SELF files (*.self);;SPRX files (*.sprx);;All Binaries (*.BIN *.self *.sprx);;All files (*.*)"));

	if (modules.isEmpty())
	{
		return;
	}

	Emu.SetForceBoot(true);
	Emu.Stop();

	guiSettings->SetValue(gui::fd_decrypt_sprx, QFileInfo(modules.first()).path());

	gui_log.notice("Decrypting binaries...");

	for (const QString& module : modules)
	{
		const std::string old_path = sstr(module);

		fs::file elf_file(old_path);

		if (elf_file && elf_file.size() >= 4 && elf_file.read<u32>() == "SCE\0"_u32)
		{
			elf_file = decrypt_self(std::move(elf_file));

			if (elf_file)
			{
				const std::string bin_ext  = module.toLower().endsWith(".sprx") ? ".prx" : ".elf";
				const std::string new_path = old_path.substr(0, old_path.find_last_of(".")) + bin_ext;

				if (fs::file new_file{new_path, fs::rewrite})
				{
					new_file.write(elf_file.to_string());
					gui_log.success("Decrypted %s", old_path);
				}
				else
				{
					gui_log.error("Failed to create %s", new_path);
				}
			}
			else
			{
				gui_log.error("Failed to decrypt %s", old_path);
			}
		}
	}

	gui_log.notice("Finished decrypting all binaries.");
}

/** Needed so that when a backup occurs of window state in guisettings, the state is current.
* Also, so that on close, the window state is preserved.
*/
void main_window::SaveWindowState()
{
	// Save gui settings
	guiSettings->SetValue(gui::mw_geometry, saveGeometry());
	guiSettings->SetValue(gui::mw_windowState, saveState());
	guiSettings->SetValue(gui::mw_mwState, m_mw->saveState());

	// Save column settings
	m_gameListFrame->SaveSettings();
	// Save splitter state
	m_debuggerFrame->SaveSettings();
}

void main_window::RepaintThumbnailIcons()
{
	QColor newColor = gui::utils::get_label_color("thumbnail_icon_color");

	auto icon = [&newColor](const QString& path)
	{
		return gui::utils::get_colorized_icon(QPixmap::fromImage(gui::utils::get_opaque_image_area(path)), Qt::black, newColor);
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
	QColor newColor = gui::utils::get_label_color("toolbar_icon_color");

	auto icon = [&newColor](const QString& path)
	{
		return gui::utils::get_colorized_icon(QIcon(path), Qt::black, newColor);
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
		.arg(newColor.red()).arg(newColor.green()).arg(newColor.blue()).arg(newColor.alpha()));

	// resize toolbar elements

	// for highdpi resize toolbar icons and height dynamically
	// choose factors to mimic Gui-Design in main_window.ui
	// TODO: delete this in case Qt::AA_EnableHighDpiScaling is enabled in main.cpp
#ifdef _WIN32
	const int toolIconHeight = menuBar()->sizeHint().height() * 1.5;
	ui->toolBar->setIconSize(QSize(toolIconHeight, toolIconHeight));
#endif

	const int toolBarHeight = ui->toolBar->sizeHint().height();

	for (const auto& act : ui->toolBar->actions())
	{
		if (act->isSeparator())
		{
			continue;
		}

		ui->toolBar->widgetForAction(act)->setMinimumWidth(toolBarHeight);
	}

	ui->sizeSliderContainer->setFixedWidth(toolBarHeight * 4);
	ui->mw_searchbar->setFixedWidth(toolBarHeight * 5);
}

void main_window::OnEmuRun(bool /*start_playtime*/)
{
	m_debuggerFrame->EnableButtons(true);
#ifdef _WIN32
	m_thumb_playPause->setToolTip(tr("Pause emulation"));
	m_thumb_playPause->setIcon(m_icon_thumb_pause);
#endif
	ui->sysPauseAct->setText(tr("&Pause\tCtrl+P"));
	ui->sysPauseAct->setIcon(m_icon_pause);
	ui->toolbar_start->setIcon(m_icon_pause);
	ui->toolbar_start->setText(tr("Pause"));
	ui->toolbar_start->setToolTip(tr("Pause emulation"));
	EnableMenus(true);
}

void main_window::OnEmuResume()
{
#ifdef _WIN32
	m_thumb_playPause->setToolTip(tr("Pause emulation"));
	m_thumb_playPause->setIcon(m_icon_thumb_pause);
#endif
	ui->sysPauseAct->setText(tr("&Pause\tCtrl+P"));
	ui->sysPauseAct->setIcon(m_icon_pause);
	ui->toolbar_start->setIcon(m_icon_pause);
	ui->toolbar_start->setText(tr("Pause"));
	ui->toolbar_start->setToolTip(tr("Pause emulation"));
}

void main_window::OnEmuPause()
{
#ifdef _WIN32
	m_thumb_playPause->setToolTip(tr("Resume emulation"));
	m_thumb_playPause->setIcon(m_icon_thumb_play);
#endif
	ui->sysPauseAct->setText(tr("&Resume\tCtrl+E"));
	ui->sysPauseAct->setIcon(m_icon_play);
	ui->toolbar_start->setIcon(m_icon_play);
	ui->toolbar_start->setText(tr("Play"));
	ui->toolbar_start->setToolTip(tr("Resume emulation"));

	// Refresh game list in order to update time played
	if (m_gameListFrame)
	{
		m_gameListFrame->Refresh();
	}
}

void main_window::OnEmuStop()
{
	m_debuggerFrame->EnableButtons(false);
	m_debuggerFrame->ClearBreakpoints();

	ui->sysPauseAct->setText(Emu.IsReady() ? tr("&Start\tCtrl+E") : tr("&Resume\tCtrl+E"));
	ui->sysPauseAct->setIcon(m_icon_play);
#ifdef _WIN32
	m_thumb_playPause->setToolTip(Emu.IsReady() ? tr("Start emulation") : tr("Resume emulation"));
	m_thumb_playPause->setIcon(m_icon_thumb_play);
#endif
	EnableMenus(false);
	if (!Emu.GetBoot().empty())
	{
		ui->toolbar_start->setEnabled(true);
		ui->toolbar_start->setIcon(m_icon_restart);
		ui->toolbar_start->setText(tr("Restart"));
		ui->toolbar_start->setToolTip(tr("Restart emulation"));
		ui->sysRebootAct->setEnabled(true);
#ifdef _WIN32
		m_thumb_restart->setEnabled(true);
#endif
	}
	else
	{
		ui->toolbar_start->setIcon(m_icon_play);
		ui->toolbar_start->setText(tr("Play"));
		ui->toolbar_start->setToolTip(Emu.IsReady() ? tr("Start emulation") : tr("Resume emulation"));
	}
	ui->actionManage_Users->setEnabled(true);

	// Refresh game list in order to update time played
	if (m_gameListFrame)
	{
		m_gameListFrame->Refresh();
	}
}

void main_window::OnEmuReady()
{
	m_debuggerFrame->EnableButtons(true);
#ifdef _WIN32
	m_thumb_playPause->setToolTip(Emu.IsReady() ? tr("Start emulation") : tr("Resume emulation"));
	m_thumb_playPause->setIcon(m_icon_thumb_play);
#endif
	ui->sysPauseAct->setText(Emu.IsReady() ? tr("&Start\tCtrl+E") : tr("&Resume\tCtrl+E"));
	ui->sysPauseAct->setIcon(m_icon_play);
	ui->toolbar_start->setIcon(m_icon_play);
	ui->toolbar_start->setText(tr("Play"));
	ui->toolbar_start->setToolTip(Emu.IsReady() ? tr("Start emulation") : tr("Resume emulation"));
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
}

void main_window::BootRecentAction(const QAction* act)
{
	if (Emu.IsRunning())
	{
		return;
	}

	const QString pth = act->data().toString();
	const std::string path = sstr(pth);
	QString nam;
	bool containsPath = false;

	int idx = -1;
	for (int i = 0; i < m_rg_entries.count(); i++)
	{
		if (m_rg_entries.at(i).first == pth)
		{
			idx = i;
			containsPath = true;
			nam = m_rg_entries.at(idx).second;
		}
	}

	// path is invalid: remove action from list return
	if ((containsPath && nam.isEmpty()) || (!QFileInfo(pth).isDir() && !QFileInfo(pth).isFile()))
	{
		if (containsPath)
		{
			// clear menu of actions
			for (auto act : m_recentGameActs)
			{
				ui->bootRecentMenu->removeAction(act);
			}

			// remove action from list
			m_rg_entries.removeAt(idx);
			m_recentGameActs.removeAt(idx);

			guiSettings->SetValue(gui::rg_entries, guiSettings->List2Var(m_rg_entries));

			gui_log.error("Recent Game not valid, removed from Boot Recent list: %s", path);

			// refill menu with actions
			for (int i = 0; i < m_recentGameActs.count(); i++)
			{
				m_recentGameActs[i]->setShortcut(tr("Ctrl+%1").arg(i + 1));
				m_recentGameActs[i]->setToolTip(m_rg_entries.at(i).second);
				ui->bootRecentMenu->addAction(m_recentGameActs[i]);
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

			int idx = m_rg_entries.indexOf(entry);
			m_rg_entries.removeAt(idx);

			guiSettings->SetValue(gui::rg_entries, guiSettings->List2Var(m_rg_entries));
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
	connect(act, &QAction::triggered, [=]() {BootRecentAction(act); });

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
	for (auto act : m_recentGameActs)
	{
		ui->bootRecentMenu->removeAction(act);
	}

	// if path already exists, remove it in order to get it to beginning
	if (m_rg_entries.contains(entry))
	{
		int idx = m_rg_entries.indexOf(entry);
		m_rg_entries.removeAt(idx);
		m_recentGameActs.removeAt(idx);
	}

	// remove oldest action at the end if needed
	if (m_rg_entries.count() == 9)
	{
		m_rg_entries.removeLast();
		m_recentGameActs.removeLast();
	}
	else if (m_rg_entries.count() > 9)
	{
		gui_log.error("Recent games entrylist too big");
	}

	if (m_rg_entries.count() < 9)
	{
		// add new action at the beginning
		m_rg_entries.prepend(entry);
		m_recentGameActs.prepend(act);
	}

	// refill menu with actions
	for (int i = 0; i < m_recentGameActs.count(); i++)
	{
		m_recentGameActs[i]->setShortcut(tr("Ctrl+%1").arg(i+1));
		m_recentGameActs[i]->setToolTip(m_rg_entries.at(i).second);
		ui->bootRecentMenu->addAction(m_recentGameActs[i]);
	}

	guiSettings->SetValue(gui::rg_entries, guiSettings->List2Var(m_rg_entries));
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
	if (m_gameListFrame)
	{
		m_gameListFrame->RepaintIcons(true);
	}

	if (m_logFrame)
	{
		m_logFrame->RepaintTextColors();
	}

	if (m_debuggerFrame)
	{
		m_debuggerFrame->ChangeColors();
	}

	RepaintToolBarIcons();
	RepaintThumbnailIcons();

	Q_EMIT RequestTrophyManagerRepaint();
}

void main_window::RetranslateUI(const QStringList& language_codes, const QString& language)
{
	UpdateLanguageActions(language_codes, language);

	ui->retranslateUi(this);

	if (m_gameListFrame)
	{
		m_gameListFrame->Refresh(true);
	}
}

void main_window::ShowTitleBars(bool show)
{
	m_gameListFrame->SetTitleBarVisible(show);
	m_debuggerFrame->SetTitleBarVisible(show);
	m_logFrame->SetTitleBarVisible(show);
}

void main_window::CreateActions()
{
	ui->exitAct->setShortcuts(QKeySequence::Quit);

	ui->toolbar_start->setEnabled(false);
	ui->toolbar_stop->setEnabled(false);

	m_categoryVisibleActGroup = new QActionGroup(this);
	m_categoryVisibleActGroup->addAction(ui->showCatHDDGameAct);
	m_categoryVisibleActGroup->addAction(ui->showCatDiscGameAct);
	m_categoryVisibleActGroup->addAction(ui->showCatPS1GamesAct);
	m_categoryVisibleActGroup->addAction(ui->showCatPS2GamesAct);
	m_categoryVisibleActGroup->addAction(ui->showCatPSPGamesAct);
	m_categoryVisibleActGroup->addAction(ui->showCatHomeAct);
	m_categoryVisibleActGroup->addAction(ui->showCatAudioVideoAct);
	m_categoryVisibleActGroup->addAction(ui->showCatGameDataAct);
	m_categoryVisibleActGroup->addAction(ui->showCatUnknownAct);
	m_categoryVisibleActGroup->addAction(ui->showCatOtherAct);
	m_categoryVisibleActGroup->setExclusive(false);

	m_iconSizeActGroup = new QActionGroup(this);
	m_iconSizeActGroup->addAction(ui->setIconSizeTinyAct);
	m_iconSizeActGroup->addAction(ui->setIconSizeSmallAct);
	m_iconSizeActGroup->addAction(ui->setIconSizeMediumAct);
	m_iconSizeActGroup->addAction(ui->setIconSizeLargeAct);

	m_listModeActGroup = new QActionGroup(this);
	m_listModeActGroup->addAction(ui->setlistModeListAct);
	m_listModeActGroup->addAction(ui->setlistModeGridAct);
}

void main_window::CreateConnects()
{
	connect(ui->bootElfAct, &QAction::triggered, this, &main_window::BootElf);
	connect(ui->bootGameAct, &QAction::triggered, this, &main_window::BootGame);
	connect(ui->actionopen_rsx_capture, &QAction::triggered, [this](){ BootRsxCapture(); });

	connect(ui->addGamesAct, &QAction::triggered, [this]() {
		QStringList paths;

		// Only select one folder for now
		paths << QFileDialog::getExistingDirectory(this, tr("Select a folder containing one or more games"), qstr(fs::get_config_dir()), QFileDialog::ShowDirsOnly);

		if (!paths.isEmpty())
		{
			for (const QString& path : paths)
			{
				AddGamesFromDir(path);
			}
			m_gameListFrame->Refresh(true);
		}
	});

	connect(ui->bootRecentMenu, &QMenu::aboutToShow, [=]
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

	connect(ui->clearRecentAct, &QAction::triggered, [this]
	{
		if (ui->freezeRecentAct->isChecked()) { return; }
		m_rg_entries.clear();
		for (auto act : m_recentGameActs)
		{
			ui->bootRecentMenu->removeAction(act);
		}
		m_recentGameActs.clear();
		guiSettings->SetValue(gui::rg_entries, guiSettings->List2Var(q_pair_list()));
	});

	connect(ui->freezeRecentAct, &QAction::triggered, [=](bool checked)
	{
		guiSettings->SetValue(gui::rg_freeze, checked);
	});

	connect(ui->bootInstallPkgAct, &QAction::triggered, [this] {InstallPackages(); });
	connect(ui->bootInstallPupAct, &QAction::triggered, [this] {InstallPup(); });
	connect(ui->exitAct, &QAction::triggered, this, &QWidget::close);

	connect(ui->batchCreatePPUCachesAct, &QAction::triggered, m_gameListFrame, &game_list_frame::BatchCreatePPUCaches);
	connect(ui->batchRemovePPUCachesAct, &QAction::triggered, m_gameListFrame, &game_list_frame::BatchRemovePPUCaches);
	connect(ui->batchRemoveSPUCachesAct, &QAction::triggered, m_gameListFrame, &game_list_frame::BatchRemoveSPUCaches);
	connect(ui->batchRemoveShaderCachesAct, &QAction::triggered, m_gameListFrame, &game_list_frame::BatchRemoveShaderCaches);
	connect(ui->batchRemoveCustomConfigurationsAct, &QAction::triggered, m_gameListFrame, &game_list_frame::BatchRemoveCustomConfigurations);
	connect(ui->batchRemoveCustomPadConfigurationsAct, &QAction::triggered, m_gameListFrame, &game_list_frame::BatchRemoveCustomPadConfigurations);

	connect(ui->removeDiskCacheAct, &QAction::triggered, this, &main_window::RemoveDiskCache);

	connect(ui->sysPauseAct, &QAction::triggered, this, &main_window::OnPlayOrPause);
	connect(ui->sysStopAct, &QAction::triggered, [=]() { Emu.Stop(); });
	connect(ui->sysRebootAct, &QAction::triggered, [=]() { Emu.Restart(); });

	connect(ui->sysSendOpenMenuAct, &QAction::triggered, [=]
	{
		sysutil_send_system_cmd(m_sys_menu_opened ? 0x0132 /* CELL_SYSUTIL_SYSTEM_MENU_CLOSE */ : 0x0131 /* CELL_SYSUTIL_SYSTEM_MENU_OPEN */, 0);
		m_sys_menu_opened = !m_sys_menu_opened;
		ui->sysSendOpenMenuAct->setText(tr("Send &%0 system menu cmd").arg(m_sys_menu_opened ? tr("close") : tr("open")));
	});

	connect(ui->sysSendExitAct, &QAction::triggered, [=]
	{
		sysutil_send_system_cmd(0x0101 /* CELL_SYSUTIL_REQUEST_EXITGAME */, 0);
	});

	auto openSettings = [=](int tabIndex)
	{
		settings_dialog dlg(guiSettings, emuSettings, tabIndex, this);
		connect(&dlg, &settings_dialog::GuiSettingsSaveRequest, this, &main_window::SaveWindowState);
		connect(&dlg, &settings_dialog::GuiSettingsSyncRequest, this, &main_window::ConfigureGuiFromSettings);
		connect(&dlg, &settings_dialog::GuiStylesheetRequest, this, &main_window::RequestGlobalStylesheetChange);
		connect(&dlg, &settings_dialog::GuiRepaintRequest, this, &main_window::RepaintGui);
		connect(&dlg, &settings_dialog::accepted, this, &main_window::NotifyEmuSettingsChange);
		connect(&dlg, &settings_dialog::accepted, m_logFrame, &log_frame::LoadSettings);
		dlg.exec();
	};

	connect(ui->confCPUAct,    &QAction::triggered, [=]() { openSettings(0); });
	connect(ui->confGPUAct,    &QAction::triggered, [=]() { openSettings(1); });
	connect(ui->confAudioAct,  &QAction::triggered, [=]() { openSettings(2); });
	connect(ui->confIOAct,     &QAction::triggered, [=]() { openSettings(3); });
	connect(ui->confSystemAct, &QAction::triggered, [=]() { openSettings(4); });

	auto openPadSettings = [this]
	{
		if (!Emu.IsStopped())
		{
			Emu.GetCallbacks().enable_pads(false);
		}
		pad_settings_dialog dlg(this);
		connect(&dlg, &QDialog::finished, [this](int/* result*/)
		{
			if (Emu.IsStopped())
			{
				return;
			}
			Emu.GetCallbacks().reset_pads(Emu.GetTitleID());
		});
		dlg.exec();
		if (!Emu.IsStopped())
		{
			Emu.GetCallbacks().enable_pads(true);
		}
	};

	connect(ui->confPadsAct, &QAction::triggered, openPadSettings);

	connect(ui->confAutopauseManagerAct, &QAction::triggered, [=]
	{
		auto_pause_settings_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->confVFSDialogAct, &QAction::triggered, [=]
	{
		vfs_dialog dlg(guiSettings, emuSettings, this);
		dlg.exec();
		m_gameListFrame->Refresh(true); // dev-hdd0 may have changed. Refresh just in case.
	});

	connect(ui->confSavedataManagerAct, &QAction::triggered, [=]
	{
		save_manager_dialog* save_manager = new save_manager_dialog(guiSettings);
		connect(this, &main_window::RequestTrophyManagerRepaint, save_manager, &save_manager_dialog::HandleRepaintUiRequest);
		save_manager->show();
	});

	connect(ui->actionManage_Trophy_Data, &QAction::triggered, [=]
	{
		trophy_manager_dialog* trop_manager = new trophy_manager_dialog(guiSettings);
		connect(this, &main_window::RequestTrophyManagerRepaint, trop_manager, &trophy_manager_dialog::HandleRepaintUiRequest);
		trop_manager->show();
	});

	connect(ui->actionManage_Skylanders_Portal, &QAction::triggered, [=]
	{
		skylander_dialog* sky_diag = skylander_dialog::get_dlg(this);
		sky_diag->show();
	});

	connect(ui->actionManage_Cheats, &QAction::triggered, [=]
	{
		cheat_manager_dialog* cheat_manager = cheat_manager_dialog::get_dlg(this);
		cheat_manager->show();
 	});

	connect(ui->actionManage_Users, &QAction::triggered, [=]
	{
		user_manager_dialog user_manager(guiSettings, this);
		user_manager.exec();
		m_gameListFrame->Refresh(true); // New user may have different games unlocked.
	});

	connect(ui->toolsCgDisasmAct, &QAction::triggered, [=]
	{
		cg_disasm_window* cgdw = new cg_disasm_window(guiSettings);
		cgdw->show();
	});

	connect(ui->toolskernel_explorerAct, &QAction::triggered, [=]
	{
		kernel_explorer* kernelExplorer = new kernel_explorer(this);
		kernelExplorer->show();
	});

	connect(ui->toolsmemory_viewerAct, &QAction::triggered, [=]
	{
		memory_viewer_panel* mvp = new memory_viewer_panel(this);
		mvp->show();
	});

	connect(ui->toolsRsxDebuggerAct, &QAction::triggered, [=]
	{
		rsx_debugger* rsx = new rsx_debugger(guiSettings);
		rsx->show();
	});

	connect(ui->toolsStringSearchAct, &QAction::triggered, [=]
	{
		memory_string_searcher* mss = new memory_string_searcher(this);
		mss->show();
	});

	connect(ui->toolsDecryptSprxLibsAct, &QAction::triggered, this, &main_window::DecryptSPRXLibraries);

	connect(ui->showDebuggerAct, &QAction::triggered, [=](bool checked)
	{
		checked ? m_debuggerFrame->show() : m_debuggerFrame->hide();
		guiSettings->SetValue(gui::mw_debugger, checked);
	});

	connect(ui->showLogAct, &QAction::triggered, [=](bool checked)
	{
		checked ? m_logFrame->show() : m_logFrame->hide();
		guiSettings->SetValue(gui::mw_logger, checked);
	});

	connect(ui->showGameListAct, &QAction::triggered, [=](bool checked)
	{
		checked ? m_gameListFrame->show() : m_gameListFrame->hide();
		guiSettings->SetValue(gui::mw_gamelist, checked);
	});

	connect(ui->showTitleBarsAct, &QAction::triggered, [=](bool checked)
	{
		ShowTitleBars(checked);
		guiSettings->SetValue(gui::mw_titleBarsVisible, checked);
	});

	connect(ui->showToolBarAct, &QAction::triggered, [=](bool checked)
	{
		ui->toolBar->setVisible(checked);
		guiSettings->SetValue(gui::mw_toolBarVisible, checked);
	});

	connect(ui->showHiddenEntriesAct, &QAction::triggered, [=](bool checked)
	{
		guiSettings->SetValue(gui::gl_show_hidden, checked);
		m_gameListFrame->SetShowHidden(checked);
		m_gameListFrame->Refresh();
	});

	connect(ui->showCompatibilityInGridAct, &QAction::triggered, m_gameListFrame, &game_list_frame::SetShowCompatibilityInGrid);

	connect(ui->refreshGameListAct, &QAction::triggered, [=]
	{
		m_gameListFrame->Refresh(true);
	});

	connect(m_categoryVisibleActGroup, &QActionGroup::triggered, [=](QAction* act)
	{
		QStringList categories;
		int id = 0;
		const bool& checked = act->isChecked();

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
			m_gameListFrame->ToggleCategoryFilter(categories, checked);
			guiSettings->SetCategoryVisibility(id, checked);
		}
	});

	connect(ui->updateAct, &QAction::triggered, [=]()
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
		m_updater.check_for_updates(false, this);
	});

	connect(ui->aboutAct, &QAction::triggered, [this]
	{
		about_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

	connect(m_iconSizeActGroup, &QActionGroup::triggered, [=](QAction* act)
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

	connect (m_gameListFrame, &game_list_frame::RequestIconSizeChange, [=](const int& val)
	{
		const int idx = ui->sizeSlider->value() + val;
		m_save_slider_pos = true;
		ResizeIcons(idx);
	});

	connect(m_listModeActGroup, &QActionGroup::triggered, [=](QAction* act)
	{
		bool is_list_act = act == ui->setlistModeListAct;
		if (is_list_act == m_is_list_mode)
			return;

		int slider_pos = ui->sizeSlider->sliderPosition();
		ui->sizeSlider->setSliderPosition(m_other_slider_pos);
		m_other_slider_pos = slider_pos;

		m_is_list_mode = is_list_act;
		m_gameListFrame->SetListMode(m_is_list_mode);
		m_categoryVisibleActGroup->setEnabled(m_is_list_mode);
	});

	connect(ui->toolbar_open, &QAction::triggered, this, &main_window::BootGame);
	connect(ui->toolbar_refresh, &QAction::triggered, [=]() { m_gameListFrame->Refresh(true); });
	connect(ui->toolbar_stop, &QAction::triggered, [=]() { Emu.Stop(); });
	connect(ui->toolbar_start, &QAction::triggered, this, &main_window::OnPlayOrPause);

	connect(ui->toolbar_fullscreen, &QAction::triggered, [=]
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

	connect(ui->toolbar_controls, &QAction::triggered, openPadSettings);
	connect(ui->toolbar_config, &QAction::triggered, [=]() { openSettings(0); });
	connect(ui->toolbar_list, &QAction::triggered, [=]() { ui->setlistModeListAct->trigger(); });
	connect(ui->toolbar_grid, &QAction::triggered, [=]() { ui->setlistModeGridAct->trigger(); });

	connect(ui->sizeSlider, &QSlider::valueChanged, this, &main_window::ResizeIcons);
	connect(ui->sizeSlider, &QSlider::sliderReleased, this, [&]
	{
		const int index = ui->sizeSlider->value();
		guiSettings->SetValue(m_is_list_mode ? gui::gl_iconSize : gui::gl_iconSizeGrid, index);
		SetIconSizeActions(index);
	});
	connect(ui->sizeSlider, &QSlider::actionTriggered, [&](int action)
	{
		if (action != QAbstractSlider::SliderNoAction && action != QAbstractSlider::SliderMove)
		{	// we only want to save on mouseclicks or slider release (the other connect handles this)
			m_save_slider_pos = true; // actionTriggered happens before the value was changed
		}
	});

	connect(ui->mw_searchbar, &QLineEdit::textChanged, m_gameListFrame, &game_list_frame::SetSearchText);
}

void main_window::CreateDockWindows()
{
	// new mainwindow widget because existing seems to be bugged for now
	m_mw = new QMainWindow();
	m_mw->setContextMenuPolicy(Qt::PreventContextMenu);

	m_gameListFrame = new game_list_frame(guiSettings, emuSettings, m_persistent_settings, m_mw);
	m_gameListFrame->setObjectName("gamelist");
	m_debuggerFrame = new debugger_frame(guiSettings, m_mw);
	m_debuggerFrame->setObjectName("debugger");
	m_logFrame = new log_frame(guiSettings, m_mw);
	m_logFrame->setObjectName("logger");

	m_mw->addDockWidget(Qt::LeftDockWidgetArea, m_gameListFrame);
	m_mw->addDockWidget(Qt::LeftDockWidgetArea, m_logFrame);
	m_mw->addDockWidget(Qt::RightDockWidgetArea, m_debuggerFrame);
	m_mw->setDockNestingEnabled(true);
	m_mw->resizeDocks({ m_logFrame }, { m_mw->sizeHint().height() / 10 }, Qt::Orientation::Vertical);
	setCentralWidget(m_mw);

	connect(m_logFrame, &log_frame::LogFrameClosed, [=]()
	{
		if (ui->showLogAct->isChecked())
		{
			ui->showLogAct->setChecked(false);
			guiSettings->SetValue(gui::mw_logger, false);
		}
	});

	connect(m_debuggerFrame, &debugger_frame::DebugFrameClosed, [=]()
	{
		if (ui->showDebuggerAct->isChecked())
		{
			ui->showDebuggerAct->setChecked(false);
			guiSettings->SetValue(gui::mw_debugger, false);
		}
	});

	connect(m_gameListFrame, &game_list_frame::GameListFrameClosed, [=]()
	{
		if (ui->showGameListAct->isChecked())
		{
			ui->showGameListAct->setChecked(false);
			guiSettings->SetValue(gui::mw_gamelist, false);
		}
	});

	connect(m_gameListFrame, &game_list_frame::RequestBoot, [this](const game_info& game, bool force_global_config)
	{
		Boot(game->info.path, game->info.serial, false, false, force_global_config);
	});

	connect(m_gameListFrame, &game_list_frame::NotifyEmuSettingsChange, this, &main_window::NotifyEmuSettingsChange);
}

void main_window::ConfigureGuiFromSettings(bool configure_all)
{
	// Restore GUI state if needed. We need to if they exist.
	if (!restoreGeometry(guiSettings->GetValue(gui::mw_geometry).toByteArray()))
	{
		// By default, set the window to 70% of the screen and the debugger frame is hidden.
		m_debuggerFrame->hide();
		resize(QGuiApplication::primaryScreen()->availableSize() * 0.7);
	}

	restoreState(guiSettings->GetValue(gui::mw_windowState).toByteArray());
	m_mw->restoreState(guiSettings->GetValue(gui::mw_mwState).toByteArray());

	ui->freezeRecentAct->setChecked(guiSettings->GetValue(gui::rg_freeze).toBool());
	m_rg_entries = guiSettings->Var2List(guiSettings->GetValue(gui::rg_entries));

	// clear recent games menu of actions
	for (auto act : m_recentGameActs)
	{
		ui->bootRecentMenu->removeAction(act);
	}
	m_recentGameActs.clear();

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
			m_recentGameActs.append(act);
			ui->bootRecentMenu->addAction(act);
		}
		else
		{
			i--; // list count is now an entry shorter so we have to repeat the same index in order to load all other entries
		}
	}

	// hide utilities from the average user
	ui->menuUtilities->menuAction()->setVisible(guiSettings->GetValue(gui::m_showDebugTab).toBool());

	ui->showLogAct->setChecked(guiSettings->GetValue(gui::mw_logger).toBool());
	ui->showGameListAct->setChecked(guiSettings->GetValue(gui::mw_gamelist).toBool());
	ui->showDebuggerAct->setChecked(guiSettings->GetValue(gui::mw_debugger).toBool());
	ui->showToolBarAct->setChecked(guiSettings->GetValue(gui::mw_toolBarVisible).toBool());
	ui->showTitleBarsAct->setChecked(guiSettings->GetValue(gui::mw_titleBarsVisible).toBool());

	m_debuggerFrame->setVisible(ui->showDebuggerAct->isChecked());
	m_logFrame->setVisible(ui->showLogAct->isChecked());
	m_gameListFrame->setVisible(ui->showGameListAct->isChecked());
	ui->toolBar->setVisible(ui->showToolBarAct->isChecked());

	ShowTitleBars(ui->showTitleBarsAct->isChecked());

	ui->showHiddenEntriesAct->setChecked(guiSettings->GetValue(gui::gl_show_hidden).toBool());
	m_gameListFrame->SetShowHidden(ui->showHiddenEntriesAct->isChecked()); // prevent GetValue in m_gameListFrame->LoadSettings

	ui->showCompatibilityInGridAct->setChecked(guiSettings->GetValue(gui::gl_draw_compat).toBool());

	ui->showCatHDDGameAct->setChecked(guiSettings->GetCategoryVisibility(Category::HDD_Game));
	ui->showCatDiscGameAct->setChecked(guiSettings->GetCategoryVisibility(Category::Disc_Game));
	ui->showCatPS1GamesAct->setChecked(guiSettings->GetCategoryVisibility(Category::PS1_Game));
	ui->showCatPS2GamesAct->setChecked(guiSettings->GetCategoryVisibility(Category::PS2_Game));
	ui->showCatPSPGamesAct->setChecked(guiSettings->GetCategoryVisibility(Category::PSP_Game));
	ui->showCatHomeAct->setChecked(guiSettings->GetCategoryVisibility(Category::Home));
	ui->showCatAudioVideoAct->setChecked(guiSettings->GetCategoryVisibility(Category::Media));
	ui->showCatGameDataAct->setChecked(guiSettings->GetCategoryVisibility(Category::Data));
	ui->showCatUnknownAct->setChecked(guiSettings->GetCategoryVisibility(Category::Unknown_Cat));
	ui->showCatOtherAct->setChecked(guiSettings->GetCategoryVisibility(Category::Others));

	// handle icon size options
	m_is_list_mode = guiSettings->GetValue(gui::gl_listMode).toBool();
	if (m_is_list_mode)
		ui->setlistModeListAct->setChecked(true);
	else
		ui->setlistModeGridAct->setChecked(true);
	m_categoryVisibleActGroup->setEnabled(m_is_list_mode);

	int icon_size_index = guiSettings->GetValue(m_is_list_mode ? gui::gl_iconSize : gui::gl_iconSizeGrid).toInt();
	m_other_slider_pos = guiSettings->GetValue(!m_is_list_mode ? gui::gl_iconSize : gui::gl_iconSizeGrid).toInt();
	ui->sizeSlider->setSliderPosition(icon_size_index);
	SetIconSizeActions(icon_size_index);

	if (configure_all)
	{
		// Handle log settings
		m_logFrame->LoadSettings();

		// Gamelist
		m_gameListFrame->LoadSettings();
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
	std::string cacheDir = Emulator::GetHdd1Dir() + "/caches";

	if (fs::is_dir(cacheDir) && fs::remove_all(cacheDir, false))
	{
		QMessageBox::information(this, tr("Cache Cleared"), tr("Disk cache was cleared successfully"));
	}
	else
	{
		QMessageBox::warning(this, tr("Error"), tr("Could not remove disk cache"));
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

/** Override the Qt close event to have the emulator stop and the application die.  May add a warning dialog in future.
*/
void main_window::closeEvent(QCloseEvent* closeEvent)
{
	if (!Emu.IsStopped() && guiSettings->GetValue(gui::ib_confirm_exit).toBool())
	{
		int result = QMessageBox::Yes;

		guiSettings->ShowConfirmationBox(tr("Exit RPCS3?"),
			tr("A game is currently running. Do you really want to close RPCS3?\n\nAny unsaved progress will be lost!\n"),
			gui::ib_confirm_exit, &result, nullptr);

		if (result != QMessageBox::Yes)
		{
			closeEvent->ignore();
			return;
		}
	}

	// Cleanly stop the emulator.
	Emu.Stop();

	SaveWindowState();

	// I need the gui settings to sync, and that means having the destructor called as guiSetting's parent is main_window.
	setAttribute(Qt::WA_DeleteOnClose);
	QMainWindow::close();

	// It's possible to have other windows open, like games.  So, force the application to die.
	QApplication::quit();
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
	if (Emu.BootGame(s_path, "", false, true))
	{
		gui_log.notice("Returned from game addition by drag and drop: %s", s_path);
	}

	// search direct subdirectories, that way we can drop one folder containing all games
	QDirIterator dir_iter(path, QDir::Dirs | QDir::NoDotAndDotDot);
	while (dir_iter.hasNext())
	{
		std::string pth = sstr(dir_iter.next());

		if (Emu.BootGame(pth, "", false, true))
		{
			gui_log.notice("Returned from game addition by drag and drop: %s", pth);
		}
	}
}

/**
Check data for valid file types and cache their paths if necessary
@param md = data containing file urls
@param savePaths = flag for path caching
@returns validity of file type
*/
int main_window::IsValidFile(const QMimeData& md, QStringList* dropPaths)
{
	int dropType = drop_type::drop_error;

	const QList<QUrl> list = md.urls(); // get list of all the dropped file urls

	for (auto&& url : list) // check each file in url list for valid type
	{
		const QString path = url.toLocalFile(); // convert url to filepath

		const QFileInfo info = path;

		// check for directories first, only valid if all other paths led to directories until now.
		if (info.isDir())
		{
			if (dropType != drop_type::drop_dir && dropType != drop_type::drop_error)
			{
				return drop_type::drop_error;
			}

			dropType = drop_type::drop_dir;
		}
		else if (info.fileName() == "PS3UPDAT.PUP")
		{
			if (list.size() != 1)
			{
				return drop_type::drop_error;
			}

			dropType = drop_type::drop_pup;
		}
		else if (info.suffix().toLower() == "pkg")
		{
			if (dropType != drop_type::drop_pkg && dropType != drop_type::drop_error)
			{
				return drop_type::drop_error;
			}

			dropType = drop_type::drop_pkg;
		}
		else if (info.suffix() == "rap")
		{
			if (dropType != drop_type::drop_rap && dropType != drop_type::drop_error)
			{
				return drop_type::drop_error;
			}

			dropType = drop_type::drop_rap;
		}
		else if (list.size() == 1)
		{
			if (info.suffix() == "rrc")
			{
				dropType = drop_type::drop_rrc;
			}
			else
			{
				dropType = drop_type::drop_game;
			}
		}
		else
		{
			return drop_type::drop_error;
		}

		if (dropPaths) // we only need to know the paths on drop
		{
			dropPaths->append(path);
		}
	}

	return dropType;
}

void main_window::dropEvent(QDropEvent* event)
{
	QStringList dropPaths;

	switch (IsValidFile(*event->mimeData(), &dropPaths)) // get valid file paths and drop type
	{
	case drop_type::drop_error:
		break;
	case drop_type::drop_pkg: // install the packages
		if (dropPaths.count() > 1)
		{
			pkg_install_dialog dlg(dropPaths, this);
			connect(&dlg, &QDialog::accepted, [this, &dlg]()
			{
				const QStringList paths = dlg.GetPathsToInstall();
				if (!paths.isEmpty())
				{
					InstallPackages(paths, false);
				}
			});
			dlg.exec();
		}
		else
		{
			InstallPackages(dropPaths, true);
		}
		break;
	case drop_type::drop_pup: // install the firmware
		InstallPup(dropPaths.first());
		break;
	case drop_type::drop_rap: // import rap files to exdata dir
		for (const auto& rap : dropPaths)
		{
			const std::string rapname = sstr(QFileInfo(rap).fileName());

			if (!fs::copy_file(sstr(rap), Emulator::GetHddDir() + "/home/" + Emu.GetUsr() + "/exdata/" + rapname, false))
			{
				gui_log.warning("Could not copy rap file by drop: %s", rapname);
			}
			else
			{
				gui_log.success("Successfully copied rap file by drop: %s", rapname);
			}
		}

		// Refresh game list since we probably unlocked some games now.
		m_gameListFrame->Refresh(true);
		break;
	case drop_type::drop_dir: // import valid games to gamelist (games.yaml)
		for (const auto& path : dropPaths)
		{
			AddGamesFromDir(path);
		}
		m_gameListFrame->Refresh(true);
		break;
	case drop_type::drop_game: // import valid games to gamelist (games.yaml)
		if (Emu.BootGame(sstr(dropPaths.first()), "", true))
		{
			gui_log.success("Elf Boot from drag and drop done: %s", sstr(dropPaths.first()));
		}
		m_gameListFrame->Refresh(true);
		break;
	case drop_type::drop_rrc: // replay a rsx capture file
		BootRsxCapture(sstr(dropPaths.first()));
	default:
		gui_log.warning("Invalid dropType in gamelist dropEvent");
		break;
	}
}

void main_window::dragEnterEvent(QDragEnterEvent* event)
{
	if (IsValidFile(*event->mimeData()))
	{
		event->accept();
	}
}

void main_window::dragMoveEvent(QDragMoveEvent* event)
{
	if (IsValidFile(*event->mimeData()))
	{
		event->accept();
	}
}

void main_window::dragLeaveEvent(QDragLeaveEvent* event)
{
	event->accept();
}
