#pragma once

#ifndef _PCSX2_PRECOMPILED_HEADER_
#define _PCSX2_PRECOMPILED_HEADER_
#endif // pragma once

//////////////////////////////////////////////////////////////////////////////////////////
// Microsoft specific STL extensions for bounds checking and stuff: Enabled in devbuilds,
// disabled in release builds. :)

#ifdef _MSC_VER
#ifdef PCSX2_DEVBUILD
#	define _SECURE_SCL 1
#	define _SECURE_SCL_THROWS 1
#else
#	define _SECURE_SCL 0
#endif
#endif

#define NOMINMAX		// Disables other libs inclusion of their own min/max macros (we use std instead)

#if defined (__linux__)  // some distributions are lower case
#	define __LINUX__
#endif

#ifdef _WIN32
// disable warning C4244: '=' : conversion from 'big' to 'small', possible loss of data
#	pragma warning(disable:4244)
#else
#	include <unistd.h>		// Non-Windows platforms need this
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Welcome wxWidgets to the party!

#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/gdicmn.h>		// for wxPoint/wxRect stuff

extern const wxRect wxDefaultRect;	// wxWidgets lacks one of its own.

//////////////////////////////////////////////////////////////////////////////////////////
// Include the STL junk that's actually handy.

#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>		// string.h under c++
#include <cstdio>		// stdio.h under c++
#include <cstdlib>
#include <iostream>

// ... and include some ANSI/POSIX C libs that are useful too, just for good measure.
// (these compile lightning fast with or without PCH, but they never change so
// might as well add them here)

#include <stddef.h>
#include <malloc.h>
#include <assert.h>
#include <sys/stat.h>
#include <pthread.h>

using std::string;		// we use it enough, so bring it into the global namespace.
using std::min;
using std::max;

typedef int BOOL;

#	undef TRUE
#	undef FALSE
#	define TRUE  1
#	define FALSE 0

//////////////////////////////////////////////////////////////////////////////////////////
// Begin Pcsx2 Includes: Add items here that are local to Pcsx2 but stay relatively
// unchanged for long periods of time, or happen to be used by almost everything, so they
// need a full recompile anyway, when modified (etc)

#include "zlib/zlib.h"
#include "PS2Etypes.h"
#include "StringUtils.h"
#include "Exceptions.h"
#include "MemcpyFast.h"


////////////////////////////////////////////////////////////////////
// Compiler/OS specific macros and defines -- Begin Section

#if defined(_MSC_VER)

#	define strnicmp _strnicmp
#	define stricmp _stricmp

#else	// must be GCC...

#	include <sys/types.h>
#	include <sys/timeb.h>

// Definitions added Feb 16, 2006 by efp
#	ifndef __declspec
#		define __declspec(x)
#	endif

static __forceinline u32 timeGetTime()
{
	struct timeb t;
	ftime(&t);
	return (u32)(t.time*1000+t.millitm);
}

#	ifndef strnicmp
#		define strnicmp strncasecmp
#	endif

#	ifndef stricmp
#		define stricmp strcasecmp
#	endif

#endif		// end GCC/Linux stuff

// compile-time assert
#ifndef C_ASSERT
#	define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#endif

#ifndef __LINUX__
#	define __unused
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Forceinline macro that is enabled for RELEASE/PUBLIC builds ONLY.  (non-inline in devel)
// This is useful because forceinline can make certain types of debugging problematic since
// functions that look like they should be called won't breakpoint since their code is inlined.
// Henceforth, use release_inline for things which we want inlined on public/release builds but
// *not* in devel builds.

#ifdef PCSX2_DEVBUILD
#	define __releaseinline
#else
#	define __releaseinline __forceinline
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Emitter Instance Identifiers.  If you add a new emitter, do it here also.
// Note: Currently most of the instances map back to 0, since existing dynarec code all
// shares iCore and must therefore all share the same emitter instance.
// (note: these don't really belong here per-se, but it's an easy spot to use for now)
enum
{
	EmitterId_R5900 = 0,
	EmitterId_R3000a = EmitterId_R5900,
	EmitterId_VU0micro = EmitterId_R5900,
	EmitterId_VU1micro = EmitterId_R5900,
	
	// Cotton's new microVU, which is iCore-free
	EmitterId_microVU0,
	EmitterId_microVU1,

	// Air's eventual IopRec, which will also be iCore-free
	EmitterId_R3000air,
		
	EmitterId_Count			// must always be last!
};
