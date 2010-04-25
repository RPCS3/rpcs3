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

#include "Utilities/wxAppWithHelpers.h"

#include <wx/fileconf.h>
#include <wx/imaglist.h>
#include <wx/apptrait.h>

#include "AppCommon.h"
#include "RecentIsoList.h"

#include "System.h"
#include "System/SysThreads.h"

#include "Utilities/HashMap.h"

class Pcsx2App;

typedef void FnType_OnThreadComplete(const wxCommandEvent& evt);
typedef void (Pcsx2App::*FnPtr_AppMethod)();

BEGIN_DECLARE_EVENT_TYPES()
	/*DECLARE_EVENT_TYPE( pxEVT_ReloadPlugins, -1 )
	DECLARE_EVENT_TYPE( pxEVT_OpenGsPanel, -1 )*/

	DECLARE_EVENT_TYPE( pxEvt_FreezeThreadFinished, -1 )
	DECLARE_EVENT_TYPE( pxEvt_CoreThreadStatus, -1 )
	DECLARE_EVENT_TYPE( pxEvt_LoadPluginsComplete, -1 )
	DECLARE_EVENT_TYPE( pxEvt_PluginStatus, -1 )
	DECLARE_EVENT_TYPE( pxEvt_SysExecute, -1 )
	DECLARE_EVENT_TYPE( pxEvt_InvokeMethod, -1 )
	DECLARE_EVENT_TYPE( pxEvt_LogicalVsync, -1 )

	DECLARE_EVENT_TYPE( pxEvt_OpenModalDialog, -1 )
	//DECLARE_EVENT_TYPE( pxEvt_StuckThread, -1 )
END_DECLARE_EVENT_TYPES()

// This is used when the GS plugin is handling its own window.  Messages from the PAD
// are piped through to an app-level message handler, which dispatches them through
// the universal Accelerator table.
static const int pxID_PadHandler_Keydown = 8030;

// Plugin ID sections are spaced out evenly at intervals to make it easy to use a
// single for-loop to create them.
static const int PluginMenuId_Interval = 0x10;

// Forces the Interface to destroy the GS viewport window when the GS plugin is
// destroyed.  This has the side effect of forcing all plugins to close and re-open
// along with the GS, since the GS viewport window handle will have changed.
static const bool CloseViewportWithPlugins = false;

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
	MenuId_IsoSelector,			// Contains a submenu of selectable "favorite" isos
	MenuId_IsoBrowse,			// Open dialog, runs selected iso.
	MenuId_Boot_CDVD,			// opens a submenu filled by CDVD plugin (usually list of drives)
	MenuId_Boot_ELF,
	MenuId_Boot_Recent,			// Menu populated with recent source bootings
	MenuId_SkipBiosToggle,		// enables the Bios Skip speedhack


	MenuId_Sys_SuspendResume,	// suspends/resumes active emulation, retains plugin states
	MenuId_Sys_Close,			// Closes the emulator (states are preserved)
	MenuId_Sys_Reset,			// Issues a complete VM reset (wipes preserved states)
	MenuId_Sys_Shutdown,		// Closes virtual machine, shuts down plugins, wipes states.
	MenuId_Sys_LoadStates,		// Opens load states submenu
	MenuId_Sys_SaveStates,		// Opens save states submenu
	MenuId_EnablePatches,

	MenuId_State_Load,
	MenuId_State_LoadOther,
	MenuId_State_Load01,		// first of many load slots
	MenuId_State_Save = MenuId_State_Load01+20,
	MenuId_State_SaveOther,
	MenuId_State_Save01,		// first of many save slots

	MenuId_State_EndSlotSection = MenuId_State_Save01+20,

	// Config Subsection
	MenuId_Config_SysSettings,
	MenuId_Config_AppSettings,
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

	MenuId_Config_Multitap0Toggle,
	MenuId_Config_Multitap1Toggle,

	// Plugin Sections
	// ---------------
	// Each plugin menu begins with its name, which is a grayed out option that's
	// intended for display purposes only.  Plugin ID sections are spaced out evenly
	// at intervals to make it easy to use a single for-loop to create them.

	MenuId_PluginBase_Name = 0x100,
	MenuId_PluginBase_Settings = 0x101,

	MenuId_Video_CoreSettings = 0x200,// includes frame timings and skippings settings


	// Miscellaneous Menu!  (Misc)
	MenuId_Website,				// Visit our awesome website!
	MenuId_Profiler,			// Enable profiler
	MenuId_Console,				// Enable console
	MenuId_Console_Stdio,		// Enable Stdio
	MenuId_CDVD_Info,

	// Debug Subsection
	MenuId_Debug_Open,			// opens the debugger window / starts a debug session
	MenuId_Debug_MemoryDump,
	MenuId_Debug_Logging,		// dialog for selection additional log options
	MenuId_Config_ResetAll,
};

namespace Exception
{
	// --------------------------------------------------------------------------
	// Exception used to perform an "errorless" termination of the app during OnInit
	// procedures.  This happens when a user cancels out of startup prompts/wizards.
	//
	class StartupAborted : public BaseException
	{
	public:
		DEFINE_EXCEPTION_COPYTORS( StartupAborted )

		StartupAborted( const wxString& msg_eng=L"Startup initialization was aborted by the user." )
		{
			// english messages only for this exception.
			BaseException::InitBaseEx( msg_eng, msg_eng );
		}
	};

}

// --------------------------------------------------------------------------------------
//  KeyAcceleratorCode
//  A custom keyboard accelerator that I like better than wx's wxAcceleratorEntry.
// --------------------------------------------------------------------------------------
struct KeyAcceleratorCode
{
	union
	{
		struct
		{
			u16		keycode;
			u16		win:1,		// win32 only.
					cmd:1,		// ctrl in win32, Command in Mac
					alt:1,
					shift:1;
		};
		u32  val32;
	};

	KeyAcceleratorCode() : val32( 0 ) {}
	KeyAcceleratorCode( const wxKeyEvent& evt );

	KeyAcceleratorCode( wxKeyCode code )
	{
		val32 = 0;
		keycode = code;
	}

	KeyAcceleratorCode& Shift()
	{
		shift = true;
		return *this;
	}

	KeyAcceleratorCode& Alt()
	{
		alt = true;
		return *this;
	}

	KeyAcceleratorCode& Win()
	{
		win = true;
		return *this;
	}

	KeyAcceleratorCode& Cmd()
	{
		cmd = true;
		return *this;
	}

	wxString ToString() const;
};


// --------------------------------------------------------------------------------------
//  GlobalCommandDescriptor
//  Describes a global command which can be invoked from the main GUI or GUI plugins.
// --------------------------------------------------------------------------------------

struct GlobalCommandDescriptor
{
	const char* Id;					// Identifier string
	void		(*Invoke)();		// Do it!!  Do it NOW!!!

	const char*	Fullname;			// Name displayed in pulldown menus
	const char*	Tooltip;			// text displayed in toolbar tooltips and menu status bars.

	int			ToolbarIconId;		// not implemented yet, leave 0 for now.
};

typedef HashTools::Dictionary<const GlobalCommandDescriptor*>	CommandDictionary;

class AcceleratorDictionary : public HashTools::HashMap<int, const GlobalCommandDescriptor*>
{
	typedef HashTools::HashMap<int, const GlobalCommandDescriptor*> _parent;

protected:

public:
	using _parent::operator[];

	AcceleratorDictionary();
	void Map( const KeyAcceleratorCode& acode, const char *searchfor );
};

// --------------------------------------------------------------------------------------
//  AppImageIds  - Config and Toolbar Images and Icons
// --------------------------------------------------------------------------------------
struct AppImageIds
{
	struct ConfigIds
	{
		int	Paths,
			Plugins,
			Speedhacks,
			Gamefixes,
			MemoryCard,
			Video,
			Cpu;

		ConfigIds()
		{
			Paths		= Plugins	=
			Speedhacks	= Gamefixes	=
			Video		= Cpu		=
			MemoryCard	= -1;
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

		ToolbarIds()
		{
			Settings	= Play	=
			PluginVideo	=
			PluginAudio	=
			PluginPad	= -1;
		}
	} Toolbars;
};

// -------------------------------------------------------------------------------------------
//  pxAppResources
// -------------------------------------------------------------------------------------------
// Container class for resources that should (or must) be unloaded prior to the ~wxApp() destructor.
// (typically this object is deleted at OnExit() or just prior to OnExit()).
//
struct pxAppResources
{
	AppImageIds					ImageId;

	ScopedPtr<wxImageList>		ConfigImages;
	ScopedPtr<wxImageList>		ToolbarImages;
	ScopedPtr<wxIconBundle>		IconBundle;
	ScopedPtr<wxBitmap>			Bitmap_Logo;

	pxAppResources();
	virtual ~pxAppResources() throw() { }
};

// --------------------------------------------------------------------------------------
//  RecentIsoList
// --------------------------------------------------------------------------------------
struct RecentIsoList
{
	ScopedPtr<RecentIsoManager>		Manager;
	ScopedPtr<wxMenu>				Menu;

	RecentIsoList();
	virtual ~RecentIsoList() throw() { }
};

// --------------------------------------------------------------------------------------
//  FramerateManager
// --------------------------------------------------------------------------------------
class FramerateManager
{
public:
	static const uint FramerateQueueDepth = 64;

protected:
	u64 m_fpsqueue[FramerateQueueDepth];
	int m_fpsqueue_writepos;
	uint m_initpause;

	uint m_FrameCounter;

public:
	FramerateManager() { Reset(); }
	virtual ~FramerateManager() throw() {}

	void Reset();
	void Resume();
	void DoFrame();
	double GetFramerate() const;
};

// =====================================================================================================
//  Pcsx2App  -  main wxApp class
// =====================================================================================================
class Pcsx2App : public wxAppWithHelpers
{
	typedef wxAppWithHelpers _parent;

	// ----------------------------------------------------------------------------
	// Event Sources!
	// These need to be at the top of the App class, because a lot of other things depend
	// on them and they are, themselves, fairly self-contained.

protected:
	EventSource<IEventListener_Plugins>		m_evtsrc_CorePluginStatus;
	EventSource<IEventListener_CoreThread>	m_evtsrc_CoreThreadStatus;
	EventSource<IEventListener_AppStatus>	m_evtsrc_AppStatus;

public:
	void AddListener( IEventListener_Plugins& listener )
	{
		m_evtsrc_CorePluginStatus.Add( listener );
	}

	void AddListener( IEventListener_CoreThread& listener )
	{
		m_evtsrc_CoreThreadStatus.Add( listener );
	}

	void AddListener( IEventListener_AppStatus& listener )
	{
		m_evtsrc_AppStatus.Add( listener );
	}

	void RemoveListener( IEventListener_Plugins& listener )
	{
		m_evtsrc_CorePluginStatus.Remove( listener );
	}

	void RemoveListener( IEventListener_CoreThread& listener )
	{
		m_evtsrc_CoreThreadStatus.Remove( listener );
	}

	void RemoveListener( IEventListener_AppStatus& listener )
	{
		m_evtsrc_AppStatus.Remove( listener );
	}

	void AddListener( IEventListener_Plugins* listener )
	{
		m_evtsrc_CorePluginStatus.Add( listener );
	}

	void AddListener( IEventListener_CoreThread* listener )
	{
		m_evtsrc_CoreThreadStatus.Add( listener );
	}

	void AddListener( IEventListener_AppStatus* listener )
	{
		m_evtsrc_AppStatus.Add( listener );
	}

	void RemoveListener( IEventListener_Plugins* listener )
	{
		m_evtsrc_CorePluginStatus.Remove( listener );
	}

	void RemoveListener( IEventListener_CoreThread* listener )
	{
		m_evtsrc_CoreThreadStatus.Remove( listener );
	}

	void RemoveListener( IEventListener_AppStatus* listener )
	{
		m_evtsrc_AppStatus.Remove( listener );
	}

	void DispatchEvent( PluginEventType evt )
	{
		if( !AffinityAssert_AllowFromMain() ) return;
		m_evtsrc_CorePluginStatus.Dispatch( evt );
	}

	void DispatchEvent( AppEventType evt )
	{
		if( !AffinityAssert_AllowFromMain() ) return;
		m_evtsrc_AppStatus.Dispatch( AppEventInfo( evt ) );
	}

	void DispatchEvent( IniInterface& ini )
	{
		if( !AffinityAssert_AllowFromMain() ) return;
		m_evtsrc_AppStatus.Dispatch( AppSettingsEventInfo( ini ) );
	}

	// ----------------------------------------------------------------------------

public:
	FramerateManager				FpsManager;
	CommandDictionary				GlobalCommands;
	AcceleratorDictionary			GlobalAccels;

protected:
	ScopedPtr<PipeRedirectionBase>	m_StdoutRedirHandle;
	ScopedPtr<PipeRedirectionBase>	m_StderrRedirHandle;

	ScopedPtr<RecentIsoList>		m_RecentIsoList;
	ScopedPtr<pxAppResources>		m_Resources;

public:
	ScopedPtr<SysCoreAllocations>	m_CoreAllocs;
	ScopedPtr<PluginManager>		m_CorePlugins;

protected:
	wxWindowID			m_id_MainFrame;
	wxWindowID			m_id_GsFrame;
	wxWindowID			m_id_ProgramLogBox;

	wxKeyEvent			m_kevt;

public:
	Pcsx2App();
	virtual ~Pcsx2App();

	void PostPluginStatus( PluginEventType pevt );
	void PostMenuAction( MenuIdentifiers menu_id ) const;
	int  IssueDialogAsModal( const wxString& dlgName );
	void PostMethod( FnPtr_AppMethod method );
	void PostIdleMethod( FnPtr_AppMethod method );
	int  DoStuckThread( PersistentThread& stuck_thread );

	void SysExecute();
	void SysExecute( CDVD_SourceType cdvdsrc, const wxString& elf_override=wxEmptyString );
	void SysReset();
	void ReloadPlugins();
	void LogicalVsync();

	GSFrame&		GetGsFrame() const;
	MainEmuFrame&	GetMainFrame() const;

	GSFrame*		GetGsFramePtr() const	{ return (GSFrame*)wxWindow::FindWindowById( m_id_GsFrame ); }
	MainEmuFrame*	GetMainFramePtr() const	{ return (MainEmuFrame*)wxWindow::FindWindowById( m_id_MainFrame ); }

	bool HasMainFrame() const	{ return GetMainFramePtr() != NULL; }

	void OpenGsPanel();
	void CloseGsPanel();
	void OnGsFrameClosed();
	void OnMainFrameClosed( wxWindowID id );

	// --------------------------------------------------------------------------
	//  Startup / Shutdown Helpers
	// --------------------------------------------------------------------------

	void DetectCpuAndUserMode();
	void OpenConsoleLog();
	void OpenMainFrame();
	void PrepForExit();
	void CleanupRestartable();
	void CleanupResources();
	void WipeUserModeSettings();
	void ReadUserModeSettings();


	// --------------------------------------------------------------------------
	//  App-wide Resources
	// --------------------------------------------------------------------------
	// All of these accessors cache the resources on first use and retain them in
	// memory until the program exits.

	wxMenu&				GetRecentIsoMenu();
	RecentIsoManager&	GetRecentIsoManager();

	pxAppResources&		GetResourceCache();
	const wxIconBundle&	GetIconBundle();
	const wxBitmap&		GetLogoBitmap();
	wxImageList&		GetImgList_Config();
	wxImageList&		GetImgList_Toolbars();

	const AppImageIds& GetImgId() const
	{
		return m_Resources->ImageId;
	}

	// --------------------------------------------------------------------------
	//  Overrides of wxApp virtuals:
	// --------------------------------------------------------------------------
	bool OnInit();
	int  OnExit();
	void CleanUp();

	void OnInitCmdLine( wxCmdLineParser& parser );
	bool OnCmdLineParsed( wxCmdLineParser& parser );
	bool OnCmdLineError( wxCmdLineParser& parser );

#ifdef __WXDEBUG__
	void OnAssertFailure( const wxChar *file, int line, const wxChar *func, const wxChar *cond, const wxChar *msg );
#endif

	// ----------------------------------------------------------------------------
	//   Console / Program Logging Helpers
	// ----------------------------------------------------------------------------
	ConsoleLogFrame* GetProgramLog();
	const ConsoleLogFrame* GetProgramLog() const;
	void ProgramLog_PostEvent( wxEvent& evt );
	void EnableAllLogging() const;
	void DisableWindowLogging() const;
	void DisableDiskLogging() const;
	void OnProgramLogClosed( wxWindowID id );

protected:
	bool InvokeMethodOnMainThread( FnPtr_AppMethod method );
	bool PostMethodToMainThread( FnPtr_AppMethod method );

	void AllocateCoreStuffs();
	void InitDefaultGlobalAccelerators();
	void BuildCommandHash();
	bool TryOpenConfigCwd();
	void CleanupOnExit();
	void OpenWizardConsole();
	void PadKeyDispatch( const keyEvent& ev );
	void CancelLoadingPlugins();

	void HandleEvent(wxEvtHandler* handler, wxEventFunction func, wxEvent& event) const;
	void HandleEvent(wxEvtHandler* handler, wxEventFunction func, wxEvent& event);

	void OnSysExecute( wxCommandEvent& evt );
	void OnLoadPluginsComplete( wxCommandEvent& evt );
	void OnPluginStatus( wxCommandEvent& evt );
	void OnCoreThreadStatus( wxCommandEvent& evt );
	void OnFreezeThreadFinished( wxCommandEvent& evt );

	void OnOpenModalDialog( wxCommandEvent& evt );
	void OnOpenDialog_StuckThread( wxCommandEvent& evt );

	void OnEmuKeyDown( wxKeyEvent& evt );

	void OnInvokeMethod( pxInvokeAppMethodEvent& evt );

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
//  AppCoreThread class
// --------------------------------------------------------------------------------------
class AppCoreThread : public SysCoreThread
{
	typedef SysCoreThread _parent;

public:
	AppCoreThread();
	virtual ~AppCoreThread() throw();

	virtual bool Suspend( bool isBlocking=true );
	virtual void Resume();
	virtual void Reset();
	virtual void Cancel( bool isBlocking=true );
	virtual void StateCheckInThread();
	virtual void ApplySettings( const Pcsx2Config& src );
	virtual void ChangeCdvdSource( CDVD_SourceType type );

protected:
	virtual void OnResumeReady();
	virtual void OnResumeInThread( bool IsSuspended );
	virtual void OnSuspendInThread();
	virtual void OnCleanupInThread();
	//virtual void VsyncInThread();
	virtual void PostVsyncToUI();
	virtual void ExecuteTaskInThread();
	virtual void DoCpuReset();

	virtual void DoThreadDeadlocked();

	virtual void CpuInitializeMess();
};

DECLARE_APP(Pcsx2App)

// --------------------------------------------------------------------------------------
//  s* macros!  ['s' stands for 'shortcut']
// --------------------------------------------------------------------------------------
// Use these for "silent fail" invocation of PCSX2 Application-related constructs.  If the
// construct (albeit wxApp, MainFrame, CoreThread, etc) is null, the requested method will
// not be invoked, and an optional "else" clause can be affixed for handling the end case.
//
// Usage Examples:
//   sMainFrame.ApplySettings();
//   sMainFrame.ApplySettings(); else Console.WriteLn( "Judge Wapner" );	// 'else' clause for handling NULL scenarios.
//
// Note!  These macros are not "syntax complete", which means they could generate unexpected
// syntax errors in some situations, and more importantly, they cannot be used for invoking
// functions with return values.
//
// Rationale: There are a lot of situations where we want to invoke a void-style method on
// various volatile object pointers (App, Corethread, MainFrame, etc).  Typically if these
// objects are NULL the most intuitive response is to simply ignore the call request and
// continue running silently.  These macros make that possible without any extra boilerplate
// conditionals or temp variable defines in the code.
//
#define sApp \
	if( Pcsx2App* __app_ = (Pcsx2App*)wxApp::GetInstance() ) (*__app_)

#define sLogFrame \
	if( ConsoleLogFrame* __conframe_ = wxGetApp().GetProgramLog() ) (*__conframe_)

#define sMainFrame \
	if( MainEmuFrame* __frame_ = GetMainFramePtr() ) (*__frame_)

#define sGSFrame \
	if( GSFrame* __gsframe_ = wxGetApp().GetGsFramePtr() ) (*__gsframe_)

// Use this within the scope of a wxWindow (wxDialog or wxFrame).  If the window has a valid menu
// bar, the command will run, otherwise it will be silently ignored. :)
#define sMenuBar \
	if( wxMenuBar* __menubar_ = GetMenuBar() ) (*__menubar_)

// --------------------------------------------------------------------------------------
//  AppOpenDialog
// --------------------------------------------------------------------------------------
template<typename DialogType>
void AppOpenDialog( wxWindow* parent )
{
	if( wxWindow* window = wxFindWindowByName( DialogType::GetNameStatic() ) )
		window->SetFocus();
	else
		(new DialogType( parent ))->Show();
}

// --------------------------------------------------------------------------------------
//  SaveSinglePluginHelper
// --------------------------------------------------------------------------------------
// A scoped convenience class for closing a single plugin and saving its state to memory.
// Emulation is suspended as needed, and is restored when the object leaves scope.  Within
// the scope of the object, code is free to call plugin re-configurations or even unload
// a plugin entirely and re-load a different plugin in its place.
//
class SaveSinglePluginHelper
{
protected:
	SafeArray<u8>			m_plugstore;
	const SafeArray<u8>*	m_whereitsat;

	bool					m_resume;
	bool					m_validstate;
	PluginsEnum_t			m_pid;

public:
	SaveSinglePluginHelper( PluginsEnum_t pid );
	virtual ~SaveSinglePluginHelper() throw();
};


extern pxDoAssertFnType AppDoAssert;

// --------------------------------------------------------------------------------------
//  External App-related Globals and Shortcuts
// --------------------------------------------------------------------------------------

extern int  EnumeratePluginsInFolder( const wxDirName& searchPath, wxArrayString* dest );
extern void LoadPluginsPassive( FnType_OnThreadComplete* onComplete );
extern void LoadPluginsImmediate();
extern void UnloadPlugins();

extern void AppLoadSettings();
extern void AppSaveSettings();
extern void AppApplySettings( const AppConfig* oldconf=NULL );

extern bool SysHasValidState();
extern void SysUpdateIsoSrcFile( const wxString& newIsoFile );
extern void SysStatus( const wxString& text );

extern bool				HasMainFrame();
extern MainEmuFrame&	GetMainFrame();
extern MainEmuFrame*	GetMainFramePtr();

extern __aligned16 AppCoreThread CoreThread;
extern __aligned16 SysMtgsThread mtgsThread;


