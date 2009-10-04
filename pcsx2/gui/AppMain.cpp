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
#include "IniInterface.h"
#include "MainFrame.h"
#include "Plugins.h"
#include "SaveState.h"

#include "Dialogs/ModalPopups.h"
#include "Dialogs/ConfigurationDialog.h"
#include "Dialogs/LogOptionsDialog.h"

#include "Utilities/HashMap.h"

IMPLEMENT_APP(Pcsx2App)

DEFINE_EVENT_TYPE( pxEVT_SemaphorePing );
DEFINE_EVENT_TYPE( pxEVT_OpenModalDialog );
DEFINE_EVENT_TYPE( pxEVT_ReloadPlugins );
DEFINE_EVENT_TYPE( pxEVT_LoadPluginsComplete );
DEFINE_EVENT_TYPE( pxEVT_AppCoreThread_Terminated );
DEFINE_EVENT_TYPE( pxEVT_FreezeFinished );
DEFINE_EVENT_TYPE( pxEVT_ThawFinished );

bool					UseAdminMode = false;
wxDirName				SettingsFolder;
bool					UseDefaultSettingsFolder = true;

ScopedPtr<AppConfig>	g_Conf;
ConfigOverrides			OverrideOptions;

AppCoreThread::AppCoreThread( PluginManager& plugins ) :
	SysCoreThread( plugins )
,	m_kevt()
{
}

AppCoreThread::~AppCoreThread() throw()
{
}

void AppCoreThread::Suspend( bool isBlocking )
{
	_parent::Suspend( isBlocking );
	if( HasMainFrame() )
		GetMainFrame().ApplySettings();

	// Clear the sticky key statuses, because hell knows what'll change while the PAD
	// plugin is suspended.

	m_kevt.m_shiftDown		= false;
	m_kevt.m_controlDown	= false;
	m_kevt.m_altDown		= false;
}

void AppCoreThread::Resume()
{
	// Thread control (suspend / resume) should only be performed from the main/gui thread.
	if( !AllowFromMainThreadOnly() ) return;

	if( sys_resume_lock )
	{
		Console.WriteLn( "SysResume: State is locked, ignoring Resume request!" );
		return;
	}
	_parent::Resume();
}

void AppCoreThread::OnResumeReady()
{
	if( m_shortSuspend ) return;

	ApplySettings( g_Conf->EmuOptions );

	if( GSopen2 != NULL )
		wxGetApp().OpenGsFrame();

	if( HasMainFrame() )
		GetMainFrame().ApplySettings();
}

// Called whenever the thread has terminated, for either regular or irregular reasons.
// Typically the thread handles all its own errors, so there's no need to have error
// handling here.  However it's a good idea to update the status of the GUI to reflect
// the new (lack of) thread status, so this posts a message to the App to do so.
void AppCoreThread::OnThreadCleanup()
{
	wxCommandEvent evt( pxEVT_AppCoreThread_Terminated );
	wxGetApp().AddPendingEvent( evt );
	_parent::OnThreadCleanup();
}

#ifdef __WXGTK__
	extern int TranslateGDKtoWXK( u32 keysym );
#endif

void AppCoreThread::StateCheck( bool isCancelable )
{
	_parent::StateCheck( isCancelable );

	const keyEvent* ev = PADkeyEvent();
	if( ev == NULL || (ev->key == 0) ) return;

	m_plugins.KeyEvent( *ev );
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
	if( !IsSuspended() ) return;

	// Re-entry guard protects against cases where code wants to manually set core settings
	// which are not part of g_Conf.  The subsequent call to apply g_Conf settings (which is
	// usually the desired behavior) will be ignored.

	static int localc = 0;
	RecursionGuard guard( localc );
	if(guard.IsReentrant()) return;
	SysCoreThread::ApplySettings( src );
}

bool HandlePluginError( Exception::PluginError& ex )
{
	if( pxDialogExists( DialogId_CoreSettings ) ) return true;

	bool result = Msgbox::OkCancel( ex.FormatDisplayMessage() +
		_("\n\nPress Ok to go to the Plugin Configuration Panel.")
	);

	if( result )
	{
		g_Conf->SettingsTabName = L"Plugins";

		// fixme: Send a message to the panel to select the failed plugin.
		if( Dialogs::ConfigurationDialog().ShowModal() == wxID_CANCEL )
			return false;
	}
	return result;
}

void AppCoreThread::ExecuteTask()
{
	try
	{
		SysCoreThread::ExecuteTask();
	}
	// ----------------------------------------------------------------------------
	catch( Exception::FileNotFound& ex )
	{
		m_plugins.Close();
		if( ex.StreamName == g_Conf->FullpathToBios() )
		{
			m_plugins.Close();
			bool result = Msgbox::OkCancel( ex.FormatDisplayMessage() +
				_("\n\nPress Ok to go to the BIOS Configuration Panel.") );

			if( result )
			{
				if( wxGetApp().ThreadedModalDialog( DialogId_BiosSelector ) == wxID_CANCEL )
				{
					// fixme: handle case where user cancels the settings dialog. (should return FALSE).
				}
				else
				{
					// fixme: automatically re-try emu startup here...
				}
			}
		}
	}
	// ----------------------------------------------------------------------------
	catch( Exception::PluginError& ex )
	{
		m_plugins.Close();
		Console.Error( ex.FormatDiagnosticMessage() );
		Msgbox::Alert( ex.FormatDisplayMessage(), _("Plugin Open Error") );

		/*if( HandlePluginError( ex ) )
		{
			// fixme: automatically re-try emu startup here...
		}*/
	}
	// ----------------------------------------------------------------------------
	// [TODO] : Add exception handling here for debuggable PS2 exceptions that allows
	// invocation of the PCSX2 debugger and such.
	//
	catch( Exception::BaseException& ex )
	{
		// Sent the exception back to the main gui thread?
		m_plugins.Close();
		Msgbox::Alert( ex.FormatDisplayMessage() );
	}
}

// Allows for activating menu actions from anywhere in PCSX2.
// And it's Thread Safe!
void Pcsx2App::PostMenuAction( MenuIdentifiers menu_id ) const
{
	if( m_MainFrame == NULL ) return;

	wxCommandEvent joe( wxEVT_COMMAND_MENU_SELECTED, menu_id );
	if( wxThread::IsMain() )
		m_MainFrame->GetEventHandler()->ProcessEvent( joe );
	else
		m_MainFrame->GetEventHandler()->AddPendingEvent( joe );
}

void Pcsx2App::PostPadKey( wxKeyEvent& evt )
{
	if( m_gsFrame == NULL )
	{
		evt.SetId( pxID_PadHandler_Keydown );
		wxGetApp().AddPendingEvent( evt );
	}
	else
	{
		evt.SetId( m_gsFrame->GetId() );
		m_gsFrame->AddPendingEvent( evt );
	}
}

int Pcsx2App::ThreadedModalDialog( DialogIdentifiers dialogId )
{
	AllowFromMainThreadOnly();

	MsgboxEventResult result;
	wxCommandEvent joe( pxEVT_OpenModalDialog, dialogId );
	joe.SetClientData( &result );
	AddPendingEvent( joe );
	result.WaitForMe.WaitNoCancel();
	return result.result;
}

// Waits for the main GUI thread to respond.  If run from the main GUI thread, returns
// immediately without error.  Use this on non-GUI threads to have them sleep until
// the GUI has processed all its pending messages.
void Pcsx2App::Ping() const
{
	if( wxThread::IsMain() ) return;

	Semaphore sema;
	wxCommandEvent bean( pxEVT_SemaphorePing );
	bean.SetClientData( &sema );
	wxGetApp().AddPendingEvent( bean );
	sema.WaitNoCancel();
}

// ----------------------------------------------------------------------------
//         Pcsx2App Event Handlers
// ----------------------------------------------------------------------------

// Invoked by the AppCoreThread when the thread has terminated itself.
void Pcsx2App::OnCoreThreadTerminated( wxCommandEvent& evt )
{

	if( HasMainFrame() )
		GetMainFrame().ApplySettings();
	m_CoreThread = NULL;
}

void Pcsx2App::OnSemaphorePing( wxCommandEvent& evt )
{
	((Semaphore*)evt.GetClientData())->Post();
}

void Pcsx2App::OnOpenModalDialog( wxCommandEvent& evt )
{
	using namespace Dialogs;

	MsgboxEventResult& evtres( *((MsgboxEventResult*)evt.GetClientData()) );
	switch( evt.GetId() )
	{
		case DialogId_CoreSettings:
		{
			static int _guard = 0;
			RecursionGuard guard( _guard );
			if( guard.IsReentrant() ) return;
			evtres.result = ConfigurationDialog().ShowModal();
		}
		break;

		case DialogId_BiosSelector:
		{
			static int _guard = 0;
			RecursionGuard guard( _guard );
			if( guard.IsReentrant() ) return;
			evtres.result = BiosSelectorDialog().ShowModal();
		}
		break;

		case DialogId_LogOptions:
		{
			static int _guard = 0;
			RecursionGuard guard( _guard );
			if( guard.IsReentrant() ) return;
			evtres.result = LogOptionsDialog().ShowModal();
		}
		break;

		case DialogId_About:
		{
			static int _guard = 0;
			RecursionGuard guard( _guard );
			if( guard.IsReentrant() ) return;
			evtres.result = AboutBoxDialog().ShowModal();
		}
		break;
	}

	evtres.WaitForMe.Post();
}

void Pcsx2App::OnMessageBox( pxMessageBoxEvent& evt )
{
	Msgbox::OnEvent( evt );
}

HashTools::HashMap<int, const GlobalCommandDescriptor*> GlobalAccels( 0, 0xffffffff );

void Pcsx2App::OnEmuKeyDown( wxKeyEvent& evt )
{
	const GlobalCommandDescriptor* cmd = NULL;
	GlobalAccels.TryGetValue( KeyAcceleratorCode( evt ).val32, cmd );
	if( cmd == NULL )
	{
		evt.Skip();
		return;
	}

	if( cmd != NULL )
	{
		DbgCon.WriteLn( "(app) Invoking command: %s", cmd->Id );
		cmd->Invoke();
	}

}

void Pcsx2App::HandleEvent(wxEvtHandler *handler, wxEventFunction func, wxEvent& event) const
{
	try
	{
		(handler->*func)(event);
	}
	// ----------------------------------------------------------------------------
	catch( Exception::PluginError& ex )
	{
		Console.Error( ex.FormatDiagnosticMessage() );
		if( !HandlePluginError( ex ) )
		{
			Console.Error( L"User-canceled plugin configuration after load failure.  Plugins not loaded!" );
			Msgbox::Alert( _("Warning!  Plugins have not been loaded.  PCSX2 will be inoperable.") );
		}
	}
	// ----------------------------------------------------------------------------
	catch( Exception::RuntimeError& ex )
	{
		// Runtime errors which have been unhandled should still be safe to recover from,
		// so lets issue a message to the user and then continue the message pump.

		Console.Error( ex.FormatDiagnosticMessage() );
		Msgbox::Alert( ex.FormatDisplayMessage() );
	}
}


// Common exit handler which can be called from any event (though really it should
// be called only from CloseWindow handlers since that's the more appropriate way
// to handle cancelable window closures)
//
// returns true if the app can close, or false if the close event was canceled by
// the glorious user, whomever (s)he-it might be.
bool Pcsx2App::PrepForExit()
{
	m_CoreThread = NULL;
	CleanupMess();

	return true;
}

int Pcsx2App::OnExit()
{
	PrepForExit();

	if( g_Conf )
		AppSaveSettings();

	while( wxGetLocale() != NULL )
		delete wxGetLocale();

	return wxApp::OnExit();
}

// This method generates debug assertions if the MainFrame handle is NULL (typically
// indicating that PCSX2 is running in NoGUI mode, or that the main frame has been
// closed).  In most cases you'll want to use HasMainFrame() to test for thread
// validity first, or use GetMainFramePtr() and manually check for NULL (choice
// is a matter of programmer preference).
MainEmuFrame& Pcsx2App::GetMainFrame() const
{
	pxAssert( ((uptr)GetTopWindow()) == ((uptr)m_MainFrame) );
	pxAssert( m_MainFrame != NULL );
	return *m_MainFrame;
}

// This method generates debug assertions if the CoreThread handle is NULL (typically
// indicating that there is no active emulation session).  In most cases you'll want
// to use HasCoreThread() to test for thread validity first, or use GetCoreThreadPtr()
// and manually check for NULL (choice is a matter of programmer preference).
SysCoreThread& Pcsx2App::GetCoreThread() const
{
	pxAssert( m_CoreThread != NULL );
	return *m_CoreThread;
}

void AppApplySettings( const AppConfig* oldconf )
{
	AllowFromMainThreadOnly();

	// Ensure existence of necessary documents folders.  Plugins and other parts
	// of PCSX2 rely on them.

	g_Conf->Folders.MemoryCards.Mkdir();
	g_Conf->Folders.Savestates.Mkdir();
	g_Conf->Folders.Snapshots.Mkdir();

	g_Conf->EmuOptions.BiosFilename = g_Conf->FullpathToBios();

	// Update the compression attribute on the Memcards folder.
	// Memcards generally compress very well via NTFS compression.

	NTFS_CompressFile( g_Conf->Folders.MemoryCards.ToString(), g_Conf->McdEnableNTFS );

	if( (oldconf == NULL) || (oldconf->LanguageId != g_Conf->LanguageId) )
	{
		wxDoNotLogInThisScope please;
		if( !i18n_SetLanguage( g_Conf->LanguageId ) )
		{
			if( !i18n_SetLanguage( wxLANGUAGE_DEFAULT ) )
			{
				i18n_SetLanguage( wxLANGUAGE_ENGLISH );
			}
		}
	}

	if( HasMainFrame() )
		GetMainFrame().ApplySettings();
	if( HasCoreThread() )
		GetCoreThread().ApplySettings( g_Conf->EmuOptions );
}

void AppLoadSettings()
{
	wxConfigBase* conf = wxConfigBase::Get( false );
	if( NULL == conf ) return;

	IniLoader loader( *conf );
	g_Conf->LoadSave( loader );

	if( HasMainFrame() )
		GetMainFrame().LoadRecentIsoList( *conf );
}

void AppSaveSettings()
{
	wxConfigBase* conf = wxConfigBase::Get( false );
	if( NULL == conf ) return;

	IniSaver saver( *conf );
	g_Conf->LoadSave( saver );

	if( HasMainFrame() )
		GetMainFrame().SaveRecentIsoList( *conf );
}

void Pcsx2App::OpenGsFrame()
{
	if( m_gsFrame != NULL ) return;

	m_gsFrame = new GSFrame( m_MainFrame, L"PCSX2" );
	m_gsFrame->SetFocus();
	pDsp = (uptr)m_gsFrame->GetHandle();
	m_gsFrame->Show();

	// The "in the main window" quickie hack...
	//pDsp = (uptr)m_MainFrame->m_background.GetHandle();
}

void Pcsx2App::OnGsFrameClosed()
{
	if( m_CoreThread != NULL )
		m_CoreThread->Suspend();
	m_gsFrame = NULL;
}

void Pcsx2App::OnProgramLogClosed()
{
	if( m_ProgramLogBox == NULL ) return;
	DisableWindowLogging();
	m_ProgramLogBox = NULL;
}

void Pcsx2App::OnMainFrameClosed()
{
	// Nothing threaded depends on the mainframe (yet) -- it all passes through the main wxApp
	// message handler.  But that might change in the future.
	if( m_MainFrame == NULL ) return;
	m_MainFrame = NULL;
}



// --------------------------------------------------------------------------------------
//  Sys/Core API and Shortcuts (for wxGetApp())
// --------------------------------------------------------------------------------------

// Executes the emulator using a saved/existing virtual machine state and currently
// configured CDVD source device.
void Pcsx2App::SysExecute()
{
	if( sys_resume_lock )
	{
		Console.WriteLn( "SysExecute: State is locked, ignoring Execute request!" );
		return;
	}

	SysReset();
	LoadPluginsImmediate();
	m_CoreThread = new AppCoreThread( *m_CorePlugins );
	m_CoreThread->Resume();
}

// Executes the specified cdvd source and optional elf file.  This command performs a
// full closure of any existing VM state and starts a fresh VM with the requested
// sources.
void Pcsx2App::SysExecute( CDVD_SourceType cdvdsrc )
{
	if( sys_resume_lock )
	{
		Console.WriteLn( "SysExecute: State is locked, ignoring Execute request!" );
		return;
	}

	SysReset();
	LoadPluginsImmediate();
	CDVDsys_SetFile( CDVDsrc_Iso, g_Conf->CurrentIso );
	CDVDsys_ChangeSource( cdvdsrc );

	m_CoreThread = new AppCoreThread( *m_CorePlugins );
	m_CoreThread->Resume();
}

void Pcsx2App::SysReset()
{
	m_CoreThread	= NULL;
}

// Returns true if there is a "valid" virtual machine state from the user's perspective.  This
// means the user has started the emulator and not issued a full reset.
// Thread Safety: The state of the system can change in parallel to execution of the
// main thread.  If you need to perform an extended length activity on the execution
// state (such as saving it), you *must* suspend the Corethread first!
__forceinline bool SysHasValidState()
{
	bool isRunning = HasCoreThread() ? GetCoreThread().IsRunning() : false;
	return isRunning || StateCopy_HasFullState();
}

// Writes text to console and updates the window status bar and/or HUD or whateverness.
// FIXME: This probably isn't thread safe. >_<
void SysStatus( const wxString& text )
{
	// mirror output to the console!
	Console.Status( text.c_str() );
	if( HasMainFrame() )
		GetMainFrame().SetStatusText( text );
}

bool HasMainFrame()
{
	return wxTheApp && wxGetApp().HasMainFrame();
}

bool HasCoreThread()
{
	return wxTheApp && wxGetApp().HasCoreThread();
}

// This method generates debug assertions if either the wxApp or MainFrame handles are
// NULL (typically indicating that PCSX2 is running in NoGUI mode, or that the main
// frame has been closed).  In most cases you'll want to use HasMainFrame() to test
// for gui validity first, or use GetMainFramePtr() and manually check for NULL (choice
// is a matter of programmer preference).
MainEmuFrame&	GetMainFrame()
{
	return wxGetApp().GetMainFrame();
}

SysCoreThread&	GetCoreThread()
{
	return wxGetApp().GetCoreThread();
}

// Returns a pointer to the main frame of the GUI (frame may be hidden from view), or
// NULL if no main frame exists (NoGUI mode and/or the frame has been destroyed).  If
// the wxApp is NULL then this will also return NULL.
MainEmuFrame*	GetMainFramePtr()
{
	return wxTheApp ? wxGetApp().GetMainFramePtr() : NULL;
}

// Returns a pointer to the CoreThread of the GUI (thread may be stopped or suspended),
// or NULL if no core thread exists, or if the wxApp is also NULL.
SysCoreThread*	GetCoreThreadPtr()
{
	return wxTheApp ? wxGetApp().GetCoreThreadPtr() : NULL;
}
