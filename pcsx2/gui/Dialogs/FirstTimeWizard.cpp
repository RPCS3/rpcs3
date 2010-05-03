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
#include "Plugins.h"

#include "ModalPopups.h"
#include "Panels/ConfigurationPanels.h"
#include <wx/file.h>
#include <wx/filepicker.h>

using namespace Panels;
using namespace pxSizerFlags;

IMPLEMENT_DYNAMIC_CLASS(ApplicableWizardPage, wxWizardPageSimple)

ApplicableWizardPage::ApplicableWizardPage( wxWizard* parent, wxWizardPage* prev, wxWizardPage* next, const wxBitmap& bitmap )
	: wxWizardPageSimple( parent, prev, next, bitmap )
{
}

// This is a hack feature substitute for prioritized apply events.  This callback is issued prior
// to the apply chain being run, allowing a panel to do some pre-apply prepwork.  PAnels implementing
// this function should not modify the g_conf state.
bool ApplicableWizardPage::PrepForApply()
{
	return true;
}


// ----------------------------------------------------------------------------
Panels::SettingsDirPickerPanel::SettingsDirPickerPanel( wxWindow* parent ) :
	DirPickerPanel( parent, FolderId_Settings, _("Settings"), _("Select a folder for PCSX2 settings") )
{
	pxSetToolTip( this, pxE( ".Tooltips:Folders:Settings",
		L"This is the folder where PCSX2 saves your settings, including settings generated "
		L"by most plugins (some older plugins may not respect this value)."
	) );

	// Insert this into the top of the staticboxsizer created by the constructor.
	GetSizer()->Insert( 0,
		new wxStaticText( this, wxID_ANY,
			pxE( ".Dialogs:SettingsDirPicker",
				L"You may optionally specify a location for your PCSX2 settings here.  If the location \n"
				L"contains existing PCSX2 settings, you will be given the option to import or overwrite them."
			), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE
		), wxSizerFlags().Expand().Border( wxBOTTOM, 6 )
	);
}

// ----------------------------------------------------------------------------
FirstTimeWizard::UsermodePage::UsermodePage( wxWizard* parent ) :
	ApplicableWizardPage( parent )
{
	SetSizer( new wxBoxSizer( wxVERTICAL ) );

	wxPanelWithHelpers& panel( *new wxPanelWithHelpers( this, wxVERTICAL ) );
	panel.SetIdealWidth( 640 );

	m_dirpick_settings	= new SettingsDirPickerPanel( &panel );
	m_panel_LangSel		= new LanguageSelectionPanel( &panel );
	m_panel_UserSel		= new DocsFolderPickerPanel( &panel );

	panel += panel.Heading(_("PCSX2 is starting from a new or unknown folder and needs to be configured."));

	panel += m_panel_LangSel		| StdCenter();
	panel += m_panel_UserSel		| pxExpand.Border( wxALL, 8 );

	panel += 6;
	panel += m_dirpick_settings		| SubGroup();

	*this += panel					| pxExpand;

	Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED,	wxCommandEventHandler(UsermodePage::OnUsermodeChanged) );

	Connect( m_panel_UserSel->GetDirPickerId(), wxEVT_COMMAND_DIRPICKER_CHANGED,		wxCommandEventHandler(UsermodePage::OnCustomDirChanged) );
}

void FirstTimeWizard::UsermodePage::OnUsermodeChanged( wxCommandEvent& evt )
{
	evt.Skip();
	m_panel_UserSel->Apply();
	g_Conf->Folders.ApplyDefaults();
	m_dirpick_settings->Reset();
}

void FirstTimeWizard::UsermodePage::OnCustomDirChanged( wxCommandEvent& evt )
{
	OnUsermodeChanged( evt );
}

bool FirstTimeWizard::UsermodePage::PrepForApply()
{
	wxDirName path( PathDefs::GetDocuments(m_panel_UserSel->GetDocsMode()) );

	if( path.FileExists() )
	{
		// FIXME: There's already a file by the same name.. not sure what we should do here.
		throw Exception::BadStream( path.ToString(),
			L"Targeted documents folder is already occupied by a file.",
			pxE( "Error:DocsFolderFileConflict",
				L"PCSX2 cannot create a documents folder in the requested location.  "
				L"The path name matches an existing file.  Delete the file or change the documents location, "
				L"and then try again."
			)
		);
	}

	if( !path.Exists() )
	{
		wxDialogWithHelpers dialog( NULL, _("Create folder?"), wxVERTICAL );
		dialog += dialog.Heading( _("PCSX2 will create the following folder for documents.  You can change this setting later, at any time.") );
		dialog += 12;
		dialog += dialog.Heading( path.ToString() );

		if( wxID_CANCEL == pxIssueConfirmation( dialog, MsgButtons().Custom(_("Create")).Cancel(), L"CreateNewFolder" ) )
			return false;
	}
	path.Mkdir();
	return true;
}

// ----------------------------------------------------------------------------
FirstTimeWizard::FirstTimeWizard( wxWindow* parent )
	: wxWizard( parent, wxID_ANY, _("PCSX2 First Time Configuration") )
	, m_page_usermode	( *new UsermodePage( this ) )
	, m_page_plugins	( *new ApplicableWizardPage( this, &m_page_usermode ) )
	, m_page_bios		( *new ApplicableWizardPage( this, &m_page_plugins ) )

	, m_panel_PluginSel	( *new PluginSelectorPanel( &m_page_plugins ) )
	, m_panel_BiosSel	( *new BiosSelectorPanel( &m_page_bios ) )
{
	// Page 2 - Plugins Panel
	// Page 3 - Bios Panel

	m_page_plugins.	SetSizer( new wxBoxSizer( wxVERTICAL ) );
	m_page_bios.	SetSizer( new wxBoxSizer( wxVERTICAL ) );

	m_page_plugins	+= m_panel_PluginSel		| StdExpand();
	m_page_bios		+= m_panel_BiosSel			| StdExpand();

	// Assign page indexes as client data
	m_page_usermode	.SetClientData( (void*)0 );
	m_page_plugins	.SetClientData( (void*)1 );
	m_page_bios		.SetClientData( (void*)2 );

	// Build the forward chain:
	//  (backward chain is built during initialization above)
	m_page_usermode	.SetNext( &m_page_plugins );
	m_page_plugins	.SetNext( &m_page_bios );

	GetPageAreaSizer() += m_page_usermode;
	GetPageAreaSizer() += m_page_plugins;

	// this doesn't descent from wxDialogWithHelpers, so we need to explicitly
	// fit and center it. :(

	Fit();
	CenterOnScreen();

	Connect( wxEVT_WIZARD_PAGE_CHANGED,				wxWizardEventHandler	(FirstTimeWizard::OnPageChanged) );
	Connect( wxEVT_WIZARD_PAGE_CHANGING,			wxWizardEventHandler	(FirstTimeWizard::OnPageChanging) );
	Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED,	wxCommandEventHandler	(FirstTimeWizard::OnDoubleClicked) );
}

FirstTimeWizard::~FirstTimeWizard() throw()
{

}

static void _OpenConsole()
{
	g_Conf->ProgLogBox.Visible = true;
	wxGetApp().OpenConsoleLog();
}

int FirstTimeWizard::ShowModal()
{
	if( IsDebugBuild ) wxGetApp().PostIdleMethod( _OpenConsole );
	return _parent::ShowModal();
}

void FirstTimeWizard::OnDoubleClicked( wxCommandEvent& evt )
{
	wxWindow* forwardButton = FindWindow( wxID_FORWARD );
	if( forwardButton == NULL ) return;

	wxCommandEvent nextpg( wxEVT_COMMAND_BUTTON_CLICKED, wxID_FORWARD );
	nextpg.SetEventObject( forwardButton );
	forwardButton->GetEventHandler()->ProcessEvent( nextpg );
}

void FirstTimeWizard::OnPageChanging( wxWizardEvent& evt )
{
	if( evt.GetPage() == NULL ) return;		// safety valve!

	int page = (int)evt.GetPage()->GetClientData();

	if( evt.GetDirection() )
	{
		// Moving forward:
		//   Apply settings from the current page...

		if( page >= 0 )
		{
			if( ApplicableWizardPage* page = wxDynamicCast( GetCurrentPage(), ApplicableWizardPage ) )
			{
				if( !page->PrepForApply() || !page->GetApplyState().ApplyAll() )
				{
					evt.Veto();
					return;
				}
			}
		}

		if( page == 0 )
		{
			if( wxFile::Exists( GetSettingsFilename() ) )
			{
				// Asks the user if they want to import or overwrite the existing settings.

				Dialogs::ImportSettingsDialog modal( this );
				if( modal.ShowModal() != wxID_OK )
				{
					evt.Veto();
					return;
				}
			}
		}
	}
	else
	{
		// Moving Backward:
		//   Some specific panels need per-init actions canceled.

		if( page == 1 )
		{
			m_panel_PluginSel.CancelRefresh();
		}
	}
}

void FirstTimeWizard::OnPageChanged( wxWizardEvent& evt )
{
	// Plugin Selector needs a special OnShow hack, because Wizard child panels don't
	// receive any Show events >_<
	if( (sptr)evt.GetPage() == (sptr)&m_page_plugins )
		m_panel_PluginSel.OnShown();

	else if( (sptr)evt.GetPage() == (sptr)&m_page_bios )
		m_panel_BiosSel.OnShown();
}
