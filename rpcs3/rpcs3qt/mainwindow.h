#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "logframe.h"
#include "debuggerframe.h"
#include "GameListFrame.h"
#include "GuiSettings.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

	bool m_sys_menu_opened;

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

signals:
	void RequestGlobalStylesheetChange(const QString& sheetFilePath);

public slots:
	void OnEmuStop();
	void OnEmuRun();
	void OnEmuResume();
	void OnEmuPause();
	void OnEmuReady();

private slots:
	void BootElf();
	void BootGame();
	void InstallPkg();
	void InstallPup();
	void Pause();
	void Stop();
	void SendOpenSysMenu();
	void SendExit();
	void OpenCustomStyleSheet();
	void Settings();
	void PadSettings();
	void AutoPauseSettings();
	void VFSManager();
	void VHDDManager();
	void SaveData();
	void ELFCompiler();
	void CgDisasm();
	void KernelExploration();
	void MemoryViewer();
	void RSXDebugger();
	void StringSearch();
	void DecryptSPRXLibraries();
	void ToggleDebugFrame(bool state);
	void ToggleLogFrame(bool state);
	void ToggleGameListFrame(bool state);
	void HideGameIcons();
	void RefreshGameList();
	void About();

	void OnDebugFrameClosed();
	void OnLogFrameClosed();
	void OnGameListFrameClosed();

protected:
	void MainWindow::closeEvent(QCloseEvent *event) override;
private:
	void CreateActions();
	void CreateConnects();
	void CreateMenus();
	void CreateDockWindows();
	void ConfigureGuiFromSettings();
	void DoSettings(bool load);
	void EnableMenus(bool enabled);
	void keyPressEvent(QKeyEvent *keyEvent);

	QAction *bootElfAct;
	QAction *bootGameAct;
	QAction *bootInstallPkgAct;
	QAction *bootInstallPupAct;
	QAction *sysPauseAct;
	QAction *sysStopAct;
	QAction *sysSendOpenMenuAct;
	QAction *sysSendExitAct;
	QAction *requestStylesheetAct;
	QAction *confSettingsAct;
	QAction *confPadAct;
	QAction *confAutopauseManagerAct;
	QAction *confVfsManagerAct;
	QAction *confVhddManagerAct;
	QAction *confSavedataManagerAct;
	QAction *toolsCompilerAct;
	QAction *toolsCgDisasmAct;
	QAction *toolsKernelExplorerAct;
	QAction *toolsMemoryViewerAct;
	QAction *toolsRsxDebuggerAct;
	QAction *toolsStringSearchAct;
	QAction *toolsDecryptSprxLibsAct;
	QAction *exitAct;
	QAction *showDebuggerAct;
	QAction *showLogAct;
	QAction *showGameListAct;
	QAction *hideGameIconsAct;
	QAction *refreshGameListAct;
	QAction *aboutAct;
	QAction *aboutQtAct;

	// Dockable widget frames
	LogFrame *logFrame;
	DebuggerFrame *debuggerFrame;
	GameListFrame *gameListFrame;
	GuiSettings *guiSettings;
};

#endif // MAINWINDOW_H
