//GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
#ifndef SPU2_H_INCLUDED
#define SPU2_H_INCLUDED
//system defines
#ifdef __LINUX__
	#include <gtk/gtk.h>
#else
#	define WINVER 0x0501
#	define _WIN32_WINNT 0x0501
#	include <windows.h>
#	include <mmsystem.h>
#endif
#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"
#include "math.h"
#include "time.h"

//SPU2 plugin defines
//#define SPU2defs		// not using the PCSX2 defs (see below)
#include "PS2Edefs.h"

#define EXPORT_C_(type) extern "C" __declspec(dllexport) type __stdcall

// We have our own versions that have the DLLExport attribute configured:

EXPORT_C_(s32)  SPU2init();
EXPORT_C_(s32)  SPU2open(void *pDsp);
EXPORT_C_(void) SPU2close();
EXPORT_C_(void) SPU2shutdown();
EXPORT_C_(void) SPU2write(u32 mem, u16 value);
EXPORT_C_(u16)  SPU2read(u32 mem);
EXPORT_C_(void) SPU2readDMA4Mem(u16 *pMem, u32 size);
EXPORT_C_(void) SPU2writeDMA4Mem(u16 *pMem, u32 size);
EXPORT_C_(void) SPU2interruptDMA4();
EXPORT_C_(void) SPU2readDMA7Mem(u16* pMem, u32 size);
EXPORT_C_(void) SPU2writeDMA7Mem(u16 *pMem, u32 size);

// all addresses passed by dma will be pointers to the array starting at baseaddr
// This function is necessary to successfully save and reload the spu2 state
EXPORT_C_(void) SPU2setDMABaseAddr(uptr baseaddr);

EXPORT_C_(void) SPU2interruptDMA7();
EXPORT_C_(u32) SPU2ReadMemAddr(int core);
EXPORT_C_(void) SPU2WriteMemAddr(int core,u32 value);
EXPORT_C_(void) SPU2irqCallback(void (*SPU2callback)(),void (*DMA4callback)(),void (*DMA7callback)());

// extended funcs
// if start is 1, starts recording spu2 data, else stops
// returns a non zero value if successful
// for now, pData is not used
EXPORT_C_(int) SPU2setupRecording(int start, void* pData);

EXPORT_C_(void) SPU2setClockPtr(u32* ptr);

EXPORT_C_(void) SPU2async(u32 cycles);
EXPORT_C_(s32) SPU2freeze(int mode, freezeData *data);
EXPORT_C_(void) SPU2configure();
EXPORT_C_(void) SPU2about();
EXPORT_C_(s32)  SPU2test();


//#define EFFECTS_DUMP

//Plugin parts
#include "config.h"
#include "defs.h"
#include "regs.h"
#include "dma.h"
#include "mixer.h"
#include "sndout.h"

#include "spu2replay.h"

#define SPU2_LOG

#include "debug.h"

// [Air] : give hints to the optimizer
//  This is primarily useful for the default case switch optimizer, which enables VC to
//  generate more compact switches.

#ifdef NDEBUG
#	define jBREAKPOINT() ((void) 0)
#	ifdef _MSC_VER
#		define jASSUME(exp) (__assume(exp))
#	else
#		define jASSUME(exp) ((void) sizeof(exp))
#	endif
#else
#	if defined(_MSC_VER)
#		define jBREAKPOINT() do { __asm int 3 } while(0)
#	else
#		define jBREAKPOINT() ((void) *(volatile char *) 0)
#	endif
#	define jASSUME(exp) if(exp) ; else jBREAKPOINT()
#endif

// disable the default case in a switch
#define jNO_DEFAULT \
{ \
	break; \
	\
default: \
	jASSUME(0); \
	break; \
}

//--------------------------------------------------------------------------------------
// Helper macros
//--------------------------------------------------------------------------------------
#ifndef SAFE_FREE
#	define SAFE_FREE(p)		{ if(p) { free(p); (p)=NULL; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#	define SAFE_DELETE_ARRAY(p)		{ if(p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_DELETE
#	define SAFE_DELETE_OBJ(p)		{ if(p) { delete (p); (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#	define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

// The SPU2 has a dynamic memory range which is used for several internal operations, such as
// registers, CORE 1/2 mixing, AutoDMAs, and some other fancy stuff.  We exclude this range
// from the cache here:
static const s32 SPU2_DYN_MEMLINE = 0x2800;

// 8 short words per encoded PCM block. (as stored in SPU2 ram)
static const int pcm_WordsPerBlock = 8;

// number of cachable ADPCM blocks (any blocks above the SPU2_DYN_MEMLINE)
static const int pcm_BlockCount = 0x100000 / pcm_WordsPerBlock;

// 28 samples per decoded PCM block (as stored in our cache)
static const int pcm_DecodedSamplesPerBlock = 28;

struct PcmCacheEntry
{
	bool Validated; 
	s16 Sampledata[pcm_DecodedSamplesPerBlock];
};

extern void spdif_set51(u32 is_5_1_out);
extern u32  spdif_init();
extern void spdif_shutdown();
extern void spdif_get_samples(s32 *samples); // fills the buffer with [l,r,c,lfe,sl,sr] if using 5.1 output, or [l,r] if using stereo

extern short *spu2regs;
extern short *_spu2mem;

extern PcmCacheEntry* pcm_cache_data;

extern s16 __forceinline * __fastcall GetMemPtr(u32 addr);
extern s16 __forceinline __fastcall spu2M_Read( u32 addr );
extern void __inline __fastcall spu2M_Write( u32 addr, s16 value );
extern void __inline __fastcall spu2M_Write( u32 addr, u16 value );

#define spu2Rs16(mmem)	(*(s16 *)((s8 *)spu2regs + ((mmem) & 0x1fff)))
#define spu2Ru16(mmem)	(*(u16 *)((s8 *)spu2regs + ((mmem) & 0x1fff)))

void SysMessage(const char *fmt, ...);

extern void VoiceStart(int core,int vc);
extern void VoiceStop(int core,int vc);

extern s32 uTicks;

extern void (* _irqcallback)();
extern void (* dma4callback)();
extern void (* dma7callback)();

extern void SetIrqCall();

extern double srate_pv;

extern s16 *input_data;
extern u32 input_data_ptr;

extern HINSTANCE hInstance;

extern int PlayMode;

extern int recording;
void RecordStart();
void RecordStop();
void RecordWrite(s16 left, s16 right);

extern CRITICAL_SECTION threadSync;

extern u32 lClocks;

extern u32* cPtr;
extern bool hasPtr;

extern bool disableFreezes;

void __fastcall TimeUpdate(u32 cClocks);

void TimestretchUpdate(int bufferusage,int buffersize);

s32  DspLoadLibrary(char *fileName, int modNum);
void DspCloseLibrary();
int  DspProcess(s16 *buffer, int samples);
void DspUpdate(); // to let the Dsp process window messages

void SndUpdateLimitMode();
//#define PCM24_S1_INTERLEAVE

#endif // SPU2_H_INCLUDED //
