
#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QProgressDialog>
#include <QDesktopWidget>

#include "SaveDataUtility.h"
#include "KernelExplorer.h"
#include "gamelistframe.h"
#include "debuggerframe.h"
#include "logframe.h"
#include "settingsdialog.h"
#include "padsettingsdialog.h"
#include "AutoPauseSettingsDialog.h"
#include "CgDisasmWindow.h"
#include "MemoryStringSearcher.h"
#include "MemoryViewer.h"
#include "RSXDebugger.h"
#include "mainwindow.h"
#include "EmuSettings.h"

#include <thread>

#include "stdafx.h"
#include "Emu\System.h"
#include "Emu\Memory\Memory.h"

#include "Crypto\unpkg.h"
#include "Crypto\unself.h"

#include "Loader\PUP.h"
#include "Loader\TAR.h"

#include "Utilities\Thread.h"
#include "Utilities\StrUtil.h"

#include "rpcs3_version.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_sys_menu_opened(false)
{
	guiSettings.reset(new GuiSettings());

	setDockNestingEnabled(true);

	//Load Icons: This needs to happen before any actions or buttons are created
	fs::is_file("Icons/play.png") ? icon_play = QIcon("Icons/play.png") : icon_play = style()->standardIcon(QStyle::SP_MediaPlay);
	fs::is_file("Icons/pause.png") ? icon_pause = QIcon("Icons/pause.png") : icon_pause = style()->standardIcon(QStyle::SP_MediaPause);
	fs::is_file("Icons/stop.png") ? icon_stop = QIcon("Icons/stop.png") : icon_stop = style()->standardIcon(QStyle::SP_MediaStop);
	fs::is_file("Icons/restart.png") ? icon_restart = QIcon("Icons/restart.png") : icon_restart = style()->standardIcon(QStyle::SP_BrowserReload);
	appIcon = QIcon("rpcs3.ico");

	CreateActions();
	CreateMenus();
	CreateDockWindows();

	setMinimumSize(350, minimumSizeHint().height());    // seems fine on win 10

	ConfigureGuiFromSettings();
	CreateConnects();

	setWindowTitle(QString::fromStdString("RPCS3 v" + rpcs3::version.to_string()));
	if (!appIcon.isNull())
	{
		setWindowIcon(appIcon);
	}

	QTimer::singleShot(1, [=]() {
		// Need to have this happen fast, but not now because connects aren't created yet.
		// So, a tricky balance in terms of time but this works.
		emit RequestGlobalStylesheetChange(guiSettings->GetCurrentStylesheetPath()); 
	});
}

MainWindow::~MainWindow()
{
}

auto Pause = [=]() {
	if (Emu.IsReady()) Emu.Run();
	else if (Emu.IsPaused()) Emu.Resume();
	else if (Emu.IsRunning()) Emu.Pause();
	else if (!Emu.GetPath().empty()) Emu.Load();
};

void MainWindow::CreateThumbnailToolbar()
{
#ifdef _WIN32
	thumb_bar = new QWinThumbnailToolBar(this);
	thumb_bar->setWindow(windowHandle());

	thumb_playPause = new QWinThumbnailToolButton(thumb_bar);
	thumb_playPause->setToolTip("Start");
	thumb_playPause->setIcon(icon_play);
	thumb_playPause->setEnabled(false);

	thumb_stop = new QWinThumbnailToolButton(thumb_bar);
	thumb_stop->setToolTip("Stop");
	thumb_stop->setIcon(icon_stop);
	thumb_stop->setEnabled(false);

	thumb_restart = new QWinThumbnailToolButton(thumb_bar);
	thumb_restart->setToolTip("Restart");
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
QIcon MainWindow::GetAppIcon()
{
	return appIcon;
}

// loads the appIcon from path and embeds it centered into an empty square icon
void MainWindow::SetAppIconFromPath(const std::string path)
{
	// get Icon for the GSFrame from path. this handles presumably all possible use cases
	QString qpath = QString::fromUtf8(path.data(), path.size());
	std::string icon_list[] = { "/ICON0.PNG", "/PS3_GAME/ICON0.PNG" };
	std::string path_list[] = { path, qpath.section("/", 0, -2).toUtf8().toStdString() ,qpath.section("/", 0, -3).toUtf8().toStdString() };
	for (std::string pth : path_list)
	{
		for (std::string ico : icon_list)
		{
			ico = pth + ico;
			if (fs::is_file(ico))
			{
				// load the image from path. It will most likely be a rectangle
				QImage source = QImage(QString::fromUtf8(ico.data(), ico.size()));
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
	appIcon = QIcon("rpcs3.ico");
}

void MainWindow::BootElf()
{
	bool stopped = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	QFileDialog dlg(this, tr("Select (S)ELF To Boot"), "", tr("(S)ELF files (*BOOT.BIN *.elf *.self);;"
		"ELF files (BOOT.BIN *.elf);;"
		"SELF files (EBOOT.BIN *.self);;"
		"BOOT files (*BOOT.BIN);;"
		"BIN files (*.bin);;"
		"All files (*.*)"));
	dlg.setAcceptMode(QFileDialog::AcceptOpen);
	dlg.setFileMode(QFileDialog::ExistingFile);

	if (dlg.exec() == QDialog::Rejected)
	{
		if (stopped) Emu.Resume();
		return;
	}

	LOG_NOTICE(LOADER, "(S)ELF: booting...");

	const std::string path = dlg.selectedFiles().first().toUtf8().toStdString();
	SetAppIconFromPath(path);
	Emu.Stop();
	Emu.SetPath(path);
	Emu.Load();

	if (Emu.IsReady()) LOG_SUCCESS(LOADER, "(S)ELF: boot done.");
}

void MainWindow::BootGame()
{
	bool stopped = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	QFileDialog dlg(this, tr("Select Game Folder"));
	dlg.setAcceptMode(QFileDialog::AcceptOpen);
	dlg.setFileMode(QFileDialog::Directory);
	dlg.setOptions(QFileDialog::ShowDirsOnly);

	if (dlg.exec() == QDialog::Rejected)
	{
		if (stopped) Emu.Resume();
		return;
	}
	Emu.Stop();
	const std::string path = dlg.selectedFiles().first().toUtf8();
	SetAppIconFromPath(path);

	if (!Emu.BootGame(path))
	{
		LOG_ERROR(GENERAL, "PS3 executable not found in selected folder (%s)", path);
	}
}

void MainWindow::InstallPkg()
{
	QString filePath = QFileDialog::getOpenFileName(this, tr("Select PKG To Install"), "", tr("PKG files (*.pkg);;All files (*.*)"));

	if (filePath == NULL)
	{
		return;
	}
	Emu.Stop();

	const std::string fileName = filePath.section("/", -1, -1).toUtf8();

	const std::string path = filePath.toUtf8();

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
					gameListFrame->Refresh();
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
		gameListFrame->Refresh();
		LOG_SUCCESS(GENERAL, "Successfully installed %s.", fileName);
		QMessageBox::information(this, tr("Success!"), tr("Successfully installed software from package!"));
#ifdef _WIN32
		taskbar_progress->hide();
		taskbar_button->~QWinTaskbarButton();
#endif
	}
}

void MainWindow::InstallPup()
{
	QString filePath = QFileDialog::getOpenFileName(this, tr("Select PS3UPDAT.PUP To Install"), "", tr("PS3 update file (PS3UPDAT.PUP)|PS3UPDAT.PUP"));

	if (filePath == NULL)
	{
		return;
	}

	Emu.Stop();

	const std::string path = filePath.toUtf8();

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
		QMessageBox::information(this, tr("Success!"), tr("Successfully installed PS3 firmware and LLE Modules!"));
#ifdef _WIN32
		taskbar_progress->hide();
		taskbar_button->~QWinTaskbarButton();
#endif
	}
}

// This is ugly, but PS3 headers shall not be included there.
extern void sysutil_send_system_cmd(u64 status, u64 param);

void MainWindow::DecryptSPRXLibraries()
{
	QFileDialog sprxdlg(this, tr("Select SPRX files"), "", tr("SPRX files (*.sprx)"));
	sprxdlg.setAcceptMode(QFileDialog::AcceptOpen);
	sprxdlg.setFileMode(QFileDialog::ExistingFiles);

	if (sprxdlg.exec() == QDialog::Rejected)
	{
		return;
	}

	Emu.Stop();

	QStringList modules = sprxdlg.selectedFiles();

	LOG_NOTICE(GENERAL, "Decrypting SPRX libraries...");

	for (QString& module : modules)
	{
		std::string prx_path = module.toUtf8();
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

void MainWindow::About()
{

	QString translatedTextAboutCaption;
	translatedTextAboutCaption = tr(
		"<h1>RPCS3</h1>"
		"A PlayStation 3 emulator and debugger.<br>"
		"RPCS3 Version: %1").arg(QString::fromStdString(rpcs3::version.to_string())
		);
	QString translatedTextAboutText;
	translatedTextAboutText = tr(
		"<br><p><b>Developers:</b> Developers: ¬DH, ¬AlexAltea, ¬Hykem, Oil, Nekotekina, Bigpet, ¬gopalsr83, ¬tambry, "
		"vlj, kd-11, jarveson, raven02, AniLeo, cornytrace, ssshadow, Numan</p>"
		"<p><b>Contributors:</b> BlackDaemon, elisha464, Aishou, krofna, xsacha, danilaml, unknownbrackets, Zangetsu38, "
		"lioncashachurch, darkf, Syphurith, Blaypeg, Survanium90, georgemoralis, ikki84</p>"
		"<p><b>Supporters:</b> Howard Garrison, EXPotemkin, Marko V., danhp, Jake (5315825), Ian Reid, Tad Sherlock, Tyler Friesen, "
		"Folzar, Payton Williams, RedPill Australia, yanghong</p>"
		"<br><p>Please see "
		"<a href=\"https://%1/\">GitHub</a>, "
		"<a href=\"https://%2/\">Website</a>, "
		"<a href=\"http://%3/\">Forum</a> or "
		"<a href=\"https://%4/\">Patreon</a>"
		" for more information.</p>"
	).arg("github.com/RPCS3",
		"rpcs3.net",
		"www.emunewz.net/forum/forumdisplay.php?fid=172",
		"www.patreon.com/Nekotekina");

	QMessageBox about(this);
	about.setMinimumWidth(500);
	about.setWindowTitle(tr("About RPCS3"));
	about.setText(translatedTextAboutCaption);
	about.setInformativeText(translatedTextAboutText);

	about.exec();
}

/** Needed so that when a backup occurs of window state in guisettings, the state is current. 
* Also, so that on close, the window state is preserved.
*/
void MainWindow::SaveWindowState()
{
	// Save gui settings
	guiSettings->WriteGuiGeometry(saveGeometry());
	guiSettings->WriteGuiState(saveState());

	// Save column settings
	gameListFrame->SaveSettings();
}

void MainWindow::OnEmuRun()
{
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

void MainWindow::OnEmuResume()
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

void MainWindow::OnEmuPause()
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

void MainWindow::OnEmuStop()
{
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

void MainWindow::OnEmuReady()
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

void MainWindow::EnableMenus(bool enabled)
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
	toolsCompilerAct->setEnabled(enabled);
	toolsKernelExplorerAct->setEnabled(enabled);
	toolsMemoryViewerAct->setEnabled(enabled);
	toolsRsxDebuggerAct->setEnabled(enabled);
	toolsStringSearchAct->setEnabled(enabled);
}

void MainWindow::CreateActions()
{
	bootElfAct = new QAction(tr("Boot (S)ELF file"), this);
	bootGameAct = new QAction(tr("Boot &Game"), this);
	bootInstallPkgAct = new QAction(tr("&Install PKG"), this);
	bootInstallPupAct = new QAction(tr("&Install Firmware"), this);

	exitAct = new QAction(tr("E&xit"), this);
	exitAct->setShortcuts(QKeySequence::Quit);
	exitAct->setStatusTip(tr("Exit the application"));

	sysPauseAct = new QAction(tr("&Pause"), this);
	sysPauseAct->setEnabled(false);
	sysPauseAct->setIcon(icon_pause);

	sysStopAct = new QAction(tr("&Stop"), this);
	sysStopAct->setShortcut(tr("Ctrl+S"));
	sysStopAct->setEnabled(false);
	sysStopAct->setIcon(icon_stop);

	sysSendOpenMenuAct = new QAction(tr("Send &open system menu cmd"), this);
	sysSendOpenMenuAct->setEnabled(false);

	sysSendExitAct = new QAction(tr("Send &exit cmd"), this);
	sysSendExitAct->setEnabled(false);

	confSettingsAct = new QAction(tr("&Settings"), this);
	confPadAct = new QAction(tr("&Keyboard Settings"), this);

	confAutopauseManagerAct = new QAction(tr("&Auto Pause Settings"), this);

	confVfsManagerAct = new QAction(tr("Virtual &File System Manager"), this);
	confVfsManagerAct->setEnabled(false);

	confVhddManagerAct = new QAction(tr("Virtual &HDD Manager"), this);
	confVhddManagerAct->setEnabled(false);

	confSavedataManagerAct = new QAction(tr("Save &Data Utility"), this);
	confSavedataManagerAct->setEnabled(true);

	toolsCompilerAct = new QAction(tr("&ELF Compiler"), this);
	toolsCompilerAct->setEnabled(false);

	toolsCgDisasmAct = new QAction(tr("&Cg Disasm"), this);
	toolsCgDisasmAct->setEnabled(true);

	toolsKernelExplorerAct = new QAction(tr("&Kernel Explorer"), this);
	toolsKernelExplorerAct->setEnabled(false);

	toolsMemoryViewerAct = new QAction(tr("&Memory Viewer"), this);
	toolsMemoryViewerAct->setEnabled(false);

	toolsRsxDebuggerAct = new QAction(tr("&RSX Debugger"), this);
	toolsRsxDebuggerAct->setEnabled(false);

	toolsStringSearchAct = new QAction(tr("&String Search"), this);
	toolsStringSearchAct->setEnabled(false);

	toolsDecryptSprxLibsAct = new QAction(tr("&Decrypt SPRX libraries"), this);

	showDebuggerAct = new QAction(tr("Show Debugger"), this);
	showDebuggerAct->setCheckable(true);

	showLogAct = new QAction(tr("Show Log/TTY"), this);
	showLogAct->setCheckable(true);

	showGameListAct = new QAction(tr("Show GameList"), this);
	showGameListAct->setCheckable(true);

	showControlsAct = new QAction(tr("Show Controls"), this);
	showControlsAct->setCheckable(true);

	refreshGameListAct = new QAction(tr("&Refresh Game List"), this);

	showCatHDDGameAct = new QAction(tr("HDD Game"), this);
	showCatHDDGameAct->setCheckable(true);

	showCatDiscGameAct = new QAction(tr("Disc Game"), this);
	showCatDiscGameAct->setCheckable(true);

	showCatHomeAct = new QAction(tr("Home"), this);
	showCatHomeAct->setCheckable(true);

	showCatAudioVideoAct = new QAction(tr("Audio/Video"), this);
	showCatAudioVideoAct->setCheckable(true);

	showCatGameDataAct = new QAction(tr("Game Data"), this);
	showCatGameDataAct->setCheckable(true);

	aboutAct = new QAction(tr("&About"), this);
	aboutAct->setStatusTip(tr("Show the application's About box"));

	aboutQtAct = new QAction(tr("About &Qt"), this);
	aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
}

void MainWindow::CreateConnects()
{
	connect(bootElfAct, &QAction::triggered, this, &MainWindow::BootElf);
	connect(bootGameAct, &QAction::triggered, this, &MainWindow::BootGame);
	connect(bootInstallPkgAct, &QAction::triggered, this, &MainWindow::InstallPkg);
	connect(bootInstallPupAct, &QAction::triggered, this, &MainWindow::InstallPup);
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
		SettingsDialog dlg(guiSettings, this);
		connect(&dlg, &SettingsDialog::GuiSettingsSaveRequest, this, &MainWindow::SaveWindowState);
		connect(&dlg, &SettingsDialog::GuiSettingsSyncRequest, [=]() {ConfigureGuiFromSettings(true); });
		connect(&dlg, &SettingsDialog::GuiStylesheetRequest, this, &MainWindow::RequestGlobalStylesheetChange);
		dlg.exec();
	});
	connect(confPadAct, &QAction::triggered, this, [=](){
		PadSettingsDialog dlg(this);
		dlg.exec();
	});
	connect(confAutopauseManagerAct, &QAction::triggered, [=](){
		AutoPauseSettingsDialog dlg(this);
		dlg.exec();
	});
	connect(toolsCompilerAct, &QAction::triggered, []() {});
	connect(confVfsManagerAct, &QAction::triggered, []() {});
	connect(confVhddManagerAct, &QAction::triggered, []() {});
	connect(confSavedataManagerAct, &QAction::triggered, [=](){
		SaveDataListDialog* sdid = new SaveDataListDialog(this, true);
		sdid->show();
	});
	connect(toolsCgDisasmAct, &QAction::triggered, [=](){
		CgDisasmWindow* cgdw = new CgDisasmWindow(this);
		cgdw->show();
	});
	connect(toolsKernelExplorerAct, &QAction::triggered, [=](){
		KernelExplorer* kernelExplorer = new KernelExplorer(this);
		kernelExplorer->show();
	});
	connect(toolsMemoryViewerAct, &QAction::triggered, [=](){
		MemoryViewerPanel* mvp = new MemoryViewerPanel(this);
		mvp->show();
	});
	connect(toolsRsxDebuggerAct, &QAction::triggered, [=](){
		RSXDebugger* rsx = new RSXDebugger(this);
		rsx->show();
	});
	connect(toolsStringSearchAct, &QAction::triggered, [=](){
		MemoryStringSearcher* mss = new MemoryStringSearcher(this);
		mss->show();
	});
	connect(toolsDecryptSprxLibsAct, &QAction::triggered, this, &MainWindow::DecryptSPRXLibraries);
	connect(showDebuggerAct, &QAction::triggered, [=](bool checked){
		checked ? debuggerFrame->show() : debuggerFrame->hide();
		guiSettings->SetDebuggerVisibility(checked);
	});
	connect(showLogAct, &QAction::triggered, [=](bool checked){
		checked ? logFrame->show() : logFrame->hide();
		guiSettings->SetLoggerVisibility(checked);
	});
	connect(showGameListAct, &QAction::triggered, [=](bool checked){
		checked ? gameListFrame->show() : gameListFrame->hide();
		guiSettings->SetGamelistVisibility(checked);
	});
	connect(showControlsAct, &QAction::triggered, this, [=](bool checked){
		checked ? controls->show() : controls->hide();
		guiSettings->SetControlsVisibility(checked);
	});
	connect(refreshGameListAct, &QAction::triggered, [=](){
		gameListFrame->Refresh();
	});
	connect(showCatHDDGameAct, &QAction::triggered, [=](bool checked){
		gameListFrame->ToggleCategoryFilter("HDD Game", checked);
		guiSettings->SetCategoryHDDGameVisibility(checked);
	});
	connect(showCatDiscGameAct, &QAction::triggered, [=](bool checked){
		gameListFrame->ToggleCategoryFilter("Disc Game", checked);
		guiSettings->SetCategoryDiscGameVisibility(checked);
	});
	connect(showCatHomeAct, &QAction::triggered, [=](bool checked){
		gameListFrame->ToggleCategoryFilter("Home", checked);
		guiSettings->SetCategoryHomeVisibility(checked);
	});
	connect(showCatAudioVideoAct, &QAction::triggered, [=](bool checked){
		gameListFrame->ToggleCategoryFilter("Audio/Video", checked);
		guiSettings->SetCategoryAudioVideoVisibility(checked);
	});
	connect(showCatGameDataAct, &QAction::triggered, [=](bool checked){
		gameListFrame->ToggleCategoryFilter("Game Data", checked);
		guiSettings->SetCategoryGameDataVisibility(checked);
	});
	connect(aboutAct, &QAction::triggered, this, &MainWindow::About);
	connect(aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);
	connect(menu_run, &QAbstractButton::clicked, Pause);
	connect(menu_stop, &QAbstractButton::clicked, [=]() { Emu.Stop(); });
	connect(menu_restart, &QAbstractButton::clicked, [=]() { Emu.Stop();	Emu.Load();	});
	connect(menu_capture_frame, &QAbstractButton::clicked, [=](){
		user_asked_for_frame_capture = true;
	});
}

void MainWindow::CreateMenus()
{
	QMenu *bootMenu = menuBar()->addMenu(tr("&Boot"));
	bootMenu->addAction(bootElfAct);
	bootMenu->addAction(bootGameAct);
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
	confMenu->addAction(confVfsManagerAct);
	confMenu->addAction(confVhddManagerAct);
	confMenu->addAction(confSavedataManagerAct);

	QMenu *toolsMenu = menuBar()->addMenu(tr("&Utilities"));
	toolsMenu->addAction(toolsCompilerAct);
	toolsMenu->addAction(toolsCgDisasmAct);
	toolsMenu->addAction(toolsKernelExplorerAct);
	toolsMenu->addAction(toolsMemoryViewerAct);
	toolsMenu->addAction(toolsRsxDebuggerAct);
	toolsMenu->addAction(toolsStringSearchAct);
	toolsMenu->addSeparator();
	toolsMenu->addAction(toolsDecryptSprxLibsAct);

	QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
	viewMenu->addAction(showLogAct);
	viewMenu->addAction(showDebuggerAct);
	viewMenu->addAction(showControlsAct);
	viewMenu->addSeparator();
	viewMenu->addAction(showGameListAct);
	viewMenu->addAction(refreshGameListAct);

	QMenu *categoryMenu = viewMenu->addMenu(tr("Show Categories"));
	categoryMenu->addAction(showCatHDDGameAct);
	categoryMenu->addAction(showCatDiscGameAct);
	categoryMenu->addAction(showCatHomeAct);
	categoryMenu->addAction(showCatAudioVideoAct);
	categoryMenu->addAction(showCatGameDataAct);

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

void MainWindow::CreateDockWindows()
{
	gameListFrame = new GameListFrame(guiSettings, this);
	gameListFrame->setObjectName("gamelist");
	debuggerFrame = new DebuggerFrame(this);
	debuggerFrame->setObjectName("debugger");
	logFrame = new LogFrame(guiSettings, this);
	logFrame->setObjectName("logger");

	addDockWidget(Qt::LeftDockWidgetArea, gameListFrame);
	addDockWidget(Qt::LeftDockWidgetArea, logFrame);
	addDockWidget(Qt::RightDockWidgetArea, debuggerFrame);

	connect(logFrame, &LogFrame::LogFrameClosed, [=]()
	{
		if (showLogAct->isChecked())
		{
			showLogAct->setChecked(false);
			guiSettings->SetLoggerVisibility(false);
		}
	});
	connect(debuggerFrame, &DebuggerFrame::DebugFrameClosed, [=](){
		if (showDebuggerAct->isChecked())
		{
			showDebuggerAct->setChecked(false);
			guiSettings->SetDebuggerVisibility(false);
		}
	});
	connect(gameListFrame, &GameListFrame::GameListFrameClosed, [=]()
	{
		if (showGameListAct->isChecked())
		{
			showGameListAct->setChecked(false);
			guiSettings->SetGamelistVisibility(false);
		}
	});
	connect(gameListFrame, &GameListFrame::RequestIconPathSet, this, &MainWindow::SetAppIconFromPath);
}

void MainWindow::ConfigureGuiFromSettings(bool configureAll)
{
	// Restore GUI state if needed. We need to if they exist.
	QByteArray geometry = guiSettings->ReadGuiGeometry();
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

	restoreState(guiSettings->ReadGuiState());

	showLogAct->setChecked(guiSettings->GetLoggerVisibility());
	showGameListAct->setChecked(guiSettings->GetGamelistVisibility());
	showDebuggerAct->setChecked(guiSettings->GetDebuggerVisibility());
	showControlsAct->setChecked(guiSettings->GetControlsVisibility());
	guiSettings->GetControlsVisibility() ? controls->show() : controls->hide();

	showCatHDDGameAct->setChecked(guiSettings->GetCategoryHDDGameVisibility());;
	showCatDiscGameAct->setChecked(guiSettings->GetCategoryDiscGameVisibility());;
	showCatHomeAct->setChecked(guiSettings->GetCategoryHomeVisibility());;
	showCatAudioVideoAct->setChecked(guiSettings->GetCategoryAudioVideoVisibility());;
	showCatGameDataAct->setChecked(guiSettings->GetCategoryGameDataVisibility());;

	if (configureAll)
	{
		// Handle log settings
		logFrame->LoadSettings();

		// Gamelist
		gameListFrame->LoadSettings();
	}
}

void MainWindow::keyPressEvent(QKeyEvent *keyEvent)
{
	switch (keyEvent->key())
	{
	case 'E': case 'e': if (Emu.IsPaused()) Emu.Resume(); else if (Emu.IsReady()) Emu.Run(); return;
	case 'P': case 'p': if (Emu.IsRunning()) Emu.Pause(); return;
	case 'S': case 's': if (!Emu.IsStopped()) Emu.Stop(); return;
	case 'R': case 'r': if (!Emu.GetPath().empty()) { Emu.Stop(); Emu.Run(); } return;
	}
}

/** Override the Qt close event to have the emulator stop and the application die.  May add a warning dialog in future. 
*/
void MainWindow::closeEvent(QCloseEvent* closeEvent)
{
	Q_UNUSED(closeEvent);

	// Cleanly stop the emulator.
	Emu.Stop();

	SaveWindowState();

	// I need the gui settings to sync, and that means having the destructor called as guiSetting's parent is mainwindow.
	setAttribute(Qt::WA_DeleteOnClose);
	QMainWindow::close();

	
	// It's possible to have other windows open, like games.  So, force the application to die.
	QApplication::quit();
}
