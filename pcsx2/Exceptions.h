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

//////////////////////////////////////////////////////////////////////////////////////////
// This class provides an easy and clean method for ensuring objects are not copyable.
class NoncopyableObject
{
protected:
	NoncopyableObject() {}
	~NoncopyableObject() {}

// Programmer's note:
//   No need to provide implementations for these methods since they should
//   never be referenced anyway.  No references?  No Linker Errors!  Noncopyable!
private:
	// Copy me?  I think not!
	explicit NoncopyableObject( const NoncopyableObject& );
	// Assign me?  I think not!
	const NoncopyableObject& operator=( const NoncopyableObject& );
};

//////////////////////////////////////////////////////////////////////////////////////////
// Base class used to implement type-safe sealed classes.
// This class should never be used directly.  Use the Sealed macro instead, which ensures
// all sealed classes derive from a unique BaseSealed (preventing them from accidentally
// circumventing sealing by inheriting from multiple sealed classes.
//
template < int T >
class __BaseSealed
{
protected:
	__BaseSealed()
	{
	}
};

// Use this macro/class as a base to seal a class from being derived from.
// This macro works by providing a unique base class with a protected constructor
// for every class that derives from it. 
#define Sealed private virtual __BaseSealed<__COUNTER__>

extern wxLocale* g_EnglishLocale;

extern wxString GetEnglish( const char* msg );
extern wxString GetTranslation( const char* msg );

namespace Exception
{
	//////////////////////////////////////////////////////////////////////////////////
	// std::exception sucks, and isn't entirely cross-platform reliable in its implementation,
	// so I made a replacement.
	//
	// Note, this class is "abstract" which means you shouldn't use it directly like, ever.
	// Use Exception::RuntimeError or Exception::LogicError instead for generic exceptions.
	//
	class BaseException
	{
	protected:
		const wxString m_message_eng;			// (untranslated) a "detailed" message of what disastrous thing has occurred!
		const wxString m_message;				// (translated) a "detailed" message of what disastrous thing has occurred!
		const wxString m_stacktrace;			// contains the stack trace string dump (unimplemented)

	public:
		virtual ~BaseException() throw()=0;	// the =0; syntax forces this class into "abstract" mode.

		// copy construct
		BaseException( const BaseException& src ) : 
			m_message_eng( src.m_message_eng ),
			m_message( src.m_message ),
			m_stacktrace( src.m_stacktrace ) { }

		// Contruction using two pre-formatted pre-translated messages
		BaseException( const wxString& msg_eng, const wxString& msg_xlt );
		
		// Construction using one translation key.
		explicit BaseException( const char* msg_eng );

		// Returns a message suitable for diagnostic / logging purposes.
		// This message is always in english, and includes a full stack trace.
		virtual wxString LogMessage() const;

		// Returns a message suitable for end-user display.
		// This message is usually meant for display in a user popup or such.
		virtual wxString DisplayMessage() const { return m_message; }
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////
	// This class is used as a base exception for things tossed by PS2 cpus (EE, IOP, etc).
	// Translation Note: These exceptions are never translated, except to issue a general
	// error message to the user (which is xspecified below).
	//
	class Ps2Generic : public BaseException
	{
	public:
		virtual ~Ps2Generic() throw() {}

		explicit Ps2Generic( const char* msg="Ps2/MIPS cpu caused a general exception" ) : 
			BaseException( msg ) { }
		explicit Ps2Generic( const wxString& msg_eng, const wxString& msg_xlt=_("Ps2/MIPS cpu caused a general exception") ) :
			BaseException( msg_eng, msg_xlt ) { }

		virtual u32 GetPc() const=0;
		virtual bool IsDelaySlot() const=0;
	};

	//////////////////////////////////////////////////////////////////////////////////
	//
	class RuntimeError : public BaseException
	{
	public:
		virtual ~RuntimeError() throw() {}
		
		RuntimeError( const RuntimeError& src ) : BaseException( src ) {}

		explicit RuntimeError( const char* msg="An unhandled runtime error has occurred, somewhere in the depths of Pcsx2's cluttered brain-matter." ) :
			BaseException( msg ) { }

		explicit RuntimeError( const wxString& msg_eng, const wxString& msg_xlt ) :
			BaseException( msg_eng, msg_xlt ) { }
	};

	// ------------------------------------------------------------------------
	class LogicError : public BaseException
	{
	public:
		virtual ~LogicError() throw() {}

		LogicError( const LogicError& src ) : BaseException( src ) {}

		explicit LogicError( const char* msg="An unhandled logic error has occurred." ) :
			BaseException( msg ) { }

		explicit LogicError( const wxString& msg_eng, const wxString& msg_xlt ) :
			BaseException( msg_eng, msg_xlt ) { }
	};
	
	//////////////////////////////////////////////////////////////////////////////////
	//
	class OutOfMemory : public RuntimeError
	{
	public:
		virtual ~OutOfMemory() throw() {}
		explicit OutOfMemory( const char* msg="Out of memory" ) :
			RuntimeError( msg ) {}

		explicit OutOfMemory( const wxString& msg_eng, const wxString& msg_xlt=_("Out of memory") ) :
			RuntimeError( msg_eng, msg_xlt ) { }
	};

	// ------------------------------------------------------------------------
	// This exception thrown any time an operation is attempted when an object
	// is in an uninitialized state.
	class InvalidOperation : public LogicError
	{
	public:
		virtual ~InvalidOperation() throw() {}
		explicit InvalidOperation( const char* msg="Attempted method call is invalid for the current object or program state." ) :
			LogicError( msg ) {}

		explicit InvalidOperation( const wxString& msg_eng, const wxString& msg_xlt ) :
			LogicError( msg_eng, msg_xlt ) { }
	};

	// ------------------------------------------------------------------------
	// This exception thrown any time an operation is attempted when an object
	// is in an uninitialized state.
	class InvalidArgument : public LogicError
	{
	public:
		virtual ~InvalidArgument() throw() {}
		explicit InvalidArgument( const char* msg="Invalid argument passed to a function." ) :
			LogicError( msg )
		{
			// assertions make debugging easier sometimes. :)
			wxASSERT( msg );
		}
	};

	// ------------------------------------------------------------------------
	// Keep those array indexers in bounds when using the SafeArray type, or you'll be
	// seeing these.
	class IndexBoundsFault : public LogicError
	{
	public:
		const wxString ArrayName;
		const int ArrayLength;
		const int BadIndex;

	public:
		virtual ~IndexBoundsFault() throw() {}
		explicit IndexBoundsFault( const wxString& objname, int index, int arrsize ) :
			LogicError( "Index is outside the bounds of an array." ),
			ArrayName( objname ),
			ArrayLength( arrsize ),
			BadIndex( index )
		{
			// assertions make debugging easier sometimes. :)
			wxASSERT( wxT("Index is outside the bounds of an array") );
		}
		
		virtual wxString LogMessage() const;
		virtual wxString DisplayMessage() const;
	};
	
	// ------------------------------------------------------------------------
	class ParseError : public RuntimeError
	{
	public:
		virtual ~ParseError() throw() {}
		explicit ParseError( const char* msg="Parse error" ) :
			RuntimeError( msg ) {}
	};	

	//////////////////////////////////////////////////////////////////////////////////
	//
	class HardwareDeficiency : public RuntimeError
	{
	public:
		explicit HardwareDeficiency( const char* msg="Your machine's hardware is incapable of running Pcsx2.  Sorry dood." ) :
			RuntimeError( msg ) {}
		virtual ~HardwareDeficiency() throw() {}
	};

	// ------------------------------------------------------------------------
	// This exception is thrown by the PS2 emulation (R5900, etc) when bad things happen
	// that force the emulation state to terminate.  The GUI should handle them by returning
	// the user to the GUI.
	class CpuStateShutdown : public RuntimeError
	{
	public:
		virtual ~CpuStateShutdown() throw() {}
		explicit CpuStateShutdown( const char* msg="Unexpected emulation shutdown" ) :
			RuntimeError( msg ) {}

		explicit CpuStateShutdown( const wxString& msg_eng, const wxString& msg_xlt=wxString() ) :
			RuntimeError( msg_eng, msg_xlt.IsEmpty() ? wxT("Unexpected emulation shutdown") : msg_xlt ) { }
	};

	// ------------------------------------------------------------------------
	class PluginFailure : public RuntimeError
	{
	public:
		wxString plugin_name;		// name of the plugin

		virtual ~PluginFailure() throw() {}

		explicit PluginFailure( const char* plugin, const char* msg="%s plugin encountered a critical error" ) :
			RuntimeError( msg )
		,	plugin_name( wxString::FromAscii(plugin) ) {}

		virtual wxString LogMessage() const;
		virtual wxString DisplayMessage() const;
	};

	// ------------------------------------------------------------------------
	class ThreadCreationError : public RuntimeError
	{
	public:
		virtual ~ThreadCreationError() throw() {}
		explicit ThreadCreationError( const char* msg="Thread could not be created." ) :
			RuntimeError( msg ) {}
	};

	//////////////////////////////////////////////////////////////////////////////////
	//                             STREAMING EXCEPTIONS

	// ------------------------------------------------------------------------
	// Generic stream error.  Contains the name of the stream and a message.
	// This exception is usually thrown via derived classes, except in the (rare) case of a generic / unknown error.
	class Stream : public RuntimeError
	{
	public:
		wxString StreamName;		// name of the stream (if applicable)

		virtual ~Stream() throw() {}

		// copy construct!
		Stream( const Stream& src ) :
			RuntimeError( src ),
			StreamName( src.StreamName ) {}

		explicit Stream(
			const wxString& objname=wxString(),
			const char* msg="General file operation error"		// general error while accessing or operating on a file or stream
		) :
			RuntimeError( msg ),
			StreamName( objname ) {}

		explicit Stream( const wxString& objname, const wxString& msg_eng, const wxString& msg_xlt=_("General file operation error") ) :
			RuntimeError( msg_eng, msg_xlt ),
			StreamName( objname ) {}

		virtual wxString LogMessage() const;
		virtual wxString DisplayMessage() const;
	};

	// ------------------------------------------------------------------------
	// A generic base error class for bad streams -- corrupted data, sudden closures, loss of
	// connection, or anything else that would indicate a failure to read the data after the
	// stream was successfully opened.
	class BadStream : public Stream
	{
	public:
		virtual ~BadStream() throw() {}
		explicit BadStream(
			const wxString& objname=wxString(),
			const char* msg="File data is corrupted or incomplete, or the stream connection closed unexpectedly"
		) :
			Stream( objname, msg ) {}
	};

	// ------------------------------------------------------------------------
	// A generic exception for odd-ball stream creation errors.
	class CreateStream : public Stream
	{
	public:
		virtual ~CreateStream() throw() {}

		explicit CreateStream(
			const char* objname,
			const char* msg="File could not be created or opened" ) :
		Stream( wxString::FromAscii( objname ), msg ) {}

		explicit CreateStream(
			const wxString& objname=wxString(),
			const char* msg="File could not be created or opened" ) :
		Stream( objname, msg ) {}	
	};

	// ------------------------------------------------------------------------
	// Exception thrown when an attempt to open a non-existent file is made.
	// (this exception can also mean file permissions are invalid)
	class FileNotFound : public CreateStream
	{
	public:
		virtual ~FileNotFound() throw() {}

		explicit FileNotFound(
			const wxString& objname=wxString(),
			const char* msg="File not found" ) :

		CreateStream( objname, msg ) {}
	};

	// ------------------------------------------------------------------------
	class AccessDenied : public CreateStream
	{
	public:
		virtual ~AccessDenied() throw() {}
		explicit AccessDenied(
			const wxString& objname=wxString(),
			const char* msg="Permission denied to file" ) :
		CreateStream( objname, msg ) {}
	};

	// ------------------------------------------------------------------------
	// Generic End of Stream exception (sometimes an error, and sometimes just used as a
	// shortcut for manual feof checks).
	class EndOfStream : public Stream
	{
	public:
		virtual ~EndOfStream() throw() {}
		explicit EndOfStream( const wxString& objname, const char* msg="End of file" ) :
			Stream( objname, msg ) {}
	};

	//////////////////////////////////////////////////////////////////////////////////
	//                            SAVESTATE EXCEPTIONS

	// Exception thrown when a corrupted or truncated savestate is encountered.
	class BadSavedState : public BadStream
	{
	public:
		virtual ~BadSavedState() throw() {}
		explicit BadSavedState(
			const wxString& objname=wxString(),
			const char* msg="Savestate data is corrupted" ) :	// or incomplete
		BadStream( objname, msg ) {}
	};

	// ------------------------------------------------------------------------
	// Exception thrown by SaveState class when a critical plugin or gzread
	class FreezePluginFailure : public RuntimeError
	{
	public:
		wxString plugin_name;		// name of the plugin
		wxString freeze_action;

		virtual ~FreezePluginFailure() throw() {}
		explicit FreezePluginFailure( const char* plugin, const char* action,
			const wxString& msg_xlt=_("Plugin error occurred while loading/saving state") )
		:
			RuntimeError( wxString(), msg_xlt )		// LogMessage / DisplayMessage build their own messages
		,	plugin_name( wxString::FromAscii(plugin) )
		,	freeze_action( wxString::FromAscii(action) ){}
		
		virtual wxString LogMessage() const;
		virtual wxString DisplayMessage() const;
	};

	// ------------------------------------------------------------------------
	// The savestate code throws Recoverable errors when it fails prior to actually modifying
	// the current emulation state.  Recoverable errors are always thrown from the SaveState
	// object construction (and never from Freeze methods).
	class StateLoadError_Recoverable : public RuntimeError
	{
	public:
		virtual ~StateLoadError_Recoverable() throw() {}
		explicit StateLoadError_Recoverable( const char* msg="Recoverable savestate load error" ) :
			RuntimeError( msg ) {}
	};

	// ------------------------------------------------------------------------
	// A recoverable exception thrown when the savestate being loaded isn't supported.
	class UnsupportedStateVersion : public StateLoadError_Recoverable
	{
	public:
		u32 Version;		// version number of the unsupported state.

	public:
		virtual ~UnsupportedStateVersion() throw() {}
		explicit UnsupportedStateVersion( int version ) :
			StateLoadError_Recoverable(),
			Version( version )
		{}

		virtual wxString LogMessage() const;
		virtual wxString DisplayMessage() const;
	};

	// ------------------------------------------------------------------------
	// A recoverable exception thrown when the CRC of the savestate does not match the
	// CRC returned by the Cdvd driver.
	// [feature not implemented yet]
	class StateCrcMismatch : public StateLoadError_Recoverable
	{
	public:
		u32 Crc_Savestate;
		u32 Crc_Cdvd;

	public:
		virtual ~StateCrcMismatch() throw() {}
		explicit StateCrcMismatch( u32 crc_save, u32 crc_cdvd )
		:	StateLoadError_Recoverable()
		,	Crc_Savestate( crc_save )
		,	Crc_Cdvd( crc_cdvd )
		{}

		virtual wxString LogMessage() const;
		virtual wxString DisplayMessage() const;
	};
}
