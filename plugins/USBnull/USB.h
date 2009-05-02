/*  USBlinuz
 *  Copyright (C) 2002-2004  USBlinuz Team
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


#ifndef __USB_H__
#define __USB_H__

#include <stdio.h>

extern "C"
{
#define USBdefs
#include "PS2Edefs.h"
}

#ifdef _WIN32

#define usleep(x)	Sleep(x / 1000)
#include <windows.h>
#include <windowsx.h>

#else

#include <gtk/gtk.h>
#include <X11/Xlib.h>

#define __inline inline

#endif

#define USB_LOG __Log

typedef struct {
  int Log;
} Config;

//extern void (*USBirq)();
extern USBcallback USBirq;
extern Config conf;
extern FILE *usbLog;


#ifdef _MSC_VER
#define EXPORT_C_(type) extern "C" __declspec(dllexport) type CALLBACK
#else
#define EXPORT_C_(type) extern "C" type
#endif

#define PSXCLK	36864000	/* 36.864 Mhz */


void SaveConfig();
void LoadConfig();

void __Log(char *fmt, ...);

void SysMessage(char *fmt, ...);

#endif
