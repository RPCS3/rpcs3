/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/*
15-09-2004 : file rewriten for work with inis (shadow)
*/

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "Common.h"
#include "win32.h"
#include <sys/stat.h>

#include "Paths.h"

int LoadConfig() {
   FILE *fp;

#ifdef ENABLE_NLS
	char text[256];
	extern int _nl_msg_cat_cntr;
#endif
    PcsxConfig *Conf = &Config;
	char *szTemp;
	char szIniFile[256], szValue[256];

	GetModuleFileName(GetModuleHandle((LPCSTR)gApp.hInstance), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return -1;
	strcpy(szTemp, "\\" CONFIG_DIR "\\pcsx2.ini");
    fp=fopen(CONFIG_DIR "\\pcsx2.ini","rt");//check if pcsx2.ini really exists
	if (!fp)
	{
		CreateDirectory(CONFIG_DIR,NULL); 
		return -1;
	}
	fclose(fp);
    //interface
	GetPrivateProfileString("Interface", "Bios", NULL, szValue, 256, szIniFile);
	strcpy(Conf->Bios, szValue);
	GetPrivateProfileString("Interface", "Lang", NULL, szValue, 256, szIniFile);
	strcpy(Conf->Lang, szValue);
	GetPrivateProfileString("Interface", "Ps2Out", NULL, szValue, 20, szIniFile);
    Conf->PsxOut = strtoul(szValue, NULL, 10);
	GetPrivateProfileString("Interface", "ThPriority", NULL, szValue, 20, szIniFile);
    Conf->ThPriority = strtoul(szValue, NULL, 10);
	GetPrivateProfileString("Interface", "PluginsDir", NULL, szValue, 256, szIniFile);
	strcpy(Conf->PluginsDir, szValue);
	GetPrivateProfileString("Interface", "BiosDir", NULL, szValue, 256, szIniFile);
	strcpy(Conf->BiosDir, szValue);
	GetPrivateProfileString("Interface", "Mcd1", NULL, szValue, 256, szIniFile);
    strcpy(Conf->Mcd1, szValue);
	GetPrivateProfileString("Interface", "Mcd2", NULL, szValue, 256, szIniFile);
    strcpy(Conf->Mcd2, szValue); 
	Config.CustomFps=GetPrivateProfileInt("Interface", "CustomFps", 0, szIniFile);
    //plugins
	GetPrivateProfileString("Plugins", "GS", NULL, szValue, 256, szIniFile);
    strcpy(Conf->GS, szValue); 
    GetPrivateProfileString("Plugins", "SPU2", NULL, szValue, 256, szIniFile);
    strcpy(Conf->SPU2, szValue);
	GetPrivateProfileString("Plugins", "CDVD", NULL, szValue, 256, szIniFile);
    strcpy(Conf->CDVD, szValue);
	GetPrivateProfileString("Plugins", "PAD1", NULL, szValue, 256, szIniFile);
    strcpy(Conf->PAD1, szValue);
	GetPrivateProfileString("Plugins", "PAD2", NULL, szValue, 256, szIniFile);
    strcpy(Conf->PAD2, szValue);
	GetPrivateProfileString("Plugins", "DEV9", NULL, szValue, 256, szIniFile);
    strcpy(Conf->DEV9, szValue);
	GetPrivateProfileString("Plugins", "USB", NULL, szValue, 256, szIniFile);
    strcpy(Conf->USB, szValue);
	GetPrivateProfileString("Plugins", "FW", NULL, szValue, 256, szIniFile);
    strcpy(Conf->FW, szValue);
	//cpu
	GetPrivateProfileString("Cpu Options", "Options", NULL, szValue, 20, szIniFile);
    Conf->Options= (u32)strtoul(szValue, NULL, 10);
	
	//Misc
	GetPrivateProfileString("Misc", "Patch", NULL, szValue, 20, szIniFile);
    Conf->Patch = strtoul(szValue, NULL, 10);

#ifdef PCSX2_DEVBUILD
	GetPrivateProfileString("Misc", "varLog", NULL, szValue, 20, szIniFile);
    varLog = strtoul(szValue, NULL, 16);
#endif
	GetPrivateProfileString("Misc", "Hacks", NULL, szValue, 20, szIniFile);
    Conf->Hacks = strtoul(szValue, NULL, 16);

#ifdef ENABLE_NLS
	sprintf(text, "LANGUAGE=%s", Conf->Lang);
#ifdef _WIN32
	gettext_putenv(text);
#else
	putenv(text);
#endif
#endif

	return 0;
}

/////////////////////////////////////////////////////////

void SaveConfig() {
	
    PcsxConfig *Conf = &Config;
	char *szTemp;
	char szIniFile[256], szValue[256];

	GetModuleFileName(GetModuleHandle((LPCSTR)gApp.hInstance), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return;
	strcpy(szTemp, "\\" CONFIG_DIR "\\pcsx2.ini");
    //interface
    sprintf(szValue,"%s",Conf->Bios);
    WritePrivateProfileString("Interface","Bios",szValue,szIniFile);
    sprintf(szValue,"%s",Conf->Lang);
    WritePrivateProfileString("Interface","Lang",szValue,szIniFile);
    sprintf(szValue,"%s",Conf->PluginsDir);
    WritePrivateProfileString("Interface","PluginsDir",szValue,szIniFile);
    sprintf(szValue,"%s",Conf->BiosDir);
    WritePrivateProfileString("Interface","BiosDir",szValue,szIniFile);
    sprintf(szValue,"%u",Conf->PsxOut);
    WritePrivateProfileString("Interface","Ps2Out",szValue,szIniFile);
    sprintf(szValue,"%u",Conf->ThPriority);
    WritePrivateProfileString("Interface","ThPriority",szValue,szIniFile);
    sprintf(szValue,"%s",Conf->Mcd1);
    WritePrivateProfileString("Interface","Mcd1",szValue,szIniFile);
    sprintf(szValue,"%s",Conf->Mcd2);
    WritePrivateProfileString("Interface","Mcd2",szValue,szIniFile);
    sprintf(szValue,"%d",Conf->CustomFps);
	WritePrivateProfileString("Interface", "CustomFps", szValue, szIniFile);
    //plugins
	sprintf(szValue,"%s",Conf->GS);
    WritePrivateProfileString("Plugins","GS",szValue,szIniFile);
	sprintf(szValue,"%s",Conf->SPU2);
    WritePrivateProfileString("Plugins","SPU2",szValue,szIniFile);
    sprintf(szValue,"%s",Conf->CDVD);
    WritePrivateProfileString("Plugins","CDVD",szValue,szIniFile);
    sprintf(szValue,"%s",Conf->PAD1);
    WritePrivateProfileString("Plugins","PAD1",szValue,szIniFile);
    sprintf(szValue,"%s",Conf->PAD2);
    WritePrivateProfileString("Plugins","PAD2",szValue,szIniFile);
    sprintf(szValue,"%s",Conf->DEV9);
    WritePrivateProfileString("Plugins","DEV9",szValue,szIniFile);
    sprintf(szValue,"%s",Conf->USB);
    WritePrivateProfileString("Plugins","USB",szValue,szIniFile);
	sprintf(szValue,"%s",Conf->FW);
    WritePrivateProfileString("Plugins","FW",szValue,szIniFile);
	//cpu
    sprintf(szValue,"%u", Conf->Options);
    WritePrivateProfileString("Cpu Options","Options",szValue,szIniFile);
	//Misc
    sprintf(szValue,"%u",Conf->Patch);
    WritePrivateProfileString("Misc","Patch",szValue,szIniFile);
	sprintf(szValue,"%x",varLog);
    WritePrivateProfileString("Misc","varLog",szValue,szIniFile);
	sprintf(szValue,"%u",Conf->Hacks);
    WritePrivateProfileString("Misc","Hacks",szValue,szIniFile);


}

