/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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


// IPU-correct yuv conversions by Pseudonym
// SSE2 Implementation by Pseudonym

#include "PrecompiledHeader.h"

#include "Common.h"
#include "IPU.h"
#include "yuv2rgb.h"
#include "mpeg2lib/Mpeg.h"

// The IPU's colour space conversion conforms to ITU-R Recommendation BT.601 if anyone wants to make a
// faster or "more accurate" implementation, but this is the precise documented integer method used by
// the hardware and is fast enough with SSE2.

#define IPU_Y_BIAS 16
#define IPU_C_BIAS 128
#define IPU_Y_COEFF 0x95	//  1.1640625
#define IPU_GCR_COEFF -0x68	// -0.8125
#define IPU_GCB_COEFF -0x32	// -0.390625
#define IPU_RCR_COEFF 0xcc	//  1.59375
#define IPU_BCB_COEFF 0x102	//  2.015625

// conforming implementation for reference, do not optimise
void yuv2rgb_reference(void)
{
	const macroblock_8& mb8 = decoder.mb8;
	macroblock_rgb32& rgb32 = decoder.rgb32;

	for (int y = 0; y < 16; y++)
		for (int x = 0; x < 16; x++)
		{
			s32 lum = (IPU_Y_COEFF * (max(0, (s32)mb8.Y[y][x] - IPU_Y_BIAS))) >> 6;
			s32 rcr = (IPU_RCR_COEFF * ((s32)mb8.Cr[y>>1][x>>1] - 128)) >> 6;
			s32 gcr = (IPU_GCR_COEFF * ((s32)mb8.Cr[y>>1][x>>1] - 128)) >> 6;
			s32 gcb = (IPU_GCB_COEFF * ((s32)mb8.Cb[y>>1][x>>1] - 128)) >> 6;
			s32 bcb = (IPU_BCB_COEFF * ((s32)mb8.Cb[y>>1][x>>1] - 128)) >> 6;

			rgb32.c[y][x].r = max(0, min(255, (lum + rcr + 1) >> 1));
			rgb32.c[y][x].g = max(0, min(255, (lum + gcr + gcb + 1) >> 1));
			rgb32.c[y][x].b = max(0, min(255, (lum + bcb + 1) >> 1));
			rgb32.c[y][x].a = 0x80; // the norm to save doing this on the alpha pass
		}
}

// Everything below is bit accurate to the IPU specification (except maybe rounding).
// Know the specification before you touch it.
#define SSE_BYTES(x) {x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x}
#define SSE_WORDS(x) {x, x, x, x, x, x, x, x}
#define SSE_COEFFICIENTS(x) SSE_WORDS((x)<<2)

struct SSE2_Tables
{
	u16	C_bias[8];			// offset -64
	u8	Y_bias[16];			// offset -48
	u16 Y_mask[8];			// offset -32
	u16 round_1bit[8];		// offset -16

	s16 Y_coefficients[8];	// offset 0
	s16 GCr_coefficients[8];// offset 16
	s16 GCb_coefficients[8];// offset 32
	s16 RCr_coefficients[8];// offset 48
	s16 BCb_coefficients[8];// offset 64
};

enum
{
	C_BIAS     = -0x40,
	Y_BIAS     = -0x30,
	Y_MASK     = -0x20,
	ROUND_1BIT = -0x10,

	Y_COEFF     = 0x00,
	GCr_COEFF   = 0x10,
	GCb_COEFF   = 0x20,
	RCr_COEFF   = 0x30,
	BCb_COEFF   = 0x40
};

static const __aligned16 SSE2_Tables sse2_tables =
{
	SSE_WORDS(0x8000),		// c_bias
	SSE_BYTES(IPU_Y_BIAS),	// y_bias
	SSE_WORDS(0xff00),		// y_mask

	// Specifying round off instead of round down as everywhere else
	// implies that this is right
	SSE_WORDS(1),		// round_1bit

	SSE_COEFFICIENTS(IPU_Y_COEFF),
	SSE_COEFFICIENTS(IPU_GCR_COEFF),
	SSE_COEFFICIENTS(IPU_GCB_COEFF),
	SSE_COEFFICIENTS(IPU_RCR_COEFF),
	SSE_COEFFICIENTS(IPU_BCB_COEFF),
};

static __aligned16 u16 yuv2rgb_temp[3][8];

// This could potentially be improved for SSE4
__ri void yuv2rgb_sse2(void)
{
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
	__asm {
		mov eax, 1
		xor esi, esi
		xor edi, edi

		// Use ecx and edx as base pointers, to allow for Mod/RM form on memOps.
		// This saves 2-3 bytes per instruction where these are used. :)
		mov ecx, offset yuv2rgb_temp
		mov edx, offset sse2_tables+64;

		align 16
tworows:
		movq xmm3, qword ptr [decoder.mb8+256+esi]
		movq xmm1, qword ptr [decoder.mb8+320+esi]
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
		pxor xmm2, xmmword ptr [edx+C_BIAS] // xmm2 <-- 8 x (Cb - 128) << 8
		pxor xmm0, xmmword ptr [edx+C_BIAS] // xmm0 <-- 8 x (Cr - 128) << 8

		movaps xmm1, xmm0
		movaps xmm3, xmm2
		pmulhw xmm1, xmmword ptr [edx+GCr_COEFF]
		pmulhw xmm3, xmmword ptr [edx+GCb_COEFF]
		pmulhw xmm0, xmmword ptr [edx+RCr_COEFF]
		pmulhw xmm2, xmmword ptr [edx+BCb_COEFF]
		paddsw xmm1, xmm3
		// store for the next line; looking at the code above
		// compared to the code below, I have to wonder whether
		// this was worth the hassle
		movaps xmmword ptr [ecx], xmm0
		movaps xmmword ptr [ecx+16], xmm1
		movaps xmmword ptr [ecx+32], xmm2
		jmp ihatemsvc

		align 16
onerow:
		movaps xmm0, xmmword ptr [ecx]
		movaps xmm1, xmmword ptr [ecx+16]
		movaps xmm2, xmmword ptr [ecx+32]

// If masm directives worked properly in inline asm, I'd be using them,
// but I'm not inclined to write ~70 line #defines to simulate them.
// Maybe the function's faster like this anyway because it's smaller?
// I'd have to write a 70 line #define to benchmark it.

ihatemsvc:
		movaps xmm3, xmm0
		movaps xmm4, xmm1
		movaps xmm5, xmm2

		movaps xmm6, xmmword ptr [decoder.mb8+edi]
		psubusb xmm6, xmmword ptr [edx+Y_BIAS]
		movaps xmm7, xmm6
		psllw xmm6, 8                    // xmm6 <- Y << 8 for pixels 0,2,4,6,8,10,12,14
		pand xmm7, xmmword ptr [edx+Y_MASK]  // xmm7 <- Y << 8 for pixels 1,3,5,7,9,11,13,15

		pmulhuw xmm6, xmmword ptr [edx+Y_COEFF]
		pmulhuw xmm7, xmmword ptr [edx+Y_COEFF]

		paddsw xmm0, xmm6
		paddsw xmm3, xmm7
		paddsw xmm1, xmm6
		paddsw xmm4, xmm7
		paddsw xmm2, xmm6
		paddsw xmm5, xmm7

		// 0x80; a constant is probably so much better
		pcmpeqb xmm7, xmm7
		psllw xmm7, 15
		psrlw xmm7, 8
		packuswb xmm7, xmm7

		// round
		movaps xmm6, xmmword ptr [edx+ROUND_1BIT]
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
		movaps xmmword ptr [decoder.rgb32+edi*4+0], xmm0
		movaps xmmword ptr [decoder.rgb32+edi*4+16], xmm1
		movaps xmmword ptr [decoder.rgb32+edi*4+32], xmm3
		movaps xmmword ptr [decoder.rgb32+edi*4+48], xmm4

		add edi, 16

		neg eax
		jl onerow // run twice

		add esi, 8
		cmp esi, 64
		jne tworows
	}

#elif defined(__GNUC__)

	// offset to the middle of the sse2 table, so that we can use 1-byte address displacement
	// to access all fields:
	static const u8* sse2_tableoffset = ((u8*)&sse2_tables) + 64;
	static const u8* mb8 = (u8*)&decoder.mb8;
	static u8* rgb32 = (u8*)&decoder.rgb32;

	__asm__ __volatile__ (
		".intel_syntax noprefix\n"
		"xor esi, esi\n"
		"xor edi, edi\n"

		".align 16\n"
"tworows_%=:\n"
		"movq xmm3, qword ptr [%[mb8]+256+esi]\n"
		"movq xmm1, qword ptr [%[mb8]+320+esi]\n"
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
		"pxor xmm2, xmmword ptr [%[sse2_tables]+%c[C_BIAS]]\n" // xmm2 <-- 8 x (Cb - 128) << 8
		"pxor xmm0, xmmword ptr [%[sse2_tables]+%c[C_BIAS]]\n" // xmm0 <-- 8 x (Cr - 128) << 8

		"movaps xmm1, xmm0\n"
		"movaps xmm3, xmm2\n"
		"pmulhw xmm1, xmmword ptr [%[sse2_tables]+%c[GCr_COEFF]]\n"
		"pmulhw xmm3, xmmword ptr [%[sse2_tables]+%c[GCb_COEFF]]\n"
		"pmulhw xmm0, xmmword ptr [%[sse2_tables]+%c[RCr_COEFF]]\n"
		"pmulhw xmm2, xmmword ptr [%[sse2_tables]+%c[BCb_COEFF]]\n"
		"paddsw xmm1, xmm3\n"
		// store for the next line; looking at the code above
		// compared to the code below, I have to wonder whether
		// this was worth the hassle
		"movaps xmmword ptr [%[yuv2rgb_temp]], xmm0\n"
		"movaps xmmword ptr [%[yuv2rgb_temp]+16], xmm1\n"
		"movaps xmmword ptr [%[yuv2rgb_temp]+32], xmm2\n"
		"jmp ihategcctoo_%=\n"

		".align 16\n"
"onerow_%=:\n"
		"movaps xmm0, xmmword ptr [%[yuv2rgb_temp]]\n"
		"movaps xmm1, xmmword ptr [%[yuv2rgb_temp]+16]\n"
		"movaps xmm2, xmmword ptr [%[yuv2rgb_temp]+32]\n"

"ihategcctoo_%=:\n"
		"movaps xmm3, xmm0\n"
		"movaps xmm4, xmm1\n"
		"movaps xmm5, xmm2\n"

		"movaps xmm6, xmmword ptr [%[mb8]+edi]\n"
		"psubusb xmm6, xmmword ptr [%[sse2_tables]+%c[Y_BIAS]]\n"
		"movaps xmm7, xmm6\n"
		"psllw xmm6, 8\n"                   // xmm6 <- Y << 8 for pixels 0,2,4,6,8,10,12,14
		"pand xmm7, xmmword ptr [%[sse2_tables]+%c[Y_MASK]]\n" // xmm7 <- Y << 8 for pixels 1,3,5,7,9,11,13,15

		"pmulhuw xmm6, xmmword ptr [%[sse2_tables]+%c[Y_COEFF]]\n"
		"pmulhuw xmm7, xmmword ptr [%[sse2_tables]+%c[Y_COEFF]]\n"

		"paddsw xmm0, xmm6\n"
		"paddsw xmm3, xmm7\n"
		"paddsw xmm1, xmm6\n"
		"paddsw xmm4, xmm7\n"
		"paddsw xmm2, xmm6\n"
		"paddsw xmm5, xmm7\n"

		// 0x80; a constant is probably so much better
		"pcmpeqb xmm7, xmm7\n"
		"psllw xmm7, 15\n"
		"psrlw xmm7, 8\n"
		"packuswb xmm7, xmm7\n"

		// round
		"movaps xmm6, xmmword ptr [%[sse2_tables]+%c[ROUND_1BIT]]\n"
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
		"movaps xmmword ptr [%[rgb32]+edi*4+0], xmm0\n"
		"movaps xmmword ptr [%[rgb32]+edi*4+16], xmm1\n"
		"movaps xmmword ptr [%[rgb32]+edi*4+32], xmm3\n"
		"movaps xmmword ptr [%[rgb32]+edi*4+48], xmm4\n"

		"add edi, 16\n"

        // run twice the onerow <=> edi = 16 or 48 or 80 etc... <=> check bit 5
		"test edi, 16\n"
		"jnz onerow_%=\n"

		"add esi, 8\n"
		"cmp esi, 64\n"
		"jne tworows_%=\n"
		".att_syntax\n"
		:
		:[C_BIAS]"i"(C_BIAS), [Y_BIAS]"i"(Y_BIAS), [Y_MASK]"i"(Y_MASK),
			[ROUND_1BIT]"i"(ROUND_1BIT), [Y_COEFF]"i"(Y_COEFF), [GCr_COEFF]"i"(GCr_COEFF),
			[GCb_COEFF]"i"(GCb_COEFF), [RCr_COEFF]"i"(RCr_COEFF), [BCb_COEFF]"i"(BCb_COEFF),
            // Use ecx and edx as base pointers, to allow for Mod/RM form on memOps.
            // This saves 2-3 bytes per instruction where these are used. :)
			[yuv2rgb_temp]"c"(yuv2rgb_temp), [sse2_tables]"d"(sse2_tableoffset),
			[mb8]"r"(mb8), [rgb32]"r"(rgb32)
		: "esi", "edi", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "memory"
	);
#else
#	error Unsupported compiler
#endif
}
