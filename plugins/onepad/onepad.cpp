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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>

#include "onepad.h"

#ifndef _WIN32

#include <unistd.h>
#else
#include "svnrev.h"
#endif

PADconf* conf;
char libraryName[256];

keyEvent event;

u16 status[2];
int status_pressure[2][MAX_KEYS];
static keyEvent s_event;
std::string s_strIniPath("inis/");
std::string s_strLogPath("logs/");
bool toggleAutoRepeat = true;

const u32 version  = PS2E_PAD_VERSION;
const u32 revision = 1;
const u32 build    = 1;    // increase that with each version

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
int curPad, curByte, curCmd, cmdLen;
int ds2mode = 0; // DS Mode at start
FILE *padLog = NULL;

pthread_spinlock_t s_mutexStatus;
pthread_mutex_t	   mutex_KeyEvent;
static u32 s_keyPress[2], s_keyRelease[2];

queue<keyEvent> ev_fifo;

static int padVib0[2];
static int padVib1[2];
static int padVibC[2];
static int padVibF[2][4];

static void InitLibraryName()
{
#ifdef _WIN32
#	ifdef PUBLIC

	// Public Release!
	// Output a simplified string that's just our name:

	strcpy(libraryName, "OnePAD");

#	elif defined( SVN_REV_UNKNOWN )

	// Unknown revision.
	// Output a name that includes devbuild status but not
	// subversion revision tags:

	strcpy(libraryName, "OnePAD"
#		ifdef PCSX2_DEBUG
	       "-Debug"
#		endif
	      );
#	else

	// Use TortoiseSVN's SubWCRev utility's output
	// to label the specific revision:

	sprintf_s(libraryName, "OnePAD r%d%s"
#		ifdef PCSX2_DEBUG
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

	strcpy(libraryName, "OnePAD"
#	ifdef PCSX2_DEBUG
	       "-Debug"
#	endif
	      );
#endif
}

EXPORT_C_(u32) PS2EgetLibType()
{
	return PS2E_LT_PAD;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	InitLibraryName();
	return libraryName;
}

EXPORT_C_(u32) PS2EgetLibVersion2(u32 type)
{
	return (version << 16) | (revision << 8) | build;
}

void __Log(const char *fmt, ...)
{
	va_list list;

	if (padLog == NULL) return;
	va_start(list, fmt);
	vfprintf(padLog, fmt, list);
	va_end(list);
}

void __LogToConsole(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	if (padLog != NULL) vfprintf(padLog, fmt, list);

	printf("OnePAD: ");
	vprintf(fmt, list);
	va_end(list);
}

void initLogging()
{
#ifdef PAD_LOG
	if (padLog) return;

    const std::string LogFile(s_strLogPath + "padLog.txt");
    padLog = fopen(LogFile.c_str(), "w");

    if (padLog)
        setvbuf(padLog, NULL,  _IONBF, 0);

	PAD_LOG("PADinit\n");
#endif
}

void CloseLogging()
{
#ifdef PAD_LOG
	if (padLog)
	{
		fclose(padLog);
		padLog = NULL;
	}
#endif
}

void clearPAD(int pad)
{
	conf->keysym_map[pad].clear();
	for (int key= 0; key < MAX_KEYS; ++key)
		set_key(pad, key, 0);
}

EXPORT_C_(s32) PADinit(u32 flags)
{
	initLogging();

	pads |= flags;
	for (int i = 0 ; i < 2 ; i++) {
		status[i] = 0xffff;
		for (int j = 0 ; j < MAX_KEYS ; j++)
			status_pressure[i][j] = 255;
	}

	LoadConfig();

	PADsetMode(0, 0);
	PADsetMode(1, 0);

	Analog::Init();

	return 0;
}

EXPORT_C_(void) PADshutdown()
{
    CloseLogging();
	if (conf) delete conf;
}

EXPORT_C_(s32) PADopen(void *pDsp)
{
	memset(&event, 0, sizeof(event));

	while (!ev_fifo.empty()) ev_fifo.pop();
	pthread_spin_init(&s_mutexStatus, PTHREAD_PROCESS_PRIVATE);
	pthread_mutex_init(&mutex_KeyEvent, NULL);
	s_keyPress[0] = s_keyPress[1] = 0;
	s_keyRelease[0] = s_keyRelease[1] = 0;

#ifdef __LINUX__
	JoystickInfo::EnumerateJoysticks(s_vjoysticks);
#endif
	return _PADopen(pDsp);
}

EXPORT_C_(void) PADsetSettingsDir(const char* dir)
{
	// Get the path to the ini directory.
    s_strIniPath = (dir==NULL) ? "inis/" : dir;
}

EXPORT_C_(void) PADsetLogDir(const char* dir)
{
	// Get the path to the log directory.
	s_strLogPath = (dir==NULL) ? "logs/" : dir;

	// Reload the log file after updated the path
    CloseLogging();
    initLogging();
}

EXPORT_C_(void) PADclose()
{
	while (!ev_fifo.empty()) ev_fifo.pop();
	pthread_spin_destroy(&s_mutexStatus);
	pthread_mutex_destroy(&mutex_KeyEvent);
	_PADclose();
}

void _PADupdate(int pad)
{
	pthread_spin_lock(&s_mutexStatus);
	status[pad] |= s_keyRelease[pad];
	status[pad] &= ~s_keyPress[pad];
	s_keyRelease[pad] = 0;
	s_keyPress[pad] = 0;
	pthread_spin_unlock(&s_mutexStatus);
}

void UpdateKeys(int pad, int keyPress, int keyRelease)
{
	pthread_spin_lock(&s_mutexStatus);
	s_keyPress[pad] |= keyPress;
	s_keyPress[pad] &= ~keyRelease;
	s_keyRelease[pad] |= keyRelease;
	s_keyRelease[pad] &= ~keyPress;
	pthread_spin_unlock(&s_mutexStatus);
}

EXPORT_C_(u32) PADquery()
{
	return 3; // both
}

void PADsetMode(int pad, int mode)
{
	padMode[pad] = mode;
	// FIXME FEEDBACK
	padVib0[pad] = 0;
	padVib1[pad] = 0;
	padVibF[pad][0] = 0;
	padVibF[pad][1] = 0;
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

EXPORT_C_(u8) PADstartPoll(int pad)
{
	//PAD_LOG("PADstartPoll: %d\n", pad);

	curPad = pad - 1;
	curByte = 0;

	return 0xff;
}

u8  _PADpoll(u8 value)
{
	u8 button_check = 0;
	int vib_small;
	int vib_big;

	if (curByte == 0)
	{
		curByte++;

		//PAD_LOG("PADpoll: cmd: %x\n", value);

		curCmd = value;
		switch (value)
		{
			case CMD_SET_VREF_PARAM: // DUALSHOCK2 ENABLER
				cmdLen = 8;
				buf = cmd40[curPad];
				return 0xf3;

			case CMD_QUERY_DS2_ANALOG_MODE: // QUERY_DS2_ANALOG_MODE
				cmdLen = 8;
				buf = cmd41[curPad];
				return 0xf3;

			case CMD_READ_DATA_AND_VIBRATE: // READ_DATA

				_PADupdate(curPad);

				stdpar[curPad][2] = status[curPad] >> 8;
				stdpar[curPad][3] = status[curPad] & 0xff;
				stdpar[curPad][4] = Analog::Pad(curPad, PAD_R_RIGHT);
				stdpar[curPad][5] = Analog::Pad(curPad, PAD_R_UP);
				stdpar[curPad][6] = Analog::Pad(curPad, PAD_L_RIGHT);
				stdpar[curPad][7] = Analog::Pad(curPad, PAD_L_UP);

				if (padMode[curPad] == 1)
					cmdLen = 20;
				else
					cmdLen = 4;

				button_check = stdpar[curPad][2] >> 4;
				switch (stdpar[curPad][3])
				{
					case 0xBF: // X
						stdpar[curPad][14] = status_pressure[curPad][PAD_CROSS];
						break;

					case 0xDF: // Circle
						stdpar[curPad][13] = status_pressure[curPad][PAD_CIRCLE];
						break;

					case 0xEF: // Triangle
						stdpar[curPad][12] = status_pressure[curPad][PAD_TRIANGLE];
						break;

					case 0x7F: // Square
						stdpar[curPad][15] = status_pressure[curPad][PAD_SQUARE];
						break;

					case 0xFB: // L1
						stdpar[curPad][16] = status_pressure[curPad][PAD_L1];
						break;

					case 0xF7: // R1
						stdpar[curPad][17] = status_pressure[curPad][PAD_R1];
						break;

					case 0xFE: // L2
						stdpar[curPad][18] = status_pressure[curPad][PAD_L2];
						break;

					case 0xFD: // R2
						stdpar[curPad][19] = status_pressure[curPad][PAD_R2];
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
				switch (button_check)
				{
					case 0xE: // UP
						stdpar[curPad][10] = status_pressure[curPad][PAD_UP];
						break;

					case 0xB: // DOWN
						stdpar[curPad][11] =status_pressure[curPad][PAD_DOWN];
						break;

					case 0x7: // LEFT
						stdpar[curPad][9] = status_pressure[curPad][PAD_LEFT];
						break;

					case 0xD: // RIGHT
						stdpar[curPad][8] = status_pressure[curPad][PAD_RIGHT];
						break;

					default:
						stdpar[curPad][8] = 0x00; // Not pressed
						stdpar[curPad][9] = 0x00; // Not pressed
						stdpar[curPad][10] = 0x00; // Not pressed
						stdpar[curPad][11] = 0x00; // Not pressed
						break;
				}
				buf = stdpar[curPad];

				// FIXME FEEDBACK. Set effect here
				/* Small Motor */
				vib_small = padVibF[curPad][0] ? 2000 : 0;
				// if ((padVibF[curPad][2] != vib_small) && (padVibC[curPad] >= 0))
				if (padVibF[curPad][2] != vib_small)
				{
					padVibF[curPad][2] = vib_small;
					// SetDeviceForceS (padVibC[curPad], vib_small);
					JoystickInfo::DoHapticEffect(0, curPad, vib_small);
				}

				/* Big Motor */
				vib_big = padVibF[curPad][1] ? 500 + 37*padVibF[curPad][1] : 0;
				// if ((padVibF[curPad][3] != vib_big) && (padVibC[curPad] >= 0))
				if (padVibF[curPad][3] != vib_big)
				{
					padVibF[curPad][3] = vib_big;
					// SetDeviceForceB (padVibC[curPad], vib_big);
					JoystickInfo::DoHapticEffect(1, curPad, vib_big);
				}

				return padID[curPad];

			case CMD_CONFIG_MODE: // CONFIG_MODE
				cmdLen = 8;
				buf = stdcfg[curPad];
				if (stdcfg[curPad][3] == 0xff)
					return 0xf3;
				else
					return padID[curPad];

			case CMD_SET_MODE_AND_LOCK: // SET_MODE_AND_LOCK
				cmdLen = 8;
				buf = stdmode[curPad];
				return 0xf3;

			case CMD_QUERY_MODEL_AND_MODE: // QUERY_MODEL_AND_MODE
				cmdLen = 8;
				buf = stdmodel[curPad];
				buf[4] = padMode[curPad];
				return 0xf3;

			case CMD_QUERY_ACT: // ??
				cmdLen = 8;
				buf = unk46[curPad];
				return 0xf3;

			case CMD_QUERY_COMB: // ??
				cmdLen = 8;
				buf = unk47[curPad];
				return 0xf3;

			case CMD_QUERY_MODE: // QUERY_MODE ??
				cmdLen = 8;
				buf = unk4c[curPad];
				return 0xf3;

			case CMD_VIBRATION_TOGGLE:
				cmdLen = 8;
				buf = unk4d[curPad];
				return 0xf3;

			case CMD_SET_DS2_NATIVE_MODE: // SET_DS2_NATIVE_MODE
				cmdLen = 8;
				padID[curPad] = 0x79; // setting ds2 mode
				ds2mode = 1; // Set DS2 Mode
				buf = cmd4f[curPad];
				return 0xf3;

			default:
				PAD_LOG("*PADpoll*: unknown cmd %x\n", value);
				break;
		}
	}

	switch (curCmd)
	{
		case CMD_READ_DATA_AND_VIBRATE:
			// FIXME FEEDBACK
			if (curByte == padVib0[curPad])
				padVibF[curPad][0] = value;
			if (curByte == padVib1[curPad])
				padVibF[curPad][1] = value;
			break;
		case CMD_CONFIG_MODE:
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

		case CMD_SET_MODE_AND_LOCK:
			if (curByte == 2)
			{
				PADsetMode(curPad, value);
			}
			break;

		case CMD_QUERY_ACT:
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

		case CMD_QUERY_MODE:
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

		case CMD_VIBRATION_TOGGLE:
			// FIXME FEEDBACK
			if (curByte >= 2)
			{
				if (curByte == padVib0[curPad])
					buf[curByte] = 0x00;
				if (curByte == padVib1[curPad])
					buf[curByte] = 0x01;
				if (value == 0x00)
				{
					padVib0[curPad] = curByte;
					// FIXME: code from SSSXPAD I'm not sure we need this part
					// if ((padID[curPad] & 0x0f) < (curByte - 1) / 2)
					// 	padID[curPad] = (padID[curPad] & 0xf0) + (curByte - 1) / 2;
				}
				else if (value == 0x01)
				{
					padVib1[curPad] = curByte;
					// FIXME: code from SSSXPAD I'm not sure we need this part
					// if ((padID[curPad] & 0x0f) < (curByte - 1) / 2)
					// 	padID[curPad] = (padID[curPad] & 0xf0) + (curByte - 1) / 2;
				}
			}
			break;
	}

	if (curByte >= cmdLen) return 0;
	return buf[curByte++];
}

EXPORT_C_(u8) PADpoll(u8 value)
{
	u8 ret;

	ret = _PADpoll(value);
	//PAD_LOG("PADpoll: %x (%d: %x)\n", value, curByte, ret);
	return ret;
}

// PADkeyEvent is called every vsync (return NULL if no event)
EXPORT_C_(keyEvent*) PADkeyEvent()
{
	s_event = event;
	event.evt = 0;
	event.key = 0;
	return &s_event;
}

#ifdef __LINUX__
EXPORT_C_(void) PADWriteEvent(keyEvent &evt)
{
	pthread_mutex_lock(&mutex_KeyEvent);
	ev_fifo.push(evt);
	pthread_mutex_unlock(&mutex_KeyEvent);
}
#endif
