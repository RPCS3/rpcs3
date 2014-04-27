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
//
//  These are the items at the list-view.
//  Each item holds:
//     - An internal slot association (or -1 if it isn't associated with an internal slot)
//     - Properties of the internal slot (when associated with one)
//     - Properties of the file associated with this list item (when associated with one)
// --------------------------------------------------------------------------------------
struct McdSlotItem
{
	int			Slot;			//0-7: internal slot. -1: unrelated to an internal slot (the rest of the files at the folder).
	bool		IsPresent;		//Whether or not a file is associated with this item (true/false when 0<=Slot<=7. Always true when Slot==-1)
	
	//Only meaningful when IsPresent==true (a file exists for this item):
	wxFileName	Filename;		// full pathname
	bool		IsFormatted;
	bool		IsPSX; // False: PS2 memory card, True: PSX memory card
	uint		SizeInMB;		// size, in megabytes!
	wxDateTime	DateCreated;
	wxDateTime	DateModified;

	//Only meaningful when 0<=Slot<=7 (associated with an internal slot):
	//  Properties of an internal slot, and translation from internal slot index to ps2 physical port (0-based-indexes).
	bool IsEnabled;					//This slot is enabled/disabled
	bool IsMultitapSlot() const;
	uint GetMtapPort() const;
	uint GetMtapSlot() const;

	bool operator==( const McdSlotItem& right ) const;
	bool operator!=( const McdSlotItem& right ) const;

	McdSlotItem()
	{
		Slot		= -1;
		
		IsPSX = false;
		IsPresent = false;
		IsEnabled = false;
	}

};

typedef std::vector<McdSlotItem> McdList;

class IMcdList
{
public:
	virtual void RefreshMcds() const=0;
	virtual int GetLength() const=0;
	virtual McdSlotItem& GetCardForViewIndex( int idx )=0;
	virtual int GetSlotIndexForViewIndex( int listViewIndex )=0;
	virtual wxDirName GetMcdPath() const=0;
	virtual void PublicApply() =0;
	virtual bool isFileAssignedToInternalSlot(const wxFileName cardFile) const =0;
	virtual void RemoveCardFromSlot(const wxFileName cardFile) =0;
	virtual bool IsNonEmptyFilesystemCards() const =0;
	virtual bool UiDuplicateCard( McdSlotItem& src, McdSlotItem& dest ) =0;
	
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
	int				m_TargetedItem;

public:
	void (*m_externHandler)(void);
	void setExternHandler(void (*f)(void)){m_externHandler=f;};
	void OnChanged(wxEvent& evt){if (m_externHandler) m_externHandler(); evt.Skip();}

	virtual ~BaseMcdListView() throw() { }
	BaseMcdListView( wxWindow* parent )
		: _parent( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_VIRTUAL )
	{
		m_externHandler=NULL;
		Connect( this->GetId(),				wxEVT_LEFT_UP, wxEventHandler(BaseMcdListView::OnChanged));

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
			pxAssert(m_FolderPicker);
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
		McdSlotItem		m_Cards[8];

		wxButton*		m_button_Rename;
		
		// Doubles as Create and Delete buttons
		wxButton*		m_button_Create;
		
		// Doubles as Mount and Unmount buttons ("Enable"/"Disable" port)
//		wxButton*		m_button_Mount;

		wxButton*		m_button_AssignUnassign; //insert/eject card
		wxButton*		m_button_Duplicate;


	public:
		virtual ~MemoryCardListPanel_Simple() throw();
		MemoryCardListPanel_Simple( wxWindow* parent );

		void UpdateUI();

		// Interface Implementation for IMcdList
		virtual int GetLength() const;

		virtual McdSlotItem& GetCardForViewIndex( int idx );
		virtual int GetSlotIndexForViewIndex( int viewIndex );
		virtual void PublicApply(){ Apply(); };
		void OnChangedListSelection(){ UpdateUI(); };
		virtual bool UiDuplicateCard( McdSlotItem& src, McdSlotItem& dest );

	protected:
		void OnCreateOrDeleteCard(wxCommandEvent& evt);
		void OnMountCard(wxCommandEvent& evt);
//		void OnRelocateCard(wxCommandEvent& evt);
		void OnRenameFile(wxCommandEvent& evt);
		void OnDuplicateFile(wxCommandEvent& evt);
		void OnAssignUnassignFile(wxCommandEvent& evt);
		
		void OnListDrag(wxListEvent& evt);
		void OnListSelectionChanged(wxListEvent& evt);
		void OnOpenItemContextMenu(wxListEvent& evt);
		void OnItemActivated(wxListEvent& evt);

		virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);

		std::vector<McdSlotItem> m_allFilesystemCards;
		McdSlotItem m_filesystemPlaceholderCard;

		virtual int GetNumFilesVisibleAsFilesystem() const;
		virtual int GetNumVisibleInternalSlots() const;
		virtual McdSlotItem& GetNthVisibleFilesystemCard(int n);
		virtual bool IsSlotVisible(int slotIndex) const;
		virtual void ReadFilesAtMcdFolder();
		virtual bool isFileAssignedAndVisibleOnList(const wxFileName cardFile) const;
		virtual bool isFileAssignedToInternalSlot(const wxFileName cardFile) const;
		virtual void RemoveCardFromSlot(const wxFileName cardFile);
		virtual bool IsNonEmptyFilesystemCards() const;

		virtual void Apply();
		virtual void AppStatusEvent_OnSettingsApplied();
		virtual void DoRefresh();
		virtual bool ValidateEnumerationStatus();

		virtual void UiRenameCard( McdSlotItem& card );
		virtual void UiCreateNewCard( McdSlotItem& card );
		virtual void UiDeleteCard( McdSlotItem& card );
		virtual void UiAssignUnassignFile( McdSlotItem& card );
		
	};

	// --------------------------------------------------------------------------------------
	//  McdConfigPanel_Toggles / McdConfigPanel_Standard / McdConfigPanel_Multitap
	// --------------------------------------------------------------------------------------
	class McdConfigPanel_Toggles : public BaseApplicableConfigPanel
	{
		typedef BaseApplicableConfigPanel _parent;

	protected:
		//pxCheckBox*		m_check_Multitap[2];
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
extern bool EnumerateMemoryCard( McdSlotItem& dest, const wxFileName& filename );
//extern bool EnumerateMemoryCard( SimpleMcdItem& dest, const wxFileName& filename );
