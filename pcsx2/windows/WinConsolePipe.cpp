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
#include "Win32.h"

#include "App.h"
#include "ConsoleLogger.h"


// --------------------------------------------------------------------------------------
//  Exception::Win32Error
// --------------------------------------------------------------------------------------

namespace Exception
{
	class Win32Error : public RuntimeError
	{
	public:
		int		ErrorId;
	
	public:
		DEFINE_EXCEPTION_COPYTORS( Win32Error )

		Win32Error( const char* msg="" )
		{
			ErrorId = GetLastError();
			BaseException::InitBaseEx( msg );
		}
		
		wxString GetMsgFromWindows() const
		{
			if (!ErrorId)
				return wxString();

			const DWORD BUF_LEN = 2048;
			TCHAR t_Msg[BUF_LEN];
			if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, ErrorId, 0, t_Msg, BUF_LEN, 0))
				return wxsFormat( L"Win32 Error #%d: %s", ErrorId, t_Msg );

			return wxsFormat( L"Win32 Error #%d (no text msg available)", ErrorId );
		}

		virtual wxString FormatDisplayMessage() const
		{
			return m_message_user + L"\n\n" + GetMsgFromWindows();
		}

		virtual wxString FormatDiagnosticMessage() const
		{
			return m_message_diag + L"\n\t" + GetMsgFromWindows();
		}
	};
}

// --------------------------------------------------------------------------------------
//  Win32 Console Pipes
//  As a courtesy and convenience, we redirect stdout/stderr to the console and logfile.
// --------------------------------------------------------------------------------------

using namespace Threading;

static void CreatePipe( HANDLE& ph_Pipe, HANDLE& ph_File )
{
	// Create a threadsafe unique name for the Pipe
	static int s32_Counter = 0;

	wxString s_PipeName;
	s_PipeName.Printf( L"\\\\.\\pipe\\pcsxPipe%X_%X_%X_%X",
					  GetCurrentProcessId(), GetCurrentThreadId(), GetTickCount(), s32_Counter++);

	SECURITY_ATTRIBUTES k_Secur;
	k_Secur.nLength              = sizeof(SECURITY_ATTRIBUTES);
	k_Secur.lpSecurityDescriptor = 0;
	k_Secur.bInheritHandle       = TRUE;

	ph_Pipe = CreateNamedPipe(s_PipeName, PIPE_ACCESS_DUPLEX, 0, 1, 2048, 2048, 0, &k_Secur);

	if (ph_Pipe == INVALID_HANDLE_VALUE)
		throw Exception::Win32Error( "Error creating Named Pipe." );

	ph_File = CreateFile(s_PipeName, GENERIC_READ|GENERIC_WRITE, 0, &k_Secur, OPEN_EXISTING, 0, NULL);

	if (ph_File == INVALID_HANDLE_VALUE)
		throw Exception::Win32Error( "Error creating Pipe Reader." );

	if (!ConnectNamedPipe(ph_Pipe, NULL))
	{
		if (GetLastError() != ERROR_PIPE_CONNECTED)
			throw Exception::Win32Error( "Error connecting Pipe." );
	}

	SetHandleInformation(ph_Pipe, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(ph_File, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
}

// Reads from the Pipe and appends the read data to ps_Data
// returns TRUE if something was printed to console, or false if the stdout/err were idle.
static bool ReadPipe(HANDLE h_Pipe, Console::Colors color )
{
	// IMPORTANT: Check if there is data that can be read.
	// The first console output will be lost if ReadFile() is called before data becomes available!
	// It does not make any sense but the following 5 lines are indispensable!!
	DWORD u32_Avail = 0;
	if (!PeekNamedPipe(h_Pipe, 0, 0, 0, &u32_Avail, 0))
		throw Exception::Win32Error( "Error peeking Pipe." );

	if (!u32_Avail)
		return false;

	char s8_Buf[2049];
	DWORD u32_Read = 0;
	do
	{
		if (!ReadFile(h_Pipe, s8_Buf, sizeof(s8_Buf)-1, &u32_Read, NULL))
		{
			if (GetLastError() != ERROR_IO_PENDING)
				throw Exception::Win32Error( "Error reading Pipe." );
		}

		// ATTENTION: The Console always prints ANSI to the pipe independent if compiled as UNICODE or MBCS!
		s8_Buf[u32_Read] = 0;
		OemToCharA(s8_Buf, s8_Buf);			// convert DOS codepage -> ANSI
		Console::Write( color, s8_Buf );	// convert ANSI -> Unicode if compiled as Unicode
	}
	while (u32_Read == sizeof(s8_Buf)-1);

	return true;
}

class WinPipeThread : public PersistentThread
{
protected:
	const HANDLE&	mh_OutPipe;
	const HANDLE&	mh_ErrPipe;

public:
	WinPipeThread( const HANDLE& outpipe, const HANDLE& errpipe ) :
		mh_OutPipe( outpipe )
	,	mh_ErrPipe( errpipe )
	//,	mk_OverOut( overout )
	//,	mk_OverErr( overerr )
	{
	}
	
	virtual ~WinPipeThread() throw()
	{
		PersistentThread::Cancel();
		SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL );
	}
	
protected:
	int ExecuteTask()
	{
		try
		{
			while( true )
			{
				Sleep( 100 );
				pthread_testcancel();
				ReadPipe(mh_OutPipe, Color_Black );
				ReadPipe(mh_ErrPipe, Color_Red );
			}
		}
		catch( Exception::Win32Error& ex )
		{
			// Log error, and fail silently.  It's not really important if the
			// pipe fails.  PCSX2 will run fine without it in any case.
			Console::Error( ex.FormatDiagnosticMessage() );
			return -2;
		}
	}
};

class WinPipeRedirection : public PipeRedirectionBase
{
	DeclareNoncopyableObject( WinPipeRedirection );

protected:
	HANDLE		mh_OutPipe;
	HANDLE		mh_ErrPipe;
	HANDLE		mh_OutFile;
	HANDLE		mh_ErrFile;

	int			m_hCrtOut;
	int			m_hCrtErr;

	FILE*		h_fpOut;
	FILE*		h_fpErr;

	WinPipeThread m_Thread;

public:
	WinPipeRedirection();
	virtual ~WinPipeRedirection() throw();
};

WinPipeRedirection::WinPipeRedirection() :
	mh_OutPipe(INVALID_HANDLE_VALUE)
,	mh_ErrPipe(INVALID_HANDLE_VALUE)
,	mh_OutFile(INVALID_HANDLE_VALUE)
,	mh_ErrFile(INVALID_HANDLE_VALUE)
,	m_hCrtOut(-1)
,	m_hCrtErr(-1)
,	h_fpOut(NULL)
,	h_fpErr(NULL)

,	m_Thread( mh_OutPipe, mh_ErrPipe )
{
	CreatePipe(mh_OutPipe, mh_OutFile ); 
	CreatePipe(mh_ErrPipe, mh_ErrFile );

	SetStdHandle( STD_OUTPUT_HANDLE, mh_OutFile );
	SetStdHandle( STD_ERROR_HANDLE, mh_ErrFile );

	m_hCrtOut = _open_osfhandle( (intptr_t)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT );
	m_hCrtErr = _open_osfhandle( (intptr_t)GetStdHandle(STD_ERROR_HANDLE), _O_TEXT );

	h_fpOut = _fdopen( m_hCrtOut, "w" );
	h_fpErr = _fdopen( m_hCrtErr, "w" );

	*stdout = *h_fpOut;
	*stderr = *h_fpErr;

	setvbuf( stdout, NULL, _IONBF, 0 );
	setvbuf( stderr, NULL, _IONBF, 0 );

	m_Thread.Start();
}

WinPipeRedirection::~WinPipeRedirection() throw()
{
	m_Thread.Cancel();

	#define safe_CloseHandle( ptr ) \
		((void) (( ( ptr != INVALID_HANDLE_VALUE ) && (!!CloseHandle( ptr ), !!0) ), ptr = INVALID_HANDLE_VALUE))

	safe_CloseHandle(mh_OutPipe);
	safe_CloseHandle(mh_ErrPipe);

	if( h_fpOut != NULL )
	{
		fclose( h_fpOut );
		h_fpOut = NULL;
	}
	
	if( h_fpErr != NULL )
	{
		fclose( h_fpErr );
		h_fpErr = NULL;
	}

	#define safe_close( ptr ) \
		((void) (( ( ptr != -1 ) && (!!_close( ptr ), !!0) ), ptr = -1))

	// CrtOut and CrtErr are closed implicitly when closing fpOut/fpErr
	// OutFile and ErrFile are closed implicitly when closing m_hCrtOut/Err
}

// The win32 specific implementation of PipeRedirection.
PipeRedirectionBase* NewPipeRedir()
{
	return new WinPipeRedirection();
}
