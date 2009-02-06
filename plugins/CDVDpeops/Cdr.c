/***************************************************************************
                            cdr.c  -  description
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
// 2004/12/25 - Pete
// - added an hack in CDVDgetTD for big dvds 
//
// 2003/11/16 - Pete
// - generic cleanup for the Peops cdvd release
//
//*************************************************************************//

/////////////////////////////////////////////////////////

#include "stdafx.h"
#include <time.h>
#include "resource.h"
#define _IN_CDR
#include "externals.h"
#define CDVDdefs
#include "PS2Etypes.h"
#include "PS2Edefs.h"
#include "libiso.h"

#ifdef DBGOUT
#define SMALLDEBUG 1   
#include <dbgout.h>
#endif

/////////////////////////////////////////////////////////
// PCSX2 CDVD interface:

EXPORT_GCC char *          CALLBACK PS2EgetLibName();
EXPORT_GCC unsigned long   CALLBACK PS2EgetLibType();
EXPORT_GCC unsigned long   CALLBACK PS2EgetLibVersion2(unsigned long type);
EXPORT_GCC long            CALLBACK CDVDinit();
EXPORT_GCC void            CALLBACK CDVDshutdown();
EXPORT_GCC long            CALLBACK CDVDopen(const char* pTitle);
EXPORT_GCC void            CALLBACK CDVDclose();
EXPORT_GCC long            CALLBACK CDVDtest();
EXPORT_GCC long            CALLBACK CDVDreadTrack(unsigned long lsn, int mode);
EXPORT_GCC unsigned char * CALLBACK CDVDgetBuffer();
EXPORT_GCC long            CALLBACK CDVDgetTN(cdvdTN *Buffer);
EXPORT_GCC long            CALLBACK CDVDgetTD(unsigned char track, cdvdTD *Buffer);
EXPORT_GCC long            CALLBACK CDVDgetDiskType();
EXPORT_GCC long            CALLBACK CDVDgetTrayStatus();

/////////////////////////////////////////////////////////
                                         
const unsigned char version  =  PS2E_CDVD_VERSION;
const unsigned char revision =  1;
const unsigned char build    =  3;

#ifdef _DEBUG
char *libraryName            =  "P.E.Op.S. CDVD (Debug, CDDA mod)";
#else
char *libraryName            =  "P.E.Op.S. CDVD (CDDA mod)";
#endif

/////////////////////////////////////////////////////////

BOOL bIsOpen=FALSE;                                    // flag: open called once
BOOL bCDDAPlay=FALSE;                                  // flag: audio is playing
int  iCDROK=0;                                         // !=0: cd is ok
int  iCDType=CDVD_TYPE_UNKNOWN;                        // CD/DVD
int  iCheckTrayStatus=0;                               // if 0 : report tray as closed, else try a real check
void *fdump;

/////////////////////////////////////////////////////////
// usual info funcs

EXPORT_GCC char * CALLBACK PS2EgetLibName()
{
 return libraryName;
}
                             
EXPORT_GCC unsigned long CALLBACK PS2EgetLibType()
{
 return PS2E_LT_CDVD;
}

EXPORT_GCC unsigned long CALLBACK PS2EgetLibVersion2(unsigned long type)
{
 return version<<16|revision<<8|build;
}
/*
EXPORT_GCC unsigned long CALLBACK PS2EgetCpuPlatform(void)
{
 return PS2E_X86;
// return PS2E_X86_64;
}*/

s32 msf_to_lba(u8 m, u8 s, u8 f) {
	u32 lsn;
	lsn = f;
	lsn+=(s - 2) * 75;
	lsn+= m * 75 * 60;
	return lsn;
}

void lba_to_msf(s32 lba, u8* m, u8* s, u8* f) {
	lba += 150;
	*m = (u8)(lba / (60*75));
	*s = (u8)((lba / 75) % 60);
	*f = (u8)(lba % 75);
}

/////////////////////////////////////////////////////////
// init: called once at library load

EXPORT_GCC long CALLBACK CDVDinit()
{
 szSUBF[0]=0;                                          // just init the filename buffers
 szPPF[0] =0;
 return 0;                                            
}

/////////////////////////////////////////////////////////
// shutdown: called once at final exit

EXPORT_GCC void CALLBACK CDVDshutdown()
{
}

/////////////////////////////////////////////////////////
// open: called, when games starts/cd has been changed

int CheckDiskType(int baseType);

EXPORT_GCC long CALLBACK CDVDopen(const char* pTitle)
{
	int i,audioTracks,dataTracks;
	cdvdTD T;
 if(bIsOpen)                                           // double-open check (if the main emu coder doesn't know what he is doing ;)
  {
   if(iCDROK<=0) return -1;
   else          return 0;
  }

 bIsOpen=TRUE;                                         // ok, open func called once
 
 ReadConfig();                                         // read user config

 BuildPPFCache();                                      // build ppf cache

 BuildSUBCache();                                      // build sub cache

 CreateREADBufs();                                     // setup generic read buffers

 CreateGenEvent();                                     // create read event

 iCDROK=OpenGenCD(iCD_AD,iCD_TA,iCD_LU);               // generic open, setup read func

 if(iCDROK<=0) {iCDROK=0;return -1;}

 ReadTOC();                                            // read the toc

 SetGenCDSpeed(0);                                     // try to change the reading speed (if wanted)

 iCDType=CDVD_TYPE_UNKNOWN;                            // let's look after the disc type
                                                       // (funny stuff taken from Xobro's/Florin's bin plugin)
 if(CDVDreadTrack(16,CDVD_MODE_2048)==0)
  {
   struct cdVolDesc *volDesc;
   volDesc=(struct cdVolDesc *)CDVDgetBuffer();
   if(volDesc)
    {                                 

//todo: CDVD_TYPE_CDDA
        
     if(volDesc->rootToc.tocSize==2048)
          iCDType = CDVD_TYPE_DETCTCD;
     else iCDType = CDVD_TYPE_DETCTDVDS;
    }
  }

  fprintf(stderr," * CDVD Disk Open: %d tracks (%d to %d):\n",sTOC.cLastTrack-sTOC.cFirstTrack+1,sTOC.cFirstTrack,sTOC.cLastTrack);

  audioTracks=dataTracks=0;
  for(i=sTOC.cFirstTrack;i<=sTOC.cLastTrack;i++)
  {
      CDVDgetTD(i,&T);
	  if(T.type==CDVD_AUDIO_TRACK) {
		  audioTracks++;
		  fprintf(stderr," * * Track %d: Audio (%d sectors)\n",i,T.lsn);
	  }
	  else {
		  dataTracks++;
		  fprintf(stderr," * * Track %d: Data (Mode %d) (%d sectors)\n",i,((T.type==CDVD_MODE1_TRACK)?1:2),T.lsn);
	  }
  }
  if((dataTracks==0)&&(audioTracks>0))
	iCDType=CDVD_TYPE_CDDA;
  else if(dataTracks>0)
	  iCDType=CheckDiskType(iCDType);

  if((iCDType==CDVD_TYPE_ILLEGAL)&&(audioTracks>0))
	  iCDType=CDVD_TYPE_CDDA;
  else if((iCDType==CDVD_TYPE_PS2CD)&&(audioTracks>0))
	  iCDType=CDVD_TYPE_PS2CDDA;
  else if((iCDType==CDVD_TYPE_PSCD)&&(audioTracks>0))
	  iCDType=CDVD_TYPE_PSCDDA;

  switch(iCDType) {
	case CDVD_TYPE_ILLEGAL: // Illegal Disc
		fprintf(stderr," * Disk Type: Illegal Disk.\n");break;
	case CDVD_TYPE_DVDV: // DVD Video
		fprintf(stderr," * Disk Type: DVD Video.\n");break;
	case CDVD_TYPE_CDDA: // Audio CD
		fprintf(stderr," * Disk Type: CDDA.\n");break;
	case CDVD_TYPE_PS2DVD: // PS2 DVD
		fprintf(stderr," * Disk Type: PS2 DVD.\n");break;
	case CDVD_TYPE_PS2CDDA: // PS2 CD (with audio)
		fprintf(stderr," * Disk Type: PS2 CD+Audio.\n");break;
	case CDVD_TYPE_PS2CD: // PS2 CD
		fprintf(stderr," * Disk Type: PS2 CD.\n");break;
	case CDVD_TYPE_PSCDDA: // PS CD (with audio)
		fprintf(stderr," * Disk Type: PS1 CD+Audio.\n");break;
	case CDVD_TYPE_PSCD: // PS CD
		fprintf(stderr," * Disk Type: PS1 CD.\n");break;
	case CDVD_TYPE_UNKNOWN: // Unknown
		fprintf(stderr," * Disk Type: Unknown.\n");break;
	case CDVD_TYPE_NODISC: // No Disc
		fprintf(stderr," * Disk Type: No Disc.\n");break;
  }

/*	if (iBlockDump)*/ {
//		fdump = isoCreate("block.dump", ISOFLAGS_BLOCKDUMP);
		fdump = NULL;
		if (fdump) {
			cdvdTD buf;
			CDVDgetTD(0, &buf);
			isoSetFormat(fdump, 0, 2352, buf.lsn);
		}
	} /*else {
		fdump = NULL;
	}*/


 return 0;                                             // ok, done
}

/////////////////////////////////////////////////////////
// close: called when emulation stops

EXPORT_GCC void CALLBACK CDVDclose()
{
 if(!bIsOpen) return;                                  // no open? no close...

	if (fdump != NULL) {
		isoClose(fdump);
	}
 bIsOpen=FALSE;                                        // no more open

 LockGenCDAccess();                                    // make sure that no more reading is happening

 if(iCDROK)                                            // cd was ok?
  {
   if(bCDDAPlay) {DoCDDAPlay(0);bCDDAPlay=FALSE;}      // -> cdda playing? stop it
   SetGenCDSpeed(1);                                   // -> repair speed
   CloseGenCD();                                       // -> cd not used anymore
  }

 UnlockGenCDAccess();

 FreeREADBufs();                                       // free read bufs
 FreeGenEvent();                                       // free event
 FreePPFCache();                                       // free ppf cache
 FreeSUBCache();                                       // free sub cache
}

/////////////////////////////////////////////////////////
// test: ah, well, always fine

EXPORT_GCC long CALLBACK CDVDtest()
{
 return 0;
}            

/////////////////////////////////////////////////////////
// readSubQ: read subq from disc (only cds have subq data)
EXPORT_GCC long CALLBACK CDVDreadSubQ(u32 lsn, cdvdSubQ* subq)
{
	u8 min, sec, frm;

	if(!bIsOpen)   CDVDopen("DVD");                            // usual checks
	if(!iCDROK)    return -1;

	// fake it
	subq->ctrl		= 4;
	subq->mode		= 1;
	subq->trackNum	= itob(1);
	subq->trackIndex= itob(1);
	
	lba_to_msf(lsn, &min, &sec, &frm);
	subq->trackM	= itob(min);
	subq->trackS	= itob(sec);
	subq->trackF	= itob(frm);
	
	subq->pad		= 0;
	
	lba_to_msf(lsn + (2*75), &min, &sec, &frm);
	subq->discM		= itob(min);
	subq->discS		= itob(sec);
	subq->discF		= itob(frm);
	return 0;
}

/////////////////////////////////////////////////////////
// gettoc: ps2 style TOC
static int layer1start = -1;
EXPORT_GCC long CALLBACK CDVDgetTOC(void* toc)
{
	u32 type;
	u8* tocBuff = (u8*)toc;

	if(!bIsOpen)  CDVDopen("DVD");                             // not open? funny emu...

 if(!iCDROK) return -1;                                  // cd not ok? 
   
  type = CDVDgetDiskType();

	if(	type == CDVD_TYPE_DVDV ||
		type == CDVD_TYPE_PS2DVD)
	{
        u32 lastaddr;

        // get dvd structure format
		// scsi command 0x43
		memset(tocBuff, 0, 2048);
		
        lastaddr = GetLastTrack1Addr();
        if(layer1start > 0 || (layer1start != -2 && lastaddr > 0x280000) ) {
            int off = 0;
            FRAMEBUF* f = (FRAMEBUF*)malloc(sizeof(FRAMEBUF));
        
            f->dwBufLen = iUsedBlockSize;
            f->dwFrameCnt = 1;


            // dual sided
            tocBuff[ 0] = 0x24;
		    tocBuff[ 1] = 0x02;
		    tocBuff[ 2] = 0xF2;
		    tocBuff[ 3] = 0x00;
		    tocBuff[ 4] = 0x41;
		    tocBuff[ 5] = 0x95;

            tocBuff[14] = 0x60; // dual sided, ptp

		    tocBuff[16] = 0x00;
		    tocBuff[17] = 0x03;
		    tocBuff[18] = 0x00;
		    tocBuff[19] = 0x00;

            if( layer1start == -1 ) {
                // search for it
                printf("PeopsCDVD: searching for layer1... ");
                for(layer1start = (lastaddr/2-0x10)&~0xf; layer1start < 0x200010; layer1start += 16) {
                    f->dwFrame = layer1start;
                    if( pReadFunc(TRUE,f) != SS_COMP ) {
                        layer1start = 0x200010;
                        break;
                    }
                    // CD001
                    if( f->BufData[off+1] == 0x43 && f->BufData[off+2] == 0x44 && f->BufData[off+3] == 0x30 && f->BufData[off+4] == 0x30 && f->BufData[off+5] == 0x31 ) {
                        break;
                    }
                }

                if( layer1start >= 0x200010 ) {
                    printf("Couldn't find second layer on dual layer... ignoring\n");
                    // fake it
		            tocBuff[ 0] = 0x04;
		            tocBuff[ 1] = 0x02;
		            tocBuff[ 2] = 0xF2;
		            tocBuff[ 3] = 0x00;
		            tocBuff[ 4] = 0x86;
		            tocBuff[ 5] = 0x72;

		            tocBuff[16] = 0x00;
		            tocBuff[17] = 0x03;
		            tocBuff[18] = 0x00;
		            tocBuff[19] = 0x00;
                    layer1start = -2;
                    return 0;
                }

                printf("found at 0x%8.8x\n", layer1start);
                layer1start = layer1start+0x30000-1;
            }

            tocBuff[20] = layer1start>>24;
		    tocBuff[21] = (layer1start>>16)&0xff;
		    tocBuff[22] = (layer1start>>8)&0xff;
		    tocBuff[23] = (layer1start>>0)&0xff;

            free(f);
        }
        else {
		    // fake it
		    tocBuff[ 0] = 0x04;
		    tocBuff[ 1] = 0x02;
		    tocBuff[ 2] = 0xF2;
		    tocBuff[ 3] = 0x00;
		    tocBuff[ 4] = 0x86;
		    tocBuff[ 5] = 0x72;

		    tocBuff[16] = 0x00;
		    tocBuff[17] = 0x03;
		    tocBuff[18] = 0x00;
		    tocBuff[19] = 0x00;
        }
	}
	else if(type == CDVD_TYPE_CDDA ||
			type == CDVD_TYPE_PS2CDDA ||
			type == CDVD_TYPE_PS2CD ||
			type == CDVD_TYPE_PSCDDA ||
			type == CDVD_TYPE_PSCD)
	{
		// cd toc
		// (could be replaced by 1 command that reads the full toc)
		u8 min, sec, frm,i;
		s32 err;
		cdvdTN diskInfo;
		cdvdTD trackInfo;
		memset(tocBuff, 0, 1024);
		if (CDVDgetTN(&diskInfo) == -1)	{ diskInfo.etrack = 0;diskInfo.strack = 1; }
		if (CDVDgetTD(0, &trackInfo) == -1) trackInfo.lsn = 0;
		
		tocBuff[0] = 0x41;
		tocBuff[1] = 0x00;
		
		//Number of FirstTrack
		tocBuff[2] = 0xA0;
		tocBuff[7] = itob(diskInfo.strack);
		
		//Number of LastTrack
		tocBuff[12] = 0xA1;
		tocBuff[17] = itob(diskInfo.etrack);

		//DiskLength
		lba_to_msf(trackInfo.lsn, &min, &sec, &frm);
		tocBuff[22] = 0xA2;
		tocBuff[27] = itob(min);
		tocBuff[28] = itob(sec);
		tocBuff[29] = itob(frm);

		fprintf(stderr,"Track 0: %d mins %d secs %d frames\n",min,sec,frm);
		
		for (i=diskInfo.strack; i<=diskInfo.etrack; i++)
		{
			err = CDVDgetTD(i, &trackInfo);
			lba_to_msf(trackInfo.lsn, &min, &sec, &frm);
			tocBuff[i*10+30] = trackInfo.type;
			tocBuff[i*10+32] = err == -1 ? 0 : itob(i);	  //number
			tocBuff[i*10+37] = itob(min);
			tocBuff[i*10+38] = itob(sec);
			tocBuff[i*10+39] = itob(frm);
			fprintf(stderr,"Track %d: %d mins %d secs %d frames\n",i,min,sec,frm);
		}
	}
	else
		return -1;
	
	return 0;
}

/////////////////////////////////////////////////////////
// gettn: first/last track num

EXPORT_GCC long CALLBACK CDVDgetTN(cdvdTN *Buffer)
{
 if(!bIsOpen)  CDVDopen("DVD");                             // not open? funny emu...

 if(!iCDROK)                                           // cd not ok? 
  {
   Buffer->strack=1;
   Buffer->etrack=1;
   return -1;
  }      

 ReadTOC();                                            // read the TOC

 Buffer->strack=sTOC.cFirstTrack;                      // get the infos
 Buffer->etrack=sTOC.cLastTrack;

 return 0;
}

/////////////////////////////////////////////////////////
// gettd: track addr

EXPORT_GCC long CALLBACK CDVDgetTD(unsigned char track, cdvdTD *Buffer)
{
 unsigned long lu,i;
 unsigned char buffer[2352];
 unsigned char *buf;
 u8 t1;

 if(!bIsOpen) CDVDopen("DVD");                              // not open? funny emu...
 
 if(!iCDROK)  return -1;                               // cd not ok? bye

 ReadTOC();                                            // read toc
 
/*
// PSEmu style:
 if(track==0)                                          // 0 = last track
  {
   lu=reOrder(sTOC.tracks[sTOC.cLastTrack].lAddr);
   addr2time(lu,buffer);
  }
 else                                                  // others: track n
  {
   lu=reOrder(sTOC.tracks[track-1].lAddr);
   addr2time(lu,buffer);
  }

 Buffer->minute = buffer[1];
 Buffer->second = buffer[2];
 Buffer->frame  = buffer[3];
 Buffer->type   = iCDType;
#ifdef DBGOUT  	
 auxprintf("Read Toc %d: %u\n",track,lu);
#endif 
*/

 lu=0;
 if(track==0)
	lu=reOrder(sTOC.tracks[sTOC.cLastTrack].lAddr);
 else
	lu=reOrder(sTOC.tracks[track].lAddr);
 //addr2time(lu,buffer);

 Buffer->lsn=lu;

 if(track==0)
      Buffer->type   = iCDType;
 else 
 {
	lu=0;
	for(i=sTOC.cFirstTrack;i<track;i++)
		lu+=sTOC.tracks[i].lAddr;

		CDVDreadTrack(lu+16,CDVD_MODE_2352); 
		buf=CDVDgetBuffer();
		
		if(buf!=NULL) memcpy(buffer,buf,2352);
		else		  memset(buffer,0,2352);

		if( (buffer[16]==01)
		&&(buffer[17]=='C')
		&&(buffer[18]=='D')
		)
			t1   = CDVD_MODE1_TRACK;
		else if( (buffer[24]==01)
			&&(buffer[25]=='C')
			&&(buffer[26]=='D')
			)
			t1   = CDVD_MODE2_TRACK;
		else t1   = CDVD_AUDIO_TRACK;

		Buffer->type=t1;
 }

 return 0;
}

/////////////////////////////////////////////////////////
// readtrack: start reading at given address

EXPORT_GCC long CALLBACK CDVDreadTrack(unsigned long lsn, int mode)
{
 if(!bIsOpen)   CDVDopen("DVD");                            // usual checks
 if(!iCDROK)    return -1;
 if(bCDDAPlay)  bCDDAPlay=FALSE;

#ifdef DBGOUT  	
 auxprintf("Read Track %u: %d\n",lsn,mode);
#endif

 lLastAccessedAddr=lsn;                                // store read track values (for getbuffer)
 iLastAccessedMode=mode;

 if(!pReadTrackFunc(lLastAccessedAddr))                // start reading
  return -1;

 return 0;
}

/////////////////////////////////////////////////////////
// getbuffer: will be called after readtrack, to get ptr 
//            to data

// small helper buffer to get bigger block sizes
unsigned char cDataAndSub[2368];

EXPORT_GCC unsigned char * CALLBACK CDVDgetBuffer()
{
 unsigned char * pbuffer;

 if(!bIsOpen) CDVDopen("DVD");

 if(pGetPtrFunc) pGetPtrFunc();                        // get ptr on thread modes

 pbuffer=pCurrReadBuf;                                 // init buffer pointer
	if (fdump != NULL) {
		isoWriteBlock(fdump, pbuffer, lLastAccessedAddr);
	}

 if(iLastAccessedMode!=iUsedMode)
  {
   switch(iLastAccessedMode)                           // what does the emu want?
    {//------------------------------------------------//
     case CDVD_MODE_2048:
      {
       if(iUsedBlockSize==2352) pbuffer+=24;
      }break;
     //------------------------------------------------//
     case CDVD_MODE_2352:
      {
       if(iUsedBlockSize==2048) 
        {
         memset(cDataAndSub,0,2368);        
         memcpy(cDataAndSub+24,pbuffer,2048);
         pbuffer=cDataAndSub;
        }
      }break;
     //------------------------------------------------// 
     case CDVD_MODE_2340:
      {
       if(iUsedBlockSize==2048) 
        {
         memset(cDataAndSub,0,2368);        
         memcpy(cDataAndSub+12,pbuffer,2048);
         pbuffer=cDataAndSub;
        }
       else pbuffer+=12;
      }break;
     //------------------------------------------------// 
     case CDVD_MODE_2328:
      {
       if(iUsedBlockSize==2048) 
        {
         memset(cDataAndSub,0,2368);        
         memcpy(cDataAndSub+0,pbuffer,2048);
         pbuffer=cDataAndSub;
        }
       else pbuffer+=24; 
      }break;
     //------------------------------------------------//
     case CDVD_MODE_2368:
      {
       if(iUsedBlockSize==2048) 
        {
         memset(cDataAndSub,0,2368);        
         memcpy(cDataAndSub+24,pbuffer,2048);
         pbuffer=cDataAndSub;
         
/*
// NO SUBCHANNEL SUPPORT RIGHT NOW!!!
    {
     if(subHead)                                           // some sub file?
      CheckSUBCache(lLastAccessedAddr);                    // -> get cached subs
     else 
     if(iUseSubReading!=1 && pCurrSubBuf)                  // no direct cd sub read?
      FakeSubData(lLastAccessedAddr);                      // -> fake the data
     memcpy(cDataAndSub,pCurrReadBuf,2352);
     if(pCurrSubBuf)
      memcpy(cDataAndSub+2352,pCurrSubBuf+12,16);
     pbuffer=cDataAndSub;
    }
*/
         
        }
      }break;
     //------------------------------------------------// 
    }
  }

#ifdef DBGOUT  	
 auxprintf("get buf %d\n",iLastAccessedMode);

/*
{
 int k;
 for(k=0;k<2352;k++)
  auxprintf("%02x ",*(pbuffer+k));
 auxprintf("\n\n"); 
}
*/
#endif

 return pbuffer;            
}

/////////////////////////////////////////////////////////

EXPORT_GCC long CALLBACK CDVDgetDiskType() 
{
 return iCDType;
}

/////////////////////////////////////////////////////////
// CDVDgetTrayStatus

EXPORT_GCC long CALLBACK CDVDgetTrayStatus() 
{
 static time_t to=0;
 static long lLastTrayState=CDVD_TRAY_CLOSE;

 if(to==time(NULL)) return lLastTrayState;               // we only check once per second
 to = time(NULL);

 lLastTrayState=CDVD_TRAY_CLOSE;                         // init state with "closed"

 if(iCheckTrayStatus)                                    // user really want a tray check
  {
   int iStatus;
   
   LockGenCDAccess();                                    // make sure that no more reading is happening
   iStatus=GetSCSIStatus(iCD_AD,iCD_TA,iCD_LU);          // get device status
   UnlockGenCDAccess();

   if(iStatus==SS_ERR) 
    lLastTrayState=CDVD_TRAY_OPEN;
  }  

#ifdef DBGOUT  	
auxprintf("check %d -> %d\n",to,lLastTrayState);
#endif


 return lLastTrayState;
}

EXPORT_GCC s32  CALLBACK CDVDctrlTrayOpen() {
	return 0;
}

EXPORT_GCC s32  CALLBACK CDVDctrlTrayClose() {
	return 0;
}



/////////////////////////////////////////////////////////
// configure: shows config window

EXPORT_GCC void CALLBACK CDVDconfigure()
{
 if(iCDROK)                                            // mmm... someone has already called Open? bad
  {MessageBeep((UINT)-1);return;}

 CreateGenEvent();                                     // we need an event handle

 DialogBox(hInst,MAKEINTRESOURCE(IDD_CONFIG),          // call dialog
           GetActiveWindow(),(DLGPROC)CDRDlgProc);

 FreeGenEvent();                                       // free event handle
}

/////////////////////////////////////////////////////////
// about: shows about window

EXPORT_GCC void CALLBACK CDVDabout()
{
 DialogBox(hInst,MAKEINTRESOURCE(IDD_ABOUT),
           GetActiveWindow(),(DLGPROC)AboutDlgProc);
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

/*
// CURRENTLY UNUSED OLD STUFF FROM PSX CD PLUGIN:

/////////////////////////////////////////////////////////
// audioplay: PLAYSECTOR is NOT BCD coded !!!

EXPORT_GCC long CALLBACK CDRplay(unsigned char * sector)
{
 if(!bIsOpen)   CDVDopen();
 if(!iCDROK)    return PSE_ERR_FATAL;

 if(!DoCDDAPlay(time2addr(sector)))                    // start playing
  return PSE_CDR_ERR_NOREAD;

 bCDDAPlay=TRUE;                                       // raise flag: we are playing

 return PSE_CDR_ERR_SUCCESS;
}

/////////////////////////////////////////////////////////
// audiostop: stops cdda playing

EXPORT_GCC long CALLBACK CDRstop(void)
{
 if(!bCDDAPlay) return PSE_ERR_FATAL;

 DoCDDAPlay(0);                                        // stop cdda

 bCDDAPlay=FALSE;                                      // reset flag: no more playing

 return PSE_CDR_ERR_SUCCESS;
}

/////////////////////////////////////////////////////////
// getdriveletter

EXPORT_GCC char CALLBACK CDRgetDriveLetter(void)
{
 if(!iCDROK) return 0;                                 // not open? no way to get the letter

 if(iInterfaceMode==2 || iInterfaceMode==3)            // w2k/xp: easy
  {
   return MapIOCTLDriveLetter(iCD_AD,iCD_TA,iCD_LU);
  }
 else                                                  // but with aspi???
  {                                                    // -> no idea yet (maybe registry read...pfff)
  }

 return 0;
}

/////////////////////////////////////////////////////////
// getstatus: pcsx func... poorly supported here
//            problem is: func will be called often, which
//            would block all of my cdr reading if I would use
//            lotsa scsi commands

struct CdrStat 
{
 unsigned long Type;
 unsigned long Status;
 unsigned char Time[3]; // current playing time
};

struct CdrStat ostat;

// reads cdr status
// type:
// 0x00 - unknown
// 0x01 - data
// 0x02 - audio
// 0xff - no cdrom
// status:
// 0x00 - unknown
// 0x02 - error
// 0x08 - seek error
// 0x10 - shell open
// 0x20 - reading
// 0x40 - seeking
// 0x80 - playing
// time:
// byte 0 - minute
// byte 1 - second
// byte 2 - frame


EXPORT_GCC long CALLBACK CDRgetStatus(struct CdrStat *stat) 
{
 int iStatus;
 static time_t to;

 if(!bCDDAPlay)  // if not playing update stat only once in a second
  { 
   if(to<time(NULL)) 
    {
     to = time(NULL);
    } 
   else 
    {
     memcpy(stat, &ostat, sizeof(struct CdrStat));
     return 0;
    }
  }

 memset(stat, 0, sizeof(struct CdrStat));

 if(!iCDROK) return -1;                                // not opened? bye

 if(bCDDAPlay)                                         // cdda is playing?
  {
   unsigned char * pB=GetCDDAPlayPosition();           // -> get pos
   stat->Type = 0x02;                                  // -> audio
   if(pB)
    {
     stat->Status|=0x80;                               // --> playing flag
     stat->Time[0]=pB[18];                             // --> and curr play time
     stat->Time[1]=pB[19];
     stat->Time[2]=pB[20];
    }
  }
 else                                                  // cdda not playing?
  { 
   stat->Type = 0x01;                                  // -> data
  }

 LockGenCDAccess();                                    // make sure that no more reading is happening
 iStatus=GetSCSIStatus(iCD_AD,iCD_TA,iCD_LU);          // get device status
 UnlockGenCDAccess();

 if(iStatus==SS_ERR) 
  {                                                    // no cdrom?
   stat->Type = 0xff;
   stat->Status|= 0x10;
  }

 memcpy(&ostat, stat, sizeof(struct CdrStat));

 return 0;
}

/////////////////////////////////////////////////////////
*/
