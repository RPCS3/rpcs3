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

#ifndef _PCSX2_STRINGUTILS_H_
#define _PCSX2_STRINGUTILS_H_

#include <string>
#include <cstdarg>
#include <sstream>

// to_string: A utility template for quick and easy inline string type conversion.
// Use to_string(intval), or to_string(float), etc.  Anything that the STL itself
// would support should be supported here. :)
template< typename T >
std::string to_string(const T& value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

// dummy structure used to type-guard the dummy parameter that's been inserted to
// allow us to use the va_list feature on references.
struct _VARG_PARAM
{
	// just some value to make the struct length 32bits instead of 8 bits, so that the
	// compiler generates somewhat more efficient code.
	uint someval;
};

#ifdef _DEBUG

#define params va_arg_dummy,
#define varg_assert()  // jASSUME( dummy == &va_arg_dummy );
// typedef the Va-Arg value to be a value type in debug builds.  The value
// type requires a little more overhead in terms of code generation, but is always
// type-safe.  The compiler will generate errors for any forgotten params value.
typedef _VARG_PARAM VARG_PARAM;

#else

#define params NULL,	// using null is faster / more compact!
#define varg_assert()	jASSUME( dummy == NULL );
// typedef the Va-Arg value to be a pointer in release builds.  Pointers
// generate more compact code by a small margin, but aren't entirely type safe since
// the compiler won't generate errors if you pass NULL or other values.
typedef _VARG_PARAM const * VARG_PARAM;

#endif

extern const _VARG_PARAM va_arg_dummy;

extern void ssprintf(std::string& dest, const char* fmt, ...);
extern void ssappendf( std::string& dest, const char* format, ...);
extern void vssprintf(std::string& dest, const char* format, va_list args);
extern void vssappendf(std::string& dest, const char* format, va_list args);

extern std::string fmt_string( const char* fmt, ... );
extern std::string vfmt_string( const char* fmt, va_list args );
#endif
