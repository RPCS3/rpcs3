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

#include <wx/config.h>

//////////////////////////////////////////////////////////////////////////////////////////
// IniInterface class (abstract base class)
//
// This is used as an interchangable interface for both loading and saving options from an
// ini/configuration file.  The LoadSave code takes an IniInterface, and the interface
// implementation defines whether the options are read or written.
//
// See also: IniLoader, IniSaver
//
class IniInterface
{
protected:
	wxConfigBase& m_Config;

public:
	virtual ~IniInterface();
	explicit IniInterface();
	explicit IniInterface( wxConfigBase& config );

	void SetPath( const wxString& path );
	void Flush();
	
	virtual void Entry( const wxString& var, wxString& value, const wxString& defvalue=wxString() )=0;
	virtual void Entry( const wxString& var, int& value, const int defvalue=0 )=0;
	virtual void Entry( const wxString& var, uint& value, const uint defvalue=0 )=0;
	virtual void Entry( const wxString& var, bool& value, const bool defvalue=0 )=0;

	virtual void Entry( const wxString& var, wxPoint& value, const wxPoint& defvalue=wxDefaultPosition )=0;
	virtual void Entry( const wxString& var, wxSize& value, const wxSize& defvalue=wxDefaultSize )=0;
	virtual void Entry( const wxString& var, wxRect& value, const wxRect& defvalue=wxDefaultRect )=0;

	virtual void EnumEntry( const wxString& var, int& value, const wxChar* const* enumArray, const int defvalue=0 )=0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// IniLoader class
//
// Implementation of the IniInterface base class, which maps ini actions to loading from
// an ini source file.
//
// See also: IniInterface
//
class IniLoader : public IniInterface
{
public:
	virtual ~IniLoader();
	explicit IniLoader();
	explicit IniLoader( wxConfigBase& config );

	void Entry( const wxString& var, wxString& value, const wxString& defvalue=wxString() );
	void Entry( const wxString& var, int& value, const int defvalue=0 );
	void Entry( const wxString& var, uint& value, const uint defvalue=0 );
	void Entry( const wxString& var, bool& value, const bool defvalue=false );

	void Entry( const wxString& var, wxPoint& value, const wxPoint& defvalue=wxDefaultPosition );
	void Entry( const wxString& var, wxSize& value, const wxSize& defvalue=wxDefaultSize );
	void Entry( const wxString& var, wxRect& value, const wxRect& defvalue=wxDefaultRect );

	void EnumEntry( const wxString& var, int& value, const wxChar* const* enumArray, const int defvalue=0 );
};

//////////////////////////////////////////////////////////////////////////////////////////
// IniSaver class
//
// Implementation of the IniInterface base class, which maps ini actions to saving to
// an ini dest file.
//
// See also: IniInterface
//
class IniSaver : public IniInterface
{
public:
	virtual ~IniSaver();
	explicit IniSaver();
	explicit IniSaver( wxConfigBase& config );

	void Entry( const wxString& var, wxString& value, const wxString& defvalue=wxString() );
	void Entry( const wxString& var, int& value, const int defvalue=0 );
	void Entry( const wxString& var, uint& value, const uint defvalue=0 );
	void Entry( const wxString& var, bool& value, const bool defvalue=false );

	void Entry( const wxString& var, wxPoint& value, const wxPoint& defvalue=wxDefaultPosition );
	void Entry( const wxString& var, wxSize& value, const wxSize& defvalue=wxDefaultSize );
	void Entry( const wxString& var, wxRect& value, const wxRect& defvalue=wxDefaultRect );

	void EnumEntry( const wxString& var, int& value, const wxChar* const* enumArray, const int defvalue=0 );
};
