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

// Pass me TRUE if the default path is to be used, and the DirPcikerCtrl disabled from use.
void Panels::PathsPanel::DirPickerPanel::UpdateCheckStatus( bool someNoteworthyBoolean )
{
	m_pickerCtrl->Enable( !someNoteworthyBoolean );
	if( someNoteworthyBoolean )
		m_pickerCtrl->SetPath( PathDefs::Get( m_FolderId ).ToString() );
}

void Panels::PathsPanel::DirPickerPanel::UseDefaultPath_Click(wxCommandEvent &event)
{
	wxASSERT( m_pickerCtrl != NULL && m_checkCtrl != NULL );
	UpdateCheckStatus( m_checkCtrl->IsChecked() );
}

// ------------------------------------------------------------------------
// If initPath is NULL, then it's assumed the default folder is to be used, which is
// obtained from invoking the specified getDefault() function.
//
Panels::PathsPanel::DirPickerPanel::DirPickerPanel( wxWindow* parent, FoldersEnum_t folderid,
		const wxString& label, const wxString& dialogLabel ) :
	BaseApplicableConfigPanel( parent )
,	m_FolderId( folderid )
{
	const bool isDefault = g_Conf->Folders.IsDefault( m_FolderId );
	wxDirName normalized( isDefault ? g_Conf->Folders[m_FolderId] : PathDefs::Get(m_FolderId) );
	normalized.Normalize();

	wxStaticBoxSizer& s_box = *new wxStaticBoxSizer( wxVERTICAL, this, label );

	// Force the Dir Picker to use a text control.  This isn't standard on Linux/GTK but it's much
	// more usable, so to hell with standards.

	m_pickerCtrl = new wxDirPickerCtrl( this, wxID_ANY, normalized.ToString(), dialogLabel,
		wxDefaultPosition, wxDefaultSize, wxDIRP_USE_TEXTCTRL | wxDIRP_DIR_MUST_EXIST
	);

	s_box.Add( m_pickerCtrl, wxSizerFlags().Border(wxLEFT | wxRIGHT | wxTOP, 5).Expand() );
	m_checkCtrl = &AddCheckBox( s_box, _("Use installation default setting") );
	m_checkCtrl->SetValue( isDefault );
	UpdateCheckStatus( isDefault );

	SetSizerAndFit( &s_box );

	Connect( m_checkCtrl->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PathsPanel::DirPickerPanel::UseDefaultPath_Click ) );
}

void Panels::PathsPanel::DirPickerPanel::Apply( AppConfig& conf )
{
	throw Exception::CannotApplySettings( this );
	conf.Folders.Set( m_FolderId, m_pickerCtrl->GetPath(), m_checkCtrl->GetValue() );
}

// ------------------------------------------------------------------------
Panels::PathsPanel::MyBasePanel::MyBasePanel( wxWindow& parent, int id ) :
	wxPanelWithHelpers( &parent, id )
,	s_main( *new wxBoxSizer( wxVERTICAL ) )
{
}

void Panels::PathsPanel::MyBasePanel::AddDirPicker( wxBoxSizer& sizer, FoldersEnum_t folderid, const wxString& label, const wxString& popupLabel, ExpandedMsgEnum tooltip )
{
	DirPickerPanel* dpan = new DirPickerPanel( this, folderid, label, popupLabel );
	dpan->SetToolTip( pxE(tooltip) );
	sizer.Add( dpan, SizerFlags::StdGroupie() );
}

// ------------------------------------------------------------------------
Panels::PathsPanel::StandardPanel::StandardPanel( wxWindow& parent, int id ) :
	MyBasePanel( parent, id )
{
	// TODO : Replace the callback mess here with the new FolderId enumeration setup. :)

	AddDirPicker( s_main, FolderId_Bios,
		_("Bios:"),
		_("Select folder with PS2 Bios"),	Msg_Tooltips_Bios );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, FolderId_Savestates,
		_("Savestates:"),
		_("Select folder for Savestates"),	Msg_Tooltips_Savestates );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, FolderId_Snapshots,
		_("Snapshots:"),
		_("Select a folder for Snapshots"),	Msg_Tooltips_Snapshots );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, FolderId_Logs,
		_("Logs/Dumps:" ),
		_("Select a folder for logs/dumps"), Msg_Tooltips_Logs );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, FolderId_MemoryCards,
		_("Memorycards:"),
		_("Select a default Memorycards folder"), Msg_Tooltips_Memorycards );

	s_main.AddSpacer( 5 );
	SetSizerAndFit( &s_main );

}

// ------------------------------------------------------------------------
Panels::PathsPanel::AdvancedPanel::AdvancedPanel( wxWindow& parent, int id ) :
	MyBasePanel( parent, id )
{
	wxStaticBoxSizer& advanced = *new wxStaticBoxSizer( wxVERTICAL, this, _("Advanced") );
	AddStaticText( advanced, pxE(Msg_Dialog_AdvancedPaths), 420, wxALIGN_CENTRE );

	AddDirPicker( advanced, FolderId_Plugins,
		_("Plugins:"),
		_("Select folder for PCSX2 plugins"), Msg_Tooltips_PluginsPath );

	advanced.AddSpacer( BetweenFolderSpace );
	AddDirPicker( advanced, FolderId_Settings,
		_("Settings:"),
		_("Select a folder for PCSX2 settings/inis"), Msg_Tooltips_SettingsPath );

	advanced.AddSpacer( 4 );
	advanced.Add( new UsermodeSelectionPanel( this ), SizerFlags::StdGroupie() );

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

void Panels::PathsPanel::Apply( AppConfig& conf )
{
}
