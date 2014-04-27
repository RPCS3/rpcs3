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
#include "ConfigurationPanels.h"

using namespace pxSizerFlags;

const wxChar* Panels::SpeedHacksPanel::GetEEcycleSliderMsg( int val )
{
	switch( val )
	{
		case 1:
			return pxEt( L"1 - Default cyclerate. This closely matches the actual speed of a real PS2 EmotionEngine."
			);

		case 2:
			return pxEt( L"2 - Reduces the EE's cyclerate by about 33%.  Mild speedup for most games with high compatibility."
			);

		case 3:
			return pxEt( L"3 - Reduces the EE's cyclerate by about 50%.  Moderate speedup, but *will* cause stuttering audio on many FMVs."
			);

		default:
			break;
	}

	return L"Unreachable Warning Suppressor!!";
}

const wxChar* Panels::SpeedHacksPanel::GetVUcycleSliderMsg( int val )
{
	switch( val )
	{
		case 0:
			return pxEt( L"0 - Disables VU Cycle Stealing.  Most compatible setting!"
			);

		case 1:
			return pxEt( L"1 - Mild VU Cycle Stealing.  Lower compatibility, but some speedup for most games."
			);

		case 2:
			return pxEt( L"2 - Moderate VU Cycle Stealing.  Even lower compatibility, but significant speedups in some games."
			);

		case 3:
			// TODO: Mention specific games that benefit from this setting here.
			return pxEt( L"3 - Maximum VU Cycle Stealing.  Usefulness is limited, as this will cause flickering visuals or slowdown in most games."
			);
		default:
			break;
	}

	return L"Unreachable Warning Suppressor!!";
}

void Panels::SpeedHacksPanel::SetEEcycleSliderMsg()
{
	m_msg_eecycle->SetLabel( GetEEcycleSliderMsg(m_slider_eecycle->GetValue()) );
}

void Panels::SpeedHacksPanel::SetVUcycleSliderMsg()
{
	m_msg_vustealer->SetLabel( GetVUcycleSliderMsg(m_slider_vustealer->GetValue()) );
}

Panels::SpeedHacksPanel::SpeedHacksPanel( wxWindow* parent )
 : BaseApplicableConfigPanel_SpecificConfig( parent )
{
	const wxSizerFlags sliderFlags( wxSizerFlags().Border( wxLEFT | wxRIGHT, 8 ).Expand() );

	m_check_Enable = new pxCheckBox( this, _("Enable speedhacks"),
		pxE( L"Speedhacks usually improve emulation speed, but can cause glitches, broken audio, and false FPS readings.  When having emulation problems, disable this panel first."
		)
	);
	m_check_Enable->SetToolTip(_("A safe and easy way to make sure that all speedhacks are completely disabled.")).SetSubPadding( 1 );

	wxPanelWithHelpers* left	= new wxPanelWithHelpers( this, wxVERTICAL );
	wxPanelWithHelpers* right	= new wxPanelWithHelpers( this, wxVERTICAL );

	left->SetMinWidth( 300 );
	right->SetMinWidth( 300 );

	m_button_Defaults = new wxButton( right, wxID_DEFAULT, _("Restore Defaults") );
	pxSetToolTip( m_button_Defaults, _("Resets all speedhack options to their defaults, which consequently turns them all OFF.") );

	// ------------------------------------------------------------------------
	// EE Cyclerate Hack Section:

	// Misc help text that I might find a home for later:
	// Cycle stealing works by 'fast-forwarding' the EE by an arbitrary number of cycles whenever VU1 micro-programs
	// are run, which works as a rough-guess skipping of what would normally be idle time spent running on the EE.

	wxPanelWithHelpers* eeSliderPanel = new wxPanelWithHelpers( left, wxVERTICAL, _("EE Cyclerate [Not Recommended]") );

	m_slider_eecycle = new wxSlider( eeSliderPanel, wxID_ANY, 1, 1, 3,
		wxDefaultPosition, wxDefaultSize, wxHORIZONTAL | wxSL_AUTOTICKS | wxSL_LABELS );

	m_msg_eecycle = new pxStaticHeading( eeSliderPanel );
	m_msg_eecycle->SetForegroundColour( wxColour( L"Red" ) );
	m_msg_eecycle->SetHeight(3);

	const wxChar* ee_tooltip = pxEt( L"Setting higher values on this slider effectively reduces the clock speed of the EmotionEngine's R5900 core cpu, and typically brings big speedups to games that fail to utilize the full potential of the real PS2 hardware."
	);

	pxSetToolTip( m_slider_eecycle, ee_tooltip );
	pxSetToolTip( m_msg_eecycle, ee_tooltip );

	// ------------------------------------------------------------------------
	// VU Cycle Stealing Hack Section:

	wxPanelWithHelpers* vuSliderPanel = new wxPanelWithHelpers( right, wxVERTICAL, _("VU Cycle Stealing [Not Recommended]") );

	m_slider_vustealer = new wxSlider( vuSliderPanel, wxID_ANY, 0, 0, 3, wxDefaultPosition, wxDefaultSize,
		wxHORIZONTAL | wxSL_AUTOTICKS | wxSL_LABELS );

	m_msg_vustealer = new pxStaticHeading( vuSliderPanel );
	m_msg_vustealer->SetForegroundColour( wxColour( L"Red" ) );
	m_msg_vustealer->SetHeight(3);

	const wxChar* vu_tooltip = pxEt( L"This slider controls the amount of cycles the VU unit steals from the EmotionEngine.  Higher values increase the number of cycles stolen from the EE for each VU microprogram the game runs."
	);

	pxSetToolTip( m_slider_vustealer, vu_tooltip );
	pxSetToolTip( m_msg_vustealer, vu_tooltip );

	// ------------------------------------------------------------------------
	// microVU Hacks Section:

	wxPanelWithHelpers* vuHacksPanel = new wxPanelWithHelpers( right, wxVERTICAL, _("microVU Hacks") );

	m_check_vuFlagHack = new pxCheckBox( vuHacksPanel, _("mVU Flag Hack"),
		_("Good Speedup and High Compatibility; may cause bad graphics... [Recommended]" ) );

	m_check_vuThread = new pxCheckBox( vuHacksPanel, _("MTVU (Multi-Threaded microVU1)"),
		_("Good Speedup and High Compatibility; may cause hanging... [Recommended if 3+ cores]") );

	m_check_vuFlagHack->SetToolTip( pxEt( L"Updates Status Flags only on blocks which will read them, instead of all the time. This is safe most of the time, and Super VU does something similar by default."
	) );

	m_check_vuThread->SetToolTip( pxEt( L"Runs VU1 on its own thread (microVU1-only). Generally a speedup on CPUs with 3 or more cores. This is safe for most games, but a few games are incompatible and may hang. In the case of GS limited games, it may be a slowdown (especially on dual core CPUs)."
	) );

	// ------------------------------------------------------------------------
	// All other hacks Section:

	wxPanelWithHelpers* miscHacksPanel = new wxPanelWithHelpers( left, wxVERTICAL, _("Other Hacks") );

	m_check_intc = new pxCheckBox( miscHacksPanel, _("Enable INTC Spin Detection"),
		_("Huge speedup for some games, with almost no compatibility side effects. [Recommended]") );

	m_check_waitloop = new pxCheckBox( miscHacksPanel, _("Enable Wait Loop Detection"),
		_("Moderate speedup for some games, with no known side effects. [Recommended]" ) );

	m_check_fastCDVD = new pxCheckBox( miscHacksPanel, _("Enable fast CDVD"),
		_("Fast disc access, less loading times. [Not Recommended]") );


	m_check_intc->SetToolTip( pxEt( L"This hack works best for games that use the INTC Status register to wait for vsyncs, which includes primarily non-3D RPG titles. Games that do not use this method of vsync will see little or no speedup from this hack."
	) );

	m_check_waitloop->SetToolTip( pxEt( L"Primarily targetting the EE idle loop at address 0x81FC0 in the kernel, this hack attempts to detect loops whose bodies are guaranteed to result in the same machine state for every iteration until a scheduled event triggers emulation of another unit.  After a single iteration of such loops, we advance to the time of the next event or the end of the processor's timeslice, whichever comes first."
	) );

	m_check_fastCDVD->SetToolTip( pxEt( L"Check HDLoader compatibility lists for known games that have issues with this. (Often marked as needing 'mode 1' or 'slow DVD'"
	) );

	// ------------------------------------------------------------------------
	//  Layout and Size ---> (!!)

	//wxFlexGridSizer& DefEnableSizer( *new wxFlexGridSizer( 3, 0, 12 ) );
	//DefEnableSizer.AddGrowableCol( 1, 1 );
	//DefEnableSizer.AddGrowableCol( 2, 10 );
	//DefEnableSizer.AddGrowableCol( 1, 1 );
	//DefEnableSizer	+= m_button_Defaults	| StdSpace().Align( wxALIGN_LEFT );
	//DefEnableSizer	+= pxStretchSpacer(1);
	//DefEnableSizer	+= m_check_Enable		| StdExpand().Align( wxALIGN_RIGHT );

	*eeSliderPanel	+= m_slider_eecycle		| sliderFlags;
	*eeSliderPanel	+= m_msg_eecycle		| sliderFlags;

	*vuSliderPanel	+= m_slider_vustealer	| sliderFlags;
	*vuSliderPanel	+= m_msg_vustealer		| sliderFlags;

	*vuHacksPanel	+= m_check_vuFlagHack;
	*vuHacksPanel	+= m_check_vuThread;
	//*vuHacksPanel	+= 57; // Aligns left and right boxes in default language and font size

	*miscHacksPanel	+= m_check_intc;
	*miscHacksPanel	+= m_check_waitloop;
	*miscHacksPanel	+= m_check_fastCDVD;

	*left	+= eeSliderPanel	| StdExpand();
	*left	+= miscHacksPanel	| StdExpand();

	*right	+= vuSliderPanel	| StdExpand();
	*right	+= vuHacksPanel		| StdExpand();
	*right	+= StdPadding;
	*right	+= m_button_Defaults| StdButton();

	s_table = new wxFlexGridSizer( 2 );
	s_table->AddGrowableCol( 0, 1 );
	s_table->AddGrowableCol( 1, 1 );
	*s_table+= left				| pxExpand;
	*s_table+= right			| pxExpand;

	*this	+= m_check_Enable;
	*this	+= new wxStaticLine( this )	| pxExpand.Border(wxLEFT | wxRIGHT, 20);
	*this	+= StdPadding;
	*this	+= s_table					| pxExpand;

	// ------------------------------------------------------------------------

	Connect( wxEVT_SCROLL_PAGEUP,	wxScrollEventHandler( SpeedHacksPanel::Slider_Click ) );
	Connect( wxEVT_SCROLL_PAGEDOWN,	wxScrollEventHandler( SpeedHacksPanel::Slider_Click ) );
	Connect( wxEVT_SCROLL_LINEUP,	wxScrollEventHandler( SpeedHacksPanel::Slider_Click ) );
	Connect( wxEVT_SCROLL_LINEDOWN,	wxScrollEventHandler( SpeedHacksPanel::Slider_Click ) );
	Connect( wxEVT_SCROLL_TOP,		wxScrollEventHandler( SpeedHacksPanel::Slider_Click ) );
	Connect( wxEVT_SCROLL_BOTTOM,	wxScrollEventHandler( SpeedHacksPanel::Slider_Click ) );

	Connect( m_slider_eecycle->GetId(),		wxEVT_SCROLL_CHANGED, wxScrollEventHandler( SpeedHacksPanel::EECycleRate_Scroll ) );
	Connect( m_slider_vustealer->GetId(),	wxEVT_SCROLL_CHANGED, wxScrollEventHandler( SpeedHacksPanel::VUCycleRate_Scroll ) );
	Connect( m_check_Enable->GetId(),		wxEVT_COMMAND_CHECKBOX_CLICKED,	wxCommandEventHandler( SpeedHacksPanel::OnEnable_Toggled ) );
	Connect( wxID_DEFAULT,					wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( SpeedHacksPanel::Defaults_Click ) );
}

void Panels::SpeedHacksPanel::EnableStuff( AppConfig* configToUse )
{
	if( !configToUse ) configToUse = g_Conf;
	wxSizerItemList& items( s_table->GetChildren() );

	wxSizerItemList::iterator it	= items.begin();
	wxSizerItemList::iterator end	= items.end();

	while( it != end )
	{
		(*it)->GetWindow()->Enable( m_check_Enable->GetValue());
		++it;
	}
}

void Panels::SpeedHacksPanel::AppStatusEvent_OnSettingsApplied()
{
	//Console.WriteLn("SpeedHacksPanel::AppStatusEvent_OnSettingsApplied()");
	ApplyConfigToGui( *g_Conf );
}

void Panels::SpeedHacksPanel::ApplyConfigToGui( AppConfig& configToApply, int flags )
{
	const bool enabled = configToApply.EnableSpeedHacks;
	Pcsx2Config::SpeedhackOptions& opts=configToApply.EmuOptions.Speedhacks;

	m_check_Enable		->SetValue( !!enabled ).Enable(!configToApply.EnablePresets);

	m_slider_eecycle	->SetValue( opts.EECycleRate + 1 );
	m_slider_vustealer	->SetValue( opts.VUCycleSteal );
	m_slider_eecycle->Enable(!configToApply.EnablePresets);
	m_slider_vustealer->Enable(!configToApply.EnablePresets);

	SetEEcycleSliderMsg();
	SetVUcycleSliderMsg();

	m_check_vuFlagHack	->SetValue(opts.vuFlagHack).Enable(!configToApply.EnablePresets);
	if( !(flags & AppConfig::APPLY_FLAG_FROM_PRESET) )
		m_check_vuThread	->SetValue(opts.vuThread);
	m_check_intc		->SetValue(opts.IntcStat).Enable(!configToApply.EnablePresets);
	m_check_waitloop	->SetValue(opts.WaitLoop).Enable(!configToApply.EnablePresets);
	m_check_fastCDVD	->SetValue(opts.fastCDVD).Enable(!configToApply.EnablePresets);

	EnableStuff( &configToApply );

	// Layout necessary to ensure changed slider text gets re-aligned properly
	Layout();

	//Console.WriteLn("SpeedHacksPanel::ApplyConfigToGui: EnabledPresets: %s", configToApply.EnablePresets?"true":"false");
}


void Panels::SpeedHacksPanel::Apply()
{
	g_Conf->EnableSpeedHacks = m_check_Enable->GetValue();

	Pcsx2Config::SpeedhackOptions& opts( g_Conf->EmuOptions.Speedhacks );

	opts.EECycleRate		= m_slider_eecycle->GetValue()-1;
	opts.VUCycleSteal		= m_slider_vustealer->GetValue();

	opts.WaitLoop			= m_check_waitloop->GetValue();
	opts.fastCDVD			= m_check_fastCDVD->GetValue();
	opts.IntcStat			= m_check_intc->GetValue();
	opts.vuFlagHack			= m_check_vuFlagHack->GetValue();
	opts.vuThread			= m_check_vuThread->GetValue();

	// If the user has a command line override specified, we need to disable it
	// so that their changes take effect
	wxGetApp().Overrides.DisableSpeedhacks = false;
}

void Panels::SpeedHacksPanel::OnEnable_Toggled( wxCommandEvent& evt )
{
	AppConfig tmp=*g_Conf;
	tmp.EnablePresets=false; //if clicked, button was enabled, so not using a preset --> let EnableStuff work

	EnableStuff( &tmp );
	evt.Skip();
}

void Panels::SpeedHacksPanel::Defaults_Click( wxCommandEvent& evt )
{
	//Can only get here presets are disabled at the GUI (= the 'Defaults' button is enabled).
	AppConfig currentConfigWithHacksReset = *g_Conf;
	currentConfigWithHacksReset.EmuOptions.Speedhacks = Pcsx2Config::SpeedhackOptions();
	currentConfigWithHacksReset.EnablePresets=false;//speed hacks gui depends on preset, apply it as if presets are disabled
	ApplyConfigToGui( currentConfigWithHacksReset );
	evt.Skip();
}

void Panels::SpeedHacksPanel::Slider_Click(wxScrollEvent &event) {
	wxSlider* slider = (wxSlider*) event.GetEventObject();
	int value = slider->GetValue();
	int eventType = event.GetEventType();
	if (eventType == wxEVT_SCROLL_PAGEUP || eventType == wxEVT_SCROLL_LINEUP) {
		if (value > slider->GetMin()) {
			slider->SetValue(value-1);
		}
	}
	else if (eventType == wxEVT_SCROLL_TOP) {
		slider->SetValue(slider->GetMin());
	}
	else if (eventType == wxEVT_SCROLL_PAGEDOWN || eventType == wxEVT_SCROLL_LINEDOWN) {
		if (value < slider->GetMax()) {
			slider->SetValue(value+1);
		}
	}
	else if (eventType == wxEVT_SCROLL_BOTTOM) {
		slider->SetValue(slider->GetMax());
	}

	event.Skip();
}

void Panels::SpeedHacksPanel::EECycleRate_Scroll(wxScrollEvent &event)
{
	SetEEcycleSliderMsg();
	event.Skip();
}

void Panels::SpeedHacksPanel::VUCycleRate_Scroll(wxScrollEvent &event)
{
	SetVUcycleSliderMsg();
	event.Skip();
}
