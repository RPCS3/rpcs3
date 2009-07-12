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
#if defined(__WXMAC__) || defined(__WXMSW__)
	int orient = wxBK_TOP;
#else
	int orient = wxBK_LEFT;
#endif

	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );
	wxListbook& listbook = *new wxListbook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, orient );

	listbook.SetImageList( &wxGetApp().GetImgList_Config() );

	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	listbook.AddPage( new PathsPanel( listbook ), L"Paths", false, cfgid.Paths );
	listbook.AddPage( new PluginSelectorPanel( listbook ), L"Plugins", false, cfgid.Plugins );
	listbook.AddPage( new SpeedHacksPanel( listbook ), L"Speedhacks", true, cfgid.Speedhacks );
	listbook.AddPage( new GameFixesPanel( listbook ), L"Game Fixes", false, cfgid.Gamefixes );

	mainSizer.Add( &listbook );
	AddOkCancel( mainSizer, true );

	SetSizerAndFit( &mainSizer );

	Center( wxCENTER_ON_SCREEN | wxBOTH );
}

