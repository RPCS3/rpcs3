#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QProgressDialog>
#include <QSizePolicy>
#include <QDesktopWidget>

#include "gamelistframe.h"
#include "debuggerframe.h"
#include "logframe.h"
#include "settingsdialog.h"
#include "padsettingsdialog.h"
#include "AutoPauseSettingsDialog.h"
#include "CgDisasmWindow.h"
#include "MemoryStringSearcher.h"
#include "mainwindow.h"

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
	guiSettings = new GuiSettings(this);

	setDockNestingEnabled(true);
	CreateActions();
	CreateConnects();
	CreateMenus();
	CreateDockWindows();

	setMinimumSize(200, minimumSizeHint().height());    // seems fine on win 10

	ConfigureGuiFromSettings();
}

MainWindow::~MainWindow()
{
}

void MainWindow::DoSettings(bool load)
{
	//auto&& cfg = g_gui_cfg[ini_name]["aui"];
	//
	//if (load)
	//{
	//	const auto& perspective = fmt::FromUTF8(cfg.Scalar());
	//
	//	m_aui_mgr.LoadPerspective(perspective.empty() ? m_aui_mgr.SavePerspective() : perspective);
	//}
	//else
	//{
	//	cfg = fmt::ToUTF8(m_aui_mgr.SavePerspective());
	//	save_gui_cfg();
	//}
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

	Emu.Stop();
	Emu.SetPath((std::string) dlg.selectedFiles().first().toUtf8());
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
			if (QMessageBox::question(this, tr("PKG Decrypter / Installer"), tr("Another installation found. Do you want to overwrite it?")) != QMessageBox::Yes)
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
				if (QMessageBox::question(this, tr("PKG Decrypter / Installer"), tr("Remove incomplete folder?")) == QMessageBox::Yes)
				{
					fs::remove_all(local_path);
					RefreshGameList();
					LOG_SUCCESS(LOADER, "PKG: removed incomplete installation in %s", local_path);
					return;
				}
				break;
			}
			// Update progress window
			pdlg.setValue(static_cast<int>(progress * pdlg.maximum()));
			QCoreApplication::processEvents();
		}

		if (progress > 0.)
		{
			pdlg.setValue(pdlg.maximum());
			std::this_thread::sleep_for(100ms);
		}
	}

	if (progress >= 1.)
	{
		RefreshGameList();
		LOG_SUCCESS(GENERAL, "Successfully installed %s.", fileName);
		QMessageBox::information(this, tr("Success!"), tr("Successfully installed software from package!"));
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
				break;
			}
			// Update progress window
			pdlg.setValue(static_cast<int>(progress));
			QCoreApplication::processEvents();
		}

		update_files_f.close();
		pup_f.close();

		if (progress > 0)
		{
			pdlg.setValue(pdlg.maximum());
			std::this_thread::sleep_for(100ms);
		}
	}

	if (progress > 0)
	{
		LOG_SUCCESS(GENERAL, "Successfully installed PS3 firmware.");
		QMessageBox::information(this, tr("Success!"), tr("Successfully installed PS3 firmware and LLE Modules!"));
	}
}

void MainWindow::Pause()
{
	if (Emu.IsReady())
	{
		Emu.Run();
	}
	else if (Emu.IsPaused())
	{
		Emu.Resume();
	}
	else if (Emu.IsRunning())
	{
		Emu.Pause();
	}
	else if (!Emu.GetPath().empty())
	{
		Emu.Load();
	}
}

void MainWindow::Stop()
{
	Emu.Stop();
}

// This is ugly, but PS3 headers shall not be included there.
extern void sysutil_send_system_cmd(u64 status, u64 param);

void MainWindow::SendExit()
{
	sysutil_send_system_cmd(0x0101 /* CELL_SYSUTIL_REQUEST_EXITGAME */, 0);
}

void MainWindow::SendOpenSysMenu()
{
	sysutil_send_system_cmd(m_sys_menu_opened ? 0x0132 /* CELL_SYSUTIL_SYSTEM_MENU_CLOSE */ : 0x0131 /* CELL_SYSUTIL_SYSTEM_MENU_OPEN */, 0);
	m_sys_menu_opened = !m_sys_menu_opened;
	sysSendOpenMenuAct->setText(tr("Send &%0 system menu cmd").arg(m_sys_menu_opened ? tr("close") : tr("open")));
}

/**
 * Prompt the user for a stylesheet and propagate the request back to the main application.
*/
void MainWindow::OpenCustomStyleSheet()
{
	QString filePath = QFileDialog::getOpenFileName(nullptr, tr("Open Custom Stylesheet"), "", tr("Stylesheet (*.qss)"));
	if (filePath.isEmpty() == false)
	{
		emit RequestGlobalStylesheetChange(filePath);
	}
}

void MainWindow::Settings()
{
	SettingsDialog dlg(this);
	dlg.exec();
}

void MainWindow::PadSettings()
{
	PadSettingsDialog dlg(this);
	dlg.exec();
}

void MainWindow::AutoPauseSettings()
{
	AutoPauseSettingsDialog dlg(this);
	dlg.exec();
}

void MainWindow::VFSManager() {}

void MainWindow::VHDDManager() {}

void MainWindow::SaveData() {}

void MainWindow::ELFCompiler() {}

void MainWindow::CgDisasm()
{
	CgDisasmWindow* cgdw = new CgDisasmWindow(this);
	cgdw->show();
}

void MainWindow::KernelExplorer() {}

void MainWindow::MemoryViewer() {}

void MainWindow::RSXDebugger() {}

void MainWindow::StringSearch()
{
	MemoryStringSearcher* mss = new MemoryStringSearcher(this);
	mss->show();
}

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

void MainWindow::ToggleDebugFrame(bool state)
{
	if (state)
	{
		debuggerFrame->show();
	}
	else
	{
		debuggerFrame->hide();
	}
}

void MainWindow::ToggleLogFrame(bool state)
{
	if (state)
	{
		logFrame->show();
	}
	else
	{
		logFrame->hide();
	}
}

void MainWindow::ToggleGameListFrame(bool state)
{
	if (state)
	{
		gameListFrame->show();
	}
	else
	{
		gameListFrame->hide();
	}
}

void MainWindow::HideGameIcons()
{

}

void MainWindow::RefreshGameList()
{
	gameListFrame->Refresh();
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

void MainWindow::OnDebugFrameClosed()
{
	if (showDebuggerAct->isChecked())
	{
		showDebuggerAct->setChecked(false);
	}
}

void MainWindow::OnLogFrameClosed()
{
	if (showLogAct->isChecked())
	{
		showLogAct->setChecked(false);
	}
}

void MainWindow::OnGameListFrameClosed()
{
	if (showGameListAct->isChecked())
	{
		showGameListAct->setChecked(false);
	}
}

void MainWindow::OnEmuRun()
{
	sysPauseAct->setText(tr("&Pause\tCtrl + P"));
	EnableMenus(true);
}

void MainWindow::OnEmuResume()
{
	sysPauseAct->setText(tr("&Pause\tCtrl + P"));
}

void MainWindow::OnEmuPause()
{
	sysPauseAct->setText(tr("&Resume\tCtrl + E"));
}

void MainWindow::OnEmuStop()
{
	sysPauseAct->setText(Emu.IsReady() ? tr("&Start\tCtrl + E") : tr("&Resume\tCtrl + E"));
	EnableMenus(false);
	if (!Emu.GetPath().empty())
	{
		sysPauseAct->setText(tr("&Restart\tCtrl + E"));
		sysPauseAct->setEnabled(true);
	}
}

void MainWindow::OnEmuReady()
{
	sysPauseAct->setText(Emu.IsReady() ? tr("&Start\tCtrl + E") : tr("&Resume\tCtrl + E"));
	EnableMenus(true);
}

void MainWindow::EnableMenus(bool enabled)
{
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

	sysStopAct = new QAction(tr("&Stop"), this);
	sysStopAct->setShortcut(tr("Ctrl+S"));
	sysStopAct->setEnabled(false);

	sysSendOpenMenuAct = new QAction(tr("Send &open system menu cmd"), this);
	sysSendOpenMenuAct->setEnabled(false);

	sysSendExitAct = new QAction(tr("Send &exit cmd"), this);
	sysSendExitAct->setEnabled(false);

	confSettingsAct = new QAction(tr("&Settings"), this);
	requestStylesheetAct = new QAction(tr("Change Stylesheet"), this);
	confPadAct = new QAction(tr("&Keyboard Settings"), this);

	confAutopauseManagerAct = new QAction(tr("&Auto Pause Settings"), this);

	confVfsManagerAct = new QAction(tr("Virtual &File System Manager"), this);
	confVfsManagerAct->setEnabled(false);

	confVhddManagerAct = new QAction(tr("Virtual &HDD Manager"), this);
	confVhddManagerAct->setEnabled(false);

	confSavedataManagerAct = new QAction(tr("Save &Data Utility"), this);
	confSavedataManagerAct->setEnabled(false);

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

	hideGameIconsAct = new QAction(tr("&Hide Game Icons"), this);
	refreshGameListAct = new QAction(tr("&Refresh Game List"), this);

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
	connect(sysPauseAct, &QAction::triggered, this, &MainWindow::Pause);
	connect(sysStopAct, &QAction::triggered, this, &MainWindow::Stop);
	connect(sysSendOpenMenuAct, &QAction::triggered, this, &MainWindow::SendOpenSysMenu);
	connect(sysSendExitAct, &QAction::triggered, this, &MainWindow::SendExit);
	connect(confSettingsAct, &QAction::triggered, this, &MainWindow::Settings);
	connect(requestStylesheetAct, &QAction::triggered, this, &MainWindow::OpenCustomStyleSheet);
	connect(confPadAct, &QAction::triggered, this, &MainWindow::PadSettings);
	connect(confAutopauseManagerAct, &QAction::triggered, this, &MainWindow::AutoPauseSettings);
	connect(confVfsManagerAct, &QAction::triggered, this, &MainWindow::VFSManager);
	connect(confVhddManagerAct, &QAction::triggered, this, &MainWindow::VHDDManager);
	connect(confSavedataManagerAct, &QAction::triggered, this, &MainWindow::SaveData);
	connect(toolsCompilerAct, &QAction::triggered, this, &MainWindow::ELFCompiler);
	connect(toolsCgDisasmAct, &QAction::triggered, this, &MainWindow::CgDisasm);
	connect(toolsKernelExplorerAct, &QAction::triggered, this, &MainWindow::KernelExplorer);
	connect(toolsMemoryViewerAct, &QAction::triggered, this, &MainWindow::MemoryViewer);
	connect(toolsRsxDebuggerAct, &QAction::triggered, this, &MainWindow::RSXDebugger);
	connect(toolsStringSearchAct, &QAction::triggered, this, &MainWindow::StringSearch);
	connect(toolsDecryptSprxLibsAct, &QAction::triggered, this, &MainWindow::DecryptSPRXLibraries);
	connect(showDebuggerAct, &QAction::triggered, guiSettings, &GuiSettings::setDebuggerVisibility);
	connect(showDebuggerAct, &QAction::triggered, this, &MainWindow::ToggleDebugFrame);
	connect(showLogAct, &QAction::triggered, guiSettings, &GuiSettings::setLoggerVisibility);
	connect(showLogAct, &QAction::triggered, this, &MainWindow::ToggleLogFrame);
	connect(showGameListAct, &QAction::triggered, guiSettings, &GuiSettings::setGamelistVisibility);
	connect(showGameListAct, &QAction::triggered, this, &MainWindow::ToggleGameListFrame);
	connect(hideGameIconsAct, &QAction::triggered, this, &MainWindow::HideGameIcons);
	connect(refreshGameListAct, &QAction::triggered, this, &MainWindow::RefreshGameList);
	connect(aboutAct, &QAction::triggered, this, &MainWindow::About);
	connect(aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);
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
	confMenu->addAction(requestStylesheetAct);
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
	viewMenu->addSeparator();
	viewMenu->addAction(showGameListAct);
	viewMenu->addAction(hideGameIconsAct);
	viewMenu->addAction(refreshGameListAct);

	QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(aboutAct);
	helpMenu->addAction(aboutQtAct);
}

void MainWindow::CreateDockWindows()
{
	gameListFrame = new GameListFrame(this);
	gameListFrame->setObjectName("gamelist");
	debuggerFrame = new DebuggerFrame(this);
	debuggerFrame->setObjectName("debugger");
	logFrame = new LogFrame(this);
	logFrame->setObjectName("logger");

	addDockWidget(Qt::LeftDockWidgetArea, gameListFrame);
	addDockWidget(Qt::LeftDockWidgetArea, logFrame);
	addDockWidget(Qt::RightDockWidgetArea, debuggerFrame);

	connect(logFrame, &LogFrame::LogFrameClosed, this, &MainWindow::OnLogFrameClosed);
	connect(debuggerFrame, &DebuggerFrame::DebugFrameClosed, this, &MainWindow::OnDebugFrameClosed);
	connect(gameListFrame, &GameListFrame::GameListFrameClosed, this, &MainWindow::OnGameListFrameClosed);
}

void MainWindow::ConfigureGuiFromSettings()
{
	// Restore GUI state if needed. We need to if the settings are not null.
	QByteArray geometry = guiSettings->readGuiGeometry();
	bool needsDefault = false;
	if (geometry.isEmpty() == false)
	{
		restoreGeometry(geometry);
	}
	else
	{
		needsDefault = true;
	}

	QByteArray state = guiSettings->readGuiState();
	if (state.isEmpty() == false)
	{
		restoreState(state);
	}
	else
	{
		needsDefault = true;
	}

	// Handle dock widget action states.  The restore state will handle hide/show
	// Minor hack until I add a CreateConnects/Disconnect method. I want to prevent an annoying merge with megamouse.
	showLogAct->blockSignals(true);
	showGameListAct->blockSignals(true);
	showDebuggerAct->blockSignals(true);

	showLogAct->setChecked(guiSettings->getLoggerVisibility());
	showGameListAct->setChecked(guiSettings->getGameListVisibility());
	showDebuggerAct->setChecked(guiSettings->getDebuggerVisibility());

	showLogAct->blockSignals(false);
	showGameListAct->blockSignals(false);
	showDebuggerAct->blockSignals(false);

	// By default, hide the debugger and set the window to 70% of the screen.
	if (needsDefault)
	{
		// Debugger frame hidden by default.
		debuggerFrame->hide();

		QSize defaultSize = QDesktopWidget().availableGeometry().size() * 0.7;
		resize(defaultSize);
	}

	setWindowTitle(QString::fromStdString("RPCS3 v" + rpcs3::version.to_string()));
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

	// Save gui settings
	guiSettings->writeGuiGeometry(saveGeometry());
	guiSettings->writeGuiState(saveState());

	// I need the gui settings to sync, and that means having the destructor called as guiSetting's parent is mainwindow.
	setAttribute(Qt::WA_DeleteOnClose);
	QMainWindow::close();

	
	// It's possible to have other windows open, like games.  So, force the application to die.
	QApplication::quit();
}
