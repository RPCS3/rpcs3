#pragma once

#include "Utilities/Log.h"

#include <QSettings>
#include <QDir>
#include <QVariant>
#include <QSize>
#include <QColor>
#include <QBitmap>
#include <QLabel>

struct GUI_SAVE
{
	QString key;
	QString name;
	QVariant def;

	GUI_SAVE() {
		key = "";
		name = "";
		def = QVariant();
	};
	GUI_SAVE(const QString& k, const QString& n, const QVariant& d) {
		key = k;
		name = n;
		def = d;
	};
};

typedef std::map<std::string, const QString> q_from_char;
typedef QPair<QString, QString> q_string_pair;
typedef QPair<QString, QSize> q_size_pair;
typedef QList<q_string_pair> q_pair_list;
typedef QList<q_size_pair> q_size_list;

namespace GUI
{
	static QString stylesheet;

	const QSize gl_icon_size_min    = QSize(40, 22);
	const QSize gl_icon_size_small  = QSize(80, 44);
	const QSize gl_icon_size_medium = QSize(160, 88);
	const QSize gl_icon_size_max    = QSize(320, 176);

	const int gl_max_slider_pos = 100;

	inline int get_Index(const QSize& current)
	{
		int size_delta = gl_icon_size_max.width() - gl_icon_size_min.width();
		int current_delta = current.width() - gl_icon_size_min.width();
		return gl_max_slider_pos * current_delta / size_delta;
	};

	inline QColor get_Label_Color(const QString& objectName, QPalette::ColorRole colorRole = QPalette::Foreground)
	{
		QLabel dummy_color;
		dummy_color.setObjectName(objectName);
		dummy_color.ensurePolished();
		return dummy_color.palette().color(colorRole);
	};

	const QString Default     = QObject::tr("default");
	const QString main_window = "main_window";
	const QString game_list   = "GameList";
	const QString logger      = "Logger";
	const QString debugger    = "Debugger";
	const QString meta        = "Meta";
	const QString fs          = "FileSystem";
	const QString gs_frame    = "GSFrame";

	const QColor mw_tool_bar_color  = QColor(227, 227, 227, 255);
	const QColor mw_tool_icon_color = QColor(64, 64, 64, 255);
	const QColor gl_icon_color      = QColor(209, 209, 209, 255);
	const QColor gl_tool_icon_color = QColor(0, 100, 231, 255);

	const GUI_SAVE rg_freeze  = GUI_SAVE(main_window, "recentGamesFrozen", false);
	const GUI_SAVE rg_entries = GUI_SAVE(main_window, "recentGamesNames", QVariant::fromValue(q_pair_list()));

	const GUI_SAVE ib_pkg_success  = GUI_SAVE( main_window, "infoBoxEnabledInstallPKG", true );
	const GUI_SAVE ib_pup_success  = GUI_SAVE( main_window, "infoBoxEnabledInstallPUP", true );
	const GUI_SAVE ib_show_welcome = GUI_SAVE( main_window, "infoBoxEnabledWelcome",    true );

	const GUI_SAVE fd_install_pkg  = GUI_SAVE( main_window, "lastExplorePathPKG",  "" );
	const GUI_SAVE fd_install_pup  = GUI_SAVE( main_window, "lastExplorePathPUP",  "" );
	const GUI_SAVE fd_boot_elf     = GUI_SAVE( main_window, "lastExplorePathELF",  "" );
	const GUI_SAVE fd_boot_game    = GUI_SAVE( main_window, "lastExplorePathGAME", "" );
	const GUI_SAVE fd_decrypt_sprx = GUI_SAVE( main_window, "lastExplorePathSPRX", "" );
	const GUI_SAVE fd_cg_disasm    = GUI_SAVE( main_window, "lastExplorePathCGD",  "" );

	const GUI_SAVE mw_debugger       = GUI_SAVE( main_window, "debuggerVisible", false );
	const GUI_SAVE mw_logger         = GUI_SAVE( main_window, "loggerVisible",   true );
	const GUI_SAVE mw_gamelist       = GUI_SAVE( main_window, "gamelistVisible", true );
	const GUI_SAVE mw_toolBarVisible = GUI_SAVE( main_window, "toolBarVisible",  true );
	const GUI_SAVE mw_toolBarColor   = GUI_SAVE( main_window, "toolBarColor",    mw_tool_bar_color);
	const GUI_SAVE mw_toolIconColor  = GUI_SAVE( main_window, "toolIconColor",   mw_tool_icon_color);
	const GUI_SAVE mw_geometry       = GUI_SAVE( main_window, "geometry",        QByteArray() );
	const GUI_SAVE mw_windowState    = GUI_SAVE( main_window, "windowState",     QByteArray() );
	const GUI_SAVE mw_mwState        = GUI_SAVE( main_window, "wwState",         QByteArray() );

	const GUI_SAVE cat_hdd_game    = GUI_SAVE( game_list, "categoryVisibleHDDGame",    true );
	const GUI_SAVE cat_disc_game   = GUI_SAVE( game_list, "categoryVisibleDiscGame",   true );
	const GUI_SAVE cat_home        = GUI_SAVE( game_list, "categoryVisibleHome",       true );
	const GUI_SAVE cat_audio_video = GUI_SAVE( game_list, "categoryVisibleAudioVideo", true );
	const GUI_SAVE cat_game_data   = GUI_SAVE( game_list, "categoryVisibleGameData",   false );
	const GUI_SAVE cat_unknown     = GUI_SAVE( game_list, "categoryVisibleUnknown",    true );
	const GUI_SAVE cat_other       = GUI_SAVE( game_list, "categoryVisibleOther",      true );

	const GUI_SAVE gl_sortAsc        = GUI_SAVE( game_list, "sortAsc",        true );
	const GUI_SAVE gl_sortCol        = GUI_SAVE( game_list, "sortCol",        1 );
	const GUI_SAVE gl_state          = GUI_SAVE( game_list, "state",          QByteArray() );
	const GUI_SAVE gl_iconSize       = GUI_SAVE( game_list, "iconSize",       get_Index(gl_icon_size_small));
	const GUI_SAVE gl_iconColor      = GUI_SAVE( game_list, "iconColor",      gl_icon_color);
	const GUI_SAVE gl_listMode       = GUI_SAVE( game_list, "listMode",       true );
	const GUI_SAVE gl_textFactor     = GUI_SAVE( game_list, "textFactor",     (qreal) 2.0 );
	const GUI_SAVE gl_marginFactor   = GUI_SAVE( game_list, "marginFactor",   (qreal) 0.09 );
	const GUI_SAVE gl_toolBarVisible = GUI_SAVE( game_list, "toolBarVisible", false);
	const GUI_SAVE gl_toolIconColor  = GUI_SAVE( game_list, "toolIconColor",  gl_tool_icon_color);

	const GUI_SAVE fs_emulator_dir_list = GUI_SAVE(fs, "emulator_dir_list", QStringList());
	const GUI_SAVE fs_dev_hdd0_list     = GUI_SAVE(fs, "dev_hdd0_list",     QStringList());
	const GUI_SAVE fs_dev_hdd1_list     = GUI_SAVE(fs, "dev_hdd1_list",     QStringList());
	const GUI_SAVE fs_dev_flash_list    = GUI_SAVE(fs, "dev_flash_list",    QStringList());
	const GUI_SAVE fs_dev_usb000_list   = GUI_SAVE(fs, "dev_usb000_list",   QStringList());

	const GUI_SAVE l_tty   = GUI_SAVE( logger, "TTY",   true );
	const GUI_SAVE l_level = GUI_SAVE( logger, "level", (uint)(logs::level::success) );
	const GUI_SAVE l_stack = GUI_SAVE( logger, "stack", false );

	const GUI_SAVE d_splitterState = GUI_SAVE( debugger, "splitterState", QByteArray());

	const GUI_SAVE m_currentConfig     = GUI_SAVE(meta, "currentConfig",     QObject::tr("CurrentSettings"));
	const GUI_SAVE m_currentStylesheet = GUI_SAVE(meta, "currentStylesheet", Default);
	const GUI_SAVE m_saveNotes         = GUI_SAVE(meta, "saveNotes",         QVariantMap());
	const GUI_SAVE m_showDebugTab      = GUI_SAVE(meta, "showDebugTab",      false);
	const GUI_SAVE m_enableUIColors    = GUI_SAVE(meta, "enableUIColors",    false);

	const GUI_SAVE gs_disableMouse = GUI_SAVE(gs_frame, "disableMouse", false);
	const GUI_SAVE gs_resize       = GUI_SAVE(gs_frame, "resize",       false);
	const GUI_SAVE gs_width        = GUI_SAVE(gs_frame, "width",        1280);
	const GUI_SAVE gs_height       = GUI_SAVE(gs_frame, "height",       720);
}

/** Class for GUI settings..
*/
class gui_settings : public QObject
{
	Q_OBJECT

public:
	explicit gui_settings(QObject* parent = nullptr);
	~gui_settings();

	QString GetSettingsDir() {
		return settingsDir.absolutePath();
	}

	/** Changes the settings file to the destination preset*/
	void ChangeToConfig(const QString& destination);

	bool GetCategoryVisibility(int cat);
	QVariant GetValue(const GUI_SAVE& entry);
	QVariant List2Var(const q_pair_list& list);
	q_pair_list Var2List(const QVariant &var);

	void ShowInfoBox(const GUI_SAVE& entry, const QString& title, const QString& text, QWidget* parent = 0);

	logs::level GetLogLevel();
	bool GetGamelistColVisibility(int col);
	QColor GetCustomColor(int col);
	QStringList GetConfigEntries();
	QString GetCurrentStylesheetPath();
	QStringList GetStylesheetEntries();
	QStringList GetGameListCategoryFilters();

	/**
		Creates a custom colored QIcon based on another QIcon
		@param icon the icon to colorize
		@param oldColor the current color of icon
		@param newColor the desired color for the new icon
		@param useSpecialMasks only used for icons with white parts and disc game icon
	*/
	static QIcon colorizedIcon(const QIcon& icon, const QColor& oldColor, const QColor& newColor, bool useSpecialMasks = false);

public Q_SLOTS:
	void Reset(bool removeMeta = false);

	/** Write value to entry */
	void SetValue(const GUI_SAVE& entry, const QVariant& value);

	/** Sets the visibility of the chosen category. */
	void SetCategoryVisibility(int cat, const bool& val);

	void SetGamelistColVisibility(int col, bool val);

	void SetCustomColor(int col, const QColor& val);

	void SaveCurrentConfig(const QString& friendlyName);

private:
	QString ComputeSettingsDir();
	void BackupSettingsToTarget(const QString& destination);

	QSettings settings;
	QDir settingsDir;
};
