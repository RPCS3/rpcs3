#pragma once

#include "util/types.hpp"
#include <string>

struct GameInfo
{
	std::string path;
	std::string icon_path;

	std::string name = "Unknown";
	std::string serial = "Unknown";
	std::string app_ver = "Unknown";
	std::string version = "Unknown";
	std::string category = "Unknown";
	std::string fw = "Unknown";

	u32 attr = 0;
	u32 bootable = 0;
	u32 parental_lvl = 0;
	u32 sound_format = 0;
	u32 resolution = 0;

	u64 size_on_disk = umax;

	std::string get_pam_path() const
	{
		if (icon_path.empty())
		{
			return {};
		}
		return icon_path.substr(0, icon_path.find_last_of("/")) + "/ICON1.PAM";
	}
};
