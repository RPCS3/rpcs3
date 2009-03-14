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
#include "frmGameFixes.h"

frmGameFixes::frmGameFixes(wxWindow* parent, int id, const wxPoint& pos, const wxSize& size, long style):
wxDialog( parent, id, _T("Game Special Fixes"), pos, size )
{
	wxStaticBox* groupbox = new wxStaticBox( this, -1, _T("PCSX2 Gamefixes"));
	wxStaticText* label_Title = new wxStaticText(
		this, wxID_ANY, _T("Some games need special settings.\nConfigure them here."), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE
	);
	wxCheckBox* chk_FPUCompareHack = new wxCheckBox(
		this, wxID_ANY, _T("FPU Compare Hack - Special fix for Digimon Rumble Arena 2.")
	);
	wxCheckBox* chk_TriAce = new wxCheckBox(
		this, wxID_ANY, _T("VU Add / Sub Hack - Special fix for Tri-Ace games!")
	);
	wxCheckBox* chk_GodWar = new wxCheckBox(
		this, wxID_ANY, _T("VU Clip Hack - Special fix for God of War")
	);

	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );
	wxStaticBoxSizer& groupSizer = *new wxStaticBoxSizer( groupbox, wxVERTICAL );
	wxSizerFlags stdSpacing( wxSizerFlags().Border( wxALL, 6 ) );

	groupSizer.Add( chk_FPUCompareHack);
	groupSizer.Add( chk_TriAce);
	groupSizer.Add( chk_GodWar);

	mainSizer.Add( label_Title,  wxSizerFlags().Align(wxALIGN_CENTER).DoubleBorder() );
	mainSizer.Add( &groupSizer,  wxSizerFlags().Align(wxALIGN_CENTER).DoubleHorzBorder() );
	mainSizer.Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ), wxSizerFlags().Align(wxALIGN_RIGHT).Border() );

	SetSizerAndFit( &mainSizer );
}


BEGIN_EVENT_TABLE(frmGameFixes, wxDialog)
    EVT_CHECKBOX(wxID_ANY, FPUCompareHack_Click)
    EVT_CHECKBOX(wxID_ANY, TriAce_Click)
    EVT_CHECKBOX(wxID_ANY, GodWar_Click)
END_EVENT_TABLE();


void frmGameFixes::FPUCompareHack_Click(wxCommandEvent &event)
{
    event.Skip();
}


void frmGameFixes::TriAce_Click(wxCommandEvent &event)
{
    event.Skip();
}


void frmGameFixes::GodWar_Click(wxCommandEvent &event)
{
    event.Skip();
}
