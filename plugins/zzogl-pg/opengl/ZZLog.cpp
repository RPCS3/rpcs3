/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
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
 
#include <stdio.h>
#include "ZZLog.h"
 
extern GSconf conf;

namespace ZZLog
{
std::string s_strLogPath("logs/");
FILE *gsLog;

bool IsLogging()
{
	// gsLog can be null if the config dialog is used prior to Pcsx2 starting an emulation session.
	// (GSinit won't have been called then)
	return (gsLog != NULL && conf.log);
}

void Open() 
{
    const std::string LogFile(s_strLogPath + "GSzzogl.log");

    gsLog = fopen(LogFile.c_str(), "w");
    if (gsLog != NULL)
        setvbuf(gsLog, NULL,  _IONBF, 0);
    else 
        SysMessage("Can't create log file %s\n", LogFile.c_str());

}

void Close()
{
	if (gsLog != NULL) {
        fclose(gsLog);
        gsLog = NULL;
    }
}

void SetDir(const char* dir)
{
	// Get the path to the log directory.
	s_strLogPath = (dir==NULL) ? "logs/" : dir;

	// Reload previously open log file
    if (gsLog) {
        Close();
        Open();
    }
}

void WriteToScreen(const char* pstr, u32 ms)
{
	ZeroGS::AddMessage(pstr, ms);
}

void _Message(const char *str)
{
	SysMessage(str);
}

void _Log(const char *str)
{
	if (IsLogging()) fprintf(gsLog, str);
}

void _WriteToConsole(const char *str)
{
	fprintf(stderr,"ZZogl-PG: %s", str);
}

void _Print(const char *str)
{
	fprintf(stderr,"ZZogl-PG: %s", str);

	if (IsLogging()) fprintf(gsLog, str);
}

void Message(const char *fmt, ...)
{
	va_list list;
	char tmp[512];

	va_start(list, fmt);
	vsprintf(tmp, fmt, list);
	va_end(list);

	SysMessage(tmp);
}

void Log(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	if (IsLogging()) vfprintf(gsLog, fmt, list);

	va_end(list);
}

void WriteToConsole(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	fprintf(stderr, "ZZogl-PG: ");
	vfprintf(stderr, fmt, list);
	va_end(list);
}

void Print(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	if (IsLogging()) vfprintf(gsLog, fmt, list);
	
	fprintf(stderr, "ZZogl-PG: ");
	vfprintf(stderr, fmt, list);

	va_end(list);
}


void WriteLn(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	if (IsLogging()) vfprintf(gsLog, fmt, list);
	
	fprintf(stderr, "ZZogl-PG: ");
	vfprintf(stderr, fmt, list);
	va_end(list);
	fprintf(stderr,"\n");
}

void Greg_Log(const char *fmt, ...)
{
#if defined(WRITE_GREG_LOGS)
	va_list list;
	char tmp[512];

	va_start(list, fmt);

	if (IsLogging()) {
        fprintf(gsLog, "GRegs: ");
        vfprintf(gsLog, fmt, list);
    }
	//fprintf(stderr,"GRegs: ");
	//vfprintf(stderr, fmt, list);

	va_end(list);

	if (IsLogging()) fprintf(gsLog, "\n");
	//fprintf(stderr,"\n");
#endif
}

void Prim_Log(const char *fmt, ...)
{
#if defined(ZEROGS_DEVBUILD) && defined(WRITE_PRIM_LOGS)
	va_list list;
	char tmp[512];

	va_start(list, fmt);

	if (conf.log /*& 0x00000010*/)
	{
		if (IsLogging()) vfprintf(gsLog, fmt, list);

		fprintf(stderr, "ZZogl-PG(PRIM): ");
		vfprintf(stderr, fmt, list);

		vprintf(fmt, list);
	}

	va_end(list);
	fprintf(stderr,"\n");

#endif
}

void GS_Log(const char *fmt, ...)
{
#ifdef ZEROGS_DEVBUILD
	va_list list;

	va_start(list, fmt);

	if (IsLogging())
	{
		vfprintf(gsLog, fmt, list);
		fprintf(gsLog, "\n");
	}
	
	fprintf(stderr, "ZZogl-PG: ");
	vfprintf(stderr, fmt, list);
	fprintf(stderr, "\n");
	
	va_end(list);
#endif
}

void Warn_Log(const char *fmt, ...)
{
#ifdef ZEROGS_DEVBUILD
	va_list list;

	va_start(list, fmt);

	if (IsLogging())
	{
		vfprintf(gsLog, fmt, list);
		fprintf(gsLog, "\n");
	}

	fprintf(stderr, "ZZogl-PG:  ");
	vfprintf(stderr, fmt, list);
	fprintf(stderr, "\n");
	
	va_end(list);
#endif
}

void Debug_Log(const char *fmt, ...)
{
#if _DEBUG
	va_list list;

	va_start(list, fmt);

	if (IsLogging())
	{
		vfprintf(gsLog, fmt, list);
		fprintf(gsLog, "\n");
	}

	fprintf(stderr, "ZZogl-PG:  ");
	vfprintf(stderr, fmt, list);
	fprintf(stderr, "\n");
	
	va_end(list);
#endif
}

void Error_Log(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	if (IsLogging())
	{
		vfprintf(gsLog, fmt, list);
		fprintf(gsLog, "\n");
	}

	fprintf(stderr, "ZZogl-PG:  ");
	vfprintf(stderr, fmt, list);
	fprintf(stderr, "\n");
	
	va_end(list);
}
};
