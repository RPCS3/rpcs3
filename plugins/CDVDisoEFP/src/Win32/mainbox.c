/*  mainbox.c
 *  Copyright (C) 2002-2005  CDVDiso Team
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


#include <windows.h>
#include <windowsx.h> // Button_
#include <windef.h> // NULL

#include <stdio.h> // sprintf()
#include <string.h> // strcpy()

#include "conf.h"
#include "isofile.h"
#include "isocompress.h" // compressdesc[]
// #include "imagetype.h" // imagedata[].name
#include "multifile.h" // multinames[]
#include "tablerebuild.h" // IsoTableRebuild()
#include "devicebox.h"
#include "conversionbox.h"
#include "progressbox.h"
#include "screens.h" // DLG_, IDC_
#include "CDVDiso.h" // progmodule
#include "mainbox.h"


const char fileselection[] = "\Disc Image Files (*.bin,*.img,*.iso,*.nrg)\0*.bin;*.img;*.iso;*.nrg\0\All Files (*.*)\0*.*\0\\0\0";


HWND mainboxwindow;
int mainboxstop;


void MainBoxDestroy() {
  if(progressboxwindow != NULL) {
    ProgressBoxDestroy();
  } // ENDIF- Do we have a Progress Window still?

  if(mainboxwindow != NULL) {
    EndDialog(mainboxwindow, FALSE);
    mainboxwindow = NULL;
  } // ENDIF- Do we have a Main Window still?
} // END MainBoxDestroy()


void MainBoxUnfocus() {
  // EnableWindow(?)
  // gtk_widget_set_sensitive(mainbox.file, FALSE);
  // gtk_widget_set_sensitive(mainbox.selectbutton, FALSE);
  // gtk_widget_set_sensitive(mainbox.okbutton, FALSE);
  // gtk_widget_set_sensitive(mainbox.devbutton, FALSE);
  // gtk_widget_set_sensitive(mainbox.convbutton, FALSE);
  ShowWindow(mainboxwindow, SW_HIDE);
} // END MainBoxUnfocus()


void MainBoxFileEvent() {
  int returnval;
  char templine[256];
  struct IsoFile *tempfile;

  GetDlgItemText(mainboxwindow, IDC_0202, templine, 256);
  returnval = IsIsoFile(templine);
  if(returnval == -1) {
    SetDlgItemText(mainboxwindow, IDC_0204, "File Type: ---");
    return;
  } // ENDIF- Not a name of any sort?

  if(returnval == -2) {
    SetDlgItemText(mainboxwindow, IDC_0204, "File Type: Not a file");
    return;
  } // ENDIF- Not a regular file?

  if(returnval == -3) {
    SetDlgItemText(mainboxwindow, IDC_0204, "File Type: Not a valid image file");
    return;
  } // ENDIF- Not an Image file?

  if(returnval == -4) {
    SetDlgItemText(mainboxwindow, IDC_0204, "File Type: Missing Table File (will rebuild)");
    return;
  } // ENDIF- Missing Compression seek table?

  tempfile = IsoFileOpenForRead(templine);
  sprintf(templine, "File Type: %s%s%s",
          multinames[tempfile->multi],
          tempfile->imagename,
          compressdesc[tempfile->compress]);
  SetDlgItemText(mainboxwindow, IDC_0204, templine);
  tempfile = IsoFileClose(tempfile);
  return;
} // END MainBoxFileEvent()


void MainBoxRefocus() {
  MainBoxFileEvent();

  // gtk_widget_set_sensitive(mainbox.file, TRUE);
  // gtk_widget_set_sensitive(mainbox.selectbutton, TRUE);
  // gtk_widget_set_sensitive(mainbox.okbutton, TRUE);
  // gtk_widget_set_sensitive(mainbox.devbutton, TRUE);
  // gtk_widget_set_sensitive(mainbox.convbutton, TRUE);
  // gtk_window_set_focus(GTK_WINDOW(mainbox.window), mainbox.file);
  // ShowWindow(mainboxwindow, SW_RESTORE); // and/or, SW_SHOW? SW_SHOWNORMAL?
  ShowWindow(mainboxwindow, SW_SHOW);
  SetActiveWindow(mainboxwindow);
} // END MainBoxRefocus()


void MainBoxCancelEvent() {
  mainboxstop = 1; // Halt all long processess...

  MainBoxDestroy();
  return;
} // END MainBoxCancelEvent()


void MainBoxOKEvent() {
  char tempisoname[256];

  MainBoxUnfocus();

  GetDlgItemText(mainboxwindow, IDC_0202, tempisoname, 256);
  if(*(tempisoname) != 0) {
    if(IsIsoFile(tempisoname) == -4) {
      IsoTableRebuild(tempisoname);
      MainBoxRefocus();
      return;
    } // ENDIF- Do we need to rebuild an image file's index before using it?

    if(IsIsoFile(tempisoname) < 0) {
      MainBoxRefocus();
      MessageBox(mainboxwindow,
                 "Not a Valid Image File.",
                 "CDVDisoEFP Message",
                 MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
      return;
    } // ENDIF- Not an ISO file? Message and Stop here.
  } // ENDIF- Do we have a name to check out?

  strcpy(conf.isoname, tempisoname);
  if(Button_GetCheck(GetDlgItem(mainboxwindow, IDC_0209)) == BST_UNCHECKED) {
    conf.startconfigure = 0; // FALSE
  } else {
    conf.startconfigure = 1; // TRUE
  } // ENDIF- Was this checkbox unchecked?
  if(Button_GetCheck(GetDlgItem(mainboxwindow, IDC_0210)) == BST_UNCHECKED) {
    conf.restartconfigure = 0; // FALSE
  } else {
    conf.restartconfigure = 1; // TRUE
  } // ENDIF- Was this checkbox unchecked?
  SaveConf();

  MainBoxCancelEvent();
  return;
} // END MainBoxOKEvent()


void MainBoxBrowseEvent() {
  OPENFILENAME filebox;
  char newfilename[256];
  BOOL returnbool;

  filebox.lStructSize = sizeof(filebox);
  filebox.hwndOwner = mainboxwindow;
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
  filebox.Flags = OFN_FILEMUSTEXIST
                | OFN_NOCHANGEDIR
                | OFN_HIDEREADONLY;
  filebox.nFileOffset = 0;
  filebox.nFileExtension = 0;
  filebox.lpstrDefExt = NULL;
  filebox.lCustData = 0;
  filebox.lpfnHook = NULL;
  filebox.lpTemplateName = NULL;

  GetDlgItemText(mainboxwindow, IDC_0202, newfilename, 256);
  returnbool = GetOpenFileName(&filebox);
  if(returnbool != FALSE) {
    SetDlgItemText(mainboxwindow, IDC_0202, newfilename);
  } // ENDIF- User actually selected a name? Save it.

  return;
} // END MainBoxBrowseEvent()


void MainBoxDeviceEvent() {
  MainBoxUnfocus();

  DialogBox(progmodule,
            MAKEINTRESOURCE(DLG_0300),
            mainboxwindow,
            (DLGPROC)DeviceBoxCallback);
  return;
} // END MainBoxBrowseEvent()


void MainBoxConversionEvent() {
  MainBoxUnfocus();

  DialogBox(progmodule,
            MAKEINTRESOURCE(DLG_0400),
            mainboxwindow,
            (DLGPROC)ConversionBoxCallback);
  return;
} // END MainBoxBrowseEvent()


void MainBoxDisplay() {
  LoadConf();

  // Adjust window position?

  // We held off setting the name until now... so description would show.
  SetDlgItemText(mainboxwindow, IDC_0202, conf.isoname);
  if(conf.startconfigure == 0) {
    Button_SetCheck(GetDlgItem(mainboxwindow, IDC_0209), BST_UNCHECKED);
  } else {
    Button_SetCheck(GetDlgItem(mainboxwindow, IDC_0209), BST_CHECKED);
  } // ENDIF- Do we need to uncheck this box?
  if(conf.restartconfigure == 0) {
    Button_SetCheck(GetDlgItem(mainboxwindow, IDC_0210), BST_UNCHECKED);
  } else {
    Button_SetCheck(GetDlgItem(mainboxwindow, IDC_0210), BST_CHECKED);
  } // ENDIF- Do we need to uncheck this box?

  // First Time - Show the window
  ShowWindow(mainboxwindow, SW_SHOWNORMAL);
} // END MainBoxDisplay()


BOOL CALLBACK MainBoxCallback(HWND window,
                              UINT msg,
                              WPARAM param,
                              LPARAM param2) {
  switch(msg) {
    case WM_INITDIALOG:
      mainboxwindow = window;
      MainBoxDisplay(); // In this case, final touches to this window.
      ProgressBoxDisplay(); // Create the Progress Box at this time.
      return(FALSE); // And let Windows display this window.
      break;

    case WM_CLOSE: // The "X" in the upper right corner was hit.
      MainBoxCancelEvent();
      return(TRUE);
      break;

    case WM_COMMAND:
      // Do we wish to capture 'ENTER/RETURN' and/or 'ESC' here?

      switch(LOWORD(param)) {
        case IDC_0202: // Filename Edit Box
          MainBoxFileEvent(); // Describe the File's type...
          return(FALSE); // Let Windows handle the actual 'edit' processing...
          break;

        case IDC_0203: // "Browse" Button
          MainBoxBrowseEvent();
          return(TRUE);
          break;

        case IDC_0205: // "Ok" Button
          MainBoxOKEvent();
          return(TRUE);
          break;

        case IDC_0206: // "Get from Disc" Button
          MainBoxDeviceEvent();
          return(TRUE);
          break;

        case IDC_0207: // "Convert" Button
          MainBoxConversionEvent();
          return(TRUE);
          break;

        case IDC_0208: // "Cancel" Button
          MainBoxCancelEvent();
          return(TRUE);
          break;
      } // ENDSWITCH param- Which object got the message?
  } // ENDSWITCH msg- what message has been sent to this window?

  return(FALSE); // Not a recognized message? Tell Windows to handle it.
} // END MainBoxEventLoop()
