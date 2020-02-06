#pragma once

#include "stdafx.h"
#include "Emu/GameInfo.h"

#include "custom_dock_widget.h"
#include "game_list.h"
#include "game_list_grid.h"
#include "emu_settings.h"
#include "persistent_settings.h"
#include "game_compatibility.h"
#include "category.h"

#include <QMainWindow>
#include <QToolBar>
#include <QLineEdit>
#include <QStackedWidget>
#include <QSet>

#include <memory>

/* Having the icons associated with the game info simplifies logic internally */
struct gui_game_info
{
	GameInfo info;
	QString localized_category;
	compat_status compat;
	QPixmap icon;
	QPixmap pxmap;
	bool hasCustomConfig;
	bool hasCustomPadConfig;
};

typedef std::shared_ptr<gui_game_info> game_info;
Q_DECLARE_METATYPE(game_info)

class game_list_frame : public custom_dock_widget
{
	Q_OBJECT

public:
	explicit game_list_frame(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, std::shared_ptr<persistent_settings> persistent_settings, QWidget *parent = nullptr);
	~game_list_frame();

	/** Fix columns with width smaller than the minimal section size */
	void FixNarrowColumns();

	/** Resizes the columns to their contents and adds a small spacing */
	void ResizeColumnsToContents(int spacing = 20);

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
	void BatchCreatePPUCaches();
	void BatchRemovePPUCaches();
	void BatchRemoveSPUCaches();
	void BatchRemoveCustomConfigurations();
	void BatchRemoveCustomPadConfigurations();
	void BatchRemoveShaderCaches();
	void SetListMode(const bool& isList);
	void SetSearchText(const QString& text);
	void SetShowCompatibilityInGrid(bool show);

private Q_SLOTS:
	void OnColClicked(int col);
	void ShowContextMenu(const QPoint &pos);
	void doubleClickedSlot(QTableWidgetItem *item);
Q_SIGNALS:
	void GameListFrameClosed();
	void RequestBoot(const game_info& game, bool force_global_config = false);
	void RequestIconSizeChange(const int& val);
	void NotifyEmuSettingsChange();
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event) override;
	void resizeEvent(QResizeEvent *event) override;
	bool eventFilter(QObject *object, QEvent *event) override;
private:
	QPixmap PaintedPixmap(const QPixmap& icon, bool paint_config_icon = false, bool paint_pad_config_icon = false, const QColor& color = QColor());
	QColor getGridCompatibilityColor(const QString& string);
	void ShowCustomConfigIcon(game_info game);
	void PopulateGameGrid(int maxCols, const QSize& image_size, const QColor& image_color);
	bool IsEntryVisible(const game_info& game);
	void SortGameList();

	int PopulateGameList();
	bool SearchMatchesApp(const QString& name, const QString& serial) const;

	bool RemoveCustomConfiguration(const std::string& title_id, game_info game = nullptr, bool is_interactive = false);
	bool RemoveCustomPadConfiguration(const std::string& title_id, game_info game = nullptr, bool is_interactive = false);
	bool RemoveShadersCache(const std::string& base_dir, bool is_interactive = false);
	bool RemovePPUCache(const std::string& base_dir, bool is_interactive = false);
	bool RemoveSPUCache(const std::string& base_dir, bool is_interactive = false);
	bool CreatePPUCache(const game_info& game);

	QString GetLastPlayedBySerial(const QString& serial);
	QString GetPlayTimeByMs(int elapsed_ms);
	std::string GetCacheDirBySerial(const std::string& serial);
	std::string GetDataDirBySerial(const std::string& serial);
	std::string CurrentSelectionIconPath();
	std::string GetStringFromU32(const u32& key, const std::map<u32, QString>& map, bool combined = false);

	game_info GetGameInfoFromItem(QTableWidgetItem* item);

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
	QMap<QString, QString> m_notes;
	QMap<QString, QString> m_titles;

	// Categories
	QStringList m_categoryFilters;

	// List Mode
	bool m_isListLayout = true;
	bool m_oldLayoutIsList = true;

	// Data
	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<emu_settings> m_emu_settings;
	std::shared_ptr<persistent_settings> m_persistent_settings;
	QList<game_info> m_game_data;
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
	bool m_drawCompatStatusToGrid = false;
};
