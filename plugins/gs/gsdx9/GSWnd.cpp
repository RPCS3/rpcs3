/* 
 *	Copyright (C) 2003-2005 Gabest
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

#include "stdafx.h"
#include "GSWnd.h"

IMPLEMENT_DYNCREATE(CGSWnd, CWnd)

CGSWnd::CGSWnd()
{
}

BOOL CGSWnd::Create(LPCSTR pTitle)
{
	CRect r;
	GetDesktopWindow()->GetWindowRect(r);
	// r.DeflateRect(r.Width()*3/8, r.Height()*3/8);
	r.DeflateRect(r.Width()/3, r.Height()/3);
	LPCTSTR wndclass = AfxRegisterWndClass(CS_VREDRAW|CS_HREDRAW|CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_ARROW), 0/*(HBRUSH)(COLOR_BTNFACE + 1)*/, 0);
	return CreateEx(0, wndclass, pTitle, WS_OVERLAPPEDWINDOW, r, NULL, 0);
}

void CGSWnd::Show(bool bShow)
{
	if(bShow)
	{
		SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
		SetForegroundWindow();
		ShowWindow(SW_SHOWNORMAL);
	}
	else
	{
		ShowWindow(SW_HIDE);
	}
}

BEGIN_MESSAGE_MAP(CGSWnd, CWnd)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

void CGSWnd::OnClose()
{
	PostMessage(WM_QUIT);
}
