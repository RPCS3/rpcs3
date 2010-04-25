/*  ZeroPAD - author: zerofrog(@gmail.com)
 *  Copyright (C) 2006-2007
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

 #include "win.h"

 void SaveConfig()
{
	char *szTemp;
	char szIniFile[256], szValue[256], szProf[256];
	int i, j;

	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if (!szTemp) return;
	strcpy(szTemp, "\\inis\\zeropad.ini");

	for (j = 0; j < 2; j++)
	{
		for (i = 0; i < PADKEYS; i++)
		{
			sprintf(szProf, "%d_%d", j, i);
			sprintf(szValue, "%d", conf.keys[j][i]);
			WritePrivateProfileString("Interface", szProf, szValue, szIniFile);
		}
	}

	sprintf(szValue, "%u", conf.log);
	WritePrivateProfileString("Interface", "Logging", szValue, szIniFile);
}

void LoadConfig()
{
	FILE *fp;
	char *szTemp;
	char szIniFile[256], szValue[256], szProf[256];
	int i, j;

	memset(&conf, 0, sizeof(conf));
#ifdef _WIN32
	conf.keys[0][0] = 'W';                      // L2
	conf.keys[0][1] = 'O';                      // R2
	conf.keys[0][2] = 'A';                      // L1
	conf.keys[0][3] = ';';                      // R1
	conf.keys[0][4] = 'I';                      // TRIANGLE
	conf.keys[0][5] = 'L';                      // CIRCLE
	conf.keys[0][6] = 'K';                      // CROSS
	conf.keys[0][7] = 'J';                      // SQUARE
	conf.keys[0][8] = 'V';      // SELECT
	conf.keys[0][11] = 'N';     // START
	conf.keys[0][12] = 'E';     // UP
	conf.keys[0][13] = 'F';     // RIGHT
	conf.keys[0][14] = 'D';     // DOWN
	conf.keys[0][15] = 'S';     // LEFT
#endif
	conf.log = 0;

	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if (!szTemp) return ;
	strcpy(szTemp, "\\inis\\zeropad.ini");
	fp = fopen("inis\\zeropad.ini", "rt");//check if usbnull.ini really exists
	if (!fp)
	{
		CreateDirectory("inis", NULL);
		SaveConfig();//save and return
		return ;
	}
	fclose(fp);

	for (j = 0; j < 2; j++)
	{
		for (i = 0; i < PADKEYS; i++)
		{
			sprintf(szProf, "%d_%d", j, i);
			GetPrivateProfileString("Interface", szProf, NULL, szValue, 20, szIniFile);
			conf.keys[j][i] = strtoul(szValue, NULL, 10);
		}
	}

	GetPrivateProfileString("Interface", "Logging", NULL, szValue, 20, szIniFile);
	conf.log = strtoul(szValue, NULL, 10);
}

void SysMessage(char *fmt, ...)
{
	va_list list;
	char tmp[512];

	va_start(list, fmt);
	vsprintf(tmp, fmt, list);
	va_end(list);
	MessageBox(0, tmp, "PADwinKeyb Msg", 0);
}
