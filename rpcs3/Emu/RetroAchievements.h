#pragma once

#ifdef RPCS3_RA_ENABLED

#include "rc_client.h"
#include <ctime>
#include <functional>
#include <string>
#include <vector>

namespace rpcs3::ra
{
	struct ra_achievement_item
	{
		uint32_t    id;
		std::string title;
		std::string description;
		std::string badge_name;
		std::string measured_progress;
		float       measured_percent;
		uint32_t    points;
		float       rarity;
		float       rarity_hardcore;
		time_t      unlock_time;
		uint8_t     state;   // RC_CLIENT_ACHIEVEMENT_STATE_*
		uint8_t     bucket;  // RC_CLIENT_ACHIEVEMENT_BUCKET_*
		uint8_t     unlocked;
	};

	struct ra_game_item
	{
		uint32_t    id;
		std::string title;
		uint32_t    num_achievements;
		uint32_t    num_unlocked;
		uint32_t    num_unlocked_hardcore;
		bool        is_current_game;
	};

	std::vector<ra_achievement_item> get_achievement_list();
	ra_game_item                     get_current_game_item();
	std::string                      get_username();

	// Async – callback fires on a background thread
	void fetch_recently_played_games(std::function<void(std::vector<ra_game_item>, bool /*success*/)> callback);
	void fetch_game_achievements_for_id(uint32_t game_id,
	                                    std::function<void(std::vector<ra_achievement_item>, std::string /*title*/, bool /*success*/)> callback);

	void initialize();
	void shutdown();

	void on_game_start();
	void on_game_stop();
	void on_frame_end();
	bool is_active();
	bool is_integration_loaded();
	std::string get_token();
	uint32_t get_user_score();

	void login(const std::string& username, const std::string& password,
	           std::function<void(bool)> callback = nullptr);
	void login_with_token(const std::string& username, const std::string& token);
	void logout();
	void set_hardcore(bool enabled);
	void set_unofficial(bool enabled);
	void set_encore(bool enabled);
	void set_spectator(bool enabled);
	bool get_hardcore_mode();
	bool consume_pending_hc_restart();
	std::string get_rich_presence_message();
	std::string get_discord_state();

#ifdef RC_CLIENT_SUPPORTS_RAINTEGRATION
	struct RAIntegrationMenuItem
	{
		uint32_t id;
		std::string label;
		bool checked;
		bool enabled;
	};
	std::vector<RAIntegrationMenuItem> get_menu_items();
	void load_integration(void* hwnd);
	void set_main_window(void* hwnd);
	void activate_menu_item(uint32_t id);
	void cancel_hc_enable();
	bool query_client_hardcore_state();
#endif

} // namespace rpcs3::ra

#endif // RPCS3_RA_ENABLED
