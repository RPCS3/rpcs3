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

#include "log_frame.h"
#include "debugger_frame.h"
#include "game_list_frame.h"
#include "gui_settings.h"

#include <memory>

class main_window : public QMainWindow
{
	Q_OBJECT

	bool m_sys_menu_opened;

	Render_Creator m_Render_Creator;

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
	QStringList m_vulkan_adapters;
#endif
#ifdef _MSC_VER
	QStringList m_d3d12_adapters;
#endif

public:
	explicit main_window(QWidget *parent = 0);
	~main_window();
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

private slots:
	void BootElf();
	void BootGame();
	void InstallPkg();
	void InstallPup();
	void DecryptSPRXLibraries();
	void About();

	void SaveWindowState();

protected:
	void closeEvent(QCloseEvent *event) override;
	void SetAppIconFromPath(const std::string path);
private:
	void CreateActions();
	void CreateConnects();
	void CreateMenus();
	void CreateDockWindows();
	void ConfigureGuiFromSettings(bool configureAll = false);
	void EnableMenus(bool enabled);
	void keyPressEvent(QKeyEvent *keyEvent);

	QAction* CreateRecentAction(const q_string_pair& entry, const uint& sc_idx);
	void BootRecentAction(const QAction* act);
	void AddRecentAction(const q_string_pair& entry);

	q_pair_list m_rg_entries;
	QMenu* m_bootRecentMenu;
	QList<QAction*> m_recentGameActs;

	QActionGroup* iconSizeActGroup;
	QActionGroup* listModeActGroup;
	QActionGroup* categoryVisibleActGroup;

	QAction *bootElfAct;
	QAction *bootGameAct;
	QAction *clearRecentAct;
	QAction *freezeRecentAct;
	QAction *bootInstallPkgAct;
	QAction *bootInstallPupAct;
	QAction *sysPauseAct;
	QAction *sysStopAct;
	QAction *sysSendOpenMenuAct;
	QAction *sysSendExitAct;
	QAction *confSettingsAct;
	QAction *confPadAct;
	QAction *confAutopauseManagerAct;
	QAction *confSavedataManagerAct;
	QAction *toolsCgDisasmAct;
	QAction *toolskernel_explorerAct;
	QAction *toolsmemory_viewerAct;
	QAction *toolsRsxDebuggerAct;
	QAction *toolsStringSearchAct;
	QAction *toolsDecryptSprxLibsAct;
	QAction *exitAct;
	QAction *showDebuggerAct;
	QAction *showLogAct;
	QAction *showGameListAct;
	QAction *showControlsAct;
	QAction *refreshGameListAct;
	QAction *showGameListToolBarAct;
	QAction* showCatHDDGameAct;
	QAction* showCatDiscGameAct;
	QAction* showCatHomeAct;
	QAction* showCatAudioVideoAct;
	QAction* showCatGameDataAct;
	QAction* showCatUnknownAct;
	QAction* setIconSizeTinyAct;
	QAction* setIconSizeSmallAct;
	QAction* setIconSizeMediumAct;
	QAction* setIconSizeLargeAct;
	QAction* setlistModeListAct;
	QAction* setlistModeGridAct;
	QAction *aboutAct;
	QAction *aboutQtAct;

	// Dockable widget frames
	log_frame *logFrame;
	debugger_frame *debuggerFrame;
	game_list_frame *gameListFrame;
	std::shared_ptr<gui_settings> guiSettings;
};

#endif // MAINWINDOW_H
