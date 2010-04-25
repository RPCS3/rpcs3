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
#include "MainFrame.h"
#include "ps2/BiosTools.h"

#include "GS.h"

__aligned16 SysMtgsThread mtgsThread;
__aligned16 AppCoreThread CoreThread;

AppCoreThread::AppCoreThread() : SysCoreThread()
{
}

AppCoreThread::~AppCoreThread() throw()
{
	AppCoreThread::Cancel();
}

void AppCoreThread::Cancel( bool isBlocking )
{
	if( !_parent::Cancel( wxTimeSpan( 0, 0, 2, 0 ) ) )
	{
		// Possible deadlock!
		throw Exception::ThreadDeadlock( this );
	}
}

void AppCoreThread::Reset()
{
	ScopedBusyCursor::SetDefault( Cursor_KindaBusy );
	_parent::Reset();
}

void AppCoreThread::DoThreadDeadlocked()
{
	wxGetApp().DoStuckThread( *this );
}

bool AppCoreThread::Suspend( bool isBlocking )
{
	ScopedBusyCursor::SetDefault( Cursor_KindaBusy );

	bool retval = _parent::Suspend( false );

	if( !retval || isBlocking )
		ScopedBusyCursor::SetDefault( Cursor_NotBusy );

	if( g_Conf->GSWindow.CloseOnEsc )
	{
		sGSFrame.Hide();
	}

	return retval;
}

static int resume_tries = 0;

void AppCoreThread::Resume()
{
	// Thread control (suspend / resume) should only be performed from the main/gui thread.
	if( !AffinityAssert_AllowFromMain() ) return;
	if( m_ExecMode == ExecMode_Opened ) return;
	if( m_ResumeProtection.IsLocked() ) return;

	if( !pxAssert( g_plugins != NULL ) ) return;

	if( sys_resume_lock > 0 )
	{
		Console.WriteLn( "SysResume: State is locked, ignoring Resume request!" );
		return;
	}

	ScopedBusyCursor::SetDefault( Cursor_KindaBusy );
	_parent::Resume();

	if( m_ExecMode != ExecMode_Opened )
	{
		// Resume failed for some reason, so update GUI statuses and post a message to
		// try again on the resume.

		wxGetApp().PostCommand( pxEvt_CoreThreadStatus, CoreThread_Suspended );

		if( (m_ExecMode != ExecMode_Closing) || (m_ExecMode != ExecMode_Pausing) )
		{
			if( ++resume_tries <= 2 )
			{
				sApp.SysExecute();
			}
			else
				Console.WriteLn( Color_Orange, "SysResume: Multiple resume retries failed.  Giving up..." );
		}
	}

	resume_tries = 0;
}

void AppCoreThread::ChangeCdvdSource( CDVD_SourceType type )
{
	g_Conf->CdvdSource = type;
	_parent::ChangeCdvdSource( type );
	sMainFrame.UpdateIsoSrcSelection();

	// TODO: Add a listener for CDVDsource changes?  Or should we bother?
}

void AppCoreThread::DoCpuReset()
{
	wxGetApp().PostCommand( pxEvt_CoreThreadStatus, CoreThread_Reset );
	_parent::DoCpuReset();
}

void AppCoreThread::OnResumeReady()
{
	ApplySettings( g_Conf->EmuOptions );

	if( !wxFile::Exists( g_Conf->CurrentIso ) )
		g_Conf->CurrentIso.Clear();

	sApp.GetRecentIsoManager().Add( g_Conf->CurrentIso );
	CDVDsys_SetFile( CDVDsrc_Iso, g_Conf->CurrentIso );

	AppSaveSettings();

	_parent::OnResumeReady();
}

void AppCoreThread::OnResumeInThread( bool isSuspended )
{
	_parent::OnResumeInThread( isSuspended );
	wxGetApp().PostCommand( pxEvt_CoreThreadStatus, CoreThread_Resumed );
}

void AppCoreThread::OnSuspendInThread()
{
	_parent::OnSuspendInThread();
	wxGetApp().PostCommand( pxEvt_CoreThreadStatus, CoreThread_Suspended );
}

// Called whenever the thread has terminated, for either regular or irregular reasons.
// Typically the thread handles all its own errors, so there's no need to have error
// handling here.  However it's a good idea to update the status of the GUI to reflect
// the new (lack of) thread status, so this posts a message to the App to do so.
void AppCoreThread::OnCleanupInThread()
{
	wxGetApp().PostCommand( pxEvt_CoreThreadStatus, CoreThread_Stopped );
	_parent::OnCleanupInThread();
}

void AppCoreThread::PostVsyncToUI()
{
	wxGetApp().LogicalVsync();
}

void AppCoreThread::StateCheckInThread()
{
	_parent::StateCheckInThread();
}

// To simplify settings application rules and re-entry conditions, the main App's implementation
// of ApplySettings requires that the caller manually ensure that the thread has been properly
// suspended.  If the thread has not been suspended, this call will fail *silently*.
void AppCoreThread::ApplySettings( const Pcsx2Config& src )
{
	//if( m_ExecMode != ExecMode_Closed ) return;

	Pcsx2Config fixup( src );
	if( !g_Conf->EnableSpeedHacks )
		fixup.Speedhacks = Pcsx2Config::SpeedhackOptions();
	if( !g_Conf->EnableGameFixes )
		fixup.Gamefixes = Pcsx2Config::GamefixOptions();

	// Re-entry guard protects against cases where code wants to manually set core settings
	// which are not part of g_Conf.  The subsequent call to apply g_Conf settings (which is
	// usually the desired behavior) will be ignored.

	static int localc = 0;
	RecursionGuard guard( localc );
	if( guard.IsReentrant() ) return;
	if( fixup == EmuConfig ) return;
	_parent::ApplySettings( fixup );
}

void AppCoreThread::CpuInitializeMess()
{
	if( m_hasValidState ) return;

	if( StateCopy_IsValid() )
	{
		// Automatic recovery system if a state exists in memory.  This is executed here
		// in order to ensure the plugins are in the proper (loaded/opened) state.

		SysClearExecutionCache();
		StateCopy_ThawFromMem_Blocking();

		m_hasValidState			= true;
		m_resetVirtualMachine	= false;
		return;
	}

	_parent::CpuInitializeMess();
}


void AppCoreThread::ExecuteTaskInThread()
{
	wxGetApp().PostCommand( pxEvt_CoreThreadStatus, CoreThread_Started );
	_parent::ExecuteTaskInThread();

	// ----------------------------------------------------------------------------
	/*catch( Exception::PluginError& ex )
	{
		if( g_plugins != NULL ) g_plugins->Close();
		Console.Error( ex.FormatDiagnosticMessage() );
		Msgbox::Alert( ex.FormatDisplayMessage(), _("Plugin Open Error") );

		if( HandlePluginError( ex ) )
		{
		// fixme: automatically re-try emu startup here...
		}
	}*/
}

