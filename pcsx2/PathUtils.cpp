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

#ifdef WIN32
// Path Separator used when creating new paths.
static const char Separator( '\\' );
// Path separators used when breaking existing paths into parts and pieces.
static const string Delimiters( "\\/" );
#else
static const char Separator = '/';
static const char Delimiters( '/' );
#endif

bool Exists( const string& path )
{
	struct stat sbuf;
	return stat( path.c_str(), &sbuf ) == 0;
}

// This function returns false if the path does not exist, or if the path exists and
// is a file.
bool isDirectory( const string& path )
{
	struct stat sbuf;
	if( stat( path.c_str(), &sbuf ) == -1 ) return false;
	return !!(sbuf.st_mode & _S_IFDIR);
}

// This function returns false if the path does not exist, or if the path exists and
// is a directory.
bool isFile( const string& path )
{
	struct stat sbuf;
	if( stat( path.c_str(), &sbuf ) == -1 ) return false;
	return !!(sbuf.st_mode & _S_IFREG);
}

// Returns the length of the file.
// returns -1 if the file is not found.
int getFileSize( const string& path )
{
	struct stat sbuf;
	if( stat( path.c_str(), &sbuf ) == -1 ) return -1;
	return sbuf.st_size;
}

bool isRooted( const string& path )
{
	// if the first character is a backslash or period, or the second character
	// a colon, it's a safe bet we're rooted.

	if( path[0] == 0 ) return FALSE;
#ifdef WIN32
	return (path[0] == '/') || (path[0] == '\\') || (path[1] == ':');
#else
	return (path[0] == Separator);
#endif
}

// Concatenates two pathnames together, inserting delimiters (backslash on win32)
// as needed! Assumes the 'dest' is allocated to at least g_MaxPath length.
void Combine( string& dest, const string& srcPath, const string& srcFile )
{
	int pathlen, guesslen;

	if( srcFile.empty() )
	{
		// No source filename?  Return the path unmodified.
		dest = srcPath;
		return;
	}

	if( isRooted( srcFile ) || srcPath.empty() )
	{
		// No source path?  Or source filename is rooted?
		// Return the filename unmodified.
		dest = srcFile;
		return;
	}

	// strip off the srcPath's trailing backslashes (if any)
	// Note: The win32 build works better if I check for both forward and backslashes.
	// This might be a problem on Linux builds or maybe it doesn't matter?

	pathlen = srcPath.length();
	while( pathlen > 0 && ((srcPath[pathlen-1] == '\\') || (srcPath[pathlen-1] == '/')) )
		--pathlen;

	// Concatenate strings:
	guesslen = pathlen + srcFile.length() + 2;

	if( guesslen >= g_MaxPath )
		throw Exception::PathTooLong();

	// Concatenate!

	dest.assign( srcPath.begin(), srcPath.begin()+pathlen );
	dest += Separator;
	dest += srcFile;
}

// Replaces the extension of the file with the one given.
void ReplaceExtension( string& dest, const string& src, const string& ext )
{
	int pos = src.find_last_of( '.' );
	if( pos == string::npos || pos == 0 )
		dest = src;
	else
		dest.assign( src.begin(), src.begin()+pos );

	if( !ext.empty() )
	{
		dest += '.';
		dest += ext;
	}
}

// finds the starting character position of a filename for the given source path.
static int _findFilenamePosition( const string& src)
{
	// note: the source path could have multiple trailing slashes.  We want to ignore those.

	unsigned int startpos = src.find_last_not_of( Delimiters );

	if(startpos == string::npos )
		return 0;

	int pos;

	if( startpos < src.length() )
	{
		string trimmed( src.begin(), src.begin()+startpos );
		pos = trimmed.find_last_of( Delimiters );
	}
	else
	{
		pos = src.find_last_of( Delimiters );
	}

	if( pos == string::npos )
		return 0;

	return pos;
}

void ReplaceFilename( string& dest, const string& src, const string& newfilename )
{
	int pos = _findFilenamePosition( src );
	
	if( pos == 0 )
		dest = src;
	else
		dest.assign( src.begin(), src.begin()+pos );

	if( !newfilename.empty() )
	{
		dest += '.';
		dest += newfilename;
	}
}

void GetFilename( const string& src, string& dest )
{
	int pos = _findFilenamePosition( src );
	dest.assign( src.begin()+pos, src.end() );
}

void GetDirectory( const string& src, string& dest )
{
	int pos = _findFilenamePosition( src );
	if( pos == 0 )
		dest.clear();
	else
		dest.assign( src.begin(), src.begin()+pos );
}

void Split( const string& src, string& destpath, string& destfile )
{
	int pos = _findFilenamePosition( src );

	if( pos == 0 )
	{
		destpath.clear();
		destfile = src;
	}
	else
	{
		destpath.assign( src.begin(), src.begin()+pos );
		destfile.assign( src.begin()+pos, src.end() );
	}
}

// Assigns the base/root directory of the given path into dest.
// Example /this/that/something.txt -> dest == "/"
void GetRootDirectory( const string& src, string& dest )
{
	int pos = src.find_first_of( Delimiters );
	if( pos == string::npos )
		dest.clear();
	else
		dest.assign( src.begin(), src.begin()+pos );
}

void CreateDirectory( const string& src )
{
#ifdef _WIN32
	_mkdir( src.c_str() );
#else
	mkdir( src.c_str(), 0755);
#endif
}

}

