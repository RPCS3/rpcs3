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
#include "System.h"
#include "App.h"
#include "MSWstuff.h"

#include "ConfigurationDialog.h"
#include "ModalPopups.h"
#include "Panels/ConfigurationPanels.h"

#include "Resources/EmbeddedImage.h"
#include "Resources/ButtonIcon_Camera.h"

#include <wx/artprov.h>
#include <wx/filepicker.h>
#include <wx/listbook.h>

DEFINE_EVENT_TYPE( pxEvt_ApplySettings )
DEFINE_EVENT_TYPE( pxEvt_SetSettingsPage )

using namespace Panels;

// configure the orientation of the listbox based on the platform

#if defined(__WXMAC__) || defined(__WXMSW__)
	static const int s_orient = wxBK_TOP;
#else
	static const int s_orient = wxBK_LEFT;
#endif

IMPLEMENT_DYNAMIC_CLASS(BaseApplicableDialog, wxDialogWithHelpers)

BaseApplicableDialog::BaseApplicableDialog( wxWindow* parent, const wxString& title, const pxDialogCreationFlags& cflags )
	: wxDialogWithHelpers( parent, title, cflags.MinWidth(425).Minimize() )
{
	Init();
}

BaseApplicableDialog::~BaseApplicableDialog() throw()
{
	m_ApplyState.DoCleanup();
}

wxString BaseApplicableDialog::GetDialogName() const
{
	pxFailDev( "This class must implement GetDialogName!" );
	return L"Unnamed";
}


void BaseApplicableDialog::Init()
{
	Connect( pxEvt_ApplySettings,	wxCommandEventHandler	(BaseApplicableDialog::OnSettingsApplied) );

	wxCommandEvent applyEvent( pxEvt_ApplySettings );
	applyEvent.SetId( GetId() );
	AddPendingEvent( applyEvent );
}

void BaseApplicableDialog::OnSettingsApplied( wxCommandEvent& evt )
{
	evt.Skip();
	if( evt.GetId() == GetId() ) AppStatusEvent_OnSettingsApplied();
}


// --------------------------------------------------------------------------------------
//  BaseConfigurationDialog  Implementations
// --------------------------------------------------------------------------------------
Dialogs::BaseConfigurationDialog::BaseConfigurationDialog( wxWindow* parent, const wxString& title, int idealWidth )
	: _parent( parent, title )
{
	SetMinWidth( idealWidth );
	m_listbook		= NULL;
	
	Connect( wxID_OK,		wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( BaseConfigurationDialog::OnOk_Click ) );
	Connect( wxID_CANCEL,	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( BaseConfigurationDialog::OnCancel_Click ) );
	Connect( wxID_APPLY,	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( BaseConfigurationDialog::OnApply_Click ) );
	Connect( wxID_SAVE,		wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( BaseConfigurationDialog::OnScreenshot_Click ) );

	Connect(				wxEVT_CLOSE_WINDOW,				wxCloseEventHandler(BaseConfigurationDialog::OnCloseWindow) );

	Connect( pxEvt_SetSettingsPage, wxCommandEventHandler( BaseConfigurationDialog::OnSetSettingsPage ) );

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
}

void Dialogs::BaseConfigurationDialog::AddListbook( wxSizer* sizer )
{
	if( !sizer ) sizer = GetSizer();
	sizer += m_listbook	| pxExpand.Border( wxLEFT | wxRIGHT, 2 );
}

void Dialogs::BaseConfigurationDialog::CreateListbook( wxImageList& bookicons )
{
	m_listbook = new wxListbook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, s_orient );
	m_listbook->SetImageList( &bookicons );
	m_ApplyState.StartBook( m_listbook );
}

void Dialogs::BaseConfigurationDialog::AddOkCancel( wxSizer* sizer )
{
	_parent::AddOkCancel( sizer, true );
	if( wxWindow* apply = FindWindow( wxID_APPLY ) ) apply->Disable();

	wxBitmapButton& screenshotButton( *new wxBitmapButton( this, wxID_SAVE, EmbeddedImage<res_ButtonIcon_Camera>().Get() ) );
	screenshotButton.SetToolTip( _("Saves a snapshot of this settings panel to a PNG file.") );

	*m_extraButtonSizer += screenshotButton|pxMiddle;
}

Dialogs::BaseConfigurationDialog::~BaseConfigurationDialog() throw()
{
}

void Dialogs::BaseConfigurationDialog::OnSetSettingsPage( wxCommandEvent& evt )
{
	if( !m_listbook ) return;
	
	size_t pages = m_labels.GetCount();
	
	for( size_t i=0; i<pages; ++i )
	{
		if( evt.GetString() == m_labels[i] )
		{
			m_listbook->SetSelection( i );
			break;
		}
	}
}

void Dialogs::BaseConfigurationDialog::SomethingChanged()
{
	if( wxWindow* apply = FindWindow( wxID_APPLY ) ) apply->Enable();
}

void Dialogs::BaseConfigurationDialog::OnSomethingChanged( wxCommandEvent& evt )
{
	evt.Skip();
	if( (evt.GetId() != wxID_OK) && (evt.GetId() != wxID_CANCEL) && (evt.GetId() != wxID_APPLY) )
		SomethingChanged();
}


void Dialogs::BaseConfigurationDialog::OnCloseWindow( wxCloseEvent& evt )
{
	if( !IsModal() ) Destroy();
	evt.Skip();
}

class ScopedOkButtonDisabler
{
protected:
	wxWindow* m_apply;
	wxWindow* m_ok;
	wxWindow* m_cancel;

public:
	ScopedOkButtonDisabler( wxWindow* parent )
	{
		m_apply		= parent->FindWindow( wxID_APPLY );
		m_ok		= parent->FindWindow( wxID_OK );
		m_cancel	= parent->FindWindow( wxID_CANCEL );

		if (m_apply)	m_apply	->Disable();
		if (m_ok)		m_ok	->Disable();
		if (m_cancel)	m_cancel->Disable();
	}

	// Use this to prevent the Apply buton from being re-enabled.
	void DetachApply()
	{
		m_apply = NULL;
	}

	void DetachAll()
	{
		m_apply = m_ok = m_cancel = NULL;
	}
	
	virtual ~ScopedOkButtonDisabler() throw()
	{
		if (m_apply)	m_apply	->Enable();
		if (m_ok)		m_ok	->Enable();
		if (m_cancel)	m_cancel->Enable();
	}
};

void Dialogs::BaseConfigurationDialog::OnOk_Click( wxCommandEvent& evt )
{
	ScopedOkButtonDisabler disabler(this);

	if( m_ApplyState.ApplyAll() )
	{
		if( wxWindow* apply = FindWindow( wxID_APPLY ) ) apply->Disable();
		if( m_listbook ) GetConfSettingsTabName() = m_labels[m_listbook->GetSelection()];
		AppSaveSettings();
		disabler.DetachAll();
		evt.Skip();
	}
}

void Dialogs::BaseConfigurationDialog::OnApply_Click( wxCommandEvent& evt )
{
	ScopedOkButtonDisabler disabler(this);

	if( m_ApplyState.ApplyAll() )
		disabler.DetachApply();

	if( m_listbook ) GetConfSettingsTabName() = m_labels[m_listbook->GetSelection()];
	AppSaveSettings();
}

void Dialogs::BaseConfigurationDialog::OnCancel_Click( wxCommandEvent& evt )
{
	evt.Skip();
	if( m_listbook ) GetConfSettingsTabName() = m_labels[m_listbook->GetSelection()];
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

	wxString pagename( m_listbook ? (L"_" + m_listbook->GetPageText( m_listbook->GetSelection() )) : wxString() );
	wxString filenameDefault;
	filenameDefault.Printf( L"%s_%s%s.png", pxGetAppName().Lower().c_str(), GetDialogName().c_str(), pagename.c_str() );
	filenameDefault.Replace( L"/", L"-" );

	wxString filename( wxFileSelector( _("Save dialog screenshots to..."), g_Conf->Folders.Snapshots.ToString(),
		filenameDefault, L"png", NULL, wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this ) );

	if( !filename.IsEmpty() )
	{
		ScopedBusyCursor busy( Cursor_ReallyBusy );
		memBmp.SaveFile( filename, wxBITMAP_TYPE_PNG );
	}
}

void Dialogs::BaseConfigurationDialog::OnSettingsApplied( wxCommandEvent& evt )
{
	evt.Skip();
	MSW_ListView_SetIconSpacing( m_listbook, GetClientSize().GetWidth() );
}