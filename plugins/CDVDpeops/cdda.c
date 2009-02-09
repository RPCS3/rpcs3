/***************************************************************************
                           cdda.c  -  description
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

#include "stdafx.h"
#define _IN_CDDA
#include "externals.h"

/////////////////////////////////////////////////////////
// starts/stops audio playing (addr==0 -> stop)
// note: no cdda support in PS2 plugins yet

BOOL DoCDDAPlay(unsigned long addr)
{
 DWORD dw;

 LockGenCDAccess();

 if(addr) dw=PlaySCSIAudio(addr,lMaxAddr-addr);        // start playing (til end of cd)
// mmm... this stop doesn't work right
// else     dw=PlayFunc(0,1);
 else                                                  // funny stop... but seems to work
  {
   unsigned char cdb[3000];   
   FRAMEBUF * f=(FRAMEBUF *)cdb;

   f->dwFrame     = 16;                                // -> use an existing address (16 will ever exist on ps2 cds/dvds)
   f->dwFrameCnt  = 1;  
   f->dwBufLen    = 2352;

   dw=pReadFunc(1,f);                                  // -> do a simply sync read... seems to stop all audio playing
  }

 UnlockGenCDAccess();

 if(dw!=SS_COMP) return FALSE;
 return TRUE;
}

/////////////////////////////////////////////////////////
// get curr playing pos

unsigned char * GetCDDAPlayPosition(void)              
{
 unsigned char * pos;

 LockGenCDAccess();

 pos=GetSCSIAudioSub();                                // get the pos (scsi command)

 UnlockGenCDAccess();

 return pos;
}

/////////////////////////////////////////////////////////
