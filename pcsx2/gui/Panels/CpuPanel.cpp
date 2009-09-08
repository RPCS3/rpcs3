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

Panels::CpuPanel::CpuPanel( wxWindow& parent, int idealWidth ) :
	BaseApplicableConfigPanel( &parent, idealWidth )
{
	wxFlexGridSizer& s_main = *new wxFlexGridSizer( 2 );
	
	s_main.AddGrowableCol( 0, 1 );
	s_main.AddGrowableCol( 1, 1 );
	
	// i18n: No point in translating PS2 CPU names :)
	wxStaticBoxSizer& s_ee  = *new wxStaticBoxSizer( wxVERTICAL, this, L"EmotionEngine" );
	wxStaticBoxSizer& s_iop = *new wxStaticBoxSizer( wxVERTICAL, this, L"IOP" );
	wxStaticBoxSizer& s_vu0 = *new wxStaticBoxSizer( wxVERTICAL, this, L"VU0" );
	wxStaticBoxSizer& s_vu1 = *new wxStaticBoxSizer( wxVERTICAL, this, L"VU1" );
	
	m_StartNewRadioGroup = true;
	AddRadioButton( s_ee, _("Interpreter"), wxEmptyString, _("Quite possibly the slowest thing in the universe.") );
	m_Option_RecEE = &AddRadioButton( s_ee, _("Recompiler [Preferred]") );

	m_StartNewRadioGroup = true;
	AddRadioButton( s_iop, _("Interpreter") );
	m_Option_RecIOP = &AddRadioButton( s_iop, _("Recompiler [Preferred]") );

	m_StartNewRadioGroup = true;
	AddRadioButton( s_vu0, _("Interpreter"), wxEmptyString, _("Vector Unit Interpreter. Slow and not very compatible. Only use for testing.") ).SetValue( true );
	m_Option_mVU0 = &AddRadioButton( s_vu0, _("microVU Recompiler [Preferred]"), wxEmptyString, _("New Vector Unit recompiler.") );
	m_Option_sVU0 = &AddRadioButton( s_vu0, _("superVU Recompiler [legacy]"), wxEmptyString, _("Useful for diagnosing possible bugs in the new mVU recompiler.") );

	m_StartNewRadioGroup = true;
	AddRadioButton( s_vu1, _("Interpreter"), wxEmptyString, _("Vector Unit Interpreter. Slow and not very compatible. Only use for testing.") ).SetValue( true );
	m_Option_mVU1 = &AddRadioButton( s_vu1, _("microVU Recompiler [Preferred]"), wxEmptyString, _("New Vector Unit recompiler.") );
	m_Option_sVU1 = &AddRadioButton( s_vu1, _("superVU Recompiler [legacy]"), wxEmptyString, _("Useful for diagnosing possible bugs in the new mVU recompiler.") );
	
	s_main.Add( &s_ee, SizerFlags::StdExpand() );
	s_main.Add( &s_iop, SizerFlags::StdExpand() );
	s_main.Add( &s_vu0, SizerFlags::StdExpand() );
	s_main.Add( &s_vu1, SizerFlags::StdExpand() );

	// [TODO] : Add advanced CPU settings -- FPU/VU rounding, clamping, etc.

	SetSizer( &s_main );
	
	// ----------------------------------------------------------------------------
	// Apply current configuration options...
	
	Pcsx2Config::RecompilerOptions& recOps( g_Conf->EmuOptions.Cpu.Recompiler );

	m_Option_RecEE->SetValue( recOps.EnableEE );
	m_Option_RecIOP->SetValue( recOps.EnableIOP );
	
	if( recOps.UseMicroVU0 )
		m_Option_mVU0->SetValue( recOps.EnableVU0 );
	else
		m_Option_sVU0->SetValue( recOps.EnableVU0 );

	if( recOps.UseMicroVU1 )
		m_Option_mVU1->SetValue( recOps.EnableVU1 );
	else
		m_Option_sVU1->SetValue( recOps.EnableVU1 );
}

void Panels::CpuPanel::Apply( AppConfig& conf )
{
	Pcsx2Config::RecompilerOptions& recOps( conf.EmuOptions.Cpu.Recompiler );
	recOps.EnableEE		= m_Option_RecEE->GetValue();
	recOps.EnableIOP	= m_Option_RecIOP->GetValue();
	recOps.EnableVU0	= m_Option_mVU0->GetValue() || m_Option_sVU0->GetValue();
	recOps.EnableVU1	= m_Option_mVU1->GetValue() || m_Option_sVU1->GetValue();

	recOps.UseMicroVU0	= m_Option_mVU0->GetValue();
	recOps.UseMicroVU1	= m_Option_mVU1->GetValue();
}
