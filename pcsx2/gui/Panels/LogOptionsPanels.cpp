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
#include "LogOptionsPanels.h"

#include "DebugTools/Debug.h"


using namespace wxHelpers;

#define newCheckBox( name, label ) \
	(wxCheckBox*) s_##name.Add( new pxCheckBox( name##Panel, wxT(label) ) )->GetWindow()

Panels::eeLogOptionsPanel::eeLogOptionsPanel( LogOptionsPanel* parent )
	: CheckedStaticBox( parent, wxHORIZONTAL, L"EE Logs" )
{
	CheckBoxDict& chks( parent->CheckBoxes );

	wxBoxSizer& s_misc = *new wxBoxSizer( wxVERTICAL );
	wxPanelWithHelpers* miscPanel = this;		// helper for our newCheckBox macro.

	chks["EE:Memory"]		= newCheckBox( misc, "Memory" );
	chks["EE:Bios"]			= newCheckBox( misc, "Bios" );
	chks["EE:MMU"]			= newCheckBox( misc, "MMU" );

	// ----------------------------------------------------------------------------
	CheckedStaticBox* disasmPanel = new CheckedStaticBox( this, wxVERTICAL, L"Disasm" );
	wxSizer& s_disasm( disasmPanel->ThisSizer );
		
	chks["EE:R5900"]		= newCheckBox( disasm, "R5900" );
	chks["EE:COP0"]			= newCheckBox( disasm, "COP0 (MMU/SysCtrl)" );
	chks["EE:COP1"]			= newCheckBox( disasm, "COP1 (FPU)" );
	chks["EE:COP2"]			= newCheckBox( disasm, "COP2 (VU0 macro)" );

	chks["EE:VU0micro"]		= newCheckBox( disasm, "VU0 micro" );
	chks["EE:VU1micro"]		= newCheckBox( disasm, "VU1 micro" );

	disasmPanel->SetValue( false );
	// ----------------------------------------------------------------------------
	CheckedStaticBox* hwPanel = new CheckedStaticBox( this, wxVERTICAL, L"Hardware" );
	wxSizer& s_hw( hwPanel->ThisSizer );

	chks["EE:KnownHw"]		= newCheckBox( hw, "Registers" );
	chks["EE:UnkownHw"]		= newCheckBox( hw, "Unknown Regs" );
	chks["EE:DMA"]			= newCheckBox( hw, "DMA" );
	chks["EE:VIF"]			= newCheckBox( hw, "VIF" );
	chks["EE:GIF"]			= newCheckBox( hw, "GIF" );
	chks["EE:SIF"]			= newCheckBox( hw, "SIF" );
	chks["EE:SPR"]			= newCheckBox( hw, "SPR" );
	chks["EE:IPU"]			= newCheckBox( hw, "IPU" );

	hwPanel->SetValue( false );
	// ----------------------------------------------------------------------------

	wxBoxSizer& eeStack = *new wxBoxSizer( wxVERTICAL );
	eeStack.Add( disasmPanel, SizerFlags::StdSpace() );
	eeStack.Add( &s_misc );

	ThisSizer.Add( hwPanel, SizerFlags::StdSpace() );
	ThisSizer.Add( &eeStack );

	SetValue( true );
}

Panels::iopLogOptionsPanel::iopLogOptionsPanel( LogOptionsPanel* parent )
	: CheckedStaticBox( parent, wxVERTICAL, L"IOP Logs" )
{
	CheckBoxDict& chks( parent->CheckBoxes );

	wxBoxSizer& s_misc = *new wxBoxSizer( wxVERTICAL );
	wxPanelWithHelpers* miscPanel = this;		// helper for our newCheckBox macro.

	chks["IOP:Memory"]		= newCheckBox( misc, "Memory" );
	chks["IOP:Bios"]		= newCheckBox( misc, "Bios" );

	// ----------------------------------------------------------------------------
	CheckedStaticBox* disasmPanel = new CheckedStaticBox( this, wxVERTICAL, L"Disasm" );
	wxSizer& s_disasm( disasmPanel->ThisSizer );

	chks["IOP:Disasm"]		= newCheckBox( disasm, "R3000A" );
	chks["IOP:COP2"]		= newCheckBox( disasm, "COP2 (Geometry)" );

	disasmPanel->SetValue( false );

	// ----------------------------------------------------------------------------
	CheckedStaticBox* hwPanel = new CheckedStaticBox( this, wxVERTICAL, L"Hardware" );
	wxSizer& s_hw( hwPanel->ThisSizer );

	chks["IOP:KnownHw"]		= newCheckBox( hw, "Registers" );
	chks["IOP:UnknownHw"]	= newCheckBox( hw, "UnknownRegs" );
	chks["IOP:DMA"]			= newCheckBox( hw, "DMA" );
	chks["IOP:Pad"]			= newCheckBox( hw, "Pad" );
	chks["IOP:CDVD"]		= newCheckBox( hw, "CDVD" );
	chks["IOP:GPU"]			= newCheckBox( hw, "GPU (PS1 only)" );

	hwPanel->SetValue( false );
	// ----------------------------------------------------------------------------

	wxBoxSizer& iopStack = *new wxBoxSizer( wxVERTICAL );
	iopStack.Add( disasmPanel, SizerFlags::StdSpace() );
	iopStack.Add( &s_misc );

	ThisSizer.Add( hwPanel, SizerFlags::StdSpace() );
	ThisSizer.Add( &iopStack );

	SetValue( true );
}

// --------------------------------------------------------------------------------------
//  LogOptionsPanel Implementations
// --------------------------------------------------------------------------------------
Panels::LogOptionsPanel::LogOptionsPanel(wxWindow* parent, int idealWidth )
	: BaseApplicableConfigPanel( parent, idealWidth )
	, m_eeSection	( *new eeLogOptionsPanel( this ) )
	, m_iopSection	( *new iopLogOptionsPanel( this ) )
{
	wxBoxSizer& mainsizer			= *new wxBoxSizer( wxVERTICAL );
	wxBoxSizer& topSizer			= *new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer&	s_misc		= *new wxStaticBoxSizer( wxHORIZONTAL, this, L"Misc" );
	wxPanelWithHelpers* miscPanel = this;		// helper for our newCheckBox macro.
	
	CheckBoxes["STDOUT"]	= newCheckBox( misc, "Log to STDOUT" );
	CheckBoxes["SYMs"]		= newCheckBox( misc, "SYMs Log" );

	//miscSizer.Add( ("ELF") );

	topSizer.Add( &m_eeSection, SizerFlags::StdSpace() ); //.Expand() );
	topSizer.Add( &m_iopSection, SizerFlags::StdSpace() ); //.Expand() );

	mainsizer.Add( &topSizer ); //, wxSizerFlags().Expand() );		// topsizer has it's own padding.
	mainsizer.Add( &s_misc, SizerFlags::StdSpace() ); //.Expand() );

	SetSizer( &mainsizer );
	Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(LogOptionsPanel::OnCheckBoxClicked) );
}


void Panels::LogOptionsPanel::OnCheckBoxClicked(wxCommandEvent &evt)
{
	m_IsDirty = true;
}

void Panels::LogOptionsPanel::Apply()
{
	CheckBoxDict& chks( CheckBoxes );

#define SetEE( name ) \
	g_Conf->EmuOptions.Trace.EE.m_##name	= chks["EE:"#name]->GetValue()

	SetEE(EnableAll);
	SetEE(Bios);
	SetEE(Memory);
	SetEE(SysCtrl);
	SetEE(VIFunpack);
	SetEE(GIFtag);

	SetEE(EnableDisasm);
	SetEE(R5900);
	SetEE(COP0);
	SetEE(COP1);
	SetEE(COP2);
	SetEE(VU0micro);
	SetEE(VU1micro);
	
	SetEE(EnableHardware);
	SetEE(KnownHw);
	SetEE(UnknownHw);
	SetEE(DMA);

	SetEE(EnableEvents);
	SetEE(Counters);
	SetEE(VIF);
	SetEE(GIF);
	SetEE(SPR);
	SetEE(IPU);
	
	// TODO -- IOP Section!
}

