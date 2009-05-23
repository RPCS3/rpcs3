/*  FWnull
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
 
#ifndef __PAD_H__
#define __PAD_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif 
#define PADdefs
#include "PS2Edefs.h"
#ifdef __cplusplus
}
#endif

#ifdef _WIN32

#include <windows.h>
#include <windowsx.h>

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
#define EXPORT_C_(type) extern "C" type
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
