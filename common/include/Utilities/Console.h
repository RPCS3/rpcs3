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

#pragma once

#include "StringHelpers.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Console Namespace -- For printing messages to the console.
//
// SysPrintf is depreciated; We should phase these in over time.
//
namespace Console
{
	enum Colors
	{
		Color_Black = 0,
		Color_Red,
		Color_Green,
		Color_Yellow,
		Color_Blue,
		Color_Magenta,
		Color_Cyan,
		Color_White
	};

	// va_args version of WriteLn, mostly for internal use only.
	extern void __fastcall _WriteLn( Colors color, const char* fmt, va_list args );

	extern void __fastcall SetTitle( const wxString& title );

	// Changes the active console color.
	// This color will be unset by calls to colored text methods
	// such as ErrorMsg and Notice.
	extern void __fastcall SetColor( Colors color );

	// Restores the console color to default (usually low-intensity white on Win32)
	extern void ClearColor();

	// The following Write functions return bool so that we can use macros to exclude
	// them from different build types.  The return values are always zero.

	// Writes a newline to the console.
	extern bool Newline();

	// Writes a line of colored text to the console, with automatic newline appendage.
	// The console color is reset to default when the operation is complete.
	extern bool WriteLn( Colors color, const char* fmt, ... );

	// Writes a formatted message to the console, with appended newline.
	extern bool WriteLn( const char* fmt, ... );

	// Writes a line of colored text to the console (no newline).
	// The console color is reset to default when the operation is complete.
	extern bool Write( Colors color, const char* fmt, ... );

	// Writes a formatted message to the console (no newline)
	extern bool Write( const char* fmt, ... );

	// Displays a message in the console with red emphasis.
	// Newline is automatically appended.
	extern bool Error( const char* fmt, ... );

	// Displays a message in the console with yellow emphasis.
	// Newline is automatically appended.
	extern bool Notice( const char* fmt, ... );

	// Displays a message in the console with yellow emphasis.
	// Newline is automatically appended.
	extern bool Status( const char* fmt, ... );


	extern bool __fastcall Write( const wxString& text );
	extern bool __fastcall Write( Colors color, const wxString& text );
	extern bool __fastcall WriteLn( const wxString& text );
	extern bool __fastcall WriteLn( Colors color, const wxString& text );

	extern bool __fastcall Error( const wxString& text );
	extern bool __fastcall Notice( const wxString& text );
	extern bool __fastcall Status( const wxString& text );
}

using Console::Color_Black;
using Console::Color_Red;
using Console::Color_Green;
using Console::Color_Blue;
using Console::Color_Magenta;
using Console::Color_Cyan;
using Console::Color_Yellow;
using Console::Color_White;

//////////////////////////////////////////////////////////////////////////////////////////
// DevCon / DbgCon

#ifdef PCSX2_DEVBUILD
#	define DevCon Console
#	define DevMsg MsgBox
#else
#	define DevCon 0&&Console
#	define DevMsg 
#endif

#ifdef PCSX2_DEBUG
#	define DbgCon Console
#else
#	define DbgCon 0&&Console
#endif
