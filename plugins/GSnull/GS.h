/*  GSnull
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

#ifndef __GS_H__
#define __GS_H__

struct _keyEvent;
typedef struct _keyEvent keyEvent;

#include <stdio.h>
#include "Pcsx2Defs.h"

#ifdef _WIN32
#	include "Windows/GSwin.h"
#endif

#ifdef __LINUX__
#	include "Linux/GSLinux.h"
#endif

#define GSdefs
#include "PS2Edefs.h"

#include "Registers.h"
#include "null/GSnull.h"

/*#ifdef _MSC_VER
#define EXPORT_C_(type) extern "C" __declspec(dllexport) type CALLBACK
#else
#define EXPORT_C_(type) extern "C" type
#endif*/

#ifdef _MSC_VER
#define EXPORT_C_(type) extern "C" type CALLBACK
#else
#define EXPORT_C_(type) extern "C" type
#endif

#define GS_LOG __Log

typedef struct
{
	int Log;
} Config;

extern Config conf;
extern FILE *gsLog;
extern u32 GSKeyEvent;
extern bool GSShift, GSAlt;

extern void (*GSirq)();

extern void __Log(char *fmt, ...);
extern void SysMessage(char *fmt, ...);
extern void SysPrintf(const char *fmt, ...);
extern void SaveConfig();
extern void LoadConfig();

#endif
