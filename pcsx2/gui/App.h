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
#include <wx/fileconf.h>
#include <wx/imaglist.h>
#include <wx/docview.h>

#include <wx/apptrait.h>

#include "AppConfig.h"
#include "System.h"
#include "ConsoleLogger.h"
#include "ps2/CoreEmuThread.h"

class IniInterface;

// ------------------------------------------------------------------------
// All Menu Options for the Main Window! :D
// ------------------------------------------------------------------------

enum MenuIdentifiers
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
	Menu_Config_BIOS,
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
	Menu_Debug_Usermode,
	Menu_Languages,

	// Language Menu
	// (Updated entirely dynamically, all values from range 1000 onward are reserved for languages)
	Menu_Language_Start = 1000
};


//////////////////////////////////////////////////////////////////////////////////////////
//
struct AppImageIds
{
	struct ConfigIds
	{
		int	Paths,
			Plugins,
			Speedhacks,
			Gamefixes,
			Video,
			Cpu;

		ConfigIds() :
			Paths( -1 )
		,	Plugins( -1 )
		,	Speedhacks( -1 )
		,	Gamefixes( -1 )
		,	Video( -1 )
		,	Cpu( -1 )
		{
		}
	} Config;

	struct ToolbarIds
	{
		int Settings,
			Play,
			Resume,
			PluginVideo,
			PluginAudio,
			PluginPad;

		ToolbarIds() :
			Settings( -1 )
		,	Play( -1 )
		,	PluginVideo( -1 )
		,	PluginAudio( -1 )
		,	PluginPad( -1 )
		{
		}
	} Toolbars;
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class pxAppTraits : public wxGUIAppTraits
{
#ifdef __WXDEBUG__
public:
	virtual bool ShowAssertDialog(const wxString& msg);

protected:
	virtual wxString GetAssertStackTrace();
#endif

};

//////////////////////////////////////////////////////////////////////////////////////////
//
class Pcsx2App : public wxApp
{
protected:
	MainEmuFrame*		m_MainFrame;
	ConsoleLogFrame*	m_ProgramLogBox;
	wxBitmap*			m_Bitmap_Logo;

	wxImageList		m_ConfigImages;
	bool			m_ConfigImagesAreLoaded;

	wxImageList*	m_ToolbarImages;		// dynamic (pointer) to allow for large/small redefinition.
	AppImageIds		m_ImageId;

public:
	Pcsx2App();
	virtual ~Pcsx2App();

	wxFrame* GetMainWindow() const;

	bool OnInit();
	int  OnExit();
	void OnInitCmdLine( wxCmdLineParser& parser );
	bool OnCmdLineParsed( wxCmdLineParser& parser );

	bool PrepForExit();
	
#ifdef __WXDEBUG__
	void OnAssertFailure( const wxChar *file, int line, const wxChar *func, const wxChar *cond, const wxChar *msg );
#endif

	const wxBitmap& GetLogoBitmap();
	wxImageList& GetImgList_Config();
	wxImageList& GetImgList_Toolbars();

	const AppImageIds& GetImgId() const { return m_ImageId; }

	MainEmuFrame& GetMainFrame() const
	{
		wxASSERT( m_MainFrame != NULL );
		return *m_MainFrame;
	}

	void PostMenuAction( MenuIdentifiers menu_id ) const;
	void Ping() const;

	// ----------------------------------------------------------------------------
	//        Console / Program Logging Helpers
	// ----------------------------------------------------------------------------
	ConsoleLogFrame* GetProgramLog()
	{
		return m_ProgramLogBox;
	}

	void CloseProgramLog()
	{
		m_ProgramLogBox->Close();

		// disable future console log messages from being sent to the window.
		m_ProgramLogBox = NULL;
	}

	void ProgramLog_CountMsg()
	{
		if( m_ProgramLogBox == NULL ) return;
		m_ProgramLogBox->CountMessage();
	}

	void ProgramLog_PostEvent( wxEvent& evt )
	{
		if( m_ProgramLogBox == NULL ) return;
		m_ProgramLogBox->GetEventHandler()->AddPendingEvent( evt );
	}

	//ConsoleLogFrame* GetConsoleFrame() const { return m_ProgramLogBox; }
	//void SetConsoleFrame( ConsoleLogFrame& frame ) { m_ProgramLogBox = &frame; }

protected:
	void ReadUserModeSettings();
	bool TryOpenConfigCwd();
	void OnSemaphorePing( wxCommandEvent& evt );
	void OnMessageBox( pxMessageBoxEvent& evt );
	void CleanupMess();
	void OpenWizardConsole();

	void HandleEvent(wxEvtHandler *handler, wxEventFunction func, wxEvent& event) const;

	// ----------------------------------------------------------------------------
	//      Override wx default exception handling behavior
	// ----------------------------------------------------------------------------

	// Just rethrow exceptions in the main loop, so that we can handle them properly in our
	// custom catch clauses in OnRun().  (ranting note: wtf is the point of this functionality
	// in wx?  Why would anyone ever want a generic catch-all exception handler that *isn't*
	// the unhandled exception handler?  Using this as anything besides a re-throw is terrible
	// program design and shouldn't even be allowed -- air)
	bool OnExceptionInMainLoop() { throw; }

	// Just rethrow unhandled exceptions to cause immediate debugger fail.
	void OnUnhandledException() { throw; }
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class AppEmuThread : public CoreEmuThread
{
public:
	AppEmuThread( const wxString& elf_file=wxEmptyString );
	virtual ~AppEmuThread() { }
	
	virtual void Resume();

protected:
	sptr ExecuteTask();
};



DECLARE_APP(Pcsx2App)

extern int EnumeratePluginsInFolder( const wxDirName& searchPath, wxArrayString* dest );
extern void LoadPlugins();
extern void InitPlugins();
extern void OpenPlugins();

extern wxRect wxGetDisplayArea();
extern bool pxIsValidWindowPosition( const wxWindow& window, const wxPoint& windowPos );

extern wxFileHistory*	g_RecentIsoList;
