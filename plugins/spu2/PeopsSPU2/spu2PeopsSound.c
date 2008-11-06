/***************************************************************************
                         spu2PeopsSound.c  -  description
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
// 2004/03/28 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

// spu2PeopsSound.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

HINSTANCE hInst=NULL;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  dwReason, 
                       LPVOID lpReserved
					 )
{

 hInst=(HINSTANCE)hModule;

 return TRUE;
}

