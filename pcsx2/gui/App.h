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
class ConsoleLogFrame;
class PipeRedirectionBase;
class AppCoreThread;

#include "Utilities/HashMap.h"
#include "Utilities/wxGuiTools.h"

#include "AppConfig.h"
#include "System.h"
#include "System/SysThreads.h"

#define AllowFromMainThreadOnly() \
	pxAssertMsg( wxThread::IsMain(), "Thread affinity violation: Call allowed from main thread only." )

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( pxEVT_SemaphorePing, -1 )
	DECLARE_EVENT_TYPE( pxEVT_OpenModalDialog, -1 )
	DECLARE_EVENT_TYPE( pxEVT_ReloadPlugins, -1 )
	DECLARE_EVENT_TYPE( pxEVT_LoadPluginsComplete, -1 )
	DECLARE_EVENT_TYPE( pxEVT_AppCoreThread_Terminated, -1 )
	DECLARE_EVENT_TYPE( pxEVT_FreezeFinished, -1 )
	DECLARE_EVENT_TYPE( pxEVT_ThawFinished, -1 )
END_DECLARE_EVENT_TYPES()

// This is used when the GS plugin is handling its own window.  Messages from the PAD
// are piped through to an app-level message handler, which dispatches them through
// the universal Accelerator table.
static const int pxID_PadHandler_Keydown = 8030;


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
	MenuId_Config_ResetAll,
};

enum DialogIdentifiers
{
	DialogId_CoreSettings = 0x800,
	DialogId_BiosSelector,
	DialogId_LogOptions,
	DialogId_About,
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
public:
	CommandDictionary				GlobalCommands;
	AcceleratorDictionary			GlobalAccels;

protected:
	wxImageList						m_ConfigImages;

	ScopedPtr<wxImageList>			m_ToolbarImages;
	ScopedPtr<wxBitmap>				m_Bitmap_Logo;
	ScopedPtr<PipeRedirectionBase>	m_StdoutRedirHandle;
	ScopedPtr<PipeRedirectionBase>	m_StderrRedirHandle;

public:
	ScopedPtr<SysCoreAllocations>	m_CoreAllocs;
	ScopedPtr<PluginManager>		m_CorePlugins;
	ScopedPtr<SysCoreThread>		m_CoreThread;

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

	void PostPadKey( wxKeyEvent& evt );
	void PostMenuAction( MenuIdentifiers menu_id ) const;
	int  ThreadedModalDialog( DialogIdentifiers dialogId );
	void Ping() const;

	bool PrepForExit();

	void SysExecute();
	void SysExecute( CDVD_SourceType cdvdsrc );
	void SysReset();

	const wxBitmap& GetLogoBitmap();
	wxImageList& GetImgList_Config();
	wxImageList& GetImgList_Toolbars();

	const AppImageIds& GetImgId() const { return m_ImageId; }

	MainEmuFrame&	GetMainFrame() const;
	SysCoreThread&	GetCoreThread() const;

	bool HasMainFrame() const	{ return m_MainFrame != NULL; }
	bool HasCoreThread() const	{ return m_CoreThread != NULL; }

	MainEmuFrame*	GetMainFramePtr() const		{ return m_MainFrame; }
	SysCoreThread*	GetCoreThreadPtr() const	{ return m_CoreThread; }

	void OpenGsFrame();
	void OnGsFrameClosed();
	void OnMainFrameClosed();
	
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
	void ProgramLog_CountMsg();
	void ProgramLog_PostEvent( wxEvent& evt );
	void EnableConsoleLogging() const;
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

	void HandleEvent(wxEvtHandler *handler, wxEventFunction func, wxEvent& event) const;

	void OnReloadPlugins( wxCommandEvent& evt );
	void OnLoadPluginsComplete( wxCommandEvent& evt );
	void OnSemaphorePing( wxCommandEvent& evt );
	void OnOpenModalDialog( wxCommandEvent& evt );
	void OnCoreThreadTerminated( wxCommandEvent& evt );

	void OnFreezeFinished( wxCommandEvent& evt );
	void OnThawFinished( wxCommandEvent& evt );

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
//  AppCoreThread class
// --------------------------------------------------------------------------------------

class AppCoreThread : public SysCoreThread
{
	typedef SysCoreThread _parent;

protected:
	wxKeyEvent		m_kevt;

public:
	AppCoreThread( PluginManager& plugins );
	virtual ~AppCoreThread() throw();

	virtual void Suspend( bool isBlocking=true );
	virtual void Resume();
	virtual void StateCheck( bool isCancelable=true );
	virtual void ApplySettings( const Pcsx2Config& src );

protected:
	virtual void OnResumeReady();
	virtual void OnThreadCleanup();
	virtual void ExecuteTask();
};

DECLARE_APP(Pcsx2App)

// --------------------------------------------------------------------------------------
//  s* macros!  ['s' stands for 'shortcut']
// --------------------------------------------------------------------------------------
// Use these for "silent fail" invocation of PCSX2 Application-related constructs.  If the
// construct (albeit wxApp, MainFrame, CoreThread, etc) is null, the requested method will
// not be invoked, and an optional "else" clause cn be affixed for handling the end case.
//
// Usage Examples:
//   sCoreThread.Suspend();		// Suspends the CoreThread, or does nothing if the CoreThread handle is NULL
//   sCoreThread.Suspend(); else Console.WriteLn( "Judge Wapner" );	// 'else' clause for handling NULL scenarios.
//
// Note!  These macros are not "syntax complete", which means they could generat unexpected
// syntax errors in some situatins, and more importantly they cannot be used for invoking 
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

#define sCoreThread \
	if( SysCoreThread* __thread_ = GetCoreThreadPtr() ) (*__thread_)

#define sMainFrame \
	if( MainEmuFrame* __frame_ = GetMainFramePtr() ) (*__frame_)


// --------------------------------------------------------------------------------------
//  External App-related Globals and Shortcuts
// --------------------------------------------------------------------------------------

extern bool sys_resume_lock;

extern int EnumeratePluginsInFolder( const wxDirName& searchPath, wxArrayString* dest );
extern void LoadPluginsPassive();
extern void LoadPluginsImmediate();
extern void UnloadPlugins();

extern void AppLoadSettings();
extern void AppSaveSettings();
extern void AppApplySettings( const AppConfig* oldconf=NULL );

extern bool SysHasValidState();

extern void SysStatus( const wxString& text );

extern bool HasMainFrame();
extern bool HasCoreThread();

extern MainEmuFrame&	GetMainFrame();
extern SysCoreThread&	GetCoreThread();

extern MainEmuFrame*	GetMainFramePtr();
extern SysCoreThread*	GetCoreThreadPtr();
