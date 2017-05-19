#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#ifdef _WIN32
#include <QWinTaskbarProgress>
#include <QWinTaskbarButton>
#include <QWinTHumbnailToolbar>
#include <QWinTHumbnailToolbutton>
#endif

#include <QMainWindow>
#include <QPushButton>
#include <QIcon>

#include "logframe.h"
#include "debuggerframe.h"
#include "GameListFrame.h"
#include "GuiSettings.h"

#include <memory>

class MainWindow : public QMainWindow
{
	Q_OBJECT

	bool m_sys_menu_opened;

	QWidget* controls;

	QIcon appIcon;
	QIcon icon_play;
	QIcon icon_pause;
	QIcon icon_stop;
	QIcon icon_restart;

	QPushButton* menu_run;
	QPushButton* menu_stop;
	QPushButton* menu_restart;
	QPushButton* menu_capture_frame;

#ifdef _WIN32
	QWinThumbnailToolBar *thumb_bar;
	QWinThumbnailToolButton *thumb_playPause;
	QWinThumbnailToolButton *thumb_stop;
	QWinThumbnailToolButton *thumb_restart;
#endif

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	void CreateThumbnailToolbar();
	QIcon GetAppIcon();

signals:
	void RequestGlobalStylesheetChange(const QString& sheetFilePath);

public slots:
	void OnEmuStop();
	void OnEmuRun();
	void OnEmuResume();
	void OnEmuPause();
	void OnEmuReady();
	void OnCaptureFrame();

private slots:
	void BootElf();
	void BootGame();
	void InstallPkg();
	void InstallPup();
	void Pause();
	void Stop();
	void Restart();
	void SendOpenSysMenu();
	void SendExit();
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
	void RSXDebuggerFrame();
	void StringSearch();
	void DecryptSPRXLibraries();
	void ToggleDebugFrame(bool state);
	void ToggleLogFrame(bool state);
	void ToggleGameListFrame(bool state);
	void ToggleControls(bool state);
	void RefreshGameList();
	void ToggleHDDGameCategory(bool state);
	void ToggleDiscGameCategory(bool state);
	void ToggleHomeCategory(bool state);
	void ToggleAudioVideoCategory(bool state);
	void ToggleGameDataCategory(bool state);
	void About();

	void OnDebugFrameClosed();
	void OnLogFrameClosed();
	void OnGameListFrameClosed();

	void SaveWindowState();

protected:
	void closeEvent(QCloseEvent *event) override;
	void SetAppIconFromPath(const std::string path);
private:
	void LoadIcons();
	void CreateActions();
	void CreateConnects();
	void CreateMenus();
	void CreateDockWindows();
	void ConfigureGuiFromSettings(bool configureAll = false);
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
	QAction *showControlsAct;
	QAction *refreshGameListAct;
	QAction* showCatHDDGameAct;
	QAction* showCatDiscGameAct;
	QAction* showCatHomeAct;
	QAction* showCatAudioVideoAct;
	QAction* showCatGameDataAct;
	QAction *aboutAct;
	QAction *aboutQtAct;

	// Dockable widget frames
	LogFrame *logFrame;
	DebuggerFrame *debuggerFrame;
	GameListFrame *gameListFrame;
	std::shared_ptr<GuiSettings> guiSettings;
};

#endif // MAINWINDOW_H
