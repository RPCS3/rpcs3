#pragma once

#include "util/types.hpp"
#include <string>

enum class game_content_type
{
	content_icon,         // ICON0.PNG
	content_video,        // ICON1.PAM
	content_sound,        // SND0.AT3
	overlay_picture,      // PIC0.PNG (16:9) or PIC2.PNG (4:3)
	background_picture,   // PIC1.PNG
	background_picture_2, // PIC3.PNG (should only exist for install or extra content discs...)
};

namespace rpcs3::utils
{
	u32 get_max_threads();

	void configure_logs(bool force_enable = false);

	u32 check_user(const std::string& user);

	bool install_pkg(const std::string& path);

	// VFS directories and disk usage
	std::vector<std::pair<std::string, u64>> get_vfs_disk_usage();
	std::string get_emu_dir();
	std::string get_games_dir();
	std::string get_hdd0_dir();
	std::string get_hdd1_dir();
	std::string get_flash_dir();
	std::string get_flash2_dir();
	std::string get_flash3_dir();
	std::string get_bdvd_dir();

	// Cache directories and disk usage
	u64 get_cache_disk_usage();
	std::string get_cache_dir();
	std::string get_cache_dir(std::string_view module_path);

	std::string get_rap_file_path(const std::string_view& rap);
	bool verify_c00_unlock_edat(const std::string_view& content_id, bool fast = false);
	std::string get_sfo_dir_from_game_path(const std::string& game_path, const std::string& title_id = "");

	std::string get_custom_config_dir();
	std::string get_custom_config_path(const std::string& identifier);

	std::string get_input_config_root();
	std::string get_input_config_dir(const std::string& title_id = "");
	std::string get_custom_input_config_path(const std::string& title_id);

	std::string get_game_content_path(game_content_type type);

	bool version_is_bigger(std::string_view v0, std::string_view v1, std::string_view serial, bool is_fw);
}
