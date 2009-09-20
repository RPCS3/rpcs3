#include "Global.h"
#include "WindowsMessaging.h"
#include "VKey.h"
#include "DeviceEnumerator.h"
#include "WndProcEater.h"
#include "WindowsKeyboard.h"
#include "WindowsMouse.h"

class WindowsMessagingKeyboard : public WindowsKeyboard {
public:
	int binding;

	WindowsMessagingKeyboard() : WindowsKeyboard(WM, L"WM Keyboard") {
	}

	int Activate(InitInfo *initInfo) {
		binding = initInfo->binding;

		EatWndProc(initInfo->hWndButton, WMKeyboardBindingWndProc, EATPROC_NO_UPDATE_WHILE_UPDATING_DEVICES);

		InitState();

		active = 1;
		return 1;
	}

	void Deactivate() {
		ReleaseExtraProc(WMKeyboardBindingWndProc);
		active = 0;
		FreeState();
	}

	int Update() {
		for (int i=10; i<256; i++) {
			CheckKey(i);
		}
		physicalControlState[VK_MENU] = 0;
		physicalControlState[VK_SHIFT] = 0;
		physicalControlState[VK_CONTROL] = 0;
		return 1;
	}

	void CheckKey(int vkey) {
		UpdateKey(vkey, 1&(((unsigned short)GetAsyncKeyState(vkey))>>15));
	}

	static ExtraWndProcResult WMKeyboardBindingWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output) {
		if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN || uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP || uMsg == WM_CHAR || uMsg == WM_UNICHAR) {
			return NO_WND_PROC;
		}
		return CONTINUE_BLISSFULLY;
	}
};

ExtraWndProcResult WMMouseWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output);

class WindowsMessagingMouse;

static WindowsMessagingMouse *wmm = 0;

class WindowsMessagingMouse : public WindowsMouse {
public:
	WindowsMessagingMouse() : WindowsMouse(WM, 1, L"WM Mouse") {
	}

	int Activate(InitInfo *initInfo) {
		// Redundant.  Should match the next line.
		if (wmm) wmm->Deactivate();

		HWND hWnd = initInfo->hWnd;
		if (initInfo->hWndButton) {
			hWnd = initInfo->hWndButton;
		}

		if (!EatWndProc(hWnd, WMMouseWndProc, EATPROC_NO_UPDATE_WHILE_UPDATING_DEVICES)) {
			Deactivate();
			return 0;
		}
		GetMouseCapture(hWnd);

		active = 1;

		wmm = this;
		AllocState();

		return 1;
	}

	void Deactivate() {
		if (active) {
			ReleaseExtraProc(WMMouseWndProc);
			ReleaseMouseCapture();
			wmm = 0;
			active = 0;
			FreeState();
		}
	}

	static ExtraWndProcResult WMMouseWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output) {
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
};


void EnumWindowsMessagingDevices() {
	dm->AddDevice(new WindowsMessagingKeyboard());
	dm->AddDevice(new WindowsMessagingMouse());
}
