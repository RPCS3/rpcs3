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
	BaseApplicableConfigPanel( parent, wxDefaultCoord )
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
	conf.Folders.Set( m_FolderId, m_pickerCtrl->GetPath(), m_checkCtrl->GetValue() );
}

// ------------------------------------------------------------------------
Panels::PathsPanel::MyBasePanel::MyBasePanel( wxWindow& parent, int idealWidth ) :
	wxPanelWithHelpers( &parent, idealWidth-12 )
,	s_main( *new wxBoxSizer( wxVERTICAL ) )
{
}

Panels::PathsPanel::DirPickerPanel& Panels::PathsPanel::MyBasePanel::AddDirPicker( wxBoxSizer& sizer,
	FoldersEnum_t folderid, const wxString& label, const wxString& popupLabel )
{
	DirPickerPanel* dpan = new DirPickerPanel( this, folderid, label, popupLabel );
	sizer.Add( dpan, SizerFlags::SubGroup() );
	return *dpan;
}

// ------------------------------------------------------------------------
Panels::PathsPanel::StandardPanel::StandardPanel( wxWindow& parent ) :
	MyBasePanel( parent )
{
	// TODO : Replace the callback mess here with the new FolderId enumeration setup. :)

	AddDirPicker( s_main, FolderId_Bios,
		_("Bios:"),
		_("Select folder with PS2 Bios") ).
		SetToolTip( pxE( ".Tooltips:Folders:Bios",
			L"This folder is where PCSX2 looks to find PS2 bios files.  The actual bios used can be "
			L"selected from the CPU dialog."
		) );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, FolderId_Savestates,
		_("Savestates:"),
		_("Select folder for Savestates") ).
		SetToolTip( pxE( ".Tooltips:Folders:Savestates",
			L"This folder is where PCSX2 records savestates; which are recorded either by using "
			L"menus/toolbars, or by pressing F1/F3 (load/save)."
		) );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, FolderId_Snapshots,
		_("Snapshots:"),
		_("Select a folder for Snapshots") ).
		SetToolTip( pxE( ".Tooltips:Folders:Snapshots",
			L"This folder is where PCSX2 saves screenshots.  Actual screenshot image format and style "
			L"may vary depending on the GS plugin being used." 
		) );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, FolderId_Logs,
		_("Logs/Dumps:" ),
		_("Select a folder for logs/dumps") ).
		SetToolTip( pxE( ".Tooltips:Folders:Logs",
			L"This folder is where PCSX2 saves its logfiles and diagnostic dumps.  Most plugins will "
			L"also adhere to this folder, however some older plugins may ignore it."
		) );

	s_main.AddSpacer( BetweenFolderSpace );
	AddDirPicker( s_main, FolderId_MemoryCards,
		_("Memorycards:"),
		_("Select a default Memorycards folder") ).
		SetToolTip( pxE( ".Tooltips:Folders:Memorycards",
			L"This is the default path where PCSX2 loads or creates its memory cards, and can be "
			L"overridden in the MemoryCard Configuration by using absolute filenames."
		) );

	s_main.AddSpacer( 5 );
	SetSizerAndFit( &s_main );
}

// ------------------------------------------------------------------------
Panels::PathsPanel::AdvancedPanel::AdvancedPanel( wxWindow& parent, int idealWidth ) :
	MyBasePanel( parent, idealWidth-9 )
{
	wxStaticBoxSizer& advanced = *new wxStaticBoxSizer( wxVERTICAL, this, _("Advanced") );
	AddStaticText( advanced, pxE( ".Panels:Folders:Advanced",
		L"Warning!! These advanced options are provided for developers and advanced testers only. "
		L"Changing these settings can cause program errors, so please be weary."
	) );

	AddDirPicker( advanced, FolderId_Plugins,
		_("Plugins:"),
		_("Select folder for PCSX2 plugins") ).
		SetToolTip( pxE( ".Tooltips:Folders:Plugins",
			L"This is the location where PCSX2 will expect to find its plugins. Plugins found in this folder "
			L"will be enumerated and are selectable from the Plugins panel."
		) );

	advanced.AddSpacer( BetweenFolderSpace );
	AddDirPicker( advanced, FolderId_Settings,
		_("Settings:"),
		_("Select a folder for PCSX2 settings/inis") ).
		SetToolTip( pxE( ".Tooltips:Folders:Settings",
			L"This is the folder where PCSX2 saves all settings, including settings generated "
			L"by most plugins.\n\nWarning: Some older versions of plugins may not respect this value."
		) );

	advanced.AddSpacer( 4 );
	advanced.Add( new UsermodeSelectionPanel( this, GetIdealWidth()-9 ), SizerFlags::SubGroup() );

	s_main.Add( &advanced, SizerFlags::SubGroup() );
	s_main.AddSpacer( 5 );

	SetSizerAndFit( &s_main );
}

// ------------------------------------------------------------------------
Panels::PathsPanel::PathsPanel( wxWindow& parent, int idealWidth ) :
	BaseApplicableConfigPanel( &parent, idealWidth )
{
	wxBoxSizer& s_main = *new wxBoxSizer( wxVERTICAL );
	wxNotebook& notebook = *new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_BOTTOM | wxNB_FIXEDWIDTH );

	notebook.AddPage( new StandardPanel( notebook ), _("Standard") );
	notebook.AddPage( new AdvancedPanel( notebook, GetIdealWidth() ), _("Advanced") );

	s_main.Add( &notebook, SizerFlags::StdSpace() );

	SetSizerAndFit( &s_main );
}

void Panels::PathsPanel::Apply( AppConfig& conf )
{
}
