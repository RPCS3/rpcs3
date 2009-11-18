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

using namespace pxSizerFlags;

template< typename WinType >
WinType* FitToDigits( WinType* win, int digits )
{
	int ex;
	win->GetTextExtent( wxString( L'0', digits+1 ), &ex, NULL );
	win->SetMinSize( wxSize( ex+10, wxDefaultCoord ) );		// +10 for text control borders/insets and junk.
	return win;
}

template<>
wxSpinCtrl* FitToDigits<wxSpinCtrl>( wxSpinCtrl* win, int digits )
{
	// HACK!!  The better way would be to create a pxSpinCtrl class that extends wxSpinCtrl and thus
	// have access to wxSpinButton::DoGetBestSize().  But since I don't want to do that, we'll just
	// make/fake it with a value it's pretty common to Win32/GTK/Mac:

	static const int MagicSpinnerSize = 18;

	int ex;
	win->GetTextExtent( wxString( L'0', digits+1 ), &ex, NULL );
	win->SetMinSize( wxSize( ex+10+MagicSpinnerSize, wxDefaultCoord ) );		// +10 for text control borders/insets and junk.
	return win;
}

// Creates a text control which is right-justified and has it's minimum width configured to suit the
// number of digits requested.
wxTextCtrl* CreateNumericalTextCtrl( wxWindow* parent, int digits )
{
	wxTextCtrl* ctrl = new wxTextCtrl( parent, wxID_ANY );
	ctrl->SetWindowStyleFlag( wxTE_RIGHT );
	FitToDigits( ctrl, digits );
	return ctrl;
}

Panels::FramelimiterPanel::FramelimiterPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	wxSizer& s_main( *GetSizer() );

	m_check_LimiterDisable = new pxCheckBox( this, _("Disable Framelimiting"),
		_("Useful for running benchmarks. Toggle this option in-game by pressing F4.") );
	
	m_check_LimiterDisable->SetToolTip( pxE( ".Tooltip:Framelimiter:Disable",
		L"Note that when Framelimiting is disabled, Turbo and SlowMotion modes will not "
		L"be available either."
	) );

	AddStaticText( s_main, pxE( ".Framelimiter:Heading",
		L"The internal framelimiter regulates the speed of the virtual machine. Adjustment values below are in "
		L"percentages of the default region-based framerate, which can also be configured below."
	) );

	s_main.Add( m_check_LimiterDisable );

	m_spin_NominalPct	= FitToDigits( new wxSpinCtrl( this ), 6 );
	m_spin_SlomoPct		= FitToDigits( new wxSpinCtrl( this ), 6 );
	m_spin_TurboPct		= FitToDigits( new wxSpinCtrl( this ), 6 );

	m_text_BaseNtsc		= CreateNumericalTextCtrl( this, 7 );
	m_text_BasePal		= CreateNumericalTextCtrl( this, 7 );

	wxFlexGridSizer& s_spins = *new wxFlexGridSizer( 5 );

	//s_spins.AddGrowableCol( 0, 1 );
	//s_spins.AddGrowableCol( 1, 1 );

	AddStaticText( s_spins, _("Base Framerate Adjust:"), wxALIGN_LEFT );
	s_spins.AddSpacer( 5 );
	s_spins.Add( m_spin_NominalPct, wxSizerFlags().Border(wxTOP, 3) );
	s_spins.Add( new wxStaticText( this, wxID_ANY, L"%" ), StdSpace() );
	s_spins.AddSpacer( 5 );

	AddStaticText( s_spins, _("Slow Motion Adjust:"), wxALIGN_LEFT );
	s_spins.AddSpacer( 5 );
	s_spins.Add( m_spin_SlomoPct, wxSizerFlags().Border(wxTOP, 3) );
	s_spins.Add( new wxStaticText( this, wxID_ANY, L"%" ), StdSpace() );
	s_spins.AddSpacer( 5 );

	AddStaticText( s_spins, _("Turbo Adjust:"), wxALIGN_LEFT );
	s_spins.AddSpacer( 5 );
	s_spins.Add( m_spin_TurboPct, wxSizerFlags().Border(wxTOP, 3) );
	s_spins.Add( new wxStaticText( this, wxID_ANY, L"%" ), StdSpace() );
	s_spins.AddSpacer( 5 );

	s_spins.AddSpacer( 15 );
	s_spins.AddSpacer( 15 );
	s_spins.AddSpacer( 15 );
	s_spins.AddSpacer( 15 );
	s_spins.AddSpacer( 15 );

	AddStaticText( s_spins, _("NTSC Framerate:"), wxALIGN_LEFT );
	s_spins.AddSpacer( 5 );
	s_spins.Add( m_text_BaseNtsc, wxSizerFlags().Align(wxALIGN_RIGHT).Border(wxTOP, 3) );
	s_spins.Add( new wxStaticText( this, wxID_ANY, _("FPS") ), StdSpace() );
	s_spins.AddSpacer( 5 );

	AddStaticText( s_spins, _("PAL Framerate:"), wxALIGN_LEFT );
	s_spins.AddSpacer( 5 );
	s_spins.Add( m_text_BasePal, wxSizerFlags().Align(wxALIGN_RIGHT).Border(wxTOP, 3) );
	s_spins.Add( new wxStaticText( this, wxID_ANY, _("FPS") ), StdSpace() );
	s_spins.AddSpacer( 5 );

	s_main.Add( &s_spins );
	

	m_spin_NominalPct->SetValue( 100 );
	m_spin_SlomoPct->SetValue( 50 );
	m_spin_TurboPct->SetValue( 100 );

	m_text_BaseNtsc->SetValue( L"59.94" );
	m_text_BasePal->SetValue( L"50.00" );

}

void Panels::FramelimiterPanel::Apply()
{
}


Panels::GSWindowSettingsPanel::GSWindowSettingsPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	m_text_WindowWidth	= CreateNumericalTextCtrl( this, 5 );
	m_text_WindowHeight	= CreateNumericalTextCtrl( this, 5 );

	m_check_CloseGS		= new pxCheckBox( this, _("Hide GS window") );

	m_check_AspectLock	= new pxCheckBox( this, _("Lock Aspect Ratio") );

	m_check_SizeLock	= new pxCheckBox( this, _("Lock Size"),
		pxE( ".Tooltips:Video:LockSize",
			L"Disables the resize border for the GS window."
		)
	);

	m_check_VsyncEnable	= new pxCheckBox( this, _("Wait for Vsync"),
		pxE( ".Tooltips:Video:Vsync",
			L"This reduces screen/refresh tearing but typically has a big performance hit.  "
			L"It may not work in windowed modes, and may not be supported by all GS plugins."
		)
	);

	m_check_Fullscreen	= new pxCheckBox( this, _("Force Fullscreen at Startup"),
		pxE( ".Panels:Video:Fullscreen",
			L"Enables automatic modeswitch to fullscreen when starting or resuming emulation."
		)
	);

	m_check_CloseGS->SetToolTip( pxE( ".Tooltips:Video:HideGS",
		L"Completely closes the often large and bulky GS window when pressing "
		L"ESC or suspending the emulator.  Might prevent memory leaks too, if you use DX10."
	) );
	
	// ----------------------------------------------------------------------------
	//  Layout and Positioning

	wxBoxSizer& s_customsize( *new wxBoxSizer( wxHORIZONTAL ) );
	s_customsize.Add( m_text_WindowWidth );
	AddStaticText( s_customsize, _("x") );
	s_customsize.Add( m_text_WindowHeight );

	//wxFlexGridSizer& s_winsize( *new wxFlexGridSizer( 2 ) );
	//s_winsize.AddGrowableCol( 0 );

	wxStaticBoxSizer& s_winsize( *new wxStaticBoxSizer( wxVERTICAL, this, _("Window Size:") ) );
	//AddStaticText( s_winsize, _("Window Size:") );
	AddStaticText( s_winsize, _("Custom Window Size: "), wxALIGN_LEFT );
	s_winsize.Add( &s_customsize, StdSpace().Border( wxLEFT | wxRIGHT | wxBOTTOM) );


	wxSizer& s_main( *GetSizer() );

	s_main.Add( &s_winsize, StdSpace() );
	
	s_main.Add( m_check_SizeLock );
	s_main.Add( m_check_AspectLock );
	s_main.Add( m_check_Fullscreen );
	s_main.Add( m_check_CloseGS );
	s_main.Add( m_check_VsyncEnable );
	
	
	m_text_WindowWidth->SetValue( L"640" );
	m_text_WindowHeight->SetValue( L"480" );
	m_check_CloseGS->SetValue( g_Conf->CloseGSonEsc );

}

void Panels::GSWindowSettingsPanel::Apply()
{
}

Panels::VideoPanel::VideoPanel( wxWindow* parent ) :
	BaseApplicableConfigPanel( parent )
{
	wxPanelWithHelpers* left	= new wxPanelWithHelpers( this, wxVERTICAL );
	wxPanelWithHelpers* right	= new wxPanelWithHelpers( this, wxVERTICAL );
	left->SetIdealWidth( (left->GetIdealWidth()-16) / 2 );
	right->SetIdealWidth( (right->GetIdealWidth()-16) / 2 );

	GSWindowSettingsPanel* winpan = new GSWindowSettingsPanel( left );
	winpan->AddStaticBox(_("Display/Window"));

	FramelimiterPanel* fpan = new FramelimiterPanel( right );
	fpan->AddStaticBox(_("Framelimiter"));
		
	wxSizer& s_main( *GetSizer() );
	wxFlexGridSizer* s_table = new wxFlexGridSizer( 2 );

	left->GetSizer()->Add( winpan, wxSizerFlags().Expand() );
	right->GetSizer()->Add( fpan, wxSizerFlags().Expand() );
	
	s_table->Add( left, StdExpand() );
	s_table->Add( right, StdExpand() );

	s_main.Add( s_table );
	
	// TODO:
	// Framelimiting / Frameskipping / Vsync
	// GS Window Options ( incl. Fullscreen )
	// MTGS Forced Synchronization
}

void Panels::VideoPanel::Apply()
{
}
