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
#include "System.h"
#include "App.h"

#include "ConfigurationDialog.h"
#include "ModalPopups.h"
#include "Panels/ConfigurationPanels.h"

#include "Resources/EmbeddedImage.h"
#include "Resources/ButtonIcon_Camera.h"

#include <wx/artprov.h>
#include <wx/filepicker.h>
#include <wx/listbook.h>

using namespace Panels;

// configure the orientation of the listbox based on the platform

#if defined(__WXMAC__) || defined(__WXMSW__)
	static const int s_orient = wxBK_TOP;
#else
	static const int s_orient = wxBK_LEFT;
#endif

IMPLEMENT_DYNAMIC_CLASS(Dialogs::BaseApplicableDialog, wxDialogWithHelpers)

Dialogs::BaseApplicableDialog::BaseApplicableDialog( wxWindow* parent, const wxString& title )
	: wxDialogWithHelpers( parent, title, false )
{
}

Dialogs::BaseApplicableDialog::BaseApplicableDialog( wxWindow* parent, const wxString& title, wxOrientation sizerOrient )
	: wxDialogWithHelpers( parent, title, sizerOrient )
{
}

Dialogs::BaseApplicableDialog::~BaseApplicableDialog() throw()
{
	m_ApplyState.DoCleanup();
}

Dialogs::BaseConfigurationDialog::BaseConfigurationDialog( wxWindow* parent, const wxString& title, wxImageList& bookicons, int idealWidth )
	: BaseApplicableDialog( parent, title, wxVERTICAL )
	, m_listbook( *new wxListbook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, s_orient ) )
{
	m_idealWidth = idealWidth;

	m_listbook.SetImageList( &bookicons );
	m_ApplyState.StartBook( &m_listbook );

	wxBitmapButton& screenshotButton( *new wxBitmapButton( this, wxID_SAVE, EmbeddedImage<res_ButtonIcon_Camera>().Get() ) );
	screenshotButton.SetToolTip( _("Saves a snapshot of this settings panel to a PNG file.") );

	*this += m_listbook;
	AddOkCancel( *GetSizer(), true );

	*m_extraButtonSizer += screenshotButton;

	Connect( wxID_OK,		wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( BaseConfigurationDialog::OnOk_Click ) );
	Connect( wxID_CANCEL,	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( BaseConfigurationDialog::OnCancel_Click ) );
	Connect( wxID_APPLY,	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( BaseConfigurationDialog::OnApply_Click ) );
	Connect( wxID_SAVE,		wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( BaseConfigurationDialog::OnScreenshot_Click ) );

	Connect(				wxEVT_CLOSE_WINDOW,				wxCloseEventHandler(BaseConfigurationDialog::OnCloseWindow) );
	
	// ----------------------------------------------------------------------------
	// Bind a variety of standard "something probably changed" events.  If the user invokes
	// any of these, we'll automatically de-gray the Apply button for this dialog box. :)

	#define ConnectSomethingChanged( command ) \
		Connect( wxEVT_COMMAND_##command,	wxCommandEventHandler( BaseConfigurationDialog::OnSomethingChanged ) );

	ConnectSomethingChanged( TEXT_UPDATED );
	ConnectSomethingChanged( TEXT_ENTER );

	ConnectSomethingChanged( RADIOBUTTON_SELECTED );
	ConnectSomethingChanged( COMBOBOX_SELECTED );
	ConnectSomethingChanged( CHECKBOX_CLICKED );
	ConnectSomethingChanged( BUTTON_CLICKED );
	ConnectSomethingChanged( CHOICE_SELECTED );
	ConnectSomethingChanged( LISTBOX_SELECTED );
	ConnectSomethingChanged( SPINCTRL_UPDATED );
	ConnectSomethingChanged( SLIDER_UPDATED );
	ConnectSomethingChanged( DIRPICKER_CHANGED );

	FindWindow( wxID_APPLY )->Disable();
}

Dialogs::BaseConfigurationDialog::~BaseConfigurationDialog() throw()
{
}


void Dialogs::BaseConfigurationDialog::OnSomethingChanged( wxCommandEvent& evt )
{
	evt.Skip();
	if( (evt.GetId() != wxID_OK) && (evt.GetId() != wxID_CANCEL) && (evt.GetId() != wxID_APPLY) )
	{
		FindWindow( wxID_APPLY )->Enable();
	}
}


void Dialogs::BaseConfigurationDialog::OnCloseWindow( wxCloseEvent& evt )
{
	if( !IsModal() ) Destroy();
	evt.Skip();
}

void Dialogs::BaseConfigurationDialog::OnOk_Click( wxCommandEvent& evt )
{
	if( m_ApplyState.ApplyAll() )
	{
		FindWindow( wxID_APPLY )->Disable();
		GetConfSettingsTabName() = m_labels[m_listbook.GetSelection()];
		AppSaveSettings();
		evt.Skip();
	}
}

void Dialogs::BaseConfigurationDialog::OnCancel_Click( wxCommandEvent& evt )
{
	evt.Skip();
	GetConfSettingsTabName() = m_labels[m_listbook.GetSelection()];
}

void Dialogs::BaseConfigurationDialog::OnApply_Click( wxCommandEvent& evt )
{
	if( m_ApplyState.ApplyAll() )
		FindWindow( wxID_APPLY )->Disable();

	GetConfSettingsTabName() = m_labels[m_listbook.GetSelection()];
	AppSaveSettings();
}


void Dialogs::BaseConfigurationDialog::OnScreenshot_Click( wxCommandEvent& evt )
{
	wxBitmap memBmp;

	{
	wxWindowDC dc( this );
	wxSize dcsize( dc.GetSize() );
	wxMemoryDC memDC( memBmp = wxBitmap( dcsize.x, dcsize.y ) );
	memDC.Blit( wxPoint(), dcsize, &dc, wxPoint() );
	}

	wxString filenameDefault;
	filenameDefault.Printf( L"pcsx2_settings_%s.png", m_listbook.GetPageText( m_listbook.GetSelection() ).c_str() );
	filenameDefault.Replace( L"/", L"-" );

	wxString filename( wxFileSelector( _("Save dialog screenshots to..."), g_Conf->Folders.Snapshots.ToString(),
		filenameDefault, L"png", NULL, wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this ) );

	if( !filename.IsEmpty() )
	{
		ScopedBusyCursor busy( Cursor_ReallyBusy );
		memBmp.SaveFile( filename, wxBITMAP_TYPE_PNG );
	}
}
