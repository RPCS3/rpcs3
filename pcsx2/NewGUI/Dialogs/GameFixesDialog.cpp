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
	AddCheckBox( groupSizer, _T("VU Add / Sub Hack - Special fix for Tri-Ace games!") );
	AddCheckBox( groupSizer, _T("VU Clip Hack - Special fix for God of War") );

	mainSizer.Add( label_Title, stdCenteredFlags );
	mainSizer.Add( &groupSizer, stdSpacingFlags );
	AddOkCancel( mainSizer );

	SetSizerAndFit( &mainSizer );
}


BEGIN_EVENT_TABLE(GameFixesDialog, wxDialog)
    EVT_CHECKBOX(wxID_ANY, FPUCompareHack_Click)
    EVT_CHECKBOX(wxID_ANY, TriAce_Click)
    EVT_CHECKBOX(wxID_ANY, GodWar_Click)
END_EVENT_TABLE();


void GameFixesDialog::FPUCompareHack_Click(wxCommandEvent &event)
{
    event.Skip();
}


void GameFixesDialog::TriAce_Click(wxCommandEvent &event)
{
    event.Skip();
}


void GameFixesDialog::GodWar_Click(wxCommandEvent &event)
{
    event.Skip();
}

}	// end namespace Dialogs