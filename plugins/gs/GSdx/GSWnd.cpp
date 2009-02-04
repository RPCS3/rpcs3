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
#include "GSWnd.h"

BEGIN_MESSAGE_MAP(GSWnd, CWnd)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

GSWnd::GSWnd()
{
}

GSWnd::~GSWnd()
{
	DestroyWindow();
}

bool GSWnd::Create(LPCTSTR title)
{
	CRect r;

	GetDesktopWindow()->GetWindowRect(r);

	CSize s(r.Width() / 3, r.Width() / 4);

	if(!GetSystemMetrics(SM_REMOTESESSION))
	{
		s.cx *= 2;
		s.cy *= 2;
	}

	r = CRect(r.CenterPoint() - CSize(s.cx / 2, s.cy / 2), s);

	LPCTSTR wc = AfxRegisterWndClass(CS_VREDRAW|CS_HREDRAW|CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_ARROW), 0, 0);

	return !!CreateEx(0, wc, title, WS_OVERLAPPEDWINDOW, r, NULL, 0);
}

void GSWnd::Show()
{
	SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	SetForegroundWindow();
	ShowWindow(SW_SHOWNORMAL);
}

void GSWnd::Hide()
{
	ShowWindow(SW_HIDE);
}

void GSWnd::OnClose()
{
	Hide();

	PostMessage(WM_QUIT);
}