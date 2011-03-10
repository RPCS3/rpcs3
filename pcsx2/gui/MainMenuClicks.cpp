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

#include "CDVD/CDVD.h"
#include "GS.h"

#include "MainFrame.h"
#include "IsoDropTarget.h"

#include "Dialogs/ModalPopups.h"
#include "Dialogs/ConfigurationDialog.h"
#include "Dialogs/LogOptionsDialog.h"

#include "Utilities/IniInterface.h"

using namespace Dialogs;

void MainEmuFrame::Menu_SysSettings_Click(wxCommandEvent &event)
{
	AppOpenDialog<SysConfigDialog>( this );
}

void MainEmuFrame::Menu_McdSettings_Click(wxCommandEvent &event)
{
	AppOpenDialog<McdConfigDialog>( this );
}

void MainEmuFrame::Menu_GameDatabase_Click(wxCommandEvent &event)
{
	AppOpenDialog<GameDatabaseDialog>( this );
}

void MainEmuFrame::Menu_WindowSettings_Click(wxCommandEvent &event)
{
	wxCommandEvent evt( pxEvt_SetSettingsPage );
	evt.SetString( L"GS Window" );
	AppOpenDialog<SysConfigDialog>( this )->GetEventHandler()->ProcessEvent( evt );
}

void MainEmuFrame::Menu_GSSettings_Click(wxCommandEvent &event)
{
	wxCommandEvent evt( pxEvt_SetSettingsPage );
	evt.SetString( L"GS" );
	AppOpenDialog<SysConfigDialog>( this )->GetEventHandler()->ProcessEvent( evt );
}

void MainEmuFrame::Menu_SelectPluginsBios_Click(wxCommandEvent &event)
{
	AppOpenDialog<ComponentsConfigDialog>( this );
}

void MainEmuFrame::Menu_Language_Click(wxCommandEvent &event)
{
	//AppOpenDialog<InterfaceConfigDialog>( this );
	InterfaceConfigDialog(this).ShowModal();
}

static void WipeSettings()
{
	wxGetApp().CleanupRestartable();
	wxGetApp().CleanupResources();

	wxRemoveFile( GetUiSettingsFilename() );
	wxRemoveFile( GetVmSettingsFilename() );

	// FIXME: wxRmdir doesn't seem to work here for some reason (possible file sharing issue
	// with a plugin that leaves a file handle dangling maybe?).  But deleting the inis folder
	// manually from explorer does work.  Can't think of a good work-around at the moment. --air

	//wxRmdir( GetSettingsFolder().ToString() );

	wxGetApp().GetRecentIsoManager().Clear();
	g_Conf = new AppConfig();
	sMainFrame.RemoveCdvdMenu();

	sApp.WipeUserModeSettings();
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
		if( !Msgbox::OkCancel( pxsFmt(
			pxE( "!Notice:DeleteSettings",
				L"This command clears %s settings and allows you to re-run the First-Time Wizard.  You will need to "
				L"manually restart %s after this operation.\n\n"
				L"WARNING!!  Click OK to delete *ALL* settings for %s and force-close the app, losing any current emulation progress.  Are you absolutely sure?"
				L"\n\n(note: settings for plugins are unaffected)"
			), pxGetAppName().c_str(), pxGetAppName().c_str(), pxGetAppName().c_str() ),
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

	if( (g_Conf->CdvdSource == CDVDsrc_Iso) && (isoFilename == g_Conf->CurrentIso) )
	{
		core_control.AllowResume();
		return result;
	}

	if( SysHasValidState() )
	{
		core_control.DisallowResume();
		wxDialogWithHelpers dialog( owner, _("Confirm ISO image change") );

		dialog += dialog.Heading(descpart1);
		dialog += dialog.GetCharHeight();
		dialog += dialog.Text(isoFilename);
		dialog += dialog.GetCharHeight();
		dialog += dialog.Heading(_("Do you want to swap discs or boot the new image (via system reset)?"));

		result = pxIssueConfirmation( dialog, MsgButtons().Reset().Cancel().Custom(_("Swap Disc"), "swap"), L"DragDrop.BootSwapIso" );
		if( result == wxID_CANCEL )
		{
			core_control.AllowResume();
			return result;
		}
	}

	SysUpdateIsoSrcFile( isoFilename );
	if( result == wxID_RESET )
	{
		core_control.DisallowResume();
		sApp.SysExecute( CDVDsrc_Iso );
	}
	else
	{
		Console.Indent().WriteLn( "HotSwapping to new ISO src image!" );
		g_Conf->CdvdSource = CDVDsrc_Iso;
		//CoreThread.ChangeCdvdSource();
		core_control.AllowResume();
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
		wxDialogWithHelpers dialog( owner, _("Confirm CDVD source change") );

		wxString changeMsg;
		changeMsg.Printf(_("You've selected to switch the CDVD source from %s to %s."),
			CDVD_SourceLabels[g_Conf->CdvdSource], CDVD_SourceLabels[newsrc] );

		dialog += dialog.Heading(changeMsg + L"\n\n" +
			_("Do you want to swap discs or boot the new image (system reset)?")
		);

		result = pxIssueConfirmation( dialog, MsgButtons().Reset().Cancel().Custom(_("Swap Disc"), "swap"), L"DragDrop.BootSwapIso" );

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
		//CoreThread.ChangeCdvdSource();
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

static wxString JoinFiletypes( const wxChar** src )
{
	wxString dest;
	while( *src != NULL )
	{
		if( *src[0] == 0 ) continue;
		if( !dest.IsEmpty() )
			dest += L";";

		dest += pxsFmt(L"*.%s", *src);

		if (wxFileName::IsCaseSensitive())
		{
			// omgosh!  the filesystem is CaSE SeNSiTiVE!!
			dest += pxsFmt(L";*.%s", *src).ToUpper();
		}

		++src;
	}
	
	return dest;
}

// Returns FALSE if the user canceled the action.
bool MainEmuFrame::_DoSelectIsoBrowser( wxString& result )
{
	static const wxChar* isoSupportedTypes[] =
	{
		L"iso", L"mdf", L"nrg", L"bin", L"img", NULL
	};

	const wxString isoSupportedLabel( JoinString(isoSupportedTypes, L" ") );
	const wxString isoSupportedList( JoinFiletypes(isoSupportedTypes) );
	
	wxArrayString isoFilterTypes;

	isoFilterTypes.Add(pxsFmt(_("All Supported (%s)"), (isoSupportedLabel + L" .dump").c_str()));
	isoFilterTypes.Add(isoSupportedList + L";*.dump");

	isoFilterTypes.Add(pxsFmt(_("Disc Images (%s)"), isoSupportedLabel.c_str() ));
	isoFilterTypes.Add(isoSupportedList);

	isoFilterTypes.Add(pxsFmt(_("Blockdumps (%s)"), L".dump" ));
	isoFilterTypes.Add(L"*.dump");

	isoFilterTypes.Add(_("All Files (*.*)"));
	isoFilterTypes.Add(L"*.*");
	
	wxFileDialog ctrl( this, _("Select CDVD source iso..."), g_Conf->Folders.RunIso.ToString(), wxEmptyString,
		JoinString(isoFilterTypes, L"|"), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

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
	static const wxChar* elfFilterType = L"ELF Files (.elf)|*.elf;*.ELF";

	wxFileDialog ctrl( this, _("Select ELF file..."), g_Conf->Folders.RunELF.ToString(), wxEmptyString,
		(wxString)elfFilterType + L"|" + _("All Files (*.*)") + L"|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST );

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

			wxDialogWithHelpers dialog( this, _("ISO file not found!") );
			dialog += dialog.Heading(
				_("An error occurred while trying to open the file:") + wxString(L"\n\n") + g_Conf->CurrentIso + L"\n\n" +
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
		wxDialogWithHelpers dialog( this, _("Confirm PS2 Reset") );
		dialog += dialog.Heading( GetMsg_ConfirmSysReset() );
		bool confirmed = (pxIssueConfirmation( dialog, MsgButtons().Yes().Cancel(), L"BootCdvd.ConfirmReset" ) != wxID_CANCEL);

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

/*//now only available at the memory card manager.
void MainEmuFrame::Menu_MultitapToggle_Click( wxCommandEvent& )
{
	g_Conf->EmuOptions.MultitapPort0_Enabled = GetMenuBar()->IsChecked( MenuId_Config_Multitap0Toggle );
	g_Conf->EmuOptions.MultitapPort1_Enabled = GetMenuBar()->IsChecked( MenuId_Config_Multitap1Toggle );
	AppApplySettings();
	AppSaveSettings();

	//evt.Skip();
}
*/

void MainEmuFrame::Menu_EnableBackupStates_Click( wxCommandEvent& )
{
	g_Conf->EmuOptions.BackupSavestate = GetMenuBar()->IsChecked( MenuId_EnableBackupStates );
	
	//without the next line, after toggling this menu-checkbox, the change only applies from the 2nd save and onwards
	//  (1st save after the toggle keeps the old pre-toggle value)..
	//  wonder what that means for all the other menu checkboxes which only use AppSaveSettings... (avih)
	AppApplySettings();
    
	AppSaveSettings();
}

void MainEmuFrame::Menu_EnablePatches_Click( wxCommandEvent& )
{
	g_Conf->EmuOptions.EnablePatches = GetMenuBar()->IsChecked( MenuId_EnablePatches );
    AppSaveSettings();
}

void MainEmuFrame::Menu_EnableCheats_Click( wxCommandEvent& )
{
	g_Conf->EmuOptions.EnableCheats  = GetMenuBar()->IsChecked( MenuId_EnableCheats );
    AppSaveSettings();
}

void MainEmuFrame::Menu_EnableHostFs_Click( wxCommandEvent& )
{
	g_Conf->EmuOptions.HostFs = GetMenuBar()->IsChecked( MenuId_EnableHostFs );
    AppSaveSettings();
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
	sApp.SysExecute();
}

void MainEmuFrame::Menu_SysShutdown_Click(wxCommandEvent &event)
{
	//if( !SysHasValidState() && !CorePlugins.AreAnyInitialized() ) return;

	UI_DisableSysShutdown();
	CoreThread.Reset();
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
	AppSaveSettings();
}

void MainEmuFrame::Menu_ShowAboutBox(wxCommandEvent &event)
{
	AppOpenDialog<AboutBoxDialog>( this );
}
