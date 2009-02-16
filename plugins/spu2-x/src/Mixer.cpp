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

#include "spu2.h"
#include <float.h>

extern void	spdif_update();

void ADMAOutLogWrite(void *lpData, u32 ulSize);

u32 core, voice;

static const s32 tbl_XA_Factor[5][2] =
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
__forceinline s32 MulShr32( s32 srcval, s32 mulval )
{
	s64 tmp = ((s64)srcval * mulval );
	return ((s32*)&tmp)[1];

	// Performance note: Using the temp var and memory reference
	// actually ends up being roughly 2x faster than using a bitshift.
	// It won't fly on big endian machines though... :)
}

__forceinline s32 clamp_mix(s32 x, u8 bitshift)
{
	return GetClamped( x, -0x8000<<bitshift, 0x7fff<<bitshift );
}

static void __forceinline XA_decode_block(s16* buffer, const s16* block, s32& prev1, s32& prev2)
{
	const s32 header = *block;
	s32 shift =  ((header>> 0)&0xF)+16;
	s32 pred1 = tbl_XA_Factor[(header>> 4)&0xF][0];
	s32 pred2 = tbl_XA_Factor[(header>> 4)&0xF][1];

	const s8* blockbytes = (s8*)&block[1];

	for(int i=0; i<14; i++, blockbytes++)
	{
		s32 pcm, pcm2;
		{
			s32 data = ((*blockbytes)<<28) & 0xF0000000;
			pcm = data>>shift;
			pcm+=((pred1*prev1)+(pred2*prev2))>>6;
			if(pcm> 32767) pcm= 32767;
			else if(pcm<-32768) pcm=-32768;
			*(buffer++) = pcm;
		}

		//prev2=prev1;
		//prev1=pcm;

		{
			s32 data = ((*blockbytes)<<24) & 0xF0000000;
			pcm2 = data>>shift;
			pcm2+=((pred1*pcm)+(pred2*prev1))>>6;
			if(pcm2> 32767) pcm2= 32767;
			else if(pcm2<-32768) pcm2=-32768;
			*(buffer++) = pcm2;
		}

		prev2=pcm;
		prev1=pcm2;
	}
}

static void __forceinline XA_decode_block_unsaturated(s16* buffer, const s16* block, s32& prev1, s32& prev2)
{
	const s32 header = *block;
	s32 shift =  ((header>> 0)&0xF)+16;
	s32 pred1 = tbl_XA_Factor[(header>> 4)&0xF][0];
	s32 pred2 = tbl_XA_Factor[(header>> 4)&0xF][1];

	const s8* blockbytes = (s8*)&block[1];

	for(int i=0; i<14; i++, blockbytes++)
	{
		s32 pcm, pcm2;
		{
			s32 data = ((*blockbytes)<<28) & 0xF0000000;
			pcm = data>>shift;
			pcm+=((pred1*prev1)+(pred2*prev2))>>6;
			// [Air] : Fast method, no saturation is performed.
			*(buffer++) = pcm;
		}

		{
			s32 data = ((*blockbytes)<<24) & 0xF0000000;
			pcm2 = data>>shift;
			pcm2+=((pred1*pcm)+(pred2*prev1))>>6;
			// [Air] : Fast method, no saturation is performed.
			*(buffer++) = pcm2;
		}

		prev2=pcm;
		prev1=pcm2;
	}
}

static void __forceinline IncrementNextA( const V_Core& thiscore, V_Voice& vc )
{
	// Important!  Both cores signal IRQ when an address is read, regardless of
	// which core actually reads the address.

	for( int i=0; i<2; i++ )
	{
		if( Cores[i].IRQEnable && (vc.NextA==Cores[i].IRQA ) )
		{ 
			if( IsDevBuild )
				ConLog(" * SPU2 Core %d: IRQ Called (IRQ passed).\n", i); 

			Spdif.Info = 4 << i;
			SetIrqCall();
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

#define XAFLAG_LOOP_END		(1ul<<0)
#define XAFLAG_LOOP			(1ul<<1)
#define XAFLAG_LOOP_START	(1ul<<2)

static void __forceinline __fastcall GetNextDataBuffered( V_Core& thiscore, V_Voice& vc, s32& Data) 
{
	if (vc.SCurrent<28)
	{
		// [Air] : skip the increment?
		//    (witness one of the rare ideal uses of a goto statement!)
		if( (vc.SCurrent&3) != 3 ) goto _skipIncrement;
	}
	else
	{
		if(vc.LoopFlags & XAFLAG_LOOP_END)
		{
			thiscore.Regs.ENDX |= (1 << voice);

			if( vc.LoopFlags & XAFLAG_LOOP )
			{
				vc.NextA=vc.LoopStartA;
			}
			else
			{
				vc.Stop();
				if( IsDevBuild )
				{
					if(MsgVoiceOff()) ConLog(" * SPU2: Voice Off by EndPoint: %d \n", voice);
					DebugCores[core].Voices[voice].lastStopReason = 1;
				}
			}
		}

		// We'll need the loop flags and buffer pointers regardless of cache status:
		// Note to Self : NextA addresses WORDS (not bytes).

		s16* memptr = GetMemPtr(vc.NextA&0xFFFFF);
		vc.LoopFlags = *memptr >> 8;	// grab loop flags from the upper byte.

		const int cacheIdx = vc.NextA / pcm_WordsPerBlock;
		PcmCacheEntry& cacheLine = pcm_cache_data[cacheIdx];
		vc.SBuffer = cacheLine.Sampledata;

		if( cacheLine.Validated )
		{
			// Cached block!  Read from the cache directly.
			// Make sure to propagate the prev1/prev2 ADPCM:

			vc.Prev1 = vc.SBuffer[27];
			vc.Prev2 = vc.SBuffer[26];

			//ConLog( " * SPU2 : Cache Hit! NextA=0x%x, cacheIdx=0x%x\n", vc.NextA, cacheIdx );

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

			s16* sbuffer = cacheLine.Sampledata;

			// saturated decoder
			XA_decode_block( sbuffer, memptr, vc.Prev1, vc.Prev2 );

			// [Air]: Testing use of a new unsaturated decoder. (benchmark needed)
			//   Chances are the saturation isn't needed, but for a very few exception games.
			//   This is definitely faster than the above version, but is it by enough to
			//   merit possible lower compatibility?  Especially now that games that make
			//   heavy use of the SPU2 via music or sfx will mostly use the cache anyway.

			//XA_decode_block_unsaturated( vc.SBuffer, memptr, vc.Prev1, vc.Prev2 );
		}

		vc.SCurrent = 0;
		if( (vc.LoopFlags & XAFLAG_LOOP_START) && !vc.LoopMode )
			vc.LoopStartA = vc.NextA;
	}

	IncrementNextA( thiscore, vc );

_skipIncrement:
	Data = vc.SBuffer[vc.SCurrent++];
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //
static void __forceinline GetNoiseValues(s32& VD) 
{
	static s32 Seed = 0x41595321;

	if(Seed&0x100) VD = (s32)((Seed&0xff)<<8);
	else if(!(Seed&0xffff)) VD = (s32)0x8000;
	else VD = (s32)0x7fff;

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
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

// Data is expected to be 16 bit signed (typical stuff!).
// volume is expected to be 32 bit signed (31 bits with reverse phase)
// Output is effectively 15 bit range, thanks to the signed volume.
static __forceinline s32 ApplyVolume(s32 data, s32 volume)
{
	//return (volume * data) >> 15;
	return MulShr32( data<<1, volume );
}

static void __forceinline UpdatePitch( V_Voice& vc )
{
	s32 pitch;

	// [Air] : re-ordered comparisons: Modulated is much more likely to be zero than voice,
	//   and so the way it was before it's have to check both voice and modulated values
	//   most of the time.  Now it'll just check Modulated and short-circuit past the voice
	//   check (not that it amounts to much, but eh every little bit helps).
	if( (vc.Modulated==0) || (voice==0) )
		pitch = vc.Pitch;
	else
		pitch = (vc.Pitch*(32768 + abs(Cores[core].Voices[voice-1].OutX)))>>15;
	
	vc.SP+=pitch;
}


static __forceinline void CalculateADSR( V_Core& thiscore, V_Voice& vc )
{
	if( vc.ADSR.Phase==0 )
	{
		vc.ADSR.Value = 0;
		return;
	}

	if( !vc.ADSR.Calculate() )
	{
		if( IsDevBuild )
		{
			if(MsgVoiceOff()) ConLog(" * SPU2: Voice Off by ADSR: %d \n", voice);
			DebugCores[core].Voices[voice].lastStopReason = 2;
		}
		vc.Stop();
		thiscore.Regs.ENDX |= (1 << voice);
	}

	jASSUME( vc.ADSR.Value >= 0 );	// ADSR should never be negative...
}

// Returns a 16 bit result in Value.
static void __forceinline GetVoiceValues_Linear(V_Core& thiscore, V_Voice& vc, s32& Value)
{
	while( vc.SP > 0 )
	{
		vc.PV2 = vc.PV1;

		GetNextDataBuffered( thiscore, vc, vc.PV1 );

		vc.SP -= 4096;
	}

	CalculateADSR( thiscore, vc );

	// Note!  It's very important that ADSR stay as accurate as possible.  By the way
	// it is used, various sound effects can end prematurely if we truncate more than
	// one or two bits.

	if(Interpolation==0)
	{
		Value = MulShr32( vc.PV1<<1, vc.ADSR.Value );
	} 
	else //if(Interpolation==1) //must be linear
	{
		s32 t0 = vc.PV2 - vc.PV1;
		Value = MulShr32( (vc.PV1<<1) - ((t0*vc.SP)>>11), vc.ADSR.Value );
	}
}

// Returns a 16 bit result in Value.
static void __forceinline GetVoiceValues_Cubic(V_Core& thiscore, V_Voice& vc, s32& Value)
{
	while( vc.SP > 0 )
	{
		vc.PV4=vc.PV3;
		vc.PV3=vc.PV2;
		vc.PV2=vc.PV1;

		GetNextDataBuffered( thiscore, vc, vc.PV1 );
		vc.PV1<<=2;
		vc.SPc = vc.SP&4095;	// just the fractional part, please!
		vc.SP-=4096;
	}

	CalculateADSR( thiscore, vc );

	s32 z0 = vc.PV3 - vc.PV4 + vc.PV1 - vc.PV2;
	s32 z1 = (vc.PV4 - vc.PV3 - z0);
	s32 z2 = (vc.PV2 - vc.PV4);

	s32 mu = vc.SPc;

	s32 val = (z0 * mu) >> 12;
	val = ((val + z1) * mu) >> 12;
	val = ((val + z2) * mu) >> 12;
	val += vc.PV2;

	// Note!  It's very important that ADSR stay as accurate as possible.  By the way
	// it is used, various sound effects can end prematurely if we truncate more than
	// one or two bits.
	Value = MulShr32( val, vc.ADSR.Value>>1 );
}

// Noise values need to be mixed without going through interpolation, since it
// can wreak havoc on the noise (causing muffling or popping).  Not that this noise
// generator is accurate in its own right.. but eh, ah well :)
static void __forceinline __fastcall GetNoiseValues(V_Core& thiscore, V_Voice& vc, s32& Data)
{
	while(vc.SP>=4096) 
	{
		GetNoiseValues( Data );
		vc.SP-=4096;
	}

	// GetNoiseValues can't set the phase zero on us unexpectedly
	// like GetVoiceValues can.  Better assert just in case though..
	jASSUME( vc.ADSR.Phase != 0 );	

	CalculateADSR( thiscore, vc );

	// Yup, ADSR applies even to noise sources...
	Data = MulShr32( Data, vc.ADSR.Value );
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

void __fastcall ReadInput(V_Core& thiscore, s32& PDataL,s32& PDataR) 
{
	if((thiscore.AutoDMACtrl&(core+1))==(core+1))
	{
		s32 tl,tr;

		if((core==1)&&((PlayMode&8)==8))
		{
			thiscore.InputPos&=~1;

			// CDDA mode
			// Source audio data is 32 bits.
			// We don't yet have the capability to handle this high res input data
			// so we just downgrade it to 16 bits for now.
			
#ifdef PCM24_S1_INTERLEAVE
			*PDataL=*(((s32*)(thiscore.ADMATempBuffer+(thiscore.InputPos<<1))));
			*PDataR=*(((s32*)(thiscore.ADMATempBuffer+(thiscore.InputPos<<1)+2)));
#else
			s32 *pl=(s32*)&(thiscore.ADMATempBuffer[thiscore.InputPos]);
			s32 *pr=(s32*)&(thiscore.ADMATempBuffer[thiscore.InputPos+0x200]);
			PDataL=*pl;
			PDataR=*pr;
#endif

			PDataL>>=1; //give 31 bit data (SndOut downsamples the rest of the way)
			PDataR>>=1;

			thiscore.InputPos+=2;
			if((thiscore.InputPos==0x100)||(thiscore.InputPos>=0x200)) {
				thiscore.AdmaInProgress=0;
				if(thiscore.InputDataLeft>=0x200)
				{
					u8 k=thiscore.InputDataLeft>=thiscore.InputDataProgress;

#ifdef PCM24_S1_INTERLEAVE
					AutoDMAReadBuffer(core,1);
#else
					AutoDMAReadBuffer(core,0);
#endif
					thiscore.AdmaInProgress=1;

					thiscore.TSA=(core<<10)+thiscore.InputPos;

					if (thiscore.InputDataLeft<0x200) 
					{
						FileLog("[%10d] AutoDMA%c block end.\n",Cycles, (core==0)?'4':'7');

						if( IsDevBuild )
						{
							if(thiscore.InputDataLeft>0)
							{
								if(MsgAutoDMA()) ConLog("WARNING: adma buffer didn't finish with a whole block!!\n");
							}
						}
						thiscore.InputDataLeft=0;
						thiscore.DMAICounter=1;
					}
				}
				thiscore.InputPos&=0x1ff;
			}

		}
		else if((core==0)&&((PlayMode&4)==4))
		{
			thiscore.InputPos&=~1;

			s32 *pl=(s32*)&(thiscore.ADMATempBuffer[thiscore.InputPos]);
			s32 *pr=(s32*)&(thiscore.ADMATempBuffer[thiscore.InputPos+0x200]);
			PDataL=*pl;
			PDataR=*pr;

			thiscore.InputPos+=2;
			if(thiscore.InputPos>=0x200) {
				thiscore.AdmaInProgress=0;
				if(thiscore.InputDataLeft>=0x200)
				{
					u8 k=thiscore.InputDataLeft>=thiscore.InputDataProgress;

					AutoDMAReadBuffer(core,0);

					thiscore.AdmaInProgress=1;

					thiscore.TSA=(core<<10)+thiscore.InputPos;

					if (thiscore.InputDataLeft<0x200) 
					{
						FileLog("[%10d] Spdif AutoDMA%c block end.\n",Cycles, (core==0)?'4':'7');

						if( IsDevBuild )
						{
							if(thiscore.InputDataLeft>0)
							{
								if(MsgAutoDMA()) ConLog("WARNING: adma buffer didn't finish with a whole block!!\n");
							}
						}
						thiscore.InputDataLeft=0;
						thiscore.DMAICounter=1;
					}
				}
				thiscore.InputPos&=0x1ff;
			}

		}
		else
		{
			if((core==1)&&((PlayMode&2)!=0))
			{
				tl=0;
				tr=0;
			}
			else
			{
				// Using the temporary buffer because this area gets overwritten by some other code.
				//*PDataL=(s32)*(s16*)(spu2mem+0x2000+(core<<10)+thiscore.InputPos);
				//*PDataR=(s32)*(s16*)(spu2mem+0x2200+(core<<10)+thiscore.InputPos);

				tl=(s32)thiscore.ADMATempBuffer[thiscore.InputPos];
				tr=(s32)thiscore.ADMATempBuffer[thiscore.InputPos+0x200];

			}

			PDataL=tl;
			PDataR=tr;

			thiscore.InputPos++;
			if((thiscore.InputPos==0x100)||(thiscore.InputPos>=0x200)) {
				thiscore.AdmaInProgress=0;
				if(thiscore.InputDataLeft>=0x200)
				{
					u8 k=thiscore.InputDataLeft>=thiscore.InputDataProgress;

					AutoDMAReadBuffer(core,0);

					thiscore.AdmaInProgress=1;

					thiscore.TSA=(core<<10)+thiscore.InputPos;

					if (thiscore.InputDataLeft<0x200) 
					{
						thiscore.AutoDMACtrl |= ~3;

						if( IsDevBuild )
						{
							FileLog("[%10d] AutoDMA%c block end.\n",Cycles, (core==0)?'4':'7');
							if(thiscore.InputDataLeft>0)
							{
								if(MsgAutoDMA()) ConLog("WARNING: adma buffer didn't finish with a whole block!!\n");
							}
						}

						thiscore.InputDataLeft = 0;
						thiscore.DMAICounter   = 1;
					}
				}
				thiscore.InputPos&=0x1ff;
			}
		}
	}
	else {
		PDataL=0;
		PDataR=0;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

static void __forceinline __fastcall ReadInputPV(V_Core& thiscore, s32& ValL,s32& ValR) 
{
	s32 DL=0, DR=0;

	u32 pitch=AutoDMAPlayRate[core];

	if(pitch==0) pitch=48000;
	
	thiscore.ADMAPV+=pitch;
	while(thiscore.ADMAPV>=48000) 
	{
		ReadInput(thiscore, DL,DR);
		thiscore.ADMAPV-=48000;
		thiscore.ADMAPL=DL;
		thiscore.ADMAPR=DR;
	}

	ValL=thiscore.ADMAPL;
	ValR=thiscore.ADMAPR;

	// Apply volumes:
	ValL = ApplyVolume( ValL, thiscore.InpL );
	ValR = ApplyVolume( ValR, thiscore.InpR );
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

// writes a signed value to the SPU2 ram
// Performs no cache invalidation -- use only for dynamic memory ranges
// of the SPU2 (between 0x0000 and SPU2_DYN_MEMLINE)
static __forceinline void spu2M_WriteFast( u32 addr, s16 value )
{
	// throw an assertion if the memory range is invalid:
#ifndef _DEBUG_FAST
	jASSUME( addr < SPU2_DYN_MEMLINE );
#endif
	*GetMemPtr( addr ) = value;
}


static __forceinline void MixVoice( V_Core& thiscore, V_Voice& vc, s32& VValL, s32& VValR )
{
	s32 Value=0;

	VValL = 0;
	VValR = 0;

	// Most games don't use much volume slide effects.  So only call the UpdateVolume
	// methods when needed by checking the flag outside the method here...

	vc.VolumeL.Update();
	vc.VolumeR.Update();

	if( vc.ADSR.Phase > 0 )
	{
		UpdatePitch( vc );

		if( vc.Noise )
			GetNoiseValues( thiscore, vc, Value );
		else
		{
			if( Interpolation == 2 )
				GetVoiceValues_Cubic( thiscore, vc, Value );
			else
				GetVoiceValues_Linear( thiscore, vc, Value );
		}

		// Record the output (used for modulation effects)
		vc.OutX = Value;

		if( IsDevBuild )
			DebugCores[core].Voices[voice].displayPeak = max(DebugCores[core].Voices[voice].displayPeak,abs(Value));

		// TODO : Implement this using high-def MulShr32.
		//   vc.VolumeL/R are 15 bits.  Value should be 32 bits (but is currently 16)

		VValL = ApplyVolume(Value,vc.VolumeL.Value);
		VValR = ApplyVolume(Value,vc.VolumeR.Value);
	}

	// Write-back of raw voice data (post ADSR applied)

	if (voice==1)      spu2M_WriteFast( 0x400 + (core<<12) + OutPos, (s16)Value );
	else if (voice==3) spu2M_WriteFast( 0x600 + (core<<12) + OutPos, (s16)Value );

}


static void __fastcall MixCore(s32& OutL, s32& OutR, s32 ExtL, s32 ExtR)
{
	s32 RVL,RVR;
	s32 SDL=0,SDR=0;
	s32 SWL=0,SWR=0;

	V_Core& thiscore( Cores[core] );

	for (voice=0;voice<24;voice++)
	{
		s32 VValL,VValR;

		V_Voice& vc( thiscore.Voices[voice] );
		MixVoice( thiscore, vc, VValL, VValR );
		
		// Note: Results from MixVoice are ranged at 16 bits.
		// Following muls are toggles only (0 or 1)

		SDL += VValL & vc.DryL;
		SDR += VValR & vc.DryR;
		SWL += VValL & vc.WetL;
		SWR += VValR & vc.WetR;
	}
	
	// Saturate final result to standard 16 bit range.
	SDL = clamp_mix( SDL );
	SDR = clamp_mix( SDR );
	SWL = clamp_mix( SWL );
	SWR = clamp_mix( SWR );
	
	// Write Mixed results To Output Area
	spu2M_WriteFast( 0x1000 + (core<<12) + OutPos, (s16)SDL );
	spu2M_WriteFast( 0x1200 + (core<<12) + OutPos, (s16)SDR );
	spu2M_WriteFast( 0x1400 + (core<<12) + OutPos, (s16)SWL );
	spu2M_WriteFast( 0x1600 + (core<<12) + OutPos, (s16)SWR );
	
	// Write mixed results to logfile (if enabled)
	
	WaveDump::WriteCore( core, CoreSrc_DryVoiceMix, SDL, SDR );
	WaveDump::WriteCore( core, CoreSrc_WetVoiceMix, SWL, SWR );

	s32 TDL,TDR;

	// Mix in the Input data
	TDL = OutL & thiscore.InpDryL;
	TDR = OutR & thiscore.InpDryR;

	// Mix in the Voice data
	TDL += SDL & thiscore.SndDryL;
	TDR += SDR & thiscore.SndDryR;

	// Mix in the External (nothing/core0) data
	TDL += ExtL & thiscore.ExtDryL;
	TDR += ExtR & thiscore.ExtDryR;
	
	if( !EffectsDisabled )
	{
		//Reverb pointer advances regardless of the FxEnable bit...
		Reverb_AdvanceBuffer( thiscore );

		if( thiscore.FxEnable )
		{
			s32 TWL,TWR;

			// Mix Input, Voice, and External data:
			TWL = OutL & thiscore.InpWetL;
			TWR = OutR & thiscore.InpWetR;
			TWL += SWL & thiscore.SndWetL;
			TWR += SWR & thiscore.SndWetR;
			TWL += ExtL & thiscore.ExtWetL; 
			TWR += ExtR & thiscore.ExtWetR;

			WaveDump::WriteCore( core, CoreSrc_PreReverb, TWL, TWR );

			DoReverb( thiscore, RVL, RVR, TWL, TWR );

			// Volume boost after effects application.  Boosting volume prior to effects
			// causes slight overflows in some games, and the volume boost is required.
			// (like all over volumes on SPU2, reverb coefficients and stuff are signed,
			// range -50% to 50%, thus *2 is needed)
			
			RVL *= 2;
			RVR *= 2;

			WaveDump::WriteCore( core, CoreSrc_PostReverb, RVL, RVR );

			TWL = ApplyVolume(RVL,thiscore.FxL);
			TWR = ApplyVolume(RVR,thiscore.FxR);

			// Mix Dry+Wet
			OutL = TDL + TWL;
			OutR = TDR + TWR;
		}
		else
		{
			WaveDump::WriteCore( core, CoreSrc_PreReverb, 0, 0 );
			WaveDump::WriteCore( core, CoreSrc_PostReverb, 0, 0 );
			OutL = TDL;
			OutR = TDR;
		}
	}
	else
	{
		OutL = TDL;
		OutR = TDR;
	}

	// Apply Master Volume.  The core will need this when the function returns.
	
	thiscore.MasterL.Update();
	thiscore.MasterR.Update();
}

// used to throttle the output rate of cache stat reports
static int p_cachestat_counter=0;

void Mix() 
{
	s32 ExtL=0, ExtR=0, OutL, OutR;

	// ****  CORE ZERO  ****

	core=0;
	if( (PlayMode&4) == 0 )
	{
		// get input data from input buffers
		ReadInputPV(Cores[0], ExtL, ExtR);
		WaveDump::WriteCore( 0, CoreSrc_Input, ExtL, ExtR );
	}

	MixCore( ExtL, ExtR, 0, 0 );

	if( (PlayMode & 4) || (Cores[0].Mute!=0) )
	{
		ExtL=0;
		ExtR=0;
	}
	else
	{
		ExtL = ApplyVolume( ExtL, Cores[0].MasterL.Value );
		ExtR = ApplyVolume( ExtR, Cores[0].MasterR.Value );
	}
	
	// Commit Core 0 output to ram before mixing Core 1:
	
	ExtL = clamp_mix( ExtL );
	ExtR = clamp_mix( ExtR );

	spu2M_WriteFast( 0x800 + OutPos, ExtL );
	spu2M_WriteFast( 0xA00 + OutPos, ExtR );

	WaveDump::WriteCore( 0, CoreSrc_External, ExtL, ExtR );

	// ****  CORE ONE  ****

	core = 1;
	if( (PlayMode&8) != 8 )
	{
		ReadInputPV(Cores[1], OutL, OutR);	// get input data from input buffers
		WaveDump::WriteCore( 1, CoreSrc_Input, OutL, OutR );
	}

	// Apply volume to the external (Core 0) input data.

	MixCore( OutL, OutR, ApplyVolume( ExtL, Cores[1].ExtL), ApplyVolume( ExtR, Cores[1].ExtR) );

	if( PlayMode & 8 )
	{
		// Experimental CDDA support
		// The CDDA overrides all other mixer output.  It's a direct feed!

		ReadInput(Cores[1], OutL, OutR);
		//WaveLog::WriteCore( 1, "CDDA-32", OutL, OutR );
	}
	else
	{
		OutL = MulShr32( OutL<<10, Cores[1].MasterL.Value );
		OutR = MulShr32( OutR<<10, Cores[1].MasterR.Value );

		// Final Clamp.
		// This could be circumvented by using 1/2th total output volume, although
		// I suspect clamping at the higher volume is more true to the PS2's real
		// implementation.

		OutL = clamp_mix( OutL, SndOutVolumeShift );
		OutR = clamp_mix( OutR, SndOutVolumeShift );
	}

	// Update spdif (called each sample)
	if(PlayMode&4)
		spdif_update();

	// AddToBuffer
	SndWrite(OutL, OutR);
	
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

-----------------------------------------------------------------------------
*/
