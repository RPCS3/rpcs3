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
#include <wx/hyperlink.h>

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
Panels::SettingsDirPickerPanel::SettingsDirPickerPanel( wxWindow* parent )
	: DirPickerPanel( parent, FolderId_Settings, _("Settings"), AddAppName(_("Select a folder for %s settings")) )
{
	pxSetToolTip( this, pxEt( "!ContextTip:Folders:Settings",
		L"This is the folder where PCSX2 saves your settings, including settings generated "
		L"by most plugins (some older plugins may not respect this value)."
	) );

	SetStaticDesc( pxE( "!Panel:Folders:Settings",
		L"You may optionally specify a location for your PCSX2 settings here.  If the location "
		L"contains existing PCSX2 settings, you will be given the option to import or overwrite them."
	) );
}

namespace Panels
{

	class FirstTimeIntroPanel : public wxPanelWithHelpers
	{
	public:
		FirstTimeIntroPanel( wxWindow* parent );
		virtual ~FirstTimeIntroPanel() throw() {}
	};
}

Panels::FirstTimeIntroPanel::FirstTimeIntroPanel( wxWindow* parent )
	: wxPanelWithHelpers( parent, wxVERTICAL )
{
	SetMinWidth( 600 );

	FastFormatUnicode faqFile;
	faqFile.Write( L"file:///%s/Docs/PCSX2 FAQ %u.%u.%u.pdf",
		InstallFolder.ToString().c_str(), PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo
	);

	wxStaticBoxSizer& langSel	= *new wxStaticBoxSizer( wxVERTICAL, this, _("Language selector") );

	langSel += new Panels::LanguageSelectionPanel( this ) | StdCenter();
	langSel += Heading(_("Change the language only if you need to.\nThe system default should be fine for most operating systems."));
	langSel += 8;

	*this += langSel | StdExpand();
	*this += GetCharHeight() * 2;

	*this += Heading(AddAppName(L"Welcome to %s!")).Bold();
	*this += GetCharHeight();

	*this += Heading(AddAppName(
		pxE( "!Wizard:Welcome",
			L"This wizard will help guide you through configuring plugins, "
			L"memory cards, and BIOS.  It is recommended if this is your first time installing "
			L"%s that you view the readme and configuration guide."
		) )
	);

	*this += GetCharHeight() * 2;

	*this	+= new wxHyperlinkCtrl( this, wxID_ANY,
		_("Configuration Guides (online)"), L"http://www.pcsx2.net/guide.php"
	) | pxCenter.Border( wxALL, 5 );
		
	*this	+= new wxHyperlinkCtrl( this, wxID_ANY,
		_("Readme / FAQ (Offline/PDF)"), faqFile.c_str()
	) | pxCenter.Border( wxALL, 5 );

}

// --------------------------------------------------------------------------------------
//  FirstTimeWizard  (implementations)
// --------------------------------------------------------------------------------------
FirstTimeWizard::FirstTimeWizard( wxWindow* parent )
	: wxWizard( parent, wxID_ANY, AddAppName(_("%s First Time Configuration")) )
	, m_page_intro		( *new ApplicableWizardPage( this ) )
	, m_page_plugins	( *new ApplicableWizardPage( this, &m_page_intro ) )
	, m_page_bios		( *new ApplicableWizardPage( this, &m_page_plugins ) )

	, m_panel_Intro		( *new FirstTimeIntroPanel( &m_page_intro ))
	, m_panel_PluginSel	( *new PluginSelectorPanel( &m_page_plugins ) )
	, m_panel_BiosSel	( *new BiosSelectorPanel( &m_page_bios ) )
{
	// Page 2 - Plugins Panel
	// Page 3 - Bios Panel

	m_page_intro.	SetSizer( new wxBoxSizer( wxVERTICAL ) );
	m_page_plugins.	SetSizer( new wxBoxSizer( wxVERTICAL ) );
	m_page_bios.	SetSizer( new wxBoxSizer( wxVERTICAL ) );

	m_page_intro	+= m_panel_Intro			| StdExpand();
	m_page_plugins	+= m_panel_PluginSel		| StdExpand();
	m_page_bios		+= m_panel_BiosSel			| StdExpand();

	// Temporary tutorial message for the BIOS, needs proof-reading!!
	m_page_bios		+= 12;
	m_page_bios		+= new pxStaticHeading( &m_page_bios,
		pxE( "!Wizard:Bios:Tutorial",
			L"PCSX2 requires a *legal* copy of the PS2 BIOS in order to run games.\n"
			L"You cannot use a copy obtained from a friend or the Internet.\n"
			L"You must dump the BIOS from your *own* Playstation 2 console."
		)
	) | StdExpand();

	// Assign page indexes as client data
	m_page_intro	.SetClientData( (void*)0 );
	m_page_plugins	.SetClientData( (void*)1 );
	m_page_bios		.SetClientData( (void*)2 );

	// Build the forward chain:
	//  (backward chain is built during initialization above)
	m_page_intro	.SetNext( &m_page_plugins );
	m_page_plugins	.SetNext( &m_page_bios );

	GetPageAreaSizer() += m_page_intro;
	GetPageAreaSizer() += m_page_plugins;

	// this doesn't descent from wxDialogWithHelpers, so we need to explicitly
	// fit and center it. :(

	Connect( wxEVT_WIZARD_PAGE_CHANGED,				wxWizardEventHandler	(FirstTimeWizard::OnPageChanged) );
	Connect( wxEVT_WIZARD_PAGE_CHANGING,			wxWizardEventHandler	(FirstTimeWizard::OnPageChanging) );
	Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED,	wxCommandEventHandler	(FirstTimeWizard::OnDoubleClicked) );

	Connect( pxID_RestartWizard,	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( FirstTimeWizard::OnRestartWizard ) );
}

FirstTimeWizard::~FirstTimeWizard() throw()
{

}

void FirstTimeWizard::OnRestartWizard( wxCommandEvent& evt )
{
	EndModal( pxID_RestartWizard );
	evt.Skip();
}

static void _OpenConsole()
{
	g_Conf->ProgLogBox.Visible = true;
	wxGetApp().OpenProgramLog();
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
			if( wxFile::Exists(GetUiSettingsFilename()) || wxFile::Exists(GetVmSettingsFilename()) )
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
