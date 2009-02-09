/***************************************************************************
                          freeze.c  -  description
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
// 2004/12/24 - Pete
// - freeze functions adapted to pcsx2-0.7
//
// 2004/04/04 - Pete
// - changed plugin to emulate PS2 spu
//
// 2003/03/20 - Pete
// - fix to prevent the new interpolations from crashing when loading a save state
//
// 2003/01/06 - Pete
// - small changes for version 1.3 adsr save state loading      
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#define _IN_FREEZE

#include "externals.h"
#include "registers.h"
#include "spu.h"
#include "regs.h"
#include "debug.h"
#include "resource.h"

////////////////////////////////////////////////////////////////////////
// freeze structs
////////////////////////////////////////////////////////////////////////

typedef struct {
	int size;
	char *data;
} SPUFreeze_t;

typedef struct
{
 char          szSPUName[8];
 unsigned long ulFreezeVersion;
 unsigned long ulFreezeSize;
 unsigned char cSPUPort[64*1024];
 unsigned char cSPURam[2*1024*1024];
} SPUFreeze_Ex_t;

typedef struct
{
 unsigned long   spuIrq0;
 unsigned long   spuIrq1;
 unsigned long   pSpuIrq0;
 unsigned long   pSpuIrq1;
 unsigned long   dummy0;
 unsigned long   dummy1;
 unsigned long   dummy2;
 unsigned long   dummy3;

 SPUCHAN  s_chan[MAXCHAN];   

} SPUOSSFreeze_t;

////////////////////////////////////////////////////////////////////////

void LoadStateV1(SPUFreeze_Ex_t * pF);                    // newest version
void LoadStateUnknown(SPUFreeze_Ex_t * pF);               // unknown format

////////////////////////////////////////////////////////////////////////
// SPUFREEZE: called by main emu on savestate load/save
////////////////////////////////////////////////////////////////////////

EXPORT_GCC long CALLBACK SPU2freeze(unsigned long ulFreezeMode,SPUFreeze_t * pFt)
{
 int i;SPUOSSFreeze_t * pFO;SPUFreeze_Ex_t * pF;

 if(!pFt) return 0;                                    // first check

 if(ulFreezeMode)                                      // save?
  {//--------------------------------------------------//
   pFt->size=sizeof(SPUFreeze_Ex_t)+sizeof(SPUOSSFreeze_t);

   if(ulFreezeMode==2) return 0;                       // emu just asking for size? bye

   if(!pFt->data) return 0;
   
   pF=(SPUFreeze_Ex_t *)pFt->data;
  
   memset(pF,0,pFt->size);

   strcpy(pF->szSPUName,"PBOSS2");
   pF->ulFreezeVersion=1;
   pF->ulFreezeSize=pFt->size;
                                                       // save mode:
   RemoveTimer();                                      // stop timer

   memcpy(pF->cSPURam,spuMem,2*1024*1024);             // copy common infos
   memcpy(pF->cSPUPort,regArea,64*1024);

   pFO=(SPUOSSFreeze_t *)(pF+1);                       // store special stuff

   pFO->spuIrq0=spuIrq2[0];
   if(pSpuIrq[0])  pFO->pSpuIrq0 = (unsigned long)pSpuIrq[0]-(unsigned long)spuMemC;
   pFO->spuIrq1=spuIrq2[1];
   if(pSpuIrq[1])  pFO->pSpuIrq1 = (unsigned long)pSpuIrq[1]-(unsigned long)spuMemC;

   for(i=0;i<MAXCHAN;i++)
    {
     memcpy((void *)&pFO->s_chan[i],(void *)&s_chan[i],sizeof(SPUCHAN));
     if(pFO->s_chan[i].pStart)
      pFO->s_chan[i].pStart-=(unsigned long)spuMemC;
     if(pFO->s_chan[i].pCurr)
      pFO->s_chan[i].pCurr-=(unsigned long)spuMemC;
     if(pFO->s_chan[i].pLoop)
      pFO->s_chan[i].pLoop-=(unsigned long)spuMemC;
    }

   SetupTimer();                                       // sound processing on again

   return 1;
   //--------------------------------------------------//
  }

                                                       // load state:
#ifdef _WINDOWS
 if(iDebugMode==1 && IsWindow(hWDebug))                // we have to disbale the debug window, if active
  DestroyWindow(hWDebug);
 hWDebug=0;

 if(IsBadReadPtr(pFt,sizeof(SPUFreeze_t)))             // check bad emu stuff
  return 0;
#endif

 if(pFt->size!=sizeof(SPUFreeze_Ex_t)+                 // not our stuff? bye
               sizeof(SPUOSSFreeze_t)) return 0;
 if(!pFt->data) return 0;

 pF=(SPUFreeze_Ex_t *)pFt->data;

 RemoveTimer();                                        // we stop processing while doing the save!

 memcpy(spuMem,pF->cSPURam,2*1024*1024);               // get ram
 memcpy(regArea,pF->cSPUPort,64*1024);

 if(!strcmp(pF->szSPUName,"PBOSS2") &&                  
    pF->ulFreezeVersion==1)
      LoadStateV1(pF);
 else LoadStateUnknown(pF);

 // repair some globals
 for(i=0x7FFE;i>=0x0000;i-=2)
  {
   SPU2write(i,regArea[i]);
  }

 // fix to prevent new interpolations from crashing
 for(i=0;i<MAXCHAN;i++) s_chan[i].SB[28]=0;

 SetupTimer();                                         // start sound processing again

#ifdef _WINDOWS
 if(iDebugMode)                                        // re-activate windows debug dialog
  {
   hWDebug=CreateDialog(hInst,MAKEINTRESOURCE(IDD_DEBUG),
                        NULL,(DLGPROC)DebugDlgProc);
   SetWindowPos(hWDebug,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
   UpdateWindow(hWDebug);
   SetFocus(hWMain);
  }
#endif

 return 1;
}

////////////////////////////////////////////////////////////////////////

void LoadStateV1(SPUFreeze_Ex_t * pF)
{
 int i;SPUOSSFreeze_t * pFO;

 pFO=(SPUOSSFreeze_t *)(pF+1);

 spuIrq2[0]   = pFO->spuIrq0;
 if(pFO->pSpuIrq0)  pSpuIrq[0]  = pFO->pSpuIrq0+spuMemC;  else pSpuIrq[0]=0;
 spuIrq2[1]   = pFO->spuIrq1;
 if(pFO->pSpuIrq1)  pSpuIrq[1]  = pFO->pSpuIrq1+spuMemC;  else pSpuIrq[1]=0;

 for(i=0;i<MAXCHAN;i++)
  {
   memcpy((void *)&s_chan[i],(void *)&pFO->s_chan[i],sizeof(SPUCHAN));

   s_chan[i].pStart+=(unsigned long)spuMemC;
   s_chan[i].pCurr+=(unsigned long)spuMemC;
   s_chan[i].pLoop+=(unsigned long)spuMemC;
   s_chan[i].iMute=0;
   s_chan[i].iIrqDone=0;
  }
}

////////////////////////////////////////////////////////////////////////

void LoadStateUnknown(SPUFreeze_Ex_t * pF)
{
 int i;

 for(i=0;i<MAXCHAN;i++)
  {
   s_chan[i].bOn=0;
   s_chan[i].bNew=0;
   s_chan[i].bStop=0;
   s_chan[i].ADSR.lVolume=0;
   s_chan[i].pLoop=spuMemC;
   s_chan[i].pStart=spuMemC;
   s_chan[i].iMute=0;
   s_chan[i].iIrqDone=0;
  }

 dwNewChannel2[0]=0;
 dwNewChannel2[1]=0;
 dwEndChannel2[0]=0;
 dwEndChannel2[1]=0;
 
 pSpuIrq[0]=0;
 pSpuIrq[1]=0;
}

////////////////////////////////////////////////////////////////////////
