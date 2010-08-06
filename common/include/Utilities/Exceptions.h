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

	public:
		virtual ~BaseException() throw()=0;	// the =0; syntax forces this class into "abstract" mode.

		const wxString& DiagMsg() const { return m_message_diag; }
		const wxString& UserMsg() const { return m_message_user; }

		wxString& DiagMsg() { return m_message_diag; }
		wxString& UserMsg() { return m_message_user; }

		BaseException& SetBothMsgs( const wxChar* msg_diag );
		BaseException& SetDiagMsg( const wxString& msg_diag );
		BaseException& SetUserMsg( const wxString& msg_user );

		// Returns a message suitable for diagnostic / logging purposes.
		// This message is always in English, and includes a full stack trace.
		virtual wxString FormatDiagnosticMessage() const;

		// Returns a message suitable for end-user display.
		// This message is usually meant for display in a user popup or such.
		virtual wxString FormatDisplayMessage() const;

		virtual void Rethrow() const=0;
		virtual BaseException* Clone() const=0;
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

		virtual void Rethrow() const=0;
		virtual Ps2Generic* Clone() const=0;
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
// ambiguity errors on virtual inheritance (it really shouldn't!).  So I have to force it to the
// BaseException base class.  Not sure if this is Stupid Standard Tricks or Stupid MSVC Tricks. --air
//
// (update: web searches indicate it's MSVC specific -- happens in 2008, not sure about 2010).
//
#define DEFINE_EXCEPTION_COPYTORS( classname, parent ) \
private: \
	typedef parent _parent; \
public: \
	virtual ~classname() throw() {} \
	virtual void Rethrow() const { throw *this; } \
	virtual classname* Clone() const { return new classname( *this ); }

#define DEFINE_EXCEPTION_MESSAGES( classname ) \
public: \
	classname& SetBothMsgs( const wxChar* msg_diag )	{ BaseException::SetBothMsgs(msg_diag);	return *this; } \
	classname& SetDiagMsg( const wxString& msg_diag )	{ m_message_diag = msg_diag;			return *this; } \
	classname& SetUserMsg( const wxString& msg_user )	{ m_message_user = msg_user;			return *this; }

#define DEFINE_RUNTIME_EXCEPTION( classname, parent, message ) \
	DEFINE_EXCEPTION_COPYTORS( classname, parent ) \
	classname() { SetDiagMsg(message); } \
	DEFINE_EXCEPTION_MESSAGES( classname )
	
	
	// ---------------------------------------------------------------------------------------
	//  RuntimeError - Generalized Exceptions with Recoverable Traits!
	// ---------------------------------------------------------------------------------------

	class RuntimeError : public BaseException
	{
		DEFINE_EXCEPTION_COPYTORS( RuntimeError, BaseException )
		DEFINE_EXCEPTION_MESSAGES( RuntimeError )

	public:
		bool	IsSilent;

		RuntimeError() { IsSilent = false; }
		RuntimeError( const std::runtime_error& ex, const wxString& prefix=wxEmptyString );
		RuntimeError( const std::exception& ex, const wxString& prefix=wxEmptyString );
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
	class CancelEvent : public RuntimeError
	{
		DEFINE_RUNTIME_EXCEPTION( CancelEvent, RuntimeError, wxLt("No reason given.") )

	public:
		explicit CancelEvent( const wxString& logmsg )
		{
			m_message_diag = logmsg;
			// overridden message formatters only use the diagnostic version...
		}

		virtual wxString FormatDisplayMessage() const;
		virtual wxString FormatDiagnosticMessage() const;
	};

	// ---------------------------------------------------------------------------------------
	//  OutOfMemory
	// ---------------------------------------------------------------------------------------
	// This exception has a custom-formatted Diagnostic string.  The parameter give when constructing
	// the exception is a block/alloc name, which is used as a formatting parameter in the diagnostic
	// output.  The default diagnostic message is "Out of memory exception, while allocating the %s."
	// where %s is filled in with the block name.
	//
	// The user string is not custom-formatted, and should contain *NO* %s tags.
	//
	class OutOfMemory : public RuntimeError
	{
		DEFINE_RUNTIME_EXCEPTION( OutOfMemory, RuntimeError, wxLt("Out of memory?!") )

	public:
		wxString	AllocDescription;

	public:
		OutOfMemory( const wxString& allocdesc );

		virtual wxString FormatDisplayMessage() const;
		virtual wxString FormatDiagnosticMessage() const;
	};

	class ParseError : public RuntimeError
	{
		DEFINE_RUNTIME_EXCEPTION( ParseError, RuntimeError, wxLt("Parse error") );
	};

	// ---------------------------------------------------------------------------------------
	// Hardware/OS Exceptions:
	//   HardwareDeficiency / VirtualMemoryMapConflict
	// ---------------------------------------------------------------------------------------

	// This exception is a specific type of OutOfMemory error that isn't "really" an out of
	// memory error.  More likely it's caused by a plugin or driver reserving a range of memory
	// we'd really like to have access to.
	class VirtualMemoryMapConflict : public OutOfMemory
	{
		DEFINE_RUNTIME_EXCEPTION( VirtualMemoryMapConflict, OutOfMemory, wxLt("Virtual memory map confict: Unable to claim specific required memory regions.") )
	};

	class HardwareDeficiency : public RuntimeError
	{
	public:
		DEFINE_RUNTIME_EXCEPTION( HardwareDeficiency, RuntimeError, wxLt("Your machine's hardware is incapable of running PCSX2.  Sorry dood.") );
	};

	// ---------------------------------------------------------------------------------------
	// Streaming (file) Exceptions:
	//   Stream / BadStream / CannotCreateStream / FileNotFound / AccessDenied / EndOfStream
	// ---------------------------------------------------------------------------------------

	#define DEFINE_STREAM_EXCEPTION_ACCESSORS( classname ) \
		virtual classname& SetStreamName( const wxString& name )	{ StreamName = name;			return *this; } \
		virtual classname& SetStreamName( const char* name )		{ StreamName = fromUTF8(name);	return *this; }

	#define DEFINE_STREAM_EXCEPTION( classname, parent, message ) \
		DEFINE_RUNTIME_EXCEPTION( classname, parent, message ) \
		classname( const wxString& filename ) { \
			StreamName = filename; \
			SetBothMsgs(message); \
		} \
		DEFINE_STREAM_EXCEPTION_ACCESSORS( classname )
	
	// Generic stream error.  Contains the name of the stream and a message.
	// This exception is usually thrown via derived classes, except in the (rare) case of a
	// generic / unknown error.
	//
	class Stream : public RuntimeError
	{
		DEFINE_STREAM_EXCEPTION( Stream, RuntimeError, wxLt("General file operation error.") )

	public:
		wxString StreamName;		// name of the stream (if applicable)

		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;
	};

	// A generic base error class for bad streams -- corrupted data, sudden closures, loss of
	// connection, or anything else that would indicate a failure to read the data after the
	// stream was successfully opened.
	//
	class BadStream : public Stream
	{
		DEFINE_STREAM_EXCEPTION( BadStream, Stream, wxLt("File data is corrupted or incomplete, or the stream connection closed unexpectedly.") )
	};

	// A generic exception for odd-ball stream creation errors.
	//
	class CannotCreateStream : public Stream
	{
		DEFINE_STREAM_EXCEPTION( CannotCreateStream, Stream, wxLt("File could not be created or opened.") )
	};

	// Exception thrown when an attempt to open a non-existent file is made.
	// (this exception can also mean file permissions are invalid)
	//
	class FileNotFound : public CannotCreateStream
	{
	public:
		DEFINE_STREAM_EXCEPTION( FileNotFound, CannotCreateStream, wxLt("File not found.") )
	};

	class AccessDenied : public CannotCreateStream
	{
	public:
		DEFINE_STREAM_EXCEPTION( AccessDenied, CannotCreateStream, wxLt("Permission denied to file.") )
	};

	// EndOfStream can be used either as an error, or used just as a shortcut for manual
	// feof checks.
	//
	class EndOfStream : public Stream
	{
	public:
		DEFINE_STREAM_EXCEPTION( EndOfStream, Stream, wxLt("Unexpected end of file or stream.") );
	};

#ifdef __WXMSW__
	// --------------------------------------------------------------------------------------
	//  Exception::WinApiError
	// --------------------------------------------------------------------------------------
	class WinApiError : public RuntimeError
	{
		DEFINE_EXCEPTION_COPYTORS( WinApiError, RuntimeError )
		DEFINE_EXCEPTION_MESSAGES( WinApiError )

	public:
		int		ErrorId;

	public:
		WinApiError();

		wxString GetMsgFromWindows() const;
		virtual wxString FormatDisplayMessage() const;
		virtual wxString FormatDiagnosticMessage() const;
	};
#endif
}

using Exception::BaseException;
