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
 
#ifndef __PCSX2DEFS_H__
#define __PCSX2DEFS_H__

#if defined (__linux__) && !defined(__LINUX__)  // some distributions are lower case
#define __LINUX__
#endif

#ifdef __CYGWIN__
#define __LINUX__
#endif

#include "Pcsx2Types.h"

// Renamed ARRAYSIZE to ArraySize -- looks nice and gets rid of Windows.h conflicts (air)
// Notes: I'd have used ARRAY_SIZE instead but ran into cross-platform lib conflicts with
// that as well.  >_<
#ifndef ArraySize
#define ArraySize(x) (sizeof(x)/sizeof((x)[0]))
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// jASSUME - give hints to the optimizer
//  This is primarily useful for the default case switch optimizer, which enables VC to
//  generate more compact switches.

#ifndef jASSUME
#	ifdef NDEBUG
#		define jBREAKPOINT() ((void) 0)
#		ifdef _MSC_VER
#			define jASSUME(exp) (__assume(exp))
#		else
#			define jASSUME(exp) ((void) sizeof(exp))
#		endif
#	else
#		if defined(_MSC_VER)
#			define jBREAKPOINT() do { __asm int 3 } while(0)
#		else
#			define jBREAKPOINT() ((void) *(volatile char *) 0)
#		endif
#		define jASSUME(exp) if(exp) ; else jBREAKPOINT()
#	endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// jNO_DEFAULT -- disables the default case in a switch, which improves switch optimization
// under MSVC.
//
// How it Works: jASSUME turns into an __assume(0) under msvc compilers, which when specified
// in the default: case of a switch tells the compiler that the case is unreachable, and so
// the compiler will not generate any code, LUTs, or conditionals to handle it.  In debug
// builds the default case will cause an assertion (meaning the jNO_DEFAULT has been used
// incorrectly, and that the default case is in fact used and needs to be handled).
//
#ifndef jNO_DEFAULT
#define jNO_DEFAULT \
{ \
default: \
	jASSUME(0); \
	break; \
}
#endif

/* common defines */
#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#endif

#ifdef _MSC_VER
// Note: building the 'extern' into PCSX2_ALIGNED16_DECL fixes Visual Assist X's intellisense.

#define PCSX2_ALIGNED(alig,x) __declspec(align(alig)) x
#define PCSX2_ALIGNED_EXTERN(alig,x) extern __declspec(align(alig)) x
#define PCSX2_ALIGNED16(x) __declspec(align(16)) x
#define PCSX2_ALIGNED16_EXTERN(x) extern __declspec(align(16)) x

#define __naked __declspec(naked)
#define __unused /*unused*/
#define __noinline __declspec(noinline) 
#define CALLBACK    __stdcall

#else

// fixme - is this needed for recent versions of GCC?  Or can we just use the first two macros
// instead for both definitions (implementations) and declarations (includes)? -- air
#define PCSX2_ALIGNED(alig,x) x __attribute((aligned(alig)))
#define PCSX2_ALIGNED16(x) x __attribute((aligned(16)))
#define PCSX2_ALIGNED_EXTERN(alig,x) extern x __attribute((aligned(alig)))
#define PCSX2_ALIGNED16_EXTERN(x) extern x __attribute((aligned(16)))

#define __naked		// GCC lacks the naked specifier
#define CALLBACK	// CALLBACK is a win32-specific mess

// GCC uses attributes for a lot of things that Visual C+ doesn't.
#define __fastcall __attribute__((fastcall))
#define __unused __attribute__((unused))
#define _inline __inline__ __attribute__((unused))
#define __forceinline __attribute__((always_inline,unused))
#define __noinline __attribute__((noinline))
#endif

typedef struct {
	int size;
	s8 *data;
} freezeData;

// event values:
#define KEYPRESS	1
#define KEYRELEASE	2

typedef struct _keyEvent {
	u32 key;
	u32 evt;
} keyEvent;

#endif