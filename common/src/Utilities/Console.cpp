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

#include "PrecompiledHeader.h"
#include "Threading.h"

using namespace Threading;
using namespace std;

namespace Console
{
	MutexLock m_writelock;

	bool __fastcall Write( Colors color, const wxString& fmt )
	{
		SetColor( color );
		Write( fmt );
		ClearColor();

		return false;
	}

	bool __fastcall WriteLn( Colors color, const wxString& fmt )
	{
		SetColor( color );
		WriteLn( fmt );
		ClearColor();
		return false;
	}

	// ------------------------------------------------------------------------
	__forceinline void __fastcall _Write( const char* fmt, va_list args )
	{
		std::string m_format_buffer;
		vssprintf( m_format_buffer, fmt, args );
		Write( wxString::FromUTF8( m_format_buffer.c_str() ) );
	}

	__forceinline void __fastcall _WriteLn( const char* fmt, va_list args )
	{
		std::string m_format_buffer;
		vssprintf( m_format_buffer, fmt, args );
		m_format_buffer += "\n";
		Write( wxString::FromUTF8( m_format_buffer.c_str() ) );
	}

	__forceinline void __fastcall _WriteLn( Colors color, const char* fmt, va_list args )
	{
		SetColor( color );
		_WriteLn( fmt, args );
		ClearColor();
	}

	// ------------------------------------------------------------------------
	bool Write( const char* fmt, ... )
	{
		va_list args;
		va_start(args,fmt);
		_Write( fmt, args );
		va_end(args);

		return false;
	}

	bool Write( Colors color, const char* fmt, ... )
	{
		va_list args;
		va_start(args,fmt);
		SetColor( color );
		_Write( fmt, args );
		ClearColor();
		va_end(args);

		return false;
	}

	// ------------------------------------------------------------------------
	bool WriteLn( const char* fmt, ... )
	{
		va_list args;
		va_start(args,fmt);
		_WriteLn( fmt, args );
		va_end(args);

		return false;
	}

	bool WriteLn( Colors color, const char* fmt, ... )
	{
		va_list args;
		va_start(args,fmt);
		_WriteLn( color, fmt, args );
		va_end(args);
		return false;
	}

	// ------------------------------------------------------------------------
	bool Error( const char* fmt, ... )
	{
		va_list args;
		va_start(args,fmt);
		_WriteLn( Color_Red, fmt, args );
		va_end(args);
		return false;
	}

	bool Notice( const char* fmt, ... )
	{
		va_list list;
		va_start(list,fmt);
		_WriteLn( Color_Yellow, fmt, list );
		va_end(list);
		return false;
	}

	bool Status( const char* fmt, ... )
	{
		va_list list;
		va_start(list,fmt);
		_WriteLn( Color_Green, fmt, list );
		va_end(list);
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

