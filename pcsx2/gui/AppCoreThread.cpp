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

AppCoreThread CoreThread;

AppCoreThread::AppCoreThread() : SysCoreThread()
,	m_kevt()
{
}

AppCoreThread::~AppCoreThread() throw()
{
}

void AppCoreThread::Reset()
{
	ScopedBusyCursor::SetDefault( Cursor_KindaBusy );

	_parent::Reset();

	wxCommandEvent evt( pxEVT_CoreThreadStatus );
	evt.SetInt( CoreStatus_Reset );
	wxGetApp().AddPendingEvent( evt );
}

bool AppCoreThread::Suspend( bool isBlocking )
{
	ScopedBusyCursor::SetDefault( Cursor_KindaBusy );
	bool retval = _parent::Suspend( isBlocking );

	if( !retval || isBlocking )
		ScopedBusyCursor::SetDefault( Cursor_NotBusy );

	// Clear the sticky key statuses, because hell knows what'll change while the PAD
	// plugin is suspended.

	m_kevt.m_shiftDown		= false;
	m_kevt.m_controlDown	= false;
	m_kevt.m_altDown		= false;

	return retval;
}

static int resume_tries = 0;

void AppCoreThread::Resume()
{
	// Thread control (suspend / resume) should only be performed from the main/gui thread.
	if( !AllowFromMainThreadOnly() ) return;
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

		wxCommandEvent evt( pxEVT_CoreThreadStatus );
		evt.SetInt( CoreStatus_Suspended );
		wxGetApp().AddPendingEvent( evt );

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

void AppCoreThread::OnResumeReady()
{
	ApplySettings( g_Conf->EmuOptions );

	if( !wxFile::Exists( g_Conf->CurrentIso ) )
		g_Conf->CurrentIso.Clear();

	sApp.GetRecentIsoList().Add( g_Conf->CurrentIso );
	CDVDsys_SetFile( CDVDsrc_Iso, g_Conf->CurrentIso );

	AppSaveSettings();

	if( GSopen2 != NULL )
		wxGetApp().OpenGsFrame();

	_parent::OnResumeReady();
}

void AppCoreThread::OnResumeInThread( bool isSuspended )
{
	_parent::OnResumeInThread( isSuspended );

	wxCommandEvent evt( pxEVT_CoreThreadStatus );
	evt.SetInt( CoreStatus_Resumed );
	wxGetApp().AddPendingEvent( evt );
}

void AppCoreThread::OnSuspendInThread()
{
	_parent::OnSuspendInThread();

	wxCommandEvent evt( pxEVT_CoreThreadStatus );
	evt.SetInt( CoreStatus_Suspended );
	wxGetApp().AddPendingEvent( evt );
}

// Called whenever the thread has terminated, for either regular or irregular reasons.
// Typically the thread handles all its own errors, so there's no need to have error
// handling here.  However it's a good idea to update the status of the GUI to reflect
// the new (lack of) thread status, so this posts a message to the App to do so.
void AppCoreThread::OnCleanupInThread()
{
	wxCommandEvent evt( pxEVT_CoreThreadStatus );
	evt.SetInt( CoreStatus_Stopped );
	wxGetApp().AddPendingEvent( evt );
	_parent::OnCleanupInThread();
}

#ifdef __WXGTK__
	extern int TranslateGDKtoWXK( u32 keysym );
#endif

void AppCoreThread::StateCheckInThread()
{
	_parent::StateCheckInThread();
	if( !pxAssert(g_plugins!=NULL) ) return;

	const keyEvent* ev = PADkeyEvent();
	if( ev == NULL || (ev->key == 0) ) return;

	g_plugins->KeyEvent( *ev );
	m_kevt.SetEventType( ( ev->evt == KEYPRESS ) ? wxEVT_KEY_DOWN : wxEVT_KEY_UP );
	const bool isDown = (ev->evt == KEYPRESS);

	#ifdef __WXMSW__
		const int vkey = wxCharCodeMSWToWX( ev->key );
	#elif defined( __WXGTK__ )
		const int vkey = TranslateGDKtoWXK( ev->key );
	#else
	#	error Unsupported Target Platform.
	#endif

	switch (vkey)
	{
		case WXK_SHIFT:		m_kevt.m_shiftDown		= isDown; return;
		case WXK_CONTROL:	m_kevt.m_controlDown	= isDown; return;
		case WXK_MENU:		m_kevt.m_altDown		= isDown; return;
	}

	m_kevt.m_keyCode = vkey;
	wxGetApp().PostPadKey( m_kevt );
}

// To simplify settings application rules and re-entry conditions, the main App's implementation
// of ApplySettings requires that the caller manually ensure that the thread has been properly
// suspended.  If the thread has mot been suspended, this call will fail *silently*.
void AppCoreThread::ApplySettings( const Pcsx2Config& src )
{
	if( m_ExecMode != ExecMode_Closed ) return;
	if( src == EmuConfig ) return;

	// Re-entry guard protects against cases where code wants to manually set core settings
	// which are not part of g_Conf.  The subsequent call to apply g_Conf settings (which is
	// usually the desired behavior) will be ignored.

	static int localc = 0;
	RecursionGuard guard( localc );
	if(guard.IsReentrant()) return;
	SysCoreThread::ApplySettings( src );
}

void AppCoreThread::ExecuteTaskInThread()
{
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
