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

#include <wx/dir.h>
#include <wx/file.h>

#include "Plugins.h"
#include "GS.h"
#include "HostGui.h"
#include "AppConfig.h"

int EnumeratePluginsInFolder( const wxDirName& searchpath, wxArrayString* dest )
{
	wxScopedPtr<wxArrayString> placebo;
	wxArrayString* realdest = dest;
	if( realdest == NULL )
		placebo.reset( realdest = new wxArrayString() );

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

using namespace Threading;

void Pcsx2App::OnReloadPlugins( wxCommandEvent& evt )
{
	ReloadPlugins();
}

void Pcsx2App::ReloadPlugins()
{
	class LoadPluginsTask : public Threading::BaseTaskThread
	{
	public:
		PluginManager* Result;
		Exception::PluginError* Ex_PluginError;
		Exception::RuntimeError* Ex_RuntimeError;

	protected:
		const wxString (&m_folders)[PluginId_Count];

	public:
		LoadPluginsTask( const wxString (&folders)[PluginId_Count] ) :
			Result( NULL )
		,	Ex_PluginError( NULL )
		,	Ex_RuntimeError( NULL )
		,	m_folders( folders )
		{
		}

		virtual ~LoadPluginsTask() throw()
		{
			BaseTaskThread::Cancel();
		}

	protected:
		void Task()
		{
			wxGetApp().Ping();
			Sleep(3);

			try
			{
				//throw Exception::PluginError( PluginId_PAD, "This one is for testing the error handler!" );
				Result = PluginManager_Create( m_folders );
			}
			catch( Exception::PluginError& ex )
			{
				Result = NULL;
				Ex_PluginError = new Exception::PluginError( ex );
			}
			catch( Exception::RuntimeError& innerEx )
			{
				// Runtime errors are typically recoverable, so handle them here
				// and prep them for re-throw on the main thread.
				Result = NULL;
				Ex_RuntimeError = new Exception::RuntimeError(
					L"A runtime error occurred on the LoadPlugins thread.\n" + innerEx.FormatDiagnosticMessage(),
					innerEx.FormatDisplayMessage()
				);
			}

			// anything else leave unhandled so that the debugger catches it!
		}
	};

	m_CoreThread.reset();
	m_CorePlugins.reset();

	wxString passins[PluginId_Count];

	const PluginInfo* pi = tbl_PluginInfo-1;
	while( ++pi, pi->shortname != NULL )
	{
		passins[pi->id] = OverrideOptions.Filenames[pi->id].GetFullPath();

		if( passins[pi->id].IsEmpty() || !wxFileExists( passins[pi->id] ) )
			passins[pi->id] = g_Conf->FullpathTo( pi->id );
	}

	LoadPluginsTask task( passins );
	task.Start();
	task.PostTask();
	task.WaitForResult();

	if( task.Result == NULL )
	{
		if( task.Ex_PluginError != NULL )
			throw *task.Ex_PluginError;
		if( task.Ex_RuntimeError != NULL )
			throw *task.Ex_RuntimeError;	// Re-Throws generic threaded errors
	}
	
	m_CorePlugins.reset( task.Result );
}

// Posts a message to the App to reload plugins.  Plugins are loaded via a background thread
// which is started on a pending event, so don't expect them to be ready  "right now."
void LoadPluginsPassive()
{
	wxCommandEvent evt( pxEVT_ReloadPlugins );
	wxGetApp().AddPendingEvent( evt );
}

// Blocks until plugins have been successfully loaded, or throws an exception if
// the user cancels the loading procedure after error.
void LoadPluginsImmediate()
{
	wxASSERT( wxThread::IsMain() );

	static int _reentrant = 0;
	EntryGuard guard( _reentrant );

	wxASSERT( !guard.IsReentrant() );
	wxGetApp().ReloadPlugins();
}

void UnloadPlugins()
{
	wxGetApp().m_CorePlugins.reset();
}
