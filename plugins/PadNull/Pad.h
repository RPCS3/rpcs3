/*  PadNull
 *  Copyright (C) 2004-2010  PCSX2 Dev Team
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

#ifndef __PAD_H__
#define __PAD_H__

#include <stdio.h>

#define PADdefs
#include "PS2Edefs.h"
#include "PS2Eext.h"

#ifdef _WIN32
#include "PadWin.h"
#else
#include "PadLinux.h"
#endif

/*#ifdef _MSC_VER
#define EXPORT_C_(type) extern "C" __declspec(dllexport) type CALLBACK
#else
#define EXPORT_C_(type) extern "C" type
#endif*/

#ifdef _MSC_VER
#define EXPORT_C_(type) extern "C" type CALLBACK
#else
#define EXPORT_C_(type) extern "C" __attribute__((externally_visible,visibility("default"))) type
#endif

#define PAD_LOG __Log

typedef struct
{
	s32 Log;
} Config;

extern Config conf;
extern FILE *padLog;
extern keyEvent event;

extern void __Log(char *fmt, ...);
extern void SysMessage(char *fmt, ...);
extern void SaveConfig();
extern void LoadConfig();

#endif
