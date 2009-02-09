#ifndef _PCSX2_PRECOMPILED_HEADER_
#define _PCSX2_PRECOMPILED_HEADER_

#if defined (__linux__)  // some distributions are lower case
#	define __LINUX__
#endif

#ifndef _WIN32
#	include <unistd.h>
#else

// For now Windows headers are needed by all modules, so include it here so
// that it compiles nice and fast...

// Force availability of to WinNT APIs (change to 0x600 to enable XP-specific APIs)
#	define WINVER 0x0501
#	define _WIN32_WINNT 0x0501

#	include <windows.h>

// disable Windows versions of min/max -- we'll use the typesafe STL versions instead.
#undef min
#undef max

#endif

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

// TODO : Add items here that are local to Pcsx2 but stay relatively unchanged for
// long periods of time.

#ifdef _WIN32
// disable warning C4244: '=' : conversion from 'big' to 'small', possible loss of data
#pragma warning(disable:4244)
#endif

using std::string;		// we use it enough, so bring it into the global namespace.

#include "zlib.h"
#include "PS2Etypes.h"
#include "StringUtils.h"

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

// functions that linux lacks...
// fixme: this should probably be in a __LINUX__ conditional rather than
// a GCC conditional (since GCC on a windows platform would have these functions)
#	define Sleep(seconds) usleep(1000*(seconds))

static __forceinline u32 timeGetTime()
{
	struct timeb t;
	ftime(&t);
	return (u32)(t.time*1000+t.millitm);
}

#	define BOOL int

#	undef TRUE
#	undef FALSE
#	define TRUE  1
#	define FALSE 0

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

#endif
