//GiGaHeRz SPU2 Driver
//Copyright (c) David Quintana <DavidQuintana@canal21.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
//
//#include "spu2.h"
#include <windows.h>
#include <commctrl.h>
#include "resource.h"

#include "../cdvd.h"

// Config Vars

// DEBUG

char source_drive;
char source_file[MAX_PATH];

char CfgFile[MAX_PATH+10] = "inis/cdvdGigaherz.ini";

void CfgSetSettingsDir( const char* dir )
{
	// a better std::string version, but it's inconvenient for other reasons.
	//CfgFile = std::string(( dir == NULL ) ? "inis/" : dir) + "cdvdGigaherz.ini";

	strcpy_s(CfgFile, (dir==NULL) ? "inis" : dir);
	strcat_s(CfgFile, "/cdvdGigaherz.ini");
}


 /*| Config File Format: |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
+--+---------------------+------------------------+
|												  |
| Option=Value									  |
|												  |
|												  |
| Boolean Values: TRUE,YES,1,T,Y mean 'true',	  |
|                 everything else means 'false'.  |
|												  |
| All Values are limited to 255 chars.			  |
|												  |
+-------------------------------------------------+
 \*_____________________________________________*/


void CfgWriteBool(char *Section, char*Name, char Value) {
	char *Data=Value?"TRUE":"FALSE";

	WritePrivateProfileString(Section,Name,Data,CfgFile);
}

void CfgWriteInt(char *Section, char*Name, int Value) {
	char Data[255];
	_itoa(Value,Data,10);

	WritePrivateProfileString(Section,Name,Data,CfgFile);
}

void CfgWriteStr(char *Section, char*Name,char *Data) {
	WritePrivateProfileString(Section,Name,Data,CfgFile);
}

/*****************************************************************************/

char CfgReadBool(char *Section,char *Name,char Default) {
	char Data[255]="";
	GetPrivateProfileString(Section,Name,"",Data,255,CfgFile);
	Data[254]=0;
	if(strlen(Data)==0) {
		CfgWriteBool(Section,Name,Default);
		return Default;
	}

	if(strcmp(Data,"1")==0) return -1;
	if(strcmp(Data,"Y")==0) return -1;
	if(strcmp(Data,"T")==0) return -1;
	if(strcmp(Data,"YES")==0) return -1;
	if(strcmp(Data,"TRUE")==0) return -1;
	return 0;
}

int CfgReadInt(char *Section, char*Name,int Default) {
	char Data[255]="";
	GetPrivateProfileString(Section,Name,"",Data,255,CfgFile);
	Data[254]=0;

	if(strlen(Data)==0) {
		CfgWriteInt(Section,Name,Default);
		return Default;
	}

	return atoi(Data);
}

void CfgReadStr(char *Section, char*Name,char *Data,int DataSize,char *Default) {
	int sl;
	GetPrivateProfileString(Section,Name,"",Data,DataSize,CfgFile);

	if(strlen(Data)==0) {
		sl=(int)strlen(Default);
		strncpy(Data,Default,sl>255?255:sl);
		CfgWriteStr(Section,Name,Data);
	}
}

/*****************************************************************************/

void ReadSettings()
{
	char temp[512];

	CfgReadStr("Config","Source",temp,511,"-");
	source_drive=temp[0];

	if(source_drive=='$')
		strcpy_s(source_file,sizeof(source_file),temp+1);
	else
		source_file[0]=0;
}

/*****************************************************************************/

void WriteSettings()
{
	char temp[511];

	temp[0]=source_drive;
	temp[1]=0;

	strcat(temp,source_file);

	CfgWriteStr("Config","Source",temp);
}

char* path[] = {
	"A:","B:","C:","D:","E:","F:","G:","H:","I:","J:","K:","L:","M:",
	"N:","O:","P:","Q:","R:","S:","T:","U:","V:","W:","X:","Y:","Z:",
};

int n,s;

#define SET_CHECK(idc,value) SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,((value)==0)?BST_UNCHECKED:BST_CHECKED,0)
#define HANDLE_CHECK(idc,hvar)	case idc: hvar=hvar?0:1; SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar==1)?BST_CHECKED:BST_UNCHECKED,0); break
#define HANDLE_CHECKNB(idc,hvar)case idc: hvar=hvar?0:1; SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar==1)?BST_CHECKED:BST_UNCHECKED,0)
#define ENABLE_CONTROL(idc,value) EnableWindow(GetDlgItem(hWnd,idc),value)

BOOL CALLBACK ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;
	char temp[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	switch(uMsg)
	{

		case WM_PAINT:
			return FALSE;
		case WM_INITDIALOG:

			n=0;s=0;

			SendMessage(GetDlgItem(hWnd,IDC_DRIVE),CB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hWnd,IDC_DRIVE),CB_ADDSTRING,0,(LPARAM)"@ (No disc)");
			for(char d='A';d<='Z';d++)
			{
				if(GetDriveType(path[d-'A'])==DRIVE_CDROM)
				{
					n++;

					SendMessage(GetDlgItem(hWnd,IDC_DRIVE),CB_ADDSTRING,0,(LPARAM)path[d-'A']);

					if(source_drive==d)
					{
						s=n;
					}
				}
			}

			SendMessage(GetDlgItem(hWnd,IDC_DRIVE),CB_SETCURSEL,s,0);

			break;
		case WM_COMMAND:
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			// Parse the menu selections:
			switch (wmId)
			{
				case IDOK:
					GetDlgItemText(hWnd,IDC_DRIVE,temp,20);
					temp[19]=0;
					source_drive=temp[0];

					WriteSettings();
					EndDialog(hWnd,0);
					break;
				case IDCANCEL:
					EndDialog(hWnd,0);
					break;

				default:
					return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

void configure()
{
	INT_PTR ret;
	ReadSettings();
	ret=DialogBoxParam(hinst,MAKEINTRESOURCE(IDD_CONFIG),GetActiveWindow(),(DLGPROC)ConfigProc,1);
	if(ret==-1)
	{
		MessageBoxEx(GetActiveWindow(),"Error Opening the config dialog.","OMG ERROR!",MB_OK,0);
		return;
	}
	ReadSettings();
}
