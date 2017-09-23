#pragma once

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

namespace Ui
{
	class main_window;
}

class main_window : public QMainWindow
{
	Q_OBJECT

	Ui::main_window *ui;

	bool m_sys_menu_opened;
	bool m_save_slider_pos = false;

	Render_Creator m_Render_Creator;

	QIcon m_appIcon;
	QIcon m_icon_play;
	QIcon m_icon_pause;
	QIcon m_icon_stop;
	QIcon m_icon_restart;
	QIcon m_icon_fullscreen_on;
	QIcon m_icon_fullscreen_off;

#ifdef _WIN32
	QIcon m_icon_thumb_play;
	QIcon m_icon_thumb_pause;
	QIcon m_icon_thumb_stop;
	QIcon m_icon_thumb_restart;
	QWinThumbnailToolBar *m_thumb_bar;
	QWinThumbnailToolButton *m_thumb_playPause;
	QWinThumbnailToolButton *m_thumb_stop;
	QWinThumbnailToolButton *m_thumb_restart;
	QStringList m_vulkan_adapters;
#endif
#ifdef _MSC_VER
	QStringList m_d3d12_adapters;
#endif

public:
	explicit main_window(std::shared_ptr<gui_settings> guiSettings, QWidget *parent = 0);
	void Init();
	~main_window();
	void CreateThumbnailToolbar();
	QIcon GetAppIcon();

Q_SIGNALS:
	void RequestGlobalStylesheetChange(const QString& sheetFilePath);

public Q_SLOTS:
	void OnEmuStop();
	void OnEmuRun();
	void OnEmuResume();
	void OnEmuPause();
	void OnEmuReady();

	void RepaintGui();

private Q_SLOTS:
	void BootElf();
	void BootGame();
	void DecryptSPRXLibraries();

	void SaveWindowState();
	void RepaintToolBarIcons();

protected:
	void closeEvent(QCloseEvent *event) override;
	void keyPressEvent(QKeyEvent *keyEvent) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void SetAppIconFromPath(const std::string path);
private:
	void RepaintToolbar();
	void CreateActions();
	void CreateConnects();
	void CreateDockWindows();
	void ConfigureGuiFromSettings(bool configureAll = false);
	void EnableMenus(bool enabled);
	void InstallPkg(const QString& dropPath = "");
	void InstallPup(const QString& dropPath = "");

	QAction* CreateRecentAction(const q_string_pair& entry, const uint& sc_idx);
	void BootRecentAction(const QAction* act);
	void AddRecentAction(const q_string_pair& entry);

	q_pair_list m_rg_entries;
	QList<QAction*> m_recentGameActs;

	QActionGroup* m_iconSizeActGroup;
	QActionGroup* m_listModeActGroup;
	QActionGroup* m_categoryVisibleActGroup;

	// Dockable widget frames
	QMainWindow *m_mw;
	log_frame *m_logFrame;
	debugger_frame *m_debuggerFrame;
	game_list_frame *m_gameListFrame;
	std::shared_ptr<gui_settings> guiSettings;
};
