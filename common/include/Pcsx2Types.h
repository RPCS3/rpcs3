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

#ifndef __PCSX2TYPES_H__
#define __PCSX2TYPES_H__

// --------------------------------------------------------------------------------------
//  Forward declarations
// --------------------------------------------------------------------------------------
// Forward declarations for wxWidgets-supporting features.
// If you aren't linking against wxWidgets libraries, then functions that
// depend on these types will not be usable (they will yield linker errors).

#ifdef __cplusplus
	class wxString;
	class FastFormatAscii;
	class FastFormatUnicode;
#endif


// --------------------------------------------------------------------------------------
//  Basic Atomic Types
// --------------------------------------------------------------------------------------

#if defined(_MSC_VER)

typedef __int8  s8;
typedef __int16 s16;
typedef __int32 s32;
typedef __int64 s64;

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

typedef unsigned int uint;

#else // _MSC_VER*/

#ifdef __LINUX__

#ifdef HAVE_STDINT_H
#include "stdint.h"

// note: char and int8_t are not interchangable types on gcc, because int8_t apparently
// maps to 'signed char' which (due to 1's compliment or something) is its own unique
// type.  This creates cross-compiler inconsistencies, in addition to being entirely
// unexpected behavior to any sane programmer, so we typecast s8 to char instead. :)

//typedef int8_t s8;
typedef char s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t uptr;
typedef intptr_t sptr;

#else // HAVE_STDINT_H

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#endif // HAVE_STDINT_H

typedef unsigned int uint;

#define LONG long
typedef union _LARGE_INTEGER
{
	long long QuadPart;
} LARGE_INTEGER;

#endif // __LINUX__
#endif //_MSC_VER

#if !defined(__LINUX__) || !defined(HAVE_STDINT_H)
#if defined(__x86_64__)
typedef u64 uptr;
typedef s64 sptr;
#else
typedef u32 uptr;
typedef s32 sptr;
#endif
#endif

// --------------------------------------------------------------------------------------
//  u128 / s128 - A rough-and-ready cross platform 128-bit datatype, Non-SSE style.
// --------------------------------------------------------------------------------------
// Note: These structs don't provide any additional constructors because C++ doesn't allow
// the use of datatypes with constructors in unions (and since unions aren't the primary
// uses of these types, that means we can't have constructors). Embedded functions for
// performing explicit conversion from 64 and 32 bit values are provided instead.
//
#ifdef __cplusplus
union u128
{
	struct  
	{
		u64 lo;
		u64 hi;
	};

	u64 _u64[2];
	u32 _u32[4];
	u16 _u16[8];
	u8  _u8[16];

	// Explicit conversion from u64. Zero-extends the source through 128 bits.
	static u128 From64( u64 src )
	{
		u128 retval;
		retval.lo = src;
		retval.hi = 0;
		return retval;
	}

	// Explicit conversion from u32. Zero-extends the source through 128 bits.
	static u128 From32( u32 src )
	{
		u128 retval;
		retval._u32[0] = src;
		retval._u32[1] = 0;
		retval.hi = 0;
		return retval;
	}

	operator u32() const { return _u32[0]; }
	operator u16() const { return _u16[0]; }
	operator u8() const { return _u8[0]; }
	
	bool operator==( const u128& right ) const
	{
		return (lo == right.lo) && (hi == right.hi);
	}

	bool operator!=( const u128& right ) const
	{
		return (lo != right.lo) || (hi != right.hi);
	}

	// In order for the following ToString() and WriteTo methods to be available, you must
	// be linking to both wxWidgets and the pxWidgets extension library.  If you are not
	// using them, then you will need to provide your own implementations of these methods.
	wxString ToString() const;
	wxString ToString64() const;
	wxString ToString8() const;
	
	void WriteTo( FastFormatAscii& dest ) const;
	void WriteTo8( FastFormatAscii& dest ) const;
	void WriteTo64( FastFormatAscii& dest ) const;
};

struct s128
{
	s64 lo;
	s64 hi;

	// explicit conversion from s64, with sign extension.
	static s128 From64( s64 src )
	{
		s128 retval = { src, (src < 0) ? -1 : 0 };
		return retval;
	}

	// explicit conversion from s32, with sign extension.
	static s128 From64( s32 src )
	{
		s128 retval = { src, (src < 0) ? -1 : 0 };
		return retval;
	}

	operator u32() const { return (s32)lo; }
	operator u16() const { return (s16)lo; }
	operator u8() const { return (s8)lo; }

	bool operator==( const s128& right ) const
	{
		return (lo == right.lo) && (hi == right.hi);
	}

	bool operator!=( const s128& right ) const
	{
		return (lo != right.lo) || (hi != right.hi);
	}
};

#else

typedef union _u128_t
{
	struct  
	{
		u64 lo;
		u64 hi;
	};

	u64 _u64[2];
	u32 _u32[4];
	u16 _u16[8];
	u8  _u8[16];
} u128;

typedef union _s128_t
{
	u64 lo;
	s64 hi;
} s128;

#endif

#endif
