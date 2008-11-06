/*  progressbox.c

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

#include <windowsx.h> // Enable_Button()

#include <windef.h> // NULL

#include <commctrl.h> // PBM_...



#include <stdio.h> // sprintf()

#include <string.h> // strcat()

#include <sys/types.h> // off64_t



#include "CDVDiso.h" // progmodule

#include "screens.h"

#include "mainbox.h"

#include "progressbox.h"





HWND progressboxwindow;

HWND progressboxbar;

int progressboxstop;



off64_t progressboxmax;

off64_t progressboxlastpct;

char progressboxline[256];

char progressboxmaxchar[16];





void ProgressBoxDestroy() {

  if(progressboxwindow != NULL) {

    DestroyWindow(progressboxwindow);

    progressboxwindow = NULL;

    progressboxbar = NULL;

  } // ENDIF- Do we have a Progress Window still?

  return;

} // END ProgressBoxDestroy()





void ProgressBoxStart(char *description, off64_t maximum) {

  SetDlgItemText(progressboxwindow, IDC_0501, description);



  progressboxmax = maximum;

  sprintf(progressboxmaxchar, "%llu", maximum);

  progressboxlastpct = 100;

  progressboxstop = 0;



  ProgressBoxTick(0);

  ShowWindow(progressboxwindow, SW_SHOW);

  SetForegroundWindow(progressboxwindow);

  return;

} // END ProgressBoxStart()





void ProgressBoxTick(off64_t current) {

  off64_t thispct;

  MSG msg;

  BOOL returnbool;



  sprintf(progressboxline, "%llu of ", current);

  strcat(progressboxline, progressboxmaxchar);

  SetDlgItemText(progressboxwindow, IDC_0503, progressboxline);



  if(progressboxmax >= 30000 ) {

    SendMessage(progressboxbar, PBM_SETPOS, current / (progressboxmax / 30000), 0);

  } else {

    SendMessage(progressboxbar, PBM_SETPOS, (current * 30000) / progressboxmax, 0);

  } // ENDIF- Our maximum # over 30000? (Avoiding divide-by-zero error)



  if(progressboxmax >= 100) {

    thispct = current / ( progressboxmax / 100 );

    if(thispct != progressboxlastpct) {

      sprintf(progressboxline, "%llu%% CDVDisoEFP Progress", thispct);

      SetWindowText(progressboxwindow, progressboxline);

      progressboxlastpct = thispct;

    } // ENDIF- Change in percentage? (Avoiding title flicker)

  } // ENDIF- Our maximum # over 100? (Avoiding divide-by-zero error)



  returnbool = PeekMessage(&msg, progressboxwindow, 0, 0, PM_REMOVE);

  while(returnbool != FALSE) {

    TranslateMessage(&msg);

    DispatchMessage(&msg);

    returnbool = PeekMessage(&msg, progressboxwindow, 0, 0, PM_REMOVE);

  } // ENDWHILE- Updating the progress window display as needed

  return;

} // END ProgressBoxTick()





void ProgressBoxStop() {

  ShowWindow(progressboxwindow, SW_HIDE);

  SetWindowText(progressboxwindow, "CDVDisoEFP Progress");

  return;

} // END ProgressBoxStop()





void ProgressBoxCancelEvent() {

  progressboxstop = 1;



  return;

} // END ProgressBoxCancelEvent()





BOOL CALLBACK ProgressBoxCallback(HWND window,

                                  UINT msg,

                                  WPARAM param,

                                  LPARAM param2) {

  switch(msg) {

    case WM_INITDIALOG:

      return(TRUE);

      break;



    case WM_CLOSE: // The "X" in the upper right corner is hit.

      ProgressBoxCancelEvent();

      return(TRUE);

      break;



    case WM_COMMAND:

      switch(LOWORD(param)) {

        case IDC_0504:

          ProgressBoxCancelEvent();

          return(TRUE);

          break;

      } // ENDSWITCH param- Which item got the message?



    // Hmm. Custom control? (for WM_GETFONT and WM_SETFONT msgs?)

  } // ENDSWITCH msg- what message has been sent to this window?



  return(FALSE); // Not a recognized message? Tell Windows to handle it.

} // ENDIF ProgressBoxCallback()





void ProgressBoxDisplay() {

  // ? progressload

  LPARAM range;



  InitCommonControls();

  // progressload.

  // progressload. = ICC_PROGRESS_CLASS

  // InitCommonControlsEx(&progressload);



  progressboxwindow = CreateDialog(progmodule,

                                   MAKEINTRESOURCE(DLG_0500),

                                   mainboxwindow,

                                   (DLGPROC) ProgressBoxCallback);



  progressboxbar = GetDlgItem(progressboxwindow, IDC_0502);

  range = MAKELPARAM(0, 30000); // Widen the range for granularity

  SendMessage(progressboxbar, PBM_SETRANGE, 0, range);



  ShowWindow(progressboxwindow, SW_SHOWNORMAL);

  ShowWindow(progressboxwindow, SW_HIDE);

  return;

} // END ProgressBoxDisplay()

