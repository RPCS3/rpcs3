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

// Returns FALSE if the user cancelled the action.
bool MainEmuFrame::_DoSelectIsoBrowser()
{
	static const wxChar* isoFilterTypes =
		L"All Supported (.iso .mdf .nrg .bin .img .dump)|*.iso;*.mdf;*.nrg;*.bin;*.img;*.dump|"
		L"Disc Images (.iso .mdf .nrg .bin .img)|*.iso;*.mdf;*.nrg;*.bin;*.img|"
		L"Blockdumps (.dump)|*.dump|"
		L"All Files (*.*)|*.*";

	wxFileDialog ctrl( this, _("Select CDVD source iso..."), g_Conf->Folders.RunIso.ToString(), wxEmptyString,
		isoFilterTypes, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

	if( ctrl.ShowModal() != wxID_CANCEL )
	{
		g_Conf->Folders.RunIso = wxFileName( ctrl.GetPath() ).GetPath();
		g_Conf->CurrentIso = ctrl.GetPath();
		wxGetApp().SaveSettings();

		UpdateIsoSrcFile();
		return true;
	}
	
	return false;
}

void MainEmuFrame::Menu_BootCdvd_Click( wxCommandEvent &event )
{
	wxGetApp().SysSuspend();

	if( !wxFileExists( g_Conf->CurrentIso ) )
	{
		if( !_DoSelectIsoBrowser() )
		{
			wxGetApp().SysResume();
			return;
		}
	}

	if( EmulationInProgress() )
	{
		// [TODO] : Add one of 'dems checkboxes that read like "[x] don't show this stupid shit again, kthx."
		bool result = Msgbox::OkCancel( pxE( ".Popup:ConfirmEmuReset", L"This will reset the emulator and your current emulation session will be lost.  Are you sure?") );

		if( !result )
		{
			wxGetApp().SysResume();
			return;
		}
	}

	g_Conf->EmuOptions.SkipBiosSplash = GetMenuBar()->IsChecked( MenuId_SkipBiosToggle );
	wxGetApp().SaveSettings();

	wxGetApp().SysExecute( g_Conf->CdvdSource );
}

void MainEmuFrame::Menu_IsoBrowse_Click( wxCommandEvent &event )
{
	wxGetApp().SysSuspend();
	_DoSelectIsoBrowser();
	wxGetApp().SysResume();
}

void MainEmuFrame::Menu_RunIso_Click( wxCommandEvent &event )
{
	wxGetApp().SysSuspend();

	if( !_DoSelectIsoBrowser() )
	{
		wxGetApp().SysResume();
		return;
	}

	wxGetApp().SysExecute( CDVDsrc_Iso );
}

void MainEmuFrame::Menu_IsoRecent_Click(wxCommandEvent &event)
{
	//Console::Status( "%d", event.GetId() - g_RecentIsoList->GetBaseId() );
	//Console::WriteLn( Color_Magenta, g_RecentIsoList->GetHistoryFile( event.GetId() - g_RecentIsoList->GetBaseId() ) );
}

void MainEmuFrame::Menu_OpenELF_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_LoadStates_Click(wxCommandEvent &event)
{
   int id = event.GetId() - MenuId_State_Load01 - 1;
   Console::WriteLn("If this were hooked up, it would load slot %d.", id);
}

void MainEmuFrame::Menu_SaveStates_Click(wxCommandEvent &event)
{
   int id = event.GetId() - MenuId_State_Save01 - 1;
   Console::WriteLn("If this were hooked up, it would save slot %d.", id);
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
	//wxGetApp()->
	GetMenuBar()->Check( MenuId_Emu_Pause, false );
}

void MainEmuFrame::Menu_EmuPause_Click(wxCommandEvent &event)
{
	if( event.IsChecked() )
		wxGetApp().SysSuspend();
	else
		wxGetApp().SysResume();
}

void MainEmuFrame::Menu_EmuReset_Click(wxCommandEvent &event)
{
	bool wasRunning = EmulationInProgress();
	wxGetApp().SysReset();

	GetMenuBar()->Check( MenuId_Emu_Pause, false );

	if( !wasRunning ) return;
	wxGetApp().SysExecute();
}

void MainEmuFrame::Menu_ConfigPlugin_Click(wxCommandEvent &event)
{
	typedef void	(CALLBACK* PluginConfigureFnptr)();
	const int eventId = event.GetId();
	PluginsEnum_t pid;
	switch (eventId) {
		case MenuId_Video_Advanced:
			pid = PluginId_GS;
			break;
		case MenuId_Audio_Advanced:
			pid = PluginId_SPU2;
			break;
		case MenuId_Pad_Advanced:
			pid = PluginId_PAD;
			break;
		default:
			pid = (PluginsEnum_t)( event.GetId() - MenuId_Config_GS );
			break;
	}

	LoadPluginsImmediate();
	if( g_plugins == NULL ) return;

	wxWindowDisabler disabler;
	//ScopedWindowDisable disabler( this );
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
