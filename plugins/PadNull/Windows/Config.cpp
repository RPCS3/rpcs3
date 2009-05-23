/*  PadNull
 *  Copyright (C) 2004-2009 PCSX2 Team
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>

#include "../Pad.h"

extern HINSTANCE hInst;
void SaveConfig() 
{

    Config *Conf1 = &conf;
	char *szTemp;
	char szIniFile[256], szValue[256];

	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return;
	strcpy(szTemp, "\\inis\\fwnull.ini");
	sprintf(szValue,"%u",Conf1->Log);
    WritePrivateProfileString("Interface", "Logging",szValue,szIniFile);

}

void LoadConfig() {
   FILE *fp;


    Config *Conf1 = &conf;
	char *szTemp;
	char szIniFile[256], szValue[256];
  
	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return ;
	strcpy(szTemp, "\\inis\\fwnull.ini");
    fp=fopen("inis\\fwnull.ini","rt");//check if firewirenull.ini really exists
	if (!fp)
	{
		CreateDirectory("inis",NULL); 
        memset(&conf, 0, sizeof(conf));
        conf.Log = 0;//default value
		SaveConfig();//save and return
		return ;
	}
	fclose(fp);
	GetPrivateProfileString("Interface", "Logging", NULL, szValue, 20, szIniFile);
	Conf1->Log = strtoul(szValue, NULL, 10);
	return ;

}

