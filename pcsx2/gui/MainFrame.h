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

class MainEmuFrame: public wxFrame
{
	// ------------------------------------------------------------------------
	//     MainEmuFrame Protected Variables
	// ------------------------------------------------------------------------
	
protected:
	wxFileHistory*	m_RecentIsoList;
    wxStatusBar&	m_statusbar;
    wxStaticBitmap	m_background;

	wxMenuBar& m_menubar;

	wxMenu& m_menuBoot;
	wxMenu& m_menuEmu;
	wxMenu& m_menuConfig;
	wxMenu& m_menuMisc;
	wxMenu& m_menuDebug;

	wxMenu& m_menuVideo;
	wxMenu& m_menuAudio;
	wxMenu& m_menuPad;

	wxMenu& m_LoadStatesSubmenu;
	wxMenu& m_SaveStatesSubmenu;

	wxMenuItem& m_MenuItem_Console;

	// ------------------------------------------------------------------------
	//     MainEmuFrame Constructors and Member Methods
	// ------------------------------------------------------------------------

public:
    MainEmuFrame(wxWindow* parent, const wxString& title);
    virtual ~MainEmuFrame();

	void OnLogBoxHidden();

	bool IsPaused() const { return GetMenuBar()->IsChecked( MenuId_Emu_Pause ); }
	void UpdateIsoSrcFile();
	void UpdateIsoSrcSelection();
	void ApplySettings();
	
protected:
	void InitLogBoxPosition( AppConfig::ConsoleLogOptions& conf );

	void OnCloseWindow(wxCloseEvent& evt);
	void OnMoveAround( wxMoveEvent& evt );

	void Menu_ConfigSettings_Click(wxCommandEvent &event);
	void Menu_SelectBios_Click(wxCommandEvent &event);

	void Menu_RunIso_Click(wxCommandEvent &event);
	void Menu_IsoBrowse_Click(wxCommandEvent &event);
	void Menu_IsoRecent_Click(wxCommandEvent &event);

	void Menu_BootCdvd_Click(wxCommandEvent &event);
	void Menu_OpenELF_Click(wxCommandEvent &event);
	void Menu_CdvdSource_Click(wxCommandEvent &event);
	void Menu_LoadStateOther_Click(wxCommandEvent &event);
	void Menu_SaveStateOther_Click(wxCommandEvent &event);
	void Menu_Exit_Click(wxCommandEvent &event);

	void Menu_EmuPause_Click(wxCommandEvent &event);
	void Menu_EmuClose_Click(wxCommandEvent &event);
	void Menu_EmuReset_Click(wxCommandEvent &event);

	void Menu_ConfigPlugin_Click(wxCommandEvent &event);

	void Menu_Debug_Open_Click(wxCommandEvent &event);
	void Menu_Debug_MemoryDump_Click(wxCommandEvent &event);
	void Menu_Debug_Logging_Click(wxCommandEvent &event);

	void Menu_ShowConsole(wxCommandEvent &event);
	void Menu_ShowAboutBox(wxCommandEvent &event);

	// ------------------------------------------------------------------------
	//     MainEmuFram Internal API for Populating Main Menu Contents
	// ------------------------------------------------------------------------

	wxMenu* MakeStatesSubMenu( int baseid ) const;
	wxMenu* MakeStatesMenu();
	wxMenu* MakeLanguagesMenu() const;
	wxMenu* MakeCdvdMenu();

	void PopulateVideoMenu();
	void PopulateAudioMenu();
	void PopulatePadMenu();
	void ConnectMenus();

	friend class Pcsx2App;
};

