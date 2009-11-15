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
	Color_StrongMagenta,
	Color_StrongOrange,	// intended for warnings
	Color_StrongGray,

	Color_StrongCyan,
	Color_StrongYellow,
	Color_StrongWhite,

    Color_Default,

	ConsoleColors_Count
};

static const ConsoleColors DefaultConsoleColor = Color_Default;


// Use fastcall for the console; should be helpful in most cases
#define __concall	__fastcall

// ----------------------------------------------------------------------------------------
//  IConsoleWriter -- For printing messages to the console.
// ----------------------------------------------------------------------------------------
// General ConsoleWrite Threading Guideline:
//   PCSX2 is a threaded environment and multiple threads can write to the console asynchronously.
//   Individual calls to ConsoleWriter APIs will be written in atomic fashion, however "partial"
//   logs may end up interrupted by logs on other threads.  This is usually not a problem for
//   WriteLn, but may be undesirable for typical uses of Write.  In cases where you want to
//   print multi-line blocks of uninterrupted logs, compound the entire log into a single large
//   string and issue that to WriteLn.
//
struct IConsoleWriter
{
	// Write implementation for internal use only. (can be NULL)
	void (__concall *DoWrite)( const wxString& fmt );

	// WriteLn implementation for internal use only.  (can be NULL)
	void (__concall *DoWriteLn)( const wxString& fmt );

	// SetColor implementation for internal use only.
	void (__concall *DoSetColor)( ConsoleColors color );

	void (__concall *Newline)();
	void (__concall *SetTitle)( const wxString& title );

	// ----------------------------------------------------------------------------
	// Public members; call these to print stuff to console!
	//
	// All functions always return false.  Return value is provided only so that we can easily
	// disable logs at compile time using the "0&&action" macro trick.

	ConsoleColors GetColor() const;
	const IConsoleWriter& SetColor( ConsoleColors color ) const;
	const IConsoleWriter& ClearColor() const;
	const IConsoleWriter& SetIndent( int tabcount=1 ) const;

	IConsoleWriter Indent( int tabcount=1 ) const;

	bool Write( ConsoleColors color, const char* fmt, ... ) const;
	bool WriteLn( ConsoleColors color, const char* fmt, ... ) const;
	bool Write( const char* fmt, ... ) const;
	bool WriteLn( const char* fmt, ... ) const;
	bool Error( const char* fmt, ... ) const;
	bool Warning( const char* fmt, ... ) const;

	bool Write( ConsoleColors color, const wxChar* fmt, ... ) const;
	bool WriteLn( ConsoleColors color, const wxChar* fmt, ... ) const;
	bool Write( const wxChar* fmt, ... ) const;
	bool WriteLn( const wxChar* fmt, ... ) const;
	bool Error( const wxChar* fmt, ... ) const;
	bool Warning( const wxChar* fmt, ... ) const;

	// internal value for indentation of individual lines.  Use the Indent() member to invoke.
	int _imm_indentation;
	
	// For internal use only.
	wxString _addIndentation( const wxString& src, int glob_indent ) const;
};

extern IConsoleWriter	Console;

// --------------------------------------------------------------------------------------
//  ConsoleIndentScope
// --------------------------------------------------------------------------------------
// Provides a scoped indentation of the IConsoleWriter interface for the current thread.
// Any console writes performed from this scope will be indented by the specified number
// of tab characters.
//
// Scoped Object Notes:  Like most scoped objects, this is intended to be used as a stack
// or temporary object only.  Using it in a situation where the object's lifespan out-lives
// a scope will almost certainly result in unintended /undefined side effects.
//
class ConsoleIndentScope
{
	DeclareNoncopyableObject( ConsoleIndentScope );

protected:
	int m_amount;

public:
	// Constructor: The specified number of tabs will be appended to the current indentation
	// setting.  The tabs will be unrolled when the object leaves scope or is destroyed.
	ConsoleIndentScope( int tabs=1 )
	{
		Console.SetIndent( m_amount = tabs );
	}

	virtual ~ConsoleIndentScope() throw()
	{
		Console.SetIndent( -m_amount );
	}
};

// --------------------------------------------------------------------------------------
//  ConsoleColorScope
// --------------------------------------------------------------------------------------
class ConsoleColorScope
{
	DeclareNoncopyableObject( ConsoleColorScope );

protected:
	ConsoleColors m_old_color;

public:
	ConsoleColorScope( ConsoleColors newcolor )
	{
		m_old_color = Console.GetColor();
		Console.SetColor( newcolor );
	}

	virtual ~ConsoleColorScope() throw()
	{
		Console.SetColor( m_old_color );
	}
};

// --------------------------------------------------------------------------------------
//  ConsoleAttrScope
// --------------------------------------------------------------------------------------
// Applies both color and tab attributes in a single object constructor.
//
class ConsoleAttrScope
{
	DeclareNoncopyableObject( ConsoleAttrScope );

protected:
	ConsoleColors	m_old_color;
	int				m_tabsize;

public:
	ConsoleAttrScope( ConsoleColors newcolor, int indent=0 )
	{
		m_old_color = Console.GetColor();
		Console.SetIndent( m_tabsize = indent );
		Console.SetColor( newcolor );
	}

	virtual ~ConsoleAttrScope() throw()
	{
		Console.SetColor( m_old_color );
		Console.SetIndent( -m_tabsize );
	}
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

#ifdef PCSX2_DEVBUILD
	extern IConsoleWriter	DevConWriter;
#	define DevCon			DevConWriter
#else
#	define DevCon			0&&ConsoleWriter_Null
#endif

#ifdef PCSX2_DEBUG
	extern IConsoleWriter	DbgConWriter;
#	define DbgCon			DbgConWriter
#else
#	define DbgCon			0&&ConsoleWriter_Null
#endif

