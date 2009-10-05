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

#ifndef _LNX_MEMZERO_H_
#define _LNX_MEMZERO_H_

// This header contains non-optimized implementation of memzero_ptr and memset8,
// memset16, etc.

template< u32 data, typename T >
static __forceinline void memset32( T& obj )
{
	// this function works on 32-bit aligned lengths of data only.
	// If the data length is not a factor of 32 bits, the C++ optimizing compiler will
	// probably just generate mysteriously broken code in Release builds. ;)

	jASSUME( (sizeof(T) & 0x3) == 0 );

	u32* dest = (u32*)&obj;
	for( int i=sizeof(T)>>2; i; --i, ++dest )
		*dest = data;
}

template< uint size >
static __forceinline void memzero_ptr( void* dest )
{
	memset( dest, 0, size );
}

template< typename T >
static __forceinline void memzero( T& obj )
{
	memset( &obj, 0, sizeof( T ) );
}

template< u8 data, typename T >
static __forceinline void memset8( T& obj )
{
	// Aligned sizes use the optimized 32 bit inline memset.  Unaligned sizes use memset.
	if( (sizeof(T) & 0x3) != 0 )
		memset( &obj, data, sizeof( T ) );
	else
		memset32<data + (data<<8) + (data<<16) + (data<<24)>( obj );
}

template< u16 data, typename T >
static __forceinline void memset16( T& obj )
{
	if( (sizeof(T) & 0x3) != 0 )
		_memset16_unaligned( &obj, data, sizeof( T ) );
	else
		memset32<data + (data<<16)>( obj );
}


// An optimized memset for 8 bit destination data.
template< u8 data, size_t bytes >
static __forceinline void memset_8( void *dest )
{
	if( bytes == 0 ) return;

	if( (bytes & 0x3) != 0 )
	{
		// unaligned data length.  No point in doing an optimized inline version (too complicated!)
		// So fall back on the compiler implementation:

		memset( dest, data, bytes );
		return;
	}

	// This function only works on 32-bit alignments of data copied.
	jASSUME( (bytes & 0x3) == 0 );

	enum
	{
		remdat = bytes>>2,
		data32 = data + (data<<8) + (data<<16) + (data<<24)
	};

	// macro to execute the x86/32 "stosd" copies.
	switch( remdat )
	{
		case 1:
			*(u32*)dest = data32;
		return;

		case 2:
			((u32*)dest)[0] = data32;
			((u32*)dest)[1] = data32;
		return;

		case 3:
			__asm__ volatile
			(
				".intel_syntax noprefix\n"
				"cld\n"
//				"mov edi, %[dest]\n"
//				"mov eax, %[data32]\n"
				"stosd\n"
				"stosd\n"
				"stosd\n"
				".att_syntax\n"
				:
				// Input specifiers: D - edi, a -- eax, c ecx
				: [dest]"D"(dest), [data32]"a"(data32)
				: "memory"
			);
		return;

		case 4:
			__asm__ volatile
			(
				".intel_syntax noprefix\n"
				"cld\n"
//				"mov edi, %[dest]\n"
//				"mov eax, %[data32]\n"
				"stosd\n"
				"stosd\n"
				"stosd\n"
				"stosd\n"
				".att_syntax\n"
				:
				:  [dest]"D"(dest), [data32]"a"(data32)
				:  "memory"

			);
		return;

		case 5:
			__asm__ volatile
			(
				".intel_syntax noprefix\n"
				"cld\n"
//				"mov edi, %[dest]\n"
//				"mov eax, %[data32]\n"
				"stosd\n"
				"stosd\n"
				"stosd\n"
				"stosd\n"
				"stosd\n"
				".att_syntax\n"
				:
				: [dest]"D"(dest), [data32]"a"(data32)
				: "memory"

			);
		return;

		default:
			__asm__ volatile
			(
				".intel_syntax noprefix\n"
				"cld\n"
//				"mov ecx, %[remdat]\n"
//				"mov edi, %[dest]\n"
//				"mov eax, %\[data32]n"
				"rep stosd\n"
				".att_syntax\n"
				:
				: [remdat]"c"(remdat), [dest]"D"(dest), [data32]"a"(data32)
				: "memory"
			);
		return;
	}
}

#endif
