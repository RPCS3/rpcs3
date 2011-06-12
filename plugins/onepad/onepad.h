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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __PAD_H__
#define __PAD_H__

#include <stdio.h>
#include <assert.h>
#include <queue>

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

#ifdef _MSC_VER
#define EXPORT_C_(type) extern "C" __declspec(dllexport) type CALLBACK
#else
#define EXPORT_C_(type) extern "C" __attribute__((externally_visible,visibility("default"))) type
#endif

extern char libraryName[256];

enum PadOptions
{
	PADOPTION_FORCEFEEDBACK = 0x1,
	PADOPTION_REVERSELX = 0x2,
	PADOPTION_REVERSELY = 0x4,
	PADOPTION_REVERSERX = 0x8,
	PADOPTION_REVERSERY = 0x10,
	PADOPTION_MOUSE_L = 0x20,
	PADOPTION_MOUSE_R = 0x40
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
	PAD_L2 = 0,
	PAD_R2,
	PAD_L1,
	PAD_R1,
	PAD_TRIANGLE,
	PAD_CIRCLE,
	PAD_CROSS,
	PAD_SQUARE,
	PAD_SELECT,
	PAD_L3,
	PAD_R3,
	PAD_START,
	PAD_UP,
	PAD_RIGHT,
	PAD_DOWN,
	PAD_LEFT,
	PAD_L_UP,
	PAD_L_RIGHT,
	PAD_L_DOWN,
	PAD_L_LEFT,
	PAD_R_UP,
	PAD_R_RIGHT,
	PAD_R_DOWN,
	PAD_R_LEFT
};

// Activate bolche's analog controls hack
// DEF_VALUE is the strength you press the control.
// Code taken from http://forums.pcsx2.net/thread-4699.html

#define DEF_VALUE	  	32766

/* end of pad.h */

extern keyEvent event;

extern queue<keyEvent> ev_fifo;
extern pthread_mutex_t	mutex_KeyEvent;

extern u16 status[2];
extern int status_pressure[2][MAX_KEYS];
extern u32 pads;

void clearPAD(int pad);
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
