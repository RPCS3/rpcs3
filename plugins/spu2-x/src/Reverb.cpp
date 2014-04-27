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
	u32 pos = ReverbX + offset;

	// Fast and simple single step wrapping, made possible by the preparation of the
	// effects buffer addresses.

	if( pos > EffectsEndA )
	{
		pos -= EffectsEndA+1;
		pos += EffectsStartA;
	}

	assert(pos >= EffectsStartA && pos <= EffectsEndA);
	return pos;
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
#if 0
	static const s32 downcoeffs[8] =
	{
		1283,  5344,  10895, 15243,
		15243, 10895,  5344,  1283
	};
#else
	// 2/3 of the above
	static const s32 downcoeffs[8] =
	{
		855,  3562,  7263, 10163,
		10163, 7263,  3562,  855
	};
#endif

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

		INPUT_SAMPLE.Left  >>= 16;
		INPUT_SAMPLE.Right >>= 16;

		s32 input_L = INPUT_SAMPLE.Left * Revb.IN_COEF_L;
		s32 input_R = INPUT_SAMPLE.Right * Revb.IN_COEF_R;

		const s32 IIR_INPUT_A0 = clamp_mix((((s32)_spu2mem[src_a0] * Revb.IIR_COEF) + input_L)>>15);
		const s32 IIR_INPUT_A1 = clamp_mix((((s32)_spu2mem[src_a1] * Revb.IIR_COEF) + input_L)>>15);
		const s32 IIR_INPUT_B0 = clamp_mix((((s32)_spu2mem[src_b0] * Revb.IIR_COEF) + input_R)>>15);
		const s32 IIR_INPUT_B1 = clamp_mix((((s32)_spu2mem[src_b1] * Revb.IIR_COEF) + input_R)>>15);

		const s32 src_dest_a0 = _spu2mem[dest_a0];
		const s32 src_dest_a1 = _spu2mem[dest_a1];
		const s32 src_dest_b0 = _spu2mem[dest_b0];
		const s32 src_dest_b1 = _spu2mem[dest_b1];

		// This section differs from Neill's doc as it uses single-mul interpolation instead
		// of 0x8000-val inversion.  (same result, faster)
		const s32 IIR_A0 = src_dest_a0 + (((IIR_INPUT_A0 - src_dest_a0) * Revb.IIR_ALPHA)>>15);
		const s32 IIR_A1 = src_dest_a1 + (((IIR_INPUT_A1 - src_dest_a1) * Revb.IIR_ALPHA)>>15);
		const s32 IIR_B0 = src_dest_b0 + (((IIR_INPUT_B0 - src_dest_b0) * Revb.IIR_ALPHA)>>15);
		const s32 IIR_B1 = src_dest_b1 + (((IIR_INPUT_B1 - src_dest_b1) * Revb.IIR_ALPHA)>>15);
		_spu2mem[dest2_a0] = clamp_mix( IIR_A0 );
		_spu2mem[dest2_a1] = clamp_mix( IIR_A1 );
		_spu2mem[dest2_b0] = clamp_mix( IIR_B0 );
		_spu2mem[dest2_b1] = clamp_mix( IIR_B1 );

		const s32 ACC0 = clamp_mix(
			((_spu2mem[acc_src_a0] * Revb.ACC_COEF_A) >> 15) +
			((_spu2mem[acc_src_b0] * Revb.ACC_COEF_B) >> 15) +
			((_spu2mem[acc_src_c0] * Revb.ACC_COEF_C) >> 15) +
			((_spu2mem[acc_src_d0] * Revb.ACC_COEF_D) >> 15)
		);

		const s32 ACC1 = clamp_mix(
			((_spu2mem[acc_src_a1] * Revb.ACC_COEF_A) >> 15) +
			((_spu2mem[acc_src_b1] * Revb.ACC_COEF_B) >> 15) +
			((_spu2mem[acc_src_c1] * Revb.ACC_COEF_C) >> 15) +
			((_spu2mem[acc_src_d1] * Revb.ACC_COEF_D) >> 15)
		);

		// The following code differs from Neill's doc as it uses the more natural single-mul
		// interpolative, instead of the funky ^0x8000 stuff.  (better result, faster)

		const s32 FB_A0 = _spu2mem[fb_src_a0];
		const s32 FB_A1 = _spu2mem[fb_src_a1];
		const s32 FB_B0 = _spu2mem[fb_src_b0];
		const s32 FB_B1 = _spu2mem[fb_src_b1];

		const s32 mix_a0 = clamp_mix(ACC0 - ((FB_A0 * Revb.FB_ALPHA) >> 15));
		const s32 mix_a1 = clamp_mix(ACC1 - ((FB_A1 * Revb.FB_ALPHA) >> 15));
		const s32 mix_b0 = clamp_mix(FB_A0 + (((ACC0 - FB_A0) * Revb.FB_ALPHA - FB_B0 * Revb.FB_X) >> 15));
		const s32 mix_b1 = clamp_mix(FB_A1 + (((ACC1 - FB_A1) * Revb.FB_ALPHA - FB_B1 * Revb.FB_X) >> 15));

		_spu2mem[mix_dest_a0] = mix_a0;
		_spu2mem[mix_dest_a1] = mix_a1;
		_spu2mem[mix_dest_b0] = mix_b0;
		_spu2mem[mix_dest_b1] = mix_b1;

		upbuf[ubpos] = clamp_mix( StereoOut32(
			mix_a0 + mix_b0,	// left
			mix_a1 + mix_b1		// right
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