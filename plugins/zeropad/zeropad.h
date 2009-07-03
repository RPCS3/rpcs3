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
#include <pthread.h>
using namespace std;

#define PADdefs
#include "PS2Edefs.h"

#ifdef __LINUX__
#include "joystick.h"
#endif
#include "analog.h"
#include "bitwise.h"

extern char libraryName[256];

#define IS_KEYBOARD(key) (key < 0x10000)
#define IS_JOYBUTTONS(key) (key >= 0x10000 && key < 0x20000) // buttons
#define IS_JOYSTICK(key) (key >= 0x20000 && key < 0x30000) // analog
#define IS_POV(key) (key >= 0x30000 && key < 0x40000) // uses analog as buttons (cares about sign)
#define IS_HAT(key) (key >= 0x40000 && key < 0x50000) // uses hat as buttons (cares about sign)

//#define IS_MOUSE(key) (key >= 0x40000 && key < 0x50000) // mouse

#define PAD_GETKEY(key) ((key) & 0xffff)
#define PAD_GETJOYID(key) (((key) & 0xf000) >> 12)
#define PAD_GETJOYBUTTON(key) ((key) & 0xff)
#define PAD_GETJOYSTICK_AXIS(key) ((key) & 0xff)
#define PAD_JOYBUTTON(joyid, buttonid) (0x10000 | ((joyid) << 12) | (buttonid))
#define PAD_JOYSTICK(joyid, axisid) (0x20000 | ((joyid) << 12) | (axisid))
#define PAD_POV(joyid, sign, axisid) (0x30000 | ((joyid) << 12) | ((sign) << 8) | (axisid))
#define PAD_HAT(joyid, dir, axisid) (0x40000 | ((joyid) << 12) | ((dir) << 8) | (axisid))

#define PAD_GETPOVSIGN(key) (((key) & 0x100) >> 8)
#define PAD_GETHATDIR(key) (((key) & ~ 0x40000) >> 8)

#ifdef __LINUX__
#define PADKEYS 28
#else
#define PADKEYS 20
#endif

#define PADOPTION_FORCEFEEDBACK 1
#define PADOPTION_REVERTLX 0x2
#define PADOPTION_REVERTLY 0x4
#define PADOPTION_REVERTRX 0x8
#define PADOPTION_REVERTRY 0x10

#ifdef _WIN32
#define PADSUBKEYS 1
#else
#define PADSUBKEYS 2
#endif

//#define EXPERIMENTAL_POV_CODE
extern int PadEnum[2][2];

typedef struct
{
	u32 keys[2 * PADSUBKEYS][PADKEYS];
	u32 log;
	u32 options;  // upper 16 bits are for pad2
} PADconf;

typedef struct
{
	u8 x, y;
} PADAnalog;

extern PADconf conf;
extern PADAnalog g_lanalog[2], g_ranalog[2];
extern FILE *padLog;
extern void initLogging();
#define PAD_LOG __Log
//#define PAD_LOG __LogToConsole

enum PadCommands
{
	CMD_SET_VREF_PARAM = 0x40,
	CMD_QUERY_DS2_ANALOG_MODE = 0x41,
	CMD_READ_DATA_AND_VIBRATE = 0x42,
	CMD_CONFIG_MODE = 0x43,
	CMD_SET_MODE_AND_LOCK = 0x44,
	CMD_QUERY_MODEL_AND_MODE = 0x45,
	CMD_QUERY_ACT = 0x46, // ??
	CMD_QUERY_COMB = 0x47, // ??
	CMD_QUERY_MODE = 0x4C, // QUERY_MODE ??
	CMD_VIBRATION_TOGGLE = 0x4D,
	CMD_SET_DS2_NATIVE_MODE = 0x4F // SET_DS2_NATIVE_MODE
};

enum gamePadValues 
{
	PAD_R_LEFT = 27,
	PAD_R_DOWN = 26,
	PAD_R_RIGHT = 25,
	PAD_R_UP = 24,
	PAD_L_LEFT = 23,
	PAD_L_DOWN = 22,
	PAD_L_RIGHT = 21,
	PAD_L_UP = 20,
	PAD_RY = 19,
	PAD_LY = 18,
	PAD_RX = 17,
	PAD_LX = 16,
	PAD_LEFT = 15,
	PAD_DOWN = 14,
	PAD_RIGHT = 13,
	PAD_UP = 12,
	PAD_START = 11,
	PAD_R3 = 10,
	PAD_L3 = 9,
	PAD_SELECT = 8,
	PAD_SQUARE = 7,
	PAD_CROSS = 6,
	PAD_CIRCLE = 5,
	PAD_TRIANGLE = 4,
	PAD_R1 = 3,
	PAD_L1 = 2,
	PAD_R2 = 1,
	PAD_L2 = 0
};

// Activate bolche's analog controls hack
// DEF_VALUE is the strength you press the control.
// Code taken from http://forums.pcsx2.net/thread-4699.html

#define DEF_VALUE	  	32766

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

void __Log(const char *fmt, ...);
void __LogToConsole(const char *fmt, ...);
void LoadConfig();
void SaveConfig();

void SysMessage(char *fmt, ...);
void UpdateKeys(int pad, int keyPress, int keyRelease);

#endif


