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

 #pragma once

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/docview.h>

#include "App.h"
#include "ps2/CoreEmuThread.h"

extern CoreEmuThread*	g_EmuThread;

//////////////////////////////////////////////////////////////////////////////////////////
//
class MainEmuFrame: public wxFrame
{
protected:
	// ------------------------------------------------------------------------
	// All Menu Options for the Main Window! :D
	// ------------------------------------------------------------------------

	enum Identifiers
	{
		// Main Menu Section
		Menu_Run = 1,
		Menu_Config,			// General config, plus non audio/video plugins.
		Menu_Video,				// Video options filled in by GS plugin
		Menu_Audio,				// audio options filled in by SPU2 plugin
		Menu_Misc,				// Misc options and help!

		// Run SubSection
		Menu_RunIso = 20,		// Opens submenu with Iso browser, and recent isos.
		Menu_IsoBrowse,			// Open dialog, runs selected iso.
		Menu_BootCDVD,			// opens a submenu filled by CDVD plugin (usually list of drives)
		Menu_RunWithoutDisc,	// used to enter the bios (subs in cdvdnull)
		Menu_RunELF,
		Menu_SkipBiosToggle,	// enables the Bios Skip  speedhack
		Menu_EnableSkipBios,	// check marked menu that toggles Skip Bios boot feature.
		Menu_PauseExec,			// suspends/resumes active emulation
		Menu_Reset,				// Issues a complete reset.
		Menu_States,			// Opens states submenu
		Menu_Run_Exit = wxID_EXIT,

		Menu_State_Load = 40,
		Menu_State_LoadOther,
		Menu_State_Load01,		// first of many load slots
		Menu_State_Save = 60,
		Menu_State_SaveOther,
		Menu_State_Save01,		// first of many save slots

		// Config Subsection
		Menu_Config_Settings = 100,
		Menu_Config_CDVD,
		Menu_Config_DEV9,
		Menu_Config_USB,
		Menu_Config_FireWire,
		Menu_Config_Patches,

		// Video Subsection
		// Top items are Pcsx2-controlled.  GS plugin items are inserted beneath.
		Menu_Video_Basics = 200,		// includes frame timings and skippings settings
		Menu_Video_Advanced,		// inserted at the bottom of the menu
		Menu_GS_Custom1 = 210,		// start index for GS custom entries (valid up to 299)
		Menu_GS_CustomMax = 299,

		// Audio subsection
		// Top items are Pcsx2-controlled.  SPU2 plugin items are inserted beneath.
		// [no items at this time]
		Menu_Audio_Advanced = 300,	// inserted at the bottom of the menu
		Menu_Audio_Custom1 = 310,
		Menu_Audio_CustomMax = 399,

		// Controller subsection
		// Top items are Pcsx2-controlled.  Pad plugin items are inserted beneath.
		// [no items at this time]
		Menu_Pad_Advanced = 400,
		Menu_Pad_Custom1 = 410,
		Menu_Pad_CustomMax = 499,

		// Miscellaneous Menu!  (Misc)
		Menu_About = wxID_ABOUT,
		Menu_Website = 500,			// Visit our awesome website!
		Menu_Profiler,				// Enable profiler
		Menu_Console,				// Enable console
		Menu_Patches,

		// Debug Subsection
		Menu_Debug_Open = 600,		// opens the debugger window / starts a debug session
		Menu_Debug_MemoryDump,
		Menu_Debug_Logging,			// dialog for selection additional log options
		Menu_Languages,

		// Language Menu
		// (Updated entirely dynamically, all values from range 1000 onward are reserved for languages)
		Menu_Language_Start = 1000
	};

	// ------------------------------------------------------------------------
	// MainEmuFrame Protected Variables
	// ------------------------------------------------------------------------
	
protected:
    wxStatusBar& m_statusbar;
    wxStaticBitmap m_background;

	wxMenuBar& m_menubar;

	wxMenu& m_menuRun;
	wxMenu& m_menuConfig;
	wxMenu& m_menuMisc;
	wxMenu& m_menuDebug;

	wxMenu& m_menuVideo;
	wxMenu& m_menuAudio;
	wxMenu& m_menuPad;

	wxMenu& m_LoadStatesSubmenu;
	wxMenu& m_SaveStatesSubmenu;

	wxMenuItem& m_MenuItem_Console;

	bool m_IsPaused;
	
	// ------------------------------------------------------------------------
	// MainEmuFrame Constructors and Member Methods
	// ------------------------------------------------------------------------

public:
    MainEmuFrame(wxWindow* parent, const wxString& title);
	void OnLogBoxHidden();

	bool IsPaused() const { return m_IsPaused; }

protected:
	void InitLogBoxPosition( AppConfig::ConsoleLogOptions& conf );

	void OnCloseWindow(wxCloseEvent& evt);
	void OnMoveAround( wxMoveEvent& evt );

	void Menu_ConfigSettings_Click(wxCommandEvent &event);

	void Menu_RunIso_Click(wxCommandEvent &event);
	void Menu_RunWithoutDisc_Click(wxCommandEvent &event);
	void Menu_IsoBrowse_Click(wxCommandEvent &event);
	void Menu_IsoRecent_Click(wxCommandEvent &event);

	void Menu_OpenELF_Click(wxCommandEvent &event);
	void Menu_LoadStateOther_Click(wxCommandEvent &event);
	void Menu_SaveStateOther_Click(wxCommandEvent &event);
	void Menu_Exit_Click(wxCommandEvent &event);

	void Menu_Pause_Click(wxCommandEvent &event);
	void Menu_Reset_Click(wxCommandEvent &event);

	void Menu_Debug_Open_Click(wxCommandEvent &event);
	void Menu_Debug_MemoryDump_Click(wxCommandEvent &event);
	void Menu_Debug_Logging_Click(wxCommandEvent &event);

	void Menu_ShowConsole(wxCommandEvent &event);
	void Menu_ShowAboutBox(wxCommandEvent &event);

	// ------------------------------------------------------------------------
	// MainEmuFram Internal API for Populating Main Menu Contents
	// ------------------------------------------------------------------------

	wxMenu* MakeStatesSubMenu( int baseid ) const;
	wxMenu* MakeStatesMenu();
	wxMenu* MakeLanguagesMenu() const;
	wxMenu* MakeIsoMenu();
	wxMenu* MakeCdvdMenu();

	void PopulateVideoMenu();
	void PopulateAudioMenu();
	void PopulatePadMenu();
	void ConnectMenus();
};

