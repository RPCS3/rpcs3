#pragma once

#include "Utilities/types.h"
#include <string>

struct GameInfo
{
	std::string path;

	std::string icon_path;
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

	GameInfo()
	{
		Reset();
	}

	void Reset()
	{
		path.clear();

		name = "Unknown";
		serial = "Unknown";
		app_ver = "Unknown";
		version = "Unknown";
		category = "Unknown";
		fw = "Unknown";

		attr = 0;
		bootable = 0;
		parental_lvl = 0;
		sound_format = 0;
		resolution = 0;
	}
};
