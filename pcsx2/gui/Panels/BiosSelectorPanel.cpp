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
#include "App.h"
#include "ConfigurationPanels.h"

#include "ps2/BiosTools.h"

#include <wx/dir.h>
#include <wx/filepicker.h>
#include <wx/listbox.h>

using namespace pxSizerFlags;

// =====================================================================================================
//  BaseSelectorPanel
// =====================================================================================================
// This base class provides event hookups and virtual functions for enumerating items in a folder.
// The most important feature of this base panel is that enumeration is done when the panel is first
// *shown*, not when it is created.  This functionality allows the panel to work either as a stand alone
// dialog, a child of a complex tabbed dialog, and as a member of a wxWizard!
//
// In addition, this panel automatically intercepts and responds to DIRPICKER messages, so that your
// panel may provide a dir picker to the user.
//
// [TODO] : wxWidgets 2.9.1 provides a class for watching directory contents changes.  When PCSX2 is
// upgraded to wx2.9/3.0, it should incorporate such functionality into this base class.  (for now
// we just provide the user with a "refresh" button).
//
Panels::BaseSelectorPanel::BaseSelectorPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent, wxVERTICAL )
{
	Connect( wxEVT_COMMAND_DIRPICKER_CHANGED,	wxFileDirPickerEventHandler	(BaseSelectorPanel::OnFolderChanged) );
	Connect( wxEVT_SHOW,						wxShowEventHandler			(BaseSelectorPanel::OnShow) );
}

Panels::BaseSelectorPanel::~BaseSelectorPanel() throw()
{
}

void Panels::BaseSelectorPanel::OnShow(wxShowEvent& evt)
{
	evt.Skip();
	if( !evt.GetShow() ) return;
	OnShown();
}

void Panels::BaseSelectorPanel::OnShown()
{
	if( !ValidateEnumerationStatus() )
		DoRefresh();
}

bool Panels::BaseSelectorPanel::Show( bool visible )
{
	if( visible )
		OnShown();

	return BaseApplicableConfigPanel::Show( visible );
}

void Panels::BaseSelectorPanel::RefreshSelections()
{
	ValidateEnumerationStatus();
	DoRefresh();
}

void Panels::BaseSelectorPanel::OnRefreshSelections( wxCommandEvent& evt )
{
	evt.Skip();
	RefreshSelections();
}

void Panels::BaseSelectorPanel::OnFolderChanged( wxFileDirPickerEvent& evt )
{
	evt.Skip();
	OnShown();
}

// =====================================================================================================
//  BiosSelectorPanel
// =====================================================================================================
Panels::BiosSelectorPanel::BiosSelectorPanel( wxWindow* parent )
	: BaseSelectorPanel( parent )
{
	SetMinWidth( 480 );

	m_ComboBox		= new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE | wxLB_SORT | wxLB_NEEDED_SB );
	m_FolderPicker	= new DirPickerPanel( this, FolderId_Bios,
		_("BIOS Search Path:"),						// static box label
		_("Select folder with PS2 BIOS roms")		// dir picker popup label
	);

	m_ComboBox->SetFont( wxFont( m_ComboBox->GetFont().GetPointSize()+1, wxFONTFAMILY_MODERN, wxNORMAL, wxNORMAL, false, L"Lucida Console" ) );
	m_ComboBox->SetMinSize( wxSize( wxDefaultCoord, std::max( m_ComboBox->GetMinSize().GetHeight(), 96 ) ) );
	
	if (InstallationMode != InstallMode_Portable)
		m_FolderPicker->SetStaticDesc( _("Click the Browse button to select a different folder where PCSX2 will look for PS2 BIOS roms.") );

	wxButton* refreshButton = new wxButton( this, wxID_ANY, _("Refresh list") );

	*this	+= Label(_("Select a BIOS rom:"));
	*this	+= m_ComboBox		| StdExpand();
	*this	+= refreshButton	| pxBorder(wxLEFT, StdPadding);
	*this	+= 8;
	*this	+= m_FolderPicker	| StdExpand();

	Connect( refreshButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(BiosSelectorPanel::OnRefreshSelections) );
}

Panels::BiosSelectorPanel::~BiosSelectorPanel() throw ()
{
}

void Panels::BiosSelectorPanel::Apply()
{
	// User never visited this tab, so there's nothing to apply.
	if( !m_BiosList ) return;

	int sel = m_ComboBox->GetSelection();
	if( sel == wxNOT_FOUND )
	{
		throw Exception::CannotApplySettings(this)
			.SetDiagMsg(L"User did not specify a valid BIOS selection.")
			.SetUserMsg( pxE( "!Notice:BIOS:InvalidSelection",
				L"Please select a valid BIOS.  If you are unable to make a valid selection "
				L"then press cancel to close the Configuration panel."
			) );
	}

	g_Conf->BaseFilenames.Bios = (*m_BiosList)[(int)m_ComboBox->GetClientData(sel)];
}

void Panels::BiosSelectorPanel::AppStatusEvent_OnSettingsApplied()
{
}

bool Panels::BiosSelectorPanel::ValidateEnumerationStatus()
{
	bool validated = true;

	// Impl Note: ScopedPtr used so that resources get cleaned up if an exception
	// occurs during file enumeration.
	ScopedPtr<wxArrayString> bioslist( new wxArrayString() );

	if( m_FolderPicker->GetPath().Exists() )
		wxDir::GetAllFiles( m_FolderPicker->GetPath().ToString(), bioslist, L"*.*", wxDIR_FILES );

	if( !m_BiosList || (*bioslist != *m_BiosList) )
		validated = false;

	m_BiosList.SwapPtr( bioslist );

	return validated;
}

void Panels::BiosSelectorPanel::DoRefresh()
{
	if( !m_BiosList ) return;

	m_ComboBox->Clear();

	const wxFileName right( g_Conf->FullpathToBios() );

	for( size_t i=0; i<m_BiosList->GetCount(); ++i )
	{
		wxString description;
		if( !IsBIOS((*m_BiosList)[i], description) ) continue;
		int sel = m_ComboBox->Append( description, (void*)i );

		if( wxFileName((*m_BiosList)[i] ) == right )
			m_ComboBox->SetSelection( sel );
	}
}
