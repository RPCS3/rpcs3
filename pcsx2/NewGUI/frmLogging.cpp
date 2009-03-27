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

//wxString frmLogging::m_DisasmWarning( "\nWarning: Disasm dumps are incredibly slow, and generate extremely large logfiles." );

frmLogging::eeLogOptionsPanel::eeLogOptionsPanel( wxWindow* parent ) :
	CheckedStaticBox( parent, wxHORIZONTAL, "EE Logs" )
{
	wxStaticBoxSizer& eeDisasm = *new wxStaticBoxSizer( wxVERTICAL, this, _T("Disasm") );
	wxStaticBoxSizer& eeHw = *new wxStaticBoxSizer( wxVERTICAL, this, _T("Hardware") );

	wxBoxSizer& eeStack = *new wxBoxSizer( wxVERTICAL );
	wxBoxSizer& eeMisc = *new wxBoxSizer( wxVERTICAL );

	//wxToolTip
	wxHelpers::AddCheckBox( this, eeDisasm,_T("Core"),		EE_CPU_LOG );
	wxHelpers::AddCheckBox( this, eeDisasm,_T("Fpu"),		EE_FPU_LOG );
	wxHelpers::AddCheckBox( this, eeDisasm,_T("VU0"),		EE_VU0_LOG );
	wxHelpers::AddCheckBox( this, eeDisasm,_T("Cop0"),		EE_COP0_LOG );
	wxHelpers::AddCheckBox( this, eeDisasm,_T("VU Macro"),	EE_VU_MACRO_LOG );

	wxHelpers::AddCheckBox( this, eeMisc,	_T("Memory"),	EE_MEM_LOG );
	wxHelpers::AddCheckBox( this, eeMisc,	_T("Bios"),		EE_BIOS_LOG );
	wxHelpers::AddCheckBox( this, eeMisc,	_T("Elf"),		EE_ELF_LOG );

	wxHelpers::AddCheckBox( this, eeHw,	_T("Registers"),EE_HW_LOG );
	wxHelpers::AddCheckBox( this, eeHw,	_T("Dma"),		EE_DMA_LOG );
	wxHelpers::AddCheckBox( this, eeHw,	_T("Vif"),		EE_VIF_LOG );
	wxHelpers::AddCheckBox( this, eeHw,	_T("SPR"),		EE_SPR_LOG );
	wxHelpers::AddCheckBox( this, eeHw,	_T("GIF"),		EE_GIF_LOG );
	wxHelpers::AddCheckBox( this, eeHw,	_T("Sif"),		EE_SIF_LOG );
	wxHelpers::AddCheckBox( this, eeHw,	_T("IPU"),		EE_IPU_LOG );
	wxHelpers::AddCheckBox( this, eeHw,	_T("RPC"),		EE_RPC_LOG );

	// Sizer hierarchy:

	eeStack.Add( &eeDisasm, stdSpacingFlags );
	eeStack.Add( &eeMisc );

	BoxSizer.Add( &eeHw, stdSpacingFlags );
	BoxSizer.Add( &eeStack );

	SetSizerAndFit( &BoxSizer, true );
}


frmLogging::frmLogging(wxWindow* parent, int id, const wxPoint& pos, const wxSize& size):
	wxDialogWithHelpers( parent, id, _T("Logging"), true, pos, size )
{
	int i;

	//wxStaticBoxSizer& eeBox = *new wxStaticBoxSizer( wxHORIZONTAL, this );
	
	eeLogOptionsPanel& eeBox = *new eeLogOptionsPanel( this );
	
	wxStaticBoxSizer& iopSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _T("IOP Logs") );

	AddCheckBox( iopSizer, _T("Disasm"),	IOP_IOP_LOG );
	AddCheckBox( iopSizer, _T("Memory"),	IOP_MEM_LOG );
	AddCheckBox( iopSizer, _T("Bios"),		IOP_BIOS_LOG );
	AddCheckBox( iopSizer, _T("Registers"),	IOP_HW_LOG);
	AddCheckBox( iopSizer, _T("Dma"),		IOP_DMA_LOG );
	AddCheckBox( iopSizer, _T("Pad"),		IOP_PAD_LOG );
	AddCheckBox( iopSizer, _T("Cdrom"),		IOP_CDR_LOG );
	AddCheckBox( iopSizer, _T("GPU (PSX)"),	IOP_GPU_LOG );
	
	wxStaticBoxSizer& miscSizer = *new wxStaticBoxSizer(  wxHORIZONTAL, this, _T("Misc") );
	AddCheckBox( miscSizer, _T("Log to STDOUT"), STDOUT_LOG );
	AddCheckBox( miscSizer, _T("SYMs Log"), SYMS_LOG );

	wxBoxSizer& mainsizer = *new wxBoxSizer( wxVERTICAL );
	wxBoxSizer& topSizer = *new wxBoxSizer( wxHORIZONTAL );

	topSizer.Add( &eeBox, stdSpacingFlags );
	topSizer.Add( &iopSizer, stdSpacingFlags );

	mainsizer.Add( &topSizer );		// topsizer has it's own padding.
	mainsizer.Add( &miscSizer, stdSpacingFlags );

	AddOkCancel( mainsizer );

	SetSizerAndFit( &mainsizer, true );

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



