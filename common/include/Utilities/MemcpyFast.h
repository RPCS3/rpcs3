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

#pragma once

#ifdef __LINUX__

#	include "lnx_memzero.h"

	extern "C" void __fastcall memcpy_amd_(void *dest, const void *src, size_t bytes);
	extern "C" u8 memcmp_mmx(const void* src1, const void* src2, int cmpsize);
	extern "C" void memxor_mmx(void* dst, const void* src1, int cmpsize);

	// This can be moved later, but Linux doesn't even compile memcpyFast.cpp, so I figured I'd stick it here for now.
	// Quadword Copy! Count is in QWCs (128 bits).  Neither source nor dest need to be aligned.
	static __forceinline void memcpy_amd_qwc(void *dest, const void *src, size_t qwc)
	{	
		// Optimization Analysis: This code is *nearly* optimal.  Do not think that using XMM
		// registers will improve copy performance, because they won't.  Use of XMMs is only
		// warranted in situations where both source and dest are guaranteed aligned to 16 bytes,
		// and even then the benefits are typically minimal (sometimes slower depending on the
		// amount of data being copied).
		//
		// Thus: MMX are alignment safe, fast, and widely available.  Lets just stick with them.
		//   --air

		// Linux Conversion note:
		//  This code would benefit nicely from having inline-able GAS syntax, since it should
		//  allow GCC to optimize the first 3 instructions out of existence in many scenarios.
		//  And its called enough times to probably merit the extra effort to ensure proper
		//  optimization. --air

		__asm__
		(
			".intel_syntax noprefix\n"
				//"mov		ecx, [%[dest]]\n"
				//"mov		edx, [%[src]]\n"
				//"mov		eax, [%[qwc]]\n"			// keep a copy of count
				"mov		eax, %[qwc]\n"
				"shr		eax, 1\n"
				"jz			memcpy_qwc_1\n"				// only one 16 byte block to copy?

				"cmp		%[qwc], 64\n"				// "IN_CACHE_COPY/32"
				"jb			memcpy_qwc_loop1\n"			// small copies should be cached (definite speedup --air)
		
			"memcpy_qwc_loop2:\n"						// 32-byte blocks, uncached copy
				"prefetchnta [%[src] + 568]\n"			// start reading ahead (tested: it helps! --air)

				"movq		mm0,[%[src]+0]\n"			// read 64 bits
				"movq		mm1,[%[src]+8]\n"
				"movq		mm2,[%[src]+16]\n"
				"movntq		[%[dest]+0], mm0\n"			// write 64 bits, bypassing the cache
				"movntq		[%[dest]+8], mm1\n"
				"movq		mm3,[%[src]+24]\n"
				"movntq		[%[dest]+16], mm2\n"
				"movntq		[%[dest]+24], mm3\n"

				"add		%[src],32\n"				// update source pointer
				"add		%[dest],32\n"				// update destination pointer
				"sub		eax,1\n"
				"jnz		memcpy_qwc_loop2\n"			// last 64-byte block?
				"sfence\n"								// flush the write buffer
				"jmp		memcpy_qwc_1\n"

			// 32-byte blocks, cached!
			// This *is* important.  Removing this and using exclusively non-temporal stores
			// results in noticeable speed loss!

			"memcpy_qwc_loop1:\n"				
				"prefetchnta [%[src] + 568]\n"			// start reading ahead (tested: it helps! --air)

				"movq		mm0,[%[src]+0]\n"			// read 64 bits
				"movq		mm1,[%[src]+8]\n"
				"movq		mm2,[%[src]+16]\n"
				"movq		[%[dest]+0], mm0\n"			// write 64 bits, bypassing the cache
				"movq		[%[dest]+8], mm1\n"
				"movq		mm3,[%[src]+24]\n"
				"movq		[%[dest]+16], mm2\n"
				"movq		[%[dest]+24], mm3\n"

				"add		%[src],32\n"				// update source pointer
				"add		%[dest],32\n"				// update destination pointer
				"sub		eax,1\n"
				"jnz		memcpy_qwc_loop1\n"			// last 64-byte block?

			"memcpy_qwc_1:\n"
				"test		[%qwc],1\n"
				"jz			memcpy_qwc_final\n"
				"movq		mm0,[%[src]]\n"
				"movq		mm1,[%[src]+8]\n"
				"movq		[%[dest]], mm0\n"
				"movq		[%[dest]+8], mm1\n"

			"memcpy_qwc_final:\n"
				"emms\n"								// clean up the MMX state
			".att_syntax\n"
					: "=&r"(dest), "=&r"(src), "=&r"(qwc)
					: [dest]"0"(dest), [src]"1"(src), [qwc]"2"(qwc)
					: "memory", "eax", "mm0", "mm1", "mm2", "mm3"
		);
	}
#else

#	include "win_memzero.h"

	extern void __fastcall memcpy_amd_(void *dest, const void *src, size_t bytes);
	extern void memcpy_amd_qwc(void *dest, const void *src, size_t bytes);
	extern u8 memcmp_mmx(const void* src1, const void* src2, int cmpsize);
	extern void memxor_mmx(void* dst, const void* src1, int cmpsize);

#endif

// Only used in the Windows version of memzero.h. But it's in Misc.cpp for some reason.
void _memset16_unaligned( void* dest, u16 data, size_t size );

#define memcpy_fast				memcpy_amd_ // Fast memcpy
#define memcpy_aligned(d,s,c)	memcpy_amd_(d,s,c)	// Memcpy with 16-byte Aligned addresses
#define memcpy_const			memcpy_amd_	// Memcpy with constant size
#define memcpy_constA			memcpy_amd_ // Memcpy with constant size and 16-byte aligned

#define memcpy_qwc(d,s,c)		memcpy_amd_qwc(d,s,c)
//#define memcpy_qwc(d,s,c)		memcpy_amd_(d,s,c*16)
