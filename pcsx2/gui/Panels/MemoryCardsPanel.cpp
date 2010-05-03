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

#include <wx/filepicker.h>
#include <wx/ffile.h>
#include <wx/dir.h>

using namespace pxSizerFlags;
using namespace Panels;


enum McdColumnType
{
	McdCol_Filename,
	McdCol_Mounted,
	McdCol_Size,
	McdCol_Formatted,
	McdCol_DateModified,
	McdCol_DateCreated,
	McdCol_Path,
	McdCol_Count
};

void MemoryCardListView::CreateColumns()
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
		{ _("Path"),			wxLIST_FORMAT_LEFT }
	};

	for( int i=0; i<McdCol_Count; ++i )
		InsertColumn( i, columns[i].name, columns[i].align, -1 );
}

void MemoryCardListView::AssignCardsList( McdList* knownCards, int length )
{
	if( knownCards == NULL ) length = 0;
	m_KnownCards = knownCards;

	SetItemCount( length );
	RefreshItems( 0, length );
}

MemoryCardListView::MemoryCardListView( wxWindow* parent )
	: wxListView( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL )
{
	m_KnownCards = NULL;
	CreateColumns();

	//m_KnownCards;
	Connect( wxEVT_COMMAND_LIST_BEGIN_DRAG,	wxListEventHandler(MemoryCardListView::OnListDrag));
}

void MemoryCardListView::OnListDrag(wxListEvent& evt)
{
	evt.Skip();

	wxFileDataObject my_data;
	my_data.AddFile( (*m_KnownCards)[ GetItemData(GetFirstSelected()) ].Filename.GetFullPath() );

	wxDropSource dragSource( this );
	dragSource.SetData( my_data );
	wxDragResult result = dragSource.DoDragDrop();
}

// return the text for the given column of the given item
wxString MemoryCardListView::OnGetItemText(long item, long column) const
{
	if( m_KnownCards == NULL ) return _parent::OnGetItemText(item, column);

	const McdListItem& it( (*m_KnownCards)[item] );

	switch( column )
	{
		case McdCol_Mounted:
		{
			if( (it.Port == -1) && (it.Slot == -1) ) return L"No";
			return wxsFormat( L"%u / %u", it.Port+1, it.Slot+1);
		}

		case McdCol_Filename:		return it.Filename.GetName();
		case McdCol_Size:			return wxsFormat( L"%u MB", it.SizeInMB );
		case McdCol_Formatted:		return it.IsFormatted ? L"Yes" : L"No";
		case McdCol_DateModified:	return it.DateModified.FormatDate();
		case McdCol_DateCreated:	return it.DateModified.FormatDate();
		case McdCol_Path:			return it.Filename.GetPath();
	}

	pxFail( "Unknown column index in MemoryCardListView -- returning an empty string." );
	return wxEmptyString;
}

// return the icon for the given item. In report view, OnGetItemImage will
// only be called for the first column. See OnGetItemColumnImage for
// details.
int MemoryCardListView::OnGetItemImage(long item) const
{
	return _parent::OnGetItemImage( item );
}

// return the icon for the given item and column.
int MemoryCardListView::OnGetItemColumnImage(long item, long column) const
{
	return _parent::OnGetItemColumnImage( item, column );
}

// return the attribute for the item (may return NULL if none)
wxListItemAttr* MemoryCardListView::OnGetItemAttr(long item) const
{
	wxListItemAttr* retval = _parent::OnGetItemAttr(item);
	//const McdListItem& it( (*m_KnownCards)[item] );
	return retval;
}


// =====================================================================================================
//  MemoryCardInfoPanel
// =====================================================================================================
MemoryCardInfoPanel::MemoryCardInfoPanel( wxWindow* parent, uint port, uint slot )
	: BaseApplicableConfigPanel( parent, wxVERTICAL ) //, wxEmptyString )
{
	m_port = port;
	m_slot = slot;

	SetMinSize( wxSize(128, 48) );

	Connect( wxEVT_PAINT, wxPaintEventHandler(MemoryCardInfoPanel::paintEvent) );
}

static void DrawTextCentered( wxDC& dc, const wxString msg )
{
	int tWidth, tHeight;
	dc.GetTextExtent( msg, &tWidth, &tHeight );
	dc.DrawText( msg, (dc.GetSize().GetWidth() - tWidth) / 2, 0 );
}

void MemoryCardInfoPanel::paintEvent(wxPaintEvent & evt)
{
	// Collect Info and Format Strings
	
	wxString fname( m_filename.GetFullPath() );
	if( fname.IsEmpty() ) fname = _("No Card (empty)");

	// Create DC and plot some text (and images!)

	wxPaintDC dc( this );
	wxFont normal( dc.GetFont() );
	wxFont bold( normal );
	normal.SetWeight( wxNORMAL );
	bold.SetWeight( wxBOLD );

	dc.SetFont( bold );
	DrawTextCentered( dc, fname );

	//dc.DrawCircle( dc.GetSize().GetWidth()/2, 24, dc.GetSize().GetWidth()/4 );
}

void MemoryCardInfoPanel::Apply()
{
	if( m_filename.IsDir() )
	{
		throw Exception::CannotApplySettings( this, 
			wxLt("Cannot use or create memorycard: the filename is an existing directory."),
			true
		);
	}

	if( m_filename.FileExists() )
	{
		// TODO : Prompt user to create	non-existing files.  For now we just creat them implicitly.
	}

	g_Conf->Mcd[m_port][m_slot].Filename = m_filename;
}

void MemoryCardInfoPanel::AppStatusEvent_OnSettingsApplied()
{
	m_filename = g_Conf->Mcd[m_port][m_slot].Filename;
	Refresh();
}


// --------------------------------------------------------------------------------------
//  MemoryCardsPanel Implementations
// --------------------------------------------------------------------------------------
Panels::MemoryCardsPanel::MemoryCardsPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	m_panel_AllKnownCards = new MemoryCardListPanel( this );


	for( uint port=0; port<2; ++port )
	{
		for( uint slot=0; slot<1; ++slot )
		{
			m_panel_cardinfo[port][slot] = new MemoryCardInfoPanel( this, port, slot );
		}
	}

	// ------------------------------------
	//       Sizer / Layout Section
	// ------------------------------------

	*this += m_panel_AllKnownCards	| StdExpand();
}

void Panels::MemoryCardsPanel::OnMultitapChecked( wxCommandEvent& evt )
{
	for( int port=0; port<2; ++port )
	{
		if( m_check_Multitap[port]->GetId() != evt.GetId() ) continue;

		for( uint slot=1; slot<4; ++slot )
		{
			//m_panel_cardinfo[port][slot]->Enable( m_check_Multitap[port]->IsChecked() );
		}
	}
}

void Panels::MemoryCardsPanel::Apply()
{
}

void Panels::MemoryCardsPanel::AppStatusEvent_OnSettingsApplied()
{
	const Pcsx2Config& emuconf( g_Conf->EmuOptions );

	for( uint port=0; port<2; ++port )
	for( uint slot=1; slot<4; ++slot )
	{
		//m_panel_cardinfo[port][slot]->Enable( emuconf.MultitapEnabled(port) );
	}
}

