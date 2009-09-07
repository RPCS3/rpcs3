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
#include "System.h"

#include "ModalPopups.h"

using namespace wxHelpers;


Dialogs::ImportSettingsDialog::ImportSettingsDialog( wxWindow* parent ) :
	wxDialogWithHelpers( parent, wxID_ANY, L"Import Existing Settings?", false )
{
	wxBoxSizer& sizer( *new wxBoxSizer( wxVERTICAL ) );

	AddStaticText( sizer,
		pxE( ".Popup:ImportExistingSettings",
			L"Existing PCSX2 settings have been found in the configured settings folder.  "
			L"Would you like to import these settings or overwrite them with PCSX2 default values?"
			L"\n\n(or press Cancel to select a different settings folder)"
		),
		wxALIGN_CENTRE, 400
	);
	
	wxBoxSizer& s_buttons = *new wxBoxSizer( wxHORIZONTAL );
	wxButton* b_import	= new wxButton( this, wxID_ANY, _("Import") );
	wxButton* b_over	= new wxButton( this, wxID_ANY, _("Overwrite") );

	s_buttons.Add( b_import,SizerFlags::StdButton() );
	s_buttons.AddSpacer( 16 );
	s_buttons.Add( b_over,	SizerFlags::StdButton() );
	s_buttons.AddSpacer( 16 );
	s_buttons.Add( new wxButton( this, wxID_CANCEL ), SizerFlags::StdButton() );

	sizer.AddSpacer( 12 );
	sizer.Add( &s_buttons, SizerFlags::StdCenter() );
	SetSizerAndFit( &sizer );

	Connect( b_import->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ImportSettingsDialog::OnImport_Click) );
	Connect( b_over->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ImportSettingsDialog::OnOverwrite_Click) );
}

void Dialogs::ImportSettingsDialog::OnImport_Click( __unused wxCommandEvent& evt )
{
	AppConfig_ReloadGlobalSettings( false );	// ... and import existing settings
	g_Conf->Folders.Bios.Mkdir();
	EndModal( wxID_OK );
}

void Dialogs::ImportSettingsDialog::OnOverwrite_Click( __unused wxCommandEvent& evt )
{
	AppConfig_ReloadGlobalSettings( true );		// ... and overwrite any existing settings
	g_Conf->Folders.Bios.Mkdir();
	EndModal( wxID_OK );
}
