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
#include "DebugTools/Debug.h"
#include "frmLogging.h"

#include <wx/statline.h>

using namespace wxHelpers;

void ConnectChildrenRecurse( wxWindow* parent, int eventType, wxObjectEventFunction handler )
{
	wxWindowList& list = parent->GetChildren();
	for( wxWindowList::iterator iter = list.begin(); iter != list.end(); ++iter)
	{
		wxWindow *current = *iter;
		ConnectChildrenRecurse( current, eventType, handler );
		parent->Connect( current->GetId(), eventType, handler );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
frmLogging::eeLogOptionsPanel::eeLogOptionsPanel( wxWindow* parent ) :
	CheckedStaticBox( parent, wxHORIZONTAL, wxT( "EE Logs" ), LogID_EEBox )
{
	wxBoxSizer& eeMisc = *new wxBoxSizer( wxVERTICAL );

	AddCheckBoxTo( this, eeMisc,	wxT("Memory"),	LogID_Memory );
	AddCheckBoxTo( this, eeMisc,	wxT("Bios"),	LogID_Bios );
	AddCheckBoxTo( this, eeMisc,	wxT("Elf"),		LogID_ELF );

	wxBoxSizer& eeStack = *new wxBoxSizer( wxVERTICAL );
	eeStack.Add( new DisasmPanel( this ), stdSpacingFlags );
	eeStack.Add( &eeMisc );

	ThisSizer.Add( new HwPanel( this ), stdSpacingFlags );
	ThisSizer.Add( &eeStack );

	SetValue( true );
	Fit();
}

frmLogging::eeLogOptionsPanel::DisasmPanel::DisasmPanel( wxWindow* parent ) :
	CheckedStaticBox( parent, wxVERTICAL, wxT( "Disasm" ), LogID_Disasm )
{
	AddCheckBox( _T("Core"),	LogID_CPU );
	AddCheckBox( _T("Fpu"),		LogID_FPU );
	AddCheckBox( _T("VU0"),		LogID_VU0 );
	AddCheckBox( _T("Cop0"),	LogID_COP0 );
	AddCheckBox( _T("VU Macro"),LogID_VU_Macro );

	SetValue( false );
	Fit();
}

frmLogging::eeLogOptionsPanel::HwPanel::HwPanel( wxWindow* parent ) :
	CheckedStaticBox( parent, wxVERTICAL, wxT( "Hardware" ), LogID_Hardware )
{
	AddCheckBox( _T("Registers"),LogID_Registers );
	AddCheckBox( _T("Dma"),		LogID_DMA );
	AddCheckBox( _T("Vif"),		LogID_VIF );
	AddCheckBox( _T("SPR"),		LogID_SPR );
	AddCheckBox( _T("GIF"),		LogID_GIF );
	AddCheckBox( _T("Sif"),		LogID_SIF );
	AddCheckBox( _T("IPU"),		LogID_IPU );
	AddCheckBox( _T("RPC"),		LogID_RPC );

	SetValue( false );
	Fit();
}

void frmLogging::eeLogOptionsPanel::OnLogChecked(wxCommandEvent &event)
{
	LogChecks checkId = (LogChecks)(int)event.m_callbackUserData;
	//ToggleLogOption( checkId );
	event.Skip();
}

//////////////////////////////////////////////////////////////////////////////////////////
//
frmLogging::iopLogOptionsPanel::iopLogOptionsPanel( wxWindow* parent ) :
	CheckedStaticBox( parent, wxVERTICAL, wxT( "IOP Logs" ), LogID_IopBox )
{
	AddCheckBox( _T("Disasm"),		LogID_Disasm);
	AddCheckBox( _T("Memory"),		LogID_Memory );
	AddCheckBox( _T("Bios"),		LogID_Bios );
	AddCheckBox( _T("Registers"),	LogID_Hardware );
	AddCheckBox( _T("Dma"),			LogID_DMA );
	AddCheckBox( _T("Pad"),			LogID_Pad );
	AddCheckBox( _T("Cdrom"),		LogID_Cdrom );
	AddCheckBox( _T("GPU (PSX)"),	LogID_GPU );

	SetValue( true );
	Fit();
};

//////////////////////////////////////////////////////////////////////////////////////////
//
frmLogging::frmLogging(wxWindow* parent, int id, const wxPoint& pos, const wxSize& size):
	wxDialogWithHelpers( parent, id, _T("Logging"), true, pos, size )
{
	eeLogOptionsPanel& eeBox = *new eeLogOptionsPanel( this );
	iopLogOptionsPanel& iopSizer = *new iopLogOptionsPanel( this );

	wxStaticBoxSizer& miscSizer = *new wxStaticBoxSizer( wxHORIZONTAL, this, _T("Misc") );
	AddCheckBox( miscSizer, _T("Log to STDOUT"), LogID_StdOut );
	AddCheckBox( miscSizer, _T("SYMs Log"), LogID_Symbols );

	wxBoxSizer& mainsizer = *new wxBoxSizer( wxVERTICAL );
	wxBoxSizer& topSizer = *new wxBoxSizer( wxHORIZONTAL );

	topSizer.Add( &eeBox, stdSpacingFlags );
	topSizer.Add( &iopSizer, stdSpacingFlags );

	mainsizer.Add( &topSizer );		// topsizer has it's own padding.
	mainsizer.Add( &miscSizer, stdSpacingFlags );

	AddOkCancel( mainsizer );

	SetSizerAndFit( &mainsizer, true );

	ConnectChildrenRecurse( this, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(frmLogging::LogChecked) );
}


void frmLogging::LogChecked(wxCommandEvent &evt)
{
	// Anything going here should be a checkbox, unless non-checkbox controls send CheckBox_Clicked commands
	// (which would seem bad).
	wxCheckBox* checker = wxStaticCast( evt.GetEventObject(), wxCheckBox );

	switch( checker->GetId() )
	{
		// [TODO] : Implement me!
	}
	
	evt.Skip();
}



