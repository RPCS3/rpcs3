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
using namespace x86Emitter;

// Max Number of qwc supported
#define _maxSize 0x400

typedef void (__fastcall *_memCpyCall)(void*, void*);
__aligned16 _memCpyCall _memcpy_vibes[_maxSize+1];

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
		xMOVAPS(xmm_t, ptr32[edx+off]);
		xMOVAPS(ptr32[ecx+off], xmm_t);
	}

	_memcpy_vibes[0] = (_memCpyCall)xGetPtr();

	xRET();
	pxAssert(((uptr)xGetPtr() - (uptr)_memCpyExec) < sizeof(_memCpyExec));

	HostSys::MemProtectStatic(_memCpyExec, Protect_ReadOnly, true);
}

void __fastcall memcpy_vibes(void * dest, void * src, int size) {
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

void __fastcall memcpy_vibes(void * dest, void * src, int size) {
	_memcpy_vibes[size](dest, src);
}

#endif