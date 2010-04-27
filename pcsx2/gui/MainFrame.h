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

 #pragma once

#include "App.h"
#include "AppSaveStates.h"

#include <wx/image.h>
#include <wx/docview.h>

struct PluginMenuAddition
{
	wxString			Text;
	wxString			HelpText;
	PS2E_MenuItemStyle	Flags;

	wxMenuItem*			Item;
	int					ItemId;

	// Optional user data pointer (or typecast integer value)
	void*				UserPtr;

	void (PS2E_CALLBACK *OnClicked)( PS2E_THISPTR* thisptr, void* userptr );
};

// --------------------------------------------------------------------------------------
//  PerPluginMenuInfo
// --------------------------------------------------------------------------------------
class PerPluginMenuInfo
{
protected:
	typedef std::vector<PluginMenuAddition> MenuItemAddonList;

	// A list of menu items belonging to this plugin's menu.
	MenuItemAddonList	m_PluginMenuItems;

	// Base index for inserting items, usually points to the position
	// after the heading entry and separator.
	int					m_InsertIndexBase;

	// Current index for inserting menu items; increments with each item
	// added by a plugin.
	int					m_InsertIndexCur;

public:
	PluginsEnum_t		PluginId;
	wxMenu&				MyMenu;

public:
	PerPluginMenuInfo() : MyMenu( *new wxMenu() )
	{
	}

	virtual ~PerPluginMenuInfo() throw();

	void Populate( PluginsEnum_t pid );
	void OnUnloaded();
	void OnLoaded();

	operator wxMenu*() { return &MyMenu; }
	operator const wxMenu*() const { return &MyMenu; }
};

// --------------------------------------------------------------------------------------
//  InvokeMenuCommand_OnSysStateUnlocked
// --------------------------------------------------------------------------------------
class InvokeMenuCommand_OnSysStateUnlocked
	: public IEventListener_SysState
	, public BaseDeletableObject
{
protected:
	MenuIdentifiers		m_menu_cmd;

public:
	InvokeMenuCommand_OnSysStateUnlocked( MenuIdentifiers menu_command )
	{
		m_menu_cmd = menu_command;
	}
	
	virtual ~InvokeMenuCommand_OnSysStateUnlocked() throw() {}

	virtual void SaveStateAction_OnCreateFinished()
	{
		wxGetApp().PostMenuAction( m_menu_cmd );
	}
};

// --------------------------------------------------------------------------------------
//  MainEmuFrame
// --------------------------------------------------------------------------------------
class MainEmuFrame : public wxFrame,
	public EventListener_Plugins,
	public EventListener_CoreThread,
	public EventListener_AppStatus
{
	typedef wxFrame _parent;
	
protected:
	bool			m_RestartEmuOnDelete;

    wxStatusBar&	m_statusbar;
    wxStaticBitmap	m_background;

	wxMenuBar&		m_menubar;

	wxMenu&			m_menuBoot;
	wxMenu&			m_menuCDVD;
	wxMenu&			m_menuSys;
	wxMenu&			m_menuConfig;
	wxMenu&			m_menuMisc;
	wxMenu&			m_menuDebug;

	wxMenu&			m_LoadStatesSubmenu;
	wxMenu&			m_SaveStatesSubmenu;

	wxMenuItem&		m_MenuItem_Console;
	wxMenuItem&		m_MenuItem_Console_Stdio;

	PerPluginMenuInfo	m_PluginMenuPacks[PluginId_Count];

	virtual void DispatchEvent( const PluginEventType& plugin_evt );
	virtual void DispatchEvent( const CoreThreadStatus& status );
	virtual void AppStatusEvent_OnSettingsLoadSave();
	virtual void AppStatusEvent_OnSettingsApplied();

public:
    MainEmuFrame(wxWindow* parent, const wxString& title);
    virtual ~MainEmuFrame() throw();

	void OnLogBoxHidden();

	bool IsPaused() const { return GetMenuBar()->IsChecked( MenuId_Sys_SuspendResume ); }
	void UpdateIsoSrcSelection();
	void RemoveCdvdMenu();
	void EnableMenuItem( int id, bool enable );

protected:
	void DoGiveHelp(const wxString& text, bool show);

	void ApplySettings();
	void ApplyCoreStatus();
	void SaveEmuOptions();

	void InitLogBoxPosition( AppConfig::ConsoleLogOptions& conf );

	void OnCloseWindow( wxCloseEvent& evt );
	void OnDestroyWindow( wxWindowDestroyEvent& evt );
	void OnMoveAround( wxMoveEvent& evt );
	void OnFocus( wxFocusEvent& evt );
	void OnActivate( wxActivateEvent& evt );

	void Menu_ConfigSettings_Click(wxCommandEvent &event);
	void Menu_AppSettings_Click(wxCommandEvent &event);
	void Menu_SelectBios_Click(wxCommandEvent &event);
	void Menu_ResetAllSettings_Click(wxCommandEvent &event);

	void Menu_IsoBrowse_Click(wxCommandEvent &event);
	void Menu_EnablePatches_Click(wxCommandEvent &event);

	void Menu_BootCdvd_Click(wxCommandEvent &event);
	void Menu_BootCdvd2_Click(wxCommandEvent &event);
	void Menu_OpenELF_Click(wxCommandEvent &event);
	void Menu_CdvdSource_Click(wxCommandEvent &event);
	void Menu_LoadStates_Click(wxCommandEvent &event);
	void Menu_SaveStates_Click(wxCommandEvent &event);
	void Menu_LoadStateOther_Click(wxCommandEvent &event);
	void Menu_SaveStateOther_Click(wxCommandEvent &event);
	void Menu_Exit_Click(wxCommandEvent &event);

	void Menu_SuspendResume_Click(wxCommandEvent &event);
	void Menu_SysReset_Click(wxCommandEvent &event);
	void Menu_SysShutdown_Click(wxCommandEvent &event);

	void Menu_ConfigPlugin_Click(wxCommandEvent &event);

	void Menu_MultitapToggle_Click(wxCommandEvent &event);

	void Menu_Debug_Open_Click(wxCommandEvent &event);
	void Menu_Debug_MemoryDump_Click(wxCommandEvent &event);
	void Menu_Debug_Logging_Click(wxCommandEvent &event);

	void Menu_ShowConsole(wxCommandEvent &event);
	void Menu_ShowConsole_Stdio(wxCommandEvent &event);
	void Menu_PrintCDVD_Info(wxCommandEvent &event);
	void Menu_ShowAboutBox(wxCommandEvent &event);

	void _DoBootCdvd();
	bool _DoSelectIsoBrowser( wxString& dest );
	bool _DoSelectELFBrowser();

// ------------------------------------------------------------------------
//     MainEmuFram Internal API for Populating Main Menu Contents
// ------------------------------------------------------------------------

	wxMenu* MakeStatesSubMenu( int baseid ) const;
	wxMenu* MakeStatesMenu();
	wxMenu* MakeLanguagesMenu() const;

	void ConnectMenus();

	friend class Pcsx2App;
};

