#include "Global.h"
#include "InputManager.h"
#include "Config.h"

#include "KeyboardHook.h"
#include "WindowsKeyboard.h"
#include "VKey.h"
#include "WndProcEater.h"

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
		// So I'll bind to left/right control/shift instead of generic ones.
		virtualControlState[VK_SHIFT] = 0;
		virtualControlState[VK_CONTROL] = 0;
		virtualControlState[VK_MENU] = 0;
		return 1;
	}

	int Activate(InitInfo *initInfo) {
		if (ikhd) ikhd->Deactivate();
		binding = initInfo->bindingIgnore;
		hWndProc = initInfo->hWndProc;

		return 0;
		printf( "(Lilypad) StartHook\n" );
		hWndProc->Eat(StartHooksWndProc, EATPROC_NO_UPDATE_WHILE_UPDATING_DEVICES);

		InitState();
		ikhd = this;
		active = 1;
		return 1;
	}

	void Deactivate() {
		printf( "(Lilypad) KillHook\n" );

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
	// Don't remove too quickly - could be other things happening in other threads.
	// Attempted fix for keyboard input occasionally not working.
	static int counter = 0;
	if (ikhd && !ikhd->hHook && ikhd->active) {
		counter = 0;
		printf( "(Lilypad) SetHook\n" );
		ikhd->hHook = SetWindowsHookEx(WH_KEYBOARD_LL, IgnoreKeyboardHook, hInst, 0);//GetCurrentThreadId());
		if (ikhd->hHook == 0) ikhd->Deactivate();
	}
	printf( "(Lilypad) HookThinking!\n" );
	counter ++;
	if (counter % 1000 == 0)
		return CONTINUE_BLISSFULLY_AND_RELEASE_PROC;
	else
		return CONTINUE_BLISSFULLY;
}

LRESULT CALLBACK IgnoreKeyboardHook(int code, WPARAM wParam, LPARAM lParam) {
	HHOOK hHook = 0;
	if (ikhd) {
		printf( "(Lilypad) Hooked!!\n" );
		hHook = ikhd->hHook;
		if (hHook) {
			if (code == HC_ACTION) {
				if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
					KBDLLHOOKSTRUCT* key = (KBDLLHOOKSTRUCT*) lParam;
					if (key->vkCode < 256) {
						for (int i=0; i<ikhd->pads[0][0].numBindings; i++) {
							if (ikhd->pads[0][0].bindings[i].controlIndex == key->vkCode) {
								return 1;
							}
						}
						if (ikhd->binding){
							ikhd->UpdateKey(key->vkCode, 1);
							return 1;
						}
					}
					if (config.vistaVolume) {
						if (key->vkCode == VK_VOLUME_DOWN) {
							SetVolume(config.volume-3);
							return 1;
						}
						if (key->vkCode == VK_VOLUME_UP) {
							SetVolume(config.volume+3);
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

