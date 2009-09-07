/*  dev9null
 *  Copyright (C) 2002-2009 pcsx2 Team
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

#ifndef __DEV9_H__
#define __DEV9_H__

#include <stdio.h>

#define DEV9defs
#include "PS2Edefs.h"

#ifdef __LINUX__
#include <gtk/gtk.h>
#else
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#endif

typedef struct {
  int Log;
} Config;

extern Config conf;
#define DEV9_LOG __Log

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

extern const unsigned char version;
extern const unsigned char revision;
extern const unsigned char build;
extern const unsigned int minor;
extern const char *libraryName;

void SaveConfig();
void LoadConfig();

extern void (*DEV9irq)(int);
extern void SysMessage(const char *fmt, ...);

extern FILE *dev9Log;
void __Log(char *fmt, ...);


#endif
