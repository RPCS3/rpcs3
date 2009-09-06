/*  GSnull
 *  Copyright (C) 2004-2009 PCSX2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#ifndef __GS_H__
#define __GS_H__

#include <stdio.h>

#ifdef _WIN32
#include "Windows/GSwin.h"
#endif

#include "Registers.h"

#ifdef __cplusplus
extern "C"
{
#endif 
#define GSdefs
#include "PS2Edefs.h"
#ifdef __cplusplus
}
#endif

#ifdef __LINUX__
#include "Linux/GSLinux.h"
#endif

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