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

// --------------------------------------------------------------------------------------
//  MemoryCardListView_Simple  (implementations)
// --------------------------------------------------------------------------------------
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

MemoryCardListView_Simple::MemoryCardListView_Simple( wxWindow* parent )
	: _parent( parent )
{
	CreateColumns();
}

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

void MemoryCardListView_Simple::SetCardCount( int length )
{
	if( !m_CardProvider ) length = 0;
	SetItemCount( length );
	Refresh();
}

// return the text for the given column of the given item
wxString MemoryCardListView_Simple::OnGetItemText(long item, long column) const
{
	if( !m_CardProvider ) return _parent::OnGetItemText(item, column);

	const McdListItem& it( m_CardProvider->GetCard(item) );

	switch( column )
	{
		case McdColS_PortSlot:		return wxsFormat( L"%u", item+1);
		case McdColS_Status:		return it.IsPresent ? ( it.IsEnabled ? _("Enabled") : _("Disabled")) : _("Missing");
		case McdColS_Size:			return it.IsPresent ? wxsFormat( L"%u MB", it.SizeInMB ) : (wxString)_("N/A");
		case McdColS_Formatted:		return it.IsFormatted ? _("Yes") : _("No");
		case McdColS_DateModified:	return it.IsPresent ? it.DateModified.FormatDate()	: (wxString)_("N/A");
		case McdColS_DateCreated:	return it.IsPresent ? it.DateCreated.FormatDate()	: (wxString)_("N/A");
	}

	pxFail( "Unknown column index in MemoryCardListView -- returning an empty string." );
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

static wxListItemAttr m_ItemAttr;

// return the attribute for the item (may return NULL if none)
wxListItemAttr* MemoryCardListView_Simple::OnGetItemAttr(long item) const
{
	//m_disabled.SetTextColour( wxLIGHT_GREY );
	//m_targeted.SetBackgroundColour( wxColour(L"Yellow") );

	if( !m_CardProvider ) return _parent::OnGetItemAttr(item);
	const McdListItem& it( m_CardProvider->GetCard(item) );

	m_ItemAttr = wxListItemAttr();		// Wipe it clean!

	if( !it.IsPresent )
		m_ItemAttr.SetTextColour( *wxLIGHT_GREY );

	if( m_TargetedItem == item )
		m_ItemAttr.SetBackgroundColour( wxColour(L"Wheat") );

	return &m_ItemAttr;
}

// --------------------------------------------------------------------------------------
//  MemoryCardListView_Advanced  (implementations)
// --------------------------------------------------------------------------------------
enum McdColumnType_Advanced
{
	McdColA_Filename,
	McdColA_Mounted,
	McdColA_Size,
	McdColA_Formatted,
	McdColA_DateModified,
	McdColA_DateCreated,
	McdColA_Count
};

MemoryCardListView_Advanced::MemoryCardListView_Advanced( wxWindow* parent )
	: _parent( parent )
{
	CreateColumns();
}

void MemoryCardListView_Advanced::CreateColumns()
{
	struct ColumnInfo
	{
		wxString			name;
		wxListColumnFormat	align;
	};

	const ColumnInfo columns[] =
	{
		{ _("Filename"),		wxLIST_FORMAT_LEFT },
		{ _("Mounted"),			wxLIST_FORMAT_CENTER },
		{ _("Size"),			wxLIST_FORMAT_LEFT },
		{ _("Formatted"),		wxLIST_FORMAT_CENTER },
		{ _("Last Modified"),	wxLIST_FORMAT_LEFT },
		{ _("Created On"),		wxLIST_FORMAT_LEFT },
		//{ _("Path"),			wxLIST_FORMAT_LEFT }
	};

	for( int i=0; i<McdColA_Count; ++i )
		InsertColumn( i, columns[i].name, columns[i].align, -1 );
}

void MemoryCardListView_Advanced::SetCardCount( int length )
{
	if( !m_CardProvider ) length = 0;
	SetItemCount( length );
	Refresh();
}

// return the text for the given column of the given item
wxString MemoryCardListView_Advanced::OnGetItemText(long item, long column) const
{
	if( !m_CardProvider ) return _parent::OnGetItemText(item, column);

	const McdListItem& it( m_CardProvider->GetCard(item) );

	switch( column )
	{
		case McdColA_Mounted:
		{
			if( !it.IsEnabled ) return _("No");
			return wxsFormat( L"%u", it.Slot+1);
		}

		case McdColA_Filename:		return it.Filename.GetName();
		case McdColA_Size:			return wxsFormat( L"%u MB", it.SizeInMB );
		case McdColA_Formatted:		return it.IsFormatted ? L"Yes" : L"No";
		case McdColA_DateModified:	return it.DateModified.FormatDate();
		case McdColA_DateCreated:	return it.DateModified.FormatDate();
	}

	pxFail( "Unknown column index in MemoryCardListView_Advanced -- returning an empty string." );
	return wxEmptyString;
}

// return the icon for the given item. In report view, OnGetItemImage will
// only be called for the first column. See OnGetItemColumnImage for
// details.
int MemoryCardListView_Advanced::OnGetItemImage(long item) const
{
	return _parent::OnGetItemImage( item );
}

// return the icon for the given item and column.
int MemoryCardListView_Advanced::OnGetItemColumnImage(long item, long column) const
{
	return _parent::OnGetItemColumnImage( item, column );
}

// return the attribute for the item (may return NULL if none)
wxListItemAttr* MemoryCardListView_Advanced::OnGetItemAttr(long item) const
{
	wxListItemAttr* retval = _parent::OnGetItemAttr(item);
	//const McdListItem& it( (*m_KnownCards)[item] );
	return retval;
}
