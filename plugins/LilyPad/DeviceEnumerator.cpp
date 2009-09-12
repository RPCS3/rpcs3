#include "Global.h"
#include "DeviceEnumerator.h"
#include "InputManager.h"
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

	EnumDualShock3s();
	EnumHookDevices();
	EnumWindowsMessagingDevices();
	EnumRawInputDevices();
	EnumXInputDevices();
	EnumDirectInputDevices(hideDXXinput);

	dm->CopyBindings(oldDm->numDevices, oldDm->devices);

	delete oldDm;
}
