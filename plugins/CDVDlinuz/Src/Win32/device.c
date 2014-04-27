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

 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

 *

 *  PCSX2 members can be contacted through their website at www.pcsx2.net.

 */





#include <windows.h>

#include <ddk/ntddcdrm.h> // IOCTL_CDROM..., IOCTL_STORAGE...

#include <ddk/ntdddisk.h> // IOCTL_DISK...

#include <stdio.h> // sprintf()

#include <time.h> // time_t



#define CDVDdefs

#include "PS2Edefs.h"



#include "logfile.h"

#include "conf.h"

#include "CD.h"

#include "DVD.h"

#include "device.h"





HANDLE devicehandle;

OVERLAPPED waitevent;



time_t lasttime;

s32 traystatus;

int traystatusmethod;

s32 disctype;

char tocbuffer[2048];





void DeviceInit() {

  devicehandle = NULL;

  waitevent.hEvent = NULL;

  waitevent.Internal = 0;

  waitevent.InternalHigh = 0;

  waitevent.Offset = 0;

  waitevent.OffsetHigh = 0;

  lasttime = 0;



  InitDisc();

} // END DeviceInit()





void InitDisc() {

  int i;



  InitCDInfo();

  InitDVDInfo();

  traystatus = CDVD_TRAY_OPEN;

  traystatusmethod = 0; // Poll all until one works

  disctype = CDVD_TYPE_NODISC;

  for(i = 0; i < 2048; i++)  tocbuffer[i] = 0;

} // END InitDisc()





s32 DiscInserted() {

  if(traystatus != CDVD_TRAY_CLOSE)  return(-1);



  if(disctype == CDVD_TYPE_ILLEGAL)  return(-1);

  // if(disctype == CDVD_TYPE_UNKNOWN)  return(-1); // Hmm. Let this one through?

  if(disctype == CDVD_TYPE_DETCTDVDD)  return(-1);

  if(disctype == CDVD_TYPE_DETCTDVDS)  return(-1);

  if(disctype == CDVD_TYPE_DETCTCD)  return(-1);

  if(disctype == CDVD_TYPE_DETCT)  return(-1);

  if(disctype == CDVD_TYPE_NODISC)  return(-1);



#ifdef VERBOSE_FUNCTION_DEVICE

  PrintLog("CDVDlinuz device: DiscInserted()");

#endif /* VERBOSE_FUNCTION_DEVICE */



  return(0);

} // END DiscInserted()





// Returns errcode (or 0 if successful)

DWORD FinishCommand(BOOL boolresult) {

  DWORD errcode;

  DWORD waitcode;



#ifdef VERBOSE_FUNCTION_DEVICE

  PrintLog("CDVDlinuz device: FinishCommand()");

#endif /* VERBOSE_FUNCTION_DEVICE */



  if(boolresult == TRUE) {

    ResetEvent(waitevent.hEvent);

    return(0);

  } // ENDIF- Device is ready? Say so.



  errcode = GetLastError();

  if(errcode == ERROR_IO_PENDING) {

#ifdef VERBOSE_FUNCTION_DEVICE

    PrintLog("CDVDlinuz device:   Waiting for completion.");

#endif /* VERBOSE_FUNCTION_DEVICE */

    waitcode = WaitForSingleObject(waitevent.hEvent, 10 * 1000); // 10 sec wait

    if((waitcode == WAIT_FAILED) || (waitcode == WAIT_ABANDONED)) {

      errcode = GetLastError();

    } else if(waitcode == WAIT_TIMEOUT) {

      errcode = 21;

      CancelIo(devicehandle); // Speculative Line

    } else {

      ResetEvent(waitevent.hEvent);

      return(0); // Success!

    } // ENDIF- Trouble waiting? (Or doesn't finish in 5 seconds?)

  } // ENDIF- Should we wait for the call to finish?



  ResetEvent(waitevent.hEvent);

  return(errcode);

} // END DeviceCommand()





s32 DeviceOpen() {

  char tempname[256];

  UINT drivetype;

  DWORD errcode;



  if(conf.devicename[0] == 0)  return(-1);



  if(devicehandle != NULL)  return(0);



#ifdef VERBOSE_FUNCTION_DEVICE

  PrintLog("CDVDlinuz device: DeviceOpen()");

#endif /* VERBOSE_FUNCTION_DEVICE */



  // InitConf();

  // LoadConf(); // Should be done at least once before this call



  // Root Directory reference

  if(conf.devicename[1] == 0) {

    sprintf(tempname, "%s:\\", conf.devicename);

  } else if((conf.devicename[1] == ':') && (conf.devicename[2] == 0)) {

    sprintf(tempname, "%s\\", conf.devicename);

  } else {

    sprintf(tempname, "%s", conf.devicename);

  } // ENDIF- Not a single drive letter? (or a letter/colon?) Copy the name in.



  drivetype = GetDriveType(tempname);

  if(drivetype != DRIVE_CDROM) {

#ifdef VERBOSE_WARNING_DEVICE

    PrintLog("CDVDlinuz device:   Not a CD-ROM!");

    PrintLog("CDVDlinuz device:     (Came back: %u)", drivetype);

    errcode = GetLastError();

    if(errcode > 0)  PrintError("CDVDlinuz device", errcode);

#endif /* VERBOSE_WARNING_DEVICE */

    return(-1);

  } // ENDIF- Not a CD-ROM? Say so!

  // Hmm. Do we want to include DRIVE_REMOVABLE... just in case?



  // Device Reference

  if(conf.devicename[1] == 0) {

    sprintf(tempname, "\\\\.\\%s:", conf.devicename);

  } else if((conf.devicename[1] == ':') && (conf.devicename[2] == 0)) {

    sprintf(tempname, "\\\\.\\%s", conf.devicename);

  } else {

    sprintf(tempname, "%s", conf.devicename);

  } // ENDIF- Not a single drive letter? (or a letter/colon?) Copy the name in.



  devicehandle = CreateFile(tempname,

                            GENERIC_READ,

                            FILE_SHARE_READ,

                            NULL,

                            OPEN_EXISTING,

                            FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN,

                            NULL);



  if(devicehandle == INVALID_HANDLE_VALUE) {

#ifdef VERBOSE_WARNING_DEVICE

    PrintLog("CDVDlinuz device:   Couldn't open device read-only! Read-Write perhaps?");

    errcode = GetLastError();

    PrintError("CDVDlinuz device", errcode);

#endif /* VERBOSE_WARNING_DEVICE */

    devicehandle = CreateFile(tempname,

                              GENERIC_READ | GENERIC_WRITE,

                              FILE_SHARE_READ | FILE_SHARE_WRITE,

                              NULL,

                              OPEN_EXISTING,

                              FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN,

                              NULL);

  } // ENDIF- Couldn't open for read? Try read/write (for those drives that insist)



  if(devicehandle == INVALID_HANDLE_VALUE) {

#ifdef VERBOSE_WARNING_DEVICE

    PrintLog("CDVDlinuz device:   Couldn't open device!");

    errcode = GetLastError();

    PrintError("CDVDlinuz device", errcode);

#endif /* VERBOSE_WARNING_DEVICE */

    devicehandle = NULL;

    return(-1);

  } // ENDIF- Couldn't open that way either? Abort.



  waitevent.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  if(waitevent.hEvent == INVALID_HANDLE_VALUE) {

#ifdef VERBOSE_WARNING_DEVICE

    PrintLog("CDVDlinuz device:   Couldn't open event handler!");

    errcode = GetLastError();

    PrintError("CDVDlinuz device", errcode);

#endif /* VERBOSE_WARNING_DEVICE */

    waitevent.hEvent = NULL;

    CloseHandle(devicehandle);

    devicehandle = NULL;

  } // ENDIF- Couldn't create an "Wait for I/O" handle? Abort.



  // More here... DeviceIoControl? for Drive Capabilities

  // DEVICE_CAPABILITIES?



  ////// Should be done just after the first DeviceOpen();

  // InitDisc(); // ?

  // DeviceTrayStatus();



  return(0);

} // END DeviceOpen()





void DeviceClose() {

  if(devicehandle == NULL)  return;



  if(devicehandle == INVALID_HANDLE_VALUE) {

    devicehandle = NULL;

    return;

  } // ENDIF- Bad value? Just clear the value.



#ifdef VERBOSE_FUNCTION_DEVICE

  PrintLog("CDVDlinuz device: DeviceClose()");

#endif /* VERBOSE_FUNCTION_DEVICE */



  if(waitevent.hEvent != NULL) {

    if(waitevent.hEvent != INVALID_HANDLE_VALUE) {

      CancelIo(devicehandle);

      CloseHandle(waitevent.hEvent);

    } // ENDIF- Is this handle actually open?

    waitevent.hEvent = NULL;

    waitevent.Offset = 0;

    waitevent.OffsetHigh = 0;

  } // ENDIF-  Reset the event handle?



  CloseHandle(devicehandle);

  devicehandle = NULL;

  return;

} // END DeviceClose()





s32 DeviceReadTrack(u32 lsn, int mode, u8 *buffer) {

  if(DiscInserted() == -1)  return(-1);



  if((disctype == CDVD_TYPE_PS2DVD) || (disctype == CDVD_TYPE_DVDV)) {

    return(DVDreadTrack(lsn, mode, buffer));

  } else {

    return(CDreadTrack(lsn, mode, buffer));

  } // ENDIF- Is this a DVD?

} // END DeviceReadTrack()





s32 DeviceBufferOffset() {

  if(DiscInserted() == -1)  return(-1);



  if((disctype == CDVD_TYPE_PS2DVD) || (disctype == CDVD_TYPE_DVDV)) {

    return(0);

  } else {

    return(CDgetBufferOffset());

  } // ENDIF- Is this a DVD?



  return(-1);

} // END DeviceBufferOffset()





s32 DeviceGetTD(u8 track, cdvdTD *cdvdtd) {

  if(DiscInserted() == -1)  return(-1);



  if((disctype == CDVD_TYPE_PS2DVD) || (disctype == CDVD_TYPE_DVDV)) {

    return(DVDgetTD(track, cdvdtd));

  } else {

    return(CDgetTD(track, cdvdtd));

  } // ENDIF- Is this a DVD?



  return(-1);

} // END DeviceGetTD()





s32 DeviceGetDiskType() {

  s32 s32result;



  if(devicehandle == NULL)  return(-1);

  if(devicehandle == INVALID_HANDLE_VALUE)  return(-1);



  if(traystatus == CDVD_TRAY_OPEN)  return(disctype);



  if(disctype != CDVD_TYPE_NODISC)  return(disctype);



#ifdef VERBOSE_FUNCTION_DEVICE

  PrintLog("CDVDlinuz device: DeviceGetDiskType()");

#endif /* VERBOSE_FUNCTION_DEVICE */



  disctype = CDVD_TYPE_DETCT;



  s32result = DVDgetDiskType();

  if(s32result != -1)  return(disctype);



  s32result = CDgetDiskType();

  if(s32result != -1)  return(disctype);



  disctype = CDVD_TYPE_UNKNOWN;

  return(disctype);

} // END DeviceGetDiskType()





BOOL DeviceTrayStatusStorage() {

  BOOL boolresult;

  DWORD byteswritten;

  DWORD errcode;



  boolresult = DeviceIoControl(devicehandle,

                               IOCTL_STORAGE_CHECK_VERIFY,

                               NULL,

                               0,

                               NULL,

                               0,

                               &byteswritten,

                               NULL);

  errcode = FinishCommand(boolresult);



  if(errcode == 0)  return(TRUE);

  if(errcode == 21)  return(FALSE); // Device not ready? (Valid error)



#ifdef VERBOSE_WARNING_DEVICE

  PrintLog("CDVDlinuz device:   Trouble detecting drive status (STORAGE)!");

  PrintError("CDVDlinuz device", errcode);

#endif /* VERBOSE_WARNING_DEVICE */

  return(FALSE);

} // END DeviceTrayStatusStorage()





BOOL DeviceTrayStatusCDRom() {

  BOOL boolresult;

  DWORD byteswritten;

  DWORD errcode;



  boolresult = DeviceIoControl(devicehandle,

                               IOCTL_CDROM_CHECK_VERIFY,

                               NULL,

                               0,

                               NULL,

                               0,

                               &byteswritten,

                               NULL);

  errcode = FinishCommand(boolresult);



  if(errcode == 0)  return(TRUE);

  if(errcode == 21)  return(FALSE); // Device not ready? (Valid error)



#ifdef VERBOSE_WARNING_DEVICE

  PrintLog("CDVDlinuz device:   Trouble detecting drive status (CDROM)!");

  PrintError("CDVDlinuz device", errcode);

#endif /* VERBOSE_WARNING_DEVICE */

  return(FALSE);

} // END DeviceTrayStatusCDRom()





BOOL DeviceTrayStatusDisk() {

  BOOL boolresult;

  DWORD byteswritten;

  DWORD errcode;



  boolresult = DeviceIoControl(devicehandle,

                               IOCTL_DISK_CHECK_VERIFY,

                               NULL,

                               0,

                               NULL,

                               0,

                               &byteswritten,

                               NULL);

  errcode = FinishCommand(boolresult);



  if(errcode == 0)  return(TRUE);

  if(errcode == 21)  return(FALSE); // Device not ready? (Valid error)



#ifdef VERBOSE_WARNING_DEVICE

  PrintLog("CDVDlinuz device:   Trouble detecting drive status (DISK)!");

  PrintError("CDVDlinuz device", errcode);

#endif /* VERBOSE_WARNING_DEVICE */

  return(FALSE);

} // END DeviceTrayStatusDisk()





s32 DeviceTrayStatus() {

  BOOL boolresult;



  if(devicehandle == NULL)  return(-1);

  if(devicehandle == INVALID_HANDLE_VALUE)  return(-1);



#ifdef VERBOSE_FUNCTION_DEVICE

  PrintLog("CDVDlinuz device: DeviceTrayStatus()");

#endif /* VERBOSE_FUNCTION_DEVICE */



  switch(traystatusmethod) {

    case 1:

      boolresult = DeviceTrayStatusStorage();

      break;

    case 2:

      boolresult = DeviceTrayStatusCDRom();

      break;

    case 3:

      boolresult = DeviceTrayStatusDisk();

      break;

    default:

      boolresult = FALSE;

      break;

  } // ENDSWITCH traystatusmethod- One method already working? Try it again.



  if(boolresult == FALSE) {

    traystatusmethod = 0;

    boolresult = DeviceTrayStatusStorage();

    if(boolresult == TRUE) {

      traystatusmethod = 1;

    } else {

      boolresult = DeviceTrayStatusCDRom();

      if(boolresult == TRUE) {

        traystatusmethod = 2;

      } else {

        boolresult = DeviceTrayStatusDisk();

        if(boolresult == TRUE)  traystatusmethod = 3;

      } // ENDIF- Did we succeed with CDRom?

    } // ENDIF- Did we succeed with Storage?

  } // Single call to already working test just failed? Test them all.



  if(boolresult == FALSE) {

    if(traystatus == CDVD_TRAY_CLOSE) {

#ifdef VERBOSE_FUNCTION_DEVICE

      PrintLog("CDVDlinuz device:   Tray just opened!");

#endif /* VERBOSE_FUNCTION_DEVICE */

      traystatus = CDVD_TRAY_OPEN;

      DeviceClose();

      DeviceOpen();

      InitDisc();

    } // ENDIF- Just opened? clear disc info

    return(traystatus);

  } // ENDIF- Still failed? Assume no disc in drive then.



  if(traystatus == CDVD_TRAY_OPEN) {

    traystatus = CDVD_TRAY_CLOSE;

#ifdef VERBOSE_FUNCTION_DEVICE

    PrintLog("CDVDlinuz device:   Tray just closed!");

#endif /* VERBOSE_FUNCTION_DEVICE */

    DeviceGetDiskType();

    return(traystatus);

  } // ENDIF- Just closed? Get disc information



  return(traystatus);

} // END DeviceTrayStatus()





s32 DeviceTrayOpen() {

  BOOL boolresult;

  DWORD byteswritten;

  DWORD errcode;



  if(devicehandle == NULL)  return(-1);

  if(devicehandle == INVALID_HANDLE_VALUE)  return(-1);



  if(traystatus == CDVD_TRAY_OPEN)  return(0);



#ifdef VERBOSE_FUNCTION_DEVICE

  PrintLog("CDVDlinuz device: DeviceOpenTray()");

#endif /* VERBOSE_FUNCTION_DEVICE */



  boolresult = DeviceIoControl(devicehandle,

                               IOCTL_STORAGE_EJECT_MEDIA,

                               NULL,

                               0,

                               NULL,

                               0,

                               &byteswritten,

                               &waitevent);

  errcode = FinishCommand(boolresult);



  if(errcode != 0) {

#ifdef VERBOSE_WARNING_DEVICE

    PrintLog("CDVDlinuz device:   Couldn't signal media to eject! (STORAGE)");

    PrintError("CDVDlinuz device", errcode);

#endif /* VERBOSE_WARNING_DEVICE */



//     boolresult = DeviceIoControl(devicehandle,

//                                  IOCTL_DISK_EJECT_MEDIA,

//                                  NULL,

//                                  0,

//                                  NULL,

//                                  0,

//                                  &byteswritten,

//                                  NULL);

//   } // ENDIF- Storage Call failed? Try Disk call.



//   if(boolresult == FALSE) {

// #ifdef VERBOSE_WARNING_DEVICE

//     PrintLog("CDVDlinuz device:   Couldn't signal media to eject! (DISK)");

//     PrintError("CDVDlinuz device", errcode);

// #endif /* VERBOSE_WARNING_DEVICE */

    return(-1);

  } // ENDIF- Disk Call failed as well? Give it up.



  return(0);

} // END DeviceTrayOpen()





s32 DeviceTrayClose() {

  BOOL boolresult;

  DWORD byteswritten;

  DWORD errcode;



  if(devicehandle == NULL)  return(-1);

  if(devicehandle == INVALID_HANDLE_VALUE)  return(-1);



  if(traystatus == CDVD_TRAY_CLOSE)  return(0);



#ifdef VERBOSE_FUNCTION_DEVICE

  PrintLog("CDVDlinuz device: DeviceCloseTray()");

#endif /* VERBOSE_FUNCTION_DEVICE */



  boolresult = DeviceIoControl(devicehandle,

                               IOCTL_STORAGE_LOAD_MEDIA,

                               NULL,

                               0,

                               NULL,

                               0,

                               &byteswritten,

                               NULL);

  errcode = FinishCommand(boolresult);



  if(errcode != 0) {

#ifdef VERBOSE_WARNING_DEVICE

    PrintLog("CDVDlinuz device:   Couldn't signal media to load! (STORAGE)");

    PrintError("CDVDlinuz device", errcode);

#endif /* VERBOSE_WARNING_DEVICE */

//     boolresult = DeviceIoControl(devicehandle,

//                                  IOCTL_CDROM_LOAD_MEDIA,

//                                  NULL,

//                                  0,

//                                  NULL,

//                                  0,

//                                  &byteswritten,

//                                  NULL);

//   } // ENDIF- Storage call failed. CDRom call?



//   if(boolresult == FALSE) {

//     errcode = GetLastError();

// #ifdef VERBOSE_WARNING_DEVICE

//     PrintLog("CDVDlinuz device:   Couldn't signal media to load! (CDROM)");

//     PrintError("CDVDlinuz device", errcode);

// #endif /* VERBOSE_WARNING_DEVICE */

//     boolresult = DeviceIoControl(devicehandle,

//                                  IOCTL_DISK_LOAD_MEDIA,

//                                  NULL,

//                                  0,

//                                  NULL,

//                                  0,

//                                  &byteswritten,

//                                  NULL);

//   } // ENDIF- CDRom call failed. Disk call?



//   if(boolresult == FALSE) {

// #ifdef VERBOSE_WARNING_DEVICE

//     PrintLog("CDVDlinuz device:   Couldn't signal media to load! (DISK)");

//     PrintError("CDVDlinuz device", errcode);

// #endif /* VERBOSE_WARNING_DEVICE */

    return(-1);

  } // ENDIF- Media not available?



  return(0);

} // END DeviceTrayClose()

