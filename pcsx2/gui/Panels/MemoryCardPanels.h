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

#include "AppCommon.h"

#include <wx/dnd.h>
#include <wx/listctrl.h>

// --------------------------------------------------------------------------------------
//  McdListItem
// --------------------------------------------------------------------------------------
struct McdListItem
{
	uint		SizeInMB;		// size, in megabytes!
	bool		IsFormatted;
	wxDateTime	DateCreated;
	wxDateTime	DateModified;
	wxFileName	Filename;		// full pathname

	int			Port;
	int			Slot;

	McdListItem()
	{
		Port = -1;
		Slot = -1;
	}

	bool operator==( const McdListItem& right ) const
	{
		return
			OpEqu(SizeInMB)		&& OpEqu(IsFormatted)	&&
			OpEqu(DateCreated)	&& OpEqu(DateModified)	&&
			OpEqu(Filename);
	}

	bool operator!=( const McdListItem& right ) const
	{
		return operator==( right );
	}
};

typedef std::vector<McdListItem> McdList;

// --------------------------------------------------------------------------------------
//  MemoryCardListView
// --------------------------------------------------------------------------------------
class MemoryCardListView : public wxListView
{
	typedef wxListView _parent;

protected:
	McdList*		m_KnownCards;

public:
	virtual ~MemoryCardListView() throw() { }
	MemoryCardListView( wxWindow* parent );

	virtual void OnListDrag(wxListEvent& evt);
	virtual void CreateColumns();
	virtual void AssignCardsList( McdList* knownCards, int length=0 );

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
	//  MemoryCardListPanel
	// --------------------------------------------------------------------------------------
	class MemoryCardListPanel
		: public BaseSelectorPanel
		, public wxFileDropTarget
	{
	protected:
		DirPickerPanel*		m_FolderPicker;
		MemoryCardListView*	m_listview;
		ScopedPtr<McdList>	m_KnownCards;

	public:
		virtual ~MemoryCardListPanel() throw() {}
		MemoryCardListPanel( wxWindow* parent );

	protected:
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
		uint			m_port;
		uint			m_slot;

		wxFileName		m_filename;
		
	public:
		virtual ~MemoryCardInfoPanel() throw() {}
		MemoryCardInfoPanel( wxWindow* parent, uint port, uint slot );
		void Apply();

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
