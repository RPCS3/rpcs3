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
#include "IniInterface.h"

#include <wx/gdicmn.h>

const wxRect wxDefaultRect( wxDefaultCoord, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord );

// Implement FixedPointTypes (for lack of a better location, for now)

#include "FixedPointTypes.inl"
template struct FixedInt<100>;
template struct FixedInt<256>;

wxDirName g_fullBaseDirName = wxDirName(L"");
void SetFullBaseDir( wxDirName appRoot )
{
	g_fullBaseDirName = appRoot;
}

static int _calcEnumLength( const wxChar* const* enumArray )
{
	int cnt = 0;
	while( *enumArray != NULL )
	{
		enumArray++;
		cnt++;
	}

	return cnt;
}

ScopedIniGroup::ScopedIniGroup( IniInterface& mommy, const wxString& group )
	: m_mom( mommy )
{
	pxAssertDev( wxStringTokenize( group, L"/" ).Count() <= 1, L"Cannot nest more than one group deep per instance of ScopedIniGroup." );
	m_mom.SetPath( group );
}

ScopedIniGroup::~ScopedIniGroup()
{
	m_mom.SetPath( L".." );
}

// --------------------------------------------------------------------------------------
//  IniInterface (implementations)
// --------------------------------------------------------------------------------------
IniInterface::IniInterface( wxConfigBase& config )
{
	m_Config = &config;
}

IniInterface::IniInterface( wxConfigBase* config )
{
	m_Config = config;
}

IniInterface::IniInterface()
{
	m_Config = wxConfigBase::Get( false );
}

IniInterface::~IniInterface()
{
	Flush();
}

void IniInterface::SetPath( const wxString& path )
{
	if( m_Config ) m_Config->SetPath( path );
}

void IniInterface::Flush()
{
	if( m_Config ) m_Config->Flush();
}


// --------------------------------------------------------------------------------------
//  IniLoader  (implementations)
// --------------------------------------------------------------------------------------
IniLoader::IniLoader( wxConfigBase& config ) : IniInterface( config ) { }
IniLoader::IniLoader( wxConfigBase* config ) : IniInterface( config ) { }

IniLoader::IniLoader() : IniInterface() {}
IniLoader::~IniLoader() throw() {}


void IniLoader::Entry( const wxString& var, wxString& value, const wxString defvalue )
{
	if( m_Config )
		m_Config->Read( var, &value, defvalue );
	else
		value = defvalue;
}

void IniLoader::Entry( const wxString& var, wxDirName& value, const wxDirName defvalue )
{
	wxString dest;
	if( m_Config ) m_Config->Read( var, &dest, wxEmptyString );

	if( dest.IsEmpty() )
		value = defvalue;
	else
		value = dest;
}

void IniLoader::Entry( const wxString& var, wxFileName& value, const wxFileName defvalue, bool isAllowRelative )
{
	wxString dest( defvalue.GetFullPath() );
	if( m_Config ) m_Config->Read( var, &dest, defvalue.GetFullPath() );
	value = dest;
	if( isAllowRelative && value.IsRelative() )
		value = g_fullBaseDirName+value;

	if( value.IsAbsolute() )
		value.Normalize();

	if (value.HasVolume())
		value.SetVolume(value.GetVolume().Upper());
}

void IniLoader::Entry( const wxString& var, int& value, const int defvalue )
{
	if( m_Config )
		m_Config->Read( var, &value, defvalue );
	else
		value = defvalue;
}

void IniLoader::Entry( const wxString& var, uint& value, const uint defvalue )
{
	if( m_Config )
		m_Config->Read( var, (int*)&value, (int)defvalue );
	else
		value = defvalue;
}

void IniLoader::Entry( const wxString& var, bool& value, const bool defvalue )
{
	// TODO : Stricter value checking on enabled/disabled?
	wxString dest(defvalue ? L"enabled" : L"disabled");
	if( m_Config ) m_Config->Read( var, &dest, dest );
	value = (dest == L"enabled") || (dest == L"1");
}

bool IniLoader::EntryBitBool( const wxString& var, bool value, const bool defvalue )
{
	// Note: 'value' param is used by inisaver only.
	bool result;
	Entry( var, result, defvalue );
	return result;
}

int IniLoader::EntryBitfield( const wxString& var, int value, const int defvalue )
{
	int result;
	Entry( var, result, defvalue );
	return result;
}

void IniLoader::Entry( const wxString& var, Fixed100& value, const Fixed100 defvalue )
{
	// Note: the "easy" way would be to convert to double and load/save that, but floating point
	// has way too much rounding error so we really need to do things out manually.. >_<

	wxString readval( value.ToString() );
	if( m_Config ) m_Config->Read( var, &readval );
	value = Fixed100::FromString( readval, value );
}

void IniLoader::Entry( const wxString& var, wxPoint& value, const wxPoint defvalue )
{
	if( !m_Config )
	{
		value = defvalue; return;
	}
	TryParse( value, m_Config->Read( var, ToString( defvalue ) ), defvalue );
}

void IniLoader::Entry( const wxString& var, wxSize& value, const wxSize defvalue )
{
	if( !m_Config )
	{
		value = defvalue; return;
	}
	TryParse( value, m_Config->Read( var, ToString( defvalue ) ), defvalue );
}

void IniLoader::Entry( const wxString& var, wxRect& value, const wxRect defvalue )
{
	if( !m_Config )
	{
		value = defvalue; return;
	}
	TryParse( value, m_Config->Read( var, ToString( defvalue ) ), defvalue );
}

void IniLoader::_EnumEntry( const wxString& var, int& value, const wxChar* const* enumArray, int defvalue )
{
	// Confirm default value sanity...

	const int cnt = _calcEnumLength( enumArray );
	if( !IndexBoundsCheck( L"IniLoader EnumDefaultValue", defvalue, cnt ) )
	{
		Console.Error( "(LoadSettings) Default enumeration index is out of bounds. Truncating." );
		defvalue = cnt-1;
	}

	// Sanity confirmed, proceed with craziness!

	if( !m_Config )
	{
		value = defvalue;
		return;
	}

	wxString retval;
	m_Config->Read( var, &retval, enumArray[defvalue] );

	int i=0;
	while( enumArray[i] != NULL && ( retval != enumArray[i] ) ) i++;

	if( enumArray[i] == NULL )
	{
		Console.Warning( L"(LoadSettings) Warning: Unrecognized value '%s' on key '%s'\n\tUsing the default setting of '%s'.",
			retval.c_str(), var.c_str(), enumArray[defvalue]
		);
		value = defvalue;
	}
	else
		value = i;
}

// --------------------------------------------------------------------------------------
//  IniSaver  (implementations)
// --------------------------------------------------------------------------------------

IniSaver::IniSaver( wxConfigBase& config ) : IniInterface( config ) { }
IniSaver::IniSaver( wxConfigBase* config ) : IniInterface( config ) { }


IniSaver::IniSaver() : IniInterface() {}
IniSaver::~IniSaver() {}

void IniSaver::Entry( const wxString& var, wxString& value, const wxString defvalue )
{
	if( !m_Config ) return;
	m_Config->Write( var, value );
}

void IniSaver::Entry( const wxString& var, wxDirName& value, const wxDirName defvalue )
{
	if( !m_Config ) return;

	/*if( value == defvalue )
		m_Config->Write( var, wxString() );
	else*/
		m_Config->Write( var, value.ToString() );
}

//If isAllowRelative is true, we're saving as relative if the file is somewhere inside the PARENT of pcsx2 folder.
//When a file is saved as relative, it's always relative to pcsx2 main folder (even if the file is outside of it).
//e.g. at the next folder structure, files at ISOs_2 and ISOs_3 will be saved relative, but files at ISOs_1 will be saved absolute.
//  -root
//   |-ISOs_1
//   |-parent_of_pcsx2_folder
//     |-ISOs_2
//     |-pcsx2_folder
//       |-pcsx2.exe
//       |-plugins
//       | |-...
//       |-ISOs_3
void IniSaver::Entry( const wxString& var, wxFileName& value, const wxFileName defvalue, bool isAllowRelative )
{
	if( !m_Config ) return;
	wxFileName res(value);

	if ( res.IsAbsolute() )
		res.Normalize();
	
	wxDirName upper( g_fullBaseDirName );
	upper.RemoveLast();
	if( isAllowRelative && upper.IsContains( value ) )
		res.MakeRelativeTo(g_fullBaseDirName.ToString());
	
	m_Config->Write( var, res.GetFullPath() );
}

void IniSaver::Entry( const wxString& var, int& value, const int defvalue )
{
	if( !m_Config ) return;
	m_Config->Write( var, value );
}

void IniSaver::Entry( const wxString& var, uint& value, const uint defvalue )
{
	if( !m_Config ) return;
	m_Config->Write( var, (int)value );
}

void IniSaver::Entry( const wxString& var, bool& value, const bool defvalue )
{
	if( !m_Config ) return;
	m_Config->Write( var, value ? L"enabled" : L"disabled" );
}

bool IniSaver::EntryBitBool( const wxString& var, bool value, const bool defvalue )
{
	if( m_Config ) m_Config->Write( var, value ? L"enabled" : L"disabled" );
	return value;
}

int IniSaver::EntryBitfield( const wxString& var, int value, const int defvalue )
{
	if( m_Config ) m_Config->Write( var, value );
	return value;
}

void IniSaver::Entry( const wxString& var, Fixed100& value, const Fixed100 defvalue )
{
	if( !m_Config ) return;

	// Note: the "easy" way would be to convert to double and load/save that, but floating point
	// has way too much rounding error so we really need to do things out manually, using strings.

	m_Config->Write( var, value.ToString() );
}

void IniSaver::Entry( const wxString& var, wxPoint& value, const wxPoint defvalue )
{
	if( !m_Config ) return;
	m_Config->Write( var, ToString( value ) );
}

void IniSaver::Entry( const wxString& var, wxSize& value, const wxSize defvalue )
{
	if( !m_Config ) return;
	m_Config->Write( var, ToString( value ) );
}

void IniSaver::Entry( const wxString& var, wxRect& value, const wxRect defvalue )
{
	if( !m_Config ) return;
	m_Config->Write( var, ToString( value ) );
}

void IniSaver::_EnumEntry( const wxString& var, int& value, const wxChar* const* enumArray, int defvalue )
{
	const int cnt = _calcEnumLength( enumArray );

	// Confirm default value sanity...

	if( !IndexBoundsCheck( L"IniSaver EnumDefaultValue", defvalue, cnt ) )
	{
		Console.Error( "(SaveSettings) Default enumeration index is out of bounds. Truncating." );
		defvalue = cnt-1;
	}

	if( !m_Config ) return;

	if( value >= cnt )
	{
		Console.Warning(  L"(SaveSettings) An illegal enumerated index was detected when saving '%s'", var.c_str() );
		Console.Indent().Warning(
			L"Illegal Value: %d\n"
			L"Using Default: %d (%s)\n",
			value, defvalue, enumArray[defvalue]
		);

		// Cause a debug assertion, since this is a fully recoverable error.
		pxAssert( value < cnt );

		value = defvalue;
	}

	m_Config->Write( var, enumArray[value] );
}

