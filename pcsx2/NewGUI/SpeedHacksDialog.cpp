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
#include "Misc.h"
#include "SpeedHacksDialog.h"
#include "System.h"

using namespace wxHelpers;

namespace Dialogs {

SpeedHacksDialog::SpeedHacksDialog( wxWindow* parent, int id ):
	wxDialogWithHelpers( parent, id, _T("Speed Hacks"), true )
{
	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );
	wxBoxSizer& secondarySizer = *new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer& sliderSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("Cycle Hacks") );
	wxStaticBoxSizer& miscSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("Misc Speed Hacks") );

	AddStaticText(mainSizer, _T("These hacks will affect the speed of PCSX2, but compromise compatability."));
	AddStaticText(mainSizer, _T("If you have issues, disable all of these and try again."));

    wxSlider* vuScale = new wxSlider(this, wxID_ANY, Config.Hacks.VUCycleSteal,  0, 4 );
    wxSlider* eeScale = new wxSlider(this, wxID_ANY, Config.Hacks.EECycleRate,  0, 2);

    AddStaticText(sliderSizer, _T("EE Cycle"));
    sliderSizer.Add( eeScale, wxEXPAND );
    AddStaticText(sliderSizer, _T("Placeholder text for EE scale position."));

    AddStaticText(sliderSizer, _T("VU Cycle"));
    sliderSizer.Add( vuScale, wxEXPAND );
    AddStaticText(sliderSizer, _T("Placeholder text for VU scale position."));


    AddCheckBox(miscSizer, _T("Enable IOP x2 Cycle rate"));
    AddStaticText(miscSizer, _T("    Small Speedup, and works well with most games."), 400);

    AddCheckBox(miscSizer, _T("WaitCycles Sync Hack") );
    AddStaticText(miscSizer, _T("    Small Speedup. Works well with most games, but it may cause certain games to crash, or freeze up during bootup or stage changes."), 400);

    AddCheckBox(miscSizer, _T("INTC Sync Hack") );
    AddStaticText(miscSizer, _T("    Huge speedup in many games, and a pretty high compatability rate (some games still work better with EE sync hacks)."), 400);

    AddCheckBox(miscSizer, _T("Idle Loop Fast-Forward (experimental)") );
    AddStaticText(miscSizer, _T("    Speedup for a few games, including FFX with no known side effects.  More later."), 400);

	secondarySizer.Add( &sliderSizer, wxEXPAND );
	secondarySizer.Add( &miscSizer, wxEXPAND );
	mainSizer.Add( &secondarySizer, stdCenteredFlags );

	AddOkCancel( mainSizer );

	SetSizerAndFit( &mainSizer );

	Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( SpeedHacksDialog::IOPCycleDouble_Click ) );
	Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( SpeedHacksDialog::WaitCycleExt_Click ) );
	Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( SpeedHacksDialog::INTCSTATSlow_Click ) );
	Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( SpeedHacksDialog::IdleLoopFF_Click ) );
}

void SpeedHacksDialog::IOPCycleDouble_Click(wxCommandEvent &event)
{
    //Config.Hacks.IOPCycleDouble = if it is clicked.
    event.Skip();
}

void SpeedHacksDialog::WaitCycleExt_Click(wxCommandEvent &event)
{
    //Config.Hacks.WaitCycleExt = if it is clicked.
    event.Skip();
}

void SpeedHacksDialog::INTCSTATSlow_Click(wxCommandEvent &event)
{
    //Config.Hacks.INTCSTATSlow = if it is clicked.
    event.Skip();
}

void SpeedHacksDialog::IdleLoopFF_Click(wxCommandEvent &event)
{
    //Config.Hacks.IdleLoopFF = if it is clicked.
    event.Skip();
}
}	// end namespace Dialogs
