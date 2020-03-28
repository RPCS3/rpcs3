#pragma once

#ifdef _WIN32
#include <QWinTHumbnailToolbar>
#include <QWinTHumbnailToolbutton>
#endif

#include <QActionGroup>
#include <QMainWindow>
#include <QIcon>
#include <QMimeData>

#include "update_manager.h"
#include "settings.h"

#include <memory>

class log_frame;
class debugger_frame;
class game_list_frame;
class gui_settings;
class emu_settings;
class persistent_settings;

struct gui_game_info;

enum class game_boot_result : u32;

namespace Ui
{
	class main_window;
}

class main_window : public QMainWindow
{
	Q_OBJECT

	Ui::main_window *ui;

	bool m_sys_menu_opened = false;
	bool m_is_list_mode = true;
	bool m_save_slider_pos = false;
	int m_other_slider_pos = 0;

	QIcon m_app_icon;
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
	QWinThumbnailToolBar *m_thumb_bar = nullptr;
	QWinThumbnailToolButton *m_thumb_playPause = nullptr;
	QWinThumbnailToolButton *m_thumb_stop = nullptr;
	QWinThumbnailToolButton *m_thumb_restart = nullptr;
	QStringList m_vulkan_adapters;
#endif

	enum drop_type
	{
		drop_error,
		drop_pkg,
		drop_pup,
		drop_rap,
		drop_dir,
		drop_game,
		drop_rrc
	};

public:
	explicit main_window(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, std::shared_ptr<persistent_settings> persistent_settings, QWidget *parent = 0);
	void Init();
	~main_window();
	QIcon GetAppIcon();

Q_SIGNALS:
	void RequestLanguageChange(const QString& language);
	void RequestGlobalStylesheetChange(const QString& stylesheet_path);
	void RequestTrophyManagerRepaint();
	void NotifyEmuSettingsChange();

public Q_SLOTS:
	void OnEmuStop();
	void OnEmuRun(bool start_playtime);
	void OnEmuResume();
	void OnEmuPause();
	void OnEmuReady();

	void RepaintGui();
	void RetranslateUI(const QStringList& language_codes, const QString& language);

private Q_SLOTS:
	void OnPlayOrPause();
	void Boot(const std::string& path, const std::string& title_id = "", bool direct = false, bool add_only = false, bool force_global_config = false);
	void BootElf();
	void BootGame();
	void BootRsxCapture(std::string path = "");
	void DecryptSPRXLibraries();
	void show_boot_error(game_boot_result result);

	void SaveWindowState();
	void ConfigureGuiFromSettings(bool configure_all = false);
	void SetIconSizeActions(int idx);
	void ResizeIcons(int index);

protected:
	void closeEvent(QCloseEvent *event) override;
	void keyPressEvent(QKeyEvent *keyEvent) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void dropEvent(QDropEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dragLeaveEvent(QDragLeaveEvent* event) override;

private:
	void RepaintToolBarIcons();
	void RepaintThumbnailIcons();
	void CreateActions();
	void CreateConnects();
	void CreateDockWindows();
	void EnableMenus(bool enabled);
	void ShowTitleBars(bool show);

	void InstallPackages(QStringList file_paths = QStringList(), bool show_confirm = true);
	void HandlePackageInstallation(QStringList file_paths = QStringList());

	void InstallPup(QString filePath = "");
	void HandlePupInstallation(QString file_path = "");

	int IsValidFile(const QMimeData& md, QStringList* drop_paths = nullptr);
	void AddGamesFromDir(const QString& path);

	QAction* CreateRecentAction(const q_string_pair& entry, const uint& sc_idx);
	void BootRecentAction(const QAction* act);
	void AddRecentAction(const q_string_pair& entry);

	void UpdateLanguageActions(const QStringList& language_codes, const QString& language);

	void RemoveDiskCache();

	QString GetCurrentTitle();

	q_pair_list m_rg_entries;
	QList<QAction*> m_recent_game_acts;

	std::shared_ptr<gui_game_info> m_selected_game;

	QActionGroup* m_icon_size_act_group = nullptr;
	QActionGroup* m_list_mode_act_group = nullptr;
	QActionGroup* m_category_visible_act_group = nullptr;

	// Dockable widget frames
	QMainWindow *m_mw = nullptr;
	log_frame* m_log_frame = nullptr;
	debugger_frame* m_debugger_frame = nullptr;
	game_list_frame* m_game_list_frame = nullptr;
	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<emu_settings> m_emu_settings;
	std::shared_ptr<persistent_settings> m_persistent_settings;

	update_manager m_updater;
};
