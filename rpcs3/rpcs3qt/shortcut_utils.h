#pragma once

namespace gui::utils
{
	bool create_shortcut(const std::string& name,
	                     const std::string& target_cli_args,
	                     const std::string& description,
	                     const std::string& src_icon_path,
	                     const std::string& target_icon_dir,
	                     bool is_desktop_shortcut);
}
