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

#include "PrecompiledHeader.h"
#include "App.h"
#include "MainFrame.h"

#include <wx/dir.h>
#include <wx/file.h>

#include "Plugins.h"
#include "GS.h"
#include "HostGui.h"
#include "AppConfig.h"

using namespace Threading;

static FnType_OnThreadComplete* Callback_PluginsLoadComplete = NULL;

// --------------------------------------------------------------------------------------
//  AppPluginManager
// --------------------------------------------------------------------------------------
// This extension of PluginManager provides event listener sources for plugins -- loading,
// unloading, open, close, shutdown, etc.
//
// FIXME : Should this be made part of the PCSX2 core emulation? (integrated into PluginManager)
//   I'm undecided on if it makes sense more in that context or in this one (interface).
//
class AppPluginManager : public PluginManager
{
	typedef PluginManager _parent;

public:
	AppPluginManager( const wxString (&folders)[PluginId_Count] ) : PluginManager( folders )
	{
		SetSettingsFolder( GetSettingsFolder().ToString() );
	}

	virtual ~AppPluginManager() throw()
	{
		sApp.PostPluginStatus( PluginsEvt_Unloaded );
	}

	void Init()
	{
		SetSettingsFolder( GetSettingsFolder().ToString() );
		_parent::Init();
		sApp.PostPluginStatus( PluginsEvt_Init );
	}
	
	void Shutdown()
	{
		_parent::Shutdown();
		sApp.PostPluginStatus( PluginsEvt_Shutdown );
	}

	void Close()
	{
		if( !NeedsClose() ) return;

		sApp.PostPluginStatus( PluginsEvt_Closing );
		_parent::Close();
		sApp.PostPluginStatus( PluginsEvt_Closed );
	}
	
	void Open()
	{
		SetSettingsFolder( GetSettingsFolder().ToString() );

		if( !NeedsOpen() ) return;

		sApp.PostPluginStatus( PluginsEvt_Opening );
		_parent::Open();
		sApp.PostPluginStatus( PluginsEvt_Opened );
	}

	// Yay, this plugin is guaranteed to always be opened first and closed last.
	bool OpenPlugin_GS()
	{
		if( GSopen2 != NULL )
		{
			sApp.OpenGsPanel();
		}

		return _parent::OpenPlugin_GS();
	}

	// Yay, this plugin is guaranteed to always be opened first and closed last.
	void ClosePlugin_GS()
	{
		if( GSopen2 == NULL || CloseViewportWithPlugins )
		{
			// All other plugins must be closed before the GS, because they all rely on
			// the GS window handle being valid.  The recursion guard will protect this
			// function from being called a million times. ;)

			static int _guard;
			RecursionGuard mess( _guard );
			if( !mess.IsReentrant() ) Close();
		}

		_parent::ClosePlugin_GS();
		sApp.CloseGsPanel();
	}


	/*void Open( PluginsEnum_t pid )
	{
		_parent::Open( pid );
	}
	
	void Close( PluginsEnum_t pid )
	{
		_parent::Close( pid );
	}*/
};

// --------------------------------------------------------------------------------------
//  LoadPluginsTask
// --------------------------------------------------------------------------------------
// On completion the thread sends a pxEvt_LoadPluginsComplete message, which contains a
// handle to this thread object.  If the load is successful, the Result var is set to
// non-NULL.  If NULL, an error occurred and the thread loads the exception into either
// Ex_PluginError or Ex_RuntimeError.
//
class LoadPluginsTask : public PersistentThread
{
	typedef PersistentThread _parent;
	
public:
	PluginManager* Result;

protected:
	wxString			m_folders[PluginId_Count];
	ScopedBusyCursor	m_hourglass;
public:
	LoadPluginsTask( const wxString (&folders)[PluginId_Count] ) : Result( NULL ), m_hourglass( Cursor_KindaBusy )
	{
		for(int i=0; i<PluginId_Count; ++i )
			m_folders[i] = folders[i];
	}

	virtual ~LoadPluginsTask() throw();

protected:
	void OnCleanupInThread();
	void ExecuteTaskInThread();
};

LoadPluginsTask::~LoadPluginsTask() throw()
{
	PersistentThread::Cancel();
}

void LoadPluginsTask::ExecuteTaskInThread()
{
	wxGetApp().Ping();

	// This is for testing of the error handler... uncomment for fun?
	//throw Exception::PluginError( PluginId_PAD, "This one is for testing the error handler!" );

	Result = new AppPluginManager( m_folders );
}

void LoadPluginsTask::OnCleanupInThread()
{
	wxGetApp().PostCommand( this, pxEvt_LoadPluginsComplete );
	_parent::OnCleanupInThread();
}

// --------------------------------------------------------------------------------------
//  SaveSinglePluginHelper  (Implementations)
// --------------------------------------------------------------------------------------

SaveSinglePluginHelper::SaveSinglePluginHelper( PluginsEnum_t pid )
	: m_plugstore( L"PluginConf Savestate" )
{
	m_whereitsat	= NULL;
	m_resume		= false;
	m_pid			= pid;
	m_validstate	= SysHasValidState();

	if( !m_validstate ) return;
	Console.WriteLn( Color_Green, L"Suspending single plugin: " + tbl_PluginInfo[m_pid].GetShortname() );

	m_resume		= CoreThread.Pause();
	m_whereitsat	= StateCopy_GetBuffer();

	if( m_whereitsat == NULL )
	{
		m_whereitsat = &m_plugstore;
		memSavingState save( m_plugstore );
		g_plugins->Freeze( m_pid, save );
	}

	g_plugins->Close( pid );
}

SaveSinglePluginHelper::~SaveSinglePluginHelper() throw()
{
	if( m_validstate )
	{
		Console.WriteLn( Color_Green, L"Recovering single plugin: " + tbl_PluginInfo[m_pid].GetShortname() );
		memLoadingState load( *m_whereitsat );
		if( m_plugstore.IsDisposed() ) load.SeekToSection( m_pid );
		g_plugins->Freeze( m_pid, load );
	}

	if( m_resume ) CoreThread.Resume();
}

/////////////////////////////////////////////////////////////////////////////////////////


int EnumeratePluginsInFolder( const wxDirName& searchpath, wxArrayString* dest )
{
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

	return searchpath.Exists() ?
		wxDir::GetAllFiles( searchpath.ToString(), realdest, wxsFormat( pattern, wxDynamicLibrary::GetDllExt()), wxDIR_FILES ) : 0;
}

void ConvertPluginFilenames( wxString (&passins)[PluginId_Count] )
{
	const PluginInfo* pi = tbl_PluginInfo; do
	{
		passins[pi->id] = OverrideOptions.Filenames[pi->id].GetFullPath();

		if( passins[pi->id].IsEmpty() || !wxFileExists( passins[pi->id] ) )
			passins[pi->id] = g_Conf->FullpathTo( pi->id );
	} while( ++pi, pi->shortname != NULL );
}

// boolean lock modified from the main thread only...
static bool plugin_load_lock = false;

void Pcsx2App::ReloadPlugins()
{
	if( SelfMethodInvoke( &Pcsx2App::ReloadPlugins ) ) return;

	if( plugin_load_lock ) return;
	CoreThread.Cancel();
	m_CorePlugins	= NULL;

	wxString passins[PluginId_Count];

	const PluginInfo* pi = tbl_PluginInfo; do
	{
		passins[pi->id] = OverrideOptions.Filenames[pi->id].GetFullPath();

		if( passins[pi->id].IsEmpty() || !wxFileExists( passins[pi->id] ) )
			passins[pi->id] = g_Conf->FullpathTo( pi->id );
	} while( ++pi, pi->shortname != NULL );

	(new LoadPluginsTask( passins ))->Start();
	// ...  and when it finishes it posts up a OnLoadPluginsComplete().  Bye. :)

	plugin_load_lock = true;
}

// Note: If the ClientData paremeter of wxCommandEvt is NULL, this message simply dispatches
// the plugged in listeners.
void Pcsx2App::OnLoadPluginsComplete( wxCommandEvent& evt )
{
	plugin_load_lock = false;

	FnType_OnThreadComplete* fn_tmp = Callback_PluginsLoadComplete;
	
	if( evt.GetClientData() != NULL )
	{
		if( !pxAssertDev( !m_CorePlugins, "LoadPlugins thread just finished, but CorePlugins state != NULL (odd!)." ) )
			m_CorePlugins = NULL;

		// scoped ptr ensures the thread object is cleaned up even on exception:
		ScopedPtr<LoadPluginsTask> killTask( (LoadPluginsTask*)evt.GetClientData() );
		killTask->RethrowException();
		m_CorePlugins = killTask->Result;
	}
	
	if( fn_tmp != NULL ) fn_tmp( evt );

	PostPluginStatus( PluginsEvt_Loaded );
}

void Pcsx2App::PostPluginStatus( PluginEventType pevt )
{
	if( !wxThread::IsMain() )
	{
		PostCommand( pxEvt_PluginStatus, pevt );
	}
	else
	{
		sApp.Source_CorePluginStatus().Dispatch( pevt );
	}
}

void Pcsx2App::OnPluginStatus( wxCommandEvent& evt )
{
	PostPluginStatus( (PluginEventType)evt.GetInt() );
}

// Posts a message to the App to reload plugins.  Plugins are loaded via a background thread
// which is started on a pending event, so don't expect them to be ready  "right now."
// If plugins are already loaded, onComplete is invoked, and the function returns with no
// other actions performed.
void LoadPluginsPassive( FnType_OnThreadComplete* onComplete )
{
	// Plugins already loaded?
	if( wxGetApp().m_CorePlugins )
	{
		if( onComplete ) onComplete( wxCommandEvent( pxEvt_LoadPluginsComplete ) );
		return;
	}

	if( onComplete )
		Callback_PluginsLoadComplete = onComplete;

	wxGetApp().ReloadPlugins();
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
void LoadPluginsImmediate()
{
	if( g_plugins != NULL ) return;

	CoreThread.Cancel();

	wxString passins[PluginId_Count];
	ConvertPluginFilenames( passins );
	wxGetApp().m_CorePlugins = new AppPluginManager( passins );

}

void UnloadPlugins()
{
	CoreThread.Cancel();
	sApp.m_CorePlugins = NULL;
}
