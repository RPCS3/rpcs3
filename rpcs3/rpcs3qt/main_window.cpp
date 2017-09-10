
#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QProgressDialog>
#include <QDesktopWidget>

#include "vfs_dialog.h"
#include "save_manager_dialog.h"
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
#include "gamepads_settings_dialog.h"

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
	delete ui;
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

	m_appIcon = QIcon(":/rpcs3.ico");

	// hide utilities from the average user
	ui->menuUtilities->menuAction()->setVisible(guiSettings->GetValue(GUI::m_showDebugTab).toBool());

	// add toolbar widgets (crappy Qt designer is not able to)
	ui->toolBar->setObjectName("mw_toolbar");
	ui->sizeSlider->setRange(0, GUI::gl_max_slider_pos);
	ui->sizeSlider->setSliderPosition(guiSettings->GetValue(GUI::gl_iconSize).toInt());
	ui->toolBar->addWidget(ui->sizeSliderContainer);
	ui->toolBar->addSeparator();
	ui->toolBar->addWidget(ui->mw_searchbar);

	// for highdpi resize toolbar icons and height dynamically
	// choose factors to mimic Gui-Design in main_window.ui
	// TODO: in case Qt::AA_EnableHighDpiScaling is enabled in main.cpp we only need the else branch
#ifdef _WIN32
	const int toolBarHeight = menuBar()->sizeHint().height() * 1.5;
	ui->toolBar->setIconSize(QSize(toolBarHeight, toolBarHeight));
#else
	const int toolBarHeight = ui->toolBar->iconSize().height();
#endif
	ui->sizeSliderContainer->setFixedWidth(toolBarHeight * 5);
	ui->sizeSlider->setFixedHeight(toolBarHeight * 0.65f);

	CreateActions();
	CreateDockWindows();

	setMinimumSize(350, minimumSizeHint().height());    // seems fine on win 10

	CreateConnects();

	setWindowTitle(QString::fromStdString("RPCS3 v" + rpcs3::version.to_string()));
	!m_appIcon.isNull() ? setWindowIcon(m_appIcon) : LOG_WARNING(GENERAL, "AppImage could not be loaded!");

	Q_EMIT RequestGlobalStylesheetChange(guiSettings->GetCurrentStylesheetPath());
	ConfigureGuiFromSettings(true);
	RepaintToolBarIcons();
	m_gameListFrame->RepaintToolBarIcons();
	
	if (!utils::has_ssse3())
	{
		QMessageBox::critical(this, "SSSE3 Error (with three S, not two)",
			"Your system does not meet the minimum requirements needed to run RPCS3.\n"
			"Your CPU does not support SSSE3 (with three S, not two).\n");

		std::exit(EXIT_FAILURE);
	}

#ifdef BRANCH
	if ("RPCS3/rpcs3/master"s != STRINGIZE(BRANCH))
#elif _MSC_VER
	fs::stat_t st;
	if (!fs::stat(fs::get_config_dir() + "rpcs3.pdb", st) || st.is_directory || st.size < 1024 * 1024 * 100)
#else
	if (false)
#endif
	{
		LOG_WARNING(GENERAL, "Experimental Build Warning! Build origin: " STRINGIZE(BRANCH));

		QMessageBox msg;
		msg.setWindowTitle("Experimental Build Warning");
		msg.setWindowIcon(m_appIcon);
		msg.setIcon(QMessageBox::Critical);
		msg.setTextFormat(Qt::RichText);
		msg.setText("Please understand that this build is not an official RPCS3 release.<br>This build contains changes that may break games, or even <b>damage</b> your data.<br>It's recommended to download and use the official build from <a href='https://rpcs3.net/download'>RPCS3 website</a>.<br><br>Build origin: " STRINGIZE(BRANCH) "<br>Do you wish to use this build anyway?");
		msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msg.setDefaultButton(QMessageBox::No);

		if (msg.exec() == QMessageBox::No)
		{
			std::exit(EXIT_SUCCESS);
		}
	}
}

void main_window::CreateThumbnailToolbar()
{
#ifdef _WIN32
	m_icon_thumb_play = QIcon(":/Icons/play_blue.png");
	m_icon_thumb_pause = QIcon(":/Icons/pause_blue.png");
	m_icon_thumb_stop = QIcon(":/Icons/stop_blue.png");
	m_icon_thumb_restart = QIcon(":/Icons/restart_blue.png");

	m_thumb_bar = new QWinThumbnailToolBar(this);
	m_thumb_bar->setWindow(windowHandle());

	m_thumb_playPause = new QWinThumbnailToolButton(m_thumb_bar);
	m_thumb_playPause->setToolTip(tr("Pause"));
	m_thumb_playPause->setIcon(m_icon_thumb_pause);
	m_thumb_playPause->setEnabled(false);

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

	connect(m_thumb_stop, &QWinThumbnailToolButton::clicked, [=]() { Emu.Stop(); });
	connect(m_thumb_restart, &QWinThumbnailToolButton::clicked, [=]() { Emu.Stop();	Emu.Load();	});
	connect(m_thumb_playPause, &QWinThumbnailToolButton::clicked, Pause);
#endif
}

// returns appIcon
QIcon main_window::GetAppIcon()
{
	return m_appIcon;
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
				m_appIcon = QIcon(QPixmap::fromImage(dest));
				return;
			}
		}
	}
	// if nothing was found reset the icon to default
	m_appIcon = QIcon(":/rpcs3.ico");
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
		AddRecentAction(GUI::Recent_Game(qstr(Emu.GetBoot()), qstr(serial + Emu.GetTitle())));
		m_gameListFrame->Refresh(true);
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
		AddRecentAction(GUI::Recent_Game(qstr(Emu.GetBoot()), qstr(serial + Emu.GetTitle())));
		m_gameListFrame->Refresh(true);
	}
}

void main_window::InstallPkg(const QString& dropPath)
{
	QString filePath = dropPath;

	if (filePath.isEmpty())
	{
		QString path_last_PKG = guiSettings->GetValue(GUI::fd_install_pkg).toString();
		filePath = QFileDialog::getOpenFileName(this, tr("Select PKG To Install"), path_last_PKG, tr("PKG files (*.pkg);;All files (*.*)"));
	}
	else
	{
		if (QMessageBox::question(this, tr("PKG Decrypter / Installer"), tr("Install package: %1?").arg(filePath),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		{
			LOG_NOTICE(LOADER, "PKG: Cancelled installation from drop. File: %s", sstr(filePath));
			return;
		}
	}

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

	//Check header
	u32 pkg_signature;
	pkg_f.seek(0);
	pkg_f.read(pkg_signature);
	if (pkg_signature != "\x7FPKG"_u32)
	{
		LOG_ERROR(LOADER, "PKG: %s is not a pkg file", fileName);
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
			if (pkg_install(pkg_f, local_path + '/', progress, path))
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
					m_gameListFrame->Refresh(true);
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
		m_gameListFrame->Refresh(true);
		LOG_SUCCESS(GENERAL, "Successfully installed %s.", fileName);
		guiSettings->ShowInfoBox(GUI::ib_pkg_success, tr("Success!"), tr("Successfully installed software from package!"), this);

#ifdef _WIN32
		taskbar_progress->hide();
		taskbar_button->~QWinTaskbarButton();
#endif
	}
}

void main_window::InstallPup(const QString& dropPath)
{
	QString filePath = dropPath;

	if (filePath.isEmpty())
	{
		QString path_last_PUP = guiSettings->GetValue(GUI::fd_install_pup).toString();
		filePath = QFileDialog::getOpenFileName(this, tr("Select PS3UPDAT.PUP To Install"), path_last_PUP, tr("PS3 update file (PS3UPDAT.PUP)"));
	}
	else
	{
		if (QMessageBox::question(this, tr("RPCS3 Firmware Installer"), tr("Install firmware: %1?").arg(filePath),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		{
			LOG_NOTICE(LOADER, "Firmware: Cancelled installation from drop. File: %s", sstr(filePath));
			return;
		}
	}

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
				if (dev_flash_tar_f.size() < 3)
				{
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
	m_gameListFrame->SaveSettings();
	// Save splitter state
	m_debuggerFrame->SaveSettings();
}

void main_window::RepaintToolBarIcons()
{
	QColor newColor;

	if (guiSettings->GetValue(GUI::m_enableUIColors).toBool())
	{
		newColor = guiSettings->GetValue(GUI::mw_toolIconColor).value<QColor>();
	}
	else
	{
		newColor = GUI::get_Label_Color("toolbar_icon_color");
	}

	auto icon = [&newColor](const QString& path)
	{
		return gui_settings::colorizedIcon(QIcon(path), GUI::mw_tool_icon_color, newColor);
	};

	m_icon_play           = icon(":/Icons/play.png");
	m_icon_pause          = icon(":/Icons/pause.png");
	m_icon_stop           = icon(":/Icons/stop.png");
	m_icon_restart        = icon(":/Icons/restart.png");
	m_icon_fullscreen_on  = icon(":/Icons/fullscreen.png");
	m_icon_fullscreen_off = icon(":/Icons/fullscreen_invert.png");

	ui->toolbar_config  ->setIcon(icon(":/Icons/configure.png"));
	ui->toolbar_controls->setIcon(icon(":/Icons/controllers.png"));
	ui->toolbar_disc    ->setIcon(icon(":/Icons/disc.png"));
	ui->toolbar_grid    ->setIcon(icon(":/Icons/grid.png"));
	ui->toolbar_list    ->setIcon(icon(":/Icons/list.png"));
	ui->toolbar_refresh ->setIcon(icon(":/Icons/refresh.png"));
	ui->toolbar_snap    ->setIcon(icon(":/Icons/screenshot.png"));
	ui->toolbar_sort    ->setIcon(icon(":/Icons/sort.png"));
	ui->toolbar_stop    ->setIcon(icon(":/Icons/stop.png"));
	
	if (Emu.IsRunning())
	{
		ui->toolbar_start->setIcon(m_icon_pause);
	}
	else if (Emu.IsStopped() && !Emu.GetPath().empty())
	{
		ui->toolbar_start->setIcon(m_icon_restart);
	}
	else
	{
		ui->toolbar_start->setIcon(m_icon_play);
	}
	
	if (isFullScreen())
	{
		ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_on);
	}
	else
	{
		ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_off);
	}

	ui->sizeSlider->setStyleSheet(ui->sizeSlider->styleSheet().append("QSlider::handle:horizontal{ background: rgba(%1, %2, %3, %4); }")
		.arg(newColor.red()).arg(newColor.green()).arg(newColor.blue()).arg(newColor.alpha()));
}

void main_window::OnEmuRun()
{
	m_debuggerFrame->EnableButtons(true);
#ifdef _WIN32
	m_thumb_playPause->setToolTip(tr("Pause emulation"));
	m_thumb_playPause->setIcon(m_icon_thumb_pause);
#endif
	ui->sysPauseAct->setText(tr("&Pause\tCtrl+P"));
	ui->sysPauseAct->setIcon(m_icon_pause);
	ui->toolbar_start->setIcon(m_icon_pause);
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
	ui->toolbar_start->setToolTip(tr("Resume emulation"));
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
	if (!Emu.GetPath().empty())
	{
		ui->toolbar_start->setEnabled(true);
		ui->toolbar_start->setIcon(m_icon_restart);
		ui->toolbar_start->setToolTip(tr("Restart emulation"));
		ui->sysRebootAct->setEnabled(true);
#ifdef _WIN32
		m_thumb_restart->setEnabled(true);
#endif
	}
	else
	{
		ui->toolbar_start->setIcon(m_icon_play);
		ui->toolbar_start->setToolTip(Emu.IsReady() ? tr("Start emulation") : tr("Resume emulation"));
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
	ui->toolbar_start->setToolTip(Emu.IsReady() ? tr("Start emulation") : tr("Resume emulation"));
	EnableMenus(true);
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

			guiSettings->SetValue(GUI::rg_entries, guiSettings->List2Var(m_rg_entries));

			LOG_ERROR(GENERAL, "Recent Game not valid, removed from Boot Recent list: %s", sstr(pth));

			// refill menu with actions
			for (int i = 0; i < m_recentGameActs.count(); i++)
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
		AddRecentAction(GUI::Recent_Game(qstr(Emu.GetBoot()), nam));
		m_gameListFrame->Refresh(true);
	}
};

QAction* main_window::CreateRecentAction(const q_string_pair& entry, const uint& sc_idx)
{
	// if path is not valid remove from list
	if (entry.second.isEmpty() || (!QFileInfo(entry.first).isDir() && !QFileInfo(entry.first).isFile()))
	{
		if (m_rg_entries.contains(entry))
		{
			LOG_ERROR(GENERAL, "Recent Game not valid, removing from Boot Recent list: %s", sstr(entry.first));

			int idx = m_rg_entries.indexOf(entry);
			m_rg_entries.removeAt(idx);

			guiSettings->SetValue(GUI::rg_entries, guiSettings->List2Var(m_rg_entries));
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
	for (int i = 0; i < m_recentGameActs.count(); i++)
	{
		m_recentGameActs[i]->setShortcut(tr("Ctrl+%1").arg(i+1));
		m_recentGameActs[i]->setToolTip(m_rg_entries.at(i).second);
		ui->bootRecentMenu->addAction(m_recentGameActs[i]);
	}

	guiSettings->SetValue(GUI::rg_entries, guiSettings->List2Var(m_rg_entries));
}

void main_window::RepaintGui()
{
	m_gameListFrame->RepaintIcons(true);
	m_gameListFrame->RepaintToolBarIcons();
	RepaintToolbar();
	RepaintToolBarIcons();
}

void main_window::RepaintToolbar()
{
	if (guiSettings->GetValue(GUI::m_enableUIColors).toBool())
	{
		QColor tbc = guiSettings->GetValue(GUI::mw_toolBarColor).value<QColor>();

		ui->toolBar->setStyleSheet(GUI::stylesheet + QString(
			"QToolBar { background-color: rgba(%1, %2, %3, %4); }"
			"QToolBar::separator {background-color: rgba(%5, %6, %7, %8); width: 1px; margin-top: 2px; margin-bottom: 2px;}"
			"QSlider { background-color: rgba(%1, %2, %3, %4); }"
			"QLineEdit { background-color: rgba(%1, %2, %3, %4); }")
			.arg(tbc.red()).arg(tbc.green()).arg(tbc.blue()).arg(tbc.alpha())
			.arg(tbc.red() - 20).arg(tbc.green() - 20).arg(tbc.blue() - 20).arg(tbc.alpha() - 20)
		);
	}
	else
	{
		ui->toolBar->setStyleSheet(GUI::stylesheet);
	}
}

void main_window::CreateActions()
{
	ui->exitAct->setShortcuts(QKeySequence::Quit);

	ui->toolbar_start->setEnabled(false);
	ui->toolbar_stop->setEnabled(false);

	m_categoryVisibleActGroup = new QActionGroup(this); 
	m_categoryVisibleActGroup->addAction(ui->showCatHDDGameAct);
	m_categoryVisibleActGroup->addAction(ui->showCatDiscGameAct);
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
		guiSettings->SetValue(GUI::rg_entries, guiSettings->List2Var(q_pair_list()));
	});

	connect(ui->freezeRecentAct, &QAction::triggered, [=](bool checked)
	{
		guiSettings->SetValue(GUI::rg_freeze, checked);
	});

	connect(ui->bootInstallPkgAct, &QAction::triggered, [this] {InstallPkg(); });
	connect(ui->bootInstallPupAct, &QAction::triggered, [this] {InstallPup(); });
	connect(ui->exitAct, &QAction::triggered, this, &QWidget::close);
	connect(ui->sysPauseAct, &QAction::triggered, Pause);
	connect(ui->sysStopAct, &QAction::triggered, [=]() { Emu.Stop(); });
	connect(ui->sysRebootAct, &QAction::triggered, [=]() { Emu.Stop();	Emu.Load();	});

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
		settings_dialog dlg(guiSettings, m_Render_Creator, tabIndex, this);
		connect(&dlg, &settings_dialog::GuiSettingsSaveRequest, this, &main_window::SaveWindowState);
		connect(&dlg, &settings_dialog::GuiSettingsSyncRequest, [=]() {ConfigureGuiFromSettings(true); });
		connect(&dlg, &settings_dialog::GuiStylesheetRequest, this, &main_window::RequestGlobalStylesheetChange);
		connect(&dlg, &settings_dialog::GuiRepaintRequest, this, &main_window::RepaintGui);
		dlg.exec();
	};

	connect(ui->confCPUAct,    &QAction::triggered, [=]() { openSettings(0); });
	connect(ui->confGPUAct,    &QAction::triggered, [=]() { openSettings(1); });
	connect(ui->confAudioAct,  &QAction::triggered, [=]() { openSettings(2); });
	connect(ui->confIOAct,     &QAction::triggered, [=]() { openSettings(3); });
	connect(ui->confSystemAct, &QAction::triggered, [=]() { openSettings(4); });

	connect(ui->confPadsAct, &QAction::triggered, this, [=]
	{
		gamepads_settings_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->confAutopauseManagerAct, &QAction::triggered, [=]
	{
		auto_pause_settings_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->confVFSDialogAct, &QAction::triggered, [=]
	{
		vfs_dialog dlg(this);
		dlg.exec();
		m_gameListFrame->Refresh(true); // dev-hdd0 may have changed. Refresh just in case.
	});

	connect(ui->confSavedataManagerAct, &QAction::triggered, [=]
	{
		save_manager_dialog* sdid = new save_manager_dialog();
		sdid->show();
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
		rsx_debugger* rsx = new rsx_debugger(this);
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
		guiSettings->SetValue(GUI::mw_debugger, checked);
	});

	connect(ui->showLogAct, &QAction::triggered, [=](bool checked)
	{
		checked ? m_logFrame->show() : m_logFrame->hide();
		guiSettings->SetValue(GUI::mw_logger, checked);
	});

	connect(ui->showGameListAct, &QAction::triggered, [=](bool checked)
	{
		checked ? m_gameListFrame->show() : m_gameListFrame->hide();
		guiSettings->SetValue(GUI::mw_gamelist, checked);
	});

	connect(ui->showToolBarAct, &QAction::triggered, [=](bool checked)
	{
		ui->toolBar->setVisible(checked);
		guiSettings->SetValue(GUI::mw_toolBarVisible, checked);
	});

	connect(ui->showGameToolBarAct, &QAction::triggered, [=](bool checked)
	{
		m_gameListFrame->SetToolBarVisible(checked);
	});

	connect(ui->refreshGameListAct, &QAction::triggered, [=]
	{
		m_gameListFrame->Refresh(true);
	});

	connect(m_categoryVisibleActGroup, &QActionGroup::triggered, [=](QAction* act)
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

		m_gameListFrame->SetCategoryActIcon(m_categoryVisibleActGroup->actions().indexOf(act), checked);
		m_gameListFrame->ToggleCategoryFilter(categories, checked);
		guiSettings->SetCategoryVisibility(id, checked);
	});

	connect(ui->aboutAct, &QAction::triggered, [this]
	{
		about_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

	auto resizeIcons = [=](const int& index)
	{
		int val = ui->sizeSlider->value();
		if (val != index)
		{
			ui->sizeSlider->setSliderPosition(index);
		}
		if (val != m_gameListFrame->GetSliderValue())
		{
			if (m_save_slider_pos)
			{
				m_save_slider_pos = false;
				guiSettings->SetValue(GUI::gl_iconSize, index);
			}
			m_gameListFrame->ResizeIcons(index);
		}
	};

	connect(m_iconSizeActGroup, &QActionGroup::triggered, [=](QAction* act)
	{
		int index;

		if (act == ui->setIconSizeTinyAct) index = 0;
		else if (act == ui->setIconSizeSmallAct) index = GUI::get_Index(GUI::gl_icon_size_small);
		else if (act == ui->setIconSizeMediumAct) index = GUI::get_Index(GUI::gl_icon_size_medium);
		else index = GUI::gl_max_slider_pos;

		resizeIcons(index);
	});

	connect (m_gameListFrame, &game_list_frame::RequestIconSizeActSet, [=](const int& idx)
	{
		if (idx < GUI::get_Index((GUI::gl_icon_size_small + GUI::gl_icon_size_min) / 2)) ui->setIconSizeTinyAct->setChecked(true);
		else if (idx < GUI::get_Index((GUI::gl_icon_size_medium + GUI::gl_icon_size_small) / 2)) ui->setIconSizeSmallAct->setChecked(true);
		else if (idx < GUI::get_Index((GUI::gl_icon_size_max + GUI::gl_icon_size_medium) / 2)) ui->setIconSizeMediumAct->setChecked(true);
		else ui->setIconSizeLargeAct->setChecked(true);

		resizeIcons(idx);
	});

	connect(m_gameListFrame, &game_list_frame::RequestSaveSliderPos, [=](const bool& save)
	{
		Q_UNUSED(save);
		m_save_slider_pos = true;
	});

	connect(m_gameListFrame, &game_list_frame::RequestListModeActSet, [=](const bool& isList)
	{
		isList ? ui->setlistModeListAct->trigger() : ui->setlistModeGridAct->trigger();
	});

	connect(m_gameListFrame, &game_list_frame::RequestCategoryActSet, [=](const int& id)
	{
		m_categoryVisibleActGroup->actions().at(id)->trigger();
	});

	connect(m_listModeActGroup, &QActionGroup::triggered, [=](QAction* act)
	{
		bool isList = act == ui->setlistModeListAct;
		m_gameListFrame->SetListMode(isList);
		m_categoryVisibleActGroup->setEnabled(isList);
	});

	connect(ui->toolbar_disc, &QAction::triggered, this, &main_window::BootGame);
	connect(ui->toolbar_refresh, &QAction::triggered, [=]() { m_gameListFrame->Refresh(true); });
	connect(ui->toolbar_stop, &QAction::triggered, [=]() { Emu.Stop(); });
	connect(ui->toolbar_start, &QAction::triggered, Pause);

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

	connect(ui->toolbar_controls, &QAction::triggered, [=]() { gamepads_settings_dialog dlg(this); dlg.exec(); });
	connect(ui->toolbar_config, &QAction::triggered, [=]() { openSettings(0); });
	connect(ui->toolbar_list, &QAction::triggered, [=]() { ui->setlistModeListAct->trigger(); });
	connect(ui->toolbar_grid, &QAction::triggered, [=]() { ui->setlistModeGridAct->trigger(); });
	connect(ui->sizeSlider, &QSlider::valueChanged, resizeIcons);
	connect(ui->sizeSlider, &QSlider::sliderReleased, this, [&] { guiSettings->SetValue(GUI::gl_iconSize, ui->sizeSlider->value()); });

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

	m_gameListFrame = new game_list_frame(guiSettings, m_Render_Creator, m_mw);
	m_gameListFrame->setObjectName("gamelist");
	m_debuggerFrame = new debugger_frame(guiSettings, m_mw);
	m_debuggerFrame->setObjectName("debugger");
	m_logFrame = new log_frame(guiSettings, m_mw);
	m_logFrame->setObjectName("logger");

	m_mw->addDockWidget(Qt::LeftDockWidgetArea, m_gameListFrame);
	m_mw->addDockWidget(Qt::LeftDockWidgetArea, m_logFrame);
	m_mw->addDockWidget(Qt::RightDockWidgetArea, m_debuggerFrame);
	m_mw->setDockNestingEnabled(true);
	setCentralWidget(m_mw);

	connect(m_logFrame, &log_frame::LogFrameClosed, [=]()
	{
		if (ui->showLogAct->isChecked())
		{
			ui->showLogAct->setChecked(false);
			guiSettings->SetValue(GUI::mw_logger, false);
		}
	});

	connect(m_debuggerFrame, &debugger_frame::DebugFrameClosed, [=]()
	{
		if (ui->showDebuggerAct->isChecked())
		{
			ui->showDebuggerAct->setChecked(false);
			guiSettings->SetValue(GUI::mw_debugger, false);
		}
	});

	connect(m_gameListFrame, &game_list_frame::GameListFrameClosed, [=]()
	{
		if (ui->showGameListAct->isChecked())
		{
			ui->showGameListAct->setChecked(false);
			guiSettings->SetValue(GUI::mw_gamelist, false);
		}
	});

	connect(m_gameListFrame, &game_list_frame::RequestIconPathSet, this, &main_window::SetAppIconFromPath);
	connect(m_gameListFrame, &game_list_frame::RequestAddRecentGame, this, &main_window::AddRecentAction);

	connect(m_gameListFrame, &game_list_frame::RequestPackageInstall, [this](const QStringList& paths)
	{
		for (const auto& path : paths)
		{
			InstallPkg(path);
		}
	});

	connect(m_gameListFrame, &game_list_frame::RequestFirmwareInstall, this, &main_window::InstallPup);
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
		m_debuggerFrame->hide();

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
	for (int i = 0; i < m_rg_entries.count(); i++)
	{
		// adjust old unformatted entries (avoid duplication)
		m_rg_entries[i] = GUI::Recent_Game(m_rg_entries[i].first, m_rg_entries[i].second);

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

	m_debuggerFrame->setVisible(ui->showDebuggerAct->isChecked());
	m_logFrame->setVisible(ui->showLogAct->isChecked());
	m_gameListFrame->setVisible(ui->showGameListAct->isChecked());
	m_gameListFrame->SetToolBarVisible(ui->showGameToolBarAct->isChecked());
	ui->toolBar->setVisible(ui->showToolBarAct->isChecked());

	RepaintToolbar();

	ui->showCatHDDGameAct->setChecked(guiSettings->GetCategoryVisibility(Category::Non_Disc_Game));
	ui->showCatDiscGameAct->setChecked(guiSettings->GetCategoryVisibility(Category::Disc_Game));
	ui->showCatHomeAct->setChecked(guiSettings->GetCategoryVisibility(Category::Home));
	ui->showCatAudioVideoAct->setChecked(guiSettings->GetCategoryVisibility(Category::Media));
	ui->showCatGameDataAct->setChecked(guiSettings->GetCategoryVisibility(Category::Data));
	ui->showCatUnknownAct->setChecked(guiSettings->GetCategoryVisibility(Category::Unknown_Cat));
	ui->showCatOtherAct->setChecked(guiSettings->GetCategoryVisibility(Category::Others));

	int idx = guiSettings->GetValue(GUI::gl_iconSize).toInt();
	int index = GUI::gl_max_slider_pos / 4;
	if (idx < index) ui->setIconSizeTinyAct->setChecked(true);
	else if (idx < index * 2) ui->setIconSizeSmallAct->setChecked(true);
	else if (idx < index * 3) ui->setIconSizeMediumAct->setChecked(true);
	else ui->setIconSizeLargeAct->setChecked(true);

	bool isListMode = guiSettings->GetValue(GUI::gl_listMode).toBool();
	if (isListMode) ui->setlistModeListAct->setChecked(true);
	else ui->setlistModeGridAct->setChecked(true);
	m_categoryVisibleActGroup->setEnabled(isListMode);

	if (configureAll)
	{
		// Handle log settings
		m_logFrame->LoadSettings();

		// Gamelist
		m_gameListFrame->LoadSettings();
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
			ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_on);
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
