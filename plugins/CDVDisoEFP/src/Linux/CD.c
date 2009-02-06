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

#include <errno.h> // errno
#include <stddef.h> // NULL
#include <string.h> // strerror()
#include <fcntl.h> // open()
#include <sys/ioctl.h> // ioctl()
#include <sys/stat.h> // open()
#include <sys/types.h> // lseek(), open()
#include <unistd.h> // close(), lseek(), (sleep())

#include <linux/cdrom.h> // CD/DVD based ioctl() and defines.

#include "../convert.h"
#include "logfile.h"
#include "device.h"
#include "CD.h"


// Constants
u8 *playstationcdname = "PLAYSTATION\0";
u8 *ps1name = "CD-XA001\0";

// CD-ROM temp storage structures (see linux/cdrom.h for details)
struct cdrom_tochdr cdheader;
struct cdrom_tocentry cdtrack;
struct cdrom_subchnl subchannel;
u8 cdtempbuffer[2352];

int cdmode; // mode of last CDVDreadTrack call (important for CDs)


// Internal Functions

void InitCDSectorInfo() {
  cdmode = -1;
} // END InitSectorInfo();


// Function Calls from CDVD.c

void InitCDInfo() {
  InitCDSectorInfo();
} // END InitDiscType()

s32 CDreadTrack(u32 lsn, int mode, u8 *buffer) {
  s32 s32result;

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: CDreadTrack(%i)", lsn);
#endif /* VERBOSE_FUNCTION */

  s32result = 0;

  if(buffer == NULL)  return(-1);

  // The CD way of figuring out where to read.
  LBAtoMSF(lsn, buffer);

  switch(mode) {
    case CDVD_MODE_2048:
    case CDVD_MODE_2328:
    case CDVD_MODE_2340:
    case CDVD_MODE_2352:
      errno = 4; // Interrupted system call... (simulated the first time)
      while(errno == 4) {
        errno = 0;
        s32result = ioctl(devicehandle, CDROMREADRAW, buffer);
      } // ENDWHILE- Continually being interrupted by the system...
      break;
    case CDVD_MODE_2368: // Unimplemented... as yet.
    default:
#ifdef VERBOSE_WARNINGS
      PrintLog("CDVD driver:   Unknown Mode %i", mode);
#endif /* VERBOSE_WARNINGS */
      return(-1); // Illegal Read Mode? Abort
      break;
  } // ENDSWITCH- Which read mode should we choose?
  if((s32result == -1) || (errno != 0)) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD driver:   Error reading CD: %i:%s", errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
    InitCDSectorInfo();
    return(-1);
  } // ENDIF- Trouble getting a track count?

  cdmode = mode; // Save mode for buffer positioning later.
  return(0); // Call accomplished
} // END CDreadTrack()

s32 CDgetBufferOffset() {
#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: CDgetBufferOffset()");
#endif /* VERBOSE_FUNCTION */

  switch(cdmode) {
      case CDVD_MODE_2048:
        return(0+24);
      case CDVD_MODE_2328:
        return(0+24);
      case CDVD_MODE_2340:
        return(0+12);
      case CDVD_MODE_2352:
        return(0+0);
      case CDVD_MODE_2368: // Unimplemented... as yet.
      default: 
#ifdef VERBOSE_WARNINGS
        PrintLog("CDVD driver:   Unknown Mode %i", cdmode);
#endif /* VERBOSE_WARNINGS */
        return(0); // Not to worry. for now.
  } // ENDSWITCH- where should we put the buffer pointer?
} // END CDgetBuffer()

// I, personally, don't see the big deal with SubQ
// However, sooner or later I'll incorporate it into the Cache Buffer system
// (backward compatibility, and all that)
s32 CDreadSubQ(u32 lsn, cdvdSubQ *subq) {
  int tempmode;
  s32 s32result;

  s32result = 0;

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: CDreadSubQ()");
#endif /* VERBOSE_FUNCTION */

  tempmode = cdmode;
  if(tempmode == -1)  tempmode = CDVD_MODE_2352;
  CDreadTrack(lsn, tempmode, cdtempbuffer);
  if((s32result == -1) || (errno != 0)) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD driver:   Error prepping CD SubQ: %i:%s", errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
    return(s32result);
  } // ENDIF- Trouble?

  subchannel.cdsc_format = CDROM_MSF;
  s32result = ioctl(devicehandle, CDROMSUBCHNL, &subchannel);
  if((s32result == -1) || (errno != 0)) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD driver:   Error reading CD SubQ: %i:%s", errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
    return(s32result);
  } // ENDIF- Trouble?

  if(subq != NULL) {
    subq->mode = subchannel.cdsc_adr;
    subq->ctrl = subchannel.cdsc_ctrl;
    subq->trackNum = subchannel.cdsc_trk;
    subq->trackIndex = subchannel.cdsc_ind;
    subq->trackM = subchannel.cdsc_reladdr.msf.minute;
    subq->trackS = subchannel.cdsc_reladdr.msf.second;
    subq->trackF = subchannel.cdsc_reladdr.msf.frame;
    subq->discM = subchannel.cdsc_absaddr.msf.minute;
    subq->discS = subchannel.cdsc_absaddr.msf.second;
    subq->discF = subchannel.cdsc_absaddr.msf.frame;
  } // ENDIF- Did the caller want all this data?

  return(0);
} // END CDVDreadSubQ()

s32 CDgetTN(cdvdTN *cdvdtn) {
#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: CDgetTN()");
#endif /* VERBOSE_FUNCTION */

  if(cdvdtn != NULL) {
    cdvdtn->strack = cdheader.cdth_trk0;
    cdvdtn->etrack = cdheader.cdth_trk1;
  } // ENDIF- programmer actually WANTS this info?

  return(0); // Call accomplished
} // END CDVDgetTN()

s32 CDgetTD(u8 newtrack, cdvdTD *cdvdtd) {
  u8 j;
  u16 k;
  char temptime[3];

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: CDgetTD()");
#endif /* VERBOSE_FUNCTION */

  j = newtrack;
  if(j == CDROM_LEADOUT)  j = 0;

  if(j == 0) {
    k = 27;
  } else {
    k = j * 10 + 37;
  } // ENDIF- Where to start hunting for this number?

  if(cdvdtd != NULL) {
    cdvdtd->type = tocbuffer[j*10 + 30];

    temptime[0] = BCDTOHEX(tocbuffer[k]);
    temptime[1] = BCDTOHEX(tocbuffer[k + 1]);
    temptime[2] = BCDTOHEX(tocbuffer[k + 2]);
    cdvdtd->lsn = MSFtoLBA(temptime);
  } // ENDIF- Does the caller REALLY want this data?

  return(0); // Call accomplished
} // END CDVDgetTD()

s32 CALLBACK CDgetDiskType(s32 ioctldisktype) {
  s32 offset;
  s32 s32result;
  int i;
  u8 j;
  int tempdisctype;
  
  offset = 0;
  errno = 0;
  i = 0;
  j = 0;
  tempdisctype = CDVD_TYPE_UNKNOWN;

#ifdef VERBOSE_FUNCTION
  PrintLog("CDVD driver: CDgetDiskType()");
#endif /* VERBOSE_FUNCTION */

  s32result = CDreadTrack(16, CDVD_MODE_2352, cdtempbuffer);
  if((s32result != 0) || (errno != 0)) {
    return(-1);
  } // ENDIF- Cannot read the CD's ISO9660 volume sector? Abort 
  disctype = CDVD_TYPE_DETCTCD;

  switch(ioctldisktype) {
    case CDS_AUDIO:
#ifdef VERBOSE_DISC_TYPE
      PrintLog("CDVD driver: Detected CDDA Audio disc.");
#endif /* VERBOSE_DISC_TYPE */
      tempdisctype = CDVD_TYPE_CDDA;
      tocbuffer[0] = 0x01;
      break;

    case CDS_DATA_1:
    case CDS_MIXED:
#ifdef VERBOSE_DISC_TYPE
      PrintLog("CDVD driver: Detected CD disc.");
#endif /* VERBOSE_DISC_TYPE */
      tocbuffer[0] = 0x41;

      CDreadTrack(16, CDVD_MODE_2048, cdtempbuffer);
      offset = CDgetBufferOffset();
      i = 0;
      while((*(playstationcdname + i) != 0) &&
            (*(playstationcdname + i) == cdtempbuffer[offset + 8 + i]))  i++;
      if(*(playstationcdname + i) == 0) {
        i = 0;
        while((*(ps1name + i) != 0) &&
              (*(ps1name + i) == cdtempbuffer[offset + 1024 + i]))  i++;
        if(*(ps1name + i) == 0) {
#ifdef VERBOSE_DISC_TYPE
          PrintLog("CDVD driver: Detected Playstation CD disc.");
#endif /* VERBOSE_DISC_TYPE */
          tempdisctype = CDVD_TYPE_PSCD;
        } else {
#ifdef VERBOSE_DISC_TYPE
          PrintLog("CDVD driver: Detected Playstation 2 CD disc.");
#endif /* VERBOSE_DISC_TYPE */
          tempdisctype = CDVD_TYPE_PS2CD;
        } // ENDIF- Did we find the CD ident? (For Playstation 1 CDs)
      } else {
        tempdisctype = CDVD_TYPE_UNKNOWN;
      } // ENDIF- Did we find the Playstation name?
      break;

    default:
      return(-1);
  } // ENDSWITCH- What has ioctl disc type come up with?

  // Collect TN data
  cdheader.cdth_trk0 = 0;
  cdheader.cdth_trk1 = 0;

  s32result = ioctl(devicehandle, CDROMREADTOCHDR, &cdheader);
  if((s32result == -1) || (errno != 0)) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD driver:   Error reading TN: (%i) %i:%s",
             s32result, errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
    cdheader.cdth_trk0 = 1;
    cdheader.cdth_trk1 = 1;
  } // ENDIF- Failed to read in track count? Assume 1 track.
#ifdef VERBOSE_DISC_INFO
  PrintLog("CDVD driver: Track Number Range: %i-%i",
           cdheader.cdth_trk0, cdheader.cdth_trk1);
#endif /* VERBOSE_DISC_INFO */
  tocbuffer[2] = 0xA0;
  tocbuffer[7] = HEXTOBCD(cdheader.cdth_trk0);
  tocbuffer[12] = 0xA1;
  tocbuffer[17] = HEXTOBCD(cdheader.cdth_trk1);

  // Collect disc TD data
  cdtrack.cdte_track = CDROM_LEADOUT;
  cdtrack.cdte_format = CDROM_LBA;
  s32result = ioctl(devicehandle, CDROMREADTOCENTRY, &cdtrack);
  if((s32result == -1) || (errno != 0)) {
#ifdef VERBOSE_WARNINGS
    PrintLog("CDVD driver:   Error reading TD for disc: (%i) %i:%s",
             s32result, errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
    return(-1);
  } // ENDIF- Trouble getting a track count?

  LBAtoMSF(cdtrack.cdte_addr.lba, &tocbuffer[27]);
#ifdef VERBOSE_DISC_INFO
  PrintLog("CDVD driver: Total Time: %i:%i",
           tocbuffer[27], tocbuffer[28]);
#endif /* VERBOSE_DISC_INFO */
  tocbuffer[27] = HEXTOBCD(tocbuffer[27]);
  tocbuffer[28] = HEXTOBCD(tocbuffer[28]);
  tocbuffer[29] = HEXTOBCD(tocbuffer[29]);

  // Collect track TD data
  for(j = cdheader.cdth_trk0; j <= cdheader.cdth_trk1; j++) {
    cdtrack.cdte_track = j; // j-1?
    cdtrack.cdte_format = CDROM_LBA;
    s32result = ioctl(devicehandle, CDROMREADTOCENTRY, &cdtrack);
    if((s32result == -1) || (errno != 0)) {
#ifdef VERBOSE_WARNINGS
      PrintLog("CDVD driver:   Error reading TD for track %i: (%i) %i:%s",
               j, s32result, errno, strerror(errno));
#endif /* VERBOSE_WARNINGS */
      // No more here...

    } else {
      LBAtoMSF(cdtrack.cdte_addr.lba, &tocbuffer[j*10 + 37]);
#ifdef VERBOSE_DISC_INFO
      PrintLog("CDVD driver: Track %i:  Data Mode %i   Disc Start Time:%i:%i.%i\n",
               j,
               cdtrack.cdte_datamode,
               tocbuffer[j*10+37],
               tocbuffer[j*10+38],
               tocbuffer[j*10+39]);
#endif /* VERBOSE_DISC_INFO */
      tocbuffer[j*10 + 30] = cdtrack.cdte_datamode;
      tocbuffer[j*10 + 32] = HEXTOBCD(j);
      tocbuffer[j*10 + 37] = HEXTOBCD(tocbuffer[j*10 + 37]);
      tocbuffer[j*10 + 38] = HEXTOBCD(tocbuffer[j*10 + 38]);
      tocbuffer[j*10 + 39] = HEXTOBCD(tocbuffer[j*10 + 39]);
    } // ENDIF- Trouble getting a track count?
  } // NEXT j- Reading each track's info in turn

  errno = 0;
  disctype = tempdisctype; // Trigger the fact we have the info (finally)
  return(disctype);
} // END CDVDgetDiskType()
