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

#include <wx/spinctrl.h>

using namespace wxHelpers;

Panels::FramelimiterPanel::FramelimiterPanel( wxWindow& parent, int idealWidth ) :
	BaseApplicableConfigPanel( &parent, idealWidth )
{
	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );

	m_check_LimiterDisable = new pxCheckBox( this, _("Disable Framelimiting"),
		_("Useful for running benchmarks. Toggle this option in-game by pressing F4.") );
	
	m_check_LimiterDisable->SetToolTip( pxE( ".Tooltip:Framelimiter:Disable",
		L"Note that when Framelimiting is disabled, Turbo and SlowMotion modes will not "
		L"be available either."
	) );

	AddStaticText( mainSizer, pxE( ".Framelimiter:Heading",
		L"The internal framelimiter regulates the speed of the virtual machine. Adjustment values below are in "
		L"percentages of the default region-based framerate, which can also be configured below."
	) );

	mainSizer.Add( m_check_LimiterDisable );

	m_spin_NominalPct	= new wxSpinCtrl( this );
	m_spin_SlomoPct		= new wxSpinCtrl( this );
	m_spin_TurboPct		= new wxSpinCtrl( this );

	(m_text_BaseNtsc	= new wxTextCtrl( this, wxID_ANY ))->SetWindowStyleFlag( wxTE_RIGHT );
	(m_text_BasePal		= new wxTextCtrl( this, wxID_ANY ))->SetWindowStyleFlag( wxTE_RIGHT );

	m_spin_NominalPct->SetValue( 100 );
	m_spin_SlomoPct->SetValue( 50 );
	m_spin_TurboPct->SetValue( 100 );
	
	m_text_BaseNtsc->SetValue( L"59.94" );
	m_text_BasePal->SetValue( L"50.00" );

	wxFlexGridSizer& s_spins = *new wxFlexGridSizer( 5 );

	//s_spins.AddGrowableCol( 0, 1 );
	//s_spins.AddGrowableCol( 1, 1 );

	AddStaticText( s_spins, _("Base Framerate Adjust:"), wxALIGN_LEFT );
	s_spins.AddSpacer( 5 );
	s_spins.Add( m_spin_NominalPct, wxSizerFlags().Border(wxTOP, 3) );
	s_spins.Add( new wxStaticText( this, wxID_ANY, L"%" ), SizerFlags::StdSpace() );
	s_spins.AddSpacer( 5 );

	AddStaticText( s_spins, _("Slow Motion Adjust:"), wxALIGN_LEFT );
	s_spins.AddSpacer( 5 );
	s_spins.Add( m_spin_SlomoPct, wxSizerFlags().Border(wxTOP, 3) );
	s_spins.Add( new wxStaticText( this, wxID_ANY, L"%" ), SizerFlags::StdSpace() );
	s_spins.AddSpacer( 5 );

	AddStaticText( s_spins, _("Turbo Adjust:"), wxALIGN_LEFT );
	s_spins.AddSpacer( 5 );
	s_spins.Add( m_spin_TurboPct, wxSizerFlags().Border(wxTOP, 3) );
	s_spins.Add( new wxStaticText( this, wxID_ANY, L"%" ), SizerFlags::StdSpace() );
	s_spins.AddSpacer( 5 );

	s_spins.AddSpacer( 15 );
	s_spins.AddSpacer( 15 );
	s_spins.AddSpacer( 15 );
	s_spins.AddSpacer( 15 );
	s_spins.AddSpacer( 15 );

	AddStaticText( s_spins, _("NTSC Framerate:"), wxALIGN_LEFT );
	s_spins.AddSpacer( 5 );
	s_spins.Add( m_text_BaseNtsc, wxSizerFlags().Border(wxTOP, 3) );
	s_spins.Add( new wxStaticText( this, wxID_ANY, _("FPS") ), SizerFlags::StdSpace() );
	s_spins.AddSpacer( 5 );

	AddStaticText( s_spins, _("PAL Framerate:"), wxALIGN_LEFT );
	s_spins.AddSpacer( 5 );
	s_spins.Add( m_text_BasePal, wxSizerFlags().Border(wxTOP, 3) );
	s_spins.Add( new wxStaticText( this, wxID_ANY, _("FPS") ), SizerFlags::StdSpace() );
	s_spins.AddSpacer( 5 );

	mainSizer.Add( &s_spins );
	
	SetSizer( &mainSizer );

}

void Panels::FramelimiterPanel::Apply()
{
}


Panels::VideoPanel::VideoPanel( wxWindow& parent, int idealWidth ) :
	BaseApplicableConfigPanel( &parent, idealWidth )
{
	m_check_CloseGS =  new pxCheckBox( this, _("Hide GS window on Suspend") );
	
	m_check_CloseGS->SetToolTip( pxE( ".Tooltip:Video:HideGS",
		L"Completely closes the often large and bulky GS window when pressing "
		L"ESC or suspending the emulator.  That way it won't get *in* the way!"
	) );

	/*&AddCheckBox( mainSizer, _(""),
		wxEmptyString,		// subtext
		pxE( ".Tooltip:Video:HideGS",
			L"Completely closes the often large and bulky GS window when pressing "
			L"ESC or suspending the emulator.  That way it won't get *in* the way!"
		)
	);*/

	m_check_CloseGS->SetValue( g_Conf->CloseGSonEsc );

	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );
	wxStaticBoxSizer& limitSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("Framelimiter") );
	
	limitSizer.Add( new FramelimiterPanel( *this, idealWidth - 32 ) );

	mainSizer.Add( m_check_CloseGS );
	mainSizer.Add( &limitSizer );

	SetSizer( &mainSizer );
	
	// TODO:
	// Framelimiting / Frameskipping / Vsync
	// GS Window Options ( incl. Fullscreen )
	// MTGS Forced Synchronization
}

void Panels::VideoPanel::Apply()
{
}
