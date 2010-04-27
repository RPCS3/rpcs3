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

#pragma once

#include <wx/filename.h>
#include "StringHelpers.h"

#define g_MaxPath 255			// 255 is safer with antiquated Win32 ASCII APIs.

//////////////////////////////////////////////////////////////////////////////////////////
//
class wxDirName : protected wxFileName
{
public:
	explicit wxDirName( const wxFileName& src )
	{
		Assign( src.GetPath(), wxEmptyString );
	}

	wxDirName() : wxFileName() {}
	wxDirName( const wxDirName& src ) : wxFileName( src ) { }
	explicit wxDirName( const char* src ) { Assign( fromUTF8(src) ); }
	explicit wxDirName( const wxString& src ) { Assign( src ); }

	// ------------------------------------------------------------------------
	void Assign( const wxString& volume, const wxString& path )
	{
		wxFileName::Assign( volume, path, wxEmptyString );
	}

	void Assign( const wxString& path )
	{
		wxFileName::Assign( path, wxEmptyString );
	}

	void Assign( const wxDirName& path )
	{
		wxFileName::Assign( path );
	}

	void Clear() { wxFileName::Clear(); }

	wxCharBuffer ToAscii() const { return GetPath().ToAscii(); }
	wxString ToString() const { return GetPath(); }

	// ------------------------------------------------------------------------
	bool IsWritable() const { return IsDirWritable(); }
	bool IsReadable() const { return IsDirReadable(); }
	bool Exists() const { return DirExists(); }
	bool FileExists() const { return wxFileName::FileExists(); }
	bool IsOk() const { return wxFileName::IsOk(); }
	bool IsRelative() const { return wxFileName::IsRelative(); }
	bool IsAbsolute() const { return wxFileName::IsAbsolute(); }

	bool SameAs( const wxDirName& filepath ) const
	{
		return wxFileName::SameAs( filepath );
	}

	// Returns the number of sub folders in this directory path
	size_t GetCount() const { return GetDirCount(); }

	// ------------------------------------------------------------------------
	wxFileName Combine( const wxFileName& right ) const;
	wxDirName Combine( const wxDirName& right ) const;

	// removes the lastmost directory from the path
	void RemoveLast() { wxFileName::RemoveDir(GetCount() - 1); }

	wxDirName& Normalize( int flags = wxPATH_NORM_ALL, const wxString& cwd = wxEmptyString );
	wxDirName& MakeRelativeTo( const wxString& pathBase = wxEmptyString );
	wxDirName& MakeAbsolute( const wxString& cwd = wxEmptyString );

	// ------------------------------------------------------------------------

	void AssignCwd( const wxString& volume = wxEmptyString ) { wxFileName::AssignCwd( volume ); }
	bool SetCwd() { return wxFileName::SetCwd(); }

	// wxWidgets is missing the const qualifier for this one!  Shame!
	void Rmdir();
	bool Mkdir();

	// ------------------------------------------------------------------------

	wxDirName& operator=(const wxDirName& dirname)	{ Assign( dirname ); return *this; }
	wxDirName& operator=(const wxString& dirname)	{ Assign( dirname ); return *this; }
	wxDirName& operator=(const char* dirname)		{ Assign( fromUTF8(dirname) ); return *this; }

	wxFileName operator+( const wxFileName& right ) const	{ return Combine( right ); }
	wxDirName operator+( const wxDirName& right )  const	{ return Combine( right ); }

	bool operator==(const wxDirName& filename) const { return SameAs(filename); }
	bool operator!=(const wxDirName& filename) const { return !SameAs(filename); }

	bool operator==(const wxFileName& filename) const { return SameAs(wxDirName(filename)); }
	bool operator!=(const wxFileName& filename) const { return !SameAs(wxDirName(filename)); }

	// compare with a filename string interpreted as a native file name
	bool operator==(const wxString& filename) const { return SameAs(wxDirName(filename)); }
	bool operator!=(const wxString& filename) const { return !SameAs(wxDirName(filename)); }

	const wxFileName& GetFilename() const { return *this; }
	wxFileName& GetFilename() { return *this; }
};

// --------------------------------------------------------------------------------------
//  Path Namespace
// --------------------------------------------------------------------------------------
// Cross-platform utilities for manipulation of paths and filenames.  Mostly these fall
// back on wxWidgets APIs internally, but are still helpful because some of wx's file stuff
// has minor glitches, or requies sloppy wxFileName typecasting.
//
namespace Path
{
	extern bool		IsRelative( const wxString& path );
	extern s64		GetFileSize( const wxString& path );

	extern wxString Normalize( const wxString& srcpath );
	extern wxString Normalize( wxDirName srcpath );

	extern wxString	Combine( const wxString& srcPath, const wxString& srcFile );
	extern wxString	Combine( const wxDirName& srcPath, const wxFileName& srcFile );
	extern wxString	Combine( const wxString& srcPath, const wxDirName& srcFile );
	extern wxString	ReplaceExtension( const wxString& src, const wxString& ext );
	extern wxString	ReplaceFilename( const wxString& src, const wxString& newfilename );
	extern wxString	GetFilename( const wxString& src );
	extern wxString	GetDirectory( const wxString& src );
	extern wxString	GetFilenameWithoutExt( const wxString& src );
	extern wxString	GetRootDirectory( const wxString& src );
}
