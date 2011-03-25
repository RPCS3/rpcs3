/*  PadNull
 *  Copyright (C) 2004-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string>
using namespace std;

#include "Pad.h"

const u8 version  = PS2E_PAD_VERSION;
const u8 revision = 0;
const u8 build    = 1;    // increase that with each version

static char *libraryName = "Padnull Driver";
string s_strIniPath="inis";
string s_strLogPath="logs";

FILE *padLog;
Config conf;
keyEvent event;
static keyEvent s_event;

EXPORT_C_(u32) PS2EgetLibType()
{
	return PS2E_LT_PAD;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	return libraryName;
}

EXPORT_C_(u32) PS2EgetLibVersion2(u32 type)
{
	return (version<<16) | (revision<<8) | build;
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

	printf("PadNull: ");
	vprintf(fmt, list);
	va_end(list);
}

EXPORT_C_(void) PADsetSettingsDir(const char* dir)
{
	s_strIniPath = (dir == NULL) ? "inis" : dir;
}

bool OpenLog() {
    bool result = true;
#ifdef PAD_LOG
    if(padLog) return result;

    const std::string LogFile(s_strLogPath + "/padnull.log");

    padLog = fopen(LogFile.c_str(), "w");
    if (padLog != NULL)
        setvbuf(padLog, NULL,  _IONBF, 0);
    else {
        SysMessage("Can't create log file %s\n", LogFile.c_str());
        result = false;
    }
	PAD_LOG("PADinit\n");
#endif
    return result;
}

EXPORT_C_(void) PADsetLogDir(const char* dir)
{
	// Get the path to the log directory.
	s_strLogPath = (dir==NULL) ? "logs" : dir;

	// Reload the log file after updated the path
	if (padLog) {
        fclose(padLog);
        padLog = NULL;
    }
    OpenLog();
}

EXPORT_C_(s32) PADinit(u32 flags)
{
	LoadConfig();

    OpenLog();

	return 0;
}

EXPORT_C_(void) PADshutdown()
{
#ifdef PAD_LOG
	if (padLog)
	{
		fclose(padLog);
		padLog = NULL;
	}
#endif
}

EXPORT_C_(s32) PADopen(void *pDsp)
{
	memset(&event, 0, sizeof(event));

	return _PADOpen(pDsp);
}

EXPORT_C_(void) PADclose()
{
	_PADClose();
}

// PADkeyEvent is called every vsync (return NULL if no event)
EXPORT_C_(keyEvent*) PADkeyEvent()
{

	s_event = event;
	event.evt = 0;

	return &s_event;
}

EXPORT_C_(u8) PADstartPoll(int pad)
{
	return 0;
}

EXPORT_C_(u8) PADpoll(u8 value)
{
	return 0;
}

// call to give a hint to the PAD plugin to query for the keyboard state. A
// good plugin will query the OS for keyboard state ONLY in this function.
// This function is necessary when multithreading because otherwise
// the PAD plugin can get into deadlocks with the thread that really owns
// the window (and input). Note that PADupdate can be called from a different
// thread than the other functions, so mutex or other multithreading primitives
// have to be added to maintain data integrity.
EXPORT_C_(u32) PADquery()
// returns: 1 if supported pad1
//			2 if supported pad2
//			3 if both are supported
{
	return 3;
}

EXPORT_C_(void) PADupdate(int pad)
{
	_PadUpdate(pad);
}

EXPORT_C_(void) PADgsDriverInfo(GSdriverInfo *info)
{
}

EXPORT_C_(s32) PADfreeze(int mode, freezeData *data)
{
	return 0;
}

EXPORT_C_(s32) PADtest()
{
	return 0;
}
