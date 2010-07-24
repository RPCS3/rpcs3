#include <stdlib.h>

#include "GS.h"
#include "Win32.h"

extern HINSTANCE hInst;


void SaveConfig()
{
	char szValue[256];
	const std::string iniFile(s_strIniPath + "zzogl-pg.ini");

	sprintf(szValue, "%u", conf.interlace);
	WritePrivateProfileString("Settings", "Interlace", szValue, iniFile.c_str());
	sprintf(szValue, "%u", conf.aa);
	WritePrivateProfileString("Settings", "Antialiasing", szValue, iniFile.c_str());
	sprintf(szValue, "%u", conf.bilinear);
	WritePrivateProfileString("Settings", "Bilinear", szValue, iniFile.c_str());
	sprintf(szValue, "%u", conf.zz_options);
	WritePrivateProfileString("Settings", "ZZOptions", szValue, iniFile.c_str());
	sprintf(szValue, "%u", conf.hacks);
	WritePrivateProfileString("Settings", "AdvancedOptions", szValue, iniFile.c_str());
	sprintf(szValue, "%u", conf.width);
	WritePrivateProfileString("Settings", "Width", szValue, iniFile.c_str());
	sprintf(szValue, "%u", conf.height);
	WritePrivateProfileString("Settings", "Height", szValue, iniFile.c_str());
}

void LoadConfig()
{
	char szValue[256];
	const std::string iniFile(s_strIniPath + "zzogl-pg.ini");

	memset(&conf, 0, sizeof(conf));
	conf.interlace = 0; // on, mode 1
	conf.mrtdepth = 1;
	conf.zz_options._u32 = 0;
	conf.hacks._u32 = 0;
	conf.bilinear = 1;
	conf.width = 640;
	conf.height = 480;

	FILE *fp = fopen(iniFile.c_str(), "rt");

	if (!fp)
	{
		SysMessage("Unable to open file!");
		CreateDirectory(s_strIniPath.c_str(), NULL);
		SaveConfig();//save and return
		return ;
	}

	fclose(fp);

	GetPrivateProfileString("Settings", "Interlace", NULL, szValue, 20, iniFile.c_str());
	conf.interlace = (u8)strtoul(szValue, NULL, 10);
	GetPrivateProfileString("Settings", "Antialiasing", NULL, szValue, 20, iniFile.c_str());
	conf.aa = (u8)strtoul(szValue, NULL, 10);
	GetPrivateProfileString("Settings", "ZZOptions", NULL, szValue, 20, iniFile.c_str());
	conf.zz_options._u32 = strtoul(szValue, NULL, 10);
	GetPrivateProfileString("Settings", "AdvancedOptions", NULL, szValue, 20, iniFile.c_str());
	conf.hacks._u32 = strtoul(szValue, NULL, 10);
	GetPrivateProfileString("Settings", "Bilinear", NULL, szValue, 20, iniFile.c_str());
	conf.bilinear = (u8)strtoul(szValue, NULL, 10);
	GetPrivateProfileString("Settings", "Width", NULL, szValue, 20, iniFile.c_str());
	conf.width = strtoul(szValue, NULL, 10);
	GetPrivateProfileString("Settings", "Height", NULL, szValue, 20, iniFile.c_str());
	conf.height = strtoul(szValue, NULL, 10);

	if (conf.aa < 0 || conf.aa > 4) conf.aa = 0;

	conf.isWideScreen = (conf.widescreen() != 0);

	switch (conf.zz_options.dimensions)
	{
		case GSDim_640:
			conf.width = 640;
			conf.height = conf.isWideScreen ? 360 : 480;
			break;

		case GSDim_800:
			conf.width = 800;
			conf.height = conf.isWideScreen ? 450 : 600;
			break;

		case GSDim_1024:
			conf.width = 1024;
			conf.height = conf.isWideScreen ? 576 : 768;
			break;

		case GSDim_1280:
			conf.width = 1280;
			conf.height = conf.isWideScreen ? 720 : 960;
			break;
	}

	// turn off all hacks by default
	conf.setWireframe(false);
	conf.setCaptureAvi(false);
	conf.setLoaded(true);

	if (conf.width <= 0 || conf.height <= 0)
	{
		conf.width = 640;
		conf.height = 480;
	}
}
