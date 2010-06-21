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
#include "ModalPopups.h"

#include <wx/stdpaths.h>

using namespace Panels;

Dialogs::PickUserModeDialog::PickUserModeDialog( wxWindow* parent )
	: BaseApplicableDialog( parent, _("PCSX2 First Time configuration") )
{
	m_panel_usersel = new DocsFolderPickerPanel( this, false );
	m_panel_langsel = new LanguageSelectionPanel( this );

	*this	+= Heading(AddAppName(L"%s is starting from a new or unknown folder and needs to be configured."));
	*this	+= m_panel_langsel	| pxSizerFlags::StdCenter();
	*this	+= m_panel_usersel	| wxSizerFlags().Expand().Border( wxALL, 8 );

	AddOkCancel( *GetSizer() );

	Connect( wxID_OK,		wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PickUserModeDialog::OnOk_Click ) );
	// TODO : Add a command event handler for language changes, that dynamically re-update contents of this window.
}

void Dialogs::PickUserModeDialog::OnOk_Click( wxCommandEvent& evt )
{
	if( m_ApplyState.ApplyAll() )
	{
		Close();
		evt.Skip();
	}
}
