#pragma once

#include "Emu/GameInfo.h"

#include "custom_dock_widget.h"
#include "game_compatibility.h"

#include <QMainWindow>
#include <QToolBar>
#include <QStackedWidget>
#include <QSet>
#include <QTableWidgetItem>

#include <memory>

class game_list;
class game_list_grid;
class gui_settings;
class emu_settings;
class persistent_settings;

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
	explicit game_list_frame(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, std::shared_ptr<persistent_settings> persistent_settings, QWidget *parent = nullptr);
	~game_list_frame();

	/** Fix columns with width smaller than the minimal section size */
	void FixNarrowColumns();

	/** Resizes the columns to their contents and adds a small spacing */
	void ResizeColumnsToContents(int spacing = 20);

	/** Refresh the gamelist with/without loading game data from files. Public so that main frame can refresh after vfs or install */
	void Refresh(const bool from_drive = false, const bool scroll_after = true);

	/** Adds/removes categories that should be shown on gamelist. Public so that main frame menu actions can apply them */
	void ToggleCategoryFilter(const QStringList& categories, bool show);

	/** Loads from settings. Public so that main frame can easily reset these settings if needed. */
	void LoadSettings();

	/** Saves settings. Public so that main frame can save this when a caching of column widths is needed for settings backup */
	void SaveSettings();

	/** Resize Gamelist Icons to size given by slider position */
	void ResizeIcons(const int& slider_pos);

	/** Repaint Gamelist Icons with new background color */
	void RepaintIcons(const bool& from_settings = false);

	void SetShowHidden(bool show);

public Q_SLOTS:
	void BatchCreatePPUCaches();
	void BatchRemovePPUCaches();
	void BatchRemoveSPUCaches();
	void BatchRemoveCustomConfigurations();
	void BatchRemoveCustomPadConfigurations();
	void BatchRemoveShaderCaches();
	void SetListMode(const bool& is_list);
	void SetSearchText(const QString& text);
	void SetShowCompatibilityInGrid(bool show);

private Q_SLOTS:
	void OnColClicked(int col);
	void ShowContextMenu(const QPoint &pos);
	void doubleClickedSlot(QTableWidgetItem *item);
	void itemSelectionChangedSlot();
Q_SIGNALS:
	void GameListFrameClosed();
	void NotifyGameSelection(const game_info& game);
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
	void PopulateGameList();
	void PopulateGameGrid(int maxCols, const QSize& image_size, const QColor& image_color);
	bool IsEntryVisible(const game_info& game);
	void SortGameList();
	bool SearchMatchesApp(const QString& name, const QString& serial) const;

	bool RemoveCustomConfiguration(const std::string& title_id, game_info game = nullptr, bool is_interactive = false);
	bool RemoveCustomPadConfiguration(const std::string& title_id, game_info game = nullptr, bool is_interactive = false);
	bool RemoveShadersCache(const std::string& base_dir, bool is_interactive = false);
	bool RemovePPUCache(const std::string& base_dir, bool is_interactive = false);
	bool RemoveSPUCache(const std::string& base_dir, bool is_interactive = false);
	bool CreatePPUCache(const game_info& game);

	QString GetLastPlayedBySerial(const QString& serial);
	std::string GetCacheDirBySerial(const std::string& serial);
	std::string GetDataDirBySerial(const std::string& serial);
	std::string CurrentSelectionIconPath();
	std::string GetStringFromU32(const u32& key, const std::map<u32, QString>& map, bool combined = false);

	game_info GetGameInfoByMode(const QTableWidgetItem* item);
	game_info GetGameInfoFromItem(const QTableWidgetItem* item);

	// Which widget we are displaying depends on if we are in grid or list mode.
	QMainWindow* m_game_dock;
	QStackedWidget* m_central_widget;

	// Game Grid
	game_list_grid* m_game_grid;

	// Game List
	game_list* m_game_list;
	std::unique_ptr<game_compatibility> m_game_compat;
	QList<QAction*> m_columnActs;
	Qt::SortOrder m_col_sort_order;
	int m_sort_column;
	QMap<QString, QString> m_notes;
	QMap<QString, QString> m_titles;

	// Categories
	QStringList m_category_filters;

	// List Mode
	bool m_is_list_layout = true;
	bool m_old_layout_is_list = true;

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
	QColor m_icon_color;
	QSize m_icon_size;
	qreal m_margin_factor;
	qreal m_text_factor;
	bool m_draw_compat_status_to_grid = false;
};
