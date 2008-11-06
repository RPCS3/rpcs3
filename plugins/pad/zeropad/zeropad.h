/*  ZeroPAD - author: zerofrog(@gmail.com)
 *  Copyright (C) 2006-2007
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

#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>

#else

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#endif

#include <vector>
#include <map>
#include <string>
using namespace std;

#define PADdefs
extern "C" {
#include "PS2Edefs.h"
}

extern char *libraryName;

#define FORIT(it, v) for(it = (v).begin(); it != (v).end(); (it)++)

#define IS_KEYBOARD(key) (key<0x10000)
#define IS_JOYBUTTONS(key) (key>=0x10000 && key<0x20000) // buttons
#define IS_JOYSTICK(key) (key>=0x20000&&key<0x30000) // analog
#define IS_POV(key) (key>=0x30000&&key<0x40000) // uses analog as buttons (cares about sign)
#define IS_MOUSE(key) (key>=0x40000&&key<0x50000) // mouse

#define PAD_GETKEY(key) ((key)&0xffff)
#define PAD_GETJOYID(key) (((key)&0xf000)>>12)
#define PAD_GETJOYBUTTON(key) ((key)&0xff)
#define PAD_GETJOYSTICK_AXIS(key) ((key)&0xff)
#define PAD_JOYBUTTON(joyid, buttonid) (0x10000|((joyid)<<12)|(buttonid))
#define PAD_JOYSTICK(joyid, axisid) (0x20000|((joyid)<<12)|(axisid))
#define PAD_POV(joyid, sign, axisid) (0x30000|((joyid)<<12)|((sign)<<8)|(axisid))
#define PAD_GETPOVSIGN(key) (((key)&0x100)>>8)

#define PADKEYS 20

#define PADOPTION_FORCEFEEDBACK 1
#define PADOPTION_REVERTLX 0x2
#define PADOPTION_REVERTLY 0x4
#define PADOPTION_REVERTRX 0x8
#define PADOPTION_REVERTRY 0x10

typedef struct {
	unsigned long keys[2][PADKEYS];
	int log;
    int options;  // upper 16 bits are for pad2
} PADconf;

typedef struct {
	u8 x,y;
} PADAnalog;

extern PADconf conf;

extern PADAnalog g_lanalog[2], g_ranalog[2];
extern FILE *padLog;
#define PAD_LOG __Log

#define PAD_RY        19
#define PAD_LY        18
#define PAD_RX        17
#define PAD_LX        16
#define PAD_LEFT      15
#define PAD_DOWN      14
#define PAD_RIGHT     13
#define PAD_UP        12
#define PAD_START     11
#define PAD_R3        10
#define PAD_L3        9
#define PAD_SELECT    8
#define PAD_SQUARE    7
#define PAD_CROSS     6
#define PAD_CIRCLE    5
#define PAD_TRIANGLE  4
#define PAD_R1        3
#define PAD_L1        2
#define PAD_R2        1
#define PAD_L2        0

/* end of pad.h */

extern keyEvent event;

extern u16 status[2];
extern u32 pads;

int POV(u32 direction, u32 angle);
s32  _PADopen(void *pDsp);
void _PADclose();
void _KeyPress(int pad, u32 key);
void _KeyRelease(int pad, u32 key);
void PADsetMode(int pad, int mode);
void _PADupdate(int pad);

void __Log(char *fmt, ...);
void LoadConfig();
void SaveConfig();

void SysMessage(char *fmt, ...);

#endif


