/*  CD.c
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
#include <ddk/ntddcdrm.h> // IOCTL_CDROM...

#include <sys/types.h> // off64_t

#define CDVDdefs
#include "PS2Edefs.h"

#include "logfile.h"
#include "../convert.h" // MSFtoLBA(), HEXTOBCD()
#include "actualfile.h"
#include "device.h" // tocbuffer[], FinishCommand()
#include "CD.h"


int actualcdmode; // -1=ReadFile, 0=YellowMode2, 1=XAForm2, 2=CDDA
DWORD cdblocksize; // 2048 to 2352
int cdoffset; // 0, 8, 16, or 24
int cdmode;


void InitCDInfo() {
  actualcdmode = -1;
  cdblocksize = 2048;
  cdmode = -1;
} // END InitCDInfo()


s32 CDreadTrackPass(u32 lsn, int mode, u8 *buffer) {
  RAW_READ_INFO rawinfo;
  BOOL boolresult;
  DWORD byteswritten;
  DWORD errcode;
  LARGE_INTEGER targetpos;
  int i;

  if((actualcdmode < -1) || (actualcdmode > 2))  return(-1);

#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVDlinuz CD: CDreadTrack(%llu, %i)", lsn, actualcdmode);
#endif /* VERBOSE_FUNCTION_DEVICE */

  if(actualcdmode >= 0) {
    rawinfo.DiskOffset.QuadPart = lsn * 2048; // Yes, 2048.
    rawinfo.SectorCount = 1;
    rawinfo.TrackMode = actualcdmode;
    boolresult = DeviceIoControl(devicehandle,
                                 IOCTL_CDROM_RAW_READ,
                                 &rawinfo,
                                 sizeof(RAW_READ_INFO),
                                 buffer,
                                 2352,
                                 &byteswritten,
                                 &waitevent);
    errcode = FinishCommand(boolresult);

    if(errcode != 0) {
#ifdef VERBOSE_WARNING_DEVICE
      PrintLog("CDVDlinuz CD:   Couldn't read a sector raw!");
      PrintError("CDVDlinuz CD", errcode);
#endif /* VERBOSE_WARNING_DEVICE */
      return(-1);
    } // ENDIF- Couldn't read a raw sector? Maybe not a CD.

  } else {
    targetpos.QuadPart = lsn * 2048;
    waitevent.Offset = targetpos.LowPart;
    waitevent.OffsetHigh = targetpos.HighPart;

    boolresult = ReadFile(devicehandle,
                          buffer + 24,
                          2048,
                          &byteswritten,
                          &waitevent);
    errcode = FinishCommand(boolresult);

    if(errcode != 0) {
#ifdef VERBOSE_WARNING_DEVICE
      PrintLog("CDVDlinuz CD:   Couldn't read a cooked sector!");
      PrintError("CDVDlinuz CD", errcode);
#endif /* VERBOSE_WARNING_DEVICE */
      return(-1);
    } // ENDIF- Trouble with seek? Report it.

    for(i = 0; i < 24; i++)  *(buffer + i) = 0;
    for(i = 24 + 2048; i < 2352; i++) *(buffer + i) = 0;
  } // ENDIF- Could we read a raw sector? Read another one!

  if(boolresult == FALSE) {
    boolresult = GetOverlappedResult(devicehandle,
                                     &waitevent,
                                     &byteswritten,
                                     FALSE);
  } // ENDIF- Did the initial call not complete? Get byteswritten for
    //   the completed call.

  if(byteswritten < 2048) {
#ifdef VERBOSE_WARNING_DEVICE
    errcode = GetLastError();
    PrintLog("CDVDlinuz CD:   Short block! only got %u out of %u bytes",
             byteswritten, cdblocksize);
    PrintError("CDVDlinuz CD", errcode);
#endif /* VERBOSE_WARNING_DEVICE */
    return(-1);
  } // ENDIF- Couldn't read a raw sector? Maybe not a CD.

  cdmode = mode;
  cdblocksize = byteswritten;
  return(0);
} // END CDreadTrackPass()


s32 CDreadTrack(u32 lsn, int mode, u8 *buffer) {
  int retval;
  int lastmode;
  int i;

  retval = CDreadTrackPass(lsn, mode, buffer);
  if(retval >= 0)  return(retval);

#ifdef VERBOSE_WARNING_DEVICE
    PrintLog("CDVDlinuz CD:   Same mode doesn't work. Scanning...");
#endif /* VERBOSE_WARNING_DEVICE */

  lastmode = actualcdmode;
  actualcdmode = 2;
  while(actualcdmode >= -1) {
    retval = CDreadTrackPass(lsn, mode, buffer);
    if(retval >= 0)  return(retval);
    actualcdmode--;
  } // ENDWHILE- Searching each mode for a way to read the sector
  actualcdmode = lastmode; // None worked? Go back to first mode.

#ifdef VERBOSE_WARNING_DEVICE
    PrintLog("CDVDlinuz CD:   No modes work. Failing sector!");
#endif /* VERBOSE_WARNING_DEVICE */

  for(i = 0; i < 2352; i++)  *(buffer + i) = 0;
  return(-1);
} // END CDreadTrack()


s32 CDgetBufferOffset() {
#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVD CD: CDgetBufferOffset()");
#endif /* VERBOSE_FUNCTION_DEVICE */

  // Send a whole CDDA record in?
  if((actualcdmode == CDDA) && (cdmode == CDVD_MODE_2352)) return(0);

  // Otherwise, send the start of the data block in...
  return(cdoffset);
} // END CDgetBufferOffset()


s32 CDreadSubQ() {
  return(-1);
} // END CDreadSubQ()


s32 CDgetTN(cdvdTN *cdvdtn) {
  cdvdtn->strack = BCDTOHEX(tocbuffer[7]);
  cdvdtn->etrack = BCDTOHEX(tocbuffer[17]);
  return(0);
} // END CDgetTN()


s32 CDgetTD(u8 newtrack, cdvdTD *cdvdtd) {
  u8 actualtrack;
  int pos;
  char temptime[3];

#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVDlinuz CD: CDgetTD()");
#endif /* VERBOSE_FUNCTION_DEVICE */

  actualtrack = newtrack;
  if(actualtrack == 0xaa)  actualtrack = 0;

  if(actualtrack == 0) {
    cdvdtd->type = 0;
    temptime[0] = BCDTOHEX(tocbuffer[27]);
    temptime[1] = BCDTOHEX(tocbuffer[28]);
    temptime[2] = BCDTOHEX(tocbuffer[29]);
    cdvdtd->lsn = MSFtoLBA(temptime);
  } else {
    pos = actualtrack * 10;
    pos += 30;
    cdvdtd->type = tocbuffer[pos];
    temptime[0] = BCDTOHEX(tocbuffer[pos + 7]);
    temptime[1] = BCDTOHEX(tocbuffer[pos + 8]);
    temptime[2] = BCDTOHEX(tocbuffer[pos + 9]);
    cdvdtd->lsn = MSFtoLBA(temptime);
  } // ENDIF- Whole disc? (or single track?)

  return(0);
} // END CDgetTD()


s32 CDgetDiskType() {
  CDROM_TOC cdinfo;
  BOOL boolresult;
  int retval;
  DWORD byteswritten;
  DWORD errcode;
  u8 iso9660name[] = "CD001\0";
  u8 playstationname[] = "PLAYSTATION\0";
  u8 ps1name[] = "CD-XA001\0";
  u8 tempbuffer[2448];
  int tempdisctype;
  int i;
  int pos;
  unsigned long volumesize;

#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVDlinuz CD: CDgetDiskType()");
#endif /* VERBOSE_FUNCTION_DEVICE */

  tempdisctype = CDVD_TYPE_UNKNOWN;

  actualcdmode = 2;
  retval = CDreadTrack(16, CDVD_MODE_2048, tempbuffer);
  if(retval < 0) {
    disctype = tempdisctype;
    return(-1);
  } // ENDIF- Couldn't read the directory sector? Abort.

  disctype = CDVD_TYPE_DETCTCD;
  tempdisctype = CDVD_TYPE_CDDA;

  cdoffset = 0;
  i = 0;
  while((cdoffset <= 24) && (iso9660name[i] != 0)) {
    i = 0;
    while((iso9660name[i] != 0) &&
          (iso9660name[i] == tempbuffer[cdoffset + 1 + i]))  i++;
    if(iso9660name[i] != 0)  cdoffset += 8;
  } // ENDWHILE- Trying to find a working offset for a ISO9660 record.

  if(iso9660name[i] != 0) {
#ifdef VERBOSE_DISC_TYPE
    PrintLog("Detected a CDDA (Music CD).");
#endif /* VERBOSE_DISC_TYPE */
    tempdisctype = CDVD_TYPE_CDDA;
    tocbuffer[0] = 0x01;

  } else {
    tocbuffer[0] = 0x41;
    i = 0;
    while((playstationname[i] != 0) &&
          (playstationname[i] == tempbuffer[cdoffset + 8 + i]))  i++;
    if(playstationname[i] != 0) {
#ifdef VERBOSE_DISC_TYPE
      PrintLog("Detected a non-Playstation CD.");
#endif /* VERBOSE_DISC_TYPE */
      tempdisctype = CDVD_TYPE_UNKNOWN;

    } else {
      i = 0;
      while((ps1name[i] != 0) &&
            (ps1name[i] == tempbuffer[cdoffset + 1024 + i]))  i++;
      if(ps1name[i] != 0) {
#ifdef VERBOSE_DISC_TYPE
        PrintLog("Detected a Playstation 2 CD.");
#endif /* VERBOSE_DISC_TYPE */
        tempdisctype = CDVD_TYPE_PS2CD;
      } else {
#ifdef VERBOSE_DISC_TYPE
        PrintLog("Detected a Playstation CD.");
#endif /* VERBOSE_DISC_TYPE */
        tempdisctype = CDVD_TYPE_PSCD;
      } // ENDIF- Is this not a PlayStation Disc?
    } // ENDIF- Is this not a PlayStation Disc?
  } // ENDIF- Is this not an ISO9660 CD? (a CDDA, in other words?)

  // Build the Fake TOC
  tocbuffer[2] = 0xA0;
  tocbuffer[7] = HEXTOBCD(1); // Starting Track
  tocbuffer[12] = 0xA1;
  tocbuffer[17] = HEXTOBCD(1); // Ending Track

  volumesize = tempbuffer[84]; // Volume size (big endian)
  volumesize *= 256;
  volumesize += tempbuffer[85];
  volumesize *= 256;
  volumesize += tempbuffer[86];
  volumesize *= 256;
  volumesize += tempbuffer[87];
#ifdef VERBOSE_DISC_INFO
  if(tempdisctype != CDVD_TYPE_CDDA) {
    PrintLog("CDVDlinuz CD:   ISO9660 size %llu", volumesize);
  } // ENDIF- Data CD? Print size in blocks.
#endif /* VERBOSE_DISC_INFO */

  LBAtoMSF(volumesize, &tocbuffer[27]);
  tocbuffer[27] = HEXTOBCD(tocbuffer[27]);
  tocbuffer[28] = HEXTOBCD(tocbuffer[28]);
  tocbuffer[29] = HEXTOBCD(tocbuffer[29]);

  tocbuffer[40] = 0x02; // Data Mode
  tocbuffer[42] = 0x01; // Track #
  LBAtoMSF(0, &tocbuffer[47]);
  tocbuffer[47] = HEXTOBCD(tocbuffer[47]);
  tocbuffer[48] = HEXTOBCD(tocbuffer[48]);
  tocbuffer[49] = HEXTOBCD(tocbuffer[49]);

  // Can we get the REAL TOC?
  boolresult = DeviceIoControl(devicehandle,
                               IOCTL_CDROM_READ_TOC,
                               NULL,
                               0,
                               &cdinfo,
                               sizeof(CDROM_TOC),
                               &byteswritten,
                               NULL);

  if(boolresult == FALSE) {
#ifdef VERBOSE_WARNING_DEVICE
    errcode = GetLastError();
    PrintLog("CDVDlinuz CD:   Can't get TOC!");
    PrintError("CDVDlinuz CD", errcode);
#endif /* VERBOSE_WARNING_DEVICE */
    disctype = tempdisctype;
    return(disctype);
  } // ENDIF- Can't read the TOC? Accept the fake TOC then.

  // Fill in the pieces of the REAL TOC.
#ifdef VERBOSE_DISC_INFO
  PrintLog("CDVDlinuz CD:   TOC First Track: %u   Last Track: %u",
           cdinfo.FirstTrack, cdinfo.LastTrack);
#endif /* VERBOSE_DISC_INFO */
  tocbuffer[7] = HEXTOBCD(cdinfo.FirstTrack);
  tocbuffer[17] = HEXTOBCD(cdinfo.LastTrack);

  // for(i = cdinfo.FirstTrack; i <= cdinfo.LastTrack; i++) {
  for(i = 0; i <= cdinfo.LastTrack - cdinfo.FirstTrack; i++) {
#ifdef VERBOSE_DISC_INFO
  PrintLog("CDVDlinuz CD:   TOC Track %u   Disc Size: %u:%u.%u   Data Mode %u",
           cdinfo.TrackData[i].TrackNumber,
           cdinfo.TrackData[i].Address[1] * 1,
           cdinfo.TrackData[i].Address[2] * 1,
           cdinfo.TrackData[i].Address[3] * 1,
           cdinfo.TrackData[i].Control * 1);
#endif /* VERBOSE_DISC_INFO */
    pos = i * 10 + 40;
    tocbuffer[pos] = cdinfo.TrackData[i].Control;
    tocbuffer[pos + 2] = HEXTOBCD(i + 1);
    tocbuffer[pos + 7] = HEXTOBCD(cdinfo.TrackData[i].Address[1]);
    tocbuffer[pos + 8] = HEXTOBCD(cdinfo.TrackData[i].Address[2]);
    tocbuffer[pos + 9] = HEXTOBCD(cdinfo.TrackData[i].Address[3]);
  } // NEXT i- Transferring Track data to the PS2 TOC


#ifdef VERBOSE_DISC_INFO
  PrintLog("CDVDlinuz CD:   TOC Disc Size: %u:%u.%u",
           cdinfo.TrackData[i].Address[1] * 1,
           cdinfo.TrackData[i].Address[2] * 1,
           cdinfo.TrackData[i].Address[3] * 1);
#endif /* VERBOSE_DISC_INFO */
  tocbuffer[27] = HEXTOBCD(cdinfo.TrackData[i].Address[1]);
  tocbuffer[28] = HEXTOBCD(cdinfo.TrackData[i].Address[2]);
  tocbuffer[29] = HEXTOBCD(cdinfo.TrackData[i].Address[3]);

  disctype = tempdisctype;
  return(disctype);
} // END CDgetDiskType()
