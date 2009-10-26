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

	// Need to use modulus here, because games can and will drop the buffer size
	// without notice, and it leads to offsets several times past the end of the buffer.

	if( pos > EffectsEndA )
	{
		//pos = EffectsStartA + ((ReverbX + offset) % (u32)EffectsBufferSize);
		pos -= EffectsEndA+1;
		pos += EffectsStartA;
	}
	return pos;
} 

void V_Core::Reverb_AdvanceBuffer()
{
	if( (Cycles & 1) && (EffectsBufferSize > 0) )
	{
		ReverbX += 1;
		if( ReverbX >= (u32)EffectsBufferSize ) ReverbX = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

StereoOut32 V_Core::DoReverb( const StereoOut32& Input )
{
	static StereoOut32 downbuf[8];
	static StereoOut32 upbuf[8];
	static int dbpos=0, ubpos=0;

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
		ubpos = (ubpos+1) & 7;
	}
	else
	{
		if( RevBuffers.NeedsUpdated )
			UpdateEffectsBufferSize();

		if( EffectsBufferSize <= 0 )
			return StereoOut32::Empty;

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

		const s32 IIR_INPUT_A0 = ((_spu2mem[src_a0] * Revb.IIR_COEF) + (INPUT_SAMPLE.Left * Revb.IN_COEF_L))>>16;
		const s32 IIR_INPUT_A1 = ((_spu2mem[src_a1] * Revb.IIR_COEF) + (INPUT_SAMPLE.Right * Revb.IN_COEF_R))>>16;
		const s32 IIR_INPUT_B0 = ((_spu2mem[src_b0] * Revb.IIR_COEF) + (INPUT_SAMPLE.Left * Revb.IN_COEF_L))>>16;
		const s32 IIR_INPUT_B1 = ((_spu2mem[src_b1] * Revb.IIR_COEF) + (INPUT_SAMPLE.Right * Revb.IN_COEF_R))>>16;

		// Faster single-mul approach to interpolation:
		// (doesn't work yet -- breaks Digital Devil Saga badly)
		const s32 IIR_A0 = IIR_INPUT_A0 + (((_spu2mem[dest_a0]-IIR_INPUT_A0) * Revb.IIR_ALPHA)>>16);
		const s32 IIR_A1 = IIR_INPUT_A1 + (((_spu2mem[dest_a1]-IIR_INPUT_A1) * Revb.IIR_ALPHA)>>16);
		const s32 IIR_B0 = IIR_INPUT_B0 + (((_spu2mem[dest_b0]-IIR_INPUT_B0) * Revb.IIR_ALPHA)>>16);
		const s32 IIR_B1 = IIR_INPUT_B1 + (((_spu2mem[dest_b1]-IIR_INPUT_B1) * Revb.IIR_ALPHA)>>16);

		_spu2mem[dest2_a0] = clamp_mix( IIR_A0 );
		_spu2mem[dest2_a1] = clamp_mix( IIR_A1 );
		_spu2mem[dest2_b0] = clamp_mix( IIR_B0 );
		_spu2mem[dest2_b1] = clamp_mix( IIR_B1 );

		const s32 ACC0 =
			((_spu2mem[acc_src_a0] * Revb.ACC_COEF_A)) +
			((_spu2mem[acc_src_b0] * Revb.ACC_COEF_B)) +
			((_spu2mem[acc_src_c0] * Revb.ACC_COEF_C)) +
			((_spu2mem[acc_src_d0] * Revb.ACC_COEF_D));

		const s32 ACC1 =
			((_spu2mem[acc_src_a1] * Revb.ACC_COEF_A)) +
			((_spu2mem[acc_src_b1] * Revb.ACC_COEF_B)) +
			((_spu2mem[acc_src_c1] * Revb.ACC_COEF_C)) +
			((_spu2mem[acc_src_d1] * Revb.ACC_COEF_D));

		const s32 FB_A0 = (_spu2mem[fb_src_a0] * Revb.FB_ALPHA);
		const s32 FB_A1 = (_spu2mem[fb_src_a1] * Revb.FB_ALPHA);

		const s32 fb_xor_a0 = _spu2mem[fb_src_a0] * ( Revb.FB_ALPHA ^ 0x8000 );
		const s32 fb_xor_a1 = _spu2mem[fb_src_a1] * ( Revb.FB_ALPHA ^ 0x8000 );

		_spu2mem[mix_dest_a0] = clamp_mix( (ACC0 - FB_A0) >> 16 );
		_spu2mem[mix_dest_a1] = clamp_mix( (ACC1 - FB_A1) >> 16 );
		_spu2mem[mix_dest_b0] = clamp_mix( (MulShr32(Revb.FB_ALPHA<<16, ACC0) - fb_xor_a0 - (_spu2mem[fb_src_b0] * Revb.FB_X)) >> 16 );
		_spu2mem[mix_dest_b1] = clamp_mix( (MulShr32(Revb.FB_ALPHA<<16, ACC1) - fb_xor_a1 - (_spu2mem[fb_src_b1] * Revb.FB_X)) >> 16 );

		upbuf[ubpos] = clamp_mix( StereoOut32(
			_spu2mem[mix_dest_a0] + _spu2mem[mix_dest_b0],	// left
			_spu2mem[mix_dest_a1] + _spu2mem[mix_dest_b1]	// right
		) );
	} 

	StereoOut32 retval;

	for( int x=0; x<8; ++x )
	{
		retval.Left  += (upbuf[(ubpos+x)&7].Left*downcoeffs[x]);
		retval.Right += (upbuf[(ubpos+x)&7].Right*downcoeffs[x]);
	}
	retval.Left  >>= (16-1); /* -1 To adjust for the null padding. */
	retval.Right >>= (16-1);

	return retval;
}
