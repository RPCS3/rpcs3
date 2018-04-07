#pragma once

#include "stdafx.h"
#include "Emu/GameInfo.h"

#include "custom_dock_widget.h"
#include "game_list.h"
#include "game_list_grid.h"
#include "emu_settings.h"
#include "game_compatibility.h"

#include <QMainWindow>
#include <QToolBar>
#include <QLineEdit>
#include <QStackedWidget>
#include <QSet>

#include <memory>

enum Category
{
	Disc_Game,
	Non_Disc_Game,
	Home,
	Media,
	Data,
	Unknown_Cat,
	Others,
};

namespace category // (see PARAM.SFO in psdevwiki.com) TODO: Disc Categories 
{
	// PS3 bootable
	const QString app_Music = QObject::tr("App Music");
	const QString app_Photo = QObject::tr("App Photo");
	const QString app_TV    = QObject::tr("App TV");
	const QString app_Video = QObject::tr("App Video");
	const QString bc_Video  = QObject::tr("Broadcast Video");
	const QString disc_Game = QObject::tr("Disc Game");
	const QString hdd_Game  = QObject::tr("HDD Game");
	const QString home      = QObject::tr("Home");
	const QString network   = QObject::tr("Network");
	const QString store_FE  = QObject::tr("Store");
	const QString web_TV    = QObject::tr("Web TV");

	// PS2 bootable
	const QString ps2_game = QObject::tr("PS2 Classics");
	const QString ps2_inst = QObject::tr("PS2 Game");

	// PS1 bootable
	const QString ps1_game = QObject::tr("PS1 Classics");

	// PSP bootable
	const QString psp_game = QObject::tr("PSP Game");
	const QString psp_mini = QObject::tr("PSP Minis");
	const QString psp_rema = QObject::tr("PSP Remasters");

	// Data
	const QString ps3_Data = QObject::tr("PS3 Game Data");
	const QString ps2_Data = QObject::tr("PS2 Emulator Data");

	// Save
	const QString ps3_Save = QObject::tr("PS3 Save Data");
	const QString psp_Save = QObject::tr("PSP Minis Save Data");

	// others
	const QString trophy  = QObject::tr("Trophy");
	const QString unknown = QObject::tr("Unknown");
	const QString other   = QObject::tr("Other");

	const q_from_char cat_boot =
	{
		{ "AM",app_Music }, // media
		{ "AP",app_Photo }, // media
		{ "AT",app_TV },    // media
		{ "AV",app_Video }, // media
		{ "BV",bc_Video },  // media
		{ "DG",disc_Game }, // disc_Game
		{ "HG",hdd_Game },  // non_disc_games
		{ "HM",home },      // home
		{ "CB",network },   // other
		{ "SF",store_FE },  // other
		{ "WT",web_TV },    // media
		{ "2P",ps2_game },  // non_disc_games
		{ "2G",ps2_inst },  // non_disc_games
		{ "1P",ps1_game },  // non_disc_games
		{ "PP",psp_game },  // non_disc_games
		{ "MN",psp_mini },  // non_disc_games
		{ "PE",psp_rema }   // non_disc_games
	};
	const q_from_char cat_data =
	{
		{ "GD",ps3_Data }, // data
		{ "2D",ps2_Data }, // data
		{ "SD",ps3_Save }, // data
		{ "MS",psp_Save }  // data
	};

	const QStringList non_disc_games = { hdd_Game, ps2_game, ps2_inst, ps1_game, psp_game, psp_mini, psp_rema };
	const QStringList media = { app_Photo, app_Video, bc_Video, app_Music, app_TV, web_TV };
	const QStringList data = { ps3_Data, ps2_Data, ps3_Save, psp_Save };
	const QStringList others = { network, store_FE, trophy, other };

	inline bool CategoryInMap(const std::string& cat, const q_from_char& map)
	{
		auto map_contains_category = [s = qstr(cat)](const auto& p)
		{
			return p.second == s;
		};

		return std::find_if(map.begin(), map.end(), map_contains_category) != map.end();
	}
}

namespace parental
{
	// These values are partly generalized. They can vary between country and category
	// Normally only values 1,2,3,5,7 and 9 are used
	const std::map<u32, QString> level
	{
		{ 1,  QObject::tr("0+") },
		{ 2,  QObject::tr("3+") },
		{ 3,  QObject::tr("7+") },
		{ 4,  QObject::tr("10+") },
		{ 5,  QObject::tr("12+") },
		{ 6,  QObject::tr("15+") },
		{ 7,  QObject::tr("16+") },
		{ 8,  QObject::tr("17+") },
		{ 9,  QObject::tr("18+") },
		{ 10, QObject::tr("Level 10") },
		{ 11, QObject::tr("Level 11") }
	};
}

namespace resolution
{
	// there might be different values for other categories
	const std::map<u32, QString> mode
	{
		{ 1 << 0, QObject::tr("480p") },
		{ 1 << 1, QObject::tr("576p") },
		{ 1 << 2, QObject::tr("720p") },
		{ 1 << 3, QObject::tr("1080p") },
		{ 1 << 4, QObject::tr("480p 16:9") },
		{ 1 << 5, QObject::tr("576p 16:9") },
	};
}

namespace sound
{
	const std::map<u32, QString> format
	{
		{ 1 << 0, QObject::tr("LPCM 2.0") },
		//{ 1 << 1, QObject::tr("LPCM ???") },
		{ 1 << 2, QObject::tr("LPCM 5.1") },
		{ 1 << 4, QObject::tr("LPCM 7.1") },
		{ 1 << 8, QObject::tr("Dolby Digital 5.1") },
		{ 1 << 9, QObject::tr("DTS 5.1") },
	};
}

/* Having the icons associated with the game info simplifies logic internally */
struct GUI_GameInfo
{
	GameInfo info;
	compat_status compat;
	QImage icon;
	QPixmap pxmap;
	bool hasCustomConfig;
};

class game_list_frame : public custom_dock_widget
{
	Q_OBJECT

public:
	explicit game_list_frame(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, QWidget *parent = nullptr);
	~game_list_frame();

	/** Refresh the gamelist with/without loading game data from files. Public so that main frame can refresh after vfs or install */
	void Refresh(const bool fromDrive = false, const bool scrollAfter = true);

	/** Adds/removes categories that should be shown on gamelist. Public so that main frame menu actions can apply them */
	void ToggleCategoryFilter(const QStringList& categories, bool show);

	/** Loads from settings. Public so that main frame can easily reset these settings if needed. */
	void LoadSettings();

	/** Saves settings. Public so that main frame can save this when a caching of column widths is needed for settings backup */
	void SaveSettings();

	/** Resize Gamelist Icons to size given by slider position */
	void ResizeIcons(const int& sliderPos);

	/** Repaint Gamelist Icons with new background color */
	void RepaintIcons(const bool& fromSettings = false);

	void SetShowHidden(bool show);

public Q_SLOTS:
	void SetListMode(const bool& isList);
	void SetSearchText(const QString& text);

private Q_SLOTS:
	bool RemoveCustomConfiguration(const std::string& base_dir, bool is_interactive = false);
	bool DeleteShadersCache(const std::string& base_dir, bool is_interactive = false);
	bool DeleteLLVMCache(const std::string& base_dir, bool is_interactive = false);
	void OnColClicked(int col);
	void ShowContextMenu(const QPoint &pos);
	void ShowSpecifiedContextMenu(const QPoint &pos, int index); // Different name because the notation for overloaded connects is messy
	void doubleClickedSlot(const QModelIndex& index);
Q_SIGNALS:
	void GameListFrameClosed();
	void RequestBoot(const std::string& path);
	void RequestIconSizeChange(const int& val);
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event) override;
	void resizeEvent(QResizeEvent *event) override;
	bool eventFilter(QObject *object, QEvent *event) override;
private:
	QPixmap PaintedPixmap(const QImage& img, bool paintConfigIcon = false);
	void PopulateGameGrid(int maxCols, const QSize& image_size, const QColor& image_color);
	bool IsEntryVisible(const GUI_GameInfo& game);
	void SortGameList();

	int PopulateGameList();
	bool SearchMatchesApp(const std::string& name, const std::string& serial);

	std::string CurrentSelectionIconPath();
	std::string GetStringFromU32(const u32& key, const std::map<u32, QString>& map, bool combined = false);

	// Which widget we are displaying depends on if we are in grid or list mode.
	QMainWindow* m_Game_Dock;
	QStackedWidget* m_Central_Widget;

	// Game Grid
	game_list_grid* m_xgrid;

	// Game List
	game_list* m_gameList;
	std::unique_ptr<game_compatibility> m_game_compat;
	QList<QAction*> m_columnActs;
	Qt::SortOrder m_colSortOrder;
	int m_sortColumn;

	// Categories
	QStringList m_categoryFilters;

	// List Mode
	bool m_isListLayout = true;
	bool m_oldLayoutIsList = true;

	// Data
	std::shared_ptr<gui_settings> xgui_settings;
	std::shared_ptr<emu_settings> xemu_settings;
	std::vector<GUI_GameInfo> m_game_data;
	QSet<QString> m_hidden_list;
	bool m_show_hidden{false};

	// Search
	QString m_search_text;

	// Icon Size
	int m_icon_size_index = 0;

	// Icons
	QColor m_Icon_Color;
	QSize m_Icon_Size = gui::gl_icon_size_min; // ensure a valid size
	qreal m_Margin_Factor;
	qreal m_Text_Factor;
};
