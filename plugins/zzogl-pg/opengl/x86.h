/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
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

#ifndef ZEROGS_X86
#define ZEROGS_X86

#include "GS.h"

#ifndef ZZNORMAL_MEMORY
// StarOcean use 24 in logo and 4HH and 4HL in menu subfont
// Tony hawk use 16, but have a lot of trouble
// This function move one blockwidth * blockheigh data block from src to dst, in assumption, that in dst we store swizzled data,
template <int psm>
inline void __fastcall SwizzleBlock(u32* dst, u32* src, int pitch, u32 WriteMask = 0xffffffff) {
	u8 B = (PSM_PIXELS_PER_WORD<psm>() > 2)? 4 : 2;

	assert ((pitch & 3) == 0 );

	u32* src1 = src;
	u32* src2 = src + pitch / 4;

	for(int j = 0; j < 4 ; j++, src1 += B * pitch / 4, src2 += B * pitch / 4)
		for(int i = 0; i < 8; i++) {
			fillPixelsFromMemory<psm>(dst, src1, i, B * j, pitch /4, 0, 0, WriteMask);
			fillPixelsFromMemory<psm>(dst, src2, i, B * j + 1, pitch / 4 , 0, 0, WriteMask);
		}
}

// Simply AA multiplication. We does not use src[j << AA], but prefer to keep more central pixel in data.
// We does not use mixing of neighbour pixels, because it does not give any noticiable bonus, but speed penalty is big.
template <u8 AA>
inline u32 mixed_pixel(u32* src, int j) {
	if (AA == 0)
		return src[j] ;

	if (AA == 1) 
		return src[(j << 1) + 1];	

	if (AA == 2)
		return src[(j << 2) + 2];
}

// We fill destination word for pixel number j (j < 8). For 16-bit storage upper size of this word is pixel of j + 8,
// and RGBA data should be convert to ARGB16.
// WARNING: floating storage is never be testing
template <int psm, bool is_float, u8 AA>
inline u32 convert_pixel(u32* src, int j) {
	if (is_float) {
		Vector_16F* fsrc = (Vector_16F*)src;									// We use simplified code for float, it seems not 
															// to be used anyway.
		if (PSM_ISHALF<psm>()) {
			return Float16ToARGB16 ( fsrc[j << AA]) + (Float16ToARGB16(fsrc[(j + 8) << AA]) << 16);
		}
		else {
			return Float16ToARGB ( fsrc[j << AA] );
		}
	}
	else {
		if (PSM_ISHALF<psm>()) {
			return RGBA32to16(mixed_pixel<AA>(src, j)) + (RGBA32to16(mixed_pixel<AA>(src, j + 8)) << 16);
		}
		else {
			return mixed_pixel<AA>(src, j);
		}
	}
}

// put data in u32 destination word for pixel x, y < 8 in swizzled block. Note, that in 16-bit target we put 2 pixels (x,y 
// and x+8, y) in the same word. 
template <int pix, int x, int y, int psm, bool is_float, u8 AA>
inline void SettleSwizzlePixel(u32* dst, u32* src, int srcpitch, u32 mask) {
	u32 tmp = convert_pixel<psm, is_float, AA>(src + y * srcpitch, x);
	MaskedOR (dst + pix, tmp, mask);										// Don't forget to use mask. 
}

// Put in dst memory location swizzled block for src. We does not calculate pixel address there at all.
template <int psm, bool is_float, u8 AA>
void __fastcall FrameSwizzleBlock(u32* dst, int sj, int si, u32* src, int srcpitch, u32 WriteMask) {
	u32 mask = HandleWritemask<psm>(WriteMask);									// This function made correct mask for 32, 24 and 16 target's

	for (int i = 0; i < 4; i++) {
		SettleSwizzlePixel<0, 0, 0, psm, is_float, AA>(dst, src, srcpitch, mask);				// it's possible to put one for here, but I don't know, what's faster
		SettleSwizzlePixel<1, 1, 0, psm, is_float, AA>(dst, src, srcpitch, mask);
		SettleSwizzlePixel<2, 0, 1, psm, is_float, AA>(dst, src, srcpitch, mask);
		SettleSwizzlePixel<3, 1, 1, psm, is_float, AA>(dst, src, srcpitch, mask);
		SettleSwizzlePixel<4, 2, 0, psm, is_float, AA>(dst, src, srcpitch, mask);
		SettleSwizzlePixel<5, 3, 0, psm, is_float, AA>(dst, src, srcpitch, mask);
		SettleSwizzlePixel<6, 2, 1, psm, is_float, AA>(dst, src, srcpitch, mask);
		SettleSwizzlePixel<7, 3, 1, psm, is_float, AA>(dst, src, srcpitch, mask);
		SettleSwizzlePixel<8, 4, 0, psm, is_float, AA>(dst, src, srcpitch, mask);
		SettleSwizzlePixel<9, 5, 0, psm, is_float, AA>(dst, src, srcpitch, mask);
		SettleSwizzlePixel<10, 4, 1, psm, is_float, AA>(dst, src, srcpitch, mask);
		SettleSwizzlePixel<11, 5, 1, psm, is_float, AA>(dst, src, srcpitch, mask);		
		SettleSwizzlePixel<12, 6, 0, psm, is_float, AA>(dst, src, srcpitch, mask);
		SettleSwizzlePixel<13, 7, 0, psm, is_float, AA>(dst, src, srcpitch, mask);
		SettleSwizzlePixel<14, 6, 1, psm, is_float, AA>(dst, src, srcpitch, mask);
		SettleSwizzlePixel<15, 7, 1, psm, is_float, AA>(dst, src, srcpitch, mask);

		src += 2 * srcpitch; 
		dst += 16;
	}
}
#endif
#endif
