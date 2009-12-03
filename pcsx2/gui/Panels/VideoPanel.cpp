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

// --------------------------------------------------------------------------------------
//  FramelimiterPanel Implementations
// --------------------------------------------------------------------------------------

Panels::FramelimiterPanel::FramelimiterPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	m_check_LimiterDisable = new pxCheckBox( this, _("Disable Framelimiting"),
		_("Useful for running benchmarks. Toggle this option in-game by pressing F4.") );
	
	m_check_LimiterDisable->SetToolTip( pxE( ".Tooltip:Framelimiter:Disable",
		L"Note that when Framelimiting is disabled, Turbo and SlowMotion modes will not "
		L"be available either."
	) );

	m_spin_NominalPct	= FitToDigits( new wxSpinCtrl( this ), 6 );
	m_spin_SlomoPct		= FitToDigits( new wxSpinCtrl( this ), 6 );
	m_spin_TurboPct		= FitToDigits( new wxSpinCtrl( this ), 6 );

	m_text_BaseNtsc		= CreateNumericalTextCtrl( this, 7 );
	m_text_BasePal		= CreateNumericalTextCtrl( this, 7 );

	m_spin_NominalPct	->SetRange( 10,  1000 );
	m_spin_SlomoPct		->SetRange(  1,  1000 );
	m_spin_TurboPct		->SetRange( 10,  1000 );

	// ------------------------------------------------------------
	// Sizers and Layouts

	*this += new pxStaticHeading( this, pxE( ".Framelimiter:Heading",
		L"The internal framelimiter regulates the speed of the virtual machine. Adjustment values below are in "
		L"percentages of the default region-based framerate, which can also be configured below." )
	);

	*this += m_check_LimiterDisable;

	wxFlexGridSizer& s_spins( *new wxFlexGridSizer( 5 ) );
	s_spins.AddGrowableCol( 0 );

	s_spins += Text(_("Base Framerate Adjust:"));
	s_spins += 5;
	s_spins += m_spin_NominalPct	| wxSF.Border(wxTOP, 3);
	s_spins += Text(L"%" );
	s_spins += 5;

	s_spins += Text(_("Slow Motion Adjust:"));
	s_spins += 5;
	s_spins += m_spin_SlomoPct		| wxSF.Border(wxTOP, 3);
	s_spins += Text(L"%" );
	s_spins += 5;

	s_spins	+= Text(_("Turbo Adjust:"));
	s_spins	+= 5;
	s_spins	+= m_spin_TurboPct		| wxSF.Border(wxTOP, 3);
	s_spins	+= Text(L"%" );
	s_spins	+= 5;

	s_spins	+= 15;
	s_spins	+= 15;
	s_spins	+= 15;
	s_spins	+= 15;
	s_spins	+= 15;

	wxFlexGridSizer& s_fps( *new wxFlexGridSizer( 5 ) );
	s_fps.AddGrowableCol( 0 );

	s_fps	+= Text(_("NTSC Framerate:"));
	s_fps	+= 5;
	s_fps	+= m_text_BaseNtsc	| wxSF.Right().Border(wxTOP, 3);
	s_fps	+= Text(_("FPS"));
	s_fps	+= 5;

	s_fps	+= Text(_("PAL Framerate:"));
	s_fps	+= 5;
	s_fps	+= m_text_BasePal	| wxSF.Right().Border(wxTOP, 3);
	s_fps	+= Text(_("FPS"));
	s_fps	+= 5;

	*this	+= s_spins	| pxExpand;
	*this	+= s_fps	| pxExpand;
	
	m_spin_NominalPct	->SetValue( 100 );
	m_spin_SlomoPct		->SetValue( 50 );
	m_spin_TurboPct		->SetValue( 100 );

	m_text_BaseNtsc		->SetValue( L"59.94" );
	m_text_BasePal		->SetValue( L"50.00" );
	
	OnSettingsChanged();
}

// --------------------------------------------------------------------------------------
//  GSWindowSetting Implementation
// --------------------------------------------------------------------------------------

Panels::GSWindowSettingsPanel::GSWindowSettingsPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	const wxString aspect_ratio_labels[] =
	{
		_("Fit to Window/Screen"),
		_("Standard (4:3)"),
		_("Widescreen (16:9)")
	};

	m_combo_AspectRatio	= new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		ArraySize(aspect_ratio_labels), aspect_ratio_labels, wxCB_READONLY );

	m_text_WindowWidth	= CreateNumericalTextCtrl( this, 5 );
	m_text_WindowHeight	= CreateNumericalTextCtrl( this, 5 );

	m_check_SizeLock	= new pxCheckBox( this, _("Disable window resize border") );
	m_check_HideMouse	= new pxCheckBox( this, _("Always hide mouse cursor") );
	m_check_CloseGS		= new pxCheckBox( this, _("Hide window on suspend") );
	m_check_Fullscreen	= new pxCheckBox( this, _("Default to Fullscreen") );
	m_check_VsyncEnable	= new pxCheckBox( this, _("Vsync Enable") );
	
	m_check_VsyncEnable->SetToolTip( pxE( ".Tooltips:Video:Vsync",
		L"Vsync eliminates screen tearing but typically has a big performance hit. "
		L"It usually only applies to fullscreen mode, and may not work with all GS plugins."
	) );
	
	m_check_HideMouse->SetToolTip( pxE( ".Tooltips:Video:HideMouse",
		L"Check this to force the mouse cursor invisible inside the GS window; useful if using "
		L"the mouse as a primary control device for gaming.  By default the mouse auto-hides after "
		L"3 seconds of inactivity."
	) );

	m_check_Fullscreen->SetToolTip( pxE( ".Tooltips:Video:Fullscreen",
		L"Enables automatic mode switch to fullscreen when starting or resuming emulation. "
		L"You can still toggle fullscreen display at any time using alt-enter."
	) );

	m_check_CloseGS->SetToolTip( pxE( ".Tooltips:Video:HideGS",
		L"Completely closes the often large and bulky GS window when pressing "
		L"ESC or suspending the emulator."
	) );
	
	// ----------------------------------------------------------------------------
	//  Layout and Positioning

	wxBoxSizer& s_customsize( *new wxBoxSizer( wxHORIZONTAL ) );
	s_customsize	+= m_text_WindowWidth;
	s_customsize	+= Text( L"x" );
	s_customsize	+= m_text_WindowHeight;

	wxFlexGridSizer& s_AspectRatio( *new wxFlexGridSizer( 2, StdPadding, StdPadding ) );
	//s_AspectRatio.AddGrowableCol( 0 );
	s_AspectRatio.AddGrowableCol( 1 );

	s_AspectRatio += Text(_("Aspect Ratio:"))		| pxMiddle;
	s_AspectRatio += m_combo_AspectRatio			| pxExpand;
	s_AspectRatio += Text(_("Custom Window Size:"))	| pxMiddle;
	s_AspectRatio += s_customsize					| pxAlignRight;

	*this += s_AspectRatio | StdExpand();
	*this += m_check_SizeLock;
	*this += new wxStaticLine( this ) | StdExpand();

	*this += m_check_Fullscreen;
	*this += m_check_VsyncEnable;
	*this += m_check_HideMouse;
	*this += m_check_CloseGS;

	OnSettingsChanged();
}

// --------------------------------------------------------------------------------------
//  VideoPanel Implementation
// --------------------------------------------------------------------------------------

Panels::VideoPanel::VideoPanel( wxWindow* parent ) :
	BaseApplicableConfigPanel( parent )
{
	wxPanelWithHelpers* left	= new wxPanelWithHelpers( this, wxVERTICAL );
	wxPanelWithHelpers* right	= new wxPanelWithHelpers( this, wxVERTICAL );
	left->SetIdealWidth( (left->GetIdealWidth()-12) / 2 );
	right->SetIdealWidth( (right->GetIdealWidth()-12) / 2 );

	m_check_SynchronousGS = new pxCheckBox( left, _("Synchronized MTGS"),
		_("For troubleshooting potential bugs in the MTGS only, as it is potentially very slow.")
	);
	
	m_check_SynchronousGS->SetToolTip(_("Enable this if you think the MTGS is causing crashes or graphical errors."));

	GSWindowSettingsPanel* winpan = new GSWindowSettingsPanel( left );
	winpan->AddFrame(_("Display/Window"));

	FramelimiterPanel* fpan = new FramelimiterPanel( right );
	fpan->AddFrame(_("Framelimiter"));
		
	wxFlexGridSizer* s_table = new wxFlexGridSizer( 2 );
	s_table->AddGrowableCol( 0 );
	s_table->AddGrowableCol( 1 );

	*left		+= winpan	| pxExpand;
	*right		+= fpan		| pxExpand;

	*left		+= 5;
	*left		+= m_check_SynchronousGS;
	
	*s_table	+= left		| StdExpand();
	*s_table	+= right	| StdExpand();

	*this		+= s_table	| pxExpand;

	m_check_SynchronousGS->SetValue( g_Conf->EmuOptions.GS.SynchronousMTGS );
}

void Panels::VideoPanel::Apply()
{
	g_Conf->EmuOptions.GS.SynchronousMTGS = m_check_SynchronousGS->GetValue();
}

void Panels::GSWindowSettingsPanel::OnSettingsChanged()
{
	const AppConfig::GSWindowOptions& conf( g_Conf->GSWindow );

	m_check_CloseGS		->SetValue( conf.CloseOnEsc );
	m_check_Fullscreen	->SetValue( conf.DefaultToFullscreen );
	m_check_HideMouse	->SetValue( conf.AlwaysHideMouse );
	m_check_SizeLock	->SetValue( conf.DisableResizeBorders );

	m_combo_AspectRatio	->SetSelection( (int)conf.AspectRatio );

	m_check_VsyncEnable	->SetValue( g_Conf->EmuOptions.GS.VsyncEnable );

	m_text_WindowWidth	->SetValue( wxsFormat( L"%d", conf.WindowSize.GetWidth() ) );
	m_text_WindowHeight	->SetValue( wxsFormat( L"%d", conf.WindowSize.GetHeight() ) );

}

void Panels::GSWindowSettingsPanel::Apply()
{
	AppConfig::GSWindowOptions& appconf( g_Conf->GSWindow );
	Pcsx2Config::GSOptions& gsconf( g_Conf->EmuOptions.GS );

	appconf.CloseOnEsc				= m_check_CloseGS	->GetValue();
	appconf.DefaultToFullscreen		= m_check_Fullscreen->GetValue();
	appconf.AlwaysHideMouse			= m_check_HideMouse	->GetValue();
	appconf.DisableResizeBorders	= m_check_SizeLock	->GetValue();

	appconf.AspectRatio		= (AspectRatioType)m_combo_AspectRatio->GetSelection();

	gsconf.VsyncEnable		= m_check_VsyncEnable->GetValue();

	long xr, yr;

	if( !m_text_WindowWidth->GetValue().ToLong( &xr ) || !m_text_WindowHeight->GetValue().ToLong( &yr ) )
		throw Exception::CannotApplySettings( this,
			L"User submitted non-numeric window size parameters!",
			_("Invalid window dimensions specified: Size cannot contain non-numeric digits! >_<")
		);

	appconf.WindowSize.x	= xr;
	appconf.WindowSize.y	= yr;
}


void Panels::FramelimiterPanel::OnSettingsChanged()
{
	const AppConfig::GSWindowOptions& appconf( g_Conf->GSWindow );
	const Pcsx2Config::GSOptions& gsconf( g_Conf->EmuOptions.GS );

	m_check_LimiterDisable->SetValue( !gsconf.FrameLimitEnable );

	m_spin_NominalPct	->SetValue( (appconf.NominalScalar * 100).ToIntRounded() );
	m_spin_TurboPct		->SetValue( (appconf.TurboScalar * 100).ToIntRounded() );
	m_spin_TurboPct		->SetValue( (appconf.SlomoScalar * 100).ToIntRounded() );
	
	m_text_BaseNtsc		->SetValue( gsconf.FramerateNTSC.ToString() );
	m_text_BasePal		->SetValue( gsconf.FrameratePAL.ToString() );
}

void Panels::FramelimiterPanel::Apply()
{
	AppConfig::GSWindowOptions& appconf( g_Conf->GSWindow );
	Pcsx2Config::GSOptions& gsconf( g_Conf->EmuOptions.GS );

	gsconf.FrameLimitEnable	= !m_check_LimiterDisable->GetValue();

	appconf.NominalScalar	= m_spin_NominalPct	->GetValue();
	appconf.TurboScalar		= m_spin_TurboPct	->GetValue();
	appconf.SlomoScalar		= m_spin_SlomoPct	->GetValue();

	double ntsc, pal;
	if( !m_text_BaseNtsc->GetValue().ToDouble( &ntsc ) || 
		!m_text_BasePal	->GetValue().ToDouble( &pal )
	)
		throw Exception::CannotApplySettings( this, wxLt("Error while parsing either NTSC or PAL framerate settings.  Settings must be valid floating point numerics.") );

	gsconf.FramerateNTSC	= ntsc;
	gsconf.FrameratePAL	= pal;
}
