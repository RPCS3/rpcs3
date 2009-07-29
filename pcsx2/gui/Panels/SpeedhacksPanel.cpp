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

#include "Misc.h"
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
			
		jNO_DEFAULT
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
			
		jNO_DEFAULT
	}
	
	return L"Unreachable Warning Suppressor!!";
}

Panels::SpeedHacksPanel::SpeedHacksPanel( wxWindow& parent, int idealWidth ) :
	BaseApplicableConfigPanel( &parent, idealWidth )
{
	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );
	wxFlexGridSizer& cycleHacksSizer = *new wxFlexGridSizer( 2 );
	
	cycleHacksSizer.AddGrowableCol( 0, 1 );
	cycleHacksSizer.AddGrowableCol( 1, 1 );

	wxStaticBoxSizer& cyclerateSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("EE Cyclerate") );
	wxStaticBoxSizer& stealerSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("VU Cycle Stealing") );
	wxStaticBoxSizer& miscSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("Other Hacks") );

	AddStaticText( mainSizer, pxE( ".Panels:Speedhacks:Overview",
		L"These hacks will usually improve the speed of PCSX2 emulation, but compromise compatibility. "
		L"If you have issues, always try disabling these hacks first."
	) );

	const wxChar* tooltip;		// needed because we duplicate tooltips across multiple controls.
	const wxSizerFlags sliderFlags( wxSizerFlags().Border( wxLEFT | wxRIGHT, 8 ).Expand() );

	// ------------------------------------------------------------------------
	// EE Cyclerate Hack Section:
	
	m_slider_eecycle = new wxSlider( this, wxID_ANY, g_Conf->Speedhacks.EECycleRate+1, 1, 3,
		wxDefaultPosition, wxDefaultSize, wxHORIZONTAL | wxSL_AUTOTICKS | wxSL_LABELS );
	
	tooltip = pxE( ".Tooltips:Speedhacks:EECycleRate Slider",
		L"Setting higher values on this slider effectively reduces the clock speed of the EmotionEngine's "
		L"R5900 core cpu, and typically brings big speedups to games that fail to utilize "
		L"the full potential of the real PS2 hardware."
	);
	
	cyclerateSizer.Add( m_slider_eecycle, sliderFlags );
	m_msg_eecycle = &AddStaticText( cyclerateSizer, GetEEcycleSliderMsg( g_Conf->Speedhacks.EECycleRate+1 ), wxALIGN_CENTRE, (GetIdealWidth()-24)/2 );
	m_msg_eecycle->SetForegroundColour( wxColour( L"Red" ) );
	m_msg_eecycle->SetSizeHints( wxSize( wxDefaultCoord, pxGetTextHeight(m_msg_eecycle, 4) ) );

	m_slider_eecycle->SetToolTip( tooltip );
	m_msg_eecycle->SetToolTip( tooltip );

	// ------------------------------------------------------------------------
	// VU Cycle Stealing Hack Section:

	m_slider_vustealer = new wxSlider( this, wxID_ANY, g_Conf->Speedhacks.VUCycleSteal, 0, 4, wxDefaultPosition, wxDefaultSize,
		wxHORIZONTAL | wxSL_AUTOTICKS | wxSL_LABELS );

	tooltip = pxE( ".Tooltips:Speedhacks:VUCycleStealing Slider",
		L"This slider controls the amount of cycles the VU unit steals from the EmotionEngine.  Higher values increase the number of "
		L"cycles stolen from the EE for each VU microprogram the game runs."
	);
	// Misc help text that I might find a home for later:
	// Cycle stealing works by 'fast-forwarding' the EE by an arbitrary number of cycles whenever VU1 micro-programs
	// are run, which works as a rough-guess skipping of what would normally be idle time spent running on the EE.
		
	stealerSizer.Add( m_slider_vustealer, wxSizerFlags().Border( wxLEFT | wxRIGHT, 8 ).Expand() );
	m_msg_vustealer = &AddStaticText(stealerSizer, GetVUcycleSliderMsg( g_Conf->Speedhacks.VUCycleSteal ), wxALIGN_CENTRE, (GetIdealWidth()-24)/2 );
	m_msg_vustealer->SetForegroundColour( wxColour( L"Red" ) );
	m_msg_vustealer->SetSizeHints( wxSize( wxDefaultCoord, pxGetTextHeight(m_msg_vustealer, 4) ) );

	m_slider_vustealer->SetToolTip( tooltip );
	m_msg_vustealer->SetToolTip( tooltip );

	// ------------------------------------------------------------------------
	// All other hacks Section:

	m_check_intc = &AddCheckBox(miscSizer, _("Enable INTC Spin Detection"),
		_("Huge speedup for some games, with almost no compatibility side effects. [Recommended]"),
		pxE( ".Tooltips:Speedhacks:INTC",
			L"This hack works best for games that use the INTC Status register to wait for vsyncs, which includes primarily non-3D "
			L"RPG titles. Games that do not use this method of vsync will see little or no speeup from this hack."
	) );

	m_check_b1fc0 = &AddCheckBox(miscSizer, _("Enable BIFC0 Spin Detection"),
		_("Moderate speedup for some games, with no known side effects. [Recommended]" ),
		pxE( ".Tooltips:Speedhacks:BIFC0",
			L"This hack works especially well for Final Fantasy X and Kingdom Hearts.  BIFC0 is the address of a specific block of "
			L"code in the EE kernel that's run repeatedly when the EE is waiting for the IOP to complete a task.  This hack detects "
			L"that and responds by fast-forwarding the EE until the IOP signals that the task is complete."
	) );

	m_check_IOPx2 = &AddCheckBox(miscSizer, _("IOP x2 cycle rate hack"),
		_("Small Speedup and works well with most games; may cause some games to hang during startup."),
		pxE( ".Tooltips:Speedhacks:IOPx2",
			L"Halves the cycle rate of the IOP, giving it an effective emulated speed of roughly 18 MHz. "
			L"The speedup is very minor, so this hack is generally not recommended."
		) );

	cycleHacksSizer.Add( &cyclerateSizer, SizerFlags::TopLevelBox() );
	cycleHacksSizer.Add( &stealerSizer, SizerFlags::TopLevelBox() );

	mainSizer.Add( &cycleHacksSizer, wxSizerFlags().Expand() );
	mainSizer.Add( &miscSizer, SizerFlags::TopLevelBox() );
	SetSizerAndFit( &mainSizer );

	Connect( m_slider_eecycle->GetId(),		wxEVT_SCROLL_CHANGED, wxScrollEventHandler( SpeedHacksPanel::EECycleRate_Scroll ) );
	Connect( m_slider_vustealer->GetId(),	wxEVT_SCROLL_CHANGED, wxScrollEventHandler( SpeedHacksPanel::VUCycleRate_Scroll ) );
}

void Panels::SpeedHacksPanel::Apply( AppConfig& conf )
{
	conf.Speedhacks.EECycleRate		= m_slider_eecycle->GetValue()-1;
	conf.Speedhacks.VUCycleSteal	= m_slider_vustealer->GetValue();

	conf.Speedhacks.BIFC0			= m_check_b1fc0->GetValue();
	conf.Speedhacks.IopCycleRate_X2	= m_check_IOPx2->GetValue();
	conf.Speedhacks.IntcStat		= m_check_intc->GetValue();
}

void Panels::SpeedHacksPanel::EECycleRate_Scroll(wxScrollEvent &event)
{
	event.Skip();
}

void Panels::SpeedHacksPanel::VUCycleRate_Scroll(wxScrollEvent &event)
{
	event.Skip();
}
