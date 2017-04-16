#ifdef QT_UI

#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QVBoxLayout>
#include <QDockWidget>

#include "gamelistframe.h"
#include "debuggerframe.h"
#include "logframe.h"
#include "settingsdialog.h"
#include "padsettingsdialog.h"
#include "AutoPauseSettingsDialog.h"
#include "mainwindow.h"
#include "rpcs3_version.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	CreateActions();
	CreateMenus();
	CreateDockWindows();

	setWindowTitle(QString::fromStdString("RPCS3 v" + rpcs3::version.to_string()));
}

MainWindow::~MainWindow()
{
}

void MainWindow::BootElf()
{
	bool stopped = false;

	QFileDialog dlg(this, "Select (S)ELF", "", "(S)ELF files (*BOOT.BIN *.elf *.self);;"
											   "ELF files (BOOT.BIN *.elf);;"
											   "SELF files (EBOOT.BIN *.self);;"
											   "BOOT files (*BOOT.BIN);;"
											   "BIN files (*.bin);;"
											   "All files (*.*)");
	dlg.setAcceptMode(QFileDialog::AcceptOpen);
	dlg.setFileMode(QFileDialog::ExistingFile);

	if (dlg.exec() == QDialog::Rejected)
	{
		qDebug() << "Rejected!";
		return;
	}
}

void MainWindow::BootGame()
{
	bool stopped = false;

	QFileDialog dlg(this, "Select game folder");
	dlg.setAcceptMode(QFileDialog::AcceptOpen);
	dlg.setFileMode(QFileDialog::Directory);
	dlg.setOptions(QFileDialog::ShowDirsOnly);

	if (dlg.exec() == QDialog::Rejected)
	{
		qDebug() << "Rejected!";
		return;
	}
}

void MainWindow::InstallPkg()
{
	QFileDialog dlg(this, "Select PKG", "", "PKG files (*.pkg);;All files (*.*)");
	dlg.setAcceptMode(QFileDialog::AcceptOpen);
	dlg.setFileMode(QFileDialog::ExistingFile);

	if (dlg.exec() == QDialog::Rejected)
	{
		qDebug() << "Rejected!";
		return;
	}

	QString dir = dlg.getOpenFileName();
	qDebug(qUtf8Printable(dir));

}

void MainWindow::InstallPup()
{
	QFileDialog dlg(this, "Select PUP", "", "PUP files (*.pup);;All files (*.*)");
	dlg.setAcceptMode(QFileDialog::AcceptOpen);
	dlg.setFileMode(QFileDialog::ExistingFile);

	if (dlg.exec() == QDialog::Rejected)
	{
		qDebug() << "Rejected!";
		return;
	}

	QString dir = dlg.getOpenFileName();
	qDebug(qUtf8Printable(dir));
}

void MainWindow::Pause()
{
	qDebug() << "MainWindow::Pause()";
}

void MainWindow::Stop()
{
	qDebug() << "MainWindow::Stop()";
}

void MainWindow::SendOpenSysMenu()
{
	qDebug() << "MainWindow::SendOpenSysMenu()";
}

void MainWindow::SendExit()
{
	qDebug() << "MainWindow::SendExit()";
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

void MainWindow::VFSManager()
{
	qDebug() << "MainWindow::VFSManager()";
}

void MainWindow::VHDDManager()
{
	qDebug() << "MainWindow::VHDDManager()";
}

void MainWindow::SaveData()
{
	qDebug() << "MainWindow::SaveData()";
}

void MainWindow::ELFCompiler()
{
	qDebug() << "MainWindow::ELFCompiler()";
}

void MainWindow::CgDisasm()
{
	qDebug() << "MainWindow::CgDisasm()";
}

void MainWindow::KernelExplorer()
{
	qDebug() << "MainWindow::KernelExplorer()";
}

void MainWindow::MemoryViewer()
{
	qDebug() << "MainWindow::MemoryViewer()";
}

void MainWindow::RSXDebugger()
{
	qDebug() << "MainWindow::RSXDebugger()";
}

void MainWindow::StringSearch()
{
	qDebug() << "MainWindow::StringSearch()";
}

void MainWindow::DecryptSPRXLibraries()
{
	QFileDialog dlg(this, "Select SPRX files", "", "SPRX files (*.sprx)");
	dlg.setAcceptMode(QFileDialog::AcceptOpen);
	dlg.setFileMode(QFileDialog::ExistingFiles);

	if (dlg.exec() == QDialog::Rejected)
	{
		return;
	}

	QStringList modules = dlg.selectedFiles();

	qDebug() << "Decrypting SPRX libraries...";

	for (QString& module : modules)
	{
		qDebug() << module;
	}

	qDebug() << "Finished decrypting all SPRX libraries.";
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
	qDebug() << "MainWindow::HideGameIcons()";
}

void MainWindow::RefreshGameList()
{
	qDebug() << "MainWindow::RefreshGameList()";
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
	about.setStyleSheet("QLabel{min-width: 500px;}");	// ¯\_(ツ)_/¯
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

void MainWindow::CreateActions()
{
	bootElfAct = new QAction(tr("Boot (S)ELF file"), this);
	connect(bootElfAct, &QAction::triggered, this, &MainWindow::BootElf);

	bootGameAct = new QAction(tr("Boot &Game"), this);
	connect(bootGameAct, &QAction::triggered, this, &MainWindow::BootGame);

	bootInstallPkgAct = new QAction(tr("&Install PKG"), this);
	connect(bootInstallPkgAct, &QAction::triggered, this, &MainWindow::InstallPkg);

	bootInstallPupAct = new QAction(tr("&Install Firmware"), this);
	connect(bootInstallPupAct, &QAction::triggered, this, &MainWindow::InstallPkg);

	exitAct = new QAction(tr("E&xit"), this);
	exitAct->setShortcuts(QKeySequence::Quit);
	exitAct->setStatusTip(tr("Exit the application"));
	connect(exitAct, &QAction::triggered, this, &QWidget::close);

	sysPauseAct = new QAction(tr("&Pause"), this);
	sysPauseAct->setEnabled(false);
	connect(sysPauseAct, &QAction::triggered, this, &MainWindow::Pause);

	sysStopAct = new QAction(tr("&Stop"), this);
	sysStopAct->setShortcut(tr("Ctrl+S"));
	sysStopAct->setEnabled(false);
	connect(sysStopAct, &QAction::triggered, this, &MainWindow::Stop);

	sysSendOpenMenuAct = new QAction(tr("Send &open system menu cmd"), this);
	sysSendOpenMenuAct->setEnabled(false);
	connect(sysSendOpenMenuAct, &QAction::triggered, this, &MainWindow::SendOpenSysMenu);

	sysSendExitAct = new QAction(tr("Send &exit cmd"), this);
	sysSendExitAct->setEnabled(false);
	connect(sysSendExitAct, &QAction::triggered, this, &MainWindow::SendExit);

	confSettingsAct = new QAction(tr("&Settings"), this);
	connect(confSettingsAct, &QAction::triggered, this, &MainWindow::Settings);

	confPadAct = new QAction(tr("&PAD Settings"), this);
	connect(confPadAct, &QAction::triggered, this, &MainWindow::PadSettings);

	confAutopauseManagerAct = new QAction(tr("&Auto Pause Settings"), this);
	connect(confAutopauseManagerAct, &QAction::triggered, this, &MainWindow::AutoPauseSettings);

	confVfsManagerAct = new QAction(tr("Virtual &File System Manager"), this);
	confVfsManagerAct->setEnabled(false);
	connect(confVfsManagerAct, &QAction::triggered, this, &MainWindow::VFSManager);

	confVhddManagerAct = new QAction(tr("Virtual &HDD Manager"), this);
	confVhddManagerAct->setEnabled(false);
	connect(confVhddManagerAct, &QAction::triggered, this, &MainWindow::VHDDManager);

	confSavedataManagerAct = new QAction(tr("Save &Data Utility"), this);
	confSavedataManagerAct->setEnabled(false);
	connect(confSavedataManagerAct, &QAction::triggered, this, &MainWindow::SaveData);

	toolsCompilerAct = new QAction(tr("&ELF Compiler"), this);
	toolsCompilerAct->setEnabled(false);
	connect(toolsCompilerAct, &QAction::triggered, this, &MainWindow::ELFCompiler);

	toolsCgDisasmAct = new QAction(tr("&Cg Disasm"), this);
	toolsCgDisasmAct->setEnabled(false);
	connect(toolsCgDisasmAct, &QAction::triggered, this, &MainWindow::CgDisasm);

	toolsKernelExplorerAct = new QAction(tr("&Kernel Explorer"), this);
	toolsKernelExplorerAct->setEnabled(false);
	connect(toolsKernelExplorerAct, &QAction::triggered, this, &MainWindow::KernelExplorer);

	toolsMemoryViewerAct = new QAction(tr("&Memory Viewer"), this);
	toolsMemoryViewerAct->setEnabled(false);
	connect(toolsMemoryViewerAct, &QAction::triggered, this, &MainWindow::MemoryViewer);

	toolsRsxDebuggerAct = new QAction(tr("&RSX Debugger"), this);
	toolsRsxDebuggerAct->setEnabled(false);
	connect(toolsRsxDebuggerAct, &QAction::triggered, this, &MainWindow::RSXDebugger);

	toolsStringSearchAct = new QAction(tr("&String Search"), this);
	toolsStringSearchAct->setEnabled(false);
	connect(toolsStringSearchAct, &QAction::triggered, this, &MainWindow::StringSearch);

	toolsSecryptSprxLibsAct = new QAction(tr("&Decrypt SPRX libraries"), this);
	connect(toolsSecryptSprxLibsAct, &QAction::triggered, this, &MainWindow::DecryptSPRXLibraries);

	showDebuggerAct = new QAction(tr("Show Debugger"), this);
	showDebuggerAct->setCheckable(true);
	showDebuggerAct->setChecked(false);
	connect(showDebuggerAct, &QAction::triggered, this, &MainWindow::ToggleDebugFrame);

	showLogAct = new QAction(tr("Show Log/TTY"), this);
	showLogAct->setCheckable(true);
	showLogAct->setChecked(true);
	connect(showLogAct, &QAction::triggered, this, &MainWindow::ToggleLogFrame);

	showGameListAct = new QAction(tr("Show GameList"), this);
	showGameListAct->setCheckable(true);
	showGameListAct->setChecked(true);

	connect(showGameListAct, &QAction::triggered, this, &MainWindow::ToggleGameListFrame);

	hideGameIconsAct = new QAction(tr("&Hide Game Icons"), this);
	connect(hideGameIconsAct, &QAction::triggered, this, &MainWindow::HideGameIcons);

	refreshGameListAct = new QAction(tr("&Refresh Game List"), this);
	connect(refreshGameListAct, &QAction::triggered, this, &MainWindow::RefreshGameList);

	aboutAct = new QAction(tr("&About"), this);
	aboutAct->setStatusTip(tr("Show the application's About box"));
	connect(aboutAct, &QAction::triggered, this, &MainWindow::About);

	aboutQtAct = new QAction(tr("About &Qt"), this);
	aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
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
	toolsMenu->addAction(toolsSecryptSprxLibsAct);

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
	debuggerFrame = new DebuggerFrame(this);
	logFrame = new LogFrame(this);

	addDockWidget(Qt::LeftDockWidgetArea, gameListFrame);
	addDockWidget(Qt::LeftDockWidgetArea, logFrame);
	addDockWidget(Qt::RightDockWidgetArea, debuggerFrame);

	debuggerFrame->hide();

	connect(logFrame, &LogFrame::LogFrameClosed, this, &MainWindow::OnLogFrameClosed);
	connect(debuggerFrame, &DebuggerFrame::DebugFrameClosed, this, &MainWindow::OnDebugFrameClosed);
	connect(gameListFrame, &GameListFrame::GameListFrameClosed, this, &MainWindow::OnGameListFrameClosed);
}

#endif // QT_UI
