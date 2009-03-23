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
#include "frmLogging.h"

#include <wx/statline.h>

using namespace wxHelpers;

frmLogging::frmLogging(wxWindow* parent, int id, const wxPoint& pos, const wxSize& size):
	wxDialogWithHelpers( parent, id, _T("Logging"), pos, size )
{
	int i;
	wxStaticBox* mainbox = new wxStaticBox( this, -1, wxEmptyString);
	wxBoxSizer *mainsizer = new wxBoxSizer( wxHORIZONTAL );
		
	wxStaticBoxSizer& eeDisasm = *new wxStaticBoxSizer( wxVERTICAL, this, _T("EE Disasm") );
	wxStaticBoxSizer& eeHw = *new wxStaticBoxSizer( wxVERTICAL, this, _T("EE Hardware") );
	
	AddCheckBox( eeDisasm, _T("Core"),		EE_CPU_LOG );
	AddCheckBox( eeDisasm, _T("Memory"),	EE_MEM_LOG );
	AddCheckBox( eeDisasm, _T("Bios"),		EE_BIOS_LOG );
	AddCheckBox( eeDisasm, _T("Elf"),		EE_ELF_LOG );
	AddCheckBox( eeDisasm, _T("Fpu"),		EE_FPU_LOG );
	AddCheckBox( eeDisasm, _T("VU0"),		EE_VU0_LOG );
	AddCheckBox( eeDisasm, _T("Cop0"),		EE_COP0_LOG );
	AddCheckBox( eeDisasm, _T("VU Macro"),	EE_VU_MACRO_LOG );

	AddCheckBox( eeHw, _T("Registers"),	EE_HW_LOG );
	AddCheckBox( eeHw, _T("Dma"),		EE_DMA_LOG );
	AddCheckBox( eeHw, _T("Vif"),		EE_VIF_LOG );
	AddCheckBox( eeHw, _T("SPR"),		EE_SPR_LOG );
	AddCheckBox( eeHw, _T("GIF"),		EE_GIF_LOG );
	AddCheckBox( eeHw, _T("Sif"),		EE_SIF_LOG );
	AddCheckBox( eeHw, _T("IPU"),		EE_IPU_LOG );
	AddCheckBox( eeHw, _T("RPC"),		EE_RPC_LOG );
	
	wxStaticBox* iopbox = new wxStaticBox( this, -1, _T("IOP Logs"));
	wxStaticBoxSizer& iopSizer = *new wxStaticBoxSizer(  iopbox, wxVERTICAL );
	AddCheckBox( iopSizer, _T("IOP Log"),	IOP_IOP_LOG );
	AddCheckBox( iopSizer, _T("Mem Log"),	IOP_MEM_LOG );
	AddCheckBox( iopSizer, _T("Hw Log"),	IOP_HW_LOG);
	AddCheckBox( iopSizer, _T("Bios Log"),	IOP_BIOS_LOG );
	AddCheckBox( iopSizer, _T("Dma Log"),	IOP_DMA_LOG );
	AddCheckBox( iopSizer, _T("Pad Log"),	IOP_PAD_LOG );
	AddCheckBox( iopSizer, _T("Gte Log"),	IOP_GTE_LOG );
	AddCheckBox( iopSizer, _T("Cdr Log"),	IOP_CDR_LOG );
	AddCheckBox( iopSizer, _T("GPU Log"),	IOP_GPU_LOG );
	
	wxStaticBox* miscbox = new wxStaticBox( this, -1, _T("Misc"));
	wxStaticBoxSizer& miscSizer = *new wxStaticBoxSizer(  miscbox, wxVERTICAL );
	AddCheckBox( miscSizer, _T("Log to STDOUT"), STDOUT_LOG );
	AddCheckBox( miscSizer, _T("SYMs Log"), SYMS_LOG );

	mainsizer->Add( &eeDisasm, stdSpacingFlags );
	mainsizer->Add( &eeHw, stdSpacingFlags );
	mainsizer->Add( &iopSizer, stdSpacingFlags );
	mainsizer->Add( &miscSizer, stdSpacingFlags );
	
	SetSizerAndFit( mainsizer, true );
	
	
	// Connect all the checkboxes to one function, and pass the checkbox id as user data.
	// (user data pointer isn't actually a pointer, but instead just the checkbox id)
	for (i = EE_CPU_LOG; i >= SYMS_LOG; i++)
		Connect( i, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(frmLogging::LogChecked), (wxObject*)i );
}


void frmLogging::LogChecked(wxCommandEvent &event)
{
	int checkId = (int)event.m_callbackUserData;
	event.Skip();
}



