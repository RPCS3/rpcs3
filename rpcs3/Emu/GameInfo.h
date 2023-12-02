#pragma once

#include "util/types.hpp"
#include <string>

struct GameInfo
{
	std::string path;
	std::string icon_path;
	std::string movie_path;

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
};
