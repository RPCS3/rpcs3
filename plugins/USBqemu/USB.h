/*  USBqemu
 *  Copyright (C) 2002-2011  PCSX2 Team
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

#pragma once

#ifndef __USB_H__
#define __USB_H__

#include <stdio.h>

#define USBdefs
#include "PS2Edefs.h"

#ifdef __WIN32__
#	include <windows.h>
#	include <windowsx.h>
#else
#	include <gtk/gtk.h>
#	define __inline inline
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define USB_LOG __Log

typedef struct {
  bool Log;
} Config;

extern Config conf;

#define PSXCLK	36864000	/* 36.864 Mhz */

void USBirq(int);

void SaveConfig();
void LoadConfig();

extern FILE *usbLog;
void __Log(char *fmt, ...);

extern void SysMessage(const char *fmt, ...);
extern void SysMessage(const wchar_t *fmt, ...);

extern HWND gsWindowHandle;

#endif
