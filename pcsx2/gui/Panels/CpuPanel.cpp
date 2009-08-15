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
	AddRadioButton( s_ee, _("Recompiler") );

	m_StartNewRadioGroup = true;
	AddRadioButton( s_iop, _("Interpreter") );
	AddRadioButton( s_iop, _("Recompiler") );

	m_StartNewRadioGroup = true;
	AddRadioButton( s_vu0, _("Interpreter") );
	AddRadioButton( s_vu0, _("microVU Recompiler [new!]") );
	AddRadioButton( s_vu0, _("superVU Recompiler [legacy]"), wxEmptyString, _("Useful for diagnosing possible bugs in the new mVU recompiler.") );

	m_StartNewRadioGroup = true;
	AddRadioButton( s_vu1, _("Interpreter") );
	AddRadioButton( s_vu1, _("microVU Recompiler [new!]") );
	AddRadioButton( s_vu1, _("superVU Recompiler [legacy]"), wxEmptyString, _("Useful for diagnosing possible bugs in the new mVU recompiler.") );
	
	s_main.Add( &s_ee, SizerFlags::StdExpand() );
	s_main.Add( &s_iop, SizerFlags::StdExpand() );
	s_main.Add( &s_vu0, SizerFlags::StdExpand() );
	s_main.Add( &s_vu1, SizerFlags::StdExpand() );
	
	SetSizerAndFit( &s_main );
}

void Panels::CpuPanel::Apply( AppConfig& conf )
{
}
