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
#include "HostGui.h"
#include "CDVD/CDVD.h"

#include "MainFrame.h"
#include "Dialogs/ModalPopups.h"
#include "Dialogs/ConfigurationDialog.h"
#include "Dialogs/LogOptionsDialog.h"

using namespace Dialogs;


void MainEmuFrame::Menu_ConfigSettings_Click(wxCommandEvent &event)
{
	if( Dialogs::ConfigurationDialog( this ).ShowModal() )
	{
		g_Conf->Save();
	}
}

void MainEmuFrame::Menu_SelectBios_Click(wxCommandEvent &event)
{
	if( Dialogs::BiosSelectorDialog( this ).ShowModal() )
	{
		g_Conf->Save();
	}
}

static const wxChar* isoFilterTypes =
	L"Iso Image (*.iso)|*.iso|Bin Image|(*.bin)|MDF Image (*.mdf)|*.mdf|Nero Image (*.nrg)|*.nrg";

void MainEmuFrame::Menu_RunIso_Click(wxCommandEvent &event)
{
	SysSuspend();

	Console::Status( L"Default Folder: " + g_Conf->Folders.RunIso.ToString() );
	wxFileDialog ctrl( this, _("Run PS2 Iso..."), g_Conf->Folders.RunIso.ToString(), wxEmptyString,
		isoFilterTypes, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

	if( ctrl.ShowModal() == wxID_CANCEL )
	{
		SysResume();
		return;
	}
	
	SysEndExecution();

	g_Conf->Folders.RunIso = ctrl.GetPath();
	g_Conf->Save();

	wxString elf_file;
	if( EmuConfig.SkipBiosSplash )
	{
		// Fetch the ELF filename and CD type from the CDVD provider.
		wxString ename( ctrl.GetFilename() );
		int result = GetPS2ElfName( ename );
		switch( result )
		{
			case 0:
				Msgbox::Alert( _("Boot failed: CDVD image is not a PS1 or PS2 game.") );
			return;

			case 1:
				Msgbox::Alert( _("Boot failed: PCSX2 does not support emulation of PS1 games.") );
			return;

			case 2:
				// PS2 game.  Valid!
				elf_file = ename;
			break;
		}
	}

	InitPlugins();
	SysExecute( new AppEmuThread( elf_file ), CDVDsrc_Iso );
}

void MainEmuFrame::Menu_RunWithoutDisc_Click(wxCommandEvent &event)
{
	if( EmulationInProgress() )
	{
		SysSuspend();

		// [TODO] : Add one of 'dems checkboxes that read like "[x] don't show this stupid shit again, kthx."
		bool result = Msgbox::OkCancel( pxE( ".Popup:ConfirmEmuReset", L"This will reset the emulator and your current emulation session will be lost.  Are you sure?") );

		if( !result )
		{
			SysResume();
			return;
		}
	}

	SysEndExecution();
	InitPlugins();
	SysExecute( new AppEmuThread(), CDVDsrc_NoDisc );
}

void MainEmuFrame::Menu_IsoRecent_Click(wxCommandEvent &event)
{
	//Console::Status( "%d", params event.GetId() - g_RecentIsoList->GetBaseId() );
	//Console::WriteLn( Color_Magenta, g_RecentIsoList->GetHistoryFile( event.GetId() - g_RecentIsoList->GetBaseId() ) );
}

void MainEmuFrame::Menu_OpenELF_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_LoadStateOther_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_SaveStateOther_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_Exit_Click(wxCommandEvent &event)
{
	Close();
}

void MainEmuFrame::Menu_EmuClose_Click(wxCommandEvent &event)
{
	SysReset();
	GetMenuBar()->Check( MenuId_Emu_Pause, false );
}

void MainEmuFrame::Menu_EmuPause_Click(wxCommandEvent &event)
{
	if( event.IsChecked() )
		SysSuspend();
	else
		SysResume();
}

void MainEmuFrame::Menu_EmuReset_Click(wxCommandEvent &event)
{
	bool wasRunning = EmulationInProgress();
	SysReset();

	GetMenuBar()->Check( MenuId_Emu_Pause, false );

	if( !wasRunning ) return;
	InitPlugins();
	SysExecute( new AppEmuThread() );
}

void MainEmuFrame::Menu_Debug_Open_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_Debug_MemoryDump_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_Debug_Logging_Click(wxCommandEvent &event)
{
	LogOptionsDialog( this, wxID_ANY ).ShowModal();
}

void MainEmuFrame::Menu_ShowConsole(wxCommandEvent &event)
{
	// Use messages to relay open/close commands (thread-safe)

	g_Conf->ProgLogBox.Visible = event.IsChecked();
	wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED, g_Conf->ProgLogBox.Visible ? wxID_OPEN : wxID_CLOSE );
	wxGetApp().ProgramLog_PostEvent( evt );
}

void MainEmuFrame::Menu_ShowAboutBox(wxCommandEvent &event)
{
	AboutBoxDialog( this, wxID_ANY ).ShowModal();
}
