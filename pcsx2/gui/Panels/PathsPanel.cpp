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
#include "ConfigurationPanels.h"

#include <wx/notebook.h>
#include <wx/stdpaths.h>

using namespace wxHelpers;
static const int BetweenFolderSpace = 5;

// ------------------------------------------------------------------------
wxDirPickerCtrl& Panels::PathsPanel::MyBasePanel::AddDirPicker( wxBoxSizer& sizer, int id, const wxDirName& defaultPath, const wxString& label, const wxString& dialogLabel, bool pathMustExist )
{
	// fixme: Should wxGTK (linux) force-enable the wxDIRP_USE_TEXTCTRL?  It's not "standard" on that platform
	// but it might still be preferred. - air

	wxDirName normalized( defaultPath );
	normalized.Normalize();

	wxDirPickerCtrl* jobe	= new wxDirPickerCtrl( this, id, normalized.ToString(), dialogLabel );
	wxStaticBoxSizer& s_box = *new wxStaticBoxSizer( wxVERTICAL, this, label );
	
	s_box.Add( jobe, wxSizerFlags().Border(wxLEFT | wxRIGHT | wxTOP, 5).Expand() );
	AddCheckBox( s_box, _("Use operating system default settings") );
	sizer.Add( &s_box, SizerFlags::StdGroupie() );
	return *jobe;
}

// ------------------------------------------------------------------------
Panels::PathsPanel::MyBasePanel::MyBasePanel( wxWindow& parent, int id ) :
	wxPanelWithHelpers( &parent, id )
{

}

// ------------------------------------------------------------------------
Panels::PathsPanel::StandardPanel::StandardPanel( wxWindow& parent, int id ) :
	MyBasePanel( parent, id )
{
	wxBoxSizer& s_main = *new wxBoxSizer( wxVERTICAL );

	AddDirPicker( s_main, wxID_ANY, g_Conf.Folders.Bios,		_("Bios:"),			_("Select folder with PS2 Bios") )
		.SetToolTip( pxE(Msg_Tooltips_Bios) );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, wxID_ANY, g_Conf.Folders.Savestates,	_("Savestates:"),	_("Select folder for Savestates") )
		.SetToolTip( pxE(Msg_Tooltips_Savestates) ); 

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, wxID_ANY, g_Conf.Folders.Snapshots,	_("Snapshots:"),	_("Select a folder for Snapshots") )
		.SetToolTip( pxE(Msg_Tooltips_Snapshots) ); 

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, wxID_ANY, g_Conf.Folders.Logs,		_("Log/Dumps:" ),	_("Select a folder for logs/dumps") )
		.SetToolTip( pxE(Msg_Tooltips_Logs) );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, wxID_ANY, g_Conf.Folders.MemoryCards, _("Memorycards:"),	_("Select a default Memorycards folder") )
		.SetToolTip( pxE(Msg_Tooltips_Memorycards) );

	s_main.AddSpacer( 5 );

	SetSizerAndFit( &s_main );
}

// ------------------------------------------------------------------------
Panels::PathsPanel::AdvancedPanel::AdvancedPanel( wxWindow& parent, int id ) :
	MyBasePanel( parent, id )
{
	wxBoxSizer& s_main = *new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer& advanced = *new wxStaticBoxSizer( wxVERTICAL, this, _("Advanced") );
	AddStaticText( advanced, pxE(Msg_Dialog_AdvancedPaths), 420, wxALIGN_CENTRE );

	AddDirPicker( advanced, wxID_ANY, g_Conf.Folders.Plugins,	_("Plugins:"),		_("Select folder for PCSX2 plugins") )
		.SetToolTip( pxE(Msg_Tooltips_PluginsPath) );

	advanced.AddSpacer( BetweenFolderSpace );
	AddDirPicker( advanced, wxID_ANY, g_Conf.Folders.Settings,	_("Settings:"),		_("Select a folder for PCSX2 settings/inis") )
		.SetToolTip( pxE(Msg_Tooltips_SettingsPath) );

	wxStaticBoxSizer& s_diag = *new wxStaticBoxSizer( wxVERTICAL, this, _("Default folder mode") );
	
	AddStaticText( s_diag,
		L"This setting only affects folders which are set to use the default folder configurations for your "
		L"operating system.  Any folders which are configured manually will override this option. ",
	400, wxALIGN_CENTRE );

	AddRadioButton( s_diag, _("Use to your Documents folder"),   _("Location: ") + wxStandardPaths::Get().GetDocumentsDir() );
	AddRadioButton( s_diag, _("Use the current working folder"), _("Location: ") + wxGetCwd() );

	s_diag.AddSpacer( 4 );

	advanced.AddSpacer( 4 );
	advanced.Add( &s_diag, SizerFlags::StdGroupie() );
	
	s_main.Add( &advanced, SizerFlags::StdGroupie() );
	s_main.AddSpacer( 5 );

	SetSizerAndFit( &s_main );
}

// ------------------------------------------------------------------------
Panels::PathsPanel::PathsPanel( wxWindow& parent, int id ) :
	wxPanelWithHelpers( &parent, id )
{
	wxBoxSizer& s_main = *new wxBoxSizer( wxVERTICAL );
	wxNotebook& notebook = *new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_BOTTOM | wxNB_FIXEDWIDTH );
	
	notebook.AddPage( new StandardPanel( notebook ), _("Standard") );
	notebook.AddPage( new AdvancedPanel( notebook ), _("Advanced") );

	s_main.Add( &notebook, SizerFlags::StdSpace() );

	SetSizerAndFit( &s_main );
}

