/* 
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdAfx.h"
#include "CDVD.h"
#include "CDVDDialog.h"

CDVDDialog::CDVDDialog(UINT id)
	: m_id(id)
	, m_hWnd(NULL)
{
}

INT_PTR CDVDDialog::DoModal()
{
	return DialogBoxParam(theApp.GetModuleHandle(), MAKEINTRESOURCE(m_id), NULL, DialogProc, (LPARAM)this);
}

INT_PTR CALLBACK CDVDDialog::DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CDVDDialog* dlg = NULL;

	if(message == WM_INITDIALOG)
	{
		dlg = (CDVDDialog*)lParam;
		SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG_PTR)dlg);
		dlg->m_hWnd = hWnd;

		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &mi);

		RECT r;
		GetWindowRect(hWnd, &r);
 		
		int x = (mi.rcWork.left + mi.rcWork.right - (r.right - r.left)) / 2;
		int y = (mi.rcWork.top + mi.rcWork.bottom - (r.bottom - r.top)) / 2;

		SetWindowPos(hWnd, NULL, x, y, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

		dlg->OnInit();
		
		return true;
	}

	dlg = (CDVDDialog*)GetWindowLongPtr(hWnd, GWL_USERDATA);
	
	return dlg != NULL ? dlg->OnMessage(message, wParam, lParam) : FALSE;
}

bool CDVDDialog::OnMessage(UINT message, WPARAM wParam, LPARAM lParam) 
{
	return message == WM_COMMAND ? OnCommand((HWND)lParam, LOWORD(wParam), HIWORD(wParam)) : false;
}

bool CDVDDialog::OnCommand(HWND hWnd, UINT id, UINT code)
{
	if(id == IDOK || id == IDCANCEL)
	{
		EndDialog(m_hWnd, id);

		return true;
	}

	return false;
}

string CDVDDialog::GetText(UINT id)
{
	string s;

	char* buff = NULL;

	for(int size = 256, limit = 65536; size < limit; size <<= 1)
	{
		buff = new char[size];

		if(GetDlgItemText(m_hWnd, id, buff, size))
		{
			s = buff;
			size = limit;
		}

		delete [] buff;
	}

	return s;
}

int CDVDDialog::GetTextAsInt(UINT id)
{
	return atoi(GetText(id).c_str());
}

void CDVDDialog::SetText(UINT id, const char* str)
{
	SetDlgItemText(m_hWnd, id, str);
}

void CDVDDialog::SetTextAsInt(UINT id, int i)
{
	char buff[32] = {0};
	itoa(i, buff, 10);
	SetText(id, buff);
}

void CDVDDialog::ComboBoxInit(UINT id, const CDVDSetting* settings, int count, uint32 selid, uint32 maxid)
{
	HWND hWnd = GetDlgItem(m_hWnd, id);

	SendMessage(hWnd, CB_RESETCONTENT, 0, 0);

	for(int i = 0; i < count; i++)
	{
		if(settings[i].id <= maxid)
		{
			string str = settings[i].name;
			
			if(!settings[i].note.empty()) 
			{
				str = str + " (" + settings[i].note + ")";
			}

			ComboBoxAppend(id, str.c_str(), (LPARAM)settings[i].id, settings[i].id == selid);
		}
	}
}

int CDVDDialog::ComboBoxAppend(UINT id, const char* str, LPARAM data, bool select)
{
	HWND hWnd = GetDlgItem(m_hWnd, id);

	int item = (int)SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)str);

	SendMessage(hWnd, CB_SETITEMDATA, item, (LPARAM)data);

	if(select)
	{
		SendMessage(hWnd, CB_SETCURSEL, item, 0);
	}

	return item;
}

bool CDVDDialog::ComboBoxGetSelData(UINT id, INT_PTR& data)
{
	HWND hWnd = GetDlgItem(m_hWnd, id);

	int item = SendMessage(hWnd, CB_GETCURSEL, 0, 0);

	if(item >= 0)
	{
		data = SendMessage(hWnd, CB_GETITEMDATA, item, 0);

		return true;
	}

	return false;
}
