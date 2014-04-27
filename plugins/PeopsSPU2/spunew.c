/***************************************************************************
                            spu.c  -  description
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
// 2005/08/29 - Pete
// - changed to 48Khz output
//
// 2004/12/25 - Pete
// - inc'd version for pcsx2-0.7
//
// 2004/04/18 - Pete
// - changed all kind of things in the plugin
//
// 2004/04/04 - Pete
// - changed plugin to emulate PS2 spu
//
// 2003/04/07 - Eric
// - adjusted cubic interpolation algorithm
//
// 2003/03/16 - Eric
// - added cubic interpolation
//
// 2003/03/01 - linuzappz
// - libraryName changes using ALSA
//
// 2003/02/28 - Pete
// - added option for type of interpolation
// - adjusted spu irqs again (Thousant Arms, Valkyrie Profile)
// - added MONO support for MSWindows DirectSound
//
// 2003/02/20 - kode54
// - amended interpolation code, goto GOON could skip initialization of gpos and cause segfault
//
// 2003/02/19 - kode54
// - moved SPU IRQ handler and changed sample flag processing
//
// 2003/02/18 - kode54
// - moved ADSR calculation outside of the sample decode loop, somehow I doubt that
//   ADSR timing is relative to the frequency at which a sample is played... I guess
//   this remains to be seen, and I don't know whether ADSR is applied to noise channels...
//
// 2003/02/09 - kode54
// - one-shot samples now process the end block before stopping
// - in light of removing fmod hack, now processing ADSR on frequency channel as well
//
// 2003/02/08 - kode54
// - replaced easy interpolation with gaussian
// - removed fmod averaging hack
// - changed .sinc to be updated from .iRawPitch, no idea why it wasn't done this way already (<- Pete: because I sometimes fail to see the obvious, haharhar :)
//
// 2003/02/08 - linuzappz
// - small bugfix for one usleep that was 1 instead of 1000
// - added iDisStereo for no stereo (Linux)
//
// 2003/01/22 - Pete
// - added easy interpolation & small noise adjustments
//
// 2003/01/19 - Pete
// - added Neill's reverb
//
// 2003/01/12 - Pete
// - added recording window handlers
//
// 2003/01/06 - Pete
// - added Neill's ADSR timings
//
// 2002/12/28 - Pete
// - adjusted spu irq handling, fmod handling and loop handling
//
// 2002/08/14 - Pete
// - added extra reverb
//
// 2002/06/08 - linuzappz
// - SPUupdate changed for SPUasync
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#define _IN_SPU

#include "externals.h"
#include "cfg.h"
#include "dsoundoss.h"
#include "regs.h"
#include "debug.h"
#include "record.h"
#include "resource.h"
#include "dma.h"
#include "registers.h"
////////////////////////////////////////////////////////////////////////
// spu version infos/name
////////////////////////////////////////////////////////////////////////

const unsigned char version  = 5;
const unsigned char revision = 1;
const unsigned char build    = 6;
static char * libraryName     = "P.E.Op.S. SPU2 DSound Driver";
static char * libraryInfo     = "P.E.Op.S. SPU2 Driver V1.6\nCoded by Pete Bernert, Saqib and the P.E.Op.S. team\n";
////////////////////////////////////////////////////////////////////////
// globals
////////////////////////////////////////////////////////////////////////

// psx buffer / addresses

unsigned short  regArea[32*1024];
 short  spuMem[2*1024*1024];
 char * spuMemC;
unsigned char * pSpuIrq[2];
unsigned char * pSpuBuffer;
unsigned char * pSpuStreamBuffer[2];

// user settings

int             iVolume=3;
int             iDebugMode=0;
int             iRecordMode=0;
int             iUseReverb=0;
int             iUseInterpolation=2;

// MAIN infos struct for each channel

SPUCHAN         s_chan[MAXCHAN+1];                     // channel + 1 infos (1 is security for fmod handling)
REVERBInfo      rvb[2];

unsigned long   dwNoiseVal=1;                          // global noise generator
int             iSPUIRQWait=1;
unsigned short  spuCtrl2[2];                           // some vars to store psx reg infos
unsigned short  spuStat2[2];
unsigned long   spuIrq2[2];
unsigned long   spuAddr2[2];                           // address into spu mem
unsigned long   spuRvbAddr2[2];
unsigned long   spuRvbAEnd2[2];
int             bEndThread=0;                          // thread handlers
int             bSpuInit=0;
int             bSPUIsOpen=0;
int             bThreadEnded=0;
int             iUseTimer=2;
int             aSyncMode=0;
unsigned long   aSyncCounter=0;
unsigned long   aSyncWait=0;
DWORD           aSyncTimerNew;
DWORD           aSyncTimerOld;
HWND    hWMain=0;                                      // window handle
HWND    hWDebug=0;
HWND    hWRecord=0;

static HANDLE   hMainThread;
unsigned long dwNewChannel2[2];                        // flags for faster testing, if new channel starts
unsigned long dwEndChannel2[2];

void (CALLBACK *irqCallbackDMA4)()=0;                  // func of main emu, called on spu irq
void (CALLBACK *irqCallbackDMA7)()=0;                  // func of main emu, called on spu irq
void (CALLBACK *irqCallbackSPU2)()=0;                  // func of main emu, called on spu irq

// certain globals (were local before, but with the new timeproc I need em global)

const int f[5][2] = {   {    0,  0  },
                        {   60,  0  },
                        {  115, -52 },
                        {   98, -55 },
                        {  122, -60 } };
int SSumR[NSSIZE];
int SSumL[NSSIZE];

extern ADMA Adma4;
extern ADMA Adma7;
DINPUT DirectInputC0, DirectInputC1;

extern unsigned short interrupt;
int SPUCycles;
extern int SPUStartCycle[2];
extern int SPUTargetCycle[2];

int iCycle=0;
short * pS;
short * pS1;

static int lastch=-1;      // last channel processed on spu irq in timer mode
static int lastns=0;       // last ns pos
static int iSecureStart=0; // secure start counter

////////////////////////////////////////////////////////////////////////
// CODE AREA
////////////////////////////////////////////////////////////////////////

// dirty inline func includes

#include "reverb.c"
#include "adsr.c"

////////////////////////////////////////////////////////////////////////
// helpers for simple interpolation

//
// easy interpolation on upsampling, no special filter, just "Pete's common sense" tm
//
// instead of having n equal sample values in a row like:
//       ____
//           |____
//
// we compare the current delta change with the next delta change.
//
// if curr_delta is positive,
//
//  - and next delta is smaller (or changing direction):
//         \.
//          -__
//
//  - and next delta significant (at least twice) bigger:
//         --_
//            \.
//
//  - and next delta is nearly same:
//          \.
//           \.
//
//
// if curr_delta is negative,
//
//  - and next delta is smaller (or changing direction):
//          _--
//         /
//
//  - and next delta significant (at least twice) bigger:
//            /
//         __-
//
//  - and next delta is nearly same:
//           /
//          /
//


INLINE void InterpolateUp(int ch)
{
 if(s_chan[ch].SB[32]==1)                              // flag == 1? calc step and set flag... and don't change the value in this pass
  {
   const int id1=s_chan[ch].SB[30]-s_chan[ch].SB[29];  // curr delta to next val
   const int id2=s_chan[ch].SB[31]-s_chan[ch].SB[30];  // and next delta to next-next val :)

   s_chan[ch].SB[32]=0;

   if(id1>0)                                           // curr delta positive
    {
     if(id2<id1)
      {s_chan[ch].SB[28]=id1;s_chan[ch].SB[32]=2;}
     else
     if(id2<(id1<<1))
      s_chan[ch].SB[28]=(id1*s_chan[ch].sinc)/0x10000L;
     else
      s_chan[ch].SB[28]=(id1*s_chan[ch].sinc)/0x20000L;
    }
   else                                                // curr delta negative
    {
     if(id2>id1)
      {s_chan[ch].SB[28]=id1;s_chan[ch].SB[32]=2;}
     else
     if(id2>(id1<<1))
      s_chan[ch].SB[28]=(id1*s_chan[ch].sinc)/0x10000L;
     else
      s_chan[ch].SB[28]=(id1*s_chan[ch].sinc)/0x20000L;
    }
  }
 else
 if(s_chan[ch].SB[32]==2)                              // flag 1: calc step and set flag... and don't change the value in this pass
  {
   s_chan[ch].SB[32]=0;

   s_chan[ch].SB[28]=(s_chan[ch].SB[28]*s_chan[ch].sinc)/0x20000L;
   if(s_chan[ch].sinc<=0x8000)
        s_chan[ch].SB[29]=s_chan[ch].SB[30]-(s_chan[ch].SB[28]*((0x10000/s_chan[ch].sinc)-1));
   else s_chan[ch].SB[29]+=s_chan[ch].SB[28];
  }
 else                                                  // no flags? add bigger val (if possible), calc smaller step, set flag1
  s_chan[ch].SB[29]+=s_chan[ch].SB[28];
}

//
// even easier interpolation on downsampling, also no special filter, again just "Pete's common sense" tm
//

INLINE void InterpolateDown(int ch)
{
 if(s_chan[ch].sinc>=0x20000)                                 // we would skip at least one val?
  {
   s_chan[ch].SB[29]+=(s_chan[ch].SB[30]-s_chan[ch].SB[29])/2; // add easy weight
   if(s_chan[ch].sinc>=0x30000)                               // we would skip even more vals?
		s_chan[ch].SB[29]+=(s_chan[ch].SB[31]-s_chan[ch].SB[30])/2;// add additional next weight
  }
}

////////////////////////////////////////////////////////////////////////
// helpers for gauss interpolation

#define gval0 (((short*)(&s_chan[ch].SB[29]))[gpos])
#define gval(x) (((short*)(&s_chan[ch].SB[29]))[(gpos+x)&3])

#include "gauss_i.h"

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// START SOUND... called by main thread to setup a new sound on a channel
////////////////////////////////////////////////////////////////////////

INLINE void StartSound(int ch)
{
 dwNewChannel2[ch/24]&=~(1<<(ch%24));                  // clear new channel bit
 dwEndChannel2[ch/24]&=~(1<<(ch%24));                  // clear end channel bit

 StartADSR(ch);
 StartREVERB(ch);

 s_chan[ch].pCurr=s_chan[ch].pStart;                   // set sample start

 s_chan[ch].s_1=0;                                     // init mixing vars
 s_chan[ch].s_2=0;
 s_chan[ch].iSBPos=28;

 s_chan[ch].bNew=0;                                    // init channel flags
 s_chan[ch].bStop=0;
 s_chan[ch].bOn=1;

 s_chan[ch].SB[29]=0;                                  // init our interpolation helpers
 s_chan[ch].SB[30]=0;

 if(iUseInterpolation>=2)                              // gauss interpolation?
      {s_chan[ch].spos=0x30000L;s_chan[ch].SB[28]=0;}  // -> start with more decoding
 else {s_chan[ch].spos=0x10000L;s_chan[ch].SB[31]=0;}  // -> no/simple interpolation starts with one 44100 decoding
}


void UpdateMainVolL()            // LEFT VOLUME
{
 short vol = regArea[PS2_C0_MVOLL];
 if(vol&0x8000)                                        // sweep?
  {
   short sInc=1;                                       // -> sweep up?
   if(vol&0x2000) sInc=-1;                             // -> or down?
   if(vol&0x1000) vol^=0xffff;                         // -> mmm... phase inverted? have to investigate this
   vol=((vol&0x7f)+1)/2;                               // -> sweep: 0..127 -> 0..64
   vol+=vol/(2*sInc);                                  // -> HACK: we don't sweep right now, so we just raise/lower the volume by the half!
   vol*=128;
  }
 else                                                  // no sweep:
  {
   if(vol&0x4000)                                      // -> mmm... phase inverted? have to investigate this
    //vol^=0xffff;
    vol=0x3fff-(vol&0x3fff);
  }

 vol&=0x3fff;
 regArea[PS2_C0_MVOLL]=vol;                           // store volume
}

////////////////////////////////////////////////////////////////////////
// RIGHT VOLUME register write
////////////////////////////////////////////////////////////////////////

void UpdateMainVolR()            // RIGHT VOLUME
{
 short vol = regArea[PS2_C0_MVOLR];
 if(vol&0x8000)                                        // comments... see above :)
  {
   short sInc=1;
   if(vol&0x2000) sInc=-1;
   if(vol&0x1000) vol^=0xffff;
   vol=((vol&0x7f)+1)/2;
   vol+=vol/(2*sInc);
   vol*=128;
  }
 else
  {
   if(vol&0x4000) //vol=vol^=0xffff;
    vol=0x3fff-(vol&0x3fff);
  }

 vol&=0x3fff;

 regArea[PS2_C0_MVOLR]=vol;
}
////////////////////////////////////////////////////////////////////////
// MAIN SPU FUNCTION
// here is the main job handler... thread, timer or direct func call
// basically the whole sound processing is done in this fat func!
////////////////////////////////////////////////////////////////////////

// 5 ms waiting phase, if buffer is full and no new sound has to get started
// .. can be made smaller (smallest val: 1 ms), but bigger waits give
// better performance

#define PAUSE_W 5
#define PAUSE_L 5000
extern unsigned long MemAddr[2];
////////////////////////////////////////////////////////////////////////

int iSpuAsyncWait=0;
extern int MMIXC0, MMIXC1;

VOID CALLBACK MAINProc(UINT nTimerId,UINT msg,DWORD dwUser,DWORD dwParam1, DWORD dwParam2)
{
  int s_1,s_2,fa,ns;
  int core = 0;
  unsigned char * start;
  unsigned int nSample;
  int d;
  int ch,predict_nr,shift_factor,flags,s;
  int gpos,bIRQReturn=0;

 while(!bEndThread)                                    // until we are shutting down
 {
   //--------------------------------------------------//
   // ok, at the beginning we are looking if there is
   // enuff free place in the dsound/oss buffer to
   // fill in new data, or if there is a new channel to start.
   // if not, we wait (thread) or return (timer/spuasync)
   // until enuff free place is available/a new channel gets
   // started
   if (aSyncMode==1)								   // Async supported? and enabled?
   {
		if (aSyncCounter<=737280)                         // If we have 10ms in the buffer, don't wait
		{
		if (aSyncWait<1000) Sleep(aSyncWait);          // Wait a little to be more Synced (No more than 1 sec)
		else Sleep (1000);
		}

		while (aSyncCounter<=368640 && !bEndThread && aSyncMode==1)
				Sleep (1);              // bEndThread/aSyncMode are needed, to avoid close problems

		aSyncCounter -= 36864;							   // 1ms more done (48Hz*768cycles/Hz)
   }
   else
   if(dwNewChannel2[0] || dwNewChannel2[1])            // new channel should start immedately?
   {                                                  // (at least one bit 0 ... MAXCHANNEL is set?)
		iSecureStart++;                                   // -> set iSecure
		if(iSecureStart>5) iSecureStart=0;                //    (if it is set 5 times - that means on 5 tries a new samples has been started - in a row, we will reset it, to give the sound update a chance)
   }
   else iSecureStart=0;                                // 0: no new channel should start

   while(!iSecureStart && !bEndThread &&               // no new start? no thread end?
		(SoundGetBytesBuffered()>TESTSIZE))           // and still enuff data in sound buffer?
   {
		iSecureStart=0;                                   // reset secure

		if(iUseTimer)                                     // no-thread mode?
		{
			return;                                         // -> and done this time (timer mode 1 or 2)
		}
                                                       // win thread mode:
		Sleep(PAUSE_W);                                   // sleep for x ms (win)

		if(dwNewChannel2[0] || dwNewChannel2[1])
			iSecureStart=1;                                  // if a new channel kicks in (or, of course, sound buffer runs low), we will leave the loop
   }
   //--------------------------------------------------// continue from irq handling in timer mode?

   if(lastch>=0)                                       // will be -1 if no continue is pending
   {
     ch=lastch; ns=lastns; lastch=-1;                  // -> setup all kind of vars to continue
	 if( s_chan[ch].iSBPos < 28 ) {
		goto GOON;                                        // -> directly jump to the continue point
	 }
   }

   //--------------------------------------------------//
   //- main channel loop                              -//
   //--------------------------------------------------//
   {
   for(ch=0;ch<MAXCHAN;ch++)                         // loop em all... we will collect 1 ms of sound of each playing channel
   {
       if(s_chan[ch].bNew)		StartSound(ch);             // start new sound
       if(!s_chan[ch].bOn)		continue;                   // channel not playing? next
	   core = ch / 24;										// Choose which core

       if(s_chan[ch].iActFreq!=s_chan[ch].iUsedFreq)
       {
         s_chan[ch].iUsedFreq	=	s_chan[ch].iActFreq;     // -> take it and calc steps
         s_chan[ch].sinc		=	s_chan[ch].iRawPitch<<4;
         if(!s_chan[ch].sinc)		s_chan[ch].sinc=1;
         if(iUseInterpolation==1)	s_chan[ch].SB[32]=1; // -> freq change in simle imterpolation mode: set flag
	   }

       ns=0;
       while(ns<NSSIZE)                                // loop until 1 ms of data is reached
       {
         while(s_chan[ch].spos>=0x10000L)
         {
           if(s_chan[ch].iSBPos==28)                   // 28 reached?
           {
             start=s_chan[ch].pCurr;                   // set up the current pos

             if (s_chan[ch].bOn==0) goto ENDX;         // special "stop" sign

             s_chan[ch].iSBPos=0;

             s_1=s_chan[ch].s_1;
             s_2=s_chan[ch].s_2;

             predict_nr		=	(int)*start;
			 start++;
             flags			=	(int)*start;
			 start++;

			 shift_factor	=	predict_nr&0xf;
             predict_nr	  >>=	4;
             // -------------------------------------- //

             for (nSample=0;nSample<28;nSample+=2,start++)
             {
               d=(int)*start;
               s=((d&0xf)<<12);
               if(s&0x8000) s|=0xffff0000;

               fa=(s >> shift_factor);
               fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
               s_2=s_1;
			   s_1=fa;
               s=((d & 0xf0) << 8);

               s_chan[ch].SB[nSample]=fa;

               if(s&0x8000) s|=0xffff0000;
               fa=(s>>shift_factor);
               fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
               s_2=s_1;
			   s_1=fa;

               s_chan[ch].SB[nSample+1]=fa;
             }

             //////////////////////////////////////////// irq check

             if(spuCtrl2[core]&0x40)                  // some irq active?
             {
				if(iDebugMode==1) logprintf("IRQ Active ch %x, C%x\r\n", ch, ch/24);

				if((pSpuIrq[core] >=  start-16 &&       // irq address reached?
					pSpuIrq[core] <= start) ||
                   ((flags&1) &&                        // special: irq on looping addr, when stop/loop flag is set
                   (pSpuIrq[core] >=  s_chan[ch].pLoop-16 &&
                    pSpuIrq[core] <= s_chan[ch].pLoop)))
                {
					s_chan[ch].iIrqDone=1;                // -> debug flag

					if(iDebugMode==1) logprintf("Sample End ch %x, C%x\r\n", ch, ch/24);

					regArea[0x7C0] |= 0x4<<core;
					irqCallbackSPU2();           // -> let's see what is happening if we call our irqs instead ;)

					if(iSPUIRQWait)                       // -> option: wait after irq for main emu
	                {
						iSpuAsyncWait=1;
						bIRQReturn=1;
					}
				}
			 }

             //////////////////////////////////////////// flag handler
			 if(flags & 0x2) s_chan[ch].bIgnoreLoop=1;				// LOOP bit

			 if(flags&0x4) s_chan[ch].pLoop=start-16;               // LOOP/START bit

	         if(flags&0x1)                               // 1: LOOP/END bit
             {
				 dwEndChannel2[core]|=(1<<(ch%24));

				 if((flags&0xF) != 0x3)  // Check if no loop is present
				 {
					s_chan[ch].bIgnoreLoop=0;
					s_chan[ch].bStop = 1;
					logprintf("Stopping\r\n");
				 }
				 else start = s_chan[ch].pLoop;
             }

             s_chan[ch].pCurr=start;                   // store values for next cycle
             s_chan[ch].s_1=s_1;
             s_chan[ch].s_2=s_2;


             ////////////////////////////////////////////

             if(bIRQReturn)                            // special return for "spu irq - wait for cpu action"
             {
                bIRQReturn=0;
               if(iUseTimer!=2)
                {
                 DWORD dwWatchTime=timeGetTime()+2500;

                 while(iSpuAsyncWait && !bEndThread &&
                       timeGetTime()<dwWatchTime)
                     Sleep(1);
                }
               else
                {
                 lastch=ch;
                 lastns=ns;

                 return;
				}
			 }

             ////////////////////////////////////////////

GOON: ;
		   }

           fa=s_chan[ch].SB[s_chan[ch].iSBPos++];      // get sample data

           if((spuCtrl2[core]&0x4000)==0) fa=0;       // muted?
           else                                        // else adjust
            {
             if(fa>32767L)  fa=32767L;
             if(fa<-32767L) fa=-32767L;
            }

           if(iUseInterpolation>=2)                    // gauss/cubic interpolation
            {
             gpos = s_chan[ch].SB[28];
             gval0 = fa;
             gpos = (gpos+1) & 3;
             s_chan[ch].SB[28] = gpos;
            }
           else
           if(iUseInterpolation==1)                    // simple interpolation
            {
             s_chan[ch].SB[28] = 0;
             s_chan[ch].SB[29] = s_chan[ch].SB[30];    // -> helpers for simple linear interpolation: delay real val for two slots, and calc the two deltas, for a 'look at the future behaviour'
             s_chan[ch].SB[30] = s_chan[ch].SB[31];
             s_chan[ch].SB[31] = fa;
             s_chan[ch].SB[32] = 1;                    // -> flag: calc new interolation
            }
           else s_chan[ch].SB[29]=fa;                  // no interpolation

           s_chan[ch].spos -= 0x10000L;
          }

         ////////////////////////////////////////////////
         // noise handler... just produces some noise data
         // surely wrong... and no noise frequency (spuCtrl&0x3f00) will be used...
         // and sometimes the noise will be used as fmod modulation... pfff

         if(s_chan[ch].bNoise)
          {
           if((dwNoiseVal<<=1)&0x80000000L)
            {
             dwNoiseVal^=0x0040001L;
             fa=((dwNoiseVal>>2)&0x7fff);
             fa=-fa;
            }
           else fa=(dwNoiseVal>>2)&0x7fff;

           // mmm... depending on the noise freq we allow bigger/smaller changes to the previous val
           fa=s_chan[ch].iOldNoise+((fa-s_chan[ch].iOldNoise)/((0x001f-((spuCtrl2[core]&0x3f00)>>9))+1));
           if(fa>32767L)  fa=32767L;
           if(fa<-32767L) fa=-32767L;
           s_chan[ch].iOldNoise=fa;

           if(iUseInterpolation<2)                     // no gauss/cubic interpolation?
            s_chan[ch].SB[29] = fa;                    // -> store noise val in "current sample" slot
          }                                            //----------------------------------------
         else                                          // NO NOISE (NORMAL SAMPLE DATA) HERE
          {//------------------------------------------//
           if(iUseInterpolation==3)                    // cubic interpolation
            {
             long xd;
             xd = ((s_chan[ch].spos) >> 1)+1;
             gpos = s_chan[ch].SB[28];

             fa  = gval(3) - 3*gval(2) + 3*gval(1) - gval0;
             fa *= (xd - (2<<15)) / 6;
             fa >>= 15;
             fa += gval(2) - gval(1) - gval(1) + gval0;
             fa *= (xd - (1<<15)) >> 1;
             fa >>= 15;
             fa += gval(1) - gval0;
             fa *= xd;
             fa >>= 15;
             fa = fa + gval0;
            }
           //------------------------------------------//
           else
           if(iUseInterpolation==2)                    // gauss interpolation
            {
             int vl, vr;
             vl = (s_chan[ch].spos >> 6) & ~3;
             gpos = s_chan[ch].SB[28];
             vr=(gauss[vl]*gval0)&~2047;
             vr+=(gauss[vl+1]*gval(1))&~2047;
             vr+=(gauss[vl+2]*gval(2))&~2047;
             vr+=(gauss[vl+3]*gval(3))&~2047;
             fa = vr>>11;
            }
           //------------------------------------------//
           else
           if(iUseInterpolation==1)                    // simple interpolation
            {
             if(s_chan[ch].sinc<0x10000L)              // -> upsampling?
                  InterpolateUp(ch);                   // --> interpolate up
             else InterpolateDown(ch);                 // --> else down
             fa=s_chan[ch].SB[29];
            }
           //------------------------------------------//
           else fa=s_chan[ch].SB[29];                  // no interpolation
          }

         s_chan[ch].sval = (MixADSR(ch) * fa) / 1023;  // add adsr

         if(s_chan[ch].bFMod==2)                       // fmod freq channel
          {
           int NP=((32768L+s_chan[ch].sval)*s_chan[ch+1].iRawPitch)>>14;

           if(NP>0x3fff) NP=0x3fff;
           else if(NP<0x1)    NP=0x1;

           NP=(48000L*NP)>>12;                     // calc frequency ( 48hz )

           s_chan[ch+1].iActFreq=NP;
           s_chan[ch+1].iUsedFreq=NP;
           s_chan[ch+1].sinc=(((NP/10)<<16)/48000);	// check , was 4800
           if(!s_chan[ch+1].sinc) s_chan[ch+1].sinc=1;
           if(iUseInterpolation==1)                    // freq change in sipmle interpolation mode
            s_chan[ch+1].SB[32]=1;

		 }
         else
         {
           //////////////////////////////////////////////
           // ok, left/right sound volume (ps2 volume goes from 0 ... 0x3fff)

           if(s_chan[ch].iMute) s_chan[ch].sval=0;                         // debug mute
           else
           {
             if(s_chan[ch].bVolumeL)
              SSumL[ns]+=(s_chan[ch].sval*s_chan[ch].iLeftVolume)>>14;

			 if(s_chan[ch].bVolumeR)
              SSumR[ns]+=(s_chan[ch].sval*s_chan[ch].iRightVolume)>>14;
           }

           //////////////////////////////////////////////
           // now let us store sound data for reverb

           if(s_chan[ch].bRVBActive) StoreREVERB(ch,ns);
          }

         ////////////////////////////////////////////////
         // ok, go on until 1 ms data of this channel is collected

         ns++;
         s_chan[ch].spos += s_chan[ch].sinc;
		}
ENDX:   ;
      }
    }

  //---------------------------------------------------//
  //- here we have another 1 ms of sound data
  //---------------------------------------------------//
  ///////////////////////////////////////////////////////
  // mix all channels (including reverb) into one buffer
  for(ns=0;ns<NSSIZE;ns++)
  {
    DirectInputC0.Left = 0;
	DirectInputC0.Right= 0;
	DirectInputC1.Left = 0;
	DirectInputC1.Right= 0;

	if((regArea[PS2_C0_MMIX] & 0xC0) && regArea[PS2_C0_ADMAS] & 0x1 && !(spuCtrl2[0] & 0x30))
	{
		DirectInputC0.Left = ((short*)spuMem)[0x2000+Adma4.Index];
		DirectInputC0.Right = ((short*)spuMem)[0x2200+Adma4.Index];
		Adma4.Index +=1;

		if(Adma4.Index == 128 || Adma4.Index == 384)
		{
			if(ADMAS4Write())
			{
				spuStat2[0]&=~0x80;
				irqCallbackDMA4();
			}
			else MemAddr[0] += 1024;
		}

		if(Adma4.Index == 512){
			Adma4.Index = 0;
		}
	}

	if((regArea[PS2_C1_MMIX] & 0xC0) && regArea[PS2_C1_ADMAS] & 0x2 && !(spuCtrl2[1] & 0x30))
	{
		DirectInputC1.Left = ((short*)spuMem)[0x2400+Adma7.Index];
		DirectInputC1.Right = ((short*)spuMem)[0x2600+Adma7.Index];
		Adma7.Index +=1;

		if(Adma7.Index == 128 || Adma7.Index == 384)
		{
			if(ADMAS7Write())
			{
				spuStat2[1]&=~0x80;
				irqCallbackDMA7();
			}else MemAddr[1] += 1024;
		}

		if(Adma7.Index == 512)	Adma7.Index = 0;
	}

  	SSumL[ns]+=MixREVERBLeft(ns,0);
    SSumL[ns]+=MixREVERBLeft(ns,1);

	if((regArea[PS2_C0_MMIX] & 0x80)) SSumL[ns] += (DirectInputC0.Left*(int)regArea[PS2_C0_BVOLL])>>16;
	if((regArea[PS2_C1_MMIX] & 0x80)) SSumL[ns] += (DirectInputC1.Left*(int)regArea[PS2_C1_BVOLL])>>16;

	UpdateMainVolL();

	d=SSumL[ns]/iVolume;
	SSumL[ns]=0;
	*pS++=(d<-32767) ? -32767	: ((d>32767) ? 32767 : d);

    SSumR[ns]+=MixREVERBRight(0);
    SSumR[ns]+=MixREVERBRight(1);

	if((regArea[PS2_C0_MMIX] & 0x40)) SSumR[ns] += (DirectInputC0.Right*(int)regArea[PS2_C0_BVOLR])>>16;
	if((regArea[PS2_C1_MMIX] & 0x40)) SSumR[ns] += (DirectInputC1.Right*(int)regArea[PS2_C1_BVOLR])>>16;

	UpdateMainVolR();
    d=SSumR[ns]/iVolume;
	SSumR[ns]=0;
	*pS++=(d<-32767) ? -32767 : ((d>32767) ? 32767 : d);
  }
  InitREVERB();

  //////////////////////////////////////////////////////
  // feed the sound
  // wanna have around 1/60 sec (16.666 ms) updates

  if(iCycle++>16)
   {
	SoundFeedVoiceData((unsigned char*)pSpuBuffer,
                        ((unsigned char *)pS)-
                        ((unsigned char *)pSpuBuffer));
    pS=(short *)pSpuBuffer;
    iCycle=0;
   }
}
 // end of big main loop...
 bThreadEnded=1;
 return;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
DWORD WINAPI MAINThreadEx(LPVOID lpParameter)
{
 MAINProc(0,0,0,0,0);
 return 0;
}

////////////////////////////////////////////////////////////////////////
// SPU ASYNC... even newer epsxe func
//  1 time every 'cycle' cycles... harhar
////////////////////////////////////////////////////////////////////////
EXPORT_GCC void CALLBACK SPU2async(unsigned long cycle)
{
	SPUCycles += cycle;
	if(interrupt & (1<<2)){
		if(SPUCycles - SPUStartCycle[1] >= SPUTargetCycle[1]){
			interrupt &= ~(1<<2);
			irqCallbackDMA7();
		}

	}

	if(interrupt & (1<<1)){
		if(SPUCycles - SPUStartCycle[0] >= SPUTargetCycle[0]){
			interrupt &= ~(1<<1);
			irqCallbackDMA4();
		}
	}

 if(iSpuAsyncWait)
  {
   iSpuAsyncWait++;
   if(iSpuAsyncWait<=64) return;
   iSpuAsyncWait=0;
  }

 if(iDebugMode==2)
  {
   if(IsWindow(hWDebug)) DestroyWindow(hWDebug);
   hWDebug=0;iDebugMode=0;
  }
 if(iRecordMode==2)
  {
   if(IsWindow(hWRecord)) DestroyWindow(hWRecord);
   hWRecord=0;iRecordMode=0;
  }

   if(iUseTimer==0)                         // does the emu support SPUAsync, is it in thread mode, and in Thread Sync ON?
   {
    aSyncMode=1;                                       // Ten, activate main function Sync system flag

    aSyncTimerOld = aSyncTimerNew;                     // Recalculate, AsyncWait (ms)
    aSyncTimerNew = timeGetTime();

    aSyncWait =(unsigned int)((aSyncTimerNew - aSyncTimerOld)/2);

    aSyncCounter += cycle ;

    return;
   }
 if(iUseTimer==2)                                      // special mode, only used in Linux by this spu (or if you enable the experimental Windows mode)
  {
	  aSyncMode = 0;
   if(!bSpuInit) return;                               // -> no init, no call
  MAINProc(0,0,0,0,0);                                // -> experimental win mode... not really tested... don't like the drawbacks
  }
}


////////////////////////////////////////////////////////////////////////
// INIT/EXIT STUFF
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// SPUINIT: this func will be called first by the main emu
////////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS
static HINSTANCE hIRE = NULL;
#endif

EXPORT_GCC long CALLBACK SPU2init(void)
{
 spuMemC=(unsigned char *)spuMem;                      // just small setup
 memset((void *)s_chan,0,MAXCHAN*sizeof(SPUCHAN));
 memset(rvb,0,2*sizeof(REVERBInfo));

 InitADSR();

 if(hIRE==NULL) hIRE=LoadLibrary("Riched32.dll ");     // needed for debug output

 return 0;
}

////////////////////////////////////////////////////////////////////////
// SETUPTIMER: init of certain buffers and threads/timers
////////////////////////////////////////////////////////////////////////

void SetupTimer(void)
{
 memset(SSumR,0,NSSIZE*sizeof(int));                   // init some mixing buffers
 memset(SSumL,0,NSSIZE*sizeof(int));
 pS=(short *)pSpuBuffer;                               // setup soundbuffer pointer
 pS1=(short *)pSpuStreamBuffer[0];                               // setup soundbuffer pointer

 bEndThread=0;                                         // init thread vars
 bSpuInit=1;                                           // flag: we are inited

 bSpuInit=1;                                           // flag: we are inited

#ifdef _WINDOWS

 if(iUseTimer==0)                                      // windows: use thread
  {
   //_beginthread(MAINThread,0,NULL);
   DWORD dw;
   hMainThread=CreateThread(NULL,0,MAINThreadEx,0,0,&dw);
   SetThreadPriority(hMainThread,
                     //THREAD_PRIORITY_TIME_CRITICAL);
                     THREAD_PRIORITY_HIGHEST);
  }
#endif
}

////////////////////////////////////////////////////////////////////////
// REMOVETIMER: kill threads/timers
////////////////////////////////////////////////////////////////////////

void RemoveTimer(void)
{
 bEndThread=1;                                         // raise flag to end thread

 if(iUseTimer!=2)                                      // windows thread?
  {
   while(!bThreadEnded) {Sleep(5L);}                   // -> wait till thread has ended
   Sleep(5L);
  }

 bSpuInit=0;

 bThreadEnded=0;                                       // no more spu is running
}

////////////////////////////////////////////////////////////////////////
// SETUPSTREAMS: init most of the spu buffers
////////////////////////////////////////////////////////////////////////

void SetupStreams(void)
{
 int i;

 pSpuBuffer=(unsigned char *)malloc(38400);            // alloc mixing buffer
 i=NSSIZE*2;

 sRVBStart[0] = (int *)malloc(i*4);                    // alloc reverb buffer
 memset(sRVBStart[0],0,i*4);
 sRVBEnd[0]  = sRVBStart[0] + i;
 sRVBPlay[0] = sRVBStart[0];
 sRVBStart[1] = (int *)malloc(i*4);                    // alloc reverb buffer
 memset(sRVBStart[1],0,i*4);
 sRVBEnd[1]  = sRVBStart[1] + i;
 sRVBPlay[1] = sRVBStart[1];

  for(i=0;i<MAXCHAN;i++)                                // loop sound channels
  {
// we don't use mutex sync... not needed, would only
// slow us down:
//   s_chan[i].hMutex=CreateMutex(NULL,FALSE,NULL);
   s_chan[i].ADSRX.SustainLevel = 1024;                // -> init sustain
   s_chan[i].iMute=0;
   s_chan[i].iIrqDone=0;
   s_chan[i].pLoop=spuMemC+(s_chan[i].iStartAdr<<1);
   s_chan[i].pStart=spuMemC+(s_chan[i].iStartAdr<<1);
   s_chan[i].pCurr=spuMemC+(s_chan[i].iStartAdr<<1);
  }
}

////////////////////////////////////////////////////////////////////////
// REMOVESTREAMS: free most buffer
////////////////////////////////////////////////////////////////////////

void RemoveStreams(void)
{
 free(pSpuBuffer);                                     // free mixing buffer
 pSpuBuffer=NULL;
 free(sRVBStart[0]);                                   // free reverb buffer
 sRVBStart[0]=0;
 free(sRVBStart[1]);                                   // free reverb buffer
 sRVBStart[1]=0;
}


////////////////////////////////////////////////////////////////////////
// SPUOPEN: called by main emu after init
////////////////////////////////////////////////////////////////////////
#include <stdio.h>
FILE * LogFile;
EXPORT_GCC long CALLBACK SPU2open(void* pWindow)
{
#ifdef _WINDOWS
 HWND hW= pWindow == NULL ? NULL : *(HWND*)pWindow;
#endif

 if(bSPUIsOpen) return 0;                              // security for some stupid main emus
 LogFile = fopen("Logs/spu2.txt","wb");
 iVolume=3;
 bEndThread=0;
 bThreadEnded=0;
 spuMemC=(unsigned char *)spuMem;
 memset((void *)s_chan,0,(MAXCHAN+1)*sizeof(SPUCHAN));
 pSpuIrq[0]=spuMemC;
 pSpuIrq[1]=spuMemC;
 dwNewChannel2[0]=0;
 dwNewChannel2[1]=0;
 dwEndChannel2[0]=0;
 dwEndChannel2[1]=0;
 spuCtrl2[0]=0;
 spuCtrl2[1]=0;
 spuStat2[0]=0;
 spuStat2[1]=0;
 spuIrq2[0]=0;
 spuIrq2[1]=0;
 spuAddr2[0]=0x0;
 spuAddr2[1]=0x0;
 spuRvbAddr2[0]=0;
 spuRvbAddr2[1]=0;
 spuRvbAEnd2[0]=0;
 spuRvbAEnd2[1]=0;

 memset(&Adma4,0,sizeof(ADMA));
 memset(&Adma7,0,sizeof(ADMA));

 memset(&DirectInputC0,0,sizeof(DINPUT));
 memset(&DirectInputC1,0,sizeof(DINPUT));

 LastWrite=0x00000000;LastPlay=0;                      // init some play vars
 if(!IsWindow(hW)) hW=GetActiveWindow();
 hWMain = hW;                                          // store hwnd

 ReadConfig();                                         // read user stuff

 SetupSound();                                         // setup midas (before init!)

 SetupStreams();                                       // prepare streaming

 SetupTimer();                                         // timer for feeding data

 bSPUIsOpen=1;

 if(iDebugMode)                                        // windows debug dialog
  {
   hWDebug=CreateDialog(hInst,MAKEINTRESOURCE(IDD_DEBUG),
                        NULL,(DLGPROC)DebugDlgProc);
   SetWindowPos(hWDebug,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
   UpdateWindow(hWDebug);
   SetFocus(hWMain);
  }

 if(iRecordMode)                                        // windows recording dialog
  {
   hWRecord=CreateDialog(hInst,MAKEINTRESOURCE(IDD_RECORD),
                        NULL,(DLGPROC)RecordDlgProc);
   SetWindowPos(hWRecord,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
   UpdateWindow(hWRecord);
   SetFocus(hWMain);
  }

 return 0;
}

////////////////////////////////////////////////////////////////////////

// not used yet
#ifndef _WINDOWS
void SPU2setConfigFile(char * pCfg)
{
 pConfigFile=pCfg;
}
#endif

////////////////////////////////////////////////////////////////////////
// SPUCLOSE: called before shutdown
////////////////////////////////////////////////////////////////////////

EXPORT_GCC void CALLBACK SPU2close(void)
{
 if(!bSPUIsOpen) return;                               // some security

 bSPUIsOpen=0;                                         // no more open
 //fclose(LogFile);
#ifdef _WINDOWS
 if(IsWindow(hWDebug)) DestroyWindow(hWDebug);
 hWDebug=0;
 if(IsWindow(hWRecord)) DestroyWindow(hWRecord);
 hWRecord=0;
#endif

 RemoveTimer();                                        // no more feeding

 RemoveSound();                                        // no more sound handling

 RemoveStreams();                                      // no more streaming
}

////////////////////////////////////////////////////////////////////////
// SPUSHUTDOWN: called by main emu on final exit
////////////////////////////////////////////////////////////////////////

EXPORT_GCC void CALLBACK SPU2shutdown(void)
{
#ifdef _WINDOWS
 if(hIRE!=NULL) {FreeLibrary(hIRE);hIRE=NULL;}
#endif

 return;
}

////////////////////////////////////////////////////////////////////////
// SPUTEST: we don't test, we are always fine ;)
////////////////////////////////////////////////////////////////////////

EXPORT_GCC long CALLBACK SPU2test(void)
{
 return 0;
}

////////////////////////////////////////////////////////////////////////
// SPUCONFIGURE: call config dialog
////////////////////////////////////////////////////////////////////////

EXPORT_GCC void CALLBACK SPU2configure(void)
{
#ifdef _WINDOWS
 DialogBox(hInst,MAKEINTRESOURCE(IDD_CFGDLG),
           GetActiveWindow(),(DLGPROC)DSoundDlgProc);
#else
 StartCfgTool("CFG");
#endif
}

////////////////////////////////////////////////////////////////////////
// SPUABOUT: show about window
////////////////////////////////////////////////////////////////////////

EXPORT_GCC void CALLBACK SPU2about(void)
{
#ifdef _WINDOWS
 DialogBox(hInst,MAKEINTRESOURCE(IDD_ABOUT),
           GetActiveWindow(),(DLGPROC)AboutDlgProc);
#else
 StartCfgTool("ABOUT");
#endif
}

////////////////////////////////////////////////////////////////////////
// SETUP CALLBACKS
// this functions will be called once,
// passes a callback that should be called on SPU-IRQ/cdda volume change
////////////////////////////////////////////////////////////////////////

EXPORT_GCC void CALLBACK SPU2irqCallback(void (CALLBACK *SPU2callback)(int),
										 void (CALLBACK *DMA4callback)(int),
										 void (CALLBACK *DMA7callback)(int))
{
 irqCallbackSPU2 = SPU2callback;
 irqCallbackDMA4 = DMA4callback;
 irqCallbackDMA7 = DMA7callback;
}

////////////////////////////////////////////////////////////////////////
// COMMON PLUGIN INFO FUNCS
////////////////////////////////////////////////////////////////////////

EXPORT_GCC char * CALLBACK PS2EgetLibName(void)
{
 return libraryName;
}

#define PS2E_LT_SPU2 0x4

EXPORT_GCC unsigned long CALLBACK PS2EgetLibType(void)
{
 return PS2E_LT_SPU2;
}

EXPORT_GCC unsigned long CALLBACK PS2EgetLibVersion2(unsigned long type)
{
 unsigned char v=version;

 // key hack to fake a lower version:
 //if(GetAsyncKeyState(VK_SHIFT)&0x8000) v--;

 // compile hack to set lib version to PCSX2 0.6 standards
 //v=2;

 return v<<16|revision<<8|build;
}

////////////////////////////////////////////////////////////////////////

