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

#include "PrecompiledHeader.h"
#include "x86emitter/x86emitter.h"
#include <xmmintrin.h>

using namespace x86Emitter;

// Max Number of qwc supported
#define _maxSize 0x400

typedef void (__fastcall *_memCpyCall)(void*, void*);
__aligned16 _memCpyCall _memcpy_vibes[_maxSize+1];

#if 1

// this version uses SSE intrinsics to perform an inline copy.  MSVC disasm shows pretty
// decent code generation on whole, but it hasn't been benchmarked at all yet --air
__fi void memcpy_vibes(void * dest, const void * src, int size) {

	float (*destxmm)[4] = (float(*)[4])dest, (*srcxmm)[4] = (float(*)[4])src;
	size_t count = size & ~15, extra = size & 15;

	destxmm -= 8 - extra, srcxmm -= 8 - extra;
	switch (extra) {
		do {
			destxmm += 16, srcxmm += 16, count -= 16;
			_mm_store_ps(&destxmm[-8][0], _mm_load_ps(&srcxmm[-8][0]));
			case 15:
				_mm_store_ps(&destxmm[-7][0], _mm_load_ps(&srcxmm[-7][0]));
			case 14:
				_mm_store_ps(&destxmm[-6][0], _mm_load_ps(&srcxmm[-6][0]));
			case 13:
				_mm_store_ps(&destxmm[-5][0], _mm_load_ps(&srcxmm[-5][0]));
			case 12:
				_mm_store_ps(&destxmm[-4][0], _mm_load_ps(&srcxmm[-4][0]));
			case 11:
				_mm_store_ps(&destxmm[-3][0], _mm_load_ps(&srcxmm[-3][0]));
			case 10:
				_mm_store_ps(&destxmm[-2][0], _mm_load_ps(&srcxmm[-2][0]));
			case  9:
				_mm_store_ps(&destxmm[-1][0], _mm_load_ps(&srcxmm[-1][0]));
			case  8:
				_mm_store_ps(&destxmm[ 0][0], _mm_load_ps(&srcxmm[ 0][0]));
			case  7:
				_mm_store_ps(&destxmm[ 1][0], _mm_load_ps(&srcxmm[ 1][0]));
			case  6:
				_mm_store_ps(&destxmm[ 2][0], _mm_load_ps(&srcxmm[ 2][0]));
			case  5:
				_mm_store_ps(&destxmm[ 3][0], _mm_load_ps(&srcxmm[ 3][0]));
			case  4:
				_mm_store_ps(&destxmm[ 4][0], _mm_load_ps(&srcxmm[ 4][0]));
			case  3:
				_mm_store_ps(&destxmm[ 5][0], _mm_load_ps(&srcxmm[ 5][0]));
			case  2:
				_mm_store_ps(&destxmm[ 6][0], _mm_load_ps(&srcxmm[ 6][0]));
			case  1:
				_mm_store_ps(&destxmm[ 7][0], _mm_load_ps(&srcxmm[ 7][0]));
			case  0: NULL;
		} while (count);
	}
}

#else
#if 1
// This version creates one function with a lot of movaps
// It jumps to the correct movaps entry-point while adding
// the proper offset for adjustment...

static __pagealigned u8   _memCpyExec[__pagesize*16];

void gen_memcpy_vibes() {
	HostSys::MemProtectStatic(_memCpyExec, Protect_ReadWrite, false);
	memset (_memCpyExec, 0xcc, sizeof(_memCpyExec));
	xSetPtr(_memCpyExec);

	int off =-(((_maxSize & 0xf) - 7) << 4);
	for (int i = _maxSize, x = 0; i > 0; i--, x=(x+1)&7, off+=16) {

		_memcpy_vibes[i] = (_memCpyCall)xGetPtr();

		if (off >=  128) {
			off  = -128;
			xADD(edx, 256);
			xADD(ecx, 256);
		}
		const xRegisterSSE xmm_t(x);
		xMOVAPS (xmm_t, ptr32[edx+off]);
		xMOVNTPS(ptr32[ecx+off], xmm_t);
	}

	_memcpy_vibes[0] = (_memCpyCall)xGetPtr();

	xRET();
	pxAssert(((uptr)xGetPtr() - (uptr)_memCpyExec) < sizeof(_memCpyExec));

	HostSys::MemProtectStatic(_memCpyExec, Protect_ReadOnly, true);
}

__fi void memcpy_vibes(void * dest, const void * src, int size) {
	int offset = ((size & 0xf) - 7) << 4;
	_memcpy_vibes[size]((void*)((uptr)dest + offset), (void*)((uptr)src + offset));
}

#else

// This version creates '_maxSize' number of different functions,
// and calls the appropriate one...

static __pagealigned u8   _memCpyExec[__pagesize*_maxSize*2];

void gen_memcpy_vibes() {
	HostSys::MemProtectStatic(_memCpyExec, Protect_ReadWrite, false);
	memset (_memCpyExec, 0xcc, sizeof(_memCpyExec));
	xSetPtr(_memCpyExec);

	for (int i = 0; i < _maxSize+1; i++) 
	{
		int off = 0;
		_memcpy_vibes[i] = (_memCpyCall)xGetAlignedCallTarget();

		for (int j = 0, x = 0; j < i; j++, x=(x+1)&7, off+=16) {
			if (off >=  128) {
				off  = -128;
				xADD(edx, 256);
				xADD(ecx, 256);
			}
			const xRegisterSSE xmm_t(x);
			xMOVAPS(xmm_t, ptr32[edx+off]);
			xMOVAPS(ptr32[ecx+off], xmm_t);
		}

		xRET();
		pxAssert(((uptr)xGetPtr() - (uptr)_memCpyExec) < sizeof(_memCpyExec));
	}

	HostSys::MemProtectStatic(_memCpyExec, Protect_ReadOnly, true);
}

__fi void memcpy_vibes(void * dest, const void * src, int size) {
	_memcpy_vibes[size](dest, src);
}

#endif
#endif

// Since MemcpyVibes is already in the project, I'll just tuck the Linux version of memcpy_amd_qwc here for the moment,
// to get around compilation issues with having it in the headers.
#ifdef __LINUX__

	// This can be moved later, but Linux doesn't even compile memcpyFast.cpp, so I figured I'd stick it here for now.
	// Quadword Copy! Count is in QWCs (128 bits).  Neither source nor dest need to be aligned.
	__fi void memcpy_amd_qwc(void *dest, const void *src, size_t qwc)
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

		__asm__ __volatile__
		(
			".intel_syntax noprefix\n"
				"mov		eax, %[qwc]\n"				// keep a copy of count for looping
				"shr		eax, 1\n"
				"jz			memcpy_qwc_1_%=\n"				// only one 16 byte block to copy?

				"cmp		eax, 64\n"					// "IN_CACHE_COPY/32"
				"jb			memcpy_qwc_loop1_%=\n"			// small copies should be cached (definite speedup --air)
		
			"memcpy_qwc_loop2_%=:\n"						// 32-byte blocks, uncached copy
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
				"jnz		memcpy_qwc_loop2_%=\n"			// last 64-byte block?
				"sfence\n"								// flush the write buffer
				"jmp		memcpy_qwc_1_%=\n"

			// 32-byte blocks, cached!
			// This *is* important.  Removing this and using exclusively non-temporal stores
			// results in noticeable speed loss!

			"memcpy_qwc_loop1_%=:\n"				
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
				"jnz		memcpy_qwc_loop1_%=\n"			// last 64-byte block?

			"memcpy_qwc_1_%=:\n"
				"test %[qwc],1\n"
				"jz			memcpy_qwc_final_%=\n"
				"movq		mm0,[%[src]]\n"
				"movq		mm1,[%[src]+8]\n"
				"movq		[%[dest]], mm0\n"
				"movq		[%[dest]+8], mm1\n"

			"memcpy_qwc_final_%=:\n"
				"emms\n"								// clean up the MMX state
			".att_syntax\n"
					: "=&r"(dest), "=&r"(src), "=&r"(qwc)
					: [dest]"0"(dest), [src]"1"(src), [qwc]"2"(qwc)
					: "memory", "eax", "mm0", "mm1", "mm2", "mm3"
		);
	}
#endif

