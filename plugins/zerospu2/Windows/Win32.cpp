/*  ZeroSPU2
 *  Copyright (C) 2006-2007 zerofrog
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

 #include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#include "../zerospu2.h"
#include "resource.h"

extern HINSTANCE hInst;
extern HWND hWMain;
/////////
// GUI //
/////////
HINSTANCE hInst;

void SysMessage(const char *fmt, ...)
{
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	vsprintf_s(tmp,fmt,list);
	va_end(list);

	MessageBox((hWMain==NULL) ? GetActiveWindow() : hWMain, tmp, "ZeroSPU2 Msg", MB_SETFOREGROUND | MB_OK);
}

BOOL CALLBACK ConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch(uMsg)
	{
		case WM_INITDIALOG:
			LoadConfig();
			if (conf.Log) CheckDlgButton(hW, IDC_LOGGING, TRUE);
			if( conf.options & OPTION_REALTIME)
				CheckDlgButton(hW, IDC_REALTIME, TRUE);
			if( conf.options & OPTION_TIMESTRETCH)
				CheckDlgButton(hW, IDC_TIMESTRETCH, TRUE);
			if( conf.options & OPTION_MUTE)
				CheckDlgButton(hW, IDC_MUTESOUND, TRUE);
			if( conf.options & OPTION_RECORDING)
				CheckDlgButton(hW, IDC_SNDRECORDING, TRUE);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDCANCEL:
					EndDialog(hW, TRUE);
					return TRUE;
				case IDOK:
					conf.options = 0;
					if (IsDlgButtonChecked(hW, IDC_LOGGING))
						 conf.Log = 1;
					else
						conf.Log = 0;

					if (IsDlgButtonChecked(hW, IDC_REALTIME))
						conf.options |= OPTION_REALTIME;
					if (IsDlgButtonChecked(hW, IDC_TIMESTRETCH))
						conf.options |= OPTION_TIMESTRETCH;
					if (IsDlgButtonChecked(hW, IDC_MUTESOUND))
						conf.options |= OPTION_MUTE;
					if (IsDlgButtonChecked(hW, IDC_SNDRECORDING))
						conf.options |= OPTION_RECORDING;

					SaveConfig();
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

void CALLBACK SPU2configure()
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIG), GetActiveWindow(), (DLGPROC)ConfigureDlgProc);
}

void CALLBACK SPU2about()
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUT), GetActiveWindow(), (DLGPROC)AboutDlgProc);
}


 // DLL INIT
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved)
{
	hInst = (HINSTANCE)hModule;
	return TRUE;                                          // very quick :)
}

void SaveConfig()
{
	Config *Conf1 = &conf;
	char szValue[256];

	string iniFile( s_strIniPath + "zerospu2.ini" );

	sprintf_s(szValue,"%u",Conf1->Log);
	WritePrivateProfileString("Interface", "Logging",szValue, iniFile.c_str());
	sprintf_s(szValue,"%u",Conf1->options);
	WritePrivateProfileString("Interface", "Options",szValue, iniFile.c_str());
}

void LoadConfig()
{
	FILE *fp;
	Config *Conf1 = &conf;
	char szValue[256];

	string iniFile( s_strIniPath + "zerospu2.ini" );

	fopen_s(&fp, iniFile.c_str(), "rt");//check if zerospu2.ini really exists

	if (!fp)
	{
		CreateDirectory(s_strIniPath.c_str(), NULL);
		memset(&conf, 0, sizeof(conf));
		conf.Log = 0;//default value
		conf.options = OPTION_TIMESTRETCH;
		SaveConfig();//save and return
		return ;
	}
	fclose(fp);

	GetPrivateProfileString("Interface", "Logging", NULL, szValue, 20, iniFile.c_str());
	Conf1->Log = strtoul(szValue, NULL, 10);
	GetPrivateProfileString("Interface", "Options", NULL, szValue, 20, iniFile.c_str());
	Conf1->options = strtoul(szValue, NULL, 10);
	return;

}

