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
#include "Common.h"

#ifdef __LINUX__
#ifndef _S_IFDIR
#define _S_IFDIR S_IFDIR
#endif

#ifndef _S_IFREG
#define _S_IFREG S_IFREG
#endif
#else
#include <direct.h>
#endif

namespace Path
{

static bool IsPathSeparator( wxChar src )
{
#ifdef WIN32
	return (src == wxFileName::GetPathSeparator()) || (src == L'/');
#else
	return src == wxFileName::GetPathSeparator();
#endif
}

// Returns the length of the file.
// returns -1 if the file is not found.
int GetFileSize( const wxString& path )
{
	wxStructStat sbuf;
	if( wxStat( path.c_str(), &sbuf ) == -1 ) return -1;
	return sbuf.st_size;
}

bool IsRooted( const wxString& path )
{
	// if the first character is a backslash or period, or the second character
	// a colon, it's a safe bet we're rooted.

	if( path[0] == 0 ) return false;
#ifdef WIN32
	return IsPathSeparator(path[0]) || ( (path[1] == ':') && IsPathSeparator(path[2]) );
#else
	return IsPathSeparator(path[0]);
#endif
}

// ------------------------------------------------------------------------
// Concatenates two pathnames together, inserting delimiters (backslash on win32)
// as needed! Assumes the 'dest' is allocated to at least g_MaxPath length.
//
wxString Combine( const wxString& srcPath, const wxString& srcFile )
{
#if 0
	int pathlen;

	if( srcFile.empty() )
	{
		// No source filename?  Return the path unmodified.
		return srcPath;
	}

	if( IsRooted( srcFile ) || srcPath.empty() )
	{
		// No source path?  Or source filename is rooted?
		// Return the filename unmodified.
		return srcFile;
	}

	// strip off the srcPath's trailing backslashes (if any)
	// Note: The win32 build works better if I check for both forward and backslashes.
	// This might be a problem on Linux builds or maybe it doesn't matter?

	pathlen = srcPath.length();
	while( pathlen > 0 && IsPathSeparator(srcPath[pathlen-1]) )
		--pathlen;

	// Concatenate strings:
	guesslen = pathlen + srcFile.length() + 2;

	if( guesslen >= g_MaxPath )
		throw Exception::PathTooLong();

	// Concatenate!

	wxString dest( srcPath.begin(), srcPath.begin()+pathlen );
	dest += Separator;
	dest += srcFile;
	return dest;
	
#else
	// Use wx's Path system for concatenation because it's pretty smart.
	
	return (wxDirName( srcPath ) + srcFile).GetFullPath();
#endif
}

wxString Combine( const wxDirName& srcPath, const wxFileName& srcFile )
{
	return (srcPath + srcFile).GetFullPath();
}

wxString Combine( const wxString& srcPath, const wxDirName& srcFile )
{
	return ((wxDirName)srcPath + srcFile).ToString();
}

// Replaces the extension of the file with the one given.
// This function works for path names as well as file names.
wxString ReplaceExtension( const wxString& src, const wxString& ext )
{
	wxString dest;

	int pos = src.find_last_of( L'.' );
	if( pos == wxString::npos || pos == 0 )
		dest = src;
	else
		dest.assign( src.begin(), src.begin()+pos );

	if( !ext.empty() )
	{
		dest += '.';
		dest += ext;
	}
	
	return dest;
}

wxString ReplaceFilename( const wxString& src, const wxString& newfilename )
{
	// Implementation note: use wxWidgets to do this job.

	wxFileName jojo( src );
	jojo.SetFullName( newfilename );
	return jojo.GetFullPath();
}

wxString GetFilename( const wxString& src )
{
	return wxFileName(src).GetFullName();
}

wxString GetFilenameWithoutExt( const wxString& src )
{
	return wxFileName(src).GetName();
}

wxString GetDirectory( const wxString& src )
{
	return wxFileName(src).GetPath();
}


// returns the base/root directory of the given path.
// Example /this/that/something.txt -> dest == "/"
wxString GetRootDirectory( const wxString& src )
{
	int pos = src.find_first_of( wxFileName::GetPathSeparators() );
	if( pos == wxString::npos )
		return wxString();
	else
		return wxString( src.begin(), src.begin()+pos );
}

void CreateDirectory( const wxString& src )
{
	wxFileName::Mkdir( src );
}

void RemoveDirectory( const wxString& src )
{
	wxFileName::Rmdir( src );
}

}

