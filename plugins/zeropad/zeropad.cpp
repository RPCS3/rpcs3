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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>

#include "zeropad.h"

#ifndef _WIN32

#include <unistd.h>
#else
#include "svnrev.h"
#endif

char libraryName[256];

PADAnalog g_lanalog[2], g_ranalog[2];
PADconf conf;

keyEvent event;

u16 status[2];
int pressure;
string s_strIniPath = "inis/zeropad.ini";

const unsigned char version  = PS2E_PAD_VERSION;
const unsigned char revision = 0;
const unsigned char build    = 2;    // increase that with each version

int PadEnum[2][2] = {{0, 2}, {1, 3}};

u32 pads = 0;
u8 stdpar[2][20] = { 
	{0xff, 0x5a, 0xff, 0xff, 0x80, 0x80, 0x80, 0x80,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00},
	{0xff, 0x5a, 0xff, 0xff, 0x80, 0x80, 0x80, 0x80,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00}
};
u8 cmd40[2][8]    = {
	{0xff, 0x5a, 0x00, 0x00, 0x02, 0x00, 0x00, 0x5a},
	{0xff, 0x5a, 0x00, 0x00, 0x02, 0x00, 0x00, 0x5a}
};
u8 cmd41[2][8]    = {
	{0xff, 0x5a, 0xff, 0xff, 0x03, 0x00, 0x00, 0x5a},
	{0xff, 0x5a, 0xff, 0xff, 0x03, 0x00, 0x00, 0x5a}
};
u8 unk46[2][8]    = {
	{0xFF, 0x5A, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0A},
	{0xFF, 0x5A, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0A}
};
u8 unk47[2][8]    = {
	{0xff, 0x5a, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00},
	{0xff, 0x5a, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00}
};
u8 unk4c[2][8]    = {
	{0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};
u8 unk4d[2][8]    = { 
	{0xff, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	{0xff, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
};
u8 cmd4f[2][8]    = { 
	{0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5a},
	{0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5a}
};
u8 stdcfg[2][8]   = { 
	{0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
}; // 2 & 3 = 0
u8 stdmode[2][8]  = { 
	{0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};
u8 stdmodel[2][8] = { 
		{0xff,
		0x5a,
		0x03, // 03 - dualshock2, 01 - dualshock
		0x02, // number of modes
		0x01, // current mode: 01 - analog, 00 - digital
		0x02,
		0x01,
		0x00},
	{0xff, 
	 0x5a,
	 0x03, // 03 - dualshock2, 01 - dualshock
	 0x02, // number of modes
	 0x01, // current mode: 01 - analog, 00 - digital
	 0x02,
	 0x01,
	 0x00}
};

u8 *buf;
int padID[2];
int padMode[2];
int curPad;
int curByte;
int curCmd;
int cmdLen;
int ds2mode = 0; // DS Mode at start
FILE *padLog = NULL;

int POV(u32 direction, u32 angle)
{
	if ((direction == 0) && (angle >=    0) && (angle < 4500))	return 1;//forward
	if ((direction == 2) && (angle >= 4500) && (angle < 13500))	return 1;//right
	if ((direction == 1) && (angle >= 13500) && (angle < 22500))	return 1;//backward
	if ((direction == 3) && (angle >= 22500) && (angle < 31500))	return 1;//left
	if ((direction == 0) && (angle >= 31500) && (angle < 36000))	return 1;//forward
	return 0;
}

void _KeyPress(int pad, u32 key)
{
	int i;

	for (int p = 0; p < PADSUBKEYS; p++)
	{
		for (i = 0; i < PADKEYS; i++)
		{
			if (key == conf.keys[PadEnum[pad][p]][i])
			{
				status[pad] &= ~(1 << i);
				return;
			}
		}
	}

	event.evt = KEYPRESS;
	event.key = key;
}

void _KeyRelease(int pad, u32 key)
{
	int i;

	for (int p = 0; p < PADSUBKEYS; p++)
	{
		for (i = 0; i < PADKEYS; i++)
		{
			if (key == conf.keys[PadEnum[pad][p]][i])
			{
				status[pad] |= (1 << i);
				return;
			}
		}
	}

	event.evt = KEYRELEASE;
	event.key = key;
}

static void InitLibraryName()
{
#ifdef _WIN32
#	ifdef PUBLIC

	// Public Release!
	// Output a simplified string that's just our name:

	strcpy(libraryName, "ZeroPAD");

#	elif defined( SVN_REV_UNKNOWN )

	// Unknown revision.
	// Output a name that includes devbuild status but not
	// subversion revision tags:

	strcpy(libraryName, "ZeroPAD"
#		ifdef _DEBUG
	       "-Debug"
#		endif
	      );
#	else

	// Use TortoiseSVN's SubWCRev utility's output
	// to label the specific revision:

	sprintf_s(libraryName, "ZeroPAD r%d%s"
#		ifdef _DEBUG
	          "-Debug"
#		else
	          "-Dev"
#		endif
	          , SVN_REV,
	          SVN_MODS ? "m" : ""
	         );
#	endif
#else
// I'll fix up SVN support later. --arcum42

	strcpy(libraryName, "ZeroPAD"
#	ifdef _DEBUG
	       "-Debug"
#	endif
	      );
#endif
}

u32 CALLBACK PS2EgetLibType()
{
	return PS2E_LT_PAD;
}

char* CALLBACK PS2EgetLibName()
{
	InitLibraryName();
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type)
{
	return (version << 16) | (revision << 8) | build;
}

void __Log(char *fmt, ...)
{
	va_list list;

	if (!conf.log || padLog == NULL) return;
	va_start(list, fmt);
	vfprintf(padLog, fmt, list);
	va_end(list);
}


s32 CALLBACK PADinit(u32 flags)
{
#ifdef PAD_LOG
	if (padLog == NULL)
	{
		padLog = fopen("logs/padLog.txt", "w");
		if (padLog) setvbuf(padLog, NULL,  _IONBF, 0);
	}
	PAD_LOG("PADinit\n");
#endif

	pads |= flags;
	status[0] = 0xffff;
	status[1] = 0xffff;

#ifdef __LINUX__
	char strcurdir[256];
	getcwd(strcurdir, 256);
	s_strIniPath = strcurdir;
	s_strIniPath += "/inis/zeropad.ini";
#endif

	LoadConfig();

	PADsetMode(0, 0);
	PADsetMode(1, 0);

	pressure = 100;
	for (int i = 0; i < 2; ++i)
	{
		g_ranalog[i].x = 0x80;
		g_ranalog[i].y = 0x80;
		g_lanalog[i].x = 0x80;
		g_lanalog[i].y = 0x80;
	}

	return 0;
}

void CALLBACK PADshutdown()
{
#ifdef PAD_LOG
	if (padLog != NULL)
	{
		fclose(padLog);
		padLog = NULL;
	}
#endif
}

s32 CALLBACK PADopen(void *pDsp)
{
	memset(&event, 0, sizeof(event));

	return _PADopen(pDsp);
}

void CALLBACK PADclose()
{
	_PADclose();
}

u32 CALLBACK PADquery()
{
	return 3; // both
}

void PADsetMode(int pad, int mode)
{
	padMode[pad] = mode;
	switch (ds2mode)
	{
		case 0: // dualshock
			switch (mode)
			{
				case 0: // digital
					padID[pad] = 0x41;
					break;

				case 1: // analog
					padID[pad] = 0x73;
					break;
			}
			break;
		case 1: // dualshock2
			switch (mode)
			{
				case 0: // digital
					padID[pad] = 0x41;
					break;

				case 1: // analog
					padID[pad] = 0x79;
					break;
			}
			break;
	}
}

u8   CALLBACK PADstartPoll(int pad)
{
	PAD_LOG("PADstartPoll: %d\n", pad);
	
	curPad = pad - 1;
	curByte = 0;

	return 0xff;
}

u8  _PADpoll(u8 value)
{
	u8 button_check = 0, button_check2 = 0;

	if (curByte == 0)
	{
		curByte++;
		
		PAD_LOG("PADpoll: cmd: %x\n", value);

		curCmd = value;
		switch (value)
		{
			case 0x40: // DUALSHOCK2 ENABLER
				cmdLen = 8;
				buf = cmd40[curPad];
				return 0xf3;

			case 0x41: // QUERY_DS2_ANALOG_MODE
				cmdLen = 8;
				buf = cmd41[curPad];
				return 0xf3;

			case 0x42: // READ_DATA

				_PADupdate(curPad);

				stdpar[curPad][2] = status[curPad] >> 8;
				stdpar[curPad][3] = status[curPad] & 0xff;
				stdpar[curPad][4] = g_ranalog[curPad].x;
				stdpar[curPad][5] = g_ranalog[curPad].y;
				stdpar[curPad][6] = g_lanalog[curPad].x;
				stdpar[curPad][7] = g_lanalog[curPad].y;
			
				if (padMode[curPad] == 1) 
					cmdLen = 20;
				else 
					cmdLen = 4;
			
				button_check2 = stdpar[curPad][2] >> 4;
				switch (stdpar[curPad][3])
				{
					case 0xBF: // X
						stdpar[curPad][14] = (pressure * 255) / 100; //0xff/(100/(100-conf.keys[curPad][16]));
						break;
					case 0xDF: // Circle
						stdpar[curPad][13] = (pressure * 255) / 100; //0xff/(100/(100-conf.keys[curPad][17]));
						break;
					case 0xEF: // Triangle
						stdpar[curPad][12] = (pressure * 255) / 100; //0xff/(100/(100-conf.keys[curPad][19]));
						break;
					case 0x7F: // Square
						stdpar[curPad][15] = (pressure * 255) / 100; //0xff/(100/(100-conf.keys[curPad][18]));
						break;
					case 0xFB: // L1
						stdpar[curPad][16] = (pressure * 255) / 100; //0xff/(100/(100-conf.keys[curPad][26]));
						break;
					case 0xF7: // R1
						stdpar[curPad][17] = (pressure * 255) / 100; //0xff/(100/(100-conf.keys[curPad][28]));
						break;
					case 0xFE: // L2
						stdpar[curPad][18] = (pressure * 255) / 100; //0xff/(100/(100-conf.keys[curPad][27]));
						break;
					case 0xFD: // R2
						stdpar[curPad][19] = (pressure * 255) / 100; //0xff/(100/(100-conf.keys[curPad][29]));
						break;
					default:
						stdpar[curPad][14] = 0x00; // Not pressed
						stdpar[curPad][13] = 0x00; // Not pressed
						stdpar[curPad][12] = 0x00; // Not pressed
						stdpar[curPad][15] = 0x00; // Not pressed
						stdpar[curPad][16] = 0x00; // Not pressed
						stdpar[curPad][17] = 0x00; // Not pressed
						stdpar[curPad][18] = 0x00; // Not pressed
						stdpar[curPad][19] = 0x00; // Not pressed
						break;
				}
				switch (button_check2)
				{
					case 0xE: // UP
						stdpar[curPad][10] = (pressure * 255) / 100; //0xff/(100/(100-conf.keys[curPad][21]));
						break;
					case 0xB: // DOWN
						stdpar[curPad][11] = (pressure * 255) / 100; //0xff/(100/(100-conf.keys[curPad][22]));
						break;
					case 0x7: // LEFT
						stdpar[curPad][9] = (pressure * 255) / 100; //0xff/(100/(100-conf.keys[curPad][23]));
						break;
					case 0xD: // RIGHT
						stdpar[curPad][8] = (pressure * 255) / 100; //0xff/(100/(100-conf.keys[curPad][24]));
						break;
					default:
						stdpar[curPad][8] = 0x00; // Not pressed
						stdpar[curPad][9] = 0x00; // Not pressed
						stdpar[curPad][10] = 0x00; // Not pressed
						stdpar[curPad][11] = 0x00; // Not pressed
						break;
				}
				buf = stdpar[curPad];
				return padID[curPad];

			case 0x43: // CONFIG_MODE
				cmdLen = 8;
				buf = stdcfg[curPad];
				if (stdcfg[curPad][3] == 0xff) return 0xf3;
				else return padID[curPad];

			case 0x44: // SET_MODE_AND_LOCK
				cmdLen = 8;
				buf = stdmode[curPad];
				return 0xf3;

			case 0x45: // QUERY_MODEL_AND_MODE
				cmdLen = 8;
				buf = stdmodel[curPad];
				buf[4] = padMode[curPad];
				return 0xf3;

			case 0x46: // ??
				cmdLen = 8;
				buf = unk46[curPad];
				return 0xf3;

			case 0x47: // ??
				cmdLen = 8;
				buf = unk47[curPad];
				return 0xf3;

			case 0x4c: // QUERY_MODE ??
				cmdLen = 8;
				buf = unk4c[curPad];
				return 0xf3;

			case 0x4d:
				cmdLen = 8;
				buf = unk4d[curPad];
				return 0xf3;

			case 0x4f: // SET_DS2_NATIVE_MODE
				cmdLen = 8;
				padID[curPad] = 0x79; // setting ds2 mode
				ds2mode = 1; // Set DS2 Mode
				buf = cmd4f[curPad];
				return 0xf3;

			default:
#ifdef PAD_LOG
				PAD_LOG("*PADpoll*: unknown cmd %x\n", value);
#endif
				break;
		}
	}

	switch (curCmd)
	{
		case 0x43:
			if (curByte == 2)
			{
				switch (value)
				{
					case 0:
						buf[2] = 0;
						buf[3] = 0;
						break;
					case 1:
						buf[2] = 0xff;
						buf[3] = 0xff;
						break;
				}
			}
			break;

		case 0x44:
			if (curByte == 2)
			{
				PADsetMode(curPad, value);
			}
			break;

		case 0x46:
			if (curByte == 2)
			{
				switch (value)
				{
					case 0: // default
						buf[5] = 0x2;
						buf[6] = 0x0;
						buf[7] = 0xA;
						break;
					case 1: // Param std conf change
						buf[5] = 0x1;
						buf[6] = 0x1;
						buf[7] = 0x14;
						break;
				}
			}
			break;

		case 0x4c:
			if (curByte == 2)
			{
				switch (value)
				{
					case 0: // mode 0 - digital mode
						buf[5] = 0x4;
						break;

					case 1: // mode 1 - analog mode
						buf[5] = 0x7;
						break;
				}
			}
			break;
	}

	if (curByte >= cmdLen) return 0;
	return buf[curByte++];
}

u8 CALLBACK PADpoll(u8 value)
{
	u8 ret;
	
	ret = _PADpoll(value);
	PAD_LOG("PADpoll: %x (%d: %x)\n", value, curByte, ret);
	return ret;
}

// PADkeyEvent is called every vsync (return NULL if no event)
static keyEvent s_event;
keyEvent* CALLBACK PADkeyEvent()
{
	s_event = event;
	event.evt = 0;
	return &s_event;
}
