/*  PADwin
 *  Copyright (C) 2002-2004  PADwin Team
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

#ifdef __WIN32__
#include <windows.h>
#include <windowsx.h>

#else

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#endif

#define PADdefs
#include "PS2Edefs.h"
#include "resource.h"

char *libraryName;

typedef struct {
	unsigned long keys[2][16];
	int log;
} PADconf;

typedef struct {
	u8 x,y;
	u8 button;
} Analog;

Analog lanalog, ranalog;
PADconf conf;

extern FILE *padLog;
#define PAD_LOG __Log


/* from pad.h */

/*
 * Button bits
 */
#define PAD_LEFT      0x8000
#define PAD_DOWN      0x4000
#define PAD_RIGHT     0x2000
#define PAD_UP        0x1000
#define PAD_START     0x0800
#define PAD_R3        0x0400
#define PAD_L3        0x0200
#define PAD_SELECT    0x0100
#define PAD_SQUARE    0x0080
#define PAD_CROSS     0x0040
#define PAD_CIRCLE    0x0020
#define PAD_TRIANGLE  0x0010
#define PAD_R1        0x0008
#define PAD_L1        0x0004
#define PAD_R2        0x0002
#define PAD_L2        0x0001

/* end of pad.h */

keyEvent event;

u16 status[2];
int pressure;
extern u32 pads;

int POV(u32 direction, u32 angle);
s32  _PADopen(void *pDsp);
void _PADclose();
void _KeyPressNE(u32 key);
void _KeyPress(u32 key);
void _KeyReleaseNE(u32 key);
void _KeyRelease(u32 key);
void PADsetMode(int pad, int mode);
void _PADupdate();

void __Log(char *fmt, ...);
void LoadConfig();
void SaveConfig();

void SysMessage(char *fmt, ...);

#endif


