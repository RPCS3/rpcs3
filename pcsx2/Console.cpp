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
#include "System.h"
#include "DebugTools/Debug.h"
#include "NewGUI/App.h"


using namespace Threading;
using namespace std;

const _VARG_PARAM va_arg_dummy = { 0 };

// Methods of the Console namespace not defined here are to be found in the platform
// dependent implementations in WinConsole.cpp and LnxConsole.cpp.

namespace Console
{
	ConsoleLogFrame* FrameHandle = NULL;
	MutexLock m_writelock;
	std::string m_format_buffer;

	void __fastcall SetTitle( const wxString& title )
	{
		ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame();
		if( FrameHandle != NULL )
			FrameHandle->SetTitle( title );
	}

	void __fastcall SetColor( Colors color )
	{
		ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame();
		if( FrameHandle != NULL )
			FrameHandle->SetColor( color );
	}

	void ClearColor()
	{
		ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame();
		if( FrameHandle != NULL )
			FrameHandle->ClearColor();		
	}

	bool Newline()
	{
		ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame();
		if( FrameHandle != NULL )
			FrameHandle->Newline();
		
		fputs( "", emuLog );
		return false;
	}

	bool __fastcall Write( const char* fmt )
	{
		ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame();
		if( FrameHandle != NULL )
			FrameHandle->Write( fmt );

		fwrite( fmt, 1, strlen( fmt ), emuLog );
		return false;
	}

	bool __fastcall Write( Colors color, const char* fmt )
	{
		ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame();
		if( FrameHandle != NULL )
		{
			FrameHandle->SetColor( color );
			FrameHandle->Write( fmt );
			FrameHandle->ClearColor();
		}

		fwrite( fmt, 1, strlen( fmt ), emuLog );
		return false;
	}

	// Writes an unformatted string of text to the console (fast!)
	// A newline is automatically appended.
	bool __fastcall WriteLn( const char* fmt )
	{
		ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame();
		if( FrameHandle != NULL )
		{
			FrameHandle->Write( fmt );
			FrameHandle->Newline();
		}

		fputs( fmt, emuLog );
		return false;
	}

	// Writes an unformatted string of text to the console (fast!)
	// A newline is automatically appended.
	bool __fastcall WriteLn( Colors color, const char* fmt )
	{
		ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame();
		if( FrameHandle != NULL )
		{
			FrameHandle->SetColor( color );
			FrameHandle->Write( fmt );
			FrameHandle->Newline();
			FrameHandle->ClearColor();
		}

		fputs( fmt, emuLog );
		return false;
	}

	__forceinline void __fastcall _WriteLn( Colors color, const char* fmt, va_list args )
	{
		ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame();

		ScopedLock locker( m_writelock );
		vssprintf( m_format_buffer, fmt, args );
		const char* cstr = m_format_buffer.c_str();
		if( FrameHandle != NULL )
		{
			FrameHandle->SetColor( color );
			FrameHandle->Write( cstr );
			FrameHandle->Newline();
			FrameHandle->ClearColor();
		}
		else
		{
			// The logging system hasn't been initialized, so log to stderr which at least
			// has a chance of being visible, and then assert (really there shouldn't be logs
			// being attempted prior to log/console initialization anyway, and the programmer
			// should replace these with MessageBoxes or something).

			if( color == Color_Red || color == Color_Yellow )
				fputs( cstr, stderr );		// log notices and errors to stderr

			wxASSERT_MSG( 0, cstr );
		}

		fputs( cstr, emuLog );
	}
		
	bool Write( const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();

		va_list list;
		va_start(list,dummy);

		ScopedLock locker( m_writelock );
		vssprintf( m_format_buffer, fmt, list );
		Write( m_format_buffer.c_str() );

		va_end(list);

		return false;
	}

	bool Write( Colors color, const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();

		va_list list;
		va_start(list,dummy);

		ScopedLock locker( m_writelock );
		vssprintf( m_format_buffer, fmt, list );
		Write( color, m_format_buffer.c_str() );

		va_end(list);
		return false;
	}

	bool WriteLn( const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();
		va_list list;
		va_start(list,dummy);

		ScopedLock locker( m_writelock );
		vssprintf( m_format_buffer, fmt, list );
		WriteLn( m_format_buffer.c_str() );

		va_end(list);
		return false;
	}

	// Writes a line of colored text to the console, with automatic newline appendage.
	bool WriteLn( Colors color, const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();

		va_list list;
		va_start(list,dummy);
		_WriteLn( Color_White, fmt, list );
		va_end(list);
		return false;
	}

	// Displays a message in the console with red emphasis.
	// Newline is automatically appended.
	bool Error( const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();

		va_list list;
		va_start(list,dummy);
		_WriteLn( Color_Red, fmt, list );
		va_end(list);
		return false;
	}

	// Displays a message in the console with yellow emphasis.
	// Newline is automatically appended.
	bool Notice( const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();

		va_list list;
		va_start(list,dummy);
		_WriteLn( Color_Yellow, fmt, list );
		va_end(list);
		return false;
	}

	// Displays a message in the console with green emphasis.
	// Newline is automatically appended.
	bool Status( const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();

		va_list list;
		va_start(list,dummy);
		_WriteLn( Color_Green, fmt, list );
		va_end(list);
		return false;
	}

	// Displays a message in the console with red emphasis.
	// Newline is automatically appended.
	bool __fastcall Error( const char* fmt )
	{
		WriteLn( Color_Red, fmt );
		return false;
	}

	// Displays a message in the console with yellow emphasis.
	// Newline is automatically appended.
	bool __fastcall Notice( const char* fmt )
	{
		WriteLn( Color_Yellow, fmt );
		return false;
	}

	// Displays a message in the console with green emphasis.
	// Newline is automatically appended.
	bool __fastcall Status( const char* fmt )
	{
		WriteLn( Color_Green, fmt );
		return false;
	}

}

namespace Msgbox
{
	bool Alert(const char* text)
	{
		wxMessageBox( text, "Pcsx2 Message", wxOK, wxGetApp().GetTopWindow() );
		return false;
	}

	bool Alert(const char* fmt, VARG_PARAM dummy, ...)
	{
		va_list list;
		va_start(list, dummy);
		Alert( vfmt_string( fmt, list ).c_str() );
		va_end(list);
		
		return false;
	}
	
	bool OkCancel( const char* fmt, VARG_PARAM dummy, ... )
	{
		va_list list;
		va_start(list, dummy);
		int result = wxMessageBox( vfmt_string( fmt, list ), "Pcsx2 Message", wxOK | wxCANCEL, wxGetApp().GetTopWindow() );
		va_end(list);

		return result == wxOK;		
	}
}