/*  GSnull
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <stdlib.h>

#include "../GS.h"

extern HINSTANCE hInst;
void SaveConfig() 
{

    Config *Conf1 = &conf;
	char *szTemp;
	char szIniFile[256], szValue[256];

	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return;
	strcpy(szTemp, "\\inis\\gsnull.ini");
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
	strcpy(szTemp, "\\inis\\gsnull.ini");
    fp=fopen("inis\\gsnull.ini","rt");//check if gsnull.ini really exists
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

