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

#include "System.h"

using namespace pxSizerFlags;

static int pxGetTextHeight( const wxWindow* wind, int rows )
{
	wxClientDC dc(wx_const_cast(wxWindow *, wind));
	dc.SetFont( wind->GetFont() );
	return (dc.GetCharHeight() + 1 ) * rows;
}

const wxChar* Panels::SpeedHacksPanel::GetEEcycleSliderMsg( int val )
{
	switch( val )
	{
		case 1:
			return pxE(	".Panels:Speedhacks:EECycleX1",
				L"1 - Default cyclerate. This closely matches the actual speed of a real PS2 EmotionEngine."
			);

		case 2:
			return pxE( ".Panels:Speedhacks:EECycleX2",
				L"2 - Reduces the EE's cyclerate by about 33%.  Mild speedup for most games with high compatibility."
			);

		case 3:
			return pxE( ".Panels:Speedhacks:EECycleX3",
				L"3 - Reduces the EE's cyclerate by about 50%.  Moderate speedup, but *will* cause stuttering "
				L"audio on many FMVs."
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
			return pxE(	".Panels:Speedhacks:VUCycleStealOff",
				L"0 - Disables VU Cycle Stealing.  Most compatible setting!"
			);

		case 1:
			return pxE( ".Panels:Speedhacks:VUCycleSteal1",
				L"1 - Mild VU Cycle Stealing.  Lower compatibility, but some speedup for most games."
			);

		case 2:
			return pxE( ".Panels:Speedhacks:VUCycleSteal2",
				L"2 - Moderate VU Cycle Stealing.  Even lower compatibility, but significant speedups in some games."
			);

		case 3:
			// TODO: Mention specific games that benefit from this setting here.
			return pxE( ".Panels:Speedhacks:VUCycleSteal3",
				L"3 - Maximum VU Cycle Stealing.  Usefulness is limited, as this will cause flickering "
				L"visuals or slowdown in most games."
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
	: BaseApplicableConfigPanel( parent )
{
	const wxSizerFlags sliderFlags( wxSizerFlags().Border( wxLEFT | wxRIGHT, 8 ).Expand() );

	pxStaticText* heading = new pxStaticHeading( this, pxE( ".Panels:Speedhacks:Overview",
		L"These hacks will usually improve the speed of PCSX2 emulation, but compromise compatibility. "
		L"If you have issues, always try disabling these hacks first."
	) );

	m_check_Enable = new pxCheckBox( this, _("Enable speedhacks"),
		_("(Warning, can cause false FPS readings and many bugs!)"));
	m_check_Enable->SetToolTip(_("The safest way to make sure that all speedhacks are completely disabled."));
	
	m_button_Defaults = new wxButton( this, wxID_DEFAULT, _("Restore Defaults") );
	pxSetToolTip( m_button_Defaults, _("Resets all speedhack options to their defaults, which consequently turns them all OFF.") );

	wxPanelWithHelpers* left	= new wxPanelWithHelpers( this, wxVERTICAL );
	wxPanelWithHelpers* right	= new wxPanelWithHelpers( this, wxVERTICAL );
	left->SetIdealWidth( (left->GetIdealWidth() - 16) / 2 );
	right->SetIdealWidth( (right->GetIdealWidth() - 16) / 2 );

	// ------------------------------------------------------------------------
	// EE Cyclerate Hack Section:

	// Misc help text that I might find a home for later:
	// Cycle stealing works by 'fast-forwarding' the EE by an arbitrary number of cycles whenever VU1 micro-programs
	// are run, which works as a rough-guess skipping of what would normally be idle time spent running on the EE.

	wxPanelWithHelpers* eeSliderPanel = new wxPanelWithHelpers( left, wxVERTICAL, _("EE Cyclerate") );

	m_slider_eecycle = new wxSlider( eeSliderPanel, wxID_ANY, 1, 1, 3,
		wxDefaultPosition, wxDefaultSize, wxHORIZONTAL | wxSL_AUTOTICKS | wxSL_LABELS );

	m_msg_eecycle = new pxStaticHeading( eeSliderPanel ) ;//, GetEEcycleSliderMsg( 1 ) );
	m_msg_eecycle->SetForegroundColour( wxColour( L"Red" ) );
	m_msg_eecycle->SetMinSize( wxSize( wxDefaultCoord, pxGetTextHeight(m_msg_eecycle, 3) ) );

	const wxChar* ee_tooltip = pxE( ".Tooltips:Speedhacks:EECycleRate Slider",
		L"Setting higher values on this slider effectively reduces the clock speed of the EmotionEngine's "
		L"R5900 core cpu, and typically brings big speedups to games that fail to utilize "
		L"the full potential of the real PS2 hardware."
	);

	pxSetToolTip( m_slider_eecycle, ee_tooltip );
	pxSetToolTip( m_msg_eecycle, ee_tooltip );
	
	// ------------------------------------------------------------------------
	// VU Cycle Stealing Hack Section:

	wxPanelWithHelpers* vuSliderPanel = new wxPanelWithHelpers( right, wxVERTICAL, _("VU Cycle Stealing") );
	
	m_slider_vustealer = new wxSlider( vuSliderPanel, wxID_ANY, 0, 0, 3, wxDefaultPosition, wxDefaultSize,
		wxHORIZONTAL | wxSL_AUTOTICKS | wxSL_LABELS );

	m_msg_vustealer = new pxStaticHeading( vuSliderPanel ); //, GetVUcycleSliderMsg( 0 ) );
	m_msg_vustealer->SetForegroundColour( wxColour( L"Red" ) );
	m_msg_vustealer->SetMinSize( wxSize( wxDefaultCoord, pxGetTextHeight(m_msg_vustealer, 3) ) );

	const wxChar* vu_tooltip = pxE( ".Tooltips:Speedhacks:VUCycleStealing Slider",
		L"This slider controls the amount of cycles the VU unit steals from the EmotionEngine.  Higher values increase the number of "
		L"cycles stolen from the EE for each VU microprogram the game runs."
	);

	pxSetToolTip( m_slider_vustealer, vu_tooltip );
	pxSetToolTip( m_msg_vustealer, vu_tooltip );

	// ------------------------------------------------------------------------
	// microVU Hacks Section:
	
	wxPanelWithHelpers* vuHacksPanel = new wxPanelWithHelpers( right, wxVERTICAL, _("microVU Hacks") );

	m_check_vuFlagHack = new pxCheckBox( vuHacksPanel, _("mVU Flag Hack"),
		_("Large Speedup and High Compatibility; may cause garbage graphics, SPS, etc...") );

	m_check_vuMinMax = new pxCheckBox( vuHacksPanel, _("mVU Min/Max Hack"),
		_("Small Speedup; may cause black screens, garbage graphics, SPS, etc...") );

	m_check_vuFlagHack->SetToolTip( pxE( ".Tooltips:Speedhacks:vuFlagHack",
		L"Updates Status Flags only on blocks which will read them, instead of all the time. "
		L"This is safe most of the time, and Super VU does something similar by default."
	) );

	m_check_vuMinMax->SetToolTip( pxE( ".Tooltips:Speedhacks:vuMinMax",
		L"Uses SSE's Min/Max Floating Point Operations instead of custom logical Min/Max routines. "
		L"Known to break Gran Turismo 4, Tekken 5."
	) );

	// ------------------------------------------------------------------------
	// All other hacks Section:

	wxPanelWithHelpers* miscHacksPanel = new wxPanelWithHelpers( left, wxVERTICAL, _("Other Hacks") );

	m_check_intc = new pxCheckBox( miscHacksPanel, _("Enable INTC Spin Detection"),
		_("Huge speedup for some games, with almost no compatibility side effects. [Recommended]") );

	m_check_b1fc0 = new pxCheckBox( miscHacksPanel, _("Enable BIFC0 Spin Detection"),
		_("Moderate speedup for some games, with no known side effects. [Recommended]" ) );
	
	m_check_IOPx2 = new pxCheckBox( miscHacksPanel, _("IOP x2 cycle rate hack"),
		_("Small Speedup and works well with most games; may cause some games to hang during startup.") );
		

	m_check_intc->SetToolTip( pxE( ".Tooltips:Speedhacks:INTC",
		L"This hack works best for games that use the INTC Status register to wait for vsyncs, which includes primarily non-3D "
		L"RPG titles. Games that do not use this method of vsync will see little or no speedup from this hack."
	) );

	m_check_b1fc0->SetToolTip( pxE( ".Tooltips:Speedhacks:BIFC0",
		L"This hack works especially well for Final Fantasy X and Kingdom Hearts.  BIFC0 is the address of a specific block of "
		L"code in the EE kernel that's run repeatedly when the EE is waiting for the IOP to complete a task.  This hack detects "
		L"that and responds by fast-forwarding the EE until the IOP signals that the task is complete."
	) );

	m_check_IOPx2->SetToolTip( pxE( ".Tooltips:Speedhacks:IOPx2",
		L"Halves the cycle rate of the IOP, giving it an effective emulated speed of roughly 18 MHz. "
		L"The speedup is very minor, so this hack is generally not recommended."
	) );

	// ------------------------------------------------------------------------
	//  Layout and Size ---> (!!)

	wxFlexGridSizer& DefEnableSizer( *new wxFlexGridSizer( 2, 0, 12 ) );
	DefEnableSizer.AddGrowableCol( 1, 1 );
	DefEnableSizer	+= m_button_Defaults	| StdSpace().Align( wxALIGN_LEFT );
	DefEnableSizer	+= m_check_Enable		| StdSpace().Align( wxALIGN_RIGHT );

	*eeSliderPanel	+= m_slider_eecycle		| sliderFlags;
	*eeSliderPanel	+= m_msg_eecycle;

	*vuSliderPanel	+= m_slider_vustealer	| sliderFlags;
	*vuSliderPanel	+= m_msg_vustealer;

	*vuHacksPanel	+= m_check_vuFlagHack;
	*vuHacksPanel	+= m_check_vuMinMax;

	*miscHacksPanel	+= m_check_intc;
	*miscHacksPanel	+= m_check_b1fc0;
	*miscHacksPanel	+= m_check_IOPx2;

	*left	+= eeSliderPanel	| StdExpand();
	*left	+= miscHacksPanel	| StdExpand();

	*right	+= vuSliderPanel	| StdExpand();
	*right	+= vuHacksPanel		| StdExpand();

	s_table = new wxFlexGridSizer( 2 );
	s_table->AddGrowableCol( 0 );
	s_table->AddGrowableCol( 1 );
	*s_table+= left				| pxExpand;
	*s_table+= right			| pxExpand;

	*this	+= heading;
	*this	+= s_table			| pxExpand;
	*this	+= DefEnableSizer	| pxExpand;

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

	AppStatusEvent_OnSettingsApplied();
}

void Panels::SpeedHacksPanel::EnableStuff()
{
	wxSizerItemList& items( s_table->GetChildren() );

	wxSizerItemList::iterator it	= items.begin();
	wxSizerItemList::iterator end	= items.end();

	while( it != end )
	{
		(*it)->GetWindow()->Enable( m_check_Enable->GetValue() );
		++it;
	}
}

void Panels::SpeedHacksPanel::AppStatusEvent_OnSettingsApplied()
{
	AppStatusEvent_OnSettingsApplied( g_Conf->EmuOptions.Speedhacks );
}

void Panels::SpeedHacksPanel::AppStatusEvent_OnSettingsApplied( const Pcsx2Config::SpeedhackOptions& opts )
{
	const bool enabled = g_Conf->EnableSpeedHacks;
	
	m_check_Enable		->SetValue( !!enabled );

	m_slider_eecycle	->SetValue( opts.EECycleRate + 1 );
	m_slider_vustealer	->SetValue( opts.VUCycleSteal );

	SetEEcycleSliderMsg();
	SetVUcycleSliderMsg();

	m_check_vuFlagHack	->SetValue(opts.vuFlagHack);
	m_check_vuMinMax	->SetValue(opts.vuMinMax);
	m_check_intc		->SetValue(opts.IntcStat);
	m_check_b1fc0		->SetValue(opts.BIFC0);
	m_check_IOPx2		->SetValue(opts.IopCycleRate_X2);
	
	EnableStuff();

	// Layout necessary to ensure changed slider text gets re-aligned properly
	Layout();
}

void Panels::SpeedHacksPanel::Apply()
{
	g_Conf->EnableSpeedHacks = m_check_Enable->GetValue();

	Pcsx2Config::SpeedhackOptions& opts( g_Conf->EmuOptions.Speedhacks );
	
	opts.EECycleRate		= m_slider_eecycle->GetValue()-1;
	opts.VUCycleSteal		= m_slider_vustealer->GetValue();

	opts.BIFC0				= m_check_b1fc0->GetValue();
	opts.IopCycleRate_X2	= m_check_IOPx2->GetValue();
	opts.IntcStat			= m_check_intc->GetValue();
	opts.vuFlagHack			= m_check_vuFlagHack->GetValue();
	opts.vuMinMax			= m_check_vuMinMax->GetValue();
}

void Panels::SpeedHacksPanel::OnEnable_Toggled( wxCommandEvent& evt )
{
	EnableStuff();
	evt.Skip();
}

void Panels::SpeedHacksPanel::Defaults_Click( wxCommandEvent& evt )
{
	AppStatusEvent_OnSettingsApplied( Pcsx2Config::SpeedhackOptions() );
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
