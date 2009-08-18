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

static wxString GetNormalizedConfigFolder( FoldersEnum_t folderId )
{
	const bool isDefault = g_Conf->Folders.IsDefault( folderId );
	wxDirName normalized( isDefault ? g_Conf->Folders[folderId] : PathDefs::Get(folderId) );
	normalized.Normalize( wxPATH_NORM_ALL );
	return normalized.ToString();
}	

// Pass me TRUE if the default path is to be used, and the DirPcikerCtrl disabled from use.
void Panels::DirPickerPanel::UpdateCheckStatus( bool someNoteworthyBoolean )
{
	m_pickerCtrl->Enable( !someNoteworthyBoolean );
	if( someNoteworthyBoolean )
	{
		wxDirName normalized( PathDefs::Get( m_FolderId ) );
		normalized.Normalize( wxPATH_NORM_ALL );
		m_pickerCtrl->SetPath( normalized.ToString() );
	}
}

void Panels::DirPickerPanel::UseDefaultPath_Click(wxCommandEvent &event)
{
	wxASSERT( m_pickerCtrl != NULL && m_checkCtrl != NULL );
	UpdateCheckStatus( m_checkCtrl->IsChecked() );
}

// ------------------------------------------------------------------------
// If initPath is NULL, then it's assumed the default folder is to be used, which is
// obtained from invoking the specified getDefault() function.
//
Panels::DirPickerPanel::DirPickerPanel( wxWindow* parent, FoldersEnum_t folderid,
		const wxString& label, const wxString& dialogLabel ) :
	BaseApplicableConfigPanel( parent, wxDefaultCoord )
,	m_FolderId( folderid )
{
	wxStaticBoxSizer& s_box = *new wxStaticBoxSizer( wxVERTICAL, this, label );

	// Force the Dir Picker to use a text control.  This isn't standard on Linux/GTK but it's much
	// more usable, so to hell with standards.

	m_pickerCtrl = new wxDirPickerCtrl( this, wxID_ANY, GetNormalizedConfigFolder( m_FolderId ), dialogLabel,
		wxDefaultPosition, wxDefaultSize, wxDIRP_USE_TEXTCTRL | wxDIRP_DIR_MUST_EXIST
	);

	s_box.Add( m_pickerCtrl, wxSizerFlags().Border(wxLEFT | wxRIGHT | wxTOP, 5).Expand() );
	m_checkCtrl = &AddCheckBox( s_box, _("Use installation default setting") );

	const bool isDefault = g_Conf->Folders.IsDefault( m_FolderId );
	m_checkCtrl->SetValue( isDefault );
	UpdateCheckStatus( isDefault );

	SetSizerAndFit( &s_box );

	Connect( m_checkCtrl->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DirPickerPanel::UseDefaultPath_Click ) );
}

void Panels::DirPickerPanel::Reset()
{
	m_pickerCtrl->SetPath( GetNormalizedConfigFolder( m_FolderId ) );
}

void Panels::DirPickerPanel::Apply( AppConfig& conf )
{
	conf.Folders.Set( m_FolderId, m_pickerCtrl->GetPath(), m_checkCtrl->GetValue() );
}

// ------------------------------------------------------------------------
Panels::BasePathsPanel::BasePathsPanel( wxWindow& parent, int idealWidth ) :
	wxPanelWithHelpers( &parent, idealWidth-12 )
,	s_main( *new wxBoxSizer( wxVERTICAL ) )
{
}

Panels::DirPickerPanel& Panels::BasePathsPanel::AddDirPicker( wxBoxSizer& sizer,
	FoldersEnum_t folderid, const wxString& label, const wxString& popupLabel )
{
	DirPickerPanel* dpan = new DirPickerPanel( this, folderid, label, popupLabel );
	sizer.Add( dpan, SizerFlags::SubGroup() );
	return *dpan;
}

// ------------------------------------------------------------------------
Panels::StandardPathsPanel::StandardPathsPanel( wxWindow& parent ) :
	BasePathsPanel( parent )
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
Panels::AdvancedPathsPanel::AdvancedPathsPanel( wxWindow& parent, int idealWidth ) :
	BasePathsPanel( parent, idealWidth-9 )

,	m_dirpick_plugins(
		AddDirPicker( s_main, FolderId_Plugins,
			_("Plugins folder:"),
			_("Select a PCSX2 plugins folder")
		)
	)

,	m_dirpick_settings( (
		s_main.AddSpacer( BetweenFolderSpace ),
		AddDirPicker( s_main, FolderId_Settings,
			_("Settings folder:"),
			_("Select location to save PCSX2 settings to")
		)
	) )
{
	m_dirpick_plugins.SetToolTip( pxE( ".Tooltips:Folders:Plugins",
		L"This is the location where PCSX2 will expect to find its plugins. Plugins found in this folder "
		L"will be enumerated and are selectable from the Plugins panel."
	) );

	m_dirpick_settings.SetToolTip( pxE( ".Tooltips:Folders:Settings",
		L"This is the folder where PCSX2 saves all settings, including settings generated "
		L"by most plugins.\n\nWarning: Some older versions of plugins may not respect this value."
	) );

	SetSizerAndFit( &s_main );
}

void Panels::AdvancedPathsPanel::Reset()
{
	m_dirpick_plugins.Reset();
	m_dirpick_settings.Reset();
}

// ------------------------------------------------------------------------
Panels::TabbedPathsPanel::TabbedPathsPanel( wxWindow& parent, int idealWidth ) :
	BaseApplicableConfigPanel( &parent, idealWidth )
{
	wxBoxSizer& s_main = *new wxBoxSizer( wxVERTICAL );
	wxNotebook& notebook = *new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_BOTTOM | wxNB_FIXEDWIDTH );

	StandardPathsPanel& stah( *new StandardPathsPanel( notebook ) );
	AdvancedPathsPanel& apah( *new AdvancedPathsPanel( notebook, GetIdealWidth() ) );
	notebook.AddPage( &stah,	_("Standard") );
	notebook.AddPage( &apah,	_("Advanced") );

	// Advanced Tab uses the Advanced Panel with some extra features.
	// This is because the extra features are not present on the Wizard version of the Advanced Panel.

	wxStaticBoxSizer& advanced = *new wxStaticBoxSizer( wxVERTICAL, this, _("Advanced") );
	AddStaticText( advanced, pxE( ".Panels:Folders:Advanced",
		L"Warning!! These advanced options are provided for developers and advanced testers only. "
		L"Changing these settings can cause program errors and may require administration privlidges, "
		L"so please be weary."
	) );

	advanced.Add( new UsermodeSelectionPanel( *this, GetIdealWidth()-9 ), SizerFlags::SubGroup() );
	advanced.AddSpacer( 4 );
	advanced.Add( &apah.Sizer(), SizerFlags::SubGroup() );

	apah.SetSizer( &advanced, false );
	apah.Fit();

	s_main.Add( &notebook, SizerFlags::StdSpace() );

	SetSizerAndFit( &s_main );
}

void Panels::TabbedPathsPanel::Apply( AppConfig& conf )
{
}
