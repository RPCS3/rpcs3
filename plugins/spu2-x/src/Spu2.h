/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2.1 of the the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef SPU2_H_INCLUDED
#define SPU2_H_INCLUDED

#include "BaseTypes.h"
#include "PS2Edefs.h"

#ifdef __LINUX__
#include <stdio.h>
#include <string.h>

//Until I get around to putting in Linux svn code, this is an unknown svn version.
#define SVN_REV_UNKNOWN
#endif

#include "ConvertUTF.h"

namespace VersionInfo
{
	static const u8 PluginApi = PS2E_SPU2_VERSION;
	static const u8 Release  = 1;
	static const u8 Revision = 0;	// increase that with each version
}

#ifdef _MSC_VER
#define EXPORT_C_(type) extern "C" __declspec(dllexport) type CALLBACK
#else
#define EXPORT_C_(type) extern "C" type
#endif

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
#include "Config.h"
#include "defs.h"
#include "regs.h"
#include "Dma.h"
#include "SndOut.h"

#include "Spu2replay.h"

#define SPU2_LOG

#include "Debug.h"

//--------------------------------------------------------------------------------------
// Helper macros
//--------------------------------------------------------------------------------------
#ifndef SAFE_FREE
#	define SAFE_FREE(p)		{ if(p) { free(p); (p)=NULL; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#	define SAFE_DELETE_ARRAY(p)		{ if(p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_DELETE_OBJ
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

extern u8 callirq;

extern void (* _irqcallback)();
extern void (* dma4callback)();
extern void (* dma7callback)();

extern void SetIrqCall();

extern double srate_pv;

extern s16 *input_data;
extern u32 input_data_ptr;

extern int PlayMode;
extern int recording;
extern bool disableFreezes;

extern u32 lClocks;
extern u32* cPtr;
extern bool hasPtr;
extern bool resetClock;

extern void SPU2writeLog( const char* action, u32 rmem, u16 value );

extern void __fastcall TimeUpdate(u32 cClocks);
extern u16 SPU_ps1_read(u32 mem);
extern void SPU_ps1_write(u32 mem, u16 value);
extern void SPU2_FastWrite( u32 rmem, u16 value );

extern void StartVoices(int core, u32 value);
extern void StopVoices(int core, u32 value);

extern s32  DspLoadLibrary(wchar_t *fileName, int modNum);
extern void DspCloseLibrary();
extern int  DspProcess(s16 *buffer, int samples);
extern void DspUpdate(); // to let the Dsp process window messages

extern void RecordStart();
extern void RecordStop();
extern void RecordWrite( const StereoOut16& sample );

extern void UpdateSpdifMode();
extern void LowPassFilterInit();
extern void InitADSR();
extern void CalculateADSR( V_Voice& vc );

extern void __fastcall ReadInput( uint core, StereoOut32& PData );


//////////////////////////////
//    The Mixer Section     //
//////////////////////////////

extern void Mix();
extern s32 clamp_mix( s32 x, u8 bitshift=0 );
extern void clamp_mix( StereoOut32& sample, u8 bitshift=0 );
extern void Reverb_AdvanceBuffer( V_Core& thiscore );
extern StereoOut32 DoReverb( V_Core& thiscore, const StereoOut32& Input );
extern s32 MulShr32( s32 srcval, s32 mulval );

//#define PCM24_S1_INTERLEAVE

#endif // SPU2_H_INCLUDED //
