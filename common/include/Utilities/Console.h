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

#include "StringHelpers.h"


enum ConsoleColors
{
	Color_Current = -1,

	Color_Black = 0,
	Color_Green,
	Color_Red,
	Color_Blue,
	Color_Magenta,
	Color_Orange,
	Color_Gray,

	Color_Cyan,			// faint visibility, intended for logging PS2/IOP output
	Color_Yellow,		// faint visibility, intended for logging PS2/IOP output
	Color_White,		// faint visibility, intended for logging PS2/IOP output

	// Strong text *may* result in mis-aligned text in the console, depending on the
	// font and the platform, so use these with caution.

	Color_StrongBlack,
	Color_StrongRed,	// intended for errors
	Color_StrongGreen,	// intended for infrequent state information
	Color_StrongBlue,	// intended for block headings
	Color_StrongOrange,	// intended for warnings
	
	ConsoleColors_Count
};

// Use fastcall for the console; should be helpful in most cases
#define __concall	__fastcall

// ----------------------------------------------------------------------------------------
//  IConsoleWriter -- For printing messages to the console.
// ----------------------------------------------------------------------------------------
//
struct IConsoleWriter
{
	// Write implementation for internal use only.
	void (__concall *DoWrite)( const wxString& fmt );

	// WriteLn implementation for internal use only.
	void (__concall *DoWriteLn)( const wxString& fmt );

	void (__concall *Newline)();

	void (__concall *SetTitle)( const wxString& title );

	// Changes the active console color.
	// This color will be unset by calls to colored text methods
	// such as ErrorMsg and Notice.
	void (__concall *SetColor)( ConsoleColors color );

	// Restores the console color to default (usually low-intensity white on Win32)
	void (__concall *ClearColor)();

	// ----------------------------------------------------------------------------
	// Public members; call these to print stuff to console!


	void Write( ConsoleColors color, const char* fmt, ... ) const;
	void WriteLn( ConsoleColors color, const char* fmt, ... ) const;
	void Write( const char* fmt, ... ) const;
	void WriteLn( const char* fmt, ... ) const;
	void Error( const char* fmt, ... ) const;
	void Warning( const char* fmt, ... ) const;

	void Write( ConsoleColors color, const wxChar* fmt, ... ) const;
	void WriteLn( ConsoleColors color, const wxChar* fmt, ... ) const;
	void Write( const wxChar* fmt, ... ) const;
	void WriteLn( const wxChar* fmt, ... ) const;
	void Error( const wxChar* fmt, ... ) const;
	void Warning( const wxChar* fmt, ... ) const;

};

extern void Console_SetActiveHandler( const IConsoleWriter& writer, FILE* flushfp=NULL );
extern const wxString& ConsoleBuffer_Get();
extern void ConsoleBuffer_Clear();
extern void ConsoleBuffer_FlushToFile( FILE *fp );

extern const IConsoleWriter		ConsoleWriter_Null;
extern const IConsoleWriter		ConsoleWriter_Stdio;
extern const IConsoleWriter		ConsoleWriter_Assert;
extern const IConsoleWriter		ConsoleWriter_Buffered;
extern const IConsoleWriter		ConsoleWriter_wxError;

extern IConsoleWriter	Console;

#ifdef PCSX2_DEVBUILD
	extern IConsoleWriter	DevConWriter;
#	define DevCon			DevConWriter
#else
#	define DevCon			ConsoleWriter_Null
#endif

#ifdef PCSX2_DEBUG
	extern IConsoleWriter	DbgConWriter;
#	define DbgCon			DbgConWriter
#else
#	define DbgCon			ConsoleWriter_Null
#endif

