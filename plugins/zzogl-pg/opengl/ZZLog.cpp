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
#include <list>
#include <cstring>
 
extern GSconf conf;

using namespace std;

static list<MESSAGE> listMsgs;
const char* logging_prefix = "ZZOgl-PG";
void ProcessMessages()
{
	FUNCLOG

	if (listMsgs.size() > 0)
	{
		int left = 25, top = 15;
		list<MESSAGE>::iterator it = listMsgs.begin();

		while (it != listMsgs.end())
		{
			DrawText(it->str, left + 1, top + 1, 0xff000000);
			DrawText(it->str, left, top, 0xffffff30);
			top += 15;

			if ((int)(it->dwTimeStamp - timeGetTime()) < 0)
				it = listMsgs.erase(it);
			else ++it;
		}
	}
}

void ZZAddMessage(const char* pstr, u32 ms)
{
	FUNCLOG
	listMsgs.push_back(MESSAGE(pstr, timeGetTime() + ms));
	ZZLog::Log("%s\n", pstr);
}

namespace ZZLog
{
std::string s_strLogPath("logs");
FILE *gsLog;
FILE *gsLogGL; // I create a separate file because it could be very verbose

bool IsLogging()
{
	// gsLog can be null if the config dialog is used prior to Pcsx2 starting an emulation session.
	// (GSinit won't have been called then)
	return (gsLog != NULL && conf.log);
}

void Open() 
{
    const std::string LogFile(s_strLogPath + "/GSzzogl.log");
    const std::string LogFileGL(s_strLogPath + "/GSzzogl_GL.log");

    gsLog = fopen(LogFile.c_str(), "w");
    if (gsLog != NULL)
        setvbuf(gsLog, NULL,  _IONBF, 0);
    else 
        SysMessage("Can't create log file %s\n", LogFile.c_str());

    gsLogGL = fopen(LogFileGL.c_str(), "w");
    if (gsLogGL != NULL)
        setvbuf(gsLogGL, NULL,  _IONBF, 0);
    else
        SysMessage("Can't create log file %s\n", LogFileGL.c_str());


}

void Close()
{
	if (gsLog != NULL) {
        fclose(gsLog);
        gsLog = NULL;
    }
	if (gsLogGL != NULL) {
        fclose(gsLogGL);
        gsLogGL = NULL;
    }
}

void SetDir(const char* dir)
{
	// Get the path to the log directory.
	s_strLogPath = (dir==NULL) ? "logs" : dir;

	// Reload previously open log file
    if (gsLog) {
        Close();
        Open();
    }
}

void WriteToScreen(const char* pstr, u32 ms)
{
	ZZAddMessage(pstr, ms);
}

void WriteToScreen2(const char* fmt, ...)
{
	va_list list;
	char tmp[512];

	va_start(list, fmt);
	vsprintf(tmp, fmt, list);
	va_end(list);

	ZZAddMessage(tmp, 5000);
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
	fprintf(stderr,"%s:  ", logging_prefix);
	fprintf(stderr,"%s", str);
}

void _Print(const char *str)
{
	fprintf(stderr,"%s:  ", logging_prefix);
	fprintf(stderr,"%s", str);

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

	fprintf(stderr, "%s:  ", logging_prefix);
	vfprintf(stderr, fmt, list);
	va_end(list);
}

void Print(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	if (IsLogging()) vfprintf(gsLog, fmt, list);
	
	fprintf(stderr, "%s:  ", logging_prefix);
	vfprintf(stderr, fmt, list);

	va_end(list);
}


void WriteLn(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	if (IsLogging()) vfprintf(gsLog, fmt, list);
	
	fprintf(stderr, "%s:  ", logging_prefix);
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

		fprintf(stderr, "%s(PRIM):  ", logging_prefix);
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
	
	fprintf(stderr, "%s:  ", logging_prefix);
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

	fprintf(stderr, "%s(Warning):  ", logging_prefix);
	vfprintf(stderr, fmt, list);
	fprintf(stderr, "\n");
	
	va_end(list);
#endif
}

void Dev_Log(const char *fmt, ...)
{
#ifdef ZEROGS_DEVBUILD
	va_list list;

	va_start(list, fmt);

	if (IsLogging())
	{
		vfprintf(gsLog, fmt, list);
		fprintf(gsLog, "\n");
	}

	fprintf(stderr, "%s:  ", logging_prefix);
	vfprintf(stderr, fmt, list);
	fprintf(stderr, "\n");
	
	va_end(list);
#endif
}

void Debug_Log(const char *fmt, ...)
{
#ifdef _DEBUG
	va_list list;

	va_start(list, fmt);

	if (IsLogging())
	{
		vfprintf(gsLog, fmt, list);
		fprintf(gsLog, "\n");
	}

	fprintf(stderr, "%s:  ", logging_prefix);
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

	fprintf(stderr, "%s:  ", logging_prefix);
	vfprintf(stderr, fmt, list);
	fprintf(stderr, "\n");
	
	va_end(list);
}

#define LOUD_DEBUGGING

#ifdef OGL4_LOG
void Check_GL_Error()
{
       unsigned int count = 64; // max. num. of messages that will be read from the log
       int bufsize = 2048;
       unsigned int* sources      = new unsigned int[count];
       unsigned int* types        = new unsigned int[count];
       unsigned int* ids   = new unsigned int[count];
       unsigned int* severities = new unsigned int[count];
       int* lengths = new int[count];
       char* messageLog = new char[bufsize];
       unsigned int retVal = glGetDebugMessageLogARB(count, bufsize, sources, types, ids, severities, lengths, messageLog);

       if(retVal > 0)
       {
             unsigned int pos = 0;
             for(unsigned int i=0; i<retVal; i++)
             {
                    GL_Error_Log(sources[i], types[i], ids[i], severities[i],
 &messageLog[pos]);
                    pos += lengths[i];
              }
       }

       delete [] sources;
       delete [] types;
       delete [] ids;
       delete [] severities;
       delete [] lengths;
       delete [] messageLog;
}

void GL_Error_Log(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, const char* message)
{
	char debType[20], debSev[5];
	static int sev_counter = 0;

	if(type == GL_DEBUG_TYPE_ERROR_ARB)
		strcpy(debType, "Error");
	else if(type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB)
		strcpy(debType, "Deprecated behavior");
	else if(type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB)
		strcpy(debType, "Undefined behavior");
	else if(type == GL_DEBUG_TYPE_PORTABILITY_ARB)
		strcpy(debType, "Portability");
	else if(type == GL_DEBUG_TYPE_PERFORMANCE_ARB)
		strcpy(debType, "Performance");
	else if(type == GL_DEBUG_TYPE_OTHER_ARB)
		strcpy(debType, "Other");
	else
		strcpy(debType, "UNKNOWN");

	if(severity == GL_DEBUG_SEVERITY_HIGH_ARB) {
		strcpy(debSev, "High");
		sev_counter++;
	}
	else if(severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
		strcpy(debSev, "Med");
	else if(severity == GL_DEBUG_SEVERITY_LOW_ARB)
		strcpy(debSev, "Low");

	#ifdef LOUD_DEBUGGING
	fprintf(stderr,"Type:%s\tSeverity:%s\tMessage:%s\n", debType, debSev,message);
	#endif

	if(gsLogGL)
	{
		fprintf(gsLogGL,"Type:%s\tSeverity:%s\tMessage:%s\n", debType, debSev,message);
	}
	//if (sev_counter > 2) assert(0);
}
#else
void Check_GL_Error() {}
void GL_Error_Log(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, const char* message) {}
#endif

};
