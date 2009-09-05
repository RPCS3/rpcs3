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

Panels::GameFixesPanel::GameFixesPanel( wxWindow& parent, int idealWidth ) :
	BaseApplicableConfigPanel( &parent, idealWidth)
{
	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );
	AddStaticText( mainSizer, _("Some games need special settings.\nEnable them here.") );

	wxStaticBoxSizer& groupSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("PCSX2 Gamefixes") );
	AddCheckBox( groupSizer, _("VU Add Hack - for Tri-Ace games!") );
	AddCheckBox( groupSizer, _( "VU Clip Flag Hack - for Persona games, maybe others.") );
	AddCheckBox( groupSizer, _("FPU Compare Hack - for Digimon Rumble Arena 2.") );
	AddCheckBox( groupSizer, _("FPU Multiply Hack - for Tales of Destiny.") );
	AddCheckBox( groupSizer, _("DMA Execution Hack - for Fatal Frame.") );
	AddCheckBox( groupSizer, _("VU XGkick Hack - for Erementar Gerad.") );

	// I may as well add this, since these aren't hooked in yet. If the consensus is against it,
	// I'll remove it.
	AddCheckBox( groupSizer, _("MPEG Hack - for Mana Khemia 1.") );

	mainSizer.Add( &groupSizer, wxSizerFlags().Centre() );

	AddStaticText( mainSizer, pxE( ".Panels:Gamefixes:Compat Warning",
		L"Enabling game fixes can cause compatibility or performance issues in other games.  You "
		L"will need to turn off fixes manually when changing games."
	));

	SetSizer( &mainSizer );

}

void Panels::GameFixesPanel::Apply( AppConfig& conf )
{

}
