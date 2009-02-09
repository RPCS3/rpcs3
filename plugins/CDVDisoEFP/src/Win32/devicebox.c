/*  devicebox.c

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

#include <windowsx.h> // ComboBox_AddString()

#include <windef.h> // NULL



#include <stdio.h> // sprintf()

#include <string.h> // strcpy()

#include <sys/stat.h> // stat()

#include <sys/types.h> // stat()

#include <unistd.h> // stat()



#define CDVDdefs

#include "PS2Edefs.h"



#include "conf.h"

#include "device.h"

#include "isofile.h"

#include "isocompress.h" // compressdesc[]

// #include "imagetype.h" // imagedata[].name

#include "multifile.h" // multinames[]

#include "toc.h"

#include "progressbox.h"

#include "mainbox.h"

#include "screens.h"

#include "devicebox.h"





HWND deviceboxwindow;





void DeviceBoxDestroy() {

  if(deviceboxwindow != NULL) {

    EndDialog(deviceboxwindow, FALSE);

    deviceboxwindow = NULL;

  } // ENDIF- Do we have a Main Window still?

} // END DeviceBoxDestroy()





void DeviceBoxUnfocus() {

  // gtk_widget_set_sensitive(devicebox.device, FALSE);

  // gtk_widget_set_sensitive(devicebox.file, FALSE);

  // gtk_widget_set_sensitive(devicebox.selectbutton, FALSE);

  // gtk_widget_set_sensitive(devicebox.compress, FALSE);

  // gtk_widget_set_sensitive(devicebox.multi, FALSE);

  // gtk_widget_set_sensitive(devicebox.okbutton, FALSE);

  // gtk_widget_set_sensitive(devicebox.cancelbutton, FALSE);

  ShowWindow(deviceboxwindow, SW_HIDE);

} // END DeviceBoxUnfocus()





void DeviceBoxDeviceEvent() {

  char tempdevice[256];

  struct stat filestat;

  int returnval;



  GetDlgItemText(deviceboxwindow, IDC_0302, tempdevice, 256);

  returnval = stat(tempdevice, &filestat);

  if(returnval == -1) {

    SetDlgItemText(deviceboxwindow, IDC_0303, "Device Type: ---");

    return;

  } // ENDIF- Not a name of any sort?



  if(S_ISDIR(filestat.st_mode) != 0) {

    SetDlgItemText(deviceboxwindow, IDC_0303, "Device Type: Not a device");

    return;

  } // ENDIF- Not a regular file?



    SetDlgItemText(deviceboxwindow, IDC_0303, "Device Type: Device Likely");

  return;

} // END DeviceBoxDeviceEvent()





void DeviceBoxFileEvent() {

  int returnval;

  char templine[256];

  struct IsoFile *tempfile;



  GetDlgItemText(deviceboxwindow, IDC_0305, templine, 256);

  returnval = IsIsoFile(templine);

  if(returnval == -1) {

    SetDlgItemText(deviceboxwindow, IDC_0307, "File Type: ---");

    return;

  } // ENDIF- Not a name of any sort?



  if(returnval == -2) {

    SetDlgItemText(deviceboxwindow, IDC_0307, "File Type: Not a file");

    return;

  } // ENDIF- Not a regular file?



  if(returnval == -3) {

    SetDlgItemText(deviceboxwindow, IDC_0307, "File Type: Not a valid image file");

    return;

  } // ENDIF- Not an image file?



  if(returnval == -4) {

    SetDlgItemText(deviceboxwindow, IDC_0307, "File Type: Missing Table File (will rebuild)");

    return;

  } // ENDIF- Not a regular file?



  tempfile = IsoFileOpenForRead(templine);

  sprintf(templine, "File Type: %s%s%s",

          multinames[tempfile->multi],

          tempfile->imagename,

          compressdesc[tempfile->compress]);

  SetDlgItemText(deviceboxwindow, IDC_0307, templine);

  tempfile = IsoFileClose(tempfile);

  return;

} // END DeviceBoxFileEvent()





void DeviceBoxRefocus() {

  DeviceBoxDeviceEvent();

  DeviceBoxFileEvent();



  // gtk_widget_set_sensitive(devicebox.device, TRUE);

  // gtk_widget_set_sensitive(devicebox.file, TRUE);

  // gtk_widget_set_sensitive(devicebox.selectbutton, TRUE);

  // gtk_widget_set_sensitive(devicebox.compress, TRUE);

  // gtk_widget_set_sensitive(devicebox.multi, TRUE);

  // gtk_widget_set_sensitive(devicebox.okbutton, TRUE);

  // gtk_widget_set_sensitive(devicebox.cancelbutton, TRUE);

  // gtk_window_set_focus(GTK_WINDOW(devicebox.window), devicebox.file);

  ShowWindow(deviceboxwindow, SW_SHOW);

  SetActiveWindow(deviceboxwindow);

} // END DeviceBoxRefocus()





void DeviceBoxCancelEvent() {

  // ShowWindow(deviceboxwindow, SW_HIDE);

  DeviceBoxDestroy();

  MainBoxRefocus();

  return;

} // END DeviceBoxCancelEvent()





void DeviceBoxOKEvent() {

  char templine[256];

  u8 tempbuffer[2352];

  struct IsoFile *tofile;

  s32 retval;

  cdvdTD cdvdtd;

  int stop;

  HWND tempitem;

  int compressmethod;

  int multi;

  int imagetype;

  int i;



  DeviceBoxUnfocus();



  GetDlgItemText(deviceboxwindow, IDC_0302, conf.devicename, 256);

  retval = DeviceOpen();

  if(retval != 0) {

    DeviceClose();

    DeviceBoxRefocus();

    MessageBox(deviceboxwindow,

               "Could not open the device",

               "CDVDisoEFP Message",

               MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);

    return;

  } // ENDIF- Trouble opening device? Abort here.



  DeviceTrayStatus();

  retval = DiscInserted();

  if(retval != 0) {

    DeviceClose();

    DeviceBoxRefocus();

    MessageBox(deviceboxwindow,

               "No disc in the device\r\nPlease put a disc in and try again.",

               "CDVDisoEFP Message",

               MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);

    return;

  } // ENDIF- Trouble opening device? Abort here.



  retval = DeviceGetTD(0, &cdvdtd); // Fish for Ending Sector

  if(retval < 0) {

    DeviceClose();

    DeviceBoxRefocus();

    MessageBox(deviceboxwindow,

               "Could not retrieve disc sector size",

               "CDVDisoEFP Message",

               MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);

    return;

  } // ENDIF- Trouble getting disc sector count?



  tempitem = GetDlgItem(deviceboxwindow, IDC_0309);

  compressmethod = ComboBox_GetCurSel(tempitem);

  tempitem = NULL;

  if(compressmethod > 0)  compressmethod += 2;



  multi = 0;

  if(IsDlgButtonChecked(deviceboxwindow, IDC_0311))  multi = 1;



  imagetype = 0;

  if((disctype != CDVD_TYPE_PS2DVD) &&

     (disctype != CDVD_TYPE_DVDV))  imagetype = 8;



  GetDlgItemText(deviceboxwindow, IDC_0305, templine, 256);

  tofile = IsoFileOpenForWrite(templine,

                               imagetype,

                               multi,

                               compressmethod);

  if(tofile == NULL) {

    DeviceClose();

    DeviceBoxRefocus();

    MessageBox(deviceboxwindow,

               "Could not create the new ISO file",

               "CDVDisoEFP Message",

               MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);

    return;

  } // ENDIF- Trouble opening the ISO file?



  // Open Progress Bar

  sprintf(templine, "%s -> %s", conf.devicename, tofile->name);

  ProgressBoxStart(templine, (off64_t) cdvdtd.lsn);



  tofile->cdvdtype = disctype;

  for(i = 0; i < 2048; i++)  tofile->toc[i] = tocbuffer[i];



  stop = 0;

  mainboxstop = 0;

  progressboxstop = 0;

  while((stop == 0) && (tofile->sectorpos < cdvdtd.lsn)) {

    if(imagetype == 0) {

      retval = DeviceReadTrack((u32) tofile->sectorpos,

                               CDVD_MODE_2048,

                               tempbuffer);

    } else {

      retval = DeviceReadTrack((u32) tofile->sectorpos,

                               CDVD_MODE_2352,

                               tempbuffer);

    } // ENDIF- Are we reading a DVD sector? (Or a CD sector?)

    if(retval < 0) {

      for(i = 0; i < 2352; i++) {

        tempbuffer[i] = 0;

      } // NEXT i- Zeroing the buffer

    } // ENDIF- Trouble reading next block?

    retval = IsoFileWrite(tofile, tempbuffer);

    if(retval < 0) {

      MessageBox(deviceboxwindow,

                 "Trouble writing new file",

                 "CDVDisoEFP Message",

                 MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);

      stop = 1;

    } // ENDIF- Trouble writing out the next block?



    ProgressBoxTick(tofile->sectorpos);



    if(mainboxstop != 0)  stop = 2;

    if(progressboxstop != 0)  stop = 2;

  } // ENDWHILE- No reason found to stop...



  ProgressBoxStop();



  if(stop == 0) {

    if(tofile->multi == 1)  tofile->name[tofile->multipos] = '0'; // First file

    strcpy(templine, tofile->name);

  } // ENDIF- Did we succeed with the transfer?



  DeviceClose();

  if(stop == 0) {

    IsoSaveTOC(tofile);

    tofile = IsoFileClose(tofile);

    SetDlgItemText(mainboxwindow, IDC_0202, templine);

  } else {

    tofile = IsoFileCloseAndDelete(tofile);

  } // ENDIF- (Failed to complete writing file? Get rid of the garbage files.)



  DeviceBoxRefocus();

  if(stop == 0)  DeviceBoxCancelEvent();

  return;

} // END DeviceBoxOKEvent()





void DeviceBoxBrowseEvent() {

  OPENFILENAME filebox;

  char newfilename[256];

  BOOL returnbool;



  filebox.lStructSize = sizeof(filebox);

  filebox.hwndOwner = deviceboxwindow;

  filebox.hInstance = NULL;

  filebox.lpstrFilter = fileselection;

  filebox.lpstrCustomFilter = NULL;

  filebox.nFilterIndex = 0;

  filebox.lpstrFile = newfilename;

  filebox.nMaxFile = 256;

  filebox.lpstrFileTitle = NULL;

  filebox.nMaxFileTitle = 0;

  filebox.lpstrInitialDir = NULL;

  filebox.lpstrTitle = NULL;

  filebox.Flags = OFN_PATHMUSTEXIST

                | OFN_NOCHANGEDIR

                | OFN_HIDEREADONLY;

  filebox.nFileOffset = 0;

  filebox.nFileExtension = 0;

  filebox.lpstrDefExt = NULL;

  filebox.lCustData = 0;

  filebox.lpfnHook = NULL;

  filebox.lpTemplateName = NULL;



  GetDlgItemText(deviceboxwindow, IDC_0305, newfilename, 256);

  returnbool = GetOpenFileName(&filebox);

  if(returnbool != FALSE) {

    SetDlgItemText(deviceboxwindow, IDC_0305, newfilename);

  } // ENDIF- User actually selected a name? Save it.



  return;

} // END DeviceBoxBrowseEvent()





void DeviceBoxDisplay() {

  char templine[256];

  HWND itemptr;

  int itemcount;



  // Adjust Window Position?



  SetDlgItemText(deviceboxwindow, IDC_0302, conf.devicename);



  // DeviceBoxDeviceEvent(); // Needed?



  GetDlgItemText(mainboxwindow, IDC_0202, templine, 256);

  SetDlgItemText(deviceboxwindow, IDC_0305, templine);



  // DeviceBoxFileEvent(); // Needed?



  itemptr = GetDlgItem(deviceboxwindow, IDC_0309); // Compression Combo

  itemcount = 0;

  while(compressnames[itemcount] != NULL) {

    ComboBox_AddString(itemptr, compressnames[itemcount]);

    itemcount++;

  } // ENDWHILE- loading compression types into combo box

  ComboBox_SetCurSel(itemptr, 0); // First Selection?

  itemptr = NULL;



  CheckDlgButton(deviceboxwindow, IDC_0311, FALSE); // Start unchecked



  DeviceInit(); // Initialize device access

} // END DeviceBoxDisplay()





BOOL CALLBACK DeviceBoxCallback(HWND window,

                                UINT msg,

                                WPARAM param,

                                LPARAM param2) {

  switch(msg) {

    case WM_INITDIALOG:

      deviceboxwindow = window;

      DeviceBoxDisplay(); // Final touches to this window

      return(FALSE); // Let Windows display this window

      break;



    case WM_CLOSE: // The "X" in the upper right corner was hit.

      DeviceBoxCancelEvent();

      return(TRUE);

      break;



    case WM_COMMAND:

      switch(LOWORD(param)) {

        case IDC_0302: // Device Edit Box

          DeviceBoxDeviceEvent();

          return(FALSE);

          break;



        case IDC_0305: // Filename Edit Box

          DeviceBoxFileEvent();

          return(FALSE);

          break;



        case IDC_0306: // "Browse" Button

          DeviceBoxBrowseEvent();

          return(TRUE);

          break;



        case IDC_0312: // "Make File" Button

          DeviceBoxOKEvent();

          return(TRUE);

          break;



        case IDC_0313: // "Cancel" Button

          DeviceBoxCancelEvent();

          return(TRUE);

          break;

      } // ENDSWITCH param- Which object got the message?

  } // ENDSWITCH msg- What message has been sent to this window?



  return(FALSE);

} // END DeviceBoxCallback()

