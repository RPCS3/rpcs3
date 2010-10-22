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

// --------------------------------------------------------------------------------------
//  DroppedTooManyFiles
// --------------------------------------------------------------------------------------
class DroppedTooManyFiles : public pxActionEvent
{
protected:
	wxWindowID	m_ownerid;

public:
	DroppedTooManyFiles( const wxWindow* window )
		: pxActionEvent()
	{
		m_ownerid = window->GetId();
	}

	virtual ~DroppedTooManyFiles() throw() { }
	virtual DroppedTooManyFiles *Clone() const { return new DroppedTooManyFiles(*this); }

protected:
	virtual void InvokeEvent()
	{
		ScopedCoreThreadPopup stopped_core;

		wxDialogWithHelpers dialog( wxWindow::FindWindowById(m_ownerid), _("Drag and Drop Error") );
		dialog += dialog.Heading(AddAppName(_("It is an error to drop multiple files onto a %s window.  One at a time please, thank you.")));
		pxIssueConfirmation( dialog, MsgButtons().Cancel() );
	}
};

// --------------------------------------------------------------------------------------
//  DroppedElf
// --------------------------------------------------------------------------------------
class DroppedElf : public pxActionEvent
{
protected:
	wxWindowID	m_ownerid;

public:
	DroppedElf( const wxWindow* window )
		: pxActionEvent()
	{
		m_ownerid = window->GetId();
	}

	virtual ~DroppedElf() throw() { }
	virtual DroppedElf *Clone() const { return new DroppedElf(*this); }

protected:
	virtual void InvokeEvent()
	{
		ScopedCoreThreadPopup stopped_core;

		bool confirmed = true;
		if( SysHasValidState() )
		{
			wxDialogWithHelpers dialog( wxWindow::FindWindowById(m_ownerid), _("Confirm PS2 Reset") );

			dialog += dialog.Heading(AddAppName(_("You have dropped the following ELF binary into %s:\n\n")));
			dialog += dialog.GetCharHeight();
			dialog += dialog.Text( g_Conf->CurrentELF );
			dialog += dialog.GetCharHeight();
			dialog += dialog.Heading(GetMsg_ConfirmSysReset());

			confirmed = (pxIssueConfirmation( dialog, MsgButtons().Reset().Cancel(), L"DragDrop.BootELF" ) != wxID_CANCEL);
		}

		if( confirmed )
		{
			g_Conf->EmuOptions.UseBOOT2Injection = true;
			sApp.SysExecute( g_Conf->CdvdSource, g_Conf->CurrentELF );
		}
		else
			stopped_core.AllowResume();
	}
};

// --------------------------------------------------------------------------------------
//  DroppedIso
// --------------------------------------------------------------------------------------
class DroppedIso : public pxActionEvent
{
protected:
	wxWindowID	m_ownerid;
	wxString	m_filename;

public:
	DroppedIso( const wxWindow* window, const wxString& filename )
		: pxActionEvent()
		, m_filename( filename )
	{
		m_ownerid = window->GetId();
	}

	virtual ~DroppedIso() throw() { }
	virtual DroppedIso *Clone() const { return new DroppedIso(*this); }

protected:
	virtual void InvokeEvent()
	{
		ScopedCoreThreadPopup stopped_core;
		SwapOrReset_Iso(wxWindow::FindWindowById(m_ownerid), stopped_core, m_filename,
			AddAppName(_("You have dropped the following ISO image into %s:"))
		);
	}
};

bool IsoDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
	// WARNING: Doing *anything* from the context of OnDropFiles will result in Windows
	// Explorer getting tied up waiting for a response from the application's message pump.
	// So whenever possible, issue messages from this function only, such that the
	// messages are processed later on after the application has allowed Explorer to resume
	// itself.
	//
	// This most likely *includes* throwing exceptions, hence all exceptions here-in being
	// caught and re-packaged as messages posted back to the application, so that the drag&
	// drop procedure is free to release the Windows Explorer from the tyranny of drag&drop
	// interplay.

	try
	{
		if( filenames.GetCount() > 1 )
		{
			wxGetApp().AddIdleEvent( DroppedTooManyFiles(m_WindowBound) );
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

			wxGetApp().PostEvent( DroppedElf(m_WindowBound) );
			return true;
		}
		}

		// ---------------
		//    ISO CHECK
		// ---------------

		isoFile iso;

		if (iso.Test( filenames[0] ))
		{
			DevCon.WriteLn( L"(Drag&Drop) Found valid ISO file type!" );
			wxGetApp().PostEvent( DroppedIso(m_WindowBound, filenames[0]) );
			return true;
		}
	}
	catch (BaseException& ex)
	{
		wxGetApp().AddIdleEvent( pxExceptionEvent(ex) );
	}
	catch (std::runtime_error& ex)
	{
		wxGetApp().AddIdleEvent( pxExceptionEvent(Exception::RuntimeError(ex)) );
	}
	return false;
}
