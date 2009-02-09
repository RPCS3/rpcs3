/***************************************************************************
                         registers.h  -  description
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
// - generic cleanup for the Peops release... register values by Kanodin &
//   his team
//
//*************************************************************************//

//###########################################################################

#define PS2_C0_SPUaddr_Hi    (0x1A8)
#define PS2_C0_SPUaddr_Lo    (0x1AA)
#define PS2_C1_SPUaddr_Hi    (0x5A8)
#define PS2_C1_SPUaddr_Lo    (0x5AA)
#define PS2_C0_SPUdata       (0x1AC)
#define PS2_C1_SPUdata       (0x5AC)

#define PS2_C0_SPUDMActrl    (0x1AE)
#define PS2_C1_SPUDMActrl    (0x5AE)

#define PS2_C0_SPUstat       (0x344)
#define PS2_C1_SPUstat       (0x744)
#define PS2_IRQINFO          (0x7C2)
#define PS2_C0_ReverbAddr_Hi (0x2E0)
#define PS2_C0_ReverbAddr_Lo (0x2E2)
#define PS2_C1_ReverbAddr_Hi (0x6E0)
#define PS2_C1_ReverbAddr_Lo (0x6E2)

#define PS2_C0_ReverbAEnd_Hi (0x33C)
#define PS2_C0_ReverbAEnd_Lo (0x33E)
#define PS2_C1_ReverbAEnd_Hi (0x73C)
#define PS2_C1_ReverbAEnd_Lo (0x73E)

#define PS2_C0_DryL1         (0x188)
#define PS2_C1_DryL1         (0x588)
#define PS2_C0_DryL2         (0x18A)
#define PS2_C1_DryL2         (0x58A)

#define PS2_C0_DryR1         (0x190)
#define PS2_C1_DryR1         (0x590)
#define PS2_C0_DryR2         (0x192)
#define PS2_C1_DryR2         (0x592)

#define PS2_C0_ATTR          (0x19A)
#define PS2_C1_ATTR          (0x59A)
#define PS2_C0_ADMAS         (0x1B0)
#define PS2_C1_ADMAS         (0x5B0)

#define PS2_C0_SPUirqAddr_Hi (0x19C)
#define PS2_C0_SPUirqAddr_Lo (0x19E)
#define PS2_C1_SPUirqAddr_Hi (0x59C)
#define PS2_C1_SPUirqAddr_Lo (0x59E)
#define PS2_C0_SPUrvolL      (0x764)
#define PS2_C0_SPUrvolR      (0x766)
#define PS2_C1_SPUrvolL      (0x028 + 0x764)
#define PS2_C1_SPUrvolR      (0x028 + 0x766)
#define PS2_C0_SPUon1        (0x1A0)
#define PS2_C0_SPUon2        (0x1A2)
#define PS2_C1_SPUon1        (0x5A0)
#define PS2_C1_SPUon2        (0x5A2)
#define PS2_C0_SPUoff1       (0x1A4)
#define PS2_C0_SPUoff2       (0x1A6)
#define PS2_C1_SPUoff1       (0x5A4)
#define PS2_C1_SPUoff2       (0x5A6)
#define PS2_C0_FMod1         (0x180)
#define PS2_C0_FMod2         (0x182)
#define PS2_C1_FMod1         (0x580)
#define PS2_C1_FMod2         (0x582)
#define PS2_C0_Noise1        (0x184)
#define PS2_C0_Noise2        (0x186)
#define PS2_C1_Noise1        (0x584)
#define PS2_C1_Noise2        (0x586)

#define PS2_C0_RVBon1_L      (0x18C)
#define PS2_C0_RVBon2_L      (0x18E)
#define PS2_C0_RVBon1_R      (0x194)
#define PS2_C0_RVBon2_R      (0x196)

#define PS2_C1_RVBon1_L      (0x58C)
#define PS2_C1_RVBon2_L      (0x58E)
#define PS2_C1_RVBon1_R      (0x594)
#define PS2_C1_RVBon2_R      (0x596)
#define PS2_C0_Reverb        (0x2E4)
#define PS2_C1_Reverb        (0x6E4)
#define PS2_C0_ReverbX       (0x774)
#define PS2_C1_ReverbX       (0x028 + 0x774)
#define PS2_C0_SPUend1       (0x340)
#define PS2_C0_SPUend2       (0x342)
#define PS2_C1_SPUend1       (0x740)
#define PS2_C1_SPUend2       (0x742)
#define PS2_C0_AVOLL		 (0x768) // Disabled really, but games read it
#define PS2_C0_AVOLR		 (0x76A) // In for logging purposes
#define PS2_C1_AVOLL         (0x790) // core external input volume (left) core 1
#define PS2_C1_AVOLR		 (0x792) // core external input volume (right) core 1
#define	PS2_C0_BVOLL		 (0x76C)
#define	PS2_C0_BVOLR		 (0x76E)
#define	PS2_C1_BVOLL		 (0x794)
#define	PS2_C1_BVOLR		 (0x796)
#define	PS2_C0_MMIX			 (0x198)
#define	PS2_C1_MMIX			 (0x598)

#define	PS2_C0_MVOLL		 (0x760)
#define	PS2_C0_MVOLR		 (0x762)
#define	PS2_C1_MVOLL		 (0x788)
#define	PS2_C1_MVOLR		 (0x78A)

// CORE 1 only

#define PS2_SPDIF_OUT        0x7C0                                // SPDIF Out: OFF/'PCM'/Bitstream/Bypass 
#define PS2_SPDIF_MODE       0x7C6 
#define PS2_SPDIF_MEDIA      0x7C8                                // SPDIF Media: 'CD'/DVD	
#define PS2_SPDIF_COPY       0x7CA                                // SPDIF Copy Protection

//###########################################################################

/*
 Included the info received in Regs.txt list by Neill Corlett - Kanodin

 Voice parameters:
 SD_VP_VOLL, SD_VP_VOLR   - Volume left/right per voice.  Assuming identical to PS1.
 SD_VP_PITCH              - Pitch scaler 0000-3FFF. Assuming identical to PS1.
 SD_VP_ADSR1, SD_VP_ADSR1 - Envelope data. Bitfields are documented as identical to PS1.
 SD_VP_ENVX               - Current envelope value. Assuming identical to PS1.
 SD_VP_VOLXL, SD_VP_VOLXR - Current voice volume left/right. Does not exist on the PS1.
                            Guessing that this is handy for the increase/decrease modes.

 Voice addresses:

 SD_VA_SSA                - Sample start address; assuming identical to PS1
 SD_VA_LSAX               - Loop start address; assuming identical to PS1
 SD_VA_NAX                - Seems to be documented as the current playing address.
                            Does not exist on PS1.

 Switches:

 SD_S_PMON                - Pitch mod; assuming identical to PS1
 SD_S_NON                 - Noise; assuming identical to PS1
 SD_S_VMIXL, SD_S_VMIXR   - Voice mix L/R. Guessing this is just a separate L/R version 
                            of the "voice enable" bits on the PS1.
 SD_S_VMIXEL, SD_S_VMIXER - Voice effect mix L/R. Guessing this is just a separate L/R
                            version of the "voice reverb enable" bits on the PS1.
 SD_S_KON, SD_S_KOFF      - Key on/off; assuming identical to PS1


 Addresses:

 SD_A_TSA                 - Transfer start address; assuming identical to PS1
 SD_A_ESA                 - Effect start address - this is probably analogous to the
                            PS1's reverb work area start address
 SD_A_EEA                 - Effect end address - this would've been fixed to 0x7FFFF on
                            the PS1; settable in 128K increments on the PS2.
 SD_A_IRQA                - IRQ address; assuming identical to PS1

 Volume parameters:

 SD_P_MVOLL, SD_P_MVOLR   - Master volume L/R; assuming identical to PS1
 SD_P_EVOLL, SD_P_EVOLR   - Effect volume L/R; assuming analogous to RVOL on the PS1
 SD_P_AVOLL, SD_P_AVOLR   - External input volume L/R
                            This is probably where CORE0 connects to CORE1
 SD_P_BVOLL, SD_P_BVOLR   - Sound data input volume - perhaps this is the volume of
                            the raw PCM auto-DMA input? analogous to CD input volume?
 SD_P_MVOLXL, SD_P_MVOLXR - Current master volume L/R; seems self-explanatory

 SD_P_MMIX                - Mixer / effect enable bits.
                             bit 11 = MSNDL  = voice output dry L
                                 10 = MSNDR  = voice output dry R
                                  9 = MSNDEL = voice output wet L
                                  8 = MSNDER = voice output wet R
                                  7 = MINL   = sound data input dry L
                                  6 = MINR   = sound data input dry R
                                  5 = MINEL  = sound data input wet L
                                  4 = MINER  = sound data input wet R
                                  3 = SINL   = core external input dry L
                                  2 = SINR   = core external input dry R
                                  1 = SINEL  = core external input wet L
                                  0 = SINER  = core external input wet R

Core attributes (SD_C)

    bit 4..5      - DMA related
    bit 6         - IRQ enable
    bit 7         - effect enable (reverb enable)
    bit 13..8     - noise clock
    bit 14        - mute

 - if you READ the two DMA related bits, if either are set, the channel is
   considered "busy" by sceSdVoiceTrans



Reverb parameters:

  Same as PS1 reverb (I used the names from my reverb doc).


Other PS2 IOP notes

 There's two DMA controllers:
 The original one at 1F801080-1F8010FF (channels 0-6)
 A new one at        1F801500-1F80157F (channels 7-13)

 They appear to function the same way - 7 channels each.

 SPU CORE0's DMA channel is 4 as per usual
 SPU CORE1's DMA channel is 7

DMA channel 10 is SIF
 
 Original INTR controller at 1F801000-1F80107F

 All interrupt handling seems to be done using the old INTR, but
 with some new bits defined:



 Reading from 1F801078 masks interrupts and returns 1 if they weren't
 masked before.  Writing 1 to 1F801078 re-enables interrupts.
 Writing 0 doesn't.  Maybe it was like that on the original PS1 too.

Six root counters:

 RTC#   address      sources           size     prescale     interrupt#
0      0x1F801100   sysclock,pixel    16 bit   1 only       4
1      0x1F801110   sysclock,hline    16 bit   1 only       5
2      0x1F801120   sysclock          16 bit   1,8          6
3      0x1F801480   sysclock,hline    32 bit   1 only       14
4      0x1F801490   sysclock          32 bit   1,8,16,256   15
5      0x1F8014A0   sysclock          32 bit   1,8,16,256   16

Count (0x0) and Compare (0x8) registers work as before, only with more bits
in the new counters.

Mode (0x4) works like this when written:

  bits 0..2    gate
  bit  3       reset on target
  bit  4       target interrupt enable
  bit  5       overflow interrupt enable
  bit  6       master enable (?)
  bit  7       ?
  bit  8       clock select
  bit  9       prescale (OLD)
  bit  10..12  ?
  bit  13..14  prescale (NEW)
  bit  15      ? always set to 1

Gate:
   TM_NO_GATE                   000
   TM_GATE_ON_Count             001
   TM_GATE_ON_ClearStart        011
   TM_GATE_ON_Clear_OFF_Start   101
   TM_GATE_ON_Start             111

   V-blank  ----+    +----------------------------+    +------
                |    |                            |    |
                |    |                            |    |
                +----+                            +----+
 TM_NO_GATE:

                0================================>============

 TM_GATE_ON_Count:

                <---->0==========================><---->0=====

 TM_GATE_ON_ClearStart:

                0====>0================================>0=====

 TM_GATE_ON_Clear_OFF_Start:

                0====><-------------------------->0====><-----

 TM_GATE_ON_Start:

                <---->0==========================>============

  reset on target: if set, counter resets to 0 when Compare value is reached

  target interrupt enable: if set, interrupt when Compare value is reached
  overflow interrupt enable: if set, interrupt when counter overflows

  master enable: if this bit is clear, the timer should do nothing.

  clock select: for counters 0, 1, and 3, setting this will select the alternate
  counter (pixel or hline)

  prescale (OLD): for counter 2 only. set this to prescale (divide) by 8.

  prescale (NEW): for counters 4 and 5 only:

  00 = prescale by 1
  01 = prescale by 8
  10 = prescale by 16
  11 = prescale by 256

Writing 0x4 also clears the counter. (I think.)

When 0x4 is read, it becomes Status:

  bit  0..10   ?
  bit  11      compare value was reached
  bit  12      count overflowed
  bit  13..15  ?

Reading probably clears these bits.



 1F8014B0 (word) - timer-related but otherwise unknown
 1F8014C0 (word) - timer-related but otherwise unknown


 don't currently know how the interrupts work for DMA ch7 yet

 1F801060 (word) - address of some kind.

 1F801450 (word) -
    if bit 3 is SET, we're in PS1 mode.
    if bit 3 is CLEAR, we're in PS2 IOP mode.

 1F802070 (byte) - unknown. status byte of some kind? visible to EE?

 1D000000-1D00007F (?) - SIF related

 1D000020 (word) - read counter of some sort?
                   sceSifInit waits for bit 0x10000 of this to be set.
 1D000030 (word) - read counter of some sort?
 1D000040 (word) - read bits 0x20, 0x40 mean something
 1D000060 (word) - used to detect whether the SIF interface exists
                   read must be 0x1D000060, or the top 20 bits must be zero 
*/

/*

// DirectX Audio SPU2 Driver for PCSX2
// audio.c by J.F. and Kanodin (hooper1@cox.net)
//
// Copyright 2003 J.F. and Kanodin, and distributed under the
// terms of the GNU General Public License, v2 or later.
// http://www.gnu.org/copyleft/gpl.html.

Included these just in case you need them J.F. - Kanodin

// Core Start Addresses
#define CORE0 0x1f900000
#define CORE1 0x1f900400


   #define IOP_INT_VBLANK  (1<<0)
   #define IOP_INT_GM      (1<<1)
   #define IOP_INT_CDROM   (1<<2)
   #define IOP_INT_DMA     (1<<3)
   #define IOP_INT_RTC0    (1<<4)
   #define IOP_INT_RTC1    (1<<5)
   #define IOP_INT_RTC2    (1<<6)
   #define IOP_INT_SIO0    (1<<7)
   #define IOP_INT_SIO1    (1<<8)
   #define IOP_INT_SPU     (1<<9)
   #define IOP_INT_PIO     (1<<10)
   #define IOP_INT_EVBLANK (1<<11)
   #define IOP_INT_DVD     (1<<12)
   #define IOP_INT_PCMCIA  (1<<13)
   #define IOP_INT_RTC3    (1<<14)
   #define IOP_INT_RTC4    (1<<15)
   #define IOP_INT_RTC5    (1<<16)
   #define IOP_INT_SIO2    (1<<17)
   #define IOP_INT_HTR0    (1<<18)
   #define IOP_INT_HTR1    (1<<19)
   #define IOP_INT_HTR2    (1<<20)
   #define IOP_INT_HTR3    (1<<21)
   #define IOP_INT_USB     (1<<22)
   #define IOP_INT_EXTR    (1<<23)
   #define IOP_INT_FWRE    (1<<24)
   #define IOP_INT_FDMA    (1<<25)    

// CORE0 => +0x000, CORE1 => +0x400

// individual voice parameter regs

#define VP_VOLL(cr, vc)  (0x400 * cr + (vc << 4))    // voice volume (left)
#define VP_VOLR(cr, vc)  (0x400 * cr + 0x002 + (vc << 4))    // voice volume (right)
#define VP_PITCH(cr, vc) (0x400 * cr + 0x004 + (vc << 4))    // voice pitch
#define VP_ADSR1(cr, vc) (0x400 * cr + 0x006 + (vc << 4))    // voice envelope (AR, DR, SL)
#define VP_ADSR2(cr, vc) (0x400 * cr + 0x008 + (vc << 4))    // voice envelope (SR, RR)
#define VP_ENVX(cr, vc)  (0x400 * cr + 0x00A + (vc << 4))    // voice envelope (current value)
#define VP_VOLXL(cr, vc) (0x400 * cr + 0x00C + (vc << 4))    // voice volume (current value left)
#define VP_VOLXR(cr, vc) (0x400 * cr + 0x00E + (vc << 4))    // voice volume (current value right)

#define VA_SSA(cr, vc)   (0x400 * cr + 0x1C0 + (vc * 12))    // voice waveform data start address
#define VA_LSAX(cr, vc)  (0x400 * cr + 0x1C4 + (vc * 12))    // voice waveform data loop address
#define VA_NAX(cr, vc)   (0x400 * cr + 0x1C8 + (vc * 12))    // voice waveform data next address

// common settings

#define S_PMON(cr)       (0x400 * cr + 0x180)                // pitch modulation on
#define S_NON(cr)        (0x400 * cr + 0x184)                // noise generator on
#define S_VMIXL(cr)      (0x400 * cr + 0x188)                // voice output mixing (dry left)
#define S_VMIXEL(cr)     (0x400 * cr + 0x18C)                // voice output mixing (wet left)
#define S_VMIXR(cr)      (0x400 * cr + 0x190)                // voice output mixing (dry right)
#define S_VMIXER(cr)     (0x400 * cr + 0x194)                // voice output mixing (wet right)
#define P_MMIX(cr)       (0x400 * cr + 0x198)                // output type after voice mixing (See paragraph below)
#define P_ATTR(cr)       (0x400 * cr + 0x19A)                // core attributes (See paragraph below)
#define A_IRQA(cr)       (0x400 * cr + 0x19C)                // IRQ address 
#define S_KON(cr)        (0x400 * cr + 0x1A0)                // key on (start voice sound generation)
#define S_KOFF(cr)       (0x400 * cr + 0x1A4)                // key off (end voice sound generation)
#define A_TSA(cr)        (0x400 * cr + 0x1A8)                // DMA transfer start address
#define P_DATA(cr)       (0x400 * cr + 0x1AC)                // DMA data register
#define P_CTRL(cr)       (0x400 * cr + 0x1AE)                // DMA control register
#define P_ADMAS(cr)      (0x400 * cr + 0x1B0)                // AutoDMA status

#define A_ESA(cr)        (0x400 * cr + 0x2E0)                // effects work area start address

#define FB_SRC_A(cr)     (0x400 * cr + 0x2E4)      
#define FB_SRC_B(cr)     (0x400 * cr + 0x2E8)      
#define IIR_DEST_A0(cr)  (0x400 * cr + 0x2EC)      
#define IIR_DEST_A1(cr)  (0x400 * cr + 0x2F0)      
#define ACC_SRC_A0(cr)   (0x400 * cr + 0x2F4)     
#define ACC_SRC_A1(cr)   (0x400 * cr + 0x2F8)
#define ACC_SRC_B0(cr)   (0x400 * cr + 0x2FC)  

#define ACC_SRC_B1(cr)   (0x400 * cr + 0x300)      
#define IIR_SRC_A0(cr)   (0x400 * cr + 0x304)      
#define IIR_SRC_A1(cr)   (0x400 * cr + 0x308)      
#define IIR_DEST_B0(cr)  (0x400 * cr + 0x30C)      
#define IIR_DEST_B1(cr)  (0x400 * cr + 0x310)      
#define ACC_SRC_C0(cr)   (0x400 * cr + 0x314)      
#define ACC_SRC_C1(cr)   (0x400 * cr + 0x318)  

#define ACC_SRC_D0(cr)   (0x400 * cr + 0x31C)      
#define ACC_SRC_D1(cr)   (0x400 * cr + 0x320)      
#define IIR_SRC_B1(cr)   (0x400 * cr + 0x324)      
#define IIR_SRC_B0(cr)   (0x400 * cr + 0x328)      
#define MIX_DEST_A0(cr)  (0x400 * cr + 0x32C)      
#define MIX_DEST_A1(cr)  (0x400 * cr + 0x330)      
#define MIX_DEST_B0(cr)  (0x400 * cr + 0x334)      
#define MIX_DEST_B1(cr)  (0x400 * cr + 0x338)      

#define A_EEA(cr)        (0x400 * cr + 0x33C)                // effects work area end address

#define P_ENDX(cr)       (0x400 * cr + 0x340)                // voice loop end status
#define P_STAT(cr)       (0x400 * cr + 0x344)                // DMA status register
#define P_ENDS(cr)       (0x400 * cr + 0x346)                // ?

// CORE0 => +0x400, CORE1 => +0x428

#define P_MVOLL(cr)      (0x28 * cr + 0x760)                 // master volume (left)
#define P_MVOLR(cr)      (0x28 * cr + 0x762)                 // master volume (right)
#define P_EVOLL(cr)      (0x28 * cr + 0x764)                 // effect return volume (left)
#define P_EVOLR(cr)      (0x28 * cr + 0x766)                 // effect return volume (right)
#define P_AVOLL(cr)      (0x28 * cr + 0x768)                 // core external input volume (left)
#define P_AVOLR(cr)      (0x28 * cr + 0x76A)                 // core external input volume (right)
#define P_BVOLL(cr)      (0x28 * cr + 0x76C)                 // sound data input volume (left)
#define P_BVOLR(cr)      (0x28 * cr + 0x76E)                 // sound data input volume (right)
#define P_MVOLXL(cr)     (0x28 * cr + 0x770)                 // current master volume (left)
#define P_MVOLXR(cr)     (0x28 * cr + 0x772)                 // current master volume (right)

#define IIR_ALPHA(cr)    (0x28 * cr + 0x774)      
#define ACC_COEF_A(cr)   (0x28 * cr + 0x776)      
#define ACC_COEF_B(cr)   (0x28 * cr + 0x778)      
#define ACC_COEF_C(cr)   (0x28 * cr + 0x77A)      
#define ACC_COEF_D(cr)   (0x28 * cr + 0x77C)      
#define IIR_COEF(cr)     (0x28 * cr + 0x77E)      
#define FB_ALPHA(cr)     (0x28 * cr + 0x780)      
#define FB_X(cr)         (0x28 * cr + 0x782)      
#define IN_COEF_L(cr)    (0x28 * cr + 0x784)      
#define IN_COEF_R(cr)    (0x28 * cr + 0x786)      

// CORE1 only => +0x400

#define SPDIF_OUT        0x7C0                               // SPDIF Out: OFF/'PCM'/Bitstream/Bypass 
#define SPDIF_MODE       0x7C6
#define SPDIF_MEDIA      0x7C8                               // SPDIF Media: 'CD'/DVD	
#define SPDIF_COPY       0x7CA                               // SPDIF Copy Protection

// PS1 SPU CORE

// individual voice settings

#define SPU_VP_PITCH(vc) (0xC04 + (vc << 4))                 // voice pitch
#define SPU_VA_SSA(vc)   (0xC06 + (vc << 4))                 // voice waveform data start address
#define SPU_VP_ADSR(vc)  (0xC08 + (vc << 4))                 // voice envelope
#define SPU_VA_SSA(vc)   (0xC0E + (vc << 4))                 // voice waveform data loop address

// common settings

#define SPU_P_MVOLL      0xD80                               // master volume (left)
#define SPU_P_MVOLR      0xD82                               // master volume (right)
#define SPU_P_RVOLL      0xD84                               // effect return volume (left)
#define SPU_P_RVOLR      0xD86                               // effect return volume (right)
#define SPU_S_KON1       0xD88                               // key on
#define SPU_S_KON2       0xD8A                               // 
#define SPU_S_KOFF1      0xD8C                               // key off
#define SPU_S_KOFF2      0xD8E                               // 
#define SPU_S_PMON1      0xD90                               // pitch modulation on
#define SPU_S_PMON2      0xD92                               // 
#define SPU_S_NON1       0xD94                               // noise generator on
#define SPU_S_NON2       0xD96                               // 
#define SPU_S_RVBON1     0xD98                               // effects on
#define SPU_S_RVBON2     0xD9A                               // 
#define SPU_S_MUTE1      0xD9C                               // voice mute
#define SPU_S_MUTE2      0xD9E                               // 

#define SPU_A_ESA        0xDA2                               // effects work area start
#define SPU_A_IRQA       0xDA4                               // IRQ address
#define SPU_A_TSA        0xDA6                               // DMA transfer start address
#define SPU_P_DATA       0xDA8                               // DMA data register
#define SPU_P_CTRL       0xDAA                               // DMA control register
#define SPU_P_STAT       0xDAE                               // DMA status register

#define SPU_P_CDL        0xDB0                               // sound data input volume (left)
#define SPU_P_CDR        0xDB2                               // sound data input volume (right)
#define SPU_P_EXTL       0xDB4                               // external input volume (left)
#define SPU_P_EXTR       0xDB6                               // external input volume (right)

#define SPU_P_REVERB     0xDC0                               // effects control


// Individual voice parameter regs CORE 0
// Only


#define VP_VOLL(cr, vc)  (0x400 * cr + (vc << 4))    // voice volume (left)
#define VP_VOLR(cr, vc)  (0x400 * cr + 0x002 + (vc << 4))    // voice volume (right)
#define VP_PITCH(cr, vc) (0x400 * cr + 0x004 + (vc << 4))    // voice pitch
#define VP_ADSR1(cr, vc) (0x400 * cr + 0x006 + (vc << 4))    // voice envelope (AR, DR, SL)
#define VP_ADSR2(cr, vc) (0x400 * cr + 0x008 + (vc << 4))    // voice envelope (SR, RR)
#define VP_ENVX(cr, vc)  (0x400 * cr + 0x00A + (vc << 4))    // voice envelope (current value)
#define VP_VOLXL(cr, vc) (0x400 * cr + 0x00C + (vc << 4))    // voice volume (current value left)
#define VP_VOLXR(cr, vc) (0x400 * cr + 0x00E + (vc << 4))    // voice volume (current value right)

#define VA_SSA(cr, vc)   (0x400 * cr + 0x1C0 + (vc * 12))    // voice waveform data start address
#define VA_LSAX(cr, vc)  (0x400 * cr + 0x1C4 + (vc * 12))    // voice waveform data loop address
#define VA_NAX(cr, vc)   (0x400 * cr + 0x1C8 + (vc * 12))    // voice waveform data next address


// CORE 0 Common Settings


#define S_PMON(cr)       (0x400 * cr + 0x180)                // pitch modulation on
#define S_NON(cr)        (0x400 * cr + 0x184)                // noise generator on
#define S_VMIXL(cr)      (0x400 * cr + 0x188)                // voice output mixing (dry left)
#define S_VMIXEL(cr)     (0x400 * cr + 0x18C)                // voice output mixing (wet left)
#define S_VMIXR(cr)      (0x400 * cr + 0x190)                // voice output mixing (dry right)
#define S_VMIXER(cr)     (0x400 * cr + 0x194)                // voice output mixing (wet right)
#define P_MMIX(cr)       (0x400 * cr + 0x198)                // output type after voice mixing (See paragraph below)
#define P_ATTR(cr)       (0x400 * cr + 0x19A)                // core attributes (See paragraph below)
#define A_IRQA(cr)       (0x400 * cr + 0x19C)                // IRQ address 
#define S_KON(cr)        (0x400 * cr + 0x1A0)                // key on (start voice sound generation)
#define S_KOFF(cr)       (0x400 * cr + 0x1A4)                // key off (end voice sound generation)
#define A_TSA(cr)        (0x400 * cr + 0x1A8)                // DMA transfer start address
#define P_DATA(cr)       (0x400 * cr + 0x1AC)                // DMA data register
#define P_CTRL(cr)       (0x400 * cr + 0x1AE)                // DMA control register
#define P_ADMAS(cr)      (0x400 * cr + 0x1B0)                // AutoDMA status

#define A_ESA(cr)        (0x400 * cr + 0x2E0)                // effects work area start address


// Core 0 Reverb Addresses


#define FB_SRC_A(cr)     (0x400 * cr + 0x2E4)      
#define FB_SRC_B(cr)     (0x400 * cr + 0x2E8)      
#define IIR_DEST_A0(cr)  (0x400 * cr + 0x2EC)      
#define IIR_DEST_A1(cr)  (0x400 * cr + 0x2F0)      
#define ACC_SRC_A0(cr)   (0x400 * cr + 0x2F4)     
#define ACC_SRC_A1(cr)   (0x400 * cr + 0x2F8)
#define ACC_SRC_B0(cr)   (0x400 * cr + 0x2FC)  

#define ACC_SRC_B1(cr)   (0x400 * cr + 0x300)      
#define IIR_SRC_A0(cr)   (0x400 * cr + 0x304)      
#define IIR_SRC_A1(cr)   (0x400 * cr + 0x308)      
#define IIR_DEST_B0(cr)  (0x400 * cr + 0x30C)      
#define IIR_DEST_B1(cr)  (0x400 * cr + 0x310)      
#define ACC_SRC_C0(cr)   (0x400 * cr + 0x314)      
#define ACC_SRC_C1(cr)   (0x400 * cr + 0x318)  

#define ACC_SRC_D0(cr)   (0x400 * cr + 0x31C)      
#define ACC_SRC_D1(cr)   (0x400 * cr + 0x320)      
#define IIR_SRC_B1(cr)   (0x400 * cr + 0x324)      
#define IIR_SRC_B0(cr)   (0x400 * cr + 0x328)      
#define MIX_DEST_A0(cr)  (0x400 * cr + 0x32C)      
#define MIX_DEST_A1(cr)  (0x400 * cr + 0x330)      
#define MIX_DEST_B0(cr)  (0x400 * cr + 0x334)      
#define MIX_DEST_B1(cr)  (0x400 * cr + 0x338)      

#define A_EEA(cr)        (0x400 * cr + 0x33C)                // effects work area end address

#define P_ENDX(cr)       (0x400 * cr + 0x340)                // voice loop end status
#define P_STAT(cr)       (0x400 * cr + 0x344)                // DMA status register
#define P_ENDS(cr)       (0x400 * cr + 0x346)                // ?


// CORE 0 Specific


#define P_MVOLL(cr)      (0x28 * cr + 0x760)                 // master volume (left)
#define P_MVOLR(cr)      (0x28 * cr + 0x762)                 // master volume (right)
#define P_EVOLL(cr)      (0x28 * cr + 0x764)                 // effect return volume (left)
#define P_EVOLR(cr)      (0x28 * cr + 0x766)                 // effect return volume (right)
#define P_AVOLL(cr)      (0x28 * cr + 0x768)                 // core external input volume (left)
#define P_AVOLR(cr)      (0x28 * cr + 0x76A)                 // core external input volume (right)
#define P_BVOLL(cr)      (0x28 * cr + 0x76C)                 // sound data input volume (left)
#define P_BVOLR(cr)      (0x28 * cr + 0x76E)                 // sound data input volume (right)
#define P_MVOLXL(cr)     (0x28 * cr + 0x770)                 // current master volume (left)
#define P_MVOLXR(cr)     (0x28 * cr + 0x772)                 // current master volume (right)


// More CORE 0 Reverb


#define IIR_ALPHA(cr)    (0x28 * cr + 0x774)      
#define ACC_COEF_A(cr)   (0x28 * cr + 0x776)      
#define ACC_COEF_B(cr)   (0x28 * cr + 0x778)      
#define ACC_COEF_C(cr)   (0x28 * cr + 0x77A)      
#define ACC_COEF_D(cr)   (0x28 * cr + 0x77C)      
#define IIR_COEF(cr)     (0x28 * cr + 0x77E)      
#define FB_ALPHA(cr)     (0x28 * cr + 0x780)      
#define FB_X(cr)         (0x28 * cr + 0x782)      
#define IN_COEF_L(cr)    (0x28 * cr + 0x784)      
#define IN_COEF_R(cr)    (0x28 * cr + 0x786) 


// CORE 1 only

#define SPDIF_OUT        0x7C0                                // SPDIF Out: OFF/'PCM'/Bitstream/Bypass 
#define SPDIF_MODE       0x7C6 
#define SPDIF_MEDIA      0x7C8                                // SPDIF Media: 'CD'/DVD	
#define SPDIF_COPY       0x7CA                                // SPDIF Copy Protection
*/

/* PS1 SPU CORE

*** The below really isn't needed, only if you ***
*** want to add SPU support to the plugin      ***
*** which I see no need to add at this time.   ***
*** individual voice settings                  ***

#define SPU_VP_PITCH(vc) (0xC04 + (vc << 4))                 // voice pitch
#define SPU_VA_SSA(vc)   (0xC06 + (vc << 4))                 // voice waveform data start address
#define SPU_VP_ADSR(vc)  (0xC08 + (vc << 4))                 // voice envelope
#define SPU_VA_SSA(vc)   (0xC0E + (vc << 4))                 // voice waveform data loop address

// common settings

#define SPU_P_MVOLL      0xD80                               // master volume (left)
#define SPU_P_MVOLR      0xD82                               // master volume (right)
#define SPU_P_RVOLL      0xD84                               // effect return volume (left)
#define SPU_P_RVOLR      0xD86                               // effect return volume (right)
#define SPU_S_KON1       0xD88                               // key on
#define SPU_S_KON2       0xD8A                               // 
#define SPU_S_KOFF1      0xD8C                               // key off
#define SPU_S_KOFF2      0xD8E                               // 
#define SPU_S_PMON1      0xD90                               // pitch modulation on
#define SPU_S_PMON2      0xD92                               // 
#define SPU_S_NON1       0xD94                               // noise generator on
#define SPU_S_NON2       0xD96                               // 
#define SPU_S_RVBON1     0xD98                               // effects on
#define SPU_S_RVBON2     0xD9A                               // 
#define SPU_S_MUTE1      0xD9C                               // voice mute
#define SPU_S_MUTE2      0xD9E                               // 

#define SPU_A_ESA        0xDA2                               // effects work area start
#define SPU_A_IRQA       0xDA4                               // IRQ address
#define SPU_A_TSA        0xDA6                               // DMA transfer start address
#define SPU_P_DATA       0xDA8                               // DMA data register
#define SPU_P_CTRL       0xDAA                               // DMA control register
#define SPU_P_STAT       0xDAE                               // DMA status register

#define SPU_P_CDL        0xDB0                               // sound data input volume (left)
#define SPU_P_CDR        0xDB2                               // sound data input volume (right)
#define SPU_P_EXTL       0xDB4                               // external input volume (left)
#define SPU_P_EXTR       0xDB6                               // external input volume (right)

#define SPU_P_REVERB     0xDC0                               // effects control
*/

/*
#define H_SPUReverbAddr  0x0da2
#define H_SPUirqAddr     0x0da4
#define H_SPUaddr        0x0da6
#define H_SPUdata        0x0da8
#define H_SPUctrl        0x0daa
#define H_SPUstat        0x0dae
#define H_SPUmvolL       0x0d80
#define H_SPUmvolR       0x0d82
#define H_SPUrvolL       0x0d84
#define H_SPUrvolR       0x0d86
#define H_SPUon1         0x0d88
#define H_SPUon2         0x0d8a
#define H_SPUoff1        0x0d8c
#define H_SPUoff2        0x0d8e
#define H_FMod1          0x0d90
#define H_FMod2          0x0d92
#define H_Noise1         0x0d94
#define H_Noise2         0x0d96
#define H_RVBon1         0x0d98
#define H_RVBon2         0x0d9a
#define H_SPUMute1       0x0d9c
#define H_SPUMute2       0x0d9e
#define H_CDLeft         0x0db0
#define H_CDRight        0x0db2
#define H_ExtLeft        0x0db4
#define H_ExtRight       0x0db6
#define H_Reverb         0x0dc0
#define H_SPUPitch0      0x0c04
#define H_SPUPitch1      0x0c14
#define H_SPUPitch2      0x0c24
#define H_SPUPitch3      0x0c34
#define H_SPUPitch4      0x0c44
#define H_SPUPitch5      0x0c54
#define H_SPUPitch6      0x0c64
#define H_SPUPitch7      0x0c74
#define H_SPUPitch8      0x0c84
#define H_SPUPitch9      0x0c94
#define H_SPUPitch10     0x0ca4
#define H_SPUPitch11     0x0cb4
#define H_SPUPitch12     0x0cc4
#define H_SPUPitch13     0x0cd4
#define H_SPUPitch14     0x0ce4
#define H_SPUPitch15     0x0cf4
#define H_SPUPitch16     0x0d04
#define H_SPUPitch17     0x0d14
#define H_SPUPitch18     0x0d24
#define H_SPUPitch19     0x0d34
#define H_SPUPitch20     0x0d44
#define H_SPUPitch21     0x0d54
#define H_SPUPitch22     0x0d64
#define H_SPUPitch23     0x0d74

#define H_SPUStartAdr0   0x0c06
#define H_SPUStartAdr1   0x0c16
#define H_SPUStartAdr2   0x0c26
#define H_SPUStartAdr3   0x0c36
#define H_SPUStartAdr4   0x0c46
#define H_SPUStartAdr5   0x0c56
#define H_SPUStartAdr6   0x0c66
#define H_SPUStartAdr7   0x0c76
#define H_SPUStartAdr8   0x0c86
#define H_SPUStartAdr9   0x0c96
#define H_SPUStartAdr10  0x0ca6
#define H_SPUStartAdr11  0x0cb6
#define H_SPUStartAdr12  0x0cc6
#define H_SPUStartAdr13  0x0cd6
#define H_SPUStartAdr14  0x0ce6
#define H_SPUStartAdr15  0x0cf6
#define H_SPUStartAdr16  0x0d06
#define H_SPUStartAdr17  0x0d16
#define H_SPUStartAdr18  0x0d26
#define H_SPUStartAdr19  0x0d36
#define H_SPUStartAdr20  0x0d46
#define H_SPUStartAdr21  0x0d56
#define H_SPUStartAdr22  0x0d66
#define H_SPUStartAdr23  0x0d76

#define H_SPULoopAdr0   0x0c0e
#define H_SPULoopAdr1   0x0c1e
#define H_SPULoopAdr2   0x0c2e
#define H_SPULoopAdr3   0x0c3e
#define H_SPULoopAdr4   0x0c4e
#define H_SPULoopAdr5   0x0c5e
#define H_SPULoopAdr6   0x0c6e
#define H_SPULoopAdr7   0x0c7e
#define H_SPULoopAdr8   0x0c8e
#define H_SPULoopAdr9   0x0c9e
#define H_SPULoopAdr10  0x0cae
#define H_SPULoopAdr11  0x0cbe
#define H_SPULoopAdr12  0x0cce
#define H_SPULoopAdr13  0x0cde
#define H_SPULoopAdr14  0x0cee
#define H_SPULoopAdr15  0x0cfe
#define H_SPULoopAdr16  0x0d0e
#define H_SPULoopAdr17  0x0d1e
#define H_SPULoopAdr18  0x0d2e
#define H_SPULoopAdr19  0x0d3e
#define H_SPULoopAdr20  0x0d4e
#define H_SPULoopAdr21  0x0d5e
#define H_SPULoopAdr22  0x0d6e
#define H_SPULoopAdr23  0x0d7e

#define H_SPU_ADSRLevel0   0x0c08
#define H_SPU_ADSRLevel1   0x0c18
#define H_SPU_ADSRLevel2   0x0c28
#define H_SPU_ADSRLevel3   0x0c38
#define H_SPU_ADSRLevel4   0x0c48
#define H_SPU_ADSRLevel5   0x0c58
#define H_SPU_ADSRLevel6   0x0c68
#define H_SPU_ADSRLevel7   0x0c78
#define H_SPU_ADSRLevel8   0x0c88
#define H_SPU_ADSRLevel9   0x0c98
#define H_SPU_ADSRLevel10  0x0ca8
#define H_SPU_ADSRLevel11  0x0cb8
#define H_SPU_ADSRLevel12  0x0cc8
#define H_SPU_ADSRLevel13  0x0cd8
#define H_SPU_ADSRLevel14  0x0ce8
#define H_SPU_ADSRLevel15  0x0cf8
#define H_SPU_ADSRLevel16  0x0d08
#define H_SPU_ADSRLevel17  0x0d18
#define H_SPU_ADSRLevel18  0x0d28
#define H_SPU_ADSRLevel19  0x0d38
#define H_SPU_ADSRLevel20  0x0d48
#define H_SPU_ADSRLevel21  0x0d58
#define H_SPU_ADSRLevel22  0x0d68
#define H_SPU_ADSRLevel23  0x0d78
*/
