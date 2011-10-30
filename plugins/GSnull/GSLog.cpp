/*  GSnull
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#include <stdio.h>
#include <assert.h>

using namespace std;

#include "GS.h"
#include "GifTransfer.h"
#include "null/GSnull.h"

std::string s_strLogPath("logs");

namespace GSLog
{
	FILE *gsLog;
	
	bool Open()
	{
		bool result = true;
		
		#ifdef GS_LOG
		const std::string LogFile(s_strLogPath + "/GSnull.log");

		gsLog = fopen(LogFile.c_str(), "w");
		
		if (gsLog != NULL)
		{
			setvbuf(gsLog, NULL,  _IONBF, 0);
		}
		else 
		{
			Message("Can't create log file %s.", LogFile.c_str());
			result = false;
		}
		
		WriteLn("GSnull plugin version %d,%d",revision,build);
		WriteLn("GS init.");
		#endif
		
		return result;
	}
	
	void Close()
	{
		#ifdef GS_LOG
		if (gsLog) 
		{
			fclose(gsLog);
			gsLog = NULL;
		}
		#endif
	}
	
	void Log(char *fmt, ...)
	{
		va_list list;

		if (!conf.Log || gsLog == NULL) return;

		va_start(list, fmt);
		vfprintf(gsLog, fmt, list);
		va_end(list);
	}
	
	void Message(char *fmt, ...)
	{
		va_list list;
		char msg[512];

		va_start(list, fmt);
		vsprintf(msg, fmt, list);
		va_end(list);
		
		SysMessage("%s\n",msg);
	}
	
	void Print(const char *fmt, ...)
	{
		va_list list;
		char msg[512];

		va_start(list, fmt);
		vsprintf(msg, fmt, list);
		va_end(list);

		GS_LOG(msg);
		fprintf(stderr, "GSnull:%s", msg);
	}
	
	
	void WriteLn(const char *fmt, ...)
	{
		va_list list;
		char msg[512];

		va_start(list, fmt);
		vsprintf(msg, fmt, list);
		va_end(list);

		GS_LOG(msg);
		fprintf(stderr, "GSnull:%s\n", msg);
	}
};
