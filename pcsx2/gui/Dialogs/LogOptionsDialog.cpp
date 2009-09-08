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
	CheckedStaticBox( parent, wxHORIZONTAL, L"EE Logs" )
{
	wxBoxSizer& eeMisc = *new wxBoxSizer( wxVERTICAL );

	AddCheckBox( eeMisc, L"Memory" );
	AddCheckBox( eeMisc, L"Bios" );
	AddCheckBox( eeMisc, L"Elf" );

	wxBoxSizer& eeStack = *new wxBoxSizer( wxVERTICAL );
	eeStack.Add( new DisasmPanel( this ), SizerFlags::StdSpace() );
	eeStack.Add( &eeMisc );

	ThisSizer.Add( new HwPanel( this ), SizerFlags::StdSpace() );
	ThisSizer.Add( &eeStack );

	SetValue( true );
	Fit();
}

LogOptionsDialog::eeLogOptionsPanel::DisasmPanel::DisasmPanel( wxWindow* parent ) :
	CheckedStaticBox( parent, wxVERTICAL, L"Disasm" )
{
	AddCheckBox( L"Core" );
	AddCheckBox( L"Fpu" );
	AddCheckBox( L"VU0" );
	AddCheckBox( L"Cop0" );
	AddCheckBox( L"VU Macro" );

	SetValue( false );
	Fit();
}

LogOptionsDialog::eeLogOptionsPanel::HwPanel::HwPanel( wxWindow* parent ) :
	CheckedStaticBox( parent, wxVERTICAL, L"Hardware" )
{
	AddCheckBox( L"Registers" );
	AddCheckBox( L"Dma" );
	AddCheckBox( L"Vif" );
	AddCheckBox( L"SPR" );
	AddCheckBox( L"GIF" );
	AddCheckBox( L"Sif" );
	AddCheckBox( L"IPU" );
	AddCheckBox( L"RPC" );

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
	CheckedStaticBox( parent, wxVERTICAL, L"IOP Logs" )
{
	AddCheckBox( L"Disasm" );
	AddCheckBox( L"Memory" );
	AddCheckBox( L"Bios" );
	AddCheckBox( L"Registers" );
	AddCheckBox( L"Dma" );
	AddCheckBox( L"Pad" );
	AddCheckBox( L"Cdrom" );
	AddCheckBox( L"GPU (PSX)" );

	SetValue( true );
	Fit();
};

//////////////////////////////////////////////////////////////////////////////////////////
//
LogOptionsDialog::LogOptionsDialog(wxWindow* parent, int id, const wxPoint& pos, const wxSize& size):
	wxDialogWithHelpers( parent, id, _("Logging"), true, pos, size )
{
	eeLogOptionsPanel& eeBox = *new eeLogOptionsPanel( this );
	iopLogOptionsPanel& iopSizer = *new iopLogOptionsPanel( this );

	wxStaticBoxSizer& miscSizer = *new wxStaticBoxSizer( wxHORIZONTAL, this, _T("Misc") );
	AddCheckBox( miscSizer, L"Log to STDOUT" );
	AddCheckBox( miscSizer, L"SYMs Log" );

	wxBoxSizer& mainsizer = *new wxBoxSizer( wxVERTICAL );
	wxBoxSizer& topSizer = *new wxBoxSizer( wxHORIZONTAL );

	// Expand comments below are from an attempt of mine to make the dialog box resizable, but it
	// only wanted to work right for the miscSizer and I couldn't figure out why the CheckStaticBox
	// panel wouldn't also resize to fit the window.. :(  -- air

	topSizer.Add( &eeBox, SizerFlags::StdSpace() ); //.Expand() );
	topSizer.Add( &iopSizer, SizerFlags::StdSpace() ); //.Expand() );

	mainsizer.Add( &topSizer ); //, wxSizerFlags().Expand() );		// topsizer has it's own padding.
	mainsizer.Add( &miscSizer, SizerFlags::StdSpace() ); //.Expand() );

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
