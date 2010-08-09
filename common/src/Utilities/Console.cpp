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

#include "PrecompiledHeader.h"
#include "Threading.h"
#include "TraceLog.h"
#include "TlsVariable.inl"

#include "RedtapeWindows.h"		// nneded for OutputDebugString

using namespace Threading;

// thread-local console indentation setting.
static DeclareTls(int) conlog_Indent( 0 );

// thread-local console color storage.
static DeclareTls(ConsoleColors) conlog_Color( DefaultConsoleColor );


static wxString	m_buffer;		// used by ConsoleBuffer
static Mutex	m_bufferlock;	// used by ConsoleBuffer

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
		(writer.WriteRaw != NULL)	&& (writer.DoWriteLn != NULL) &&
		(writer.Newline != NULL)	&& (writer.SetTitle != NULL) &&
		(writer.DoSetColor != NULL),
		"Invalid IConsoleWriter object!  All function pointer interfaces must be implemented."
	);

	if( &writer != &ConsoleWriter_Buffered )
	{
		ScopedLock lock( m_bufferlock );
		if( !ConsoleBuffer_Get().IsEmpty() )
			writer.DoWriteLn( ConsoleBuffer_Get() );
	}

	const_cast<IConsoleWriter&>(Console)= writer;

#ifdef PCSX2_DEVBUILD
	const_cast<IConsoleWriter&>(DevCon)	= writer;
#endif

#ifdef PCSX2_DEBUG
	const_cast<IConsoleWriter&>(DbgCon)	= writer;
#endif
}

// Writes text to the Visual Studio Output window (Microsoft Windows only).
// On all other platforms this pipes to Stdout instead.
void MSW_OutputDebugString( const wxString& text )
{
#if defined(__WXMSW__) && !defined(__WXMICROWIN__)
	static bool hasDebugger = wxIsDebuggerRunning();
	if( hasDebugger ) OutputDebugString( text );
#else
	// send them to stderr
	wxPrintf(L"%s\n", text.c_str());
	fflush(stderr);
#endif
}

// --------------------------------------------------------------------------------------
//  ConsoleNull
// --------------------------------------------------------------------------------------

static void __concall ConsoleNull_SetTitle( const wxString& title ) {}
static void __concall ConsoleNull_DoSetColor( ConsoleColors color ) {}
static void __concall ConsoleNull_Newline() {}
static void __concall ConsoleNull_DoWrite( const wxString& fmt ) {}
static void __concall ConsoleNull_DoWriteLn( const wxString& fmt ) {}

const IConsoleWriter ConsoleWriter_Null =
{
	ConsoleNull_DoWrite,
	ConsoleNull_DoWriteLn,
	ConsoleNull_DoSetColor,

	ConsoleNull_DoWrite,
	ConsoleNull_Newline,
	ConsoleNull_SetTitle,

	0,		// instance-level indentation (should always be 0)
};

// --------------------------------------------------------------------------------------
//  Console_Stdout
// --------------------------------------------------------------------------------------

#ifdef __LINUX__
static __fi const wxChar* GetLinuxConsoleColor(ConsoleColors color)
{
    switch(color)
    {
        case Color_Black: return L"\033[30m";
        case Color_StrongBlack: return L"\033[30m\033[1m";

        case Color_Red: return L"\033[31m";
        case Color_StrongRed: return L"\033[31m\033[1m";

        case Color_Green: return L"\033[32m";
        case Color_StrongGreen: return L"\033[32m\033[1m";

        case Color_Yellow: return L"\033[33m";
        case Color_StrongYellow: return L"\033[33m\033[1m";

        case Color_Blue: return L"\033[34m";
        case Color_StrongBlue: return L"\033[34m\033[1m";

        // No orange, so use magenta.
        case Color_Orange:
        case Color_Magenta: return L"\033[35m";
        case Color_StrongOrange:
        case Color_StrongMagenta: return L"\033[35m\033[1m";

        case Color_Cyan: return L"\033[36m";
        case Color_StrongCyan: return L"\033[36m\033[1m";

        // Use 'white' instead of grey.
        case Color_Gray:
        case Color_White: return L"\033[37m";
        case Color_StrongGray:
        case Color_StrongWhite: return L"\033[37m\033[1m";

        // On some other value being passed, clear any formatting.
        case Color_Default:
        default: return L"\033[0m";
    }
}
#endif

// One possible default write action at startup and shutdown is to use the stdout.
static void __concall ConsoleStdout_DoWrite( const wxString& fmt )
{
	MSW_OutputDebugString( fmt );	
}

// Default write action at startup and shutdown is to use the stdout.
static void __concall ConsoleStdout_DoWriteLn( const wxString& fmt )
{
	MSW_OutputDebugString( fmt + L"\n" );
}

static void __concall ConsoleStdout_Newline()
{
	MSW_OutputDebugString( L"\n" );
}

static void __concall ConsoleStdout_DoSetColor( ConsoleColors color )
{
#ifdef __LINUX__
	wxPrintf(L"\033[0m");
    wxPrintf(GetLinuxConsoleColor(color));
#endif
}

static void __concall ConsoleStdout_SetTitle( const wxString& title )
{
#ifdef __LINUX__
	wxPrintf(L"\033]0;%s\007", title.c_str());
#endif
}

const IConsoleWriter ConsoleWriter_Stdout =
{
	ConsoleStdout_DoWrite,			// Writes without newlines go to buffer to avoid error log spam.
	ConsoleStdout_DoWriteLn,
	ConsoleStdout_DoSetColor,

	ConsoleNull_DoWrite,			// writes from re-piped stdout are ignored here, lest we create infinite loop hell >_<
	ConsoleStdout_Newline,
	ConsoleStdout_SetTitle,
	0,		// instance-level indentation (should always be 0)
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
	ConsoleNull_DoSetColor,

	ConsoleNull_DoWrite,
	ConsoleNull_Newline,
	ConsoleNull_SetTitle,

	0,		// instance-level indentation (should always be 0)
};

// --------------------------------------------------------------------------------------
//  ConsoleBuffer
// --------------------------------------------------------------------------------------

const wxString& ConsoleBuffer_Get()
{
	return m_buffer;
}

void ConsoleBuffer_Clear()
{
	ScopedLock lock( m_bufferlock );
	m_buffer.clear();
}

// Flushes the contents of the ConsoleBuffer to the specified destination file stream, and
// clears the buffer contents to 0.
void ConsoleBuffer_FlushToFile( FILE *fp )
{
	ScopedLock lock( m_bufferlock );
	if( fp == NULL || m_buffer.IsEmpty() ) return;
	px_fputs( fp, m_buffer.ToUTF8() );
	m_buffer.clear();
}

static void __concall ConsoleBuffer_DoWrite( const wxString& fmt )
{
	ScopedLock lock( m_bufferlock );
	m_buffer += fmt;
}

static void __concall ConsoleBuffer_DoWriteLn( const wxString& fmt )
{
	ScopedLock lock( m_bufferlock );
	m_buffer += fmt + L"\n";
}

const IConsoleWriter ConsoleWriter_Buffered =
{
	ConsoleBuffer_DoWrite,			// Writes without newlines go to buffer to avoid assertion spam.
	ConsoleBuffer_DoWriteLn,
	ConsoleNull_DoSetColor,

	ConsoleBuffer_DoWrite,
	ConsoleNull_Newline,
	ConsoleNull_SetTitle,

	0,		// instance-level indentation (should always be 0)
};

// --------------------------------------------------------------------------------------
//  Console_wxLogError
// --------------------------------------------------------------------------------------

static void __concall Console_wxLogError_DoWriteLn( const wxString& fmt )
{
	if( !m_buffer.IsEmpty() )
	{
		wxLogError( m_buffer );
		m_buffer.clear();
	}
	wxLogError( fmt );
}

const IConsoleWriter ConsoleWriter_wxError =
{
	ConsoleBuffer_DoWrite,			// Writes without newlines go to buffer to avoid error log spam.
	Console_wxLogError_DoWriteLn,
	ConsoleNull_DoSetColor,

	ConsoleBuffer_DoWrite,
	ConsoleNull_Newline,
	ConsoleNull_SetTitle,

	0,		// instance-level indentation (should always be 0)
};

// =====================================================================================================
//  IConsoleWriter Implementations
// =====================================================================================================
// (all non-virtual members that do common work and then pass the result through DoWrite
//  or DoWriteLn)

// Parameters:
//   glob_indent - this parameter is used to specify a global indentation setting.  It is used by
//      WriteLn function, but defaults to 0 for Warning and Error calls.  Local indentation always
//      applies to all writes.
wxString IConsoleWriter::_addIndentation( const wxString& src, int glob_indent=0 ) const
{
	const int indent = glob_indent + _imm_indentation;
	if( indent == 0 ) return src;

	wxString result( src );
	const wxString indentStr( L'\t', indent );
	result.Replace( L"\n", L"\n" + indentStr );
	return indentStr + result;
}

// Sets the indentation to be applied to all WriteLn's.  The indentation is added to the
// primary write, and to any newlines specified within the write.  Note that this applies
// to calls to WriteLn *only* -- calls to Write bypass the indentation parser.
const IConsoleWriter& IConsoleWriter::SetIndent( int tabcount ) const
{
	conlog_Indent += tabcount;
	pxAssert( conlog_Indent >= 0 );
	return *this;
}

IConsoleWriter IConsoleWriter::Indent( int tabcount ) const
{
	IConsoleWriter retval = *this;
	retval._imm_indentation = tabcount;
	return retval;
}

// Changes the active console color.
// This color will be unset by calls to colored text methods
// such as ErrorMsg and Notice.
const IConsoleWriter& IConsoleWriter::SetColor( ConsoleColors color ) const
{
	// Ignore current color requests since, well, the current color is already set. ;)
	if( color == Color_Current ) return *this;

	pxAssertMsg( (color > Color_Current) && (color < ConsoleColors_Count), "Invalid ConsoleColor specified." );

	if( conlog_Color != color )
		DoSetColor( conlog_Color = color );

	return *this;
}

ConsoleColors IConsoleWriter::GetColor() const
{
	return conlog_Color;
}

// Restores the console color to default (usually black, or low-intensity white if the console uses a black background)
const IConsoleWriter& IConsoleWriter::ClearColor() const
{
	if( conlog_Color != DefaultConsoleColor )
		DoSetColor( conlog_Color = DefaultConsoleColor );

	return *this;
}

// --------------------------------------------------------------------------------------
//  ASCII/UTF8 (char*)
// --------------------------------------------------------------------------------------

bool IConsoleWriter::FormatV( const char* fmt, va_list args ) const
{
	DoWriteLn( _addIndentation( pxsFmtV(fmt,args), conlog_Indent ) );
	return false;
}

bool IConsoleWriter::WriteLn( const char* fmt, ... ) const
{
	va_list args;
	va_start(args,fmt);
	FormatV(fmt, args);
	va_end(args);

	return false;
}

bool IConsoleWriter::WriteLn( ConsoleColors color, const char* fmt, ... ) const
{
	va_list args;
	va_start(args,fmt);
	ConsoleColorScope cs( color );
	FormatV(fmt, args);
	va_end(args);

	return false;
}

bool IConsoleWriter::Error( const char* fmt, ... ) const
{
	va_list args;
	va_start(args,fmt);
	ConsoleColorScope cs( Color_StrongRed );
	FormatV(fmt, args);
	va_end(args);

	return false;
}

bool IConsoleWriter::Warning( const char* fmt, ... ) const
{
	va_list args;
	va_start(args,fmt);
	ConsoleColorScope cs( Color_StrongOrange );
	FormatV(fmt, args);
	va_end(args);

	return false;
}

// --------------------------------------------------------------------------------------
//  Write Variants - Unicode/UTF16 style
// --------------------------------------------------------------------------------------

bool IConsoleWriter::FormatV( const wxChar* fmt, va_list args ) const
{
	DoWriteLn( _addIndentation( pxsFmtV( fmt, args ), conlog_Indent ) );
	return false;
}

bool IConsoleWriter::WriteLn( const wxChar* fmt, ... ) const
{
	va_list args;
	va_start(args,fmt);
	FormatV(fmt,args);
	va_end(args);

	return false;
}

bool IConsoleWriter::WriteLn( ConsoleColors color, const wxChar* fmt, ... ) const
{
	va_list args;
	va_start(args,fmt);
	ConsoleColorScope cs( color );
	FormatV(fmt,args);
	va_end(args);

	return false;
}

bool IConsoleWriter::Error( const wxChar* fmt, ... ) const
{
	va_list args;
	va_start(args,fmt);
	ConsoleColorScope cs( Color_StrongRed );
	FormatV(fmt,args);
	va_end(args);

	return false;
}

bool IConsoleWriter::Warning( const wxChar* fmt, ... ) const
{
	va_list args;
	va_start(args,fmt);
	ConsoleColorScope cs( Color_StrongOrange );
	FormatV(fmt,args);
	va_end(args);

	return false;
}

// --------------------------------------------------------------------------------------
//  ConsoleColorScope / ConsoleIndentScope
// --------------------------------------------------------------------------------------

ConsoleColorScope::ConsoleColorScope( ConsoleColors newcolor )
{
	m_IsScoped = false;
	m_newcolor = newcolor;
	EnterScope();
}

ConsoleColorScope::~ConsoleColorScope() throw()
{
	LeaveScope();
}

void ConsoleColorScope::EnterScope()
{
	if (!m_IsScoped)
	{
		m_old_color = Console.GetColor();
		Console.SetColor( m_newcolor );
		m_IsScoped = true;
	}
}

void ConsoleColorScope::LeaveScope()
{
	m_IsScoped = m_IsScoped && (Console.SetColor( m_old_color ), false);
}

ConsoleIndentScope::ConsoleIndentScope( int tabs )
{
	m_IsScoped = false;
	m_amount = tabs;
	EnterScope();
}

ConsoleIndentScope::~ConsoleIndentScope() throw()
{
	LeaveScope();
}

void ConsoleIndentScope::EnterScope()
{
	m_IsScoped = m_IsScoped || (Console.SetIndent( m_amount ),true);
}

void ConsoleIndentScope::LeaveScope()
{
	m_IsScoped = m_IsScoped && (Console.SetIndent( -m_amount ),false);
}


ConsoleAttrScope::ConsoleAttrScope( ConsoleColors newcolor, int indent )
{
	m_old_color = Console.GetColor();
	Console.SetIndent( m_tabsize = indent );
	Console.SetColor( newcolor );
}

ConsoleAttrScope::~ConsoleAttrScope() throw()
{
	Console.SetColor( m_old_color );
	Console.SetIndent( -m_tabsize );
}


// --------------------------------------------------------------------------------------
//  Default Writer for C++ init / startup:
// --------------------------------------------------------------------------------------
// Currently all build types default to Stdout, which is very functional on Linux but not
// always so useful on Windows (which itself lacks a proper stdout console without using
// platform specific code).  Under windows Stdout will attempt to write to the IDE Debug
// console, if one is available (such as running pcsx2 via MSVC).  If not available, then
// the log message will pretty much be lost into the ether.
// 
#define _DefaultWriter_	ConsoleWriter_Stdout

const IConsoleWriter	Console				= _DefaultWriter_;
const IConsoleWriter	DevConWriter		= _DefaultWriter_;
bool					DevConWriterEnabled	= false;

#ifdef PCSX2_DEBUG
const IConsoleWriter	DbgConWriter		= _DefaultWriter_;
#endif

const NullConsoleWriter	NullCon = {};


// --------------------------------------------------------------------------------------
//  BaseTraceLogAttr
// --------------------------------------------------------------------------------------

// Returns the translated description of this trace log!
const wxChar* BaseTraceLogAttr::GetDescription() const
{
	return (Description!=NULL) ? pxGetTranslation(Description) : wxEmptyString;
}

// --------------------------------------------------------------------------------------
//  ConsoleLogSource
// --------------------------------------------------------------------------------------

// Writes to the console using the specified color.  This overrides the default color setting
// for this log.
bool ConsoleLogSource::WriteV( ConsoleColors color, const char *fmt, va_list list ) const
{
	ConsoleColorScope cs(color);
	DoWrite( pxsFmtV(fmt,list).GetResult() );
	return false;
}

bool ConsoleLogSource::WriteV( ConsoleColors color, const wxChar *fmt, va_list list ) const
{
	ConsoleColorScope cs(color);
	DoWrite( pxsFmtV(fmt,list) );
	return false;
}

// Writes to the console using the source's default color.  Note that the source's default
// color will always be used, thus ConsoleColorScope() will not be effectual unless the
// console's default color is Color_Default.
bool ConsoleLogSource::WriteV( const char *fmt, va_list list ) const
{
	WriteV( DefaultColor, fmt, list );
	return false;
}

bool ConsoleLogSource::WriteV( const wxChar *fmt, va_list list ) const
{
	WriteV( DefaultColor, fmt, list );
	return false;
}

ConsoleLogSource::ConsoleLogSource()
{
	// SourceLogs are usually pretty heavy spam, so default to something
	// reasonably non-intrusive:
	DefaultColor	= Color_Gray;
	//pxConLogSources_AllKnown.Add(this);
}

ConsoleLogSource_Threading pxConLog_Thread;
