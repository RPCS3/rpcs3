/*  DVD.c

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

#include <ddk/ntddcdvd.h> // IOCTL_DVD...



#include <sys/types.h> // off64_t



#define CDVDdefs

#include "PS2Edefs.h"



#include "logfile.h"

#include "device.h" // FinishCommand()





struct {

  DVD_DESCRIPTOR_HEADER h;

  DVD_LAYER_DESCRIPTOR d;

} layer;

// DVD_LAYER_DESCRIPTOR layer;

// DVD_COPYRIGHT_DESCRIPTOR copyright;

// DVD_DISK_KEY_DESCRIPTOR disckey;

// DVD_BCA_DESCRIPTOR bca;

// DVD_MANUFACTURER_DESCRIPTOR manufact;





void InitDVDInfo() {

  layer.d.EndDataSector = 0;

} // END InitDVDInfo()





s32 DVDGetStructures() {

  DVD_SESSION_ID sessionid;

  DVD_READ_STRUCTURE request;

  DWORD byteswritten;

  BOOL boolresult;

  DWORD errcode;

  s32 retval;



#ifdef VERBOSE_FUNCTION_DEVICE

  PrintLog("CDVDlinuz DVD: DVDgetStructures()");

#endif /* VERBOSE_FUNCTION_DEVICE */



  boolresult = DeviceIoControl(devicehandle,

                               IOCTL_DVD_START_SESSION,

                               NULL,

                               0,

                               &sessionid,

                               sizeof(DVD_SESSION_ID),

                               &byteswritten,

                               &waitevent);

  errcode = FinishCommand(boolresult);

  if(errcode != 0) {

#ifdef VERBOSE_WARNING_DEVICE

    PrintLog("CDVDlinuz DVD:   Couldn't start session!");

    PrintError("CDVDlinuz DVD", errcode);

#endif /* VERBOSE_WARNING_DEVICE */

    return(-1);

  } // ENDIF- Couldn't start a user session on the DVD drive? Fail.



  request.BlockByteOffset.QuadPart = 0;

  request.Format = DvdPhysicalDescriptor;

  request.SessionId = sessionid;

  request.LayerNumber = 0;

  retval = 0;

  boolresult = DeviceIoControl(devicehandle,

                               IOCTL_DVD_READ_STRUCTURE,

                               &request,

                               sizeof(DVD_READ_STRUCTURE),

                               &layer,

                               sizeof(layer),

                               &byteswritten,

                               &waitevent);

  errcode = FinishCommand(boolresult);

  if(errcode != 0) {

#ifdef VERBOSE_WARNING_DEVICE

    PrintLog("CDVDlinuz DVD:   Couldn't get layer data!");

    PrintError("CDVDlinuz DVD", errcode);

#endif /* VERBOSE_WARNING_DEVICE */

    retval = -1;

  } // ENDIF- Couldn't get layer data? (Including DVD size) Abort.



#ifdef VERBOSE_DISC_INFO

  switch(layer.d.BookType) {

    case 0:

      PrintLog("CDVDlinuz DVD:   Book Type: DVD-ROM");

      break;

    case 1:

      PrintLog("CDVDlinuz DVD:   Book Type: DVD-RAM");

      break;

    case 2:

      PrintLog("CDVDlinuz DVD:   Book Type: DVD-R");

      break;

    case 3:

      PrintLog("CDVDlinuz DVD:   Book Type: DVD-RW");

      break;

    case 9:

      PrintLog("CDVDlinuz DVD:   Book Type: DVD+RW");

      break;

    default:

      PrintLog("CDVDlinuz DVD:   Book Type: Unknown (%i)", layer.d.BookType);

      break;

  } // ENDSWITCH- Displaying the Book Type

  PrintLog("CDVDlinuz DVD:   Book Version %i", layer.d.BookVersion);

  switch(layer.d.MinimumRate) {

    case 0:

      PrintLog("CDVDlinuz DVD:   Use Minimum Rate for: DVD-ROM");

      break;

    case 1:

      PrintLog("CDVDlinuz DVD:   Use Minimum Rate for: DVD-RAM");

      break;

    case 2:

      PrintLog("CDVDlinuz DVD:   Use Minimum Rate for: DVD-R");

      break;

    case 3:

      PrintLog("CDVDlinuz DVD:   Use Minimum Rate for: DVD-RW");

      break;

    case 9:

      PrintLog("CDVDlinuz DVD:   Use Minimum Rate for: DVD+RW");

      break;

    default:

      PrintLog("CDVDlinuz DVD:   Use Minimum Rate for: Unknown (%i)", layer.d.MinimumRate);

      break;

  } // ENDSWITCH- Displaying the Minimum (Spin?) Rate

  switch(layer.d.DiskSize) {

    case 0:

      PrintLog("CDVDlinuz DVD:   Physical Disk Size: 120mm");

      break;

    case 1:

      PrintLog("CDVDlinuz DVD:   Physical Disk Size: 80mm");

      break;

    default:

      PrintLog("CDVDlinuz DVD:   Physical Disk Size: Unknown (%i)", layer.d.DiskSize);

      break;

  } // ENDSWITCH- What's the Disk Size?

  switch(layer.d.LayerType) {

    case 1:

      PrintLog("CDVDlinuz DVD:   Layer Type: Read-Only");

      break;

    case 2:

      PrintLog("CDVDlinuz DVD:   Layer Type: Recordable");

      break;

    case 4:

      PrintLog("CDVDlinuz DVD:   Layer Type: Rewritable");

      break;

    default:

      PrintLog("CDVDlinuz DVD:   Layer Type: Unknown (%i)", layer.d.LayerType);

      break;

  } // ENDSWITCH- Displaying the Layer Type

  switch(layer.d.TrackPath) {

    case 0:

      PrintLog("CDVDlinuz DVD:   Track Path: PTP");

      break;

    case 1:

      PrintLog("CDVDlinuz DVD:   Track Path: OTP");

      break;

    default:

      PrintLog("CDVDlinuz DVD:   Track Path: Unknown (%i)", layer.d.TrackPath);

      break;

  } // ENDSWITCH- What's Track Path Layout?

  PrintLog("CDVDlinuz DVD:   Number of Layers: %i", layer.d.NumberOfLayers + 1);

  switch(layer.d.TrackDensity) {

    case 0:

      PrintLog("CDVDlinuz DVD:   Track Density: .74 m/track");

      break;

    case 1:

      PrintLog("CDVDlinuz DVD:   Track Density: .8 m/track");

      break;

    case 2:

      PrintLog("CDVDlinuz DVD:   Track Density: .615 m/track");

      break;

    default:

      PrintLog("CDVDlinuz DVD:   Track Density: Unknown (%i)", layer.d.TrackDensity);

      break;

  } // ENDSWITCH- Displaying the Layer Type

  switch(layer.d.LinearDensity) {

    case 0:

      PrintLog("CDVDlinuz DVD:   Linear Density: .267 m/bit");

      break;

    case 1:

      PrintLog("CDVDlinuz DVD:   Linear Density: .293 m/bit");

      break;

    case 2:

      PrintLog("CDVDlinuz DVD:   Linear Density: .409 to .435 m/bit");

      break;

    case 4:

      PrintLog("CDVDlinuz DVD:   Linear Density: .280 to .291 m/bit");

      break;

    case 8:

      PrintLog("CDVDlinuz DVD:   Linear Density: .353 m/bit");

      break;

    default:

      PrintLog("CDVDlinuz DVD:   Linear Density: Unknown (%i)", layer.d.LinearDensity);

      break;

  } // ENDSWITCH- Displaying the Book Type

  if(layer.d.StartingDataSector == 0x30000) {

    PrintLog("CDVDlinuz DVD:   Starting Sector: %lu (DVD-ROM, DVD-R, DVD-RW)",

             layer.d.StartingDataSector);

  } else if(layer.d.StartingDataSector == 0x31000) {

    PrintLog("CDVDlinuz DVD:   Starting Sector: %lu (DVD-RAM, DVD+RW)",

             layer.d.StartingDataSector);

  } else {

    PrintLog("CDVDlinuz DVD:   Starting Sector: %lu", layer.d.StartingDataSector);

  } // ENDLONGIF- What does the starting sector tell us?

  PrintLog("CDVDlinuz DVD:   End of Layer 0: %lu", layer.d.EndLayerZeroSector);

  PrintLog("CDVDlinuz DVD:   Ending Sector: %lu", layer.d.EndDataSector);

  if(layer.d.BCAFlag != 0)  PrintLog("CDVDlinuz DVD:   BCA data present");

#endif /* VERBOSE_DISC_INFO */



  boolresult = DeviceIoControl(devicehandle,

                               IOCTL_DVD_END_SESSION,

                               &sessionid,

                               sizeof(DVD_SESSION_ID),

                               NULL,

                               0,

                               &byteswritten,

                               &waitevent);

  errcode = FinishCommand(boolresult);

  if(errcode != 0) {

#ifdef VERBOSE_WARNING_DEVICE

    PrintLog("CDVDlinuz DVD:   Couldn't end the session!");

    PrintError("CDVDlinuz DVD", errcode);

#endif /* VERBOSE_WARNING_DEVICE */

  } // ENDIF- Couldn't end the user session? Report it.



  return(retval);

} // END DVDGetStructures()





s32 DVDreadTrack(u32 lsn, int mode, u8 *buffer) {

  LARGE_INTEGER targetpos;

  DWORD byteswritten;

  BOOL boolresult;

  DWORD errcode;



#ifdef VERBOSE_FUNCTION_DEVICE

  PrintLog("CDVDlinuz DVD: DVDreadTrack(%lu)", lsn);

#endif /* VERBOSE_FUNCTION_DEVICE */



  targetpos.QuadPart = lsn * 2048;

  waitevent.Offset = targetpos.LowPart;

  waitevent.OffsetHigh = targetpos.HighPart;



  boolresult = ReadFile(devicehandle,

                        buffer,

                        2048,

                        &byteswritten,

                        &waitevent);

  errcode = FinishCommand(boolresult);



  if(errcode != 0) {

#ifdef VERBOSE_WARNING_DEVICE

    PrintLog("CDVDlinuz DVD:   Couldn't read sector!");

    PrintError("CDVDlinuz DVD", errcode);

#endif /* VERBOSE_WARNING_DEVICE */

    return(-1);

  } // ENDIF- Trouble with the command? Report it.



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

             byteswritten, 2048);

    PrintError("CDVDlinuz CD", errcode);

#endif /* VERBOSE_WARNING_DEVICE */

    return(-1);

  } // ENDIF- Didn't get enough bytes? Report and Abort!



  return(0);

} // END DVDreadTrack()





s32 DVDgetTN(cdvdTN *cdvdtn) {

#ifdef VERBOSE_FUNCTION_DEVICE

  PrintLog("CDVDlinuz DVD: DVDgetTN()");

#endif /* VERBOSE_FUNCTION_DEVICE */



  if(cdvdtn != NULL) {

    cdvdtn->strack = 1;

    cdvdtn->etrack = 1;

  } // ENDIF- Does the user really want this data?

  return(0);

} // END DVDgetTN()





s32 DVDgetTD(u8 newtrack, cdvdTD *cdvdtd) {

#ifdef VERBOSE_FUNCTION_DEVICE

  PrintLog("CDVDlinuz DVD: DVDgetTD()");

#endif /* VERBOSE_FUNCTION_DEVICE */



  if((newtrack >= 2) && (newtrack != 0xAA))  return(-1); // Wrong track



  if(cdvdtd != NULL) {

    cdvdtd->type = 0;

    cdvdtd->lsn = layer.d.EndDataSector - layer.d.StartingDataSector + 1;

  } // ENDIF- Does the user really want this data?

  return(0);

} // END DVDgetTD()





s32 DVDgetDiskType() {

  char playstationname[] = "PLAYSTATION\0";

  int retval;

  s32 tempdisctype;

  char tempbuffer[2048];

  int i;



#ifdef VERBOSE_FUNCTION_DEVICE

  PrintLog("CDVDlinuz DVD: DVDgetDiskType()");

#endif /* VERBOSE_FUNCTION_DEVICE */



  retval = DVDGetStructures();

  if(retval < 0)  return(-1); // Can't get DVD structures? Not a DVD then.

  if(layer.d.EndDataSector == 0)  return(-1); // Missing info? Abort.



  retval = DVDreadTrack(16, CDVD_MODE_2048, tempbuffer);

  if(retval < 0) {

    return(-1);

  } // ENDIF- Couldn't read the ISO9660 volume track? Fail.



  tempdisctype = CDVD_TYPE_UNKNOWN;

  if(layer.d.NumberOfLayers == 0) {

#ifdef VERBOSE_DISC_INFO

    PrintLog("CDVDlinuz DVD:   Found Single-Sided DVD.");

#endif /* VERBOSE_DISC_INFO */

    disctype = CDVD_TYPE_DETCTDVDS;

  } else {

#ifdef VERBOSE_DISC_INFO

    PrintLog("CDVDlinuz DVD:   Found Dual-Sided DVD.");

#endif /* VERBOSE_DISC_INFO */

    disctype = CDVD_TYPE_DETCTDVDD;

  } // ENDIF- Are we looking at a single layer DVD? (NumberOfLayers + 1)



  i = 0;

  while((playstationname[i] != 0) &&

        (playstationname[i] == tempbuffer[8 + i]))  i++;

  if(playstationname[i] == 0) {

#ifdef VERBOSE_DISC_TYPE

    PrintLog("CDVDlinuz DVD:   Found Playstation 2 DVD.");

#endif /* VERBOSE_DISC_TYPE */

    tempdisctype = CDVD_TYPE_PS2DVD;

  } else {

#ifdef VERBOSE_DISC_TYPE

    PrintLog("CDVDlinuz DVD:   Guessing it's a Video DVD.");

#endif /* VERBOSE_DISC_TYPE */

    tempdisctype = CDVD_TYPE_DVDV;

  } // ENDIF- Is this a playstation disc?



  for(i = 0; i < 2048; i++)  tocbuffer[i] = 0;



  if(layer.d.NumberOfLayers == 0) {

    tocbuffer[0] = 0x04;

    tocbuffer[4] = 0x86;

    tocbuffer[5] = 0x72;

  } else {

    tocbuffer[0] = 0x24;

    tocbuffer[4] = 0x41;

    tocbuffer[5] = 0x95;

  } // ENDIF- Are we looking at a single layer DVD? (NumberOfLayers + 1)



  tocbuffer[1] = 0x02;

  tocbuffer[2] = 0xF2;

  tocbuffer[3] = 0x00;



  tocbuffer[16] = 0x00;

  tocbuffer[17] = 0x03;

  tocbuffer[18] = 0x00;

  tocbuffer[19] = 0x00;



  disctype = tempdisctype;

  return(disctype);

} // END DVDgetDiskType()

