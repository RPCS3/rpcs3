/*  device.c
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

#include <errno.h> // errno
#include <fcntl.h> // open()
#include <stddef.h> // NULL
#include <stdlib.h> // getenv()
#include <string.h> // strerror()
#include <sys/ioctl.h> // ioctl()
#include <sys/stat.h> // open()
#include <sys/types.h> // open()
#include <time.h> // time_t, time(), struct timeval
#include <unistd.h> // close(), select()

#include <linux/cdrom.h> // CD/DVD based ioctl() and defines.

// missing on some files for some reason.........
#ifndef CDC_IOCTLS
#define CDC_IOCTLS 0x400
#endif

#include "logfile.h"
#include "conf.h"
#include "CD.h"
#include "DVD.h"
#include "device.h"

#include "../PS2Etypes.h" // u32, u8, s32


// Globals

int devicehandle; // File Handle for the device/drive
s32 devicecapability; // Capability Flags
time_t lasttime; // Time marker (in case something gets called too often)
s32 traystatus; // Is the CD/DVD tray open?

s32 disctype; // Type of disc in drive (Video DVD, PSX CD, etc.)
u8 tocbuffer[2048];


void DeviceInit() {
  devicehandle = -1;
  devicecapability = 0;
  lasttime = time(NULL);

  InitDisc();
} // END DeviceInit()


// Called by DeviceOpen(), DeviceGetDiskType()
void InitDisc() {
  int i;

#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVD device: InitDisc()");
#endif /* VERBOSE_FUNCTION_DEVICE */

  if((disctype == CDVD_TYPE_PS2DVD) ||
     (disctype == CDVD_TYPE_DVDV)) {
    InitDVDInfo();
  } // ENDIF- Clean out DVD Disc Info?

  if((disctype == CDVD_TYPE_PS2CD) ||
     (disctype == CDVD_TYPE_PS2CDDA) ||
     (disctype == CDVD_TYPE_PSCD) ||
     (disctype == CDVD_TYPE_PSCDDA) ||
     (disctype == CDVD_TYPE_CDDA)) {
    InitCDInfo();
  } // ENDIF- Clean out DVD Disc Info?

  disctype = CDVD_TYPE_NODISC;
  for(i = 0; i > sizeof(tocbuffer); i++)  tocbuffer[i] = 0x00;
} // END InitDisc()


s32 DiscInserted() {
  if(devicehandle == -1)  return(-1);
  if(traystatus == CDVD_TRAY_OPEN)  return(-1);
  if(disctype == CDVD_TYPE_ILLEGAL)  return(-1);
  // if(disctype == CDVD_TYPE_UNKNOWN)  return(-1); // Hmm. Let this one through?
  if(disctype == CDVD_TYPE_DETCTDVDD)  return(-1);
  if(disctype == CDVD_TYPE_DETCTDVDS)  return(-1);
  if(disctype == CDVD_TYPE_DETCTCD)  return(-1);
  if(disctype == CDVD_TYPE_DETCT)  return(-1);
  if(disctype == CDVD_TYPE_NODISC)  return(-1);

#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVD device: DiscInserted()");
#endif /* VERBOSE_FUNCTION_DEVICE */

  return(0);
} // END DiscInserted()


// Called by DeviceTrayStatus() and CDVDopen()
s32 DeviceOpen() {
  // s32 s32result;

  errno = 0;

  if(devicehandle != -1) {
#ifdef VERBOSE_WARNING_DEVICE
    PrintLog("CDVD device:   Device already open!");
#endif /* VERBOSE_WARNING_DEVICE */
    return(0);
  } // ENDIF- Is the CD/DVD already in use? That's fine.

#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVD device: DeviceOpen()");
#endif /* VERBOSE_FUNCTION_DEVICE */

  // InitConf();
  // LoadConf(); // Should be done once before making this call

  devicehandle = open(conf.devicename, O_RDONLY | O_NONBLOCK);
  if(devicehandle == -1) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD device:   Error opening device: %i:%s", errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
    return(-1);
  } // ENDIF- Failed to open device? Abort

  // Note: Hmm. Need a minimum capability in case this fails?
  devicecapability = ioctl(devicehandle, CDROM_GET_CAPABILITY);
  if(errno != 0) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD device:   Error getting device capabilities: %i:%s", errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
    close(devicehandle);
    devicehandle = -1;
    devicecapability = 0;
    return(-1);
  } // ENDIF- Can't read drive capabilities? Close and Abort.

#ifdef VERBOSE_DISC_TYPE
  PrintLog("CDVD device: Device Type(s)");
  if(devicecapability < CDC_CD_R)  PrintLog("CDVD device:   CD");
  if(devicecapability & CDC_CD_R)  PrintLog("CDVD device:   CD-R");
  if(devicecapability & CDC_CD_RW)  PrintLog("CDVD device:   CD-RW");
  if(devicecapability & CDC_DVD)  PrintLog("CDVD device:   DVD");
  if(devicecapability & CDC_DVD_R)  PrintLog("CDVD device:   DVD-R");
  if(devicecapability & CDC_DVD_RAM)  PrintLog("CDVD device:   DVD-RAM");
#endif /* VERBOSE_DISC_TYPE */
#ifdef VERBOSE_DISC_INFO
  PrintLog("CDVD device: Device Capabilities:");
  if(devicecapability & CDC_CLOSE_TRAY)  PrintLog("CDVD device:   Can close a tray");
  if(devicecapability & CDC_OPEN_TRAY)  PrintLog("CDVD device:   Can open a tray");
  // if(devicecapability & CDC_LOCK)  PrintLog("CDVD device:   Can lock the drive door");
  if(devicecapability & CDC_SELECT_SPEED)  PrintLog("CDVD device:   Can change spin speed");
  // if(devicecapability & CDC_SELECT_DISC)  PrintLog("CDVD device:   Can change disks (multi-disk tray)");
  // if(devicecapability & CDC_MULTI_SESSION)  PrintLog("CDVD device:   Can read multi-session disks");
  // if(devicecapability & CDC_MCN)  PrintLog("CDVD device:   Can read Medium Catalog Numbers (maybe)");
  if(devicecapability & CDC_MEDIA_CHANGED)  PrintLog("CDVD device:   Can tell if the disc was changed");
  if(devicecapability & CDC_PLAY_AUDIO)  PrintLog("CDVD device:   Can play audio disks");
  // if(devicecapability & CDC_RESET)  PrintLog("CDVD device:   Can reset the device");
  if(devicecapability & CDC_IOCTLS)  PrintLog("CDVD device:   Odd IOCTLs. Not sure of compatability");
  if(devicecapability & CDC_DRIVE_STATUS)  PrintLog("CDVD device:   Can monitor the drive tray");
#endif /* VERBOSE_DISC_INFO */

  ////// Should be called after an open (instead of inside of one)
  // InitDisc();
  // traystatus = CDVD_TRAY_OPEN; // Start with Tray Open
  // DeviceTrayStatus(); // Now find out for sure.

  return(0); // Device opened and ready for use.
} // END DeviceOpen()


// Called by DeviceTrayStatus(), CDVDclose(), and CDVDshutdown()
void DeviceClose() {
  if(devicehandle == -1) {
#ifdef VERBOSE_FUNCTION_DEVICE
    PrintLog("CDVD device:   Device already closed");
#endif /* VERBOSE_FUNCTION_DEVICE */
    return;
  } // ENDIF- Device already closed? Ok.

#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVD device: DeviceClose()");
#endif /* VERBOSE_FUNCTION_DEVICE */

  InitDisc();
  close(devicehandle);
  devicehandle = -1;
  devicecapability = 0;
  return;
} // END CDVDclose()


s32 DeviceReadTrack(u32 lsn, int mode, u8 *buffer) {
  s32 s32result;

#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVD device: DeviceReadTrack(%i)", lsn);
#endif /* VERBOSE_FUNCTION_DEVICE */

  if(DiscInserted() == -1)  return(-1);

  // Get that data
  if((disctype == CDVD_TYPE_PS2DVD) || (disctype == CDVD_TYPE_DVDV)) {
    s32result = DVDreadTrack(lsn, mode, buffer);
  } else {
    s32result = CDreadTrack(lsn, mode, buffer);
  } //ENDIF- Read a DVD sector or a CD sector?

  return(s32result);
} // END DeviceReadTrack()


s32 DeviceBufferOffset() {
#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVD device: DeviceBufferOffset()");
#endif /* VERBOSE_FUNCTION_DEVICE */

  if(DiscInserted() == -1)  return(-1);

  if((disctype == CDVD_TYPE_PS2DVD) || (disctype == CDVD_TYPE_DVDV)) {
    return(0);
  } else {
    return(CDgetBufferOffset());
  } // ENDIF- Is this a DVD?
} // END DeviceBufferOffset()


s32 DeviceGetTD(u8 track, cdvdTD *cdvdtd) {
  if(DiscInserted() == -1)  return(-1);

  if((disctype == CDVD_TYPE_PS2DVD) || (disctype == CDVD_TYPE_DVDV)) {
    return(DVDgetTD(track, cdvdtd));
  } else {
    return(CDgetTD(track, cdvdtd));
  } // ENDIF- Is this a DVD?
} // END DeviceGetTD()


// Called by DeviceTrayStatus()
s32 DeviceGetDiskType() {
  s32 s32result;
  s32 ioctldisktype;

  errno = 0;

  if(devicehandle == -1) {
    return(-1);
  } // ENDIF- Someone forget to open the device?

  if(traystatus == CDVD_TRAY_OPEN) {
    return(disctype);
  } // ENDIF- Is the device tray open? No disc to check yet.

  if(disctype != CDVD_TYPE_NODISC) {
    return(disctype);
  } // ENDIF- Already checked? Drive still closed? Disc hasn't changed.

#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVD device: DeviceGetDiskType()");
#endif /* VERBOSE_FUNCTION_DEVICE */
  disctype = CDVD_TYPE_DETCT;

  ioctldisktype = ioctl(devicehandle, CDROM_DISC_STATUS);
  if(errno != 0) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD device:   Trouble reading Disc Type!");
    PrintLog("CDVD device:     Error: %i:%s", errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
    disctype = CDVD_TYPE_UNKNOWN;
    return(disctype);
  } // ENDIF- Trouble probing for a disc?

  s32result = DVDgetDiskType(ioctldisktype);
  if(s32result != -1) {
    return(disctype);
  } // ENDIF- Did we find a disc type?

  s32result = CDgetDiskType(ioctldisktype);
  if(s32result != -1) {
    return(disctype);
  } // ENDIF- Did we find a disc type?

  disctype = CDVD_TYPE_UNKNOWN; // Not a CD? Not a DVD? Is is peanut butter?
  return(disctype);
} // END CDVDgetDiskType()


// Called by PollLoop() and CDVDgetTrayStatus()
s32 DeviceTrayStatus() {
  s32 s32result;

  errno = 0;

#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVD device: DeviceTrayStatus()");
#endif /* VERBOSE_FUNCTION_DEVICE */

  if(devicehandle == -1) {
    return(-1);
  } // ENDIF- Someone forget to open the device?

  if((devicecapability & CDC_DRIVE_STATUS) != 0) {
    s32result = ioctl(devicehandle, CDROM_DRIVE_STATUS);
    if(s32result < 0) {
#ifdef VERBOSE_WARNINGS
      PrintLog("CDVD device:   Trouble reading Drive Status!");
      PrintLog("CDVD device:     Error: (%i) %i:%s", s32result, errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
      s32result = CDS_TRAY_OPEN;
    } // ENDIF- Failure to get status? Assume it's open.
    errno = 0;

  } else {
    s32result = ioctl(devicehandle, CDROM_DISC_STATUS);
    if(errno != 0) {
#ifdef VERBOSE_WARNINGS
      PrintLog("CDVD device:   Trouble detecting Disc Status presense!");
      PrintLog("CDVD device:     Error: (%i) %i:%s", s32result, errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
      s32result = CDS_TRAY_OPEN;
      errno = 0;
    } // ENDIF- Trouble?
    if(s32result == CDS_NO_DISC) {
      s32result = CDS_TRAY_OPEN;
    } // ENDIF- Is there no disc in the device? Guess the tray is open
  } // ENDIF- Can we poll the tray directly? (Or look at disc status instead?)

  if(s32result == CDS_TRAY_OPEN) {
    traystatus = CDVD_TRAY_OPEN;
    if(disctype != CDVD_TYPE_NODISC) {
      DeviceClose(); // Kind of severe way of flushing all buffers.
      DeviceOpen();
      InitDisc();
    } // ENDIF- Tray just opened... clear disc info
  } else {
    traystatus = CDVD_TRAY_CLOSE;
    if(disctype == CDVD_TYPE_NODISC) {
      DeviceGetDiskType();
    } // ENDIF- Tray just closed? Get disc information
  } // ENDIF- Do we detect an open tray?
  return(traystatus);
} // END CDVD_getTrayStatus()


s32 DeviceTrayOpen() {
  s32 s32result;

  errno = 0;

  if(devicehandle == -1) {
    return(-1);
  } // ENDIF- Someone forget to open the device?

  if((devicecapability & CDC_OPEN_TRAY) == 0) {
    return(-1);
  } // ENDIF- Don't have open capability? Error out.

  // Tray already open? Exit.
  if(traystatus == CDVD_TRAY_OPEN)  return(0);

#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVD device: DeviceTrayOpen()");
#endif /* VERBOSE_FUNCTION_DEVICE */

  s32result = ioctl(devicehandle, CDROMEJECT);
#ifdef VERBOSE_WARNINGS
  if((s32result != 0) || (errno != 0)) {
    PrintLog("CDVD device:   Could not open the tray!");
    PrintLog("CDVD device:     Error: (%i) %i:%s", s32result, errno, strerror(errno));
  } // ENDIF- Trouble?
#endif /* VERBOSE_WARNINGS */
  return(s32result);
} // END DeviceTrayOpen()


s32 DeviceTrayClose() {
  s32 s32result;

  errno = 0;

  if(devicehandle == -1) {
    return(-1);
  } // ENDIF- Someone forget to open the device?

  if((devicecapability & CDC_CLOSE_TRAY) == 0) {
    return(-1);
  } // ENDIF- Don't have close capability? Error out.

  // Tray already closed? Exit.
  if(traystatus == CDVD_TRAY_CLOSE)  return(0);

#ifdef VERBOSE_FUNCTION_DEVICE
  PrintLog("CDVD device: DeviceTrayClose()");
#endif /* VERBOSE_FUNCTION_DEVICE */

  s32result = ioctl(devicehandle, CDROMCLOSETRAY);
#ifdef VERBOSE_WARNINGS
  if((s32result != 0) || (errno != 0)) {
    PrintLog("CDVD device:   Could not close the tray!");
    PrintLog("CDVD device:     Error: (%i) %i:%s", s32result, errno, strerror(errno));
  } // ENDIF- Trouble?
#endif /* VERBOSE_WARNINGS */
  return(s32result);
} // END DeviceTrayClose()

