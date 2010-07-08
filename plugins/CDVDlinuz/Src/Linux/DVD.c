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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */

#include <errno.h> // errno
#include <stddef.h> // NULL
#include <stdio.h> // sprintf()
#include <string.h> // strerror(), memset(), memcpy()
#include <fcntl.h> // open()
#include <sys/ioctl.h> // ioctl()
#include <sys/stat.h> // open()
#include <sys/types.h> // lseek(), open()
#include <unistd.h> // close(), lseek(), (sleep())

#include <linux/cdrom.h> // CD/DVD based ioctl() and defines.

#include "logfile.h"
#include "device.h"
#include "DVD.h"

#include "../PS2Etypes.h" // u8, u32


// Constants
u8 *playstationname = "PLAYSTATION\0";

// DVD storage structures (see linux/cdrom.h for details)
dvd_struct dvdphysical;
dvd_struct dvdcopyright[DVD_LAYERS];
dvd_struct dvdbca;
dvd_struct dvdmanufact[DVD_LAYERS];

u32 dvdlastlsn;
u8 dvdtempbuffer[2064];


// Internal Functions

void InitDVDSectorInfo() {
  dvdlastlsn = 0xffffffff;
} // END InitSectorInfo();

void HexDump(u8 *strptr, u8 count) {
  int i;
  u8 ch[2];
  char hexline[81];
  int hexlinepos;

  ch[1] = 0;

  if(count == 0)  count = 16;
  if((count < 1) || (count > 16))  return;

  hexlinepos = 0;
  hexlinepos += sprintf(&hexline[hexlinepos], "CDVD driver: ");

  for(i = 0; i < count; i++) {
    hexlinepos += sprintf(&hexline[hexlinepos], "%.2x ", (*(strptr + i)) * 1);
  } // NEXT i- printing each new Hex number

  for(i = 0; i < count; i++) {
    if((*(strptr + i) < 32) || (*(strptr + i) > 127)) {
      hexlinepos += sprintf(&hexline[hexlinepos], ".");
    } else {
      ch[0] = *(strptr + i);
      hexlinepos += sprintf(&hexline[hexlinepos], "%s", ch);
    } // ENDIF- Is this an unprintable character?
  } // NEXT i- printing each new character
  PrintLog(hexline);
} // ENDIF HexDump()


//// DVD Structure Functions

s32 DVDreadPhysical() {
  s32 s32result;
  u8 i;

  errno = 0;

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: DVDreadPhysical()\n");
#endif /* VERBOSE_FUNCTION */

  memset(&dvdphysical, 0, sizeof(dvd_struct));
  dvdphysical.type = DVD_STRUCT_PHYSICAL;

  i = DVD_LAYERS;
  while(i > 0) {
    i--;
    dvdphysical.physical.layer_num = i;
    errno = 0;
    s32result = ioctl(devicehandle, DVD_READ_STRUCT, &dvdphysical);
  } // ENDWHILE- reading in all physical layers...

  if((s32result == -1) || (errno != 0)) {
    dvdphysical.type = 0xFF;
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD driver:   Error getting Physical structure: (%i) %i:%s", s32result, errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
    return(-1);
  } // ENDIF- Problem with reading Layer 0 of the physical data? Abort

  i = 3;
  while((i > 0) && (dvdphysical.physical.layer[i].end_sector == 0))  i--;
  dvdphysical.physical.layer_num = i;

#ifdef VERBOSE_DISC_INFO
  PrintLog("CDVD driver: Physical Characteristics");
  PrintLog("CDVD driver:   Number of Layers: %i",
         (s32) dvdphysical.physical.layer_num + 1);
  for(i = 0; i <= dvdphysical.physical.layer_num; i++) {
    PrintLog("CDVD driver:     Layer Number %i", i);
    switch(dvdphysical.physical.layer[i].book_type) {
      case 0:
        PrintLog("CDVD driver:       Book Type: DVD-ROM");
        break;
      case 1:
        PrintLog("CDVD driver:       Book Type: DVD-RAM");
        break;
      case 2:
        PrintLog("CDVD driver:       Book Type: DVD-R");
        break;
      case 3:
        PrintLog("CDVD driver:       Book Type: DVD-RW");
        break;
      case 9:
        PrintLog("CDVD driver:       Book Type: DVD+RW");
        break;
      default:
        PrintLog("CDVD driver:       Book Type: Unknown (%i)",
                 dvdphysical.physical.layer[i].book_type);
        break;
    } // ENDSWITCH- Displaying the Book Type
    PrintLog("CDVD driver:       Book Version %i",
             dvdphysical.physical.layer[i].book_version);
    switch(dvdphysical.physical.layer[i].min_rate) {
      case 0:
        PrintLog("CDVD driver:       Use Minimum Rate for: DVD-ROM");
        break;
      case 1:
        PrintLog("CDVD driver:       Use Minimum Rate for: DVD-RAM");
        break;
      case 2:
        PrintLog("CDVD driver:       Use Minimum Rate for: DVD-R");
        break;
      case 3:
        PrintLog("CDVD driver:       Use Minimum Rate for: DVD-RW");
        break;
      case 9:
        PrintLog("CDVD driver:       Use Minimum Rate for: DVD+RW");
        break;
      default:
        PrintLog("CDVD driver:       Use Minimum Rate for: Unknown (%i)",
                 dvdphysical.physical.layer[i].min_rate);
        break;
    } // ENDSWITCH- Displaying the Minimum (Spin?) Rate
    switch(dvdphysical.physical.layer[i].disc_size) {
      case 0:
        PrintLog("CDVD driver:       Physical Disk Size: 120mm");
        break;
      case 1:
        PrintLog("CDVD driver:       Physical Disk Size: 80mm");
        break;
      default:
        PrintLog("CDVD driver:       Physical Disk Size: Unknown (%i)",
                 dvdphysical.physical.layer[i].disc_size);
        break;
    } // ENDSWITCH- What's the Disk Size?
    switch(dvdphysical.physical.layer[i].layer_type) {
      case 1:
        PrintLog("CDVD driver:       Layer Type: Read-Only");
        break;
      case 2:
        PrintLog("CDVD driver:       Layer Type: Recordable");
        break;
      case 4:
        PrintLog("CDVD driver:       Layer Type: Rewritable");
        break;
      default:
        PrintLog("CDVD driver:       Layer Type: Unknown (%i)",
                 dvdphysical.physical.layer[i].layer_type);
        break;
    } // ENDSWITCH- Displaying the Layer Type
    switch(dvdphysical.physical.layer[i].track_path) {
      case 0:
        PrintLog("CDVD driver:       Track Path: PTP");
        break;
      case 1:
        PrintLog("CDVD driver:       Track Path: OTP");
        break;
      default:
        PrintLog("CDVD driver:       Track Path: Unknown (%i)",
                 dvdphysical.physical.layer[i].track_path);
        break;
    } // ENDSWITCH- What's Track Path Layout?
    // PrintLog("CDVD driver:       Disc Size %i   Layer Type %i   Track Path %i   Nlayers %i",
    //          dvdphysical.physical.layer[i].nlayers);
    switch(dvdphysical.physical.layer[i].track_density) {
      case 0:
        PrintLog("CDVD driver:       Track Density: .74 m/track");
        break;
      case 1:
        PrintLog("CDVD driver:       Track Density: .8 m/track");
        break;
      case 2:
        PrintLog("CDVD driver:       Track Density: .615 m/track");
        break;
      default:
        PrintLog("CDVD driver:       Track Density: Unknown (%i)",
                 dvdphysical.physical.layer[i].track_density);
        break;
    } // ENDSWITCH- Displaying the Track Density
    switch(dvdphysical.physical.layer[i].linear_density) {
      case 0:
        PrintLog("CDVD driver:       Linear Density: .267 m/bit");
        break;
      case 1:
        PrintLog("CDVD driver:       Linear Density: .293 m/bit");
        break;
      case 2:
        PrintLog("CDVD driver:       Linear Density: .409 to .435 m/bit");
        break;
      case 4:
        PrintLog("CDVD driver:       Linear Density: .280 to .291 m/bit");
        break;
      case 8:
        PrintLog("CDVD driver:       Linear Density: .353 m/bit");
        break;
      default:
        PrintLog("CDVD driver:       Linear Density: Unknown (%i)",
                 dvdphysical.physical.layer[i].linear_density);
        break;
    } // ENDSWITCH- Displaying the Linear Density
    if(dvdphysical.physical.layer[i].start_sector == 0x30000) {
      PrintLog("CDVD driver:       Starting Sector: %lu (DVD-ROM, DVD-R, DVD-RW)",
               dvdphysical.physical.layer[i].start_sector);
    } else if(dvdphysical.physical.layer[i].start_sector == 0x31000) {
      PrintLog("CDVD driver:       Starting Sector: %lu (DVD-RAM, DVD+RW)",
               dvdphysical.physical.layer[i].start_sector);
    } else {
      PrintLog("CDVD driver:       Starting Sector: %lu",
               dvdphysical.physical.layer[i].start_sector);
    } // ENDLONGIF- What does the starting sector tell us?
    PrintLog("CDVD driver:       End of Layer 0: %lu",
             dvdphysical.physical.layer[i].end_sector_l0);
    PrintLog("CDVD driver:       Ending Sector: %lu",
             dvdphysical.physical.layer[i].end_sector);
    if(dvdphysical.physical.layer[i].bca != 0)
      PrintLog("CDVD driver:       BCA data present");
  } // NEXT i- Work our way through each layer...
#endif /* VERBOSE_DISC_INFO */

  return(0); // Success. Physical data stored for perusal.
} // END DVDreadPhysical()

s32 DVDreadCopyright() {
  s32 s32result;
  u8 i;
  int successflag;

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: DVDreadCopyright()");
#endif /* VERBOSE_FUNCTION */

  successflag = 0;

  for(i = 0; i <= dvdphysical.physical.layer_num; i++) {
    memset(&dvdcopyright[i], 0, sizeof(dvd_struct));
    dvdcopyright[i].type = DVD_STRUCT_COPYRIGHT;
    dvdcopyright[i].copyright.layer_num = i;
    errno = 0;
    s32result = ioctl(devicehandle, DVD_READ_STRUCT, &dvdcopyright[i]);
    if(s32result == 0) {
      successflag = 1;
    } else {
      dvdcopyright[i].type = 0xFF;
    } // ENDIF-
  } // NEXT i- Getting copyright data for every known layer

  if(successflag == 0) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD driver:   Error getting Copyright info: (%i) %i:%s", s32result, errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
    return(-1);
  } // ENDIF- Problem with read of physical data?

#ifdef VERBOSE_DISC_INFO
  PrintLog("CDVD driver: Copyright Information\n");
  for(i = 0; i <= dvdphysical.physical.layer_num; i++) {
    if(dvdcopyright[i].type != 0xFF) {
      PrintLog("CDVD driver:   Layer Number %i   CPST %i   RMI %i",
               dvdcopyright[i].copyright.layer_num,
               dvdcopyright[i].copyright.cpst,
               dvdcopyright[i].copyright.rmi);
    } // ENDIF- Were we successful reading this one?
  } // NEXT i- Printing out all copyright info found...
#endif /* VERBOSE_DISC_INFO */

  errno = 0;
  return(0); // Success. Copyright data stored for perusal.
} // END DVDreadCopyright()

s32 DVDreadBCA() {
  s32 s32result;
  int i;

  i = 0;
  errno = 0;

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: DVDreadBCA()");
#endif /* VERBOSE_FUNCTION */

  memset(&dvdbca, 0, sizeof(dvd_struct));
  dvdbca.type = DVD_STRUCT_BCA;
  s32result = ioctl(devicehandle, DVD_READ_STRUCT, &dvdbca);
  if((s32result == -1) || (errno != 0)) {
    dvdbca.type = 0xFF;
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD driver:   Error getting BCA: (%i) %i:%s", s32result, errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
    return(-1);
  } // ENDIF- Problem with read of physical data?

#ifdef VERBOSE_DISC_INFO
  PrintLog("CDVD driver: BCA   Length %i   Value:",
         dvdbca.bca.len);
  for(i = 0; i < 188-15; i += 16) {
    HexDump(dvdbca.bca.value+i, 16);
  } // NEXT i- dumping whole key data
#endif /* VERBOSE_DISC_INFO */

  return(0); // Success. BCA data stored for perusal.
} // END DVDreadBCA()

s32 DVDreadManufact() {
  s32 s32result;
  u8 i;
  int successflag;
  int j;

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: DVDreadManufact()");
#endif /* VERBOSE_FUNCTION */

  j = 0;
  successflag = 0;

  for(i = 0; i <= dvdphysical.physical.layer_num; i++) {
    memset(&dvdmanufact[i], 0, sizeof(dvd_struct));
    dvdmanufact[i].type = DVD_STRUCT_MANUFACT;
    dvdmanufact[i].manufact.layer_num = i;
    errno = 0;
    s32result = ioctl(devicehandle, DVD_READ_STRUCT, &dvdmanufact[i]);
    if((s32result != 0) || (errno != 0)) {
      dvdmanufact[i].type = 0xFF;
    } else {
      successflag = 1;
    } // ENDIF- Did we fail to read in some manufacturer data?
  } // NEXT i- Collecting manufacturer data from all layers

  if(successflag == 0) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD driver:   Error getting Manufact: (%i) %i:%s", s32result, errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
    return(-1);
  } // ENDIF- Problem with read of physical data?

#ifdef VERBOSE_DISC_INFO
  PrintLog("CDVD driver: Manufact Data");
  for(i = 0; i <= dvdphysical.physical.layer_num; i++) {
    if(dvdmanufact[i].type != 0xFF) {
      PrintLog("CDVD driver:   Layer %i   Length %i   Value:",
               dvdmanufact[i].manufact.layer_num,
               dvdmanufact[i].manufact.len);
      for(j = 0; j < 128-15; j += 16) {
        HexDump(dvdmanufact[i].manufact.value+j, 16);
      } // NEXT j- dumping whole key data
    } // ENDIF- Do we have data at this layer?
  } // NEXT i- Running through all the layers
#endif /* VERBOSE_DISC_INFO */

  errno = 0;
  return(0); // Success. Manufacturer's data stored for perusal.
} // END DVDreadManufact()


// External Functions

// Function Calls from CDVD.c

void InitDVDInfo() {
  int j;

  dvdphysical.type = 0xFF; // Using for empty=0xff, full!=0xff test
  dvdbca.type = 0xFF;
  for(j = 0; j < DVD_LAYERS; j++) {
    dvdcopyright[j].type = 0xFF;
    dvdmanufact[j].type = 0xFF;
  } // NEXT j- Zeroing each layer of data
  InitDVDSectorInfo();
} // END InitDiscType()

s32 DVDreadTrack(u32 lsn, int mode, u8 *buffer) {
  s32 s32result;
  off64_t offsettarget;
  off64_t offsetresult;

  errno = 0;
  s32result = 0;
  offsetresult = 0;

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: DVDreadTrack(%i)", lsn);
#endif /* VERBOSE_FUNCTION */

  if(lsn != dvdlastlsn + 1) {
    offsettarget = lsn;
    offsettarget *= 2048;
    errno = 4;
    while(errno == 4) {
      errno = 0;
      offsetresult = lseek64(devicehandle, offsettarget, SEEK_SET);
    } // ENDWHILE- waiting for the system interruptions to cease.
    if((offsetresult < 0) || (errno != 0)) {
#ifdef VERBOSE_WARNINGS
      PrintLog("CDVD driver:   Error on seek: %i:%s", errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
      InitDVDSectorInfo();
      return(-1);
    } // ENDIF- trouble with seek? Reset pointer and abort
  } // ENDIF- Do we have to seek a new position to read?

  errno = 4;
  while(errno == 4) {
    errno = 0;
    s32result = read(devicehandle, buffer, 2048);
  } // ENDWHILE- waiting for the system interruptions to cease.
  if((s32result != 2048) || (errno != 0)) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD driver:   DVD Short Block, Size: %i", s32result);
    PrintLog("CDVD driver:     Error: %i:%s", errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
    InitDVDSectorInfo();
    return(-1);
  } // ENDIF- Trouble reading the data? Reset pointer and abort

  dvdlastlsn = lsn;
  return(0); // Call accomplished
} // END DVDreadTrack()

s32 DVDgetTN(cdvdTN *cdvdtn) {
#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: CDVDgetTN()");
#endif /* VERBOSE_FUNCTION */

  // Going to treat this as one large track for now.
  if(cdvdtn != NULL) {
    cdvdtn->strack = 1;
    cdvdtn->etrack = 1;
  } // ENDIF- programmer actually WANTS this info?

  return(0); // Call accomplished
} // END DVDgetTN()

s32 DVDgetTD(u8 newtrack, cdvdTD *cdvdtd) {
#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: CDVDgetTD()");
#endif /* VERBOSE_FUNCTION */

  if((newtrack >= 2) && (newtrack != CDROM_LEADOUT))  return(-1);

  if(cdvdtd != NULL) {
    cdvdtd->lsn = dvdphysical.physical.layer[0].end_sector
                - dvdphysical.physical.layer[0].start_sector
                + 1;
    cdvdtd->type = CDVD_MODE_2048;
  } // ENDIF- Does the caller REALLY want this data?

  return(0); // Call accomplished
} // END DVDgetTD()

s32 DVDgetDiskType(s32 ioctldisktype) {
  s32 s32result;
  int i;
  s32 tempdisctype;

  errno = 0;
  s32result = 0;
  i = 0;
  tempdisctype = CDVD_TYPE_UNKNOWN;

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: DVDgetDiskType()");
#endif /* VERBOSE_FUNCTION */

  if((ioctldisktype != CDS_DATA_1) && (ioctldisktype != CDS_MIXED)) {
    return(-1);
  } // ENDIF- Not a data disc we know of? Abort then

  s32result = DVDreadPhysical();
  if((s32result != 0) || (errno != 0)) {
    return(-1);
  } // ENDIF- Error reading the DVD physical structure? Not a DVD after all.

  if(dvdphysical.physical.layer[0].end_sector >= (2048*1024)) {
#ifdef VERBOSE_DISC_TYPE
    PrintLog("CDVD driver: DVD Found (Dual-Sided)");
#endif /* VERBOSE_DISC_TYPE */
    disctype = CDVD_TYPE_DETCTDVDD;
  } else {
#ifdef VERBOSE_DISC_TYPE
    PrintLog("CDVD driver: DVD Found (Single-Sided)");
#endif /* VERBOSE_DISC_TYPE */
    disctype = CDVD_TYPE_DETCTDVDS;
  } // ENDIF- Definitely a DVD. Size Test?

  // Read in the rest of the structures...
  DVDreadCopyright();
  DVDreadBCA();
  DVDreadManufact();

  // Test for "Playstation" header
  s32result = DVDreadTrack(16, CDVD_MODE_2048, dvdtempbuffer);
  if(s32result != 0) {
    return(-1);
  } else {
    i = 0;
    while((*(playstationname + i) != 0) &&
          (*(playstationname + i) == dvdtempbuffer[8 + i])) {
      i++;
    } // ENDWHILE- Checking each letter of PLAYSTATION name for a match
    if(*(playstationname + i) == 0) {
#ifdef VERBOSE_DISC_TYPE
      PrintLog("CDVD driver: Detected Playstation 2 DVD");
#endif /* VERBOSE_DISC_TYPE */
      tempdisctype = CDVD_TYPE_PS2DVD;
    } else {
#ifdef VERBOSE_DISC_TYPE
      PrintLog("CDVD driver: Guessing it's a Video DVD");
#endif /* VERBOSE_DISC_TYPE */
      tempdisctype = CDVD_TYPE_DVDV;
    } // ENDIF- Did we find the Playstation name?
  } // ENDIF- Error reading disc volume information? Invalidate Disc

  if(dvdphysical.physical.layer[0].end_sector >= (2048*1024)) {
    tocbuffer[0] = 0x24; // Dual-Sided DVD
    tocbuffer[4] = 0x41;
    tocbuffer[5] = 0x95;
  } else {
    tocbuffer[0] = 0x04; // Single-Sided DVD
    tocbuffer[4] = 0x86;
    tocbuffer[5] = 0x72;
  } // ENDIF- Are there too many sectors for a single-sided disc?

  tocbuffer[1] = 0x02;
  tocbuffer[2] = 0xF2;
  tocbuffer[3] = 0x00;

  tocbuffer[16] = 0x00;
  tocbuffer[17] = 0x03;
  tocbuffer[18] = 0x00;
  tocbuffer[19] = 0x00;

  disctype = tempdisctype; // Triggers the fact the other info is available
  return(disctype);
} // END DVDgetDiskType()
