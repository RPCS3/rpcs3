/***************************************************************************
                            cfg.c  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2004/04/04 - Pete
// - changed plugin to emulate PS2 spu
//
// 2003/06/07 - Pete
// - added Linux NOTHREADLIB define
//
// 2003/02/28 - Pete
// - added option for kode54's interpolation and linuzappz's mono mode
//
// 2003/01/19 - Pete
// - added Neill's reverb
//
// 2002/08/04 - Pete
// - small linux bug fix: now the cfg file can be in the main emu directory as well
//
// 2002/06/08 - linuzappz
// - Added combo str for SPUasync, and MAXMODE is now defined as 2
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#define _IN_CFG

#include "externals.h"

////////////////////////////////////////////////////////////////////////
// WINDOWS CONFIG/ABOUT HANDLING
////////////////////////////////////////////////////////////////////////

#include "resource.h"

////////////////////////////////////////////////////////////////////////
// simple about dlg handler
////////////////////////////////////////////////////////////////////////

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch(uMsg)
  {
   case WM_COMMAND:
    {
     switch(LOWORD(wParam))
      {case IDOK:  EndDialog(hW,TRUE);return TRUE;}
    }
  }
 return FALSE;
}

////////////////////////////////////////////////////////////////////////
// READ CONFIG: from win registry
////////////////////////////////////////////////////////////////////////

// timer mode 2 (spuupdate sync mode) can be enabled for windows
// by setting MAXMODE to 2.
// Attention: that mode is not much tested, maybe the dsound buffers
// need to get adjusted to use that mode safely. Also please note:
// sync sound updates will _always_ cause glitches, if the system is
// busy by, for example, long lasting cdrom accesses. OK, you have
// be warned :)

#define MAXMODE 2
//#define MAXMODE 1

void ReadConfig(void)
{
 HKEY myKey;
 DWORD temp;
 DWORD type;
 DWORD size;
                                            // init vars
 iVolume=3;
 iDebugMode=0;
 iRecordMode=0;
 iUseReverb=0;
 iUseInterpolation=2;
 iUseTimer = 2;

 if(RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\PS2Eplugin\\SPU2\\PeopsSound",0,KEY_ALL_ACCESS,&myKey)==ERROR_SUCCESS)
  {
   size = 4;
   if(RegQueryValueEx(myKey,"Volume",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iVolume=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"DebugMode",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iDebugMode=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"RecordMode",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iRecordMode=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseReverb",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseReverb=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseInterpolation",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseInterpolation=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseTimer",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseTimer=(int)temp;


   RegCloseKey(myKey);
  }

 if(iVolume<1) iVolume=1;
 if(iVolume>5) iVolume=5;
}

////////////////////////////////////////////////////////////////////////
// WRITE CONFIG: in win registry
////////////////////////////////////////////////////////////////////////

void WriteConfig(void)
{
 HKEY myKey;
 DWORD myDisp;
 DWORD temp;

 RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\PS2Eplugin\\SPU2\\PeopsSound",0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&myKey,&myDisp);
 temp=iVolume;
 RegSetValueEx(myKey,"Volume",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iDebugMode;
 RegSetValueEx(myKey,"DebugMode",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iRecordMode;
 RegSetValueEx(myKey,"RecordMode",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseReverb;
 RegSetValueEx(myKey,"UseReverb",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseInterpolation;
 RegSetValueEx(myKey,"UseInterpolation",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseTimer;
 RegSetValueEx(myKey,"UseTimer",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 RegCloseKey(myKey);
}

////////////////////////////////////////////////////////////////////////
// INIT WIN CFG DIALOG
////////////////////////////////////////////////////////////////////////

BOOL OnInitDSoundDialog(HWND hW)
{
 HWND hWC;

 ReadConfig();

 hWC=GetDlgItem(hW,IDC_VOLUME);
 ComboBox_AddString(hWC, "0: Mute");
 ComboBox_AddString(hWC, "1: low");
 ComboBox_AddString(hWC, "2: medium");
 ComboBox_AddString(hWC, "3: loud");
 ComboBox_AddString(hWC, "4: loudest");
 ComboBox_SetCurSel(hWC,5-iVolume);
 if(iDebugMode)  CheckDlgButton(hW,IDC_DEBUGMODE,TRUE);
 if(iRecordMode) CheckDlgButton(hW,IDC_RECORDMODE,TRUE);
 if(iUseTimer==0)   CheckDlgButton(hW,IDC_TIMER,TRUE);

 hWC=GetDlgItem(hW,IDC_USEREVERB);
 ComboBox_AddString(hWC, "0: No reverb (fastest)");
 ComboBox_AddString(hWC, "1: SPU2 reverb (may be buggy, not tested yet)");
 ComboBox_SetCurSel(hWC,iUseReverb);

 hWC=GetDlgItem(hW,IDC_INTERPOL);
 ComboBox_AddString(hWC, "0: None (fastest)");
 ComboBox_AddString(hWC, "1: Simple interpolation");
 ComboBox_AddString(hWC, "2: Gaussian interpolation (good quality)");
 ComboBox_AddString(hWC, "3: Cubic interpolation (better treble)");
 ComboBox_SetCurSel(hWC,iUseInterpolation);

 return TRUE;
}

////////////////////////////////////////////////////////////////////////
// WIN CFG DLG OK
////////////////////////////////////////////////////////////////////////

void OnDSoundOK(HWND hW)
{
 HWND hWC;

 if(IsDlgButtonChecked(hW,IDC_TIMER))
  iUseTimer=0; else iUseTimer=2;

 hWC=GetDlgItem(hW,IDC_VOLUME);
 iVolume=5-ComboBox_GetCurSel(hWC);

 hWC=GetDlgItem(hW,IDC_USEREVERB);
 iUseReverb=ComboBox_GetCurSel(hWC);

 hWC=GetDlgItem(hW,IDC_INTERPOL);
 iUseInterpolation=ComboBox_GetCurSel(hWC);

 if(IsDlgButtonChecked(hW,IDC_DEBUGMODE))
  iDebugMode=1; else iDebugMode=0;

 if(IsDlgButtonChecked(hW,IDC_RECORDMODE))
  iRecordMode=1; else iRecordMode=0;

 WriteConfig();                                        // write registry

 EndDialog(hW,TRUE);
}

////////////////////////////////////////////////////////////////////////
// WIN CFG DLG CANCEL
////////////////////////////////////////////////////////////////////////

void OnDSoundCancel(HWND hW)
{
 EndDialog(hW,FALSE);
}

////////////////////////////////////////////////////////////////////////
// WIN CFG PROC
////////////////////////////////////////////////////////////////////////

BOOL CALLBACK DSoundDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch(uMsg)
  {
   case WM_INITDIALOG:
     return OnInitDSoundDialog(hW);

   case WM_COMMAND:
    {
     switch(LOWORD(wParam))
      {
       case IDCANCEL:     OnDSoundCancel(hW);return TRUE;
       case IDOK:         OnDSoundOK(hW);   return TRUE;
	  }
    }
  }
 return FALSE;
}

