/*  ZeroSPU2
 *  Copyright (C) 2006-2010 zerofrog
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __SPU2_H__
#define __SPU2_H__

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define SPU2defs
#include "PS2Edefs.h"

#include "reg.h"
#include "misc.h"

#include <string>
#include <vector>
using namespace std;

extern FILE *spu2Log;
extern string s_strIniPath;

// Prints most of the function names of the callbacks as they are called by pcsx2. 
// I'm keeping the code in because I have a feeling it will come in handy.
//#define PRINT_CALLBACKS

#ifdef PRINT_CALLBACKS
#define LOG_CALLBACK printf
#else
#define LOG_CALLBACK 0&&
#endif

#define ZEROSPU2_DEVBUILD
#ifdef ZEROSPU2_DEVBUILD
#define SPU2_LOG __Log  //dev mode
#else
#define SPU2_LOG 0&&
#endif

#define ERROR_LOG printf
#define WARN_LOG printf

#define  SPU2_VERSION  PS2E_SPU2_VERSION
#define SPU2_REVISION 0
#define  SPU2_BUILD	4	// increase that with each version
#define SPU2_MINOR 6

enum zerospu2_options
{
	OPTION_TIMESTRETCH = 1, // stretches samples without changing pitch to reduce cracking
	OPTION_REALTIME = 2, // sync to real time instead of ps2 time
	OPTION_MUTE = 4,   // don't output anything
	OPTION_RECORDING = 8
};

// ADSR constants
enum adsr_ms
{
	ATTACK_MS 		= 494L,
	DECAYHALF_MS 	= 286L,
	DECAY_MS 		= 572L,
	SUSTAIN_MS		= 441L,
	RELEASE_MS		= 437L
};

const s32 CYCLES_PER_MS = 36864000 / 1000;

const u32 AUDIO_BUFFER = 2048;

const u32 NSSIZE = 48;	  // ~ 1 ms of data
const u32 NSFRAMES = 16;	  // gather at least NSFRAMES of NSSIZE before submitting
const u32 NSPACKETS = 24;
const u32 NS_TOTAL_SIZE = NSSIZE * NSFRAMES;

const u32 SPU_NUMBER_VOICES = 48;

const u32 SAMPLE_RATE = 48000L;
#define RECORD_FILENAME "zerospu2.wav"

extern s8 *spu2regs;
extern u16* spu2mem;
extern s32 iFMod[NSSIZE];
extern u32 MemAddr[2];
extern u32 dwNoiseVal;						  // global noise generator

// functions of main emu, called on spu irq
extern void (*irqCallbackSPU2)();
extern void (*irqCallbackDMA4)();
extern void (*irqCallbackDMA7)();

extern s32 SPUCycles, SPUWorkerCycles;
extern s32 SPUStartCycle[2];
extern s32 SPUTargetCycle[2];

extern u16 interrupt;

typedef struct {
	s32 Log;
	s32 options;
} Config;

extern Config conf;

void __Log(char *fmt, ...);
void __LogToConsole(const char *fmt, ...);
void SaveConfig();
void LoadConfig();
void SysMessage(char *fmt, ...);

extern void LogRawSound(void* pleft, int leftstride, void* pright, int rightstride, int numsamples);
extern void LogPacketSound(void* packet, int memsize);

// simulate SPU2 for 1ms
extern void SPU2Worker();

// hardware sound functions
int SetupSound(); // if successful, returns 0
void RemoveSound();
int SoundGetBytesBuffered();
// returns 0 is successful, else nonzero
void SoundFeedVoiceData(unsigned char* pSound,long lBytes);

static __forceinline void clamp16(s32 &dest)
{
	if (dest < -32768L)
		dest = -32768L;
	else if (dest > 32767L) 
		dest = 32767L;
}

static __forceinline void clampandwrite16(s16 &dest, s32 &value)
{
	if (value < -32768)
		dest = -32768;
	else if (value > 32767) 
		dest = 32767;
	else 
		dest = (s16)value;
}

struct tSPU_ATTR
{
	u16 extCd : 1;
	u16 extAudio : 1;
	u16 cdreverb : 1; 
	u16 extr : 1;	  // external reverb
	u16 dma : 2;	   // 1 - no dma, 2 - write, 3 - read
	u16 irq : 1;
	u16 reverb : 1;
	u16 noiseFreq : 6;
	u16 spuUnmute : 1;
	u16 spuon : 1;
};
#define channel_test(channel) ((channel == 4) || (channel == 0))
#define spu2Rs16(mem)	(*(s16*)&spu2regs[(mem) & 0xffff])
#define spu2Ru16(mem)	(*(u16*)&spu2regs[(mem) & 0xffff])
#define spu2attr0	(*(tSPU_ATTR*)&spu2regs[REG_C0_CTRL])
#define spu2attr1	(*(tSPU_ATTR*)&spu2regs[REG_C1_CTRL])

static __forceinline u16 c_offset(u32 ch)
{
	return channel_test(ch) ? 0x0 : 0x400;
}

static __forceinline tSPU_ATTR spu2attr(u32 channel)
{
	return channel_test(channel) ? spu2attr0 : spu2attr1;
}

static __forceinline bool spu2admas(u32 channel)
{
	return channel_test(channel) ? !!(spu2Ru16(REG_C0_ADMAS) & 0x1) : !!(spu2Ru16(REG_C1_ADMAS) & 0x2);
}

static __forceinline u16 spu2mmix(u32 channel)
{
	return channel_test(channel) ? spu2Ru16(REG_C0_MMIX) : spu2Ru16(REG_C1_MMIX);
}

static __forceinline u16 spu2stat(u32 channel)
{
	return channel_test(channel) ? spu2Ru16(REG_C0_SPUSTAT) : spu2Ru16(REG_C1_SPUSTAT);
}

static __forceinline void spu2stat_clear_80(u32 channel)
{
	if channel_test(channel)
		spu2Ru16(REG_C0_SPUSTAT) &= ~0x80;
	else
		spu2Ru16(REG_C1_SPUSTAT) &= ~0x80;
}

static __forceinline void spu2stat_set_80(u32 channel)
{
	if channel_test(channel)
		spu2Ru16(REG_C0_SPUSTAT) |= 0x80;
	else
		spu2Ru16(REG_C1_SPUSTAT) |= 0x80;
}

#define IRQINFO spu2Ru16(REG_IRQINFO)

#define spu2_core_regs_0	(*(core_registers*)&spu2regs[0x0])
#define spu2_core_regs_1	(*(core_registers*)&spu2regs[0x400])

static __forceinline u32 SPU2_GET32BIT(u32 lo, u32 hi)
{
	return (((u32)(spu2Ru16(hi) & 0x3f) << 16) | (u32)spu2Ru16(lo));
}

static __forceinline void SPU2_SET32BIT(u32 value, u32 lo, u32 hi)
{
	spu2Ru16(hi) = ((value) >> 16) & 0x3f;
	spu2Ru16(lo) = (value) & 0xffff;
}

#define C0_IRQA() C_IRQA(0)
#define C1_IRQA() C_IRQA(1)
#define C0_SPUADDR() C_SPUADDR(0)
#define C1_SPUADDR() C_SPUADDR(1)
#define C0_SPUADDR_SET(val) C_SPUADDR_SET(val, 0)
#define C1_SPUADDR_SET(val) C_SPUADDR_SET(val, 1)

static __forceinline u32 C_IRQA(s32 c)
{
	if (c == 0) 
		return SPU2_GET32BIT(REG_C0_IRQA_LO, REG_C0_IRQA_HI);
	else
		return SPU2_GET32BIT(REG_C1_IRQA_LO, REG_C1_IRQA_HI);
}

static __forceinline u32 C_SPUADDR(s32 c)
{
	if (c == 0)
		return SPU2_GET32BIT(REG_C0_SPUADDR_LO, REG_C0_SPUADDR_HI);
	else
		return SPU2_GET32BIT(REG_C1_SPUADDR_LO, REG_C1_SPUADDR_HI);
}

static __forceinline void C_SPUADDR_SET(u32 value, s32 c)
{
	if (c == 0)
		SPU2_SET32BIT(value, REG_C0_SPUADDR_LO, REG_C0_SPUADDR_HI);
	else
		SPU2_SET32BIT(value, REG_C1_SPUADDR_LO, REG_C1_SPUADDR_HI);
}

#if defined(_MSC_VER)
#pragma pack(1)
#endif
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
			u16 Sweep1 : 1;	// always one
		} sweep;
		u16 word;
	} left, right;

	u16 pitch : 14;		// 1000 - no pitch, 2000 - pitch + 1, etc
	u16 res0 : 2;

	u16 SustainLvl : 4;
	u16 DecayRate : 4;
	u16 AttackRate : 7; 
	u16 AttackExp : 1;	 // if 0, linear
	
	u16 ReleaseRate : 5;
	u16 ReleaseExp : 1;	// if 0, linear
	u16 SustainRate : 7;
	u16 res1 : 1;
	u16 SustainDec : 1;	// if 0, inc
	u16 SustainExp : 1;	// if 0, linear
	
	u16 AdsrVol;
	u16 Address;		   // add / 8
	u16 RepeatAddr;		// gets reset when sample starts
#if defined(_MSC_VER)
};						//+22
#else
} __attribute__((packed));
#endif

// ADSR INFOS PER   CHANNEL
struct ADSRInfoEx
{
	s32			State;
	s32			AttackModeExp;
	s32			AttackRate;
	s32			DecayRate;
	s32			SustainLevel;
	s32			SustainModeExp;
	s32			SustainIncrease;
	s32			SustainRate;
	s32			ReleaseModeExp;
	s32			ReleaseRate;
	s32			EnvelopeVol;
	s32			lVolume;
};

#define SPU_VOICE_STATE_SIZE (sizeof(VOICE_PROCESSED)-4*sizeof(void*))

struct VOICE_PROCESSED
{
	VOICE_PROCESSED()
	{
		memset(this, 0, sizeof(VOICE_PROCESSED));
	}

	void SetVolume(int right);
	void StartSound();
	void VoiceChangeFrequency();
	void InterpolateUp();
	void InterpolateDown();
	void FModChangeFrequency(int ns);
	s32 iGetNoiseVal();
	void StoreInterpolationVal(int fa);
	s32 iGetInterpolationVal();
	s32 iGetVal();
	void Stop();

	tSPU_ATTR* GetCtrl();

	// start save state
	s32 leftvol, rightvol;	 // left right volumes

	s32 iSBPos;							 // mixing stuff
	s32 SB[32+32];
	s32 spos;
	s32 sinc;

	s32 iIrqDone;						   // debug irq done flag
	s32 s_1, s_2;								// last decoding infos
	s32 iOldNoise;						  // old noise val for this channel   
	s32 iActFreq, iUsedFreq;			// current psx pitch & pc pitch

	s32 iStartAddr, iLoopAddr, iNextAddr;
	s32 bFMod;

	ADSRInfoEx ADSRX;							  // next ADSR settings (will be moved to active on sample start)
	s32 memoffset;				  // if first core, 0, if second, 0x400
	s32 chanid; // channel id

	bool bIgnoreLoop, bNew, bNoise, bReverb, bOn, bStop, bVolChanged;
	bool bVolumeR, bVolumeL;
	
	// end save state

	///////////////////
	// Sound Buffers //
	///////////////////
	u8* pStart;		   // start and end addresses
	u8* pLoop, *pCurr;

	_SPU_VOICE* pvoice;
	u32 memchannel;
	void init(s32 i)
	{
		chanid = i;
		
		if (chanid > 23)
		{
			memoffset = 0x400;
			memchannel = 1;
		}
		else
		{
			memoffset = 0x0;
			memchannel = 0;
		}
		
		pLoop = pStart = pCurr = (u8*)spu2mem;

		pvoice = (_SPU_VOICE*)((u8*)spu2regs + memoffset) + (i % 24);
		ADSRX.SustainLevel = 1024;				// -> init sustain
	}
};

struct AUDIOBUFFER
{
	u8* pbuf;
	u32 len;

	// 1 if new channels started in this packet
	// Variable used to smooth out sound by concentrating on new voices
	u32 timestamp; // in microseconds, only used for time stretching
	u32 avgtime;
	s32 newchannels;
};

struct ADMA
{
	u16*			  MemAddr;
	s32			  Index;
	s32			  AmountLeft;
	s32			  Enabled; 
	// used to make sure that ADMA doesn't get interrupted with a writeDMA call
};

extern ADMA adma[2];

struct SPU2freezeData
{
	u32 version;
	u8 spu2regs[0x10000];
	u8 spu2mem[0x200000];
	u16 interrupt;
	s32 nSpuIrq[2];
	u32 dwNewChannel2[2], dwEndChannel2[2];
	u32 dwNoiseVal;
	s32 iFMod[NSSIZE];
	u32 MemAddr[2];
	ADMA adma[2];
	u32 AdmaMemAddr[2];

	s32 SPUCycles, SPUWorkerCycles;
	s32 SPUStartCycle[2];
	s32 SPUTargetCycle[2];

	s32 voicesize;
	VOICE_PROCESSED voices[SPU_NUMBER_VOICES+1];
};

#endif /* __SPU2_H__ */
