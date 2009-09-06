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

using namespace wxHelpers;
static wxCheckBox* game_fix_checkbox[NUM_OF_GAME_FIXES];

Panels::GameFixesPanel::GameFixesPanel( wxWindow& parent, int idealWidth ) :
	BaseApplicableConfigPanel( &parent, idealWidth)
{
	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );
	AddStaticText( mainSizer, _("Some games need special settings.\nEnable them here.") );

	wxStaticBoxSizer& groupSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("PCSX2 Gamefixes") );
	game_fix_checkbox[0] = &AddCheckBox( groupSizer, _("VU Add Hack - for Tri-Ace games!") );
	game_fix_checkbox[0]->SetValue(EmuConfig.Gamefixes.VuAddSubHack);

	game_fix_checkbox[1] = &AddCheckBox( groupSizer, _( "VU Clip Flag Hack - for Persona games, maybe others.") );
	game_fix_checkbox[1]->SetValue(EmuConfig.Gamefixes.VuClipFlagHack);

	game_fix_checkbox[2] = &AddCheckBox( groupSizer, _("FPU Compare Hack - for Digimon Rumble Arena 2.") );
	game_fix_checkbox[2]->SetValue(EmuConfig.Gamefixes.FpuCompareHack);

	game_fix_checkbox[3] = &AddCheckBox( groupSizer, _("FPU Multiply Hack - for Tales of Destiny.") );
	game_fix_checkbox[3]->SetValue(EmuConfig.Gamefixes.FpuMulHack);

	game_fix_checkbox[4] = &AddCheckBox( groupSizer, _("DMA Execution Hack - for Fatal Frame.") );
	game_fix_checkbox[4]->SetValue(EmuConfig.Gamefixes.DMAExeHack);

	game_fix_checkbox[5] = &AddCheckBox( groupSizer, _("VU XGkick Hack - for Erementar Gerad.") );
	game_fix_checkbox[5]->SetValue(EmuConfig.Gamefixes.XgKickHack);

	// I may as well add this, since these aren't hooked in yet. If the consensus is against it,
	// I'll remove it.
	game_fix_checkbox[6] = &AddCheckBox( groupSizer, _("MPEG Hack - for Mana Khemia 1.") );
	game_fix_checkbox[6]->SetValue(EmuConfig.Gamefixes.MpegHack);

	mainSizer.Add( &groupSizer, wxSizerFlags().Centre() );

	AddStaticText( mainSizer, pxE( ".Panels:Gamefixes:Compat Warning",
		L"Enabling game fixes can cause compatibility or performance issues in other games.  You "
		L"will need to turn off fixes manually when changing games."
	));

	SetSizer( &mainSizer );

}

// There's a better way to do this. This was quicker to hack in, though, and can always be replaced later.
static void set_game_fix(int num, bool status)
{
    switch (num)
    {
        case 0: EmuConfig.Gamefixes.VuAddSubHack = status; break;
        case 1: EmuConfig.Gamefixes.VuClipFlagHack = status; break;
        case 2: EmuConfig.Gamefixes.FpuCompareHack = status; break;
        case 3: EmuConfig.Gamefixes.FpuMulHack = status; break;
        case 4: EmuConfig.Gamefixes.DMAExeHack = status; break;
        case 5: EmuConfig.Gamefixes.XgKickHack = status; break;
        case 6: EmuConfig.Gamefixes.MpegHack = status; break;
        default: break;
    }
    return;
}

void Panels::GameFixesPanel::Apply( AppConfig& conf )
{
    for (int i = 0; i < NUM_OF_GAME_FIXES; i++)
    {
        set_game_fix(i, game_fix_checkbox[i]->GetValue());
    }
}
