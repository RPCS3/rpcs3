#pragma once

#include "gui_game_info.h"
#include "shortcut_utils.h"

#include <QFuture>
#include <QObject>

class progress_dialog;
class game_list_frame;
class gui_settings;

class game_list_actions : QObject
{
	Q_OBJECT

public:
	game_list_actions(game_list_frame* frame, std::shared_ptr<gui_settings> gui_settings);
	virtual ~game_list_actions();

	enum content_type
	{
		NO_CONTENT    = 0,
		DISC          = (1 << 0),
		DATA          = (1 << 1),
		LOCKS         = (1 << 2),
		CACHES        = (1 << 3),
		CUSTOM_CONFIG = (1 << 4),
		ICONS         = (1 << 5),
		SHORTCUTS     = (1 << 6),
		SAVESTATES    = (1 << 7),
		CAPTURES      = (1 << 8),
		RECORDINGS    = (1 << 9),
		SCREENSHOTS   = (1 << 10)
	};

	struct content_info
	{
		u16 content_types = NO_CONTENT; // Always set by SetContentList()
		bool clear_on_finish = true;    // Always overridden by BatchRemoveContentLists()

		bool is_single_selection = false;
		u16 in_games_dir_count = 0;
		QString info;
		std::map<std::string, std::set<std::string>> name_list;
		std::map<std::string, std::set<std::string>> path_list;
		std::set<std::string> disc_list;
		std::set<std::string> removed_disc_list; // Filled in by RemoveContentList()
	};

	static bool IsGameRunning(const std::string& serial);

	void CreateShortcuts(const std::vector<game_info>& games, const std::set<gui::utils::shortcut_location>& locations);

	void ShowRemoveGameDialog(const std::vector<game_info>& games);
	void ShowGameInfoDialog(const std::vector<game_info>& games);
	void ShowDiskUsageDialog();

	// NOTES:
	//   - SetContentList() MUST always be called to set the content's info to be removed by:
	//     - RemoveContentList()
	//     - BatchRemoveContentLists()
	//
	void SetContentList(u16 content_types, const content_info& content_info);
	void ClearContentList(bool refresh = false);
	content_info GetContentInfo(const std::vector<game_info>& games);

	bool ValidateRemoval(const std::string& serial, const std::string& path, const std::string& desc, bool is_interactive = false);
	bool ValidateBatchRemoval(const std::string& desc, bool is_interactive = false);

	static bool CreateCPUCaches(const std::string& path, const std::string& serial = {}, bool is_fast_compilation = false);
	static bool CreateCPUCaches(const game_info& game, bool is_fast_compilation = false);
	bool RemoveCustomConfiguration(const std::string& serial, const game_info& game = nullptr, bool is_interactive = false);
	bool RemoveCustomPadConfiguration(const std::string& serial, const game_info& game = nullptr, bool is_interactive = false);
	bool RemoveShaderCache(const std::string& serial, bool is_interactive = false);
	bool RemovePPUCache(const std::string& serial, bool is_interactive = false);
	bool RemoveSPUCache(const std::string& serial, bool is_interactive = false);
	bool RemoveHDD1Cache(const std::string& serial, bool is_interactive = false);
	bool RemoveAllCaches(const std::string& serial, bool is_interactive = false);
	bool RemoveContentList(const std::string& serial, bool is_interactive = false);

	void BatchCreateCPUCaches(const std::vector<game_info>& games = {}, bool is_fast_compilation = false, bool is_interactive = false);
	void BatchRemoveCustomConfigurations(const std::vector<game_info>& games = {}, bool is_interactive = false);
	void BatchRemoveCustomPadConfigurations(const std::vector<game_info>& games = {}, bool is_interactive = false);
	void BatchRemoveShaderCaches(const std::vector<game_info>& games = {}, bool is_interactive = false);
	void BatchRemovePPUCaches(const std::vector<game_info>& games = {}, bool is_interactive = false);
	void BatchRemoveSPUCaches(const std::vector<game_info>& games = {}, bool is_interactive = false);
	void BatchRemoveHDD1Caches(const std::vector<game_info>& games = {}, bool is_interactive = false);
	void BatchRemoveAllCaches(const std::vector<game_info>& games = {}, bool is_interactive = false);
	void BatchRemoveContentLists(const std::vector<game_info>& games = {}, bool is_interactive = false);

	static bool RemoveContentPath(const std::string& path, const std::string& desc);
	static u32 RemoveContentPathList(const std::set<std::string>& path_list, const std::string& desc);
	static bool RemoveContentBySerial(const std::string& base_dir, const std::string& serial, const std::string& desc);

private:
	game_list_frame* m_game_list_frame = nullptr;
	std::shared_ptr<gui_settings> m_gui_settings;
	QFuture<void> m_disk_usage_future;

	// NOTE:
	//   m_content_info is used by:
	//   - SetContentList()
	//   - ClearContentList()
	//   - GetContentInfo()
	//   - RemoveContentList()
	//   - BatchRemoveContentLists()
	//
	content_info m_content_info;

	void BatchActionBySerials(progress_dialog* pdlg, const std::set<std::string>& serials,
		QString progressLabel, std::function<bool(const std::string&)> action,
		std::function<void(u32, u32)> cancel_log, std::function<void()> action_on_finish, bool refresh_on_finish,
		bool can_be_concurrent = false, std::function<bool()> should_wait_cb = {});
};
