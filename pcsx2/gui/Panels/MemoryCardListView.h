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

namespace Panels
{
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

		//wxStaticText*	m_label;
		wxTextCtrl*		m_label;

	public:
		virtual ~MemoryCardInfoPanel() throw() {}
		MemoryCardInfoPanel( wxWindow* parent, uint port, uint slot );
		void Apply();

	protected:
		void AppStatusEvent_OnSettingsApplied();
		void paintEvent( wxPaintEvent& evt );

	};

};
