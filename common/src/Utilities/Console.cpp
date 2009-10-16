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

// This function re-assigns the console log writer(s) to the specified target.  It makes sure
// to flush any contents from the buffered console log (which typically accumulates due to
// log suspension during log file/window re-init operations) into the new log.
//
// Important!  Only Assert and Null console loggers are allowed during C++ startup init (when
// the program or DLL first loads).  Other log targets rely on the static buffer and a
// threaded mutex lock, which are only valid after C++ initialization has finished.
void Console_SetActiveHandler( const IConsoleWriter& writer, FILE* flushfp )
{
	pxAssertDev(
		(writer.DoWrite != NULL)	&& (writer.DoWriteLn != NULL) &&
		(writer.Newline != NULL)	&& (writer.SetTitle != NULL) &&
		(writer.SetColor != NULL)	&& (writer.ClearColor != NULL),
		"Invalid IConsoleWriter object!  All function pointer interfaces must be implemented."
	);
	
	if( !ConsoleBuffer_Get().IsEmpty() )
		writer.DoWriteLn( ConsoleBuffer_Get() );

	Console	= writer;

#ifdef PCSX2_DEVBUILD
	DevCon	= writer;
#endif

#ifdef PCSX2_DEBUG
	DbgCon	= writer;
#endif
}

// --------------------------------------------------------------------------------------
//  ConsoleNull
// --------------------------------------------------------------------------------------

static void __concall ConsoleNull_SetTitle( const wxString& title ) {}
static void __concall ConsoleNull_SetColor( ConsoleColors color ) {}
static void __concall ConsoleNull_ClearColor() {}
static void __concall ConsoleNull_Newline() {}
static void __concall ConsoleNull_DoWrite( const wxString& fmt ) {}
static void __concall ConsoleNull_DoWriteLn( const wxString& fmt ) {}

const IConsoleWriter ConsoleWriter_Null =
{
	ConsoleNull_DoWrite,
	ConsoleNull_DoWriteLn,

	ConsoleNull_Newline,

	ConsoleNull_SetTitle,
	ConsoleNull_SetColor,
	ConsoleNull_ClearColor,
};

// --------------------------------------------------------------------------------------
//  Console_Stdio
// --------------------------------------------------------------------------------------

// One possible default write action at startup and shutdown is to use the stdout.
static void __concall ConsoleStdio_DoWrite( const wxString& fmt )
{
	wxPrintf( fmt );
}

// Default write action at startup and shutdown is to use the stdout.
static void __concall ConsoleStdio_DoWriteLn( const wxString& fmt )
{
	wxPrintf( fmt + L"\n" );
}

const IConsoleWriter ConsoleWriter_Stdio =
{
	ConsoleStdio_DoWrite,			// Writes without newlines go to buffer to avoid error log spam.
	ConsoleStdio_DoWriteLn,

	ConsoleNull_Newline,

	ConsoleNull_SetTitle,
	ConsoleNull_SetColor,
	ConsoleNull_ClearColor,
};

// --------------------------------------------------------------------------------------
//  ConsoleAssert
// --------------------------------------------------------------------------------------

static void __concall ConsoleAssert_DoWrite( const wxString& fmt )
{
	pxFail( L"Console class has not been initialized; Message written:\n\t" + fmt );
}

static void __concall ConsoleAssert_DoWriteLn( const wxString& fmt )
{
	pxFail( L"Console class has not been initialized; Message written:\n\t" + fmt );
}

const IConsoleWriter ConsoleWriter_Assert =
{
	ConsoleAssert_DoWrite,
	ConsoleAssert_DoWriteLn,

	ConsoleNull_Newline,

	ConsoleNull_SetTitle,
	ConsoleNull_SetColor,
	ConsoleNull_ClearColor,
};

// --------------------------------------------------------------------------------------
//  ConsoleBuffer
// --------------------------------------------------------------------------------------

static wxString	m_buffer;

const wxString& ConsoleBuffer_Get()
{
	return m_buffer;
}

void ConsoleBuffer_Clear()
{
	m_buffer.Clear();
}

// Flushes the contents of the ConsoleBuffer to the specified destination file stream, and
// clears the buffer contents to 0.
void ConsoleBuffer_FlushToFile( FILE *fp )
{
	if( fp == NULL || m_buffer.IsEmpty() ) return;
	px_fputs( fp, m_buffer.ToUTF8() );
	m_buffer.Clear();
}

static void __concall ConsoleBuffer_DoWrite( const wxString& fmt )
{
	m_buffer += fmt;
}

static void __concall ConsoleBuffer_DoWriteLn( const wxString& fmt )
{
	m_buffer += fmt + L"\n";
}

const IConsoleWriter ConsoleWriter_Buffered =
{
	ConsoleBuffer_DoWrite,			// Writes without newlines go to buffer to avoid assertion spam.
	ConsoleBuffer_DoWriteLn,

	ConsoleNull_Newline,

	ConsoleNull_SetTitle,
	ConsoleNull_SetColor,
	ConsoleNull_ClearColor,
};

// --------------------------------------------------------------------------------------
//  Console_wxLogError
// --------------------------------------------------------------------------------------

static void __concall Console_wxLogError_DoWriteLn( const wxString& fmt )
{
	if( !m_buffer.IsEmpty() )
	{
		wxLogError( m_buffer );
		m_buffer.Clear();
	}
	wxLogError( fmt );
}

const IConsoleWriter ConsoleWriter_wxError =
{
	ConsoleBuffer_DoWrite,			// Writes without newlines go to buffer to avoid error log spam.
	Console_wxLogError_DoWriteLn,

	ConsoleNull_Newline,

	ConsoleNull_SetTitle,
	ConsoleNull_SetColor,
	ConsoleNull_ClearColor,
};

// =====================================================================================================
//  IConsole Interfaces
// =====================================================================================================
// (all non-virtual members that do common work and then pass the result through DoWrite
//  or DoWriteLn)

// Writes a line of colored text to the console (no newline).
// The console color is reset to default when the operation is complete.
void IConsoleWriter::Write( ConsoleColors color, const wxString& fmt ) const
{
	SetColor( color );
	Write( fmt );
	ClearColor();
}

// Writes a line of colored text to the console, with automatic newline appendage.
// The console color is reset to default when the operation is complete.
void IConsoleWriter::WriteLn( ConsoleColors color, const wxString& fmt ) const
{
	SetColor( color );
	WriteLn( fmt );
	ClearColor();
}

void IConsoleWriter::_Write( const char* fmt, va_list args ) const
{
	std::string m_format_buffer;
	vssprintf( m_format_buffer, fmt, args );
	Write( fromUTF8( m_format_buffer.c_str() ) );
}

void IConsoleWriter::_WriteLn( const char* fmt, va_list args ) const
{
	std::string m_format_buffer;
	vssprintf( m_format_buffer, fmt, args );
	WriteLn( fromUTF8( m_format_buffer.c_str() ) );
}

void IConsoleWriter::_WriteLn( ConsoleColors color, const char* fmt, va_list args ) const
{
	SetColor( color );
	_WriteLn( fmt, args );
	ClearColor();
}

void IConsoleWriter::Write( const char* fmt, ... ) const
{
	va_list args;
	va_start(args,fmt);
	_Write( fmt, args );
	va_end(args);
}

void IConsoleWriter::Write( ConsoleColors color, const char* fmt, ... ) const
{
	va_list args;
	va_start(args,fmt);
	SetColor( color );
	_Write( fmt, args );
	ClearColor();
	va_end(args);
}

void IConsoleWriter::WriteLn( const char* fmt, ... ) const
{
	va_list args;
	va_start(args,fmt);
	_WriteLn( fmt, args );
	va_end(args);
}

void IConsoleWriter::WriteLn( ConsoleColors color, const char* fmt, ... ) const
{
	va_list args;
	va_start(args,fmt);
	_WriteLn( color, fmt, args );
	va_end(args);
}

void IConsoleWriter::Write( const wxString& src ) const
{
	DoWrite( src );
}

void IConsoleWriter::WriteLn( const wxString& src ) const
{
	DoWriteLn( src );
}

void IConsoleWriter::Error( const char* fmt, ... ) const
{
	va_list args;
	va_start(args,fmt);
	_WriteLn( Color_Red, fmt, args );
	va_end(args);
}

void IConsoleWriter::Notice( const char* fmt, ... ) const
{
	va_list list;
	va_start(list,fmt);
	_WriteLn( Color_Yellow, fmt, list );
	va_end(list);
}

void IConsoleWriter::Status( const char* fmt, ... ) const
{
	va_list list;
	va_start(list,fmt);
	_WriteLn( Color_Green, fmt, list );
	va_end(list);
}

void IConsoleWriter::Error( const wxString& src ) const
{
	WriteLn( Color_Red, src );
}

void IConsoleWriter::Notice( const wxString& src ) const
{
	WriteLn( Color_Yellow, src );
}

void IConsoleWriter::Status( const wxString& src ) const
{
	WriteLn( Color_Green, src );
}

// --------------------------------------------------------------------------------------
//  Default Writer for C++ init / startup:
// --------------------------------------------------------------------------------------
// In GUI modes under Windows I default to Assert, because windows lacks a qualified universal
// program console.  In console mode I use Stdio instead, since the program is pretty well
// promised a valid console in any platform (except maybe Macs, which probably consider consoles
// a fundamental design flaw or something).

#if wxUSE_GUI && defined(__WXMSW__)
#	define _DefaultWriter_	ConsoleWriter_Assert
#else
#	define _DefaultWriter_	ConsoleWriter_Stdio
#endif

// Important!  Only Assert and Null console loggers are allowed for initial console targeting.
// Other log targets rely on the static buffer and a threaded mutex lock, which are only valid
// after C++ initialization has finished.

IConsoleWriter	Console	= _DefaultWriter_;

#ifdef PCSX2_DEVBUILD
	IConsoleWriter	DevConWriter= _DefaultWriter_;
#endif

#ifdef PCSX2_DEBUG
	IConsoleWriter	DbgConWriter= _DefaultWriter_;
#endif

