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





#include <fcntl.h> // open

#include <stdio.h> // vsprintf()

#include <stdarg.h> // va_start(), va_end(), vsprintf()

#include <sys/stat.h> // mkdir(), open()

#include <sys/types.h> // mkdir(), open()

#include <unistd.h> // close(), write(), unlink()



#include "logfile.h"





int logfile;

char logfiletemp[2048];





void InitLog() {

  // Token comment line

#ifdef VERBOSE_LOGFILE

  mkdir("./logs", 0755);



  unlink("./logs/CDVDlog.txt");

#endif /* VERBOSE LOGFILE */

} // END InitLog();





int OpenLog() {

  // Token comment line

#ifdef VERBOSE_LOGFILE

  logfile = -1;

  logfile = open("./logs/CDVDlog.txt", O_WRONLY | O_CREAT | O_APPEND, 0755);

  if(logfile == -1)  return(-1);

#endif /* VERBOSE LOGFILE */



  return(0);

} // END OpenLog();





void CloseLog() {

  // Token comment line

#ifdef VERBOSE_LOGFILE

  if(logfile != -1) {

    close(logfile);

    logfile = -1;

  } // ENDIF- Is the log file actually open? Close it.

#endif /* VERBOSE LOGFILE */

} // END CloseLog()





void PrintLog(const char *fmt, ...) {

  // Token comment line

#ifdef VERBOSE_LOGFILE

  va_list list;

  int len;



  if(logfile == -1)  return; // Log file not open.



  va_start(list, fmt);

  vsprintf(logfiletemp, fmt, list);

  va_end(list);



  len = 0;

  while((len < 2048) && (logfiletemp[len] != 0))  len++;

  if((len > 0) && (logfiletemp[len-1] == '\n'))  len--;

  if((len > 0) && (logfiletemp[len-1] == '\r'))  len--;

  logfiletemp[len] = 0; // Slice off the last "\r\n"...



  write(logfile, logfiletemp, len);

  write(logfile, "\r\n", 2); // ... and write out your own.

#endif /* VERBOSE LOGFILE */

} // END PrintLog()

