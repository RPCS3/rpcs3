#pragma once

#ifdef RPCS3_RA_ENABLED

#include "rc_client.h"
#include <functional>
#include <string>
#include <vector>

namespace rpcs3::ra
{
	void initialize();
	void shutdown();

	void on_game_start();
	void on_game_stop();
	void on_frame_end();
	void on_pause();
	void on_resume();

	bool is_active();
	std::string get_token();

	void login(const std::string& username, const std::string& password,
	           std::function<void(bool)> callback = nullptr);
	void login_with_token(const std::string& username, const std::string& token);
	void logout();

#ifdef RC_CLIENT_SUPPORTS_RAINTEGRATION
	struct RAIntegrationMenuItem
	{
		uint32_t id;
		std::string label;
		bool checked;
	};
	std::vector<RAIntegrationMenuItem> get_menu_items();
	void load_integration(void* hwnd);
	void set_main_window(void* hwnd);
	void activate_menu_item(uint32_t id);
#endif

} // namespace rpcs3::ra

#endif // RPCS3_RA_ENABLED
