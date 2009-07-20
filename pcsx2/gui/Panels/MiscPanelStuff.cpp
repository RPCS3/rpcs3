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
Panels::UsermodeSelectionPanel::UsermodeSelectionPanel( wxWindow* parent ) : 
	BaseApplicableConfigPanel( parent )
,	m_radio_user( NULL )
,	m_radio_cwd( NULL )
{
	wxStaticBoxSizer& s_boxer = *new wxStaticBoxSizer( wxVERTICAL, this, _( "Usermode Selection" ) );
	AddStaticText( s_boxer,
		L"Please select your preferred default location for PCSX2 user-level documents below "
		L"(includes memory cards, screenshots, settings, and savestates).  "
		L"These folder locations can be overridden at any time using the Core Settings panel.",
	480, wxALIGN_CENTRE );

	m_radio_user	= &AddRadioButton( s_boxer, _("User Documents (recommended)"),   _("Location: ") + wxStandardPaths::Get().GetDocumentsDir() );
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
