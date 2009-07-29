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
#include "ConfigurationPanels.h"

#include <wx/stdpaths.h>

using namespace wxHelpers;

Panels::StaticApplyState Panels::g_ApplyState;

// -----------------------------------------------------------------------
// This method should be called by the parent dalog box of a configuration 
// on dialog destruction.  It asserts if the ApplyList hasn't been cleaned up
// and then cleans it up forcefully.
//
void Panels::StaticApplyState::DoCleanup()
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

// -----------------------------------------------------------------------
// Returns false if one of the panels fails input validation (in which case dialogs
// should not be closed, etc).
//
bool Panels::StaticApplyState::ApplyAll()
{
	bool retval = true;
	try
	{
		AppConfig confcopy( *g_Conf );

		PanelApplyList_t::iterator yay = PanelList.begin();
		while( yay != PanelList.end() )
		{
			//DbgCon::Status( L"Writing settings for: " + (*yay)->GetLabel() );
			(*yay)->Apply( confcopy );
			yay++;
		}

		// If an exception is thrown above, this code below won't get run.
		// (conveniently skipping any option application! :D)

		*g_Conf = confcopy;
		g_Conf->Apply();
		g_Conf->Save();
	}
	catch( Exception::CannotApplySettings& ex )
	{
		wxMessageBox( ex.DisplayMessage(), _("Cannot apply settings...") );
		
		if( ex.GetPanel() != NULL )
			ex.GetPanel()->SetFocusToMe();

		retval = false;
		
	}
	
	return retval;
}

// -----------------------------------------------------------------------
Panels::UsermodeSelectionPanel::UsermodeSelectionPanel( wxWindow* parent, int idealWidth ) : 
	BaseApplicableConfigPanel( parent, idealWidth )
,	m_radio_user( NULL )
,	m_radio_cwd( NULL )
{
	wxStaticBoxSizer& s_boxer = *new wxStaticBoxSizer( wxVERTICAL, this, _( "Usermode Selection" ) );
	AddStaticText( s_boxer, pxE( ".Panels:Usermode:Explained", 
		L"Please select your preferred default location for PCSX2 user-level documents below "
		L"(includes memory cards, screenshots, settings, and savestates).  "
		L"These folder locations can be overridden at any time using the Core Settings panel."
	) );

	m_radio_user	= &AddRadioButton( s_boxer, _("User Documents (recommended)"),   _("Location: ") + wxStandardPaths::Get().GetDocumentsDir() );
	s_boxer.AddSpacer( 4 );
	m_radio_cwd		= &AddRadioButton( s_boxer, _("Current working folder (intended for developer use only)"), _("Location: ") + wxGetCwd() );

	s_boxer.AddSpacer( 4 );
	SetSizerAndFit( &s_boxer );
}

void Panels::UsermodeSelectionPanel::Apply( AppConfig& conf )
{
	if( !m_radio_cwd->GetValue() && !m_radio_user->GetValue() )
		throw Exception::CannotApplySettings( this, wxLt( "You must select one of the available user modes before proceeding." ) );
	conf.UseAdminMode = m_radio_cwd->GetValue();
}

// -----------------------------------------------------------------------
Panels::LanguageSelectionPanel::LanguageSelectionPanel( wxWindow* parent, int idealWidth ) : 
	BaseApplicableConfigPanel( parent, idealWidth )
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
	
	SetSizerAndFit( &s_lang );
}

void Panels::LanguageSelectionPanel::Apply( AppConfig& conf )
{
	// The combo box's order is sorted and may not match our m_langs order, so
	// we have to do a string comparison to find a match:

	wxString sel( m_picker->GetString( m_picker->GetSelection() ) );

	conf.LanguageId = wxLANGUAGE_DEFAULT;	// use this if no matches found
	int size = m_langs.size();
	for( int i=0; i<size; ++i )
	{
		if( m_langs[i].englishName == sel )
		{
			conf.LanguageId = m_langs[i].wxLangId;
			break;
		}
	}
}
