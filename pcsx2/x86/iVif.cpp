/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"

#include "Common.h"
#include "Vif.h"
#include "VUmicro.h"

#include <xmmintrin.h>
#include <emmintrin.h>

// sse2 highly optimized vif (~200 separate functions are built) zerofrog(@gmail.com)
extern u32 g_vif1Masks[48], g_vif0Masks[48];
extern u32 g_vif1HasMask3[4], g_vif0HasMask3[4];

//static const u32 writearr[4] = { 0xffffffff, 0, 0, 0 };
//static const u32 rowarr[4] = { 0, 0xffffffff, 0, 0 };
//static const u32 colarr[4] = { 0, 0, 0xffffffff, 0 };
//static const u32 updatearr[4] = {0xffffffff, 0xffffffff, 0xffffffff, 0 };

// arranged in writearr, rowarr, colarr, updatearr
static PCSX2_ALIGNED16(u32 s_maskarr[16][4]) = {
	0xffffffff, 0x00000000, 0x00000000, 0xffffffff,
	0xffff0000, 0x0000ffff, 0x00000000, 0xffffffff,
	0xffff0000, 0x00000000, 0x0000ffff, 0xffffffff,
	0xffff0000, 0x00000000, 0x00000000, 0xffff0000,
	0x0000ffff, 0xffff0000, 0x00000000, 0xffffffff,
	0x00000000, 0xffffffff, 0x00000000, 0xffffffff,
	0x00000000, 0xffff0000, 0x0000ffff, 0xffffffff,
	0x00000000, 0xffff0000, 0x00000000, 0xffff0000,
	0x0000ffff, 0x00000000, 0xffff0000, 0xffffffff,
	0x00000000, 0x0000ffff, 0xffff0000, 0xffffffff,
	0x00000000, 0x00000000, 0xffffffff, 0xffffffff,
	0x00000000, 0x00000000, 0xffff0000, 0xffff0000,
	0x0000ffff, 0x00000000, 0x00000000, 0x0000ffff,
	0x00000000, 0x0000ffff, 0x00000000, 0x0000ffff,
	0x00000000, 0x00000000, 0x0000ffff, 0x0000ffff,
	0x00000000, 0x00000000, 0x00000000, 0x00000000
};

extern u8 s_maskwrite[256];

extern "C" PCSX2_ALIGNED16(u32 s_TempDecompress[4]) = {0};

//#if defined(_MSC_VER)

void __fastcall SetNewMask(u32* vif1masks, u32* hasmask, u32 mask, u32 oldmask)
{
    u32 i;
	u32 prev = 0;
	FreezeXMMRegs(1);
	for(i = 0; i < 4; ++i, mask >>= 8, oldmask >>= 8, vif1masks += 16) {

		prev |= s_maskwrite[mask&0xff];//((mask&3)==3)||((mask&0xc)==0xc)||((mask&0x30)==0x30)||((mask&0xc0)==0xc0);
		hasmask[i] = prev;

		if( (mask&0xff) != (oldmask&0xff) ) {
			__m128i r0, r1, r2, r3;
			r0 = _mm_load_si128((__m128i*)&s_maskarr[mask&15][0]);
			r2 = _mm_unpackhi_epi16(r0, r0);
			r0 = _mm_unpacklo_epi16(r0, r0);

			r1 = _mm_load_si128((__m128i*)&s_maskarr[(mask>>4)&15][0]);
			r3 = _mm_unpackhi_epi16(r1, r1);
			r1 = _mm_unpacklo_epi16(r1, r1);

			_mm_storel_pi((__m64*)&vif1masks[0], *(__m128*)&r0);
			_mm_storel_pi((__m64*)&vif1masks[2], *(__m128*)&r1);
			_mm_storeh_pi((__m64*)&vif1masks[4], *(__m128*)&r0);
			_mm_storeh_pi((__m64*)&vif1masks[6], *(__m128*)&r1);

			_mm_storel_pi((__m64*)&vif1masks[8], *(__m128*)&r2);
			_mm_storel_pi((__m64*)&vif1masks[10], *(__m128*)&r3);
			_mm_storeh_pi((__m64*)&vif1masks[12], *(__m128*)&r2);
			_mm_storeh_pi((__m64*)&vif1masks[14], *(__m128*)&r3);
		}
	}
	FreezeXMMRegs(0);
}


/*#else // gcc
// Is this really supposed to be assembly for gcc and C for Windows?
void __fastcall SetNewMask(u32* vif1masks, u32* hasmask, u32 mask, u32 oldmask)
{
    u32 i;
	u32 prev = 0;
	FreezeXMMRegs(1);

	for(i = 0; i < 4; ++i, mask >>= 8, oldmask >>= 8, vif1masks += 16) {

		prev |= s_maskwrite[mask&0xff];//((mask&3)==3)||((mask&0xc)==0xc)||((mask&0x30)==0x30)||((mask&0xc0)==0xc0);
		hasmask[i] = prev;

		if( (mask&0xff) != (oldmask&0xff) ) {
            u8* p0 = (u8*)&s_maskarr[mask&15][0];
            u8* p1 = (u8*)&s_maskarr[(mask>>4)&15][0];

            __asm__(".intel_syntax noprefix\n"
                "movaps xmm0, [%0]\n"
                "movaps xmm1, [%1]\n"
                "movaps xmm2, xmm0\n"
                "punpcklwd xmm0, xmm0\n"
                "punpckhwd xmm2, xmm2\n"
                "movaps xmm3, xmm1\n"
                "punpcklwd xmm1, xmm1\n"
                "punpckhwd xmm3, xmm3\n"
                "movq [%2], xmm0\n"
                "movq [%2+8], xmm1\n"
                "movhps [%2+16], xmm0\n"
                "movhps [%2+24], xmm1\n"
                "movq [%2+32], xmm2\n"
                "movq [%2+40], xmm3\n"
                "movhps [%2+48], xmm2\n"
                "movhps [%2+56], xmm3\n"
                    ".att_syntax\n" : : "r"(p0), "r"(p1), "r"(vif1masks) );
		}
	}
	FreezeXMMRegs(0);
}

#endif*/
