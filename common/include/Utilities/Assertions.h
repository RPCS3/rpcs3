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

// ----------------------------------------------------------------------------------------
//  pxAssert / pxAssertDev / pxFail / pxFailDev
// ----------------------------------------------------------------------------------------
// Standard debug-ony "nothrow" (pxAssert) and devel-style "throw" (pxAssertDev) style
// assertions.  All assertions act as valid conditional statements that return the result
// of the specified conditional; useful for handling failed assertions in a "graceful" fashion
// when utilizing the "ignore" feature of assertion debugging.  (Release builds *always* return
// true for assertion success, but no actual assertion check is performed).
//
// Performance: All assertion types optimize into __assume() directives in Release builds.
//
// pxAssertDev is an assertion tool for Devel builds, intended for sanity checking and/or
// bounds checking variables in areas which are not performance critical.
//
// How it works: pxAssertDev throws an exception of type Exception::LogicError if the assertion
// conditional is false.  Typically for the end-user, this exception is handled by the general
// exception handler defined by the application, which (should eventually) create some state
// dumps and other information for troubleshooting purposes.
//
// From a debugging environment, you can trap your pxAssertDev by either breakpointing the
// exception throw code in pxOnAssert, or by adding Exception::LogicError to your First-Chance
// Exception catch list (Visual Studio, under the Debug->Exceptions menu/dialog).  You should
// have LogicErrors enabled as First-Chance exceptions regardless, so do it now. :)
//
// Credits Notes: These macros are based on a combination of wxASSERT, MSVCRT's assert
// and the ATL's Assertion/Assumption macros.  the best of all worlds!

#if defined(PCSX2_DEBUG)

#	define pxAssertMsg(cond, msg)		( (!!(cond)) || \
		(pxOnAssert(__TFILE__, __LINE__, __WXFUNCTION__, _T(#cond), msg), likely(cond)) )

#	define pxAssertDev(cond,msg)	pxAssertMsg(cond, msg)

#	define pxFail(msg)				pxAssertMsg(false, msg)
#	define pxFailDev(msg)			pxAssertDev(false, msg)

#elif defined(PCSX2_DEVBUILD)

	// Devel builds use __assume for standard assertions and call pxOnAssertDevel
	// for AssertDev brand assertions (which typically throws a LogicError exception).

#	define pxAssertMsg(cond, msg)	(__assume(cond), likely(cond))

#	define pxAssertDev(cond, msg)	( (!!(cond)) || \
		(pxOnAssert(__TFILE__, __LINE__, __WXFUNCTION__, _T(#cond), msg), likely(cond)) )

#	define pxFail(msg)			(__assume(false), false)
#	define pxFailDev(msg	)	pxAssertDev(false, msg)

#else

	// Release Builds just use __assume as an optimization, and return the conditional
	// as a result (which is optimized to nil if unused).

#	define pxAssertMsg(cond, msg)	(__assume(cond), likely(cond))
#	define pxAssertDev(cond, msg)	(__assume(cond), likely(cond))
#	define pxFail(msg)				(__assume(false), false)
#	define pxFailDev(msg)			(__assume(false), false)

#endif

#define pxAssert(cond)				pxAssertMsg(cond, (wxChar*)NULL)

// Performs an unsigned index bounds check, and generates a debug assertion if the check fails.
// For stricter checking in Devel builds as well as debug builds (but possibly slower), use
// IndexBoundsCheckDev.

#define IndexBoundsCheck( objname, idx, sze )		pxAssertMsg( (uint)(idx) < (uint)(sze), \
	wxsFormat( L"Array index out of bounds accessing object '%s' (index=%d, size=%d)", objname, (idx), (sze) ) )

#define IndexBoundsCheckDev( objname, idx, sze )	pxAssertDev( (uint)(idx) < (uint)(sze), \
	wxsFormat( L"Array index out of bounds accessing object '%s' (index=%d, size=%d)", objname, (idx), (sze) ) )


extern void pxOnAssert( const wxChar* file, int line, const char* func, const wxChar* cond, const wxChar* msg);
extern void pxOnAssert( const wxChar* file, int line, const char* func, const wxChar* cond, const char* msg);

