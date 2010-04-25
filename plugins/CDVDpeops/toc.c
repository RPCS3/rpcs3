/***************************************************************************
                            toc.c  -  description
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

/////////////////////////////////////////////////////////

#include "stdafx.h"
#define _IN_TOC
#include "externals.h"

/////////////////////////////////////////////////////////

TOC sTOC;

/////////////////////////////////////////////////////////
// read toc


void ReadTOC(void)
{
 unsigned char xbuffer[4];DWORD dwStatus;

 LockGenCDAccess();

 memset(&(sTOC),0,sizeof(sTOC));                       // init toc infos

 dwStatus=GetSCSITOC((LPTOC)&sTOC);                    // get toc by scsi... may change that for ioctrl in xp/2k?

 UnlockGenCDAccess();

 if(dwStatus!=SS_COMP) return;

#ifdef DBGOUT
 auxprintf("TOC Last %d, max %08x,%08x\n",sTOC.cLastTrack,sTOC.tracks[sTOC.cLastTrack].lAddr,reOrder(sTOC.tracks[sTOC.cLastTrack].lAddr));
#endif
                                                      // re-order it to psemu pro standards
 addr2time(reOrder(sTOC.tracks[sTOC.cLastTrack].lAddr),xbuffer);

#ifdef DBGOUT
 auxprintf("TOC %d, %d, %d, %d\n",
           xbuffer[0],xbuffer[1],xbuffer[2],xbuffer[3]  );
#endif

 xbuffer[0]=itob(xbuffer[0]);
 xbuffer[1]=itob(xbuffer[1]);
 xbuffer[2]=itob(xbuffer[2]);
 xbuffer[3]=itob(xbuffer[3]);
 lMaxAddr=time2addrB(xbuffer);                         // get max data adr
}

/////////////////////////////////////////////////////////
// get the highest address of first (=data) track

unsigned long GetLastTrack1Addr(void)
{
 unsigned char xbuffer[4];DWORD dwStatus;
 unsigned long lmax;
 TOC xTOC;

 LockGenCDAccess();

 memset(&(xTOC),0,sizeof(xTOC));

 dwStatus=GetSCSITOC((LPTOC)&xTOC);

 UnlockGenCDAccess();

 if(dwStatus!=SS_COMP) return 0;

 addr2time(reOrder(xTOC.tracks[1].lAddr),xbuffer);

 xbuffer[0]=itob(xbuffer[0]);
 xbuffer[1]=itob(xbuffer[1]);
 xbuffer[2]=itob(xbuffer[2]);
 xbuffer[3]=itob(xbuffer[3]);

 lmax=time2addrB(xbuffer);
 if(lmax<150) return 0;

 return lmax-150;
}

/////////////////////////////////////////////////////////
