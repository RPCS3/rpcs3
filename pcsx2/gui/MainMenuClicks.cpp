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
#include "GS.h"

#include "MainFrame.h"
#include "Dialogs/ModalPopups.h"
#include "Dialogs/ConfigurationDialog.h"
#include "Dialogs/LogOptionsDialog.h"

#include "IniInterface.h"

using namespace Dialogs;

extern wxString GetMsg_ConfirmSysReset();

void MainEmuFrame::SaveEmuOptions()
{
    if (wxConfigBase* conf = GetAppConfig())
	{
		IniSaver saver(*conf);
		g_Conf->EmuOptions.LoadSave(saver);
	}
}

void MainEmuFrame::Menu_ConfigSettings_Click(wxCommandEvent &event)
{
	Dialogs::ConfigurationDialog( this ).ShowModal();
}

void MainEmuFrame::Menu_SelectBios_Click(wxCommandEvent &event)
{
	Dialogs::BiosSelectorDialog( this ).ShowModal();
}

void MainEmuFrame::Menu_CdvdSource_Click( wxCommandEvent &event )
{
	CDVD_SourceType newSource = CDVDsrc_NoDisc;

	switch( event.GetId() )
	{
		case MenuId_Src_Iso:	newSource = CDVDsrc_Iso;	break;
		case MenuId_Src_Plugin:	newSource = CDVDsrc_Plugin;	break;
		case MenuId_Src_NoDisc: newSource = CDVDsrc_NoDisc;	break;

		jNO_DEFAULT
	}
	CoreThread.ChangeCdvdSource( newSource );
}

// Returns FALSE if the user cancelled the action.
bool MainEmuFrame::_DoSelectIsoBrowser( wxString& result )
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
		result = ctrl.GetPath();
		g_Conf->Folders.RunIso = wxFileName( result ).GetPath();
		return true;
	}

	return false;
}

bool MainEmuFrame::_DoSelectELFBrowser()
{
	static const wxChar* elfFilterTypes =
		L"ELF Files (.elf)|*.elf|"
		L"All Files (*.*)|*.*";

	wxFileDialog ctrl( this, _("Select ELF file..."), g_Conf->Folders.RunELF.ToString(), wxEmptyString,
		elfFilterTypes, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

	if( ctrl.ShowModal() != wxID_CANCEL )
	{
		g_Conf->Folders.RunELF = wxFileName( ctrl.GetPath() ).GetPath();
		g_Conf->CurrentELF = ctrl.GetPath();
		return true;
	}

	return false;
}

void MainEmuFrame::Menu_BootCdvd_Click( wxCommandEvent &event )
{
	ScopedCoreThreadSuspend core;

	if( g_Conf->CdvdSource == CDVDsrc_Iso )
	{
		bool selector = g_Conf->CurrentIso.IsEmpty();

		if( !selector && !wxFileExists(g_Conf->CurrentIso) )
		{
			// User has an iso selected from a previous run, but it doesn't exist anymore.
			// Issue a courtesy popup and then an Iso Selector to choose a new one.
			
			Dialogs::ExtensibleConfirmation( this, ConfButtons().OK(), _("ISO file not found!"),
				_("An error occurred while trying to open the file:\n\n") + g_Conf->CurrentIso + L"\n\n" + 
				_("Error: The configured ISO file does not exist.  Click OK to select a new ISO source for CDVD.")
			).ShowModal();
			
			selector = true;
		}
		
		if( selector )
		{
			wxString result;
			if( !_DoSelectIsoBrowser( result ) )
			{
				core.Resume();
				return;
			}

			SysUpdateIsoSrcFile( result );
		}
	}

	if( SysHasValidState() )
	{
		ExtensibleConfirmation dialog( this, ConfButtons().Yes().Cancel(),
			_("Confirm PS2 Reset"), GetMsg_ConfirmSysReset()
		);
		
		bool confirmed = (IssueConfirmation( dialog, L"BootCdvd:ConfirmReset" ) != wxID_CANCEL);

		if( !confirmed )
		{
			core.Resume();
			return;
		}
	}

	sApp.SysExecute( g_Conf->CdvdSource );
}

void MainEmuFrame::Menu_IsoBrowse_Click( wxCommandEvent &event )
{
	ScopedCoreThreadSuspend core;
	wxString result;

	if( _DoSelectIsoBrowser( result ) )
	{
		// This command does an on-the-fly change of CD media without automatic reset.
		// (useful for disc swapping)

		SysUpdateIsoSrcFile( result );
	}

	core.Resume();
}

void MainEmuFrame::Menu_MultitapToggle_Click( wxCommandEvent &event )
{
	g_Conf->EmuOptions.MultitapPort0_Enabled = GetMenuBar()->IsChecked( MenuId_Config_Multitap0Toggle );
	g_Conf->EmuOptions.MultitapPort1_Enabled = GetMenuBar()->IsChecked( MenuId_Config_Multitap1Toggle );
	AppSaveSettings();
}

void MainEmuFrame::Menu_SkipBiosToggle_Click( wxCommandEvent &event )
{
	g_Conf->EmuOptions.SkipBiosSplash = GetMenuBar()->IsChecked( MenuId_SkipBiosToggle );
	SaveEmuOptions();
}

void MainEmuFrame::Menu_EnablePatches_Click( wxCommandEvent &event )
{
	g_Conf->EmuOptions.EnablePatches = GetMenuBar()->IsChecked( MenuId_EnablePatches );
    SaveEmuOptions();
}

void MainEmuFrame::Menu_OpenELF_Click(wxCommandEvent &event)
{
	bool resume = CoreThread.Suspend();
	if( _DoSelectELFBrowser() )
	{
		sApp.SysExecute( g_Conf->CdvdSource, g_Conf->CurrentELF );
	}

	if( resume ) CoreThread.Resume();
}

void MainEmuFrame::Menu_LoadStates_Click(wxCommandEvent &event)
{
	States_SetCurrentSlot( event.GetId() - MenuId_State_Load01 - 1 );
	States_DefrostCurrentSlot();
}

void MainEmuFrame::Menu_SaveStates_Click(wxCommandEvent &event)
{
	States_SetCurrentSlot( event.GetId() - MenuId_State_Save01 - 1 );
	States_FreezeCurrentSlot();
}

void MainEmuFrame::Menu_LoadStateOther_Click(wxCommandEvent &event)
{
   Console.WriteLn("If this were hooked up, it would load a savestate file.");
}

void MainEmuFrame::Menu_SaveStateOther_Click(wxCommandEvent &event)
{
   Console.WriteLn("If this were hooked up, it would save a savestate file.");
}

void MainEmuFrame::Menu_Exit_Click(wxCommandEvent &event)
{
	Close();
}

void MainEmuFrame::Menu_SuspendResume_Click(wxCommandEvent &event)
{
	if( !SysHasValidState() ) return;

	if( !CoreThread.Suspend() )
	{
		sApp.SysExecute();
	}

	// Disable the menu item.  The state of the menu is indeterminate until the core thread
	// has responded (it updates status after the plugins are loaded and emulation has
	// engaged successfully).

	GetMenuBar()->Enable( MenuId_Sys_SuspendResume, false );
}

void MainEmuFrame::Menu_SysReset_Click(wxCommandEvent &event)
{
	if( !SysHasValidState() ) return;
	bool resume = CoreThread.Suspend();

	sApp.SysReset();

	if( resume )
	{
		sApp.SysExecute();
	}

	GetMenuBar()->Enable( MenuId_Sys_Reset, resume );
}

void MainEmuFrame::Menu_ConfigPlugin_Click(wxCommandEvent &event)
{
	const int eventId = event.GetId() - MenuId_PluginBase_Settings;

	PluginsEnum_t pid = (PluginsEnum_t)(eventId / PluginMenuId_Interval);
	if( !pxAssertDev( (eventId >= 0) || (pid < PluginId_Count), "Invalid plugin identifier passed to ConfigPlugin event handler." ) ) return;

	LoadPluginsImmediate();
	if( g_plugins == NULL ) return;

	wxWindowDisabler disabler;
	SaveSinglePluginHelper helper( pid );
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

void MainEmuFrame::Menu_ShowConsole_Stdio(wxCommandEvent &event)
{
	g_Conf->EmuOptions.ConsoleToStdio = GetMenuBar()->IsChecked( MenuId_Console_Stdio );
	SaveEmuOptions();
}

void MainEmuFrame::Menu_PrintCDVD_Info(wxCommandEvent &event)
{
	g_Conf->EmuOptions.CdvdVerboseReads = GetMenuBar()->IsChecked( MenuId_CDVD_Info );
	SaveEmuOptions();
}

void MainEmuFrame::Menu_ShowAboutBox(wxCommandEvent &event)
{
	AboutBoxDialog( this, wxID_ANY ).ShowModal();
}
