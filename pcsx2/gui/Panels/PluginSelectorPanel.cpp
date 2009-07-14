/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"
#include "Plugins.h"
#include "ConfigurationPanels.h"

#include <wx/dynlib.h>
#include <wx/dir.h>

using namespace wxHelpers;
using namespace Threading;

struct PluginInfo
{
	const char* shortname;
	PluginsEnum_t id;
	int typemask;
	int version;			// minimum version required / supported
};

// Yay, order of this array shouldn't be important. :)
static const PluginInfo tbl_PluginInfo[] = 
{
	{ "GS",		PluginId_GS,	PS2E_LT_GS,		PS2E_GS_VERSION },
	{ "PAD",	PluginId_PAD,	PS2E_LT_PAD,	PS2E_PAD_VERSION },
	{ "SPU2",	PluginId_SPU2,	PS2E_LT_SPU2,	PS2E_SPU2_VERSION },
	{ "CDVD",	PluginId_CDVD,	PS2E_LT_CDVD,	PS2E_CDVD_VERSION },
	{ "DEV9",	PluginId_DEV9,	PS2E_LT_DEV9,	PS2E_DEV9_VERSION },
	{ "USB",	PluginId_USB,	PS2E_LT_USB,	PS2E_USB_VERSION },
	{ "FW",		PluginId_FW,	PS2E_LT_FW,		PS2E_FW_VERSION },

	// SIO is currently unused (legacy?)
	//{ "SIO",	PluginId_SIO,	PS2E_LT_SIO,	PS2E_SIO_VERSION }

};

namespace Exception
{
	class NotPcsxPlugin : public Stream
	{
	public:
		virtual ~NotPcsxPlugin() throw() {}
		explicit NotPcsxPlugin( const wxString& objname ) :
			Stream( objname, wxLt("Dynamic library is not a PCSX2 plugin (or is an unsupported m_version)") ) {}	
	};

};

DECLARE_EVENT_TYPE(wxEVT_EnumeratedNext, -1)
DECLARE_EVENT_TYPE(wxEVT_EnumerationFinished, -1)

DEFINE_EVENT_TYPE(wxEVT_EnumeratedNext)
DEFINE_EVENT_TYPE(wxEVT_EnumerationFinished)

//////////////////////////////////////////////////////////////////////////////////////////
//
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
	//   BadStream     - thrown if the provided file is simply not a loadable DLL.
	//   NotPcsxPlugin - thrown if the DLL is not a PCSX2 plugin, or if it's of an unsupported version.
	//
	PluginEnumerator( const wxString& plugpath ) :
		m_plugpath( plugpath )
	,	m_plugin()
	{
		if( !m_plugin.Load( m_plugpath ) )
			throw Exception::BadStream( m_plugpath, "File is not a valid dynamic library" );

		m_GetLibType		= (_PS2EgetLibType)m_plugin.GetSymbol( L"PS2EgetLibType" );
		m_GetLibName		= (_PS2EgetLibName)m_plugin.GetSymbol( L"PS2EgetLibName" );
		m_GetLibVersion2	= (_PS2EgetLibVersion2)m_plugin.GetSymbol( L"PS2EgetLibVersion2" );
		
		if( m_GetLibType == NULL || m_GetLibName == NULL || m_GetLibVersion2 == NULL )
		{
			throw Exception::NotPcsxPlugin( m_plugpath );
		}
		m_type = m_GetLibType();
	}

	// Parameters:
	//   pluginTypeIndex  - Value from 1 to 8 which represents the plugin's index.
	//
	bool CheckVersion( int pluginTypeIndex ) const
	{
		const PluginInfo& info( tbl_PluginInfo[pluginTypeIndex] );
		if( m_type & info.typemask )
		{
			int version = m_GetLibVersion2( info.typemask );
			if ( ((version >> 16)&0xff) == tbl_PluginInfo[pluginTypeIndex].version )
				return true;

			Console::Notice("%s Plugin %s:  Version %x != %x", params info.shortname, m_plugpath, 0xff&(version >> 16), info.version);
		}
		return false;
	}
	
	bool Test( int pluginTypeIndex ) const
	{
		// all test functions use the same parameterless API, so just pick one arbitrarily (I pick PAD!)
		_PADtest testfunc = (_PADtest)m_plugin.GetSymbol( wxString::FromAscii( tbl_PluginInfo[pluginTypeIndex].shortname ) + L"test" );
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

// ------------------------------------------------------------------------
Panels::PluginSelectorPanel::StatusPanel::StatusPanel( wxWindow* parent, int pluginCount ) :
	wxPanelWithHelpers( parent )
,	m_gauge( *new wxGauge( this, wxID_ANY, pluginCount ) )
,	m_label( *new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE ) )
,	m_progress( 0 )
{
	wxBoxSizer& s_main = *new wxBoxSizer( wxVERTICAL );

	AddStaticText( s_main, _( "Enumerating available plugins..." ), 0, wxALIGN_CENTRE );
	s_main.Add( &m_gauge, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT, 32 ) );
	s_main.Add( &m_label, SizerFlags::StdExpand() );

	SetSizerAndFit( &s_main );
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

// ------------------------------------------------------------------------
Panels::PluginSelectorPanel::ComboBoxPanel::ComboBoxPanel( PluginSelectorPanel* parent ) :
	wxPanelWithHelpers( parent )
{
	wxFlexGridSizer& s_plugin = *new wxFlexGridSizer( NumPluginTypes, 3, 16, 10 );
	s_plugin.SetFlexibleDirection( wxHORIZONTAL );
	s_plugin.AddGrowableCol( 1 );		// expands combo boxes to full width.

	for( int i=0; i<NumPluginTypes; ++i )
	{
		s_plugin.Add(
			new wxStaticText( this, wxID_ANY, wxString::FromAscii( tbl_PluginInfo[i].shortname ) ),
			wxSizerFlags().Border( wxTOP | wxLEFT, 2 )
		);
		s_plugin.Add(
			m_combobox[i] = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ),
			wxSizerFlags().Expand()
		);
		s_plugin.Add( new wxButton( this, wxID_ANY, L"Configure..." ) );
	}

	SetSizerAndFit( &s_plugin );	
}

void Panels::PluginSelectorPanel::ComboBoxPanel::Reset()
{
	for( int i=0; i<NumPluginTypes; ++i )
		m_combobox[i]->Clear();
}

// ------------------------------------------------------------------------
Panels::PluginSelectorPanel::PluginSelectorPanel( wxWindow& parent ) :
	BaseApplicableConfigPanel( &parent )
,	m_FileList()
,	m_StatusPanel( *new StatusPanel( this,
		wxDir::GetAllFiles( g_Conf.Folders.Plugins.ToString(), &m_FileList, wxsFormat( L"*%s", wxDynamicLibrary::GetDllExt()), wxDIR_FILES )
	) )
,	m_ComboBoxes( *new ComboBoxPanel( this ) )
,	m_Uninitialized( true )
,	m_EnumeratorThread( NULL )
{
	wxBoxSizer& s_main = *new wxBoxSizer( wxVERTICAL );

	s_main.Add( &m_ComboBoxes, SizerFlags::StdExpand().ReserveSpaceEvenIfHidden() );

	s_main.AddSpacer( 4 );
	AddStaticText( s_main, _("Tip: Any installed plugins that are not compatible with your hardware or operating system will be listed below a separator."), 388, wxALIGN_CENTRE );
	s_main.AddSpacer( 4 );
	
	s_main.Add( &m_StatusPanel, SizerFlags::StdExpand().ReserveSpaceEvenIfHidden() );

	// refresh button used for diagnostics... (don't think there's a point to having one otherwise) --air
	//wxButton* refresh = new wxButton( this, wxID_ANY, L"Refresh" );
	//s_main.Add( refresh );
	//Connect( refresh->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PluginSelectorPanel::OnRefresh ) );
	
	SetSizerAndFit( &s_main );
	
	Connect( GetId(), wxEVT_SHOW, wxShowEventHandler( PluginSelectorPanel::OnShow ) );

	Connect( wxEVT_EnumeratedNext,		wxCommandEventHandler( PluginSelectorPanel::OnProgress ) );
	Connect( wxEVT_EnumerationFinished,	wxCommandEventHandler( PluginSelectorPanel::OnEnumComplete ) );
}

Panels::PluginSelectorPanel::~PluginSelectorPanel()
{
	// Random crashes on null function pointer if we don't clean up the event...
	Disconnect( GetId(), wxEVT_SHOW, wxShowEventHandler( PluginSelectorPanel::OnShow ) );
	
	// Kill the thread if it's alive.

	safe_delete( m_EnumeratorThread );
}

bool Panels::PluginSelectorPanel::Apply( AppConfig& conf )
{
	for( int i=0; i<NumPluginTypes; ++i )
	{
		int sel = m_ComboBoxes.Get(i).GetSelection();
		if( sel == wxNOT_FOUND ) continue;

		wxFileName relative( m_FileList[(int)m_ComboBoxes.Get(i).GetClientData(sel)] );
		relative.MakeRelativeTo( g_Conf.Folders.Plugins.ToString() );
		conf.BaseFilenames.Plugins[tbl_PluginInfo[i].id] = relative.GetFullPath();
	}
	
	return true;
}

void Panels::PluginSelectorPanel::DoRefresh()
{
	m_Uninitialized = false;

	// Disable all controls until enumeration is complete.

	m_ComboBoxes.Hide();
	m_StatusPanel.Show();

	// Use a thread to load plugins.
	safe_delete( m_EnumeratorThread );
	m_EnumeratorThread = new EnumThread( *this );
	m_EnumeratorThread->Start();
}

// ------------------------------------------------------------------------
void Panels::PluginSelectorPanel::OnShow( wxShowEvent& evt )
{
	evt.Skip();
	if( !evt.GetShow() ) return;

	if( !m_Uninitialized ) return;

	DoRefresh();
}

void Panels::PluginSelectorPanel::OnRefresh( wxCommandEvent& evt )
{
	m_ComboBoxes.Reset();
	DoRefresh();	
}

void Panels::PluginSelectorPanel::OnEnumComplete( wxCommandEvent& evt )
{
	safe_delete( m_EnumeratorThread );

	// fixme: Default plugins should be picked based on the timestamp of the DLL or something?
	//  (for now we just force it to selection zero if nothing's selected)
	
	int emptyBoxes = 0;
	for( int i=0; i<NumPluginTypes; ++i )
	{
		if( m_ComboBoxes.Get(i).GetCount() <= 0 )
			emptyBoxes++;

		else if( m_ComboBoxes.Get(i).GetSelection() == wxNOT_FOUND )
			m_ComboBoxes.Get(i).SetSelection( 0 );
	}
	
	if( emptyBoxes > 0 )
	{
		wxMessageBox( pxE( Msg_Popup_MissingPlugins),
			_("PCSX2 Error - Plugin components not found") );
	}

	m_ComboBoxes.Show();
	m_StatusPanel.Hide();
	m_StatusPanel.Reset();
}


void Panels::PluginSelectorPanel::OnProgress( wxCommandEvent& evt )
{
	size_t evtidx = evt.GetExtraLong();
	m_StatusPanel.AdvanceProgress( (evtidx < m_FileList.Count()-1) ?
		m_FileList[evtidx + 1] : _("Completing tasks...")
	);

	EnumeratedPluginInfo& result( m_EnumeratorThread->Results[evtidx] );

	for( int i=0; i<NumPluginTypes; ++i )
	{
		if( result.TypeMask & tbl_PluginInfo[i].typemask )
		{
			if( result.PassedTest & tbl_PluginInfo[i].typemask )
			{
				int sel = m_ComboBoxes.Get(i).Append( wxsFormat( L"%s %s [%s]",
					result.Name.c_str(), result.Version[i].c_str(), Path::GetFilenameWithoutExt( m_FileList[evtidx] ).c_str() ),
					(void*)evtidx
				);
				
				wxFileName left( m_FileList[evtidx] );
				wxFileName right( g_Conf.FullpathTo(tbl_PluginInfo[i].id) );

				left.MakeRelativeTo( g_Conf.Folders.Plugins.ToString() );
				right.MakeRelativeTo( g_Conf.Folders.Plugins.ToString() );

				Console::WriteLn( left.GetFullPath() );
				Console::WriteLn( right.GetFullPath() );

				if( left == right )
					m_ComboBoxes.Get(i).SetSelection( sel );
			}
			else
			{
				// plugin failed the test.
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////////////////////
// EnumThread Method Implementations

Panels::PluginSelectorPanel::EnumThread::EnumThread( PluginSelectorPanel& master ) :
	Thread()
,	Results( new EnumeratedPluginInfo[master.FileCount()] )
,	m_master( master )
,	m_cancel( false )
{
}

Panels::PluginSelectorPanel::EnumThread::~EnumThread()
{
	safe_delete_array( Results );
	Close();
}

void Panels::PluginSelectorPanel::EnumThread::Close()
{
	m_cancel = true;
	Threading::Sleep( 1 );
	Thread::Close();
}

int Panels::PluginSelectorPanel::EnumThread::Callback()
{
	for( int curidx=0; curidx < m_master.FileCount() && !m_cancel; ++curidx )
	{
		try
		{
			PluginEnumerator penum( m_master.GetFilename( curidx ) );
			EnumeratedPluginInfo& result( Results[curidx] );

			result.Name = penum.GetName();
			for( int pidx=0; pidx<NumPluginTypes; ++pidx )
			{
				result.TypeMask |= tbl_PluginInfo[pidx].typemask;
				if( penum.CheckVersion( pidx ) )
				{
					result.PassedTest |= tbl_PluginInfo[pidx].typemask;
					penum.GetVersionString( result.Version[pidx], pidx );
				}
			}
		}
		catch( Exception::BadStream& ex )
		{
			Console::Status( ex.LogMessage() );
		}
		catch( Exception::NotPcsxPlugin& ex )
		{
			Console::Status( ex.LogMessage() );
		}

		wxCommandEvent yay( wxEVT_EnumeratedNext );
		yay.SetExtraLong( curidx );
		m_master.GetEventHandler()->AddPendingEvent( yay );
	}

	m_master.GetEventHandler()->AddPendingEvent( wxCommandEvent( wxEVT_EnumerationFinished ) );

	return 0;
}
