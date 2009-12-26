/*  OnePAD - author: arcum42(@gmail.com)
 *  Copyright (C) 2009
 *
 *  Based on ZeroPAD, author zerofrog@gmail.com
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
#include "controller.h"

extern char libraryName[256];

enum PadOptions
{
	PADOPTION_FORCEFEEDBACK = 0x1,
	PADOPTION_REVERTLX = 0x2,
	PADOPTION_REVERTLY = 0x4,
	PADOPTION_REVERTRX = 0x8,
	PADOPTION_REVERTRY = 0x10
};

extern FILE *padLog;
extern void initLogging();
extern bool toggleAutoRepeat;

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

void clearPAD();
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

#ifdef __cplusplus

#ifdef _MSC_VER
#define EXPORT_C_(type) extern "C" __declspec(dllexport) type CALLBACK
#else
#define EXPORT_C_(type) extern "C" type
#endif

#else

#ifdef _MSC_VER
#define EXPORT_C_(type) __declspec(dllexport) type __stdcall
#else
#define EXPORT_C_(type) type
#endif

#endif

#endif


