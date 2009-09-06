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

#include "Dependencies.h"

extern void DevAssert( bool condition, const char* msg );

namespace Exception
{
	//////////////////////////////////////////////////////////////////////////////////
	// std::exception sucks, and isn't entirely cross-platform reliable in its implementation,
	// so I made a replacement.  The internal messages are non-const, which means that a
	// catch clause can optionally modify them and then re-throw to a top-level handler.
	//
	// Note, this class is "abstract" which means you shouldn't use it directly like, ever.
	// Use Exception::RuntimeError or Exception::LogicError instead for generic exceptions.
	//
	// Because exceptions are the (only!) really useful example of multiple inheritance,
	// this class has only a trivial constructor, and must be manually initialized using
	// InitBaseEx() or by individual member assignments.  This is because C++ multiple inheritence
	// is, by design, a lot of fail, especially when class initializers are mixed in.
	//
	// [TODO] : Add an InnerException component, and Clone() facility.
	//
	class BaseException
	{
	protected:
		wxString m_message_diag;		// (untranslated) a "detailed" message of what disastrous thing has occurred!
		wxString m_message_user;		// (translated) a "detailed" message of what disastrous thing has occurred!
		wxString m_stacktrace;			// contains the stack trace string dump (unimplemented)

	public:
		virtual ~BaseException() throw()=0;	// the =0; syntax forces this class into "abstract" mode.

		/*
		// copy construct
		BaseException( const BaseException& src ) : 
			m_message_diag( src.m_message_diag ),
			m_message_user( src.m_message_user ),
			m_stacktrace( src.m_stacktrace )
		{ }

		// trivial constructor, to appease the C++ multiple virtual inheritence gods. (CMVIGs!)
		BaseException() {}*/

		const wxString& DiagMsg() const { return m_message_diag; }
		const wxString& UserMsg() const { return m_message_user; }

		wxString& DiagMsg() { return m_message_diag; }
		wxString& UserMsg() { return m_message_user; }

		// Returns a message suitable for diagnostic / logging purposes.
		// This message is always in English, and includes a full stack trace.
		virtual wxString FormatDiagnosticMessage() const;

		// Returns a message suitable for end-user display.
		// This message is usually meant for display in a user popup or such.
		virtual wxString FormatDisplayMessage() const { return m_message_user; }

	protected:
		// Construction using two pre-formatted pre-translated messages
		void InitBaseEx( const wxString& msg_eng, const wxString& msg_xlt );

		// Construction using one translation key.
		void InitBaseEx( const char* msg_eng );
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////
	// Ps2Generic
	// This class is used as a base exception for things tossed by PS2 cpus (EE, IOP, etc).
	//
	// Implementation note: does not derive from BaseException, so that we can use different
	// catch block hierarchies to handle them (if needed).
	//
	// Translation Note: Currently these exceptions are never translated.  English/diagnostic
	// format only. :)
	//
	class Ps2Generic
	{
	protected:
		wxString m_message;			// a "detailed" message of what disastrous thing has occurred!

	public:
		virtual ~Ps2Generic() throw() {}

		virtual u32 GetPc() const=0;
		virtual bool IsDelaySlot() const=0;
		virtual wxString Message() const { return m_message; }
	};

// Some helper macros for defining the standard constructors of internationalized constructors
// Parameters:
//  classname - Yeah, the name of this class being defined. :)
//
//  defmsg - default message (in english), which will be used for both english and i18n messages.
//     The text string will be passed through the translator, so if it's int he gettext database
//     it will be optionally translated.
//
#define DEFINE_EXCEPTION_COPYTORS( classname ) \
	virtual ~classname() throw() {}

#define DEFINE_RUNTIME_EXCEPTION( classname, defmsg ) \
	DEFINE_EXCEPTION_COPYTORS( classname ) \
 \
	explicit classname( const char* msg=defmsg )							{ BaseException::InitBaseEx( msg ); } \
	explicit classname( const wxString& msg_eng, const wxString& msg_xlt )	{ BaseException::InitBaseEx( msg_eng, msg_xlt); }

#define DEFINE_LOGIC_EXCEPTION( classname, defmsg ) \
	DEFINE_EXCEPTION_COPYTORS( classname ) \
 \
	explicit classname( const char* msg=defmsg )		{ BaseException::InitBaseEx( msg ); } \
	explicit classname( const wxString& msg_eng )		{ BaseException::InitBaseEx( msg_eng, wxEmptyString ); }

	// ---------------------------------------------------------------------------------------
	// Generalized Exceptions: RuntimeError / LogicError / AssertionFailure
	// ---------------------------------------------------------------------------------------

	class RuntimeError : public virtual BaseException
	{
	public:
		DEFINE_RUNTIME_EXCEPTION( RuntimeError, wxLt("An unhandled runtime error has occurred, somewhere in the depths of Pcsx2's cluttered brain-matter.") )
	};

	// LogicErrors do not need translated versions, since they are typically obscure, and the
	// user wouldn't benefit from being able to understand them anyway. :)
	class LogicError : public virtual BaseException
	{
	public:
		DEFINE_LOGIC_EXCEPTION( LogicError, wxLt("An unhandled logic error has occurred.") )
	};

	// ---------------------------------------------------------------------------------------
	// OutOfMemory / InvalidOperation / InvalidArgument / IndexBoundsFault / ParseError
	// ---------------------------------------------------------------------------------------

	class OutOfMemory : public virtual RuntimeError
	{
	public:
		DEFINE_RUNTIME_EXCEPTION( OutOfMemory, wxLt("Out of Memory") )
	};

	// This exception thrown any time an operation is attempted when an object
	// is in an uninitialized state.
	//
	class InvalidOperation : public virtual LogicError
	{
	public:
		DEFINE_LOGIC_EXCEPTION( InvalidOperation, "Attempted method call is invalid for the current object or program state." )
	};

	// This exception thrown any time an operation is attempted when an object
	// is in an uninitialized state.
	//
	class InvalidArgument : public virtual LogicError
	{
	public:
		DEFINE_LOGIC_EXCEPTION( InvalidArgument, "Invalid argument passed to a function." )
	};

	// Keep those array indexers in bounds when using the SafeArray type, or you'll be
	// seeing these.
	//
	class IndexBoundsFault : public virtual LogicError
	{
	public:
		wxString ArrayName;
		int ArrayLength;
		int BadIndex;

	public:
		DEFINE_EXCEPTION_COPYTORS( IndexBoundsFault )
		
		IndexBoundsFault( const wxString& objname, int index, int arrsize )
		{
			BaseException::InitBaseEx( "Index is outside the bounds of an array." );

			ArrayName	= objname;
			ArrayLength	= arrsize;
			BadIndex	= index;
		}

		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;
	};
	
	class ParseError : public RuntimeError
	{
	public:
		DEFINE_RUNTIME_EXCEPTION( ParseError, "Parse error" );
	};	

	// ---------------------------------------------------------------------------------------
	// Hardware/OS Exceptions:
	//   HardwareDeficiency / CpuStateShutdown / PluginFailure / ThreadCreationError
	// ---------------------------------------------------------------------------------------

	class HardwareDeficiency : public virtual RuntimeError
	{
	public:
		DEFINE_RUNTIME_EXCEPTION( HardwareDeficiency, wxLt("Your machine's hardware is incapable of running Pcsx2.  Sorry dood.") );
	};

	class ThreadCreationError : public virtual RuntimeError
	{
	public:
		DEFINE_RUNTIME_EXCEPTION( ThreadCreationError, wxLt("Thread could not be created.") );
	};

	// ---------------------------------------------------------------------------------------
	// Streaming (file) Exceptions:
	//   Stream / BadStream / CreateStream / FileNotFound / AccessDenied / EndOfStream
	// ---------------------------------------------------------------------------------------

#define DEFINE_STREAM_EXCEPTION( classname, defmsg ) \
	DEFINE_EXCEPTION_COPYTORS( classname ) \
 \
	explicit classname( const wxString& objname=wxString(), const char* msg=defmsg ) \
	{ \
		BaseException::InitBaseEx( msg ); \
		StreamName = objname; \
	} \
	explicit classname( const wxString& objname, const wxString& msg_eng, const wxString& msg_xlt ) \
	{ \
		BaseException::InitBaseEx( msg_eng, msg_xlt ); \
		StreamName = objname; \
	} \
	explicit classname( const char* objname, const char* msg=defmsg ) \
	{ \
		BaseException::InitBaseEx( msg ); \
		StreamName = wxString::FromUTF8( objname ); \
	} \
	explicit classname( const char* objname, const wxString& msg_eng, const wxString& msg_xlt ) \
	{ \
		BaseException::InitBaseEx( msg_eng, msg_xlt ); \
		StreamName = wxString::FromUTF8( objname ); \
	} \
	explicit classname( const char* objname, const wxString& msg_eng ) \
	{ \
		BaseException::InitBaseEx( msg_eng, msg_eng ); \
		StreamName = wxString::FromUTF8( objname ); \
	} \
	explicit classname( const wxString& objname, const wxString& msg_eng ) \
	{ \
		BaseException::InitBaseEx( msg_eng, msg_eng ); \
		StreamName = objname; \
	}

	// Generic stream error.  Contains the name of the stream and a message.
	// This exception is usually thrown via derived classes, except in the (rare) case of a
	// generic / unknown error.
	//
	class Stream : public virtual RuntimeError
	{
	public:
		wxString StreamName;		// name of the stream (if applicable)

	public:
		DEFINE_STREAM_EXCEPTION( Stream, "General file operation error." )

		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;
	};

	// A generic base error class for bad streams -- corrupted data, sudden closures, loss of
	// connection, or anything else that would indicate a failure to read the data after the
	// stream was successfully opened.
	//
	class BadStream : public virtual Stream
	{
	public:
		DEFINE_STREAM_EXCEPTION( BadStream, wxLt("File data is corrupted or incomplete, or the stream connection closed unexpectedly") )
	};

	// A generic exception for odd-ball stream creation errors.
	//
	class CreateStream : public virtual Stream
	{
	public:
		DEFINE_STREAM_EXCEPTION( CreateStream, wxLt("File could not be created or opened") )
	};

	// Exception thrown when an attempt to open a non-existent file is made.
	// (this exception can also mean file permissions are invalid)
	//
	class FileNotFound : public virtual CreateStream
	{
	public:
		DEFINE_STREAM_EXCEPTION( FileNotFound, wxLt("File not found") )
	};

	class AccessDenied : public virtual CreateStream
	{
	public:
		DEFINE_STREAM_EXCEPTION( AccessDenied, wxLt("Permission denied to file") )
	};

	// EndOfStream can be used either as an error, or used just as a shortcut for manual
	// feof checks.
	//
	class EndOfStream : public virtual Stream
	{
	public:
		DEFINE_STREAM_EXCEPTION( EndOfStream, wxLt("Unexpected end of file") );
	};

	// ---------------------------------------------------------------------------------------
	// Savestate Exceptions:
	//   BadSavedState / FreezePluginFailure / StateLoadError / UnsupportedStateVersion /
	//   StateCrcMismatch
	// ---------------------------------------------------------------------------------------

	// Exception thrown when a corrupted or truncated savestate is encountered.
	//
	class BadSavedState : public virtual BadStream
	{
	public:
		DEFINE_STREAM_EXCEPTION( BadSavedState, wxLt("Savestate data is corrupted or incomplete") )
	};

	// Exception thrown by SaveState class when a plugin returns an error during state
	// load or save. 
	//
	class FreezePluginFailure : public virtual RuntimeError
	{
	public:
		wxString PluginName;		// name of the plugin
		wxString FreezeAction;

	public:
		DEFINE_EXCEPTION_COPYTORS( FreezePluginFailure )

		explicit FreezePluginFailure( const char* plugin, const char* action )
		{
			PluginName = wxString::FromUTF8( plugin );
			FreezeAction = wxString::FromUTF8( action );
		}

		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;
	};

	// thrown when the savestate being loaded isn't supported.
	//
	class UnsupportedStateVersion : public virtual BadSavedState
	{
	public:
		u32 Version;		// version number of the unsupported state.

	public:
		DEFINE_EXCEPTION_COPYTORS( UnsupportedStateVersion )
		
		explicit UnsupportedStateVersion( int version, const wxString& objname=wxEmptyString )
		{
			StreamName = objname;
			Version = version;
		}

		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;
	};

	// A recoverable exception thrown when the CRC of the savestate does not match the
	// CRC returned by the Cdvd driver.
	// [feature not implemented yet]
	//
	class StateCrcMismatch : public virtual BadSavedState
	{
	public:
		u32 Crc_Savestate;
		u32 Crc_Cdvd;

	public:
		DEFINE_EXCEPTION_COPYTORS( StateCrcMismatch )

		explicit StateCrcMismatch( u32 crc_save, u32 crc_cdvd, const wxString& objname=wxEmptyString )
		{
			StreamName		= objname;
			Crc_Savestate	= crc_save;
			Crc_Cdvd		= crc_cdvd;
		}

		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;
	};
}
