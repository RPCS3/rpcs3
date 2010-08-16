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

// These macros are currently not needed (anymore), but might be needed aain in the future --air
#define TraceLog_ImplementBaseAPI(thistype)
#define ConsoleLog_ImplementBaseAPI(thistype)

// --------------------------------------------------------------------------------------
//  TraceLogDescriptor
// --------------------------------------------------------------------------------------
// Provides textual information for use by UIs; to give the end user a selection screen for
// enabling/disabling logs, and also for saving the log settings to INI.
//
struct TraceLogDescriptor
{
	// short name, alphanumerics only: used for saving/loading options.
	const wxChar*	ShortName;

	// Standard UI name for this log source.  Used in menus, options dialogs.
	const wxChar*	Name;

	// Length description for use as a tooltip or menu item description.
	const wxChar*	Description;

	wxString GetShortName() const
	{
		pxAssumeDev(Name, "Tracelog descriptors require a valid name!");
		return ShortName ? ShortName : Name;
	}
};

// --------------------------------------------------------------------------------------
//  BaseTraceLogSource
// --------------------------------------------------------------------------------------
// This class houses the base attributes for any trace log (to file or to console), which
// only includes the logfile's name, description, and enabled bit (for UIs and ini files),
// and an IsActive() method for determining if the log should be written or not.
//
// Derived classes then provide their own Write/DoWrite functions that format and write
// the log to the intended target(s).
//
// All individual calls to log write functions should be responsible for checking the
// status of the log via the IsActive() method manually (typically done via macro).  This
// is done in favor of internal checks because most logs include detailed/formatted
// information, which itself can take a lot of cpu power to prepare.  If the IsActive()
// check is done top-level, the parameters' calculations can be skipped.  If the IsActive()
// check is done internally as part of the Write/Format calls, all parameters have to be
// resolved regardless of if the log is actually active.
// 
class BaseTraceLogSource
{
protected:
	const TraceLogDescriptor* m_Descriptor;

public:
	// Indicates if the user has enabled this specific log.  This boolean only represents
	// the configured status of this log, and does *NOT* actually mean the log is active
	// even when TRUE.  Because many tracelogs have master enablers that act on a group
	// of logs, logging checks should always use IsActive() instead to determine if a log
	// should be processed or not.
	bool			Enabled;

protected:
	BaseTraceLogSource() {}

public:
	TraceLog_ImplementBaseAPI(BaseTraceLogSource)

	BaseTraceLogSource( const TraceLogDescriptor* desc )
	{
		pxAssumeDev( desc, "Trace logs must have a valid (non-NULL) descriptor." );
		Enabled = false;
		m_Descriptor = desc;
	}

	// Provides a categorical identifier, typically in "group.subgroup.subgroup" form.
	// (use periods in favor of colons, since they do not require escape characters when
	// written to ini/config files).
	virtual wxString GetCategory() const	{ return wxEmptyString; }

	// This method should be used to determine if a log should be generated or not.
	// See the class overview comments for details on how and why this method should
	// be used.
	virtual bool IsActive() const			{ return Enabled; }

	virtual wxString GetShortName() const	{ return m_Descriptor->GetShortName(); }
	virtual const wxChar* GetName() const	{ return m_Descriptor->Name; }
	virtual const wxChar* GetDescription() const
	{
		return (m_Descriptor->Description!=NULL) ? pxGetTranslation(m_Descriptor->Description) : wxEmptyString;
	}

	virtual bool HasDescription() const		{ return m_Descriptor->Description != NULL;}

};

// --------------------------------------------------------------------------------------
//  TextFileTraceLog
// --------------------------------------------------------------------------------------
// This class is tailored for performance logging to file.  It does not support console
// colors or wide/unicode text conversion.
//
class TextFileTraceLog : public BaseTraceLogSource
{
public:
	TextFileTraceLog( const TraceLogDescriptor* desc )
		: BaseTraceLogSource( desc )
	{	
	}

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
class ConsoleLogSource : public BaseTraceLogSource
{
public:
	ConsoleColors	DefaultColor;

protected:
	ConsoleLogSource() {}

public:
	ConsoleLog_ImplementBaseAPI(ConsoleLogSource)

	ConsoleLogSource( const TraceLogDescriptor* desc, ConsoleColors defaultColor = Color_Gray )
		: BaseTraceLogSource(desc)
	{
		DefaultColor	= defaultColor;
	}

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

