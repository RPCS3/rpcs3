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

static LPF_data lowpass_left( 11000, SampleRate );
static LPF_data lowpass_right( 11000, SampleRate );

static __forceinline s32 RevbGetIndexer( V_Core& thiscore, s32 offset )
{
	u32 pos = thiscore.ReverbX + offset;

	// Need to use modulus here, because games can and will drop the buffer size
	// without notice, and it leads to offsets several times past the end of the buffer.

	if( pos > thiscore.EffectsEndA )
	{
		//pos = thiscore.EffectsStartA + ((thiscore.ReverbX + offset) % (u32)thiscore.EffectsBufferSize);
		pos -= thiscore.EffectsEndA+1;
		pos += thiscore.EffectsStartA;
	}
	return pos;
} 

void Reverb_AdvanceBuffer( V_Core& thiscore )
{
	if( (Cycles & 1) && (thiscore.EffectsBufferSize > 0) )
	{
		//thiscore.ReverbX = RevbGetIndexer( thiscore, 1 );
		thiscore.ReverbX += 1;

		if( thiscore.ReverbX >= (u32)thiscore.EffectsBufferSize ) thiscore.ReverbX = 0;

		//thiscore.ReverbX += 1;
		//if(thiscore.ReverbX >= (u32)thiscore.EffectsBufferSize )
		//	thiscore.ReverbX %= (u32)thiscore.EffectsBufferSize;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

StereoOut32 DoReverb( V_Core& thiscore, const StereoOut32& Input )
{
	// Reverb processing occurs at 24khz, so we skip processing every other sample,
	// and use the previous calculation for this core instead.

	if( (Cycles&1)==0 )
	{
		StereoOut32 retval( thiscore.LastEffect );
		
		// Make sure and pass input through the LPF.  The result can be discarded.
		// This gives the LPF a better sampling from which to kill offending frequencies.

		lowpass_left.sample( Input.Left / 32768.0 );
		lowpass_right.sample( Input.Right / 32768.0 );
		
		//thiscore.LastEffect = Input;
		return retval;
	}
	else  
	{
		if( thiscore.RevBuffers.NeedsUpdated )
			thiscore.UpdateEffectsBufferSize();

		if( thiscore.EffectsBufferSize <= 0 )
		{
			// StartA is past EndA, so effects are disabled.
			//ConLog( " * SPU2: Effects disabled due to leapfrogged EffectsStart." );
			
			// Should we return zero here, or the input sample?
			// Because reverb gets an *2 mul, returning input seems dangerous, so I opt for silence.
			
			//return Input;
			return StereoOut32::Empty;
		}

		// Advance the current reverb buffer pointer, and cache the read/write addresses we'll be
		// needing for this session of reverb.
		
		const u32 src_a0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.IIR_SRC_A0 );
		const u32 src_a1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.IIR_SRC_A1 );
		const u32 src_b0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.IIR_SRC_B0 );
		const u32 src_b1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.IIR_SRC_B1 );

		const u32 dest_a0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.IIR_DEST_A0 );
		const u32 dest_a1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.IIR_DEST_A1 );
		const u32 dest_b0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.IIR_DEST_B0 );
		const u32 dest_b1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.IIR_DEST_B1 );
		
		const u32 dest2_a0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.IIR_DEST_A0 + 1 );
		const u32 dest2_a1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.IIR_DEST_A1 + 1 );
		const u32 dest2_b0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.IIR_DEST_B0 + 1 );
		const u32 dest2_b1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.IIR_DEST_B1 + 1 );
		
		const u32 acc_src_a0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.ACC_SRC_A0 );
		const u32 acc_src_b0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.ACC_SRC_B0 );
		const u32 acc_src_c0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.ACC_SRC_C0 );
		const u32 acc_src_d0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.ACC_SRC_D0 );

		const u32 acc_src_a1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.ACC_SRC_A1 );
		const u32 acc_src_b1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.ACC_SRC_B1 );
		const u32 acc_src_c1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.ACC_SRC_C1 );
		const u32 acc_src_d1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.ACC_SRC_D1 );

		const u32 fb_src_a0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.FB_SRC_A0 );
		const u32 fb_src_a1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.FB_SRC_A1 );
		const u32 fb_src_b0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.FB_SRC_B0 );
		const u32 fb_src_b1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.FB_SRC_B1 );

		const u32 mix_dest_a0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.MIX_DEST_A0 );
		const u32 mix_dest_a1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.MIX_DEST_A1 );
		const u32 mix_dest_b0 = RevbGetIndexer( thiscore, thiscore.RevBuffers.MIX_DEST_B0 );
		const u32 mix_dest_b1 = RevbGetIndexer( thiscore, thiscore.RevBuffers.MIX_DEST_B1 );

		// -----------------------------------------
		//    End Buffer Pointers, Begin Reverb!
		// -----------------------------------------

		//StereoOut32 INPUT_SAMPLE( thiscore.LastEffect + Input );

		// Note: LowPass on the input!  Very important.  Some games like DDS get terrible feedback otherwise.
		// Decisions, Decisions!  Should we mix in the 22khz sample skipped, or not?
		// First one mixes in the 22hkz sample.  Second one does not.

		/*StereoOut32 INPUT_SAMPLE(
			(s32)(lowpass_left.sample( (Input.Left+thiscore.LastEffect.Left) / 32768.0 ) * 32768.0),
			(s32)(lowpass_right.sample( (Input.Right+thiscore.LastEffect.Right) / 32768.0 ) * 32768.0)
		);*/

		StereoOut32 INPUT_SAMPLE(
			(s32)(lowpass_left.sample( Input.Left / 32768.0 ) * 32768.0),
			(s32)(lowpass_right.sample( Input.Right / 32768.0 ) * 32768.0)
		);
		
		const s32 IIR_INPUT_A0 = ((_spu2mem[src_a0] * thiscore.Revb.IIR_COEF) + (INPUT_SAMPLE.Left * thiscore.Revb.IN_COEF_L))>>16;
		const s32 IIR_INPUT_A1 = ((_spu2mem[src_a1] * thiscore.Revb.IIR_COEF) + (INPUT_SAMPLE.Right * thiscore.Revb.IN_COEF_R))>>16;
		const s32 IIR_INPUT_B0 = ((_spu2mem[src_b0] * thiscore.Revb.IIR_COEF) + (INPUT_SAMPLE.Left * thiscore.Revb.IN_COEF_L))>>16;
		const s32 IIR_INPUT_B1 = ((_spu2mem[src_b1] * thiscore.Revb.IIR_COEF) + (INPUT_SAMPLE.Right * thiscore.Revb.IN_COEF_R))>>16;

		const s32 IIR_A0 = (IIR_INPUT_A0 * thiscore.Revb.IIR_ALPHA) + (_spu2mem[dest_a0] * (0x7fff - thiscore.Revb.IIR_ALPHA));
		const s32 IIR_A1 = (IIR_INPUT_A1 * thiscore.Revb.IIR_ALPHA) + (_spu2mem[dest_a1] * (0x7fff - thiscore.Revb.IIR_ALPHA));
		const s32 IIR_B0 = (IIR_INPUT_B0 * thiscore.Revb.IIR_ALPHA) + (_spu2mem[dest_b0] * (0x7fff - thiscore.Revb.IIR_ALPHA));
		const s32 IIR_B1 = (IIR_INPUT_B1 * thiscore.Revb.IIR_ALPHA) + (_spu2mem[dest_b1] * (0x7fff - thiscore.Revb.IIR_ALPHA));
		_spu2mem[dest2_a0] = clamp_mix( IIR_A0 >> 16 );
		_spu2mem[dest2_a1] = clamp_mix( IIR_A1 >> 16 );
		_spu2mem[dest2_b0] = clamp_mix( IIR_B0 >> 16 );
		_spu2mem[dest2_b1] = clamp_mix( IIR_B1 >> 16 );

		// Faster single-mul approach to interpolation:
		// (doesn't work yet -- breaks Digital Devil Saga badly)
		/*const s32 IIR_A0 = IIR_INPUT_A0 + (((_spu2mem[dest_a0]-IIR_INPUT_A0) * thiscore.Revb.IIR_ALPHA)>>16);
		const s32 IIR_A1 = IIR_INPUT_A1 + (((_spu2mem[dest_a1]-IIR_INPUT_A1) * thiscore.Revb.IIR_ALPHA)>>16);
		const s32 IIR_B0 = IIR_INPUT_B0 + (((_spu2mem[dest_b0]-IIR_INPUT_B0) * thiscore.Revb.IIR_ALPHA)>>16);
		const s32 IIR_B1 = IIR_INPUT_B1 + (((_spu2mem[dest_b1]-IIR_INPUT_B1) * thiscore.Revb.IIR_ALPHA)>>16);

		_spu2mem[dest2_a0] = clamp_mix( IIR_A0 );
		_spu2mem[dest2_a1] = clamp_mix( IIR_A1 );
		_spu2mem[dest2_b0] = clamp_mix( IIR_B0 );
		_spu2mem[dest2_b1] = clamp_mix( IIR_B1 );*/

		const s32 ACC0 =
			((_spu2mem[acc_src_a0] * thiscore.Revb.ACC_COEF_A)) +
			((_spu2mem[acc_src_b0] * thiscore.Revb.ACC_COEF_B)) +
			((_spu2mem[acc_src_c0] * thiscore.Revb.ACC_COEF_C)) +
			((_spu2mem[acc_src_d0] * thiscore.Revb.ACC_COEF_D));

		const s32 ACC1 =
			((_spu2mem[acc_src_a1] * thiscore.Revb.ACC_COEF_A)) +
			((_spu2mem[acc_src_b1] * thiscore.Revb.ACC_COEF_B)) +
			((_spu2mem[acc_src_c1] * thiscore.Revb.ACC_COEF_C)) +
			((_spu2mem[acc_src_d1] * thiscore.Revb.ACC_COEF_D));

		const s32 FB_A0 = (_spu2mem[fb_src_a0] * thiscore.Revb.FB_ALPHA);
		const s32 FB_A1 = (_spu2mem[fb_src_a1] * thiscore.Revb.FB_ALPHA);

		const s32 fb_xor_a0 = _spu2mem[fb_src_a0] * ( thiscore.Revb.FB_ALPHA ^ 0x8000 );
		const s32 fb_xor_a1 = _spu2mem[fb_src_a1] * ( thiscore.Revb.FB_ALPHA ^ 0x8000 );

		_spu2mem[mix_dest_a0] = clamp_mix( (ACC0 - FB_A0) >> 16 );
		_spu2mem[mix_dest_a1] = clamp_mix( (ACC1 - FB_A1) >> 16 );
		_spu2mem[mix_dest_b0] = clamp_mix( (MulShr32(thiscore.Revb.FB_ALPHA<<16, ACC0) - fb_xor_a0 - (_spu2mem[fb_src_b0] * thiscore.Revb.FB_X)) >> 16 );
		_spu2mem[mix_dest_b1] = clamp_mix( (MulShr32(thiscore.Revb.FB_ALPHA<<16, ACC1) - fb_xor_a1 - (_spu2mem[fb_src_b1] * thiscore.Revb.FB_X)) >> 16 );

		thiscore.LastEffect.Left  = _spu2mem[mix_dest_a0] + _spu2mem[mix_dest_b0];
		thiscore.LastEffect.Right = _spu2mem[mix_dest_a1] + _spu2mem[mix_dest_b1];

		clamp_mix( thiscore.LastEffect );
		
		//thiscore.LastEffect.Left = (s32)(lowpass_left.sample( thiscore.LastEffect.Left / 32768.0 ) * 32768.0);
		//thiscore.LastEffect.Right = (s32)(lowpass_right.sample( thiscore.LastEffect.Right / 32768.0 ) * 32768.0);

		return thiscore.LastEffect;
	} 
}
