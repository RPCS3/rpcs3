#pragma once

#include <wx/wx.h>
#include <wx/image.h>

class frmMain: public wxFrame
{
public:
    frmMain(wxWindow* parent, int id, const wxString& title, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, long style=wxDEFAULT_FRAME_STYLE);

protected:

	enum Identifiers
	{
		// Main Menu Section	
		Menu_Run = 1,
		Menu_Config,			// General config, plus non audio/video plugins.
		Menu_Video,				// Video options filled in by GS plugin
		Menu_Audio,				// audio options filled in by SPU2 plugin
		Menu_Misc,				// Misc options and help!

		// Run SubSection
		Menu_QuickBootCD = 60,	// boots games using the bios stub method (skips power-on checks)
		Menu_FullBootCD,		// boots games through the bios.
		Menu_BootNoCD,			// used to enter the bios (subs in cdvdnull)
		Menu_RunELF,
		Menu_SuspendExec,			// suspends active emulation
		Menu_ResumeExec,		// restores active emulation
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
		Menu_SelectPlugins = 100,
		Menu_Config_CDVD,
		Menu_Config_DEV9,
		Menu_Config_USB,
		Menu_Config_FireWire,

		Menu_Config_Memcards,
		Menu_Config_SpeedHacks,
		Menu_Config_Gamefixes,
		Menu_Config_Patches,
		Menu_Config_Advanced,

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

	wxMenu* MakeStatesSubMenu( int baseid ) const;
	wxMenu* MakeStatesMenu();
	wxMenu* MakeLanguagesMenu() const;
	
	void PopulateVideoMenu();
	void PopulateAudioMenu();
	void PopulatePadMenu();
	

private:
    void set_properties();

protected:
    wxMenuBar& m_menubar;
    wxStatusBar& m_statusbar;
    wxStaticBitmap& m_background;
    
	wxMenu& m_menuRun;
	wxMenu& m_menuConfig;
	wxMenu& m_menuMisc;

	wxMenu& m_menuVideo;
	wxMenu& m_menuAudio;
	wxMenu& m_menuPad;

	wxMenu& m_LoadStatesSubmenu;
	wxMenu& m_SaveStatesSubmenu;

    DECLARE_EVENT_TABLE();

	//////////////////////////////////////////////////////////////////////////////////////////
	// Menu Options for the Main Window! :D
	
public:
	void Menu_QuickBootCD_Click(wxCommandEvent &event);
    void Menu_BootCD_Click(wxCommandEvent &event);
	void Menu_BootNoCD_Click(wxCommandEvent &event);

    void Menu_OpenELF_Click(wxCommandEvent &event);
    void Menu_LoadStateOther_Click(wxCommandEvent &event);
    void Menu_SaveStateOther_Click(wxCommandEvent &event);
    void Menu_Exit_Click(wxCommandEvent &event);

    void Menu_Suspend_Click(wxCommandEvent &event);
	void Menu_Resume_Click(wxCommandEvent &event);
    void Menu_Reset_Click(wxCommandEvent &event);

	void Menu_Gamefixes_Click( wxCommandEvent& event );
};

