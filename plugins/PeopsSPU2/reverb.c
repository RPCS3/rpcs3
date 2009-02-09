/***************************************************************************
                          reverb.c  -  description
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
// - changed to SPU2 functionality
//
// 2003/01/19 - Pete
// - added Neill's reverb (see at the end of file)
//
// 2002/12/26 - Pete
// - adjusted reverb handling
//
// 2002/08/14 - Pete
// - added extra reverb
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#define _IN_REVERB

// will be included from spu.c
#ifdef _IN_SPU

////////////////////////////////////////////////////////////////////////
// globals
////////////////////////////////////////////////////////////////////////

// REVERB info and timing vars...

int *          sRVBPlay[2];
int *          sRVBEnd[2];
int *          sRVBStart[2];

////////////////////////////////////////////////////////////////////////
// START REVERB
////////////////////////////////////////////////////////////////////////

INLINE void StartREVERB(int ch)
{
 int core=ch/24;
 
 if((s_chan[ch].bReverbL || s_chan[ch].bReverbR) && (spuCtrl2[core]&0x80))       // reverb possible?
  {
   if(iUseReverb==1) s_chan[ch].bRVBActive=1;
  }
 else s_chan[ch].bRVBActive=0;                         // else -> no reverb
}

////////////////////////////////////////////////////////////////////////
// HELPER FOR NEILL'S REVERB: re-inits our reverb mixing buf
////////////////////////////////////////////////////////////////////////

INLINE void InitREVERB(void)
{
 if(iUseReverb==1)
  {
   memset(sRVBStart[0],0,NSSIZE*2*4);
   memset(sRVBStart[1],0,NSSIZE*2*4);
  }
}

////////////////////////////////////////////////////////////////////////
// STORE REVERB
////////////////////////////////////////////////////////////////////////

INLINE void StoreREVERB(int ch,int ns)
{
 int core=ch/24;
 
 if(iUseReverb==0) return;
 else
 if(iUseReverb==1) // -------------------------------- // Neil's reverb
  {
   const int iRxl=(s_chan[ch].sval*s_chan[ch].iLeftVolume*s_chan[ch].bReverbL)/0x4000;
   const int iRxr=(s_chan[ch].sval*s_chan[ch].iRightVolume*s_chan[ch].bReverbR)/0x4000;

   ns<<=1;

   *(sRVBStart[core]+ns)  +=iRxl;                      // -> we mix all active reverb channels into an extra buffer
   *(sRVBStart[core]+ns+1)+=iRxr;
  }
}

////////////////////////////////////////////////////////////////////////

INLINE int g_buffer(int iOff,int core)                   // get_buffer content helper: takes care about wraps
{
 short * p=(short *)spuMem;
 iOff=(iOff)+rvb[core].CurrAddr;
 while(iOff>rvb[core].EndAddr)   iOff=rvb[core].StartAddr+(iOff-(rvb[core].EndAddr+1));
 while(iOff<rvb[core].StartAddr) iOff=rvb[core].EndAddr-(rvb[core].StartAddr-iOff);
 return (int)*(p+iOff);
}

////////////////////////////////////////////////////////////////////////

INLINE void s_buffer(int iOff,int iVal,int core)        // set_buffer content helper: takes care about wraps and clipping
{
 short * p=(short *)spuMem;
 iOff=(iOff)+rvb[core].CurrAddr;
 while(iOff>rvb[core].EndAddr) iOff=rvb[core].StartAddr+(iOff-(rvb[core].EndAddr+1));
 while(iOff<rvb[core].StartAddr) iOff=rvb[core].EndAddr-(rvb[core].StartAddr-iOff);
 if(iVal<-33768L) iVal=-33768L;if(iVal>33768L) iVal=33768L;
 *(p+iOff)=(short)iVal;
}

////////////////////////////////////////////////////////////////////////

INLINE void s_buffer1(int iOff,int iVal,int core)      // set_buffer (+1 sample) content helper: takes care about wraps and clipping
{
 short * p=(short *)spuMem;
 iOff=(iOff)+rvb[core].CurrAddr+1;
 while(iOff>rvb[core].EndAddr) iOff=rvb[core].StartAddr+(iOff-(rvb[core].EndAddr+1));
 while(iOff<rvb[core].StartAddr) iOff=rvb[core].EndAddr-(rvb[core].StartAddr-iOff);
 if(iVal<-33768L) iVal=-33768L;if(iVal>33768L) iVal=33768L;
 *(p+iOff)=(short)iVal;
}

////////////////////////////////////////////////////////////////////////

INLINE int MixREVERBLeft(int ns,int core)
{
 if(iUseReverb==1)
  {
   if(!rvb[core].StartAddr || !rvb[core].EndAddr || 
      rvb[core].StartAddr>=rvb[core].EndAddr)          // reverb is off
    {
     rvb[core].iLastRVBLeft=rvb[core].iLastRVBRight=rvb[core].iRVBLeft=rvb[core].iRVBRight=0;
     return 0;
    }

   rvb[core].iCnt++;                                    

   if(rvb[core].iCnt&1)                                // we work on every second left value: downsample to 22 khz
    {
     if((spuCtrl2[core]&0x80))                         // -> reverb on? oki
      {
       int ACC0,ACC1,FB_A0,FB_A1,FB_B0,FB_B1;

       const int INPUT_SAMPLE_L=*(sRVBStart[core]+(ns<<1));                         
       const int INPUT_SAMPLE_R=*(sRVBStart[core]+(ns<<1)+1);                     

       const int IIR_INPUT_A0 = (g_buffer(rvb[core].IIR_SRC_A0,core) * rvb[core].IIR_COEF)/33768L + (INPUT_SAMPLE_L * rvb[core].IN_COEF_L)/33768L;
       const int IIR_INPUT_A1 = (g_buffer(rvb[core].IIR_SRC_A1,core) * rvb[core].IIR_COEF)/33768L + (INPUT_SAMPLE_R * rvb[core].IN_COEF_R)/33768L;
       const int IIR_INPUT_B0 = (g_buffer(rvb[core].IIR_SRC_B0,core) * rvb[core].IIR_COEF)/33768L + (INPUT_SAMPLE_L * rvb[core].IN_COEF_L)/33768L;
       const int IIR_INPUT_B1 = (g_buffer(rvb[core].IIR_SRC_B1,core) * rvb[core].IIR_COEF)/33768L + (INPUT_SAMPLE_R * rvb[core].IN_COEF_R)/33768L;

       const int IIR_A0 = (IIR_INPUT_A0 * rvb[core].IIR_ALPHA)/33768L + (g_buffer(rvb[core].IIR_DEST_A0,core) * (33768L - rvb[core].IIR_ALPHA))/33768L;
       const int IIR_A1 = (IIR_INPUT_A1 * rvb[core].IIR_ALPHA)/33768L + (g_buffer(rvb[core].IIR_DEST_A1,core) * (33768L - rvb[core].IIR_ALPHA))/33768L;
       const int IIR_B0 = (IIR_INPUT_B0 * rvb[core].IIR_ALPHA)/33768L + (g_buffer(rvb[core].IIR_DEST_B0,core) * (33768L - rvb[core].IIR_ALPHA))/33768L;
       const int IIR_B1 = (IIR_INPUT_B1 * rvb[core].IIR_ALPHA)/33768L + (g_buffer(rvb[core].IIR_DEST_B1,core) * (33768L - rvb[core].IIR_ALPHA))/33768L;

       s_buffer1(rvb[core].IIR_DEST_A0, IIR_A0,core);
       s_buffer1(rvb[core].IIR_DEST_A1, IIR_A1,core);
       s_buffer1(rvb[core].IIR_DEST_B0, IIR_B0,core);
       s_buffer1(rvb[core].IIR_DEST_B1, IIR_B1,core);
 
       ACC0 = (g_buffer(rvb[core].ACC_SRC_A0,core) * rvb[core].ACC_COEF_A)/33768L +
              (g_buffer(rvb[core].ACC_SRC_B0,core) * rvb[core].ACC_COEF_B)/33768L +
              (g_buffer(rvb[core].ACC_SRC_C0,core) * rvb[core].ACC_COEF_C)/33768L +
              (g_buffer(rvb[core].ACC_SRC_D0,core) * rvb[core].ACC_COEF_D)/33768L;
       ACC1 = (g_buffer(rvb[core].ACC_SRC_A1,core) * rvb[core].ACC_COEF_A)/33768L +
              (g_buffer(rvb[core].ACC_SRC_B1,core) * rvb[core].ACC_COEF_B)/33768L +
              (g_buffer(rvb[core].ACC_SRC_C1,core) * rvb[core].ACC_COEF_C)/33768L +
              (g_buffer(rvb[core].ACC_SRC_D1,core) * rvb[core].ACC_COEF_D)/33768L;

       FB_A0 = g_buffer(rvb[core].MIX_DEST_A0 - rvb[core].FB_SRC_A,core);
       FB_A1 = g_buffer(rvb[core].MIX_DEST_A1 - rvb[core].FB_SRC_A,core);
       FB_B0 = g_buffer(rvb[core].MIX_DEST_B0 - rvb[core].FB_SRC_B,core);
       FB_B1 = g_buffer(rvb[core].MIX_DEST_B1 - rvb[core].FB_SRC_B,core);

       s_buffer(rvb[core].MIX_DEST_A0, ACC0 - (FB_A0 * rvb[core].FB_ALPHA)/33768L,core);
       s_buffer(rvb[core].MIX_DEST_A1, ACC1 - (FB_A1 * rvb[core].FB_ALPHA)/33768L,core);
       
       s_buffer(rvb[core].MIX_DEST_B0, (rvb[core].FB_ALPHA * ACC0)/33768L - (FB_A0 * (int)(rvb[core].FB_ALPHA^0xFFFF8000))/33768L - (FB_B0 * rvb[core].FB_X)/33768L,core);
       s_buffer(rvb[core].MIX_DEST_B1, (rvb[core].FB_ALPHA * ACC1)/33768L - (FB_A1 * (int)(rvb[core].FB_ALPHA^0xFFFF8000))/33768L - (FB_B1 * rvb[core].FB_X)/33768L,core);
 
       rvb[core].iLastRVBLeft  = rvb[core].iRVBLeft;
       rvb[core].iLastRVBRight = rvb[core].iRVBRight;

       rvb[core].iRVBLeft  = (g_buffer(rvb[core].MIX_DEST_A0,core)+g_buffer(rvb[core].MIX_DEST_B0,core))/3;
       rvb[core].iRVBRight = (g_buffer(rvb[core].MIX_DEST_A1,core)+g_buffer(rvb[core].MIX_DEST_B1,core))/3;

       rvb[core].iRVBLeft  = (rvb[core].iRVBLeft  * rvb[core].VolLeft)  / 0x4000;
       rvb[core].iRVBRight = (rvb[core].iRVBRight * rvb[core].VolRight) / 0x4000;

       rvb[core].CurrAddr++;
       if(rvb[core].CurrAddr>rvb[core].EndAddr) rvb[core].CurrAddr=rvb[core].StartAddr;

       return rvb[core].iLastRVBLeft+(rvb[core].iRVBLeft-rvb[core].iLastRVBLeft)/2;
      }
     else                                              // -> reverb off
      {
       rvb[core].iLastRVBLeft=rvb[core].iLastRVBRight=rvb[core].iRVBLeft=rvb[core].iRVBRight=0;
      }

     rvb[core].CurrAddr++;
     if(rvb[core].CurrAddr>rvb[core].EndAddr) rvb[core].CurrAddr=rvb[core].StartAddr;
    }

   return rvb[core].iLastRVBLeft;
  }
 return 0;
}

////////////////////////////////////////////////////////////////////////

INLINE int MixREVERBRight(int core)
{
 if(iUseReverb==1)                                     // Neill's reverb:
  {
   int i=rvb[core].iLastRVBRight+(rvb[core].iRVBRight-rvb[core].iLastRVBRight)/2;
   rvb[core].iLastRVBRight=rvb[core].iRVBRight;
   return i;                                           // -> just return the last right reverb val (little bit scaled by the previous right val)
  }
 return 0;
}

////////////////////////////////////////////////////////////////////////

#endif

/*
-----------------------------------------------------------------------------
PSX reverb hardware notes
by Neill Corlett
-----------------------------------------------------------------------------

Yadda yadda disclaimer yadda probably not perfect yadda well it's okay anyway
yadda yadda.

-----------------------------------------------------------------------------

Basics
------

- The reverb buffer is 22khz 16-bit mono PCM.
- It starts at the reverb address given by 1DA2, extends to
  the end of sound RAM, and wraps back to the 1DA2 address.

Setting the address at 1DA2 resets the current reverb work address.

This work address ALWAYS increments every 1/22050 sec., regardless of
whether reverb is enabled (bit 7 of 1DAA set).

And the contents of the reverb buffer ALWAYS play, scaled by the
"reverberation depth left/right" volumes (1D84/1D86).
(which, by the way, appear to be scaled so 3FFF=approx. 1.0, 4000=-1.0)

-----------------------------------------------------------------------------

Register names
--------------

These are probably not their real names.
These are probably not even correct names.
We will use them anyway, because we can.

1DC0: FB_SRC_A       (offset)
1DC2: FB_SRC_B       (offset)
1DC4: IIR_ALPHA      (coef.)
1DC6: ACC_COEF_A     (coef.)
1DC8: ACC_COEF_B     (coef.)
1DCA: ACC_COEF_C     (coef.)
1DCC: ACC_COEF_D     (coef.)
1DCE: IIR_COEF       (coef.)
1DD0: FB_ALPHA       (coef.)
1DD2: FB_X           (coef.)
1DD4: IIR_DEST_A0    (offset)
1DD6: IIR_DEST_A1    (offset)
1DD8: ACC_SRC_A0     (offset)
1DDA: ACC_SRC_A1     (offset)
1DDC: ACC_SRC_B0     (offset)
1DDE: ACC_SRC_B1     (offset)
1DE0: IIR_SRC_A0     (offset)
1DE2: IIR_SRC_A1     (offset)
1DE4: IIR_DEST_B0    (offset)
1DE6: IIR_DEST_B1    (offset)
1DE8: ACC_SRC_C0     (offset)
1DEA: ACC_SRC_C1     (offset)
1DEC: ACC_SRC_D0     (offset)
1DEE: ACC_SRC_D1     (offset)
1DF0: IIR_SRC_B1     (offset)
1DF2: IIR_SRC_B0     (offset)
1DF4: MIX_DEST_A0    (offset)
1DF6: MIX_DEST_A1    (offset)
1DF8: MIX_DEST_B0    (offset)
1DFA: MIX_DEST_B1    (offset)
1DFC: IN_COEF_L      (coef.)
1DFE: IN_COEF_R      (coef.)

The coefficients are signed fractional values.
-32768 would be -1.0
 32768 would be  1.0 (if it were possible... the highest is of course 32767)

The offsets are (byte/8) offsets into the reverb buffer.
i.e. you multiply them by 8, you get byte offsets.
You can also think of them as (samples/4) offsets.
They appear to be signed.  They can be negative.
None of the documented presets make them negative, though.

Yes, 1DF0 and 1DF2 appear to be backwards.  Not a typo.

-----------------------------------------------------------------------------

What it does
------------

We take all reverb sources:
- regular channels that have the reverb bit on
- cd and external sources, if their reverb bits are on
and mix them into one stereo 44100hz signal.

Lowpass/downsample that to 22050hz.  The PSX uses a proper bandlimiting
algorithm here, but I haven't figured out the hysterically exact specifics.
I use an 8-tap filter with these coefficients, which are nice but probably
not the real ones:

0.037828187894
0.157538631280
0.321159685278
0.449322115345
0.449322115345
0.321159685278
0.157538631280
0.037828187894

So we have two input samples (INPUT_SAMPLE_L, INPUT_SAMPLE_R) every 22050hz.

* IN MY EMULATION, I divide these by 2 to make it clip less.
  (and of course the L/R output coefficients are adjusted to compensate)
  The real thing appears to not do this.

At every 22050hz tick:
- If the reverb bit is enabled (bit 7 of 1DAA), execute the reverb
  steady-state algorithm described below
- AFTERWARDS, retrieve the "wet out" L and R samples from the reverb buffer
  (This part may not be exactly right and I guessed at the coefs. TODO: check later.)
  L is: 0.333 * (buffer[MIX_DEST_A0] + buffer[MIX_DEST_B0])
  R is: 0.333 * (buffer[MIX_DEST_A1] + buffer[MIX_DEST_B1])
- Advance the current buffer position by 1 sample

The wet out L and R are then upsampled to 44100hz and played at the
"reverberation depth left/right" (1D84/1D86) volume, independent of the main
volume.

-----------------------------------------------------------------------------

Reverb steady-state
-------------------

The reverb steady-state algorithm is fairly clever, and of course by
"clever" I mean "batshit insane".

buffer[x] is relative to the current buffer position, not the beginning of
the buffer.  Note that all buffer offsets must wrap around so they're
contained within the reverb work area.

Clipping is performed at the end... maybe also sooner, but definitely at
the end.

IIR_INPUT_A0 = buffer[IIR_SRC_A0] * IIR_COEF + INPUT_SAMPLE_L * IN_COEF_L;
IIR_INPUT_A1 = buffer[IIR_SRC_A1] * IIR_COEF + INPUT_SAMPLE_R * IN_COEF_R;
IIR_INPUT_B0 = buffer[IIR_SRC_B0] * IIR_COEF + INPUT_SAMPLE_L * IN_COEF_L;
IIR_INPUT_B1 = buffer[IIR_SRC_B1] * IIR_COEF + INPUT_SAMPLE_R * IN_COEF_R;

IIR_A0 = IIR_INPUT_A0 * IIR_ALPHA + buffer[IIR_DEST_A0] * (1.0 - IIR_ALPHA);
IIR_A1 = IIR_INPUT_A1 * IIR_ALPHA + buffer[IIR_DEST_A1] * (1.0 - IIR_ALPHA);
IIR_B0 = IIR_INPUT_B0 * IIR_ALPHA + buffer[IIR_DEST_B0] * (1.0 - IIR_ALPHA);
IIR_B1 = IIR_INPUT_B1 * IIR_ALPHA + buffer[IIR_DEST_B1] * (1.0 - IIR_ALPHA);

buffer[IIR_DEST_A0 + 1sample] = IIR_A0;
buffer[IIR_DEST_A1 + 1sample] = IIR_A1;
buffer[IIR_DEST_B0 + 1sample] = IIR_B0;
buffer[IIR_DEST_B1 + 1sample] = IIR_B1;

ACC0 = buffer[ACC_SRC_A0] * ACC_COEF_A +
       buffer[ACC_SRC_B0] * ACC_COEF_B +
       buffer[ACC_SRC_C0] * ACC_COEF_C +
       buffer[ACC_SRC_D0] * ACC_COEF_D;
ACC1 = buffer[ACC_SRC_A1] * ACC_COEF_A +
       buffer[ACC_SRC_B1] * ACC_COEF_B +
       buffer[ACC_SRC_C1] * ACC_COEF_C +
       buffer[ACC_SRC_D1] * ACC_COEF_D;

FB_A0 = buffer[MIX_DEST_A0 - FB_SRC_A];
FB_A1 = buffer[MIX_DEST_A1 - FB_SRC_A];
FB_B0 = buffer[MIX_DEST_B0 - FB_SRC_B];
FB_B1 = buffer[MIX_DEST_B1 - FB_SRC_B];

buffer[MIX_DEST_A0] = ACC0 - FB_A0 * FB_ALPHA;
buffer[MIX_DEST_A1] = ACC1 - FB_A1 * FB_ALPHA;
buffer[MIX_DEST_B0] = (FB_ALPHA * ACC0) - FB_A0 * (FB_ALPHA^0x8000) - FB_B0 * FB_X;
buffer[MIX_DEST_B1] = (FB_ALPHA * ACC1) - FB_A1 * (FB_ALPHA^0x8000) - FB_B1 * FB_X;

-----------------------------------------------------------------------------
*/

