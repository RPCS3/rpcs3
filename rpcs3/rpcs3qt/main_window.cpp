
#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QProgressDialog>
#include <QDesktopWidget>

#include "vfs_dialog.h"
#include "save_data_list_dialog.h"
#include "kernel_explorer.h"
#include "game_list_frame.h"
#include "debugger_frame.h"
#include "log_frame.h"
#include "settings_dialog.h"
#include "pad_settings_dialog.h"
#include "auto_pause_settings_dialog.h"
#include "cg_disasm_window.h"
#include "memory_string_searcher.h"
#include "memory_viewer_panel.h"
#include "rsx_debugger.h"
#include "main_window.h"
#include "emu_settings.h"
#include "about_dialog.h"

#include <thread>

#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/Memory.h"

#include "Crypto/unpkg.h"
#include "Crypto/unself.h"

#include "Loader/PUP.h"
#include "Loader/TAR.h"

#include "Utilities/Thread.h"
#include "Utilities/StrUtil.h"

#include "rpcs3_version.h"
#include "Utilities/sysinfo.h"

#include "ui_main_window.h"

inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

main_window::main_window(std::shared_ptr<gui_settings> guiSettings, QWidget *parent) : QMainWindow(parent), guiSettings(guiSettings), m_sys_menu_opened(false), ui(new Ui::main_window)
{
}

main_window::~main_window()
{
}

auto Pause = []()
{
	if (Emu.IsReady()) Emu.Run();
	else if (Emu.IsPaused()) Emu.Resume();
	else if (Emu.IsRunning()) Emu.Pause();
	else if (!Emu.GetPath().empty()) Emu.Load();
};

/* An init method is used so that RPCS3App can create the necessary connects before calling init (specifically the stylesheet connect).  
 * Simplifies logic a bit.
 */
void main_window::Init()
{
	ui->setupUi(this);

	// Load Icons: This needs to happen before any actions or buttons are created
	RepaintToolBarIcons();
	appIcon = QIcon(":/rpcs3.ico");

	// add toolbar widgets (crappy Qt designer is not able to)
	ui->sizeSlider->setRange(0, GUI::gl_icon_size.size() - 1);
	// get icon size from list
	int icon_size_index = 0;
	QString icon_Size_Str = guiSettings->GetValue(GUI::gl_iconSize).toString();
	for (int i = 0; i < GUI::gl_icon_size.count(); i++)
	{
		if (GUI::gl_icon_size.at(i).first == icon_Size_Str)
		{
			icon_size_index = i;
			break;
		}
	}
	ui->sizeSlider->setSliderPosition(icon_size_index);
	ui->toolBar->addWidget(ui->sizeSliderContainer);
	ui->toolBar->addSeparator();
	ui->toolBar->addWidget(ui->searchBar);

	// for highdpi resize toolbar icons and height dynamically
	// choose factors to mimic Gui-Design in main_window.ui
	const int toolBarHeight = menuBar()->sizeHint().height() * 1.5;
	ui->toolBar->setIconSize(QSize(toolBarHeight, toolBarHeight));
	ui->sizeSliderContainer->setFixedWidth(toolBarHeight * 5);
	ui->sizeSlider->setFixedHeight(toolBarHeight * 0.65f);

	CreateActions();
	CreateDockWindows();

	setMinimumSize(350, minimumSizeHint().height());    // seems fine on win 10

	CreateConnects();

	setWindowTitle(QString::fromStdString("RPCS3 v" + rpcs3::version.to_string()));
	!appIcon.isNull() ? setWindowIcon(appIcon) : LOG_WARNING(GENERAL, "AppImage could not be loaded!");

	Q_EMIT RequestGlobalStylesheetChange(guiSettings->GetCurrentStylesheetPath());
	ConfigureGuiFromSettings(true);
	
	if (!utils::has_ssse3())
	{
		QMessageBox::critical(this, "SSSE3 Error (with three S, not two)",
			"Your system does not meet the minimum requirements needed to run RPCS3.\n"
			"Your CPU does not support SSSE3 (with three S, not two).\n");

		std::exit(EXIT_FAILURE);
	}
}

void main_window::CreateThumbnailToolbar()
{
#ifdef _WIN32
	icon_thumb_play = QIcon(":/Icons/play_blue.png");
	icon_thumb_pause = QIcon(":/Icons/pause_blue.png");
	icon_thumb_stop = QIcon(":/Icons/stop_blue.png");
	icon_thumb_restart = QIcon(":/Icons/restart_blue.png");

	thumb_bar = new QWinThumbnailToolBar(this);
	thumb_bar->setWindow(windowHandle());

	thumb_playPause = new QWinThumbnailToolButton(thumb_bar);
	thumb_playPause->setToolTip(tr("Pause"));
	thumb_playPause->setIcon(icon_thumb_pause);
	thumb_playPause->setEnabled(false);

	thumb_stop = new QWinThumbnailToolButton(thumb_bar);
	thumb_stop->setToolTip(tr("Stop"));
	thumb_stop->setIcon(icon_thumb_stop);
	thumb_stop->setEnabled(false);

	thumb_restart = new QWinThumbnailToolButton(thumb_bar);
	thumb_restart->setToolTip(tr("Restart"));
	thumb_restart->setIcon(icon_thumb_restart);
	thumb_restart->setEnabled(false);

	thumb_bar->addButton(thumb_playPause);
	thumb_bar->addButton(thumb_stop);
	thumb_bar->addButton(thumb_restart);

	connect(thumb_stop, &QWinThumbnailToolButton::clicked, [=]() { Emu.Stop(); });
	connect(thumb_restart, &QWinThumbnailToolButton::clicked, [=]() { Emu.Stop();	Emu.Load();	});
	connect(thumb_playPause, &QWinThumbnailToolButton::clicked, Pause);
#endif
}

// returns appIcon
QIcon main_window::GetAppIcon()
{
	return appIcon;
}

// loads the appIcon from path and embeds it centered into an empty square icon
void main_window::SetAppIconFromPath(const std::string path)
{
	// get Icon for the gs_frame from path. this handles presumably all possible use cases
	QString qpath = qstr(path);
	std::string icon_list[] = { "/ICON0.PNG", "/PS3_GAME/ICON0.PNG" };
	std::string path_list[] = { path, sstr(qpath.section("/", 0, -2)), sstr(qpath.section("/", 0, -3)) };
	for (std::string pth : path_list)
	{
		if (!fs::is_dir(pth)) continue;

		for (std::string ico : icon_list)
		{
			ico = pth + ico;
			if (fs::is_file(ico))
			{
				// load the image from path. It will most likely be a rectangle
				QImage source = QImage(qstr(ico));
				int edgeMax = std::max(source.width(), source.height());
				
				// create a new transparent image with square size and same format as source (maybe handle other formats than RGB32 as well?)
				QImage::Format format = source.format() == QImage::Format_RGB32 ? QImage::Format_ARGB32 : source.format();
				QImage dest = QImage(edgeMax, edgeMax, format);
				dest.fill(QColor("transparent"));

				// get the location to draw the source image centered within the dest image.
				QPoint destPos = source.width() > source.height() ? QPoint(0, (source.width() - source.height()) / 2)
				                                                  : QPoint((source.height() - source.width()) / 2, 0);

				// Paint the source into/over the dest
				QPainter painter(&dest);
				painter.drawImage(destPos, source);
				painter.end();

				// set Icon
				appIcon = QIcon(QPixmap::fromImage(dest));
				return;
			}
		}
	}
	// if nothing was found reset the icon to default
	appIcon = QIcon(":/rpcs3.ico");
}

void main_window::BootElf()
{
	bool stopped = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	QString path_last_ELF = guiSettings->GetValue(GUI::fd_boot_elf).toString();
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

	LOG_NOTICE(LOADER, "(S)ELF: booting...");

	// If we resolved the filepath earlier we would end up setting the last opened dir to the unwanted
	// game folder in case of having e.g. a Game Folder with collected links to elf files.
	// Don't set last path earlier in case of cancelled dialog
	guiSettings->SetValue(GUI::fd_boot_elf, filePath);
	const std::string path = sstr(QFileInfo(filePath).canonicalFilePath());

	SetAppIconFromPath(path);
	Emu.Stop();

	if (!Emu.BootGame(path, true))
	{
		LOG_ERROR(GENERAL, "PS3 executable not found at path (%s)", path);
	}
	else
	{
		LOG_SUCCESS(LOADER, "(S)ELF: boot done.");

		const std::string serial = Emu.GetTitleID().empty() ? "" : "[" + Emu.GetTitleID() + "] ";
		AddRecentAction(q_string_pair(qstr(Emu.GetBoot()), qstr(serial + Emu.GetTitle())));
		gameListFrame->Refresh(true);
	}
}

void main_window::BootGame()
{
	bool stopped = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	QString path_last_Game = guiSettings->GetValue(GUI::fd_boot_game).toString();
	QString dirPath = QFileDialog::getExistingDirectory(this, tr("Select Game Folder"), path_last_Game, QFileDialog::ShowDirsOnly);

	if (dirPath == NULL)
	{
		if (stopped) Emu.Resume();
		return;
	}
	Emu.Stop();
	guiSettings->SetValue(GUI::fd_boot_game, QFileInfo(dirPath).path());
	const std::string path = sstr(dirPath);
	SetAppIconFromPath(path);

	if (!Emu.BootGame(path))
	{
		LOG_ERROR(GENERAL, "PS3 executable not found in selected folder (%s)", path);
	}
	else
	{
		LOG_SUCCESS(LOADER, "Boot Game: boot done.");

		const std::string serial = Emu.GetTitleID().empty() ? "" : "[" + Emu.GetTitleID() + "] ";
		AddRecentAction(q_string_pair(qstr(Emu.GetBoot()), qstr(serial + Emu.GetTitle())));
		gameListFrame->Refresh(true);
	}
}

void main_window::InstallPkg()
{
	QString path_last_PKG = guiSettings->GetValue(GUI::fd_install_pkg).toString();
	QString filePath = QFileDialog::getOpenFileName(this, tr("Select PKG To Install"), path_last_PKG, tr("PKG files (*.pkg);;All files (*.*)"));

	if (filePath == NULL)
	{
		return;
	}
	Emu.Stop();

	guiSettings->SetValue(GUI::fd_install_pkg, QFileInfo(filePath).path());
	const std::string fileName = sstr(QFileInfo(filePath).fileName());
	const std::string path = sstr(filePath);

	// Open PKG file
	fs::file pkg_f(path);

	if (!pkg_f || pkg_f.size() < 64)
	{
		LOG_ERROR(LOADER, "PKG: Failed to open %s", path);
		return;
	}

	// Get title ID
	std::vector<char> title_id(9);
	pkg_f.seek(55);
	pkg_f.read(title_id);
	pkg_f.seek(0);

	// Get full path
	const auto& local_path = Emu.GetHddDir() + "game/" + std::string(std::begin(title_id), std::end(title_id));

	if (!fs::create_dir(local_path))
	{
		if (fs::is_dir(local_path))
		{
			if (QMessageBox::question(this, tr("PKG Decrypter / Installer"), tr("Another installation found. Do you want to overwrite it?"),	
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
			{
				LOG_ERROR(LOADER, "PKG: Cancelled installation to existing directory %s", local_path);
				return;
			}
		}
		else
		{
			LOG_ERROR(LOADER, "PKG: Could not create the installation directory %s", local_path);
			return;
		}
	}

	QProgressDialog pdlg(tr("Installing package ... please wait ..."), tr("Cancel"), 0, 1000, this);

	pdlg.setWindowTitle(tr("RPCS3 Package Installer"));
	pdlg.setWindowModality(Qt::WindowModal);
	pdlg.setFixedSize(500, pdlg.height());
	pdlg.show();

#ifdef _WIN32
	QWinTaskbarButton *taskbar_button = new QWinTaskbarButton();
	taskbar_button->setWindow(windowHandle());
	QWinTaskbarProgress *taskbar_progress = taskbar_button->progress();
	taskbar_progress->setRange(0, 1000);
	taskbar_progress->setVisible(true);

#endif

	// Synchronization variable
	atomic_t<double> progress(0.);
	{
		// Run PKG unpacking asynchronously
		scope_thread worker("PKG Installer", [&]
		{
			if (pkg_install(pkg_f, local_path + '/', progress))
			{
				progress = 1.;
				return;
			}

			// TODO: Ask user to delete files on cancellation/failure?
			progress = -1.;
		});
		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), std::abs(progress) < 1.)
		{
			if (pdlg.wasCanceled())
			{
				progress -= 1.;
#ifdef _WIN32
				taskbar_progress->hide();
				taskbar_button->~QWinTaskbarButton();
#endif
				if (QMessageBox::question(this, tr("PKG Decrypter / Installer"), tr("Remove incomplete folder?"),
					QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
				{
					fs::remove_all(local_path);
					gameListFrame->Refresh(true);
					LOG_SUCCESS(LOADER, "PKG: removed incomplete installation in %s", local_path);
					return;
				}
				break;
			}
			// Update progress window
			pdlg.setValue(static_cast<int>(progress * pdlg.maximum()));
#ifdef _WIN32
			taskbar_progress->setValue(static_cast<int>(progress * taskbar_progress->maximum()));
#endif
			QCoreApplication::processEvents();
		}

		if (progress > 0.)
		{
			pdlg.setValue(pdlg.maximum());
#ifdef _WIN32
			taskbar_progress->setValue(taskbar_progress->maximum());
#endif
			std::this_thread::sleep_for(100ms);
		}
	}

	if (progress >= 1.)
	{
		gameListFrame->Refresh(true);
		LOG_SUCCESS(GENERAL, "Successfully installed %s.", fileName);
		guiSettings->ShowInfoBox(GUI::ib_pkg_success, tr("Success!"), tr("Successfully installed software from package!"), this);

#ifdef _WIN32
		taskbar_progress->hide();
		taskbar_button->~QWinTaskbarButton();
#endif
	}
}

void main_window::InstallPup()
{
	QString path_last_PUP = guiSettings->GetValue(GUI::fd_install_pup).toString();
	QString filePath = QFileDialog::getOpenFileName(this, tr("Select PS3UPDAT.PUP To Install"), path_last_PUP, tr("PS3 update file (PS3UPDAT.PUP)"));

	if (filePath == NULL)
	{
		return;
	}

	Emu.Stop();

	guiSettings->SetValue(GUI::fd_install_pup, QFileInfo(filePath).path());
	const std::string path = sstr(filePath);

	fs::file pup_f(path);
	pup_object pup(pup_f);
	if (!pup)
	{
		LOG_ERROR(GENERAL, "Error while installing firmware: PUP file is invalid.");
		QMessageBox::critical(this, tr("Failure!"), tr("Error while installing firmware: PUP file is invalid."));
		return;
	}

	fs::file update_files_f = pup.get_file(0x300);
	tar_object update_files(update_files_f);
	auto updatefilenames = update_files.get_filenames();

	updatefilenames.erase(std::remove_if(
		updatefilenames.begin(), updatefilenames.end(), [](std::string s) { return s.find("dev_flash_") == std::string::npos; }),
		updatefilenames.end());

	std::string version_string = pup.get_file(0x100).to_string();
	version_string.erase(version_string.find('\n'));

	const std::string cur_version = "4.81";

	if (version_string < cur_version &&
		QMessageBox::question(this, tr("RPCS3 Firmware Installer"), tr("Old firmware detected.\nThe newest firmware version is %1 and you are trying to install version %2\nContinue installation?").arg(QString::fromStdString(cur_version), QString::fromStdString(version_string)),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
	{
		return;
	}

	QProgressDialog pdlg(tr("Installing firmware version %1\nPlease wait...").arg(QString::fromStdString(version_string)), tr("Cancel"), 0, static_cast<int>(updatefilenames.size()), this);
	pdlg.setWindowTitle(tr("RPCS3 Firmware Installer"));
	pdlg.setWindowModality(Qt::WindowModal);
	pdlg.setFixedSize(500, pdlg.height());
	pdlg.show();

#ifdef _WIN32
	QWinTaskbarButton *taskbar_button = new QWinTaskbarButton();
	taskbar_button->setWindow(windowHandle());
	QWinTaskbarProgress *taskbar_progress = taskbar_button->progress();
	taskbar_progress->setRange(0, static_cast<int>(updatefilenames.size()));
	taskbar_progress->setVisible(true);
#endif

	// Synchronization variable
	atomic_t<int> progress(0);
	{
		// Run asynchronously
		scope_thread worker("Firmware Installer", [&]
		{
			for (auto updatefilename : updatefilenames)
			{
				if (progress == -1) break;

				fs::file updatefile = update_files.get_file(updatefilename);

				SCEDecrypter self_dec(updatefile);
				self_dec.LoadHeaders();
				self_dec.LoadMetadata(SCEPKG_ERK, SCEPKG_RIV);
				self_dec.DecryptData();

				auto dev_flash_tar_f = self_dec.MakeFile();
				if (dev_flash_tar_f.size() < 3) {
					LOG_ERROR(GENERAL, "Error while installing firmware: PUP contents are invalid.");
					QMessageBox::critical(this, tr("Failure!"), tr("Error while installing firmware: PUP contents are invalid."));
					progress = -1;
				}

				tar_object dev_flash_tar(dev_flash_tar_f[2]);
				if (!dev_flash_tar.extract(fs::get_config_dir()))
				{
					LOG_ERROR(GENERAL, "Error while installing firmware: TAR contents are invalid.");
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
#ifdef _WIN32
				taskbar_progress->hide();
				taskbar_button->~QWinTaskbarButton();
#endif				
				break;
			}
			// Update progress window
			pdlg.setValue(static_cast<int>(progress));
#ifdef _WIN32
			taskbar_progress->setValue(static_cast<int>(progress));
#endif
			QCoreApplication::processEvents();
		}

		update_files_f.close();
		pup_f.close();

		if (progress > 0)
		{
			pdlg.setValue(pdlg.maximum());
#ifdef _WIN32
			taskbar_progress->setValue(taskbar_progress->maximum());
#endif
			std::this_thread::sleep_for(100ms);
		}
	}

	if (progress > 0)
	{
		LOG_SUCCESS(GENERAL, "Successfully installed PS3 firmware version %s.", version_string);
		guiSettings->ShowInfoBox(GUI::ib_pup_success, tr("Success!"), tr("Successfully installed PS3 firmware and LLE Modules!"), this);

#ifdef _WIN32
		taskbar_progress->hide();
		taskbar_button->~QWinTaskbarButton();
#endif
	}
}

// This is ugly, but PS3 headers shall not be included there.
extern void sysutil_send_system_cmd(u64 status, u64 param);

void main_window::DecryptSPRXLibraries()
{
	QString path_last_SPRX = guiSettings->GetValue(GUI::fd_decrypt_sprx).toString();
	QStringList modules = QFileDialog::getOpenFileNames(this, tr("Select SPRX files"), path_last_SPRX, tr("SPRX files (*.sprx)"));

	if (modules.isEmpty())
	{
		return;
	}

	Emu.Stop();

	guiSettings->SetValue(GUI::fd_decrypt_sprx, QFileInfo(modules.first()).path());

	LOG_NOTICE(GENERAL, "Decrypting SPRX libraries...");

	for (QString& module : modules)
	{
		std::string prx_path = sstr(module);
		const std::string& prx_dir = fs::get_parent_dir(prx_path);

		fs::file elf_file(prx_path);

		if (elf_file && elf_file.size() >= 4 && elf_file.read<u32>() == "SCE\0"_u32)
		{
			const std::size_t prx_ext_pos = prx_path.find_last_of('.');
			const std::string& prx_ext = fmt::to_upper(prx_path.substr(prx_ext_pos != -1 ? prx_ext_pos : prx_path.size()));
			const std::string& prx_name = prx_path.substr(prx_dir.size());

			elf_file = decrypt_self(std::move(elf_file));

			prx_path.erase(prx_path.size() - 4, 1); // change *.sprx to *.prx

			if (elf_file)
			{
				if (fs::file new_file{ prx_path, fs::rewrite })
				{
					new_file.write(elf_file.to_string());
					LOG_SUCCESS(GENERAL, "Decrypted %s", prx_dir + prx_name);
				}
				else
				{
					LOG_ERROR(GENERAL, "Failed to create %s", prx_path);
				}
			}
			else
			{
				LOG_ERROR(GENERAL, "Failed to decrypt %s", prx_dir + prx_name);
			}
		}
	}

	LOG_NOTICE(GENERAL, "Finished decrypting all SPRX libraries.");
}

/** Needed so that when a backup occurs of window state in guisettings, the state is current. 
* Also, so that on close, the window state is preserved.
*/
void main_window::SaveWindowState()
{
	// Save gui settings
	guiSettings->SetValue(GUI::mw_geometry, saveGeometry());
	guiSettings->SetValue(GUI::mw_windowState, saveState());
	guiSettings->SetValue(GUI::mw_mwState, m_mw->saveState());

	// Save column settings
	gameListFrame->SaveSettings();
	// Save splitter state
	debuggerFrame->SaveSettings();
}

void main_window::RepaintToolBarIcons()
{
	QColor newColor = guiSettings->GetValue(GUI::mw_toolIconColor).value<QColor>();

	icon_play = gui_settings::colorizedIcon(QIcon(":/Icons/play.png"), GUI::mw_tool_icon_color, newColor);
	icon_pause = gui_settings::colorizedIcon(QIcon(":/Icons/pause.png"), GUI::mw_tool_icon_color, newColor);
	icon_stop = gui_settings::colorizedIcon(QIcon(":/Icons/stop.png"), GUI::mw_tool_icon_color, newColor);
	icon_restart = gui_settings::colorizedIcon(QIcon(":/Icons/restart.png"), GUI::mw_tool_icon_color, newColor);
	icon_fullscreen_on = gui_settings::colorizedIcon(QIcon(":/Icons/fullscreen.png"), GUI::mw_tool_icon_color, newColor);
	icon_fullscreen_off = gui_settings::colorizedIcon(QIcon(":/Icons/fullscreen_invert.png"), GUI::mw_tool_icon_color, newColor);

	ui->toolbar_config->setIcon(gui_settings::colorizedIcon(QIcon(":/Icons/configure.png"), GUI::mw_tool_icon_color, newColor));
	ui->toolbar_controls->setIcon(gui_settings::colorizedIcon(QIcon(":/Icons/controllers.png"), GUI::mw_tool_icon_color, newColor));
	ui->toolbar_disc->setIcon(gui_settings::colorizedIcon(QIcon(":/Icons/disc.png"), GUI::mw_tool_icon_color, newColor));
	ui->toolbar_grid->setIcon(gui_settings::colorizedIcon(QIcon(":/Icons/grid.png"), GUI::mw_tool_icon_color, newColor));
	ui->toolbar_list->setIcon(gui_settings::colorizedIcon(QIcon(":/Icons/list.png"), GUI::mw_tool_icon_color, newColor));
	ui->toolbar_refresh->setIcon(gui_settings::colorizedIcon(QIcon(":/Icons/refresh.png"), GUI::mw_tool_icon_color, newColor));
	ui->toolbar_snap->setIcon(gui_settings::colorizedIcon(QIcon(":/Icons/screenshot.png"), GUI::mw_tool_icon_color, newColor));
	ui->toolbar_sort->setIcon(gui_settings::colorizedIcon(QIcon(":/Icons/sort.png"), GUI::mw_tool_icon_color, newColor));
	ui->toolbar_stop->setIcon(gui_settings::colorizedIcon(QIcon(":/Icons/stop.png"), GUI::mw_tool_icon_color, newColor));
	
	if (Emu.IsRunning())
	{
		ui->toolbar_start->setIcon(icon_pause);
	}
	else if (Emu.IsStopped() && !Emu.GetPath().empty())
	{
		ui->toolbar_start->setIcon(icon_restart);
	}
	else
	{
		ui->toolbar_start->setIcon(icon_play);
	}
	
	if (isFullScreen())
	{
		ui->toolbar_fullscreen->setIcon(icon_fullscreen_on);
	}
	else
	{
		ui->toolbar_fullscreen->setIcon(icon_fullscreen_off);
	}

	ui->sizeSlider->setStyleSheet(QString("QSlider::handle:horizontal{ background: rgba(%1, %2, %3, %4); }")
		.arg(newColor.red()).arg(newColor.green()).arg(newColor.blue()).arg(newColor.alpha()));
}

void main_window::OnEmuRun()
{
	debuggerFrame->EnableButtons(true);
#ifdef _WIN32
	thumb_playPause->setToolTip(tr("Pause emulation"));
	thumb_playPause->setIcon(icon_thumb_pause);
#endif
	ui->sysPauseAct->setText(tr("&Pause\tCtrl+P"));
	ui->sysPauseAct->setIcon(icon_pause);
	ui->toolbar_start->setIcon(icon_pause);
	ui->toolbar_start->setToolTip(tr("Pause emulation"));
	EnableMenus(true);
}

void main_window::OnEmuResume()
{
#ifdef _WIN32
	thumb_playPause->setToolTip(tr("Pause emulation"));
	thumb_playPause->setIcon(icon_thumb_pause);
#endif
	ui->sysPauseAct->setText(tr("&Pause\tCtrl+P"));
	ui->sysPauseAct->setIcon(icon_pause);
	ui->toolbar_start->setIcon(icon_pause);
	ui->toolbar_start->setToolTip(tr("Pause emulation"));
}

void main_window::OnEmuPause()
{
#ifdef _WIN32
	thumb_playPause->setToolTip(tr("Resume emulation"));
	thumb_playPause->setIcon(icon_thumb_play);
#endif
	ui->sysPauseAct->setText(tr("&Resume\tCtrl+E"));
	ui->sysPauseAct->setIcon(icon_play);
	ui->toolbar_start->setIcon(icon_play);
	ui->toolbar_start->setToolTip(tr("Resume emulation"));
}

void main_window::OnEmuStop()
{
	debuggerFrame->EnableButtons(false);
	debuggerFrame->ClearBreakpoints();

	ui->sysPauseAct->setText(Emu.IsReady() ? tr("&Start\tCtrl+E") : tr("&Resume\tCtrl+E"));
	ui->sysPauseAct->setIcon(icon_play);
#ifdef _WIN32
	thumb_playPause->setToolTip(Emu.IsReady() ? tr("Start emulation") : tr("Resume emulation"));
	thumb_playPause->setIcon(icon_thumb_play);
#endif
	EnableMenus(false);
	if (!Emu.GetPath().empty())
	{
		ui->toolbar_start->setEnabled(true);
		ui->toolbar_start->setIcon(icon_restart);
		ui->toolbar_start->setToolTip(tr("Restart emulation"));
		ui->sysRebootAct->setEnabled(true);
#ifdef _WIN32
		thumb_restart->setEnabled(true);
#endif
	}
	else
	{
		ui->toolbar_start->setIcon(icon_play);
		ui->toolbar_start->setToolTip(Emu.IsReady() ? tr("Start emulation") : tr("Resume emulation"));
	}
}

void main_window::OnEmuReady()
{
	debuggerFrame->EnableButtons(true);
#ifdef _WIN32
	thumb_playPause->setToolTip(Emu.IsReady() ? tr("Start emulation") : tr("Resume emulation"));
	thumb_playPause->setIcon(icon_thumb_play);
#endif
	ui->sysPauseAct->setText(Emu.IsReady() ? tr("&Start\tCtrl+E") : tr("&Resume\tCtrl+E"));
	ui->sysPauseAct->setIcon(icon_play);
	ui->toolbar_start->setIcon(icon_play);
	ui->toolbar_start->setToolTip(Emu.IsReady() ? tr("Start emulation") : tr("Resume emulation"));
	EnableMenus(true);
}

void main_window::EnableMenus(bool enabled)
{
	// Thumbnail Buttons
#ifdef _WIN32
	thumb_playPause->setEnabled(enabled);
	thumb_stop->setEnabled(enabled);
	thumb_restart->setEnabled(enabled);
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
	if (containsPath && nam.isEmpty() || !QFileInfo(pth).isDir() && !QFileInfo(pth).isFile())
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

			guiSettings->SetValue(GUI::rg_entries, guiSettings->List2Var(m_rg_entries));

			LOG_ERROR(GENERAL, "Recent Game not valid, removed from Boot Recent list: %s", sstr(pth));

			// refill menu with actions
			for (uint i = 0; i < m_recentGameActs.count(); i++)
			{
				m_recentGameActs[i]->setShortcut(tr("Ctrl+%1").arg(i + 1));
				m_recentGameActs[i]->setToolTip(m_rg_entries.at(i).second);
				ui->bootRecentMenu->addAction(m_recentGameActs[i]);
			}

			LOG_WARNING(GENERAL, "Boot Recent list refreshed");

			return;
		}

		LOG_ERROR(GENERAL, "Path invalid and not in m_rg_paths: %s", sstr(pth));

		return;
	}

	SetAppIconFromPath(sstr(pth));

	Emu.Stop();

	if (!Emu.BootGame(sstr(pth), true))
	{
		LOG_ERROR(LOADER, "Failed to boot %s", sstr(pth));
	}
	else
	{
		LOG_SUCCESS(LOADER, "Boot from Recent List: done");
		AddRecentAction(q_string_pair(qstr(Emu.GetBoot()), nam));
		gameListFrame->Refresh(true);
	}
};

QAction* main_window::CreateRecentAction(const q_string_pair& entry, const uint& sc_idx)
{
	// if path is not valid remove from list
	if (entry.second.isEmpty() || !QFileInfo(entry.first).isDir() && !QFileInfo(entry.first).isFile())
	{
		if (m_rg_entries.contains(entry))
		{
			int idx = m_rg_entries.indexOf(entry);
			m_rg_entries.removeAt(idx);

			guiSettings->SetValue(GUI::rg_entries, guiSettings->List2Var(m_rg_entries));

			LOG_ERROR(GENERAL, "Recent Game not valid, removed from Boot Recent list: %s", sstr(entry.first));
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
};

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
		LOG_ERROR(LOADER, "Recent games entrylist too big");
	}

	if (m_rg_entries.count() < 9)
	{
		// add new action at the beginning
		m_rg_entries.prepend(entry);
		m_recentGameActs.prepend(act);
	}
	
	// refill menu with actions
	for (uint i = 0; i < m_recentGameActs.count(); i++)
	{
		m_recentGameActs[i]->setShortcut(tr("Ctrl+%1").arg(i+1));
		m_recentGameActs[i]->setToolTip(m_rg_entries.at(i).second);
		ui->bootRecentMenu->addAction(m_recentGameActs[i]);
	}

	guiSettings->SetValue(GUI::rg_entries, guiSettings->List2Var(m_rg_entries));
}

void main_window::CreateActions()
{
	ui->exitAct->setShortcuts(QKeySequence::Quit);

	ui->toolbar_start->setEnabled(false);
	ui->toolbar_stop->setEnabled(false);

	categoryVisibleActGroup = new QActionGroup(this); 
	categoryVisibleActGroup->addAction(ui->showCatHDDGameAct);
	categoryVisibleActGroup->addAction(ui->showCatDiscGameAct);
	categoryVisibleActGroup->addAction(ui->showCatHomeAct);
	categoryVisibleActGroup->addAction(ui->showCatAudioVideoAct);
	categoryVisibleActGroup->addAction(ui->showCatGameDataAct);
	categoryVisibleActGroup->addAction(ui->showCatUnknownAct);
	categoryVisibleActGroup->addAction(ui->showCatOtherAct);
	categoryVisibleActGroup->setExclusive(false);

	iconSizeActGroup = new QActionGroup(this);
	iconSizeActGroup->addAction(ui->setIconSizeTinyAct);
	iconSizeActGroup->addAction(ui->setIconSizeSmallAct);
	iconSizeActGroup->addAction(ui->setIconSizeMediumAct);
	iconSizeActGroup->addAction(ui->setIconSizeLargeAct);

	listModeActGroup = new QActionGroup(this);
	listModeActGroup->addAction(ui->setlistModeListAct);
	listModeActGroup->addAction(ui->setlistModeGridAct);
}

void main_window::CreateConnects()
{
	connect(ui->bootElfAct, &QAction::triggered, this, &main_window::BootElf);
	connect(ui->bootGameAct, &QAction::triggered, this, &main_window::BootGame);
	connect(ui->bootRecentMenu, &QMenu::aboutToShow, [=]() {
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
	connect(ui->clearRecentAct, &QAction::triggered, [this](){
		if (ui->freezeRecentAct->isChecked()) { return; }
		m_rg_entries.clear();
		for (auto act : m_recentGameActs)
		{
			ui->bootRecentMenu->removeAction(act);
		}
		m_recentGameActs.clear();
		guiSettings->SetValue(GUI::rg_entries, guiSettings->List2Var(q_pair_list()));
	});
	connect(ui->freezeRecentAct, &QAction::triggered, [=](bool checked) {
		guiSettings->SetValue(GUI::rg_freeze, checked);
	});
	connect(ui->bootInstallPkgAct, &QAction::triggered, this, &main_window::InstallPkg);
	connect(ui->bootInstallPupAct, &QAction::triggered, this, &main_window::InstallPup);
	connect(ui->exitAct, &QAction::triggered, this, &QWidget::close);
	connect(ui->sysPauseAct, &QAction::triggered, Pause);
	connect(ui->sysStopAct, &QAction::triggered, [=]() { Emu.Stop(); });
	connect(ui->sysRebootAct, &QAction::triggered, [=]() { Emu.Stop();	Emu.Load();	});
	connect(ui->sysSendOpenMenuAct, &QAction::triggered, [=](){
		sysutil_send_system_cmd(m_sys_menu_opened ? 0x0132 /* CELL_SYSUTIL_SYSTEM_MENU_CLOSE */ : 0x0131 /* CELL_SYSUTIL_SYSTEM_MENU_OPEN */, 0);
		m_sys_menu_opened = !m_sys_menu_opened;
		ui->sysSendOpenMenuAct->setText(tr("Send &%0 system menu cmd").arg(m_sys_menu_opened ? tr("close") : tr("open")));
	});
	connect(ui->sysSendExitAct, &QAction::triggered, [=](){
		sysutil_send_system_cmd(0x0101 /* CELL_SYSUTIL_REQUEST_EXITGAME */, 0);
	});

	auto openSettings = [=](int tabIndex)
	{
		settings_dialog dlg(guiSettings, m_Render_Creator, tabIndex, this);
		connect(&dlg, &settings_dialog::GuiSettingsSaveRequest, this, &main_window::SaveWindowState);
		connect(&dlg, &settings_dialog::GuiSettingsSyncRequest, [=]() {ConfigureGuiFromSettings(true); });
		connect(&dlg, &settings_dialog::GuiStylesheetRequest, this, &main_window::RequestGlobalStylesheetChange);
		connect(&dlg, &settings_dialog::ToolBarRepaintRequest, this, &main_window::RepaintToolBarIcons);
		connect(&dlg, &settings_dialog::ToolBarRepaintRequest, gameListFrame, &game_list_frame::RepaintToolBarIcons);
		connect(&dlg, &settings_dialog::accepted, [this](){
			gameListFrame->LoadSettings();
			QColor tbc = guiSettings->GetValue(GUI::mw_toolBarColor).value<QColor>();
			ui->toolBar->setStyleSheet(QString(
				"QToolBar { background-color: rgba(%1, %2, %3, %4); }"
				"QToolBar::separator {background-color: rgba(%5, %6, %7, %8); width: 1px; margin-top: 2px; margin-bottom: 2px;}"
				"QSlider { background-color: rgba(%1, %2, %3, %4); }"
				"QLineEdit { background-color: rgba(%1, %2, %3, %4); }")
				.arg(tbc.red()).arg(tbc.green()).arg(tbc.blue()).arg(tbc.alpha())
				.arg(tbc.red() - 20).arg(tbc.green() - 20).arg(tbc.blue() - 20).arg(tbc.alpha() - 20));
		});
		dlg.exec();
	};
	connect(ui->confCPUAct,    &QAction::triggered, [=]() { openSettings(0); });
	connect(ui->confGPUAct,    &QAction::triggered, [=]() { openSettings(1); });
	connect(ui->confAudioAct,  &QAction::triggered, [=]() { openSettings(2); });
	connect(ui->confIOAct,     &QAction::triggered, [=]() { openSettings(3); });
	connect(ui->confSystemAct, &QAction::triggered, [=]() { openSettings(4); });

	connect(ui->confPadAct, &QAction::triggered, this, [=](){
		pad_settings_dialog dlg(this);
		dlg.exec();
	});
	connect(ui->confAutopauseManagerAct, &QAction::triggered, [=](){
		auto_pause_settings_dialog dlg(this);
		dlg.exec();
	});
	connect(ui->confVFSDialogAct, &QAction::triggered, [=]() {
		vfs_dialog dlg(this);
		dlg.exec();
		gameListFrame->Refresh(true); // dev-hdd0 may have changed. Refresh just in case.
	});
	connect(ui->confSavedataManagerAct, &QAction::triggered, [=](){
		save_data_list_dialog* sdid = new save_data_list_dialog({}, 0, false, this);
		sdid->show();
	});
	connect(ui->toolsCgDisasmAct, &QAction::triggered, [=](){
		cg_disasm_window* cgdw = new cg_disasm_window(guiSettings, this);
		cgdw->show();
	});
	connect(ui->toolskernel_explorerAct, &QAction::triggered, [=](){
		kernel_explorer* kernelExplorer = new kernel_explorer(this);
		kernelExplorer->show();
	});
	connect(ui->toolsmemory_viewerAct, &QAction::triggered, [=](){
		memory_viewer_panel* mvp = new memory_viewer_panel(this);
		mvp->show();
	});
	connect(ui->toolsRsxDebuggerAct, &QAction::triggered, [=](){
		rsx_debugger* rsx = new rsx_debugger(this);
		rsx->show();
	});
	connect(ui->toolsStringSearchAct, &QAction::triggered, [=](){
		memory_string_searcher* mss = new memory_string_searcher(this);
		mss->show();
	});
	connect(ui->toolsDecryptSprxLibsAct, &QAction::triggered, this, &main_window::DecryptSPRXLibraries);
	connect(ui->showDebuggerAct, &QAction::triggered, [=](bool checked){
		checked ? debuggerFrame->show() : debuggerFrame->hide();
		guiSettings->SetValue(GUI::mw_debugger, checked);
	});
	connect(ui->showLogAct, &QAction::triggered, [=](bool checked){
		checked ? logFrame->show() : logFrame->hide();
		guiSettings->SetValue(GUI::mw_logger, checked);
	});
	connect(ui->showGameListAct, &QAction::triggered, [=](bool checked){
		checked ? gameListFrame->show() : gameListFrame->hide();
		guiSettings->SetValue(GUI::mw_gamelist, checked);
	});
	connect(ui->showToolBarAct, &QAction::triggered, [=](bool checked) {
		ui->toolBar->setVisible(checked);
		guiSettings->SetValue(GUI::mw_toolBarVisible, checked);
	});
	connect(ui->showGameToolBarAct, &QAction::triggered, [=](bool checked) {
		gameListFrame->SetToolBarVisible(checked);
	});
	connect(ui->refreshGameListAct, &QAction::triggered, [=](){
		gameListFrame->Refresh(true);
	});
	connect(categoryVisibleActGroup, &QActionGroup::triggered, [=](QAction* act)
	{
		QStringList categories;
		int id;
		const bool& checked = act->isChecked();

		if      (act == ui->showCatHDDGameAct)    categories += category::non_disc_games, id = Category::Non_Disc_Game;
		else if (act == ui->showCatDiscGameAct)   categories += category::disc_Game, id = Category::Disc_Game;
		else if (act == ui->showCatHomeAct)       categories += category::home, id = Category::Home;
		else if (act == ui->showCatAudioVideoAct) categories += category::media, id = Category::Media;
		else if (act == ui->showCatGameDataAct)   categories += category::data, id = Category::Data;
		else if (act == ui->showCatUnknownAct)    categories += category::unknown, id = Category::Unknown_Cat;
		else if (act == ui->showCatOtherAct)      categories += category::others, id = Category::Others;
		else LOG_WARNING(GENERAL, "categoryVisibleActGroup: category action not found");

		gameListFrame->SetCategoryActIcon(categoryVisibleActGroup->actions().indexOf(act), checked);
		gameListFrame->ToggleCategoryFilter(categories, checked);
		guiSettings->SetCategoryVisibility(id, checked);
	});
	connect(ui->aboutAct, &QAction::triggered, [this]() {
		about_dialog dlg(this);
		dlg.exec();
	});
	connect(ui->aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);
	auto resizeIcons = [=](const int& index){
		if (ui->sizeSlider->value() != index)
		{
			ui->sizeSlider->setSliderPosition(index);
		}
		gameListFrame->ResizeIcons(GUI::gl_icon_size.at(index).first, GUI::gl_icon_size.at(index).second, index);
	};
	connect(iconSizeActGroup, &QActionGroup::triggered, [=](QAction* act)
	{
		int index;

		if (act == ui->setIconSizeTinyAct) index = 0;
		else if (act == ui->setIconSizeSmallAct) index = 1;
		else if (act == ui->setIconSizeMediumAct) index = 2;
		else index = 3;

		resizeIcons(index);
	});
	connect (gameListFrame, &game_list_frame::RequestIconSizeActSet, [=](const int& idx)
	{
		iconSizeActGroup->actions().at(idx)->trigger();
	});
	connect(gameListFrame, &game_list_frame::RequestListModeActSet, [=](const bool& isList)
	{
		isList ? ui->setlistModeListAct->trigger() : ui->setlistModeGridAct->trigger();
	});
	connect(gameListFrame, &game_list_frame::RequestCategoryActSet, [=](const int& id)
	{
		categoryVisibleActGroup->actions().at(id)->trigger();
	});
	connect(listModeActGroup, &QActionGroup::triggered, [=](QAction* act)
	{
		bool isList = act == ui->setlistModeListAct;
		gameListFrame->SetListMode(isList);
		categoryVisibleActGroup->setEnabled(isList);
	});
	connect(ui->toolbar_disc, &QAction::triggered, this, &main_window::BootGame);
	connect(ui->toolbar_refresh, &QAction::triggered, [=]() { gameListFrame->Refresh(true); });
	connect(ui->toolbar_stop, &QAction::triggered, [=]() { Emu.Stop(); });
	connect(ui->toolbar_start, &QAction::triggered, Pause);
	//connect(ui->toolbar_snap, &QAction::triggered, [=]() {});
	connect(ui->toolbar_fullscreen, &QAction::triggered, [=]() {
		if (isFullScreen())
		{
			showNormal();
			ui->toolbar_fullscreen->setIcon(icon_fullscreen_on);
		}
		else
		{
			showFullScreen();
			ui->toolbar_fullscreen->setIcon(icon_fullscreen_off);
		}
	});
	connect(ui->toolbar_controls, &QAction::triggered, [=]() { pad_settings_dialog dlg(this); dlg.exec(); });
	connect(ui->toolbar_config, &QAction::triggered, [=]() { openSettings(0); });
	connect(ui->toolbar_list, &QAction::triggered, [=]() { ui->setlistModeListAct->trigger(); });
	connect(ui->toolbar_grid, &QAction::triggered, [=]() { ui->setlistModeGridAct->trigger(); });
	//connect(ui->toolbar_sort, &QAction::triggered, gameListFrame, sort);
	connect(ui->sizeSlider, &QSlider::valueChanged, resizeIcons);
	connect(ui->searchBar, &QLineEdit::textChanged, gameListFrame, &game_list_frame::SetSearchText);
}

void main_window::CreateDockWindows()
{
	// new mainwindow widget because existing seems to be bugged for now
	m_mw = new QMainWindow();

	gameListFrame = new game_list_frame(guiSettings, m_Render_Creator, m_mw);
	gameListFrame->setObjectName("gamelist");
	debuggerFrame = new debugger_frame(guiSettings, m_mw);
	debuggerFrame->setObjectName("debugger");
	logFrame = new log_frame(guiSettings, m_mw);
	logFrame->setObjectName("logger");

	m_mw->addDockWidget(Qt::LeftDockWidgetArea, gameListFrame);
	m_mw->addDockWidget(Qt::LeftDockWidgetArea, logFrame);
	m_mw->addDockWidget(Qt::RightDockWidgetArea, debuggerFrame);
	m_mw->setDockNestingEnabled(true);
	setCentralWidget(m_mw);

	connect(logFrame, &log_frame::LogFrameClosed, [=]()
	{
		if (ui->showLogAct->isChecked())
		{
			ui->showLogAct->setChecked(false);
			guiSettings->SetValue(GUI::mw_logger, false);
		}
	});
	connect(debuggerFrame, &debugger_frame::DebugFrameClosed, [=](){
		if (ui->showDebuggerAct->isChecked())
		{
			ui->showDebuggerAct->setChecked(false);
			guiSettings->SetValue(GUI::mw_debugger, false);
		}
	});
	connect(gameListFrame, &game_list_frame::GameListFrameClosed, [=]()
	{
		if (ui->showGameListAct->isChecked())
		{
			ui->showGameListAct->setChecked(false);
			guiSettings->SetValue(GUI::mw_gamelist, false);
		}
	});
	connect(gameListFrame, &game_list_frame::RequestIconPathSet, this, &main_window::SetAppIconFromPath);
	connect(gameListFrame, &game_list_frame::RequestAddRecentGame, this, &main_window::AddRecentAction);
}

void main_window::ConfigureGuiFromSettings(bool configureAll)
{
	// Restore GUI state if needed. We need to if they exist.
	QByteArray geometry = guiSettings->GetValue(GUI::mw_geometry).toByteArray();
	if (geometry.isEmpty() == false)
	{
		restoreGeometry(geometry);
	}
	else
	{	// By default, set the window to 70% of the screen and the debugger frame is hidden.
		debuggerFrame->hide();

		QSize defaultSize = QDesktopWidget().availableGeometry().size() * 0.7;
		resize(defaultSize);
	}

	restoreState(guiSettings->GetValue(GUI::mw_windowState).toByteArray());
	m_mw->restoreState(guiSettings->GetValue(GUI::mw_mwState).toByteArray());

	ui->freezeRecentAct->setChecked(guiSettings->GetValue(GUI::rg_freeze).toBool());
	m_rg_entries = guiSettings->Var2List(guiSettings->GetValue(GUI::rg_entries));

	// clear recent games menu of actions
	for (auto act : m_recentGameActs)
	{
		ui->bootRecentMenu->removeAction(act);
	}
	m_recentGameActs.clear();
	// Fill the recent games menu
	for (uint i = 0; i < m_rg_entries.count(); i++)
	{
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

	ui->showLogAct->setChecked(guiSettings->GetValue(GUI::mw_logger).toBool());
	ui->showGameListAct->setChecked(guiSettings->GetValue(GUI::mw_gamelist).toBool());
	ui->showDebuggerAct->setChecked(guiSettings->GetValue(GUI::mw_debugger).toBool());
	ui->showToolBarAct->setChecked(guiSettings->GetValue(GUI::mw_toolBarVisible).toBool());
	ui->showGameToolBarAct->setChecked(guiSettings->GetValue(GUI::gl_toolBarVisible).toBool());

	debuggerFrame->setVisible(ui->showDebuggerAct->isChecked());
	logFrame->setVisible(ui->showLogAct->isChecked());
	gameListFrame->setVisible(ui->showGameListAct->isChecked());
	gameListFrame->SetToolBarVisible(ui->showGameToolBarAct->isChecked());
	ui->toolBar->setVisible(ui->showToolBarAct->isChecked());

	QColor tbc = guiSettings->GetValue(GUI::mw_toolBarColor).value<QColor>();
	ui->toolBar->setStyleSheet(QString(
		"QToolBar { background-color: rgba(%1, %2, %3, %4); }"
		"QToolBar::separator {background-color: rgba(%5, %6, %7, %8); width: 1px; margin-top: 2px; margin-bottom: 2px;}"
		"QSlider { background-color: rgba(%1, %2, %3, %4); }"
		"QLineEdit { background-color: rgba(%1, %2, %3, %4); }")
		.arg(tbc.red()).arg(tbc.green()).arg(tbc.blue()).arg(tbc.alpha())
		.arg(tbc.red() - 20).arg(tbc.green() - 20).arg(tbc.blue() - 20).arg(tbc.alpha() - 20));

	ui->showCatHDDGameAct->setChecked(guiSettings->GetCategoryVisibility(Category::Non_Disc_Game));
	ui->showCatDiscGameAct->setChecked(guiSettings->GetCategoryVisibility(Category::Disc_Game));
	ui->showCatHomeAct->setChecked(guiSettings->GetCategoryVisibility(Category::Home));
	ui->showCatAudioVideoAct->setChecked(guiSettings->GetCategoryVisibility(Category::Media));
	ui->showCatGameDataAct->setChecked(guiSettings->GetCategoryVisibility(Category::Data));
	ui->showCatUnknownAct->setChecked(guiSettings->GetCategoryVisibility(Category::Unknown_Cat));
	ui->showCatOtherAct->setChecked(guiSettings->GetCategoryVisibility(Category::Others));

	QString key = guiSettings->GetValue(GUI::gl_iconSize).toString();
	if (key == GUI::gl_icon_key_large) ui->setIconSizeLargeAct->setChecked(true);
	else if (key == GUI::gl_icon_key_medium) ui->setIconSizeMediumAct->setChecked(true);
	else if (key == GUI::gl_icon_key_small) ui->setIconSizeSmallAct->setChecked(true);
	else ui->setIconSizeTinyAct->setChecked(true);

	bool isListMode = guiSettings->GetValue(GUI::gl_listMode).toBool();
	if (isListMode) ui->setlistModeListAct->setChecked(true);
	else ui->setlistModeGridAct->setChecked(true);
	categoryVisibleActGroup->setEnabled(isListMode);

	if (configureAll)
	{
		// Handle log settings
		logFrame->LoadSettings();

		// Gamelist
		gameListFrame->LoadSettings();
	}
}

void main_window::keyPressEvent(QKeyEvent *keyEvent)
{
	if ((keyEvent->modifiers() & Qt::AltModifier) && keyEvent->key() == Qt::Key_Return || isFullScreen() && keyEvent->key() == Qt::Key_Escape)
	{
		ui->toolbar_fullscreen->trigger();
	}

	if (keyEvent->modifiers() & Qt::ControlModifier)
	{
		switch (keyEvent->key())
		{
		case Qt::Key_E: if (Emu.IsPaused()) Emu.Resume(); else if (Emu.IsReady()) Emu.Run(); return;
		case Qt::Key_P: if (Emu.IsRunning()) Emu.Pause(); return;
		case Qt::Key_S: if (!Emu.IsStopped()) Emu.Stop(); return;
		case Qt::Key_R: if (!Emu.GetPath().empty()) { Emu.Stop(); Emu.Run(); } return;
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
			ui->toolbar_fullscreen->setIcon(icon_fullscreen_on);
		}
	}
}

/** Override the Qt close event to have the emulator stop and the application die.  May add a warning dialog in future. 
*/
void main_window::closeEvent(QCloseEvent* closeEvent)
{
	Q_UNUSED(closeEvent);

	// Cleanly stop the emulator.
	Emu.Stop();

	SaveWindowState();

	// I need the gui settings to sync, and that means having the destructor called as guiSetting's parent is main_window.
	setAttribute(Qt::WA_DeleteOnClose);
	QMainWindow::close();

	// It's possible to have other windows open, like games.  So, force the application to die.
	QApplication::quit();
}
