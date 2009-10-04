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
public:
	Exception::PluginError* Ex_PluginError;
	Exception::RuntimeError* Ex_RuntimeError;
	PluginManager* Result;

protected:
	wxString m_folders[PluginId_Count];

public:
	LoadPluginsTask( const wxString (&folders)[PluginId_Count] ) :
		Ex_PluginError( NULL )
	,	Ex_RuntimeError( NULL )
	,	Result( NULL )
	{
		for(int i=0; i<PluginId_Count; ++i )
			m_folders[i] = folders[i];

		Start();
	}

	virtual ~LoadPluginsTask() throw();

protected:
	void OnStart() {}
	void OnThreadCleanup() {}
	void ExecuteTask();
};

static ScopedPtr<LoadPluginsTask> _loadTask;

LoadPluginsTask::~LoadPluginsTask() throw()
{
	if( _loadTask )
		_loadTask.DetachPtr();		// avoids recursive deletion

	PersistentThread::Cancel();
	_loadTask = NULL;
}

void LoadPluginsTask::ExecuteTask()
{
	wxGetApp().Ping();
	Sleep(3);

	wxCommandEvent evt( pxEVT_LoadPluginsComplete );
	evt.SetClientData( this );

	try
	{
		// This is for testing of the error handler... uncomment for fun?
		//throw Exception::PluginError( PluginId_PAD, "This one is for testing the error handler!" );

		Result = PluginManager_Create( m_folders );
	}
	catch( Exception::PluginError& ex )
	{
		Ex_PluginError = new Exception::PluginError( ex );
	}
	catch( Exception::RuntimeError& innerEx )
	{
		// Runtime errors are typically recoverable, so handle them here
		// and prep them for re-throw on the main thread.
		Ex_RuntimeError = new Exception::RuntimeError(
			L"A runtime error occurred on the LoadPlugins thread.\n" + innerEx.FormatDiagnosticMessage(),
			innerEx.FormatDisplayMessage()
		);
	}
	// anything else leave unhandled so that the debugger catches it!

	wxGetApp().AddPendingEvent( evt );
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

void Pcsx2App::OnReloadPlugins( wxCommandEvent& evt )
{
	ReloadPlugins();
}

void Pcsx2App::OnLoadPluginsComplete( wxCommandEvent& evt )
{
	// scoped ptr ensures the thread object is cleaned up even on exception:
	ScopedPtr<LoadPluginsTask> killTask( (LoadPluginsTask*)evt.GetClientData() );
	m_CorePlugins = killTask->Result;

	if( !m_CorePlugins )
	{
		if( killTask->Ex_PluginError != NULL )
			throw *killTask->Ex_PluginError;
		if( killTask->Ex_RuntimeError != NULL )
			throw *killTask->Ex_RuntimeError;	// Re-Throws generic threaded errors
	}
}

void Pcsx2App::ReloadPlugins()
{
	if( _loadTask ) return;

	m_CoreThread	= NULL;
	m_CorePlugins	= NULL;

	wxString passins[PluginId_Count];

	const PluginInfo* pi = tbl_PluginInfo; do
	{
		passins[pi->id] = OverrideOptions.Filenames[pi->id].GetFullPath();

		if( passins[pi->id].IsEmpty() || !wxFileExists( passins[pi->id] ) )
			passins[pi->id] = g_Conf->FullpathTo( pi->id );
	} while( ++pi, pi->shortname != NULL );

	_loadTask.Delete() = new LoadPluginsTask( passins );
	// ...  and when it finishes it posts up a OnLoadPluginsComplete().  Bye. :)
}

// Posts a message to the App to reload plugins.  Plugins are loaded via a background thread
// which is started on a pending event, so don't expect them to be ready  "right now."
// If plugins are already loaded then no action is performed.
void LoadPluginsPassive()
{
	if( g_plugins ) return;

	wxCommandEvent evt( pxEVT_ReloadPlugins );
	wxGetApp().AddPendingEvent( evt );
}

// Blocks until plugins have been successfully loaded, or throws an exception if
// the user cancels the loading procedure after error.  If plugins are already loaded
// then no action is performed.
void LoadPluginsImmediate()
{
	AllowFromMainThreadOnly();
	if( g_plugins ) return;

	static int _reentrant = 0;
	RecursionGuard guard( _reentrant );

	pxAssertMsg( !guard.IsReentrant(), "Recrsive calls to this function are prohibited." );
	wxGetApp().ReloadPlugins();
	while( _loadTask )
	{
		Sleep( 10 );
		wxGetApp().ProcessPendingEvents();
	}
}

void UnloadPlugins()
{
	wxGetApp().m_CorePlugins = NULL;
}
