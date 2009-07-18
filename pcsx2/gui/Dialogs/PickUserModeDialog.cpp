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
#include "ModalPopups.h"

#include <wx/stdpaths.h>

using namespace wxHelpers;

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
		throw Exception::CannotApplySettings( wxLt( "You must select one of the available user modes before proceeding." ) );
	conf.UseAdminMode = m_radio_cwd->GetValue();
}


Dialogs::PickUserModeDialog::PickUserModeDialog( wxWindow* parent, int id ) :
	wxDialogWithHelpers( parent, id, _("PCSX2 First Time configuration"), false )
,	m_panel_usersel( new Panels::UsermodeSelectionPanel( this ) )
{
	wxBoxSizer& s_main = *new wxBoxSizer( wxVERTICAL );

	AddStaticText( s_main, _("PCSX2 is starting from a new or unknown folder and needs to be configured."),
		0, wxALIGN_CENTRE );
	s_main.AddSpacer( 8 );
	s_main.Add( m_panel_usersel, SizerFlags::StdGroupie() );

	//new wxListCt

	AddOkCancel( s_main );
	SetSizerAndFit( &s_main );
	
	Connect( wxID_OK,		wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PickUserModeDialog::OnOk_Click ) );
}

void Dialogs::PickUserModeDialog::OnOk_Click( wxCommandEvent& evt )
{
	AppConfig confcopy( *g_Conf );

	try
	{
		m_panel_usersel->Apply( confcopy );

		*g_Conf = confcopy;
		g_Conf->Apply();
		g_Conf->Save();

		Close();
		evt.Skip();
	}
	catch( Exception::CannotApplySettings& ex )
	{
		wxMessageBox( ex.DisplayMessage() + L"\n" + _("You may press Cancel to close the program."), _("Cannot apply settings...") );
	}
}
