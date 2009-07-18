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

#include <wx/bookctrl.h>
#include <wx/artprov.h>

using namespace wxHelpers;
using namespace Panels;

// configure the orientation of the listbox based on the platform

#if defined(__WXMAC__) || defined(__WXMSW__)
	static const int s_orient = wxBK_TOP;
#else
	static const int s_orient = wxBK_LEFT;
#endif


Dialogs::ConfigurationDialog::ConfigurationDialog( wxWindow* parent, int id ) :
	wxDialogWithHelpers( parent, id, _T("PCSX2 Configuration"), true )
,	m_listbook( *new wxListbook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, s_orient ) )
{
	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );

	m_listbook.SetImageList( &wxGetApp().GetImgList_Config() );
	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	m_listbook.AddPage( new PathsPanel( m_listbook ),			_("Folders"), false, cfgid.Paths );
	m_listbook.AddPage( new PluginSelectorPanel( m_listbook ),	_("Plugins"), false, cfgid.Plugins );
	m_listbook.AddPage( new SpeedHacksPanel( m_listbook ),		_("Speedhacks"), false, cfgid.Speedhacks );
	m_listbook.AddPage( new GameFixesPanel( m_listbook ),		_("Game Fixes"), false, cfgid.Gamefixes );

	mainSizer.Add( &m_listbook );
	AddOkCancel( mainSizer, true );

	SetSizerAndFit( &mainSizer );

	Center( wxCENTER_ON_SCREEN | wxBOTH );
	
	Connect( wxID_OK,		wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigurationDialog::OnOk_Click ) );
	Connect( wxID_APPLY,	wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigurationDialog::OnApply_Click ) );
}

Dialogs::ConfigurationDialog::~ConfigurationDialog()
{
}

bool Dialogs::ConfigurationDialog::ApplySettings()
{
	AppConfig confcopy( *g_Conf );

	try
	{
		int pagecount = m_listbook.GetPageCount();
		for( int i=0; i<pagecount; ++i )
		{
			BaseApplicableConfigPanel* panel = (BaseApplicableConfigPanel*)m_listbook.GetPage(i);
			panel->Apply( confcopy );
		}
		
		*g_Conf = confcopy;
		g_Conf->Apply();
		g_Conf->Save();
	}
	catch( Exception::CannotApplySettings& ex )
	{
		wxMessageBox( ex.DisplayMessage(), _("Cannot apply settings...") );
		
		// TODO : Automatically switch focus to the panel that failed.
		return false;
	}
	return true;
}

void Dialogs::ConfigurationDialog::OnOk_Click( wxCommandEvent& evt )
{
	evt.Skip();
	if( ApplySettings() ) Close();
}

void Dialogs::ConfigurationDialog::OnApply_Click( wxCommandEvent& evt )
{
	evt.Skip();
	ApplySettings();
}
