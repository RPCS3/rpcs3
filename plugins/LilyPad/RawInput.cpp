#include "Global.h"
#include "WindowsMessaging.h"
#include "VKey.h"
#include "DeviceEnumerator.h"
#include "WndProcEater.h"
#include "WindowsKeyboard.h"
#include "WindowsMouse.h"

#include "Config.h"

typedef BOOL (CALLBACK *_RegisterRawInputDevices)(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize);
typedef UINT (CALLBACK *_GetRawInputDeviceInfo)(HANDLE hDevice, UINT uiCommand, LPVOID pData, PUINT pcbSize);
typedef UINT (CALLBACK *_GetRawInputData)(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader);
typedef UINT (CALLBACK *_GetRawInputDeviceList)(PRAWINPUTDEVICELIST pRawInputDeviceList, PUINT puiNumDevices, UINT cbSize);

_RegisterRawInputDevices pRegisterRawInputDevices = 0;
_GetRawInputDeviceInfo pGetRawInputDeviceInfo = 0;
_GetRawInputData pGetRawInputData = 0;
_GetRawInputDeviceList pGetRawInputDeviceList = 0;

ExtraWndProcResult RawInputWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output);

int GetRawKeyboards(HWND hWnd) {
	RAWINPUTDEVICE Rid;
	Rid.hwndTarget = hWnd;

	Rid.dwFlags = 0;
	Rid.usUsagePage = 0x01;
	Rid.usUsage = 0x06;
	return pRegisterRawInputDevices(&Rid, 1, sizeof(Rid));
}

void ReleaseRawKeyboards() {
	RAWINPUTDEVICE Rid;
	Rid.hwndTarget = 0;

	Rid.dwFlags = RIDEV_REMOVE;
	Rid.usUsagePage = 0x01;
	Rid.usUsage = 0x06;
	pRegisterRawInputDevices(&Rid, 1, sizeof(Rid));
}

int GetRawMice(HWND hWnd) {
	RAWINPUTDEVICE Rid;
	Rid.hwndTarget = hWnd;

	Rid.dwFlags = RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE;
	Rid.usUsagePage = 0x01;
	Rid.usUsage = 0x02;
	return pRegisterRawInputDevices(&Rid, 1, sizeof(Rid));
}

void ReleaseRawMice() {
	RAWINPUTDEVICE Rid;
	Rid.hwndTarget = 0;

	Rid.dwFlags = RIDEV_REMOVE;
	Rid.usUsagePage = 0x01;
	Rid.usUsage = 0x02;
	pRegisterRawInputDevices(&Rid, 1, sizeof(Rid));
}

// Count of active raw keyboard devices.
// when it gets to 0, release them all.
static int rawKeyboardActivatedCount = 0;
// Same for mice.
static int rawMouseActivatedCount = 0;

class RawInputKeyboard : public WindowsKeyboard {
public:
	HANDLE hDevice;

	RawInputKeyboard(HANDLE hDevice, wchar_t *name, wchar_t *instanceID=0) : WindowsKeyboard(RAW, name, instanceID) {
		this->hDevice = hDevice;
	}

	int Activate(void *d) {
		InitInfo *info = (InitInfo*)d;
		Deactivate();
		HWND hWnd = info->hWnd;
		if (info->hWndButton) {
			hWnd = info->hWndButton;
		}
		active = 1;
		if (!rawKeyboardActivatedCount++) {
			if (!rawMouseActivatedCount && !EatWndProc(hWnd, RawInputWndProc)) {
				Deactivate();
				return 0;
			}
			if (!GetRawKeyboards(hWnd)) {
				Deactivate();
				return 0;
			}
		}

		InitState();
		return 1;
	}

	void Deactivate() {
		FreeState();
		if (active) {
			active = 0;
			rawKeyboardActivatedCount --;
			if (!rawKeyboardActivatedCount) {
				ReleaseRawKeyboards();
				if (!rawMouseActivatedCount)
					ReleaseExtraProc(RawInputWndProc);
			}
		}
	}
};

class RawInputMouse : public WindowsMouse {
public:
	HANDLE hDevice;

	RawInputMouse(HANDLE hDevice, wchar_t *name, wchar_t *instanceID=0, wchar_t *productID=0) : WindowsMouse(RAW, 0, name, instanceID, productID) {
		this->hDevice = hDevice;
	}

	int Activate(void *d) {
		InitInfo *info = (InitInfo*)d;
		Deactivate();
		HWND hWnd = info->hWnd;
		if (info->hWndButton) {
			hWnd = info->hWndButton;
		}

		active = 1;

		// Have to be careful with order.  At worst, one unmatched call to ReleaseRawMice on
		// EatWndProc fail.  In all other cases, no unmatched initialization/cleanup
		// lines.
		if (!rawMouseActivatedCount++) {
			GetMouseCapture(hWnd);
			if (!rawKeyboardActivatedCount && !EatWndProc(hWnd, RawInputWndProc)) {
				Deactivate();
				return 0;
			}
			if (!GetRawMice(hWnd)) {
				Deactivate();
				return 0;
			}
		}

		AllocState();
		return 1;
	}

	void Deactivate() {
		FreeState();
		if (active) {
			active = 0;
			rawMouseActivatedCount --;
			if (!rawMouseActivatedCount) {
				ReleaseRawMice();
				ReleaseMouseCapture();
				if (!rawKeyboardActivatedCount) {
					ReleaseExtraProc(RawInputWndProc);
				}
			}
		}
	}
};

ExtraWndProcResult RawInputWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output) {
	if (uMsg == WM_INPUT) {
		if (GET_RAWINPUT_CODE_WPARAM (wParam) == RIM_INPUT && pGetRawInputData) {
			RAWINPUT in;
			unsigned int size = sizeof(RAWINPUT);
			if (0 < pGetRawInputData((HRAWINPUT)lParam, RID_INPUT, &in, &size, sizeof(RAWINPUTHEADER))) {
				for (int i=0; i<dm->numDevices; i++) {
					Device *dev = dm->devices[i];
					if (dev->api != RAW || !dev->active) continue;
					if (in.header.dwType == RIM_TYPEKEYBOARD && dev->type == KEYBOARD) {
						RawInputKeyboard* rik = (RawInputKeyboard*)dev;
						if (rik->hDevice != in.header.hDevice) continue;

						u32 uMsg = in.data.keyboard.Message;
						if (!(in.data.keyboard.VKey>>8))
							rik->UpdateKey((u8) in.data.keyboard.VKey, (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN));
					}
					else if (in.header.dwType == RIM_TYPEMOUSE && dev->type == MOUSE) {
						RawInputMouse* rim = (RawInputMouse*)dev;
						if (rim->hDevice != in.header.hDevice) continue;
						if (in.data.mouse.usFlags) {
							// Never been set for me, and specs on what most of them
							// actually mean is sorely lacking.  Also, specs erroneously
							// indicate MOUSE_MOVE_RELATIVE is a flag, when it's really
							// 0...
							continue;
						}

						unsigned short buttons = in.data.mouse.usButtonFlags & 0x3FF;
						int button = 0;
						while (buttons) {
							if (buttons & 3) {
								// 2 is up, 1 is down.  Up takes precedence over down.
								rim->UpdateButton(button, !(buttons & 2));
							}
							button++;
							buttons >>= 2;
						}
						if (in.data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
							rim->UpdateAxis(2, ((short)in.data.mouse.usButtonData)/WHEEL_DELTA);
						}
						if (in.data.mouse.lLastX  || in.data.mouse.lLastY) {
							rim->UpdateAxis(0, in.data.mouse.lLastX);
							rim->UpdateAxis(1, in.data.mouse.lLastY);
						}
					}
				}
			}
		}
	}
	else if (uMsg == WM_ACTIVATE) {
		for (int i=0; i<dm->numDevices; i++) {
			Device *dev = dm->devices[i];
			if (dev->api != RAW || dev->physicalControlState == 0) continue;
			memset(dev->physicalControlState, 0, sizeof(int) * dev->numPhysicalControls);
		}
	}
	else if (uMsg == WM_SIZE && rawMouseActivatedCount) {
		// Doesn't really matter for raw mice, as I disable legacy stuff, but shouldn't hurt.
		WindowsMouse::WindowResized(hWnd);
	}

	return CONTINUE_BLISSFULLY;
}

int InitializeRawInput() {
	static int RawInputFailed = 0;
	if (RawInputFailed) return 0;
	if (!pGetRawInputDeviceList) {
		HMODULE user32 = LoadLibrary(L"user32.dll");
		if (user32) {
			if (!(pRegisterRawInputDevices = (_RegisterRawInputDevices) GetProcAddress(user32, "RegisterRawInputDevices")) ||
				!(pGetRawInputDeviceInfo = (_GetRawInputDeviceInfo) GetProcAddress(user32, "GetRawInputDeviceInfoW")) ||
				!(pGetRawInputData = (_GetRawInputData) GetProcAddress(user32, "GetRawInputData")) ||
				!(pGetRawInputDeviceList = (_GetRawInputDeviceList) GetProcAddress(user32, "GetRawInputDeviceList"))) {
					FreeLibrary(user32);
					RawInputFailed = 1;
					return 0;
			}
		}
	}
	return 1;
}

void EnumRawInputDevices() {
	int count = 0;
	if (InitializeRawInput() && pGetRawInputDeviceList(0, (unsigned int*)&count, sizeof(RAWINPUTDEVICELIST)) != (UINT)-1 && count > 0) {
		wchar_t *instanceID = (wchar_t *) malloc(41000*sizeof(wchar_t));
		wchar_t *keyName = instanceID + 11000;
		wchar_t *displayName = keyName + 10000;
		wchar_t *productID = displayName + 10000;

		RAWINPUTDEVICELIST *list = (RAWINPUTDEVICELIST*) malloc(sizeof(RAWINPUTDEVICELIST) * count);
		int keyboardCount = 1;
		int mouseCount = 1;
		count = pGetRawInputDeviceList(list, (unsigned int*)&count, sizeof(RAWINPUTDEVICELIST));

		// Not necessary, but reminder that count is -1 on failure.
		if (count > 0) {
			for (int i=0; i<count; i++) {
				if (list[i].dwType != RIM_TYPEKEYBOARD && list[i].dwType != RIM_TYPEMOUSE) continue;

				UINT bufferLen = 10000;
				int nameLen = pGetRawInputDeviceInfo(list[i].hDevice, RIDI_DEVICENAME, instanceID, &bufferLen);
				if (nameLen >= 4) {
					// nameLen includes terminating null.
					nameLen--;

					// Strip out GUID parts of instanceID to make it a generic product id,
					// and reformat it to point to registry entry containing device description.
					wcscpy(productID, instanceID);
					wchar_t *temp = 0;
					for (int j=0; j<3; j++) {
						wchar_t *s = wcschr(productID, '#');
						if (!s) break;
						*s = '\\';
						if (j==2) {
							*s = 0;
						}
						if (j==1) temp = s;
					}

					wsprintfW(keyName, L"SYSTEM\\CurrentControlSet\\Enum%s", productID+3);
					if (temp) *temp = 0;
					displayName[0] = 0;
					HKEY hKey;
					if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyName, 0, KEY_QUERY_VALUE, &hKey)) {
						DWORD type;
						DWORD len = 10000 * sizeof(wchar_t);
						if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"DeviceDesc", 0, &type, (BYTE*)displayName, &len) &&
							len && type == REG_SZ) {
								wchar_t *temp2 = wcsrchr(displayName, ';');
								if (!temp2) temp2 = displayName;
								else temp2++;
								// Could do without this, but more effort than it's worth.
								wcscpy(keyName, temp2);
						}
						RegCloseKey(hKey);
					}
					if (list[i].dwType == RIM_TYPEKEYBOARD) {
						if (!displayName[0]) wsprintfW(displayName, L"Raw Keyboard %i", keyboardCount++);
						else wsprintfW(displayName, L"Raw KB: %s", keyName);
						dm->AddDevice(new RawInputKeyboard(list[i].hDevice, displayName, instanceID));
					}
					else if (list[i].dwType == RIM_TYPEMOUSE) {
						if (!displayName[0]) wsprintfW(displayName, L"Raw Mouse %i", mouseCount++);
						else wsprintfW(displayName, L"Raw MS: %s", keyName);
						dm->AddDevice(new RawInputMouse(list[i].hDevice, displayName, instanceID, productID));
					}
				}
			}
		}
		free(list);
		free(instanceID);
		dm->AddDevice(new RawInputKeyboard(0, L"Simulated Keyboard"));
		dm->AddDevice(new RawInputMouse(0, L"Simulated Mouse"));
	}
}
