#pragma once

namespace gui::utils
{
	enum shortcut_location
	{
		desktop,
		applications,
#ifdef _WIN32
		rpcs3_shortcuts,
#endif
	};

	bool create_shortcut(const std::string& name,
	                     const std::string& serial,
	                     const std::string& target_cli_args,
	                     const std::string& description,
	                     const std::string& src_icon_path,
	                     const std::string& target_icon_dir,
	                     shortcut_location shortcut_location);
}
