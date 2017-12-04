#pragma once

#include "Utilities/Log.h"

#include <QSettings>
#include <QDir>
#include <QVariant>
#include <QSize>
#include <QColor>
#include <QBitmap>
#include <QLabel>

struct Compat_Status
{
	QString date;
	QString color;
	QString text;
	QString tooltip;
};

struct gui_save
{
	QString key;
	QString name;
	QVariant def;

	gui_save()
	{
		key = "";
		name = "";
		def = QVariant();
	};

	gui_save(const QString& k, const QString& n, const QVariant& d)
	{
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

namespace gui
{
	static QString stylesheet;

	enum game_list_columns
	{
		column_icon,
		column_name,
		column_serial,
		column_firmware,
		column_version,
		column_category,
		column_path,
		column_resolution,
		column_sound,
		column_parental,
		column_compat,

		column_count
	};

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

	inline QFont get_Label_Font(const QString& objectName)
	{
		QLabel dummy_font;
		dummy_font.setObjectName(objectName);
		dummy_font.ensurePolished();
		return dummy_font.font();
	};

	inline QString get_Single_Line(const QString& multi_line_string)
	{
		QString single_line_string = multi_line_string;
		single_line_string.replace("\n"," ");
		return single_line_string;
	}

	inline q_string_pair Recent_Game(const QString& path, const QString& title)
	{
		return q_string_pair(path, get_Single_Line(title));
	}

	const QString Default     = QObject::tr("default");
	const QString main_window = "main_window";
	const QString game_list   = "GameList";
	const QString logger      = "Logger";
	const QString debugger    = "Debugger";
	const QString meta        = "Meta";
	const QString fs          = "FileSystem";
	const QString gs_frame    = "GSFrame";
	const QString trophy      = "Trophy";
	const QString savedata    = "SaveData";

	const QColor gl_icon_color       = QColor(209, 209, 209, 255);
	const QColor gl_tool_icon_color  = QColor(  0, 100, 231, 255);
	const QColor mw_tool_icon_color  = QColor( 64,  64,  64, 255);
	const QColor mw_tool_bar_color   = QColor(227, 227, 227, 255);
	const QColor mw_thumb_icon_color = QColor(  0, 100, 231, 255);

	const gui_save rg_freeze  = gui_save(main_window, "recentGamesFrozen", false);
	const gui_save rg_entries = gui_save(main_window, "recentGamesNames",  QVariant::fromValue(q_pair_list()));

	const gui_save ib_pkg_success  = gui_save(main_window, "infoBoxEnabledInstallPKG", true );
	const gui_save ib_pup_success  = gui_save(main_window, "infoBoxEnabledInstallPUP", true );
	const gui_save ib_show_welcome = gui_save(main_window, "infoBoxEnabledWelcome",    true );

	const gui_save fd_install_pkg  = gui_save(main_window, "lastExplorePathPKG",  "" );
	const gui_save fd_install_pup  = gui_save(main_window, "lastExplorePathPUP",  "" );
	const gui_save fd_boot_elf     = gui_save(main_window, "lastExplorePathELF",  "" );
	const gui_save fd_boot_game    = gui_save(main_window, "lastExplorePathGAME", "" );
	const gui_save fd_decrypt_sprx = gui_save(main_window, "lastExplorePathSPRX", "" );
	const gui_save fd_cg_disasm    = gui_save(main_window, "lastExplorePathCGD",  "" );

	const gui_save mw_debugger       = gui_save(main_window, "debuggerVisible", false );
	const gui_save mw_logger         = gui_save(main_window, "loggerVisible",   true );
	const gui_save mw_gamelist       = gui_save(main_window, "gamelistVisible", true );
	const gui_save mw_toolBarVisible = gui_save(main_window, "toolBarVisible",  true );
	const gui_save mw_toolBarColor   = gui_save(main_window, "toolBarColor",    mw_tool_bar_color);
	const gui_save mw_toolIconColor  = gui_save(main_window, "toolIconColor",   mw_tool_icon_color);
	const gui_save mw_geometry       = gui_save(main_window, "geometry",        QByteArray() );
	const gui_save mw_windowState    = gui_save(main_window, "windowState",     QByteArray() );
	const gui_save mw_mwState        = gui_save(main_window, "wwState",         QByteArray() );

	const gui_save cat_hdd_game    = gui_save(game_list, "categoryVisibleHDDGame",    true );
	const gui_save cat_disc_game   = gui_save(game_list, "categoryVisibleDiscGame",   true );
	const gui_save cat_home        = gui_save(game_list, "categoryVisibleHome",       true );
	const gui_save cat_audio_video = gui_save(game_list, "categoryVisibleAudioVideo", true );
	const gui_save cat_game_data   = gui_save(game_list, "categoryVisibleGameData",   false );
	const gui_save cat_unknown     = gui_save(game_list, "categoryVisibleUnknown",    true );
	const gui_save cat_other       = gui_save(game_list, "categoryVisibleOther",      true );

	const gui_save gl_sortAsc        = gui_save(game_list, "sortAsc",        true );
	const gui_save gl_sortCol        = gui_save(game_list, "sortCol",        1 );
	const gui_save gl_state          = gui_save(game_list, "state",          QByteArray() );
	const gui_save gl_iconSize       = gui_save(game_list, "iconSize",       get_Index(gl_icon_size_small));
	const gui_save gl_iconColor      = gui_save(game_list, "iconColor",      gl_icon_color);
	const gui_save gl_listMode       = gui_save(game_list, "listMode",       true );
	const gui_save gl_textFactor     = gui_save(game_list, "textFactor",     (qreal) 2.0 );
	const gui_save gl_marginFactor   = gui_save(game_list, "marginFactor",   (qreal) 0.09 );
	const gui_save gl_toolBarVisible = gui_save(game_list, "toolBarVisible", false);
	const gui_save gl_toolIconColor  = gui_save(game_list, "toolIconColor",  gl_tool_icon_color);

	const gui_save fs_emulator_dir_list = gui_save(fs, "emulator_dir_list", QStringList());
	const gui_save fs_dev_hdd0_list     = gui_save(fs, "dev_hdd0_list",     QStringList());
	const gui_save fs_dev_hdd1_list     = gui_save(fs, "dev_hdd1_list",     QStringList());
	const gui_save fs_dev_flash_list    = gui_save(fs, "dev_flash_list",    QStringList());
	const gui_save fs_dev_usb000_list   = gui_save(fs, "dev_usb000_list",   QStringList());

	const gui_save l_tty   = gui_save(logger, "TTY",   true );
	const gui_save l_level = gui_save(logger, "level", (uint)(logs::level::success) );
	const gui_save l_stack = gui_save(logger, "stack", false );

	const gui_save d_splitterState = gui_save(debugger, "splitterState", QByteArray());

	const gui_save m_currentConfig     = gui_save(meta, "currentConfig",     QObject::tr("CurrentSettings"));
	const gui_save m_currentStylesheet = gui_save(meta, "currentStylesheet", Default);
	const gui_save m_saveNotes         = gui_save(meta, "saveNotes",         QVariantMap());
	const gui_save m_showDebugTab      = gui_save(meta, "showDebugTab",      false);
	const gui_save m_enableUIColors    = gui_save(meta, "enableUIColors",    false);

	const gui_save gs_disableMouse = gui_save(gs_frame, "disableMouse", false);
	const gui_save gs_resize       = gui_save(gs_frame, "resize",       false);
	const gui_save gs_width        = gui_save(gs_frame, "width",        1280);
	const gui_save gs_height       = gui_save(gs_frame, "height",       720);

	const gui_save tr_icon_height   = gui_save(trophy, "icon_height",   75);
	const gui_save tr_show_locked   = gui_save(trophy, "show_locked",   true);
	const gui_save tr_show_unlocked = gui_save(trophy, "show_unlocked", true);
	const gui_save tr_show_hidden   = gui_save(trophy, "show_hidden",   false);
	const gui_save tr_show_bronze   = gui_save(trophy, "show_bronze",   true);
	const gui_save tr_show_silver   = gui_save(trophy, "show_silver",   true);
	const gui_save tr_show_gold     = gui_save(trophy, "show_gold",     true);
	const gui_save tr_show_platinum = gui_save(trophy, "show_platinum", true);
	const gui_save tr_geometry      = gui_save(trophy, "geometry",      QByteArray());

	const gui_save sd_geometry = gui_save(savedata, "geometry", QByteArray());
}

/** Class for GUI settings..
*/
class gui_settings : public QObject
{
	Q_OBJECT

public:
	explicit gui_settings(QObject* parent = nullptr);
	~gui_settings();

	QString GetSettingsDir();

	/** Changes the settings file to the destination preset*/
	void ChangeToConfig(const QString& destination);

	bool GetCategoryVisibility(int cat);
	QVariant GetValue(const gui_save& entry);
	QVariant List2Var(const q_pair_list& list);
	q_pair_list Var2List(const QVariant &var);

	void ShowInfoBox(const gui_save& entry, const QString& title, const QString& text, QWidget* parent = 0);

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
	static QIcon colorizedIcon(const QIcon& icon, const QColor& oldColor, const QColor& newColor, bool useSpecialMasks = false, bool colorizeAll = false);
	static QPixmap colorizedPixmap(const QPixmap& old_pixmap, const QColor& oldColor, const QColor& newColor, bool useSpecialMasks = false, bool colorizeAll = false);
	static QImage GetOpaqueImageArea(const QString& path);

public Q_SLOTS:
	void Reset(bool removeMeta = false);

	/** Write value to entry */
	void SetValue(const gui_save& entry, const QVariant& value);

	/** Sets the visibility of the chosen category. */
	void SetCategoryVisibility(int cat, const bool& val);

	void SetGamelistColVisibility(int col, bool val);

	void SetCustomColor(int col, const QColor& val);

	void SaveCurrentConfig(const QString& friendlyName);

private:
	QString ComputeSettingsDir();
	void BackupSettingsToTarget(const QString& destination);

	QSettings m_settings;
	QDir m_settingsDir;
};
