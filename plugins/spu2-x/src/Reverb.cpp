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

//static LPF_data lowpass_left( 11000, SampleRate );
//static LPF_data lowpass_right( 11000, SampleRate );

static s32 EffectsBufferIndexer( V_Core& thiscore, s32 offset )
{
	u32 pos = thiscore.EffectsStartA + thiscore.ReverbX + offset;

	// Need to use modulus here, because games can and will drop the buffer size
	// without notice, and it leads to offsets several times past the end of the buffer.

	if( pos > thiscore.EffectsEndA )
	{
		pos = thiscore.EffectsStartA + ((thiscore.ReverbX + offset) % (u32)thiscore.EffectsBufferSize);
		//pos -= thiscore.EffectsEndA+1;
		//pos += thiscore.EffectsStartA;
	}
	else if( pos < thiscore.EffectsStartA )
	{
		pos = thiscore.EffectsEndA+1 - ((thiscore.ReverbX + offset) % (u32)thiscore.EffectsBufferSize );
		//pos -= thiscore.EffectsStartA;
		//pos += thiscore.EffectsEndA+1;
	}
	return pos;
} 

/*void LowPass(s32& VL, s32& VR)
{
	VL = (s32)( lowpass_left.sample(VL/65536.0) * 65536.0 );
	VR = (s32)( lowpass_right.sample(VR/65536.0) * 65536.0 );
}*/

void Reverb_AdvanceBuffer( V_Core& thiscore )
{
	if( (Cycles & 1) && (thiscore.EffectsBufferSize > 0) )
	{
		thiscore.ReverbX += 1;
		if(thiscore.ReverbX >= (u32)thiscore.EffectsBufferSize )
			thiscore.ReverbX %= (u32)thiscore.EffectsBufferSize;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

void DoReverb( V_Core& thiscore, s32& OutL, s32& OutR, s32 InL, s32 InR)
{
	// Reverb processing occurs at 24khz, so we skip processing every other sample,
	// and use the previous calculation for this core instead.

	if( thiscore.EffectsBufferSize <= 0 )
	{
		// StartA is past EndA, so effects are disabled.
		OutL = InL;
		OutR = InR;
		//ConLog( " * SPU2: Effects disabled due to leapfrogged EffectsStart." );
		return;
	}

	if((Cycles&1)==0) 
	{
		OutL = thiscore.LastEffectL;
		OutR = thiscore.LastEffectR;
		
		thiscore.LastEffectL = InL;
		thiscore.LastEffectR = InR;
	}
	else  
	{
		// Advance the current reverb buffer pointer, and cache the read/write addresses we'll be
		// needing for this session of reverb.
		
		const u32 src_a0 = EffectsBufferIndexer( thiscore, thiscore.Revb.IIR_SRC_A0 );
		const u32 src_a1 = EffectsBufferIndexer( thiscore, thiscore.Revb.IIR_SRC_A1 );
		const u32 src_b0 = EffectsBufferIndexer( thiscore, thiscore.Revb.IIR_SRC_B0 );
		const u32 src_b1 = EffectsBufferIndexer( thiscore, thiscore.Revb.IIR_SRC_B1 );

		const u32 dest_a0 = EffectsBufferIndexer( thiscore, thiscore.Revb.IIR_DEST_A0 );
		const u32 dest_a1 = EffectsBufferIndexer( thiscore, thiscore.Revb.IIR_DEST_A1 );
		const u32 dest_b0 = EffectsBufferIndexer( thiscore, thiscore.Revb.IIR_DEST_B0 );
		const u32 dest_b1 = EffectsBufferIndexer( thiscore, thiscore.Revb.IIR_DEST_B1 );
		
		const u32 dest2_a0 = EffectsBufferIndexer( thiscore, thiscore.Revb.IIR_DEST_A0 + 1 );
		const u32 dest2_a1 = EffectsBufferIndexer( thiscore, thiscore.Revb.IIR_DEST_A1 + 1 );
		const u32 dest2_b0 = EffectsBufferIndexer( thiscore, thiscore.Revb.IIR_DEST_B0 + 1 );
		const u32 dest2_b1 = EffectsBufferIndexer( thiscore, thiscore.Revb.IIR_DEST_B1 + 1 );
		
		const u32 acc_src_a0 = EffectsBufferIndexer( thiscore, thiscore.Revb.ACC_SRC_A0 );
		const u32 acc_src_b0 = EffectsBufferIndexer( thiscore, thiscore.Revb.ACC_SRC_B0 );
		const u32 acc_src_c0 = EffectsBufferIndexer( thiscore, thiscore.Revb.ACC_SRC_C0 );
		const u32 acc_src_d0 = EffectsBufferIndexer( thiscore, thiscore.Revb.ACC_SRC_D0 );

		const u32 acc_src_a1 = EffectsBufferIndexer( thiscore, thiscore.Revb.ACC_SRC_A1 );
		const u32 acc_src_b1 = EffectsBufferIndexer( thiscore, thiscore.Revb.ACC_SRC_B1 );
		const u32 acc_src_c1 = EffectsBufferIndexer( thiscore, thiscore.Revb.ACC_SRC_C1 );
		const u32 acc_src_d1 = EffectsBufferIndexer( thiscore, thiscore.Revb.ACC_SRC_D1 );

		const u32 fb_src_a0 = EffectsBufferIndexer( thiscore, thiscore.Revb.MIX_DEST_A0 - thiscore.Revb.FB_SRC_A );
		const u32 fb_src_a1 = EffectsBufferIndexer( thiscore, thiscore.Revb.MIX_DEST_A1 - thiscore.Revb.FB_SRC_A );
		const u32 fb_src_b0 = EffectsBufferIndexer( thiscore, thiscore.Revb.MIX_DEST_B0 - thiscore.Revb.FB_SRC_B );
		const u32 fb_src_b1 = EffectsBufferIndexer( thiscore, thiscore.Revb.MIX_DEST_B1 - thiscore.Revb.FB_SRC_B );

		const u32 mix_dest_a0 = EffectsBufferIndexer( thiscore, thiscore.Revb.MIX_DEST_A0 );
		const u32 mix_dest_a1 = EffectsBufferIndexer( thiscore, thiscore.Revb.MIX_DEST_A1 );
		const u32 mix_dest_b0 = EffectsBufferIndexer( thiscore, thiscore.Revb.MIX_DEST_B0 );
		const u32 mix_dest_b1 = EffectsBufferIndexer( thiscore, thiscore.Revb.MIX_DEST_B1 );

		// -----------------------------------------
		//    End Buffer Pointers, Begin Reverb!
		// -----------------------------------------

		const s32 INPUT_SAMPLE_L = (thiscore.LastEffectL+InL);
		const s32 INPUT_SAMPLE_R = (thiscore.LastEffectR+InR);
		
		//const s32 INPUT_SAMPLE_L = (s32)( lowpass_left.sample( (thiscore.LastEffectL+InL)/65536.0 ) * 65536.0 );
		//const s32 INPUT_SAMPLE_R = (s32)( lowpass_right.sample( (thiscore.LastEffectR+InR)/65536.0 ) * 65536.0 );

		const s32 IIR_INPUT_A0 = ((_spu2mem[src_a0] * thiscore.Revb.IIR_COEF) + (INPUT_SAMPLE_L * thiscore.Revb.IN_COEF_L))>>16;
		const s32 IIR_INPUT_A1 = ((_spu2mem[src_a1] * thiscore.Revb.IIR_COEF) + (INPUT_SAMPLE_R * thiscore.Revb.IN_COEF_R))>>16;
		const s32 IIR_INPUT_B0 = ((_spu2mem[src_b0] * thiscore.Revb.IIR_COEF) + (INPUT_SAMPLE_L * thiscore.Revb.IN_COEF_L))>>16;
		const s32 IIR_INPUT_B1 = ((_spu2mem[src_b1] * thiscore.Revb.IIR_COEF) + (INPUT_SAMPLE_R * thiscore.Revb.IN_COEF_R))>>16;

		const s32 IIR_A0 = (IIR_INPUT_A0 * thiscore.Revb.IIR_ALPHA) + (_spu2mem[dest_a0] * (0x7fff - thiscore.Revb.IIR_ALPHA));
		const s32 IIR_A1 = (IIR_INPUT_A1 * thiscore.Revb.IIR_ALPHA) + (_spu2mem[dest_a1] * (0x7fff - thiscore.Revb.IIR_ALPHA));
		const s32 IIR_B0 = (IIR_INPUT_B0 * thiscore.Revb.IIR_ALPHA) + (_spu2mem[dest_b0] * (0x7fff - thiscore.Revb.IIR_ALPHA));
		const s32 IIR_B1 = (IIR_INPUT_B1 * thiscore.Revb.IIR_ALPHA) + (_spu2mem[dest_b1] * (0x7fff - thiscore.Revb.IIR_ALPHA));

		_spu2mem[dest2_a0] = clamp_mix( IIR_A0 >> 16 );
		_spu2mem[dest2_a1] = clamp_mix( IIR_A1 >> 16 );
		_spu2mem[dest2_b0] = clamp_mix( IIR_B0 >> 16 );
		_spu2mem[dest2_b1] = clamp_mix( IIR_B1 >> 16 );

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
		const s32 FB_B0 = (_spu2mem[fb_src_b0] * (0x7fff - thiscore.Revb.FB_ALPHA)); //>>16;
		const s32 FB_B1 = (_spu2mem[fb_src_b1] * (0x7fff - thiscore.Revb.FB_ALPHA)); //>>16;

		const s32 fb_xor_a0 = (_spu2mem[fb_src_a0] * ( thiscore.Revb.FB_ALPHA ^ 0x8000 ))>>2;
		const s32 fb_xor_a1 = (_spu2mem[fb_src_a1] * ( thiscore.Revb.FB_ALPHA ^ 0x8000 ))>>2;

		_spu2mem[mix_dest_a0] = clamp_mix( (ACC0 - FB_A0) >> 16 );
		_spu2mem[mix_dest_a1] = clamp_mix( (ACC1 - FB_A1) >> 16 );
		_spu2mem[mix_dest_b0] = clamp_mix( (MulShr32(thiscore.Revb.FB_ALPHA<<14, ACC0) - fb_xor_a0 - ((_spu2mem[fb_src_b0] * thiscore.Revb.FB_X)>>2)) >> 14 );
		_spu2mem[mix_dest_b1] = clamp_mix( (MulShr32(thiscore.Revb.FB_ALPHA<<14, ACC1) - fb_xor_a1 - ((_spu2mem[fb_src_b1] * thiscore.Revb.FB_X)>>2)) >> 14 );

		OutL = thiscore.LastEffectL = clamp_mix(_spu2mem[mix_dest_a0] + _spu2mem[mix_dest_b0]);
		OutR = thiscore.LastEffectR = clamp_mix(_spu2mem[mix_dest_a1] + _spu2mem[mix_dest_b1]);
	} 
}
