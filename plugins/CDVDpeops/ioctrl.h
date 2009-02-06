/***************************************************************************
                          ioctrl.h  -  description
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

void OpenIOCTLHandle(int iA,int iT,int iL);
void CloseIOCTLHandle(void);
char MapIOCTLDriveLetter(int iA,int iT,int iL);
int  GetIOCTLCDDrives(char * pDList);
HANDLE OpenIOCTLFile(char cLetter,BOOL bAsync);
void GetIOCTLAdapter(HANDLE hF,int * iDA,int * iDT,int * iDL);
DWORD IOCTLSendASPI32Command(LPSRB pSrb);
DWORD ReadIOCTL_Raw(BOOL bWait,FRAMEBUF * f);
DWORD ReadIOCTL_Raw_Sub(BOOL bWait,FRAMEBUF * f);
