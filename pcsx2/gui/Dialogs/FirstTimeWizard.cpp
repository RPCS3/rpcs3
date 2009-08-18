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
#include "System.h"
#include "Plugins.h"

#include "ModalPopups.h"
#include "Panels/ConfigurationPanels.h"

using namespace wxHelpers;
using namespace Panels;

template< typename T >
static T& MakeWizWidget( int pageid, wxWizardPage& src )
{
	g_ApplyState.SetCurrentPage( pageid );
	return *new T( src, 620 );
}

FirstTimeWizard::FirstTimeWizard( wxWindow* parent ) :
	wxWizard( (g_ApplyState.StartWizard(), parent), wxID_ANY, _("PCSX2 First Time Configuration") )
,	m_page_usermode( *new wxWizardPageSimple( this ) )
//,	m_page_paths( *new wxWizardPageSimple( this, &m_page_usermode ) )
,	m_page_plugins( *new wxWizardPageSimple( this, &m_page_usermode ) )

,	m_panel_LangSel( MakeWizWidget<LanguageSelectionPanel>( 0, m_page_usermode ) )
,	m_panel_UsermodeSel( MakeWizWidget<UsermodeSelectionPanel>( 0, m_page_usermode ) )
,	m_panel_Paths( MakeWizWidget<AdvancedPathsPanel>( 0, m_page_usermode ) )
,	m_panel_PluginSel( MakeWizWidget<PluginSelectorPanel>( 1, m_page_plugins ) )
{
	// Page 1 - User Mode and Language Selectors
	wxBoxSizer& usermodeSizer( *new wxBoxSizer( wxVERTICAL ) );
	AddStaticTextTo( &m_page_usermode, usermodeSizer, _("PCSX2 is starting from a new or unknown folder and needs to be configured.") );
	usermodeSizer.Add( &m_panel_LangSel, SizerFlags::StdCenter() );
	usermodeSizer.Add( &m_panel_UsermodeSel, wxSizerFlags().Expand().Border( wxALL, 8 ) );

	//AddStaticTextTo( this, usermodeSizer, _( "Advanced users can optionally configure these folders." ) );
	usermodeSizer.AddSpacer( 6 );
	usermodeSizer.Add( &m_panel_Paths, wxSizerFlags().Expand().Border( wxALL, 4 ) );
	m_page_usermode.SetSizerAndFit( &usermodeSizer );

	// Page 2 - Advanced Paths Panel
	/*wxBoxSizer& pathsSizer( *new wxBoxSizer( wxVERTICAL ) );
	pathsSizer.Add( &m_panel_Paths, SizerFlags::StdExpand() );
	m_page_paths.SetSizerAndFit( &pathsSizer );*/

	// Page 3 - Plugins Panel
	wxBoxSizer& pluginSizer( *new wxBoxSizer( wxVERTICAL ) );
	pluginSizer.Add( &m_panel_PluginSel, SizerFlags::StdExpand() );
	m_page_plugins.SetSizerAndFit( &pluginSizer );

	// Assign page indexes as client data
	m_page_usermode.SetClientData	( (void*)0 );
	//m_page_paths.SetClientData		( (void*)1 );
	m_page_plugins.SetClientData	( (void*)1 );

	// Build the forward chain:
	//  (backward chain is built during initialization above)
	m_page_usermode.SetNext	( &m_page_plugins );
	//m_page_paths.SetNext	( &m_page_plugins );

	GetPageAreaSizer()->Add( &m_page_usermode );
	CenterOnScreen();

	Connect( wxEVT_WIZARD_PAGE_CHANGED,				wxWizardEventHandler( FirstTimeWizard::OnPageChanged ) );
	Connect( wxEVT_WIZARD_PAGE_CHANGING,			wxWizardEventHandler( FirstTimeWizard::OnPageChanging ) );

	Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED,	wxCommandEventHandler(FirstTimeWizard::OnUsermodeChanged) );
}

FirstTimeWizard::~FirstTimeWizard()
{
	g_ApplyState.DoCleanup();
}

void FirstTimeWizard::OnPageChanging( wxWizardEvent& evt )
{
	//evt.Skip();
	if( evt.GetDirection() )
	{
		// Moving forward:
		//   Apply settings from the current page...

		int page = (int)evt.GetPage()->GetClientData();

		if( page >= 0 )
		{
			if( !g_ApplyState.ApplyPage( page ) )
			{
				evt.Veto();
				return;
			}
			AppConfig_ReloadGlobalSettings( true );		// ... and overwrite any existing settings

			// [TODO] : The user should be prompted if they want to overwrite existing
			// settings or import them instead.
		}

		if( page == 0 )
		{
			// test plugins folder for validity.  If it doesn't exist or is empty then
			// the user needs to "try again" :)
			
			if( !g_Conf->Folders.Plugins.Exists() )
			{
				Msgbox::Alert( _( "The selected plugins folder does not exist.  You must select a valid PCSX2 plugins folder that exists and contains plugins." ) );
				evt.Veto();
			}
			else
			{
				if( 0 == EnumeratePluginsFolder(NULL) )
				{
					Msgbox::Alert( _( "The selected plugins folder is empty.  You must select a valid PCSX2 plugins folder that actually contains plugins." ) );
					evt.Veto();
				}
			}
		}
	}
}

void FirstTimeWizard::OnPageChanged( wxWizardEvent& evt )
{
	// Plugin Selector needs a special OnShow hack, because Wizard child panels don't
	// receive any Show events >_<
	if( (sptr)evt.GetPage() == (sptr)&m_page_plugins )
		m_panel_PluginSel.OnShow();
}

void FirstTimeWizard::OnUsermodeChanged( wxCommandEvent& evt )
{
	//wxLogError( L"Awesome" );

	m_panel_UsermodeSel.Apply( *g_Conf );	// this assigns the current user mode to g_ApplyState.UseAdminMode
	if( g_ApplyState.UseAdminMode == UseAdminMode ) return;

	UseAdminMode = g_ApplyState.UseAdminMode;
	g_Conf->Folders.ApplyDefaults();
	m_panel_Paths.Reset();
}

/*
wxString biosfolder( Path::Combine( conf.GetDefaultDocumentsFolder(), conf.Folders.Bios ) );

if( wxDir::Exists( biosfolder ) )
{
	if( wxDir::GetAllFiles( biosfolder, NULL, L"*.bin", wxDIR_FILES ) )
}

if( !wxDir::Exists( biosfolder ) )
{
	Msgbox::Alert( ("First Time Installation:\n\n") + BIOS_GetMsg_Required() );
}
*/