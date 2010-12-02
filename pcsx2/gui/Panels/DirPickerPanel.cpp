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
#include "ConfigurationPanels.h"
#include "Dialogs/ModalPopups.h"

#include <wx/stdpaths.h>
#include <wx/file.h>
#include <wx/dir.h>
#include <wx/filepicker.h>

using namespace pxSizerFlags;

static wxString GetNormalizedConfigFolder( FoldersEnum_t folderId )
{
	return Path::Normalize( g_Conf->Folders.IsDefault( folderId ) ? PathDefs::Get(folderId) : g_Conf->Folders[folderId] );
}

// Pass me TRUE if the default path is to be used, and the DirPickerCtrl disabled from use.
void Panels::DirPickerPanel::UpdateCheckStatus( bool someNoteworthyBoolean )
{
	m_pickerCtrl->Enable( !someNoteworthyBoolean );
	if( someNoteworthyBoolean )
	{
		wxString normalized( Path::Normalize( PathDefs::Get( m_FolderId ) ) );
		m_pickerCtrl->SetPath( normalized );

		wxFileDirPickerEvent event( m_pickerCtrl->GetEventType(), m_pickerCtrl, m_pickerCtrl->GetId(), normalized );
		m_pickerCtrl->GetEventHandler()->ProcessEvent(event);
	}
}

void Panels::DirPickerPanel::UseDefaultPath_Click( wxCommandEvent &evt )
{
	evt.Skip();
	pxAssert( (m_pickerCtrl != NULL) && (m_checkCtrl != NULL) );
	UpdateCheckStatus( m_checkCtrl ? m_checkCtrl->IsChecked() : false );
}

void Panels::DirPickerPanel::Explore_Click( wxCommandEvent &evt )
{
	wxString path( m_pickerCtrl->GetPath() );
	if( !wxDirExists(path) )
	{
		wxDialogWithHelpers createPathDlg( NULL, _("Path does not exist") );
		createPathDlg.SetMinWidth( 600 );

		createPathDlg += createPathDlg.Text( path ) | StdCenter();

		createPathDlg += createPathDlg.Heading( pxE( "!Notice:DirPicker:CreatePath",
			L"The specified path/directory does not exist.  Would you like to create it?" )
		);

		wxWindowID result = pxIssueConfirmation( createPathDlg,
			MsgButtons().Custom(_("Create")).Cancel(),
			L"DirPicker:CreateOnExplore"
		);

		if( result == wxID_CANCEL ) return;
		wxDirName(path).Mkdir();
	}

	pxExplore( path );
}

// ------------------------------------------------------------------------
// If initPath is NULL, then it's assumed the default folder is to be used, which is
// obtained from invoking the specified getDefault() function.
//
Panels::DirPickerPanel::DirPickerPanel( wxWindow* parent, FoldersEnum_t folderid, const wxString& label, const wxString& dialogLabel )
	: BaseApplicableConfigPanel( parent, wxVERTICAL, label )
{
	Init( folderid, dialogLabel, false );
}

Panels::DirPickerPanel::DirPickerPanel( wxWindow* parent, FoldersEnum_t folderid, const wxString& dialogLabel )
	: BaseApplicableConfigPanel( parent, wxVERTICAL )
{
	Init( folderid, dialogLabel, true );
}

void Panels::DirPickerPanel::Init( FoldersEnum_t folderid, const wxString& dialogLabel, bool isCompact )
{
	m_FolderId		= folderid;
	m_pickerCtrl	= NULL;
	m_checkCtrl		= NULL;

	// Force the Dir Picker to use a text control.  This isn't standard on Linux/GTK but it's much
	// more usable, so to hell with standards.

	wxString normalized( GetNormalizedConfigFolder( m_FolderId ) );

	if( wxFile::Exists( normalized ) )
	{
		// The default path is invalid... What should we do here? hmm..
	}

	//if( !wxDir::Exists( normalized ) )
	//	wxMkdir( normalized );

	if( !isCompact )
	{
		m_checkCtrl = new pxCheckBox( this, _("Use default setting") );

		pxSetToolTip( m_checkCtrl, pxEt( "!ContextTip:DirPicker:UseDefault",
			L"When checked this folder will automatically reflect the default associated with PCSX2's current usermode setting. " )
		);

		Connect( m_checkCtrl->GetId(),	wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DirPickerPanel::UseDefaultPath_Click ) );
	}

	m_pickerCtrl = new wxDirPickerCtrl( this, wxID_ANY, wxEmptyString, dialogLabel,
		wxDefaultPosition, wxDefaultSize, wxDIRP_USE_TEXTCTRL | wxDIRP_DIR_MUST_EXIST
	);

#ifndef __WXGTK__
	// GTK+ : The wx implementation of Explore isn't reliable, so let's not even put the
	// button on the dialogs for now.

	wxButton* b_explore( new wxButton( this, wxID_ANY, _("Open in Explorer") ) );
	pxSetToolTip( b_explore, _("Open an explorer window to this folder.") );
	Connect( b_explore->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( DirPickerPanel::Explore_Click ) );
#endif

	if( isCompact )
	{
		wxFlexGridSizer& s_compact( *new wxFlexGridSizer( 2, 0, 4 ) );
		s_compact.AddGrowableCol( 0 );
		s_compact += m_pickerCtrl | pxExpand;
#ifndef __WXGTK__
		s_compact += b_explore;
#endif
		*this += s_compact	| pxExpand; //.Border(wxLEFT | wxRIGHT | wxTOP, 5);
	}
	else
	{
		wxBoxSizer& s_lower( *new wxBoxSizer( wxHORIZONTAL ) );

		s_lower += m_checkCtrl	| pxMiddle;
#ifndef __WXGTK__
		s_lower += pxStretchSpacer(1);
		s_lower += b_explore;
#endif

		*this += m_pickerCtrl	| pxExpand.Border(wxLEFT | wxRIGHT | wxTOP, StdPadding);
		*this += s_lower		| pxExpand.Border(wxLEFT | wxRIGHT, StdPadding);
	}

	// wx warns when paths don't exist, but this is typically normal when the wizard
	// creates its child controls.  So let's ignore them.
	wxDoNotLogInThisScope please;
	AppStatusEvent_OnSettingsApplied();	// forces default settings based on g_Conf
}

Panels::DirPickerPanel& Panels::DirPickerPanel::SetStaticDesc( const wxString& msg )
{
	GetSizer()->Insert( 0, &Heading( msg ), pxExpand );
	return *this;
}

Panels::DirPickerPanel& Panels::DirPickerPanel::SetToolTip( const wxString& tip )
{
	pxSetToolTip( this, tip );
	return *this;
}

wxWindowID Panels::DirPickerPanel::GetId() const
{
	return m_pickerCtrl->GetId();
}

void Panels::DirPickerPanel::Reset()
{
	const bool isDefault = g_Conf->Folders.IsDefault( m_FolderId );

	if( m_checkCtrl )
		m_checkCtrl->SetValue( isDefault );

	if( m_pickerCtrl )
	{
		// Important!  The dirpicker panel stuff, due to however it's put together
		// needs to check the enable status of this panel before setting the child
		// panel's enable status.

		m_pickerCtrl->Enable( IsEnabled() ? ( m_checkCtrl ? !isDefault : true ) : false );
		m_pickerCtrl->SetPath( GetNormalizedConfigFolder( m_FolderId ) );
	}
}

bool Panels::DirPickerPanel::Enable( bool enable )
{
	m_pickerCtrl->Enable( enable ? (!m_checkCtrl || m_checkCtrl->GetValue()) : false );
	return _parent::Enable( enable );
}


void Panels::DirPickerPanel::AppStatusEvent_OnSettingsApplied()
{
	Reset();
}

void Panels::DirPickerPanel::Apply()
{
	if( !m_pickerCtrl ) return;

	const wxString path( m_pickerCtrl->GetPath() );

	if( !wxDir::Exists( path ) )
	{
		wxDialogWithHelpers dialog( NULL, _("Create folder?") );
		dialog += dialog.Heading(AddAppName(_("A configured folder does not exist.  Should %s try to create it?")));
		dialog += 12;
		dialog += dialog.Heading( path );

		if( wxID_CANCEL == pxIssueConfirmation( dialog, MsgButtons().Custom(_("Create")).Cancel(), L"CreateNewFolder" ) )
			throw Exception::CannotApplySettings( this );
	}

	wxDirName(path).Mkdir();
	g_Conf->Folders.Set( m_FolderId, m_pickerCtrl->GetPath(), m_checkCtrl ? m_checkCtrl->GetValue() : false );
}

wxDirName Panels::DirPickerPanel::GetPath() const
{
	// The (x) ? y : z construct doesn't like y and z to be different types in gcc.
	if (m_pickerCtrl)
		return wxDirName(m_pickerCtrl->GetPath());
	else
		return wxDirName(wxEmptyString);
}

void Panels::DirPickerPanel::SetPath( const wxString& newPath )
{
	m_pickerCtrl->SetPath( newPath );
}
