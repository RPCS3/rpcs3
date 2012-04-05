/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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
#include "AppCoreThread.h"
#include "RecentIsoList.h"
#include "IsoDropTarget.h"
#include "Utilities/IniInterface.h"

extern wxString GetMsg_IsoImageChanged();

// FIXME : This needs to handle removed/missing ISOs somehow, although I'm not sure the
// best approach.  I think I'd prefer for missing entries to only be removed when they
// are selected.  This also means we'll need to add some sort of "no current selection"
// menu option that's defaulted to when a selection is deemed missing (since just randomly
// selecting another iso would be undesirable).


RecentIsoManager::RecentIsoManager( wxMenu* menu, int firstIdForMenuItems_or_wxID_ANY )
	: m_Menu( menu )
	, m_firstIdForMenuItems_or_wxID_ANY ( firstIdForMenuItems_or_wxID_ANY )
	, m_MaxLength( g_Conf->RecentIsoCount )
{
	m_cursel	= 0;
	m_Separator	= NULL;

	IniLoader loader;
	LoadListFrom(loader);

	Connect( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(RecentIsoManager::OnChangedSelection) );
}

RecentIsoManager::~RecentIsoManager() throw()
{
	Disconnect( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(RecentIsoManager::OnChangedSelection) );
}

void RecentIsoManager::OnChangedSelection( wxCommandEvent& evt )
{
	uint cnt = m_Items.size();
	uint i=0;
	for( ; i<cnt; ++i )
	{
		if( (m_Items[i].ItemPtr != NULL) && (m_Items[i].ItemPtr->GetId() == evt.GetId()) ) break;
	}
	
	if( i >= m_Items.size() )
	{
		evt.Skip();
		return;
	}

	// Actually there is no change on the selection so the event can be skip
	// Note: It also avoids a deadlock which appears when the core thread is already paused
	// and ScopedCoreThreadPopup try to stop the thread (GSOpen1 code path)
	if( (g_Conf->CdvdSource == CDVDsrc_Iso) && (m_Items[i].Filename == g_Conf->CurrentIso) )
	{
		evt.Skip();
		return;
	}


	m_cursel = i;

	ScopedCoreThreadPopup stopped_core;

#ifdef __LINUX__
	// Likely not what was intended, but it compiles for the moment...
	SwapOrReset_Iso( NULL, stopped_core, m_Items[i].Filename, GetMsg_IsoImageChanged());
#else
	// Getting a window from the menu?
	SwapOrReset_Iso( m_Menu->GetWindow(), stopped_core, m_Items[i].Filename, GetMsg_IsoImageChanged());
#endif

	stopped_core.AllowResume();
}

void RecentIsoManager::RemoveAllFromMenu()
{
	if( m_Menu == NULL ) return;

	int cnt = m_Items.size();
	for( int i=0; i<cnt; ++i )
	{
		RecentItem& curitem( m_Items[i] );
		if( curitem.ItemPtr == NULL ) continue;
		m_Menu->Destroy( curitem.ItemPtr );
		curitem.ItemPtr = NULL;
	}
	
	if( m_Separator != NULL )
	{
		m_Menu->Destroy( m_Separator );
		m_Separator = NULL;
	}
}

void RecentIsoManager::Clear()
{
	RemoveAllFromMenu();
	m_Items.clear();
	Repopulate();
}

void RecentIsoManager::Repopulate()
{
	int cnt = m_Items.size();
	if( cnt <= 0 ) return;

	m_Separator = m_Menu->AppendSeparator();
	
	// The following line is important
	// Update: It also makes debug WX assert on Windows. Can we review wether this is needed in Linux at all?
	// Below is a version that doesn't trigger the assert (WX on Windows complains about an empty string), but 
	// ideally this line should get removed.

	//wxString temp = L"isupresswarnings";
	//m_Menu->Remove( m_Menu->Append( -1, temp ) );
	m_Menu->Remove( m_Menu->Append( -1, wxEmptyString ) );
	
	//Note: the internal recent iso list (m_Items) has the most recent item last (also at the INI file)
	//  but the menu is composed in reverse order such that the most recent item appears at the top.
	for( int i=cnt-1; i>=0; --i )
		InsertIntoMenu( i );
}

void RecentIsoManager::Add( const wxString& src )
{
	if( src.IsEmpty() ) return;

	wxString normalized( Path::Normalize( src ) );

	int cnt = m_Items.size();

	if( cnt <= 0 )
	{
		pxAssert( m_Separator == NULL );
		m_Separator = m_Menu->AppendSeparator();
	}
	else
	{
		for( int i=0; i<cnt; ++i )
		{
			if( m_Items[i].Filename == normalized )
			{
				m_cursel = i;
				if( m_Items[i].ItemPtr != NULL )
					m_Items[i].ItemPtr->Check();
				return;
			}
		}
	}

	//New item doesn't exist at the menu/internal-list - add it, and refresh the menu.
	RemoveAllFromMenu();

	m_Items.push_back( RecentItem( normalized ) );

	while( m_Items.size() > m_MaxLength )
		m_Items.erase( m_Items.begin() );

	Repopulate();
	m_Items[m_cursel = m_Items.size()-1].ItemPtr->Check();
}

//id here is the position index at the internal list of recent ISOs (m_Items)
void RecentIsoManager::InsertIntoMenu( int id )
{
	if( m_Menu == NULL ) return;
	RecentItem& curitem( m_Items[id] );
	
	int wxid=wxID_ANY;
	if (this->m_firstIdForMenuItems_or_wxID_ANY != wxID_ANY)
		wxid = this->m_firstIdForMenuItems_or_wxID_ANY + id;

	curitem.ItemPtr = m_Menu->AppendRadioItem( wxid, Path::GetFilename(curitem.Filename), curitem.Filename );
	bool exists = wxFileExists( curitem.Filename );

	if( m_cursel == id && exists )
		curitem.ItemPtr->Check();

	if ( !exists )
		curitem.ItemPtr->Enable( false );
}

void RecentIsoManager::LoadListFrom( IniInterface& ini )
{
	if (!ini.IsOk()) return;

	ini.GetConfig().SetRecordDefaults( false );

	RemoveAllFromMenu();

	m_MaxLength = g_Conf->RecentIsoCount;
	ScopedIniGroup groupie( ini, L"RecentIso" );
	for( uint i=0; i<m_MaxLength; ++i )
	{
		wxFileName loadtmp(L"");
		ini.Entry( pxsFmt( L"Filename%02d", i ), loadtmp, loadtmp, true );
		if( loadtmp.GetFullName()!=L"" ) Add( loadtmp.GetFullPath() );
	}
	Add( g_Conf->CurrentIso );

	ini.GetConfig().SetRecordDefaults( true );
}

void RecentIsoManager::AppStatusEvent_OnSettingsApplied()
{
	// TODO : Implement application of Recent Iso List "maximum" history option
}

void RecentIsoManager::AppStatusEvent_OnUiSettingsLoadSave( const AppSettingsEventInfo& evt )
{
	IniInterface& ini( evt.GetIni() );

	if( ini.IsSaving() )
	{
		// Wipe existing recent iso list if we're saving, because our size might have changed
		// and that could leave some residual entries in the config.

		ini.GetConfig().SetRecordDefaults( false );

		ini.GetConfig().DeleteGroup( L"RecentIso" );
		ScopedIniGroup groupie( ini, L"RecentIso" );

		int cnt = m_Items.size();
		for( int i=0; i<cnt; ++i )
		{
			wxFileName item_filename = wxFileName(m_Items[i].Filename);
			ini.Entry( pxsFmt( L"Filename%02d", i ),  item_filename, wxFileName(L""), IsPortable());
		}
		
		ini.GetConfig().SetRecordDefaults( true );
	}
	else
	{
		LoadListFrom(ini);
	}
}
