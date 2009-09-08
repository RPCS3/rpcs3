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

#pragma once

#ifdef __LINUX__

#	include "lnx_memzero.h"

	extern "C" void __fastcall memcpy_amd_(void *dest, const void *src, size_t bytes);
	extern "C" u8 memcmp_mmx(const void* src1, const void* src2, int cmpsize);
	extern "C" void memxor_mmx(void* dst, const void* src1, int cmpsize);

#else

#	include "win_memzero.h"

	extern void __fastcall memcpy_amd_(void *dest, const void *src, size_t bytes);
	extern u8 memcmp_mmx(const void* src1, const void* src2, int cmpsize);
	extern void memxor_mmx(void* dst, const void* src1, int cmpsize);

#endif

// Only used in the Windows version of memzero.h. But it's in Misc.cpp for some reason.
void _memset16_unaligned( void* dest, u16 data, size_t size );

#define memcpy_fast memcpy_amd_
#define memcpy_aligned memcpy_amd_
