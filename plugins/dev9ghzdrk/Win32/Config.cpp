/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014 David Quintana [gigaherz]
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdlib.h>

#include <winsock2.h>
#include "..\DEV9.h"

#define GetKeyV(name, var, s, t) \
	size = s; type = t; \
	RegQueryValueEx(myKey, name, 0, &type, (LPBYTE) var, &size);

#define GetKeyVdw(name, var) \
	GetKeyV(name, var, 4, REG_DWORD);

#define SetKeyV(name, var, s, t) \
	RegSetValueEx(myKey, name, 0, t, (LPBYTE) var, s);

#define SetKeyVdw(name, var) \
	SetKeyV(name, var, 4, REG_DWORD);

void SaveConf() {
	HKEY myKey;
	DWORD myDisp;

	RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\PS2Eplugin\\DEV9\\DEV9linuz", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &myKey, &myDisp);
	SetKeyV("Eth", config.Eth, strlen(config.Eth), REG_SZ);
	SetKeyV("Hdd", config.Hdd, strlen(config.Hdd), REG_SZ);
	SetKeyVdw("HddSize", &config.HddSize);
	SetKeyVdw("ethEnable", &config.ethEnable);
	SetKeyVdw("hddEnable", &config.hddEnable);

	RegCloseKey(myKey);
}

void LoadConf() {
	HKEY myKey;
	DWORD type, size;

	memset(&config, 0, sizeof(config));
	strcpy(config.Hdd, HDD_DEF);
	config.HddSize=8*1024;
	strcpy(config.Eth, ETH_DEF);

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\PS2Eplugin\\DEV9\\DEV9linuz", 0, KEY_ALL_ACCESS, &myKey)!=ERROR_SUCCESS) {
		SaveConf(); return;
	}
	GetKeyV("Eth", config.Eth, sizeof(config.Eth), REG_SZ);
	GetKeyV("Hdd", config.Hdd, sizeof(config.Hdd), REG_SZ);
	GetKeyVdw("HddSize", &config.HddSize);
	GetKeyVdw("ethEnable", &config.ethEnable);
	GetKeyVdw("hddEnable", &config.hddEnable);

	RegCloseKey(myKey);
}

