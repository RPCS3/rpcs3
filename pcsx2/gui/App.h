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
#include <wx/apptrait.h>

#include "IniInterface.h"

#include "Utilities/HashMap.h"

#include "AppCommon.h"
#include "RecentIsoList.h"

#include "System.h"
#include "System/SysThreads.h"

typedef void FnType_OnThreadComplete(const wxCommandEvent& evt);

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( pxEVT_OpenModalDialog, -1 )
	DECLARE_EVENT_TYPE( pxEVT_ReloadPlugins, -1 )
	DECLARE_EVENT_TYPE( pxEVT_SysExecute, -1 )
	DECLARE_EVENT_TYPE( pxEVT_LoadPluginsComplete, -1 )
	DECLARE_EVENT_TYPE( pxEVT_CoreThreadStatus, -1 )
	DECLARE_EVENT_TYPE( pxEVT_FreezeThreadFinished, -1 )
	DECLARE_EVENT_TYPE( pxEVT_Ping, -1 )
END_DECLARE_EVENT_TYPES()

// This is used when the GS plugin is handling its own window.  Messages from the PAD
// are piped through to an app-level message handler, which dispatches them through
// the universal Accelerator table.
static const int pxID_PadHandler_Keydown = 8030;

// Plugin ID sections are spaced out evenly at intervals to make it easy to use a
// single for-loop to create them.
static const int PluginMenuId_Interval = 0x10;

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


	MenuId_Sys_SuspendResume,			// suspends/resumes active emulation, retains plugin states
	MenuId_Sys_Close,			// Closes the emulator (states are preserved)
	MenuId_Sys_Reset,			// Issues a complete reset (wipes preserved states)
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
			Video,
			Cpu;

		ConfigIds()
		{
			Paths		= Plugins	=
			Speedhacks	= Gamefixes	=
			Video		= Cpu		= -1;
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

	ScopedPtr<wxMenu>			RecentIsoMenu;
	ScopedPtr<RecentIsoManager>	RecentIsoList;

	pxAppResources();
	~pxAppResources() throw() { }
};

struct MsgboxEventResult
{
	Semaphore	WaitForMe;
	int			result;

	MsgboxEventResult()
	{
		result = 0;
	}
};

// --------------------------------------------------------------------------------------
//  AppIniSaver / AppIniLoader
// --------------------------------------------------------------------------------------
class AppIniSaver : public IniSaver
{
public:
	AppIniSaver();
	virtual ~AppIniSaver() {}
};

class AppIniLoader : public IniLoader
{
public:
	AppIniLoader();
	virtual ~AppIniLoader() {}
};

// =====================================================================================================
//  Pcsx2App  -  main wxApp class
// =====================================================================================================
class Pcsx2App : public wxApp
{
	// ----------------------------------------------------------------------------
	// Event Sources!
	// These need to be at the top of the App class, because a lot of other things depend
	// on them and they are, themselves, fairly self-contained.

protected:
	EventSource<PluginEventType>m_evtsrc_CorePluginStatus;
	CmdEvt_Source				m_evtsrc_CoreThreadStatus;
	EventSource<int>			m_evtsrc_SettingsApplied;
	EventSource<IniInterface>	m_evtsrc_SettingsLoadSave;
	EventSource<AppEventType>	m_evtsrc_AppStatus;

public:
	CmdEvt_Source& Source_CoreThreadStatus()		{ return m_evtsrc_CoreThreadStatus; }
	EventSource<int>& Source_SettingsApplied()		{ return m_evtsrc_SettingsApplied; }
	EventSource<AppEventType>& Source_AppStatus()	{ return m_evtsrc_AppStatus; }
	EventSource<PluginEventType>& Source_CorePluginStatus()	{ return m_evtsrc_CorePluginStatus; }
	EventSource<IniInterface>& Source_SettingsLoadSave()	{ return m_evtsrc_SettingsLoadSave; }
	// ----------------------------------------------------------------------------
	
public:
	CommandDictionary				GlobalCommands;
	AcceleratorDictionary			GlobalAccels;

protected:
	ScopedPtr<PipeRedirectionBase>	m_StdoutRedirHandle;
	ScopedPtr<PipeRedirectionBase>	m_StderrRedirHandle;
	ScopedPtr<pxAppResources>		m_Resources;

public:
	ScopedPtr<SysCoreAllocations>	m_CoreAllocs;
	ScopedPtr<PluginManager>		m_CorePlugins;

protected:
	// Note: Pointers to frames should not be scoped because wxWidgets handles deletion
	// of these objects internally.
	MainEmuFrame*				m_MainFrame;
	GSFrame*					m_gsFrame;
	ConsoleLogFrame*			m_ProgramLogBox;

	std::vector<Semaphore*>		m_PingWhenIdle;

public:
	Pcsx2App();
	virtual ~Pcsx2App();

	void Ping();

	void PostPadKey( wxKeyEvent& evt );
	void PostMenuAction( MenuIdentifiers menu_id ) const;
	int  IssueModalDialog( const wxString& dlgName );

	bool PrepForExit( bool canCancel );

	void SysExecute();
	void SysExecute( CDVD_SourceType cdvdsrc, const wxString& elf_override=wxEmptyString );
	void SysReset();

	GSFrame&		GetGSFrame() const;
	GSFrame*		GetGSFramePtr() const	{ return m_gsFrame; }

	MainEmuFrame&	GetMainFrame() const;
	MainEmuFrame*	GetMainFramePtr() const	{ return m_MainFrame; }
	bool HasMainFrame() const	{ return m_MainFrame != NULL; }

	void OpenGsFrame();
	void OnGsFrameClosed();
	void OnMainFrameClosed();

	// --------------------------------------------------------------------------
	//  App-wide Resources
	// --------------------------------------------------------------------------
	// All of these accessors cache the resources on first use and retain them in
	// memory until the program exits.

	wxMenu&				GetRecentIsoMenu();
	RecentIsoManager&	GetRecentIsoList();
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
	void ProgramLog_PostEvent( wxEvent& evt );
	void EnableAllLogging() const;
	void DisableWindowLogging() const;
	void DisableDiskLogging() const;
	void OnProgramLogClosed();

protected:
	void InitDefaultGlobalAccelerators();
	void BuildCommandHash();
	void ReadUserModeSettings();
	bool TryOpenConfigCwd();
	void CleanupMess();
	void OpenWizardConsole();
	void PingDispatch( const char* action );
	
	void HandleEvent(wxEvtHandler* handler, wxEventFunction func, wxEvent& event) const;
	void HandleEvent(wxEvtHandler* handler, wxEventFunction func, wxEvent& event);

	void OnSysExecute( wxCommandEvent& evt );
	void OnReloadPlugins( wxCommandEvent& evt );
	void OnLoadPluginsComplete( wxCommandEvent& evt );
	void OnOpenModalDialog( wxCommandEvent& evt );
	void OnCoreThreadStatus( wxCommandEvent& evt );

	void OnFreezeThreadFinished( wxCommandEvent& evt );

	void OnMessageBox( pxMessageBoxEvent& evt );
	void OnEmuKeyDown( wxKeyEvent& evt );

	void OnIdleEvent( wxIdleEvent& evt );
	void OnPingEvent( pxPingEvent& evt );

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

enum CoreThreadStatus
{
	CoreStatus_Indeterminate,
	CoreStatus_Resumed,
	CoreStatus_Suspended,
	CoreStatus_Reset,
	CoreStatus_Stopped,
};

class AppCoreThread : public SysCoreThread
{
	typedef SysCoreThread _parent;

protected:
	wxKeyEvent		m_kevt;

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
	virtual void DispatchKeyEventToUI( const keyEvent& ev );
	virtual void ExecuteTaskInThread();
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
	if( GSFrame* __gsframe_ = wxGetApp().GetGSFramePtr() ) (*__gsframe_)

// Use this within the scope of a wxWindow (wxDialog or wxFrame).  If the window has a valid menu
// bar, the command will run, otherwise it will be silently ignored. :)
#define sMenuBar \
	if( wxMenuBar* __menubar_ = GetMenuBar() ) (*__menubar_)

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


