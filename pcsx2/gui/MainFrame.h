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

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/docview.h>

#include "App.h"

// --------------------------------------------------------------------------------------
//  GSPanel
// --------------------------------------------------------------------------------------
class GSPanel : public wxWindow
{
	typedef wxWindow _parent;

protected:
	AcceleratorDictionary		m_Accels;
	EventListenerBinding<int>	m_Listener_SettingsApplied;
	wxTimer						m_HideMouseTimer;
	bool						m_CursorShown;
	bool						m_HasFocus;

public:
	GSPanel( wxWindow* parent );
	virtual ~GSPanel() throw();

	void DoResize();
	void DoShowMouse();

protected:
	static void __evt_fastcall OnSettingsApplied( void* obj, int& evt );
	
#ifdef __WXMSW__
	virtual WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
#endif

	void InitDefaultAccelerators();

	void DoSettingsApplied();

	void OnCloseWindow( wxCloseEvent& evt );
	void OnResize(wxSizeEvent& event);
	void OnShowMouse( wxMouseEvent& evt );
	void OnHideMouseTimeout( wxTimerEvent& evt );
	void OnKeyDown( wxKeyEvent& evt );
	void OnFocus( wxFocusEvent& evt );
	void OnFocusLost( wxFocusEvent& evt );
};

// --------------------------------------------------------------------------------------
//  GSFrame
// --------------------------------------------------------------------------------------
class GSFrame : public wxFrame
{
	typedef wxFrame _parent;

protected:
	EventListenerBinding<int>	m_Listener_SettingsApplied;
	GSPanel*					m_gspanel;
	wxStaticText*				m_label_Disabled;

public:
	GSFrame(wxWindow* parent, const wxString& title);
	virtual ~GSFrame() throw();

	wxWindow* GetViewport();

	bool Show( bool shown=true );

protected:
	void OnMove( wxMoveEvent& evt );
	void OnResize( wxSizeEvent& evt );
	void OnActivate( wxActivateEvent& evt );

	void DoSettingsApplied();

	static void __evt_fastcall OnSettingsApplied( void* obj, int& evt );
};

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
//  MainEmuFrame
// --------------------------------------------------------------------------------------
class MainEmuFrame : public wxFrame
{
protected:
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

	CmdEvt_ListenerBinding					m_Listener_CoreThreadStatus;
	EventListenerBinding<PluginEventType>	m_Listener_CorePluginStatus;
	EventListenerBinding<int>				m_Listener_SettingsApplied;
	EventListenerBinding<IniInterface>		m_Listener_SettingsLoadSave;

public:
    MainEmuFrame(wxWindow* parent, const wxString& title);
    virtual ~MainEmuFrame() throw();

	void OnLogBoxHidden();

	bool IsPaused() const { return GetMenuBar()->IsChecked( MenuId_Sys_SuspendResume ); }
	void UpdateIsoSrcSelection();

protected:
	static void __evt_fastcall OnCoreThreadStatusChanged( void* obj, wxCommandEvent& evt );
	static void __evt_fastcall OnCorePluginStatusChanged( void* obj, PluginEventType& evt );
	static void __evt_fastcall OnSettingsApplied( void* obj, int& evt );
	static void __evt_fastcall OnSettingsLoadSave( void* obj, IniInterface& evt );

	void ApplySettings();
	void ApplyCoreStatus();
	void ApplyPluginStatus();

	void SaveEmuOptions();

	void InitLogBoxPosition( AppConfig::ConsoleLogOptions& conf );

	void OnCloseWindow( wxCloseEvent& evt );
	void OnMoveAround( wxMoveEvent& evt );
	void OnFocus( wxFocusEvent& evt );
	void OnActivate( wxActivateEvent& evt );

	void Menu_ConfigSettings_Click(wxCommandEvent &event);
	void Menu_AppSettings_Click(wxCommandEvent &event);
	void Menu_SelectBios_Click(wxCommandEvent &event);

	void Menu_IsoBrowse_Click(wxCommandEvent &event);
	void Menu_SkipBiosToggle_Click(wxCommandEvent &event);
	void Menu_EnablePatches_Click(wxCommandEvent &event);

	void Menu_BootCdvd_Click(wxCommandEvent &event);
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

