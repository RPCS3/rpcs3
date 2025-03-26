#pragma once

#include "settings.h"
#include "util/logs.hpp"

#include <QVariant>
#include <QSize>
#include <QColor>
#include <QMessageBox>
#include <QWindow>

namespace gui
{
	extern QString stylesheet;
	extern bool custom_stylesheet_active;

	enum custom_roles
	{
		game_role = Qt::UserRole + 1337,
	};

	enum class game_list_columns
	{
		icon,
		name,
		serial,
		firmware,
		version,
		category,
		path,
		move,
		resolution,
		sound,
		parental,
		last_play,
		playtime,
		compat,
		dir_size,

		count
	};

	enum class trophy_list_columns
	{
		icon = 0,
		name = 1,
		description = 2,
		type = 3,
		is_unlocked = 4,
		id = 5,
		platinum_link = 6,
		time_unlocked = 7,

		count
	};

	enum class trophy_game_list_columns
	{
		icon = 0,
		name = 1,
		progress = 2,
		trophies = 3,

		count
	};

	enum class savestate_game_list_columns
	{
		icon = 0,
		name = 1,
		savestates = 2,

		count
	};

	enum class savestate_list_columns
	{
		name = 0,
		compatible = 1,
		date = 2,
		path = 3,

		count
	};

	QString get_savestate_game_list_column_name(savestate_game_list_columns col);
	QString get_savestate_list_column_name(savestate_list_columns col);
	QString get_trophy_game_list_column_name(trophy_game_list_columns col);
	QString get_trophy_list_column_name(trophy_list_columns col);
	QString get_game_list_column_name(game_list_columns col);

	const QSize gl_icon_size_min    = QSize(40, 22);
	const QSize gl_icon_size_small  = QSize(80, 44);
	const QSize gl_icon_size_medium = QSize(160, 88);
	const QSize gl_icon_size_max    = QSize(320, 176);

	const int gl_max_slider_pos = 100;

	inline int get_Index(const QSize& current)
	{
		const int size_delta = gl_icon_size_max.width() - gl_icon_size_min.width();
		const int current_delta = current.width() - gl_icon_size_min.width();
		return gl_max_slider_pos * current_delta / size_delta;
	}

	inline q_string_pair Recent_Game(const QString& path, const QString& title)
	{
		return q_string_pair(path, title.simplified()); // simplified() forces single line text
	}

	const QString Settings          = "CurrentSettings";
	const QString DefaultStylesheet = "default";
	const QString NoStylesheet      = "none";
	const QString NativeStylesheet  = "native";
	const QString DarkStylesheet    = "Darker Style by TheMitoSan";

	const QString main_window  = "main_window";
	const QString game_list    = "GameList";
	const QString logger       = "Logger";
	const QString debugger     = "Debugger";
	const QString rsx          = "RSX_Debugger";
	const QString meta         = "Meta";
	const QString fs           = "FileSystem";
	const QString gs_frame     = "GSFrame";
	const QString trophy       = "Trophy";
	const QString patches      = "Patches";
	const QString localization = "Localization";
	const QString pad_settings = "PadSettings";
	const QString config       = "Config";
	const QString log_viewer   = "LogViewer";
	const QString sc           = "Shortcuts";
	const QString navigation   = "PadNavigation";
	const QString savestate    = "Savestate";

	const QString update_on   = "true";
	const QString update_off  = "false";
	const QString update_auto = "auto";
	const QString update_bkg  = "background";

	const QColor gl_icon_color = QColor(240, 240, 240, 255);

	const gui_save rg_freeze  = gui_save(main_window, "recentGamesFrozen", false);
	const gui_save rg_entries = gui_save(main_window, "recentGamesNames",  QVariant::fromValue(q_pair_list()));

	const gui_save rs_freeze  = gui_save(main_window, "recentSavestatesFrozen", false);
	const gui_save rs_entries = gui_save(main_window, "recentSavestatesNames",  QVariant::fromValue(q_pair_list()));

	const gui_save ib_skip_version = gui_save(main_window, "infoBoxSkipVersion",       "");
	const gui_save ib_pkg_success  = gui_save(main_window, "infoBoxEnabledInstallPKG", true);
	const gui_save ib_pup_success  = gui_save(main_window, "infoBoxEnabledInstallPUP", true);
	const gui_save ib_show_welcome = gui_save(main_window, "infoBoxEnabledWelcome",    true);
	const gui_save ib_confirm_exit = gui_save(main_window, "confirmationBoxExitGame",  true);
	const gui_save ib_confirm_boot = gui_save(main_window, "confirmationBoxBootGame",  true);
	const gui_save ib_obsolete_cfg = gui_save(main_window, "confirmationObsoleteCfg",  true);
	const gui_save ib_same_buttons = gui_save(main_window, "confirmationSameButtons",  true);
	const gui_save ib_restart_hint = gui_save(main_window, "confirmationRestart",      true);

	const gui_save fd_install_pkg  = gui_save(main_window, "lastExplorePathPKG",  "");
	const gui_save fd_install_pup  = gui_save(main_window, "lastExplorePathPUP",  "");
	const gui_save fd_boot_elf     = gui_save(main_window, "lastExplorePathELF",  "");
	const gui_save fd_boot_game    = gui_save(main_window, "lastExplorePathGAME", "");
	const gui_save fd_decrypt_sprx = gui_save(main_window, "lastExplorePathSPRX", "");
	const gui_save fd_cg_disasm    = gui_save(main_window, "lastExplorePathCGD",  "");
	const gui_save fd_log_viewer   = gui_save(main_window, "lastExplorePathLOG",  "");
	const gui_save fd_ext_mself    = gui_save(main_window, "lastExplorePathExMSELF",  "");
	const gui_save fd_ext_tar      = gui_save(main_window, "lastExplorePathExTAR",  "");
	const gui_save fd_insert_disc  = gui_save(main_window, "lastExplorePathDISC", "");
	const gui_save fd_cfg_check    = gui_save(main_window, "lastExplorePathCfgChk", "");
	const gui_save fd_save_elf     = gui_save(main_window, "lastExplorePathSaveElf", "");
	const gui_save fd_save_log     = gui_save(main_window, "lastExplorePathSaveLog", "");

	const gui_save mw_debugger         = gui_save(main_window, "debuggerVisible",  false);
	const gui_save mw_logger           = gui_save(main_window, "loggerVisible",    true);
	const gui_save mw_gamelist         = gui_save(main_window, "gamelistVisible",  true);
	const gui_save mw_toolBarVisible   = gui_save(main_window, "toolBarVisible",   true);
	const gui_save mw_titleBarsVisible = gui_save(main_window, "titleBarsVisible", true);
	const gui_save mw_geometry         = gui_save(main_window, "geometry",         QByteArray());
	const gui_save mw_windowState      = gui_save(main_window, "windowState",      QByteArray());
	const gui_save mw_mwState          = gui_save(main_window, "mwState",          QByteArray());

	const gui_save cat_hdd_game    = gui_save(game_list, "categoryVisibleHDDGame",    true);
	const gui_save cat_disc_game   = gui_save(game_list, "categoryVisibleDiscGame",   true);
	const gui_save cat_ps1_game    = gui_save(game_list, "categoryVisiblePS1Game",    true);
	const gui_save cat_ps2_game    = gui_save(game_list, "categoryVisiblePS2Game",    true);
	const gui_save cat_psp_game    = gui_save(game_list, "categoryVisiblePSPGame",    true);
	const gui_save cat_home        = gui_save(game_list, "categoryVisibleHome",       true);
	const gui_save cat_audio_video = gui_save(game_list, "categoryVisibleAudioVideo", true);
	const gui_save cat_game_data   = gui_save(game_list, "categoryVisibleGameData",   false);
	const gui_save cat_os          = gui_save(game_list, "categoryVisibleOS",         false);
	const gui_save cat_unknown     = gui_save(game_list, "categoryVisibleUnknown",    true);
	const gui_save cat_other       = gui_save(game_list, "categoryVisibleOther",      true);

	const gui_save grid_cat_hdd_game    = gui_save(game_list, "gridCategoryVisibleHDDGame",    true);
	const gui_save grid_cat_disc_game   = gui_save(game_list, "gridCategoryVisibleDiscGame",   true);
	const gui_save grid_cat_ps1_game    = gui_save(game_list, "gridCategoryVisiblePS1Game",    true);
	const gui_save grid_cat_ps2_game    = gui_save(game_list, "gridCategoryVisiblePS2Game",    true);
	const gui_save grid_cat_psp_game    = gui_save(game_list, "gridCategoryVisiblePSPGame",    true);
	const gui_save grid_cat_home        = gui_save(game_list, "gridCategoryVisibleHome",       true);
	const gui_save grid_cat_audio_video = gui_save(game_list, "gridCategoryVisibleAudioVideo", true);
	const gui_save grid_cat_game_data   = gui_save(game_list, "gridCategoryVisibleGameData",   false);
	const gui_save grid_cat_os          = gui_save(game_list, "gridCategoryVisibleOS",         false);
	const gui_save grid_cat_unknown     = gui_save(game_list, "gridCategoryVisibleUnknown",    true);
	const gui_save grid_cat_other       = gui_save(game_list, "gridCategoryVisibleOther",      true);

	const gui_save gl_sortAsc      = gui_save(game_list, "sortAsc",      true);
	const gui_save gl_sortCol      = gui_save(game_list, "sortCol",      1);
	const gui_save gl_state        = gui_save(game_list, "state",        QByteArray());
	const gui_save gl_iconSize     = gui_save(game_list, "iconSize",     get_Index(gl_icon_size_small));
	const gui_save gl_iconSizeGrid = gui_save(game_list, "iconSizeGrid", get_Index(gl_icon_size_small));
	const gui_save gl_iconColor    = gui_save(game_list, "iconColor",    gl_icon_color);
	const gui_save gl_listMode     = gui_save(game_list, "listMode",     true);
	const gui_save gl_textFactor   = gui_save(game_list, "textFactor",   qreal{2.0});
	const gui_save gl_marginFactor = gui_save(game_list, "marginFactor", qreal{0.09});
	const gui_save gl_show_hidden  = gui_save(game_list, "show_hidden",  false);
	const gui_save gl_hidden_list  = gui_save(game_list, "hidden_list",  QStringList());
	const gui_save gl_draw_compat  = gui_save(game_list, "draw_compat",  false);
	const gui_save gl_pref_gd_icon = gui_save(game_list, "pref_gd_icon", false);
	const gui_save gl_custom_icon  = gui_save(game_list, "custom_icon",  true);
	const gui_save gl_hover_gifs   = gui_save(game_list, "hover_gifs",   true);

	const gui_save fs_emulator_dir_list = gui_save(fs, "emulator_dir_list", QStringList());
	const gui_save fs_dev_hdd0_list     = gui_save(fs, "dev_hdd0_list",     QStringList());
	const gui_save fs_dev_hdd1_list     = gui_save(fs, "dev_hdd1_list",     QStringList());
	const gui_save fs_dev_flash_list    = gui_save(fs, "dev_flash_list",    QStringList());
	const gui_save fs_dev_flash2_list   = gui_save(fs, "dev_flash2_list",   QStringList());
	const gui_save fs_dev_flash3_list   = gui_save(fs, "dev_flash3_list",   QStringList());
	const gui_save fs_dev_bdvd_list     = gui_save(fs, "dev_bdvd_list",     QStringList());
	const gui_save fs_games_list        = gui_save(fs, "games_list",        QStringList());
	const gui_save fs_dev_usb_list      = gui_save(fs, "dev_usb00X_list",   QStringList()); // Used as a template for all usb paths

	const gui_save l_tty       = gui_save(logger, "TTY",       true);
	const gui_save l_level     = gui_save(logger, "level",     static_cast<uchar>(logs::level::success));
	const gui_save l_prefix    = gui_save(logger, "prefix_on", false);
	const gui_save l_stack_err = gui_save(logger, "ERR_stack", true);
	const gui_save l_stack     = gui_save(logger, "stack",     true);
	const gui_save l_stack_tty = gui_save(logger, "TTY_stack", false);
	const gui_save l_ansi_code = gui_save(logger, "ANSI_code", true);
	const gui_save l_limit     = gui_save(logger, "limit",     1000);
	const gui_save l_limit_tty = gui_save(logger, "TTY_limit", 1000);

	const gui_save d_splitterState = gui_save(debugger, "splitterState", QByteArray());

	const gui_save rsx_geometry = gui_save(rsx, "geometry", QByteArray());
	const gui_save rsx_states   = gui_save(rsx, "states",   QVariantMap());

#ifdef __APPLE__
	const gui_save m_currentStylesheet = gui_save(meta, "currentStylesheet", "native (macOS)");
#else
	const gui_save m_currentStylesheet = gui_save(meta, "currentStylesheet", DefaultStylesheet);
#endif
	const gui_save m_showDebugTab      = gui_save(meta, "showDebugTab",      false);
	const gui_save m_attachCommandLine = gui_save(meta, "attachCommandLine", false);
	const gui_save m_enableUIColors    = gui_save(meta, "enableUIColors",    false);
	const gui_save m_richPresence      = gui_save(meta, "useRichPresence",   true);
	const gui_save m_discordState      = gui_save(meta, "discordState",      "");
	const gui_save m_check_upd_start   = gui_save(meta, "checkUpdateStart",  update_on);

	const gui_save gs_disableMouse      = gui_save(gs_frame, "disableMouse",          false);
	const gui_save gs_disableKbHotkeys  = gui_save(gs_frame, "disableKbHotkeys",      false);
	const gui_save gs_showMouseFs       = gui_save(gs_frame, "showMouseInFullscreen", false);
	const gui_save gs_lockMouseFs       = gui_save(gs_frame, "lockMouseInFullscreen", true);
	const gui_save gs_resize            = gui_save(gs_frame, "resize",                false);
	const gui_save gs_resize_manual     = gui_save(gs_frame, "resizeManual",          true);
	const gui_save gs_screen            = gui_save(gs_frame, "screen",                0);
	const gui_save gs_width             = gui_save(gs_frame, "width",                 1280);
	const gui_save gs_height            = gui_save(gs_frame, "height",                720);
	const gui_save gs_hideMouseIdle     = gui_save(gs_frame, "hideMouseOnIdle",       false);
	const gui_save gs_hideMouseIdleTime = gui_save(gs_frame, "hideMouseOnIdleTime",   2000);
	const gui_save gs_geometry          = gui_save(gs_frame, "geometry",              QRect());
	const gui_save gs_visibility        = gui_save(gs_frame, "visibility",            QWindow::Visibility::AutomaticVisibility);

	const gui_save ss_icon_color      = gui_save(trophy, "icon_color",    gl_icon_color);
	const gui_save ss_game_icon_size  = gui_save(trophy, "game_icon_size",  25);
	const gui_save ss_geometry        = gui_save(trophy, "geometry",        QByteArray());
	const gui_save ss_splitterState   = gui_save(trophy, "splitterState",   QByteArray());
	const gui_save ss_games_state     = gui_save(trophy, "games_state",     QByteArray());
	const gui_save ss_savestate_state = gui_save(trophy, "savestate_state", QByteArray());

	const gui_save tr_icon_color    = gui_save(trophy, "icon_color",    gl_icon_color);
	const gui_save tr_icon_height   = gui_save(trophy, "icon_height",   75);
	const gui_save tr_game_iconSize = gui_save(trophy, "game_iconSize", 25);
	const gui_save tr_show_locked   = gui_save(trophy, "show_locked",   true);
	const gui_save tr_show_unlocked = gui_save(trophy, "show_unlocked", true);
	const gui_save tr_show_hidden   = gui_save(trophy, "show_hidden",   false);
	const gui_save tr_show_bronze   = gui_save(trophy, "show_bronze",   true);
	const gui_save tr_show_silver   = gui_save(trophy, "show_silver",   true);
	const gui_save tr_show_gold     = gui_save(trophy, "show_gold",     true);
	const gui_save tr_show_platinum = gui_save(trophy, "show_platinum", true);
	const gui_save tr_geometry      = gui_save(trophy, "geometry",      QByteArray());
	const gui_save tr_splitterState = gui_save(trophy, "splitterState", QByteArray());
	const gui_save tr_games_state   = gui_save(trophy, "games_state",   QByteArray());
	const gui_save tr_trophy_state  = gui_save(trophy, "trophy_state",  QByteArray());

	const gui_save pm_show_owned     = gui_save(patches, "show_owned",     false);
	const gui_save pm_geometry       = gui_save(patches, "geometry",       QByteArray());
	const gui_save pm_splitter_state = gui_save(patches, "splitter_state", QByteArray());

	const gui_save sd_geometry   = gui_save(savedata, "geometry",   QByteArray());
	const gui_save sd_icon_size  = gui_save(savedata, "icon_size",  60);
	const gui_save sd_icon_color = gui_save(savedata, "icon_color", gl_icon_color);

	const gui_save um_geometry    = gui_save(users, "geometry",    QByteArray());

	const gui_save cfg_geometry = gui_save(config, "geometry", QByteArray());

	const gui_save loc_language = gui_save(localization, "language", "en");

	const gui_save pads_show_emulated = gui_save(pad_settings, "show_emulated_values", false);
	const gui_save pads_geometry      = gui_save(pad_settings, "geometry",             QByteArray());

	const gui_save lv_show_timestamps = gui_save(log_viewer, "show_timestamps", true);
	const gui_save lv_show_threads    = gui_save(log_viewer, "show_threads",    true);
	const gui_save lv_log_levels      = gui_save(log_viewer, "log_levels",      0b11111111u);

	const gui_save sc_shortcuts = gui_save(sc, "shortcuts", QVariantMap());

	const gui_save nav_enabled = gui_save(navigation, "pad_input_enabled",      false);
	const gui_save nav_global  = gui_save(navigation, "allow_global_pad_input", false);
}

/** Class for GUI settings..
*/
class gui_settings : public settings
{
	Q_OBJECT

public:
	explicit gui_settings(QObject* parent = nullptr);

	bool GetCategoryVisibility(int cat, bool is_list_mode) const;

	void ShowConfirmationBox(const QString& title, const QString& text, const gui_save& entry, int* result, QWidget* parent);
	void ShowInfoBox(const QString& title, const QString& text, const gui_save& entry, QWidget* parent);
	bool GetBootConfirmation(QWidget* parent, const gui_save& gui_save_entry = gui_save());

	logs::level GetLogLevel() const;
	bool GetSavestateGamelistColVisibility(gui::savestate_game_list_columns col) const;
	bool GetSavestateListColVisibility(gui::savestate_list_columns col) const;
	bool GetTrophyGamelistColVisibility(gui::trophy_game_list_columns col) const;
	bool GetTrophylistColVisibility(gui::trophy_list_columns col) const;
	bool GetGamelistColVisibility(gui::game_list_columns col) const;
	QColor GetCustomColor(int col) const;
	QStringList GetStylesheetEntries() const;
	QStringList GetGameListCategoryFilters(bool is_list_mode) const;

	static QSize SizeFromSlider(int pos);

	/** Sets the visibility of the chosen category. */
	void SetCategoryVisibility(int cat, bool val, bool is_list_mode) const;

	void SetSavestateGamelistColVisibility(gui::savestate_game_list_columns col, bool val) const;
	void SetSavestateListColVisibility(gui::savestate_list_columns col, bool val) const;
	void SetTrophyGamelistColVisibility(gui::trophy_game_list_columns col, bool val) const;
	void SetTrophylistColVisibility(gui::trophy_list_columns col, bool val) const;
	void SetGamelistColVisibility(gui::game_list_columns col, bool val) const;

	void SetCustomColor(int col, const QColor& val) const;

private:
	static gui_save GetGuiSaveForSavestateGameColumn(gui::savestate_game_list_columns col);
	static gui_save GetGuiSaveForSavestateColumn(gui::savestate_list_columns col);
	static gui_save GetGuiSaveForTrophyGameColumn(gui::trophy_game_list_columns col);
	static gui_save GetGuiSaveForTrophyColumn(gui::trophy_list_columns col);
	static gui_save GetGuiSaveForGameColumn(gui::game_list_columns col);
	static gui_save GetGuiSaveForCategory(int cat, bool is_list_mode);

	void ShowBox(QMessageBox::Icon icon, const QString& title, const QString& text, const gui_save& entry, int* result, QWidget* parent, bool always_on_top);
};
