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

namespace Ui {
	class main_window;
}

class main_window : public QMainWindow
{
	Q_OBJECT

	Ui::main_window *ui;

	bool m_sys_menu_opened;

	Render_Creator m_Render_Creator;

	QIcon appIcon;
	QIcon icon_play;
	QIcon icon_pause;
	QIcon icon_stop;
	QIcon icon_restart;

#ifdef _WIN32
	QIcon icon_thumb_play;
	QIcon icon_thumb_pause;
	QIcon icon_thumb_stop;
	QIcon icon_thumb_restart;
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

Q_SIGNALS:
	void RequestGlobalStylesheetChange(const QString& sheetFilePath);

public Q_SLOTS:
	void OnEmuStop();
	void OnEmuRun();
	void OnEmuResume();
	void OnEmuPause();
	void OnEmuReady();

private Q_SLOTS:
	void BootElf();
	void BootGame();
	void InstallPkg();
	void InstallPup();
	void DecryptSPRXLibraries();

	void SaveWindowState();

protected:
	void closeEvent(QCloseEvent *event) override;
	void SetAppIconFromPath(const std::string path);
private:
	void CreateActions();
	void CreateConnects();
	void CreateDockWindows();
	void ConfigureGuiFromSettings(bool configureAll = false);
	void EnableMenus(bool enabled);
	void keyPressEvent(QKeyEvent *keyEvent);
	void mouseDoubleClickEvent(QMouseEvent *event);

	QAction* CreateRecentAction(const q_string_pair& entry, const uint& sc_idx);
	void BootRecentAction(const QAction* act);
	void AddRecentAction(const q_string_pair& entry);

	q_pair_list m_rg_entries;
	QList<QAction*> m_recentGameActs;

	QActionGroup* iconSizeActGroup;
	QActionGroup* listModeActGroup;
	QActionGroup* categoryVisibleActGroup;

	// Dockable widget frames
	QMainWindow *m_mw;
	log_frame *logFrame;
	debugger_frame *debuggerFrame;
	game_list_frame *gameListFrame;
	std::shared_ptr<gui_settings> guiSettings;
};
