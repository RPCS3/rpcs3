/***************************************************************************
                         externals.h  -  description
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

/////////////////////////////////////////////////////////

#ifndef _IN_CDDA

#endif

/////////////////////////////////////////////////////////

#ifndef _IN_CDR

extern BOOL bIsOpen;
extern BOOL bCDDAPlay;
extern int  iCDROK;
extern char *libraryName;
extern int  iCheckTrayStatus;
extern void *fdump;

#endif

/////////////////////////////////////////////////////////

#ifndef _IN_PEOPS

extern HINSTANCE hInst;

#endif

/////////////////////////////////////////////////////////

#ifndef _IN_CFG

#endif

/////////////////////////////////////////////////////////

#ifndef _IN_GENERIC

extern int        iCD_AD;
extern int        iCD_TA;
extern int        iCD_LU;
extern int        iRType;
extern int        iUseSpeedLimit;
extern int        iSpeedLimit;
extern int        iNoWait;
extern int        iMaxRetry;
extern int        iShowReadErr;
extern HANDLE     hEvent;
extern HINSTANCE  hASPI;
extern READFUNC   pReadFunc;
extern DEINITFUNC pDeInitFunc;
extern int        iInterfaceMode;
extern int        iWantedBlockSize;
extern int        iUsedBlockSize;
extern int        iUsedMode;
extern int        iBlockDump;

extern DWORD (*pGetASPI32SupportInfo)(void);
extern DWORD (*pSendASPI32Command)(LPSRB);

#endif

/////////////////////////////////////////////////////////

#ifndef _IN_IOCTL

extern HANDLE     hIOCTL;
extern DWORD      dwIOCTLAttr;
extern OVERLAPPED ovcIOCTL;
extern SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptIOCTL;
extern RAW_READ_INFO                        rawIOCTL;

#endif

/////////////////////////////////////////////////////////

#ifndef _IN_PPF

extern int  iUsePPF;
extern char szPPF[];
extern PPF_CACHE * ppfCache;
extern PPF_DATA  * ppfHead;
extern int         iPPFNum;

#endif

/////////////////////////////////////////////////////////

#ifndef _IN_READ

extern READTRACKFUNC   pReadTrackFunc;
extern GETPTRFUNC      pGetPtrFunc;

extern int iUseCaching;
extern int iUseDataCache;
extern int iTryAsync;
extern int iBufSel;

extern unsigned char * pMainBuffer;
extern unsigned char * pCurrReadBuf;
extern unsigned char * pFirstReadBuf;
extern unsigned char * pAsyncBuffer;

extern unsigned long   lMaxAddr;
extern unsigned long   lLastAddr;
extern unsigned long   lLastAsyncAddr;

extern unsigned char * ptrBuffer[];
extern unsigned char * pAsyncFirstReadBuf[];
extern unsigned long   lLastAccessedAddr;
extern int             iLastAccessedMode;

extern HANDLE          hReadThread;
extern BOOL            bThreadEnded;
extern HANDLE          hThreadEvent[];
extern HANDLE          hThreadMutex[];

#endif

/////////////////////////////////////////////////////////

#ifndef _IN_SCSI

extern SRB_ExecSCSICmd sx;
extern BOOL bDoWaiting;

#endif

/////////////////////////////////////////////////////////

#ifndef _IN_SUB

extern unsigned char * pCurrSubBuf;
extern int  iUseSubReading;
extern char szSUBF[];
extern SUB_CACHE * subCache;
extern SUB_DATA  * subHead;
extern int         iSUBNum;
extern unsigned char SubCData[];
extern unsigned char SubAData[];

#endif

/////////////////////////////////////////////////////////

#ifndef _IN_TOC

extern TOC sTOC;

#endif

/////////////////////////////////////////////////////////
