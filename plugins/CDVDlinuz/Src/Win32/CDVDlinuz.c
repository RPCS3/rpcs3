/*  CDVDlinuz.c

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



#include <time.h> // time(), time_t



#define CDVDdefs

#include "../PS2Edefs.h"



#include "device.h"

#include "CD.h"

#include "DVD.h"

#include "../buffer.h"

#include "../convert.h"

#include "conf.h"

#include "logfile.h"

#include "../version.h"

#include "screens.h"

#include "mainbox.h" // Initialize mainboxwindow

#include "CDVDlinuz.h"





HINSTANCE progmodule;





BOOL APIENTRY DllMain(HANDLE hModule,

                      DWORD param,

                      LPVOID reserved) {





  switch(param) {

    case DLL_PROCESS_ATTACH:

      progmodule = hModule;

      // mainboxwindow = NULL;

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

  InitLog();

  if(OpenLog() != 0)  return(-1); // Couldn't open Log File? Abort.

  

#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDinit()");

#endif /* VERBOSE_FUNCTION_INTERFACE */



  InitConf();



  DeviceInit();

  InitCDInfo();

  InitDVDInfo();



  mainboxwindow = NULL;



  return(0);

} // END CDVDinit()





void CALLBACK CDVDshutdown() {

#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDshutdown()");

#endif /* VERBOSE_FUNCTION_INTERFACE */



  DeviceClose();



  // Close Windows as well? (Just in case)



  CloseLog();

} // END CDVDshutdown()





s32 CALLBACK CDVDopen() {

  int retval;



#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDopen()");

#endif /* VERBOSE_FUNCTION_INTERFACE */



  InitBuffer();



  LoadConf();



  retval = DeviceOpen();

  if(retval == 0) {

    DeviceTrayStatus();

  } // ENDIF- Did we open the device? Poll for media.

  return(retval);

} // END CDVDopen()





void CALLBACK CDVDclose() {

#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDclose()");

#endif /* VERBOSE_FUNCTION_INTERFACE */

  DeviceClose();

} // END CDVDclose()





s32  CALLBACK CDVDreadSubQ(u32 lsn, cdvdSubQ* subq) {

  char temptime[3];

  int i;

  int pos;

  u32 tracklsn;



#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDreadSubQ()");

#endif /* VERBOSE_FUNCTION_INTERFACE */



  if(DiscInserted() != 0)  return(-1);



  if((disctype == CDVD_TYPE_PS2DVD) || (disctype == CDVD_TYPE_DVDV)) {

    return(-1); // DVDs don't have SubQ data

  } // ENDIF- Trying to get a SubQ from a DVD?



  // fake it

  i = BCDTOHEX(tocbuffer[7]);

  pos = i * 10;

  pos += 30;

  temptime[0] = BCDTOHEX(tocbuffer[pos + 7]);

  temptime[1] = BCDTOHEX(tocbuffer[pos + 8]);

  temptime[2] = BCDTOHEX(tocbuffer[pos + 9]);

  tracklsn = MSFtoLBA(temptime);

  while((i < BCDTOHEX(tocbuffer[17])) && (tracklsn < lsn)) {

    i++;

    pos = i * 10;

    pos += 30;

    temptime[0] = BCDTOHEX(tocbuffer[pos + 7]);

    temptime[1] = BCDTOHEX(tocbuffer[pos + 8]);

    temptime[2] = BCDTOHEX(tocbuffer[pos + 9]);

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

  PrintLog("CDVDlinuz interface: CDVDgetTN()");

#endif /* VERBOSE_FUNCTION_INTERFACE */



  if(DiscInserted() != 0)  return(-1);



  if((disctype == CDVD_TYPE_PS2DVD) || (disctype == CDVD_TYPE_DVDV)) {

    Buffer->strack = 1;

    Buffer->etrack = 1;

  } else {

    Buffer->strack = BCDTOHEX(tocbuffer[7]);

    Buffer->etrack = BCDTOHEX(tocbuffer[17]);

  } // ENDIF- Retrieve track info from a DVD? (or a CD?)



  return(0);

} // END CDVDgetTN()





s32 CALLBACK CDVDgetTD(u8 track, cdvdTD *buffer) {

#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDgetTD()");

#endif /* VERBOSE_FUNCTION_INTERFACE */



  return(DeviceGetTD(track, buffer));

} // END CDVDgetTD()





s32  CALLBACK CDVDgetTOC(void* toc) {

  int i;



#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDgetTOC()");

#endif /* VERBOSE_FUNCTION_INTERFACE */



  if(DiscInserted() != 0)  return(-1);



  for(i = 0; i < 2048; i++)  *(((char *) toc) + i) = tocbuffer[i];

  return(0);

} // END CDVDgetTOC()





s32 CALLBACK CDVDreadTrack(u32 lsn, int mode) {

  int retval;



#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDreadTrack(%u)", lsn);

#endif /* VERBOSE_FUNCTION_INTERFACE */



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



  retval = DeviceReadTrack(lsn, mode, bufferlist[replacebuffer].buffer);

  bufferlist[replacebuffer].lsn = lsn;

  bufferlist[replacebuffer].mode = mode;

  bufferlist[replacebuffer].offset = DeviceBufferOffset();



  if(retval != 0) {

    bufferlist[replacebuffer].mode = -1; // Error! flag buffer as such.

  } else {

    if((disctype != CDVD_TYPE_PS2DVD) && (disctype != CDVD_TYPE_DVDV)) {

      if(mode == CDVD_MODE_2352) {

        CDreadSubQ(lsn, &bufferlist[replacebuffer].subq);

      } // ENDIF- Read subq as well?

    } // ENDIF- Read a DVD buffer or a CD buffer?

  } // ENDIF-Read ok? Fill out rest of buffer info.

  AddListBuffer(replacebuffer);

  return(retval);

} // END CDVDreadTrack()





u8* CALLBACK CDVDgetBuffer() {

#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDgetBuffer()");

#endif /* VERBOSE_FUNCTION_INTERFACE */



  if(DiscInserted() == -1)  return(NULL);



  if(userbuffer == 0xffff) {

#ifdef VERBOSE_WARNINGS

    PrintLog("CDVDlinuz interface:   Not pointing to a buffer!");

#endif /* VERBOSE_WARNINGS */

    return(NULL); // No buffer reference?

  } // ENDIF- user buffer not pointing at anything? Abort



  if(bufferlist[userbuffer].mode < 0) {

#ifdef VERBOSE_WARNINGS

    PrintLog("CDVDlinuz interface:   Error retrieving sector (ReadTrack call)");

#endif /* VERBOSE_WARNINGS */

    return(NULL); // Bad Sector?

  } // ENDIF- Trouble reading physical sector? Tell them.



  return(bufferlist[userbuffer].buffer + bufferlist[userbuffer].offset);

} // END CDVDgetBuffer()





s32 CALLBACK CDVDgetDiskType() {

#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDgetDiskType()");

#endif /* VERBOSE_FUNCTION_INTERFACE */



  if(lasttime != time(NULL)) {

    lasttime = time(NULL);

    DeviceTrayStatus();

  } // ENDIF- Has enough time passed between calls?



  return(disctype);

} // END CDVDgetDiskType()





s32 CALLBACK CDVDgetTrayStatus() {

#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDgetTrayStatus()");

#endif /* VERBOSE_FUNCTION_INTERFACE */



  if(lasttime != time(NULL)) {

    lasttime = time(NULL);

    DeviceTrayStatus();

  } // ENDIF- Has enough time passed between calls?



  return(traystatus);

} // END CDVDgetTrayStatus()





s32 CALLBACK CDVDctrlTrayOpen() {

#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDctrlTrayOpen()");

#endif /* VERBOSE_FUNCTION_INTERFACE */



  return(DeviceTrayOpen());

} // END CDVDctrlTrayOpen()





s32 CALLBACK CDVDctrlTrayClose() {

#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDctrlTrayClose()");

#endif /* VERBOSE_FUNCTION_INTERFACE */



  return(DeviceTrayClose());

} // END CDVDctrlTrayClose()





s32 CALLBACK CDVDtest() {

  int retval;



  errno = 0;



  if(devicehandle != NULL) {

#ifdef VERBOSE_WARNINGS

    PrintLog("CDVDlinuz interface:   Device already open");

#endif /* VERBOSE_WARNINGS */

    return(0);

  } // ENDIF- Is the CD/DVD already in use? That's fine.



#ifdef VERBOSE_FUNCTION_INTERFACE

  PrintLog("CDVDlinuz interface: CDVDtest()");

#endif /* VERBOSE_FUNCTION_INTERFACE */



  retval = DeviceOpen();

  DeviceClose();

  return(retval);

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

