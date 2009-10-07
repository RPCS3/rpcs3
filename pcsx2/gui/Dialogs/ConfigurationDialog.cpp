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
#include "System.h"
#include "App.h"

#include "ConfigurationDialog.h"
#include "Panels/ConfigurationPanels.h"

#include <wx/artprov.h>
#include <wx/listbook.h>
#include <wx/listctrl.h>

#ifdef __WXMSW__
#	include <commctrl.h>		// needed for Vista icon spacing fix.
#endif

using namespace wxHelpers;
using namespace Panels;

// configure the orientation of the listbox based on the platform

#if defined(__WXMAC__) || defined(__WXMSW__)
	static const int s_orient = wxBK_TOP;
#else
	static const int s_orient = wxBK_LEFT;
#endif

static const int IdealWidth = 500;

template< typename T >
void Dialogs::ConfigurationDialog::AddPage( const char* label, int iconid )
{
	const wxString labelstr( fromUTF8( label ) );
	const int curidx = m_labels.Add( labelstr );
	g_ApplyState.SetCurrentPage( curidx );
	m_listbook.AddPage( new T( m_listbook, IdealWidth ),	wxGetTranslation( labelstr ),
		( labelstr == g_Conf->SettingsTabName ), iconid );
}

Dialogs::ConfigurationDialog::ConfigurationDialog( wxWindow* parent, int id ) :
	wxDialogWithHelpers( parent, id, _("PCSX2 Configuration"), true )
,	m_listbook( *new wxListbook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, s_orient ) )
{
	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );

	m_listbook.SetImageList( &wxGetApp().GetImgList_Config() );
	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	g_ApplyState.StartBook( &m_listbook );

	AddPage<CpuPanel>			( wxLt("CPU"),			cfgid.Cpu );
	AddPage<VideoPanel>			( wxLt("GS/Video"),		cfgid.Video );
	AddPage<SpeedHacksPanel>	( wxLt("Speedhacks"),	cfgid.Speedhacks );
	AddPage<GameFixesPanel>		( wxLt("Game Fixes"),	cfgid.Gamefixes );
	AddPage<PluginSelectorPanel>( wxLt("Plugins"),		cfgid.Plugins );
	AddPage<StandardPathsPanel>	( wxLt("Folders"),		cfgid.Paths );

	mainSizer.Add( &m_listbook );
	AddOkCancel( mainSizer, true );

	FindWindow( wxID_APPLY )->Disable();

	SetSizerAndFit( &mainSizer );
	CenterOnScreen();

#ifdef __WXMSW__
	// Depending on Windows version and user appearance settings, the default icon spacing can be
	// way over generous.  This little bit of Win32-specific code ensures proper icon spacing, scaled
	// to the size of the frame's ideal width.

	ListView_SetIconSpacing( (HWND)m_listbook.GetListView()->GetHWND(),
		(IdealWidth-6) / m_listbook.GetPageCount(), g_Conf->Listbook_ImageSize+32		// y component appears to be ignored
	);
#endif

	Connect( wxID_OK,		wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigurationDialog::OnOk_Click ) );
	Connect( wxID_CANCEL,	wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigurationDialog::OnCancel_Click ) );
	Connect( wxID_APPLY,	wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigurationDialog::OnApply_Click ) );

	// ----------------------------------------------------------------------------
	// Bind a variety of standard "something probably changed" events.  If the user invokes
	// any of these, we'll automatically de-gray the Apply button for this dialog box. :)

	#define ConnectSomethingChanged( command ) \
		Connect( wxEVT_COMMAND_##command,	wxCommandEventHandler( ConfigurationDialog::OnSomethingChanged ) );

	ConnectSomethingChanged( RADIOBUTTON_SELECTED );
	ConnectSomethingChanged( COMBOBOX_SELECTED );
	ConnectSomethingChanged( CHECKBOX_CLICKED );
	ConnectSomethingChanged( BUTTON_CLICKED );
	ConnectSomethingChanged( CHOICE_SELECTED );
	ConnectSomethingChanged( LISTBOX_SELECTED );
	ConnectSomethingChanged( SPINCTRL_UPDATED );
	ConnectSomethingChanged( SLIDER_UPDATED );
	ConnectSomethingChanged( DIRPICKER_CHANGED );
}

Dialogs::ConfigurationDialog::~ConfigurationDialog() throw()
{
	g_ApplyState.DoCleanup();
}

void Dialogs::ConfigurationDialog::OnOk_Click( wxCommandEvent& evt )
{
	if( g_ApplyState.ApplyAll( false ) )
	{
		FindWindow( wxID_APPLY )->Disable();
		g_Conf->SettingsTabName = m_labels[m_listbook.GetSelection()];
		AppSaveSettings();

		Close();
		evt.Skip();
	}
}

void Dialogs::ConfigurationDialog::OnCancel_Click( wxCommandEvent& evt )
{
	evt.Skip();
	g_Conf->SettingsTabName = m_labels[m_listbook.GetSelection()];
}

void Dialogs::ConfigurationDialog::OnApply_Click( wxCommandEvent& evt )
{
	if( g_ApplyState.ApplyAll( false ) )
		FindWindow( wxID_APPLY )->Disable();

	g_Conf->SettingsTabName = m_labels[m_listbook.GetSelection()];
	AppSaveSettings();
}


// ----------------------------------------------------------------------------
Dialogs::BiosSelectorDialog::BiosSelectorDialog( wxWindow* parent, int id ) :
	wxDialogWithHelpers( parent, id, _("BIOS Selector"), false )
{
	wxBoxSizer& bleh( *new wxBoxSizer( wxVERTICAL ) );

	Panels::BaseSelectorPanel* selpan = new Panels::BiosSelectorPanel( *this, 500 );

	bleh.Add( selpan, SizerFlags::StdExpand() );
	AddOkCancel( bleh, false );

	SetSizerAndFit( &bleh );

	Connect( wxID_OK,		wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BiosSelectorDialog::OnOk_Click ) );
	Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(BiosSelectorDialog::OnDoubleClicked) );

	selpan->OnShown();
}

void Dialogs::BiosSelectorDialog::OnOk_Click( wxCommandEvent& evt )
{
	if( g_ApplyState.ApplyAll() )
	{
		Close();
		evt.Skip();
	}
}

void Dialogs::BiosSelectorDialog::OnDoubleClicked( wxCommandEvent& evt )
{
	wxWindow* forwardButton = FindWindow( wxID_OK );
	if( forwardButton == NULL ) return;

	wxCommandEvent nextpg( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK );
	nextpg.SetEventObject( forwardButton );
	forwardButton->GetEventHandler()->ProcessEvent( nextpg );
}
