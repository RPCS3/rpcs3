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
#include "PrecompiledHeader.h"
#include "Win32.h"

// Translates an Errno code into an exception.
// Throws an exception based on the given error code (usually taken from ANSI C's errno)
void StreamException_ThrowFromErrno( const wxString& streamname, errno_t errcode )
{
	if( errcode == 0 ) return;

	switch( errcode )
	{
		case EINVAL:
			throw Exception::InvalidArgument( "Invalid argument" );

		case EACCES:	// Access denied!
			throw Exception::AccessDenied( streamname );

		case EMFILE:	// Too many open files!
			throw Exception::CreateStream( streamname, "Too many open files" );	// File handle allocation failure

		case EEXIST:
			throw Exception::CreateStream( streamname, "File already exists" );

		case ENOENT:	// File not found!
			throw Exception::FileNotFound( streamname );

		case EPIPE:
			throw Exception::BadStream( streamname, "Broken pipe" );

		case EBADF:
			throw Exception::BadStream( streamname, "Bad file number" );

		default:
			throw Exception::Stream( streamname,
				wxsFormat( L"General file/stream error [errno: %d]", errcode )
			);
	}
}

// Throws an exception based on the value returned from GetLastError.
// Performs an option return value success/fail check on hresult.
void StreamException_ThrowLastError( const wxString& streamname, HANDLE result )
{
	if( result != INVALID_HANDLE_VALUE ) return;

	int error = GetLastError();
	
	switch( error )
	{
		case ERROR_FILE_NOT_FOUND:
			throw Exception::FileNotFound( streamname );
		
		case ERROR_PATH_NOT_FOUND:
			throw Exception::FileNotFound( streamname );
		
		case ERROR_TOO_MANY_OPEN_FILES:
			throw Exception::CreateStream( streamname, "Too many open files" );
		
		case ERROR_ACCESS_DENIED:
			throw Exception::AccessDenied( streamname );

		case ERROR_INVALID_HANDLE:
			throw Exception::InvalidOperation( "Stream object or handle is invalid" );

		case ERROR_SHARING_VIOLATION:
			throw Exception::AccessDenied( streamname, "Sharing violation" );

		default:
		{
			throw Exception::Stream( streamname,
				wxsFormat( L"General Win32 File/stream error [GetLastError: %d]", error )
			);
		}
	}
}

// returns TRUE if an error occurred.
bool StreamException_LogFromErrno( const wxString& streamname, const wxChar* action, errno_t result )
{
	try
	{
		StreamException_ThrowFromErrno( streamname, result );
	}
	catch( Exception::Stream& ex )
	{
		Console::Notice( wxsFormat( L"%s: %s", action, ex.FormatDiagnosticMessage().c_str() ) );
		return true;
	}
	return false;
}

// returns TRUE if an error occurred.
bool StreamException_LogLastError( const wxString& streamname, const wxChar* action, HANDLE result )
{
	try
	{
		StreamException_ThrowLastError( streamname, result );
	}
	catch( Exception::Stream& ex )
	{
		Console::Notice( wxsFormat( L"%s: %s", action, ex.FormatDiagnosticMessage().c_str() ) );
		return true;
	}
	return false;
}

// Sets the NTFS compression flag for a directory or file. This function does not operate
// recursively.  Set compressStatus to false to decompress compressed files (and do nothing
// to already decompressed files).
//
// Exceptions thrown: None.
//   (Errors are logged to console.  Failures are considered non-critical)
//
void NTFS_CompressFile( const wxString& file, bool compressStatus )
{
	bool isFile = !wxDirExists( file );

	if( isFile && !wxFileExists( file ) ) return;
	if( !wxIsWritable( file ) ) return;

	const DWORD flags = isFile ? FILE_ATTRIBUTE_NORMAL : (FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_DIRECTORY);

	HANDLE bloated_crap = CreateFile( file,
		FILE_GENERIC_WRITE | FILE_GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		flags,
		NULL
	);

	// Fail silently -- non-compression of files and folders is not an errorable offense.

	if( !StreamException_LogLastError( file, L"NTFS Compress Notice", bloated_crap ) )
	{
		DWORD bytesReturned = 0;
		DWORD compressMode = compressStatus ? COMPRESSION_FORMAT_DEFAULT : COMPRESSION_FORMAT_NONE;
		
		BOOL result = DeviceIoControl(
			bloated_crap, FSCTL_SET_COMPRESSION,
			&compressMode, 2, NULL, 0,
			&bytesReturned, NULL
		);
		
		if( !result )
			StreamException_LogLastError( file, L"NTFS Compress Notice" );

		CloseHandle( bloated_crap );
	}
}
