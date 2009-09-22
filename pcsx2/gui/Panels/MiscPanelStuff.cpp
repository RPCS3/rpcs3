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

#include "App.h"

#include "ps2/BiosTools.h"
#include <wx/stdpaths.h>

using namespace wxHelpers;

Panels::StaticApplyState Panels::g_ApplyState;

// -----------------------------------------------------------------------
// This method should be called by the parent dalog box of a configuration
// on dialog destruction.  It asserts if the ApplyList hasn't been cleaned up
// and then cleans it up forcefully.
//
void Panels::StaticApplyState::DoCleanup() throw()
{
	wxASSERT_MSG( PanelList.size() != 0, L"PanelList list hasn't been cleaned up." );
	PanelList.clear();
	ParentBook = NULL;
}

void Panels::StaticApplyState::StartBook( wxBookCtrlBase* book )
{
	DevAssert( ParentBook == NULL, "An ApplicableConfig session is already in progress." );
	ParentBook = book;
}

void Panels::StaticApplyState::StartWizard()
{
	DevAssert( ParentBook == NULL, "An ApplicableConfig session is already in progress." );
}

// -----------------------------------------------------------------------
//
// Parameters:
//  pageid - identifier of the page to apply settings for.  All other pages will be
//     skipped.  If pageid is negative (-1) then all pages are applied.
//
// Returns false if one of the panels fails input validation (in which case dialogs
// should not be closed, etc).
//
bool Panels::StaticApplyState::ApplyPage( int pageid, bool saveOnSuccess )
{
	bool retval = true;

	// Save these settings so we can restore them if the Apply fails.

	bool		oldAdminMode		= UseAdminMode;
	wxDirName	oldSettingsFolder	= SettingsFolder;
	bool		oldUseDefSet		= UseDefaultSettingsFolder;

	AppConfig confcopy( *g_Conf );

	try
	{
		PanelApplyList_t::iterator yay = PanelList.begin();
		while( yay != PanelList.end() )
		{
			//DbgCon::Status( L"Writing settings for: " + (*yay)->GetLabel() );
			if( (pageid < 0) || (*yay)->IsOnPage( pageid ) )
				(*yay)->Apply();
			yay++;
		}

		// If an exception is thrown above, this code below won't get run.
		// (conveniently skipping any option application! :D)

		AppApplySettings( &confcopy );
		if( saveOnSuccess )
			AppSaveSettings();
	}
	catch( Exception::CannotApplySettings& ex )
	{
		UseAdminMode = oldAdminMode;
		SettingsFolder = oldSettingsFolder;
		UseDefaultSettingsFolder = oldUseDefSet;
		*g_Conf = confcopy;

		wxMessageBox( ex.FormatDisplayMessage(), _("Cannot apply settings...") );

		if( ex.GetPanel() != NULL )
			ex.GetPanel()->SetFocusToMe();

		retval = false;
	}
	catch( ... )
	{
		UseAdminMode = oldAdminMode;
		SettingsFolder = oldSettingsFolder;
		UseDefaultSettingsFolder = oldUseDefSet;
		*g_Conf = confcopy;

		throw;
	}

	return retval;
}

// Returns false if one of the panels fails input validation (in which case dialogs
// should not be closed, etc).
bool Panels::StaticApplyState::ApplyAll( bool saveOnSuccess )
{
	return ApplyPage( -1, saveOnSuccess );
}

// -----------------------------------------------------------------------
Panels::UsermodeSelectionPanel::UsermodeSelectionPanel( wxWindow& parent, int idealWidth, bool isFirstTime ) :
	BaseApplicableConfigPanel( &parent, idealWidth )
,	m_radio_user( NULL )
,	m_radio_cwd( NULL )
{
	const wxString usermodeExplained( pxE( ".Panels:Usermode:Explained",
		L"Please select your preferred default location for PCSX2 user-level documents below "
		L"(includes memory cards, screenshots, settings, and savestates).  "
		L"These folder locations can be overridden at any time using the Core Settings panel."
	) );

	const wxString usermodeWarning( pxE( ".Panels:Usermode:Warning",
		L"You can change the preferred default location for PCSX2 user-level documents here "
		L"(includes memory cards, screenshots, settings, and savestates).  "
		L"This option only affects Standard Paths which are set to use the installation default value."
	) );

	wxStaticBoxSizer& s_boxer = *new wxStaticBoxSizer( wxVERTICAL, this, _( "Usermode Selection" ) );
	AddStaticText( s_boxer, isFirstTime ? usermodeExplained : usermodeWarning );

	m_radio_user	= &AddRadioButton( s_boxer, _("User Documents (recommended)"),   _("Location: ") + wxStandardPaths::Get().GetDocumentsDir() );
	s_boxer.AddSpacer( 4 );
	m_radio_cwd		= &AddRadioButton( s_boxer, _("Current working folder (intended for developer use only)"), _("Location: ") + wxGetCwd(),
		_("This setting requires administration privileges from your operating system.") );

	s_boxer.AddSpacer( 4 );
	SetSizer( &s_boxer );
}

void Panels::UsermodeSelectionPanel::Apply()
{
	if( !m_radio_cwd->GetValue() && !m_radio_user->GetValue() )
		throw Exception::CannotApplySettings( this, wxLt( "You must select one of the available user modes before proceeding." ) );

	UseAdminMode = m_radio_cwd->GetValue();
}

// -----------------------------------------------------------------------
Panels::LanguageSelectionPanel::LanguageSelectionPanel( wxWindow& parent, int idealWidth ) :
	BaseApplicableConfigPanel( &parent, idealWidth )
,	m_langs()
,	m_picker( NULL )
{
	i18n_EnumeratePackages( m_langs );

	int size = m_langs.size();
	int cursel = 0;
	wxString* compiled = new wxString[size];
	wxString configLangName( wxLocale::GetLanguageName( wxLANGUAGE_DEFAULT ) );

	for( int i=0; i<size; ++i )
	{
		compiled[i].Printf( L"%s", m_langs[i].englishName.c_str() ); //, xltNames[i].c_str() );
		if( m_langs[i].englishName == configLangName )
			cursel = i;
	}

	m_picker = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		size, compiled, wxCB_READONLY | wxCB_SORT );
	m_picker->SetSelection( cursel );

	wxBoxSizer& s_lang = *new wxBoxSizer( wxHORIZONTAL );
	AddStaticText( s_lang, _("Select a language: "), wxALIGN_CENTRE_VERTICAL );
	s_lang.AddSpacer( 5 );
	s_lang.Add( m_picker, SizerFlags::StdSpace() );

	SetSizer( &s_lang );
}

void Panels::LanguageSelectionPanel::Apply()
{
	// The combo box's order is sorted and may not match our m_langs order, so
	// we have to do a string comparison to find a match:

	wxString sel( m_picker->GetString( m_picker->GetSelection() ) );

	g_Conf->LanguageId = wxLANGUAGE_DEFAULT;	// use this if no matches found
	int size = m_langs.size();
	for( int i=0; i<size; ++i )
	{
		if( m_langs[i].englishName == sel )
		{
			g_Conf->LanguageId = m_langs[i].wxLangId;
			break;
		}
	}
}
