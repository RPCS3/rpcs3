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
#include "GSdx.h"
#include "GSDialog.h"
#include "GSVector.h"

GSDialog::GSDialog(UINT id)
	: m_id(id)
	, m_hWnd(NULL)
{
}

INT_PTR GSDialog::DoModal()
{
	return DialogBoxParam(theApp.GetModuleHandle(), MAKEINTRESOURCE(m_id), GetActiveWindow(), DialogProc, (LPARAM)this);
}

INT_PTR CALLBACK GSDialog::DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	GSDialog* dlg = NULL;

	if(message == WM_INITDIALOG)
	{
		dlg = (GSDialog*)lParam;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)dlg);
		dlg->m_hWnd = hWnd;

		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &mi);

		GSVector4i r;
		GetWindowRect(hWnd, r);

		int x = (mi.rcWork.left + mi.rcWork.right - r.width()) / 2;
		int y = (mi.rcWork.top + mi.rcWork.bottom - r.height()) / 2;

		SetWindowPos(hWnd, NULL, x, y, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

		dlg->OnInit();

		return true;
	}

	dlg = (GSDialog*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	return dlg != NULL ? dlg->OnMessage(message, wParam, lParam) : FALSE;
}

bool GSDialog::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	return message == WM_COMMAND ? OnCommand((HWND)lParam, LOWORD(wParam), HIWORD(wParam)) : false;
}

bool GSDialog::OnCommand(HWND hWnd, UINT id, UINT code)
{
	if(id == IDOK || id == IDCANCEL)
	{
		EndDialog(m_hWnd, id);

		return true;
	}

	return false;
}

string GSDialog::GetText(UINT id)
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

int GSDialog::GetTextAsInt(UINT id)
{
	return atoi(GetText(id).c_str());
}

void GSDialog::SetText(UINT id, const char* str)
{
	SetDlgItemText(m_hWnd, id, str);
}

void GSDialog::SetTextAsInt(UINT id, int i)
{
	char buff[32] = {0};
	itoa(i, buff, 10);
	SetText(id, buff);
}

void GSDialog::ComboBoxInit(UINT id, const vector<GSSetting>& settings, uint32 selid, uint32 maxid)
{
	HWND hWnd = GetDlgItem(m_hWnd, id);

	SendMessage(hWnd, CB_RESETCONTENT, 0, 0);

	for(size_t i = 0; i < settings.size(); i++)
	{
		const GSSetting& s = settings[i];

		if(s.id <= maxid)
		{
			string str(s.name);

			if(!s.note.empty())
			{
				str = str + " (" + s.note + ")";
			}

			ComboBoxAppend(id, str.c_str(), (LPARAM)s.id, s.id == selid);
		}
	}
}

int GSDialog::ComboBoxAppend(UINT id, const char* str, LPARAM data, bool select)
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

bool GSDialog::ComboBoxGetSelData(UINT id, INT_PTR& data)
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
