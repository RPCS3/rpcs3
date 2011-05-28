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
#include "Lowpass.h"

// Low pass filters: Change these to 32 for a speedup (benchmarks needed to see if
// the speed gain is worth the quality drop)

//static LowPassFilter64 lowpass_left( 11000, SampleRate );
//static LowPassFilter64 lowpass_right( 11000, SampleRate );

__forceinline s32 V_Core::RevbGetIndexer( s32 offset )
{
	u32 pos = ReverbX + offset; // Apparently some reverb presets use offsets outside of the ps2 memory ...

	// Fast and simple single step wrapping, made possible by the preparation of the
	// effects buffer addresses.

	if( pos > EffectsEndA )
	{
		pos -= EffectsEndA+1;
		pos += EffectsStartA;
	}

	return pos;
}

u32 WrapAround(V_Core& thiscore, u32 offset)
{
	return (thiscore.ReverbX + offset) % thiscore.EffectsBufferSize;
}

void V_Core::Reverb_AdvanceBuffer()
{
	if( RevBuffers.NeedsUpdated )
			UpdateEffectsBufferSize();

	if( (Cycles & 1) && (EffectsBufferSize > 0) )
	{
		ReverbX += 1;
		if( ReverbX >= (u32)EffectsBufferSize ) ReverbX = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

StereoOut32 V_Core::DoReverb( const StereoOut32& Input )
{
	static const s32 downcoeffs[8] =
	{
		1283,  5344,  10895, 15243,
		15243, 10895,  5344,  1283
	};

	downbuf[dbpos] = Input;
	dbpos = (dbpos+1) & 7;

	// Reverb processing occurs at 24khz, so we skip processing every other sample,
	// and use the previous calculation for this core instead.

	if( (Cycles&1) == 0 )
	{
		// Important: Factor silence into the upsampler here, otherwise the reverb engine
		// develops a nasty feedback loop.

		upbuf[ubpos] = StereoOut32::Empty;
	}
	else
	{
		if( EffectsBufferSize <= 0 )
		{
			ubpos = (ubpos+1) & 7;
			return StereoOut32::Empty;
		}

		// Advance the current reverb buffer pointer, and cache the read/write addresses we'll be
		// needing for this session of reverb.

		const u32 src_a0 = RevbGetIndexer( RevBuffers.IIR_SRC_A0 );
		const u32 src_a1 = RevbGetIndexer( RevBuffers.IIR_SRC_A1 );
		const u32 src_b0 = RevbGetIndexer( RevBuffers.IIR_SRC_B0 );
		const u32 src_b1 = RevbGetIndexer( RevBuffers.IIR_SRC_B1 );

		const u32 dest_a0 = RevbGetIndexer( RevBuffers.IIR_DEST_A0 );
		const u32 dest_a1 = RevbGetIndexer( RevBuffers.IIR_DEST_A1 );
		const u32 dest_b0 = RevbGetIndexer( RevBuffers.IIR_DEST_B0 );
		const u32 dest_b1 = RevbGetIndexer( RevBuffers.IIR_DEST_B1 );

		const u32 dest2_a0 = RevbGetIndexer( RevBuffers.IIR_DEST_A0 + 1 );
		const u32 dest2_a1 = RevbGetIndexer( RevBuffers.IIR_DEST_A1 + 1 );
		const u32 dest2_b0 = RevbGetIndexer( RevBuffers.IIR_DEST_B0 + 1 );
		const u32 dest2_b1 = RevbGetIndexer( RevBuffers.IIR_DEST_B1 + 1 );

		const u32 acc_src_a0 = RevbGetIndexer( RevBuffers.ACC_SRC_A0 );
		const u32 acc_src_b0 = RevbGetIndexer( RevBuffers.ACC_SRC_B0 );
		const u32 acc_src_c0 = RevbGetIndexer( RevBuffers.ACC_SRC_C0 );
		const u32 acc_src_d0 = RevbGetIndexer( RevBuffers.ACC_SRC_D0 );

		const u32 acc_src_a1 = RevbGetIndexer( RevBuffers.ACC_SRC_A1 );
		const u32 acc_src_b1 = RevbGetIndexer( RevBuffers.ACC_SRC_B1 );
		const u32 acc_src_c1 = RevbGetIndexer( RevBuffers.ACC_SRC_C1 );
		const u32 acc_src_d1 = RevbGetIndexer( RevBuffers.ACC_SRC_D1 );

		const u32 fb_src_a0 = RevbGetIndexer( RevBuffers.FB_SRC_A0 );
		const u32 fb_src_a1 = RevbGetIndexer( RevBuffers.FB_SRC_A1 );
		const u32 fb_src_b0 = RevbGetIndexer( RevBuffers.FB_SRC_B0 );
		const u32 fb_src_b1 = RevbGetIndexer( RevBuffers.FB_SRC_B1 );

		const u32 mix_dest_a0 = RevbGetIndexer( RevBuffers.MIX_DEST_A0 );
		const u32 mix_dest_a1 = RevbGetIndexer( RevBuffers.MIX_DEST_A1 );
		const u32 mix_dest_b0 = RevbGetIndexer( RevBuffers.MIX_DEST_B0 );
		const u32 mix_dest_b1 = RevbGetIndexer( RevBuffers.MIX_DEST_B1 );

		// -----------------------------------------
		//          Optimized IRQ Testing !
		// -----------------------------------------

		// This test is enhanced by using the reverb effects area begin/end test as a
		// shortcut, since all buffer addresses are within that area.  If the IRQA isn't
		// within that zone then the "bulk" of the test is skipped, so this should only
		// be a slowdown on a few evil games.

		for( uint i=0; i<2; i++ )
		{
			if( Cores[i].IRQEnable && ((Cores[i].IRQA >= EffectsStartA) && (Cores[i].IRQA <= EffectsEndA)) )
			{
				if(	(Cores[i].IRQA == src_a0)		||	(Cores[i].IRQA == src_a1)		||
					(Cores[i].IRQA == src_b0)		||	(Cores[i].IRQA == src_b1)		||

					(Cores[i].IRQA == dest_a0)		||	(Cores[i].IRQA == dest_a1)		||
					(Cores[i].IRQA == dest_b0)		||	(Cores[i].IRQA == dest_b1)		||

					(Cores[i].IRQA == dest2_a0)		||	(Cores[i].IRQA == dest2_a1)		||
					(Cores[i].IRQA == dest2_b0)		||	(Cores[i].IRQA == dest2_b1)		||

					(Cores[i].IRQA == acc_src_a0)	||	(Cores[i].IRQA == acc_src_a1)	||
					(Cores[i].IRQA == acc_src_b0)	||	(Cores[i].IRQA == acc_src_b1)	||
					(Cores[i].IRQA == acc_src_c0)	||	(Cores[i].IRQA == acc_src_c1)	||
					(Cores[i].IRQA == acc_src_d0)	||	(Cores[i].IRQA == acc_src_d1)	||

					(Cores[i].IRQA == fb_src_a0)	||	(Cores[i].IRQA == fb_src_a1)	||
					(Cores[i].IRQA == fb_src_b0)	||	(Cores[i].IRQA == fb_src_b1)	||

					(Cores[i].IRQA == mix_dest_a0)	||	(Cores[i].IRQA == mix_dest_a1)	||
					(Cores[i].IRQA == mix_dest_b0)	||	(Cores[i].IRQA == mix_dest_b1) )
				{
					//printf("Core %d IRQ Called (Reverb). IRQA = %x\n",i,addr);
					SetIrqCall(i);
				}
			}
		}

		// -----------------------------------------
		//         Begin Reverb Processing !
		// -----------------------------------------

		StereoOut32 INPUT_SAMPLE;

		for( int x=0; x<8; ++x )
		{
			INPUT_SAMPLE.Left += (downbuf[(dbpos+x)&7].Left * downcoeffs[x]);
			INPUT_SAMPLE.Right += (downbuf[(dbpos+x)&7].Right * downcoeffs[x]);
		}
		
		// This is gotta be a Schroeder Reverberator:
		// http://cnx.org/content/m15491/latest/

		//////////////////////////////////////////////////////////////
		// Part 1: Input filter block (FIR filter)
		// Purpose: Filter and write data to the sample queues for the echos below

		INPUT_SAMPLE.Left  >>= 16;
		INPUT_SAMPLE.Right >>= 16;

		s32 input_L = (INPUT_SAMPLE.Left * Revb.IN_COEF_L);
		s32 input_R = (INPUT_SAMPLE.Right * Revb.IN_COEF_R);

		const s32 IIR_A0 = (_spu2mem[src_a0] * Revb.IIR_COEF) + input_L;
		const s32 IIR_A1 = (_spu2mem[src_a1] * Revb.IIR_COEF) + input_R;
		const s32 IIR_B0 = (_spu2mem[src_b0] * Revb.IIR_COEF) + input_L;
		const s32 IIR_B1 = (_spu2mem[src_b1] * Revb.IIR_COEF) + input_R;

		const s32 IIR_C0 = _spu2mem[dest_a0]* Revb.IIR_ALPHA;
		const s32 IIR_C1 = _spu2mem[dest_a1]* Revb.IIR_ALPHA;
		const s32 IIR_D0 = _spu2mem[dest_b0]* Revb.IIR_ALPHA;
		const s32 IIR_D1 = _spu2mem[dest_b1]* Revb.IIR_ALPHA;

		_spu2mem[dest2_a0] = clamp_mix( (IIR_A0-IIR_C0) >> 16 );
		_spu2mem[dest2_a1] = clamp_mix( (IIR_A1-IIR_C1) >> 16 );
		_spu2mem[dest2_b0] = clamp_mix( (IIR_B0-IIR_D0) >> 16 );
		_spu2mem[dest2_b1] = clamp_mix( (IIR_B1-IIR_D1) >> 16 );


		//////////////////////////////////////////////////////////////
		// Part 2: Comb filters (echos)
		// Purpose: Create the primary reflections on the virtual walls

		s32 ACC0 = (
			((_spu2mem[acc_src_a0] * Revb.ACC_COEF_A)) +
			((_spu2mem[acc_src_b0] * Revb.ACC_COEF_B)) +
			((_spu2mem[acc_src_c0] * Revb.ACC_COEF_C)) +
			((_spu2mem[acc_src_d0] * Revb.ACC_COEF_D))
		); // >> 16;

		s32 ACC1 = (
			((_spu2mem[acc_src_a1] * Revb.ACC_COEF_A)) +
			((_spu2mem[acc_src_b1] * Revb.ACC_COEF_B)) +
			((_spu2mem[acc_src_c1] * Revb.ACC_COEF_C)) +
			((_spu2mem[acc_src_d1] * Revb.ACC_COEF_D))
		); // >> 16;

	
		//////////////////////////////////////////////////////////////
		// Part 3: All-pass filters
		// Purpose: Create actual reverberation sound effect

		// First

		// Take delayed input
		s32 FB_A0 = _spu2mem[fb_src_a0]; // 16
		s32 FB_A1 = _spu2mem[fb_src_a1];

		// Apply gain and add to input
		s32 MIX_A0 = (ACC0 + FB_A0 * Revb.FB_ALPHA)>>16; // 32 + 16*16 = 32
		s32 MIX_A1 = (ACC1 + FB_A1 * Revb.FB_ALPHA)>>16;

		// Write to queue
		_spu2mem[mix_dest_a0] = clamp_mix(MIX_A0);
		_spu2mem[mix_dest_a1] = clamp_mix(MIX_A1);

		// Apply second gain and add
		ACC0 += (FB_A0 << 16) - MIX_A0 * Revb.FB_ALPHA;
		ACC1 += (FB_A1 << 16) - MIX_A1 * Revb.FB_ALPHA;

		//////////////////////////////////////////////////////////////

		// Second

		// Take delayed input
		s32 FB_B0 = _spu2mem[fb_src_b0]; // 16
		s32 FB_B1 = _spu2mem[fb_src_b1];

		// Apply gain and add to input
		s32 MIX_B0 = (ACC0 + FB_B0 * Revb.FB_X)>>16; // 32 + 16*16 = 32
		s32 MIX_B1 = (ACC1 + FB_B1 * Revb.FB_X)>>16;

		// Write to queue
		_spu2mem[mix_dest_b0] = clamp_mix(MIX_B0);
		_spu2mem[mix_dest_b1] = clamp_mix(MIX_B1);

		// Apply second gain and add
		ACC0 += (FB_B0 << 16) - MIX_B0 * Revb.FB_X;
		ACC1 += (FB_B1 << 16) - MIX_B1 * Revb.FB_X;

		upbuf[ubpos] = clamp_mix( StereoOut32(
			ACC0>>16,	// left
			ACC1>>16	// right
		) );
	}

	StereoOut32 retval;

	//for( int x=0; x<8; ++x )
	//{
	//	retval.Left  += (upbuf[(ubpos+x)&7].Left*downcoeffs[x]);
	//	retval.Right += (upbuf[(ubpos+x)&7].Right*downcoeffs[x]);
	//}

	if( (Cycles&1) == 0 )
	{
		retval.Left = (upbuf[(ubpos+5)&7].Left + upbuf[(ubpos+7)&7].Left)>>1;
		retval.Right = (upbuf[(ubpos+5)&7].Right + upbuf[(ubpos+7)&7].Right)>>1;
	}
	else
	{
		retval.Left = upbuf[(ubpos+6)&7].Left;
		retval.Right = upbuf[(ubpos+6)&7].Right;
	}

	// Notes:
	//  the first -1 is to adjust for the null padding in every other upbuf sample (which
	//  halves the overall volume).
	//  The second +1 divides by two, which is part of Neill's suggestion to divide by 3.
	//
	// According Neill the final result should be divided by 3, but currently the output
	// is way too quiet for that to fly.  In fact no division at all might be better.
	// In any case the problem always seems to be that the reverb isn't resonating enough
	// (indicating short buffers or bad coefficient math?), not that it isn't loud enough.

	//retval.Left  >>= (16-1 + 1);
	//retval.Right >>= (16-1 + 1);

	ubpos = (ubpos+1) & 7;

	return retval;
}

StereoOut32 V_Core::DoReverb_Fake( const StereoOut32& Input )
{
	if(!FakeReverbActive /*|| (Cycles&1) == 0*/)
		return StereoOut32::Empty;

	V_Core& thiscore(Cores[Index]);

	s16* Base = GetMemPtr(thiscore.EffectsStartA);

	s32 accL = 0;
	s32 accR = 0;

	///////////////////////////////////////////////////////////
	// part 0: Parameters

	// Input volumes
	const s32 InputL = -0x3fff;
	const s32 InputR = -0x3fff;

	// Echo 1: Positive, short delay
	const u32 Echo1L = 0x3700;
	const u32 Echo1R = 0x2704;
	const s32 Echo1A = 0x5000 / 8;

	// Echo 2: Negative, slightly longer delay, quiet
	const u32 Echo2L = 0x2f10;
	const u32 Echo2R = 0x1f04;
	const s32 Echo2A = 0x4c00 / 8;

	// Echo 3: Negative, longer delay, full feedback
	const u32 Echo3L = 0x2800;
	const u32 Echo3R = 0x1b34;
	const s32 Echo3A = 0xb800 / 8;

	// Echo 4: Negative, longer delay, full feedback
	const u32 Echo4L = 0x2708;
	const u32 Echo4R = 0x1704;
	const s32 Echo4A = 0xbc00 / 8;

	// Output control:
	const u32 Mix1L = thiscore.Revb.MIX_DEST_A0;
	const u32 Mix1R = thiscore.Revb.MIX_DEST_A1;
	const u32 Mix2L = thiscore.Revb.MIX_DEST_B0;
	const u32 Mix2R = thiscore.Revb.MIX_DEST_B1;

	const u32 CrossChannelL = 0x4694;
	const u32 CrossChannelR = 0x52e4;
	const u32 CrossChannelA = thiscore.Revb.FB_ALPHA / 8;

	///////////////////////////////////////////////////////////
	// part 1: input

	const s32 inL = Input.Left  * InputL;
	const s32 inR = Input.Right * InputR;

	accL += inL;
	accR += inR;

	///////////////////////////////////////////////////////////
	// part 2: straight echos

	s32 e1L = Base[WrapAround(thiscore,Echo1L  )] * Echo1A;
	s32 e1R = Base[WrapAround(thiscore,Echo1R+1)] * Echo1A;

	accL += e1L;
	accR += e1R;

	s32 e2L = Base[WrapAround(thiscore,Echo2L  )] * Echo2A;
	s32 e2R = Base[WrapAround(thiscore,Echo2R+1)] * Echo2A;

	accL += e2L;
	accR += e2R;

	s32 e3L = Base[WrapAround(thiscore,Echo3L  )] * Echo3A;
	s32 e3R = Base[WrapAround(thiscore,Echo3R+1)] * Echo3A;


	accL += e3L;
	accR += e3R;

	s32 e4L = Base[WrapAround(thiscore,Echo4L  )] * Echo4A;
	s32 e4R = Base[WrapAround(thiscore,Echo4R+1)] * Echo4A;

	accL += e4L;
	accR += e4R;

	///////////////////////////////////////////////////////////
	// part 4: cross-channel feedback

	s32 ccL = Base[WrapAround(thiscore,CrossChannelL+1)] * CrossChannelA;
	s32 ccR = Base[WrapAround(thiscore,CrossChannelR  )] * CrossChannelA;
	accL += ccL;
	accR += ccR;

	///////////////////////////////////////////////////////////
	// part N-1: normalize output

	accL >>= 15;
	accR >>= 15;

	///////////////////////////////////////////////////////////
	// part N: write output

	s32 tmpL = accL>>5; //  reduce the volume
	s32 tmpR = accR>>5;


	Base[WrapAround(thiscore,Mix1L)] = clamp_mix(accL-tmpL);
	Base[WrapAround(thiscore,Mix1R)] = clamp_mix(accR-tmpR);
	Base[WrapAround(thiscore,Mix2L)] = clamp_mix(accL-tmpL);
	Base[WrapAround(thiscore,Mix2R)] = clamp_mix(accR-tmpR);

	s32 returnL = Base[WrapAround(thiscore,Mix1L)] + Base[WrapAround(thiscore,Mix2L)];
	s32 returnR = Base[WrapAround(thiscore,Mix1R)] + Base[WrapAround(thiscore,Mix2R)];

	return StereoOut32(returnL,returnR);
}
