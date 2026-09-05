#pragma once

#include "game_list.h"
#include "game_list_actions.h"
#include "gui_game_info.h"
#include "custom_dock_widget.h"
#include "content_integrity.h"

#include "util/auto_typemap.hpp"
#include "Emu/config_mode.h"
#include "Emu/game_enumeration.h"

#include <QMainWindow>
#include <QStackedWidget>
#include <QSet>
#include <QHash>
#include <QFutureWatcher>

#include <memory>
#include <optional>
#include <set>

class config_database;
class game_list_table;
class game_list_grid;
class gui_settings;
class emu_settings;
class persistent_settings;
class progress_dialog;
class Localized;

class QTableWidgetItem;

class game_list_frame : public custom_dock_widget
{
	Q_OBJECT

public:
	explicit game_list_frame(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, std::shared_ptr<persistent_settings> persistent_settings, QWidget* parent = nullptr);
	~game_list_frame();

	/** Refresh the gamelist with/without loading game data from files. Public so that main frame can refresh after vfs or install */
	void Refresh(const bool from_drive = false, const std::vector<std::string>& serials_to_remove_from_yml = {}, const bool scroll_after = true);

	/** Adds/removes categories that should be shown on gamelist. Public so that main frame menu actions can apply them */
	void ToggleCategoryFilter(const QStringList& categories, bool show);

	/** Restrict the game list to the games of a user defined game collection. An empty name shows every game. */
	void SetGameCollection(const QString& name);

	/** Re-reads the members of the selected game collection, after games were moved between collections */
	void ReloadGameCollection();

	/** How many games of each of the given collections the library holds. Membership may name games that
	    are not installed, and those are what this leaves out; the other filters - category, hidden, broken,
	    completed, the search box - are not applied, so the number does not move while the user types.
	    Selecting the collection can therefore show fewer than the menu says.
	    The list comes from the caller, which has already read it to build the entries being counted. */
	QHash<QString, qsizetype> CountGamesPerCollection(const QStringList& collections) const;

	/** Loads from settings. Public so that main frame can easily reset these settings if needed. */
	void LoadSettings();

	/** Saves settings. Public so that main frame can save this when a caching of column widths is needed for settings backup */
	void SaveSettings();

	/** Resize Gamelist Icons to size given by slider position */
	void ResizeIcons(int slider_pos);

	/** Repaint Gamelist Icons with new background color */
	void RepaintIcons(bool from_settings = false);

	void SetShowHidden(bool show);

	void SetShowBroken(bool show);

	void SetShowCompleted(bool show);

	content_integrity* GetIsoIntegrity() const { return ensure(m_iso_integrity); }
	content_integrity* GetPsnContentIntegrity() const { return ensure(m_psn_content_integrity); }
	content_integrity* GetPsnDlcIntegrity() const { return ensure(m_psn_dlc_integrity); }
	content_integrity* GetPsnUpdateIntegrity() const { return ensure(m_psn_update_integrity); }
	game_compatibility* GetGameCompatibility() const { return ensure(m_game_compat); }
	config_database* GetConfigDatabase() const { return ensure(m_config_db); }
	const std::vector<game_info>& GetGameInfo() const { return m_game_data; }
	std::shared_ptr<game_list_actions> actions() const { return m_game_list_actions; }
	std::shared_ptr<gui_settings> get_gui_settings() const { return m_gui_settings; }
	std::shared_ptr<emu_settings> get_emu_settings() const { return m_emu_settings; }
	std::shared_ptr<persistent_settings> get_persistent_settings() const { return m_persistent_settings; }
	std::map<QString, QString>& notes() { return m_notes; }
	std::map<QString, QString>& titles() { return m_titles; }
	QSet<QString>& hidden_list() { return m_hidden_list; }
	QSet<QString>& broken_list() { return m_broken_list; }
	QSet<QString>& completed_list() { return m_completed_list; }

	bool IsEntryVisible(const game_info& game, bool search_fallback = false) const;

	void ShowCustomConfigIcon(const game_info& game);

	void stop_movie();

	// Enqueue slot for refreshed signal
	// Allowing for an individual container for each distinct use case (currently disabled and contains only one such entry)
	template <typename KeySlot = void, typename Func>
	void AddRefreshedSlot(Func&& func)
	{
		// NOTE: Remove assert when the need for individual containers arises
		static_assert(std::is_void_v<KeySlot>);

		connect(this, &game_list_frame::Refreshed, this, [this, func = std::move(func)]() mutable
		{
			func(m_refresh_funcs_manage_type->get<GameIdsTable<KeySlot>>().m_done_paths);
		}, Qt::SingleShotConnection);
	}

public Q_SLOTS:
	void SetListMode(bool is_list);
	void SetSearchText(const QString& text);
	void SetShowCompatibilityInGrid(bool show);
	void SetPreferGameDataIcons(bool enabled);
	void SetShowCustomIcons(bool show);
	void SetPlayHoverGifs(bool play);
	void SetPlayHoverMusic(bool play);
	void FocusAndSelectFirstEntryIfNoneIs();

private Q_SLOTS:
	void OnParsingFinished();
	void OnRefreshFinished();
	void OnCompatFinished();
	void OnConfigDatabaseFinished();
	void OnColClicked(int col);
	void ShowContextMenu(const QPoint& pos);
	void doubleClickedSlot(QTableWidgetItem* item);
	void doubleClickedSlot(const game_info& game);
	void ItemSelectionChangedSlot();

Q_SIGNALS:
	void GameListFrameClosed();
	void NotifyGameSelection(const game_info& game);
	void RequestBoot(const game_info& game, cfg_mode config_mode = cfg_mode::custom, const std::string& config_path = "", const std::string& savestate = "");
	void RequestIconSizeChange(int val);
	void NotifyEmuSettingsChange();
	void FocusToSearchBar();
	void Refreshed();
	void RequestSaveStateManager(const game_info& game);

protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event) override;
	bool eventFilter(QObject *object, QEvent *event) override;

private:
	template <typename KeyType>
	struct GameIdsTable
	{
		// List of game paths an operation has been done on for the use of the slot function
		std::set<std::string> m_done_paths;
	};

	void UpdateWindowTitle(const std::vector<game_info>& matching_apps);

	QString get_header_text(int col) const;
	QString get_action_text(int col) const;

	bool SearchMatchesApp(const QString& name, const QString& serial, bool fallback = false) const;

	std::set<std::string> CurrentSelectionPaths();

	game_info GetGameInfoByMode(const QTableWidgetItem* item) const;
	static game_info GetGameInfoFromItem(const QTableWidgetItem* item);

	void WaitAndAbortRepaintThreads();
	void WaitAndAbortSizeCalcThreads();

	std::shared_ptr<game_list_actions> m_game_list_actions;

	// Which widget we are displaying depends on if we are in grid or list mode.
	QMainWindow* m_game_dock = nullptr;
	QStackedWidget* m_central_widget = nullptr;

	// Game Grid
	game_list_grid* m_game_grid = nullptr;

	// Game List
	game_list_table* m_game_list = nullptr;
	content_integrity* m_iso_integrity = nullptr;
	content_integrity* m_psn_content_integrity = nullptr;
	content_integrity* m_psn_dlc_integrity = nullptr;
	content_integrity* m_psn_update_integrity = nullptr;
	game_compatibility* m_game_compat = nullptr;
	config_database* m_config_db = nullptr;
	progress_dialog* m_progress_dialog = nullptr;
	std::map<int, QAction*> m_column_acts;
	Qt::SortOrder m_col_sort_order{};
	int m_sort_column{};
	bool m_initial_refresh_done = false;
	std::map<QString, QString> m_notes;
	std::map<QString, QString> m_titles;

	// Categories
	QStringList m_category_filters;
	QStringList m_grid_category_filters;

	// User defined game collection used as an additional filter. Empty means "All Games".
	QString m_game_collection;
	QSet<QString> m_game_collection_serials;

	// List Mode
	bool m_is_list_layout = true;
	bool m_old_layout_is_list = true;

	// Data
	void add_game_apply_extras(gui_game_info& game);
	class gui_game_enumeration : public game_enumeration<gui_game_info>
	{
	public:
		gui_game_enumeration(game_list_frame& parent) : game_enumeration<gui_game_info>(), m_parent(parent) {}
		void add_game_apply_extras(gui_game_info& game) override { m_parent.add_game_apply_extras(game); }
	private:
		game_list_frame& m_parent;
	};
	gui_game_enumeration m_game_enumeration;

	std::shared_ptr<Localized> m_localized;
	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<emu_settings> m_emu_settings;
	std::shared_ptr<persistent_settings> m_persistent_settings;
	std::vector<game_info> m_game_data;
	QSet<QString> m_serials;
	std::mutex m_games_mutex;
	const std::array<int, 1> m_parsing_threads{0};
	QFutureWatcher<void> m_parsing_watcher;
	QFutureWatcher<void> m_refresh_watcher;
	QSet<QString> m_hidden_list;
	bool m_show_hidden{false};
	QSet<QString> m_broken_list;
	bool m_show_broken{false};
	QSet<QString> m_completed_list;
	bool m_show_completed{false};

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
	bool m_prefer_game_data_icons = false;
	bool m_show_custom_icons = true;
	bool m_play_hover_movies = true;
	bool m_play_hover_music = true;
	std::optional<auto_typemap<game_list_frame>> m_refresh_funcs_manage_type{std::in_place};
};
