#pragma once

#include <QActionGroup>
#include <QMainWindow>
#include <QIcon>
#include <QList>
#include <QUrl>
#include <QMimeData>

#include "update_manager.h"
#include "settings.h"
#include "shortcut_handler.h"
#include "Emu/config_mode.h"

#include <memory>

class log_frame;
class debugger_frame;
class game_list_frame;
class gui_settings;
class emu_settings;
class persistent_settings;
class kernel_explorer;
class system_cmd_dialog;
class gui_pad_thread;

struct gui_game_info;

enum class game_boot_result : u32;

namespace compat
{
	struct package_info;
}

namespace Ui
{
	class main_window;
}

class main_window : public QMainWindow
{
	Q_OBJECT

	std::unique_ptr<Ui::main_window> ui;

	bool m_is_list_mode = true;
	bool m_save_slider_pos = false;
	bool m_requested_show_logs_on_exit = false;
	int m_other_slider_pos = 0;

	QIcon m_app_icon;
	QIcon m_icon_play;
	QIcon m_icon_pause;
	QIcon m_icon_restart;
	QIcon m_icon_fullscreen_on;
	QIcon m_icon_fullscreen_off;

	enum class drop_type
	{
		drop_error,
		drop_rap_edat_pkg,
		drop_pup,
		drop_psf,
		drop_dir,
		drop_game,
		drop_rrc
	};

public:
	explicit main_window(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, std::shared_ptr<persistent_settings> persistent_settings, QWidget *parent = nullptr);
	~main_window();
	bool Init(bool with_cli_boot);
	QIcon GetAppIcon() const;
	void OnMissingFw();
	bool InstallPackages(QStringList file_paths = {}, bool from_boot = false);
	void InstallPup(QString file_path = "");

Q_SIGNALS:
	void RequestLanguageChange(const QString& language);
	void RequestGlobalStylesheetChange();
	void RequestDialogRepaint();
	void NotifyEmuSettingsChange();
	void NotifyWindowCloseEvent(bool closed);
	void NotifyShortcutHandlers();

public Q_SLOTS:
	void OnEmuStop();
	void OnEmuRun(bool start_playtime);
	void OnEmuResume() const;
	void OnEmuPause() const;
	void OnEmuReady() const;
	void OnEnableDiscEject(bool enabled) const;
	void OnEnableDiscInsert(bool enabled) const;
	void OnAddBreakpoint(u32 addr) const;

	void RepaintGui();
	void RetranslateUI(const QStringList& language_codes, const QString& language_code);

private Q_SLOTS:
	void OnPlayOrPause();
	void Boot(const std::string& path, const std::string& title_id = "", bool direct = false, bool refresh_list = false, cfg_mode config_mode = cfg_mode::custom, const std::string& config_path = "");
	void BootElf();
	void BootTest();
	void BootGame();
	void BootVSH();
	void BootSavestate();
	void BootRsxCapture(std::string path = "");
	void DecryptSPRXLibraries();
	void show_boot_error(game_boot_result status);

	void SaveWindowState() const;
	void SetIconSizeActions(int idx) const;
	void ResizeIcons(int index);

	void RemoveHDD1Caches();
	void RemoveAllCaches();
	void RemoveSavestates();
	void CleanUpGameList();

	void RemoveFirmwareCache();
	void CreateFirmwareCache();

	void handle_shortcut(gui::shortcuts::shortcut shortcut_key, const QKeySequence& key_sequence);
	void update_gui_pad_thread();

protected:
	void closeEvent(QCloseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void dropEvent(QDropEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dragLeaveEvent(QDragLeaveEvent* event) override;

private:
	void ConfigureGuiFromSettings();
	void RepaintToolBarIcons();
	void RepaintThumbnailIcons();
	void CreateActions();
	void CreateConnects();
	void CreateDockWindows();
	void EnableMenus(bool enabled) const;
	void ShowTitleBars(bool show) const;
	void ShowOptionalGamePreparations(const QString& title, const QString& message, std::map<std::string, QString> game_path);

	static bool InstallFileInExData(const std::string& extension, const QString& path, const std::string& filename);

	bool HandlePackageInstallation(QStringList file_paths, bool from_boot);

	void HandlePupInstallation(const QString& file_path, const QString& dir_path = "");
	void ExtractPup();

	void ExtractTar();
	void ExtractMSELF();

	QList<QUrl> m_drop_file_url_list;
	u64 m_drop_file_timestamp = umax;
	drop_type m_drop_file_cached_drop_type = drop_type::drop_error;
	drop_type IsValidFile(const QMimeData& md, QStringList* drop_paths = nullptr);
	void AddGamesFromDirs(QStringList&& paths);

	QAction* CreateRecentAction(const q_string_pair& entry, u32 sc_idx, bool is_savestate);
	void BootRecentAction(const QAction* act, bool is_savestate);
	void AddRecentAction(const q_string_pair& entry, bool is_savestate);

	void UpdateLanguageActions(const QStringList& language_codes, const QString& language);
	void UpdateFilterActions();

	static QString GetCurrentTitle();

	struct recent_game_wrapper
	{
		q_pair_list entries;
		QList<QAction*> actions;
	};
	recent_game_wrapper m_recent_game {};
	recent_game_wrapper m_recent_save {};

	std::shared_ptr<gui_game_info> m_selected_game;

	QActionGroup* m_icon_size_act_group = nullptr;
	QActionGroup* m_list_mode_act_group = nullptr;
	QActionGroup* m_category_visible_act_group = nullptr;

	// Dockable widget frames
	QMainWindow *m_mw = nullptr;
	log_frame* m_log_frame = nullptr;
	debugger_frame* m_debugger_frame = nullptr;
	game_list_frame* m_game_list_frame = nullptr;
	kernel_explorer* m_kernel_explorer = nullptr;
	system_cmd_dialog* m_system_cmd_dialog = nullptr;
	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<emu_settings> m_emu_settings;
	std::shared_ptr<persistent_settings> m_persistent_settings;

	update_manager m_updater;
	QAction* m_download_menu_action = nullptr;

	shortcut_handler* m_shortcut_handler = nullptr;

	std::unique_ptr<gui_pad_thread> m_gui_pad_thread;
};
