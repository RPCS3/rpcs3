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
#include "System.h"
#include "App.h"

#include "ConfigurationDialog.h"
#include "ModalPopups.h"
#include "Panels/ConfigurationPanels.h"

#include <wx/filepicker.h>

using namespace Panels;

// ----------------------------------------------------------------------------
Dialogs::BiosSelectorDialog::BiosSelectorDialog( wxWindow* parent )
	: BaseApplicableDialog( parent, _("BIOS Selector"), wxVERTICAL )
{
	SetName( GetNameStatic() );

	m_idealWidth = 500;

	Panels::BaseSelectorPanel* selpan = new Panels::BiosSelectorPanel( this );

	*this += selpan		| pxSizerFlags::StdExpand();
	AddOkCancel( *GetSizer(), false );

	Connect( wxID_OK,		wxEVT_COMMAND_BUTTON_CLICKED,			wxCommandEventHandler(BiosSelectorDialog::OnOk_Click) );
	Connect(				wxEVT_COMMAND_LISTBOX_DOUBLECLICKED,	wxCommandEventHandler(BiosSelectorDialog::OnDoubleClicked) );

	selpan->OnShown();
}

void Dialogs::BiosSelectorDialog::OnOk_Click( wxCommandEvent& evt )
{
	if( m_ApplyState.ApplyAll() )
	{
		Close();
		evt.Skip();
	}
}

void Dialogs::BiosSelectorDialog::OnDoubleClicked( wxCommandEvent& evt )
{
	wxWindow* forwardButton = FindWindow( wxID_OK );
	if( forwardButton == NULL ) return;

	wxCommandEvent nextpg( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK );
	nextpg.SetEventObject( forwardButton );
	forwardButton->GetEventHandler()->ProcessEvent( nextpg );
}
