#ifndef PCSX2_PRECOMPILED_HEADER
#define PCSX2_PRECOMPILED_HEADER

//#pragma once		// no dice, causes problems in GCC PCH (which doesn't really work very well

// Disable some pointless warnings...
#ifdef _MSC_VER
#	pragma warning(disable:4250) //'class' inherits 'method' via dominance
#	pragma warning(disable:4996) //ignore the stricmp deprecated warning
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Define PCSX2's own i18n helpers.  These override the wxWidgets helpers and provide
// additional functionality.
//
#define WXINTL_NO_GETTEXT_MACRO
#undef _
#define _(s)		pxGetTranslation(_T(s))

// macro provided for tagging translation strings, without actually running them through the
// translator (which the _() does automatically, and sometimes we don't want that).  This is
// a shorthand replacement for wxTRANSLATE.
#define wxLt(a)		(a)

#define NOMINMAX		// Disables other libs inclusion of their own min/max macros (we use std instead)

//////////////////////////////////////////////////////////////////////////////////////////
// Welcome wxWidgets to the party!

#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/gdicmn.h>		// for wxPoint/wxRect stuff
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/filename.h>

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

#undef TRUE
#undef FALSE
#define TRUE  1
#define FALSE 0

#ifndef wxASSERT_MSG_A
#	define wxASSERT_MSG_A( cond, msg ) wxASSERT_MSG( cond, wxString::FromAscii(msg).c_str() );
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Begin Pcsx2 Includes: Add items here that are local to Pcsx2 but stay relatively
// unchanged for long periods of time, or happen to be used by almost everything, so they
// need a full recompile anyway, when modified (etc)

#include "zlib/zlib.h"
#include "Pcsx2Defs.h"
#include "i18n.h"
#include "Config.h"
#include "Utilities/wxBaseTools.h"
#include "Utilities/ScopedPtr.h"
#include "Utilities/Path.h"
#include "Utilities/Console.h"
#include "Utilities/Exceptions.h"
#include "Utilities/MemcpyFast.h"
#include "Utilities/General.h"
#include "x86emitter/tools.h"

// Linux isn't set up for svn version numbers yet.
#ifdef __LINUX__
#	define SVN_REV 0
#	define SVN_MODS 0
#endif

//////////////////////////////////////////////////////////////////////////////////////////
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

#	ifndef strnicmp
#		define strnicmp strncasecmp
#	endif

#	ifndef stricmp
#		define stricmp strcasecmp
#	endif

#endif		// end GCC/Linux stuff

#endif
