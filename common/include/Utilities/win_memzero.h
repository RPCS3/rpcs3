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

#ifdef _MSC_VER
#	pragma warning(disable:4063)	// case '1' is not a valid value for switch()
#endif

// These functions are meant for memset operations of constant length only.
// For dynamic length clears, use the C-compiler provided memset instead.

// MemZero Code Strategies:
//  I use a trick to help the MSVC compiler optimize it's asm code better.  The compiler
//  won't optimize local variables very well because it insists in storing them on the
//  stack and then loading them out of the stack when I use them from inline ASM, and
//  it won't allow me to use template parameters in inline asm code either.  But I can
//  assign the template parameters to enums, and then use the enums from asm code.
// Yeah, silly, but it works. :D  (air)

//  All methods defined in this header use template in combination with the aforementioned
//  enumerations to generate very efficient and compact inlined code.  These optimized
//  memsets work on the theory that most uses of memset involve static arrays and
//  structures, which are constant in size, thus allowing us to generate optimal compile-
//  time code for each use of the function.

// Use of CLD (Clear Direction Flag):
//  On Windows platforms the ABI declares that the direction flag should be cleared upon
//  entry of *any* function.  Therefore there is no need to have CLD prior to our use of
//  rep strosd here.

// Notes on XMM0's "storage" area (_xmm_backup):
// Unfortunately there's no way to guarantee alignment for this variable.  If I use the
// __declspec(aligned(16)) decorator, MSVC fails to inline the function since stack
// alignment requires prep work.  And for the same reason it's not possible to check the
// alignment of the stack at compile time, so I'm forced to use movups to store and
// retrieve xmm0.

// MSVC Template Issue:
//  MSVC treats int template parameters like macro insertions.  That is, if you have a
//  a template parameter in the form of "func<10-5>()", MSVC inserts 10-5 into the
//  templated function, causing order-of-operation problems (sigh).  The normal fix would
//  be to assign the template parameter to a static const int inside each function, but that
//  won't fly with the enums optimization.  So in order to fix the problem I define a macro
//  that encapsulates the template parameter inside parenthesis for us:

#define MZFbytes (_bytes)

// This is an implementation of the memzero_ptr fast memset routine (for zero-clears only).
template< size_t _bytes >
static __fi void memzero_ptr( void *dest )
{
	if( MZFbytes == 0 ) return;

	// This function only works on 32-bit alignments.  For anything else we just fall back
	// on the compiler-provided implementation of memset...

	if( (MZFbytes & 0x3) != 0 )
	{
		memset( dest, 0, MZFbytes );
		return;
	}

#if 0
	// SSE-based memory clear.  Currently disabled so to avoid unnecessary dependence on
	// SSE cpu instruction sets.  (memzero typically isn't used in any performance critical
	// situations anyway)
	enum
	{
		remainder = MZFbytes & 127,
		bytes128 = MZFbytes / 128
	};

	// Initial check -- if the length is not a multiple of 16 then fall back on
	// using rep movsd methods.  Handling these unaligned clears in a more efficient
	// manner isn't necessary in pcsx2 (meaning they aren't used in speed-critical
	// scenarios).

	if( (MZFbytes & 0xf) == 0 )
	{
		if( ((uptr)dest & 0xf) != 0 )
		{
			// UNALIGNED COPY MODE.
			// For unaligned copies we have a threshold of at least 128 vectors.  Anything
			// less and it's probably better off just falling back on the rep movsd.
			if( bytes128 > 128 )
			{
				__asm
				{
					mov ecx,dest
					pxor xmm0,xmm0
					mov eax,bytes128

				_loop_6:
					movups [ecx],xmm0
					movups [ecx+0x10],xmm0
					movups [ecx+0x20],xmm0
					movups [ecx+0x30],xmm0
					movups [ecx+0x40],xmm0
					movups [ecx+0x50],xmm0
					movups [ecx+0x60],xmm0
					movups [ecx+0x70],xmm0
					sub ecx,-128
					sub eax,1
					jnz _loop_6;
				}
				if( remainder != 0 )
				{
					// Copy the remainder in reverse (using the decrementing eax as our indexer)
					__asm
					{
						mov eax, remainder

					_loop_5:
						movups [ecx+eax],xmm0;
						sub eax,16;
						jnz _loop_5;
					}
				}
				return;
			}
		}
		else if( bytes128 > 48 )
		{
			// ALIGNED COPY MODE
			// Data is aligned and the size of data is large enough to merit a nice
			// fancy chunk of unrolled goodness:

			__asm
			{
				mov ecx,dest
				pxor xmm0,xmm0
				mov eax,bytes128

			_loop_8:
				movaps [ecx],xmm0
				movaps [ecx+0x10],xmm0
				movaps [ecx+0x20],xmm0
				movaps [ecx+0x30],xmm0
				movaps [ecx+0x40],xmm0
				movaps [ecx+0x50],xmm0
				movaps [ecx+0x60],xmm0
				movaps [ecx+0x70],xmm0
				sub ecx,-128
				sub eax,1
				jnz _loop_8;
			}
			if( remainder != 0 )
			{
				// Copy the remainder in reverse (using the decrementing eax as our indexer)
				__asm
				{
					mov eax, remainder

				_loop_10:
					movaps [ecx+eax],xmm0
					sub eax,16;
					jnz _loop_10;
				}
			}
			return;
		}
	}
	#endif

	// This function only works on 32-bit alignments.
	pxAssume( (MZFbytes & 0x3) == 0 );
	pxAssume( ((uptr)dest & 0x3) == 0 );

	enum
	{
		remdat = MZFbytes >> 2
	};

	// This case statement handles 5 special-case sizes (small blocks)
	// in addition to the generic large block that uses rep stosd.

	switch( remdat )
	{
		case 1:
			*(u32*)dest = 0;
		return;

		case 2:
			*(u64*)dest = 0;
		return;

		case 3:
			__asm
			{
				mov edi, dest
				xor eax, eax
				stosd
				stosd
				stosd
			}
		return;

		case 4:
			__asm
			{
				mov edi, dest
				xor eax, eax
				stosd
				stosd
				stosd
				stosd
			}
		return;

		case 5:
			__asm
			{
				mov edi, dest
				xor eax, eax
				stosd
				stosd
				stosd
				stosd
				stosd
			}
		return;

		default:
			__asm
			{
				mov ecx, remdat
				mov edi, dest
				xor eax, eax
				rep stosd
			}
		return;
	}
}

// An optimized memset for 8 bit destination data.
template< u8 data, size_t _bytes >
static __fi void memset_8( void *dest )
{
	if( MZFbytes == 0 ) return;

	if( (MZFbytes & 0x3) != 0 )
	{
		// unaligned data length.  No point in doing an optimized inline version (too complicated!)
		// So fall back on the compiler implementation:

		memset( dest, data, MZFbytes );
		return;
	}

	/*static const size_t remainder = MZFbytes & 127;
	static const size_t bytes128 = MZFbytes / 128;
	if( bytes128 > 32 )
	{
		// This function only works on 128-bit alignments.
		pxAssume( (MZFbytes & 0xf) == 0 );
		pxAssume( ((uptr)dest & 0xf) == 0 );

		__asm
		{
			mov eax,bytes128
			mov ecx,dest
			movss xmm0,data

			align 16

		_loop_8:
			movaps [ecx],xmm0;
			movaps [ecx+0x10],xmm0;
			movaps [ecx+0x20],xmm0;
			movaps [ecx+0x30],xmm0;
			movaps [ecx+0x40],xmm0;
			movaps [ecx+0x50],xmm0;
			movaps [ecx+0x60],xmm0;
			movaps [ecx+0x70],xmm0;
			sub ecx,-128
			dec eax;
			jnz _loop_8;
		}
		if( remainder != 0 )
		{
			// Copy the remainder in reverse (using the decrementing eax as our indexer)
			__asm
			{
				mov eax, remainder

			_loop_10:
				movaps [ecx+eax],xmm0;
				sub eax,16;
				jnz _loop_10;
			}
		}
	}*/

	// This function only works on 32-bit alignments of data copied.
	pxAssume( (MZFbytes & 0x3) == 0 );

	enum
	{
		remdat = MZFbytes >> 2,
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
			__asm
			{
				mov edi, dest;
				mov eax, data32;
				stosd;
				stosd;
				stosd;
			}
		return;

		case 4:
			__asm
			{
				mov edi, dest;
				mov eax, data32;
				stosd;
				stosd;
				stosd;
				stosd;
			}
		return;

		case 5:
			__asm
			{
				mov edi, dest;
				mov eax, data32;
				stosd;
				stosd;
				stosd;
				stosd;
				stosd;
			}
		return;

		default:
			__asm
			{
				mov ecx, remdat;
				mov edi, dest;
				mov eax, data32;
				rep stosd;
			}
		return;
	}
}

template< u16 data, size_t _bytes >
static __fi void memset_16( void *dest )
{
	if( MZFbytes == 0 ) return;

	// Assertion: data length must be a multiple of 16 or 32 bits
	pxAssume( (MZFbytes & 0x1) == 0 );

	if( (MZFbytes & 0x3) != 0 )
	{
		// Unaligned data length.  No point in doing an optimized inline version (too complicated with
		// remainders and such).

		_memset16_unaligned( dest, data, MZFbytes );
		return;
	}

	//u64 _xmm_backup[2];

	// This function only works on 32-bit alignments of data copied.
	pxAssume( (MZFbytes & 0x3) == 0 );

	enum
	{
		remdat = MZFbytes >> 2,
		data32 = data + (data<<16)
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
			__asm
			{
				mov edi, dest;
				mov eax, data32;
				stosd;
				stosd;
				stosd;
			}
		return;

		case 4:
			__asm
			{
				mov edi, dest;
				mov eax, data32;
				stosd;
				stosd;
				stosd;
				stosd;
			}
		return;

		case 5:
			__asm
			{
				mov edi, dest;
				mov eax, data32;
				stosd;
				stosd;
				stosd;
				stosd;
				stosd;
			}
		return;

		default:
			__asm
			{
				mov ecx, remdat;
				mov edi, dest;
				mov eax, data32;
				rep stosd;
			}
		return
	}
}

template< u32 data, size_t MZFbytes >
static __fi void memset_32( void *dest )
{
	if( MZFbytes == 0 ) return;

	// Assertion: data length must be a multiple of 32 bits
	pxAssume( (MZFbytes & 0x3) == 0 );

	//u64 _xmm_backup[2];

	// This function only works on 32-bit alignments of data copied.
	// If the data length is not a factor of 32 bits, the C++ optimizing compiler will
	// probably just generate mysteriously broken code in Release builds. ;)

	pxAssume( (MZFbytes & 0x3) == 0 );

	enum
	{
		remdat = MZFbytes>>2,
		data32 = data
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
			__asm
			{
				mov edi, dest;
				mov eax, data32;
				stosd;
				stosd;
				stosd;
			}
		return;

		case 4:
			__asm
			{
				mov edi, dest;
				mov eax, data32;
				stosd;
				stosd;
				stosd;
				stosd;
			}
		return;

		case 5:
			__asm
			{
				mov edi, dest;
				mov eax, data32;
				stosd;
				stosd;
				stosd;
				stosd;
				stosd;
			}
		return;

		default:
			__asm
			{
				mov ecx, remdat;
				mov edi, dest;
				mov eax, data32;
				rep stosd;
			}
		return
	}
}

// This method can clear any object-like entity -- which is anything that is not a pointer.
// Structures, static arrays, etc.  No need to include sizeof() crap, this does it automatically
// for you!
template< typename T >
static __fi void memzero( T& object )
{
	memzero_ptr<sizeof(T)>( &object );
}

// This method clears an object with the given 8 bit value.
template< u8 data, typename T >
static __fi void memset8( T& object )
{
	memset_8<data, sizeof(T)>( &object );
}

// This method clears an object with the given 16 bit value.
template< u16 data, typename T >
static __fi void memset16( T& object )
{
	memset_16<data, sizeof(T)>( &object );
}

// This method clears an object with the given 32 bit value.
template< u32 data, typename T >
static __fi void memset32( T& object )
{
	memset_32<data, sizeof(T)>( &object );
}

#undef MZFbytes
