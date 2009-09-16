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
#include "Plugins.h"

#include "ModalPopups.h"
#include "Panels/ConfigurationPanels.h"
#include "wx//file.h"

using namespace wxHelpers;
using namespace Panels;

template< typename T >
static T& MakeWizWidget( int pageid, wxWizardPage& src )
{
	g_ApplyState.SetCurrentPage( pageid );
	return *new T( src, 620 );
}

// ----------------------------------------------------------------------------
Panels::SettingsDirPickerPanel::SettingsDirPickerPanel( wxWindow* parent ) :
	DirPickerPanel( parent, FolderId_Settings, _("Settings"), _("Select a folder for PCSX2 settings") )
{
	SetToolTip( pxE( ".Tooltips:Folders:Settings",
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

	//SetSizerAndFit( GetSizer(), false );
}

// ----------------------------------------------------------------------------
FirstTimeWizard::UsermodePage::UsermodePage( wxWizard* parent ) :
	wxWizardPageSimple( (g_ApplyState.SetCurrentPage( 0 ), parent) )

,	m_dirpick_settings( *new SettingsDirPickerPanel( this ) )
,	m_panel_LangSel( *new LanguageSelectionPanel( *this, 608 ) )
,	m_panel_UserSel( *new UsermodeSelectionPanel( *this, 608 ) )
{
	wxBoxSizer& usermodeSizer( *new wxBoxSizer( wxVERTICAL ) );
	AddStaticTextTo( this, usermodeSizer, _("PCSX2 is starting from a new or unknown folder and needs to be configured.") );

	usermodeSizer.Add( &m_panel_LangSel, SizerFlags::StdCenter() );
	usermodeSizer.Add( &m_panel_UserSel, wxSizerFlags().Expand().Border( wxALL, 8 ) );

	usermodeSizer.AddSpacer( 6 );
	usermodeSizer.Add( &m_dirpick_settings, SizerFlags::SubGroup() );
	SetSizer( &usermodeSizer );

	Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED,	wxCommandEventHandler(FirstTimeWizard::UsermodePage::OnUsermodeChanged) );
}

void FirstTimeWizard::UsermodePage::OnUsermodeChanged( wxCommandEvent& evt )
{
	m_panel_UserSel.Apply();
	g_Conf->Folders.ApplyDefaults();
	m_dirpick_settings.Reset();
}

// ----------------------------------------------------------------------------
FirstTimeWizard::FirstTimeWizard( wxWindow* parent ) :
	wxWizard( (g_ApplyState.StartWizard(), parent), wxID_ANY, _("PCSX2 First Time Configuration") )
,	m_page_usermode( *new UsermodePage( this ) )
,	m_page_plugins( *new wxWizardPageSimple( this, &m_page_usermode ) )
,	m_page_bios( *new wxWizardPageSimple( this, &m_page_plugins ) )

,	m_panel_PluginSel( MakeWizWidget<PluginSelectorPanel>( 1, m_page_plugins ) )
,	m_panel_BiosSel( MakeWizWidget<BiosSelectorPanel>( 2, m_page_bios ) )
{
	// Page 2 - Plugins Panel
	wxBoxSizer& pluginSizer( *new wxBoxSizer( wxVERTICAL ) );
	pluginSizer.Add( &m_panel_PluginSel, SizerFlags::StdExpand() );
	m_page_plugins.SetSizer( &pluginSizer );

	// Page 3 - Bios Panel
	wxBoxSizer& biosSizer( *new wxBoxSizer( wxVERTICAL ) );
	biosSizer.Add( &m_panel_BiosSel, SizerFlags::StdExpand() );
	m_page_bios.SetSizer( &biosSizer );

	// Assign page indexes as client data
	m_page_usermode.SetClientData	( (void*)0 );
	m_page_plugins.SetClientData	( (void*)1 );
	m_page_bios.SetClientData		( (void*)2 );

	// Build the forward chain:
	//  (backward chain is built during initialization above)
	m_page_usermode.SetNext	( &m_page_plugins );
	m_page_plugins.SetNext	( &m_page_bios );

	GetPageAreaSizer()->Add( &m_page_usermode );
	GetPageAreaSizer()->Add( &m_page_plugins );
	CenterOnScreen();

	Connect( wxEVT_WIZARD_PAGE_CHANGED,		wxWizardEventHandler( FirstTimeWizard::OnPageChanged ) );
	Connect( wxEVT_WIZARD_PAGE_CHANGING,	wxWizardEventHandler( FirstTimeWizard::OnPageChanging ) );

	Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(FirstTimeWizard::OnDoubleClicked) );
}

FirstTimeWizard::~FirstTimeWizard()
{
	g_ApplyState.DoCleanup();
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
			if( !g_ApplyState.ApplyPage( page, false ) )
			{
				evt.Veto();
				return;
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
				m_panel_PluginSel.ReloadSettings();
				m_panel_BiosSel.ReloadSettings();
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
