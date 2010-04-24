/*  mainbox.c

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

// #include <windowsx.h> // Button_

#include <windef.h> // NULL



#include <stdio.h> // sprintf()

#include <string.h> // strcpy()

#include <sys/stat.h> // stat()

#include <sys/types.h> // stat()

#include <unistd.h> // stat()



#include "conf.h"

#include "device.h"

// #include "imagetype.h" // imagedata[].name

#include "screens.h" // DLG_, IDC_

#include "CDVDlinuz.h" // progmodule

#include "mainbox.h"





HWND mainboxwindow;





void MainBoxDestroy() {

  if(mainboxwindow != NULL) {

    EndDialog(mainboxwindow, FALSE);

    mainboxwindow = NULL;

  } // ENDIF- Do we have a Main Window still?

} // END MainBoxDestroy()





void MainBoxUnfocus() {

  // EnableWindow(?)

  // gtk_widget_set_sensitive(mainbox.device, FALSE);

  ShowWindow(mainboxwindow, SW_HIDE);

} // END MainBoxUnfocus()





void MainBoxDeviceEvent() {

  char tempdevice[256];

  struct stat filestat;

  int returnval;



  GetDlgItemText(mainboxwindow, IDC_0202, tempdevice, 256);

  returnval = stat(tempdevice, &filestat);

  if(returnval == -1) {

    SetDlgItemText(mainboxwindow, IDC_0203, "Device Type: ---");

    return;

  } // ENDIF- Not a name of any sort?



  if(S_ISDIR(filestat.st_mode) != 0) {

    SetDlgItemText(mainboxwindow, IDC_0203, "Device Type: Not a device");

    return;

  } // ENDIF- Not a regular file?



    SetDlgItemText(mainboxwindow, IDC_0203, "Device Type: Device Likely");

  return;

} // END MainBoxFileEvent()





void MainBoxRefocus() {

  MainBoxDeviceEvent();



  // gtk_widget_set_sensitive(mainbox.device, TRUE);

  // gtk_window_set_focus(GTK_WINDOW(mainbox.window), mainbox.device);

  // ShowWindow(mainboxwindow, SW_RESTORE); // and/or, SW_SHOW? SW_SHOWNORMAL?

  ShowWindow(mainboxwindow, SW_SHOW);

  SetActiveWindow(mainboxwindow);

} // END MainBoxRefocus()





void MainBoxCancelEvent() {

  MainBoxDestroy();

  return;

} // END MainBoxCancelEvent()





void MainBoxOKEvent() {

  int retval;



  MainBoxUnfocus();



  GetDlgItemText(mainboxwindow, IDC_0202, conf.devicename, 256);

  retval = DeviceOpen();

  DeviceClose();

  if(retval != 0) {

    MainBoxRefocus();

    MessageBox(mainboxwindow,

               "Could not open the device",

               "CDVDlinuz Message",

               MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);

    return;

  } // ENDIF- Trouble opening device? Abort here.



  SaveConf();



  MainBoxCancelEvent();

  return;

} // END MainBoxOKEvent()





void MainBoxDisplay() {

  InitConf(); // Man, am I boiling mad! CDVDinit() should have been called first!

  LoadConf();



  // Adjust window position?



  // We held off setting the name until now... so description would show.

  SetDlgItemText(mainboxwindow, IDC_0202, conf.devicename);



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

      return(FALSE); // And let Windows display this window.

      break;



    case WM_CLOSE: // The "X" in the upper right corner was hit.

      MainBoxCancelEvent();

      return(TRUE);

      break;



    case WM_COMMAND:

      // Do we wish to capture 'ENTER/RETURN' and/or 'ESC' here?



      switch(LOWORD(param)) {

        case IDC_0202: // Devicename Edit Box

          MainBoxDeviceEvent(); // Describe the File's type...

          return(FALSE); // Let Windows handle the actual 'edit' processing...

          break;



        case IDC_0204: // "Ok" Button

          MainBoxOKEvent();

          return(TRUE);

          break;



        case IDC_0205: // "Cancel" Button

          MainBoxCancelEvent();

          return(TRUE);

          break;

      } // ENDSWITCH param- Which object got the message?

  } // ENDSWITCH msg- what message has been sent to this window?



  return(FALSE); // Not a recognized message? Tell Windows to handle it.

} // END MainBoxEventLoop()

