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
#include "ps2/BiosTools.h"

#include "Dialogs/ModalPopups.h"
#include "Dialogs/ConfigurationDialog.h"
#include "Dialogs/LogOptionsDialog.h"

#include "Utilities/HashMap.h"

#ifdef __WXMSW__
#	include <wx/msw/wrapwin.h>		// needed to implement the app!
#endif

IMPLEMENT_APP(Pcsx2App)

/*DEFINE_EVENT_TYPE( pxEVT_ReloadPlugins );
DEFINE_EVENT_TYPE( pxEVT_OpenGsPanel );*/

DEFINE_EVENT_TYPE( pxEvt_FreezeThreadFinished );
DEFINE_EVENT_TYPE( pxEvt_CoreThreadStatus );
DEFINE_EVENT_TYPE( pxEvt_LoadPluginsComplete );
DEFINE_EVENT_TYPE( pxEvt_PluginStatus );
DEFINE_EVENT_TYPE( pxEvt_SysExecute );
DEFINE_EVENT_TYPE( pxEvt_OpenModalDialog );
DEFINE_EVENT_TYPE( pxEvt_InvokeMethod );
DEFINE_EVENT_TYPE( pxEvt_LogicalVsync );

#include "Utilities/EventSource.inl"
template class EventSource< IniInterface >;
template class EventSource< AppEventType >;
template class EventSource< PluginEventType >;

bool					UseAdminMode = false;
wxDirName				SettingsFolder;
bool					UseDefaultSettingsFolder = true;

ScopedPtr<AppConfig>	g_Conf;
ConfigOverrides			OverrideOptions;

static bool HandlePluginError( Exception::PluginError& ex )
{
	if( pxDialogExists( L"CoreSettings" ) ) return true;

	bool result = Msgbox::OkCancel( ex.FormatDisplayMessage() +
		_("\n\nPress Ok to go to the Plugin Configuration Panel.")
	);

	if( result )
	{
		g_Conf->SysSettingsTabName = L"Plugins";

		// fixme: Send a message to the panel to select the failed plugin.
		if( wxGetApp().IssueModalDialog( Dialogs::SysConfigDialog::GetNameStatic() ) == wxID_CANCEL )
			return false;
	}
	return result;
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

// --------------------------------------------------------------------------------------
//  pxInvokeMethodEvent
// --------------------------------------------------------------------------------------
// Unlike pxPingEvent, the Semaphore belonging to this event is typically posted when the
// invoked method is completed.  If the method can be executed in non-blocking fashion then
// it should leave the semaphore postback NULL.
//
class pxInvokeMethodEvent : public pxPingEvent
{
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN(pxInvokeMethodEvent)

protected:
	FnType_AppMethod	m_Method;

public:
	virtual ~pxInvokeMethodEvent() throw() { }
	virtual pxInvokeMethodEvent *Clone() const { return new pxInvokeMethodEvent(*this); }

	explicit pxInvokeMethodEvent( int msgtype, FnType_AppMethod method=NULL, Semaphore* sema=NULL )
		: pxPingEvent( msgtype, sema )
	{
		m_Method = method;
	}

	explicit pxInvokeMethodEvent( FnType_AppMethod method=NULL, Semaphore* sema=NULL )
		: pxPingEvent( pxEvt_InvokeMethod, sema )
	{
		m_Method = method;
	}

	explicit pxInvokeMethodEvent( FnType_AppMethod method, Semaphore& sema )
		: pxPingEvent( pxEvt_InvokeMethod, &sema )
	{
		m_Method = method;
	}
	
	pxInvokeMethodEvent( const pxInvokeMethodEvent& src )
		: pxPingEvent( src )
	{
		m_Method = src.m_Method;
	}

	void Invoke() const
	{
		if( m_Method ) (wxGetApp().*m_Method)();
		if( m_PostBack ) m_PostBack->Post();
	}
	
	void SetMethod( FnType_AppMethod method )
	{
		m_Method = method;
	}
};


IMPLEMENT_DYNAMIC_CLASS( pxInvokeMethodEvent, pxPingEvent )

void Pcsx2App::OnInvokeMethod( pxInvokeMethodEvent& evt )
{
	evt.Invoke();		// wow this is easy!
}

#ifdef __WXGTK__
extern int TranslateGDKtoWXK( u32 keysym );
#endif

void Pcsx2App::PadKeyDispatch( const keyEvent& ev )
{
	m_kevt.SetEventType( ( ev.evt == KEYPRESS ) ? wxEVT_KEY_DOWN : wxEVT_KEY_UP );
	const bool isDown = (ev.evt == KEYPRESS);

#ifdef __WXMSW__
	const int vkey = wxCharCodeMSWToWX( ev.key );
#elif defined( __WXGTK__ )
	const int vkey = TranslateGDKtoWXK( ev.key );
#else
#	error Unsupported Target Platform.
#endif

	switch (vkey)
	{
		case WXK_SHIFT:		m_kevt.m_shiftDown		= isDown; return;
		case WXK_CONTROL:	m_kevt.m_controlDown	= isDown; return;

		case WXK_ALT:		// ALT/MENU are usually the same key?  I'm confused.
		case WXK_MENU:		m_kevt.m_altDown		= isDown; return;
	}

	m_kevt.m_keyCode = vkey;

	// HACK: Legacy PAD plugins expect PCSX2 to ignore keyboard messages on the
	// GS window while the PAD plugin is open, so send messages to the APP handler
	// only if *either* the GS or PAD plugins are in legacy mode.

	if( m_gsFrame == NULL || (PADopen != NULL) )
	{
		if( m_kevt.GetEventType() == wxEVT_KEY_DOWN )
		{
			m_kevt.SetId( pxID_PadHandler_Keydown );
			wxGetApp().ProcessEvent( m_kevt );
		}
	}
	else
	{
		m_kevt.SetId( m_gsFrame->GetViewport()->GetId() );
		m_gsFrame->ProcessEvent( m_kevt );
	}
}

// OnLogicalVsync - Event received from the AppCoreThread (EEcore) for each vsync,
// roughly 50/60 times a second when frame limiting is enabled, and up to 10,000 
// times a second if not (ok, not quite, but you get the idea... I hope.)
void Pcsx2App::LogicalVsync()
{
	if( SelfMethodPost( &Pcsx2App::LogicalVsync ) ) return;

	if( !SysHasValidState() || g_plugins == NULL ) return;

	// Only call PADupdate here if we're using GSopen2.  Legacy GSopen plugins have the
	// GS window belonging to the MTGS thread.
	if( (PADupdate != NULL) && (GSopen2 != NULL) && (m_gsFrame != NULL) )
		PADupdate(0);

	const keyEvent* ev = PADkeyEvent();

	if( (ev != NULL) && (ev->key != 0) )
	{
		// Give plugins first try to handle keys.  If none of them handles the key, it will
		// be passed to the main user interface.

		if( !g_plugins->KeyEvent( *ev ) )
			PadKeyDispatch( *ev );
	}
}

// ----------------------------------------------------------------------------
//         Pcsx2App Event Handlers
// ----------------------------------------------------------------------------

// Invoked by the AppCoreThread when it's internal status has changed.
// evt.GetInt() reflects the status at the time the message was sent, which may differ
// from the actual status.  Typically listeners bound to this will want to use direct
// polling of the CoreThread rather than the belated status.
void Pcsx2App::OnCoreThreadStatus( wxCommandEvent& evt )
{
	// Clear the sticky key statuses, because hell knows what'll change while the PAD
	// plugin is suspended.

	m_kevt.m_shiftDown		= false;
	m_kevt.m_controlDown	= false;
	m_kevt.m_altDown		= false;

	m_evtsrc_CoreThreadStatus.Dispatch( evt );
	ScopedBusyCursor::SetDefault( Cursor_NotBusy );
	CoreThread.RethrowException();
}

void Pcsx2App::OnOpenModalDialog( wxCommandEvent& evt )
{
	pxAssertDev( !evt.GetString().IsEmpty(), wxNullChar );

	MsgboxEventResult* evtres = (MsgboxEventResult*)evt.GetClientData();

	wxWindowID result = IssueModalDialog( evt.GetString() );

	if( evtres != NULL )
	{
		evtres->result = result;
		evtres->WaitForMe.Post();
	}
}

int Pcsx2App::IssueModalDialog( const wxString& dlgName )
{
	if( dlgName.IsEmpty() ) return wxID_CANCEL;
	
	if( !wxThread::IsMain() )
	{
		MsgboxEventResult result;
		PostCommand( &result, pxEvt_OpenModalDialog, 0, 0, dlgName );
		result.WaitForMe.WaitNoCancel();
		return result.result;
	}
	
	if( wxWindow* window = wxFindWindowByName( dlgName ) )
	{
		if( wxDialog* dialog = wxDynamicCast( window, wxDialog ) )
		{
			window->SetFocus();
			
			// It's legal to call ShowModal on a non-modal dialog, therefore making
			// it modal in nature for the needs of whatever other thread of action wants
			// to block against it:

			if( !dialog->IsModal() )
			{
				int result = dialog->ShowModal();
				dialog->Destroy();
				return result;
			}
		}
	}
	else
	{
		using namespace Dialogs;

		if( dlgName == SysConfigDialog::GetNameStatic() )
			return SysConfigDialog().ShowModal();
		if( dlgName == AppConfigDialog::GetNameStatic() )
			return AppConfigDialog().ShowModal();
		if( dlgName == BiosSelectorDialog::GetNameStatic() )
			return BiosSelectorDialog().ShowModal();
		if( dlgName == LogOptionsDialog::GetNameStatic() )
			return LogOptionsDialog().ShowModal();
		if( dlgName == AboutBoxDialog::GetNameStatic() )
			return AboutBoxDialog().ShowModal();
	}
	
	return wxID_CANCEL;
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

// Returns a string message telling the user to consult guides for obtaining a legal BIOS.
// This message is in a function because it's used as part of several dialogs in PCSX2 (there
// are multiple variations on the BIOS and BIOS folder checks).
wxString BIOS_GetMsg_Required()
{
	return pxE( ".Popup:BiosDumpRequired",
		L"\n\n"
		L"PCSX2 requires a PS2 BIOS in order to run.  For legal reasons, you *must* obtain \n"
		L"a BIOS from an actual PS2 unit that you own (borrowing doesn't count).\n"
		L"Please consult the FAQs and Guides for further instructions.\n"
	);
}


void Pcsx2App::HandleEvent(wxEvtHandler* handler, wxEventFunction func, wxEvent& event) const
{
	const_cast<Pcsx2App*>(this)->HandleEvent( handler, func, event );
}

void Pcsx2App::HandleEvent(wxEvtHandler* handler, wxEventFunction func, wxEvent& event)
{
	try {
		(handler->*func)(event);
	}
	// ----------------------------------------------------------------------------
	catch( Exception::BiosLoadFailed& ex )
	{
		wxDialogWithHelpers dialog( NULL, _("PS2 BIOS Error"), wxVERTICAL );
		dialog += dialog.Heading( ex.FormatDisplayMessage() + BIOS_GetMsg_Required() + _("\nPress Ok to go to the BIOS Configuration Panel.") );
		dialog += new ModalButtonPanel( &dialog, MsgButtons().OKCancel() );
		
		if( dialog.ShowModal() == wxID_CANCEL )
			Console.Warning( "User denied option to re-configure BIOS." );

		if( IssueModalDialog( Dialogs::BiosSelectorDialog::GetNameStatic() ) != wxID_CANCEL )
		{
			SysExecute();
		}
		else
			Console.Warning( "User canceled BIOS configuration." );
	}
	// ----------------------------------------------------------------------------
	catch( Exception::SaveStateLoadError& ex)
	{
		// Saved state load failed.
		Console.Warning( ex.FormatDiagnosticMessage() );
		CoreThread.Resume();
	}
	catch( Exception::PluginInitError& ex )
	{
		if( m_CorePlugins ) m_CorePlugins->Shutdown();

		Console.Error( ex.FormatDiagnosticMessage() );
		if( !HandlePluginError( ex ) )
		{
			Console.Error( L"User-canceled plugin configuration after plugin initialization failure.  Plugins unloaded." );
			Msgbox::Alert( _("Warning!  System plugins have not been loaded.  PCSX2 may be inoperable.") );
		}
	}
	catch( Exception::PluginError& ex )
	{
		if( m_CorePlugins ) m_CorePlugins->Close();

		Console.Error( ex.FormatDiagnosticMessage() );
		if( !HandlePluginError( ex ) )
		{
			Console.Error( L"User-canceled plugin configuration; Plugins not loaded!" );
			Msgbox::Alert( _("Warning!  System plugins have not been loaded.  PCSX2 may be inoperable.") );
		}
	}
	// ----------------------------------------------------------------------------
	catch( Exception::ThreadTimedOut& ex )
	{
		// [TODO]  Bind a listener to the CoreThread status, and automatically close the dialog
		// if the thread starts responding while we're waiting (not hard in fact, but I'm getting
		// a little tired, so maybe later!)  --air
	
		Console.Warning( ex.FormatDiagnosticMessage() );
		wxDialogWithHelpers dialog( NULL, _("PCSX2 Unresponsive Thread"), wxVERTICAL );
		
		dialog += dialog.Heading( ex.FormatDisplayMessage() + L"\n\n" +
			pxE( ".Popup Error:Thread Deadlock Actions",
				L"'Ignore' to continue waiting for the thread to respond.\n"
				L"'Cancel' to attempt to cancel the thread.\n"
				L"'Terminate' to quit PCSX2 immediately.\n"
			)
		);

		int result = pxIssueConfirmation( dialog, MsgButtons().Ignore().Cancel().Custom( _("Terminate") ) );
		
		if( result == pxID_CUSTOM )
		{
			// fastest way to kill the process!
			// (note: SIGTERM is a "handled" kill that performs shutdown stuff, which typically juse crashes anyway)
			wxKill( wxGetProcessId(), wxSIGKILL );
		}
		else if( result == wxID_CANCEL )
		{
			// Attempt to cancel the thread:
			ex.Thread().Cancel();
		}

		// Ignore does nothing...
	}
	// ----------------------------------------------------------------------------
	catch( Exception::CancelEvent& ex )
	{
		Console.Warning( ex.FormatDiagnosticMessage() );
	}
	catch( Exception::RuntimeError& ex )
	{
		// Runtime errors which have been unhandled should still be safe to recover from,
		// so lets issue a message to the user and then continue the message pump.

		Console.Error( ex.FormatDiagnosticMessage() );
		Msgbox::Alert( ex.FormatDisplayMessage() );
	}
}

static void __evt_fastcall OnStateSaveFinished( void* obj, wxCommandEvent& evt )
{
	if( evt.GetInt() == CoreStatus_Resumed )
	{
		wxGetApp().PostMenuAction( MenuId_Exit );
		wxGetApp().Source_CoreThreadStatus().Remove( NULL, OnStateSaveFinished );
	}
}

// Common exit handler which can be called from any event (though really it should
// be called only from CloseWindow handlers since that's the more appropriate way
// to handle cancelable window closures)
//
// returns true if the app can close, or false if the close event was canceled by
// the glorious user, whomever (s)he-it might be.
bool Pcsx2App::PrepForExit( bool canCancel )
{
	// If a savestate is saving, we should wait until it finishes.  Otherwise the user
	// might lose data.
	
	if( StateCopy_IsBusy() )
	{
		Source_CoreThreadStatus().Add( NULL, OnStateSaveFinished );
		throw Exception::CancelEvent( "Savestate in progress, cannot close program (close event delayed)" );
	}

	/*
	if( canCancel )
	{
		// TODO: Confirm with the user?
		// Problem: Suspend is often slow because it needs to wait until the current EE frame
		// has finished processing (if the GS or logging has incurred severe overhead this makes
		// closing PCSX2 difficult).  A non-blocking suspend with modal dialog might suffice
		// however. --air
		
		bool resume = CoreThread.Suspend();
		if( false )
		{
			if(resume) CoreThread.Resume();
			return false;
		}
	}*/

	AppEventType toSend = AppStatus_Exiting;
	m_evtsrc_AppStatus.Dispatch( toSend );
	CleanupMess();

	return true;
}

int Pcsx2App::OnExit()
{
	CleanupMess();

	if( g_Conf )
		AppSaveSettings();

	sMainFrame.RemoveEventHandler( &GetRecentIsoList() );

	m_Resources = NULL;

	return wxApp::OnExit();
}

// This method generates debug assertions if the MainFrame handle is NULL (typically
// indicating that PCSX2 is running in NoGUI mode, or that the main frame has been
// closed).  In most cases you'll want to use HasMainFrame() to test for thread
// validity first, or use GetMainFramePtr() and manually check for NULL (choice
// is a matter of programmer preference).
MainEmuFrame& Pcsx2App::GetMainFrame() const
{
	pxAssume( ((uptr)GetTopWindow()) == ((uptr)m_MainFrame) );
	pxAssume( m_MainFrame != NULL );
	return *m_MainFrame;
}

GSFrame& Pcsx2App::GetGSFrame() const
{
	pxAssume( m_gsFrame != NULL );
	return *m_gsFrame;
}

void AppApplySettings( const AppConfig* oldconf )
{
	AffinityAssert_AllowFromMain();

	g_Conf->Folders.ApplyDefaults();

	// Ensure existence of necessary documents folders.  Plugins and other parts
	// of PCSX2 rely on them.

	g_Conf->Folders.MemoryCards.Mkdir();
	g_Conf->Folders.Savestates.Mkdir();
	g_Conf->Folders.Snapshots.Mkdir();

	g_Conf->EmuOptions.BiosFilename = g_Conf->FullpathToBios();

	ScopedCoreThreadSuspend suspend_core;

	if( g_plugins != NULL )
		g_plugins->SetSettingsFolder( GetSettingsFolder().ToString() );
	
	RelocateLogfile();

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

	int toSend = 0;
	sApp.Source_SettingsApplied().Dispatch( toSend );
	suspend_core.Resume();
}

static wxFileConfig _dud_config;

AppIniSaver::AppIniSaver()
	: IniSaver( (GetAppConfig() != NULL) ? *GetAppConfig() : _dud_config )
{
}

AppIniLoader::AppIniLoader()
	: IniLoader( (GetAppConfig() != NULL) ? *GetAppConfig() : _dud_config )
{
}

void AppLoadSettings()
{
	if( !AffinityAssert_AllowFromMain() ) return;

	AppIniLoader loader;
	g_Conf->LoadSave( loader );
	wxGetApp().Source_SettingsLoadSave().Dispatch( loader );
}

void AppSaveSettings()
{
	if( !AffinityAssert_AllowFromMain() ) return;

	AppIniSaver saver;
	g_Conf->LoadSave( saver );
	wxGetApp().Source_SettingsLoadSave().Dispatch( saver );
}

// This function works something like setjmp/longjmp, in that the return value indicates if the
// function actually executed the specified method or not.
//
// Returns:
//   FALSE if the method was not posted to the main thread (meaning this IS the main thread!)
//   TRUE if the method was posted.
//
bool Pcsx2App::SelfMethodInvoke( FnType_AppMethod method )
{
	if( wxThread::IsMain() ) return false;

	Semaphore sem;
	pxInvokeMethodEvent evt( method, sem );
	AddPendingEvent( evt );
	sem.Wait();

	return true;
}

bool Pcsx2App::SelfMethodPost( FnType_AppMethod method )
{
	if( wxThread::IsMain() ) return false;
	pxInvokeMethodEvent evt( method );
	AddPendingEvent( evt );
	return true;
}

void Pcsx2App::OpenGsPanel()
{
	if( SelfMethodInvoke( &Pcsx2App::OpenGsPanel ) ) return;

	if( m_gsFrame == NULL )
	{
		m_gsFrame = new GSFrame( m_MainFrame, L"PCSX2" );
		m_gsFrame->SetFocus();
	}
	
	pxAssumeDev( !GetPluginManager().IsOpen( PluginId_GS ), "GS Plugin must be closed prior to opening a new Gs Panel!" );

	m_gsFrame->Show();
	pDsp = (uptr)m_gsFrame->GetViewport()->GetHandle();

	// The "in the main window" quickie hack...
	//pDsp = (uptr)m_MainFrame->m_background.GetHandle();
}

void Pcsx2App::CloseGsPanel()
{
	if( SelfMethodInvoke( &Pcsx2App::CloseGsPanel ) ) return;

	if( m_gsFrame != NULL )
	{
		if( GSPanel* woot = (GSPanel*)m_gsFrame->FindWindowByName(L"GSPanel") )
			woot->Destroy();
	}
}

void Pcsx2App::OnGsFrameClosed()
{
	CoreThread.Suspend();
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

static int _sysexec_cdvdsrc_type = -1;
static wxString _sysexec_elf_override;

static void _sendmsg_SysExecute()
{
	if( !CoreThread.AcquireResumeLock() )
	{
		DbgCon.WriteLn( "(SysExecute) another resume lock or message is already pending; no message posted." );
		return;
	}

	AppSaveSettings();
	wxGetApp().PostCommand( pxEvt_SysExecute, _sysexec_cdvdsrc_type );
}

static void OnSysExecuteAfterPlugins( const wxCommandEvent& loadevt )
{
	if( (wxTheApp == NULL) || !((Pcsx2App*)wxTheApp)->m_CorePlugins ) return;
	_sendmsg_SysExecute();
}

// Executes the emulator using a saved/existing virtual machine state and currently
// configured CDVD source device.
void Pcsx2App::SysExecute()
{
	_sysexec_cdvdsrc_type = -1;
	_sysexec_elf_override = CoreThread.GetElfOverride();

	if( !m_CorePlugins )
	{
		LoadPluginsPassive( OnSysExecuteAfterPlugins );
		return;
	}

	DbgCon.WriteLn( Color_Gray, "(SysExecute) Queuing request to re-execute existing VM state." );
	_sendmsg_SysExecute();
}

// Executes the specified cdvd source and optional elf file.  This command performs a
// full closure of any existing VM state and starts a fresh VM with the requested
// sources.
void Pcsx2App::SysExecute( CDVD_SourceType cdvdsrc, const wxString& elf_override )
{
	_sysexec_cdvdsrc_type = (int)cdvdsrc;
	_sysexec_elf_override = elf_override;

	if( !m_CorePlugins )
	{
		LoadPluginsPassive( OnSysExecuteAfterPlugins );
		return;
	}

	DbgCon.WriteLn( Color_Gray, "(SysExecute) Queuing request for new VM state." );
	_sendmsg_SysExecute();
}

// This message performs actual system execution (as dictated by SysExecute variants).
// It is implemented as a message handler so that it can be triggered in response to
// the completion of other dependent activities, namely loading plugins.
void Pcsx2App::OnSysExecute( wxCommandEvent& evt )
{
	CoreThread.ReleaseResumeLock();

	if( sys_resume_lock > 0 )
	{
		Console.WriteLn( "SysExecute: State is locked, ignoring Execute request!" );
		return;
	}

	// if something unloaded plugins since this messages was queued then it's best to ignore
	// it, because apparently too much stuff is going on and the emulation states are wonky.
	if( !m_CorePlugins ) return;

	DbgCon.WriteLn( Color_Gray, "(MainThread) SysExecute received." );

	if( evt.GetInt() != -1 ) CoreThread.Reset(); else CoreThread.Suspend();
	CDVDsys_SetFile( CDVDsrc_Iso, g_Conf->CurrentIso );
	if( evt.GetInt() != -1 )
		CDVDsys_ChangeSource( (CDVD_SourceType)evt.GetInt() );
	else if( CDVD == NULL )
		CDVDsys_ChangeSource( CDVDsrc_NoDisc );
	
	if( !CoreThread.HasValidState() )
		CoreThread.SetElfOverride( _sysexec_elf_override );

	CoreThread.Resume();
}

// Full system reset stops the core thread and unloads all core plugins *completely*.
void Pcsx2App::SysReset()
{
	StateCopy_Clear();

	CoreThread.Reset();
	CoreThread.ReleaseResumeLock();
	m_CorePlugins = NULL;
}

// Returns true if there is a "valid" virtual machine state from the user's perspective.  This
// means the user has started the emulator and not issued a full reset.
// Thread Safety: The state of the system can change in parallel to execution of the
// main thread.  If you need to perform an extended length activity on the execution
// state (such as saving it), you *must* suspend the Corethread first!
__forceinline bool SysHasValidState()
{
	return CoreThread.HasValidState() || StateCopy_IsValid();
}

// Writes text to console and updates the window status bar and/or HUD or whateverness.
// FIXME: This probably isn't thread safe. >_<
void SysStatus( const wxString& text )
{
	// mirror output to the console!
	Console.WriteLn( text.c_str() );
	sMainFrame.SetStatusText( text );
}

// Applies a new active iso source file
void SysUpdateIsoSrcFile( const wxString& newIsoFile )
{
	g_Conf->CurrentIso = newIsoFile;
	sMainFrame.UpdateIsoSrcSelection();
}

bool HasMainFrame()
{
	return wxTheApp && wxGetApp().HasMainFrame();
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

// Returns a pointer to the main frame of the GUI (frame may be hidden from view), or
// NULL if no main frame exists (NoGUI mode and/or the frame has been destroyed).  If
// the wxApp is NULL then this will also return NULL.
MainEmuFrame*	GetMainFramePtr()
{
	return wxTheApp ? wxGetApp().GetMainFramePtr() : NULL;
}

SysCoreThread& GetCoreThread()
{
	return CoreThread;
}

SysMtgsThread& GetMTGS()
{
	return mtgsThread;
}

SysCoreAllocations& GetSysCoreAlloc()
{
	return *wxGetApp().m_CoreAllocs;
}
