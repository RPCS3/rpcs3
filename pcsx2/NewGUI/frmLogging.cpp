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

frmLogging::frmLogging(wxWindow* parent, int id, const wxPoint& pos, const wxSize& size, long style):
wxDialog( parent, id, _T("Logging"), pos, size )
{
	int i;
	wxStaticBox* mainbox = new wxStaticBox( this, -1, wxEmptyString);
	wxBoxSizer *mainsizer = new wxBoxSizer( wxHORIZONTAL );
		
	wxStaticBox* eebox = new wxStaticBox( this, -1, _T("EE Logs"));
	wxStaticBoxSizer *eeSizer = new wxStaticBoxSizer(  eebox, wxVERTICAL );
	eeSizer->Add(new wxCheckBox(this, EE_CPU_LOG, _T("Cpu Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_MEM_LOG, _T("Mem Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_HW_LOG, _T("Hw Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_DMA_LOG, _T("Dma Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_BIOS_LOG, _T("Bios Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_ELF_LOG, _T("Elf Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_FPU_LOG, _T("Fpu Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_MMI_LOG, _T("MMI Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_VU0_LOG, _T("VU0 Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_COP0_LOG, _T("Cop0 Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_VIF_LOG, _T("Vif Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_SPR_LOG, _T("SPR Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_GIF_LOG, _T("GIF Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_SIF_LOG, _T("Sif Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_IPU_LOG, _T("IPU Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_VU_MACRO_LOG, _T("VU Macro Log")), 0, wxEXPAND);
	eeSizer->Add(new wxCheckBox(this, EE_RPC_LOG, _T("RPC Log")), 0, wxEXPAND);
	
	mainsizer->Add(eeSizer, 0, wxEXPAND);
	
	wxStaticBox* iopbox = new wxStaticBox( this, -1, _T("IOP Logs"));
	wxStaticBoxSizer *iopSizer = new wxStaticBoxSizer(  iopbox, wxVERTICAL );
	iopSizer->Add(new wxCheckBox(this, IOP_IOP_LOG, _T("IOP Log")), 0, wxEXPAND);
	iopSizer->Add(new wxCheckBox(this, IOP_MEM_LOG, _T("Mem Log")), 0, wxEXPAND);
	iopSizer->Add(new wxCheckBox(this, IOP_HW_LOG, _T("Hw Log")), 0, wxEXPAND);
	iopSizer->Add(new wxCheckBox(this, IOP_BIOS_LOG, _T("Bios Log")), 0, wxEXPAND);
	iopSizer->Add(new wxCheckBox(this, IOP_DMA_LOG, _T("Dma Log")), 0, wxEXPAND);
	iopSizer->Add(new wxCheckBox(this, IOP_PAD_LOG, _T("Pad Log")), 0, wxEXPAND);
	iopSizer->Add(new wxCheckBox(this, IOP_GTE_LOG, _T("Gte Log")), 0, wxEXPAND);
	iopSizer->Add(new wxCheckBox(this, IOP_CDR_LOG, _T("Cdr Log")), 0, wxEXPAND);
	iopSizer->Add(new wxCheckBox(this, IOP_GPU_LOG, _T("GPU Log")), 0, wxEXPAND);
	
	mainsizer->Add(iopSizer, 0, wxEXPAND);
	
	wxStaticBox* miscbox = new wxStaticBox( this, -1, _T("Misc"));
	wxStaticBoxSizer *miscSizer = new wxStaticBoxSizer(  miscbox, wxVERTICAL );
	miscSizer->Add(new wxCheckBox(this, STDOUT_LOG, _T("Log to STDOUT")), 0, wxEXPAND);
	miscSizer->Add(new wxCheckBox(this, SYMS_LOG, _T("SYMs Log")), 0, wxEXPAND);

	mainsizer->Add(miscSizer, 0, wxEXPAND);
	
	SetSizerAndFit( mainsizer, true );
	
	// Connect all the checkboxes to one function, and pass the checkbox id as user data.
	for (i = EE_CPU_LOG; i >= SYMS_LOG; i++)
		Connect(i, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(frmLogging::LogChecked), i);
}


void frmLogging::LogChecked(wxCommandEvent &event)
{
	// The checkbox checked should be in event.m_callbackUserData.
	event.Skip();
}



