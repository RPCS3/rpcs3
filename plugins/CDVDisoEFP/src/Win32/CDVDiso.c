/*  CDVDiso.c
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


#include <windows.h> // BOOL, CALLBACK, APIENTRY
#include <windef.h> // NULL

#define CDVDdefs
#include "../PS2Edefs.h"

#include "conf.h"
#include "actualfile.h"
#include "../isofile.h"
#include "logfile.h"
#include "../convert.h"
#include "../version.h"
#include "screens.h"
#include "mainbox.h" // Initialize mainboxwindow
#include "progressbox.h" // Initialize progressboxwindow
#include "conversionbox.h" // Initialize conversionboxwindow
#include "devicebox.h" // Initialize deviceboxwindow
#include "CDVDiso.h"


struct IsoFile *isofile;
char isobuffer[2448];
char isocdcheck[2048];
int isomode;
int deviceopencount;

HINSTANCE progmodule;


BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD param,
                      LPVOID reserved) {


  switch(param) {
    case DLL_PROCESS_ATTACH:
      progmodule = hModule;
      // mainboxwindow = NULL;
      // progressboxwindow = NULL;
      // conversionboxwindow = NULL;
      // deviceboxwindow = NULL;
      return(TRUE);
      break;
    case DLL_PROCESS_DETACH:
      // CDVDshutdown();
      return(TRUE);
      break;
    case DLL_THREAD_ATTACH:
      return(TRUE);
      break;
    case DLL_THREAD_DETACH:
      return(TRUE);
      break;
  } // ENDSWITCH param- What does the OS want with us?

  return(FALSE); // Wasn't on list? Wasn't handled.
} // END DllMain()


char* CALLBACK PS2EgetLibName() {
  return(libname);
} // END PS2EgetLibName()


u32 CALLBACK PS2EgetLibType() {
  return(PS2E_LT_CDVD);
} // END PS2getLibType()


u32 CALLBACK PS2EgetLibVersion2(u32 type) {
  return((version << 16) | (revision << 8) | build);
} // END PS2EgetLibVersion2()


s32 CALLBACK CDVDinit() {
  int i;

  InitLog();
  if(OpenLog() != 0)  return(-1); // Couldn't open Log File? Abort.
  
#ifdef VERBOSE_FUNCTION_INTERFACE
  PrintLog("CDVDiso interface: CDVDinit()");
#endif /* VERBOSE_FUNCTION_INTERFACE */

  InitConf();

  isofile = NULL;
  isomode = -1;
  deviceopencount = 0;
  for(i = 0; i < 2048; i++)  isocdcheck[i] = 0;

  mainboxwindow = NULL;
  progressboxwindow = NULL;
  conversionboxwindow = NULL;
  deviceboxwindow = NULL;

  return(0);
} // END CDVDinit()


void CALLBACK CDVDshutdown() {
#ifdef VERBOSE_FUNCTION_INTERFACE
  PrintLog("CDVDiso interface: CDVDshutdown()");
#endif /* VERBOSE_FUNCTION_INTERFACE */

  isofile = IsoFileClose(isofile);

  // Close Windows as well? (Just in case)

  CloseLog();
} // END CDVDshutdown()


s32 CALLBACK CDVDopen(const char* pTitleFilename) {
  HWND lastwindow;
  int i;
  int retval;

#ifdef VERBOSE_FUNCTION_INTERFACE
  PrintLog("CDVDiso interface: CDVDopen()");
#endif /* VERBOSE_FUNCTION_INTERFACE */

  lastwindow = GetActiveWindow();
  LoadConf();

  if( pTitleFilename != NULL ) strcpy(conf.isoname, pTitleFilename);

  if((conf.isoname[0] == 0) || (conf.isoname[0] == '[') ||
     ((conf.startconfigure == 1) && (deviceopencount == 0)) ||
     ((conf.restartconfigure == 1) && (deviceopencount > 0))) {
    DialogBox(progmodule,
              MAKEINTRESOURCE(DLG_0200),
              lastwindow,
              (DLGPROC)MainBoxCallback);
    SetActiveWindow(lastwindow);
    LoadConf();
    // Blank out the name in config file afterwards? Seems excessive.
  } // ENDIF- Haven't initialized the configure program yet? Do so now.
  lastwindow = NULL;

  isofile = IsoFileOpenForRead(conf.isoname);
  if(isofile == NULL) {
#ifdef VERBOSE_FUNCTION_INTERFACE
    PrintLog("CDVDiso interface:   Failed to open ISO file!");
#endif /* VERBOSE_FUNCTION_INTERFACE */
    // return(-1); // Removed to simulate disc not in drive.
    for(i = 0; i < 2048; i++)  isocdcheck[i] = 0;
    return(0);
  } // ENDIF- Trouble opening file? Abort.

  retval = IsoFileSeek(isofile, 16);
  if(retval != 0)  return(-1);
  retval = IsoFileRead(isofile, isobuffer);
  if(retval != 0)  return(-1);

  if(deviceopencount > 0) {
    i = 0;
    while((i < 2048) && (isocdcheck[i] == isobuffer[i]))  i++;
    if(i == 2048)  deviceopencount = 0; // Same CD/DVD? No delay.
  } // ENDIF- Is this a restart? Check for disc change.

  for(i = 0; i < 2048; i++)  isocdcheck[i] = isobuffer[i];

  return(0);
} // END CDVDopen()


void CALLBACK CDVDclose() {
#ifdef VERBOSE_FUNCTION_INTERFACE
  PrintLog("CDVDiso interface: CDVDclose()");
#endif /* VERBOSE_FUNCTION_INTERFACE */
  isofile = IsoFileClose(isofile);
  deviceopencount = 50;
} // END CDVDclose()


s32  CALLBACK CDVDreadSubQ(u32 lsn, cdvdSubQ* subq) {
  char temptime[3];
  int i;
  int pos;
  u32 tracklsn;

#ifdef VERBOSE_FUNCTION_INTERFACE
  PrintLog("CDVDiso interface: CDVDreadSubQ()");
#endif /* VERBOSE_FUNCTION_INTERFACE */

  if(isofile == NULL)  return(-1);
  if(deviceopencount > 0) {
    deviceopencount--;
    if(deviceopencount > 0)  return(-1);
  } // ENDIF- Still simulating device tray open?

  if((isofile->cdvdtype == CDVD_TYPE_PS2DVD) ||
     (isofile->cdvdtype == CDVD_TYPE_DVDV)) {
    return(-1); // DVDs don't have SubQ data
  } // ENDIF- Trying to get a SubQ from a DVD?

  // fake it
  i = BCDTOHEX(isofile->toc[7]);
  pos = i * 10;
  pos += 30;
  temptime[0] = BCDTOHEX(isofile->toc[pos + 7]);
  temptime[1] = BCDTOHEX(isofile->toc[pos + 8]);
  temptime[2] = BCDTOHEX(isofile->toc[pos + 9]);
  tracklsn = MSFtoLBA(temptime);
  while((i < BCDTOHEX(isofile->toc[17])) && (tracklsn < lsn)) {
    i++;
    pos = i * 10;
    pos += 30;
    temptime[0] = BCDTOHEX(isofile->toc[pos + 7]);
    temptime[1] = BCDTOHEX(isofile->toc[pos + 8]);
    temptime[2] = BCDTOHEX(isofile->toc[pos + 9]);
    tracklsn = MSFtoLBA(temptime);
  } // ENDIF- Loop through tracks searching for lsn track
  i--;

  subq->ctrl       = 4;
  subq->mode       = 1;
  subq->trackNum   = HEXTOBCD(i);
  subq->trackIndex = HEXTOBCD(i);
	
  LBAtoMSF(lsn - tracklsn, temptime);
  subq->trackM = HEXTOBCD(temptime[0]);
  subq->trackS = HEXTOBCD(temptime[1]);
  subq->trackF = HEXTOBCD(temptime[2]);
	
  subq->pad = 0;
	
  // lba_to_msf(lsn + (2*75), &min, &sec, &frm);
  LBAtoMSF(lsn, temptime);
  subq->discM = HEXTOBCD(temptime[0]);
  subq->discS = HEXTOBCD(temptime[1]);
  subq->discF = HEXTOBCD(temptime[2]);

  return(0);
} // END CDVDreadSubQ()


s32 CALLBACK CDVDgetTN(cdvdTN *Buffer) {

#ifdef VERBOSE_FUNCTION_INTERFACE
  PrintLog("CDVDiso interface: CDVDgetTN()");
#endif /* VERBOSE_FUNCTION_INTERFACE */

  if(isofile == NULL)  return(-1);
  if(deviceopencount > 0) {
    deviceopencount--;
    if(deviceopencount > 0)  return(-1);
  } // ENDIF- Still simulating device tray open?

  if((isofile->cdvdtype == CDVD_TYPE_PS2DVD) ||
     (isofile->cdvdtype == CDVD_TYPE_DVDV)) {
    Buffer->strack = 1;
    Buffer->etrack = 1;
  } else {
    Buffer->strack = BCDTOHEX(isofile->toc[7]);
    Buffer->etrack = BCDTOHEX(isofile->toc[17]);
  } // ENDIF- Retrieve track info from a DVD? (or a CD?)

  return(0);
} // END CDVDgetTN()


s32 CALLBACK CDVDgetTD(u8 track, cdvdTD *Buffer) {
  u8 actualtrack;
  int pos;
  char temptime[3];

#ifdef VERBOSE_FUNCTION_INTERFACE
  PrintLog("CDVDiso interface: CDVDgetTD()");
#endif /* VERBOSE_FUNCTION_INTERFACE */

  if(isofile == NULL)  return(-1);
  if(deviceopencount > 0) {
    deviceopencount--;
    if(deviceopencount > 0)  return(-1);
  } // ENDIF- Still simulating device tray open?

  actualtrack = track;
  if(actualtrack == 0xaa)  actualtrack = 0;

  if((isofile->cdvdtype == CDVD_TYPE_PS2DVD) ||
     (isofile->cdvdtype == CDVD_TYPE_DVDV)) {
    if (actualtrack <= 1) {
      Buffer->type = 0;
      Buffer->lsn = isofile->filesectorsize;
    } else {
      Buffer->type = CDVD_MODE1_TRACK;
      Buffer->lsn = 0;
    } // ENDIF- Whole disc? (or single track?)
  } else {
    if (actualtrack == 0) {
      Buffer->type = 0;
      temptime[0] = BCDTOHEX(isofile->toc[27]);
      temptime[1] = BCDTOHEX(isofile->toc[28]);
      temptime[2] = BCDTOHEX(isofile->toc[29]);
      Buffer->lsn = MSFtoLBA(temptime);
    } else {
      pos = actualtrack * 10;
      pos += 30;
      Buffer->type = isofile->toc[pos];
      temptime[0] = BCDTOHEX(isofile->toc[pos + 7]);
      temptime[1] = BCDTOHEX(isofile->toc[pos + 8]);
      temptime[2] = BCDTOHEX(isofile->toc[pos + 9]);
      Buffer->lsn = MSFtoLBA(temptime);
    } // ENDIF- Whole disc? (or single track?)
  } // ENDIF- Retrieve track info from a DVD? (or a CD?)

  return(0);
} // END CDVDgetTD()


s32  CALLBACK CDVDgetTOC(void* toc) {
  int i;

#ifdef VERBOSE_FUNCTION_INTERFACE
  PrintLog("CDVDiso interface: CDVDgetTOC()");
#endif /* VERBOSE_FUNCTION_INTERFACE */

  if(isofile == NULL)  return(-1);
  if(deviceopencount > 0) {
    deviceopencount--;
    if(deviceopencount > 0)  return(-1);
  } // ENDIF- Still simulating device tray open?

  for(i = 0; i < 2048; i++)  *(((char *) toc) + i) = isofile->toc[i];
  return(0);
} // END CDVDgetTOC()


s32 CALLBACK CDVDreadTrack(u32 lsn, int mode) {
  int retval;

#ifdef VERBOSE_FUNCTION_INTERFACE
  PrintLog("CDVDiso interface: CDVDreadTrack(%u)", lsn);
#endif /* VERBOSE_FUNCTION_INTERFACE */

  if(isofile == NULL)  return(-1);
  if(deviceopencount > 0) {
    deviceopencount--;
    if(deviceopencount > 0)  return(-1);
  } // ENDIF- Still simulating device tray open?

  retval = IsoFileSeek(isofile, (off64_t) lsn);
  if(retval != 0) {
#ifdef VERBOSE_FUNCTION_INTERFACE
    PrintLog("CDVDiso interface:   Trouble finding the sector!");
#endif /* VERBOSE_FUNCTION_INTERFACE */
    return(-1);
  } // ENDIF- Trouble finding the sector?

  retval = IsoFileRead(isofile, isobuffer);
  if(retval != 0) {
#ifdef VERBOSE_FUNCTION_INTERFACE
    PrintLog("CDVDiso interface:   Trouble reading the sector!");
#endif /* VERBOSE_FUNCTION_INTERFACE */
    return(-1);
  } // ENDIF- Trouble finding the sector?

  isomode = mode;
  return(0);
} // END CDVDreadTrack()


u8* CALLBACK CDVDgetBuffer() {
  int offset;

#ifdef VERBOSE_FUNCTION_INTERFACE
  PrintLog("CDVDiso interface: CDVDgetBuffer()");
#endif /* VERBOSE_FUNCTION_INTERFACE */

  if(isofile == NULL)  return(NULL);
  if(deviceopencount > 0) {
    deviceopencount--;
    if(deviceopencount > 0)  return(NULL);
  } // ENDIF- Still simulating device tray open?

  offset = 0;
  switch(isomode) {
    case CDVD_MODE_2352:
      offset = 0;
      break;
    case CDVD_MODE_2340:
      offset = 12;
      break;
    case CDVD_MODE_2328:
    case CDVD_MODE_2048:
      offset = 24;
      break;
  } // ENDSWITCH isomode- offset to where data it wants is.

  if(offset > isofile->blockoffset)  offset = isofile->blockoffset;

  return(isobuffer + offset);
} // END CDVDgetBuffer()


s32 CALLBACK CDVDgetDiskType() {
#ifdef VERBOSE_FUNCTION_INTERFACE
  // PrintLog("CDVDiso interface: CDVDgetDiskType()");
#endif /* VERBOSE_FUNCTION_INTERFACE */

  if(isofile == NULL)  return(CDVD_TYPE_NODISC);
  if(deviceopencount > 0) {
    deviceopencount--;
    if(deviceopencount > 0)  return(CDVD_TYPE_DETCT);
  } // ENDIF- Still simulating device tray open?

  return(isofile->cdvdtype);
} // END CDVDgetDiskType()


s32 CALLBACK CDVDgetTrayStatus() {
#ifdef VERBOSE_FUNCTION_INTERFACE
  // PrintLog("CDVDiso interface: CDVDgetTrayStatus()");
#endif /* VERBOSE_FUNCTION_INTERFACE */

  if(isofile == NULL)  return(CDVD_TRAY_OPEN);
  if(deviceopencount > 30) {
    deviceopencount--;
    return(CDVD_TRAY_OPEN);
  } // ENDIF- Still simulating device tray open?

  return(CDVD_TRAY_CLOSE);
} // END CDVDgetTrayStatus()


s32 CALLBACK CDVDctrlTrayOpen() {
  HWND lastwindow;
  int i;
  int retval;

#ifdef VERBOSE_FUNCTION_INTERFACE
  PrintLog("CDVDiso interface: CDVDctrlTrayOpen()");
#endif /* VERBOSE_FUNCTION_INTERFACE */
  // CDVDclose();
  isofile = IsoFileClose(isofile);
  deviceopencount = 50;

  // CDVDopen();
  lastwindow = GetActiveWindow();
  LoadConf();
  if((conf.isoname[0] == 0) || (conf.isoname[0] == '[') ||
     ((conf.restartconfigure == 1) && (deviceopencount > 0))) {
    DialogBox(progmodule,
              MAKEINTRESOURCE(DLG_0200),
              lastwindow,
              (DLGPROC)MainBoxCallback);
    SetActiveWindow(lastwindow);
    LoadConf();
    // Blank out the name in config file afterwards? Seems excessive.
  } // ENDIF- Haven't initialized the configure program yet? Do so now.
  lastwindow = NULL;
  deviceopencount = 0; // Temp line!
  // NOTE: What happened to repetitive polling when disc not in drive?

  isofile = IsoFileOpenForRead(conf.isoname);
  if(isofile == NULL) {
#ifdef VERBOSE_FUNCTION_INTERFACE
    PrintLog("CDVDiso interface:   Failed to open ISO file!");
#endif /* VERBOSE_FUNCTION_INTERFACE */
    // return(-1); // Removed to simulate disc not in drive.
    for(i = 0; i < 2048; i++)  isocdcheck[i] = 0;
    return(0);
  } // ENDIF- Trouble opening file? Abort.

  retval = IsoFileSeek(isofile, 16);
  if(retval != 0)  return(-1);
  retval = IsoFileRead(isofile, isobuffer);
  if(retval != 0)  return(-1);

  if(deviceopencount > 0) {
    i = 0;
    while((i < 2048) && (isocdcheck[i] == isobuffer[i]))  i++;
    if(i == 2048)  deviceopencount = 0; // Same CD/DVD? No delay.
  } // ENDIF- Is this a restart? Check for disc change.

  for(i = 0; i < 2048; i++)  isocdcheck[i] = isobuffer[i];

  return(0);
} // END CDVDctrlTrayOpen()


s32 CALLBACK CDVDctrlTrayClose() {
#ifdef VERBOSE_FUNCTION_INTERFACE
  PrintLog("CDVDiso interface: CDVDctrlTrayClose()");
#endif /* VERBOSE_FUNCTION_INTERFACE */
  return(0);
} // END CDVDctrlTrayClose()


s32 CALLBACK CDVDtest() {
#ifdef VERBOSE_FUNCTION_INTERFACE
  PrintLog("CDVDiso interface: CDVDtest()");
#endif /* VERBOSE_FUNCTION_INTERFACE */
  // InitConf(); // Shouldn't need this. Doesn't CDVDInit() get called first?
  LoadConf();

  if(conf.isoname[0] == 0)  return(0); // No name chosen yet. Catch on Open()
  if(IsIsoFile(conf.isoname) == 0)  return(0); // Valid name. Go.
  return(-1); // Invalid name - reconfigure first.
  // Note really need this? Why not just return(0)...
} // END CDVDtest()


void CALLBACK CDVDconfigure() {
  HWND lastwindow;

  lastwindow = GetActiveWindow();
  DialogBox(progmodule,
            MAKEINTRESOURCE(DLG_0200),
            lastwindow,
            (DLGPROC)MainBoxCallback);
  SetActiveWindow(lastwindow);
  lastwindow = NULL;
  return;
} // END CDVDconfigure()


BOOL CALLBACK AboutCallback(HWND window, UINT msg, WPARAM param, LPARAM param2) {
  switch(msg) {
    case WM_COMMAND:
      switch(LOWORD(param)) {
        case IDC_0104: // "Ok" Button
          EndDialog(window, FALSE);
          return(TRUE);
          break;
      } // ENDSWITCH param- Which Windows Message Command?

    case WM_CLOSE:
      EndDialog(window, FALSE);
      return(TRUE);
      break;
  } // ENDSWITCH msg- what message has been sent to this window?

  return(FALSE); // Not a recognisable message. Pass it back to the OS.
} // END AboutCallback()

void CALLBACK CDVDabout() {
  HWND lastwindow;

  lastwindow = GetActiveWindow();
  DialogBox(progmodule,
            MAKEINTRESOURCE(DLG_0100),
            lastwindow,
            (DLGPROC)AboutCallback);
  SetActiveWindow(lastwindow);
  return;
} // END CDVDabout()
