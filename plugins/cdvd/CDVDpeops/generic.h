/***************************************************************************
                         generic.h  -  description
                             -------------------
    begin                : Wed Sep 18 2002
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
// 2002/09/19 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

void  CreateGenEvent(void);
void  FreeGenEvent(void);
DWORD WaitGenEvent(DWORD dwMS);
void  LockGenCDAccess(void);
void  UnlockGenCDAccess(void);
void  WaitUntilDriveIsReady(void);
void  SetGenCDSpeed(int iReset);
void  GetBESubReadFunc(void);
int   GetGenReadFunc(int iRM);
int   OpenGenCD(int iA,int iT,int iL);
void  CloseGenCD(void);
void  OpenGenInterface(void);
void  CloseGenInterface(void);
int   GetGenCDDrives(char * pDList);

