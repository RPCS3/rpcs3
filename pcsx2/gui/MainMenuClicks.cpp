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
#include "HostGui.h"

#include "CDVD/CDVD.h"
#include "GS.h"

#include "MainFrame.h"
#include "Dialogs/ModalPopups.h"
#include "Dialogs/ConfigurationDialog.h"
#include "Dialogs/LogOptionsDialog.h"

#include "IniInterface.h"
#include "IsoDropTarget.h"

using namespace Dialogs;

void MainEmuFrame::SaveEmuOptions()
{
    if (wxConfigBase* conf = GetAppConfig())
	{
		IniSaver saver(*conf);
		g_Conf->EmuOptions.LoadSave(saver);
	}
}

void MainEmuFrame::Menu_SysSettings_Click(wxCommandEvent &event)
{
	AppOpenDialog<SysConfigDialog>( this );
}

void MainEmuFrame::Menu_McdSettings_Click(wxCommandEvent &event)
{
	AppOpenDialog<McdConfigDialog>( this );
}

void MainEmuFrame::Menu_AppSettings_Click(wxCommandEvent &event)
{
	AppOpenDialog<AppConfigDialog>( this );
}

void MainEmuFrame::Menu_SelectBios_Click(wxCommandEvent &event)
{
	AppOpenDialog<BiosSelectorDialog>( this );
}


static void WipeSettings()
{
	wxGetApp().CleanupRestartable();
	wxGetApp().CleanupResources();

	wxRemoveFile( GetSettingsFilename() );

	// FIXME: wxRmdir doesn't seem to work here for some reason (possible file sharing issue
	// with a plugin that leaves a file handle dangling maybe?).  But deleting the inis folder
	// manually from explorer does work.  Can't think of a good work-around at the moment. --air

	//wxRmdir( GetSettingsFolder().ToString() );

	wxGetApp().GetRecentIsoManager().Clear();
	g_Conf = new AppConfig();
	sMainFrame.RemoveCdvdMenu();
}

void MainEmuFrame::RemoveCdvdMenu()
{
	if( wxMenuItem* item = m_menuCDVD.FindItem(MenuId_IsoSelector) )
		m_menuCDVD.Remove( item );
}

void MainEmuFrame::Menu_ResetAllSettings_Click(wxCommandEvent &event)
{
	if( IsBeingDeleted() || m_RestartEmuOnDelete ) return;

	{
		ScopedCoreThreadPopup suspender;
		if( !Msgbox::OkCancel(
			pxE( ".Popup Warning:DeleteSettings",
				L"This command clears PCSX2 settings and allows you to re-run the First-Time Wizard.  You will need to "
				L"manually restart PCSX2 after this operation.\n\n"
				L"WARNING!!  Click OK to delete *ALL* settings for PCSX2 and force PCSX2 to shudown, losing any current emulation progress.  Are you absolutely sure?"
				L"\n\n(note: settings for plugins are unaffected)"
			),
			_("Reset all settings?") ) )
		{
			suspender.AllowResume();
			return;
		}
	}

	WipeSettings();
	wxGetApp().PostMenuAction( MenuId_Exit );
}

// Return values:
//   wxID_CANCEL - User canceled the action outright.
//   wxID_RESET  - User wants to reset the emu in addition to swap discs
//   (anything else) - Standard swap, no reset.  (hotswap!)
wxWindowID SwapOrReset_Iso( wxWindow* owner, IScopedCoreThread& core_control, const wxString& isoFilename, const wxString& descpart1 )
{
	wxWindowID result = wxID_CANCEL;

	if( g_Conf->CdvdSource == CDVDsrc_Iso && isoFilename == g_Conf->CurrentIso )
	{
		core_control.AllowResume();
		return result;
	}

	if( SysHasValidState() )
	{
		core_control.DisallowResume();
		wxDialogWithHelpers dialog( owner, _("Confirm ISO image change"), wxVERTICAL );

		dialog += dialog.Heading(descpart1 +
			isoFilename + L"\n\n" +
			_("Do you want to swap discs or boot the new image (via system reset)?")
		);

		result = pxIssueConfirmation( dialog, MsgButtons().Reset().Cancel().Custom(_("Swap Disc")), L"DragDrop:BootSwapIso" );
		if( result == wxID_CANCEL )
		{
			core_control.AllowResume();
			return result;
		}
	}

	SysUpdateIsoSrcFile( isoFilename );
	if( result != wxID_RESET )
	{
		Console.Indent().WriteLn( "HotSwapping to new ISO src image!" );
		g_Conf->CdvdSource = CDVDsrc_Iso;
		CoreThread.ChangeCdvdSource();
		core_control.AllowResume();
	}
	else
	{
		core_control.DisallowResume();
		sApp.SysExecute( CDVDsrc_Iso );
	}

	return result;
}

wxWindowID SwapOrReset_CdvdSrc( wxWindow* owner, CDVD_SourceType newsrc )
{
	if(newsrc == g_Conf->CdvdSource) return wxID_CANCEL;
	wxWindowID result = wxID_CANCEL;
	ScopedCoreThreadPopup core;

	if( SysHasValidState() )
	{
		wxDialogWithHelpers dialog( owner, _("Confirm CDVD source change"), wxVERTICAL );

		wxString changeMsg;
		changeMsg.Printf(_("You've selected to switch the CDVD source from %s to %s."),
			CDVD_SourceLabels[g_Conf->CdvdSource], CDVD_SourceLabels[newsrc] );

		dialog += dialog.Heading(changeMsg + L"\n\n" +
			_("Do you want to swap discs or boot the new image (system reset)?")
		);

		result = pxIssueConfirmation( dialog, MsgButtons().Reset().Cancel().Custom(_("Swap Disc")), L"DragDrop:BootSwapIso" );

		if( result == wxID_CANCEL )
		{
			core.AllowResume();
			sMainFrame.UpdateIsoSrcSelection();
			return result;
		}
	}

	CDVD_SourceType oldsrc = g_Conf->CdvdSource;
	g_Conf->CdvdSource = newsrc;

	if( result != wxID_RESET )
	{
		Console.Indent().WriteLn( L"(CdvdSource) HotSwapping CDVD source types from %s to %s.", CDVD_SourceLabels[oldsrc], CDVD_SourceLabels[newsrc] );
		CoreThread.ChangeCdvdSource();
		sMainFrame.UpdateIsoSrcSelection();
		core.AllowResume();
	}
	else
	{
		core.DisallowResume();
		sApp.SysExecute( g_Conf->CdvdSource );
	}

	return result;
}

// Returns FALSE if the user canceled the action.
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

void MainEmuFrame::_DoBootCdvd()
{
	ScopedCoreThreadPause paused_core;

	if( g_Conf->CdvdSource == CDVDsrc_Iso )
	{
		bool selector = g_Conf->CurrentIso.IsEmpty();

		if( !selector && !wxFileExists(g_Conf->CurrentIso) )
		{
			// User has an iso selected from a previous run, but it doesn't exist anymore.
			// Issue a courtesy popup and then an Iso Selector to choose a new one.

			wxDialogWithHelpers dialog( this, _("ISO file not found!"), wxVERTICAL );
			dialog += dialog.Heading(
				_("An error occurred while trying to open the file:\n\n") + g_Conf->CurrentIso + L"\n\n" +
				_("Error: The configured ISO file does not exist.  Click OK to select a new ISO source for CDVD.")
			);

			pxIssueConfirmation( dialog, MsgButtons().OK() );

			selector = true;
		}

		if( selector )
		{
			wxString result;
			if( !_DoSelectIsoBrowser( result ) )
			{
				paused_core.AllowResume();
				return;
			}

			SysUpdateIsoSrcFile( result );
		}
	}

	if( SysHasValidState() )
	{
		wxDialogWithHelpers dialog( this, _("Confirm PS2 Reset"), wxVERTICAL );
		dialog += dialog.Heading( GetMsg_ConfirmSysReset() );
		bool confirmed = (pxIssueConfirmation( dialog, MsgButtons().Yes().Cancel(), L"BootCdvd:ConfirmReset" ) != wxID_CANCEL);

		if( !confirmed )
		{
			paused_core.AllowResume();
			return;
		}
	}

	sApp.SysExecute( g_Conf->CdvdSource );
}

void MainEmuFrame::Menu_CdvdSource_Click( wxCommandEvent &event )
{
	CDVD_SourceType newsrc = CDVDsrc_NoDisc;

	switch( event.GetId() )
	{
		case MenuId_Src_Iso:	newsrc = CDVDsrc_Iso;		break;
		case MenuId_Src_Plugin:	newsrc = CDVDsrc_Plugin;	break;
		case MenuId_Src_NoDisc: newsrc = CDVDsrc_NoDisc;	break;
		jNO_DEFAULT
	}

	SwapOrReset_CdvdSrc(this, newsrc);
}

void MainEmuFrame::Menu_BootCdvd_Click( wxCommandEvent &event )
{
	g_Conf->EmuOptions.UseBOOT2Injection = false;
	_DoBootCdvd();
}

void MainEmuFrame::Menu_BootCdvd2_Click( wxCommandEvent &event )
{
	g_Conf->EmuOptions.UseBOOT2Injection = true;
	_DoBootCdvd();
}

wxString GetMsg_IsoImageChanged()
{
	return _("You have selected the following ISO image into PCSX2:\n\n");
}

void MainEmuFrame::Menu_IsoBrowse_Click( wxCommandEvent &event )
{
	ScopedCoreThreadPopup core;
	wxString isofile;

	if( !_DoSelectIsoBrowser(isofile) )
	{
		core.AllowResume();
		return;
	}
	
	SwapOrReset_Iso(this, core, isofile, GetMsg_IsoImageChanged());
	AppSaveSettings();		// save the new iso selection; update menus!
}

void MainEmuFrame::Menu_MultitapToggle_Click( wxCommandEvent& )
{
	g_Conf->EmuOptions.MultitapPort0_Enabled = GetMenuBar()->IsChecked( MenuId_Config_Multitap0Toggle );
	g_Conf->EmuOptions.MultitapPort1_Enabled = GetMenuBar()->IsChecked( MenuId_Config_Multitap1Toggle );
	AppApplySettings();
	SaveEmuOptions();

	//evt.Skip();
}

void MainEmuFrame::Menu_EnablePatches_Click( wxCommandEvent& )
{
	g_Conf->EmuOptions.EnablePatches = GetMenuBar()->IsChecked( MenuId_EnablePatches );
    SaveEmuOptions();
}

void MainEmuFrame::Menu_OpenELF_Click(wxCommandEvent&)
{
	ScopedCoreThreadClose stopped_core;
	if( _DoSelectELFBrowser() )
	{
		g_Conf->EmuOptions.UseBOOT2Injection = true;
		sApp.SysExecute( g_Conf->CdvdSource, g_Conf->CurrentELF );
	}

	stopped_core.AllowResume();
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

class SysExecEvent_ToggleSuspend : public SysExecEvent
{
public:
	virtual ~SysExecEvent_ToggleSuspend() throw() {}

	wxString GetEventName() const { return L"ToggleSuspendResume"; }

protected:
	void InvokeEvent()
	{
		if( CoreThread.IsOpen() )
			CoreThread.Suspend();
		else
			CoreThread.Resume();
	}
};

class SysExecEvent_Restart : public SysExecEvent
{
public:
	virtual ~SysExecEvent_Restart() throw() {}

	wxString GetEventName() const { return L"SysRestart"; }

	wxString GetEventMessage() const
	{
		return _("Restarting PS2 Virtual Machine...");
	}

protected:
	void InvokeEvent()
	{
		sApp.SysShutdown();
		sApp.SysExecute();
	}
};

void MainEmuFrame::Menu_SuspendResume_Click(wxCommandEvent &event)
{
	if( !SysHasValidState() ) return;

	// Disable the menu item.  The state of the menu is indeterminate until the core thread
	// has responded (it updates status after the plugins are loaded and emulation has
	// engaged successfully).

	EnableMenuItem( MenuId_Sys_SuspendResume, false );
	GetSysExecutorThread().PostEvent( new SysExecEvent_ToggleSuspend() );
}

void MainEmuFrame::Menu_SysReset_Click(wxCommandEvent &event)
{
	UI_DisableSysReset();
	GetSysExecutorThread().PostEvent( new SysExecEvent_Restart() );
}

void MainEmuFrame::Menu_SysShutdown_Click(wxCommandEvent &event)
{
	if( !SysHasValidState() && !CorePlugins.AreAnyInitialized() ) return;

	UI_DisableSysShutdown();
	sApp.SysShutdown();
}

void MainEmuFrame::Menu_ConfigPlugin_Click(wxCommandEvent &event)
{
	const int eventId = event.GetId() - MenuId_PluginBase_Settings;

	PluginsEnum_t pid = (PluginsEnum_t)(eventId / PluginMenuId_Interval);

	// Don't try to call the Patches config dialog until we write one.
	if (event.GetId() == MenuId_Config_Patches) return;

	if( !pxAssertDev( (eventId >= 0) || (pid < PluginId_Count), "Invalid plugin identifier passed to ConfigPlugin event handler." ) ) return;

	wxWindowDisabler disabler;
	ScopedCoreThreadPause paused_core( new SysExecEvent_SaveSinglePlugin(pid) );
	GetCorePlugins().Configure( pid );
}

void MainEmuFrame::Menu_Debug_Open_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_Debug_MemoryDump_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_Debug_Logging_Click(wxCommandEvent &event)
{
	AppOpenDialog<LogOptionsDialog>( this );
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
	const_cast<Pcsx2Config&>(EmuConfig).CdvdVerboseReads = g_Conf->EmuOptions.CdvdVerboseReads;		// read-only in core thread, so it's safe to modify.
	SaveEmuOptions();
}

void MainEmuFrame::Menu_ShowAboutBox(wxCommandEvent &event)
{
	AppOpenDialog<AboutBoxDialog>( this );
}
