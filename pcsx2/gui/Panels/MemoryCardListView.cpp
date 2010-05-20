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
#include "ConfigurationPanels.h"
#include "MemoryCardPanels.h"

#include "Dialogs/ConfigurationDialog.h"

using namespace pxSizerFlags;
using namespace Panels;

enum McdColumnType_Simple
{
	McdColS_PortSlot,	// port and slot of the card
	McdColS_Status,		// either Enabled/Disabled, or Missing (no card).
	McdColS_Size,
	McdColS_Formatted,
	McdColS_DateModified,
	McdColS_DateCreated,
	McdColS_Count
};

void MemoryCardListView_Simple::CreateColumns()
{
	struct ColumnInfo
	{
		wxString			name;
		wxListColumnFormat	align;
	};

	const ColumnInfo columns[] =
	{
		{ _("Slot"),			wxLIST_FORMAT_LEFT },
		{ _("Status"),			wxLIST_FORMAT_CENTER },
		{ _("Size"),			wxLIST_FORMAT_LEFT },
		{ _("Formatted"),		wxLIST_FORMAT_CENTER },
		{ _("Modified"),		wxLIST_FORMAT_LEFT },
		{ _("Created On"),		wxLIST_FORMAT_LEFT },
	};

	for( int i=0; i<McdColS_Count; ++i )
		InsertColumn( i, columns[i].name, columns[i].align, -1 );
}

MemoryCardListView_Simple::MemoryCardListView_Simple( wxWindow* parent )
	: _parent( parent )
{
	CreateColumns();
	Connect( wxEVT_COMMAND_LIST_BEGIN_DRAG,	wxListEventHandler(MemoryCardListView_Advanced::OnListDrag));
}

void MemoryCardListView_Simple::SetCardCount( int length )
{
	if( !m_CardProvider ) length = 0;
	SetItemCount( length );
	Refresh();
}

void MemoryCardListView_Simple::OnListDrag(wxListEvent& evt)
{
	evt.Skip();
}

// return the text for the given column of the given item
wxString MemoryCardListView_Simple::OnGetItemText(long item, long column) const
{
	if( !m_CardProvider ) return _parent::OnGetItemText(item, column);

	const McdListItem& it( m_CardProvider->GetCard(item) );

	switch( column )
	{
		case McdColS_PortSlot:		return wxsFormat( L"%u/%u", m_CardProvider->GetPort(item)+1, m_CardProvider->GetSlot(item)+1);
		case McdColS_Status:		return it.IsPresent ? ( it.IsEnabled ? _("Enabled") : _("Disabled")) : _("Missing");
		case McdColS_Size:			return it.IsPresent ? wxsFormat( L"%u MB", it.SizeInMB ) : wxT("N/A");
		case McdColS_Formatted:		return it.IsFormatted ? _("Yes") : _("No");
		case McdColS_DateModified:	return it.DateModified.FormatDate();
		case McdColS_DateCreated:	return it.DateCreated.FormatDate();
		//case McdCol_Path:			return it.Filename.GetPath();
	}

	pxFail( "Unknown column index in MemoryCardListView_Advanced -- returning an empty string." );
	return wxEmptyString;
}

// return the icon for the given item. In report view, OnGetItemImage will
// only be called for the first column. See OnGetItemColumnImage for
// details.
int MemoryCardListView_Simple::OnGetItemImage(long item) const
{
	return _parent::OnGetItemImage( item );
}

// return the icon for the given item and column.
int MemoryCardListView_Simple::OnGetItemColumnImage(long item, long column) const
{
	return _parent::OnGetItemColumnImage( item, column );
}

// return the attribute for the item (may return NULL if none)
wxListItemAttr* MemoryCardListView_Simple::OnGetItemAttr(long item) const
{
	wxListItemAttr* retval = _parent::OnGetItemAttr(item);
	//const McdListItem& it( (*m_KnownCards)[item] );
	return retval;
}
