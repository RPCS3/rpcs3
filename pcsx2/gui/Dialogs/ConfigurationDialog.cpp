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
#include "Misc.h"
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


Dialogs::ConfigurationDialog::ConfigurationDialog( wxWindow* parent, int id ) :
	wxDialogWithHelpers( parent, id, _("PCSX2 Configuration"), true )
,	m_listbook( *new wxListbook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, s_orient ) )
{
	static const int IdealWidth = 500;

	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );

	m_listbook.SetImageList( &wxGetApp().GetImgList_Config() );
	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	g_ApplyState.StartBook( &m_listbook );

	g_ApplyState.SetCurrentPage( m_listbook.GetPageCount() );
	m_listbook.AddPage( new CpuPanel( m_listbook, IdealWidth ),				_("CPU"), false, cfgid.Cpu );

	g_ApplyState.SetCurrentPage( m_listbook.GetPageCount() );
	m_listbook.AddPage( new VideoPanel( m_listbook, IdealWidth ),			_("GS/Video"), false, cfgid.Video );

	g_ApplyState.SetCurrentPage( m_listbook.GetPageCount() );
	m_listbook.AddPage( new SpeedHacksPanel( m_listbook, IdealWidth ),		_("Speedhacks"), false, cfgid.Speedhacks );

	g_ApplyState.SetCurrentPage( m_listbook.GetPageCount() );
	m_listbook.AddPage( new GameFixesPanel( m_listbook, IdealWidth ),		_("Game Fixes"), false, cfgid.Gamefixes );

	g_ApplyState.SetCurrentPage( m_listbook.GetPageCount() );
	m_listbook.AddPage( new PluginSelectorPanel( m_listbook, IdealWidth ),	_("Plugins"), false, cfgid.Plugins );

	g_ApplyState.SetCurrentPage( m_listbook.GetPageCount() );
	m_listbook.AddPage( new StandardPathsPanel( m_listbook ),				_("Folders"), false, cfgid.Paths );

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

Dialogs::ConfigurationDialog::~ConfigurationDialog()
{
	g_ApplyState.DoCleanup();
}

void Dialogs::ConfigurationDialog::OnOk_Click( wxCommandEvent& evt )
{
	if( g_ApplyState.ApplyAll() )
	{
		Close();
		evt.Skip();
	}
}

void Dialogs::ConfigurationDialog::OnApply_Click( wxCommandEvent& evt )
{
	FindWindow( wxID_APPLY )->Disable();
	g_ApplyState.ApplyAll();
}


// ----------------------------------------------------------------------------
Dialogs::BiosSelectorDialog::BiosSelectorDialog( wxWindow* parent ) :
	wxDialogWithHelpers( parent, wxID_ANY, _("BIOS Selector"), false )
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
