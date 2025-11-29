#pragma once

#include "game_list.h"
#include "custom_dock_widget.h"
#include "shortcut_utils.h"
#include "Utilities/lockless.h"
#include "Utilities/mutex.h"
#include "util/auto_typemap.hpp"
#include "Emu/config_mode.h"

#include <QMainWindow>
#include <QToolBar>
#include <QStackedWidget>
#include <QSet>
#include <QTableWidgetItem>
#include <QFutureWatcher>
#include <QTimer>

#include <memory>
#include <optional>
#include <set>

class game_list_table;
class game_list_grid;
class gui_settings;
class emu_settings;
class persistent_settings;
class progress_dialog;

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

	/** Loads from settings. Public so that main frame can easily reset these settings if needed. */
	void LoadSettings();

	/** Saves settings. Public so that main frame can save this when a caching of column widths is needed for settings backup */
	void SaveSettings();

	/** Resize Gamelist Icons to size given by slider position */
	void ResizeIcons(const int& slider_pos);

	/** Repaint Gamelist Icons with new background color */
	void RepaintIcons(const bool& from_settings = false);

	void SetShowHidden(bool show);

	game_compatibility* GetGameCompatibility() const { return m_game_compat; }

	const std::vector<game_info>& GetGameInfo() const;

	void CreateShortcuts(const std::vector<game_info>& games, const std::set<gui::utils::shortcut_location>& locations);

	bool IsEntryVisible(const game_info& game, bool search_fallback = false) const;

public Q_SLOTS:
	void BatchCreateCPUCaches(const std::vector<game_info>& game_data = {}, bool is_fast_compilation = false);
	void BatchRemovePPUCaches();
	void BatchRemoveSPUCaches();
	void BatchRemoveCustomConfigurations();
	void BatchRemoveCustomPadConfigurations();
	void BatchRemoveShaderCaches();
	void SetListMode(const bool& is_list);
	void SetSearchText(const QString& text);
	void SetShowCompatibilityInGrid(bool show);
	void SetPreferGameDataIcons(bool enabled);
	void SetShowCustomIcons(bool show);
	void SetPlayHoverGifs(bool play);
	void FocusAndSelectFirstEntryIfNoneIs();

private Q_SLOTS:
	void OnParsingFinished();
	void OnRefreshFinished();
	void OnCompatFinished();
	void OnColClicked(int col);
	void ShowContextMenu(const QPoint &pos);
	void doubleClickedSlot(QTableWidgetItem* item);
	void doubleClickedSlot(const game_info& game);
	void ItemSelectionChangedSlot();
Q_SIGNALS:
	void GameListFrameClosed();
	void NotifyGameSelection(const game_info& game);
	void RequestBoot(const game_info& game, cfg_mode config_mode = cfg_mode::custom, const std::string& config_path = "", const std::string& savestate = "");
	void RequestIconSizeChange(const int& val);
	void NotifyEmuSettingsChange();
	void FocusToSearchBar();
	void Refreshed();
	void RequestSaveStateManager(const game_info& game);
	void NotifyBatchedGameActionFinished();

public:
	template <typename KeyType>
	struct GameIdsTable
	{
		// List of game paths an operation has been done on for the use of the slot function
		std::set<std::string> m_done_paths;
	};

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

protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event) override;
	bool eventFilter(QObject *object, QEvent *event) override;
private:
	void push_path(const std::string& path, std::vector<std::string>& legit_paths);

	void ShowCustomConfigIcon(const game_info& game);
	bool SearchMatchesApp(const QString& name, const QString& serial, bool fallback = false) const;

	bool RemoveCustomConfiguration(const std::string& title_id, const game_info& game = nullptr, bool is_interactive = false);
	bool RemoveCustomPadConfiguration(const std::string& title_id, const game_info& game = nullptr, bool is_interactive = false);
	bool RemoveShadersCache(const std::string& base_dir, bool is_interactive = false);
	bool RemovePPUCache(const std::string& base_dir, bool is_interactive = false);
	bool RemoveSPUCache(const std::string& base_dir, bool is_interactive = false);
	void RemoveHDD1Cache(const std::string& base_dir, const std::string& title_id, bool is_interactive = false);
	static bool CreateCPUCaches(const std::string& path, const std::string& serial = {}, bool is_fast_compilation = false);
	static bool CreateCPUCaches(const game_info& game, bool is_fast_compilation = false);

	static bool RemoveContentPath(const std::string& path, const std::string& desc);
	static u32 RemoveContentPathList(const std::vector<std::string>& path_list, const std::string& desc);
	static bool RemoveContentBySerial(const std::string& base_dir, const std::string& serial, const std::string& desc);
	static std::vector<std::string> GetDirListBySerial(const std::string& base_dir, const std::string& serial);
	void BatchActionBySerials(progress_dialog* pdlg, const std::set<std::string>& serials, QString progressLabel, std::function<bool(const std::string&)> action, std::function<void(u32, u32)> cancel_log, bool refresh_on_finish, bool can_be_concurrent = false, std::function<bool()> should_wait_cb = {});
	static std::string GetCacheDirBySerial(const std::string& serial);
	static std::string GetDataDirBySerial(const std::string& serial);
	std::string CurrentSelectionPath();

	game_info GetGameInfoByMode(const QTableWidgetItem* item) const;
	static game_info GetGameInfoFromItem(const QTableWidgetItem* item);

	void WaitAndAbortRepaintThreads();
	void WaitAndAbortSizeCalcThreads();

	// Which widget we are displaying depends on if we are in grid or list mode.
	QMainWindow* m_game_dock = nullptr;
	QStackedWidget* m_central_widget = nullptr;

	// Game Grid
	game_list_grid* m_game_grid = nullptr;

	// Game List
	game_list_table* m_game_list = nullptr;
	game_compatibility* m_game_compat = nullptr;
	progress_dialog* m_progress_dialog = nullptr;
	QList<QAction*> m_columnActs;
	Qt::SortOrder m_col_sort_order{};
	int m_sort_column{};
	bool m_initial_refresh_done = false;
	std::map<QString, QString> m_notes;
	std::map<QString, QString> m_titles;

	// Categories
	QStringList m_category_filters;
	QStringList m_grid_category_filters;

	// List Mode
	bool m_is_list_layout = true;
	bool m_old_layout_is_list = true;

	// Data
	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<emu_settings> m_emu_settings;
	std::shared_ptr<persistent_settings> m_persistent_settings;
	std::vector<game_info> m_game_data;
	struct path_entry
	{
		std::string path;
		bool is_disc{};
		bool is_from_yml{};
	};
	std::vector<path_entry> m_path_entries;
	shared_mutex m_path_mutex;
	std::set<std::string> m_path_list;
	QSet<QString> m_serials;
	QMutex m_games_mutex;
	lf_queue<game_info> m_games;
	const std::array<int, 1> m_parsing_threads{0};
	QFutureWatcher<void> m_parsing_watcher;
	QFutureWatcher<void> m_refresh_watcher;
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
	bool m_prefer_game_data_icons = false;
	bool m_show_custom_icons = true;
	bool m_play_hover_movies = true;
	std::optional<auto_typemap<game_list_frame>> m_refresh_funcs_manage_type{std::in_place};
};
