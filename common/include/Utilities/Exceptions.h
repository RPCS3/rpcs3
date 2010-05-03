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

#include "Dependencies.h"
#include "StringHelpers.h"

// --------------------------------------------------------------------------------------
//  DESTRUCTOR_CATCHALL - safe destructor helper
// --------------------------------------------------------------------------------------
// In C++ destructors *really* need to be "nothrow" garaunteed, otherwise you can have
// disasterous nested exception throws during the unwinding process of an originating
// exception.  Use this macro to dispose of these dangerous exceptions, and generate a
// friendly error log in their wake.
//
#define __DESTRUCTOR_CATCHALL( funcname ) \
	catch( BaseException& ex ) \
	{ \
		Console.Error( "Unhandled BaseException in %s (ignored!):", funcname ); \
		Console.Error( ex.FormatDiagnosticMessage() ); \
	} \
	catch( std::exception& ex ) \
	{ \
		Console.Error( "Unhandled std::exception in %s (ignored!):", funcname ); \
		Console.Error( ex.what() ); \
	}

#define DESTRUCTOR_CATCHALL		__DESTRUCTOR_CATCHALL( __pxFUNCTION__ )

namespace Exception
{
	int MakeNewType();

	// --------------------------------------------------------------------------------------
	//  BaseException
	// --------------------------------------------------------------------------------------
	// std::exception sucks, and isn't entirely cross-platform reliable in its implementation,
	// so I made a replacement.  The internal messages are non-const, which means that a
	// catch clause can optionally modify them and then re-throw to a top-level handler.
	//
	// Note, this class is "abstract" which means you shouldn't use it directly like, ever.
	// Use Exception::RuntimeError instead for generic exceptions.
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

		virtual void Rethrow() const=0;
		virtual BaseException* Clone() const=0;

	protected:
		// Construction using two pre-formatted pre-translated messages
		void InitBaseEx( const wxString& msg_eng, const wxString& msg_xlt );

		// Construction using one translation key.
		void InitBaseEx( const char* msg_eng );
	};

	// --------------------------------------------------------------------------------------
	//  Ps2Generic Exception
	// --------------------------------------------------------------------------------------
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
// BUGZ??  I'd rather use 'classname' on the Clone() prototype, but for some reason it generates
// ambiguity errors on virtual inheritence (it really shouldn't!).  So I have to force it to the
// BaseException base class.  Not sure if this is Stupid Standard Tricks or Stupid MSVC Tricks. --air
//
// (update: web searches indicate it's MSVC specific -- happens in 2008, not sure about 2010).
//
#define DEFINE_EXCEPTION_COPYTORS( classname ) \
	virtual ~classname() throw() {} \
	virtual void Rethrow() const { throw *this; } \
	virtual BaseException* Clone() const { return new classname( *this ); }

// This is here because MSVC's support for covariant return types on Clone() is broken, and will
// not work with virtual class inheritance (see DEFINE_EXCEPTION_COPYTORS for details)
#define DEFINE_EXCEPTION_COPYTORS_COVARIANT( classname ) \
	virtual ~classname() throw() {} \
	virtual void Rethrow() const { throw *this; } \
	virtual classname* Clone() const { return new classname( *this ); }

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
	//  RuntimeError - Generalized Exceptions with Recoverable Traits!
	// ---------------------------------------------------------------------------------------

	class RuntimeError : public virtual BaseException
	{
	public:
		bool	IsSilent;
	public:
		DEFINE_RUNTIME_EXCEPTION( RuntimeError, wxLt("An unhandled runtime error has occurred, somewhere in the depths of Pcsx2's cluttered brain-matter.") )

		RuntimeError( const std::runtime_error& ex, const wxString& prefix=wxEmptyString );
	};

	// --------------------------------------------------------------------------------------
	//  CancelAppEvent  -  Exception for canceling an event in a non-verbose fashion
	// --------------------------------------------------------------------------------------
	// Typically the PCSX2 interface issues popup dialogs for runtime errors.  This exception
	// instead issues a "silent" cancelation that is handled by the app gracefully (generates
	// log, and resumes messages queue processing).
	//
	// I chose to have this exception derive from RuntimeError, since if one is thrown from outside
	// an App message loop we'll still want it to be handled in a reasonably graceful manner.
	class CancelEvent : public virtual RuntimeError
	{
	public:
		DEFINE_EXCEPTION_COPYTORS( CancelEvent )

		explicit CancelEvent( const char* logmsg )
		{
			m_message_diag = fromUTF8( logmsg );
			// overridden message formatters only use the diagnostic version...
		}

		explicit CancelEvent( const wxString& logmsg=L"No reason given." )
		{
			m_message_diag = logmsg;
			// overridden message formatters only use the diagnostic version...
		}

		virtual wxString FormatDisplayMessage() const;
		virtual wxString FormatDiagnosticMessage() const;
	};

	// --------------------------------------------------------------------------------------
	class ObjectIsNull : public virtual CancelEvent
	{
	public:
		wxString ObjectName;

		DEFINE_EXCEPTION_COPYTORS( ObjectIsNull )

		explicit ObjectIsNull( const char* objname="unspecified" )
		{
			m_message_diag = fromUTF8( objname );
			// overridden message formatters only use the diagnostic version...
		}

		virtual wxString FormatDisplayMessage() const;
		virtual wxString FormatDiagnosticMessage() const;
	};

	// ---------------------------------------------------------------------------------------
	//  OutOfMemory / InvalidOperation / InvalidArgument / ParseError
	// ---------------------------------------------------------------------------------------

	class OutOfMemory : public virtual RuntimeError
	{
	public:
		DEFINE_RUNTIME_EXCEPTION( OutOfMemory, wxLt("Out of Memory") )
	};

	class ParseError : public RuntimeError
	{
	public:
		DEFINE_RUNTIME_EXCEPTION( ParseError, "Parse error" );
	};

	// ---------------------------------------------------------------------------------------
	// Hardware/OS Exceptions:
	//   HardwareDeficiency / VirtualMemoryMapConflict
	// ---------------------------------------------------------------------------------------

	// This exception is a specific type of OutOfMemory error that isn't "really" an out of
	// memory error.  More likely it's caused by a plugin or driver reserving a range of memory
	// we'd really like to have access to.
	class VirtualMemoryMapConflict : public virtual OutOfMemory
	{
	public:
		DEFINE_RUNTIME_EXCEPTION( VirtualMemoryMapConflict, wxLt("Virtual memory map confict: Unable to claim specific required memory regions.") )
	};

	class HardwareDeficiency : public virtual RuntimeError
	{
	public:
		DEFINE_RUNTIME_EXCEPTION( HardwareDeficiency, wxLt("Your machine's hardware is incapable of running PCSX2.  Sorry dood.") );
	};

	// ---------------------------------------------------------------------------------------
	// Streaming (file) Exceptions:
	//   Stream / BadStream / CannotCreateStream / FileNotFound / AccessDenied / EndOfStream
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
		StreamName = fromUTF8( objname ); \
	} \
	explicit classname( const char* objname, const wxString& msg_eng, const wxString& msg_xlt ) \
	{ \
		BaseException::InitBaseEx( msg_eng, msg_xlt ); \
		StreamName = fromUTF8( objname ); \
	} \
	explicit classname( const char* objname, const wxString& msg_eng ) \
	{ \
		BaseException::InitBaseEx( msg_eng, msg_eng ); \
		StreamName = fromUTF8( objname ); \
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
		DEFINE_STREAM_EXCEPTION( BadStream, wxLt("File data is corrupted or incomplete, or the stream connection closed unexpectedly.") )
	};

	// A generic exception for odd-ball stream creation errors.
	//
	class CannotCreateStream : public virtual Stream
	{
	public:
		DEFINE_STREAM_EXCEPTION( CannotCreateStream, wxLt("File could not be created or opened.") )
	};

	// Exception thrown when an attempt to open a non-existent file is made.
	// (this exception can also mean file permissions are invalid)
	//
	class FileNotFound : public virtual CannotCreateStream
	{
	public:
		DEFINE_STREAM_EXCEPTION( FileNotFound, wxLt("File not found.") )
	};

	class AccessDenied : public virtual CannotCreateStream
	{
	public:
		DEFINE_STREAM_EXCEPTION( AccessDenied, wxLt("Permission denied to file.") )
	};

	// EndOfStream can be used either as an error, or used just as a shortcut for manual
	// feof checks.
	//
	class EndOfStream : public virtual Stream
	{
	public:
		DEFINE_STREAM_EXCEPTION( EndOfStream, wxLt("Unexpected end of file or stream.") );
	};

#ifdef __WXMSW__
	// --------------------------------------------------------------------------------------
	//  Exception::WinApiError
	// --------------------------------------------------------------------------------------
	class WinApiError : public RuntimeError
	{
	public:
		int		ErrorId;

	public:
		DEFINE_EXCEPTION_COPYTORS( WinApiError )

			WinApiError( const char* msg="" );

		wxString GetMsgFromWindows() const;
		virtual wxString FormatDisplayMessage() const;
		virtual wxString FormatDiagnosticMessage() const;
	};
#endif
}

using Exception::BaseException;
