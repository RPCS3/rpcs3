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
#include "GameFixesDialog.h"
#include "System.h"

using namespace wxHelpers;

namespace Dialogs {

GameFixesDialog::GameFixesDialog( wxWindow* parent, int id ):
	wxDialogWithHelpers( parent, id, _T("Game Special Fixes"), true )
{
	wxStaticText* label_Title = new wxStaticText(
		this, wxID_ANY, _T("Some games need special settings.\nConfigure them here."), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE
	);

	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );
	wxStaticBoxSizer& groupSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("PCSX2 Gamefixes") );

	AddCheckBox( groupSizer, _T("FPU Compare Hack - Special fix for Digimon Rumble Arena 2.") );
	AddCheckBox( groupSizer, _T("FPU Multiply Hack - Special fix for Tales of Destiny.") );
	AddCheckBox( groupSizer, _T("VU Add / Sub Hack - Special fix for Tri-Ace games!") );
	AddCheckBox( groupSizer, _T("VU Clip Hack - Special fix for God of War") );

	mainSizer.Add( label_Title, stdCenteredFlags );
	mainSizer.Add( &groupSizer, stdSpacingFlags );
	AddOkCancel( mainSizer );

	SetSizerAndFit( &mainSizer );

	Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( GameFixesDialog::FPUCompareHack_Click ) );
	Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( GameFixesDialog::FPUMultHack_Click ) );
	Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( GameFixesDialog::TriAce_Click ) );
	Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( GameFixesDialog::GodWar_Click ) );

}

void GameFixesDialog::FPUCompareHack_Click(wxCommandEvent &event)
{
    //Config.GameFixes |= is_checked ? FLAG_FPU_Compare : 0;
    event.Skip();
}

void GameFixesDialog::FPUMultHack_Click(wxCommandEvent &event)
{
    //Config.GameFixes |= is_checked ? FLAG_FPU_MUL : 0;
    event.Skip();
}

void GameFixesDialog::TriAce_Click(wxCommandEvent &event)
{
	//Config.GameFixes |= is_checked ? FLAG_VU_ADD_SUB : 0;
    event.Skip();
}


void GameFixesDialog::GodWar_Click(wxCommandEvent &event)
{
    //Config.GameFixes |= is_checked ? FLAG_VU_CLIP : 0;
    event.Skip();
}

}	// end namespace Dialogs
