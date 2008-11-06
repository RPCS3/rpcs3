/*  logfile.c
 *  Copyright (C) 2002-2005  PCSX2 Team
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
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */


#include <windows.h>

// #include <fcntl.h> // open()
// #include <io.h> // mkdir()
#include <stddef.h> // NULL
#include <stdio.h> // vsprintf()
#include <stdarg.h> // va_start(), va_end(), vsprintf()
// #include <sys/stat.h> // open()
// #include <sys/types.h> // open()

#include "logfile.h"


HANDLE logfile;
char logfiletemp[2048];


void InitLog() {
  // Token comment line
#ifdef VERBOSE_LOGFILE
  CreateDirectory("logs", NULL);

  DeleteFile("logs\\CDVDlog.txt");
  logfile = NULL;
#endif /* VERBOSE LOGFILE */
} // END InitLog();


int OpenLog() {
  // Token comment line
#ifdef VERBOSE_LOGFILE
  logfile = CreateFile("logs\\CDVDlog.txt",
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_FLAG_SEQUENTIAL_SCAN,
                       NULL);
  if(logfile == INVALID_HANDLE_VALUE) {
    logfile = NULL;
    return(-1);
  } // ENDIF- Failed to open? Say so.
#endif /* VERBOSE LOGFILE */

  return(0);
} // END OpenLog();


void CloseLog() {
  // Token comment line
#ifdef VERBOSE_LOGFILE
  if(logfile != NULL) {
    CloseHandle(logfile);
    logfile = NULL;
  } // ENDIF- Is the log file actually open? Close it.
#endif /* VERBOSE LOGFILE */
} // END CloseLog()


void PrintLog(const char *fmt, ...) {
  DWORD byteswritten;

  // Token comment line
#ifdef VERBOSE_LOGFILE
  va_list list;
  int len;

  if(logfile == NULL)  return; // Log file not open... yet.

  va_start(list, fmt);
  vsprintf(logfiletemp, fmt, list);
  va_end(list);

  len = 0;
  while((len < 2048) && (logfiletemp[len] != 0))  len++;
  if((len > 0) && (logfiletemp[len-1] == '\n'))  len--;
  if((len > 0) && (logfiletemp[len-1] == '\r'))  len--;
  logfiletemp[len] = 0; // Slice off the last "\r\n"...

  WriteFile(logfile, logfiletemp, len, &byteswritten, NULL);
  WriteFile(logfile, "\r\n", 2, &byteswritten, NULL);
#endif /* VERBOSE LOGFILE */
} // END PrintLog()

void PrintError(const char *header, DWORD errcode) {
#ifdef VERBOSE_LOGFILE
  TCHAR errmsg[256];

  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | 80,
                NULL,
                errcode,
                0,
                errmsg,
                256,
                NULL);
  PrintLog("%s:     (%u) %s", header, errcode, errmsg);
#endif /* VERBOSE_WARNING_DEVICE */
} // END PrintError()
