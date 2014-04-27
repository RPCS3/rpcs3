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

// This module contains implementations of _aligned_malloc for platforms that don't have
// it built into their CRT/libc.

#include "PrecompiledHeader.h"

struct AlignedMallocHeader
{
	u32 size;		// size of the allocated buffer (minus alignment and header)
	void* baseptr;	// offset of the original allocated pointer
};

static const uint headsize = sizeof(AlignedMallocHeader);

void* __fastcall pcsx2_aligned_malloc(size_t size, size_t align)
{
	pxAssert( align < 0x10000 );

	u8* p = (u8*)malloc(size+align+headsize);

	// start alignment calculations from past the header.
	uptr pasthead = (uptr)(p+headsize);
	uptr aligned = (pasthead + align-1) & ~(align-1);

	AlignedMallocHeader* header = (AlignedMallocHeader*)(aligned-headsize);
	pxAssert( (uptr)header >= (uptr)p );

	header->baseptr = p;
	header->size = size;

	return (void*)aligned;
}

void* __fastcall pcsx2_aligned_realloc(void* handle, size_t size, size_t align)
{
	pxAssert( align < 0x10000 );

	void* newbuf = pcsx2_aligned_malloc( size, align );

	if( handle != NULL )
	{
		AlignedMallocHeader* header = (AlignedMallocHeader*)((uptr)handle - headsize);
		memcpy_fast( newbuf, handle, std::min( size, header->size ) );
		free( header->baseptr );
	}
	return newbuf;
}

__fi void pcsx2_aligned_free(void* pmem)
{
	if( pmem == NULL ) return;
	AlignedMallocHeader* header = (AlignedMallocHeader*)((uptr)pmem - headsize);
	free( header->baseptr );
}

// ----------------------------------------------------------------------------
// And for lack of a better home ...


// Special unaligned memset used when all other optimized memsets fail (it's called from
// memzero_obj and stuff).
__fi void _memset16_unaligned( void* dest, u16 data, size_t size )
{
	pxAssume( (size & 0x1) == 0 );

	u16* dst = (u16*)dest;
	for(int i=size; i; --i, ++dst )
		*dst = data;
}

__fi void HostSys::Munmap( void* base, u32 size )
{
	Munmap( (uptr)base, size );
}
