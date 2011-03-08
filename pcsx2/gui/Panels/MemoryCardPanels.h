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

#pragma once

#include "AppCommon.h"

#include <wx/dnd.h>
#include <wx/listctrl.h>

struct ListViewColumnInfo
{
	const wxChar*		name;
	int					width;
	wxListColumnFormat	align;
};

// --------------------------------------------------------------------------------------
//  McdListItem / IMcdList
// --------------------------------------------------------------------------------------
struct McdListItem
{
	bool		IsPresent;
	bool		IsEnabled;
	bool		IsFormatted;

	uint		SizeInMB;		// size, in megabytes!
	wxDateTime	DateCreated;
	wxDateTime	DateModified;

	int			Slot;

	wxFileName	Filename;		// full pathname (optional)

	McdListItem()
	{
		//Port = -1;
		Slot		= -1;
		
		IsPresent = false;
		IsEnabled = false;
	}
	
	bool IsMultitapSlot() const;
	uint GetMtapPort() const;
	uint GetMtapSlot() const;

	bool operator==( const McdListItem& right ) const;
	bool operator!=( const McdListItem& right ) const;
};

typedef std::vector<McdListItem> McdList;

class IMcdList
{
public:
	virtual void RefreshMcds() const=0;
	virtual int GetLength() const=0;
	virtual McdListItem& GetCard( int idx )=0;
	virtual wxDirName GetMcdPath() const=0;
};

// --------------------------------------------------------------------------------------
//  BaseMcdListView
// --------------------------------------------------------------------------------------
class BaseMcdListView : public wxListView
{
	typedef wxListView _parent;

protected:
	IMcdList*		m_CardProvider;

	// specifies the target of a drag&drop operation
	int					m_TargetedItem;

public:
	virtual ~BaseMcdListView() throw() { }
	BaseMcdListView( wxWindow* parent )
		: _parent( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_VIRTUAL )
	{
		m_CardProvider = NULL;
	}

	virtual void SetCardCount( int length )=0;
	virtual void SetMcdProvider( IMcdList* face );

	virtual void LoadSaveColumns( IniInterface& ini );
	virtual const ListViewColumnInfo& GetDefaultColumnInfo( uint idx ) const=0;

	virtual IMcdList& GetMcdProvider();
	virtual void SetTargetedItem( int sel );
};

// --------------------------------------------------------------------------------------
//  MemoryCardListView_Simple
// --------------------------------------------------------------------------------------
class MemoryCardListView_Simple : public BaseMcdListView
{
	typedef BaseMcdListView _parent;

public:
	virtual ~MemoryCardListView_Simple() throw() { }
	MemoryCardListView_Simple( wxWindow* parent );

	void CreateColumns();

	virtual void SetCardCount( int length );
	virtual const ListViewColumnInfo& GetDefaultColumnInfo( uint idx ) const;

protected:
	// Overrides for wxLC_VIRTUAL
	virtual wxString OnGetItemText(long item, long column) const;
	virtual int OnGetItemImage(long item) const;
	virtual int OnGetItemColumnImage(long item, long column) const;
	virtual wxListItemAttr *OnGetItemAttr(long item) const;
};


namespace Panels
{
	// --------------------------------------------------------------------------------------
	//  BaseMcdListPanel
	// --------------------------------------------------------------------------------------
	class BaseMcdListPanel
		: public BaseSelectorPanel
		, public wxFileDropTarget
		, public IMcdList		// derived classes need to implement this
	{
		typedef BaseSelectorPanel _parent;
		
	protected:
		DirPickerPanel*		m_FolderPicker;
		BaseMcdListView*	m_listview;
		wxButton*			m_btn_Refresh;

		wxBoxSizer*			s_leftside_buttons;
		wxBoxSizer*			s_rightside_buttons;

		bool				m_MultitapEnabled[2];

		virtual void RefreshMcds() const;

		virtual wxDirName GetMcdPath() const
		{
			pxAssume(m_FolderPicker);
			return m_FolderPicker->GetPath();
		}

	public:
		virtual ~BaseMcdListPanel() throw() {}
		BaseMcdListPanel( wxWindow* parent );

		void CreateLayout();
		void SetMultitapEnabled( uint port, bool enabled )
		{
			m_MultitapEnabled[port] = enabled;
			RefreshMcds();
		}

		virtual void Apply();
		virtual void AppStatusEvent_OnSettingsApplied();
	};

	// --------------------------------------------------------------------------------------
	//  MemoryCardListPanel_Simple
	// --------------------------------------------------------------------------------------
	class MemoryCardListPanel_Simple
		: public BaseMcdListPanel
	{
		typedef BaseMcdListPanel _parent;

	protected:
		McdListItem		m_Cards[8]; 
		
		// Doubles as Create and Delete buttons
		wxButton*		m_button_Create;
		
		// Doubles as Mount and Unmount buttons
		wxButton*		m_button_Mount;

	public:
		virtual ~MemoryCardListPanel_Simple() throw() {}
		MemoryCardListPanel_Simple( wxWindow* parent );

		void UpdateUI();

		// Interface Implementation for IMcdList
		virtual int GetLength() const;

		virtual McdListItem& GetCard( int idx );

	protected:
		void OnCreateCard(wxCommandEvent& evt);
		void OnMountCard(wxCommandEvent& evt);
		void OnRelocateCard(wxCommandEvent& evt);
		
		void OnListDrag(wxListEvent& evt);
		void OnListSelectionChanged(wxListEvent& evt);
		void OnOpenItemContextMenu(wxListEvent& evt);

		virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);

		virtual void Apply();
		virtual void AppStatusEvent_OnSettingsApplied();
		virtual void DoRefresh();
		virtual bool ValidateEnumerationStatus();
	};

	// --------------------------------------------------------------------------------------
	//  McdConfigPanel_Toggles / McdConfigPanel_Standard / McdConfigPanel_Multitap
	// --------------------------------------------------------------------------------------
	class McdConfigPanel_Toggles : public BaseApplicableConfigPanel
	{
		typedef BaseApplicableConfigPanel _parent;

	protected:
		pxCheckBox*		m_check_Multitap[2];
		pxCheckBox*		m_check_Ejection;

		//moved to the system menu, just below "Save State"
		//pxCheckBox*		m_check_SavestateBackup;

	public:
		McdConfigPanel_Toggles( wxWindow* parent );
		virtual ~McdConfigPanel_Toggles() throw() { }
		void Apply();

	protected:
		void AppStatusEvent_OnSettingsApplied();
		void OnMultitapClicked();
	};

};

//avih: is the first one used??
extern bool EnumerateMemoryCard( McdListItem& dest, const wxFileName& filename );
//extern bool EnumerateMemoryCard( SimpleMcdItem& dest, const wxFileName& filename );
