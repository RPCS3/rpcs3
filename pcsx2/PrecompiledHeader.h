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

#ifndef PCSX2_PRECOMPILED_HEADER
#define PCSX2_PRECOMPILED_HEADER

//#pragma once		// no dice, causes problems in GCC PCH (which doesn't really work very well anyway)

// Disable some pointless warnings...
#ifdef _MSC_VER
#	pragma warning(disable:4250) //'class' inherits 'method' via dominance
#	pragma warning(disable:4996) //ignore the stricmp deprecated warning
#endif

#include "Utilities/Dependencies.h"

#define NOMINMAX		// Disables other libs inclusion of their own min/max macros (we use std instead)

//////////////////////////////////////////////////////////////////////////////////////////
// Welcome wxWidgets to the party!

#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/filename.h>

//////////////////////////////////////////////////////////////////////////////////////////
// Include the STL junk that's actually handy.

#include <stdexcept>
#include <vector>
#include <list>
#include <cstring>		// string.h under c++
#include <cstdio>		// stdio.h under c++
#include <cstdlib>

// ... and include some ANSI/POSIX C libs that are useful too, just for good measure.
// (these compile lightning fast with or without PCH, but they never change so
// might as well add them here)

#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>

using std::min;
using std::max;

typedef int BOOL;

#undef TRUE
#undef FALSE
#define TRUE  1
#define FALSE 0


//////////////////////////////////////////////////////////////////////////////////////////
// Begin Pcsx2 Includes: Add items here that are local to Pcsx2 but stay relatively
// unchanged for long periods of time, or happen to be used by almost everything, so they
// need a full recompile anyway, when modified (etc)

#include "Pcsx2Defs.h"
#include "i18n.h"

#include "Utilities/FixedPointTypes.h"
#include "Utilities/wxBaseTools.h"
#include "Utilities/ScopedPtr.h"
#include "Utilities/Path.h"
#include "Utilities/Console.h"
#include "Utilities/MemcpyFast.h"
#include "Utilities/General.h"
#include "x86emitter/tools.h"

#include "Config.h"

typedef void FnType_Void();
typedef FnType_Void* Fnptr_Void;

// --------------------------------------------------------------------------------------
//  Compiler/OS specific macros and defines 
// --------------------------------------------------------------------------------------

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

#endif		// end GCC/Linux stuff

#endif
