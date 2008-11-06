/*  ZeroSPU2
 *  Copyright (C) 2006-2007 zerofrog
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

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <string.h>
#include <malloc.h>

extern "C" {
#define SPU2defs
#include "PS2Edefs.h"
}

#ifdef __LINUX__
#include <unistd.h>
#include <gtk/gtk.h>
#include <sys/timeb.h>    // ftime(), struct timeb

#define Sleep(ms) usleep(1000*ms)

inline unsigned long timeGetTime()
{
#ifdef _WIN32
    _timeb t;
    _ftime(&t);
#else
    timeb t;
    ftime(&t);
#endif

    return (unsigned long)(t.time*1000+t.millitm);
}

#include <sys/time.h>

#else
#include <windows.h>
#include <windowsx.h>

#include <sys/timeb.h>    // ftime(), struct timeb
#endif


inline u64 GetMicroTime()
{
#ifdef _WIN32
    extern LARGE_INTEGER g_counterfreq;
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return count.QuadPart * 1000000 / g_counterfreq.QuadPart;
#else
    timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec*1000000+t.tv_usec;
#endif
}

#include <string>
#include <vector>
using namespace std;

extern FILE *spu2Log;

#ifdef _DEBUG
#define SPU2_LOG __Log  //debug mode
#else
#define SPU2_LOG 0&&
#endif

#define  SPU2_VERSION  PS2E_SPU2_VERSION
#define SPU2_REVISION 0
#define  SPU2_BUILD    4    // increase that with each version
#define SPU2_MINOR 6

#define OPTION_TIMESTRETCH 1 // stretches samples without changing pitch to reduce cracking
#define OPTION_REALTIME 2 // sync to real time instead of ps2 time
#define OPTION_MUTE 4   // don't output anything
#define OPTION_RECORDING 8

typedef struct {
    int Log;
    int options;
} Config;

extern Config conf;

void __Log(char *fmt, ...);
void SaveConfig();
void LoadConfig();
void SysMessage(char *fmt, ...);

void LogRawSound(void* pleft, int leftstride, void* pright, int rightstride, int numsamples);
void LogPacketSound(void* packet, int memsize);

// hardware sound functions
int SetupSound(); // if successful, returns 0
void RemoveSound();
int SoundGetBytesBuffered();
// returns 0 is successful, else nonzero
void SoundFeedVoiceData(unsigned char* pSound,long lBytes);

#if !defined(_MSC_VER) && !defined(HAVE_ALIGNED_MALLOC)

#include <assert.h>

// declare linux equivalents
extern  __forceinline void* pcsx2_aligned_malloc(size_t size, size_t align)
{
    assert( align < 0x10000 );
	char* p = (char*)malloc(size+align);
	int off = 2+align - ((int)(uptr)(p+2) % align);

	p += off;
	*(u16*)(p-2) = off;

	return p;
}

extern __forceinline void pcsx2_aligned_free(void* pmem)
{
    if( pmem != NULL ) {
        char* p = (char*)pmem;
        free(p - (int)*(u16*)(p-2));
    }
}

#define _aligned_malloc pcsx2_aligned_malloc
#define _aligned_free pcsx2_aligned_free

#endif

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
#define REG_C0_VMIXL1      0x0188 
#define REG_C0_VMIXL2      0x018A
#define REG_C0_VMIXR1      0x0190
#define REG_C0_VMIXR2      0x0192
#define REG_C0_MMIX       0x0198 
#define REG_C0_CTRL       0x019A 
#define REG_C0_IRQA_HI   0x019C 
#define REG_C0_IRQA_LO   0x019E
#define REG_C0_SPUON1    0x1A0
#define REG_C0_SPUON2    0x1A2
#define REG_C0_SPUOFF1   0x1A4
#define REG_C0_SPUOFF2   0x1A6
#define REG_C0_SPUADDR_HI 0x01A8 
#define REG_C0_SPUADDR_LO 0x01AA
#define REG_C0_SPUDATA   0x01AC 
#define REG_C0_DMACTRL   0x01AE 
#define REG_C0_ADMAS     0x01B0 
#define REG_C0_END1       0x0340 
#define REG_C0_END2       0x0342 
#define REG_C0_SPUSTAT  0x0344
#define REG_C0_BVOLL      0x076C 
#define REG_C0_BVOLR      0x076E 

//#define SPU2_TRANS_BY_DMA     (0x0<<3)
//#define SPU2_TRANS_BY_IO      (0x1<<3)
//
//#define SPU2_BLOCK_ONESHOT (0<<4) 
//#define SPU2_BLOCK_LOOP (1<<4) 
//#define SPU2_BLOCK_HANDLER (1<<7)
//#define SPU2_BLOCK_C0_VOICE1    ( 0x0<<8 )
//#define SPU2_BLOCK_C0_VOICE3    ( 0x1<<8 )
//#define SPU2_BLOCK_C1_SINL      ( 0x2<<8 )
//#define SPU2_BLOCK_C1_SINR      ( 0x3<<8 )
//#define SPU2_BLOCK_C1_VOICE1    ( 0x4<<8 )
//#define SPU2_BLOCK_C1_VOICE3    ( 0x5<<8 )
//#define SPU2_BLOCK_C0_MEMOUTL   ( 0x6<<8 )
//#define SPU2_BLOCK_C0_MEMOUTR   ( 0x7<<8 )
//#define SPU2_BLOCK_C0_MEMOUTEL  ( 0x8<<8 )
//#define SPU2_BLOCK_C0_MEMOUTER  ( 0x9<<8 )
//#define SPU2_BLOCK_C1_MEMOUTL   ( 0xa<<8 )
//#define SPU2_BLOCK_C1_MEMOUTR   ( 0xb<<8 )
//#define SPU2_BLOCK_C1_MEMOUTEL  ( 0xc<<8 )
//#define SPU2_BLOCK_C1_MEMOUTER  ( 0xd<<8 )

//#define SPU2_BLOCK_COUNT(x)  ( (x)<<12 )
//
//#define SPU2_TRANS_STATUS_WAIT  1
//#define SPU2_TRANS_STATUS_CHECK 0
//
//#define SPU2_REV_MODE_OFF  	   0
//#define SPU2_REV_MODE_ROOM	   1
//#define SPU2_REV_MODE_STUDIO_A	   2
//#define SPU2_REV_MODE_STUDIO_B	   3
//#define SPU2_REV_MODE_STUDIO_C	   4
//#define SPU2_REV_MODE_HALL	   5
//#define SPU2_REV_MODE_SPACE	   6
//#define SPU2_REV_MODE_ECHO	   7
//#define SPU2_REV_MODE_DELAY	   8
//#define SPU2_REV_MODE_PIPE	   9
//#define SPU2_REV_MODE_MAX		   10
//#define SPU2_REV_MODE_CLEAR_WA	   0x100
//#define SPU2_REV_MODE_CLEAR_CH0	   0x000
//#define SPU2_REV_MODE_CLEAR_CH1	   0x200
//#define SPU2_REV_MODE_CLEAR_CH_MASK  0x200
//
//#define SPU2_REV_SIZE_OFF  	   0x80
//#define SPU2_REV_SIZE_ROOM	   0x26c0
//#define SPU2_REV_SIZE_STUDIO_A	   0x1f40
//#define SPU2_REV_SIZE_STUDIO_B	   0x4840
//#define SPU2_REV_SIZE_STUDIO_C	   0x6fe0
//#define SPU2_REV_SIZE_HALL	   0xade0
//#define SPU2_REV_SIZE_SPACE	   0xf6c0
//#define SPU2_REV_SIZE_ECHO	   0x18040
//#define SPU2_REV_SIZE_DELAY	   0x18040
//#define SPU2_REV_SIZE_PIPE	   0x3c00
//
//#define SPU2_SOUNDOUT_TOPADDR        0x0
//#define SPU2_SOUNDIN_TOPADDR         0x4000
//#define SPU2_USERAREA_TOPADDR        0x5010
//
//#define SPU2_INIT_COLD	0
//#define SPU2_INIT_HOT	1

#define REG_C1_FMOD1       0x0580 
#define REG_C1_FMOD2       0x0582
#define REG_S_NON        0x0184 
#define REG_C1_VMIXL1      0x0588 
#define REG_C1_VMIXL2      0x058A
#define REG_S_VMIXEL     0x018C 
#define REG_C1_VMIXR1      0x0590 
#define REG_C1_VMIXR2      0x0592
#define REG_S_VMIXER     0x0194 
#define REG_C1_MMIX       0x0598 
#define REG_C1_IRQA_HI   0x059C 
#define REG_C1_IRQA_LO   0x059E
#define REG_C1_SPUON1    0x5A0
#define REG_C1_SPUON2    0x5A2
#define REG_C1_SPUOFF1   0x5A4
#define REG_C1_SPUOFF2   0x5A6
#define REG_C1_SPUADDR_HI 0x05A8 
#define REG_C1_SPUADDR_LO 0x05AA
#define REG_C1_SPUDATA   0x05AC 
#define REG_C1_DMACTRL   0x05AE 
#define REG_VA_SSA       0x01C0 
#define REG_VA_LSAX      0x01C4 
#define REG_VA_NAX       0x01C8 
#define REG_A_ESA        0x02E0 
#define REG_A_EEA        0x033C 
#define REG_C1_END1       0x0740 
#define REG_C1_END2       0x0742
#define REG_C1_CTRL       0x059A		
#define REG_C1_ADMAS     0x05B0 
#define REG_C1_SPUSTAT   0x0744
#define REG_P_MVOLL      0x0760 
#define REG_P_MVOLR      0x0762 
#define REG_P_EVOLL      0x0764 
#define REG_P_EVOLR      0x0766 
#define REG_P_AVOLL      0x0768 
#define REG_P_AVOLR      0x076A 
#define REG_C1_BVOLL      0x0794 
#define REG_C1_BVOLR      0x0796 
#define REG_P_MVOLXL     0x0770 
#define REG_P_MVOLXR     0x0772 
#define SPDIF_OUT        0x07C0 
#define REG_IRQINFO      0x07C2	
#define SPDIF_MODE       0x07C6			
#define SPDIF_MEDIA      0x07C8			

#define SPU_AUTODMA_ONESHOT 0  //spu2
#define SPU_AUTODMA_LOOP 1   //spu2
#define SPU_AUTODMA_START_ADDR (1 << 1)   //spu2

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

#define C0_SPUADDR_SET(value) SPU2_SET32BIT(value, REG_C0_SPUADDR_LO, REG_C0_SPUADDR_HI)
#define C1_SPUADDR_SET(value) SPU2_SET32BIT(value, REG_C1_SPUADDR_LO, REG_C1_SPUADDR_HI)

#define SPU_NUMBER_VOICES       48

struct SPU_CONTROL_
{
    u16 extCd : 1;
    u16 extAudio : 1;
    u16 cdreverb : 1; 
    u16 extr : 1;      // external reverb
    u16 dma : 2;       // 1 - no dma, 2 - write, 3 - read
    u16 irq : 1;
    u16 reverb : 1;
    u16 noiseFreq : 6;
    u16 spuUnmute : 1;
    u16 spuon : 1;
};

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
#if defined(_MSC_VER)
};						//+22
#else
} __attribute__((packed));
#endif

// ADSR INFOS PER   CHANNEL
struct ADSRInfoEx
{
    int            State;
    int            AttackModeExp;
    int            AttackRate;
    int            DecayRate;
    int            SustainLevel;
    int            SustainModeExp;
    int            SustainIncrease;
    int            SustainRate;
    int            ReleaseModeExp;
    int            ReleaseRate;
    int            EnvelopeVol;
    long           lVolume;
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
    int iGetNoiseVal();
    void StoreInterpolationVal(int fa);
    int iGetInterpolationVal();
    void Stop();

    SPU_CONTROL_* GetCtrl();

    // start save state
    int leftvol, rightvol;     // left right volumes

    int iSBPos;                             // mixing stuff
    int SB[32+32];
    int spos;
    int sinc;

    int iIrqDone;                           // debug irq done flag
    int s_1;                                // last decoding infos
    int s_2;
    int iOldNoise;                          // old noise val for this channel   
    int               iActFreq;                           // current psx pitch
    int               iUsedFreq;                          // current pc pitch

    int iStartAddr, iLoopAddr, iNextAddr;
    int bFMod;

    ADSRInfoEx ADSRX;                              // next ADSR settings (will be moved to active on sample start)
    int memoffset;                  // if first core, 0, if second, 0x400
    int chanid; // channel id

    bool bIgnoreLoop, bNew, bNoise, bReverb, bOn, bStop, bVolChanged;
    bool bVolumeR, bVolumeL;
    
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
    unsigned short * MemAddr;
    int			  Index;
    int			  AmountLeft;
    int			  Enabled; // used to make sure that ADMA doesn't get interrupted with a writeDMA call
};

#endif /* __SPU2_H__ */
