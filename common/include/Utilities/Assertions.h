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

#ifndef __pxFUNCTION__
#if defined(__GNUG__)
#	define __pxFUNCTION__	__PRETTY_FUNCTION__
#else
#	define __pxFUNCTION__	__FUNCTION__
#endif
#endif

#ifndef wxNullChar
#	define wxNullChar ((wxChar*)NULL)
#endif

// FnChar_t - function name char type; typedef'd in case it ever changes between compilers
//   (ie, a compiler decides to wchar_t it instead of char/UTF8).
typedef char FnChar_t;

// --------------------------------------------------------------------------------------
//  DiagnosticOrigin
// --------------------------------------------------------------------------------------
struct DiagnosticOrigin
{
	const wxChar*	srcfile;
	const FnChar_t*	function;
	const wxChar*	condition;
	int				line;

	DiagnosticOrigin( const wxChar *_file, int _line, const FnChar_t *_func, const wxChar* _cond = NULL )
		: srcfile( _file )
		, function( _func )
		, condition( _cond )
		, line( _line )
	{
	}

	wxString ToString( const wxChar* msg=NULL ) const;
};

// Returns ture if the assertion is to trap into the debugger, or false if execution
// of the program should continue unimpeded.
typedef bool pxDoAssertFnType(const DiagnosticOrigin& origin, const wxChar *msg);

extern pxDoAssertFnType pxAssertImpl_LogIt;

extern pxDoAssertFnType* pxDoAssert;

// ----------------------------------------------------------------------------------------
//  pxAssert / pxAssertDev
// ----------------------------------------------------------------------------------------
// Standard "nothrow" assertions.  All assertions act as valid conditional statements that
// return the result of the specified conditional; useful for handling failed assertions in
// a "graceful" fashion when utilizing the "ignore" feature of assertion debugging.
// These macros are mostly intended for "pseudo-weak" assumptions within code, most often for
// testing threaded user interface code (threading of the UI is a prime example since often
// even very robust assertions can fail in very rare conditions, due to the complex variety
// of ways the user can invoke UI events).
//
// All macros return TRUE if the assertion succeeds, or FALSE if the assertion failed
// (thus matching the condition of the assertion itself).
//
// pxAssertDev is an assertion tool for Devel builds, intended for sanity checking and/or
// bounds checking variables in areas which are not performance critical.  Another common
// use is for checking thread affinity on utility functions.
//
// Credits: These macros are based on a combination of wxASSERT, MSVCRT's assert and the
// ATL's Assertion/Assumption macros.  the best of all worlds!

// --------------------------------------------------------------------------------------
//  pxAssume / pxAssumeDev / pxFail / pxFailDev
// --------------------------------------------------------------------------------------
// Assumptions are hints to the compiler that the condition will always be true,
// the condition should never fail under any circumstance in release builds
// or else you might get incorrect compiler generated code.
//
// Performance: All assumption/fail  types optimize into __assume()/likely() directives in
// Release builds (non-dev varieties optimize as such in Devel builds as well).
// __assume(0) is a special form of __assume() which tells the compiler that the code path
// is not reachable and will cause undefined results if it is reachable...
//
// Having pxFail and pxFailDev translate into __assume statements is very dangerous, since
// it can lead to the compiler optimizing out code and leading to crashes in dev/release
// builds. To have code optimized, explicitly use pxAssume(false) or pxAssumeDev(false,msg);

#define pxDiagSpot			DiagnosticOrigin( __TFILE__, __LINE__, __pxFUNCTION__ )
#define pxAssertSpot(cond)	DiagnosticOrigin( __TFILE__, __LINE__, __pxFUNCTION__, _T(#cond) )

// pxAssertRel ->
// Special release-mode assertion.  Limited use since stack traces in release mode builds
// (especially with LTCG) are highly suspect.  But when troubleshooting crashes that only
// rear ugly heads in optimized builds, this is one of the few tools we have.

#define pxAssertRel(cond, msg)		( (likely(cond)) || (pxOnAssert(pxAssertSpot(cond), msg), false) )
#define pxAssumeRel(cond, msg)		((void) ( (!likely(cond)) && (pxOnAssert(pxAssertSpot(cond), msg), false) ))
#define pxFailRel(msg)				pxAssertRel(false, msg)

#if defined(PCSX2_DEBUG)

#	define pxAssertMsg(cond, msg)	pxAssertRel(cond, msg)
#	define pxAssertDev(cond, msg)	pxAssertMsg(cond, msg)

#	define pxAssumeMsg(cond, msg)	pxAssumeRel(cond, msg)
#	define pxAssumeDev(cond, msg)	pxAssumeRel(cond, msg)

#	define pxFail(msg)				pxAssertMsg(false, msg)
#	define pxFailDev(msg)			pxAssertDev(false, msg)

#elif defined(PCSX2_DEVBUILD)

	// Devel builds now will give you a release-mode assertion dialog window if any of the
	// following macro's 'cond' field is false.
	// Note: Only use pxAssume/Msg/Dev if you know what you're doing, __assume is supposed
	// to be used as an optimization hint, yet many devs have been using psAssume
	// thinking its the same as an assertion.
	// __assume(0) is also very dangerous because it is a special case of __assume() which
	// tells the compiler that the code path is not reachable, and it can cause unpredictable
	// results if the code path can be reached.
	// i.e. if (1) { __assume(0); something(); } 
	// In the above example, something() may never be called.
	// __assume(0)'s real use is in optimizing stuff such as "default:" cases on a switch
	// statement. See jNO_DEFAULT

#	define pxAssertMsg(cond, msg)	pxAssertRel(cond, msg)
#	define pxAssertDev(cond, msg)	pxAssertRel(cond, msg)

#	define pxAssumeMsg(cond, msg)	pxAssumeRel(cond, msg) //(__assume(cond))
#	define pxAssumeDev(cond, msg)	pxAssumeRel(cond, msg)

#	define pxFail(msg)				pxAssertDev(false, msg)
#	define pxFailDev(msg)			pxAssertDev(false, msg)

#else

	// Release Builds just use __assume as an optimization, and return the conditional
	// as a result (which is optimized to nil if unused).

#	define pxAssertMsg(cond, msg)	(likely(cond))
#	define pxAssertDev(cond, msg)	(likely(cond))

#	define pxAssumeMsg(cond, msg)	(__assume(cond))
#	define pxAssumeDev(cond, msg)	(__assume(cond))

#	define pxFail(msg)				do{} while(0)
#	define pxFailDev(msg)			do{} while(0)

#endif

#define pxAssert(cond)				pxAssertMsg(cond, wxNullChar)
#define pxAssume(cond)				pxAssumeMsg(cond, wxNullChar)

#define pxAssertRelease( cond, msg )

// Performs an unsigned index bounds check, and generates a debug assertion if the check fails.
// For stricter checking in Devel builds as well as debug builds (but possibly slower), use
// IndexBoundsCheckDev.

#define IndexBoundsCheck( objname, idx, sze )		pxAssertMsg( (uint)(idx) < (uint)(sze), \
	pxsFmt( L"Array index out of bounds accessing object '%s' (index=%d, size=%d)", objname, (idx), (sze) ) )

#define IndexBoundsCheckDev( objname, idx, sze )	pxAssertDev( (uint)(idx) < (uint)(sze), \
	pxsFmt( L"Array index out of bounds accessing object '%s' (index=%d, size=%d)", objname, (idx), (sze) ) )

#define IndexBoundsAssume( objname, idx, sze )		pxAssumeMsg( (uint)(idx) < (uint)(sze), \
	pxsFmt( L"Array index out of bounds accessing object '%s' (index=%d, size=%d)", objname, (idx), (sze) ) )

#define IndexBoundsAssumeDev( objname, idx, sze )	pxAssumeDev( (uint)(idx) < (uint)(sze), \
	pxsFmt( L"Array index out of bounds accessing object '%s' (index=%d, size=%d)", objname, (idx), (sze) ) )


extern void pxOnAssert( const DiagnosticOrigin& origin, const wxChar* msg=NULL );
extern void pxOnAssert( const DiagnosticOrigin& origin, const char* msg );

// --------------------------------------------------------------------------------------
// jNO_DEFAULT -- disables the default case in a switch, which improves switch optimization
// under MSVC.
// --------------------------------------------------------------------------------------
// How it Works: pxAssumeDev turns into an __assume(0) under msvc compilers, which when specified
// in the 'default:' case of a switch tells the compiler that the case is unreachable, so
// that it will not generate any code, LUTs, or conditionals to handle it.
//
// * In debug/devel builds the default case will cause an assertion.
//
#ifndef jNO_DEFAULT
#	define jNO_DEFAULT \
		default: \
		{ \
			pxAssumeDev( 0, "Incorrect usage of jNO_DEFAULT detected (default case is not unreachable!)" ); \
			break; \
		}
#endif
