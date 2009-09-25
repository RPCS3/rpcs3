#include "Global.h"
#include "InputManager.h"

#include "DeviceEnumerator.h"
#include "WindowsMessaging.h"
#include "DirectInput.h"
#include "KeyboardHook.h"
#include "RawInput.h"
#include "XInput.h"
#include "HidDevice.h"
#include "DualShock3.h"

void EnumDevices(int hideDXXinput) {
	// Needed for enumeration of some device types.
	dm->ReleaseInput();
	InputDeviceManager *oldDm = dm;
	dm = new InputDeviceManager();

	EnumHookDevices();
	EnumWindowsMessagingDevices();
	EnumRawInputDevices();
	EnumDualShock3s();
	EnumXInputDevices();
	EnumDirectInputDevices(hideDXXinput);

	dm->CopyBindings(oldDm->numDevices, oldDm->devices);

	delete oldDm;
}
