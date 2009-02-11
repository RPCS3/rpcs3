#include "Global.h"

#include "KeyboardHook.h"
#include "InputManager.h"
#include "WindowsKeyboard.h"
#include "Config.h"
#include "VKey.h"
#include "WndProcEater.h"
#include "DeviceEnumerator.h"

extern HINSTANCE hInst;
LRESULT CALLBACK IgnoreKeyboardHook(int code, WPARAM wParam, LPARAM lParam);

ExtraWndProcResult StartHooksWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output);

class IgnoreKeyboardHookDevice;
static IgnoreKeyboardHookDevice *ikhd = 0;

class IgnoreKeyboardHookDevice : public WindowsKeyboard {
public:
	HHOOK hHook;
	// When binding, eat all keys
	int binding;

	IgnoreKeyboardHookDevice() : WindowsKeyboard(IGNORE_KEYBOARD, L"Ignore Keyboard") {
		// Don't want to send keypress/keyrelease messages.
		for (int i=0; i<numPhysicalControls; i++) {
			physicalControls[i].vkey = 0;
		}
		hHook = 0;
	}

	int Update() {
		//if (!GetKeyboardState(buttonState)) return 0;
		for (int i=0; i<=4; i++) virtualControlState[i] = 0;
		// So I'll bind to left/right control/shift instead of generic ones.
		virtualControlState[VK_SHIFT] = 0;
		virtualControlState[VK_CONTROL] = 0;
		virtualControlState[VK_MENU] = 0;
		return 1;
	}

	int Activate(void *d) {
		if (ikhd) ikhd->Deactivate();
		InitInfo *info = (InitInfo*) d;
		binding = info->bindingIgnore;
		if (info->hWndButton)
			EatWndProc(info->hWndButton, StartHooksWndProc);
		else
			EatWndProc(info->hWnd, StartHooksWndProc);
		InitState();
		ikhd = this;
		active = 1;
		return 1;
	}

	void Deactivate() {
		FreeState();
		if (active) {
			if (hHook) {
				UnhookWindowsHookEx(hHook);
				hHook = 0;
			}
			if (ikhd == this) ikhd = 0;
			active = 0;
		}
	}
};

void EnumHookDevices() {
	dm->AddDevice(new IgnoreKeyboardHookDevice());
}

// Makes sure hooks are started in correct thread.
ExtraWndProcResult StartHooksWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output) {
	if (ikhd && !ikhd->hHook && ikhd->active) {
		ikhd->hHook = SetWindowsHookEx(WH_KEYBOARD_LL, IgnoreKeyboardHook, hInst, 0);
		if (ikhd->hHook == 0) ikhd->Deactivate();
	}
	return CONTINUE_BLISSFULLY_AND_RELEASE_PROC;
}

LRESULT CALLBACK IgnoreKeyboardHook(int code, WPARAM wParam, LPARAM lParam) {
	HHOOK hHook = 0;
	if (ikhd) {
		hHook = ikhd->hHook;
		if (hHook) {
			if (code == HC_ACTION) {
				if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
					KBDLLHOOKSTRUCT* key = (KBDLLHOOKSTRUCT*) lParam;
					if (key->vkCode < 256) {
						for (int i=0; i<ikhd->pads[0].numBindings; i++) {
							if (ikhd->pads[0].bindings[i].controlIndex == key->vkCode) {
								return 1;
							}
						}
						if (ikhd->binding){
							ikhd->UpdateKey(key->vkCode, 1);
							return 1;
						}
					}
				}
				else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
					KBDLLHOOKSTRUCT* key = (KBDLLHOOKSTRUCT*) lParam;
					if (ikhd->binding && key->vkCode < 256)
						ikhd->UpdateKey(key->vkCode, 0);
				}
			}
			return CallNextHookEx(hHook, code, wParam, lParam);
		}
	}
	return CallNextHookEx(hHook, code, wParam, lParam);
}

