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
	if (vkey > 7 && vkey < 256) {
		int newState = state * FULLY_DOWN;
		if (newState != physicalControlState[vkey]) {
			// Check for alt-F4 to avoid toggling skip mode incorrectly.
			if (vkey != VK_F4 || !(physicalControlState[VK_MENU] || physicalControlState[VK_RMENU] || physicalControlState[VK_LMENU])) {
				int event = KEYPRESS;
				if (!newState) event = KEYRELEASE;
				QueueKeyEvent(vkey, event);
			}
		}
		physicalControlState[vkey] = newState;
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
