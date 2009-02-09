/*  CDVDlinuz.c
 *  Copyright (C) 2002-2005  CDVDlinuz Team
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

#include <errno.h> // errno
#include <fcntl.h> // open()
#include <stddef.h> // NULL
#include <stdio.h> // printf()
#include <stdlib.h> // getenv(), system()
#include <string.h> // strerror(), sprintf()
#include <sys/ioctl.h> // ioctl()
#include <sys/stat.h> // stat()
#include <sys/types.h> // stat()
#include <time.h> // time_t, time(), struct timeval
#include <unistd.h> // stat()

#ifndef __LINUX__
#ifdef __linux__
#define __LINUX__
#endif /* __linux__ */
#endif /* No __LINUX__ */

#define CDVDdefs
#include "PS2Edefs.h"
// #include "PS2Etypes.h"

#include "CDVDlinuz.h"

#include "buffer.h"
#include "conf.h"
#include "logfile.h"
#include "CD.h" // InitCDInfo()
#include "DVD.h" // InitDVDInfo()
#include "device.h"

#include "../version.h"


// Globals

time_t lasttime;


// Interface Functions

u32 CALLBACK PS2EgetLibType() {
  return(PS2E_LT_CDVD); // Library Type CDVD
} // END PS2EgetLibType()


u32 CALLBACK PS2EgetLibVersion2(u32 type) {
  return((version<<16)|(revision<<8)|build);
} // END PS2EgetLibVersion2()


char* CALLBACK PS2EgetLibName() {
  return(libname);
} // END PS2EgetLibName()


s32 CALLBACK CDVDinit() {
  errno = 0;

  InitLog();
  if(OpenLog() != 0)  return(-1);

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD interface: CDVDinit()");
#endif /* VERBOSE_FUNCTION */

  InitConf();

  devicehandle = -1;
  devicecapability = 0;
  lasttime = time(NULL);

  // Initialize DVD.c and CD.c as well
  InitDisc();
  InitDVDInfo();
  InitCDInfo();

  return(0);
} // END CDVDinit()


void CALLBACK CDVDshutdown() {
#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD interface: CDVDshutdown()");
#endif /* VERBOSE_FUNCTION */

  DeviceClose();
  CloseLog();
} // END CDVDshutdown()


s32 CALLBACK CDVDopen(const char* pTitleFilename) {
  s32 s32result;

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD interface: CDVDopen()");
#endif /* VERBOSE_FUNCTION */

  InitBuffer();

  LoadConf();

  errno = 0;
  s32result = DeviceOpen();
  if(s32result != 0)  return(s32result);
  if(errno != 0)  return(-1);

  return(0);
} // END CDVDopen();


void CALLBACK CDVDclose() {
#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD interface: CDVDclose()");
#endif /* VERBOSE_FUNCTION */

  DeviceClose();
} // END CDVDclose()


s32 CALLBACK CDVDreadTrack(u32 lsn, int mode) {
  s32 s32result;

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD interface: CDVDreadTrack(%i)", lsn);
#endif /* VERBOSE_FUNCTION */

  s32result = 0;
  errno = 0;

  if(DiscInserted() == -1)  return(-1);

  if(userbuffer < BUFFERMAX) {
    if((bufferlist[userbuffer].lsn == lsn) &&
       (bufferlist[userbuffer].mode == mode)) {
      return(0);
    } // ENDIF- And it's the right one?
  } // ENDIF- Are we already pointing at the buffer?

  userbuffer = FindListBuffer(lsn);
  if(userbuffer < BUFFERMAX) {
    if((bufferlist[userbuffer].lsn == lsn) &&
       (bufferlist[userbuffer].mode == mode)) {
      return(0);
    } // ENDIF- And it was the right one?
  } // ENDIF- Was a buffer found in the cache?

  replacebuffer++;
  if(replacebuffer >= BUFFERMAX)  replacebuffer = 0;
  userbuffer = replacebuffer;

  if(bufferlist[replacebuffer].upsort != 0xffff) {
    RemoveListBuffer(replacebuffer);
  } // ENDIF- Reference already in place? Remove it.

  s32result = DeviceReadTrack(lsn, mode, bufferlist[replacebuffer].buffer);
  bufferlist[replacebuffer].lsn = lsn;
  bufferlist[replacebuffer].mode = mode;
  bufferlist[replacebuffer].offset = DeviceBufferOffset();

  if((s32result != 0) || (errno != 0)) {
    bufferlist[replacebuffer].mode = -1; // Error! flag buffer as such.
  } else {
    if((disctype != CDVD_TYPE_PS2DVD) && (disctype != CDVD_TYPE_DVDV)) {
      if(mode == CDVD_MODE_2352) {
        CDreadSubQ(lsn, &bufferlist[replacebuffer].subq);
        errno = 0;
      } // ENDIF- Read subq as well?
    } // ENDIF- Read a DVD buffer or a CD buffer?
  } // ENDIF-Read ok? Fill out rest of buffer info.
  AddListBuffer(replacebuffer);
  return(s32result);
} // END CDVDreadTrack()


u8* CALLBACK CDVDgetBuffer() {
#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD interface: CDVDgetBuffer()");
#endif /* VERBOSE_FUNCTION */

  if(DiscInserted() == -1)  return(NULL);

  if(userbuffer == 0xffff) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD interface:   Not pointing to a buffer!");
#endif /* VERBOSE_WARNINGS */
    return(NULL); // No buffer reference?
  } // ENDIF- user buffer not pointing at anything? Abort

  if(bufferlist[userbuffer].mode < 0) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD interface:   Error in buffer!");
#endif /* VERBOSE_WARNINGS */
    return(NULL); // Bad Sector?
  } // ENDIF- Trouble reading physical sector? Tell them.

  return(bufferlist[userbuffer].buffer + bufferlist[userbuffer].offset);
} // END CDVDgetBuffer()


// Note: without the lsn, I could pull the SubQ data directly from
//   the stored buffer (in buffer.h). Oh, well.
s32 CALLBACK CDVDreadSubQ(u32 lsn, cdvdSubQ *subq) {
#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD interface: CDVDreadSubQ(%i)", lsn);
#endif /* VERBOSE_FUNCTION */

  if(DiscInserted() == -1)  return(-1);

  // DVDs don't have SubQ data
  if(disctype == CDVD_TYPE_PS2DVD)  return(-1);
  if(disctype == CDVD_TYPE_DVDV)  return(-1);

  return(CDreadSubQ(lsn, subq));
} // END CDVDreadSubQ()


s32 CALLBACK CDVDgetTN(cdvdTN *cdvdtn) {
#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD interface: CDVDgetTN()");
#endif /* VERBOSE_FUNCTION */

  if(DiscInserted() == -1)  return(-1);

  if((disctype == CDVD_TYPE_PS2DVD) || (disctype == CDVD_TYPE_DVDV)) {
    return(DVDgetTN(cdvdtn));
  } else {
    return(CDgetTN(cdvdtn));
  } // ENDIF- Are we looking at a DVD?
} // END CDVDgetTN()


s32 CALLBACK CDVDgetTD(u8 newtrack, cdvdTD *cdvdtd) {
#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD interface: CDVDgetTD()");
#endif /* VERBOSE_FUNCTION */

  if(DiscInserted() == -1)  return(-1);

  if((disctype == CDVD_TYPE_PS2DVD) || (disctype == CDVD_TYPE_DVDV)) {
    return(DVDgetTD(newtrack, cdvdtd));
  } else {
    return(CDgetTD(newtrack, cdvdtd));
  } // ENDIF- Are we looking at a DVD?
} // END CDVDgetTD()


s32 CALLBACK CDVDgetTOC(void *toc) {
  // A structure to fill in, or at least some documentation on what
  // the PS2 expects from this call would be more helpful than a 
  // "void *".

  union {
    void *voidptr;
    u8 *u8ptr;
  } tocptr;
  s32 i;

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD interface: CDVDgetTOC()");
#endif /* VERBOSE_FUNCTION */

  if(toc == NULL)  return(-1);
  if(DiscInserted() == -1)  return(-1);

  tocptr.voidptr = toc;
  for(i = 0; i < 1024; i++)  *(tocptr.u8ptr + i) = tocbuffer[i];
  tocptr.voidptr = NULL;

  return(0);
} // END CDVDgetTOC()


s32 CALLBACK CDVDgetDiskType() {
#ifdef VERBOSE_FUNCTION
  // Called way too often in boot part of bios to be left in.
  // PrintLog("CDVD interface: CDVDgetDiskType()");
#endif /* VERBOSE_FUNCTION */

  if(lasttime != time(NULL)) {
    lasttime = time(NULL);
    DeviceTrayStatus();
  } // ENDIF- Has enough time passed between calls?

  return(disctype);
} // END CDVDgetDiskType()


s32 CALLBACK CDVDgetTrayStatus() {
#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD interface: CDVDgetTrayStatus()");
#endif /* VERBOSE_FUNCTION */

  if(lasttime != time(NULL)) {
    lasttime = time(NULL);
    DeviceTrayStatus();
  } // ENDIF- Has enough time passed between calls?

  return(traystatus);
} // END CDVDgetTrayStatus()


s32 CALLBACK CDVDctrlTrayOpen() {
#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD interface: CDVDctrlTrayOpen()");
#endif /* VERBOSE_FUNCTION */

  return(DeviceTrayOpen());
} // END CDVDctrlTrayOpen()


s32 CALLBACK CDVDctrlTrayClose() {
#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD interface: CDVDctrlTrayClose()");
#endif /* VERBOSE_FUNCTION */

  return(DeviceTrayClose());
} // END CDVDctrlTrayClose()


void CALLBACK CDVDconfigure() {
  ExecCfg("configure");
} // END CDVDconfigure()


void CALLBACK CDVDabout() {
  ExecCfg("about");
} // END CDVDabout()


s32 CALLBACK CDVDtest() {
  s32 s32result;

  errno = 0;

  if(devicehandle != -1) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD interface:   Device already open");
#endif /* VERBOSE_WARNINGS */
    return(0);
  } // ENDIF- Is the CD/DVD already in use? That's fine.

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: CDVDtest()");
#endif /* VERBOSE_FUNCTION */

  s32result = DeviceOpen();
  DeviceClose();
  return(s32result);
} // END CDVDtest()
