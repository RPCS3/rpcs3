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
#include "Utilities/ScopedPtr.h"
#include "ConfigurationPanels.h"

#include <wx/dynlib.h>
#include <wx/dir.h>

// Allows us to force-disable threading for debugging/troubleshooting
static const bool DisableThreading =
#ifdef __LINUX__
	true;		// linux appears to have threading issues with loadlibrary.
#else
	false;
#endif

using namespace wxHelpers;
using namespace Threading;

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(pxEVT_EnumeratedNext, -1)
	DECLARE_EVENT_TYPE(pxEVT_EnumerationFinished, -1)
	DECLARE_EVENT_TYPE(pxEVT_ShowStatusBar, -1)
END_DECLARE_EVENT_TYPES()

DEFINE_EVENT_TYPE(pxEVT_EnumeratedNext)
DEFINE_EVENT_TYPE(pxEVT_EnumerationFinished);
DEFINE_EVENT_TYPE(pxEVT_ShowStatusBar);

typedef s32		(CALLBACK* PluginTestFnptr)();
typedef void	(CALLBACK* PluginConfigureFnptr)();

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

			Console::Notice("%s Plugin %s:  Version %x != %x", info.shortname, m_plugpath.c_str(), 0xff&(version >> 16), info.version);
		}
		return false;
	}

	bool Test( int pluginTypeIndex ) const
	{
		// all test functions use the same parameterless API, so just pick one arbitrarily (I pick PAD!)
		PluginTestFnptr testfunc = (PluginTestFnptr)m_plugin.GetSymbol( wxString::FromAscii( tbl_PluginInfo[pluginTypeIndex].shortname ) + L"test" );
		if( testfunc == NULL ) return false;
		return (testfunc() == 0);
	}

	wxString GetName() const
	{
		wxASSERT( m_GetLibName != NULL );
		return wxString::FromAscii(m_GetLibName());
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

Panels::PluginSelectorPanel::StatusPanel::StatusPanel( wxWindow* parent ) :
	wxPanelWithHelpers( parent )
,	m_gauge( *new wxGauge( this, wxID_ANY, 10 ) )
,	m_label( *new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE ) )
,	m_progress( 0 )
{
	wxBoxSizer& s_main = *new wxBoxSizer( wxVERTICAL );

	AddStaticText( s_main, _( "Enumerating available plugins..." ) );
	s_main.Add( &m_gauge, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT, 32 ) );
	s_main.Add( &m_label, SizerFlags::StdExpand() );

	// The status bar only looks right if I use SetSizerAndFit() here.
	SetSizerAndFit( &s_main );
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

// ------------------------------------------------------------------------
Panels::PluginSelectorPanel::ComboBoxPanel::ComboBoxPanel( PluginSelectorPanel* parent ) :
	wxPanelWithHelpers( parent )
,	m_FolderPicker(	*new DirPickerPanel( this, FolderId_Plugins,
		_("Plugins Search Path:"),
		_("Select a folder with PCSX2 plugins") )
	)
{
	wxBoxSizer& s_main( *new wxBoxSizer( wxVERTICAL ) );
	wxFlexGridSizer& s_plugin( *new wxFlexGridSizer( NumPluginTypes, 3, 16, 10 ) );
	s_plugin.SetFlexibleDirection( wxHORIZONTAL );
	s_plugin.AddGrowableCol( 1 );		// expands combo boxes to full width.

	for( int i=0; i<NumPluginTypes; ++i )
	{
		s_plugin.Add(
			new wxStaticText( this, wxID_ANY, tbl_PluginInfo[i].GetShortname() ),
			wxSizerFlags().Border( wxTOP | wxLEFT, 2 )
		);
		s_plugin.Add(
			m_combobox[i] = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ),
			wxSizerFlags().Expand()
		);
		wxButton* bleh = new wxButton( this, ButtonId_Configure, L"Configure..." );
		bleh->SetClientData( (void*)(int)tbl_PluginInfo[i].id );
		s_plugin.Add( bleh );
	}

	m_FolderPicker.SetStaticDesc( _("Click the Browse button to select a different folder for PCSX2 plugins.") );

	s_main.Add( &s_plugin, wxSizerFlags().Expand() );
	s_main.AddSpacer( 6 );
	s_main.Add( &m_FolderPicker, SizerFlags::StdExpand() );

	SetSizer( &s_main );
}

void Panels::PluginSelectorPanel::ComboBoxPanel::Reset()
{
	for( int i=0; i<NumPluginTypes; ++i )
	{
		m_combobox[i]->Clear();
		m_combobox[i]->SetSelection( wxNOT_FOUND );
		m_combobox[i]->SetValue( wxEmptyString );
	}
}

// ------------------------------------------------------------------------
Panels::PluginSelectorPanel::PluginSelectorPanel( wxWindow& parent, int idealWidth ) :
	BaseSelectorPanel( parent, idealWidth )
,	m_StatusPanel( *new StatusPanel( this ) )
,	m_ComponentBoxes( *new ComboBoxPanel( this ) )
{
	// note: the status panel is a floating window, so that it can be positioned in the
	// center of the dialog after it's been fitted to the contents.

	wxBoxSizer& s_main( *new wxBoxSizer( wxVERTICAL ) );
	s_main.Add( &m_ComponentBoxes, SizerFlags::StdExpand().ReserveSpaceEvenIfHidden() );

	m_StatusPanel.Hide();
	m_ComponentBoxes.Hide();

	// refresh button used for diagnostics... (don't think there's a point to having one otherwise) --air
	//wxButton* refresh = new wxButton( this, wxID_ANY, L"Refresh" );
	//s_main.Add( refresh );
	//Connect( refresh->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PluginSelectorPanel::OnRefresh ) );

	SetSizer( &s_main );

	Connect( pxEVT_EnumeratedNext,		wxCommandEventHandler( PluginSelectorPanel::OnProgress ) );
	Connect( pxEVT_EnumerationFinished,	wxCommandEventHandler( PluginSelectorPanel::OnEnumComplete ) );
	Connect( pxEVT_ShowStatusBar,		wxCommandEventHandler( PluginSelectorPanel::OnShowStatusBar ) );
	Connect( ButtonId_Configure, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PluginSelectorPanel::OnConfigure_Clicked ) );
}

Panels::PluginSelectorPanel::~PluginSelectorPanel()
{
	CancelRefresh();		// in case the enumeration thread is currently refreshing...
}

void Panels::PluginSelectorPanel::ReloadSettings()
{
	m_ComponentBoxes.GetDirPicker().Reset();
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

	for( int i=0; i<NumPluginTypes; ++i )
	{
		int sel = m_ComponentBoxes.Get(i).GetSelection();
		if( sel == wxNOT_FOUND )
		{
			wxString plugname( tbl_PluginInfo[i].GetShortname() );

			throw Exception::CannotApplySettings( this,
				// English Log
				wxsFormat( L"PluginSelectorPanel: Invalid or missing selection for the %s plugin.", plugname.c_str() ),

				// Translated
				wxsFormat( L"Please select a valid plugin for the %s.", plugname.c_str() ) + L"\n\n" + GetApplyFailedMsg()
			);
		}

		g_Conf->BaseFilenames.Plugins[tbl_PluginInfo[i].id] = GetFilename((int)m_ComponentBoxes.Get(i).GetClientData(sel));
	}

	// ----------------------------------------------------------------------------
	// Make sure folders are up to date, and try to load/reload plugins if needed...

	g_Conf->Folders.ApplyDefaults();

	// Need to unload the current emulation state if the user changed plugins, because
	// the whole plugin system needs to be re-loaded.

	const PluginInfo* pi = tbl_PluginInfo; do {
		if( g_Conf->FullpathTo( pi->id ) != g_Conf->FullpathTo( pi->id ) )
			break;
	} while( ++pi, pi->shortname != NULL );

	if( pi->shortname != NULL )
	{
		if( wxGetApp().m_CoreThread )
		{
			// [TODO] : Post notice that this shuts down existing emulation, and may not safely recover.
		}

		wxGetApp().SysReset();
	}

	if( !wxGetApp().m_CorePlugins )
	{
		try
		{
			LoadPluginsImmediate();
		}
		catch( Exception::PluginError& ex )
		{
			// Rethrow PluginLoadErrors as a failure to Apply...

			wxString plugname( tbl_PluginInfo[ex.PluginId].GetShortname() );

			throw Exception::CannotApplySettings( this,
				// English Log
				ex.FormatDiagnosticMessage(),

				// Translated
				wxsFormat( L"The selected %s plugin failed to load.", plugname.c_str() ) + L"\n\n" + GetApplyFailedMsg()
			);
		}
	}
}

void Panels::PluginSelectorPanel::CancelRefresh()
{
}

void Panels::PluginSelectorPanel::DoRefresh()
{
	m_ComponentBoxes.Reset();
	if( !m_FileList )
	{
		wxCommandEvent evt;
		OnEnumComplete( evt );
		return;
	}

	// Disable all controls until enumeration is complete.
	// Show status bar for plugin enumeration.  Use a pending event so that
	// the window's size can get initialized properly before trying to custom-
	// fit the status panel to it.

	m_ComponentBoxes.Hide();
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
	m_EnumeratorThread = NULL;			// make sure the thread is STOPPED, just in case...

	bool validated = true;

	// re-enumerate plugins, and if anything changed then we need to wipe
	// the contents of the combo boxes and re-enumerate everything.

	// Impl Note: ScopedPtr used so that resources get cleaned up if an exception
	// occurs during file enumeration.
	ScopedPtr<wxArrayString> pluginlist( new wxArrayString() );

	int pluggers = EnumeratePluginsInFolder( m_ComponentBoxes.GetPluginsPath(), pluginlist );

	if( !m_FileList || (*pluginlist != *m_FileList) )
		validated = false;

	if( pluggers == 0 )
	{
		m_FileList = NULL;
		return validated;
	}

	m_FileList.SwapPtr( pluginlist );

	m_StatusPanel.SetGaugeLength( pluggers );

	return validated;
}

void Panels::PluginSelectorPanel::OnConfigure_Clicked( wxCommandEvent& evt )
{
	PluginsEnum_t pid = (PluginsEnum_t)(int)((wxEvtHandler*)evt.GetEventObject())->GetClientData();

	int sel = m_ComponentBoxes.Get(pid).GetSelection();
	if( sel == wxNOT_FOUND ) return;
	wxDynamicLibrary dynlib( (*m_FileList)[(int)m_ComponentBoxes.Get(pid).GetClientData(sel)] );
	if( PluginConfigureFnptr configfunc = (PluginConfigureFnptr)dynlib.GetSymbol( tbl_PluginInfo[pid].GetShortname() + L"configure" ) )
	{
		wxWindowDisabler disabler;
		configfunc();
	}
}

void Panels::PluginSelectorPanel::OnShowStatusBar( wxCommandEvent& evt )
{
	m_StatusPanel.SetSize( m_ComponentBoxes.GetSize().GetWidth() - 8, wxDefaultCoord );
	m_StatusPanel.CentreOnParent();
	m_StatusPanel.Show();
}

void Panels::PluginSelectorPanel::OnEnumComplete( wxCommandEvent& evt )
{
	m_EnumeratorThread = NULL;

	// fixme: Default plugins should be picked based on the timestamp of the DLL or something?
	//  (for now we just force it to selection zero if nothing's selected)

	int emptyBoxes = 0;
	for( int i=0; i<NumPluginTypes; ++i )
	{
		if( m_ComponentBoxes.Get(i).GetCount() <= 0 )
			emptyBoxes++;

		else if( m_ComponentBoxes.Get(i).GetSelection() == wxNOT_FOUND )
			m_ComponentBoxes.Get(i).SetSelection( 0 );
	}

	m_ComponentBoxes.Show();
	m_StatusPanel.Hide();
	m_StatusPanel.Reset();
}


void Panels::PluginSelectorPanel::OnProgress( wxCommandEvent& evt )
{
	if( !m_FileList ) return;

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

	m_StatusPanel.AdvanceProgress( (evtidx < m_FileList->Count()-1) ?
		(*m_FileList)[evtidx + 1] : wxString(_("Completing tasks..."))
	);

	EnumeratedPluginInfo& result( m_EnumeratorThread->Results[evtidx] );

	if( result.TypeMask == 0 )
	{
		Console::Error( L"Some kinda plugin failure: " + (*m_FileList)[evtidx] );
	}

	for( int i=0; i<NumPluginTypes; ++i )
	{
		if( result.TypeMask & tbl_PluginInfo[i].typemask )
		{
			if( result.PassedTest & tbl_PluginInfo[i].typemask )
			{
				int sel = m_ComponentBoxes.Get(i).Append( wxsFormat( L"%s %s [%s]",
					result.Name.c_str(), result.Version[i].c_str(), Path::GetFilenameWithoutExt( (*m_FileList)[evtidx] ).c_str() ),
					(void*)evtidx
				);

				wxFileName left( (*m_FileList)[evtidx] );
				wxFileName right( g_Conf->FullpathTo(tbl_PluginInfo[i].id) );

				left.MakeAbsolute();
				right.MakeAbsolute();

				if( left == right )
					m_ComponentBoxes.Get(i).SetSelection( sel );
			}
		}
	}
}


// --------------------------------------------------------------------------------------
//  EnumThread   method implementations
// --------------------------------------------------------------------------------------

Panels::PluginSelectorPanel::EnumThread::EnumThread( PluginSelectorPanel& master ) :
	PersistentThread()
,	Results( master.FileCount(), L"PluginSelectorResults" )
,	m_master( master )
{
	Results.MatchLengthToAllocatedSize();
}

void Panels::PluginSelectorPanel::EnumThread::DoNextPlugin( int curidx )
{
	DbgCon::WriteLn( L"Enumerating Plugin: " + m_master.GetFilename( curidx ) );

	try
	{
		EnumeratedPluginInfo& result( Results[curidx] );
		result.TypeMask = 0;

		PluginEnumerator penum( m_master.GetFilename( curidx ) );

		result.Name = penum.GetName();
		for( int pidx=0; pidx<PluginId_Count; ++pidx )
		{
			const PluginsEnum_t pid = (PluginsEnum_t)pidx;
			result.TypeMask |= tbl_PluginInfo[pid].typemask;
			if( penum.CheckVersion( pid ) )
			{
				result.PassedTest |= tbl_PluginInfo[pid].typemask;
				penum.GetVersionString( result.Version[pid], pidx );
			}
		}
	}
	catch( Exception::BadStream& ex )
	{
		Console::Status( ex.FormatDiagnosticMessage() );
	}

	wxCommandEvent yay( pxEVT_EnumeratedNext );
	yay.SetExtraLong( curidx );
	m_master.GetEventHandler()->AddPendingEvent( yay );
}

sptr Panels::PluginSelectorPanel::EnumThread::ExecuteTask()
{
	DevCon::Status( "Plugin Enumeration Thread started..." );

	wxGetApp().Ping();		// gives the gui thread some time to refresh
	Sleep( 3 );

	for( int curidx=0; curidx < m_master.FileCount(); ++curidx )
	{
		DoNextPlugin( curidx );
		if( (curidx & 3) == 3 ) wxGetApp().Ping();		// gives the gui thread some time to refresh
		pthread_testcancel();
	}

	wxCommandEvent done( pxEVT_EnumerationFinished );
	m_master.GetEventHandler()->AddPendingEvent( done );

	DevCon::Status( "Plugin Enumeration Thread complete!" );
	return 0;
}
