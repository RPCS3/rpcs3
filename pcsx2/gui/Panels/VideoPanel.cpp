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
#include "App.h"
#include "Dialogs/ConfigurationDialog.h"
#include "ConfigurationPanels.h"

#include <wx/spinctrl.h>

using namespace pxSizerFlags;

// --------------------------------------------------------------------------------------
//  FramelimiterPanel Implementations
// --------------------------------------------------------------------------------------

Panels::FramelimiterPanel::FramelimiterPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	SetMinWidth( 280 );

	m_check_LimiterDisable = new pxCheckBox( this, _("Disable Framelimiting"),
		_("Useful for running benchmarks. Toggle this option in-game by pressing F4.") );

	m_check_LimiterDisable->SetToolTip( pxEt( "!ContextTip:Framelimiter:Disable",
		L"Note that when Framelimiting is disabled, Turbo and SlowMotion modes will not "
		L"be available either."
	) );

	pxFitToDigits( m_spin_NominalPct	= new wxSpinCtrl( this ), 6 );
	pxFitToDigits( m_spin_SlomoPct		= new wxSpinCtrl( this ), 6 );
	pxFitToDigits( m_spin_TurboPct		= new wxSpinCtrl( this ), 6 );

	m_text_BaseNtsc		= CreateNumericalTextCtrl( this, 7 );
	m_text_BasePal		= CreateNumericalTextCtrl( this, 7 );

	m_spin_NominalPct	->SetRange( 10,  1000 );
	m_spin_SlomoPct		->SetRange(  1,  1000 );
	m_spin_TurboPct		->SetRange( 10,  1000 );

	// ------------------------------------------------------------
	// Sizers and Layouts

	*this += m_check_LimiterDisable;

	wxFlexGridSizer& s_spins( *new wxFlexGridSizer( 5 ) );
	s_spins.AddGrowableCol( 0 );

	s_spins += Label(_("Base Framerate Adjust:"))	| StdExpand();
	s_spins += 5;
	s_spins += m_spin_NominalPct					| pxBorder(wxTOP, 3);
	s_spins += Label(L"%")							| StdExpand();
	s_spins += 5;

	s_spins += Label(_("Slow Motion Adjust:"))		| StdExpand();
	s_spins += 5;
	s_spins += m_spin_SlomoPct						| pxBorder(wxTOP, 3);
	s_spins += Label(L"%")							| StdExpand();
	s_spins += 5;

	s_spins	+= Label(_("Turbo Adjust:"))			| StdExpand();
	s_spins	+= 5;
	s_spins	+= m_spin_TurboPct						| pxBorder(wxTOP, 3);
	s_spins	+= Label(L"%" )							| StdExpand();
	s_spins	+= 5;

	s_spins	+= 15;
	s_spins	+= 15;
	s_spins	+= 15;
	s_spins	+= 15;
	s_spins	+= 15;

	wxFlexGridSizer& s_fps( *new wxFlexGridSizer( 5 ) );
	s_fps.AddGrowableCol( 0 );

	s_fps	+= Label(_("NTSC Framerate:"))	| StdExpand();
	s_fps	+= 5;
	s_fps	+= m_text_BaseNtsc				| pxBorder(wxTOP, 2).Right();
	s_fps	+= Label(_("FPS"))				| StdExpand();
	s_fps	+= 5;

	s_fps	+= Label(_("PAL Framerate:"))	| StdExpand();
	s_fps	+= 5;
	s_fps	+= m_text_BasePal				| pxBorder(wxTOP, 2).Right();
	s_fps	+= Label(_("FPS"))				| StdExpand();
	s_fps	+= 5;

	*this	+= s_spins	| pxExpand;
	*this	+= s_fps	| pxExpand;

	*this	+= 5;

	//*this	+= Heading( pxE( "!Panel:Framelimiter:Heading",
	*this	+= new pxStaticText( this, pxE( "!Panel:Framelimiter:Heading",
		L"The internal framelimiter regulates the speed of the virtual machine. Adjustment values below are in "
		L"percentages of the default region-based framerate, which can also be configured below." )
	);

	AppStatusEvent_OnSettingsApplied();
}

void Panels::FramelimiterPanel::AppStatusEvent_OnSettingsApplied()
{
	const AppConfig::GSWindowOptions& appwin( g_Conf->GSWindow );
	const AppConfig::FramerateOptions& appfps( g_Conf->Framerate );
	const Pcsx2Config::GSOptions& gsconf( g_Conf->EmuOptions.GS );

	m_check_LimiterDisable->SetValue( !gsconf.FrameLimitEnable );

	m_spin_NominalPct	->SetValue( appfps.NominalScalar.Raw );
	m_spin_TurboPct		->SetValue( appfps.TurboScalar.Raw );
	m_spin_SlomoPct		->SetValue( appfps.SlomoScalar.Raw );

	m_text_BaseNtsc		->SetValue( gsconf.FramerateNTSC.ToString() );
	m_text_BasePal		->SetValue( gsconf.FrameratePAL.ToString() );
}

void Panels::FramelimiterPanel::Apply()
{
	AppConfig::FramerateOptions& appfps( g_Conf->Framerate );
	Pcsx2Config::GSOptions& gsconf( g_Conf->EmuOptions.GS );

	gsconf.FrameLimitEnable	= !m_check_LimiterDisable->GetValue();

	appfps.NominalScalar.Raw	= m_spin_NominalPct	->GetValue();
	appfps.TurboScalar.Raw		= m_spin_TurboPct	->GetValue();
	appfps.SlomoScalar.Raw		= m_spin_SlomoPct	->GetValue();

	try {
		gsconf.FramerateNTSC	= Fixed100::FromString( m_text_BaseNtsc->GetValue() );
		gsconf.FrameratePAL		= Fixed100::FromString( m_text_BasePal->GetValue() );
	}
	catch( Exception::ParseError& )
	{
		throw Exception::CannotApplySettings( this )
			.SetDiagMsg(pxsFmt(
				L"Error while parsing either NTSC or PAL framerate settings.\n\tNTSC Input = %s\n\tPAL Input  = %s",
				m_text_BaseNtsc->GetValue().c_str(), m_text_BasePal->GetValue().c_str()
			) )
			.SetUserMsg(_t("Error while parsing either NTSC or PAL framerate settings.  Settings must be valid floating point numerics."));
	}

	appfps.SanityCheck();
}

// --------------------------------------------------------------------------------------
//  FrameSkipPanel Implementations
// --------------------------------------------------------------------------------------

Panels::FrameSkipPanel::FrameSkipPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	SetMinWidth( 280 );
	/*m_check_EnableSkipOnTurbo = new pxCheckBox( this, _("Use Frameskip for Turbo") );

	m_check_EnableSkip = new pxCheckBox( this, _("Use Frameskip"),
		_(".") );

	m_check_EnableSkip->SetToolTip( pxEt( "!ContextTip:Frameskip:Disable",
		L""
		L""
	) );

	m_check_EnableSkipOnTurbo->SetToolTip( pxEt( "!ContextTip:Frameskip:UseForTurbo",
		L"Recommended option! Since frameskipping glitches typically aren't as annoying when you're "
		L" just trying to speed through stuff."
	) );*/

	const RadioPanelItem FrameskipOptions[] =
	{
		RadioPanelItem(
			_("Disabled [default]")
		),

		RadioPanelItem(
			_("Skip when on Turbo only")
		),

		RadioPanelItem(
			_("Constant skipping"),
			wxEmptyString,
			_("Normal and Turbo limit rates skip frames.  Slow motion mode will still disable frameskipping.")
		),
	};

	m_radio_SkipMode = new pxRadioPanel( this, FrameskipOptions );
	m_radio_SkipMode->Realize();

	pxFitToDigits( m_spin_FramesToDraw	= new wxSpinCtrl( this ), 6 );
	pxFitToDigits( m_spin_FramesToSkip	= new wxSpinCtrl( this ), 6 );

	// Set tooltips for spinners.


	// ------------------------------------------------------------
	// Sizers and Layouts

	//*this += m_check_EnableSkipOnTurbo;
	//*this += m_check_EnableSkip;

	*this += m_radio_SkipMode;

	wxFlexGridSizer& s_spins( *new wxFlexGridSizer( 4 ) );
	//s_spins.AddGrowableCol( 0 );

	s_spins += m_spin_FramesToDraw			| pxBorder(wxTOP, 2);
	s_spins += 10;
	s_spins += Label(_("Frames to Draw"))	| StdExpand();
	s_spins += 10;

	s_spins += m_spin_FramesToSkip			| pxBorder(wxTOP, 2);
	s_spins += 10;
	s_spins += Label(_("Frames to Skip"))	| StdExpand();
	s_spins += 10;

	*this	+= s_spins	| StdExpand();

	*this	+= Text( pxE( "!Panel:Frameskip:Heading",
		L"Notice: Due to PS2 hardware design, precise frame skipping is impossible. "
		L"Enabling it will cause severe graphical errors in some games." )
	) | StdExpand();

	AppStatusEvent_OnSettingsApplied();
}

void Panels::FrameSkipPanel::AppStatusEvent_OnSettingsApplied()
{
	const AppConfig::FramerateOptions& appfps( g_Conf->Framerate );
	const Pcsx2Config::GSOptions& gsconf( g_Conf->EmuOptions.GS );

	//m_check_EnableSkip		->SetValue( !appfps.SkipOnLimit );
	//m_check_EnableSkipOnTurbo	->SetValue( !appfps.SkipOnTurbo );

	m_radio_SkipMode	->SetSelection( appfps.SkipOnLimit ? 2 : (appfps.SkipOnTurbo ? 1 : 0) );

	m_spin_FramesToDraw	->SetValue( gsconf.FramesToDraw );
	m_spin_FramesToSkip	->SetValue( gsconf.FramesToSkip );
}

void Panels::FrameSkipPanel::Apply()
{
	AppConfig::FramerateOptions& appfps( g_Conf->Framerate );
	Pcsx2Config::GSOptions& gsconf( g_Conf->EmuOptions.GS );

	gsconf.FramesToDraw = m_spin_FramesToDraw->GetValue();
	gsconf.FramesToSkip = m_spin_FramesToSkip->GetValue();

	switch( m_radio_SkipMode->GetSelection() )
	{
		case 0:
			appfps.SkipOnLimit = false;
			appfps.SkipOnTurbo = false;
		break;

		case 1:
			appfps.SkipOnLimit = false;
			appfps.SkipOnTurbo = true;
		break;

		case 2:
			appfps.SkipOnLimit = true;
			appfps.SkipOnTurbo = true;
		break;
	}

	appfps.SanityCheck();
}

// --------------------------------------------------------------------------------------
//  VideoPanel Implementation
// --------------------------------------------------------------------------------------

Panels::VideoPanel::VideoPanel( wxWindow* parent ) :
	BaseApplicableConfigPanel( parent )
{
	wxPanelWithHelpers* left	= new wxPanelWithHelpers( this, wxVERTICAL );
	wxPanelWithHelpers* right	= new wxPanelWithHelpers( this, wxVERTICAL );

	m_check_SynchronousGS = new pxCheckBox( right, _("Use Synchronized MTGS"),
		_t("For troubleshooting potential bugs in the MTGS only, as it is potentially very slow.")
	);

	m_check_DisableOutput = new pxCheckBox( right, _("Disable all GS output"),
		_t("Completely disables all GS plugin activity; ideal for benchmarking EEcore components.")
	);

	m_check_SynchronousGS->SetToolTip( pxEt( "!ContextTip:GS:SyncMTGS",
		L"Enable this if you think MTGS thread sync is causing crashes or graphical errors.")
	) ;

	m_check_DisableOutput->SetToolTip( pxEt( "!ContextTip:GS:DisableOutput",
		L"Removes any benchmark noise caused by the MTGS thread or GPU overhead.  This option is best used in conjunction with savestates: "
		L"save a state at an ideal scene, enable this option, and re-load the savestate.\n\n"
		L"Warning: This option can be enabled on-the-fly but typically cannot be disabled on-the-fly (video will typically be garbage)."
	) );

	//GSWindowSettingsPanel* winpan = new GSWindowSettingsPanel( left );
	//winpan->AddFrame(_("Display/Window"));

	FrameSkipPanel* span = new FrameSkipPanel( right );
	span->AddFrame(_("Frame Skipping"));

	FramelimiterPanel* fpan = new FramelimiterPanel( left );
	fpan->AddFrame(_("Framelimiter"));

	wxFlexGridSizer* s_table = new wxFlexGridSizer( 2 );
	s_table->AddGrowableCol( 0, 1 );
	s_table->AddGrowableCol( 1, 1 );

	*right		+= span		| pxExpand;
	*right		+= 5;
	*right		+= m_check_SynchronousGS;
	*right		+= m_check_DisableOutput;

	*left		+= fpan		| pxExpand;
	*left		+= 5;

	*s_table	+= left		| StdExpand();
	*s_table	+= right	| StdExpand();

	*this		+= s_table	| pxExpand;

	AppStatusEvent_OnSettingsApplied();
}

void Panels::VideoPanel::OnOpenWindowSettings( wxCommandEvent& evt )
{
	AppOpenDialog<Dialogs::ComponentsConfigDialog>( this );

	// don't evt.skip, this prevents the Apply button from being activated. :)
}

void Panels::VideoPanel::Apply()
{
	g_Conf->EmuOptions.GS.SynchronousMTGS	= m_check_SynchronousGS->GetValue();
	g_Conf->EmuOptions.GS.DisableOutput		= m_check_DisableOutput->GetValue();
}

void Panels::VideoPanel::AppStatusEvent_OnSettingsApplied()
{
	m_check_SynchronousGS->SetValue( g_Conf->EmuOptions.GS.SynchronousMTGS );
	m_check_DisableOutput->SetValue( g_Conf->EmuOptions.GS.DisableOutput );
}
