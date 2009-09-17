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
#include <wx/fileconf.h>
#include <wx/imaglist.h>
#include <wx/docview.h>

#include <wx/apptrait.h>

class IniInterface;
class MainEmuFrame;
class GSFrame;

#include "AppConfig.h"
#include "System.h"
#include "ConsoleLogger.h"
#include "ps2/CoreEmuThread.h"


BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( pxEVT_SemaphorePing, -1 )
	DECLARE_EVENT_TYPE( pxEVT_OpenModalDialog, -1 )
	DECLARE_EVENT_TYPE( pxEVT_ReloadPlugins, -1 )
	DECLARE_EVENT_TYPE( pxEVT_LoadPluginsComplete, -1 )
END_DECLARE_EVENT_TYPES()

// ------------------------------------------------------------------------
// All Menu Options for the Main Window! :D
// ------------------------------------------------------------------------

enum MenuIdentifiers
{
	// Main Menu Section
	MenuId_Boot = 1,
	MenuId_Emulation,
	MenuId_Config,				// General config, plus non audio/video plugins.
	MenuId_Video,				// Video options filled in by GS plugin
	MenuId_Audio,				// audio options filled in by SPU2 plugin
	MenuId_Misc,				// Misc options and help!

	MenuId_Exit = wxID_EXIT,
	MenuId_About = wxID_ABOUT,

	MenuId_EndTopLevel = 20,

	// Run SubSection
	MenuId_Cdvd_Source,
	MenuId_Src_Iso,
	MenuId_Src_Plugin,
	MenuId_Src_NoDisc,
	MenuId_Boot_Iso,			// Opens submenu with Iso browser, and recent isos.
	MenuId_IsoBrowse,			// Open dialog, runs selected iso.
	MenuId_Boot_CDVD,			// opens a submenu filled by CDVD plugin (usually list of drives)
	MenuId_Boot_ELF,
	MenuId_Boot_Recent,			// Menu populated with recent source bootings
	MenuId_SkipBiosToggle,		// enables the Bios Skip speedhack


	MenuId_Emu_Pause,			// suspends/resumes active emulation, retains plugin states
	MenuId_Emu_Close,			// Closes the emulator (states are preserved)
	MenuId_Emu_Reset,			// Issues a complete reset (wipes preserved states)
	MenuId_Emu_LoadStates,		// Opens load states submenu
	MenuId_Emu_SaveStates,		// Opens save states submenu
	MenuId_EnablePatches,

	MenuId_State_Load,
	MenuId_State_LoadOther,
	MenuId_State_Load01,		// first of many load slots
	MenuId_State_Save = MenuId_State_Load01+20,
	MenuId_State_SaveOther,
	MenuId_State_Save01,		// first of many save slots

	MenuId_State_EndSlotSection = MenuId_State_Save01+20,

	// Config Subsection
	MenuId_Config_Settings,
	MenuId_Config_BIOS,

	// Plugin ID order is important.  Must match the order in tbl_PluginInfo.
	MenuId_Config_GS,
	MenuId_Config_PAD,
	MenuId_Config_SPU2,
	MenuId_Config_CDVD,
	MenuId_Config_USB,
	MenuId_Config_FireWire,
	MenuId_Config_DEV9,
	MenuId_Config_Patches,

	// Video Subsection
	// Top items are PCSX2-controlled.  GS plugin items are inserted beneath.
	MenuId_Video_Basics,		// includes frame timings and skippings settings
	MenuId_Video_Advanced,		// inserted at the bottom of the menu

	// Audio subsection
	// Top items are PCSX2-controlled.  SPU2 plugin items are inserted beneath.
	// [no items at this time]
	MenuId_Audio_Advanced,		// inserted at the bottom of the menu

	// Controller subsection
	// Top items are PCSX2-controlled.  Pad plugin items are inserted beneath.
	// [no items at this time]
	MenuId_Pad_Advanced,

	// Miscellaneous Menu!  (Misc)
	MenuId_Website,				// Visit our awesome website!
	MenuId_Profiler,			// Enable profiler
	MenuId_Console,				// Enable console

	// Debug Subsection
	MenuId_Debug_Open,			// opens the debugger window / starts a debug session
	MenuId_Debug_MemoryDump,
	MenuId_Debug_Logging,		// dialog for selection additional log options
	MenuId_Debug_Usermode,
};

enum DialogIdentifiers
{
	DialogId_CoreSettings = 0x800,
	DialogId_BiosSelector,
	DialogId_LogOptions,
	DialogId_About,
};

//////////////////////////////////////////////////////////////////////////////////////////
// ScopedWindowDisable
//
// This class is a fix helper for WXGTK ports of PCSX2, which need the current window to
// be disabled in order for plugin-created modal dialogs to receive messages.  This disabling
// causes problems in Win32/MSW, where some plugins' modal dialogs will cause all PCSX2
// windows to minimize on closure.
//
class ScopedWindowDisable
{
	DeclareNoncopyableObject( ScopedWindowDisable )

protected:
	wxWindow& m_window;

public:
	ScopedWindowDisable( wxWindow* whee ) :
		m_window( *whee )
	{
	#ifdef __WXGTK__
		wxASSERT( whee != NULL );
		m_window.Disable();
	#endif
	}

	~ScopedWindowDisable()
	{
#ifdef __WXGTK__
		m_window.Enable();
		m_window.SetFocus();
#endif
	}
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


struct MsgboxEventResult
{
	Semaphore	WaitForMe;
	int			result;

	MsgboxEventResult() :
		WaitForMe(), result( 0 )
	{
	}
};

// --------------------------------------------------------------------------------------
//  Pcsx2App  -  main wxApp class
// --------------------------------------------------------------------------------------

class Pcsx2App : public wxApp
{
protected:
	wxImageList						m_ConfigImages;

	wxScopedPtr<wxImageList>		m_ToolbarImages;
	wxScopedPtr<wxBitmap>			m_Bitmap_Logo;

	wxScopedPtr<EmuCoreAllocations>	m_CoreAllocs;

public:
	wxScopedPtr<PluginManager>		m_CorePlugins;
	wxScopedPtr<CoreEmuThread>		m_CoreThread;

protected:
	// Note: Pointers to frames should not be scoped because wxWidgets handles deletion
	// of these objects internally.
	MainEmuFrame*		m_MainFrame;
	GSFrame*			m_gsFrame;
	ConsoleLogFrame*	m_ProgramLogBox;

	bool				m_ConfigImagesAreLoaded;
	AppImageIds			m_ImageId;

public:
	Pcsx2App();
	virtual ~Pcsx2App();

	void ReloadPlugins();

	void ApplySettings( const AppConfig* oldconf = NULL );
	void LoadSettings();
	void SaveSettings();

	void PostMenuAction( MenuIdentifiers menu_id ) const;
	int  ThreadedModalDialog( DialogIdentifiers dialogId );
	void Ping() const;

	bool PrepForExit();

	// Executes the emulator using a saved/existing virtual machine state and currently
	// configured CDVD source device.
	// Debug assertions:
	void SysExecute();
	void SysExecute( CDVD_SourceType cdvdsrc );

	void SysResume()
	{
		if( !m_CoreThread ) return;
		m_CoreThread->Resume();
	}

	void SysSuspend()
	{
		if( !m_CoreThread ) return;
		m_CoreThread->Suspend();
	}

	void SysReset()
	{
		m_CoreThread.reset();
		m_CorePlugins.reset();
	}

	bool EmuInProgress() const
	{
		return m_CoreThread && m_CoreThread->IsRunning();
	}

	const wxBitmap& GetLogoBitmap();
	wxImageList& GetImgList_Config();
	wxImageList& GetImgList_Toolbars();

	const AppImageIds& GetImgId() const { return m_ImageId; }

	MainEmuFrame& GetMainFrame() const
	{
		wxASSERT( ((uptr)GetTopWindow()) == ((uptr)m_MainFrame) );
		wxASSERT( m_MainFrame != NULL );
		return *m_MainFrame;
	}

	// --------------------------------------------------------------------------
	//  Overrides of wxApp virtuals:
	// --------------------------------------------------------------------------
	bool OnInit();
	int  OnExit();

	void OnInitCmdLine( wxCmdLineParser& parser );
	bool OnCmdLineParsed( wxCmdLineParser& parser );
	bool OnCmdLineError( wxCmdLineParser& parser );

#ifdef __WXDEBUG__
	void OnAssertFailure( const wxChar *file, int line, const wxChar *func, const wxChar *cond, const wxChar *msg );
#endif

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
		if ((wxTheApp == NULL) || ( m_ProgramLogBox == NULL )) return;
		m_ProgramLogBox->CountMessage();
	}

	void ProgramLog_PostEvent( wxEvent& evt )
	{
		if ((wxTheApp == NULL) || ( m_ProgramLogBox == NULL )) return;
		m_ProgramLogBox->GetEventHandler()->AddPendingEvent( evt );
	}

	//ConsoleLogFrame* GetConsoleFrame() const { return m_ProgramLogBox; }
	//void SetConsoleFrame( ConsoleLogFrame& frame ) { m_ProgramLogBox = &frame; }

protected:
	void ReadUserModeSettings();
	bool TryOpenConfigCwd();
	void CleanupMess();
	void OpenWizardConsole();

	void HandleEvent(wxEvtHandler *handler, wxEventFunction func, wxEvent& event) const;

	void OnReloadPlugins( wxCommandEvent& evt );
	void OnLoadPluginsComplete( wxCommandEvent& evt );
	void OnSemaphorePing( wxCommandEvent& evt );
	void OnOpenModalDialog( wxCommandEvent& evt );
	void OnMessageBox( pxMessageBoxEvent& evt );
	void OnEmuKeyDown( wxKeyEvent& evt );

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


// --------------------------------------------------------------------------------------
//  AppEmuThread class
// --------------------------------------------------------------------------------------

class AppEmuThread : public CoreEmuThread
{
protected:
	wxKeyEvent		m_kevt;

public:
	AppEmuThread( PluginManager& plugins );
	virtual ~AppEmuThread() throw() { }

	virtual void Resume();
	virtual void StateCheck();

protected:
	sptr ExecuteTask();
};

DECLARE_APP(Pcsx2App)

class EntryGuard
{
public:
	int& Counter;
	EntryGuard( int& counter ) : Counter( counter )
	{ ++Counter; }

	virtual ~EntryGuard() throw()
	{ --Counter; }

	bool IsReentrant() const { return Counter > 1; }
};

extern int EnumeratePluginsInFolder( const wxDirName& searchPath, wxArrayString* dest );
extern void LoadPluginsPassive();
extern void LoadPluginsImmediate();
extern void UnloadPlugins();

extern wxRect wxGetDisplayArea();
extern bool pxIsValidWindowPosition( const wxWindow& window, const wxPoint& windowPos );

extern bool HandlePluginError( Exception::PluginError& ex );
extern bool EmulationInProgress();

