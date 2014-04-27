/*  LilyPad - Pad plugin for PS2 Emulator
 *  Copyright (C) 2002-2014  PCSX2 Dev Team/ChickenLiver
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU Lesser General Public License as published by the Free
 *  Software Found- ation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with PCSX2.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Global.h"
#include "InputManager.h"
#include "WindowsMessaging.h"
#include "VKey.h"
#include "DeviceEnumerator.h"
#include "WndProcEater.h"
#include "WindowsKeyboard.h"
#include "WindowsMouse.h"

ExtraWndProcResult WindowsMessagingWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output);

class WindowsMessagingKeyboard;
class WindowsMessagingMouse;

static WindowsMessagingKeyboard *wmk = 0;
static WindowsMessagingMouse *wmm = 0;

class WindowsMessagingKeyboard : public WindowsKeyboard {
public:

	WindowsMessagingKeyboard() : WindowsKeyboard(WM, L"WM Keyboard") {
	}

	int Activate(InitInfo *initInfo) {
		// Redundant.  Should match the next line.
		// Deactivate();
		if (wmk) wmk->Deactivate();

		hWndProc = initInfo->hWndProc;

		if (!wmm)
			hWndProc->Eat(WindowsMessagingWndProc, EATPROC_NO_UPDATE_WHILE_UPDATING_DEVICES);

		wmk = this;
		InitState();

		active = 1;
		return 1;
	}

	void Deactivate() {
		if (active) {
			if (!wmm)
				hWndProc->ReleaseExtraProc(WindowsMessagingWndProc);
			wmk = 0;
			active = 0;
			FreeState();
		}
	}


	void CheckKey(int vkey) {
		UpdateKey(vkey, 1&(((unsigned short)GetAsyncKeyState(vkey))>>15));
	}
};

class WindowsMessagingMouse : public WindowsMouse {
public:

	WindowsMessagingMouse() : WindowsMouse(WM, 1, L"WM Mouse") {
	}

	int Activate(InitInfo *initInfo) {
		// Redundant.  Should match the next line.
		// Deactivate();
		if (wmm) wmm->Deactivate();
		hWndProc = initInfo->hWndProc;

		if (!wmk)
			hWndProc->Eat(WindowsMessagingWndProc, EATPROC_NO_UPDATE_WHILE_UPDATING_DEVICES);

		GetMouseCapture(hWndProc->hWndEaten);

		active = 1;

		wmm = this;
		AllocState();

		return 1;
	}

	void Deactivate() {
		if (active) {
			if (!wmk)
				hWndProc->ReleaseExtraProc(WindowsMessagingWndProc);
			ReleaseMouseCapture();
			wmm = 0;
			active = 0;
			FreeState();
		}
	}
};

ExtraWndProcResult WindowsMessagingWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output) {
	if (wmk) {
		if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN || uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) {
			if (wParam == VK_SHIFT) {
				wmk->CheckKey(VK_RSHIFT);
				wmk->CheckKey(VK_LSHIFT);
			}
			else if (wParam == VK_CONTROL) {
				wmk->CheckKey(VK_RCONTROL);
				wmk->CheckKey(VK_LCONTROL);
			}
			else if (wParam == VK_MENU) {
				wmk->CheckKey(VK_RMENU);
				wmk->CheckKey(VK_LMENU);
			}
			else
				wmk->UpdateKey(wParam, (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN));
			return NO_WND_PROC;
		}
		// Needed to prevent default handling of keys in some situations.
		else if (uMsg == WM_CHAR || uMsg == WM_UNICHAR) {
			return NO_WND_PROC;
		}
		else if (uMsg == WM_ACTIVATE) {
			// Not really needed, but doesn't hurt.
			memset(wmk->physicalControlState, 0, sizeof(int) * wmk->numPhysicalControls);
		}
	}
	if (wmm) {
		if (uMsg == WM_MOUSEMOVE) {
			POINT p;
			GetCursorPos(&p);
			// Need check to prevent cursor movement cascade.
			if (p.x != wmm->center.x || p.y != wmm->center.y) {
				wmm->UpdateAxis(0, p.x - wmm->center.x);
				wmm->UpdateAxis(1, p.y - wmm->center.y);

				SetCursorPos(wmm->center.x, wmm->center.y);
			}
			return NO_WND_PROC;
		}
		else if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP) {
			wmm->UpdateButton(0, uMsg == WM_LBUTTONDOWN);
			return NO_WND_PROC;
		}
		else if (uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP) {
			wmm->UpdateButton(1, uMsg == WM_RBUTTONDOWN);
			return NO_WND_PROC;
		}
		else if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP) {
			wmm->UpdateButton(2, uMsg == WM_MBUTTONDOWN);
			return NO_WND_PROC;
		}
		else if (uMsg == WM_XBUTTONDOWN || uMsg == WM_XBUTTONUP) {
			wmm->UpdateButton(3+((wParam>>16) == XBUTTON2), uMsg == WM_XBUTTONDOWN);
			return NO_WND_PROC;
		}
		else if (uMsg == WM_MOUSEWHEEL) {
			wmm->UpdateAxis(2, ((int)wParam>>16)/WHEEL_DELTA);
			return NO_WND_PROC;
		}
		else if (uMsg == WM_MOUSEHWHEEL) {
			wmm->UpdateAxis(3, ((int)wParam>>16)/WHEEL_DELTA);
			return NO_WND_PROC;
		}
		else if (uMsg == WM_SIZE && wmm->active) {
			WindowsMouse::WindowResized(hWnd);
		}
		// Taken care of elsewhere.  When binding, killing focus means stop reading input.
		// When running PCSX2, I release all mouse and keyboard input elsewhere.
		/*else if (uMsg == WM_KILLFOCUS) {
			wmm->Deactivate();
		}//*/
	}
	return CONTINUE_BLISSFULLY;
}

void EnumWindowsMessagingDevices() {
	dm->AddDevice(new WindowsMessagingKeyboard());
	dm->AddDevice(new WindowsMessagingMouse());
}
