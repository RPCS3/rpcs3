/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2.1 of the the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#pragma once

#ifndef _DIALOGS_H_
#define _DIALOGS_H_

#include "../Spu2.h"

#include <commctrl.h>
#include <initguid.h>

extern HINSTANCE hInstance;

#define SET_CHECK(idc,value) SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,((value)==0)?BST_UNCHECKED:BST_CHECKED,0)
#define HANDLE_CHECK(idc,hvar)	case idc: (hvar) = !(hvar); SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar)?BST_CHECKED:BST_UNCHECKED,0); break
#define HANDLE_CHECKNB(idc,hvar)case idc: (hvar) = !(hvar); SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar)?BST_CHECKED:BST_UNCHECKED,0)
#define ENABLE_CONTROL(idc,value) EnableWindow(GetDlgItem(hWnd,idc),value)

#define INIT_SLIDER(idc,minrange,maxrange,tickfreq,pagesize,linesize) \
			SendMessage(GetDlgItem(hWnd,idc),TBM_SETRANGEMIN,FALSE,minrange); \
			SendMessage(GetDlgItem(hWnd,idc),TBM_SETRANGEMAX,FALSE,maxrange); \
			SendMessage(GetDlgItem(hWnd,idc),TBM_SETTICFREQ,tickfreq,0); \
			SendMessage(GetDlgItem(hWnd,idc),TBM_SETPAGESIZE,0,pagesize); \
			SendMessage(GetDlgItem(hWnd,idc),TBM_SETLINESIZE,0,linesize)

#define HANDLE_SCROLL_MESSAGE(idc,idcDisplay) \
	if((HWND)lParam == GetDlgItem(hWnd,idc)) return DoHandleScrollMessage( GetDlgItem(hWnd,idcDisplay), wParam, lParam )

namespace DebugConfig
{
	extern void ReadSettings();
	extern void WriteSettings();
	extern void OpenDialog();
	extern void EnableControls( HWND hWnd );
}

extern int SendDialogMsg( HWND hwnd, int dlgId, UINT code, WPARAM wParam, LPARAM lParam);
extern HRESULT GUIDFromString( const char *str, LPGUID guid );

extern void AssignSliderValue( HWND idcwnd, HWND hwndDisplay, int value );
extern void AssignSliderValue( HWND hWnd, int idc, int editbox, int value );
extern int GetSliderValue( HWND hWnd, int idc );
extern BOOL DoHandleScrollMessage( HWND hwndDisplay, WPARAM wParam, LPARAM lParam );

bool CfgFindName( const TCHAR *Section, const TCHAR* Name);

void CfgWriteBool(const TCHAR* Section, const TCHAR* Name, bool Value);
void CfgWriteInt(const TCHAR* Section, const TCHAR* Name, int Value);
void CfgWriteStr(const TCHAR* Section, const TCHAR* Name, const wstring& Data);

bool CfgReadBool(const TCHAR *Section,const TCHAR* Name, bool Default);
void CfgReadStr(const TCHAR* Section, const TCHAR* Name, wstring Data, int DataSize, const TCHAR* Default);
void CfgReadStr(const TCHAR* Section, const TCHAR* Name, TCHAR* Data, int DataSize, const TCHAR* Default);
int CfgReadInt(const TCHAR* Section, const TCHAR* Name,int Default);


// Items Specific to DirectSound
#define STRFY(x) #x
#define verifyc(x) Verifyc(x,STRFY(x))

extern void Verifyc(HRESULT hr, const char* fn);

struct ds_device_data
{
	std::wstring name;
	GUID guid;
	bool hasGuid;
};

#endif
