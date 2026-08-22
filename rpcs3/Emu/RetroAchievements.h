#pragma once

#ifdef RPCS3_RA_ENABLED

#include "rc_client.h"
#include <functional>
#include <string>

namespace rpcs3::ra
{
	void initialize();
	void shutdown();

	void on_game_start(const std::string& game_path);
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

} // namespace rpcs3::ra

#endif // RPCS3_RA_ENABLED
