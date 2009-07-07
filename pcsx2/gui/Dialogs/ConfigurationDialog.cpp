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
#include "Misc.h"
#include "System.h"
#include "App.h"

#include "ConfigurationDialog.h"
#include "Panels/ConfigurationPanels.h"

#include <wx/bookctrl.h>
#include <wx/artprov.h>
#include <wx/listbook.h>

using namespace wxHelpers;
using namespace Panels;

Dialogs::ConfigurationDialog::ConfigurationDialog( wxWindow* parent, int id ) :
	wxDialogWithHelpers( parent, id, _T("PCSX2 Configuration"), true )
{
	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );
	wxListbook& Notebook = *new wxListbook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLB_TOP );

	Notebook.SetImageList( &wxGetApp().GetImgList_Config() );

	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	Notebook.AddPage( new PathsPanel( Notebook ), L"Paths", false, cfgid.Paths );
	//Notebook->AddPage( new PluginSelectionPanel( Notebook ), L"Plugins" );
	Notebook.AddPage( new SpeedHacksPanel( Notebook ), L"Speedhacks", true, cfgid.Speedhacks );
	Notebook.AddPage( new GameFixesPanel( Notebook ), L"Game Fixes", false, cfgid.Gamefixes );

	mainSizer.Add( &Notebook );
	AddOkCancel( mainSizer );

	SetSizerAndFit( &mainSizer );

	Center();
}

