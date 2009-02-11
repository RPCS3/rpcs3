#include "global.h"
#include "VKey.h"
#include "WindowsKeyboard.h"
#include "KeyboardQueue.h"

WindowsKeyboard::WindowsKeyboard(DeviceAPI api, wchar_t *displayName, wchar_t *instanceID, wchar_t *deviceID) :
Device(api, KEYBOARD, displayName, instanceID, deviceID) {
	for (int i=0; i<256; i++) {
		AddPhysicalControl(PSHBTN, i, i);
	}
}

wchar_t *WindowsKeyboard::GetPhysicalControlName(PhysicalControl *control) {
	int id = control->id;
	if (control->type == PSHBTN && id >= 0 && id < 256) {
		wchar_t *w =  GetVKStringW(id);
		if (w) return w;
	}
	return Device::GetPhysicalControlName(control);
}

void WindowsKeyboard::UpdateKey(int vkey, int state) {
	if (vkey > 4 && vkey < 256) {
		physicalControlState[vkey] = (state << 16);
	}
}

void WindowsKeyboard::InitState() {
	AllocState();
	for (int vkey=5; vkey<256; vkey++) {
		int value = (unsigned short)(((short)GetAsyncKeyState(vkey))>>15);
		value += value&1;
		if (vkey == VK_CONTROL || vkey == VK_MENU || vkey == VK_SHIFT) {
			value = 0;
		}
		physicalControlState[vkey] = value;
	}
}
