#pragma once

struct gui_game_info;

namespace gui::utils
{
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
	                     shortcut_location shortcut_location);

	bool create_shortcuts(const std::shared_ptr<gui_game_info>& game,
	                      const std::set<gui::utils::shortcut_location>& locations);

	void remove_shortcuts(const std::string& name, const std::string& serial);
}
