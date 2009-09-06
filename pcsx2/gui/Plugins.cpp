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
#include "Utilities/ScopedPtr.h"

#include <wx/dir.h>
#include <wx/file.h>

#include "Plugins.h"
#include "GS.h"
#include "HostGui.h"
#include "AppConfig.h"

int EnumeratePluginsInFolder( const wxDirName& searchpath, wxArrayString* dest )
{
	wxScopedPtr<wxArrayString> placebo;
	wxArrayString* realdest = dest;
	if( realdest == NULL )
		placebo.reset( realdest = new wxArrayString() );

#ifdef __WXMSW__
	// Windows pretty well has a strict "must end in .dll" rule.
	wxString pattern( L"*%s" );
#else
	// Other platforms seem to like to version their libs after the .so extension:
	//    blah.so.3.1.fail?
	wxString pattern( L"*%s*" );
#endif

	return searchpath.Exists() ?
		wxDir::GetAllFiles( searchpath.ToString(), realdest, wxsFormat( pattern, wxDynamicLibrary::GetDllExt()), wxDIR_FILES ) : 0;
}

void LoadPlugins()
{
	if( g_plugins != NULL ) return;
	wxString passins[PluginId_Count];

	const PluginInfo* pi = tbl_PluginInfo-1;
	while( ++pi, pi->shortname != NULL )
		passins[pi->id] = g_Conf->FullpathTo( pi->id );

	g_plugins = PluginManager_Create( passins );
}

void InitPlugins()
{
	if( g_plugins == NULL )
		LoadPlugins();

	GetPluginManager().Init();
}

// All plugins except the CDVD are opened here.  The CDVD must be opened manually by
// the GUI, depending on the user's menu/config in use.
//
// Exceptions:
//   PluginFailure - if any plugin fails to initialize or open.
//   ThreadCreationError - If the MTGS thread fails to be created.
//
void OpenPlugins()
{
	if( g_plugins == NULL )
		InitPlugins();

	GetPluginManager().Open();
}
