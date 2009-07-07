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
#include "LogOptionsDialog.h"

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

namespace Dialogs
{
//////////////////////////////////////////////////////////////////////////////////////////
//
LogOptionsDialog::eeLogOptionsPanel::eeLogOptionsPanel( wxWindow* parent ) :
	CheckedStaticBox( parent, wxHORIZONTAL, L"EE Logs", LogID_EEBox )
{
	wxBoxSizer& eeMisc = *new wxBoxSizer( wxVERTICAL );

	AddCheckBoxTo( this, eeMisc, L"Memory",	LogID_Memory );
	AddCheckBoxTo( this, eeMisc, L"Bios",	LogID_Bios );
	AddCheckBoxTo( this, eeMisc, L"Elf",		LogID_ELF );

	wxBoxSizer& eeStack = *new wxBoxSizer( wxVERTICAL );
	eeStack.Add( new DisasmPanel( this ), stdSpacingFlags );
	eeStack.Add( &eeMisc );

	ThisSizer.Add( new HwPanel( this ), stdSpacingFlags );
	ThisSizer.Add( &eeStack );

	SetValue( true );
	Fit();
}

LogOptionsDialog::eeLogOptionsPanel::DisasmPanel::DisasmPanel( wxWindow* parent ) :
	CheckedStaticBox( parent, wxVERTICAL, L"Disasm" , LogID_Disasm )
{
	AddCheckBox( L"Core",	LogID_CPU );
	AddCheckBox( L"Fpu",		LogID_FPU );
	AddCheckBox( L"VU0",		LogID_VU0 );
	AddCheckBox( L"Cop0",	LogID_COP0 );
	AddCheckBox( L"VU Macro",LogID_VU_Macro );

	SetValue( false );
	Fit();
}

LogOptionsDialog::eeLogOptionsPanel::HwPanel::HwPanel( wxWindow* parent ) :
	CheckedStaticBox( parent, wxVERTICAL, L"Hardware", LogID_Hardware )
{
	AddCheckBox( L"Registers", LogID_Registers );
	AddCheckBox( L"Dma",		LogID_DMA );
	AddCheckBox( L"Vif",		LogID_VIF );
	AddCheckBox( L"SPR",		LogID_SPR );
	AddCheckBox( L"GIF",		LogID_GIF );
	AddCheckBox( L"Sif",		LogID_SIF );
	AddCheckBox( L"IPU",		LogID_IPU );
	AddCheckBox( L"RPC",		LogID_RPC );

	SetValue( false );
	Fit();
}

void LogOptionsDialog::eeLogOptionsPanel::OnLogChecked(wxCommandEvent &event)
{
	//LogChecks checkId = (LogChecks)(int)event.m_callbackUserData;
	//ToggleLogOption( checkId );
	event.Skip();
}

//////////////////////////////////////////////////////////////////////////////////////////
//
LogOptionsDialog::iopLogOptionsPanel::iopLogOptionsPanel( wxWindow* parent ) :
	CheckedStaticBox( parent, wxVERTICAL, L"IOP Logs", LogID_IopBox )
{
	AddCheckBox( L"Disasm",		LogID_Disasm);
	AddCheckBox( L"Memory",		LogID_Memory );
	AddCheckBox( L"Bios",		LogID_Bios );
	AddCheckBox( L"Registers",	LogID_Hardware );
	AddCheckBox( L"Dma",		LogID_DMA );
	AddCheckBox( L"Pad",		LogID_Pad );
	AddCheckBox( L"Cdrom",		LogID_Cdrom );
	AddCheckBox( L"GPU (PSX)",	LogID_GPU );

	SetValue( true );
	Fit();
};

//////////////////////////////////////////////////////////////////////////////////////////
//
LogOptionsDialog::LogOptionsDialog(wxWindow* parent, int id, const wxPoint& pos, const wxSize& size):
	wxDialogWithHelpers( parent, id, L"Logging", true, pos, size )
{
	eeLogOptionsPanel& eeBox = *new eeLogOptionsPanel( this );
	iopLogOptionsPanel& iopSizer = *new iopLogOptionsPanel( this );

	wxStaticBoxSizer& miscSizer = *new wxStaticBoxSizer( wxHORIZONTAL, this, _T("Misc") );
	AddCheckBox( miscSizer, L"Log to STDOUT", LogID_StdOut );
	AddCheckBox( miscSizer, L"SYMs Log", LogID_Symbols );

	wxBoxSizer& mainsizer = *new wxBoxSizer( wxVERTICAL );
	wxBoxSizer& topSizer = *new wxBoxSizer( wxHORIZONTAL );

	// Expand comments below are from an attempt of mine to make the dialog box resizable, but it
	// only wanted to work right for the miscSizer and I couldn't figure out why the CheckStaticBox
	// panel wouldn't also resize to fit the window.. :(  -- air

	topSizer.Add( &eeBox, stdSpacingFlags ); //.Expand() );
	topSizer.Add( &iopSizer, stdSpacingFlags ); //.Expand() );

	mainsizer.Add( &topSizer ); //, wxSizerFlags().Expand() );		// topsizer has it's own padding.
	mainsizer.Add( &miscSizer, stdSpacingFlags ); //.Expand() );

	AddOkCancel( mainsizer );

	SetSizerAndFit( &mainsizer, true );

	ConnectChildrenRecurse( this, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(LogOptionsDialog::LogChecked) );
}


void LogOptionsDialog::LogChecked(wxCommandEvent &evt)
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

}		// End Namespace Dialogs
