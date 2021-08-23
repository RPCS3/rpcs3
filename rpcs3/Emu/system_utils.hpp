#pragma once

#include "util/types.hpp"
#include <string>

namespace rpcs3::utils
{
	u32 get_max_threads();

	void configure_logs();

	u32 check_user(const std::string& user);

	bool install_pkg(const std::string& path);

#ifdef _WIN32
	std::string get_exe_dir();
#endif

	std::string get_emu_dir();
	std::string get_hdd0_dir();
	std::string get_hdd1_dir();
	std::string get_cache_dir();

	std::string get_rap_file_path(const std::string& rap);
	std::string get_sfo_dir_from_game_path(const std::string& game_path, const std::string& title_id = "");

	std::string get_custom_config_dir();
	std::string get_custom_config_path(const std::string& title_id, bool get_deprecated_path = false);

	std::string get_input_config_root();
	std::string get_input_config_dir(const std::string& title_id = "");
	std::string get_custom_input_config_path(const std::string& title_id);
}
