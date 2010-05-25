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

// --------------------------------------------------------------------------------------
//  McdListItem
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
	virtual const McdListItem& GetCard( int idx ) const=0;
	virtual McdListItem& GetCard( int idx )=0;

	virtual wxDirName GetMcdPath() const=0;
};

class BaseMcdListView : public wxListView
{
	typedef wxListView _parent;

protected:
	const IMcdList*		m_CardProvider;

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

	virtual void SetMcdProvider( IMcdList* face )
	{
		m_CardProvider = face;
		SetCardCount( m_CardProvider ? m_CardProvider->GetLength() : 0 );
	}
	
	virtual const IMcdList& GetMcdProvider() const
	{
		pxAssume( m_CardProvider );
		return *m_CardProvider;
	}
	
	virtual void SetTargetedItem( int sel )
	{
		if( m_TargetedItem == sel ) return;

		if( m_TargetedItem >= 0 ) RefreshItem( m_TargetedItem );
		m_TargetedItem = sel;
		RefreshItem( sel );
	}
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

protected:
	// Overrides for wxLC_VIRTUAL
	virtual wxString OnGetItemText(long item, long column) const;
	virtual int OnGetItemImage(long item) const;
	virtual int OnGetItemColumnImage(long item, long column) const;
	virtual wxListItemAttr *OnGetItemAttr(long item) const;
};


// --------------------------------------------------------------------------------------
//  MemoryCardListView_Advanced
// --------------------------------------------------------------------------------------
class MemoryCardListView_Advanced : public BaseMcdListView
{
	typedef BaseMcdListView _parent;

public:
	virtual ~MemoryCardListView_Advanced() throw() { }
	MemoryCardListView_Advanced( wxWindow* parent );

	void CreateColumns();
	virtual void SetCardCount( int length );

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

		void AppStatusEvent_OnSettingsApplied();
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

		virtual const McdListItem& GetCard( int idx ) const;
		virtual McdListItem& GetCard( int idx );

	protected:
		void OnCreateCard(wxCommandEvent& evt);
		void OnMountCard(wxCommandEvent& evt);
		
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
	//  MemoryCardInfoPanel
	// --------------------------------------------------------------------------------------
	class MemoryCardInfoPanel : public BaseApplicableConfigPanel
	{
	protected:
		//uint			m_port;
		uint			m_slot;

		wxString		m_DisplayName;
		wxString		m_ErrorMessage;
		ScopedPtr<McdListItem> m_cardInfo;
		
	public:
		virtual ~MemoryCardInfoPanel() throw() {}
		MemoryCardInfoPanel( wxWindow* parent, uint slot );
		void Apply();
		void Eject();

	protected:
		void AppStatusEvent_OnSettingsApplied();
		void paintEvent( wxPaintEvent& evt );

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

		#ifdef __WXMSW__
		pxCheckBox*		m_check_CompressNTFS;
		#endif
	
	public:
		McdConfigPanel_Toggles( wxWindow* parent );
		virtual ~McdConfigPanel_Toggles() throw() { }
		void Apply();

	protected:
		void AppStatusEvent_OnSettingsApplied();
		void OnMultitapClicked();
	};

	class McdConfigPanel_Standard : public BaseApplicableConfigPanel
	{
		typedef BaseApplicableConfigPanel _parent;

	protected:
		MemoryCardInfoPanel*	m_panel_cardinfo[2];

	public:
		McdConfigPanel_Standard( wxWindow* parent );
		virtual ~McdConfigPanel_Standard() throw() { }
		void Apply();

	protected:
		void AppStatusEvent_OnSettingsApplied();
	};

	class McdConfigPanel_Multitap : public BaseApplicableConfigPanel
	{
		typedef BaseApplicableConfigPanel _parent;

	protected:
		int				m_port;
		pxCheckBox*		m_check_Multitap;

	public:
		McdConfigPanel_Multitap( wxWindow* parent, int port=0 );
		virtual ~McdConfigPanel_Multitap() throw() { }
		void Apply();

	protected:
		void OnMultitapChecked( wxCommandEvent& evt );
		void AppStatusEvent_OnSettingsApplied();
	};

};

extern bool EnumerateMemoryCard( McdListItem& dest, const wxFileName& filename );
//extern bool EnumerateMemoryCard( SimpleMcdItem& dest, const wxFileName& filename );
