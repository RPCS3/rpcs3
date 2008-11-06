/*  FWnull 
 *  Copyright (C) 2004-2005 PCSX2 Team
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


#ifndef __FW_H__
#define __FW_H__

#include <stdio.h>

#define FWdefs
#include "PS2Edefs.h"

#ifdef _WIN32

#include <windows.h>
#include <windowsx.h>

#else

#include <gtk/gtk.h>
#include <X11/Xlib.h>

#define __inline inline

#endif

#define FW_LOG __Log

typedef struct {
  int Log;
} Config;

Config conf;
void (*FWirq)();

void SaveConfig();
void LoadConfig();

FILE *fwLog;
void __Log(char *fmt, ...);

void SysMessage(char *fmt, ...);

#endif
