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
#include "Plugins.h"
#include "SaveState.h"
#include "Utilities/ScopedPtr.h"
#include "ConfigurationPanels.h"
#include "Dialogs/ModalPopups.h"

#include <wx/dynlib.h>
#include <wx/dir.h>

// Allows us to force-disable threading for debugging/troubleshooting
static const bool DisableThreading =
#ifdef __LINUX__
	true;		// linux appears to have threading issues with loadlibrary.
#else
	false;
#endif

using namespace pxSizerFlags;
using namespace Threading;

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(pxEVT_EnumeratedNext, -1)
	DECLARE_EVENT_TYPE(pxEVT_EnumerationFinished, -1)
	DECLARE_EVENT_TYPE(pxEVT_ShowStatusBar, -1)
END_DECLARE_EVENT_TYPES()

DEFINE_EVENT_TYPE(pxEVT_EnumeratedNext)
DEFINE_EVENT_TYPE(pxEVT_EnumerationFinished);
DEFINE_EVENT_TYPE(pxEVT_ShowStatusBar);

typedef s32		(CALLBACK* TestFnptr)();
typedef void	(CALLBACK* ConfigureFnptr)();

//////////////////////////////////////////////////////////////////////////////////////////
//
namespace Exception
{
	class NotEnumerablePlugin : public BadStream
	{
	public:
		DEFINE_STREAM_EXCEPTION( NotEnumerablePlugin, wxLt("File is not a PCSX2 plugin") );
	};
}

// --------------------------------------------------------------------------------------
//  PluginEnumerator class
// --------------------------------------------------------------------------------------

class PluginEnumerator
{
protected:
	wxString			m_plugpath;
	wxDynamicLibrary	m_plugin;

	_PS2EgetLibType     m_GetLibType;
	_PS2EgetLibName     m_GetLibName;
	_PS2EgetLibVersion2 m_GetLibVersion2;

	u32 m_type;

public:

	// Constructor!
	//
	// Possible Exceptions:
	//   BadStream				- thrown if the provided file is simply not a loadable DLL.
	//   NotEnumerablePlugin	- thrown if the DLL is not a PCSX2 plugin, or if it's of an unsupported version.
	//
	PluginEnumerator( const wxString& plugpath ) :
		m_plugpath( plugpath )
	,	m_plugin()
	{
		if( !m_plugin.Load( m_plugpath ) )
			throw Exception::BadStream( m_plugpath, "File is not a valid dynamic library." );

		wxDoNotLogInThisScope please;
		m_GetLibType		= (_PS2EgetLibType)m_plugin.GetSymbol( L"PS2EgetLibType" );
		m_GetLibName		= (_PS2EgetLibName)m_plugin.GetSymbol( L"PS2EgetLibName" );
		m_GetLibVersion2	= (_PS2EgetLibVersion2)m_plugin.GetSymbol( L"PS2EgetLibVersion2" );

		if( m_GetLibType == NULL || m_GetLibName == NULL || m_GetLibVersion2 == NULL )
			throw Exception::NotEnumerablePlugin( m_plugpath );

		m_type = m_GetLibType();
	}

	bool CheckVersion( PluginsEnum_t pluginTypeIndex ) const
	{
		const PluginInfo& info( tbl_PluginInfo[pluginTypeIndex] );
		if( m_type & info.typemask )
		{
			int version = m_GetLibVersion2( info.typemask );
			if ( ((version >> 16)&0xff) == tbl_PluginInfo[pluginTypeIndex].version )
				return true;

			Console.Warning("%s Plugin %s:  Version %x != %x", info.shortname, m_plugpath.c_str(), 0xff&(version >> 16), info.version);
		}
		return false;
	}

	bool Test( int pluginTypeIndex ) const
	{
		// all test functions use the same parameterless API, so just pick one arbitrarily (I pick PAD!)
		TestFnptr testfunc = (TestFnptr)m_plugin.GetSymbol( fromUTF8( tbl_PluginInfo[pluginTypeIndex].shortname ) + L"test" );
		if( testfunc == NULL ) return false;
		return (testfunc() == 0);
	}

	wxString GetName() const
	{
		pxAssert( m_GetLibName != NULL );
		return fromUTF8(m_GetLibName());
	}

	void GetVersionString( wxString& dest, int pluginTypeIndex ) const
	{
		const PluginInfo& info( tbl_PluginInfo[pluginTypeIndex] );
		int version = m_GetLibVersion2( info.typemask );
		dest.Printf( L"%d.%d.%d", (version>>8)&0xff, version&0xff, (version>>24)&0xff );
	}
};

static const wxString failed_separator( L"--------   Unsupported Plugins  --------" );

// --------------------------------------------------------------------------------------
//  PluginSelectorPanel  implementations
// --------------------------------------------------------------------------------------

Panels::PluginSelectorPanel::StatusPanel::StatusPanel( wxWindow* parent )
	: wxPanelWithHelpers( parent, wxVERTICAL )
	, m_gauge( *new wxGauge( this, wxID_ANY, 10 ) )
	, m_label( *new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE ) )
{
	m_progress = 0;

	m_gauge.SetToolTip( _("I'm givin' her all she's got, Captain!") );

	*this	+= new pxStaticHeading( this, _( "Enumerating available plugins..." ) );
	*this	+= m_gauge	| wxSF.Expand().Border( wxLEFT | wxRIGHT, 32 );
	*this	+= m_label	| StdExpand();

	Fit();
}

void Panels::PluginSelectorPanel::StatusPanel::SetGaugeLength( int len )
{
	m_gauge.SetRange( len );
}

void Panels::PluginSelectorPanel::StatusPanel::AdvanceProgress( const wxString& msg )
{
	m_label.SetLabel( msg );
	m_gauge.SetValue( ++m_progress );
}

void Panels::PluginSelectorPanel::StatusPanel::Reset()
{
	m_gauge.SetValue( m_progress = 0 );
	m_label.SetLabel( wxEmptyString );
}

// Id for all Configure buttons (any non-negative arbitrary integer will do)
static const int ButtonId_Configure = 51;

// =====================================================================================================
//  PluginSelectorPanel::ComboBoxPanel
// =====================================================================================================
Panels::PluginSelectorPanel::ComboBoxPanel::ComboBoxPanel( PluginSelectorPanel* parent )
	: wxPanelWithHelpers( parent, wxVERTICAL )
	, m_Listener_CorePluginStatus( wxGetApp().Source_CorePluginStatus(), EventListener<PluginEventType>	( this, OnCorePluginStatusChanged ) )
	, m_FolderPicker(	*new DirPickerPanel( this, FolderId_Plugins,
		_("Plugins Search Path:"),
		_("Select a folder with PCSX2 plugins") )
	)
{
	wxFlexGridSizer& s_plugin( *new wxFlexGridSizer( PluginId_Count, 3, 16, 10 ) );
	s_plugin.SetFlexibleDirection( wxHORIZONTAL );
	s_plugin.AddGrowableCol( 1 );		// expands combo boxes to full width.

	const PluginInfo* pi = tbl_PluginInfo; do
	{
		const PluginsEnum_t pid = pi->id;
		wxStaticText* text = new wxStaticText( this, wxID_ANY, pi->GetShortname() );

		m_combobox[pid] = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );

		m_configbutton[pid] = new wxButton( this, ButtonId_Configure, L"Configure..." );
		m_configbutton[pid]->SetClientData( (void*)(int)pid );

		s_plugin	+= text				| wxSF.Border( wxTOP | wxLEFT, 2 );
		s_plugin	+= m_combobox[pid]	| wxSF.Expand();
		s_plugin	+= m_configbutton[pid];
	} while( ++pi, pi->shortname != NULL );

	m_FolderPicker.SetStaticDesc( _("Click the Browse button to select a different folder for PCSX2 plugins.") );

	*this	+= s_plugin			| wxSF.Expand();
	*this	+= 6;
	*this	+= m_FolderPicker	| StdExpand();
}

void Panels::PluginSelectorPanel::ComboBoxPanel::Reset()
{
	for( int i=0; i<PluginId_Count; ++i )
	{
		m_combobox[i]->Clear();
		m_combobox[i]->SetSelection( wxNOT_FOUND );
		m_combobox[i]->SetValue( wxEmptyString );

		m_configbutton[i]->Disable();
	}
}

// =====================================================================================================
//  PluginSelectorPanel
// =====================================================================================================
void __evt_fastcall Panels::PluginSelectorPanel::OnCorePluginStatusChanged( void* obj, PluginEventType& evt )
{
	if( obj == NULL ) return;
	if( (evt != PluginsEvt_Loaded) && (evt != PluginsEvt_Unloaded) ) return;		// everything else we don't care about

	PluginSelectorPanel& panel = *(PluginSelectorPanel*)obj;

	const PluginInfo* pi = tbl_PluginInfo; do {
		panel.m_ComponentBoxes->GetConfigButton(pi->id).Enable(
			(panel.m_FileList==NULL) ? false :
			g_Conf->FullpathMatchTest( pi->id, (*panel.m_FileList)[pi->id] )
		);
	} while( ++pi, pi->shortname != NULL );

}

Panels::PluginSelectorPanel::PluginSelectorPanel( wxWindow* parent, int idealWidth )
	: BaseSelectorPanel( parent )
	, m_Listener_CorePluginStatus( wxGetApp().Source_CorePluginStatus(), EventListener<PluginEventType>	( this, OnCorePluginStatusChanged ) )
{
	if( idealWidth != wxDefaultCoord ) m_idealWidth = idealWidth;

	m_StatusPanel		= new StatusPanel( this );
	m_ComponentBoxes	= new ComboBoxPanel( this );

	// note: the status panel is a floating window, so that it can be positioned in the
	// center of the dialog after it's been fitted to the contents.

	*this	+= m_ComponentBoxes | StdExpand().ReserveSpaceEvenIfHidden();

	m_StatusPanel->Hide();
	m_ComponentBoxes->Hide();

	// refresh button used for diagnostics... (don't think there's a point to having one otherwise) --air
	//wxButton* refresh = new wxButton( this, wxID_ANY, L"Refresh" );
	//s_main.Add( refresh );
	//Connect( refresh->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PluginSelectorPanel::OnRefresh ) );

	Connect( pxEVT_EnumeratedNext,				wxCommandEventHandler( PluginSelectorPanel::OnProgress ) );
	Connect( pxEVT_EnumerationFinished,			wxCommandEventHandler( PluginSelectorPanel::OnEnumComplete ) );
	Connect( pxEVT_ShowStatusBar,				wxCommandEventHandler( PluginSelectorPanel::OnShowStatusBar ) );
	Connect( wxEVT_COMMAND_COMBOBOX_SELECTED,	wxCommandEventHandler( PluginSelectorPanel::OnPluginSelected ) );

	Connect( ButtonId_Configure, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PluginSelectorPanel::OnConfigure_Clicked ) );

	OnSettingsChanged();
}

Panels::PluginSelectorPanel::~PluginSelectorPanel() throw()
{
	CancelRefresh();		// in case the enumeration thread is currently refreshing...
}

void Panels::PluginSelectorPanel::OnSettingsChanged()
{
	m_ComponentBoxes->GetDirPicker().Reset();
}

static wxString GetApplyFailedMsg()
{
	return pxE( ".Popup Error:PluginSelector:ApplyFailed",
		L"All plugins must have valid selections for PCSX2 to run.  If you are unable to make\n"
		L"a valid selection due to missing plugins or an incomplete install of PCSX2, then\n"
		L"press cancel to close the Configuration panel."
	);
}

void Panels::PluginSelectorPanel::Apply()
{
	// user never entered plugins panel?  Skip application since combo boxes are invalid/uninitialized.
	if( !m_FileList ) return;

	AppConfig curconf( *g_Conf );

	const PluginInfo* pi = tbl_PluginInfo; do
	{
		const PluginsEnum_t pid = pi->id;
		int sel = m_ComponentBoxes->Get(pid).GetSelection();
		if( sel == wxNOT_FOUND )
		{
			wxString plugname( pi->GetShortname() );

			throw Exception::CannotApplySettings( this,
				// English Log
				wxsFormat( L"PluginSelectorPanel: Invalid or missing selection for the %s plugin.", plugname.c_str() ),

				// Translated
				wxsFormat( L"Please select a valid plugin for the %s.", plugname.c_str() ) + L"\n\n" + GetApplyFailedMsg()
			);
		}

		g_Conf->BaseFilenames.Plugins[pid] = GetFilename((int)m_ComponentBoxes->Get(pid).GetClientData(sel));
	} while( ++pi, pi->shortname != NULL );

	// ----------------------------------------------------------------------------
	// Make sure folders are up to date, and try to load/reload plugins if needed...

	g_Conf->Folders.ApplyDefaults();

	// Need to unload the current emulation state if the user changed plugins, because
	// the whole plugin system needs to be re-loaded.

	pi = tbl_PluginInfo; do {
		if( g_Conf->FullpathTo( pi->id ) != curconf.FullpathTo( pi->id ) )
			break;
	} while( ++pi, pi->shortname != NULL );

	bool isSuspended = false;

	if( pi->shortname != NULL )
	{
		if( CoreThread.IsRunning() )
		{
			// [TODO] : Post notice that this shuts down existing emulation, and may not safely recover.
			wxDialogWithHelpers dialog( this, _("Shutdown PS2 virtual machine?"), wxVERTICAL );
			
			dialog += dialog.Heading( pxE( ".Popup:PluginSelector:ConfirmShutdown",
				L"Warning!  Changing plugins requires a complete shutdown and reset of the PS2 virtual machine. "
				L"PCSX2 will attempt to save and restore the state, but if the newly selected plugins are "
				L"incompatible the recovery may fail, and current progress will be lost."
				L"\n\n"
				L"Are you sure you want to apply settings now?"
			) );

			int result = pxIssueConfirmation( dialog, MsgButtons().OK().Cancel(), L"PluginSelector:ConfirmShutdown" );

			if( result == wxID_CANCEL )
				throw Exception::CannotApplySettings( this, "Cannot apply settings: canceled by user because plugins changed while the emulation state was active.", false );

			// FIXME : We only actually have to save plugins here, except the recovery code
			// in SysCoreThread isn't quite set up yet to handle that (I think...) --air

			isSuspended = CoreThread.Suspend();
			StateCopy_FreezeToMem_Blocking();
		}

		// Don't use SysShutdown, it clears the StateCopy.
		CoreThread.Cancel();
		wxGetApp().m_CorePlugins = NULL;
	}

	if( !wxGetApp().m_CorePlugins )
	{
		try {
			LoadPluginsImmediate();
		}
		catch( Exception::PluginError& ex )
		{
			// Rethrow PluginLoadErrors as a failure to Apply...

			wxString plugname( tbl_PluginInfo[ex.PluginId].GetShortname() );

			throw Exception::CannotApplySettings( this,
				// Diagnostic
				ex.FormatDiagnosticMessage(),

				// Translated
				wxsFormat( _("The selected %s plugin failed to load.\n\nReason: %s\n\n"),
					plugname.c_str(), ex.FormatDisplayMessage().c_str()
				) + GetApplyFailedMsg()
			);
		}
	}

	if( isSuspended ) CoreThread.Resume();
}

void Panels::PluginSelectorPanel::CancelRefresh()
{
}

// This method is a callback from the BaseSelectorPanel.  It is called when the page is shown
// and the page's enumerated selections are valid (meaning we should start our enumeration
// thread!)
void Panels::PluginSelectorPanel::DoRefresh()
{
	m_ComponentBoxes->Reset();
	if( !m_FileList )
	{
		wxCommandEvent evt;
		OnEnumComplete( evt );
		return;
	}

	// Disable all controls until enumeration is complete
	m_ComponentBoxes->Hide();

	// (including next button if it's a Wizard)
	wxWindow* forwardButton = GetGrandParent()->FindWindow( wxID_FORWARD );
	if( forwardButton != NULL )
		forwardButton->Disable();

	// Show status bar for plugin enumeration.  Use a pending event so that
	// the window's size can get initialized properly before trying to custom-
	// fit the status panel to it.
	wxCommandEvent evt( pxEVT_ShowStatusBar );
	GetEventHandler()->AddPendingEvent( evt );

	m_EnumeratorThread.Delete() = new EnumThread( *this );

	if( DisableThreading )
		m_EnumeratorThread->DoNextPlugin( 0 );
	else
		m_EnumeratorThread->Start();
}

bool Panels::PluginSelectorPanel::ValidateEnumerationStatus()
{
	if( m_EnumeratorThread ) return true;		// Cant reset file lists while we're busy enumerating...

	bool validated = true;

	// re-enumerate plugins, and if anything changed then we need to wipe
	// the contents of the combo boxes and re-enumerate everything.

	// Impl Note: ScopedPtr used so that resources get cleaned up if an exception
	// occurs during file enumeration.
	ScopedPtr<wxArrayString> pluginlist( new wxArrayString() );

	int pluggers = EnumeratePluginsInFolder( m_ComponentBoxes->GetPluginsPath(), pluginlist );

	if( !m_FileList || (*pluginlist != *m_FileList) )
		validated = false;

	if( pluggers == 0 )
	{
		m_FileList = NULL;
		return validated;
	}

	m_FileList.SwapPtr( pluginlist );

	m_StatusPanel->SetGaugeLength( pluggers );

	return validated;
}

void Panels::PluginSelectorPanel::OnPluginSelected( wxCommandEvent& evt )
{
	if( IsBeingDeleted() || m_ComponentBoxes->IsBeingDeleted() ) return;

	const PluginInfo* pi = tbl_PluginInfo; do
	{
		wxComboBox& box = m_ComponentBoxes->Get(pi->id);
		if( box.GetId() == evt.GetId() )
		{
			// Button is enabled if:
			//   (a) plugins aren't even loaded yet.
			//   (b) current selection matches exactly the currently configured/loaded plugin.

			bool isSame = (g_plugins==NULL) || g_Conf->FullpathMatchTest( pi->id, (*m_FileList)[(int)box.GetClientData(box.GetSelection())] );
			m_ComponentBoxes->GetConfigButton( pi->id ).Enable( isSame );
			
			if( !isSame ) evt.Skip();		// enabled Apply button! :D
			return;
		}
	} while( ++pi, pi->shortname != NULL );
}

void Panels::PluginSelectorPanel::OnConfigure_Clicked( wxCommandEvent& evt )
{
	if( IsBeingDeleted() ) return;

	PluginsEnum_t pid = (PluginsEnum_t)(int)((wxEvtHandler*)evt.GetEventObject())->GetClientData();

	int sel = m_ComponentBoxes->Get(pid).GetSelection();
	if( sel == wxNOT_FOUND ) return;

	// Only allow configuration if the selected plugin matches exactly our currently loaded one.
	// Otherwise who knows what sort of funny business could happen configuring a plugin while
	// another instance/version is running. >_<

	const wxString filename( (*m_FileList)[(int)m_ComponentBoxes->Get(pid).GetClientData(sel)] );

	if( g_plugins != NULL && !g_Conf->FullpathMatchTest( pid, filename ) )
	{
		Console.Warning( "(PluginSelector) Plugin name mismatch, configuration request ignored." );
		return;
	}

	wxDynamicLibrary dynlib( filename );

	if( ConfigureFnptr configfunc = (ConfigureFnptr)dynlib.GetSymbol( tbl_PluginInfo[pid].GetShortname() + L"configure" ) )
	{
		wxWindowDisabler disabler;
		SaveSinglePluginHelper helper( pid );
		g_plugins->Configure( pid );
	}
}

void Panels::PluginSelectorPanel::OnShowStatusBar( wxCommandEvent& evt )
{
	m_StatusPanel->SetSize( m_ComponentBoxes->GetSize().GetWidth() - 8, wxDefaultCoord );
	m_StatusPanel->CentreOnParent();
	m_StatusPanel->Show();
}

void Panels::PluginSelectorPanel::OnEnumComplete( wxCommandEvent& evt )
{
	m_EnumeratorThread = NULL;

	// fixme: Default plugins should be picked based on the timestamp of the DLL or something?
	//  (for now we just force it to selection zero if nothing's selected)

	int emptyBoxes = 0;
	const PluginInfo* pi = tbl_PluginInfo; do
	{
		const PluginsEnum_t pid = pi->id;
		if( m_ComponentBoxes->Get(pid).GetCount() <= 0 )
			emptyBoxes++;

		else if( m_ComponentBoxes->Get(pid).GetSelection() == wxNOT_FOUND )
		{
			m_ComponentBoxes->Get(pid).SetSelection( 0 );
			m_ComponentBoxes->GetConfigButton(pid).Enable( g_plugins == NULL );
		}
	} while( ++pi, pi->shortname != NULL );

	m_ComponentBoxes->Show();
	m_StatusPanel->Hide();
	m_StatusPanel->Reset();

	wxWindow* forwardButton = GetGrandParent()->FindWindow( wxID_FORWARD );
	if( forwardButton != NULL )
		forwardButton->Enable();
}


void Panels::PluginSelectorPanel::OnProgress( wxCommandEvent& evt )
{
	if( !m_FileList ) return;

	// The thread can get canceled and replaced with a new thread, which means all
	// pending messages should be ignored.
	if( m_EnumeratorThread != (EnumThread*)evt.GetClientData() ) return;

	const size_t evtidx = evt.GetExtraLong();

	if( DisableThreading )
	{
		const int nextidx = evtidx+1;
		if( nextidx == m_FileList->Count() )
		{
			wxCommandEvent done( pxEVT_EnumerationFinished );
			GetEventHandler()->AddPendingEvent( done );
		}
		else
			m_EnumeratorThread->DoNextPlugin( nextidx );
	}

	m_StatusPanel->AdvanceProgress( (evtidx < m_FileList->Count()-1) ?
		(*m_FileList)[evtidx + 1] : wxString(_("Completing tasks..."))
	);

	EnumeratedPluginInfo& result( m_EnumeratorThread->Results[evtidx] );

	if( result.TypeMask == 0 )
	{
		Console.Error( L"Some kinda plugin failure: " + (*m_FileList)[evtidx] );
	}

	const PluginInfo* pi = tbl_PluginInfo; do
	{
		const PluginsEnum_t pid = pi->id;
		if( result.TypeMask & pi->typemask )
		{
			if( result.PassedTest & pi->typemask )
			{
				int sel = m_ComponentBoxes->Get(pid).Append( wxsFormat( L"%s %s [%s]",
					result.Name.c_str(), result.Version[pid].c_str(), Path::GetFilenameWithoutExt( (*m_FileList)[evtidx] ).c_str() ),
					(void*)evtidx
				);

				if( g_Conf->FullpathMatchTest( pid, (*m_FileList)[evtidx] ) )
				{
					m_ComponentBoxes->Get(pid).SetSelection( sel );
					m_ComponentBoxes->GetConfigButton(pid).Enable();
				}
			}
		}
	} while( ++pi, pi->shortname != NULL );
}


// --------------------------------------------------------------------------------------
//  EnumThread   method implementations
// --------------------------------------------------------------------------------------

Panels::PluginSelectorPanel::EnumThread::EnumThread( PluginSelectorPanel& master )
	: PersistentThread()
	, Results( master.FileCount(), L"PluginSelectorResults" )
	, m_master( master )
	, m_hourglass( Cursor_KindaBusy )
{
	Results.MatchLengthToAllocatedSize();
}

void Panels::PluginSelectorPanel::EnumThread::DoNextPlugin( int curidx )
{
	DbgCon.Indent().WriteLn( L"Enumerating Plugin: " + m_master.GetFilename( curidx ) );

	try
	{
		EnumeratedPluginInfo& result( Results[curidx] );
		result.TypeMask = 0;

		PluginEnumerator penum( m_master.GetFilename( curidx ) );

		result.Name = penum.GetName();
		const PluginInfo* pi = tbl_PluginInfo; do
		{
			const PluginsEnum_t pid = pi->id;
			result.TypeMask |= pi->typemask;
			if( penum.CheckVersion( pid ) )
			{
				result.PassedTest |= tbl_PluginInfo[pid].typemask;
				penum.GetVersionString( result.Version[pid], pid );
			}
		} while( ++pi, pi->shortname != NULL );
	}
	catch( Exception::BadStream& ex )
	{
		Console.Warning( ex.FormatDiagnosticMessage() );
	}

	wxCommandEvent yay( pxEVT_EnumeratedNext );
	yay.SetClientData( this );
	yay.SetExtraLong( curidx );
	m_master.GetEventHandler()->AddPendingEvent( yay );
}

void Panels::PluginSelectorPanel::EnumThread::ExecuteTaskInThread()
{
	DevCon.WriteLn( "Plugin Enumeration Thread started..." );

	YieldToMain();
	Sleep( 15 );		// give the window some time to paint.

	for( int curidx=0; curidx < m_master.FileCount(); ++curidx )
	{
		DoNextPlugin( curidx );

		// speed isn't critical here, but the pretty status bar sure is.
		// second try yield should give the status bars UI a "good" chance to refresh before we advance. :)

		YieldToMain();
		YieldToMain();
		//Sleep(150);		// uncomment this to slow down the selector, for debugging threading.
	}

	wxCommandEvent done( pxEVT_EnumerationFinished );
	done.SetClientData( this );
	m_master.GetEventHandler()->AddPendingEvent( done );

	DevCon.WriteLn( "Plugin Enumeration Thread complete!" );
}
