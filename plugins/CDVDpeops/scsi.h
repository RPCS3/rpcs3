/***************************************************************************
                           scsi.h  -  description
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
// 2002/09/28 - linuzappz
// - added GetSCSIStatus
//
// 2002/09/19 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

int GetSCSIDevice(int iA,int iT,int iL);
int GetSCSIStatus(int iA,int iT,int iL);
DWORD GetSCSITOC(LPTOC toc);
int GetSCSICDDrives(char * pDList);
DWORD PlaySCSIAudio(unsigned long start,unsigned long len);
unsigned char * GetSCSIAudioSub(void);
BOOL TestSCSIUnitReady(void);
DWORD SetSCSISpeed(DWORD dwSpeed);
DWORD ReadSCSI_BE(BOOL bWait,FRAMEBUF * f);
DWORD ReadSCSI_BE_Sub(BOOL bWait,FRAMEBUF * f);
DWORD ReadSCSI_BE_Sub_1(BOOL bWait,FRAMEBUF * f);
DWORD InitSCSI_28_1(void);
DWORD InitSCSI_28_2(void);
DWORD DeInitSCSI_28(void);
DWORD ReadSCSI_28(BOOL bWait,FRAMEBUF * f);
DWORD ReadSCSI_28_Sub(BOOL bWait,FRAMEBUF * f);
DWORD InitSCSI_28_2048(void);
DWORD ReadSCSI_28_2048(BOOL bWait,FRAMEBUF * f);
DWORD ReadSCSI_28_2048_Ex(BOOL bWait,FRAMEBUF * f);
int ReadSub_BE_2(unsigned long addr,unsigned char * pBuf,int iNum);
int ReadSub_BE_2_1(unsigned long addr,unsigned char * pBuf,int iNum);
int ReadSub_D8(unsigned long addr,unsigned char * pBuf,int iNum);
void DecodeSub_BE_2_1(unsigned char * pBuf);
DWORD CheckSCSIReadMode(void);
DWORD ReadSCSI_Dummy(BOOL bWait,FRAMEBUF * f);

