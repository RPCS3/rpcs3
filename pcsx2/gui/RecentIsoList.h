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

#pragma once

// --------------------------------------------------------------------------------------
//  RecentIsoManager
// --------------------------------------------------------------------------------------
class RecentIsoManager : public wxEvtHandler
{
protected:
	struct RecentItem
	{
		wxString	Filename;
		wxMenuItem*	ItemPtr;
		
		RecentItem() { ItemPtr = NULL; }

		RecentItem( const wxString& src )
			: Filename( src )
		{
			ItemPtr = NULL;
		}
	};

	std::vector<RecentItem> m_Items;

	wxMenu*		m_Menu;
	uint		m_MaxLength;
	int			m_cursel;
	
	wxMenuItem* m_Separator;

	EventListenerBinding<IniInterface>		m_Listener_SettingsLoadSave;
	EventListenerBinding<int>				m_Listener_SettingsApplied;

public:
	RecentIsoManager( wxMenu* menu );
	virtual ~RecentIsoManager() throw();

	void RemoveAllFromMenu();
	void Repopulate();
	void Add( const wxString& src );
	
protected:
	void InsertIntoMenu( int id );
	void DoSettingsLoadSave( IniInterface& ini );
	void DoSettingsApplied( int& val );
	void OnChangedSelection( wxCommandEvent& evt );

	static void __evt_fastcall OnSettingsLoadSave( void* obj, IniInterface& evt );
	static void __evt_fastcall OnSettingsApplied( void* obj, int& evt );
};

