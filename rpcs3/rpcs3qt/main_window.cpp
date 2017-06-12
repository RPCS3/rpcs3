
#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QProgressDialog>
#include <QDesktopWidget>
#include <QDesktopServices>

#include "save_data_utility.h"
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

inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

main_window::main_window(QWidget *parent) : QMainWindow(parent), m_sys_menu_opened(false)
{
	guiSettings.reset(new gui_settings());

	setDockNestingEnabled(true);

	// Get Render Adapters
	m_Render_Creator = Render_Creator();

	//Load Icons: This needs to happen before any actions or buttons are created
	icon_play = QIcon(":/Icons/play.png");
	icon_pause = QIcon(":/Icons/pause.png");
	icon_stop = QIcon(":/Icons/stop.png");
	icon_restart = QIcon(":/Icons/restart.png");
	appIcon = QIcon(":/rpcs3.ico");

	CreateActions();
	CreateMenus();
	CreateDockWindows();

	setMinimumSize(350, minimumSizeHint().height());    // seems fine on win 10

	ConfigureGuiFromSettings(true);
	CreateConnects();

	setWindowTitle(QString::fromStdString("RPCS3 v" + rpcs3::version.to_string()));
	!appIcon.isNull() ? setWindowIcon(appIcon) : LOG_WARNING(GENERAL, "AppImage could not be loaded!");

	QTimer::singleShot(1, [=]() {
		// Need to have this happen fast, but not now because connects aren't created yet.
		// So, a tricky balance in terms of time but this works.
		emit RequestGlobalStylesheetChange(guiSettings->GetCurrentStylesheetPath()); 
	});
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

void main_window::CreateThumbnailToolbar()
{
#ifdef _WIN32
	thumb_bar = new QWinThumbnailToolBar(this);
	thumb_bar->setWindow(windowHandle());

	thumb_playPause = new QWinThumbnailToolButton(thumb_bar);
	thumb_playPause->setToolTip(tr("Start"));
	thumb_playPause->setIcon(icon_play);
	thumb_playPause->setEnabled(false);

	thumb_stop = new QWinThumbnailToolButton(thumb_bar);
	thumb_stop->setToolTip(tr("Stop"));
	thumb_stop->setIcon(icon_stop);
	thumb_stop->setEnabled(false);

	thumb_restart = new QWinThumbnailToolButton(thumb_bar);
	thumb_restart->setToolTip(tr("Restart"));
	thumb_restart->setIcon(icon_restart);
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
	std::string path_list[] = { path, sstr(qpath.section("/", 0, -2)) ,sstr(qpath.section("/", 0, -3)) };
	for (std::string pth : path_list)
	{
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
		AddRecentAction(q_string_pair(qstr(path), qstr(serial + Emu.GetTitle())));
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
		AddRecentAction(q_string_pair(qstr(path), qstr(serial + Emu.GetTitle())));
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
	const auto& local_path = Emu.GetGameDir() + std::string(std::begin(title_id), std::end(title_id));

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

	QProgressDialog pdlg(tr("Installing firmware ... please wait ..."), tr("Cancel"), 0, static_cast<int>(updatefilenames.size()), this);
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
		LOG_SUCCESS(GENERAL, "Successfully installed PS3 firmware.");
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

void main_window::About()
{
	QDialog* about = new QDialog(this);

	QPushButton* gitHub = new QPushButton(tr("GitHub"), about);
	QPushButton* website = new QPushButton(tr("Website"), about);
	QPushButton* forum = new QPushButton(tr("Forum"), about);
	QPushButton* patreon = new QPushButton(tr("Patreon"), about);
	QPushButton* close = new QPushButton(tr("Close"), about);
	close->setDefault(true);

	QLabel* icon = new QLabel(this);
	icon->setPixmap(QIcon(":/rpcs3.ico").pixmap(96, 96));

	QLabel* caption = new QLabel(tr(
		"<h1>RPCS3</h1>"
		"A PlayStation 3 emulator and debugger.<br>"
		"RPCS3 Version: %1").arg(qstr(rpcs3::version.to_string())
		));
	QLabel* developers = new QLabel(tr(
		"<p><b>Developers:</b><br>¬DH<br>¬AlexAltea<br>¬Hykem<br>Oil<br>Nekotekina<br>Bigpet<br>¬gopalsr83<br>¬tambry<br>"
		"vlj<br>kd-11<br>jarveson<br>raven02<br>AniLeo<br>cornytrace<br>ssshadow<br>Numan</p>"
	));
	QLabel* contributors = new QLabel(tr(
		"<p><b>Contributors:</b><br>BlackDaemon<br>elisha464<br>Aishou<br>krofna<br>xsacha<br>danilaml<br>unknownbrackets<br>Zangetsu38<br>"
		"lioncash<br>achurch<br>darkf<br>Syphurith<br>Blaypeg<br>Survanium90<br>georgemoralis<br>ikki84<br>hcorion<br>Megamouse<br>flash-fire</p>"
	));
	QLabel* supporters = new QLabel(tr(
		"<p><b>Supporters:</b><br>Howard Garrison<br>EXPotemkin<br>Marko V.<br>danhp<br>Jake (5315825)<br>Ian Reid<br>Tad Sherlock<br>Tyler Friesen<br>"
		"Folzar<br>Payton Williams<br>RedPill Australia<br>yanghong<br>Mohammed El-Serougi<br>Дима ~Ximer13~ Кулин<br>James Reed<br>BaroqueSonata</p>"
	));
	icon->setAlignment(Qt::AlignLeft);
	caption->setAlignment(Qt::AlignLeft);
	developers->setAlignment(Qt::AlignTop);
	contributors->setAlignment(Qt::AlignTop);
	supporters->setAlignment(Qt::AlignTop);

	// Caption Layout
	QVBoxLayout* caption_layout = new QVBoxLayout();
	caption_layout->setAlignment(Qt::AlignLeft);
	caption_layout->addSpacing(15);
	caption_layout->addWidget(caption);

	// Header Section
	QHBoxLayout* header_layout = new QHBoxLayout();
	header_layout->setAlignment(Qt::AlignCenter);
	header_layout->addLayout(caption_layout);
	header_layout->addStretch();
	header_layout->addWidget(icon);
	header_layout->addStretch();

	// Names Section
	QHBoxLayout* text_layout = new QHBoxLayout();
	text_layout->setAlignment(Qt::AlignTop);
	text_layout->addWidget(developers);
	text_layout->addWidget(contributors);
	text_layout->addWidget(supporters);

	// Button Section
	QHBoxLayout* button_layout = new QHBoxLayout();
	button_layout->addWidget(gitHub);
	button_layout->addWidget(website);
	button_layout->addWidget(forum);
	button_layout->addWidget(patreon);
	button_layout->addStretch();
	button_layout->addWidget(close);

	// Main Layout
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(header_layout);
	layout->addLayout(text_layout);
	layout->addSpacing(20);
	layout->addLayout(button_layout);

	// Events
	connect(gitHub, &QAbstractButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://www.github.com/RPCS3")); });
	connect(website, &QAbstractButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://www.rpcs3.net")); });
	connect(forum, &QAbstractButton::clicked, [] { QDesktopServices::openUrl(QUrl("http://www.emunewz.net/forum/forumdisplay.php?fid=172")); });
	connect(patreon, &QAbstractButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://www.patreon.com/Nekotekina")); });
	connect(close, &QAbstractButton::clicked, about, &QWidget::close);

	// Create About Dialog
	about->setWindowTitle(tr("About RPCS3"));
	about->setWindowFlags(about->windowFlags() & ~Qt::WindowContextHelpButtonHint);
	about->setMinimumWidth(500);
	about->setLayout(layout);
	about->setFixedSize(about->sizeHint());
	about->exec();
}

/** Needed so that when a backup occurs of window state in guisettings, the state is current. 
* Also, so that on close, the window state is preserved.
*/
void main_window::SaveWindowState()
{
	// Save gui settings
	guiSettings->SetValue(GUI::mw_geometry, saveGeometry());
	guiSettings->SetValue(GUI::mw_windowState, saveState());

	// Save column settings
	gameListFrame->SaveSettings();
}

void main_window::OnEmuRun()
{
	debuggerFrame->EnableButtons(true);
#ifdef _WIN32
	thumb_playPause->setToolTip(tr("Pause"));
	thumb_playPause->setIcon(icon_pause);
#endif
	sysPauseAct->setText(tr("&Pause\tCtrl+P"));
	sysPauseAct->setIcon(icon_pause);
	menu_run->setText(tr("&Pause"));
	menu_run->setIcon(icon_pause);
	EnableMenus(true);
}

void main_window::OnEmuResume()
{
#ifdef _WIN32
	thumb_playPause->setToolTip(tr("Pause"));
	thumb_playPause->setIcon(icon_pause);
#endif
	sysPauseAct->setText(tr("&Pause\tCtrl+P"));
	sysPauseAct->setIcon(icon_pause);
	menu_run->setText(tr("&Pause"));
	menu_run->setIcon(icon_pause);
}

void main_window::OnEmuPause()
{
#ifdef _WIN32
	thumb_playPause->setToolTip(tr("Resume"));
	thumb_playPause->setIcon(icon_play);
#endif
	sysPauseAct->setText(tr("&Resume\tCtrl+E"));
	sysPauseAct->setIcon(icon_play);
	menu_run->setText(tr("&Resume"));
	menu_run->setIcon(icon_play);
}

void main_window::OnEmuStop()
{
	debuggerFrame->EnableButtons(false);
#ifdef _WIN32
	thumb_playPause->setToolTip(Emu.IsReady() ? tr("Start") : tr("Resume"));
	thumb_playPause->setIcon(icon_play);
#endif
	menu_run->setText(Emu.IsReady() ? tr("&Start") : tr("&Resume"));
	menu_run->setIcon(icon_play);
	EnableMenus(false);
	if (!Emu.GetPath().empty())
	{
		sysPauseAct->setText(tr("&Restart\tCtrl+E"));
		sysPauseAct->setIcon(icon_restart);
		sysPauseAct->setEnabled(true);
		menu_restart->setEnabled(true);
#ifdef _WIN32
		thumb_restart->setEnabled(true);
#endif
	}
	else
	{
		sysPauseAct->setText(Emu.IsReady() ? tr("&Start\tCtrl+E") : tr("&Resume\tCtrl+E"));
		sysPauseAct->setIcon(icon_play);
	}
}

void main_window::OnEmuReady()
{
#ifdef _WIN32
	thumb_playPause->setToolTip(Emu.IsReady() ? tr("Start") : tr("Resume"));
	thumb_playPause->setIcon(icon_play);
#endif
	sysPauseAct->setText(Emu.IsReady() ? tr("&Start\tCtrl+E") : tr("&Resume\tCtrl+E"));
	sysPauseAct->setIcon(icon_play);
	menu_run->setText(Emu.IsReady() ? tr("&Start") : tr("&Resume"));
	menu_run->setIcon(icon_play);
	EnableMenus(true);
}

extern bool user_asked_for_frame_capture;

void main_window::EnableMenus(bool enabled)
{
	// Thumbnail Buttons
#ifdef _WIN32
	thumb_playPause->setEnabled(enabled);
	thumb_stop->setEnabled(enabled);
	thumb_restart->setEnabled(enabled);
#endif

	// Buttons
	menu_run->setEnabled(enabled);
	menu_stop->setEnabled(enabled);
	menu_restart->setEnabled(enabled);

	// Emulation
	sysPauseAct->setEnabled(enabled);
	sysStopAct->setEnabled(enabled);

	// PS3 Commands
	sysSendOpenMenuAct->setEnabled(enabled);
	sysSendExitAct->setEnabled(enabled);

	// Tools
	toolskernel_explorerAct->setEnabled(enabled);
	toolsmemory_viewerAct->setEnabled(enabled);
	toolsRsxDebuggerAct->setEnabled(enabled);
	toolsStringSearchAct->setEnabled(enabled);
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
				m_bootRecentMenu->removeAction(act);
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
				m_bootRecentMenu->addAction(m_recentGameActs[i]);
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
		AddRecentAction(q_string_pair(pth, nam));
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
	if (freezeRecentAct->isChecked())
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
		m_bootRecentMenu->removeAction(act);
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
		m_bootRecentMenu->addAction(m_recentGameActs[i]);
	}

	guiSettings->SetValue(GUI::rg_entries, guiSettings->List2Var(m_rg_entries));
}

void main_window::CreateActions()
{
	bootElfAct = new QAction(tr("Boot (S)ELF file"), this);
	bootGameAct = new QAction(tr("Boot &Game"), this);
	bootInstallPkgAct = new QAction(tr("&Install PKG"), this);
	bootInstallPupAct = new QAction(tr("&Install Firmware"), this);

	clearRecentAct = new QAction(tr("&Clear List"), this);
	freezeRecentAct = new QAction(tr("&Freeze List"), this);
	freezeRecentAct->setCheckable(true);

	exitAct = new QAction(tr("E&xit"), this);
	exitAct->setShortcuts(QKeySequence::Quit);
	exitAct->setStatusTip(tr("Exit the application"));

	sysPauseAct = new QAction(tr("&Pause"), this);
	sysPauseAct->setEnabled(false);
	sysPauseAct->setIcon(icon_pause);

	sysStopAct = new QAction(tr("&Stop"), this);
	sysStopAct->setEnabled(false);
	sysStopAct->setIcon(icon_stop);

	sysSendOpenMenuAct = new QAction(tr("Send &open system menu cmd"), this);
	sysSendOpenMenuAct->setEnabled(false);

	sysSendExitAct = new QAction(tr("Send &exit cmd"), this);
	sysSendExitAct->setEnabled(false);

	confSettingsAct = new QAction(tr("&Settings"), this);
	confPadAct = new QAction(tr("&Keyboard Settings"), this);

	confAutopauseManagerAct = new QAction(tr("&Auto Pause Settings"), this);
	confAutopauseManagerAct->setEnabled(false);

	confSavedataManagerAct = new QAction(tr("Save &Data Utility"), this);
	confSavedataManagerAct->setEnabled(false);

	toolsCgDisasmAct = new QAction(tr("&Cg Disasm"), this);
	toolsCgDisasmAct->setEnabled(true);

	toolskernel_explorerAct = new QAction(tr("&Kernel Explorer"), this);
	toolskernel_explorerAct->setEnabled(false);

	toolsmemory_viewerAct = new QAction(tr("&Memory Viewer"), this);
	toolsmemory_viewerAct->setEnabled(false);

	toolsRsxDebuggerAct = new QAction(tr("&RSX Debugger"), this);
	toolsRsxDebuggerAct->setEnabled(false);

	toolsStringSearchAct = new QAction(tr("&String Search"), this);
	toolsStringSearchAct->setEnabled(false);

	toolsDecryptSprxLibsAct = new QAction(tr("SPRX &Decryption"), this);

	showDebuggerAct = new QAction(tr("Show Debugger"), this);
	showDebuggerAct->setCheckable(true);

	showLogAct = new QAction(tr("Show Log/TTY"), this);
	showLogAct->setCheckable(true);

	showGameListAct = new QAction(tr("Show Game List"), this);
	showGameListAct->setCheckable(true);

	showControlsAct = new QAction(tr("Show Controls"), this);
	showControlsAct->setCheckable(true);

	showGameListToolBarAct = new QAction(tr("Show Tool Bar"), this);
	showGameListToolBarAct->setCheckable(true);

	refreshGameListAct = new QAction(tr("&Refresh Game List"), this);

	showCatHDDGameAct = new QAction(category::hdd_Game, this);
	showCatHDDGameAct->setCheckable(true);

	showCatDiscGameAct = new QAction(category::disc_Game, this);
	showCatDiscGameAct->setCheckable(true);

	showCatHomeAct = new QAction(category::home, this);
	showCatHomeAct->setCheckable(true);

	showCatAudioVideoAct = new QAction(category::audio_Video, this);
	showCatAudioVideoAct->setCheckable(true);

	showCatGameDataAct = new QAction(category::game_Data, this);
	showCatGameDataAct->setCheckable(true);

	showCatUnknownAct = new QAction(category::unknown, this);
	showCatUnknownAct->setCheckable(true);

	categoryVisibleActGroup = new QActionGroup(this); 
	categoryVisibleActGroup->addAction(showCatHDDGameAct);
	categoryVisibleActGroup->addAction(showCatDiscGameAct);
	categoryVisibleActGroup->addAction(showCatHomeAct);
	categoryVisibleActGroup->addAction(showCatAudioVideoAct);
	categoryVisibleActGroup->addAction(showCatGameDataAct);
	categoryVisibleActGroup->addAction(showCatUnknownAct);
	categoryVisibleActGroup->setExclusive(false);

	setIconSizeTinyAct = new QAction(tr("Tiny"), this);
	setIconSizeTinyAct->setCheckable(true);

	setIconSizeSmallAct = new QAction(tr("Small"), this);
	setIconSizeSmallAct->setCheckable(true);

	setIconSizeMediumAct = new QAction(tr("Medium"), this);
	setIconSizeMediumAct->setCheckable(true);

	setIconSizeLargeAct = new QAction(tr("Large"), this);
	setIconSizeLargeAct->setCheckable(true);

	iconSizeActGroup = new QActionGroup(this);
	iconSizeActGroup->addAction(setIconSizeTinyAct);
	iconSizeActGroup->addAction(setIconSizeSmallAct);
	iconSizeActGroup->addAction(setIconSizeMediumAct);
	iconSizeActGroup->addAction(setIconSizeLargeAct);
	setIconSizeSmallAct->setChecked(true);

	setlistModeListAct = new QAction(tr("List"), this);
	setlistModeListAct->setCheckable(true);

	setlistModeGridAct = new QAction(tr("Grid"), this);
	setlistModeGridAct->setCheckable(true);

	listModeActGroup = new QActionGroup(this);
	listModeActGroup->addAction(setlistModeListAct);
	listModeActGroup->addAction(setlistModeGridAct);
	setlistModeListAct->setChecked(true);

	aboutAct = new QAction(tr("&About"), this);
	aboutAct->setStatusTip(tr("Show the application's About box"));

	aboutQtAct = new QAction(tr("About &Qt"), this);
	aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
}

void main_window::CreateConnects()
{
	connect(bootElfAct, &QAction::triggered, this, &main_window::BootElf);
	connect(bootGameAct, &QAction::triggered, this, &main_window::BootGame);
	connect(m_bootRecentMenu, &QMenu::aboutToShow, [=]() {
		// Enable/Disable Recent Games List
		const bool stopped = Emu.IsStopped();
		for (auto act : m_bootRecentMenu->actions())
		{
			if (act != freezeRecentAct && act != clearRecentAct)
			{
				act->setEnabled(stopped);
			}
		}
	});
	connect(clearRecentAct, &QAction::triggered, [this](){
		if (freezeRecentAct->isChecked()) { return; }
		m_rg_entries.clear();
		for (auto act : m_recentGameActs)
		{
			m_bootRecentMenu->removeAction(act);
		}
		m_recentGameActs.clear();
		guiSettings->SetValue(GUI::rg_entries, guiSettings->List2Var(q_pair_list()));
	});
	connect(freezeRecentAct, &QAction::triggered, [=](bool checked) {
		guiSettings->SetValue(GUI::rg_freeze, checked);
	});
	connect(bootInstallPkgAct, &QAction::triggered, this, &main_window::InstallPkg);
	connect(bootInstallPupAct, &QAction::triggered, this, &main_window::InstallPup);
	connect(exitAct, &QAction::triggered, this, &QWidget::close);
	connect(sysPauseAct, &QAction::triggered, Pause);
	connect(sysStopAct, &QAction::triggered, [=]() { Emu.Stop(); });
	connect(sysSendOpenMenuAct, &QAction::triggered, [=](){
		sysutil_send_system_cmd(m_sys_menu_opened ? 0x0132 /* CELL_SYSUTIL_SYSTEM_MENU_CLOSE */ : 0x0131 /* CELL_SYSUTIL_SYSTEM_MENU_OPEN */, 0);
		m_sys_menu_opened = !m_sys_menu_opened;
		sysSendOpenMenuAct->setText(tr("Send &%0 system menu cmd").arg(m_sys_menu_opened ? tr("close") : tr("open")));
	});
	connect(sysSendExitAct, &QAction::triggered, [=](){
		sysutil_send_system_cmd(0x0101 /* CELL_SYSUTIL_REQUEST_EXITGAME */, 0);
	});
	connect(confSettingsAct, &QAction::triggered, [=](){
		settings_dialog dlg(guiSettings, m_Render_Creator, this);
		connect(&dlg, &settings_dialog::GuiSettingsSaveRequest, this, &main_window::SaveWindowState);
		connect(&dlg, &settings_dialog::GuiSettingsSyncRequest, [=]() {ConfigureGuiFromSettings(true); });
		connect(&dlg, &settings_dialog::GuiStylesheetRequest, this, &main_window::RequestGlobalStylesheetChange);
		dlg.exec();
	});
	connect(confPadAct, &QAction::triggered, this, [=](){
		pad_settings_dialog dlg(this);
		dlg.exec();
	});
	connect(confAutopauseManagerAct, &QAction::triggered, [=](){
		auto_pause_settings_dialog dlg(this);
		dlg.exec();
	});
	connect(confSavedataManagerAct, &QAction::triggered, [=](){
		save_data_list_dialog* sdid = new save_data_list_dialog(this, true);
		sdid->show();
	});
	connect(toolsCgDisasmAct, &QAction::triggered, [=](){
		cg_disasm_window* cgdw = new cg_disasm_window(this);
		cgdw->show();
	});
	connect(toolskernel_explorerAct, &QAction::triggered, [=](){
		kernel_explorer* kernelExplorer = new kernel_explorer(this);
		kernelExplorer->show();
	});
	connect(toolsmemory_viewerAct, &QAction::triggered, [=](){
		memory_viewer_panel* mvp = new memory_viewer_panel(this);
		mvp->show();
	});
	connect(toolsRsxDebuggerAct, &QAction::triggered, [=](){
		rsx_debugger* rsx = new rsx_debugger(this);
		rsx->show();
	});
	connect(toolsStringSearchAct, &QAction::triggered, [=](){
		memory_string_searcher* mss = new memory_string_searcher(this);
		mss->show();
	});
	connect(toolsDecryptSprxLibsAct, &QAction::triggered, this, &main_window::DecryptSPRXLibraries);
	connect(showDebuggerAct, &QAction::triggered, [=](bool checked){
		checked ? debuggerFrame->show() : debuggerFrame->hide();
		guiSettings->SetValue(GUI::mw_debugger, checked);
	});
	connect(showLogAct, &QAction::triggered, [=](bool checked){
		checked ? logFrame->show() : logFrame->hide();
		guiSettings->SetValue(GUI::mw_logger, checked);
	});
	connect(showGameListAct, &QAction::triggered, [=](bool checked){
		checked ? gameListFrame->show() : gameListFrame->hide();
		guiSettings->SetValue(GUI::mw_gamelist, checked);
	});
	connect(showControlsAct, &QAction::triggered, this, [=](bool checked){
		checked ? controls->show() : controls->hide();
		guiSettings->SetValue(GUI::mw_controls, checked);
	});
	connect(showGameListToolBarAct, &QAction::triggered, this, [=](bool checked){
		gameListFrame->SetToolBarVisible(checked);
	});
	connect(refreshGameListAct, &QAction::triggered, [=](){
		gameListFrame->Refresh(true);
	});
	connect(categoryVisibleActGroup, &QActionGroup::triggered, [=](QAction* act)
	{
		QString cat;
		const bool& checked = act->isChecked();

		if      (act == showCatHDDGameAct)    cat = category::hdd_Game;
		else if (act == showCatDiscGameAct)   cat = category::disc_Game;
		else if (act == showCatHomeAct)       cat = category::home;
		else if (act == showCatAudioVideoAct) cat = category::audio_Video;
		else if (act == showCatGameDataAct)   cat = category::game_Data;
		else if (act == showCatUnknownAct)    cat = category::unknown;

		gameListFrame->SetCategoryActIcon(categoryVisibleActGroup->actions().indexOf(act), checked);
		gameListFrame->ToggleCategoryFilter(cat, checked);
		guiSettings->SetCategoryVisibility(cat, checked);
	});
	connect(aboutAct, &QAction::triggered, this, &main_window::About);
	connect(aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);
	connect(menu_run, &QAbstractButton::clicked, Pause);
	connect(menu_stop, &QAbstractButton::clicked, [=]() { Emu.Stop(); });
	connect(menu_restart, &QAbstractButton::clicked, [=]() { Emu.Stop();	Emu.Load();	});
	connect(menu_capture_frame, &QAbstractButton::clicked, [=](){
		user_asked_for_frame_capture = true;
	});
	connect(iconSizeActGroup, &QActionGroup::triggered, [=](QAction* act)
	{
		QString key;

		if (act == setIconSizeLargeAct) key = GUI::gl_icon_key_large;
		else if (act == setIconSizeMediumAct) key = GUI::gl_icon_key_medium;
		else if (act == setIconSizeSmallAct) key = GUI::gl_icon_key_small;
		else key = GUI::gl_icon_key_tiny;

		guiSettings->SetValue(GUI::gl_iconSize, key);

		for (int i = 0; i < GUI::gl_icon_size.count(); i++)
		{
			if (GUI::gl_icon_size.at(i).first == key)
			{
				gameListFrame->ResizeIcons(GUI::gl_icon_size.at(i).second, i);
				break;
			}
		}
	});
	connect (gameListFrame, &game_list_frame::RequestIconSizeActSet, [=](const int& idx)
	{
		iconSizeActGroup->actions().at(idx)->trigger();
	});
	connect(gameListFrame, &game_list_frame::RequestListModeActSet, [=](const bool& isList)
	{
		isList ? setlistModeListAct->trigger() : setlistModeGridAct->trigger();
	});
	connect(gameListFrame, &game_list_frame::RequestCategoryActSet, [=](const int& id)
	{
		categoryVisibleActGroup->actions().at(id)->trigger();
	});
	connect(listModeActGroup, &QActionGroup::triggered, [=](QAction* act)
	{
		bool isList = act == setlistModeListAct;
		gameListFrame->SetListMode(isList);
		categoryVisibleActGroup->setEnabled(isList);
	});
}

void main_window::CreateMenus()
{
	QMenu *bootMenu = menuBar()->addMenu(tr("&Boot"));
	bootMenu->addAction(bootElfAct);
	bootMenu->addAction(bootGameAct);

	m_bootRecentMenu = bootMenu->addMenu(tr("Boot Recent"));
	m_bootRecentMenu->setToolTipsVisible(true);
	m_bootRecentMenu->addAction(clearRecentAct);
	m_bootRecentMenu->addAction(freezeRecentAct);
	m_bootRecentMenu->addSeparator();

	bootMenu->addSeparator();
	bootMenu->addAction(bootInstallPkgAct);
	bootMenu->addAction(bootInstallPupAct);
	bootMenu->addSeparator();
	bootMenu->addAction(exitAct);

	QMenu *sysMenu = menuBar()->addMenu(tr("&System"));
	sysMenu->addAction(sysPauseAct);
	sysMenu->addAction(sysStopAct);
	sysMenu->addSeparator();
	sysMenu->addAction(sysSendOpenMenuAct);
	sysMenu->addAction(sysSendExitAct);

	QMenu *confMenu = menuBar()->addMenu(tr("&Config"));
	confMenu->addAction(confSettingsAct);
	confMenu->addAction(confPadAct);
	confMenu->addSeparator();
	confMenu->addAction(confAutopauseManagerAct);
	confMenu->addSeparator();
	confMenu->addAction(confSavedataManagerAct);

	QMenu *toolsMenu = menuBar()->addMenu(tr("&Utilities"));
	toolsMenu->addAction(toolsCgDisasmAct);
	toolsMenu->addAction(toolskernel_explorerAct);
	toolsMenu->addAction(toolsmemory_viewerAct);
	toolsMenu->addAction(toolsRsxDebuggerAct);
	toolsMenu->addAction(toolsStringSearchAct);
	toolsMenu->addAction(toolsDecryptSprxLibsAct);

	QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
	viewMenu->addAction(showLogAct);
	viewMenu->addAction(showDebuggerAct);
	viewMenu->addAction(showControlsAct);
	viewMenu->addSeparator();
	viewMenu->addAction(showGameListAct);
	viewMenu->addAction(showGameListToolBarAct);
	viewMenu->addAction(refreshGameListAct);

	QMenu *categoryMenu = viewMenu->addMenu(tr("Show Categories"));
	categoryMenu->addActions(categoryVisibleActGroup->actions());

	QMenu *iconSizeMenu = viewMenu->addMenu(tr("Icon Size"));
	iconSizeMenu->addActions(iconSizeActGroup->actions());

	QMenu *listModeMenu = viewMenu->addMenu(tr("Game List Mode"));
	listModeMenu->addActions(listModeActGroup->actions());

	QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(aboutAct);
	helpMenu->addAction(aboutQtAct);

	QHBoxLayout* controls_layout = new QHBoxLayout();
	menu_run = new QPushButton(tr("Start"));
	menu_stop = new QPushButton(tr("Stop"));
	menu_restart = new QPushButton(tr("Restart"));
	menu_capture_frame = new QPushButton(tr("Capture Frame"));
	menu_run->setEnabled(false);
	menu_stop->setEnabled(false);
	menu_restart->setEnabled(false);
	menu_run->setIcon(icon_play);
	menu_stop->setIcon(icon_stop);
	menu_restart->setIcon(icon_restart);
	controls_layout->addWidget(menu_run);
	controls_layout->addWidget(menu_stop);
	controls_layout->addWidget(menu_restart);
	controls_layout->addWidget(menu_capture_frame);
	controls_layout->addSpacing(5);
	controls_layout->setContentsMargins(0, 0, 0, 0);
	controls = new QWidget(this);
	controls->setLayout(controls_layout);
	menuBar()->setCornerWidget(controls, Qt::TopRightCorner);
}

void main_window::CreateDockWindows()
{
	gameListFrame = new game_list_frame(guiSettings, m_Render_Creator, this);
	gameListFrame->setObjectName("gamelist");
	debuggerFrame = new debugger_frame(this);
	debuggerFrame->setObjectName("debugger");
	logFrame = new log_frame(guiSettings, this);
	logFrame->setObjectName("logger");

	addDockWidget(Qt::LeftDockWidgetArea, gameListFrame);
	addDockWidget(Qt::LeftDockWidgetArea, logFrame);
	addDockWidget(Qt::RightDockWidgetArea, debuggerFrame);

	connect(logFrame, &log_frame::log_frameClosed, [=]()
	{
		if (showLogAct->isChecked())
		{
			showLogAct->setChecked(false);
			guiSettings->SetValue(GUI::mw_logger, false);
		}
	});
	connect(debuggerFrame, &debugger_frame::DebugFrameClosed, [=](){
		if (showDebuggerAct->isChecked())
		{
			showDebuggerAct->setChecked(false);
			guiSettings->SetValue(GUI::mw_debugger, false);
		}
	});
	connect(gameListFrame, &game_list_frame::game_list_frameClosed, [=]()
	{
		if (showGameListAct->isChecked())
		{
			showGameListAct->setChecked(false);
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

	freezeRecentAct->setChecked(guiSettings->GetValue(GUI::rg_freeze).toBool());
	m_rg_entries = guiSettings->Var2List(guiSettings->GetValue(GUI::rg_entries));

	// Fill the recent games menu
	for (uint i = 0; i < m_rg_entries.count(); i++)
	{
		// create new action
		QAction* act = CreateRecentAction(m_rg_entries[i], i + 1);

		// add action to menu
		if (act)
		{
			m_recentGameActs.append(act);
			m_bootRecentMenu->addAction(act);
		}
		else
		{
			i--; // list count is now an entry shorter so we have to repeat the same index in order to load all other entries
		}
	}

	showLogAct->setChecked(guiSettings->GetValue(GUI::mw_logger).toBool());
	showGameListAct->setChecked(guiSettings->GetValue(GUI::mw_gamelist).toBool());
	showDebuggerAct->setChecked(guiSettings->GetValue(GUI::mw_debugger).toBool());
	showControlsAct->setChecked(guiSettings->GetValue(GUI::mw_controls).toBool());
	showGameListToolBarAct->setChecked(guiSettings->GetValue(GUI::gl_toolBarVisible).toBool());
	guiSettings->GetValue(GUI::mw_controls).toBool() ? controls->show() : controls->hide();

	showCatHDDGameAct->setChecked(guiSettings->GetCategoryVisibility(category::hdd_Game));
	showCatDiscGameAct->setChecked(guiSettings->GetCategoryVisibility(category::disc_Game));
	showCatHomeAct->setChecked(guiSettings->GetCategoryVisibility(category::home));
	showCatAudioVideoAct->setChecked(guiSettings->GetCategoryVisibility(category::audio_Video));
	showCatGameDataAct->setChecked(guiSettings->GetCategoryVisibility(category::game_Data));
	showCatUnknownAct->setChecked(guiSettings->GetCategoryVisibility(category::unknown));

	QString key = guiSettings->GetValue(GUI::gl_iconSize).toString();
	if (key == GUI::gl_icon_key_large) setIconSizeLargeAct->setChecked(true);
	else if (key == GUI::gl_icon_key_medium) setIconSizeMediumAct->setChecked(true);
	else if (key == GUI::gl_icon_key_small) setIconSizeSmallAct->setChecked(true);
	else setIconSizeTinyAct->setChecked(true);


	bool isListMode = guiSettings->GetValue(GUI::gl_listMode).toBool();
	if (isListMode) setlistModeListAct->setChecked(true);
	else setlistModeGridAct->setChecked(true);
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
	if (keyEvent->modifiers() == Qt::ControlModifier)
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
