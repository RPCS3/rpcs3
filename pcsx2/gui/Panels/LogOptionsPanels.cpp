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
	: CheckedStaticBox( parent, wxVERTICAL, L"EE Logs" )
{
	CheckBoxDict& chks( parent->CheckBoxes );

	wxStaticBoxSizer& s_misc = *new wxStaticBoxSizer( wxVERTICAL, this, L"General" );
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

	hwPanel->SetValue( false );
	// ----------------------------------------------------------------------------
	CheckedStaticBox* evtPanel = new CheckedStaticBox( this, wxVERTICAL, L"Events" );
	wxSizer& s_evt( evtPanel->ThisSizer );

	chks["EE:Counters"]		= newCheckBox( evt, "Counters" );
	chks["EE:Memcards"]		= newCheckBox( evt, "Memcards" );
	chks["EE:VIF"]			= newCheckBox( evt, "VIF" );
	chks["EE:GIF"]			= newCheckBox( evt, "GIF" );
	chks["EE:IPU"]			= newCheckBox( evt, "IPU" );
	chks["EE:SPR"]			= newCheckBox( evt, "SPR" );

	evtPanel->SetValue( false );
	// ----------------------------------------------------------------------------
	wxFlexGridSizer& eeTable( *new wxFlexGridSizer( 2, 5 ) );
	
	eeTable.Add( &s_misc, SizerFlags::SubGroup() );
	eeTable.Add( hwPanel, SizerFlags::SubGroup() );
	eeTable.Add( evtPanel, SizerFlags::SubGroup() );
	eeTable.Add( disasmPanel, SizerFlags::SubGroup() );

	ThisSizer.AddSpacer( 4 );
	ThisSizer.Add( &eeTable );

	SetValue( true );
}

Panels::iopLogOptionsPanel::iopLogOptionsPanel( LogOptionsPanel* parent )
	: CheckedStaticBox( parent, wxHORIZONTAL, L"IOP Logs" )
{
	CheckBoxDict& chks( parent->CheckBoxes );

	wxStaticBoxSizer& s_misc = *new wxStaticBoxSizer( wxVERTICAL, this, L"General" );
	wxPanelWithHelpers* miscPanel = this;		// helper for our newCheckBox macro.

	chks["IOP:Memory"]		= newCheckBox( misc, "Memory" );
	chks["IOP:Bios"]		= newCheckBox( misc, "Bios" );
	chks["IOP:GPU"]			= newCheckBox( misc, "GPU (PS1 only)" );

	// ----------------------------------------------------------------------------
	CheckedStaticBox* disasmPanel = new CheckedStaticBox( this, wxVERTICAL, L"Disasm" );
	wxSizer& s_disasm( disasmPanel->ThisSizer );

	chks["IOP:R3000A"]		= newCheckBox( disasm, "R3000A" );
	chks["IOP:COP2"]		= newCheckBox( disasm, "COP2 (Geometry)" );

	disasmPanel->SetValue( false );
	// ----------------------------------------------------------------------------
	CheckedStaticBox* hwPanel = new CheckedStaticBox( this, wxVERTICAL, L"Hardware" );
	wxSizer& s_hw( hwPanel->ThisSizer );

	chks["IOP:KnownHw"]		= newCheckBox( hw, "Registers" );
	chks["IOP:UnknownHw"]	= newCheckBox( hw, "UnknownRegs" );
	chks["IOP:DMA"]			= newCheckBox( hw, "DMA" );

	hwPanel->SetValue( false );
	// ----------------------------------------------------------------------------
	CheckedStaticBox* evtPanel = new CheckedStaticBox( this, wxVERTICAL, L"Events" );
	wxSizer& s_evt( evtPanel->ThisSizer );

	chks["IOP:PAD"]			= newCheckBox( evt, "Pad" );
	chks["IOP:SPU2"]		= newCheckBox( evt, "SPU2" );
	chks["IOP:CDVD"]		= newCheckBox( evt, "CDVD" );
	chks["IOP:USB"]			= newCheckBox( evt, "USB" );
	chks["IOP:FW"]			= newCheckBox( evt, "FW" );

	// ----------------------------------------------------------------------------

	wxFlexGridSizer& iopTable( *new wxFlexGridSizer( 2, 5 ) );

	iopTable.Add( &s_misc, SizerFlags::SubGroup() );
	iopTable.Add( hwPanel, SizerFlags::SubGroup() );
	iopTable.Add( evtPanel, SizerFlags::SubGroup() );
	iopTable.Add( disasmPanel, SizerFlags::SubGroup() );

	ThisSizer.AddSpacer( 4 );
	ThisSizer.Add( &iopTable );

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
	
	CheckBoxes["SYMs"]		= newCheckBox( misc, "SYMs Log" );

	//miscSizer.Add( ("ELF") );

	topSizer.Add( &m_eeSection, SizerFlags::StdSpace() );
	topSizer.Add( &m_iopSection, SizerFlags::StdSpace() );

	mainsizer.Add( &topSizer );
	mainsizer.Add( &s_misc, SizerFlags::StdSpace() );

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

