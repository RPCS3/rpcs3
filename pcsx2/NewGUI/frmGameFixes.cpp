
#include "PrecompiledHeader.h"
#include "Misc.h"
#include "frmGameFixes.h"

frmGameFixes::frmGameFixes(wxWindow* parent, int id, const wxPoint& pos, const wxSize& size, long style):
    wxDialog( parent, id, _t("Game Special Fixes"), pos, size )
{
	wxStaticBox* groupbox			= new wxStaticBox( this, -1, _t("PCSX2 Gamefixes"));
	wxStaticText* label_Title		= new wxStaticText(
		this, wxID_ANY, "Some games need special settings.\nConfigure them here.", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE
	);
	wxCheckBox* chk_FPUCompareHack	= new wxCheckBox(
		this, wxID_ANY, "FPU Compare Hack - Special fix for Digimon Rumble Arena 2."
	);
	wxCheckBox* chk_TriAce			= new wxCheckBox(
		this, wxID_ANY, "VU Add / Sub Hack - Special fix for Tri-Ace games!"
	);
	wxCheckBox* chk_GodWar			= new wxCheckBox(
		this, wxID_ANY, "VU Clip Hack - Special fix for God of War"
	);

	wxBoxSizer& mainSizer	= *new wxBoxSizer( wxVERTICAL );
	wxStaticBoxSizer& groupSizer = *new wxStaticBoxSizer( groupbox, wxVERTICAL );
	wxSizerFlags stdSpacing( wxSizerFlags().Border( wxALL, 6 ) );

	groupSizer.Add( chk_FPUCompareHack, stdSpacing );
	groupSizer.Add( chk_TriAce, stdSpacing );
	groupSizer.Add( chk_GodWar, stdSpacing );

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
