#include <stdlib.h>

#include "GS.h"
#include "Win32.h"

extern HINSTANCE hInst;


void SaveConfig() {

	char *szTemp;
	char szIniFile[256], szValue[256];

	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return;
	strcpy(szTemp, "\\inis\\zerogs.ini");

	sprintf(szValue,"%u",conf.interlace);
	WritePrivateProfileString("Settings", "Interlace",szValue,szIniFile);
	sprintf(szValue,"%u",conf.aa);
	WritePrivateProfileString("Settings", "Antialiasing",szValue,szIniFile);
	sprintf(szValue,"%u",conf.bilinear);
	WritePrivateProfileString("Settings", "Bilinear",szValue,szIniFile);
	sprintf(szValue,"%u",conf.options);
	WritePrivateProfileString("Settings", "Options",szValue,szIniFile);
	sprintf(szValue,"%u",conf.gamesettings);
	WritePrivateProfileString("Settings", "AdvancedOptions",szValue,szIniFile);
}

void LoadConfig() {

	FILE *fp;
	char *szTemp;
	char szIniFile[256], szValue[256];
  
	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	memset(&conf, 0, sizeof(conf));
	conf.interlace = 0; // on, mode 1
	conf.mrtdepth = 1;
	conf.options = 0;
	conf.bilinear = 1;
	conf.width = 640;
	conf.height = 480;

	if(!szTemp) return ;
	strcpy(szTemp, "\\inis\\zerogs.ini");
	fp=fopen("inis\\zerogs.ini","rt");
	if (!fp)
	{
		CreateDirectory("inis",NULL);
		SaveConfig();//save and return
		return ;
	}
	fclose(fp);

	GetPrivateProfileString("Settings", "Interlace", NULL, szValue, 20, szIniFile);
	conf.interlace = (u8)strtoul(szValue, NULL, 10);
	GetPrivateProfileString("Settings", "Antialiasing", NULL, szValue, 20, szIniFile);
	conf.aa = (u8)strtoul(szValue, NULL, 10);
	GetPrivateProfileString("Settings", "Options", NULL, szValue, 20, szIniFile);
	conf.options = strtoul(szValue, NULL, 10);
	GetPrivateProfileString("Settings", "AdvancedOptions", NULL, szValue, 20, szIniFile);
	conf.gamesettings = strtoul(szValue, NULL, 10);
	GetPrivateProfileString("Settings", "Bilinear", NULL, szValue, 20, szIniFile);
	conf.bilinear = strtoul(szValue, NULL, 10);

	if( conf.aa < 0 || conf.aa > 2 ) conf.aa = 0;

	switch(conf.options&GSOPTION_WINDIMS) {
		case GSOPTION_WIN640:
			conf.width = 640;
			conf.height = 480;
			break;
		case GSOPTION_WIN800:
			conf.width = 800;
			conf.height = 600;
			break;
		case GSOPTION_WIN1024:
			conf.width = 1024;
			conf.height = 768;
			break;
		case GSOPTION_WIN1280:
			conf.width = 1280;
			conf.height = 960;
			break;
	}
	
	// turn off all hacks by defaultof
	conf.options &= ~(GSOPTION_FULLSCREEN|GSOPTION_WIREFRAME|GSOPTION_CAPTUREAVI);
	conf.options |= GSOPTION_LOADED;

	if( conf.width <= 0 || conf.height <= 0 ) {
		conf.width = 640;
		conf.height = 480;
	}
}
