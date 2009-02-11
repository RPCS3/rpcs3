#ifndef CONFIG_H
#define CONFIG_H

#include "InputManager.h"
#include "PS2Etypes.h"

struct GeneralConfig {
public:
	u8 disablePad[2];

	u8 mouseUnfocus;
	u8 disableScreenSaver;
	u8 closeHacks;

	u8 axisButtons;
	DeviceAPI keyboardApi;
	DeviceAPI mouseApi;
	struct {
		u8 directInput;
		u8 xInput;
	} gameApis;
	u8 debug;
	u8 background;
	u8 multipleBinding;
	u8 forceHide;
	u8 GH2;

	// Derived value, calculated by GetInput().
	u8 ignoreKeys;

	u8 GSThreadUpdates;
	u8 escapeFullscreenHack;

	u8 guitar[2];
	u8 AutoAnalog[2];

	u8 saveStateTitle;

	wchar_t lastSaveConfigPath[MAX_PATH+1];
	wchar_t lastSaveConfigFileName[MAX_PATH+1];
};

extern GeneralConfig config;

void UnloadConfigs();

/*
inline int GetNeededInput(HWND hWnd, int configMode) {
	return GetInput(hWnd, flags, configMode);
}*/
void AddIgnore(LPARAM k);

int LoadSettings(int force = 0, wchar_t *file = 0);

void CALLBACK PADconfigure();

// Refreshes the set of enabled devices.
void RefreshEnabledDevices(int updateDeviceList = 0);

#endif
