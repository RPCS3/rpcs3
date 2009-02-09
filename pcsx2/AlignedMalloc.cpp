/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

// This module contains implementations of _aligned_malloc for platforms that don't have
// it built into their CRT/libc.

#include "PrecompiledHeader.h"
#include "System.h"


struct AlignedMallocHeader
{
	u32 size;		// size of the allocated buffer (minus alignment and header)
	void* baseptr;	// offset of the original allocated pointer
};

static const uint headsize = sizeof(AlignedMallocHeader);

void* __fastcall pcsx2_aligned_malloc(size_t size, size_t align)
{
	jASSUME( align < 0x10000 );

	u8* p = (u8*)malloc(size+align+headsize);

	// start alignment calculations from past the header.
	uptr pasthead = (uptr)(p+headsize);
	uptr aligned = (pasthead + align-1) & ~(align-1);

	AlignedMallocHeader* header = (AlignedMallocHeader*)(aligned-headsize);
	jASSUME( (uptr)header >= (uptr)p );

	header->baseptr = p;
	header->size = size;

	return (void*)aligned;
}

void* __fastcall pcsx2_aligned_realloc(void* handle, size_t size, size_t align)
{
	if( handle == NULL ) return NULL;
	jASSUME( align < 0x10000 );

	AlignedMallocHeader* header = (AlignedMallocHeader*)((uptr)handle - headsize);

	void* newbuf = pcsx2_aligned_malloc( size, align );
	memcpy_fast( newbuf, handle, std::min( size, header->size ) );

	free( header->baseptr );
	return newbuf;
}

__forceinline void pcsx2_aligned_free(void* pmem)
{
	if( pmem == NULL ) return;
	AlignedMallocHeader* header = (AlignedMallocHeader*)((uptr)pmem - headsize);
	free( header->baseptr );
}
