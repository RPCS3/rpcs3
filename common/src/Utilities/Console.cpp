/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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

#include "PrecompiledHeader.h"
#include "Threading.h"

using namespace Threading;
using namespace std;

const _VARG_PARAM va_arg_dummy = { 0 };

namespace Console
{
	MutexLock m_writelock;
	std::string m_format_buffer;

	// ------------------------------------------------------------------------
	bool __fastcall Write( Colors color, const char* fmt )
	{
		SetColor( color );
		Write( fmt );
		ClearColor();

		return false;
	}

	// ------------------------------------------------------------------------
	bool __fastcall Write( Colors color, const wxString& fmt )
	{
		SetColor( color );
		Write( fmt );
		ClearColor();

		return false;
	}

	// ------------------------------------------------------------------------
	bool __fastcall WriteLn( const char* fmt )
	{
		Write( fmt );
		Newline();
		return false;
	}

	bool __fastcall WriteLn( Colors color, const char* fmt )
	{
		SetColor( color );
		Write( fmt );
		Newline();
		ClearColor();

		return false;
	}

	// ------------------------------------------------------------------------
	bool __fastcall WriteLn( const wxString& fmt )
	{
		Write( fmt );
		Newline();
		return false;
	}

	bool __fastcall WriteLn( Colors color, const wxString& fmt )
	{
		SetColor( color );
		Write( fmt );
		Newline();
		ClearColor();
		return false;
	}

	// ------------------------------------------------------------------------
	__forceinline void __fastcall _Write( const char* fmt, va_list args )
	{
		ScopedLock locker( m_writelock );
		vssprintf( m_format_buffer, fmt, args );
		const char* cstr = m_format_buffer.c_str();

		Write( fmt );
	}

	__forceinline void __fastcall _WriteLn( const char* fmt, va_list args )
	{
		_Write( fmt, args );
		Newline();
	}

	__forceinline void __fastcall _WriteLn( Colors color, const char* fmt, va_list args )
	{
		SetColor( color );
		_WriteLn( fmt, args );
		ClearColor();
	}
		
	// ------------------------------------------------------------------------
	bool Write( const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();

		va_list args;
		va_start(args,dummy);
		_Write( fmt, args );
		va_end(args);

		return false;
	}

	bool Write( Colors color, const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();

		va_list args;
		va_start(args,dummy);
		SetColor( color );
		_Write( fmt, args );
		ClearColor();
		va_end(args);

		return false;
	}

	// ------------------------------------------------------------------------
	bool WriteLn( const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();
		va_list args;
		va_start(args,dummy);
		_WriteLn( fmt, args );
		va_end(args);

		return false;
	}

	// ------------------------------------------------------------------------
	bool WriteLn( Colors color, const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();

		va_list args;
		va_start(args,dummy);
		_WriteLn( color, fmt, args );
		va_end(args);
		return false;
	}

	// ------------------------------------------------------------------------
	bool Error( const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();

		va_list args;
		va_start(args,dummy);
		_WriteLn( Color_Red, fmt, args );
		va_end(args);
		return false;
	}

	// ------------------------------------------------------------------------
	bool Notice( const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();

		va_list list;
		va_start(list,dummy);
		_WriteLn( Color_Yellow, fmt, list );
		va_end(list);
		return false;
	}

	// ------------------------------------------------------------------------
	bool Status( const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();

		va_list list;
		va_start(list,dummy);
		_WriteLn( Color_Green, fmt, list );
		va_end(list);
		return false;
	}

	// ------------------------------------------------------------------------
	bool __fastcall Error( const char* fmt )
	{
		WriteLn( Color_Red, fmt );
		return false;
	}

	bool __fastcall Notice( const char* fmt )
	{
		WriteLn( Color_Yellow, fmt );
		return false;
	}

	bool __fastcall Status( const char* fmt )
	{
		WriteLn( Color_Green, fmt );
		return false;
	}

	// ------------------------------------------------------------------------
	bool __fastcall Error( const wxString& src )
	{
		WriteLn( Color_Red, src );
		return false;
	}

	bool __fastcall Notice( const wxString& src )
	{
		WriteLn( Color_Yellow, src );
		return false;
	}

	bool __fastcall Status( const wxString& src )
	{
		WriteLn( Color_Green, src );
		return false;
	}

}

