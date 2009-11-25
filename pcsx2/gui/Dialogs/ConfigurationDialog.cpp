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
#include "ModalPopups.h"
#include "Panels/ConfigurationPanels.h"

#include <wx/artprov.h>
#include <wx/listbook.h>
#include <wx/listctrl.h>
#include <wx/filepicker.h>
//#include "wx/clipbrd.h"

#ifdef __WXMSW__
#	include <wx/msw/wrapwin.h>		// needed for Vista icon spacing fix.
#	include <commctrl.h>
#endif

using namespace Panels;

// configure the orientation of the listbox based on the platform

#if defined(__WXMAC__) || defined(__WXMSW__)
	static const int s_orient = wxBK_TOP;
#else
	static const int s_orient = wxBK_LEFT;
#endif

template< typename T >
void Dialogs::ConfigurationDialog::AddPage( const char* label, int iconid )
{
	const wxString labelstr( fromUTF8( label ) );
	const int curidx = m_labels.Add( labelstr );
	g_ApplyState.SetCurrentPage( curidx );
	m_listbook.AddPage( new T( &m_listbook ), wxGetTranslation( labelstr ),
		( labelstr == g_Conf->SettingsTabName ), iconid );
}

Dialogs::ConfigurationDialog::ConfigurationDialog( wxWindow* parent, int id ) :
	wxDialogWithHelpers( parent, id, _("PCSX2 Configuration"), true )
,	m_listbook( *new wxListbook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, s_orient ) )
{
	m_idealWidth = 600;

	SetSizer( new wxBoxSizer( wxVERTICAL ) );

	m_listbook.SetImageList( &wxGetApp().GetImgList_Config() );
	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	g_ApplyState.StartBook( &m_listbook );

	AddPage<CpuPanelEE>			( wxLt("EE/IOP"),		cfgid.Cpu );
	AddPage<CpuPanelVU>			( wxLt("VUs"),			cfgid.Cpu );
	AddPage<VideoPanel>			( wxLt("GS/Video"),		cfgid.Video );
	AddPage<SpeedHacksPanel>	( wxLt("Speedhacks"),	cfgid.Speedhacks );
	AddPage<GameFixesPanel>		( wxLt("Game Fixes"),	cfgid.Gamefixes );
	AddPage<PluginSelectorPanel>( wxLt("Plugins"),		cfgid.Plugins );
	AddPage<StandardPathsPanel>	( wxLt("Folders"),		cfgid.Paths );

	*this += m_listbook;
	AddOkCancel( *GetSizer(), true );
	*m_extraButtonSizer += new wxButton( this, wxID_SAVE, _("Screenshot!") );
	
	FindWindow( wxID_APPLY )->Disable();

	Fit();
	CenterOnScreen();

#ifdef __WXMSW__
	// Depending on Windows version and user appearance settings, the default icon spacing can be
	// way over generous.  This little bit of Win32-specific code ensures proper icon spacing, scaled
	// to the size of the frame's ideal width.

	ListView_SetIconSpacing( (HWND)m_listbook.GetListView()->GetHWND(),
		(m_idealWidth-6) / m_listbook.GetPageCount(), g_Conf->Listbook_ImageSize+32		// y component appears to be ignored
	);
#endif

	Connect( wxID_OK,		wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigurationDialog::OnOk_Click ) );
	Connect( wxID_CANCEL,	wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigurationDialog::OnCancel_Click ) );
	Connect( wxID_APPLY,	wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigurationDialog::OnApply_Click ) );
	Connect( wxID_SAVE,		wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigurationDialog::OnScreenshot_Click ) );

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
	if( g_ApplyState.ApplyAll() )
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
	if( g_ApplyState.ApplyAll() )
		FindWindow( wxID_APPLY )->Disable();

	g_Conf->SettingsTabName = m_labels[m_listbook.GetSelection()];
	AppSaveSettings();
}


void Dialogs::ConfigurationDialog::OnScreenshot_Click( wxCommandEvent& evt )
{
	wxBitmap memBmp;

	{
	wxWindowDC dc( this );
	wxSize dcsize( dc.GetSize() );
	wxMemoryDC memDC( memBmp = wxBitmap( dcsize.x, dcsize.y ) );
	memDC.Blit( wxPoint(), dcsize, &dc, wxPoint() );
	}

	wxString filenameDefault;
	filenameDefault.Printf( L"pcsx2_settings_%s.png", m_listbook.GetPageText( m_listbook.GetSelection() ).c_str() );
	filenameDefault.Replace( L"/", L"-" );

	wxString filename( wxFileSelector( _("Save dialog screenshots to..."), g_Conf->Folders.Snapshots.ToString(),
		filenameDefault, L"png", NULL, wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this ) );

	if( !filename.IsEmpty() )
		memBmp.SaveFile( filename, wxBITMAP_TYPE_PNG );
}

// ----------------------------------------------------------------------------
Dialogs::BiosSelectorDialog::BiosSelectorDialog( wxWindow* parent, int id )
	: wxDialogWithHelpers( parent, id, _("BIOS Selector"), false )
{
	m_idealWidth = 500;

	wxBoxSizer& bleh( *new wxBoxSizer( wxVERTICAL ) );
	Panels::BaseSelectorPanel* selpan = new Panels::BiosSelectorPanel( this );

	bleh.Add( selpan, pxSizerFlags::StdExpand() );
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
