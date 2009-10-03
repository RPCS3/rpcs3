/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Mixer.h"

// --------------------------------------------------------------------------------------
//  SPU2 Memory Indexers
// --------------------------------------------------------------------------------------

#define spu2Rs16(mmem)	(*(s16 *)((s8 *)spu2regs + ((mmem) & 0x1fff)))
#define spu2Ru16(mmem)	(*(u16 *)((s8 *)spu2regs + ((mmem) & 0x1fff)))

extern s16*	__fastcall GetMemPtr(u32 addr);
extern s16	__fastcall spu2M_Read( u32 addr );
extern void	__fastcall spu2M_Write( u32 addr, s16 value );
extern void	__fastcall spu2M_Write( u32 addr, u16 value );


struct V_VolumeLR
{
	static V_VolumeLR Max;

	s32		Left;
	s32		Right;

	V_VolumeLR() {}
	V_VolumeLR( s32 both ) :
		Left( both ),
		Right( both )
	{
	}
	
	void DebugDump( FILE* dump, const char* title );
};

struct V_VolumeSlide
{
	// Holds the "original" value of the volume for this voice, prior to slides.
	// (ie, the volume as written to the register)
	
	s16		Reg_VOL;
	s32		Value;
	s8		Increment;
	s8		Mode;
	
public:
	V_VolumeSlide() {}
	V_VolumeSlide( s16 regval, s32 fullvol ) :
		Reg_VOL( regval ),
		Value( fullvol ),
		Increment( 0 ),
		Mode( 0 )
	{
	}

	void Update();
	void RegSet( u16 src );		// used to set the volume from a register source (16 bit signed)
	void DebugDump( FILE* dump, const char* title, const char* nameLR );
};

struct V_VolumeSlideLR
{
	static V_VolumeSlideLR Max;

	V_VolumeSlide Left;
	V_VolumeSlide Right;

public:
	V_VolumeSlideLR() {}
		
	V_VolumeSlideLR( s16 regval, s32 bothval ) :
		Left( regval, bothval ),
		Right( regval, bothval )
	{
	}

	void Update()
	{
		Left.Update();
		Right.Update();
	}
	
	void DebugDump( FILE* dump, const char* title );
};

struct V_ADSR
{
	union
	{
		u32	reg32;
		
		struct  
		{
			u16 regADSR1;
			u16 regADSR2;
		};
		
		struct 
		{
			u32	SustainLevel:4,
				DecayRate:4,
				AttackRate:7,
				AttackMode:1,	// 0 for linear (+lin), 1 for pseudo exponential (+exp)

				ReleaseRate:5,
				ReleaseMode:1,	// 0 for linear (-lin), 1 for exponential (-exp)
				SustainRate:7,
				SustainMode:3;	// 0 = +lin, 1 = -lin, 2 = +exp, 3 = -exp
		};
	};

	s32		Value;		// Ranges from 0 to 0x7fffffff (signed values are clamped to 0) [Reg_ENVX]
	u8		Phase;		// monitors current phase of ADSR envelope
	bool	Releasing;	// Ready To Release, triggered by Voice.Stop();

public:
	bool Calculate();
};


struct V_Voice
{
	u32 PlayCycle;		// SPU2 cycle where the Playing started

	V_VolumeSlideLR Volume;

// Envelope
	V_ADSR ADSR;
// Pitch (also Reg_PITCH)
	s16 Pitch; 
// Loop Start address (also Reg_LSAH/L)
	u32 LoopStartA; 
// Sound Start address (also Reg_SSAH/L)
	u32 StartA;
// Next Read Data address (also Reg_NAXH/L)
	u32 NextA;
// Voice Decoding State
	s32 Prev1;
	s32 Prev2;

	// Pitch Modulated by previous voice
	bool Modulated;
	// Source (Wave/Noise)
	bool Noise;

	s8 LoopMode;
	s8 LoopFlags;

// Sample pointer (19:12 bit fixed point)
	s32 SP;

// Sample pointer for Cubic Interpolation
// Cubic interpolation mixes a sample behind Linear, so that it
// can have sample data to either side of the end points from which
// to extrapolate.  This SP represents that late sample position.
	s32 SPc;

// Previous sample values - used for interpolation
// Inverted order of these members to match the access order in the
//   code (might improve cache hits).
	s32 PV4;
	s32 PV3;
	s32 PV2;
	s32 PV1;

// Last outputted audio value, used for voice modulation.
	s32 OutX;

// SBuffer now points directly to an ADPCM cache entry.
	s16 *SBuffer;

// sample position within the current decoded packet.
	s32 SCurrent;

	void Start();
	void Stop();
};

// ** Begin Debug-only variables section **
// Separated from the V_Voice struct to improve cache performance of
// the Public Release build.
struct V_VoiceDebug
{
	s8 FirstBlock;
	s32 SampleData;
	s32 PeakX;
	s32 displayPeak;
	s32 lastSetStartA;
};

struct V_CoreDebug
{
	V_VoiceDebug Voices[24];
// Last Transfer Size
	u32 lastsize;
};

// Debug tracking information - 24 voices and 2 cores.
extern V_CoreDebug DebugCores[2];

struct V_Reverb
{
	s16 IN_COEF_L;
	s16 IN_COEF_R;

	u32 FB_SRC_A;
	u32 FB_SRC_B;

	s16 FB_ALPHA;
	s16 FB_X;
	
	u32 IIR_SRC_A0;
	u32 IIR_SRC_A1;
	u32 IIR_SRC_B1;
	u32 IIR_SRC_B0;
	u32 IIR_DEST_A0;
	u32 IIR_DEST_A1;
	u32 IIR_DEST_B0;
	u32 IIR_DEST_B1;

	s16 IIR_ALPHA;
	s16 IIR_COEF;

	u32 ACC_SRC_A0;
	u32 ACC_SRC_A1;
	u32 ACC_SRC_B0;
	u32 ACC_SRC_B1;
	u32 ACC_SRC_C0;
	u32 ACC_SRC_C1;
	u32 ACC_SRC_D0;
	u32 ACC_SRC_D1;

	s16 ACC_COEF_A;
	s16 ACC_COEF_B;
	s16 ACC_COEF_C;
	s16 ACC_COEF_D;

	u32 MIX_DEST_A0;
	u32 MIX_DEST_A1;
	u32 MIX_DEST_B0;
	u32 MIX_DEST_B1;
};

struct V_ReverbBuffers
{
	s32 FB_SRC_A0;
	s32 FB_SRC_B0;
	s32 FB_SRC_A1;
	s32 FB_SRC_B1;

	s32 IIR_SRC_A0;
	s32 IIR_SRC_A1;
	s32 IIR_SRC_B1;
	s32 IIR_SRC_B0;
	s32 IIR_DEST_A0;
	s32 IIR_DEST_A1;
	s32 IIR_DEST_B0;
	s32 IIR_DEST_B1;

	s32 ACC_SRC_A0;
	s32 ACC_SRC_A1;
	s32 ACC_SRC_B0;
	s32 ACC_SRC_B1;
	s32 ACC_SRC_C0;
	s32 ACC_SRC_C1;
	s32 ACC_SRC_D0;
	s32 ACC_SRC_D1;

	s32 MIX_DEST_A0;
	s32 MIX_DEST_A1;
	s32 MIX_DEST_B0;
	s32 MIX_DEST_B1;
	
	bool NeedsUpdated;
};

struct V_SPDIF
{
	u16 Out;
	u16 Info;
	u16 Unknown1;
	u16 Mode;
	u16 Media;
	u16 Unknown2;
	u16 Protection;
};

struct V_CoreRegs
{
	u32 PMON;
	u32 NON;
	u32 VMIXL;
	u32 VMIXR;
	u32 VMIXEL;
	u32 VMIXER;
	u32 ENDX;

	u16 MMIX;
	u16 STATX;
	u16 ATTR;
	u16 _1AC;
};

struct V_VoiceGates
{
	s16 DryL;	// 'AND Gate' for Direct Output to Left Channel
	s16 DryR;	// 'AND Gate' for Direct Output for Right Channel
	s16 WetL;	// 'AND Gate' for Effect Output for Left Channel
	s16 WetR;	// 'AND Gate' for Effect Output for Right Channel
};

struct V_CoreGates
{
	union
	{
		u128 v128;

		struct  
		{
			s16 InpL;	// Sound Data Input to Direct Output (Left)
			s16 InpR;	// Sound Data Input to Direct Output (Right)
			s16 SndL;	// Voice Data to Direct Output (Left)
			s16 SndR;	// Voice Data to Direct Output (Right)
			s16 ExtL;	// External Input to Direct Output (Left)
			s16 ExtR;	// External Input to Direct Output (Right)
		};
	};
	
};

struct VoiceMixSet
{
	static const VoiceMixSet Empty;
	StereoOut32 Dry, Wet;

	VoiceMixSet() {}
	VoiceMixSet( const StereoOut32& dry, const StereoOut32& wet ) :
		Dry( dry ),
		Wet( wet )
	{
	}
};

struct V_Core
{
	static const uint NumVoices = 24;

	int				Index;			// Core index identifier.

	// Voice Gates -- These are SSE-related values, and must always be
	// first to ensure 16 byte alignment

	V_VoiceGates	VoiceGates[NumVoices];
	V_CoreGates		DryGate;
	V_CoreGates		WetGate;

	V_VolumeSlideLR	MasterVol;		// Master Volume
	V_VolumeLR		ExtVol;			// Volume for External Data Input
	V_VolumeLR		InpVol;			// Volume for Sound Data Input
	V_VolumeLR		FxVol;			// Volume for Output from Effects 

	V_Voice			Voices[NumVoices];
	
	u32				IRQA;			// Interrupt Address
	u32				TSA;			// DMA Transfer Start Address
	u32				TDA;			// DMA Transfer Data Address (Internal...)

	bool			IRQEnable;		// Interrupt Enable
	bool			FxEnable;		// Effect Enable
	bool			Mute;			// Mute
	bool			AdmaInProgress;

	s8				DMABits;		// DMA related?
	s8				NoiseClk;		// Noise Clock
	u16				AutoDMACtrl;	// AutoDMA Status
	s32				DMAICounter;	// DMA Interrupt Counter
	u32				InputDataLeft;	// Input Buffer
	u32				InputPos;
	u32				InputDataProgress;

	V_Reverb		Revb;			// Reverb Registers
	V_ReverbBuffers	RevBuffers;		// buffer pointers for reverb, pre-calculated and pre-clipped.
	u32				EffectsStartA;
	u32				EffectsEndA;
	u32				ReverbX;
	
	// Current size of the effects buffer.  Pre-caculated when the effects start
	// or end position registers are written.  CAN BE NEGATIVE OR ZERO, in which
	// case reverb should be disabled.
	s32				EffectsBufferSize;
	
	V_CoreRegs		Regs;			// Registers

	// Last samples to pass through the effects processor.
	// Used because the effects processor works at 24khz and just pulls
	// from this for the odd Ts.
	StereoOut32		LastEffect;

	u8				InitDelay;
	u8				CoreEnabled;

	u8				AttrBit0;
	u8				AttrBit4;
	u8				AttrBit5;

	u16*			DMAPtr;
	u32				MADR;
	u32				TADR;

	// HACK -- This is a temp buffer which is (or isn't?) used to circumvent some memory
	// corruption that originates elsewhere in the plugin. >_<  The actual ADMA buffer
	// is an area mapped to SPU2 main memory.
	s16				ADMATempBuffer[0x1000];

// ----------------------------------------------------------------------------------
//  V_Core Methods
// ----------------------------------------------------------------------------------

	// uninitialized constructor
	V_Core() : Index( -1 ), DMAPtr( NULL ) {}
	V_Core( int idx );			// our badass constructor
	virtual ~V_Core() throw();

	void	Reset( int index );
	void	UpdateEffectsBufferSize();

	s32		EffectsBufferIndexer( s32 offset ) const;
	void	UpdateFeedbackBuffersA();
	void	UpdateFeedbackBuffersB();

	void	WriteRegPS1( u32 mem, u16 value );
	u16		ReadRegPS1( u32 mem );
	
// --------------------------------------------------------------------------------------
//  Mixer Section
// --------------------------------------------------------------------------------------

	StereoOut32 Mix( const VoiceMixSet& inVoices, const StereoOut32& Input, const StereoOut32& Ext );
	void		Reverb_AdvanceBuffer();
	StereoOut32 DoReverb( const StereoOut32& Input );
	s32			RevbGetIndexer( s32 offset );

	StereoOut32 ReadInput();
	StereoOut32 ReadInputPV();
	StereoOut32 ReadInput_HiFi( bool isCDDA );
	
// --------------------------------------------------------------------------
//  DMA Section
// --------------------------------------------------------------------------

	// Returns the index of the DMA channel (4 for Core 0, or 7 for Core 1)
	int GetDmaIndex() const
	{
		return (Index == 0) ? 4 : 7;
	}

	// returns either '4' or '7'
	char GetDmaIndexChar() const
	{
		return 0x30 + GetDmaIndex();
	}

	__forceinline u16 DmaRead()
	{
		const u16 ret = (u16)spu2M_Read(TDA);
		++TDA; TDA &= 0xfffff;
		return ret;
	}

	__forceinline void DmaWrite(u16 value)
	{
		spu2M_Write( TSA, value );
		++TSA; TSA &= 0xfffff;
	}

	void LogAutoDMA( FILE* fp );
	
	void DoDMAwrite(u16* pMem, u32 size);
	void DoDMAread(u16* pMem, u32 size);

	void AutoDMAReadBuffer(int mode);
	void StartADMAWrite(u16 *pMem, u32 sz);
	void PlainDMAWrite(u16 *pMem, u32 sz);
};

extern V_Core	Cores[2];
extern V_SPDIF	Spdif;

// Output Buffer Writing Position (the same for all data);
extern s16		OutPos;
// Input Buffer Reading Position (the same for all data);
extern s16		InputPos;
// SPU Mixing Cycles ("Ticks mixed" counter)
extern u32		Cycles;

extern short*	spu2regs;
extern short*	_spu2mem;
extern int		PlayMode;

extern void SetIrqCall();
extern void StartVoices(int core, u32 value);
extern void StopVoices(int core, u32 value);
extern void InitADSR();
extern void CalculateADSR( V_Voice& vc );

extern void spdif_set51(u32 is_5_1_out);
extern u32  spdif_init();
extern void spdif_shutdown();
extern void spdif_get_samples(s32 *samples); // fills the buffer with [l,r,c,lfe,sl,sr] if using 5.1 output, or [l,r] if using stereo
extern void UpdateSpdifMode();

namespace Savestate
{
	struct DataBlock;

	extern s32 __fastcall FreezeIt( DataBlock& spud );
	extern s32 __fastcall ThawIt( DataBlock& spud );
	extern s32 __fastcall SizeIt();
}

// --------------------------------------------------------------------------------------
//  ADPCM Decoder Cache
// --------------------------------------------------------------------------------------

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

extern PcmCacheEntry* pcm_cache_data;
