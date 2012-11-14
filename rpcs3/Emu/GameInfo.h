#pragma once

struct GameInfo
{
	wxString root;

	wxString name;
	wxString serial;
	wxString app_ver;
	wxString category;
	wxString fw;

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
		root = wxEmptyString;

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