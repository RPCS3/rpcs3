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

#pragma once

#include "Utilities/wxAppWithHelpers.h"

#include <wx/fileconf.h>
#include <wx/imaglist.h>
#include <wx/apptrait.h>

#include "pxEventThread.h"

#include "AppCommon.h"
#include "AppCoreThread.h"
#include "RecentIsoList.h"
#include "AppGameDatabase.h"

#include "System.h"
#include "System/SysThreads.h"

#include "Utilities/HashMap.h"

class Pcsx2App;

typedef void FnType_OnThreadComplete(const wxCommandEvent& evt);
typedef void (Pcsx2App::*FnPtr_Pcsx2App)();

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( pxEvt_LoadPluginsComplete, -1 )
	DECLARE_EVENT_TYPE( pxEvt_LogicalVsync, -1 )
	DECLARE_EVENT_TYPE( pxEvt_ThreadTaskTimeout_SysExec, -1 )
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
	MenuId_Boot_CDVD,
	MenuId_Boot_CDVD2,
	MenuId_Boot_ELF,
	MenuId_Boot_Recent,			// Menu populated with recent source bootings


	MenuId_Sys_SuspendResume,	// suspends/resumes active emulation, retains plugin states
	MenuId_Sys_Restart,			// Issues a complete VM reset (wipes preserved states)
	MenuId_Sys_Shutdown,		// Closes virtual machine, shuts down plugins, wipes states.
	MenuId_Sys_LoadStates,		// Opens load states submenu
	MenuId_Sys_SaveStates,		// Opens save states submenu
	MenuId_EnablePatches,
	MenuId_EnableCheats,
	MenuId_EnableHostFs,

	MenuId_State_Load,
	MenuId_State_LoadOther,
	MenuId_State_Load01,		// first of many load slots
	MenuId_State_Save = MenuId_State_Load01+20,
	MenuId_State_SaveOther,
	MenuId_State_Save01,		// first of many save slots

	MenuId_State_EndSlotSection = MenuId_State_Save01+20,

	// Config Subsection
	MenuId_Config_SysSettings,
	MenuId_Config_McdSettings,
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
	MenuId_Video_WindowSettings,

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
	ScopedPtr<AppGameDatabase>	GameDB;

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

class StartupOptions
{
public:
	bool			ForceWizard;
	bool			ForceConsole;

	// Disables the fast boot option when auto-running games.  This option only applies
	// if SysAutoRun is also true.
	bool			NoFastBoot;

	// Specifies the Iso file to boot; used only if SysAutoRun is enabled and CdvdSource
	// is set to ISO.
	wxString		IsoFile;

	// Specifies the CDVD source type to use when AutoRunning
	CDVD_SourceType CdvdSource;

	// Indicates if PCSX2 should autorun the configured CDVD source and/or ISO file.
	bool			SysAutoRun;

	StartupOptions()
	{
		ForceWizard				= false;
		NoFastBoot				= false;
		ForceConsole			= false;
		SysAutoRun				= false;
		CdvdSource				= CDVDsrc_NoDisc;
	}
};

enum GsWindowMode_t
{
	GsWinMode_Unspecified = 0,
	GsWinMode_Windowed,
	GsWinMode_Fullscreen,
};

class CommandlineOverrides
{
public:
	AppConfig::FilenameOptions	Filenames;
	wxDirName		SettingsFolder;
	wxFileName		SettingsFile;

	bool			DisableSpeedhacks;

	// Note that gamefixes in this array should only be honored if the
	// "HasCustomGamefixes" boolean is also enabled.
	bool			UseGamefix[GamefixId_COUNT];
	bool			ApplyCustomGamefixes;

	GsWindowMode_t	GsWindowMode;

public:
	CommandlineOverrides()
	{
		DisableSpeedhacks		= false;
		ApplyCustomGamefixes	= false;
		GsWindowMode			= GsWinMode_Unspecified;
	}
	
	// Returns TRUE if either speedhacks or gamefixes are being overridden.
	bool HasCustomHacks() const
	{
		return DisableSpeedhacks || ApplyCustomGamefixes;
	}

	bool HasSettingsOverride() const
	{
		return SettingsFolder.IsOk() || SettingsFile.IsOk();
	}

	bool HasPluginsOverride() const
	{
		for( int i=0; i<PluginId_Count; ++i )
			if( Filenames.Plugins[i].IsOk() ) return true;

		return false;
	}
};

// --------------------------------------------------------------------------------------
//  Pcsx2AppTraits
// --------------------------------------------------------------------------------------
// Overrides and customizes some default wxWidgets behaviors.  This class is instanized by
// calls to Pcsx2App::CreateTraits(), which is called from wxWidgets as-needed.  wxWidgets
// does cache an instance of the traits, so the object construction need not be trivial
// (translation: it can be complicated-ish -- it won't affect performance).
//
class Pcsx2AppTraits : public wxGUIAppTraits
{
	typedef wxGUIAppTraits _parent;

public:
	virtual ~Pcsx2AppTraits() {}
	wxMessageOutput* CreateMessageOutput();
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
	
	void DispatchEvent( PluginEventType evt );
	void DispatchEvent( AppEventType evt );
	void DispatchEvent( CoreThreadStatus evt );
	void DispatchEvent( IniInterface& ini );

	// ----------------------------------------------------------------------------
protected:
	int								m_PendingSaves;
	bool							m_ScheduledTermination;
	bool							m_UseGUI;
	
public:
	FramerateManager				FpsManager;
	CommandDictionary				GlobalCommands;
	AcceleratorDictionary			GlobalAccels;

	StartupOptions					Startup;
	CommandlineOverrides			Overrides;

	ScopedPtr<wxTimer>				m_timer_Termination;

protected:
	ScopedPtr<PipeRedirectionBase>	m_StdoutRedirHandle;
	ScopedPtr<PipeRedirectionBase>	m_StderrRedirHandle;

	ScopedPtr<RecentIsoList>		m_RecentIsoList;
	ScopedPtr<pxAppResources>		m_Resources;

	Threading::Mutex				m_mtx_Resources;
	Threading::Mutex				m_mtx_LoadingGameDB;

public:
	// Executor Thread for complex VM/System tasks.  This thread is used to execute such tasks
	// in parallel to the main message pump, to allow the main pump to run without fear of
	// blocked threads stalling the GUI.
	ExecutorThread					SysExecutorThread;
	ScopedPtr<SysCoreAllocations>	m_CoreAllocs;

protected:
	wxWindowID			m_id_MainFrame;
	wxWindowID			m_id_GsFrame;
	wxWindowID			m_id_ProgramLogBox;

	wxKeyEvent			m_kevt;

public:
	Pcsx2App();
	virtual ~Pcsx2App();

	void PostMenuAction( MenuIdentifiers menu_id ) const;
	void PostAppMethod( FnPtr_Pcsx2App method );
	void PostIdleAppMethod( FnPtr_Pcsx2App method );

	void SysApplySettings();
	void SysExecute();
	void SysExecute( CDVD_SourceType cdvdsrc, const wxString& elf_override=wxEmptyString );
	void SysShutdown();
	void LogicalVsync();
	
	GSFrame&		GetGsFrame() const;
	MainEmuFrame&	GetMainFrame() const;

	GSFrame*		GetGsFramePtr() const	{ return (GSFrame*)wxWindow::FindWindowById( m_id_GsFrame ); }
	MainEmuFrame*	GetMainFramePtr() const	{ return (MainEmuFrame*)wxWindow::FindWindowById( m_id_MainFrame ); }

	bool HasMainFrame() const	{ return GetMainFramePtr() != NULL; }

	void OpenGsPanel();
	void CloseGsPanel();
	void OnGsFrameClosed( wxWindowID id );
	void OnMainFrameClosed( wxWindowID id );

	// --------------------------------------------------------------------------
	//  Startup / Shutdown Helpers
	// --------------------------------------------------------------------------

	void DetectCpuAndUserMode();
	void OpenProgramLog();
	void OpenMainFrame();
	void PrepForExit();
	void CleanupRestartable();
	void CleanupResources();
	void WipeUserModeSettings();
	void ReadUserModeSettings();

	void StartPendingSave();
	void ClearPendingSave();

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
	
	AppGameDatabase* GetGameDatabase();

	// --------------------------------------------------------------------------
	//  Overrides of wxApp virtuals:
	// --------------------------------------------------------------------------
	wxAppTraits* CreateTraits();
	bool OnInit();
	int  OnExit();
	void CleanUp();

	void OnInitCmdLine( wxCmdLineParser& parser );
	bool OnCmdLineParsed( wxCmdLineParser& parser );
	bool OnCmdLineError( wxCmdLineParser& parser );
	bool ParseOverrides( wxCmdLineParser& parser );

#ifdef __WXDEBUG__
	void OnAssertFailure( const wxChar *file, int line, const wxChar *func, const wxChar *cond, const wxChar *msg );
#endif

	Threading::MutexRecursive	m_mtx_ProgramLog;
	ConsoleLogFrame*			m_ptr_ProgramLog;

	// ----------------------------------------------------------------------------
	//   Console / Program Logging Helpers
	// ----------------------------------------------------------------------------
	ConsoleLogFrame* GetProgramLog();
	const ConsoleLogFrame* GetProgramLog() const;
	void ProgramLog_PostEvent( wxEvent& evt );
	Threading::Mutex& GetProgramLogLock();

	void EnableAllLogging();
	void DisableWindowLogging() const;
	void DisableDiskLogging() const;
	void OnProgramLogClosed( wxWindowID id );

protected:
	bool AppRpc_TryInvoke( FnPtr_Pcsx2App method );
	bool AppRpc_TryInvokeAsync( FnPtr_Pcsx2App method );

	void AllocateCoreStuffs();
	void InitDefaultGlobalAccelerators();
	void BuildCommandHash();
	bool TryOpenConfigCwd();
	void CleanupOnExit();
	void OpenWizardConsole();
	void PadKeyDispatch( const keyEvent& ev );
	
	void HandleEvent(wxEvtHandler* handler, wxEventFunction func, wxEvent& event) const;
	void HandleEvent(wxEvtHandler* handler, wxEventFunction func, wxEvent& event);

	void OnScheduledTermination( wxTimerEvent& evt );
	void OnEmuKeyDown( wxKeyEvent& evt );
	void OnSysExecutorTaskTimeout( wxTimerEvent& evt );
	void OnDestroyWindow( wxWindowDestroyEvent& evt );

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

// Use this within the scope of a wxWindow (wxDialog or wxFrame).  If the window has a valid menu
// bar, the command will run, otherwise it will be silently ignored. :)
#define sMenuBar \
	if( wxMenuBar* __menubar_ = GetMenuBar() ) (*__menubar_)

// --------------------------------------------------------------------------------------
//  AppOpenDialog
// --------------------------------------------------------------------------------------
// Returns a wxWindow handle to the opened window.
//
template<typename DialogType>
wxWindow* AppOpenDialog( wxWindow* parent=NULL )
{
	wxWindow* window = wxFindWindowByName( L"Dialog:" + DialogType::GetNameStatic() );
	
	if( !window ) window = new DialogType( parent );

	window->Show();
	window->SetFocus();
	return window;
}

extern pxDoAssertFnType AppDoAssert;

// --------------------------------------------------------------------------------------
//  External App-related Globals and Shortcuts
// --------------------------------------------------------------------------------------

extern int  EnumeratePluginsInFolder( const wxDirName& searchPath, wxArrayString* dest );
extern void LoadPluginsPassive();
extern void LoadPluginsImmediate();
extern void UnloadPlugins();
extern void ShutdownPlugins();

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
extern __aligned16 AppPluginManager CorePlugins;


extern void UI_UpdateSysControls();

extern void UI_DisableStateActions();
extern void UI_EnableStateActions();

extern void UI_DisableSysActions();
extern void UI_EnableSysActions();

extern void UI_DisableSysReset();
extern void UI_DisableSysShutdown();


#define AffinityAssert_AllowFrom_SysExecutor() \
	pxAssertMsg( wxGetApp().SysExecutorThread.IsSelf(), "Thread affinity violation: Call allowed from SysExecutor thread only." )

#define AffinityAssert_DisallowFrom_SysExecutor() \
	pxAssertMsg( !wxGetApp().SysExecutorThread.IsSelf(), "Thread affinity violation: Call is *not* allowed from SysExecutor thread." )

extern ExecutorThread& GetSysExecutorThread();
