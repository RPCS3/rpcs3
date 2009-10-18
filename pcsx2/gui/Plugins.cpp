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
//  LoadPluginsTask
// --------------------------------------------------------------------------------------
// On completion the thread sends a pxEVT_LoadPluginsComplete message, which contains a
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
	wxString m_folders[PluginId_Count];

public:
	LoadPluginsTask( const wxString (&folders)[PluginId_Count] ) : Result( NULL )
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
	Yield(3);

	// This is for testing of the error handler... uncomment for fun?
	//throw Exception::PluginError( PluginId_PAD, "This one is for testing the error handler!" );

	Result = PluginManager_Create( m_folders );
}

void LoadPluginsTask::OnCleanupInThread()
{
	wxCommandEvent evt( pxEVT_LoadPluginsComplete );
	evt.SetClientData( this );
	wxGetApp().AddPendingEvent( evt );

	_parent::OnCleanupInThread();
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

void Pcsx2App::OnReloadPlugins( wxCommandEvent& evt )
{
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

	//m_evtsrc_PluginLoadFinished.Dispatch( evt );
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
		if( onComplete ) onComplete( wxCommandEvent( pxEVT_LoadPluginsComplete ) );
		return;
	}

	if( onComplete )
		Callback_PluginsLoadComplete = onComplete;

	wxCommandEvent evt( pxEVT_ReloadPlugins );
	wxGetApp().AddPendingEvent( evt );
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
	wxGetApp().m_CorePlugins = PluginManager_Create( passins );

}

void UnloadPlugins()
{
	CoreThread.Cancel();
	wxGetApp().m_CorePlugins = NULL;
}
