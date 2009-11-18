/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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
#include "ConfigurationPanels.h"

#include <wx/stdpaths.h>
#include <wx/file.h>
#include <wx/dir.h>
#include <wx/filepicker.h>

using namespace wxHelpers;

static wxString GetNormalizedConfigFolder( FoldersEnum_t folderId )
{
	const bool isDefault = g_Conf->Folders.IsDefault( folderId );
	wxDirName normalized( isDefault ? PathDefs::Get(folderId) : g_Conf->Folders[folderId] );
	normalized.Normalize( wxPATH_NORM_ALL );
	return normalized.ToString();
}

// Pass me TRUE if the default path is to be used, and the DirPickerCtrl disabled from use.
void Panels::DirPickerPanel::UpdateCheckStatus( bool someNoteworthyBoolean )
{
	m_pickerCtrl->Enable( !someNoteworthyBoolean );
	if( someNoteworthyBoolean )
	{
		wxDirName normalized( PathDefs::Get( m_FolderId ) );
		normalized.Normalize( wxPATH_NORM_ALL );
		m_pickerCtrl->SetPath( normalized.ToString() );

		wxFileDirPickerEvent event( m_pickerCtrl->GetEventType(), m_pickerCtrl, m_pickerCtrl->GetId(), normalized.ToString() );
		m_pickerCtrl->GetEventHandler()->ProcessEvent(event);
	}
}

void Panels::DirPickerPanel::UseDefaultPath_Click( wxCommandEvent &evt )
{
	evt.Skip();
	pxAssert( (m_pickerCtrl != NULL) && (m_checkCtrl != NULL) );
	UpdateCheckStatus( m_checkCtrl->IsChecked() );
}

void Panels::DirPickerPanel::Explore_Click( wxCommandEvent &evt )
{
	pxExplore( m_pickerCtrl->GetPath() );
}

// ------------------------------------------------------------------------
// If initPath is NULL, then it's assumed the default folder is to be used, which is
// obtained from invoking the specified getDefault() function.
//
Panels::DirPickerPanel::DirPickerPanel( wxWindow* parent, FoldersEnum_t folderid, const wxString& label, const wxString& dialogLabel )
	: BaseApplicableConfigPanel( parent, wxVERTICAL, label )
{
	m_FolderId		= folderid;
	m_pickerCtrl	= NULL;
	m_checkCtrl		= NULL;

	m_checkCtrl = new pxCheckBox( this, _("Use default setting") );

	wxFlexGridSizer& s_lower( *new wxFlexGridSizer( 2, 0, 4 ) );

	s_lower.AddGrowableCol( 1 );

	// Force the Dir Picker to use a text control.  This isn't standard on Linux/GTK but it's much
	// more usable, so to hell with standards.

	wxString normalized( GetNormalizedConfigFolder( m_FolderId ) );

	if( wxFile::Exists( normalized ) )
	{
		// The default path is invalid... What should we do here? hmm..
	}

	if( !wxDir::Exists( normalized ) )
		wxMkdir( normalized );

	m_pickerCtrl = new wxDirPickerCtrl( this, wxID_ANY, wxEmptyString, dialogLabel,
		wxDefaultPosition, wxDefaultSize, wxDIRP_USE_TEXTCTRL | wxDIRP_DIR_MUST_EXIST
	);

	GetSizer()->Add( m_pickerCtrl, wxSizerFlags().Border(wxLEFT | wxRIGHT | wxTOP, 5).Expand() );
	s_lower.Add( m_checkCtrl );

	pxSetToolTip( m_checkCtrl, pxE( ".Tooltip:DirPicker:UseDefault",
		L"When checked this folder will automatically reflect the default associated with PCSX2's current usermode setting. " )
	);

#ifndef __WXGTK__
	// GTK+ : The wx implementation of Explore isn't reliable, so let's not even put the
	// button on the dialogs for now.

	wxButton* b_explore( new wxButton( this, wxID_ANY, _("Open in Explorer") ) );
	pxSetToolTip( b_explore, _("Open an explorer window to this folder.") );
	s_lower.Add( b_explore, pxSizerFlags::StdButton().Align( wxALIGN_RIGHT ) );
	Connect( b_explore->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( DirPickerPanel::Explore_Click ) );
#endif

	GetSizer()->Add( &s_lower, wxSizerFlags().Expand() );

	Connect( m_checkCtrl->GetId(),	wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DirPickerPanel::UseDefaultPath_Click ) );

	// wx warns when paths don't exist, but this is typically normal when the wizard 
	// creates its child controls.  So let's ignore them.
	wxDoNotLogInThisScope please;
	Reset();	// forces default settings based on g_Conf
}

Panels::DirPickerPanel& Panels::DirPickerPanel::SetStaticDesc( const wxString& msg )
{
	InsertStaticTextAt( this, *GetSizer(), 0, msg );
	//SetSizer( GetSizer(), false );
	return *this;
}

Panels::DirPickerPanel& Panels::DirPickerPanel::SetToolTip( const wxString& tip )
{
	pxSetToolTip( this, tip );
	return *this;
}

void Panels::DirPickerPanel::Reset()
{
	const bool isDefault = g_Conf->Folders.IsDefault( m_FolderId );
	m_checkCtrl->SetValue( isDefault );
	m_pickerCtrl->Enable( !isDefault );
	m_pickerCtrl->SetPath( GetNormalizedConfigFolder( m_FolderId ) );
}

void Panels::DirPickerPanel::Apply()
{
	g_Conf->Folders.Set( m_FolderId, m_pickerCtrl->GetPath(), m_checkCtrl->GetValue() );
}

wxDirName Panels::DirPickerPanel::GetPath() const
{
	return wxDirName( m_pickerCtrl->GetPath() );
}
