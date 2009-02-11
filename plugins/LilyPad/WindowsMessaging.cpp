#include "global.h"
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

	int Activate(void *d) {
		InitInfo *info = (InitInfo*)d;
		// Redundant.  Should match the next line.
		// Deactivate();
		if (wmk) wmk->Deactivate();
		HWND hWnd = info->hWnd;
		if (info->hWndButton) {
			hWnd = info->hWndButton;
			// hWndDlg = info->hWnd;
		}
		if (!wmm && !EatWndProc(hWnd, WindowsMessagingWndProc)) {
			Deactivate();
			return 0;
		}

		wmk = this;
		InitState();

		active = 1;
		return 1;
	}

	void Deactivate() {
		FreeState();
		if (active) {
			if (!wmm)
				ReleaseExtraProc(WindowsMessagingWndProc);
			active = 0;
			wmk = 0;
		}
		// hWndDlg = 0;
	}


	void CheckKey(int vkey) {
		UpdateKey(vkey, 1&(((unsigned short)GetAsyncKeyState(vkey))>>15));
	}
};

class WindowsMessagingMouse : public WindowsMouse {
public:

	WindowsMessagingMouse() : WindowsMouse(WM, 1, L"WM Mouse") {
	}

	int Activate(void *d) {
		InitInfo *info = (InitInfo*)d;
		// Redundant.  Should match the next line.
		// Deactivate();
		if (wmm) wmm->Deactivate();
		HWND hWnd = info->hWnd;
		if (info->hWndButton) {
			hWnd = info->hWndButton;
		}

		if (!wmk && !EatWndProc(hWnd, WindowsMessagingWndProc)) {
			Deactivate();
			return 0;
		}

		SetCapture(hWnd);
		ShowCursor(0);

		GetCursorPos(&origCursorPos);
		active = 1;
		RECT r;
		GetWindowRect(hWnd, &r);
		ClipCursor(&r);
		center.x = (r.left + r.right)/2;
		center.y = (r.top + r.bottom)/2;
		SetCursorPos(center.x, center.y);

		wmm = this;
		AllocState();

		return 1;
	}

	void Deactivate() {
		FreeState();
		if (active) {
			ClipCursor(0);
			ReleaseCapture();
			ShowCursor(1);
			SetCursorPos(origCursorPos.x, origCursorPos.y);
			if (!wmk)
				ReleaseExtraProc(WindowsMessagingWndProc);
			active = 0;
			wmm = 0;
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
		/*
		else if (uMsg == WM_KILLFOCUS) {
			wmm->Deactivate();
		}//*/
	}
	return CONTINUE_BLISSFULLY;
}

void EnumWindowsMessagingDevices() {
	dm->AddDevice(new WindowsMessagingKeyboard());
	dm->AddDevice(new WindowsMessagingMouse());
}
