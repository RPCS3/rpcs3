#include <stdlib.h>

#include "GS.h"
#include "Win32.h"

extern HINSTANCE hInst;


void SaveConfig()
{
	wxChar szValue[256];
	const wxString iniFile( wxString::FromUTF8(s_strIniPath.c_str()) + L"zzogl-pg.ini");

	wxSprintf(szValue, L"%u", conf.interlace);
	WritePrivateProfileString(L"Settings", L"Interlace", szValue, iniFile);
	wxSprintf(szValue, L"%u", conf.aa);
	WritePrivateProfileString(L"Settings", L"Antialiasing", szValue, iniFile);
	wxSprintf(szValue, L"%u", conf.bilinear);
	WritePrivateProfileString(L"Settings", L"Bilinear", szValue, iniFile);
	wxSprintf(szValue, L"%u", conf.zz_options);
	WritePrivateProfileString(L"Settings", L"ZZOptions", szValue, iniFile);
	wxSprintf(szValue, L"%u", conf.hacks);
	WritePrivateProfileString(L"Settings", L"AdvancedOptions", szValue, iniFile);
	wxSprintf(szValue, L"%u", conf.width);
	WritePrivateProfileString(L"Settings", L"Width", szValue, iniFile);
	wxSprintf(szValue, L"%u", conf.height);
	WritePrivateProfileString(L"Settings", L"Height", szValue, iniFile);
	wxSprintf(szValue, L"%u", conf.SkipDraw);
	WritePrivateProfileString(L"Settings", L"SkipDraw", szValue, iniFile);
}

void LoadConfig()
{
	wxChar szValue[256];
	const wxString iniFile( wxString::FromUTF8(s_strIniPath.c_str()) + L"zzogl-pg.ini");

	memset(&conf, 0, sizeof(conf));
	conf.interlace = 0; // on, mode 1
	conf.mrtdepth = 1;
	conf.zz_options._u32 = 0;
	conf.hacks._u32 = 0;
	conf.bilinear = 1;
	conf.width = 640;
	conf.height = 480;
	conf.SkipDraw = 0;
	conf.disableHacks = 0;

	FILE *fp = wxFopen(iniFile, L"rt");

	if (!fp)
	{
		SysMessage("Unable to open ZZOgl-PG's ini file!");
		CreateDirectory(wxString::FromUTF8(s_strIniPath.c_str()), NULL);
		SaveConfig();//save and return
		return ;
	}

	fclose(fp);

	GetPrivateProfileString(L"Settings", L"Interlace", NULL, szValue, 20, iniFile);
	conf.interlace = (u8)wxStrtoul(szValue, NULL, 10);
	GetPrivateProfileString(L"Settings", L"Antialiasing", NULL, szValue, 20, iniFile);
	conf.aa = (u8)wxStrtoul(szValue, NULL, 10);
	GetPrivateProfileString(L"Settings", L"ZZOptions", NULL, szValue, 20, iniFile);
	conf.zz_options._u32 = wxStrtoul(szValue, NULL, 10);
	GetPrivateProfileString(L"Settings", L"AdvancedOptions", NULL, szValue, 20, iniFile);
	conf.hacks._u32 = wxStrtoul(szValue, NULL, 10);
	GetPrivateProfileString(L"Settings", L"Bilinear", NULL, szValue, 20, iniFile);
	conf.bilinear = (u8)wxStrtoul(szValue, NULL, 10);
	GetPrivateProfileString(L"Settings", L"Width", NULL, szValue, 20, iniFile);
	conf.width = wxStrtoul(szValue, NULL, 10);
	GetPrivateProfileString(L"Settings", L"Height", NULL, szValue, 20, iniFile);
	conf.height = wxStrtoul(szValue, NULL, 10);
	GetPrivateProfileString(L"Settings", L"SkipDraw", NULL, szValue, 20, iniFile);
	conf.SkipDraw = wxStrtoul(szValue, NULL, 10);

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
