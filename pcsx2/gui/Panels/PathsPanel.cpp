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

void Panels::PathsPanel::DirPickerPanel::UseDefaultPath_Click(wxCommandEvent &event)
{
	wxASSERT( m_pickerCtrl != NULL && m_checkCtrl != NULL );
	m_pickerCtrl->Enable( !m_checkCtrl->IsChecked() );
	m_pickerCtrl->SetPath( m_GetDefaultFunc().ToString() );
}

// ------------------------------------------------------------------------
Panels::PathsPanel::DirPickerPanel::DirPickerPanel( wxWindow* parent, const wxDirName& initPath, wxDirName (*getDefault)(),
		const wxString& label, const wxString& dialogLabel ) :
	wxPanelWithHelpers( parent, wxID_ANY )
,	m_GetDefaultFunc( getDefault )
{
	wxDirName normalized( initPath );
	normalized.Normalize();

	wxStaticBoxSizer& s_box = *new wxStaticBoxSizer( wxVERTICAL, this, label );

	// Force the Dir Picker to use a text control.  This isn't standard on Linux/GTK but it's much
	// more usable, so to hell with standards.

	m_pickerCtrl = new wxDirPickerCtrl( this, wxID_ANY, normalized.ToString(), dialogLabel,
		wxDefaultPosition, wxDefaultSize, wxDIRP_USE_TEXTCTRL | wxDIRP_DIR_MUST_EXIST
	);

	s_box.Add( m_pickerCtrl, wxSizerFlags().Border(wxLEFT | wxRIGHT | wxTOP, 5).Expand() );
	m_checkCtrl = &AddCheckBox( s_box, _("Use operating system default settings") );

	SetSizerAndFit( &s_box );

	Connect( m_checkCtrl->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PathsPanel::DirPickerPanel::UseDefaultPath_Click ) );
}

// ------------------------------------------------------------------------
Panels::PathsPanel::MyBasePanel::MyBasePanel( wxWindow& parent, int id ) :
	wxPanelWithHelpers( &parent, id )
,	s_main( *new wxBoxSizer( wxVERTICAL ) )
{
}

void Panels::PathsPanel::MyBasePanel::AddDirPicker( wxBoxSizer& sizer, const wxDirName& initPath, wxDirName (*getDefaultFunc)(), const wxString& label, const wxString& popupLabel, ExpandedMsgEnum tooltip )
{
	DirPickerPanel* dpan = new DirPickerPanel( this, initPath, getDefaultFunc, label, popupLabel );
	dpan->SetToolTip( pxE(tooltip) );
	sizer.Add( dpan, SizerFlags::StdGroupie() );
}

// ------------------------------------------------------------------------
Panels::PathsPanel::StandardPanel::StandardPanel( wxWindow& parent, int id ) :
	MyBasePanel( parent, id )
{
	AddDirPicker( s_main, g_Conf.Folders.Bios, PathDefs::GetBios,
		_("Bios:"),			_("Select folder with PS2 Bios"),	Msg_Tooltips_Bios );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, g_Conf.Folders.Savestates, PathDefs::GetSavestates,
		_("Savestates:"),	_("Select folder for Savestates"),	Msg_Tooltips_Savestates );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, g_Conf.Folders.Snapshots, PathDefs::GetSnapshots,
		_("Snapshots:"),	_("Select a folder for Snapshots"),	Msg_Tooltips_Snapshots );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, g_Conf.Folders.Logs, PathDefs::GetLogs,
		_("Logs/Dumps:" ),	_("Select a folder for logs/dumps"), Msg_Tooltips_Logs );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, g_Conf.Folders.MemoryCards, PathDefs::GetMemoryCards,
		_("Memorycards:"),	_("Select a default Memorycards folder"), Msg_Tooltips_Memorycards );

	s_main.AddSpacer( 5 );
	SetSizerAndFit( &s_main );

}

// ------------------------------------------------------------------------
Panels::PathsPanel::AdvancedPanel::AdvancedPanel( wxWindow& parent, int id ) :
	MyBasePanel( parent, id )
{
	wxStaticBoxSizer& advanced = *new wxStaticBoxSizer( wxVERTICAL, this, _("Advanced") );
	AddStaticText( advanced, pxE(Msg_Dialog_AdvancedPaths), 420, wxALIGN_CENTRE );

	AddDirPicker( advanced, g_Conf.Folders.Plugins, PathDefs::GetPlugins,
		_("Plugins:"),		_("Select folder for PCSX2 plugins"), Msg_Tooltips_PluginsPath );

	advanced.AddSpacer( BetweenFolderSpace );
	AddDirPicker( advanced, g_Conf.Folders.Settings, PathDefs::GetSettings,
		_("Settings:"),		_("Select a folder for PCSX2 settings/inis"), Msg_Tooltips_SettingsPath );

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
Panels::PathsPanel::PathsPanel( wxWindow& parent ) :
	BaseApplicableConfigPanel( &parent )
{
	wxBoxSizer& s_main = *new wxBoxSizer( wxVERTICAL );
	wxNotebook& notebook = *new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_BOTTOM | wxNB_FIXEDWIDTH );

	notebook.AddPage( new StandardPanel( notebook ), _("Standard") );
	notebook.AddPage( new AdvancedPanel( notebook ), _("Advanced") );

	s_main.Add( &notebook, SizerFlags::StdSpace() );

	SetSizerAndFit( &s_main );
}

bool Panels::PathsPanel::Apply( AppConfig& conf )
{
	return true;
}
