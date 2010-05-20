/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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
using namespace pxSizerFlags;

// ----------------------------------------------------------------------------
Dialogs::BiosSelectorDialog::BiosSelectorDialog( wxWindow* parent )
	: BaseApplicableDialog( parent, _("BIOS Selector"), wxVERTICAL )
{
	m_idealWidth = 500;

	m_selpan = new Panels::BiosSelectorPanel( this );

	*this += m_selpan		| StdExpand();
	AddOkCancel();

	Connect( wxID_OK,		wxEVT_COMMAND_BUTTON_CLICKED,			wxCommandEventHandler(BiosSelectorDialog::OnOk_Click) );
	Connect(				wxEVT_COMMAND_LISTBOX_DOUBLECLICKED,	wxCommandEventHandler(BiosSelectorDialog::OnDoubleClicked) );
}

bool Dialogs::BiosSelectorDialog::Show( bool show )
{
	if( show && m_selpan )
		m_selpan->OnShown();

	return _parent::Show( show );
}

int Dialogs::BiosSelectorDialog::ShowModal()
{
	if( m_selpan )
		m_selpan->OnShown();
		
	return _parent::ShowModal();
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
