/***************************************************************************
                         cdvdPeops.c  -  description
                             -------------------
    begin                : Sun Nov 16 2003
    copyright            : (C) 2003 by Pete Bernert
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
// 2003/11/16 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

// cdrPeops.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "cdvdPeops.h"
#define _IN_PEOPS
#include "externals.h"

/////////////////////////////////////////////////////////

HINSTANCE hInst=0;

/////////////////////////////////////////////////////////
// get selected interface mode from registry: needed,
// if user has w2k and aspi available, so the plugin
// can know, what he wants to use

int iGetUserInterfaceMode(void)
{
 HKEY myKey;DWORD temp;DWORD type;DWORD size;
 int iRet=0;

 if(RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\PS2Eplugin\\CDVD\\CDVDPeops",0,KEY_ALL_ACCESS,&myKey)==ERROR_SUCCESS)
  {
   size = 4;
   if(RegQueryValueEx(myKey,"InterfaceMode",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iRet=(int)temp;
   RegCloseKey(myKey);
  }
 return iRet;
}

/////////////////////////////////////////////////////////
// dll entry point

BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
 hInst=(HINSTANCE)hModule;

 switch (ul_reason_for_call)
  {//--------------------------------------------------//
   case DLL_PROCESS_ATTACH:
    iInterfaceMode=iGetUserInterfaceMode();            // get interface on startup
    OpenGenInterface();                                // open interface (can be changed in the config window)
    break;
   //--------------------------------------------------//
   case DLL_PROCESS_DETACH:
    CloseGenInterface();                               // close interface
    break;
   //--------------------------------------------------//
   case DLL_THREAD_ATTACH:
    break;
   //--------------------------------------------------//
   case DLL_THREAD_DETACH:
    break;
   //--------------------------------------------------//
  }
 return TRUE;
}

/////////////////////////////////////////////////////////
