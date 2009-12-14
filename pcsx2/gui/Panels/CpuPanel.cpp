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

using namespace pxSizerFlags;

Panels::BaseAdvancedCpuOptions::BaseAdvancedCpuOptions( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	wxStaticBoxSizer*	s_round( new wxStaticBoxSizer( wxVERTICAL, this, _("Round Mode") ) );
	wxStaticBoxSizer*	s_clamp( new wxStaticBoxSizer( wxVERTICAL, this, _("Clamping Mode") ) );

	m_Option_FTZ		= new pxCheckBox( this, _("Flush to Zero") );
	m_Option_DAZ		= new pxCheckBox( this, _("Denormals are Zero") );

	const RadioPanelItem RoundModeChoices[] = 
	{
		RadioPanelItem(_("Nearest")),
		RadioPanelItem(_("Negative")),
		RadioPanelItem(_("Positive")),
		RadioPanelItem(_("Chop / Zero"))
	};
	
	const RadioPanelItem ClampModeChoices[] = 
	{
		RadioPanelItem(_("None")),
		RadioPanelItem(_("Normal")),	
	};

	m_RoundModePanel = new pxRadioPanel( this, RoundModeChoices );
	m_ClampModePanel = new pxRadioPanel( this, ClampModeChoices );

	// Highlight Default Options:
	
	m_RoundModePanel->SetDefaultItem( 3 );
	m_ClampModePanel->SetDefaultItem( 1 );

	// ---------------------------------
	//    The Fitting And Sizing Area
	// ---------------------------------

	wxFlexGridSizer& grid = *new wxFlexGridSizer( 4 );

	// Clever proportions selected for a fairly nice spacing, with the third
	// column serving as a buffer between static box and a pair of checkboxes.

	grid.AddGrowableCol( 0, 17 );
	grid.AddGrowableCol( 1, 22 );
	grid.AddGrowableCol( 2, 1 );
	grid.AddGrowableCol( 3, 19 );

	wxBoxSizer& s_daz( *new wxBoxSizer( wxVERTICAL ) );
	s_daz	+= 12;
	s_daz	+= m_Option_FTZ;
	s_daz	+= m_Option_DAZ;
	s_daz	+= 4;
	s_daz	+= 22;
	s_daz	+= new wxButton( this, wxID_DEFAULT, _("Restore Defaults") ) | pxCenter;

	*s_round+= m_RoundModePanel		| StdExpand();
	*s_clamp+= m_ClampModePanel		| StdExpand();

	grid	+= s_round				| SubGroup();
	grid	+= s_clamp				| SubGroup();
	grid	+= new wxBoxSizer( wxVERTICAL );		// spacer column!
	grid	+= &s_daz				| pxExpand;

	*this	+= grid					| StdExpand();

	Connect( wxID_DEFAULT, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BaseAdvancedCpuOptions::OnRestoreDefaults ) );
}

void Panels::BaseAdvancedCpuOptions::OnRestoreDefaults(wxCommandEvent &evt)
{
	m_RoundModePanel->SetSelection( 3 );		// Roundmode chop
	m_ClampModePanel->SetSelection( 1 );		// clamp mode normal

	m_Option_DAZ->SetValue(true);
	m_Option_FTZ->SetValue(true);
	
	evt.Skip();
}

Panels::AdvancedOptionsFPU::AdvancedOptionsFPU( wxWindow* parent )
	: BaseAdvancedCpuOptions( parent )
{
	AddFrame(_("EE/FPU Advanced Recompiler Options"));

	m_ClampModePanel->Append(_("Extra + Preserve Sign"));
	m_ClampModePanel->Append(_("Full"));

	m_RoundModePanel->Realize();
	m_ClampModePanel->Realize();

	OnSettingsChanged();
}


Panels::AdvancedOptionsVU::AdvancedOptionsVU( wxWindow* parent )
	: BaseAdvancedCpuOptions( parent )
{
	AddFrame(_("VU0 / VU1 Advanced Recompiler Options"));

	m_ClampModePanel->Append(_("Extra"));
	m_ClampModePanel->Append(_("Extra + Preserve Sign"));

	m_RoundModePanel->Realize();
	m_ClampModePanel->Realize();

	OnSettingsChanged();
}

Panels::CpuPanelEE::CpuPanelEE( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	const RadioPanelItem tbl_CpuTypes_EE[] =
	{
		RadioPanelItem(_("Interpreter"))
		.SetToolTip(_("Quite possibly the slowest thing in the universe.")),
			
		RadioPanelItem(_("Recompiler"))
		.SetToolTip(_("Performs just-in-time binary translation of 64-bit MIPS-IV machine code to x86."))
	};
	
	const RadioPanelItem tbl_CpuTypes_IOP[] =
	{
		RadioPanelItem(_("Interpreter"))
		.SetToolTip(_("Pretty slow; provided for diagnostic purposes only.")),

		RadioPanelItem(_("Recompiler"))
		.SetToolTip(_("Performs just-in-time binary translation of 32-bit MIPS-I machine code to x86."))
	};

	
	m_panel_RecEE	= &(new pxRadioPanel( this, tbl_CpuTypes_EE ))->SetDefaultItem( 1 );
	m_panel_RecIOP	= &(new pxRadioPanel( this, tbl_CpuTypes_IOP ))->SetDefaultItem( 1 );

	m_panel_RecEE->Realize();
	m_panel_RecIOP->Realize();

	// ---------------------------------
	//    The Fitting And Sizing Area
	// ---------------------------------

	wxFlexGridSizer&	s_recs( *new wxFlexGridSizer( 2 ) );

	s_recs.AddGrowableCol( 0, 1 );
	s_recs.AddGrowableCol( 1, 1 );

	// i18n: No point in translating PS2 CPU names :)
	wxStaticBoxSizer& s_ee	( *new wxStaticBoxSizer( wxVERTICAL, this, L"EmotionEngine" ) );
	wxStaticBoxSizer& s_iop	( *new wxStaticBoxSizer( wxVERTICAL, this, L"IOP" ) );

	s_ee	+= m_panel_RecEE	| StdExpand();
	s_iop	+= m_panel_RecIOP	| StdExpand();

	s_recs	+= s_ee				| SubGroup();
	s_recs	+= s_iop			| SubGroup();

	*this	+= &s_recs							| StdExpand();
	*this	+= new wxStaticLine( this )			| wxSF.Border(wxALL, 18).Expand();
	*this	+= new AdvancedOptionsFPU( this )	| StdExpand();

	OnSettingsChanged();
}

Panels::CpuPanelVU::CpuPanelVU( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	const RadioPanelItem tbl_CpuTypes_VU[] =
	{
		RadioPanelItem(_("Interpreter"))
		.SetToolTip(_("Vector Unit Interpreter. Slow and not very compatible. Only use for diagnostics.")),

		RadioPanelItem(_("microVU Recompiler"))
		.SetToolTip(_("New Vector Unit recompiler with much improved compatibility. Recommended.")),
		
		RadioPanelItem(_("superVU Recompiler [legacy]"))
		.SetToolTip(_("Useful for diagnosing bugs or clamping issues in the new mVU recompiler."))
	};

	m_panel_VU0 = &(new pxRadioPanel( this, tbl_CpuTypes_VU ))	->SetDefaultItem( 1 );
	m_panel_VU1 = &(new pxRadioPanel( this, tbl_CpuTypes_VU ))	->SetDefaultItem( 1 );

	m_panel_VU0->Realize();
	m_panel_VU1->Realize();

	// ---------------------------------
	//    The Fitting And Sizing Area
	// ---------------------------------

	wxFlexGridSizer&	s_recs( *new wxFlexGridSizer( 2 ) );

	s_recs.AddGrowableCol( 0, 1 );
	s_recs.AddGrowableCol( 1, 1 );

	wxStaticBoxSizer& s_vu0( *new wxStaticBoxSizer( wxVERTICAL, this, L"VU0" ) );
	wxStaticBoxSizer& s_vu1( *new wxStaticBoxSizer( wxVERTICAL, this, L"VU1" ) );

	s_vu0	+= m_panel_VU0	| StdExpand();
	s_vu1	+= m_panel_VU1	| StdExpand();

	s_recs	+= s_vu0		| SubGroup();
	s_recs	+= s_vu1		| SubGroup();

	*this	+= &s_recs							| StdExpand();
	*this	+= new wxStaticLine( this )			| wxSF.Border(wxALL, 18).Expand();
	*this	+= new AdvancedOptionsVU( this )	| StdExpand();

	OnSettingsChanged();
}

void Panels::CpuPanelEE::Apply()
{
	Pcsx2Config::RecompilerOptions& recOps( g_Conf->EmuOptions.Cpu.Recompiler );
	recOps.EnableEE		= !!m_panel_RecEE->GetSelection();
	recOps.EnableIOP	= !!m_panel_RecIOP->GetSelection();
}

void Panels::CpuPanelEE::OnSettingsChanged()
{
	m_panel_RecEE->Enable( x86caps.hasStreamingSIMD2Extensions );
	m_panel_RecIOP->Enable( x86caps.hasStreamingSIMD2Extensions );

	const Pcsx2Config::RecompilerOptions& recOps( g_Conf->EmuOptions.Cpu.Recompiler );
	m_panel_RecEE->SetSelection( (int)recOps.EnableEE );
	m_panel_RecIOP->SetSelection( (int)recOps.EnableIOP );
}

void Panels::CpuPanelVU::Apply()
{
	Pcsx2Config::RecompilerOptions& recOps( g_Conf->EmuOptions.Cpu.Recompiler );
	recOps.EnableVU0	= m_panel_VU0->GetSelection() > 0;
	recOps.EnableVU1	= m_panel_VU1->GetSelection() > 0;

	recOps.UseMicroVU0	= m_panel_VU0->GetSelection() == 1;
	recOps.UseMicroVU1	= m_panel_VU1->GetSelection() == 1;
}

void Panels::CpuPanelVU::OnSettingsChanged()
{
	m_panel_VU0->EnableItem( 1, x86caps.hasStreamingSIMD2Extensions );
	m_panel_VU0->EnableItem( 2, x86caps.hasStreamingSIMD2Extensions );

	m_panel_VU1->EnableItem( 1, x86caps.hasStreamingSIMD2Extensions );
	m_panel_VU1->EnableItem( 2, x86caps.hasStreamingSIMD2Extensions );

	Pcsx2Config::RecompilerOptions& recOps( g_Conf->EmuOptions.Cpu.Recompiler );
	if( recOps.UseMicroVU0 )
		m_panel_VU0->SetSelection( recOps.EnableVU0 ? 1 : 0 );
	else
		m_panel_VU0->SetSelection( recOps.EnableVU0 ? 2 : 0 );

	if( recOps.UseMicroVU1 )
		m_panel_VU1->SetSelection( recOps.EnableVU1 ? 1 : 0 );
	else
		m_panel_VU1->SetSelection( recOps.EnableVU1 ? 2 : 0 );
}

void Panels::BaseAdvancedCpuOptions::ApplyRoundmode( SSE_MXCSR& mxcsr )
{
	mxcsr.RoundingControl	= m_RoundModePanel->GetSelection();
	mxcsr.DenormalsAreZero	= m_Option_DAZ->GetValue();
	mxcsr.FlushToZero		= m_Option_FTZ->GetValue();
}

void Panels::AdvancedOptionsFPU::Apply()
{
	Pcsx2Config::CpuOptions& cpuOps( g_Conf->EmuOptions.Cpu );
	Pcsx2Config::RecompilerOptions& recOps( cpuOps.Recompiler );

	cpuOps.sseMXCSR = Pcsx2Config::CpuOptions().sseMXCSR;		// set default
	ApplyRoundmode( cpuOps.sseMXCSR );

	const int clampSel		= m_ClampModePanel->GetSelection();

	recOps.fpuOverflow		= clampSel >= 1;
	recOps.fpuExtraOverflow	= clampSel >= 2;
	recOps.fpuFullMode		= clampSel >= 3;

	cpuOps.ApplySanityCheck();
}

void Panels::AdvancedOptionsFPU::OnSettingsChanged()
{
	const Pcsx2Config::CpuOptions& cpuOps( g_Conf->EmuOptions.Cpu );
	const Pcsx2Config::RecompilerOptions& recOps( cpuOps.Recompiler );

	m_Option_FTZ->SetValue( cpuOps.sseMXCSR.FlushToZero );
	m_Option_DAZ->SetValue( cpuOps.sseMXCSR.DenormalsAreZero );

	m_RoundModePanel->SetSelection( cpuOps.sseMXCSR.RoundingControl );

	if( recOps.fpuFullMode )			m_ClampModePanel->SetSelection( 3 );
	else if( recOps.fpuExtraOverflow )	m_ClampModePanel->SetSelection( 2 );
	else if( recOps.fpuOverflow )		m_ClampModePanel->SetSelection( 1 );
	else								m_ClampModePanel->SetSelection( 0 );
}

void Panels::AdvancedOptionsVU::Apply()
{
	Pcsx2Config::CpuOptions& cpuOps( g_Conf->EmuOptions.Cpu );
	Pcsx2Config::RecompilerOptions& recOps( cpuOps.Recompiler );

	cpuOps.sseVUMXCSR = Pcsx2Config::CpuOptions().sseVUMXCSR;		// set default
	ApplyRoundmode( cpuOps.sseVUMXCSR );

	const int clampSel		= m_ClampModePanel->GetSelection();

	recOps.vuOverflow		= clampSel >= 1;
	recOps.vuExtraOverflow	= clampSel >= 2;
	recOps.vuSignOverflow	= clampSel >= 3;

	cpuOps.ApplySanityCheck();
}

void Panels::AdvancedOptionsVU::OnSettingsChanged()
{
	const Pcsx2Config::CpuOptions& cpuOps( g_Conf->EmuOptions.Cpu );
	const Pcsx2Config::RecompilerOptions& recOps( cpuOps.Recompiler );

	m_Option_FTZ->SetValue( cpuOps.sseVUMXCSR.FlushToZero );
	m_Option_DAZ->SetValue( cpuOps.sseVUMXCSR.DenormalsAreZero );

	m_RoundModePanel->SetSelection( cpuOps.sseVUMXCSR.RoundingControl );

	if( recOps.vuSignOverflow )			m_ClampModePanel->SetSelection( 3 );
	else if( recOps.vuExtraOverflow )	m_ClampModePanel->SetSelection( 2 );
	else if( recOps.vuOverflow )		m_ClampModePanel->SetSelection( 1 );
	else								m_ClampModePanel->SetSelection( 0 );
}

