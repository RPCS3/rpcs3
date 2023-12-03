#pragma once

#include "util/types.hpp"
#include <string>

struct GameInfo
{
	std::string path;
	std::string icon_path;
	std::string movie_path;

	std::string name;
	std::string serial;
	std::string app_ver;
	std::string version;
	std::string category;
	std::string fw;

	u32 attr = 0;
	u32 bootable = 0;
	u32 parental_lvl = 0;
	u32 sound_format = 0;
	u32 resolution = 0;

	u64 size_on_disk = umax;
};
