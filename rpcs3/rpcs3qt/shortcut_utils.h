#pragma once

struct gui_game_info;
class iso_archive;

namespace gui::utils
{
	class steam_shortcut;

	enum class shortcut_location
	{
		desktop,
		applications,
		steam,
#ifdef _WIN32
		rpcs3_shortcuts,
#endif
	};

	bool create_shortcut(const std::string& name,
	                     const std::string& path,
	                     const std::string& serial,
	                     const std::string& target_cli_args,
	                     const std::string& description,
	                     const std::string& src_icon_path,
	                     const std::string& target_icon_dir,
	                     const std::string& src_banner_path,
	                     shortcut_location shortcut_location,
	                     std::shared_ptr<steam_shortcut> steam_sc = nullptr,
	                     std::shared_ptr<iso_archive> archive = nullptr);

	bool batch_create_shortcuts(const std::vector<std::shared_ptr<gui_game_info>>& games,
	                            const std::set<shortcut_location>& locations);

	void batch_remove_shortcuts(const std::vector<std::pair<std::string /*name*/, std::string /*serial*/>>& games);
}
