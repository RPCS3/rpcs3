#include <stdlib.h>

#include "GS.h"
#include "Win32.h"

extern HINSTANCE hInst;


void SaveConfig() {

    GSconf *Conf1 = &conf;
	char *szTemp;
	char szIniFile[256], szValue[256];

	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return;
	strcpy(szTemp, "\\inis\\gssoft.ini");

	sprintf(szValue,"%u",Conf1->fmode.height);
    WritePrivateProfileString("Settings", "FmodeHeight",szValue,szIniFile);
	sprintf(szValue,"%u",Conf1->fmode.width);
    WritePrivateProfileString("Settings", "FmodeWidth",szValue,szIniFile);
	sprintf(szValue,"%u",Conf1->wmode.height);
    WritePrivateProfileString("Settings", "WmodeHeight",szValue,szIniFile);
	sprintf(szValue,"%u",Conf1->wmode.width);
    WritePrivateProfileString("Settings", "WmodeWidth",szValue,szIniFile);
	sprintf(szValue,"%u",Conf1->fullscreen);
    WritePrivateProfileString("Settings", "Fullscreen",szValue,szIniFile);
	sprintf(szValue,"%u",Conf1->fps);
    WritePrivateProfileString("Settings", "Fps",szValue,szIniFile);
	sprintf(szValue,"%u",Conf1->frameskip);
    WritePrivateProfileString("Settings", "FrameSkip",szValue,szIniFile);
	sprintf(szValue,"%u",Conf1->record);
    WritePrivateProfileString("Settings", "Record",szValue,szIniFile);
	sprintf(szValue,"%u",Conf1->cache);
    WritePrivateProfileString("Settings", "Cache",szValue,szIniFile);
	sprintf(szValue,"%u",Conf1->cachesize);
    WritePrivateProfileString("Settings", "Cachesize",szValue,szIniFile);
	sprintf(szValue,"%u",Conf1->codec);
    WritePrivateProfileString("Settings", "Codec",szValue,szIniFile);
	sprintf(szValue,"%u",Conf1->filter);
    WritePrivateProfileString("Settings", "Filter",szValue,szIniFile);
	sprintf(szValue,"%u",Conf1->log);
    WritePrivateProfileString("Settings", "Log",szValue,szIniFile);
}

void LoadConfig() {

    FILE *fp;
    GSconf *Conf1 = &conf;  
	char *szTemp;
	char szIniFile[256], szValue[256];
  
	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return ;
	strcpy(szTemp, "\\inis\\gssoft.ini");
    fp=fopen("inis\\gssoft.ini","rt");
	if (!fp)
	{
		CreateDirectory("inis",NULL); 
	    memset(&conf, 0, sizeof(conf));
	    conf.fmode.width  = 640;
	    conf.fmode.height = 480;
	    conf.wmode.width  = 640;
	    conf.wmode.height = 480;
	    conf.fullscreen   = 0;
	    conf.cachesize    = 128;
		SaveConfig();//save and return
		return ;
	}
	fclose(fp);



   	GetPrivateProfileString("Settings", "FmodeHeight", NULL, szValue, 20, szIniFile);
	Conf1->fmode.height = strtoul(szValue, NULL, 10);
   	GetPrivateProfileString("Settings", "FmodeWidth", NULL, szValue, 20, szIniFile);
	Conf1->fmode.width= strtoul(szValue, NULL, 10);
   	GetPrivateProfileString("Settings", "WmodeHeight", NULL, szValue, 20, szIniFile);
	Conf1->wmode.height= strtoul(szValue, NULL, 10);
   	GetPrivateProfileString("Settings", "WmodeWidth", NULL, szValue, 20, szIniFile);
	Conf1->wmode.width= strtoul(szValue, NULL, 10);
   	GetPrivateProfileString("Settings", "Fullscreen", NULL, szValue, 20, szIniFile);
	Conf1->fullscreen= strtoul(szValue, NULL, 10);
   	GetPrivateProfileString("Settings", "Fps", NULL, szValue, 20, szIniFile);
	Conf1->fps= strtoul(szValue, NULL, 10);
   	GetPrivateProfileString("Settings", "FrameSkip", NULL, szValue, 20, szIniFile);
	Conf1->frameskip= strtoul(szValue, NULL, 10);
   	GetPrivateProfileString("Settings", "Record", NULL, szValue, 20, szIniFile);
	Conf1->record = strtoul(szValue, NULL, 10);
   	GetPrivateProfileString("Settings", "Cache", NULL, szValue, 20, szIniFile);
	Conf1->cache= strtoul(szValue, NULL, 10);
   	GetPrivateProfileString("Settings", "Cachesize", NULL, szValue, 20, szIniFile);
	Conf1->cachesize= strtoul(szValue, NULL, 10);
   	GetPrivateProfileString("Settings", "Codec", NULL, szValue, 20, szIniFile);
	Conf1->codec= strtoul(szValue, NULL, 10);
   	GetPrivateProfileString("Settings", "Filter", NULL, szValue, 20, szIniFile);
	Conf1->filter= strtoul(szValue, NULL, 10);
   	GetPrivateProfileString("Settings", "Log", NULL, szValue, 20, szIniFile);
	Conf1->log = strtoul(szValue, NULL, 10);


	if (conf.fullscreen) cmode = &conf.fmode;
	else cmode = &conf.wmode;
}

