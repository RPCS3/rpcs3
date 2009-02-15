/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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

#include "PrecompiledHeader.h"

/**************************
*
* patchbrowser.c contains all the src of patchbrowser window
* no interaction with emulation code
***************************/

#include "Win32.h"
#include "Common.h"
#include "resource.h"

/*
 * TODO:
 * - not topmost
 * - resize stuff
 * - ask to save in exit (check if changed)
 */
BOOL CALLBACK PatchBDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	int tmpi,i;
	char fileName[MAX_PATH], *tmpStr;
	FILE *fp;

	switch(uMsg) {

		case WM_INITDIALOG:
            SetWindowText(hW, _("Patches Browser"));
            Button_SetText(GetDlgItem(hW,IDC_REFRESHPATCHLIST), _("Refresh List"));
            Button_SetText(GetDlgItem(hW,IDC_NEWPATCH), _("New Patch"));
			Button_SetText(GetDlgItem(hW,IDC_SAVEPATCH), _("Save Patch"));
			Button_SetText(GetDlgItem(hW,IDC_EXITPB), _("Exit"));
            Static_SetText(GetDlgItem(hW,IDC_GAMENAMESEARCH), _("Search game name patch:")); 
			//List Patches
			ListPatches ((HWND) hW);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {

				case IDC_NEWPATCH:

					i = Save_Patch_Proc(fileName);
					if ( i == FALSE || (fp = fopen(fileName,"a")) == NULL ) { 
						MessageBox(hW,(LPCTSTR)"Couldn't create the file.",NULL,(UINT)MB_ICONERROR);
						return FALSE;
					}

					fclose(fp);
					i = MessageBox(hW,(LPCTSTR)"File created sucessfully.\nClear textbox?",NULL,(UINT)(MB_YESNO|MB_ICONQUESTION));
					if (i==IDYES) SetDlgItemText(hW, IDC_PATCHTEXT, (LPCTSTR)""); 

					return TRUE;

				case IDC_SAVEPATCH:

					i = Save_Patch_Proc(fileName);
					if ( i == FALSE || (fp = fopen(fileName,"w")) == NULL ) { 
						MessageBox(hW,(LPCTSTR)"Couldn't save the file.",NULL,(UINT)MB_ICONERROR);
						return FALSE;
					}

					tmpi = SendDlgItemMessage(hW, IDC_PATCHTEXT, EM_GETLINECOUNT, (WPARAM)NULL, (LPARAM)NULL);
					i=0;
					for (;tmpi>=0;tmpi--)
						i += SendDlgItemMessage(hW, IDC_PATCHTEXT, EM_LINELENGTH, (WPARAM)tmpi, (LPARAM)NULL);

					tmpStr = (char *) malloc(i);
					sprintf(tmpStr,"");
					SendDlgItemMessage(hW, IDC_PATCHTEXT, WM_GETTEXT, (WPARAM)i, (LPARAM)tmpStr);

					//remove \r
					for (i=0,tmpi=0; tmpStr[i]!='\0'; i++)
						if (tmpStr[i] != '\r')
							tmpStr[tmpi++] = tmpStr[i];
					tmpStr[tmpi] = '\0';

					fputs(tmpStr,fp);

					fclose(fp);
					free(tmpStr);

					MessageBox(hW,(LPCTSTR)"File saved sucessfully.",NULL,(UINT)MB_ICONINFORMATION);

					return TRUE;

				case IDC_REFRESHPATCHLIST:

					//List Patches
					ListPatches ((HWND) hW);

					return TRUE;

				case IDC_EXITPB:

					//Close Dialog
					EndDialog(hW, FALSE);

					return TRUE;

				case IDC_PATCHCRCLIST:

					//Get selected item
					tmpi = SendDlgItemMessage(hW, IDC_PATCHCRCLIST, LB_GETCURSEL, 0, 0);
					SendDlgItemMessage(hW, IDC_PATCHCRCLIST, LB_GETTEXT, (WPARAM)tmpi, (LPARAM)fileName);

					return ReadPatch ((HWND) hW, fileName);

				case IDC_PATCHNAMELIST:

					//Get selected item
					tmpi = SendDlgItemMessage(hW, IDC_PATCHNAMELIST, LB_GETCURSEL, 0, 0);
					SendDlgItemMessage(hW, IDC_PATCHNAMELIST, LB_GETTEXT, (WPARAM)tmpi, (LPARAM)fileName);

					//another small hack :p
					//eg. SOCOM Demo PAL (7dd01dd9.pnach)
					for (i=0;i<(int)strlen(fileName);i++)
						if (fileName[i] == '(') tmpi = i;

					sprintf(fileName,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
						fileName[tmpi+1],fileName[tmpi+2],fileName[tmpi+3],
						fileName[tmpi+4],fileName[tmpi+5],fileName[tmpi+6],
						fileName[tmpi+7],fileName[tmpi+8],fileName[tmpi+9],
						fileName[tmpi+10],fileName[tmpi+11],fileName[tmpi+12],
						fileName[tmpi+13],fileName[tmpi+14]);

					//sanity check
					if (fileName[tmpi+15] != ')') return FALSE;

					return ReadPatch ((HWND) hW, fileName);

				case IDC_SEARCHPATCHTEXT:

					//get text
					SendDlgItemMessage(hW, IDC_SEARCHPATCHTEXT, EM_GETLINE, 0, (LPARAM)fileName);
					//search
					tmpi = SendDlgItemMessage(hW, IDC_PATCHNAMELIST, LB_FINDSTRING, (WPARAM)-1, (LPARAM)fileName);
					//select match item 
					SendDlgItemMessage(hW, IDC_PATCHNAMELIST, LB_SETCURSEL, (WPARAM)tmpi, (LPARAM)NULL);

					return TRUE;
			}
			return TRUE;

		case WM_CLOSE:
			EndDialog(hW, FALSE);
			break;

	}
	return FALSE;
}
void ListPatches (HWND hW) {

	int i, tmpi, filesize, totalPatch=0, totalSize=0;
	char tmpStr[MAX_PATH], *fileData;
	WIN32_FIND_DATA FindData;
	HANDLE Find;
	FILE *fp;

	//clear listbox's
	SendDlgItemMessage(hW, IDC_PATCHCRCLIST, (UINT)LB_RESETCONTENT, (WPARAM)NULL, (LPARAM)NULL);
	SendDlgItemMessage(hW, IDC_PATCHNAMELIST, (UINT)LB_RESETCONTENT, (WPARAM)NULL, (LPARAM)NULL);

	//sprintf(tmpStr,"%s*.pnach", Config.PatchDir)
	sprintf(tmpStr, "patches\\*.pnach");

	Find = FindFirstFile(tmpStr, &FindData);

	do {
		if (Find==INVALID_HANDLE_VALUE) break;

		sprintf(tmpStr,"%s", FindData.cFileName);

		//add file name to crc list
		SendDlgItemMessage(hW, IDC_PATCHCRCLIST, (UINT) LB_ADDSTRING, (WPARAM)NULL, (LPARAM)tmpStr);

		//sprintf(tmpStr,"%s%s", Config.PatchDir, FindData.cFileName)
		sprintf(tmpStr,"patches\\%s", FindData.cFileName);

		fp = fopen(tmpStr, "r");
		if (fp == NULL) break;

		fseek(fp, 0, SEEK_END);
		filesize = ftell(fp);
		totalSize += filesize;
		fseek(fp, 0, SEEK_SET);

		fileData = (char *) malloc(filesize+1024);
		sprintf(fileData,"");

		//read file
		while((tmpi=fgetc(fp)) != EOF) 
			sprintf(fileData,"%s%c",fileData,tmpi);

		//small hack :p
		for(i=0;i<filesize;i++) {
			if (fileData[i] == 'g' && fileData[i+1] == 'a' && 
				fileData[i+2] == 'm' && fileData[i+3] == 'e' && 
				fileData[i+4] == 't' && fileData[i+5] == 'i' &&
				fileData[i+6] == 't' && fileData[i+7] == 'l' && 
				fileData[i+8] == 'e' && fileData[i+9] == '=')  {

					for(i=i+10,tmpi=0;i<filesize;i++,tmpi++) {
						if (fileData[i] == '\n') break;
						tmpStr[tmpi] = fileData[i];
					}
					tmpStr[tmpi] = '\0';
					break;
				}
		}

		//remove " in the string
		for (i=0,tmpi=0; tmpStr[i]!='\0'; i++)
			if (tmpStr[i] != '"')
				tmpStr[tmpi++] = tmpStr[i];
		tmpStr[tmpi] = '\0';

		//remove spaces at the left of the string
		sprintf(tmpStr,lTrim(tmpStr));

		sprintf(tmpStr,"%s (%s)",tmpStr,FindData.cFileName);

		//add game name to patch name list
		SendDlgItemMessage(hW, IDC_PATCHNAMELIST, (UINT) LB_ADDSTRING, (WPARAM)NULL, (LPARAM)tmpStr);
		
		totalPatch++;
		sprintf(fileData,"");
		fclose(fp);

	} while (FindNextFile(Find,&FindData));

	if (Find!=INVALID_HANDLE_VALUE) FindClose(Find);

	sprintf(tmpStr,"Patches Browser | Patches Found: %d | Total Filesize: %.2f KB",
		totalPatch,(float)totalSize/1024);
	SetWindowText(hW, tmpStr);
}

int ReadPatch (HWND hW, char fileName[1024]) {

	FILE * fp;
	int tmpi, filesize, i;
	char filePath[MAX_PATH],*fileData;

	sprintf(filePath,"patches\\%s", fileName);

	fp = fopen(filePath, "r");
	if (fp == NULL) return FALSE;

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	//filesize+1024 cause bellow i'm gonna add stuff to the string
	//TODO: change the 1024 to a more apropriated value
	fileData = (char *) malloc(filesize+1024);
	sprintf(fileData,"");

	while((tmpi=fgetc(fp)) != EOF) 
		sprintf(fileData,"%s%c",fileData,tmpi);

	//for some reason windows editbox only show the newline
	//when it have \r\n don't ask me why :p
	//so what i did was to every \n put \r\n
	for (i=0;i<filesize;i++) {
		if (fileData[i] == '\n' ) {
			for (tmpi=filesize;tmpi>i;tmpi--)
				fileData[tmpi] = fileData[tmpi-1];
			fileData[i] = '\r';
			fileData[i+1] = '\n';
			i++;
			}
	}

	SetDlgItemText(hW, IDC_PATCHTEXT, (LPCTSTR)fileData);

	sprintf(fileData,"");
	fclose(fp);

	return TRUE;
}


//Left Trim (remove the spaces at the left of a string)
char * lTrim (char *s) {
	int count=0,i,tmpi;

	for (i=0;i<(int)strlen(s); i++) {
		if (s[i] == ' ') count++;
		else {
			for (tmpi=0;tmpi<(int)strlen(s);tmpi++)
				s[tmpi] = s[tmpi+count];
			break;
		}
	}
	return s;
}


BOOL Save_Patch_Proc( char * filename )  {

	OPENFILENAME ofn;
	char szFileName[ 256 ];
	char szFileTitle[ 256 ];
	char * filter = "Patch Files (*.pnach)\0*.pnach\0ALL Files (*.*)\0*.*";

	memset( &szFileName, 0, sizeof( szFileName ) );
	memset( &szFileTitle, 0, sizeof( szFileTitle ) );

	ofn.lStructSize			= sizeof( OPENFILENAME );
	ofn.hwndOwner			= gApp.hWnd;
	ofn.lpstrFilter			= filter;
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 1;
	ofn.lpstrFile			= szFileName;
	ofn.nMaxFile			= 256;
	ofn.lpstrInitialDir		= NULL;
	ofn.lpstrFileTitle		= szFileTitle;
	ofn.nMaxFileTitle		= 256;
	ofn.lpstrTitle			= NULL;
	ofn.lpstrDefExt			= "TXT";
	ofn.Flags				= OFN_EXPLORER | OFN_LONGNAMES | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

	if ( GetSaveFileName( &ofn ) ) {

		strcpy( filename, szFileName );

		return TRUE;             
	}
	else {
		return FALSE;
	}
}
