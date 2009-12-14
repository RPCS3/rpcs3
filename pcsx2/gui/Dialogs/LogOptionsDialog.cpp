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
#include "LogOptionsDialog.h"
#include "Panels/LogOptionsPanels.h"

#include <wx/statline.h>

using namespace Panels;

Dialogs::LogOptionsDialog::LogOptionsDialog( wxWindow* parent )
	: wxDialogWithHelpers( parent, _("Trace Logging"), true )
{
	SetName( GetNameStatic() );

	m_idealWidth = 480;

	wxBoxSizer& mainsizer = *new wxBoxSizer( wxVERTICAL );
	mainsizer.Add( new LogOptionsPanel( this ) );

	AddOkCancel( mainsizer, true );
	FindWindow( wxID_APPLY )->Disable();

	SetSizerAndFit( &mainsizer, true );

	Connect( wxID_OK,		wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LogOptionsDialog::OnOk_Click ) );
	Connect( wxID_APPLY,	wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LogOptionsDialog::OnApply_Click ) );
}

void Dialogs::LogOptionsDialog::OnOk_Click( wxCommandEvent& evt )
{
	if( g_ApplyState.ApplyAll() )
	{
		FindWindow( wxID_APPLY )->Disable();
		AppSaveSettings();

		Close();
		evt.Skip();
	}
}

void Dialogs::LogOptionsDialog::OnApply_Click( wxCommandEvent& evt )
{
	if( g_ApplyState.ApplyAll() )
		FindWindow( wxID_APPLY )->Disable();

	AppSaveSettings();
}
