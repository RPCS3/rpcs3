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

using namespace wxHelpers;

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
				L"3 - Reduces the EE's cyclerate by about 50%.  Moderate speedup, but *will* cause studdering "
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
				L"1 - Mild VU Cycle Stealing.  High compatibility with some speedup for most games."
			);

		case 2:
			return pxE( ".Panels:Speedhacks:VUCycleSteal2",
				L"2 - Moderate VU Cycle Stealing.  Moderate compatibility with significant speedups in some games."
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

Panels::SpeedHacksPanel::SpeedHacksPanel( wxWindow& parent, int idealWidth ) :
	BaseApplicableConfigPanel( &parent, idealWidth )
{
	const Pcsx2Config::SpeedhackOptions& opts( g_Conf->EmuOptions.Speedhacks );

	// ------------------------------------------------------------------------
	// microVU Hacks Section:
	// ------------------------------------------------------------------------

	m_check_vuFlagHack = new pxCheckBox( this, _("mVU Flag Hack"),
		_("Large Speedup and High Compatibility; may cause garbage graphics, SPS, etc...") );

	m_check_vuFlagHack->SetToolTip( pxE( ".Tooltips:Speedhacks:vuFlagHack",
		L"Updates Status Flags only on blocks which will read them, instead of all the time. "
		L"This is safe most of the time, and Super VU does something similar by default."
	) );

	m_check_vuMinMax = new pxCheckBox( this, _("mVU Min/Max Hack"),
		_("Small Speedup; may cause black screens, garbage graphics, SPS, etc...") );

	m_check_vuMinMax->SetToolTip( pxE( ".Tooltips:Speedhacks:vuMinMax",
		L"Uses SSE's Min/Max Floating Point Operations instead of custom logical Min/Max routines. "
		L"Known to break Gran Tourismo 4, Tekken 5."
	) );


	m_check_vuFlagHack->SetValue(opts.vuFlagHack);
	m_check_vuMinMax->SetValue(opts.vuMinMax);

	// ------------------------------------------------------------------------
	// All other hacks Section:
	// ------------------------------------------------------------------------

	m_check_intc = new pxCheckBox( this, _("Enable INTC Spin Detection"),
		_("Huge speedup for some games, with almost no compatibility side effects. [Recommended]") );

	m_check_intc->SetToolTip( pxE( ".Tooltips:Speedhacks:INTC",
		L"This hack works best for games that use the INTC Status register to wait for vsyncs, which includes primarily non-3D "
		L"RPG titles. Games that do not use this method of vsync will see little or no speeup from this hack."
	) );

	m_check_b1fc0 = new pxCheckBox( this, _("Enable BIFC0 Spin Detection"),
		_("Moderate speedup for some games, with no known side effects. [Recommended]" ) );
	
	m_check_b1fc0->SetToolTip( pxE( ".Tooltips:Speedhacks:BIFC0",
		L"This hack works especially well for Final Fantasy X and Kingdom Hearts.  BIFC0 is the address of a specific block of "
		L"code in the EE kernel that's run repeatedly when the EE is waiting for the IOP to complete a task.  This hack detects "
		L"that and responds by fast-forwarding the EE until the IOP signals that the task is complete."
	) );

	m_check_IOPx2 = new pxCheckBox( this, _("IOP x2 cycle rate hack"),
		_("Small Speedup and works well with most games; may cause some games to hang during startup.") );
		
	m_check_IOPx2->SetToolTip( pxE( ".Tooltips:Speedhacks:IOPx2",
		L"Halves the cycle rate of the IOP, giving it an effective emulated speed of roughly 18 MHz. "
		L"The speedup is very minor, so this hack is generally not recommended."
	) );

	m_check_intc->SetValue(opts.IntcStat);
	m_check_b1fc0->SetValue(opts.BIFC0);
	m_check_IOPx2->SetValue(opts.IopCycleRate_X2);

	// ----------------------------------------------------------------------------
	//  Layout Section  (Sizers Bindings)
	// ----------------------------------------------------------------------------

	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );
	wxFlexGridSizer& cycleHacksSizer = *new wxFlexGridSizer( 2 );

	cycleHacksSizer.AddGrowableCol( 0, 1 );
	cycleHacksSizer.AddGrowableCol( 1, 1 );

	wxStaticBoxSizer& cyclerateSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("EE Cyclerate") );
	wxStaticBoxSizer& stealerSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("VU Cycle Stealing") );
	wxStaticBoxSizer& microVUSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("microVU Hacks") );
	wxStaticBoxSizer& miscSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("Other Hacks") );

	AddStaticText( mainSizer, pxE( ".Panels:Speedhacks:Overview",
		L"These hacks will usually improve the speed of PCSX2 emulation, but compromise compatibility. "
		L"If you have issues, always try disabling these hacks first."
	) );

	const wxChar* tooltip;		// needed because we duplicate tooltips across multiple controls.
	const wxSizerFlags sliderFlags( wxSizerFlags().Border( wxLEFT | wxRIGHT, 8 ).Expand() );

	// ------------------------------------------------------------------------
	// EE Cyclerate Hack Section:
	// ------------------------------------------------------------------------

	m_slider_eecycle = new wxSlider( this, wxID_ANY, opts.EECycleRate+1, 1, 3,
		wxDefaultPosition, wxDefaultSize, wxHORIZONTAL | wxSL_AUTOTICKS | wxSL_LABELS );

	tooltip = pxE( ".Tooltips:Speedhacks:EECycleRate Slider",
		L"Setting higher values on this slider effectively reduces the clock speed of the EmotionEngine's "
		L"R5900 core cpu, and typically brings big speedups to games that fail to utilize "
		L"the full potential of the real PS2 hardware."
	);

	cyclerateSizer.Add( m_slider_eecycle, sliderFlags );
	m_msg_eecycle = &AddStaticText( cyclerateSizer, GetEEcycleSliderMsg( opts.EECycleRate+1 ), wxALIGN_CENTRE, (GetIdealWidth()-24)/2 );
	m_msg_eecycle->SetForegroundColour( wxColour( L"Red" ) );
	m_msg_eecycle->SetSizeHints( wxSize( wxDefaultCoord, pxGetTextHeight(m_msg_eecycle, 4) ) );

	pxSetToolTip( m_slider_eecycle, tooltip );
	pxSetToolTip( m_msg_eecycle, tooltip );

	// ------------------------------------------------------------------------
	// VU Cycle Stealing Hack Section:
	// ------------------------------------------------------------------------

	m_slider_vustealer = new wxSlider( this, wxID_ANY, opts.VUCycleSteal, 0, 3, wxDefaultPosition, wxDefaultSize,
		wxHORIZONTAL | wxSL_AUTOTICKS | wxSL_LABELS );

	tooltip = pxE( ".Tooltips:Speedhacks:VUCycleStealing Slider",
		L"This slider controls the amount of cycles the VU unit steals from the EmotionEngine.  Higher values increase the number of "
		L"cycles stolen from the EE for each VU microprogram the game runs."
	);
	// Misc help text that I might find a home for later:
	// Cycle stealing works by 'fast-forwarding' the EE by an arbitrary number of cycles whenever VU1 micro-programs
	// are run, which works as a rough-guess skipping of what would normally be idle time spent running on the EE.

	stealerSizer.Add( m_slider_vustealer, wxSizerFlags().Border( wxLEFT | wxRIGHT, 8 ).Expand() );
	m_msg_vustealer = &AddStaticText(stealerSizer, GetVUcycleSliderMsg( opts.VUCycleSteal ), wxALIGN_CENTRE, (GetIdealWidth()-24)/2 );
	m_msg_vustealer->SetForegroundColour( wxColour( L"Red" ) );
	m_msg_vustealer->SetSizeHints( wxSize( wxDefaultCoord, pxGetTextHeight(m_msg_vustealer, 4) ) );

	pxSetToolTip( m_slider_vustealer, tooltip );
	pxSetToolTip( m_msg_vustealer, tooltip );


	microVUSizer.Add( m_check_vuFlagHack );
	microVUSizer.Add( m_check_vuMinMax );

	miscSizer.Add( m_check_intc );
	miscSizer.Add( m_check_b1fc0 );
	miscSizer.Add( m_check_IOPx2 );

	cycleHacksSizer.Add( &cyclerateSizer, SizerFlags::TopLevelBox() );
	cycleHacksSizer.Add( &stealerSizer, SizerFlags::TopLevelBox() );

	mainSizer.Add( &cycleHacksSizer, wxSizerFlags().Expand() );
	mainSizer.Add( &microVUSizer, SizerFlags::TopLevelBox() );
	mainSizer.Add( &miscSizer, SizerFlags::TopLevelBox() );
	SetSizer( &mainSizer );

	// There has to be a cleaner way to do this...
	int ids[2] = {m_slider_eecycle->GetId(), m_slider_vustealer->GetId()};
	for (int i=0; i<2; i++) {
		Connect( ids[i], wxEVT_SCROLL_PAGEUP,	wxScrollEventHandler( SpeedHacksPanel::Slider_Click ) );
		Connect( ids[i], wxEVT_SCROLL_PAGEDOWN,	wxScrollEventHandler( SpeedHacksPanel::Slider_Click ) );
		Connect( ids[i], wxEVT_SCROLL_LINEUP,	wxScrollEventHandler( SpeedHacksPanel::Slider_Click ) );
		Connect( ids[i], wxEVT_SCROLL_LINEDOWN,	wxScrollEventHandler( SpeedHacksPanel::Slider_Click ) );
		Connect( ids[i], wxEVT_SCROLL_TOP,		wxScrollEventHandler( SpeedHacksPanel::Slider_Click ) );
		Connect( ids[i], wxEVT_SCROLL_BOTTOM,	wxScrollEventHandler( SpeedHacksPanel::Slider_Click ) );
	}

	Connect( m_slider_eecycle->GetId(),		wxEVT_SCROLL_CHANGED, wxScrollEventHandler( SpeedHacksPanel::EECycleRate_Scroll ) );
	Connect( m_slider_vustealer->GetId(),	wxEVT_SCROLL_CHANGED, wxScrollEventHandler( SpeedHacksPanel::VUCycleRate_Scroll ) );
}

void Panels::SpeedHacksPanel::Apply()
{
	Pcsx2Config::SpeedhackOptions& opts( g_Conf->EmuOptions.Speedhacks );
	opts.EECycleRate		= m_slider_eecycle->GetValue()-1;
	opts.VUCycleSteal		= m_slider_vustealer->GetValue();

	opts.BIFC0				= m_check_b1fc0->GetValue();
	opts.IopCycleRate_X2	= m_check_IOPx2->GetValue();
	opts.IntcStat			= m_check_intc->GetValue();
	opts.vuFlagHack			= m_check_vuFlagHack->GetValue();
	opts.vuMinMax			= m_check_vuMinMax->GetValue();
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
}

void Panels::SpeedHacksPanel::EECycleRate_Scroll(wxScrollEvent &event)
{
    m_msg_eecycle->SetLabel(GetEEcycleSliderMsg(m_slider_eecycle->GetValue()));
	event.Skip();
}

void Panels::SpeedHacksPanel::VUCycleRate_Scroll(wxScrollEvent &event)
{
    m_msg_vustealer->SetLabel(GetVUcycleSliderMsg(m_slider_vustealer->GetValue()));
	event.Skip();
}
