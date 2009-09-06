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
		wxGetApp().SaveSettings();
	}
}

void MainEmuFrame::Menu_SelectBios_Click(wxCommandEvent &event)
{
	if( Dialogs::BiosSelectorDialog( this ).ShowModal() )
	{
		wxGetApp().SaveSettings();
	}
}

void MainEmuFrame::Menu_CdvdSource_Click( wxCommandEvent &event )
{
	switch( event.GetId() )
	{
		case MenuId_Src_Iso:	g_Conf->CdvdSource = CDVDsrc_Iso;		break;
		case MenuId_Src_Plugin:	g_Conf->CdvdSource = CDVDsrc_Plugin;	break;
		case MenuId_Src_NoDisc: g_Conf->CdvdSource = CDVDsrc_NoDisc;	break;

		jNO_DEFAULT
	}
	UpdateIsoSrcSelection();
	wxGetApp().SaveSettings();
}

void MainEmuFrame::Menu_BootCdvd_Click( wxCommandEvent &event )
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

	CDVDsys_SetFile( CDVDsrc_Iso, g_Conf->CurrentIso );
	SysExecute( new AppEmuThread(), g_Conf->CdvdSource );
}

static const wxChar* isoFilterTypes =
	L"All Supported (.iso .mdf .nrg .bin .img .dump)|*.iso;*.mdf;*.nrg;*.bin;*.img;*.dump|"
	L"Disc Images (.iso .mdf .nrg .bin .img)|*.iso;*.mdf;*.nrg;*.bin;*.img|"
	L"Blockdumps (.dump)|*.dump|"
	L"All Files (*.*)|*.*";


void MainEmuFrame::Menu_IsoBrowse_Click( wxCommandEvent &event )
{
	SysSuspend();

	wxFileDialog ctrl( this, _("Select CDVD source iso..."), g_Conf->Folders.RunIso.ToString(), wxEmptyString,
		isoFilterTypes, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

	if( ctrl.ShowModal() != wxID_CANCEL )
	{
		g_Conf->Folders.RunIso = wxFileName( ctrl.GetPath() ).GetPath();
		g_Conf->CurrentIso = ctrl.GetPath();
		wxGetApp().SaveSettings();

		UpdateIsoSrcFile();
	}

	SysResume();
}

void MainEmuFrame::Menu_RunIso_Click( wxCommandEvent &event )
{
	SysSuspend();

	wxFileDialog ctrl( this, _("Run PS2 Iso..."), g_Conf->Folders.RunIso.ToString(), wxEmptyString,
		isoFilterTypes, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

	if( ctrl.ShowModal() == wxID_CANCEL )
	{
		SysResume();
		return;
	}

	SysEndExecution();

	g_Conf->Folders.RunIso = wxFileName( ctrl.GetPath() ).GetPath();
	g_Conf->CurrentIso = ctrl.GetPath();
	wxGetApp().SaveSettings();

	UpdateIsoSrcFile();

	wxString elf_file;
	if( EmuConfig.SkipBiosSplash )
	{
		// Fetch the ELF filename and CD type from the CDVD provider.
		wxString ename( g_Conf->CurrentIso );
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

			jNO_DEFAULT
		}
	}

	InitPlugins();
	SysExecute( new AppEmuThread( elf_file ), CDVDsrc_Iso );
}

/*void MainEmuFrame::Menu_RunWithoutDisc_Click(wxCommandEvent &event)
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
}*/

void MainEmuFrame::Menu_IsoRecent_Click(wxCommandEvent &event)
{
	//Console::Status( "%d", params event.GetId() - g_RecentIsoList->GetBaseId() );
	//Console::WriteLn( Color_Magenta, g_RecentIsoList->GetHistoryFile( event.GetId() - g_RecentIsoList->GetBaseId() ) );
}

void MainEmuFrame::Menu_OpenELF_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_LoadStates_Click(wxCommandEvent &event)
{
   int id = event.GetId() - MenuId_State_Load01 - 1;
   Console::WriteLn("If this were hooked up, it would load slot %d.", params id);
}

void MainEmuFrame::Menu_SaveStates_Click(wxCommandEvent &event)
{
   int id = event.GetId() - MenuId_State_Save01 - 1;
   Console::WriteLn("If this were hooked up, it would save slot %d.", params id);
}

void MainEmuFrame::Menu_LoadStateOther_Click(wxCommandEvent &event)
{
   Console::WriteLn("If this were hooked up, it would load a savestate file.");
}

void MainEmuFrame::Menu_SaveStateOther_Click(wxCommandEvent &event)
{
   Console::WriteLn("If this were hooked up, it would save a savestate file.");
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

void MainEmuFrame::Menu_ConfigPlugin_Click(wxCommandEvent &event)
{
	typedef void	(CALLBACK* PluginConfigureFnptr)();
	const PluginsEnum_t pid = (PluginsEnum_t)( event.GetId() - MenuId_Config_GS );

	LoadPlugins();
	ScopedWindowDisable disabler( this );
	g_plugins->Configure( pid );
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
