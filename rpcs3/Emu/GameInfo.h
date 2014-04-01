#pragma once

struct GameInfo
{
	std::string root;

	std::string name;
	std::string serial;
	std::string app_ver;
	std::string category;
	std::string fw;

	u32 attr;
	u32 bootable;
	u32 parental_lvl;
	u32 sound_format;
	u32 resolution;

	GameInfo()
	{
		Reset();
	}

	void Reset()
	{
		root = "";

		name = "Unknown";
		serial = "Unknown";
		app_ver = "Unknown";
		category = "Unknown";
		fw = "Unknown";

		attr = 0;
		bootable = 0;
		parental_lvl = 0;
		sound_format = 0;
		resolution = 0;
	}
};

extern GameInfo CurGameInfo;