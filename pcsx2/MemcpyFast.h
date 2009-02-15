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

#ifndef __MEMCPY_FAST_H__
#define __MEMCPY_FAST_H__

//#include "Misc.h"

void _memset16_unaligned( void* dest, u16 data, size_t size );

#if defined(_WIN32) && !defined(__x86_64__)

	// The new simplified memcpy_amd_ is now faster than memcpy_raz_.
	// memcpy_amd_ also does mmx register saving, negating the need for freezeregs (code cleanup!)
	// Additionally, using one single memcpy implementation keeps the code cache cleaner.

	//extern void __fastcall memcpy_raz_udst(void *dest, const void *src, size_t bytes);
	//extern void __fastcall memcpy_raz_usrc(void *dest, const void *src, size_t bytes);
	//extern void __fastcall memcpy_raz_(void *dest, const void *src, size_t bytes);
	extern void __fastcall memcpy_amd_(void *dest, const void *src, size_t bytes);

#	include "windows/memzero.h"
#	define memcpy_fast memcpy_amd_
#	define memcpy_aligned memcpy_amd_

#else

	// for now linux uses the GCC memcpy/memset implementations.
	#define memcpy_fast memcpy
	#define memcpy_raz_ memcpy
	#define memcpy_raz_u memcpy

	#define memcpy_aligned memcpy
	#define memcpy_raz_u memcpy

	#include "Linux/memzero.h"

#endif


extern u8 memcmp_mmx(const void* src1, const void* src2, int cmpsize);
extern void memxor_mmx(void* dst, const void* src1, int cmpsize);

#endif
