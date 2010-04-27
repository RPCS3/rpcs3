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
#include "App.h"
#include "MainFrame.h"
#include "IsoDropTarget.h"

#include "Dialogs/ModalPopups.h"

#include "CDVD/IsoFileFormats.h"

#include <wx/wfstream.h>


wxString GetMsg_ConfirmSysReset()
{
	return pxE( ".Popup:ConfirmSysReset",
		L"This action will reset the existing PS2 virtual machine state; "
		L"all current progress will be lost.  Are you sure?"
	);
}

bool IsoDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
	ScopedCoreThreadPopup stopped_core;

	if( filenames.GetCount() > 1 )
	{
		wxDialogWithHelpers dialog( m_WindowBound, _("Drag and Drop Error"), wxVERTICAL );
		dialog += dialog.Heading( _("It is an error to drop multiple files onto a PCSX2 window.  One at a time please, thank you.") );
		pxIssueConfirmation( dialog, MsgButtons().Cancel() );
		return false;
	}

	Console.WriteLn( L"(Drag&Drop) Received filename: " + filenames[0] );

	// ---------------
	//    ELF CHECK
	// ---------------
	{
	wxFileInputStream filechk( filenames[0] );

	if( !filechk.IsOk() )
		throw Exception::CannotCreateStream( filenames[0] );

	u8 ident[16];
	filechk.Read( ident, 16 );
	static const u8 elfIdent[4] = { 0x7f, 'E', 'L', 'F' };

	if( ((u32&)ident) == ((u32&)elfIdent) )
	{
		Console.WriteLn( L"(Drag&Drop) Found ELF file type!" );

		g_Conf->CurrentELF = filenames[0];

		bool confirmed = true;
		if( SysHasValidState() )
		{
			wxDialogWithHelpers dialog( m_WindowBound, _("Confirm PS2 Reset"), wxVERTICAL );

			dialog += dialog.Heading(
				_("You have dropped the following ELF binary into PCSX2:\n\n") +
				filenames[0] + L"\n\n" + GetMsg_ConfirmSysReset()
			);

			confirmed = (pxIssueConfirmation( dialog, MsgButtons().Reset().Cancel(), L"DragDrop:BootELF" ) != wxID_CANCEL);
		}

		if( confirmed )
		{
			sApp.SysExecute( g_Conf->CdvdSource, g_Conf->CurrentELF );
		}
		else
			stopped_core.AllowResume();

		return true;
	}
	}

	// ---------------
	//    ISO CHECK
	// ---------------

	// FIXME : The whole IsoFileFormats api (meaning isoOpen / isoDetect / etc) needs to be
	//   converted to C++ and wxInputStream .  Until then this is a nasty little exception unsafe
	//   hack ;)

	isoFile iso;
	memzero(iso);
	iso.handle = _openfile(filenames[0].ToUTF8(), O_RDONLY);

	if( iso.handle == NULL )
		throw Exception::CannotCreateStream( filenames[0] );

	if (isoDetect(&iso))
	{
		Console.WriteLn( L"(Drag&Drop) Found valid ISO file type!" );
		SwapOrReset_Iso(m_WindowBound, stopped_core, filenames[0], _("You have dropped the following ISO image into PCSX2:\n\n"));
	}

	_closefile( iso.handle );
	return true;
}
