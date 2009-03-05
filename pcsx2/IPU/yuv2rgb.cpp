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

// IPU-correct yuv conversions by Pseudonym
// SSE2 Implementation by Pseudonym

#include "PrecompiledHeader.h"

#include "System.h"
#include "IPU.h"
#include "yuv2rgb.h"

// Everything below is bit accurate to the IPU specification (except maybe rounding).
// Know the specification before you touch it.

PCSX2_ALIGNED16(u16 C_bias)[8] = {0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000};
PCSX2_ALIGNED16(u8 Y_bias)[16] = {16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16};
#define SSE_COEFFICIENTS(name, x) \
	PCSX2_ALIGNED16(u16 name)[8] = {x<<2,x<<2,x<<2,x<<2,x<<2,x<<2,x<<2,x<<2};
SSE_COEFFICIENTS(Y_coefficients, 0x95);    // 1.1640625
SSE_COEFFICIENTS(RCr_coefficients, 0xcc);  // 1.59375
SSE_COEFFICIENTS(GCr_coefficients, (-0x68));  // -0.8125
SSE_COEFFICIENTS(GCb_coefficients, (-0x32));  // -0.390625
SSE_COEFFICIENTS(BCb_coefficients, 0x102); // 2.015625
PCSX2_ALIGNED16(u16 Y_mask)[8] = {0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,0xff00};
// Specifying round off instead of round down as everywhere else
// implies that this is right
PCSX2_ALIGNED16(u16 round_1bit)[8] = {1,1,1,1,1,1,1,1};
PCSX2_ALIGNED16(u16 yuv2rgb_temp)[3][8];

// This could potentially be improved for SSE4
void yuv2rgb_sse2(void)
{
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
	__asm {
		mov eax, 1
		mov esi, 0
		mov edi, 0

		align 16
tworows:
		movq xmm3, qword ptr [mb8+256+esi]
		movq xmm1, qword ptr [mb8+320+esi]
		pxor xmm2, xmm2
		pxor xmm0, xmm0
		// could skip the movq but punpck requires 128-bit alignment
		// for some reason, so two versions would be needed,
		// bloating the function (further)
		punpcklbw xmm2, xmm3
		punpcklbw xmm0, xmm1
		// unfortunately I don't think this will matter despite being
		// technically potentially a little faster, but this is
		// equivalent to an add or sub
		pxor xmm2, xmmword ptr [C_bias] // xmm2 <-- 8 x (Cb - 128) << 8
		pxor xmm0, xmmword ptr [C_bias] // xmm0 <-- 8 x (Cr - 128) << 8

		movaps xmm1, xmm0
		movaps xmm3, xmm2
		pmulhw xmm1, xmmword ptr [GCr_coefficients]
		pmulhw xmm3, xmmword ptr [GCb_coefficients]
		pmulhw xmm0, xmmword ptr [RCr_coefficients]
		pmulhw xmm2, xmmword ptr [BCb_coefficients]
		paddsw xmm1, xmm3
		// store for the next line; looking at the code above
		// compared to the code below, I have to wonder whether
		// this was worth the hassle
		movaps xmmword ptr [yuv2rgb_temp], xmm0
		movaps xmmword ptr [yuv2rgb_temp+16], xmm1
		movaps xmmword ptr [yuv2rgb_temp+32], xmm2
		jmp ihatemsvc

		align 16
onerow:
		movaps xmm0, xmmword ptr [yuv2rgb_temp]
		movaps xmm1, xmmword ptr [yuv2rgb_temp+16]
		movaps xmm2, xmmword ptr [yuv2rgb_temp+32]

// If masm directives worked properly in inline asm, I'd be using them,
// but I'm not inclined to write ~70 line #defines to simulate them.
// Maybe the function's faster like this anyway because it's smaller?
// I'd have to write a 70 line #define to benchmark it.

ihatemsvc:
		movaps xmm3, xmm0
		movaps xmm4, xmm1
		movaps xmm5, xmm2

		movaps xmm6, xmmword ptr [mb8+edi]
		psubusb xmm6, xmmword ptr [Y_bias]
		movaps xmm7, xmm6
		psllw xmm6, 8                    // xmm6 <- Y << 8 for pixels 0,2,4,6,8,10,12,14
		pand xmm7, xmmword ptr [Y_mask]  // xmm7 <- Y << 8 for pixels 1,3,5,7,9,11,13,15

		pmulhuw xmm6, xmmword ptr [Y_coefficients]
		pmulhuw xmm7, xmmword ptr [Y_coefficients]

		paddsw xmm0, xmm6
		paddsw xmm3, xmm7
		paddsw xmm1, xmm6
		paddsw xmm4, xmm7
		paddsw xmm2, xmm6
		paddsw xmm5, xmm7

		// round
		movaps xmm6, xmmword ptr [round_1bit]
		paddw xmm0, xmm6
		paddw xmm1, xmm6
		paddw xmm2, xmm6
		paddw xmm3, xmm6
		paddw xmm4, xmm6
		paddw xmm5, xmm6
		psraw xmm0, 1
		psraw xmm1, 1
		psraw xmm2, 1
		psraw xmm3, 1
		psraw xmm4, 1
		psraw xmm5, 1

		// combine even and odd bytes
		packuswb xmm0, xmm3
		packuswb xmm1, xmm4
		packuswb xmm2, xmm5
		movhlps xmm3, xmm0
		movhlps xmm4, xmm1
		movhlps xmm5, xmm2
		punpcklbw xmm0, xmm3 // Red bytes, back in order 
		punpcklbw xmm1, xmm4 // Green ""
		punpcklbw xmm2, xmm5 // Blue ""
		movaps xmm3, xmm0
		movaps xmm4, xmm1
		movaps xmm5, xmm2

		// Create RGBA (we could generate A here, but we don't) quads
		punpcklbw xmm0, xmm1
		punpcklbw xmm2, xmm7
		movaps xmm1, xmm0
		punpcklwd xmm0, xmm2
		punpckhwd xmm1, xmm2

		punpckhbw xmm3, xmm4
		punpckhbw xmm5, xmm7
		movaps xmm4, xmm3
		punpcklwd xmm3, xmm5
		punpckhwd xmm4, xmm5

		// at last
		movaps xmmword ptr [rgb32+edi*4+0], xmm0
		movaps xmmword ptr [rgb32+edi*4+16], xmm1
		movaps xmmword ptr [rgb32+edi*4+32], xmm3
		movaps xmmword ptr [rgb32+edi*4+48], xmm4

		add edi, 16

		neg eax
		jl onerow // run twice

		add esi, 8
		cmp esi, 64
		jne tworows
	}
#elif defined(__GNUC__)
	asm(
		".intel_syntax noprefix\n"
		"mov eax, 1\n"
		"mov esi, 0\n"
		"mov edi, 0\n"

		".align 16\n"
"tworows:\n"
		"movq xmm3, qword ptr [mb8+256+esi]\n"
		"movq xmm1, qword ptr [mb8+320+esi]\n"
		"pxor xmm2, xmm2\n"
		"pxor xmm0, xmm0\n"
		// could skip the movq but punpck requires 128-bit alignment
		// for some reason, so two versions would be needed,
		// bloating the function (further)
		"punpcklbw xmm2, xmm3\n"
		"punpcklbw xmm0, xmm1\n"
		// unfortunately I don't think this will matter despite being
		// technically potentially a little faster, but this is
		// equivalent to an add or sub
		"pxor xmm2, xmmword ptr [C_bias]\n" // xmm2 <-- 8 x (Cb - 128) << 8
		"pxor xmm0, xmmword ptr [C_bias]\n" // xmm0 <-- 8 x (Cr - 128) << 8

		"movaps xmm1, xmm0\n"
		"movaps xmm3, xmm2\n"
		"pmulhw xmm1, xmmword ptr [GCr_coefficients]\n"
		"pmulhw xmm3, xmmword ptr [GCb_coefficients]\n"
		"pmulhw xmm0, xmmword ptr [RCr_coefficients]\n"
		"pmulhw xmm2, xmmword ptr [BCb_coefficients]\n"
		"paddsw xmm1, xmm3\n"
		// store for the next line; looking at the code above
		// compared to the code below, I have to wonder whether
		// this was worth the hassle
		"movaps xmmword ptr [yuv2rgb_temp], xmm0\n"
		"movaps xmmword ptr [yuv2rgb_temp+16], xmm1\n"
		"movaps xmmword ptr [yuv2rgb_temp+32], xmm2\n"
		"jmp ihategcctoo\n"

		".align 16\n"
"onerow:\n"
		"movaps xmm0, xmmword ptr [yuv2rgb_temp]\n"
		"movaps xmm1, xmmword ptr [yuv2rgb_temp+16]\n"
		"movaps xmm2, xmmword ptr [yuv2rgb_temp+32]\n"

"ihategcctoo:\n"
		"movaps xmm3, xmm0\n"
		"movaps xmm4, xmm1\n"
		"movaps xmm5, xmm2\n"

		"movaps xmm6, xmmword ptr [mb8+edi]\n"
		"psubusb xmm6, xmmword ptr [Y_bias]\n"
		"movaps xmm7, xmm6\n"
		"psllw xmm6, 8\n"                   // xmm6 <- Y << 8 for pixels 0,2,4,6,8,10,12,14
		"pand xmm7, xmmword ptr [Y_mask]\n" // xmm7 <- Y << 8 for pixels 1,3,5,7,9,11,13,15

		"pmulhuw xmm6, xmmword ptr [Y_coefficients]\n"
		"pmulhuw xmm7, xmmword ptr [Y_coefficients]\n"

		"paddsw xmm0, xmm6\n"
		"paddsw xmm3, xmm7\n"
		"paddsw xmm1, xmm6\n"
		"paddsw xmm4, xmm7\n"
		"paddsw xmm2, xmm6\n"
		"paddsw xmm5, xmm7\n"

		// round
		"movaps xmm6, xmmword ptr [round_1bit]\n"
		"paddw xmm0, xmm6\n"
		"paddw xmm1, xmm6\n"
		"paddw xmm2, xmm6\n"
		"paddw xmm3, xmm6\n"
		"paddw xmm4, xmm6\n"
		"paddw xmm5, xmm6\n"
		"psraw xmm0, 1\n"
		"psraw xmm1, 1\n"
		"psraw xmm2, 1\n"
		"psraw xmm3, 1\n"
		"psraw xmm4, 1\n"
		"psraw xmm5, 1\n"

		// combine even and odd bytes
		"packuswb xmm0, xmm3\n"
		"packuswb xmm1, xmm4\n"
		"packuswb xmm2, xmm5\n"
		"movhlps xmm3, xmm0\n"
		"movhlps xmm4, xmm1\n"
		"movhlps xmm5, xmm2\n"
		"punpcklbw xmm0, xmm3\n" // Red bytes, back in order
		"punpcklbw xmm1, xmm4\n" // Green ""
		"punpcklbw xmm2, xmm5\n" // Blue ""
		"movaps xmm3, xmm0\n"
		"movaps xmm4, xmm1\n"
		"movaps xmm5, xmm2\n"

		// Create RGBA (we could generate A here, but we don't) quads
		"punpcklbw xmm0, xmm1\n"
		"punpcklbw xmm2, xmm7\n"
		"movaps xmm1, xmm0\n"
		"punpcklwd xmm0, xmm2\n"
		"punpckhwd xmm1, xmm2\n"

		"punpckhbw xmm3, xmm4\n"
		"punpckhbw xmm5, xmm7\n"
		"movaps xmm4, xmm3\n"
		"punpcklwd xmm3, xmm5\n"
		"punpckhwd xmm4, xmm5\n"

		// at last
		"movaps xmmword ptr [rgb32+edi*4+0], xmm0\n"
		"movaps xmmword ptr [rgb32+edi*4+16], xmm1\n"
		"movaps xmmword ptr [rgb32+edi*4+32], xmm3\n"
		"movaps xmmword ptr [rgb32+edi*4+48], xmm4\n"

		"add edi, 16\n"

		"neg eax\n"
		"jl onerow\n" // run twice

		"add esi, 8\n"
		"cmp esi, 64\n"
		"jne tworows\n"
		".att_syntax\n"
	);
#else
#error Unsupported compiler
#endif
}

void yuv2rgb_init(void)
{
	/* For later reimplementation of C version */
}
