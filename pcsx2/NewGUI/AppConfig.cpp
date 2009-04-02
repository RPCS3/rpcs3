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
#include "App.h"

static const wxSize DefaultConsoleSize( 540, 540 );

AppConfig::AppConfig( const wxString& filename ) :
	wxFileConfig( wxEmptyString, wxEmptyString, filename, wxEmptyString, wxCONFIG_USE_RELATIVE_PATH )
{
	SetRecordDefaults();
	wxConfigBase::Set( this );
}

void AppConfig::LoadSettings()
{
	wxString spos( Read( wxT("ConLogDisplayPosition"), "docked" ) );

	if( spos == "docked" )
		ConLogBox.AutoDock = true;
	else
		ConLogBox.AutoDock = !TryParse( ConLogBox.DisplayPos, spos );

	TryParse( ConLogBox.DisplaySize, Read( wxT("ConLogDisplaySize"), ToString( DefaultConsoleSize ) ), DefaultConsoleSize );
	Read( wxT( "ConLogVisible" ), &ConLogBox.Show, IsDevBuild );

	TryParse( MainGuiPosition, Read( wxT("MainWindowPosition"), ToString( wxDefaultPosition ) ), wxDefaultPosition );

	Flush();
}

void AppConfig::SaveSettings()
{
	Write( wxT("ConLogDisplayPosition"), ConLogBox.AutoDock ? "docked" : ToString( ConLogBox.DisplayPos ) );
}
