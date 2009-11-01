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

using namespace wxHelpers;

Panels::AdvancedOptionsFPU::AdvancedOptionsFPU( wxWindow& parent, int idealWidth ) : 
	BaseApplicableConfigPanel( &parent, idealWidth )
{
	wxStaticBoxSizer& s_adv	= *new wxStaticBoxSizer( wxVERTICAL, this, _("EE/FPU Advanced Recompiler Options") );
	wxStaticBoxSizer& s_round = *new wxStaticBoxSizer( wxVERTICAL, this, _("Round Mode") );
	wxStaticBoxSizer& s_clamp = *new wxStaticBoxSizer( wxVERTICAL, this, _("Clamping Mode") );

	wxFlexGridSizer& grid = *new wxFlexGridSizer( 4 );

	// Clever proportions selected for a fairly nice spacing, with the third
	// colum serving as a buffer between static box and a pair of checkboxes.

	grid.AddGrowableCol( 0, 8 );
	grid.AddGrowableCol( 1, 10 );
	grid.AddGrowableCol( 2, 1 );
	grid.AddGrowableCol( 3, 7 );

	AddRadioButton( s_round, _("Nearest") );
	AddRadioButton( s_round, _("Negative") );
	AddRadioButton( s_round, _("Positive") );
	AddRadioButton( s_round, _("Chop / Zero") );

	m_StartNewRadioGroup = true;
	AddRadioButton( s_clamp, _("None") );
	AddRadioButton( s_clamp, _("Normal") );
	AddRadioButton( s_clamp, _("Extra + Preserve Sign") );
	AddRadioButton( s_clamp, _("Full") );

	wxBoxSizer& s_daz( *new wxBoxSizer( wxVERTICAL ) );
	s_daz.AddSpacer( 16 );
	AddCheckBox( s_daz, _("Flush to Zero") );
	s_daz.AddSpacer( 6 );
	AddCheckBox( s_daz, _("Denormals are Zero") );

	grid.Add( &s_round, SizerFlags::SubGroup() );
	grid.Add( &s_clamp, SizerFlags::SubGroup() );
	grid.Add( new wxBoxSizer( wxVERTICAL ) );		// spacer column!
	grid.Add( &s_daz, wxSizerFlags().Expand() );

	s_adv.Add( &grid, SizerFlags::StdExpand() );
	
	SetSizer( &s_adv );
}


Panels::AdvancedOptionsVU::AdvancedOptionsVU( wxWindow& parent, int idealWidth ) : 
	BaseApplicableConfigPanel( &parent, idealWidth )
{
	wxStaticBoxSizer& s_adv	= *new wxStaticBoxSizer( wxVERTICAL, this, _("VU0 / VU1 Advanced Recompiler Options") );
	wxStaticBoxSizer& s_round = *new wxStaticBoxSizer( wxVERTICAL, this, _("Round Mode") );
	wxStaticBoxSizer& s_clamp = *new wxStaticBoxSizer( wxVERTICAL, this, _("Clamping Mode") );

	wxFlexGridSizer& grid = *new wxFlexGridSizer( 4 );

	// Clever proportions selected for a fairly nice spacing, with the third
	// colum serving as a buffer between static box and a pair of checkboxes.

	grid.AddGrowableCol( 0, 8 );
	grid.AddGrowableCol( 1, 10 );
	grid.AddGrowableCol( 2, 1 );
	grid.AddGrowableCol( 3, 7 );

	AddRadioButton( s_round, _("Nearest") );
	AddRadioButton( s_round, _("Negative") );
	AddRadioButton( s_round, _("Positive") );
	AddRadioButton( s_round, _("Chop / Zero") );

	m_StartNewRadioGroup = true;
	AddRadioButton( s_clamp, _("None") );
	AddRadioButton( s_clamp, _("Normal") );
	AddRadioButton( s_clamp, _("Extra") );
	AddRadioButton( s_clamp, _("Extra + Preserve Sign") );

	wxBoxSizer& s_daz( *new wxBoxSizer( wxVERTICAL ) );
	s_daz.AddSpacer( 16 );
	AddCheckBox( s_daz, _("Flush to Zero") );
	s_daz.AddSpacer( 6 );
	AddCheckBox( s_daz, _("Denormals are Zero") );

	grid.Add( &s_round, SizerFlags::SubGroup() );
	grid.Add( &s_clamp, SizerFlags::SubGroup() );
	grid.Add( new wxBoxSizer( wxVERTICAL ) );		// spacer column!
	grid.Add( &s_daz, wxSizerFlags().Expand() );

	s_adv.Add( &grid, SizerFlags::StdExpand() );

	SetSizer( &s_adv );
}

void Panels::AdvancedOptionsFPU::Apply()
{
}

void Panels::AdvancedOptionsVU::Apply()
{
}

Panels::CpuPanelEE::CpuPanelEE( wxWindow& parent, int idealWidth ) :
	BaseApplicableConfigPanel( &parent, idealWidth )
{
	wxBoxSizer& s_main = *new wxBoxSizer( wxVERTICAL );
	wxFlexGridSizer& s_recs = *new wxFlexGridSizer( 2 );
	
	s_recs.AddGrowableCol( 0, 1 );
	s_recs.AddGrowableCol( 1, 1 );
	
	// i18n: No point in translating PS2 CPU names :)
	wxStaticBoxSizer& s_ee  = *new wxStaticBoxSizer( wxVERTICAL, this, L"EmotionEngine" );
	wxStaticBoxSizer& s_iop = *new wxStaticBoxSizer( wxVERTICAL, this, L"IOP" );
	
	m_StartNewRadioGroup = true;
	AddRadioButton( s_ee, _("Interpreter"), wxEmptyString, _("Quite possibly the slowest thing in the universe.") );
	m_Option_RecEE = &AddRadioButton( s_ee, _("Recompiler [Preferred]") );

	m_StartNewRadioGroup = true;
	AddRadioButton( s_iop, _("Interpreter") );
	m_Option_RecIOP = &AddRadioButton( s_iop, _("Recompiler [Preferred]") );

	s_recs.Add( &s_ee, SizerFlags::SubGroup() );
	s_recs.Add( &s_iop, SizerFlags::SubGroup() );

	s_main.Add( &s_recs, SizerFlags::StdExpand() );
	s_main.Add( new wxStaticLine( this ), wxSizerFlags().Border(wxALL, 24).Expand() );
	s_main.Add( new AdvancedOptionsFPU( *this, idealWidth ), SizerFlags::StdExpand() );

	SetSizer( &s_main );

	// ----------------------------------------------------------------------------
	// Apply current configuration options...
	
	Pcsx2Config::RecompilerOptions& recOps( g_Conf->EmuOptions.Cpu.Recompiler );

	m_Option_RecEE->SetValue( recOps.EnableEE );
	m_Option_RecIOP->SetValue( recOps.EnableIOP );
}

Panels::CpuPanelVU::CpuPanelVU( wxWindow& parent, int idealWidth ) :
	BaseApplicableConfigPanel( &parent, idealWidth )
{
	wxBoxSizer& s_main = *new wxBoxSizer( wxVERTICAL );
	wxFlexGridSizer& s_recs = *new wxFlexGridSizer( 2 );

	s_recs.AddGrowableCol( 0, 1 );
	s_recs.AddGrowableCol( 1, 1 );

	wxStaticBoxSizer& s_vu0 = *new wxStaticBoxSizer( wxVERTICAL, this, L"VU0" );
	wxStaticBoxSizer& s_vu1 = *new wxStaticBoxSizer( wxVERTICAL, this, L"VU1" );

	m_StartNewRadioGroup = true;
	AddRadioButton( s_vu0, _("Interpreter"), wxEmptyString, _("Vector Unit Interpreter. Slow and not very compatible. Only use for testing.") ).SetValue( true );
	m_Option_mVU0 = &AddRadioButton( s_vu0, _("microVU Recompiler [Preferred]"), wxEmptyString, _("New Vector Unit recompiler.") );
	m_Option_sVU0 = &AddRadioButton( s_vu0, _("superVU Recompiler [legacy]"), wxEmptyString, _("Useful for diagnosing possible bugs in the new mVU recompiler.") );

	m_StartNewRadioGroup = true;
	AddRadioButton( s_vu1, _("Interpreter"), wxEmptyString, _("Vector Unit Interpreter. Slow and not very compatible. Only use for testing.") ).SetValue( true );
	m_Option_mVU1 = &AddRadioButton( s_vu1, _("microVU Recompiler [Preferred]"), wxEmptyString, _("New Vector Unit recompiler.") );
	m_Option_sVU1 = &AddRadioButton( s_vu1, _("superVU Recompiler [legacy]"), wxEmptyString, _("Useful for diagnosing possible bugs in the new mVU recompiler.") );

	s_recs.Add( &s_vu0, SizerFlags::SubGroup() );
	s_recs.Add( &s_vu1, SizerFlags::SubGroup() );

	s_main.Add( &s_recs, SizerFlags::StdExpand() );
	s_main.Add( new wxStaticLine( this ), wxSizerFlags().Border(wxALL, 24).Expand() );
	s_main.Add( new AdvancedOptionsVU( *this, idealWidth ), SizerFlags::StdExpand() );

	SetSizer( &s_main );

	// ----------------------------------------------------------------------------
	// Apply current configuration options...

	Pcsx2Config::RecompilerOptions& recOps( g_Conf->EmuOptions.Cpu.Recompiler );
	if( recOps.UseMicroVU0 )
		m_Option_mVU0->SetValue( recOps.EnableVU0 );
	else
		m_Option_sVU0->SetValue( recOps.EnableVU0 );

	if( recOps.UseMicroVU1 )
		m_Option_mVU1->SetValue( recOps.EnableVU1 );
	else
		m_Option_sVU1->SetValue( recOps.EnableVU1 );
}

void Panels::CpuPanelEE::Apply()
{
	Pcsx2Config::RecompilerOptions& recOps( g_Conf->EmuOptions.Cpu.Recompiler );
	recOps.EnableEE		= m_Option_RecEE->GetValue();
	recOps.EnableIOP	= m_Option_RecIOP->GetValue();
}

void Panels::CpuPanelVU::Apply()
{
	Pcsx2Config::RecompilerOptions& recOps( g_Conf->EmuOptions.Cpu.Recompiler );
	recOps.EnableVU0	= m_Option_mVU0->GetValue() || m_Option_sVU0->GetValue();
	recOps.EnableVU1	= m_Option_mVU1->GetValue() || m_Option_sVU1->GetValue();

	recOps.UseMicroVU0	= m_Option_mVU0->GetValue();
	recOps.UseMicroVU1	= m_Option_mVU1->GetValue();
}
