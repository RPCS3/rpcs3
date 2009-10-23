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

#pragma once

#include "Utilities/SafeArray.h"
#include "Utilities/Threading.h"		// to use threading stuff, include the Threading namespace in your file.
#include "CDVD/CDVDaccess.h"

static const int PCSX2_VersionHi	= 0;
static const int PCSX2_VersionMid	= 9;
static const int PCSX2_VersionLo	= 7;

class SysCoreThread;

// --------------------------------------------------------------------------------------
//  SysCoreAllocations class
// --------------------------------------------------------------------------------------
class SysCoreAllocations
{
public:
	// This set of booleans defaults to false and are only set TRUE if the corresponding
	// recompilers succeeded to initialize/allocate.  The host application should honor
	// these booleans when selecting between recompiler or interpreter, since recompilers
	// will fail to operate if these are "false."

	bool	RecSuccess_EE:1,
			RecSuccess_IOP:1,
			RecSuccess_VU0:1,
			RecSuccess_VU1:1;

protected:

public:
	SysCoreAllocations();
	virtual ~SysCoreAllocations() throw();

	bool HadSomeFailures( const Pcsx2Config::RecompilerOptions& recOpts ) const;

protected:
	void CleanupMess() throw();
};


extern void SysDetect();				// Detects cpu type and fills cpuInfo structs.
extern void SysClearExecutionCache();	// clears recompiled execution caches!


extern u8 *SysMmapEx(uptr base, u32 size, uptr bounds, const char *caller="Unnamed");
extern void vSyncDebugStuff( uint frame );
extern void NTFS_CompressFile( const wxString& file, bool compressStatus=true );

// --------------------------------------------------------------------------------------
//  PCSX2_SEH - Defines existence of "built in" Structured Exception Handling support.
// --------------------------------------------------------------------------------------
// This should be available on Windows, via Microsoft or Intel compilers (I'm pretty sure Intel
// supports native SEH model).  GNUC in Windows, or any compiler in a non-windows platform, will
// need to use setjmp/longjmp instead to exit recompiled code.
//

//#define PCSX2_SEH		0		// use this to force disable SEH on win32, to test setjmp functionality.

#ifndef PCSX2_SEH
#	if defined(_WIN32) && !defined(__GNUC__)
#		define PCSX2_SEH	1
#	else
#		define PCSX2_SEH	0
#	endif
#endif

// special macro which disables inlining on functions that require their own function stackframe.
// This is due to how Win32 handles structured exception handling.  Linux uses signals instead
// of SEH, and so these functions can be inlined.
#ifdef _WIN32
#	define __unique_stackframe __noinline
#else
#	define __unique_stackframe
#endif


//////////////////////////////////////////////////////////////////////////////////////////
// Different types of message boxes that the emulator can employ from the friendly confines
// of it's blissful unawareness of whatever GUI it runs under. :)  All message boxes exhibit
// blocking behavior -- they prompt the user for action and only return after the user has
// responded to the prompt.
//

class pxMessageBoxEvent;

namespace Msgbox
{
	extern void OnEvent( pxMessageBoxEvent& evt );

	extern bool Alert( const wxString& text, const wxString& caption=L"PCSX2 Message", int icon=wxICON_EXCLAMATION );
	extern bool OkCancel( const wxString& text, const wxString& caption=L"PCSX2 Message", int icon=0 );
	extern bool YesNo( const wxString& text, const wxString& caption=L"PCSX2 Message", int icon=wxICON_QUESTION );

	extern int  Assertion( const wxString& text, const wxString& stacktrace );
	extern void Except( const Exception::BaseException& src );
}

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( pxEVT_MSGBOX, -1 )
	DECLARE_EVENT_TYPE( pxEVT_CallStackBox, -1 )
END_DECLARE_EVENT_TYPES()
