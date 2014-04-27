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

#include "Global.h"

// Games have turned out to be surprisingly sensitive to whether a parked, silent voice is being fully emulated.
// With Silent Hill: Shattered Memories requiring full processing for no obvious reason, we've decided to
// disable the optimisation until we can tie it to the game database.
#define NEVER_SKIP_VOICES 1

void ADMAOutLogWrite(void *lpData, u32 ulSize);

static const s32 tbl_XA_Factor[16][2] =
{
	{    0,   0 },
	{   60,   0 },
	{  115, -52 },
	{   98, -55 },
	{  122, -60 }
};


// Performs a 64-bit multiplication between two values and returns the
// high 32 bits as a result (discarding the fractional 32 bits).
// The combined fractional bits of both inputs must be 32 bits for this
// to work properly.
//
// This is meant to be a drop-in replacement for times when the 'div' part
// of a MulDiv is a constant.  (example: 1<<8, or 4096, etc)
//
// [Air] Performance breakdown: This is over 10 times faster than MulDiv in
//   a *worst case* scenario.  It's also more accurate since it forces the
//   caller to  extend the inputs so that they make use of all 32 bits of
//   precision.
//
static __forceinline s32 MulShr32( s32 srcval, s32 mulval )
{
	return (s64)srcval * mulval >> 32;
}

__forceinline s32 clamp_mix( s32 x, u8 bitshift )
{
	return GetClamped( x, -0x8000<<bitshift, 0x7fff<<bitshift );
}

#if _MSC_VER
__forceinline
// Without the keyword static, gcc compilation fails on the inlining...
// Unfortunately the function is also used in Reverb.cpp. In order to keep the code 
// clean we just disable it.
// We will need link-time code generation / Whole Program optimization to do a clean
// inline. Gcc 4.5 has the experimental options -flto, -fwhopr and -fwhole-program to
// do it but it still experimental...
#endif
StereoOut32 clamp_mix( const StereoOut32& sample, u8 bitshift )
{
	// We should clampify between -0x8000 and 0x7fff, however some audio output
	// modules or sound drivers could (will :p) overshoot with that. So giving it a small safety.
	
	return StereoOut32(
		GetClamped( sample.Left, -0x7f00<<bitshift, 0x7f00<<bitshift ),
		GetClamped( sample.Right, -0x7f00<<bitshift, 0x7f00<<bitshift )
	);
}

static void __forceinline XA_decode_block(s16* buffer, const s16* block, s32& prev1, s32& prev2)
{
	const s32 header = *block;
	const s32 shift =  (header&0xF)+16;
	const int id = header >> 4 & 0xF;
	if (id > 4 && MsgToConsole())
		ConLog("* SPU2-X: Unknown ADPCM coefficients table id %d\n", id);
	const s32 pred1 = tbl_XA_Factor[id][0];
	const s32 pred2 = tbl_XA_Factor[id][1];

	const s8* blockbytes	= (s8*)&block[1];
	const s8* blockend		= &blockbytes[13];

	for(; blockbytes<=blockend; ++blockbytes)
	{
		s32 data	= ((*blockbytes)<<28) & 0xF0000000;
		s32 pcm		= (data >> shift) + (((pred1*prev1)+(pred2*prev2)+32) >> 6);

		Clampify( pcm, -0x8000, 0x7fff );
		*(buffer++) = pcm;

		data		= ((*blockbytes)<<24) & 0xF0000000;
		s32 pcm2	= (data >> shift) + (((pred1*pcm)+(pred2*prev1)+32) >> 6);

		Clampify( pcm2, -0x8000, 0x7fff );
		*(buffer++) = pcm2;

		prev2 = pcm;
		prev1 = pcm2;
	}
}

static void __forceinline IncrementNextA(V_Core& thiscore, uint voiceidx)
{
	V_Voice &vc(thiscore.Voices[voiceidx]);

	// Important!  Both cores signal IRQ when an address is read, regardless of
	// which core actually reads the address.

	for( uint i=0; i<2; i++ )
	{
		if( Cores[i].IRQEnable && (vc.NextA==Cores[i].IRQA ) )
		{
			//if( IsDevBuild )
			//	ConLog(" * SPU2 Core %d: IRQ Requested (IRQA (%05X) passed; voice %d).\n", i, Cores[i].IRQA, thiscore.Index * 24 + voiceidx);

			SetIrqCall(i);
		}
	}

	vc.NextA++;
	vc.NextA&=0xFFFFF;
}

// decoded pcm data, used to cache the decoded data so that it needn't be decoded
// multiple times.  Cache chunks are decoded when the mixer requests the blocks, and
// invalided when DMA transfers and memory writes are performed.
PcmCacheEntry *pcm_cache_data = NULL;

int g_counter_cache_hits = 0;
int g_counter_cache_misses = 0;
int g_counter_cache_ignores = 0;

// LOOP/END sets the ENDX bit and sets NAX to LSA, and the voice is muted if LOOP is not set
// LOOP seems to only have any effect on the block with LOOP/END set, where it prevents muting the voice
// (the documented requirement that every block in a loop has the LOOP bit set is nonsense according to tests)
// LOOP/START sets LSA to NAX unless LSA was written manually since sound generation started
// (see LoopMode, the method by which this is achieved on the real SPU2 is unknown)
#define XAFLAG_LOOP_END		(1ul<<0)
#define XAFLAG_LOOP			(1ul<<1)
#define XAFLAG_LOOP_START	(1ul<<2)

static __forceinline s32 GetNextDataBuffered( V_Core& thiscore, uint voiceidx )
{
	V_Voice& vc( thiscore.Voices[voiceidx] );

	if( (vc.SCurrent&3) == 0 )
	{
		IncrementNextA( thiscore, voiceidx );
		
		if ((vc.NextA & 7) == 0) // vc.SCurrent == 24 equivalent
		{
			if(vc.LoopFlags & XAFLAG_LOOP_END)
			{
				thiscore.Regs.ENDX |= (1 << voiceidx);
				vc.NextA = vc.LoopStartA | 1;
				if (!(vc.LoopFlags & XAFLAG_LOOP))
				{
					vc.Stop();

					if( IsDevBuild )
					{
						if(MsgVoiceOff()) ConLog("* SPU2-X: Voice Off by EndPoint: %d \n", voiceidx);
					}
				}
			}
			else
				vc.NextA++; // no, don't IncrementNextA here.  We haven't read the header yet.
		}
	}

	if( vc.SCurrent == 28 )
	{
		vc.SCurrent = 0;

		// We'll need the loop flags and buffer pointers regardless of cache status:

		for (int i=0; i<2; i++)
			if (Cores[i].IRQEnable && Cores[i].IRQA == (vc.NextA & 0xFFFF8))
				SetIrqCall(i);

		s16* memptr = GetMemPtr(vc.NextA & 0xFFFF8);
		vc.LoopFlags = *memptr >> 8;	// grab loop flags from the upper byte.

		if( (vc.LoopFlags & XAFLAG_LOOP_START) && !vc.LoopMode )
			vc.LoopStartA = vc.NextA & 0xFFFF8;

		const int cacheIdx = vc.NextA / pcm_WordsPerBlock;
		PcmCacheEntry& cacheLine = pcm_cache_data[cacheIdx];
		vc.SBuffer = cacheLine.Sampledata;

		if( cacheLine.Validated )
		{
			// Cached block!  Read from the cache directly.
			// Make sure to propagate the prev1/prev2 ADPCM:

			vc.Prev1 = vc.SBuffer[27];
			vc.Prev2 = vc.SBuffer[26];

			//ConLog( "* SPU2-X: Cache Hit! NextA=0x%x, cacheIdx=0x%x\n", vc.NextA, cacheIdx );

			if( IsDevBuild )
				g_counter_cache_hits++;
		}
		else
		{
			// Only flag the cache if it's a non-dynamic memory range.
			if( vc.NextA >= SPU2_DYN_MEMLINE )
				cacheLine.Validated = true;

			if( IsDevBuild )
			{
				if( vc.NextA < SPU2_DYN_MEMLINE )
					g_counter_cache_ignores++;
				else
					g_counter_cache_misses++;
			}

			XA_decode_block( vc.SBuffer, memptr, vc.Prev1, vc.Prev2 );
		}
	}

	return vc.SBuffer[vc.SCurrent++];
}

static __forceinline void GetNextDataDummy(V_Core& thiscore, uint voiceidx)
{
	V_Voice& vc( thiscore.Voices[voiceidx] );

	IncrementNextA( thiscore, voiceidx );
	
	if ((vc.NextA & 7) == 0) // vc.SCurrent == 24 equivalent
	{
		if(vc.LoopFlags & XAFLAG_LOOP_END)
		{
			thiscore.Regs.ENDX |= (1 << voiceidx);
			vc.NextA = vc.LoopStartA | 1;
		}
		else
			vc.NextA++; // no, don't IncrementNextA here.  We haven't read the header yet.
	}

	if (vc.SCurrent == 28)
	{
		for (int i=0; i<2; i++)
			if (Cores[i].IRQEnable && Cores[i].IRQA == (vc.NextA & 0xFFFF8))
				SetIrqCall(i);

		vc.LoopFlags = *GetMemPtr(vc.NextA&0xFFFF8) >> 8;	// grab loop flags from the upper byte.

		if ((vc.LoopFlags & XAFLAG_LOOP_START) && !vc.LoopMode)
			vc.LoopStartA = vc.NextA & 0xFFFF8;

		vc.SCurrent = 0;
	}

	vc.SP -= 4096 * (4 - (vc.SCurrent & 3));
	vc.SCurrent += 4 - (vc.SCurrent & 3);
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

static s32 __forceinline GetNoiseValues()
{
	static s32 Seed = 0x41595321;
	s32 retval = 0x8000;
	
	if( Seed&0x100 )
		retval = (Seed&0xff) << 8;
	else if( Seed&0xffff )
		retval = 0x7fff;
#ifdef _WIN32
	__asm {
		MOV eax,Seed
		ROR eax,5
		XOR eax,0x9a
		MOV ebx,eax
		ROL eax,2
		ADD eax,ebx
		XOR eax,ebx
		ROR eax,3
		MOV Seed,eax
	}
#else
	__asm__ (
		".intel_syntax\n"
		"MOV %%eax,%1\n"
		"ROR %%eax,5\n"
		"XOR %%eax,0x9a\n"
		"MOV %%esi,%%eax\n"
		"ROL %%eax,2\n"
		"ADD %%eax,%%esi\n"
		"XOR %%eax,%%esi\n"
		"ROR %%eax,3\n"
		"MOV %0,%%eax\n"
		".att_syntax\n" : "=r"(Seed) :"r"(Seed)
		: "%eax", "%esi"
		);
#endif
	return retval;
}
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

// Data is expected to be 16 bit signed (typical stuff!).
// volume is expected to be 32 bit signed (31 bits with reverse phase)
// Data is shifted up by 1 bit to give the output an effective 16 bit range.
static __forceinline s32 ApplyVolume(s32 data, s32 volume)
{
	//return (volume * data) >> 15;
	return MulShr32( data<<1, volume );
}

static __forceinline StereoOut32 ApplyVolume( const StereoOut32& data, const V_VolumeLR& volume )
{
	return StereoOut32(
		ApplyVolume( data.Left, volume.Left ),
		ApplyVolume( data.Right, volume.Right )
	);
}

static __forceinline StereoOut32 ApplyVolume( const StereoOut32& data, const V_VolumeSlideLR& volume )
{
	return StereoOut32(
		ApplyVolume( data.Left, volume.Left.Value ),
		ApplyVolume( data.Right, volume.Right.Value )
	);
}

static void __forceinline UpdatePitch( uint coreidx, uint voiceidx )
{
	V_Voice& vc( Cores[coreidx].Voices[voiceidx] );
	s32 pitch;

	// [Air] : re-ordered comparisons: Modulated is much more likely to be zero than voice,
	//   and so the way it was before it's have to check both voice and modulated values
	//   most of the time.  Now it'll just check Modulated and short-circuit past the voice
	//   check (not that it amounts to much, but eh every little bit helps).
	if( (vc.Modulated==0) || (voiceidx==0) )
		pitch = vc.Pitch;
	else
		pitch = GetClamped((vc.Pitch*(32768 + Cores[coreidx].Voices[voiceidx-1].OutX))>>15, 0, 0x3fff);

	vc.SP+=pitch;
}


static __forceinline void CalculateADSR( V_Core& thiscore, uint voiceidx )
{
	V_Voice& vc( thiscore.Voices[voiceidx] );

	if( vc.ADSR.Phase==0 )
	{
		vc.ADSR.Value = 0;
		return;
	}

	if( !vc.ADSR.Calculate() )
	{
		if( IsDevBuild )
		{
			if(MsgVoiceOff()) ConLog("* SPU2-X: Voice Off by ADSR: %d \n", voiceidx);
		}
		vc.Stop();
	}

	jASSUME( vc.ADSR.Value >= 0 );	// ADSR should never be negative...
}

/*
   Tension: 65535 is high, 32768 is normal, 0 is low
*/
template<s32 i_tension>
 __forceinline
static s32 HermiteInterpolate(
	s32 y0, // 16.0
	s32 y1, // 16.0
	s32 y2, // 16.0
	s32 y3, // 16.0
	s32 mu  //  0.12
	)
{
	s32 m00 = ((y1-y0)*i_tension) >> 16; // 16.0
	s32 m01 = ((y2-y1)*i_tension) >> 16; // 16.0
	s32 m0  = m00 + m01;

	s32 m10 = ((y2-y1)*i_tension) >> 16; // 16.0
	s32 m11 = ((y3-y2)*i_tension) >> 16; // 16.0
	s32 m1  = m10 + m11;

	s32 val = ((  2*y1 +   m0 + m1 - 2*y2) * mu) >> 12; // 16.0
	val = ((val - 3*y1 - 2*m0 - m1 + 3*y2) * mu) >> 12; // 16.0
	val = ((val        +   m0            ) * mu) >> 11; // 16.0

	return(val + (y1<<1));
}

__forceinline
static s32 CatmullRomInterpolate(
	s32 y0, // 16.0
	s32 y1, // 16.0
	s32 y2, // 16.0
	s32 y3, // 16.0
	s32 mu  //  0.12
	)
{
	//q(t) = 0.5 *(    	(2 * P1) +
	//	(-P0 + P2) * t +
	//	(2*P0 - 5*P1 + 4*P2 - P3) * t2 +
	//	(-P0 + 3*P1- 3*P2 + P3) * t3)

	s32 a3 = (-  y0 + 3*y1 - 3*y2 + y3);
	s32 a2 = ( 2*y0 - 5*y1 + 4*y2 - y3);
	s32 a1 = (-  y0        +   y2     );
	s32 a0 = (        2*y1            );

	s32 val = ((a3  ) * mu) >> 12;
	val = ((a2 + val) * mu) >> 12;
	val = ((a1 + val) * mu) >> 12;

	return (a0 + val);
}

__forceinline
static s32 CubicInterpolate(
	s32 y0, // 16.0
	s32 y1, // 16.0
	s32 y2, // 16.0
	s32 y3, // 16.0
	s32 mu  //  0.12
	)
{
	const s32 a0 = y3 - y2 - y0 + y1;
	const s32 a1 = y0 - y1 - a0;
	const s32 a2 = y2 - y0;

	s32 val = ((  a0) * mu) >> 12;
	val = ((val + a1) * mu) >> 12;
	val = ((val + a2) * mu) >> 11;

	return(val + (y1<<1));
}

// Returns a 16 bit result in Value.
// Uses standard template-style optimization techniques to statically generate five different
// versions of this function (one for each type of interpolation).
template< int InterpType >
static __forceinline s32 GetVoiceValues( V_Core& thiscore, uint voiceidx )
{
	V_Voice& vc( thiscore.Voices[voiceidx] );

	while( vc.SP > 0 )
	{
		if( InterpType >= 2 )
		{
			vc.PV4 = vc.PV3;
			vc.PV3 = vc.PV2;
		}
		vc.PV2 = vc.PV1;
		vc.PV1 = GetNextDataBuffered( thiscore, voiceidx );
		vc.SP -= 4096;
	}

	const s32 mu = vc.SP + 4096;

	switch( InterpType )
	{
		case 0: return vc.PV1<<1;
		case 1: return (vc.PV1<<1) - (( (vc.PV2 - vc.PV1) * vc.SP)>>11);

		case 2: return CubicInterpolate				(vc.PV4, vc.PV3, vc.PV2, vc.PV1, mu);
		case 3: return HermiteInterpolate<16384>	(vc.PV4, vc.PV3, vc.PV2, vc.PV1, mu);
		case 4: return CatmullRomInterpolate		(vc.PV4, vc.PV3, vc.PV2, vc.PV1, mu);

		jNO_DEFAULT;
	}

	return 0;		// technically unreachable!
}

// Noise values need to be mixed without going through interpolation, since it
// can wreak havoc on the noise (causing muffling or popping).  Not that this noise
// generator is accurate in its own right.. but eh, ah well :)
static __forceinline s32 GetNoiseValues( V_Core& thiscore, uint voiceidx )
{
	V_Voice& vc( thiscore.Voices[voiceidx] );

	s32 retval = GetNoiseValues();

	/*while(vc.SP>=4096)
	{
		retval = GetNoiseValues();
		vc.SP-=4096;
	}*/

	// GetNoiseValues can't set the phase zero on us unexpectedly
	// like GetVoiceValues can.  Better assert just in case though..
	jASSUME( vc.ADSR.Phase != 0 );

	return retval;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

// writes a signed value to the SPU2 ram
// Performs no cache invalidation -- use only for dynamic memory ranges
// of the SPU2 (between 0x0000 and SPU2_DYN_MEMLINE)
static __forceinline void spu2M_WriteFast( u32 addr, s16 value )
{ 
	// Fixes some of the oldest hangs in pcsx2's history! :p
	for( uint i=0; i<2; i++ )
	{
		if( Cores[i].IRQEnable && Cores[i].IRQA == addr )
		{
			//printf("Core %d special write IRQ Called (IRQ passed). IRQA = %x\n",i,addr);
			SetIrqCall(i);
		}
	}
	// throw an assertion if the memory range is invalid:
#ifndef DEBUG_FAST
	jASSUME( addr < SPU2_DYN_MEMLINE );
#endif
	*GetMemPtr( addr ) = value;
}


static __forceinline StereoOut32 MixVoice( uint coreidx, uint voiceidx )
{
	V_Core& thiscore( Cores[coreidx] );
	V_Voice& vc( thiscore.Voices[voiceidx] );

	// If this assertion fails, it mans SCurrent is being corrupted somewhere, or is not initialized
	// properly.  Invalid values in SCurrent will cause errant IRQs and corrupted audio.
	pxAssertMsg( (vc.SCurrent <= 28) && (vc.SCurrent != 0), "Current sample should always range from 1->28" );

	// Most games don't use much volume slide effects.  So only call the UpdateVolume
	// methods when needed by checking the flag outside the method here...
	// (Note: Ys 6 : Ark of Nephistm uses these effects)

	vc.Volume.Update();

	// SPU2 Note: The spu2 continues to process voices for eternity, always, so we
	// have to run through all the motions of updating the voice regardless of it's
	// audible status.  Otherwise IRQs might not trigger and emulation might fail.

	if( vc.ADSR.Phase > 0 )
	{
		UpdatePitch( coreidx, voiceidx );

		s32 Value = 0;

		if( vc.Noise )
			Value = GetNoiseValues( thiscore, voiceidx );
		else
		{
			// Optimization : Forceinline'd Templated Dispatch Table.  Any halfwit compiler will
			// turn this into a clever jump dispatch table (no call/rets, no compares, uber-efficient!)

			switch( Interpolation )
			{
				case 0: Value = GetVoiceValues<0>( thiscore, voiceidx ); break;
				case 1: Value = GetVoiceValues<1>( thiscore, voiceidx ); break;
				case 2: Value = GetVoiceValues<2>( thiscore, voiceidx ); break;
				case 3: Value = GetVoiceValues<3>( thiscore, voiceidx ); break;
				case 4: Value = GetVoiceValues<4>( thiscore, voiceidx ); break;

				jNO_DEFAULT;
			}
		}

		// Update and Apply ADSR  (applies to normal and noise sources)
		//
		// Note!  It's very important that ADSR stay as accurate as possible.  By the way
		// it is used, various sound effects can end prematurely if we truncate more than
		// one or two bits.  Best result comes from no truncation at all, which is why we
		// use a full 64-bit multiply/result here.

		CalculateADSR( thiscore, voiceidx );
		Value	= MulShr32( Value, vc.ADSR.Value );
		
		// Store Value for eventual modulation later
		// Pseudonym's Crest calculation idea. Actually calculates a crest, unlike the old code which was just peak.
		if(vc.PV1 < vc.NextCrest)
		{
			vc.OutX = MulShr32(vc.NextCrest, vc.ADSR.Value);
			vc.NextCrest = -0x8000;
		}
		if(vc.PV1 > vc.PV2)
		{
			vc.NextCrest = vc.PV1;
		}

		if( IsDevBuild )
			DebugCores[coreidx].Voices[voiceidx].displayPeak = std::max(DebugCores[coreidx].Voices[voiceidx].displayPeak,(s32)vc.OutX);

		// Write-back of raw voice data (post ADSR applied)

		if (voiceidx==1)      spu2M_WriteFast( ( (0==coreidx) ? 0x400 : 0xc00 ) + OutPos, vc.OutX );
		else if (voiceidx==3) spu2M_WriteFast( ( (0==coreidx) ? 0x600 : 0xe00 ) + OutPos, vc.OutX );
			
		return ApplyVolume( StereoOut32( Value, Value ), vc.Volume );
	}
	else
	{
		// Continue processing voice, even if it's "off". Or else we miss interrupts! (Fatal Frame engine died because of this.)
		if (NEVER_SKIP_VOICES
			|| (*GetMemPtr(vc.NextA & 0xFFFF8) >> 8 & 3) != 3 || vc.LoopStartA != (vc.NextA & ~7)		// not in a tight loop
			|| (Cores[0].IRQEnable && (Cores[0].IRQA & ~7) == vc.LoopStartA)	// or should be interrupting regularly
			|| (Cores[1].IRQEnable && (Cores[1].IRQA & ~7) == vc.LoopStartA)
			|| !(thiscore.Regs.ENDX & 1 << voiceidx))						// or isn't currently flagged as having passed the endpoint
		{
			UpdatePitch(coreidx, voiceidx);

			while (vc.SP > 0)
				GetNextDataDummy(thiscore, voiceidx); // Dummy is enough
		}

		// Write-back of raw voice data (some zeros since the voice is "dead")
		if (voiceidx==1)      spu2M_WriteFast( ( (0==coreidx) ? 0x400 : 0xc00 ) + OutPos, 0 );
		else if (voiceidx==3) spu2M_WriteFast( ( (0==coreidx) ? 0x600 : 0xe00 ) + OutPos, 0 );

		return StereoOut32( 0, 0 );
	}
}

const VoiceMixSet VoiceMixSet::Empty( (StereoOut32()), (StereoOut32()) );	// Don't use SteroOut32::Empty because C++ doesn't make any dep/order checks on global initializers.

static __forceinline void MixCoreVoices( VoiceMixSet& dest, const uint coreidx )
{
	V_Core& thiscore( Cores[coreidx] );

	for( uint voiceidx=0; voiceidx<V_Core::NumVoices; ++voiceidx )
	{
		StereoOut32 VVal( MixVoice( coreidx, voiceidx ) );

		// Note: Results from MixVoice are ranged at 16 bits.

		dest.Dry.Left	+= VVal.Left	& thiscore.VoiceGates[voiceidx].DryL;
		dest.Dry.Right	+= VVal.Right	& thiscore.VoiceGates[voiceidx].DryR;
		dest.Wet.Left	+= VVal.Left	& thiscore.VoiceGates[voiceidx].WetL;
		dest.Wet.Right	+= VVal.Right	& thiscore.VoiceGates[voiceidx].WetR;
	}
}

StereoOut32 V_Core::Mix( const VoiceMixSet& inVoices, const StereoOut32& Input, const StereoOut32& Ext )
{
	MasterVol.Update();

	// Saturate final result to standard 16 bit range.
	const VoiceMixSet Voices( clamp_mix( inVoices.Dry ), clamp_mix( inVoices.Wet ) );

	// Write Mixed results To Output Area
	spu2M_WriteFast( ( (0==Index) ? 0x1000 : 0x1800 ) + OutPos, Voices.Dry.Left );
	spu2M_WriteFast( ( (0==Index) ? 0x1200 : 0x1A00 ) + OutPos, Voices.Dry.Right );
	spu2M_WriteFast( ( (0==Index) ? 0x1400 : 0x1C00 ) + OutPos, Voices.Wet.Left );
	spu2M_WriteFast( ( (0==Index) ? 0x1600 : 0x1E00 ) + OutPos, Voices.Wet.Right );

	// Write mixed results to logfile (if enabled)

	WaveDump::WriteCore( Index, CoreSrc_DryVoiceMix, Voices.Dry );
	WaveDump::WriteCore( Index, CoreSrc_WetVoiceMix, Voices.Wet );

	// Mix in the Input data

	StereoOut32 TD(
		Input.Left	& DryGate.InpL,
		Input.Right	& DryGate.InpR
	);

	// Mix in the Voice data
	TD.Left		+= Voices.Dry.Left	& DryGate.SndL;
	TD.Right	+= Voices.Dry.Right	& DryGate.SndR;

	// Mix in the External (nothing/core0) data
	TD.Left		+= Ext.Left			& DryGate.ExtL;
	TD.Right	+= Ext.Right		& DryGate.ExtR;

	// User-level Effects disabling.  Nice speedup but breaks games that depend on
	// reverb IRQs (very few -- if you find one name it here!).
	if( EffectsDisabled ) return TD;

	// ----------------------------------------------------------------------------
	//    Reverberation Effects Processing
	// ----------------------------------------------------------------------------
	// SPU2 has an FxEnable bit which seems to disable all reverb processing *and*
	// output, but does *not* disable the advancing buffers.  IRQs are not triggered
	// and reverb is rendered silent.
	//
	// Technically we should advance the buffers even when fx are disabled.  However
	// there are two things that make this very unlikely to matter:
	//
	//  1. Any SPU2 app wanting to avoid noise or pops needs to clear the reverb buffers
	//     when adjusting settings anyway; so the read/write positions in the reverb
	//     buffer after FxEnabled is set back to 1 doesn't really matter.
	//
	//  2. Writes to ESA (and possibly EEA) reset the buffer pointers to 0.
	//
	// On the other hand, updating the buffer is cheap and easy, so might as well. ;) 

	Reverb_AdvanceBuffer(); // Updates the reverb work area as well, if needed.
	
	// ToDo:
	// Bad EndA causes memory corruption. Bad for us, unknown on PS2!
	if (!FxEnable || EffectsEndA >= 0x100000) return TD;

	StereoOut32 TW;

	// Mix Input, Voice, and External data:

	TW.Left		 = Input.Left		& WetGate.InpL;
	TW.Right	 = Input.Right		& WetGate.InpR;

	TW.Left		+= Voices.Wet.Left	& WetGate.SndL;
	TW.Right	+= Voices.Wet.Right	& WetGate.SndR;
	TW.Left		+= Ext.Left			& WetGate.ExtL;
	TW.Right	+= Ext.Right		& WetGate.ExtR;

	WaveDump::WriteCore( Index, CoreSrc_PreReverb, TW );

	StereoOut32 RV = DoReverb( TW );

	WaveDump::WriteCore( Index, CoreSrc_PostReverb, RV );

	// Mix Dry + Wet
	// (master volume is applied later to the result of both outputs added together).
	return TD + ApplyVolume( RV, FxVol );
}

// Filters that work on the final output to de-alias and equlize it.
// Taken from http://nenolod.net/projects/upse/
#define OVERALL_SCALE (0.87f)

StereoOut32 Apply_Frequency_Response_Filter(StereoOut32 &SoundStream)
{
	static FrequencyResponseFilter FRF = FrequencyResponseFilter();

	s32 in, out;
	s32 l, r;
	s32 mid, side;

	l = SoundStream.Left;
	r = SoundStream.Right;

	mid  = l + r;
	side = l - r;

	in = mid;
	out = FRF.la0 * in + FRF.la1 * FRF.lx1 + FRF.la2 * FRF.lx2 - FRF.lb1 * FRF.ly1 - FRF.lb2 * FRF.ly2;

	FRF.lx2 = FRF.lx1;
	FRF.lx1 = in;

	FRF.ly2 = FRF.ly1;
	FRF.ly1 = out;

	mid = out;

	l = ((0.5) * (OVERALL_SCALE)) * (mid + side);
	r = ((0.5) * (OVERALL_SCALE)) * (mid - side);

	in = l;
	out = FRF.ha0 * in + FRF.ha1 * FRF.History_One_In.Left + FRF.ha2 * FRF.History_Two_In.Left - FRF.hb1 * FRF.History_One_Out.Left - FRF.hb2 * FRF.History_Two_Out.Left;
	FRF.History_Two_In.Left = FRF.History_One_In.Left; FRF.History_One_In.Left = in;
	FRF.History_Two_Out.Left = FRF.History_One_Out.Left; FRF.History_One_Out.Left = out;
	l = out;

	in = r;
	out = FRF.ha0 * in + FRF.ha1 * FRF.History_One_In.Right + FRF.ha2 * FRF.History_Two_In.Right - FRF.hb1 * FRF.History_One_Out.Right - FRF.hb2 * FRF.History_Two_Out.Right;
	FRF.History_Two_In.Right = FRF.History_One_In.Right; FRF.History_One_In.Right = in;
	FRF.History_Two_Out.Right = FRF.History_One_Out.Right; FRF.History_One_Out.Right = out;
	r = out;

	//clamp_mix(l);
	//clamp_mix(r);

	SoundStream.Left = l;
	SoundStream.Right = r;

	return SoundStream;
}

StereoOut32 Apply_Dealias_Filter(StereoOut32 &SoundStream)
{
	static StereoOut32 Old = StereoOut32::Empty;

	s32 l, r;

	l = SoundStream.Left;
	r = SoundStream.Right;

	l += (l - Old.Left);
	r += (r - Old.Right);

	Old.Left = SoundStream.Left;
	Old.Right = SoundStream.Right;

	SoundStream.Left = l;
	SoundStream.Right = r;

	return SoundStream;
}

// used to throttle the output rate of cache stat reports
static int p_cachestat_counter=0;

// Gcc does not want to inline it when lto is enabled because some functions growth too much.
// The function is big enought to see any speed impact. -- Gregory
#ifndef __LINUX__
__forceinline
#endif
void Mix()
{
	// Note: Playmode 4 is SPDIF, which overrides other inputs.
	StereoOut32 InputData[2] =
	{
		// SPDIF is on Core 0:
		// Fixme: 
		// 1. We do not have an AC3 decoder for the bitstream. 
		// 2. Games usually provide a normal ADMA stream as well and want to see it getting read!
		/*(PlayMode&4) ? StereoOut32::Empty : */ApplyVolume( Cores[0].ReadInput(), Cores[0].InpVol ),

		// CDDA is on Core 1:
		(PlayMode&8) ? StereoOut32::Empty : ApplyVolume( Cores[1].ReadInput(), Cores[1].InpVol )
	};

	WaveDump::WriteCore( 0, CoreSrc_Input, InputData[0] );
	WaveDump::WriteCore( 1, CoreSrc_Input, InputData[1] );

	// Todo: Replace me with memzero initializer!
	VoiceMixSet VoiceData[2] = { VoiceMixSet::Empty, VoiceMixSet::Empty };	// mixed voice data for each core.
	MixCoreVoices( VoiceData[0], 0 );
	MixCoreVoices( VoiceData[1], 1 );

	StereoOut32 Ext( Cores[0].Mix( VoiceData[0], InputData[0], StereoOut32::Empty ) );

	if( (PlayMode & 4) || (Cores[0].Mute!=0) )
		Ext = StereoOut32::Empty;
	else
	{
		Ext = clamp_mix( ApplyVolume( Ext, Cores[0].MasterVol ) );
	}

	// Commit Core 0 output to ram before mixing Core 1:

	spu2M_WriteFast( 0x800 + OutPos, Ext.Left );
	spu2M_WriteFast( 0xA00 + OutPos, Ext.Right );
	WaveDump::WriteCore( 0, CoreSrc_External, Ext );

	ApplyVolume( Ext, Cores[1].ExtVol );
	StereoOut32 Out( Cores[1].Mix( VoiceData[1], InputData[1], Ext ) );

	if( PlayMode & 8 )
	{
		// Experimental CDDA support
		// The CDDA overrides all other mixer output.  It's a direct feed!

		Out = Cores[1].ReadInput_HiFi();
		//WaveLog::WriteCore( 1, "CDDA-32", OutL, OutR );
	}
	else
	{
		Out.Left = MulShr32( Out.Left<<(SndOutVolumeShift+1), Cores[1].MasterVol.Left.Value );
		Out.Right = MulShr32( Out.Right<<(SndOutVolumeShift+1), Cores[1].MasterVol.Right.Value );

		#ifdef DEBUG_KEYS
		if(postprocess_filter_enabled)
		#endif
		{
			if(postprocess_filter_dealias)
			{
				// Dealias filter emphasizes the highs too much.
				Out = Apply_Dealias_Filter ( Out );
			}
			Out = Apply_Frequency_Response_Filter ( Out );
		}

		// Final Clamp!
		// Like any good audio system, the PS2 pumps the volume and incurs some distortion in its
		// output, giving us a nice thumpy sound at times.  So we add 1 above (2x volume pump) and
		// then clamp it all here.
		
		// Edit: I'm sorry Jake, but I know of no good audio system that arbitrary distorts and clips
		// output by design.
		// Good thing though that this code gets the volume exactly right, as per tests :)
		Out = clamp_mix( Out, SndOutVolumeShift );
	}
	
	// Configurable output volume
	Out.Left *= FinalVolume;
	Out.Right *= FinalVolume;
	
	SndBuffer::Write( Out );

	// Update AutoDMA output positioning
	OutPos++;
	if (OutPos>=0x200) OutPos=0;

	if( IsDevBuild )
	{
		p_cachestat_counter++;
		if(p_cachestat_counter > (48000*10) )
		{
			p_cachestat_counter = 0;
			if( MsgCache() ) ConLog( " * SPU2 > CacheStats > Hits: %d  Misses: %d  Ignores: %d\n",
				g_counter_cache_hits,
				g_counter_cache_misses,
				g_counter_cache_ignores );

			g_counter_cache_hits =
			g_counter_cache_misses =
			g_counter_cache_ignores = 0;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

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

Air notes:
  The above is effectivly the same as:
    buffer[MIX_DEST_B0] = (ACC0 * FB_ALPHA) + (FB_A0 * (1.0-FB_ALPHA)) - FB_B0 * FB_X;
    buffer[MIX_DEST_B1] = (ACC1 * FB_ALPHA) + (FB_A1 * (1.0-FB_ALPHA)) - FB_B1 * FB_X;

  Which reduces to:
    buffer[MIX_DEST_B0] = FB_A0 + ((ACC0-FB_A0) * FB_ALPHA) - FB_B0 * FB_X;
    buffer[MIX_DEST_B1] = FB_A1 + ((ACC1-FB_A1) * FB_ALPHA) - FB_B1 * FB_X;



-----------------------------------------------------------------------------
*/
