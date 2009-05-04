#pragma once

#ifndef _PCSX2_PRECOMPILED_HEADER_
#define _PCSX2_PRECOMPILED_HEADER_
#endif // pragma once

//////////////////////////////////////////////////////////////////////////////////////////
// Microsoft specific STL extensions for bounds checking and stuff: Enabled in devbuilds,
// disabled in release builds. :)

#ifdef _MSC_VER
#	pragma warning(disable:4244)	// disable warning C4244: '=' : conversion from 'big' to 'small', possible loss of data
#	ifdef PCSX2_DEVBUILD
#		define _SECURE_SCL 1
#		define _SECURE_SCL_THROWS 1
#	else
#		define _SECURE_SCL 0
#	endif
#endif

#define NOMINMAX		// Disables other libs inclusion of their own min/max macros (we use std instead)

#ifndef _WIN32
#	include <unistd.h>		// Non-Windows platforms need this
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Custom version of jNO_DEFAULT macro for devel builds.
// Raises an exception if the default case is reached.  This notifies us that a jNO_DEFAULT
// directive has been used incorrectly.
//
// MSVC Note: To stacktrace LogicError exceptions, add Exception::LogicError to the C++ First-
// Chance Exception list (under Debug->Exceptions menu).
//
#ifdef PCSX2_DEVBUILD
#define jNO_DEFAULT \
{ \
default: \
	throw Exception::LogicError( "Incorrect usage of jNO_DEFAULT detected (default case is not unreachable!)" ); \
	break; \
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Include the STL junk that's actually handy.

#include <stdexcept>
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
// unchanged for long periods of time.

#include "zlib/zlib.h"
#include "Pcsx2Defs.h"
#include "MemcpyFast.h"
#include "StringUtils.h"
#include "Exceptions.h"

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

/////////////////////////////////////////////////////////////////////////
// GNU GetText / NLS 

#ifdef ENABLE_NLS

#ifdef _WIN32
#include "libintlmsc.h"
#else
#include <locale.h>
#include <libintl.h>
#endif

#undef _
#define _(String) dgettext (PACKAGE, String)
#ifdef gettext_noop
#  define N_(String) gettext_noop (String)
#else
#  define N_(String) (String)
#endif

#else

#define _(msgid) msgid
#define N_(msgid) msgid

#endif		// ENABLE_NLS

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
// Dev / Debug conditionals --
//   Consts for using if() statements instead of uglier #ifdef macros.

#ifdef PCSX2_DEVBUILD
static const bool IsDevBuild = true;
#else
static const bool IsDevBuild = false;
#endif

#ifdef _DEBUG
static const bool IsDebugBuild = true;
#else
static const bool IsDebugBuild = false;
#endif
