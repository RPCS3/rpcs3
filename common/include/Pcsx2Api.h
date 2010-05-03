/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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


#ifndef __PCSX2API_H__
#define __PCSX2API_H__

// Note; this header is experimental, and will be a shifting target. Only use this if you are willing to repeatedly fix breakage.

/*
 *  Based on PS2E Definitions by
	   linuzappz@hotmail.com,
 *          shadowpcsx2@yahoo.gr,
 *          and florinsasu@hotmail.com
 */

#include "Pcsx2Types.h"
#include "Pcsx2Defs.h"

// Indicate to use the new versions.
#define NEW_PLUGIN_APIS

#ifdef _MSC_VER
#define EXPORT_C(type) extern "C" __declspec(dllexport) type CALLBACK
#else
#define EXPORT_C(type) extern "C" type
#endif

EXPORT_C(u32) PS2EgetLibType(void);
EXPORT_C(u32) PS2EgetLibVersion2(u32 type);
EXPORT_C(char*) PS2EgetLibName(void);

// Extended functions.

// allows the plugin to see the whole configuration when started up.
// Intended for them to get the ini and plugin paths, but could allow for other things as well.
EXPORT_C_(void) PS2EpassConfig(PcsxConfig Config);

// Alternately, this function serves the same purpose, but would work for emulators outside
// of pcsx2.
EXPORT_C_(void) PS2EpassIniPath(const char *path);

// PS2EgetLibType returns (may be OR'd)
enum {
PS2E_LT_GS =  0x01,
PS2E_LT_PAD = 0x02,		// -=[ OBSOLETE ]=-
PS2E_LT_SPU2 = 0x04,
PS2E_LT_CDVD = 0x08,
PS2E_LT_DEV9 = 0x10,
PS2E_LT_USB = 0x20,
PS2E_LT_FW =  0x40,
PS2E_LT_SIO = 0x80
} PluginLibType;

// PS2EgetLibVersion2 (high 16 bits)
enum {
PS2E_GS_VERSION =  0x0006,
PS2E_PAD_VERSION  = 0x0002,	// -=[ OBSOLETE ]=-
PS2E_SPU2_VERSION = 0x0005,
PS2E_CDVD_VERSION = 0x0005,
PS2E_DEV9_VERSION = 0x0003,
PS2E_USB_VERSION = 0x0003,
PS2E_FW_VERSION =  0x0002,
PS2E_SIO_VERSION = 0x0001
} PluginLibVersion;

// freeze modes:
enum {
FREEZE_LOAD = 0,
FREEZE_SAVE = 1,
FREEZE_SIZE = 2
} FreezeModes;

typedef struct _GSdriverInfo {
	char name[8];
	void *common;
} GSdriverInfo;

#ifdef _MSC_VER
typedef struct _winInfo { // unsupported values must be set to zero
	HWND hWnd;
	HMENU hMenu;
	HWND hStatusWnd;
} winInfo;
#endif

#endif //  __PCSX2API_H__
