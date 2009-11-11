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

Panels::BaseAdvancedCpuOptions::BaseAdvancedCpuOptions( wxWindow& parent, int idealWidth ) :
	BaseApplicableConfigPanel( &parent, idealWidth )
,	s_adv( *new wxStaticBoxSizer( wxVERTICAL, this ) )
,	s_round( *new wxStaticBoxSizer( wxVERTICAL, this, _("Round Mode") ) )
,	s_clamp( *new wxStaticBoxSizer( wxVERTICAL, this, _("Clamping Mode") ) )
{

	m_Option_FTZ		= new pxCheckBox( this, _("Flush to Zero") );
	m_Option_DAZ		= new pxCheckBox( this, _("Denormals are Zero") );

	wxFlexGridSizer& grid = *new wxFlexGridSizer( 4 );

	// Clever proportions selected for a fairly nice spacing, with the third
	// column serving as a buffer between static box and a pair of checkboxes.

	grid.AddGrowableCol( 0, 17 );
	grid.AddGrowableCol( 1, 22 );
	grid.AddGrowableCol( 2, 1 );
	grid.AddGrowableCol( 3, 19 );

	m_StartNewRadioGroup = true;
	m_Option_Round[0]	= &AddRadioButton( s_round, _("Nearest") );
	m_Option_Round[1]	= &AddRadioButton( s_round, _("Negative") );
	m_Option_Round[2]	= &AddRadioButton( s_round, _("Positive") );
	m_Option_Round[3]	= &AddRadioButton( s_round, _("Chop / Zero") );

	m_StartNewRadioGroup = true;
	m_Option_None		= &AddRadioButton( s_clamp, _("None") );
	m_Option_Normal		= &AddRadioButton( s_clamp, _("Normal") );

	wxBoxSizer& s_daz( *new wxBoxSizer( wxVERTICAL ) );
	s_daz.AddSpacer( 12 );
	s_daz.Add( m_Option_FTZ );
	s_daz.Add( m_Option_DAZ );
	s_daz.AddSpacer( 4 );
	s_daz.AddSpacer( 22 );
	s_daz.Add( new wxButton( this, wxID_DEFAULT, _("Restore Defaults") ), wxSizerFlags().Align( wxALIGN_CENTRE ) );

	grid.Add( &s_round, SizerFlags::SubGroup() );
	grid.Add( &s_clamp, SizerFlags::SubGroup() );
	grid.Add( new wxBoxSizer( wxVERTICAL ) );		// spacer column!
	grid.Add( &s_daz, wxSizerFlags().Expand() );

	s_adv.Add( &grid, SizerFlags::StdExpand() );

	SetSizer( &s_adv );

	Connect( wxID_DEFAULT, wxCommandEventHandler( BaseAdvancedCpuOptions::OnRestoreDefaults ) );
}

void Panels::BaseAdvancedCpuOptions::OnRestoreDefaults(wxCommandEvent &evt)
{
	m_Option_Round[3]->SetValue(true);
	m_Option_Normal->SetValue(true);

	m_Option_DAZ->SetValue(true);
	m_Option_FTZ->SetValue(true);
}

Panels::AdvancedOptionsFPU::AdvancedOptionsFPU( wxWindow& parent, int idealWidth ) :
	BaseAdvancedCpuOptions( parent, idealWidth )
{
	s_adv.GetStaticBox()->SetLabel(_("EE/FPU Advanced Recompiler Options"));

	m_Option_ExtraSign	= &AddRadioButton( s_clamp, _("Extra + Preserve Sign") );
	m_Option_Full		= &AddRadioButton( s_clamp, _("Full") );

	Pcsx2Config::CpuOptions& cpuOps( g_Conf->EmuOptions.Cpu );
	Pcsx2Config::RecompilerOptions& recOps( cpuOps.Recompiler );

	m_Option_FTZ->SetValue( cpuOps.sseMXCSR.FlushToZero );
	m_Option_DAZ->SetValue( cpuOps.sseMXCSR.DenormalsAreZero );

	m_Option_Round[cpuOps.sseMXCSR.RoundingControl]->SetValue( true );

	m_Option_Normal->SetValue( recOps.fpuOverflow );
	m_Option_ExtraSign->SetValue( recOps.fpuExtraOverflow );
	m_Option_Full->SetValue( recOps.fpuFullMode );
	m_Option_None->SetValue( !recOps.fpuOverflow && !recOps.fpuExtraOverflow && !recOps.fpuFullMode );
}


Panels::AdvancedOptionsVU::AdvancedOptionsVU( wxWindow& parent, int idealWidth ) :
	BaseAdvancedCpuOptions( parent, idealWidth )
{
	s_adv.GetStaticBox()->SetLabel(_("VU0 / VU1 Advanced Recompiler Options"));

	m_Option_Extra		= &AddRadioButton( s_clamp, _("Extra") );
	m_Option_ExtraSign	= &AddRadioButton( s_clamp, _("Extra + Preserve Sign") );

	Pcsx2Config::CpuOptions& cpuOps( g_Conf->EmuOptions.Cpu );
	Pcsx2Config::RecompilerOptions& recOps( cpuOps.Recompiler );

	m_Option_FTZ->SetValue( cpuOps.sseVUMXCSR.FlushToZero );
	m_Option_DAZ->SetValue( cpuOps.sseVUMXCSR.DenormalsAreZero );

	m_Option_Round[cpuOps.sseVUMXCSR.RoundingControl]->SetValue( true );

	m_Option_Normal->SetValue( recOps.vuOverflow );
	m_Option_Extra->SetValue( recOps.vuExtraOverflow );
	m_Option_ExtraSign->SetValue( recOps.vuSignOverflow );
	m_Option_None->SetValue( !recOps.vuOverflow && !recOps.vuExtraOverflow && !recOps.vuSignOverflow );
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

void Panels::BaseAdvancedCpuOptions::ApplyRoundmode( SSE_MXCSR& mxcsr )
{
	for( int i=0; i<4; ++i )
	{
		if( m_Option_Round[i]->GetValue() )
		{
			mxcsr.RoundingControl = i;
			break;
		}
	}

	mxcsr.DenormalsAreZero	= m_Option_DAZ->GetValue();
	mxcsr.FlushToZero		= m_Option_FTZ->GetValue();
}

void Panels::AdvancedOptionsFPU::Apply()
{
	Pcsx2Config::CpuOptions& cpuOps( g_Conf->EmuOptions.Cpu );
	Pcsx2Config::RecompilerOptions& recOps( cpuOps.Recompiler );

	cpuOps.sseMXCSR = Pcsx2Config::CpuOptions().sseMXCSR;		// set default
	ApplyRoundmode( cpuOps.sseMXCSR );

	recOps.fpuExtraOverflow	= m_Option_ExtraSign->GetValue();
	recOps.fpuOverflow		= m_Option_Normal->GetValue() || recOps.fpuExtraOverflow;
	recOps.fpuFullMode		= m_Option_Full->GetValue();

	cpuOps.ApplySanityCheck();
}

void Panels::AdvancedOptionsVU::Apply()
{
	Pcsx2Config::CpuOptions& cpuOps( g_Conf->EmuOptions.Cpu );
	Pcsx2Config::RecompilerOptions& recOps( cpuOps.Recompiler );

	cpuOps.sseVUMXCSR = Pcsx2Config::CpuOptions().sseVUMXCSR;		// set default
	ApplyRoundmode( cpuOps.sseVUMXCSR );

	recOps.vuSignOverflow	= m_Option_ExtraSign->GetValue();
	recOps.vuExtraOverflow	= m_Option_Extra->GetValue() || recOps.vuSignOverflow;
	recOps.vuOverflow		= m_Option_Normal->GetValue() || recOps.vuExtraOverflow;

	cpuOps.ApplySanityCheck();
}
