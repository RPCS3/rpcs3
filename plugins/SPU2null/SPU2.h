/*  SPU2null
 *  Copyright (C) 2002-2005  SPU2null Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __SPU2_H__
#define __SPU2_H__

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <string.h>

extern "C"
{
#define SPU2defs
#include "PS2Edefs.h"
}
#include "PS2Eext.h"

#ifdef _MSC_VER
#include <windows.h>
#include <windowsx.h>
#endif

#ifdef _MSC_VER
#define EXPORT_C_(type) extern "C" __declspec(dllexport) type CALLBACK
#else
#define EXPORT_C_(type) extern "C" type
#endif

extern FILE *spu2Log;
#define SPU2_LOG __Log  //debug mode

extern const u8 version;
extern const u8 revision;
extern const u8 build;
extern const u32 minor;
extern char *libraryName;

typedef struct
{
	s32 Log;
} Config;

extern Config conf;

void __Log(char *fmt, ...);
void SaveConfig();
void LoadConfig();
void SysMessage(char *fmt, ...);

////////////////////
// SPU2 Registers //
////////////////////
#define REG_VP_VOLL      0x0000
#define REG_VP_VOLR      0x0002
#define REG_VP_PITCH     0x0004
#define REG_VP_ADSR1     0x0006
#define REG_VP_ADSR2     0x0008
#define REG_VP_ENVX      0x000A
#define REG_VP_VOLXL     0x000C
#define REG_VP_VOLXR     0x000E
#define REG_C0_FMOD1       0x0180
#define REG_C0_FMOD2       0x0182
#define REG_C1_FMOD1       0x0580
#define REG_C1_FMOD2       0x0582
#define REG_S_NON        0x0184
#define REG_S_VMIXL      0x0188
#define REG_S_VMIXEL     0x018C
#define REG_S_VMIXR      0x0190
#define REG_S_VMIXER     0x0194
#define REG_C0_MMIX       0x0198
#define REG_C1_MMIX       0x0598
#define REG_C0_CTRL       0x019A
#define REG_C0_IRQA_HI   0x019C
#define REG_C0_IRQA_LO   0x019D
#define REG_C1_IRQA_HI   0x059C
#define REG_C1_IRQA_LO   0x059D
#define REG_C0_SPUON1    0x1A0
#define REG_C0_SPUON2    0x1A2
#define REG_C1_SPUON1    0x5A0
#define REG_C1_SPUON2    0x5A2
#define REG_C0_SPUOFF1   0x1A4
#define REG_C0_SPUOFF2   0x1A6
#define REG_C1_SPUOFF1   0x5A4
#define REG_C1_SPUOFF2   0x5A6
#define REG_C0_SPUADDR_HI 0x01A8
#define REG_C0_SPUADDR_LO 0x01AA
#define REG_C1_SPUADDR_HI 0x05A8
#define REG_C1_SPUADDR_LO 0x05AA
#define REG_C0_SPUDATA   0x01AC
#define REG_C1_SPUDATA   0x05AC
#define REG_C0_DMACTRL   0x01AE
#define REG_C1_DMACTRL   0x05AE
#define REG_C0_ADMAS     0x01B0
#define REG_VA_SSA       0x01C0
#define REG_VA_LSAX      0x01C4
#define REG_VA_NAX       0x01C8
#define REG_A_ESA        0x02E0
#define REG_A_EEA        0x033C
#define REG_C0_END1       0x0340
#define REG_C0_END2       0x0342
#define REG_C1_END1       0x0740
#define REG_C1_END2       0x0742
#define REG_C0_SPUSTAT  0x0344 	//not sure!
#define REG_C1_CTRL       0x059A
#define REG_C1_ADMAS     0x05B0
#define REG_C1_SPUSTAT   0x0744 	//not sure!
#define REG_P_MVOLL      0x0760
#define REG_P_MVOLR      0x0762
#define REG_P_EVOLL      0x0764
#define REG_P_EVOLR      0x0766
#define REG_P_AVOLL      0x0768
#define REG_P_AVOLR      0x076A
#define REG_P_BVOLL      0x076C
#define REG_P_BVOLR      0x076E
#define REG_P_MVOLXL     0x0770
#define REG_P_MVOLXR     0x0772
#define SPDIF_OUT        0x07C0
#define REG_IRQINFO          0x07C2
#define SPDIF_MODE       0x07C6
#define SPDIF_MEDIA      0x07C8

#define spu2Rs16(mem)	(*(s16*)&spu2regs[(mem) & 0xffff])
#define spu2Ru16(mem)	(*(u16*)&spu2regs[(mem) & 0xffff])
//#define spu2Rs32(mem)	(*(s32*)&spu2regs[(mem) & 0xffff])
//#define spu2Ru32(mem)	(*(u32*)&spu2regs[(mem) & 0xffff])

#define IRQINFO spu2Ru16(REG_IRQINFO)

#define SPU2_GET32BIT(lo,hi) (((u32)(spu2Ru16(hi)&0x3f)<<16)|(u32)spu2Ru16(lo))
#define SPU2_SET32BIT(value, lo, hi) { \
    spu2Ru16(hi) = ((value)>>16)&0x3f; \
    spu2Ru16(lo) = (value)&0xffff; \
    } \

#define C0_IRQA SPU2_GET32BIT(REG_C0_IRQA_LO, REG_C0_IRQA_HI)
#define C1_IRQA SPU2_GET32BIT(REG_C1_IRQA_LO, REG_C1_IRQA_HI)

#define C0_SPUADDR SPU2_GET32BIT(REG_C0_SPUADDR_LO, REG_C0_SPUADDR_HI)
#define C1_SPUADDR SPU2_GET32BIT(REG_C1_SPUADDR_LO, REG_C1_SPUADDR_HI)

#define C0_SPUADDR_SET(value) SPU2_SET32BIT(value, REG_C0_IRQA_LO, REG_C0_IRQA_HI)
#define C1_SPUADDR_SET(value) SPU2_SET32BIT(value, REG_C1_IRQA_LO, REG_C1_IRQA_HI)

#define SPU_NUMBER_VOICES       48

struct SPU_CONTROL_
{
	u16 spuon : 1;
	u16 spuUnmute : 1;
	u16 noiseFreq : 6;
	u16 reverb : 1;
	u16 irq : 1;
	u16 dma : 2;       // 1 - no dma, 2 - write, 3 - read
	u16 extr : 1;      // external reverb
	u16 cdreverb : 1;
	u16 extAudio : 1;
	u16 extCd : 1;
};

// the layout of each voice in wSpuRegs
struct _SPU_VOICE
{
    union
    {
        struct {
		u16 Vol : 14;
		u16 Inverted : 1;
		u16 Sweep0 : 1;
        } vol;
	struct {
		u16 Vol : 7;
		u16 res1 : 5;
		u16 Inverted : 1;
		u16 Decrease : 1;  // if 0, increase
		u16 ExpSlope : 1;  // if 0, linear slope
		u16 Sweep1 : 1;    // always one
	} sweep;
	u16 word;
} left, right;

	u16 pitch : 14;        // 1000 - no pitch, 2000 - pitch + 1, etc
	u16 res0 : 2;

	u16 SustainLvl : 4;
	u16 DecayRate : 4;
	u16 AttackRate : 7;
	u16 AttackExp : 1;     // if 0, linear

	u16 ReleaseRate : 5;
	u16 ReleaseExp : 1;    // if 0, linear
	u16 SustainRate : 7;
	u16 res1 : 1;
	u16 SustainDec : 1;    // if 0, inc
	u16 SustainExp : 1;    // if 0, linear

	u16 AdsrVol;
	u16 Address;           // add / 8
	u16 RepeatAddr;        // gets reset when sample starts
};

// ADSR INFOS PER   CHANNEL
struct ADSRInfoEx
{
	s32            State;
	s32            AttackModeExp;
	s32            AttackRate;
	s32            DecayRate;
	s32            SustainLevel;
	s32            SustainModeExp;
	s32            SustainIncrease;
	s32            SustainRate;
	s32            ReleaseModeExp;
	s32            ReleaseRate;
	s32            EnvelopeVol;
	s32           lVolume;
};

#define NSSIZE      48      // ~ 1 ms of data
#define NSFRAMES    16      // gather at least NSFRAMES of NSSIZE before submitting
#define NSPACKETS 4
#define SPU_VOICE_STATE_SIZE (sizeof(VOICE_PROCESSED)-4*sizeof(void*))

struct VOICE_PROCESSED
{
	VOICE_PROCESSED()
	{
		memset(this, 0, sizeof(VOICE_PROCESSED));
	}
	~VOICE_PROCESSED()
	{
	}

	void SetVolume(int right);
	void StartSound();
	void VoiceChangeFrequency();
	void FModChangeFrequency(int ns);
	void Stop();

	SPU_CONTROL_* GetCtrl();

	// start save state

	s32 iSBPos;                             // mixing stuff
	s32 spos;
	s32 sinc;

	s32 iActFreq;                           // current psx pitch
	s32 iUsedFreq;                          // current pc pitch

	s32 iStartAddr, iLoopAddr, iNextAddr;

	ADSRInfoEx ADSRX;                              // next ADSR settings (will be moved to active on sample start)
	bool bIgnoreLoop, bNew, bNoise, bReverb, bOn, bStop, bVolChanged;
	s32 memoffset;                  // if first core, 0, if second, 0x400

	// end save state

	///////////////////
	// Sound Buffers //
	///////////////////
	u8* pStart;           // start and end addresses
	u8* pLoop, *pCurr;

	_SPU_VOICE* pvoice;
};

struct ADMA
{
	u16 * MemAddr;
	s32	  IntPointer;
	s32 Index;
	s32 AmountLeft;
	s32 Enabled;
};

#endif /* __SPU2_H__ */
