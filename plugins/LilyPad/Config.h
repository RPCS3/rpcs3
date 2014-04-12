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

#ifndef CONFIG_H
#define CONFIG_H

extern u8 ps2e;

enum PadType {
	DisabledPad,
	Dualshock2Pad,
	GuitarPad
};

struct PadConfig {
	PadType type;
	u8 autoAnalog;
};

struct GeneralConfig {
public:
	PadConfig padConfigs[2][4];

	u8 closeHacks;

	DeviceAPI keyboardApi;
	DeviceAPI mouseApi;

	// Derived value, calculated by GetInput().
	u8 ignoreKeys;

	union {
		struct {
			u8 forceHide;
			u8 mouseUnfocus;
			u8 background;
			u8 multipleBinding;

			struct {
				u8 directInput;
				u8 xInput;
				u8 dualShock3;
			} gameApis;

			u8 multitap[2];

			u8 escapeFullscreenHack;
			u8 disableScreenSaver;
			u8 debug;

			u8 saveStateTitle;
			u8 GH2;
			u8 turboKeyHack;

			u8 vistaVolume;
		};
		u8 bools[1];
	};

	int volume;

	// Unlike the others, not a changeable value.
	DWORD osVersion;

	wchar_t lastSaveConfigPath[MAX_PATH+1];
	wchar_t lastSaveConfigFileName[MAX_PATH+1];
};

extern GeneralConfig config;

void UnloadConfigs();

void AddIgnore(LPARAM k);

void SetVolume(int volume);

int LoadSettings(int force = 0, wchar_t *file = 0);

// Refreshes the set of enabled devices.
void RefreshEnabledDevices(int updateDeviceList = 0);

void Configure();
#endif
