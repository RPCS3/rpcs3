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

#include "PrecompiledHeader.h"
#include "App.h"
#include "AppSaveStates.h"
#include "GSFrame.h"

#include <wx/dir.h>
#include <wx/file.h>

#include "Plugins.h"
#include "GS.h"
#include "AppConfig.h"

using namespace Threading;

// The GS plugin needs to be opened to save/load the state during plugin configuration, but
// the window shouldn't. This blocks it. :)
static bool s_DisableGsWindow = false;

__aligned16 AppCorePlugins CorePlugins;

SysCorePlugins& GetCorePlugins()
{
	return CorePlugins;
}


// --------------------------------------------------------------------------------------
//  CorePluginsEvent
// --------------------------------------------------------------------------------------
class CorePluginsEvent : public pxActionEvent
{
	typedef pxActionEvent _parent;

protected:
	PluginEventType		m_evt;

public:
	virtual ~CorePluginsEvent() throw() {}
	CorePluginsEvent* Clone() const { return new CorePluginsEvent( *this ); }

	explicit CorePluginsEvent( PluginEventType evt, SynchronousActionState* sema=NULL )
		: pxActionEvent( sema )
	{
		m_evt = evt;
	}

	explicit CorePluginsEvent( PluginEventType evt, SynchronousActionState& sema )
		: pxActionEvent( sema )
	{
		m_evt = evt;
	}

	CorePluginsEvent( const CorePluginsEvent& src )
		: pxActionEvent( src )
	{
		m_evt = src.m_evt;
	}
	
	void SetEventType( PluginEventType evt ) { m_evt = evt; }
	PluginEventType GetEventType() { return m_evt; }

protected:
	void InvokeEvent()
	{
		sApp.DispatchEvent( m_evt );
	}
};

static void PostPluginStatus( PluginEventType pevt )
{
	sApp.PostAction( CorePluginsEvent( pevt ) );
}

static void ConvertPluginFilenames( wxString (&passins)[PluginId_Count] )
{
	const PluginInfo* pi = tbl_PluginInfo; do
	{
		passins[pi->id] = wxGetApp().Overrides.Filenames[pi->id].GetFullPath();

		if( passins[pi->id].IsEmpty() || !wxFileExists( passins[pi->id] ) )
			passins[pi->id] = g_Conf->FullpathTo( pi->id );
	} while( ++pi, pi->shortname != NULL );
}

typedef void (AppCorePlugins::*FnPtr_AppPluginManager)();
typedef void (AppCorePlugins::*FnPtr_AppPluginPid)( PluginsEnum_t pid );

// --------------------------------------------------------------------------------------
//  SysExecEvent_AppPluginManager
// --------------------------------------------------------------------------------------
class SysExecEvent_AppPluginManager : public SysExecEvent
{
protected:
	FnPtr_AppPluginManager	m_method;
	
public:
	wxString GetEventName() const { return L"CorePluginsMethod"; }
	virtual ~SysExecEvent_AppPluginManager() throw() {}
	SysExecEvent_AppPluginManager* Clone() const { return new SysExecEvent_AppPluginManager( *this ); }

	SysExecEvent_AppPluginManager( FnPtr_AppPluginManager method )
	{
		m_method = method;
	}
	
protected:
	void InvokeEvent()
	{
		if( m_method ) (CorePlugins.*m_method)();
	}
};

// --------------------------------------------------------------------------------------
//  LoadSinglePluginEvent
// --------------------------------------------------------------------------------------
class LoadSinglePluginEvent : public pxActionEvent
{
	typedef pxActionEvent _parent;
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN(LoadSinglePluginEvent)

protected:
	wxString		m_filename;
	PluginsEnum_t	m_pid;

public:
	virtual ~LoadSinglePluginEvent() throw() { }
	virtual LoadSinglePluginEvent *Clone() const { return new LoadSinglePluginEvent(*this); }
	
	LoadSinglePluginEvent( PluginsEnum_t pid = PluginId_GS, const wxString& filename=wxEmptyString )
		: m_filename( filename )
	{
		m_pid = pid;
	}

protected:
	void InvokeEvent()
	{
		GetCorePlugins().Load( m_pid, m_filename );
	}
};

// --------------------------------------------------------------------------------------
//  SinglePluginMethodEvent
// --------------------------------------------------------------------------------------
class SinglePluginMethodEvent : public pxActionEvent
{
	typedef pxActionEvent _parent;
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN(SinglePluginMethodEvent)

protected:
	PluginsEnum_t			m_pid;
	FnPtr_AppPluginPid		m_method;

public:
	virtual ~SinglePluginMethodEvent() throw() { }
	virtual SinglePluginMethodEvent *Clone() const { return new SinglePluginMethodEvent(*this); }
	
	SinglePluginMethodEvent( FnPtr_AppPluginPid method=NULL,  PluginsEnum_t pid = PluginId_GS )
	{
		m_pid		= pid;
		m_method	= method;
	}
	
protected:
	void InvokeEvent()
	{
		//GetCorePlugins().Unload( m_pid );
		if( m_method ) (CorePlugins.*m_method)( m_pid );
	}
};

IMPLEMENT_DYNAMIC_CLASS( LoadSinglePluginEvent,	 pxActionEvent );
IMPLEMENT_DYNAMIC_CLASS( SinglePluginMethodEvent, pxActionEvent );

// --------------------------------------------------------------------------------------
//  AppCorePlugins
// --------------------------------------------------------------------------------------
//
// Thread Affinity Notes:
//  It's important to ensure that Load/Unload/Init/Shutdown are all called from the
//  MAIN/UI Thread only.  Those APIs are allowed to issue modal popups, and as such
//  are only safe when invoked form the UI thread.  Under windows the popups themselves
//  will typically work from any thread, but some common control activities will fail
//  (such as opening the browser windows).  On Linux it's probably just highly unsafe, period.
//
//  My implementation is to execute the main Load/Init/Shutdown/Unload procedure on the
//  SysExecutor, and then dispatch each individual plugin to the main thread.  This keeps
//  the main thread from being completely busy while plugins are loaded and initialized.
//  (responsiveness is bliss!!) -- air
//
AppCorePlugins::AppCorePlugins()
{
}

AppCorePlugins::~AppCorePlugins() throw()
{
}

static void _SetSettingsFolder()
{
	if (wxGetApp().Rpc_TryInvoke( _SetSettingsFolder )) return;
	CorePlugins.SetSettingsFolder( GetSettingsFolder().ToString() );
}

static void _SetLogFolder()
{
	if (wxGetApp().Rpc_TryInvoke( _SetLogFolder )) return;
	CorePlugins.SetLogFolder( GetLogFolder().ToString() );
}

void AppCorePlugins::Load( PluginsEnum_t pid, const wxString& srcfile )
{ 
	if( !wxThread::IsMain() )
	{
		LoadSinglePluginEvent evt( pid, srcfile );
		wxGetApp().ProcessAction( evt);
		Sleep( 5 );
		return;
	}
	
	_parent::Load( pid, srcfile );
}

void AppCorePlugins::Unload( PluginsEnum_t pid )
{
	if( !wxThread::IsMain() )
	{
		SinglePluginMethodEvent evt( &AppCorePlugins::Unload, pid );
		wxGetApp().ProcessAction( evt );
		Sleep( 5 );
		return;
	}

	_parent::Unload( pid );
}

void AppCorePlugins::Load( const wxString (&folders)[PluginId_Count] )
{
	if( !pxAssert(!AreLoaded()) ) return;

	_SetLogFolder();
	SendLogFolder();

	_SetSettingsFolder();
	SendSettingsFolder();

	_parent::Load( folders );
	PostPluginStatus( CorePlugins_Loaded );
}

void AppCorePlugins::Unload()
{
	_parent::Unload();
	PostPluginStatus( CorePlugins_Unloaded );
}

void AppCorePlugins::Init( PluginsEnum_t pid )
{
	if( !wxTheApp ) return;

	if( !wxThread::IsMain() )
	{
		SinglePluginMethodEvent evt(&AppCorePlugins::Init, pid);
		wxGetApp().ProcessAction( evt );
		Sleep( 5 );
		return;
	}

	_parent::Init( pid );
}

void AppCorePlugins::Shutdown( PluginsEnum_t pid )
{
	if( !wxThread::IsMain() && wxTheApp )
	{
		SinglePluginMethodEvent evt( &AppCorePlugins::Shutdown, pid );
		wxGetApp().ProcessAction( evt );
		Sleep( 5 );
		return;
	}

	_parent::Shutdown( pid );
}

bool AppCorePlugins::Init()
{
	if( !NeedsInit() ) return false;

    _SetLogFolder();
	SendLogFolder();

	_SetSettingsFolder();
	SendSettingsFolder();

	if (_parent::Init())
	{
		PostPluginStatus( CorePlugins_Init );
		return true;
	}
	return false;
}

bool AppCorePlugins::Shutdown()
{
	if (_parent::Shutdown())
	{
		PostPluginStatus( CorePlugins_Shutdown );
		return true;
	}
	return false;
}

void AppCorePlugins::Close()
{
	AffinityAssert_AllowFrom_CoreThread();

	if( !NeedsClose() ) return;

	PostPluginStatus( CorePlugins_Closing );
	_parent::Close();
	PostPluginStatus( CorePlugins_Closed );
}

void AppCorePlugins::Open()
{
	AffinityAssert_AllowFrom_CoreThread();

	/*if( !GetSysExecutorThread().IsSelf() )
	{
		GetSysExecutorThread().ProcessEvent( new SysExecEvent_AppPluginManager( &AppCorePlugins::Open ) );
		return;
	}*/

    SetLogFolder( GetLogFolder().ToString() );
	SetSettingsFolder( GetSettingsFolder().ToString() );

	if( !NeedsOpen() ) return;

	PostPluginStatus( CorePlugins_Opening );
	_parent::Open();
	PostPluginStatus( CorePlugins_Opened );
}

// Yay, this plugin is guaranteed to always be opened first and closed last.
bool AppCorePlugins::OpenPlugin_GS()
{
	if( GSopen2 && !s_DisableGsWindow )
	{
		sApp.OpenGsPanel();
	}

	bool retval = _parent::OpenPlugin_GS();

	if( g_LimiterMode == Limit_Turbo )
		GSsetVsync( false );
		
	return retval;
}

// Yay, this plugin is guaranteed to always be opened first and closed last.
void AppCorePlugins::ClosePlugin_GS()
{
	_parent::ClosePlugin_GS();
	if( CloseViewportWithPlugins && GetMTGS().IsSelf() && GSopen2 ) sApp.CloseGsPanel();
}


// --------------------------------------------------------------------------------------
//  LoadCorePluginsEvent
// --------------------------------------------------------------------------------------
class LoadCorePluginsEvent : public SysExecEvent
{
protected:
	wxString			m_folders[PluginId_Count];

public:
	LoadCorePluginsEvent()
	{
		ConvertPluginFilenames( m_folders );
	}
	
	wxString GetEventName() const
	{
		return L"LoadCorePlugins";
	}

	wxString GetEventMessage() const
	{
		return _("Loading PS2 system plugins...");
	}

protected:
	void InvokeEvent()
	{
		CorePlugins.Load( m_folders );
	}
	~LoadCorePluginsEvent() throw() {}
};

// --------------------------------------------------------------------------------------
//  Public API / Interface
// --------------------------------------------------------------------------------------

int EnumeratePluginsInFolder( const wxDirName& searchpath, wxArrayString* dest )
{
	if (!searchpath.Exists()) return 0;

	ScopedPtr<wxArrayString> placebo;
	wxArrayString* realdest = dest;
	if( realdest == NULL )
		placebo = realdest = new wxArrayString();		// placebo is our /dev/null -- gets deleted when done

#ifdef __WXMSW__
	// Windows pretty well has a strict "must end in .dll" rule.
	wxString pattern( L"*%s" );
#else
	// Other platforms seem to like to version their libs after the .so extension:
	//    blah.so.3.1.fail?
	wxString pattern( L"*%s*" );
#endif

	wxDir::GetAllFiles( searchpath.ToString(), realdest, pxsFmt( pattern, wxDynamicLibrary::GetDllExt()), wxDIR_FILES );
	
	// SECURITY ISSUE:  (applies primarily to Windows, but is a good idea on any platform)
	//   The search folder order for plugins can vary across operating systems, and in some poorly designed
	//   cases (old versions of windows), the search order is a security hazard because it does not
	//   search where you might really expect.  In our case wedo not want *any* searching.  The only
	//   plugins we want to load are the ones we found in the directly the user specified, so make
	//   sure all paths are FULLY QUALIFIED ABSOLUTE PATHS.
	//
	// (for details, read: http://msdn.microsoft.com/en-us/library/ff919712.aspx )

	for (uint i=0; i<realdest->GetCount(); ++i )
	{
		(*realdest)[i] = Path::MakeAbsolute((*realdest)[i]);
	}

	return realdest->GetCount();
}

// Posts a message to the App to reload plugins.  Plugins are loaded via a background thread
// which is started on a pending event, so don't expect them to be ready  "right now."
// If plugins are already loaded, onComplete is invoked, and the function returns with no
// other actions performed.
void LoadPluginsPassive()
{
	AffinityAssert_AllowFrom_MainUI();

	// Plugins already loaded?
	if( !CorePlugins.AreLoaded() )
	{
		wxGetApp().SysExecutorThread.PostEvent( new LoadCorePluginsEvent() );
	}
}

static void _LoadPluginsImmediate()
{
	if( CorePlugins.AreLoaded() ) return;

	wxString passins[PluginId_Count];
	ConvertPluginFilenames( passins );
	CorePlugins.Load( passins );
}

void LoadPluginsImmediate()
{
	AffinityAssert_AllowFrom_SysExecutor();
	_LoadPluginsImmediate();
}

// Performs a blocking load of plugins.  If the emulation thread is active, it is shut down
// automatically to prevent race conditions (it depends on plugins).
//
// Exceptions regarding plugin failures will propagate out of this function, so be prepared
// to handle them.
//
// Note that this is not recommended for most situations, but coding improper passive loads
// is probably worse, so if in doubt use this and air will fix it up for you later. :)
//
void ScopedCoreThreadClose::LoadPlugins()
{
	DbgCon.WriteLn("(ScopedCoreThreadClose) Loading plugins!");
	_LoadPluginsImmediate();
}


class SysExecEvent_UnloadPlugins : public SysExecEvent
{
public:
	wxString GetEventName() const { return L"UnloadPlugins"; }

	virtual ~SysExecEvent_UnloadPlugins() throw() {}
	SysExecEvent_UnloadPlugins* Clone() const { return new SysExecEvent_UnloadPlugins(*this); }

	virtual bool AllowCancelOnExit() const { return false; }
	virtual bool IsCriticalEvent() const { return true; }

	void InvokeEvent()
	{
		CoreThread.Cancel();
		CorePlugins.Unload();
	}
};

class SysExecEvent_ShutdownPlugins : public SysExecEvent
{
public:
	wxString GetEventName() const { return L"ShutdownPlugins"; }

	virtual ~SysExecEvent_ShutdownPlugins() throw() {}
	SysExecEvent_ShutdownPlugins* Clone() const { return new SysExecEvent_ShutdownPlugins(*this); }

	virtual bool AllowCancelOnExit() const { return false; }
	virtual bool IsCriticalEvent() const { return true; }

	void InvokeEvent()
	{
		CoreThread.Cancel();
		CorePlugins.Shutdown();
	}
};

void UnloadPlugins()
{
	GetSysExecutorThread().PostEvent( new SysExecEvent_UnloadPlugins() );
}

void ShutdownPlugins()
{
	GetSysExecutorThread().PostEvent( new SysExecEvent_ShutdownPlugins() );
}

void SysExecEvent_SaveSinglePlugin::InvokeEvent()
{
	s_DisableGsWindow = true;		// keeps the GS window smooth by avoiding closing the window

	ScopedCoreThreadPause paused_core;
	//_LoadPluginsImmediate();

	if( CorePlugins.AreLoaded() )
	{
		ScopedPtr<VmStateBuffer> plugstore;

		if( CoreThread.HasActiveMachine() )
		{
			Console.WriteLn( Color_Green, L"Suspending single plugin: " + tbl_PluginInfo[m_pid].GetShortname() );
			memSavingState save( plugstore=new VmStateBuffer(L"StateCopy_SinglePlugin") );
			GetCorePlugins().Freeze( m_pid, save );
		}
			
		GetCorePlugins().Close( m_pid );
		_post_and_wait( paused_core );

		if( plugstore )
		{
			Console.WriteLn( Color_Green, L"Recovering single plugin: " + tbl_PluginInfo[m_pid].GetShortname() );
			memLoadingState load( plugstore );
			GetCorePlugins().Freeze( m_pid, load );
			GetCorePlugins().Close( m_pid );		// hack for stupid GS plugins.
		}
	}

	s_DisableGsWindow = false;
	paused_core.AllowResume();
}

void SysExecEvent_SaveSinglePlugin::CleanupEvent()
{
	s_DisableGsWindow = false;
	_parent::CleanupEvent();
}
