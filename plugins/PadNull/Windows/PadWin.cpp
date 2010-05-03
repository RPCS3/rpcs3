/*  PadNull
 *  Copyright (C) 2004-2010  PCSX2 Dev Team
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

#include "PadWin.h"

LRESULT WINAPI PADwndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC GSwndProc = NULL;
HWND GShwnd = NULL;

LRESULT WINAPI PADwndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_KEYDOWN:
			if (lParam & 0x40000000)return TRUE;

			event.evt = KEYPRESS;
			event.key = wParam;
			break;

		case WM_KEYUP:
			event.evt = KEYRELEASE;
			event.key = wParam;
			break;

		case WM_DESTROY:
		case WM_QUIT:
			event.evt = KEYPRESS;
			event.key = VK_ESCAPE;
			return GSwndProc(hWnd, msg, wParam, lParam);

		default:
			return GSwndProc(hWnd, msg, wParam, lParam);
	};

	return TRUE;
}

void _PadUpdate(int pad)
{
}

s32 _PADOpen(void *pDsp)
{
    GShwnd = (HWND)*(long*)pDsp;

	if (GShwnd != NULL && GSwndProc != NULL)
	{
		// revert
		SetWindowLongPtr(GShwnd, GWLP_WNDPROC, (LPARAM)(WNDPROC)(GSwndProc));
	}

    GSwndProc = (WNDPROC)GetWindowLongPtr(GShwnd, GWLP_WNDPROC);
    GSwndProc = ((WNDPROC)SetWindowLongPtr(GShwnd, GWLP_WNDPROC, (LPARAM)(WNDPROC)(PADwndProc)));
	return 0;
}

void  _PADClose()
{
    if (GShwnd != NULL && GSwndProc != NULL)
	{
        SetWindowLongPtr(GShwnd, GWLP_WNDPROC, (LPARAM)(WNDPROC)(GSwndProc));
        GSwndProc = NULL;
        GShwnd = NULL;
    }
}
