/*  GSnull
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

#include "../GS.h"

HINSTANCE HInst;
HWND GShwnd = NULL;

LRESULT CALLBACK MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int GSOpenWindow(void *pDsp, char *Title)
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, 
					GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
					"PS2EMU_GSNULL", NULL };
	RegisterClassEx( &wc );

	GShwnd = CreateWindowEx( WS_EX_CLIENTEDGE, "PS2EMU_GSNULL", Title,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 240, 120, NULL, NULL, wc.hInstance, NULL);

	if(GShwnd == NULL) 
	{
		GS_LOG("Failed to create window. Exiting...");
		return -1;
	}

	if( pDsp != NULL ) *(uptr*)pDsp = (uptr)GShwnd;

	return 0;
}

void GSCloseWindow()
{
	DestroyWindow( GShwnd );
}

void GSProcessMessages()
{
}

// GSkeyEvent gets called when there is a keyEvent from the PAD plugin
void HandleKeyEvent(keyEvent *ev)
{
}

EXPORT_C_(s32) GSsetWindowInfo(winInfo *info)
{
	return 0;
}