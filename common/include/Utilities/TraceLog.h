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

#pragma once

#include "Console.h"

#define TraceLog_ImplementBaseAPI(thistype) \
	thistype& SetDescription( const wxChar* desc ) { \
		Description = desc; \
		return *this; \
	} \
 	thistype& SetName( const wxChar* name ) { \
		Name = name; \
		return *this; \
	} \
 	thistype& SetShortName( const wxChar* name ) { \
		ShortName = name; \
		return *this; \
	}

#define ConsoleLog_ImplementBaseAPI(thistype) \
	TraceLog_ImplementBaseAPI(thistype) \
 	thistype& SetColor( ConsoleColors color ) { \
		DefaultColor = color; \
		return *this; \
	}

// --------------------------------------------------------------------------------------
//  BaseTraceLog
// --------------------------------------------------------------------------------------
class BaseTraceLogAttr
{
public:
	bool			Enabled;

	// short name, alphanumerics only: used for saving/loading options.
	const wxChar*	ShortName;

	// Standard UI name for this log source.  Used in menus, options dialogs.
	const wxChar*	Name;
	const wxChar*	Description;

public:
	TraceLog_ImplementBaseAPI(BaseTraceLogAttr)

	BaseTraceLogAttr()
	{
		Enabled = false;
		ShortName = NULL;
	}

	virtual wxString GetShortName() const	{ return ShortName ? ShortName : Name; }
	virtual wxString GetCategory() const	{ return wxEmptyString; }
	virtual bool IsEnabled() const			{ return Enabled; }

	const wxChar* GetDescription() const;
};

// --------------------------------------------------------------------------------------
//  TextFileTraceLog
// --------------------------------------------------------------------------------------
// This class is tailored for performance logging to file.  It does not support console
// colors or wide/unicode text conversion.
//
class TextFileTraceLog : public BaseTraceLogAttr
{
protected:

public:
	bool Write( const char* fmt, ... ) const
	{
		va_list list;
		va_start( list, fmt );
		WriteV( fmt, list );
		va_end( list );

		return false;
	}

	bool WriteV( const char *fmt, va_list list ) const
	{
		FastFormatAscii ascii;
		ApplyPrefix(ascii);
		ascii.WriteV( fmt, list );
		DoWrite( ascii );
		return false;
	}

	virtual void ApplyPrefix( FastFormatAscii& ascii ) const {}
	virtual void DoWrite( const char* fmt ) const=0;
};


// --------------------------------------------------------------------------------------
//  ConsoleLogSource
// --------------------------------------------------------------------------------------
// This class is tailored for logging to console.  It applies default console color attributes
// to all writes, and supports both char and wxChar (Ascii and UF8/16) formatting.
//
class ConsoleLogSource : public BaseTraceLogAttr
{
public:
	ConsoleColors	DefaultColor;

public:
	ConsoleLog_ImplementBaseAPI(ConsoleLogSource)

	ConsoleLogSource();

	// Writes to the console using the source's default color.  Note that the source's default
	// color will always be used, thus ConsoleColorScope() will not be effectual unless the
	// console's default color is Color_Default.
	bool Write( const char* fmt, ... ) const
	{
		va_list list;
		va_start( list, fmt );
		WriteV( fmt, list );
		va_end( list );

		return false;
	}

	bool Write( const wxChar* fmt, ... ) const
	{
		va_list list;
		va_start( list, fmt );
		WriteV( fmt, list );
		va_end( list );

		return false;
	}

	// Writes to the console using the specified color.  This overrides the default color setting
	// for this log.
	bool Write( ConsoleColors color, const char* fmt, ... ) const
	{
		va_list list;
		va_start( list, fmt );
		WriteV( color, fmt, list );
		va_end( list );

		return false;
	}

	bool Write( ConsoleColors color, const wxChar* fmt, ... ) const
	{
		va_list list;
		va_start( list, fmt );
		WriteV( color, fmt, list );
		va_end( list );

		return false;
	}

	// Writes to the console using bold yellow text -- overrides the log source's default
	// color settings.
	bool Warn( const wxChar* fmt, ... ) const
	{
		va_list list;
		va_start( list, fmt );
		WriteV( Color_StrongYellow, fmt, list );
		va_end( list );

		return false;
	}

	// Writes to the console using bold red text -- overrides the log source's default
	// color settings.
	bool Error( const wxChar* fmt, ... ) const
	{
		va_list list;
		va_start( list, fmt );
		WriteV( Color_StrongRed, fmt, list );
		va_end( list );

		return false;
	}

	bool WriteV( const char *fmt, va_list list ) const;
	bool WriteV( const wxChar *fmt, va_list list ) const;

	bool WriteV( ConsoleColors color, const char *fmt, va_list list ) const;
	bool WriteV( ConsoleColors color, const wxChar *fmt, va_list list ) const;

	virtual void DoWrite( const wxChar* msg ) const
	{
		Console.DoWriteLn( msg );
	}
};

#if 0
// --------------------------------------------------------------------------------------
//  pxConsoleLogList
// --------------------------------------------------------------------------------------
struct pxConsoleLogList
{
	uint				count;
	ConsoleLogSource*	source[128];

	void Add( ConsoleLogSource* trace )
	{
		if( !pxAssertDev( count < ArraySize(source),
			wxsFormat( L"Trace log initialization list is already maxed out.  The tracelog for'%s' will not be enumerable.", trace->Name ) )
			) return;

		source[count++] = trace;
	}

	uint GetCount() const
	{
		return count;
	}
	
	ConsoleLogSource& operator[]( uint idx )
	{
		pxAssumeDev( idx < count, "SysTraceLog index is out of bounds." );
		pxAssume( source[idx] != NULL );

		return *source[idx];
	}

	const ConsoleLogSource& operator[]( uint idx ) const
	{
		pxAssumeDev( idx < count, "SysTraceLog index is out of bounds." );
		pxAssume( source[idx] != NULL );

		return *source[idx];
	}
};

extern pxConsoleLogList pxConLogSources_AllKnown;

#endif
