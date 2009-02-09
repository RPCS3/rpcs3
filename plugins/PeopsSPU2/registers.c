/***************************************************************************
                         registers.c  -  description
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
// 2004/04/04 - Pete
// - changed plugin to emulate PS2 spu
//
// 2003/02/09 - kode54
// - removed &0x3fff from reverb volume registers, fixes a few games,
//   hopefully won't be breaking anything
//
// 2003/01/19 - Pete
// - added Neill's reverb
//
// 2003/01/06 - Pete
// - added Neill's ADSR timings
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#define _IN_REGISTERS

#include "externals.h"
#include "registers.h"
#include "regs.h"
#include "reverb.h"
#include "debug.h"

int MMIXC0 = 0, MMIXC1 = 0;
/*
// adsr time values (in ms) by James Higgs ... see the end of
// the adsr.c source for details

#define ATTACK_MS     514L
#define DECAYHALF_MS  292L
#define DECAY_MS      584L
#define SUSTAIN_MS    450L
#define RELEASE_MS    446L
*/

// we have a timebase of 1.020408f ms, not 1 ms... so adjust adsr defines
#define ATTACK_MS      494L
#define DECAYHALF_MS   286L
#define DECAY_MS       572L
#define SUSTAIN_MS     441L
#define RELEASE_MS     437L

extern void (CALLBACK *irqCallbackSPU2)();                  // func of main emu, called on spu irq


////////////////////////////////////////////////////////////////////////
// WRITE REGISTERS: called by main emu
////////////////////////////////////////////////////////////////////////

EXPORT_GCC void CALLBACK SPU2write(unsigned long reg, unsigned short val)
{
 long r=reg&0xffff;



 regArea[r] = val;

 if(spuCtrl2[0]&0x40 && spuIrq2[0] == r){
			//regArea[0x7C0] |= 0x4;
            regArea[PS2_IRQINFO] |= 4;
			irqCallbackSPU2();
	  }
 if(spuCtrl2[1]&0x40 && spuIrq2[1] == r){
			//regArea[0x7C0] |= 0x8;
            regArea[PS2_IRQINFO] |= 8;
			irqCallbackSPU2();
	  }
 if((r>=0x0000 && r<0x0180)||(r>=0x0400 && r<0x0580))  // some channel info?
  {
   int rx=0;
   int ch=0;
   if(r>=0x400) { 
	   rx= r - 0x400;
	   ch=(rx>>4)+24;
   } else {
	    rx= r;
	   ch=(rx>>4);
   }
   
   switch(rx&0x0f)
    {
     //------------------------------------------------// r volume
     case 0:                                           
       SetVolumeL((unsigned char)ch,val);
#ifdef _WINDOWS
		if(iDebugMode==1) logprintf("W_REG Volume Left - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, val);
#endif
       break;
     //------------------------------------------------// l volume
     case 2:                                           
       SetVolumeR((unsigned char)ch,val);
#ifdef _WINDOWS
		if(iDebugMode==1) logprintf("W_REG Volume Right - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, val);
#endif
       break;
     //------------------------------------------------// pitch
     case 4:                                           
       SetPitch(ch,val);
#ifdef _WINDOWS
		if(iDebugMode==1) logprintf("W_REG Pitch - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, val);
#endif
       break;
     //------------------------------------------------// level with pre-calcs
     case 6:
       {
        const unsigned long lval=val;unsigned long lx;
        //---------------------------------------------//
        s_chan[ch].ADSRX.AttackModeExp=(lval&0x8000)?1:0; 
        s_chan[ch].ADSRX.AttackRate=(lval>>8) & 0x007f;
        s_chan[ch].ADSRX.DecayRate=(lval>>4) & 0x000f;
        s_chan[ch].ADSRX.SustainLevel=lval & 0x000f;
        //---------------------------------------------//
        if(!iDebugMode) break;
        //---------------------------------------------// stuff below is only for debug mode

        s_chan[ch].ADSR.AttackModeExp=(lval&0x8000)?1:0;        //0x007f

        lx=(((lval>>8) & 0x007f)>>2);                  // attack time to run from 0 to 100% volume
        lx=min(31,lx);                                 // no overflow on shift!
        if(lx) 
         { 
          lx = (1<<lx);
          if(lx<2147483) lx=(lx*ATTACK_MS)/10000L;     // another overflow check
          else           lx=(lx/10000L)*ATTACK_MS;
          if(!lx) lx=1;
         }
        s_chan[ch].ADSR.AttackTime=lx;                

        s_chan[ch].ADSR.SustainLevel=                 // our adsr vol runs from 0 to 1024, so scale the sustain level
         (1024*((lval) & 0x000f))/15;

        lx=(lval>>4) & 0x000f;                         // decay:
        if(lx)                                         // our const decay value is time it takes from 100% to 0% of volume
         {
          lx = ((1<<(lx))*DECAY_MS)/10000L;
          if(!lx) lx=1;
         }
        s_chan[ch].ADSR.DecayTime =                   // so calc how long does it take to run from 100% to the wanted sus level
         (lx*(1024-s_chan[ch].ADSR.SustainLevel))/1024;
       }
#ifdef _WINDOWS
	   if(iDebugMode==1) logprintf("W_REG ADSR1 - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, val);
#endif
      break;
     //------------------------------------------------// adsr times with pre-calcs
     case 8:
      {
       const unsigned long lval=val;unsigned long lx;

       //----------------------------------------------//
       s_chan[ch].ADSRX.SustainModeExp = (lval&0x8000)?1:0;
       s_chan[ch].ADSRX.SustainIncrease= (lval&0x4000)?0:1;
       s_chan[ch].ADSRX.SustainRate = (lval>>6) & 0x007f;
       s_chan[ch].ADSRX.ReleaseModeExp = (lval&0x0020)?1:0;
       s_chan[ch].ADSRX.ReleaseRate = lval & 0x001f;
       //----------------------------------------------//
       if(!iDebugMode) break;
       //----------------------------------------------// stuff below is only for debug mode

       s_chan[ch].ADSR.SustainModeExp = (lval&0x8000)?1:0;
       s_chan[ch].ADSR.ReleaseModeExp = (lval&0x0020)?1:0;
                   
       lx=((((lval>>6) & 0x007f)>>2));                 // sustain time... often very high
       lx=min(31,lx);                                  // values are used to hold the volume
       if(lx)                                          // until a sound stop occurs
        {                                              // the highest value we reach (due to 
         lx = (1<<lx);                                 // overflow checking) is: 
         if(lx<2147483) lx=(lx*SUSTAIN_MS)/10000L;     // 94704 seconds = 1578 minutes = 26 hours... 
         else           lx=(lx/10000L)*SUSTAIN_MS;     // should be enuff... if the stop doesn't 
         if(!lx) lx=1;                                 // come in this time span, I don't care :)
        }
       s_chan[ch].ADSR.SustainTime = lx;

       lx=(lval & 0x001f);
       s_chan[ch].ADSR.ReleaseVal     =lx;
       if(lx)                                          // release time from 100% to 0%
        {                                              // note: the release time will be
         lx = (1<<lx);                                 // adjusted when a stop is coming,
         if(lx<2147483) lx=(lx*RELEASE_MS)/10000L;     // so at this time the adsr vol will 
         else           lx=(lx/10000L)*RELEASE_MS;     // run from (current volume) to 0%
         if(!lx) lx=1;
        }
       s_chan[ch].ADSR.ReleaseTime=lx;

       if(lval & 0x4000)                               // add/dec flag
            s_chan[ch].ADSR.SustainModeDec=-1;
       else s_chan[ch].ADSR.SustainModeDec=1;
      }
#ifdef _WINDOWS
		if(iDebugMode==1) logprintf("W_REG ADSR2 - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, val);
#endif
     break;
	 default:
#ifdef _WINDOWS
		if(iDebugMode==1) logprintf("W_REG Unknown Mixing reg - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, val);
#endif
        break;
     //------------------------------------------------//
    }

   return;
  }

 if((r>=0x01c0 && r<0x02E0)||(r>=0x05c0 && r<0x06E0))  // some channel info?
  {
   int ch=0;unsigned long rx=r;

    ch=((rx & 0x3ff)-0x1c0) / 0xC;
   
   rx-=ch*0xC;
   if(rx>=0x400) {ch+=24;rx-=0x400;}

   switch(rx)
    {
     //------------------------------------------------//
     case 0x1C0:
      s_chan[ch].iStartAdr=(((unsigned long)val&0x3f)<<16)|(s_chan[ch].iStartAdr&0xFFFF);
      s_chan[ch].pStart=spuMemC+(s_chan[ch].iStartAdr<<1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG Start Addr Hi - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, val);
#endif
      break;
     case 0x1C2:
      s_chan[ch].iStartAdr=(s_chan[ch].iStartAdr & 0x3f0000) | (val & 0xFFFF);
      s_chan[ch].pStart=spuMemC+(s_chan[ch].iStartAdr<<1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG Start Addr Lo - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, val);
#endif
      break;
     //------------------------------------------------//
     case 0x1C4:
      s_chan[ch].iLoopAdr=(((unsigned long)val&0x3f)<<16)|(s_chan[ch].iLoopAdr&0xFFFF);
      s_chan[ch].pLoop=spuMemC+(s_chan[ch].iLoopAdr<<1);
       if(s_chan[ch].iLoopAdr)s_chan[ch].bIgnoreLoop=1;
	  else s_chan[ch].bIgnoreLoop=0;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG Loop Addr Hi - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, val);
#endif
      break;
     case 0x1C6:
      s_chan[ch].iLoopAdr=(s_chan[ch].iLoopAdr & 0x3f0000) | (val & 0xFFFF);
      s_chan[ch].pLoop=spuMemC+(s_chan[ch].iLoopAdr<<1);
      if(s_chan[ch].iLoopAdr)s_chan[ch].bIgnoreLoop=1;
	  else s_chan[ch].bIgnoreLoop=0;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG Loop Addr Lo - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, val);
#endif
      break;
     //------------------------------------------------//
     case 0x1C8:
      // unused... check if it gets written as well
      s_chan[ch].iNextAdr=(((unsigned long)val&0x3f)<<16)|(s_chan[ch].iNextAdr&0xFFFF);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG Next Addr Hi - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, val);
#endif
      break;
     case 0x1CA:
      // unused... check if it gets written as well
      s_chan[ch].iNextAdr=(s_chan[ch].iNextAdr & 0x3f0000) | (val & 0xFFFF);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG Next Addr Lo - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, val);
#endif
      break;
     //------------------------------------------------//
    }

   return;
  } 

 switch(r)
   {
    //-------------------------------------------------//
    case PS2_C0_SPUaddr_Hi:
      spuAddr2[0] = (((unsigned long)val&0x3f)<<16)|(spuAddr2[0]&0xFFFF);	// 6bits
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG TSAH - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_SPUaddr_Lo:
      spuAddr2[0] = (spuAddr2[0] & 0x3F0000) | (val & 0xFFFF);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG TSAL - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_SPUaddr_Hi:
      spuAddr2[1] = (((unsigned long)val&0x3f)<<16)|(spuAddr2[1]&0xFFFF);	// 6bits
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG TSAH - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_SPUaddr_Lo:
      spuAddr2[1] = (spuAddr2[1] & 0x3F0000) | (val & 0xFFFF);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG TSAL - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_SPUdata:
      spuMem[spuAddr2[0]] = val;
	  if(spuCtrl2[0]&0x40 && spuIrq2[0] == spuAddr2[0]){
			regArea[0x7C0] |= 0x4;
            regArea[PS2_IRQINFO] |= 4;
			irqCallbackSPU2();
	  }
	  spuAddr2[0]++;
      if(spuAddr2[0]>0xFFFFF) spuAddr2[0] = 0;
	  spuStat2[0]|=0x80;
	  spuCtrl2[0]&=~0x30;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG SPU Data - Core 0 Addr %X Val %X\r\n", spuAddr2[0]-1, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_SPUdata:
      spuMem[spuAddr2[1]] = val;
      
	  if(spuCtrl2[1]&0x40 && spuIrq2[1] == spuAddr2[1]){
			regArea[0x7C0] |= 0x8;
            regArea[PS2_IRQINFO] |= 8;
			irqCallbackSPU2();
	  }
	  spuAddr2[1]++;
      if(spuAddr2[1]>0xFFFFF) spuAddr2[1] = 0;
	  spuStat2[1]|=0x80;
	  spuCtrl2[1]&=~0x30;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG SPU Data - Core 1 Addr %X Val %X\r\n", spuAddr2[1]-1, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_ATTR:
      spuCtrl2[0]=val;
	  if(!(val & 0x40))regArea[PS2_IRQINFO] &= ~0x4;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG ATTR - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
	  case PS2_C0_ADMAS:
	regArea[(PS2_C0_ADMAS)]=val;
	///regArea[(0x344)>>1]&= ~0x80;
	//regArea[(0x744)>>1]&= ~0x80;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG ADMA STAT - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
	break;
	case PS2_C1_ADMAS:
	regArea[(PS2_C1_ADMAS)]=val;
	//regArea[(0x344)>>1]&= ~0x80;
	//regArea[(0x744)>>1]&= ~0x80;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG ADMA STAT - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
	break;
    case PS2_C1_ATTR:
      spuCtrl2[1]=val;
	  if(!(val & 0x40))regArea[PS2_IRQINFO] &= ~0x8;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG ATTR - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_SPUstat:
      spuStat2[0]=val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG SPU STAT - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_SPUstat:
      spuStat2[1]=val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG SPU STAT - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_ReverbAddr_Hi:
      spuRvbAddr2[0] = (((unsigned long)val&0x3f)<<16)|(spuRvbAddr2[0]&0xFFFF);
      SetReverbAddr(0);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG ESAH - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_ReverbAddr_Lo:
      spuRvbAddr2[0] = (spuRvbAddr2[0] & 0x3f0000) | (val & 0xFFFF);
      SetReverbAddr(0);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG ESAL - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_ReverbAEnd_Hi:
      spuRvbAEnd2[0] = (((unsigned long)val&0x3f)<<16)/*|(spuRvbAEnd2[0]&0xFFFF)*/;
      rvb[0].EndAddr=spuRvbAEnd2[0];
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG EEAH - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_ReverbAEnd_Hi:
      spuRvbAEnd2[1] = (((unsigned long)val&0x3f)<<16)/*|(spuRvbAEnd2[1]&0xFFFF)*/;
      rvb[1].EndAddr=spuRvbAEnd2[1];
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG EEAH - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_ReverbAddr_Hi:
      spuRvbAddr2[1] = (((unsigned long)val&0x3f)<<16)|(spuRvbAddr2[1]&0xFFFF);
      SetReverbAddr(1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG ESAH - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_ReverbAddr_Lo:
      spuRvbAddr2[1] = (spuRvbAddr2[1] & 0x3f0000) | (val & 0xFFFF);
      SetReverbAddr(1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG ESAL - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_SPUirqAddr_Hi:
      spuIrq2[0] = (((unsigned long)val&0x3f)<<16)|(spuIrq2[0]&0xFFFF); // 6bits
      pSpuIrq[0]=spuMemC+(spuIrq2[0]<<1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG IRQAH - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_SPUirqAddr_Lo:
      spuIrq2[0] = (spuIrq2[0] & 0x3F0000) | (val & 0xFFFF);
      pSpuIrq[0]=spuMemC+(spuIrq2[0]<<1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG IRQAL - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_SPUirqAddr_Hi:
      spuIrq2[1] = (((unsigned long)val&0x3f)<<16)|(spuIrq2[1]&0xFFFF);  // 6bits
      pSpuIrq[1]=spuMemC+(spuIrq2[1]<<1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG IRQAH - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_SPUirqAddr_Lo:
      spuIrq2[1] = (spuIrq2[1] & 0x3F0000) | (val & 0xFFFF);
      pSpuIrq[1]=spuMemC+(spuIrq2[1]<<1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG IRQAL - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_IRQINFO: 
      // regArea[PS2_IRQINFO] = 0; // clear
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG IRQINFO - Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, val);
#endif
       break;
    case PS2_C0_AVOLL:
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG AVOL Left - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		break;
	case PS2_C0_AVOLR:
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG AVOL Right - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		break;
	case PS2_C1_AVOLL:
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG AVOL Left - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		break;
	case PS2_C1_AVOLR:
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG AVOL Right - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		break;
    case PS2_C0_SPUrvolL:
      rvb[0].VolLeft=val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG EVOL Left - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_SPUrvolR:
      rvb[0].VolRight=val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG EVOL Right - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_SPUrvolL:
      rvb[1].VolLeft=val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG EVOL Left - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_SPUrvolR:
      rvb[1].VolRight=val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG EVOL Right - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_SPUon1:
      SoundOn(0,16,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG KON0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_SPUon2:
      SoundOn(16,24,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG KON1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_SPUon1:
      SoundOn(24,40,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG KON0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_SPUon2:
      SoundOn(40,48,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG KON1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_SPUoff1:
      SoundOff(0,16,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG KOF0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_SPUoff2:
      SoundOff(16,24,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG KOF1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_SPUoff1:
      SoundOff(24,40,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG KOF0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_SPUoff2:
      SoundOff(40,48,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG KOF1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_SPUend1:
	  dwEndChannel2[0] &= 0xff0000; // According to manual all bits are cleared by writing an arbitary value
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG ENDX0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    case PS2_C0_SPUend2:
      dwEndChannel2[0] &= 0xffff;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG ENDX1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_SPUend1:
	  dwEndChannel2[1] &= 0xff0000;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG ENDX0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    case PS2_C1_SPUend2:
      dwEndChannel2[1] &= 0xffff;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG ENDX1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_FMod1:
      FModOn(0,16,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG PMON0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_FMod2:
      FModOn(16,24,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG PMON1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_FMod1:
      FModOn(24,40,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG PMON0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_FMod2:
      FModOn(40,48,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG PMON1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_Noise1:
      NoiseOn(0,16,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG NON0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_Noise2:
      NoiseOn(16,24,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG NON1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_Noise1:
      NoiseOn(24,40,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG NON0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_Noise2:
      NoiseOn(40,48,val);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG NON1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_MVOLL:
		 regArea[PS2_C0_MVOLL] = val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG MVOL Left - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		 break;
	case PS2_C0_MVOLR:
		 regArea[PS2_C0_MVOLR] = val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG MVOL Right - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		 break;
	 case PS2_C1_MVOLL:
		 regArea[PS2_C0_MVOLL] = val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG MVOL Left - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		 break;
	case PS2_C1_MVOLR:
		 regArea[PS2_C0_MVOLR] = val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG MVOL Right - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		 break;

	case PS2_C0_BVOLL:
		 regArea[PS2_C0_BVOLL] = val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG BVOL Left - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		 break;
	case PS2_C0_BVOLR:
		 regArea[PS2_C0_BVOLR] = val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG BVOL Right - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		 break;
	case PS2_C1_BVOLL:
		 regArea[PS2_C1_BVOLL] = val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG BVOL Left - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		 break;
	case PS2_C1_BVOLR:
		 regArea[PS2_C1_BVOLR] = val;
 #ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG BVOL Right - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		 break;
    case PS2_C0_MMIX:
         regArea[PS2_C0_MMIX] = val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG MMIX - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		 break;
    case PS2_C1_MMIX:
         regArea[PS2_C1_MMIX] = val;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG MMIX - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
		 break;
    case PS2_C0_DryL1:
      VolumeOn(0,16,val,0);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXL0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_DryL2:
      VolumeOn(16,24,val,0);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXL1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_DryL1:
      VolumeOn(24,40,val,0);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXL0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_DryL2:
      VolumeOn(40,48,val,0);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXL1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_DryR1:
      VolumeOn(0,16,val,1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXR0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_DryR2:
      VolumeOn(16,24,val,1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXR1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_DryR1:
      VolumeOn(24,40,val,1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXR0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_DryR2:
      VolumeOn(40,48,val,1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXR1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_RVBon1_L:
      ReverbOn(0,16,val,0);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXEL0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_RVBon2_L:
      ReverbOn(16,24,val,0);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXEL1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_RVBon1_L:
      ReverbOn(24,40,val,0);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXEL0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_RVBon2_L:
      ReverbOn(40,48,val,0);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXEL1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_RVBon1_R:
      ReverbOn(0,16,val,1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXER0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_RVBon2_R:
      ReverbOn(16,24,val,1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXER1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_RVBon1_R:
      ReverbOn(24,40,val,1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXER0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C1_RVBon2_R:
      ReverbOn(40,48,val,1);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("W_REG VMIXER1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, val);
#endif
      break;
    //-------------------------------------------------//
    case PS2_C0_Reverb+0:
      rvb[0].FB_SRC_A=(((unsigned long)val&0x3f)<<16)|(rvb[0].FB_SRC_A&0xFFFF);
      break;
    case PS2_C0_Reverb+2:
      rvb[0].FB_SRC_A=(rvb[0].FB_SRC_A & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+4:
      rvb[0].FB_SRC_B=(((unsigned long)val&0x3f)<<16)|(rvb[0].FB_SRC_B&0xFFFF);
      break;
    case PS2_C0_Reverb+6:
      rvb[0].FB_SRC_B=(rvb[0].FB_SRC_B & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+8:
      rvb[0].IIR_DEST_A0=(((unsigned long)val&0x3f)<<16)|(rvb[0].IIR_DEST_A0&0xFFFF);
      break;
    case PS2_C0_Reverb+10:
      rvb[0].IIR_DEST_A0=(rvb[0].IIR_DEST_A0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+12:
      rvb[0].IIR_DEST_A1=(((unsigned long)val&0x3f)<<16)|(rvb[0].IIR_DEST_A1&0xFFFF);
      break;
    case PS2_C0_Reverb+14:
      rvb[0].IIR_DEST_A1=(rvb[0].IIR_DEST_A1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+16:
      rvb[0].ACC_SRC_A0=(((unsigned long)val&0x3f)<<16)|(rvb[0].ACC_SRC_A0&0xFFFF);
      break;
    case PS2_C0_Reverb+18:
      rvb[0].ACC_SRC_A0=(rvb[0].ACC_SRC_A0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+20:
      rvb[0].ACC_SRC_A1=(((unsigned long)val&0x3f)<<16)|(rvb[0].ACC_SRC_A1&0xFFFF);
      break;
    case PS2_C0_Reverb+22:
      rvb[0].ACC_SRC_A1=(rvb[0].ACC_SRC_A1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+24:
      rvb[0].ACC_SRC_B0=(((unsigned long)val&0x3f)<<16)|(rvb[0].ACC_SRC_B0&0xFFFF);
      break;
    case PS2_C0_Reverb+26:
      rvb[0].ACC_SRC_B0=(rvb[0].ACC_SRC_B0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+28:
      rvb[0].ACC_SRC_B1=(((unsigned long)val&0x3f)<<16)|(rvb[0].ACC_SRC_B1&0xFFFF);
      break;
    case PS2_C0_Reverb+30:
      rvb[0].ACC_SRC_B1=(rvb[0].ACC_SRC_B1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+32:
      rvb[0].IIR_SRC_A0=(((unsigned long)val&0x3f)<<16)|(rvb[0].IIR_SRC_A0&0xFFFF);
      break;
    case PS2_C0_Reverb+34:
      rvb[0].IIR_SRC_A0=(rvb[0].IIR_SRC_A0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+36:
      rvb[0].IIR_SRC_A1=(((unsigned long)val&0x3f)<<16)|(rvb[0].IIR_SRC_A1&0xFFFF);
      break;
    case PS2_C0_Reverb+38:
      rvb[0].IIR_SRC_A1=(rvb[0].IIR_SRC_A1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+40:
      rvb[0].IIR_DEST_B0=(((unsigned long)val&0x3f)<<16)|(rvb[0].IIR_DEST_B0&0xFFFF);
      break;
    case PS2_C0_Reverb+42:
      rvb[0].IIR_DEST_B0=(rvb[0].IIR_DEST_B0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+44:
      rvb[0].IIR_DEST_B1=(((unsigned long)val&0x3f)<<16)|(rvb[0].IIR_DEST_B1&0xFFFF);
      break;
    case PS2_C0_Reverb+46:
      rvb[0].IIR_DEST_B1=(rvb[0].IIR_DEST_B1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+48:
      rvb[0].ACC_SRC_C0=(((unsigned long)val&0x3f)<<16)|(rvb[0].ACC_SRC_C0&0xFFFF);
      break;
    case PS2_C0_Reverb+50:
      rvb[0].ACC_SRC_C0=(rvb[0].ACC_SRC_C0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+52:
      rvb[0].ACC_SRC_C1=(((unsigned long)val&0x3f)<<16)|(rvb[0].ACC_SRC_C1&0xFFFF);
      break;
    case PS2_C0_Reverb+54:
      rvb[0].ACC_SRC_C1=(rvb[0].ACC_SRC_C1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+56:
      rvb[0].ACC_SRC_D0=(((unsigned long)val&0x3f)<<16)|(rvb[0].ACC_SRC_D0&0xFFFF);
      break;
    case PS2_C0_Reverb+58:
      rvb[0].ACC_SRC_D0=(rvb[0].ACC_SRC_D0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+60:
      rvb[0].ACC_SRC_D1=(((unsigned long)val&0x3f)<<16)|(rvb[0].ACC_SRC_D1&0xFFFF);
      break;
    case PS2_C0_Reverb+62:
      rvb[0].ACC_SRC_D1=(rvb[0].ACC_SRC_D1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+64:
      rvb[0].IIR_SRC_B1=(((unsigned long)val&0x3f)<<16)|(rvb[0].IIR_SRC_B1&0xFFFF);
      break;
    case PS2_C0_Reverb+66:
      rvb[0].IIR_SRC_B1=(rvb[0].IIR_SRC_B1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+68:
      rvb[0].IIR_SRC_B0=(((unsigned long)val&0x3f)<<16)|(rvb[0].IIR_SRC_B0&0xFFFF);
      break;
    case PS2_C0_Reverb+70:
      rvb[0].IIR_SRC_B0=(rvb[0].IIR_SRC_B0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+72:
      rvb[0].MIX_DEST_A0=(((unsigned long)val&0x3f)<<16)|(rvb[0].MIX_DEST_A0&0xFFFF);
      break;
    case PS2_C0_Reverb+74:
      rvb[0].MIX_DEST_A0=(rvb[0].MIX_DEST_A0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+76:
      rvb[0].MIX_DEST_A1=(((unsigned long)val&0x3f)<<16)|(rvb[0].MIX_DEST_A1&0xFFFF);
      break;
    case PS2_C0_Reverb+78:
      rvb[0].MIX_DEST_A1=(rvb[0].MIX_DEST_A1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+80:
      rvb[0].MIX_DEST_B0=(((unsigned long)val&0x3f)<<16)|(rvb[0].MIX_DEST_B0&0xFFFF);
      break;
    case PS2_C0_Reverb+82:
      rvb[0].MIX_DEST_B0=(rvb[0].MIX_DEST_B0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_Reverb+84:
      rvb[0].MIX_DEST_B1=(((unsigned long)val&0x3f)<<16)|(rvb[0].MIX_DEST_B1&0xFFFF);
      break;
    case PS2_C0_Reverb+86:
      rvb[0].MIX_DEST_B1=(rvb[0].MIX_DEST_B1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C0_ReverbX+0:  rvb[0].IIR_ALPHA=(short)val;      break;
    case PS2_C0_ReverbX+2:  rvb[0].ACC_COEF_A=(short)val;     break;
    case PS2_C0_ReverbX+4:  rvb[0].ACC_COEF_B=(short)val;     break;
    case PS2_C0_ReverbX+6:  rvb[0].ACC_COEF_C=(short)val;     break;
    case PS2_C0_ReverbX+8:  rvb[0].ACC_COEF_D=(short)val;     break;
    case PS2_C0_ReverbX+10: rvb[0].IIR_COEF=(short)val;       break;
    case PS2_C0_ReverbX+12: rvb[0].FB_ALPHA=(short)val;       break;
    case PS2_C0_ReverbX+14: rvb[0].FB_X=(short)val;           break;
    case PS2_C0_ReverbX+16: rvb[0].IN_COEF_L=(short)val;      break;
    case PS2_C0_ReverbX+18: rvb[0].IN_COEF_R=(short)val;      break;
    //-------------------------------------------------//
    case PS2_C1_Reverb+0:
      rvb[1].FB_SRC_A=(((unsigned long)val&0x3f)<<16)|(rvb[1].FB_SRC_A&0xFFFF);
      break;
    case PS2_C1_Reverb+2:
      rvb[1].FB_SRC_A=(rvb[1].FB_SRC_A & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+4:
      rvb[1].FB_SRC_B=(((unsigned long)val&0x3f)<<16)|(rvb[1].FB_SRC_B&0xFFFF);
      break;
    case PS2_C1_Reverb+6:
      rvb[1].FB_SRC_B=(rvb[1].FB_SRC_B & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+8:
      rvb[1].IIR_DEST_A0=(((unsigned long)val&0x3f)<<16)|(rvb[1].IIR_DEST_A0&0xFFFF);
      break;
    case PS2_C1_Reverb+10:
      rvb[1].IIR_DEST_A0=(rvb[1].IIR_DEST_A0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+12:
      rvb[1].IIR_DEST_A1=(((unsigned long)val&0x3f)<<16)|(rvb[1].IIR_DEST_A1&0xFFFF);
      break;
    case PS2_C1_Reverb+14:
      rvb[1].IIR_DEST_A1=(rvb[1].IIR_DEST_A1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+16:
      rvb[1].ACC_SRC_A0=(((unsigned long)val&0x3f)<<16)|(rvb[1].ACC_SRC_A0&0xFFFF);
      break;
    case PS2_C1_Reverb+18:
      rvb[1].ACC_SRC_A0=(rvb[1].ACC_SRC_A0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+20:
      rvb[1].ACC_SRC_A1=(((unsigned long)val&0x3f)<<16)|(rvb[1].ACC_SRC_A1&0xFFFF);
      break;
    case PS2_C1_Reverb+22:
      rvb[1].ACC_SRC_A1=(rvb[1].ACC_SRC_A1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+24:
      rvb[1].ACC_SRC_B0=(((unsigned long)val&0x3f)<<16)|(rvb[1].ACC_SRC_B0&0xFFFF);
      break;
    case PS2_C1_Reverb+26:
      rvb[1].ACC_SRC_B0=(rvb[1].ACC_SRC_B0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+28:
      rvb[1].ACC_SRC_B1=(((unsigned long)val&0x3f)<<16)|(rvb[1].ACC_SRC_B1&0xFFFF);
      break;
    case PS2_C1_Reverb+30:
      rvb[1].ACC_SRC_B1=(rvb[1].ACC_SRC_B1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+32:
      rvb[1].IIR_SRC_A0=(((unsigned long)val&0x3f)<<16)|(rvb[1].IIR_SRC_A0&0xFFFF);
      break;
    case PS2_C1_Reverb+34:
      rvb[1].IIR_SRC_A0=(rvb[1].IIR_SRC_A0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+36:
      rvb[1].IIR_SRC_A1=(((unsigned long)val&0x3f)<<16)|(rvb[1].IIR_SRC_A1&0xFFFF);
      break;
    case PS2_C1_Reverb+38:
      rvb[1].IIR_SRC_A1=(rvb[1].IIR_SRC_A1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+40:
      rvb[1].IIR_DEST_B0=(((unsigned long)val&0x3f)<<16)|(rvb[1].IIR_DEST_B0&0xFFFF);
      break;
    case PS2_C1_Reverb+42:
      rvb[1].IIR_DEST_B0=(rvb[1].IIR_DEST_B0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+44:
      rvb[1].IIR_DEST_B1=(((unsigned long)val&0x3f)<<16)|(rvb[1].IIR_DEST_B1&0xFFFF);
      break;
    case PS2_C1_Reverb+46:
      rvb[1].IIR_DEST_B1=(rvb[1].IIR_DEST_B1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+48:
      rvb[1].ACC_SRC_C0=(((unsigned long)val&0x3f)<<16)|(rvb[1].ACC_SRC_C0&0xFFFF);
      break;
    case PS2_C1_Reverb+50:
      rvb[1].ACC_SRC_C0=(rvb[1].ACC_SRC_C0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+52:
      rvb[1].ACC_SRC_C1=(((unsigned long)val&0x3f)<<16)|(rvb[1].ACC_SRC_C1&0xFFFF);
      break;
    case PS2_C1_Reverb+54:
      rvb[1].ACC_SRC_C1=(rvb[1].ACC_SRC_C1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+56:
      rvb[1].ACC_SRC_D0=(((unsigned long)val&0x3f)<<16)|(rvb[1].ACC_SRC_D0&0xFFFF);
      break;
    case PS2_C1_Reverb+58:
      rvb[1].ACC_SRC_D0=(rvb[1].ACC_SRC_D0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+60:
      rvb[1].ACC_SRC_D1=(((unsigned long)val&0x3f)<<16)|(rvb[1].ACC_SRC_D1&0xFFFF);
      break;
    case PS2_C1_Reverb+62:
      rvb[1].ACC_SRC_D1=(rvb[1].ACC_SRC_D1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+64:
      rvb[1].IIR_SRC_B1=(((unsigned long)val&0x3f)<<16)|(rvb[1].IIR_SRC_B1&0xFFFF);
      break;
    case PS2_C1_Reverb+66:
      rvb[1].IIR_SRC_B1=(rvb[1].IIR_SRC_B1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+68:
      rvb[1].IIR_SRC_B0=(((unsigned long)val&0x3f)<<16)|(rvb[1].IIR_SRC_B0&0xFFFF);
      break;
    case PS2_C1_Reverb+70:
      rvb[1].IIR_SRC_B0=(rvb[1].IIR_SRC_B0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+72:
      rvb[1].MIX_DEST_A0=(((unsigned long)val&0x3f)<<16)|(rvb[1].MIX_DEST_A0&0xFFFF);
      break;
    case PS2_C1_Reverb+74:
      rvb[1].MIX_DEST_A0=(rvb[1].MIX_DEST_A0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+76:
      rvb[1].MIX_DEST_A1=(((unsigned long)val&0x3f)<<16)|(rvb[1].MIX_DEST_A1&0xFFFF);
      break;
    case PS2_C1_Reverb+78:
      rvb[1].MIX_DEST_A1=(rvb[1].MIX_DEST_A1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+80:
      rvb[1].MIX_DEST_B0=(((unsigned long)val&0x3f)<<16)|(rvb[1].MIX_DEST_B0&0xFFFF);
      break;
    case PS2_C1_Reverb+82:
      rvb[1].MIX_DEST_B0=(rvb[1].MIX_DEST_B0 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_Reverb+84:
      rvb[1].MIX_DEST_B1=(((unsigned long)val&0x3f)<<16)|(rvb[1].MIX_DEST_B1&0xFFFF);
      break;
    case PS2_C1_Reverb+86:
      rvb[1].MIX_DEST_B1=(rvb[1].MIX_DEST_B1 & 0x3f0000) | ((val) & 0xFFFF);
      break;
    case PS2_C1_ReverbX+0:  rvb[1].IIR_ALPHA=(short)val;      break;
    case PS2_C1_ReverbX+2:  rvb[1].ACC_COEF_A=(short)val;     break;
    case PS2_C1_ReverbX+4:  rvb[1].ACC_COEF_B=(short)val;     break;
    case PS2_C1_ReverbX+6:  rvb[1].ACC_COEF_C=(short)val;     break;
    case PS2_C1_ReverbX+8:  rvb[1].ACC_COEF_D=(short)val;     break;
    case PS2_C1_ReverbX+10: rvb[1].IIR_COEF=(short)val;       break;
    case PS2_C1_ReverbX+12: rvb[1].FB_ALPHA=(short)val;       break;
    case PS2_C1_ReverbX+14: rvb[1].FB_X=(short)val;           break;
    case PS2_C1_ReverbX+16: rvb[1].IN_COEF_L=(short)val;      break;
    case PS2_C1_ReverbX+18: rvb[1].IN_COEF_R=(short)val;      break;
	case PS2_SPDIF_OUT:
#ifdef _WINDOWS
 if(iDebugMode==1) logprintf("W_REG SPDIF_OUT Addr %X Value %X - Not Implemented!\r\n",reg&0xFFFF,val);
#endif
	break;
	case PS2_SPDIF_MODE:
#ifdef _WINDOWS
 if(iDebugMode==1) logprintf("W_REG SPDIF_MODE Addr %X Value %X - Not Implemented!\r\n",reg&0xFFFF,val);
#endif
	break;
	case PS2_SPDIF_MEDIA:
#ifdef _WINDOWS
 if(iDebugMode==1) logprintf("W_REG SPDIF_OUT Addr %X Value %X - Not Implemented!\r\n",reg&0xFFFF,val);
#endif
	break;
	case PS2_SPDIF_COPY:
#ifdef _WINDOWS
 if(iDebugMode==1) logprintf("W_REG SPDIF_COPY Addr %X Value %X - Not Implemented!\r\n",reg&0xFFFF,val);
#endif
	break;
	default: //All other writes
#ifdef _WINDOWS
 if(iDebugMode==1) logprintf("W_REG Unknown Register Addr %X Value %X\r\n",reg&0xFFFF,val);
#endif
	break;
   }

 iSpuAsyncWait=0;

}

////////////////////////////////////////////////////////////////////////
// READ REGISTER: called by main emu
////////////////////////////////////////////////////////////////////////
EXPORT_GCC unsigned short CALLBACK SPU2read(unsigned long reg)
{
 long r=reg&0xffff;
 unsigned int ret = 0;

if(spuCtrl2[0]&0x40 && spuIrq2[0] == r){
			//regArea[0x7C0] |= 0x4;
            regArea[PS2_IRQINFO] |= 4;
			irqCallbackSPU2();
	  }
if(spuCtrl2[1]&0x40 && spuIrq2[1] == r){
			//regArea[0x7C0] |= 0x8;
            regArea[PS2_IRQINFO] |= 8;
			irqCallbackSPU2();
	  }
 if((r>=0x0000 && r<0x0180)||(r>=0x0400 && r<0x0580))  // some channel info?
  {
  int addr=0;
  int ch=((r & 0x3ff)>>4);
  if(r>=0x400) ch+=24;

   switch(r&0x0f)
    {
	 case 0: //Left Volume
		 ret = s_chan[ch].iLeftVolRaw;
#ifdef _WINDOWS
		if(iDebugMode==1) logprintf("R_REG Volume Left - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, ret);
#endif
		 break;
	 case 2: //Right Volume
		  ret = s_chan[ch].iRightVolRaw;
#ifdef _WINDOWS
		if(iDebugMode==1) logprintf("R_REG Volume Right - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, ret);
#endif
		  break;
	 case 4: //Pitch
		 ret = regArea[r];
#ifdef _WINDOWS
		if(iDebugMode==1) logprintf("R_REG Pitch - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, ret);
#endif
		 break;
	 case 6: //ASDR1
		 ret = regArea[r];
#ifdef _WINDOWS
		if(iDebugMode==1) logprintf("R_REG ASDR1 - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, ret);
#endif
		 break;
	 case 8: //ASDR2
		 ret = regArea[r];
 #ifdef _WINDOWS
		if(iDebugMode==1) logprintf("R_REG ASDR2 - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, ret);
#endif
		 break;
	 case 10: //Envelope (current)
		 ret = (unsigned short)(s_chan[ch].ADSRX.EnvelopeVol>>16);
#ifdef _WINDOWS
		if(iDebugMode==1) logprintf("R_REG Envelope - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, ret);
#endif
		 break;
  
    }

  }else
	 if((r>=0x01c0 && r<0x02E0)||(r>=0x05c0 && r<0x06E0))  // some channel info?
	  {
	   int ch=0;unsigned long rx=r;
	    
	   ch=((rx & 0x3ff)-0x1c0) / 0xC;
	   
	   rx-=ch*0xC;
	   if(rx>=0x400) {ch+=24;rx-=0x400;}
	  
	   switch(rx)
		{
			case 0x1C0:
		  ret = (((s_chan[ch].pStart-spuMemC)>>17)&0x3F);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG Start Addr Hi - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, ret);
#endif
		  break;
		 case 0x1C2:
		  ret = (((s_chan[ch].pStart-spuMemC)>>1)&0xFFFF);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG Start Addr Lo - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, ret);
#endif
		  break;
		 //------------------------------------------------//
		 case 0x1C4:
		  ret = (((s_chan[ch].pLoop-spuMemC)>>17)&0x3F);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG Loop Addr Hi - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, ret);
#endif
		  break;
		 case 0x1C6:
		  ret = (((s_chan[ch].pLoop-spuMemC)>>1)&0xFFFF);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG Loop Addr Lo - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, ret);
#endif
		  break;
		 //------------------------------------------------//
		 case 0x1C8:
		  ret = ((((s_chan[ch].pCurr)-spuMemC)>>17)&0x3F);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG Next Addr Hi - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, ret);
#endif
		  break;
		 case 0x1CA:
		  ret = ((((s_chan[ch].pCurr)-spuMemC)>>1)&0xFFFF);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG Next Addr Lo - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, ret);
#endif
		  break;
		 default:
			 ret = regArea[rx];
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG Channel Addr Unknown - Channel %d Core %d Addr %X Val %X\r\n", (ch > 23? (ch - 24) : ch), ch/24, reg&0xFFFF, ret);
#endif
			 break;

		 //------------------------------------------------//
		}
	}else{
		 switch(r)
		  {
		   //--------------------------------------------------//
		   case PS2_C0_SPUend1:
			 ret = (unsigned short)((dwEndChannel2[0]&0xFFFF));
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG ENDX 0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;
		   case PS2_C0_SPUend2:
			 ret =  (unsigned short)((dwEndChannel2[0]>>16));
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG ENDX 1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;
		   //--------------------------------------------------//
		   case PS2_C1_SPUend1:
			 ret = (unsigned short)((dwEndChannel2[1]&0xFFFF));
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG ENDX 0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		   break;
		   case PS2_C1_SPUend2:
			ret = (unsigned short)((dwEndChannel2[1]>>16));
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG ENDX 1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		   break;
		   //--------------------------------------------------//
		   case PS2_C0_ATTR:
			 ret = spuCtrl2[0];
 #ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG Core Attr - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		   break;
		   //--------------------------------------------------//
		   case PS2_C1_ATTR:
			ret = spuCtrl2[1];
 #ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG Core Attr - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		   break;
		   //--------------------------------------------------//
		   case PS2_C0_SPUstat:
			 ret = spuStat2[0];
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG SPU Status - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		   break;
		   //--------------------------------------------------//
		   case PS2_C1_SPUstat:
			 ret = spuStat2[1];
 #ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG SPU Status - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		   break;
		   //--------------------------------------------------//
		   case PS2_C0_SPUdata:
			 {
			  unsigned short s=spuMem[spuAddr2[0]];
		      
			  if(spuCtrl2[0]&0x40 && spuIrq2[0] == spuAddr2[0]){
					regArea[0x7C0] |= 0x4;
					regArea[PS2_IRQINFO] |= 4;
					irqCallbackSPU2();
			  }
			  spuAddr2[0]++;
			  if(spuAddr2[0]>0xFFFFF) spuAddr2[0] = 0;
			  spuStat2[0]|=0x80;
			  spuCtrl2[0]&=~0x30;
			  ret = s;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG SPU Data - Core 0 Addr %X SSA %x Val %X\r\n", reg&0xFFFF, spuAddr2[0]-1, ret);
#endif
			 }
		   break;
		   //--------------------------------------------------//
		   case PS2_C1_SPUdata:
			 {
			  unsigned short s=spuMem[spuAddr2[1]];
		      
			  if(spuCtrl2[1]&0x40 && spuIrq2[1] == spuAddr2[1]){
					regArea[0x7C0] |= 0x8;
					regArea[PS2_IRQINFO] |= 8;
					irqCallbackSPU2();
			  }
			  spuAddr2[1]++;
			  if(spuAddr2[1]>0xFFFFF) spuAddr2[1] = 0;
			  spuStat2[1]|=0x80;
			  spuCtrl2[1]&=~0x30;
			  ret = s;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG SPU Data - Core 1 Addr %X SSA %x Val %X\r\n", reg&0xFFFF, spuAddr2[1]-1, ret);
#endif
			 }
			 break;
		   //--------------------------------------------------//
		   case PS2_C0_SPUaddr_Hi:
			 ret = (unsigned short)((spuAddr2[0]>>16)&0x3F);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG TSAH - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;
		   case PS2_C0_SPUaddr_Lo:
			 ret = (unsigned short)((spuAddr2[0]&0xFFFF));
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG TSAL - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;
		   //--------------------------------------------------//
		   case PS2_C1_SPUaddr_Hi:
			 ret = (unsigned short)((spuAddr2[1]>>16)&0x3F);
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG TSAH - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;
		   case PS2_C1_SPUaddr_Lo:
			 ret = (unsigned short)((spuAddr2[1]&0xFFFF));
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG TSAL - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;
			 case PS2_C0_ReverbAEnd_Hi:
			  ret = regArea[PS2_C0_ReverbAEnd_Hi];
#ifdef _WINDOWS
					if(iDebugMode==1) logprintf("R_REG EEAH - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			  break;
			//-------------------------------------------------//
			case PS2_C1_ReverbAEnd_Hi:
				 ret = regArea[PS2_C1_ReverbAEnd_Hi];
#ifdef _WINDOWS
					if(iDebugMode==1) logprintf("R_REG EEAH - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			  break;
		    case PS2_C0_ReverbAddr_Lo:
				  ret = regArea[PS2_C0_ReverbAddr_Lo];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG ESAL - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;

			case PS2_C1_ReverbAddr_Lo:
				  ret = regArea[PS2_C1_ReverbAddr_Lo];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG ESAL - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
		   case PS2_IRQINFO:
			   ret = regArea[PS2_IRQINFO];
			  // regArea[PS2_IRQINFO] = 0; // clear
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG IRQINFO - Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			  break;
		   case PS2_C0_ADMAS:
			ret = regArea[(PS2_C0_ADMAS)];
			///regArea[(0x344)>>1]&= ~0x80;
			//regArea[(0x744)>>1]&= ~0x80;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG ADMA STAT - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				break;
				case PS2_C1_ADMAS:
			ret = regArea[(PS2_C1_ADMAS)];
			//regArea[(0x344)>>1]&= ~0x80;
			//regArea[(0x744)>>1]&= ~0x80;
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG ADMA STAT - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				break;
		   //--------------------------------------------------//
				case PS2_C0_SPUrvolL:
		  ret = rvb[0].VolLeft;
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG EVOL Left - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C0_SPUrvolR:
		  ret = rvb[0].VolRight;
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG EVOL Right - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C1_SPUrvolL:
		  ret = rvb[1].VolLeft;
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG EVOL Left - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C1_SPUrvolR:
		  ret = rvb[1].VolRight;
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG EVOL Right - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C0_SPUon1:
		  ret = regArea[PS2_C0_SPUon1];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG KON0 - Core 0 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C0_SPUon2:
		  ret = regArea[PS2_C0_SPUon2];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG KON1 - Core 0 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C1_SPUon1:
		  ret = regArea[PS2_C1_SPUon1];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG KON0 - Core 1 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C1_SPUon2:
		  ret = regArea[PS2_C1_SPUon2];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG KON1 - Core 1 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C0_SPUoff1:
		  ret = regArea[PS2_C0_SPUoff1];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG KOF0 - Core 0 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C0_SPUoff2:
		  ret = regArea[PS2_C0_SPUoff2];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG KOF1 - Core 0 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C1_SPUoff1:
		  ret = regArea[PS2_C1_SPUoff1];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG KOF0 - Core 1 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C1_SPUoff2:
		  ret = regArea[PS2_C1_SPUoff2];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG KOF1 - Core 1 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C0_FMod1:
		  ret = regArea[PS2_C0_FMod1];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG PMON0 - Core 0 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C0_FMod2:
		  ret = regArea[PS2_C0_FMod2];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG PMON1 - Core 0 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C1_FMod1:
		  ret = regArea[PS2_C1_FMod1];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG PMON0 - Core 1 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C1_FMod2:
		  ret = regArea[PS2_C1_FMod2];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG PMON1 - Core 1 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C0_Noise1:
		  ret = regArea[PS2_C0_Noise1];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG NON0 - Core 0 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C0_Noise2:
		 ret = regArea[PS2_C0_Noise2];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG NON1 - Core 0 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C1_Noise1:
				ret = regArea[PS2_C1_Noise1];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG NON0 - Core 1 Addr %X Val %X - Not Implemented!\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
			case PS2_C1_Noise2:
				ret = regArea[PS2_C1_Noise2];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG NON1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		  break;
		//-------------------------------------------------//
		  case PS2_C0_AVOLL:
				 ret =  regArea[PS2_C0_AVOLL];
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG AVOL Left - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		break;
			case PS2_C0_AVOLR:
				ret =  regArea[PS2_C0_AVOLR];
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG AVOL Right - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		break;
			case PS2_C1_AVOLL:
				 ret =  regArea[PS2_C1_AVOLL];
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG AVOL Left - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		break;
			case PS2_C1_AVOLR:
				ret =  regArea[PS2_C1_AVOLR];
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG AVOL Right - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
		break;
			case PS2_C0_MVOLL:
			ret =  regArea[PS2_C0_MVOLL];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG MVOL Left - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;
			case PS2_C0_MVOLR:
			ret =  regArea[PS2_C0_MVOLR];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG MVOL Right - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;
		 case PS2_C1_MVOLL:
			 regArea[PS2_C0_MVOLL];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG MVOL Left - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;
			case PS2_C1_MVOLR:
			 ret = regArea[PS2_C0_MVOLR];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG MVOL Right - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;

			case PS2_C0_BVOLL:
			 ret = regArea[PS2_C0_BVOLL];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG BVOL Left - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;
			case PS2_C0_BVOLR:
			 ret = regArea[PS2_C0_BVOLR];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG BVOL Right - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;
			case PS2_C1_BVOLL:
			 ret = regArea[PS2_C1_BVOLL];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG BVOL Left - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;
			case PS2_C1_BVOLR:
			 ret = regArea[PS2_C1_BVOLR];
	 #ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG BVOL Right - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
			 break;
				case PS2_C0_MMIX:
				ret = regArea[PS2_C0_MMIX];
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG MMIX - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				break;
				case PS2_C1_MMIX:
				ret = regArea[PS2_C1_MMIX];
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG MMIX - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
					break;
				case PS2_C0_DryL1:
				  ret = regArea[PS2_C0_DryL1];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXL0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C0_DryL2:
				  ret = regArea[PS2_C0_DryL2];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXL1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C1_DryL1:
				  ret = regArea[PS2_C1_DryL1];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXL0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C1_DryL2:
				  ret = regArea[PS2_C1_DryL2];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXL1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C0_DryR1:
				  ret = regArea[PS2_C0_DryR1];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXR0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C0_DryR2:
				  ret = regArea[PS2_C0_DryR2];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXR1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C1_DryR1:
				  ret = regArea[PS2_C1_DryR1];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXR0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C1_DryR2:
				  ret = regArea[PS2_C1_DryR2];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXR1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C0_RVBon1_L:
				  ret = regArea[PS2_C0_RVBon1_L];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXEL0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C0_RVBon2_L:
				  ret = regArea[PS2_C0_RVBon2_L];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXEL1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C1_RVBon1_L:
				  ret = regArea[PS2_C1_RVBon1_L];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXEL0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C1_RVBon2_L:
				  ret = regArea[PS2_C1_RVBon2_L];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXEL1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C0_RVBon1_R:
				  ret = regArea[PS2_C0_RVBon1_R];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXER0 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C0_RVBon2_R:
				  ret = regArea[PS2_C0_RVBon2_R];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXER1 - Core 0 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C1_RVBon1_R:
				  ret = regArea[PS2_C1_RVBon1_R];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXER0 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				//-------------------------------------------------//
				case PS2_C1_RVBon2_R:
				  ret = regArea[PS2_C1_RVBon2_R];
#ifdef _WINDOWS
				if(iDebugMode==1) logprintf("R_REG VMIXER1 - Core 1 Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
				  break;
				case PS2_SPDIF_OUT:
				ret = regArea[PS2_SPDIF_OUT];
#ifdef _WINDOWS
			 if(iDebugMode==1) logprintf("R_REG SPDIF_OUT Addr %X Value %X - Not Implemented!\r\n",reg&0xFFFF,ret);
#endif
					break;
				case PS2_SPDIF_MODE:
				ret = regArea[PS2_SPDIF_MODE];
#ifdef _WINDOWS
			 if(iDebugMode==1) logprintf("R_REG SPDIF_MODE Addr %X Value %X - Not Implemented!\r\n",reg&0xFFFF,ret);
#endif
					break;
				case PS2_SPDIF_MEDIA:
				ret = regArea[PS2_SPDIF_MEDIA];
#ifdef _WINDOWS
			 if(iDebugMode==1) logprintf("R_REG SPDIF_OUT Addr %X Value %X - Not Implemented!\r\n",reg&0xFFFF,ret);
#endif
					break;
				case PS2_SPDIF_COPY:
				ret = regArea[PS2_SPDIF_COPY];
#ifdef _WINDOWS
			 if(iDebugMode==1) logprintf("R_REG SPDIF_COPY Addr %X Value %X - Not Implemented!\r\n",reg&0xFFFF,ret);
#endif
					break;
			default:
				ret = regArea[r];
#ifdef _WINDOWS
			if(iDebugMode==1) logprintf("R_REG Unknown Register - Addr %X Val %X\r\n", reg&0xFFFF, ret);
#endif
					break;
		  }
 /*#ifdef _WINDOWS
 if(iDebugMode==1) logprintf("R_REG %X Val %X\r\n",reg&0xFFFF, ret);
#endif*/
	}

 
 return ret;
}
 
////////////////////////////////////////////////////////////////////////
// SOUND ON register write
////////////////////////////////////////////////////////////////////////

void SoundOn(int start,int end,unsigned short val)     // SOUND ON PSX COMAND
{
 int ch;

 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if((val&1) && s_chan[ch].pStart)                    // mmm... start has to be set before key on !?!
    {
	 //s_chan[ch].bStop = 0;
	if(ch > 24){
			int tempchan = ch-24;
			if(tempchan > 15) {
			   regArea[PS2_C1_SPUoff2] &= ~(1<<(tempchan-16));
			}else{
			   regArea[PS2_C1_SPUoff1] &= ~(1<<tempchan);
			}
		} else {
			int tempchan = ch;
			if(tempchan > 15) {
			   regArea[PS2_C0_SPUoff2] &= ~(1<<(tempchan-16));
			}else{
			   regArea[PS2_C0_SPUoff1] &= ~(1<<tempchan);
			}
		}
     s_chan[ch].bNew=1;
     dwNewChannel2[ch/24]|=(1<<(ch%24));               // bitfield for faster testing
    }
  }
}

////////////////////////////////////////////////////////////////////////
// SOUND OFF register write
////////////////////////////////////////////////////////////////////////

void SoundOff(int start,int end,unsigned short val)    // SOUND OFF PSX COMMAND
{
 int ch;
 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if(val&1)                                           // && s_chan[i].bOn)  mmm...
    {
	if(ch > 24){
			int tempchan = ch-24;
			if(tempchan > 15) {
			   regArea[PS2_C1_SPUon2] &= ~(1<<(tempchan-16));
			}else{
			   regArea[PS2_C1_SPUon1] &= ~(1<<tempchan);
			}
		} else {
			int tempchan = ch;
			if(tempchan > 15) {
			   regArea[PS2_C0_SPUon2] &= ~(1<<(tempchan-16));
			}else{
			   regArea[PS2_C0_SPUon1] &= ~(1<<tempchan);
			}
		}
     s_chan[ch].bStop=1;
    }                                                  
  }
}

////////////////////////////////////////////////////////////////////////
// FMOD register write
////////////////////////////////////////////////////////////////////////

void FModOn(int start,int end,unsigned short val)      // FMOD ON PSX COMMAND
{
 int ch;

 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if(val&1)                                           // -> fmod on/off
    {
     if(ch>0) 
      {
       s_chan[ch].bFMod=1;                             // --> sound channel
       s_chan[ch-1].bFMod=2;                           // --> freq channel
      }
    }
   else
    {
     s_chan[ch].bFMod=0;                               // --> turn off fmod
    }
  }
}

////////////////////////////////////////////////////////////////////////
// NOISE register write
////////////////////////////////////////////////////////////////////////

void NoiseOn(int start,int end,unsigned short val)     // NOISE ON PSX COMMAND
{
 int ch;

 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if(val&1)                                           // -> noise on/off
    {
     s_chan[ch].bNoise=1;
    }
   else 
    {
     s_chan[ch].bNoise=0;
    }
  }
}

////////////////////////////////////////////////////////////////////////
// LEFT VOLUME register write
////////////////////////////////////////////////////////////////////////

// please note: sweep and phase invert are wrong... but I've never seen
// them used

void SetVolumeL(unsigned char ch,short vol)            // LEFT VOLUME
{
 s_chan[ch].iLeftVolRaw=vol;

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
 s_chan[ch].iLeftVolume=vol;                           // store volume
}

////////////////////////////////////////////////////////////////////////
// RIGHT VOLUME register write
////////////////////////////////////////////////////////////////////////

void SetVolumeR(unsigned char ch,short vol)            // RIGHT VOLUME
{
 s_chan[ch].iRightVolRaw=vol;

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

 s_chan[ch].iRightVolume=vol;
}

////////////////////////////////////////////////////////////////////////
// PITCH register write
////////////////////////////////////////////////////////////////////////

void SetPitch(int ch,unsigned short val)               // SET PITCH
{
 int NP;
 if(val>0x3fff) NP=0x3fff;                             // get pitch val
 else           NP=val;

 s_chan[ch].iRawPitch=NP;

 NP=(48000L*NP)/4096L;	                               // calc frequency(48000Hz)
 if(NP<1) NP=1;                                        // some security
 s_chan[ch].iActFreq=NP;                               // store frequency
}

////////////////////////////////////////////////////////////////////////
// REVERB register write
////////////////////////////////////////////////////////////////////////

void ReverbOn(int start,int end,unsigned short val,int iRight)  // REVERB ON PSX COMMAND
{
 int ch;

 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if(val&1)                                           // -> reverb on/off
    {
     if(iRight) s_chan[ch].bReverbR=1;
     else       s_chan[ch].bReverbL=1;
    }
   else 
    {
     if(iRight) s_chan[ch].bReverbR=0;
     else       s_chan[ch].bReverbL=0;
    }
  }
}

////////////////////////////////////////////////////////////////////////
// REVERB START register write
////////////////////////////////////////////////////////////////////////

void SetReverbAddr(int core)
{
 long val=spuRvbAddr2[core];
 
 if(rvb[core].StartAddr!=val)
  {
   if(val<=0x27ff)
    {
     rvb[core].StartAddr=rvb[core].CurrAddr=0;
    }
   else
    {
     rvb[core].StartAddr=val;
     rvb[core].CurrAddr=rvb[core].StartAddr;
    } 
  }
}

////////////////////////////////////////////////////////////////////////
// DRY LEFT/RIGHT per voice switches
////////////////////////////////////////////////////////////////////////

void VolumeOn(int start,int end,unsigned short val,int iRight)  // VOLUME ON PSX COMMAND
{
 int ch;

 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if(val&1)                                           // -> reverb on/off
    {
     if(iRight) s_chan[ch].bVolumeR=1;
     else       s_chan[ch].bVolumeL=1;
    }
   else 
    {
     if(iRight) s_chan[ch].bVolumeR=0;
     else       s_chan[ch].bVolumeL=0;
    }
  }
}


