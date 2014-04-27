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
#include "Win32.h"

#include "App.h"
#include "ConsoleLogger.h"

// --------------------------------------------------------------------------------------
//  Win32 Console Pipes
//  As a courtesy and convenience, we redirect stdout/stderr to the console and logfile.
// --------------------------------------------------------------------------------------

using namespace Threading;

// --------------------------------------------------------------------------------------
//  WinPipeThread
// --------------------------------------------------------------------------------------
class WinPipeThread : public pxThread
{
	typedef pxThread _parent;

protected:
	const HANDLE& m_outpipe;
	const ConsoleColors m_color;

public:
	WinPipeThread( const HANDLE& outpipe, ConsoleColors color )
		: m_outpipe( outpipe )
		, m_color( color )
	{
		m_name = (m_color == Color_Red) ? L"Redirect_Stderr" : L"Redirect_Stdout";
	}

	virtual ~WinPipeThread() throw()
	{
		_parent::Cancel();
	}

protected:
	void ExecuteTaskInThread()
	{
		::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL );
		if( m_outpipe == INVALID_HANDLE_VALUE ) return;

		try
		{
			char s8_Buf[2049];
			DWORD u32_Read = 0;

			while( true )
			{
				if( !ReadFile(m_outpipe, s8_Buf, sizeof(s8_Buf)-1, &u32_Read, NULL) )
				{
					DWORD result = GetLastError();
					if( result == ERROR_HANDLE_EOF || result == ERROR_BROKEN_PIPE ) break;
					if( result == ERROR_IO_PENDING )
					{
						Yield( 10 );
						continue;
					}

					throw Exception::WinApiError().SetDiagMsg(L"ReadFile from pipe failed.");
				}

				if( u32_Read <= 3 )
				{
					// Windows has a habit of sending 1 or 2 characters of every message, and then sending
					// the rest in a second message.  This is "ok" really, except our Console class is hardly
					// free of overhead, so it's helpful if we can concatenate the couple of messages together.
					// But we don't want to break the ability to print progressive status bars, like '....'
					// so I use a clever Yield/Peek loop combo that keeps reading as long as there's new data
					// immediately being fed to our pipe. :)  --air

					DWORD u32_avail = 0;

					do
					{
						Yield();
						if( !PeekNamedPipe(m_outpipe, 0, 0, 0, &u32_avail, 0) )
							throw Exception::WinApiError().SetDiagMsg(L"Error peeking Pipe.");

						if( u32_avail == 0 ) break;

						DWORD loopread;
						if( !ReadFile(m_outpipe, &s8_Buf[u32_Read], sizeof(s8_Buf)-u32_Read-1, &loopread, NULL) ) break;
						u32_Read += loopread;

					} while( u32_Read < sizeof(s8_Buf)-32 );
				}

				// ATTENTION: The Console always prints ANSI to the pipe independent if compiled as UNICODE or MBCS!
				s8_Buf[u32_Read] = 0;

				ConsoleColorScope cs(m_color);
				Console.DoWriteFromStdout( fromUTF8(s8_Buf) );

				TestCancel();
			}
		}
		catch( Exception::RuntimeError& ex )
		{
			// Log error, and fail silently.  It's not really important if the
			// pipe fails.  PCSX2 will run fine without it in any case.
			Console.Error( ex.FormatDiagnosticMessage() );
		}
	}
};

// --------------------------------------------------------------------------------------
//  WinPipeRedirection
// --------------------------------------------------------------------------------------
class WinPipeRedirection : public PipeRedirectionBase
{
	DeclareNoncopyableObject( WinPipeRedirection );

protected:
	DWORD		m_stdhandle;
	FILE*		m_stdfp;
	FILE		m_stdfp_copy;

	HANDLE		m_readpipe;
	HANDLE		m_writepipe;
	int			m_crtFile;
	FILE*		m_fp;

	WinPipeThread m_Thread;

public:
	WinPipeRedirection( FILE* stdstream );
	virtual ~WinPipeRedirection() throw();

	void Cleanup() throw();
};

WinPipeRedirection::WinPipeRedirection( FILE* stdstream )
	: m_Thread( m_readpipe, (stdstream == stderr) ? Color_Red : Color_Black )
{
	m_stdhandle		= ( stdstream == stderr ) ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE;
	m_stdfp			= stdstream;
	m_stdfp_copy	= *stdstream;

	m_readpipe		= INVALID_HANDLE_VALUE;
	m_writepipe		= INVALID_HANDLE_VALUE;
	m_crtFile		= -1;
	m_fp			= NULL;

	pxAssert( (stdstream == stderr) || (stdstream == stdout) );

	try
	{
		if( 0 == CreatePipe( &m_readpipe, &m_writepipe, NULL, 0 ) )
			throw Exception::WinApiError().SetDiagMsg(L"CreatePipe failed.");

		if( 0 == SetStdHandle( m_stdhandle, m_writepipe ) )
			throw Exception::WinApiError().SetDiagMsg(L"SetStdHandle failed.");

		// Note: Don't use GetStdHandle to "confirm" the handle.
		//
		// Under Windows7, and possibly Vista, GetStdHandle for STDOUT will return NULL
		// after it's been assigned a custom write pipe (this differs from XP, which
		// returns the assigned handle).  Amusingly, the GetStdHandle succeeds for STDERR
		// and also tends to succeed when the app is run from the MSVC debugger.
		//
		// Fortunately, there's no need to use GetStdHandle anyway, so long as SetStdHandle
		// didn't error.

		m_crtFile = _open_osfhandle( (intptr_t)m_writepipe, _O_TEXT );
		if( m_crtFile == -1 )
			throw Exception::RuntimeError().SetDiagMsg( L"_open_osfhandle returned -1." );

		m_fp = _fdopen( m_crtFile, "w" );
		if( m_fp == NULL )
			throw Exception::RuntimeError().SetDiagMsg( L"_fdopen returned NULL." );

		*m_stdfp = *m_fp;		// omg hack.  but it works >_<
		setvbuf( stdstream, NULL, _IONBF, 0 );

		m_Thread.Start();
	}
	catch( Exception::BaseThreadError& ex )
	{
		// thread object will become invalid because of scoping after we leave
		// the constructor, so re-pack a new exception:

		Cleanup();
		throw Exception::RuntimeError().SetDiagMsg( ex.FormatDiagnosticMessage() ).SetUserMsg( ex.FormatDisplayMessage() );
	}
	catch( BaseException& ex )
	{
		Cleanup();
		ex.DiagMsg() = (wxString)((stdstream==stdout) ? L"STDOUT" : L"STDERR") + L" Redirection Init failed: " + ex.DiagMsg();
		throw;
	}
	catch( ... )
	{
		// C++ doesn't execute the object destructor automatically, because it's fail++
		// (and I'm *not* encapsulating each handle into its own object >_<)

		Cleanup();
		throw;
	}
}

WinPipeRedirection::~WinPipeRedirection()
{
	Cleanup();
}

void WinPipeRedirection::Cleanup() throw()
{
	// restore the old handle we so graciously hacked earlier ;)
	//  (or don't and suffer CRT crashes!  ahaha!)

	if( m_stdfp != NULL )
		*m_stdfp = m_stdfp_copy;

	// Cleanup Order Notes:
	//  * The redirection thread is most likely blocking on ReadFile(), so we can't Cancel yet, lest we deadlock --
	//    Closing the writepipe (either directly or through the fp/crt handles) issues an EOF to the thread,
	//    so it's safe to Cancel afterward.
	//
	//  * The seemingly redundant series of checks here are designed to handle cases where the pipe init fails
	//    mid-init (in which case the writepipe might be allocated while the fp/crtFile are still invalid, etc).

	if( m_fp != NULL )
	{
		fclose( m_fp );
		m_fp = NULL;

		m_crtFile	= -1;						// crtFile is closed implicitly when closing m_fp
		m_writepipe = INVALID_HANDLE_VALUE;		// same for the write end of the pipe
	}

	if( m_crtFile != -1 )
	{
		_close( m_crtFile );
		m_crtFile	= -1;						// m_file is closed implicitly when closing crtFile
		m_writepipe = INVALID_HANDLE_VALUE;		// same for the write end of the pipe (I assume)
	}

	if( m_writepipe != INVALID_HANDLE_VALUE )
	{
		CloseHandle( m_writepipe );
		m_writepipe = INVALID_HANDLE_VALUE;
	}

	m_Thread.Cancel();

	if( m_readpipe != INVALID_HANDLE_VALUE )
	{
		CloseHandle( m_readpipe );
		m_readpipe = INVALID_HANDLE_VALUE;
	}
}

// The win32 specific implementation of PipeRedirection.
PipeRedirectionBase* NewPipeRedir( FILE* stdstream )
{
	try
	{
		return new WinPipeRedirection( stdstream );
	}
	catch( Exception::RuntimeError& ex )
	{
		// Entirely non-critical errors.  Log 'em and move along.
		Console.Error( ex.FormatDiagnosticMessage() );
	}

	return NULL;
}
