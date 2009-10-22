/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
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

// arranged in writearr, rowarr, colarr, updatearr
static const __aligned16 u32 s_maskarr[16][4] = {
	{0xffffffff, 0x00000000, 0x00000000, 0xffffffff},
	{0xffff0000, 0x0000ffff, 0x00000000, 0xffffffff},
	{0xffff0000, 0x00000000, 0x0000ffff, 0xffffffff},
	{0xffff0000, 0x00000000, 0x00000000, 0xffff0000},
	{0x0000ffff, 0xffff0000, 0x00000000, 0xffffffff},
	{0x00000000, 0xffffffff, 0x00000000, 0xffffffff},
	{0x00000000, 0xffff0000, 0x0000ffff, 0xffffffff},
	{0x00000000, 0xffff0000, 0x00000000, 0xffff0000},
	{0x0000ffff, 0x00000000, 0xffff0000, 0xffffffff},
	{0x00000000, 0x0000ffff, 0xffff0000, 0xffffffff},
	{0x00000000, 0x00000000, 0xffffffff, 0xffffffff},
	{0x00000000, 0x00000000, 0xffff0000, 0xffff0000},
	{0x0000ffff, 0x00000000, 0x00000000, 0x0000ffff},
	{0x00000000, 0x0000ffff, 0x00000000, 0x0000ffff},
	{0x00000000, 0x00000000, 0x0000ffff, 0x0000ffff},
	{0x00000000, 0x00000000, 0x00000000, 0x00000000}
};

extern u8 s_maskwrite[256];

// Dear C++: Please don't mangle this name, thanks!
extern "C" __aligned16 u32 s_TempDecompress[4];
__aligned16 u32 s_TempDecompress[4] = {0};

// Note: this function used to break regularly on Linux due to stack alignment.
// Refer to old revisions of this code if it breaks again for workarounds.
void __fastcall SetNewMask(u32* vif1masks, u32* hasmask, u32 mask, u32 oldmask)
{
	u32 i;
	u32 prev = 0;
	
	XMMRegisters::Freeze();
	for(i = 0; i < 4; ++i, mask >>= 8, oldmask >>= 8, vif1masks += 16) {

		prev |= s_maskwrite[mask&0xff];
		hasmask[i] = prev;

		if ((mask&0xff) != (oldmask&0xff))
		{
			__m128i r0, r1, r2, r3;
			r0 = _mm_load_si128((__m128i*)&s_maskarr[mask&15][0]); // Tends to crash Linux,
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
    XMMRegisters::Thaw();
}
