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
	extern void memcpy_amd_qwc(void *dest, const void *src, size_t bytes);

#else

#	include "win_memzero.h"

	extern void __fastcall memcpy_amd_(void *dest, const void *src, size_t bytes);
	extern void memcpy_amd_qwc(void *dest, const void *src, size_t bytes);
	extern u8 memcmp_mmx(const void* src1, const void* src2, int cmpsize);
	extern void memxor_mmx(void* dst, const void* src1, int cmpsize);

#endif

// Only used in the Windows version of memzero.h. But it's in Misc.cpp for some reason.
void _memset16_unaligned( void* dest, u16 data, size_t size );

// MemcpyVibes.cpp functions
extern void memcpy_vibes(void * dest, const void * src, int size);
extern void gen_memcpy_vibes();

#define memcpy_fast				memcpy_amd_  // Fast memcpy
#define memcpy_aligned(d,s,c)	memcpy_amd_(d,s,c)	// Memcpy with 16-byte Aligned addresses
#define memcpy_const			memcpy_amd_	 // Memcpy with constant size
#define memcpy_constA			memcpy_amd_  // Memcpy with constant size and 16-byte aligned
#define memcpy_qwc_				memcpy_vibes // Memcpy in aligned qwc increments, with 0x400 qwc or less
#define memcpy_qwc(d,s,c)		memcpy_amd_qwc(d,s,c)

// Useful alternative if we think memcpy_amd_qwc is buggy
//#define memcpy_qwc(d,s,c)		memcpy_amd_(d,s,c*16)
