/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SPU2X_GLOBAL_H_
#define _SPU2X_GLOBAL_H_

#define NOMINMAX

struct StereoOut16;
struct StereoOut32;
struct StereoOutFloat;

struct V_Core;

namespace soundtouch
{
	class SoundTouch;
}

#include <assert.h>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <ctime>
#include <stdexcept>

// This will be brought in later anyways, but if we bring it in now, it'll avoid
// warnings about redefining __LINUX__.
#include "Utilities/Dependencies.h"
#include "Pcsx2Defs.h"
#include "Pcsx2Types.h"

namespace VersionInfo
{
	static const u8 Release  = 1;
	static const u8 Revision = 5;	// increase that with each version
}

//////////////////////////////////////////////////////////////////////////
// Override Win32 min/max macros with the STL's type safe and macro
// free varieties (much safer!)

#undef min
#undef max

template< typename T >
static __forceinline void Clampify( T& src, T min, T max )
{
	src = std::min( std::max( src, min ), max );
}

template< typename T >
static __forceinline T GetClamped( T src, T min, T max )
{
	return std::min( std::max( src, min ), max );
}

extern void SysMessage(const char *fmt, ...);
extern void SysMessage(const wchar_t *fmt, ...);

//////////////////////////////////////////////////////////////
// Dev / Debug conditionals --
//   Consts for using if() statements instead of uglier #ifdef macros.
//   Abbreviated macros for dev/debug only consoles and msgboxes.

#ifdef PCSX2_DEVBUILD
#	define DevMsg MsgBox
#else
#	define DevMsg
#endif

#ifdef PCSX2_DEVBUILD
#	define SPU2_LOG
#endif

// Uncomment to enable debug keys on numpad (0 to 5)
//#define DEBUG_KEYS

#include "Utilities/Exceptions.h"
#include "Utilities/SafeArray.h"

#include "defs.h"
#include "regs.h"

#include "Config.h"
#include "Debug.h"
#include "SndOut.h"

#endif
