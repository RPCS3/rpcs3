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

#include "PrecompiledHeader.h"
#include "ConfigurationPanels.h"
#include "MemoryCardListView.h"

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

void Panels::MemoryCardListView::OnListDrag(wxListEvent& evt)
{
	evt.Skip();

	wxFileDataObject my_data;
	my_data.AddFile( (*m_KnownCards)[ GetItemData(GetFirstSelected()) ].Filename.GetFullPath() );

	wxDropSource dragSource( this );
	dragSource.SetData( my_data );
	wxDragResult result = dragSource.DoDragDrop();
}

// return the text for the given column of the given item
wxString Panels::MemoryCardListView::OnGetItemText(long item, long column) const
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
int Panels::MemoryCardListView::OnGetItemImage(long item) const
{
	return _parent::OnGetItemImage( item );
}

// return the icon for the given item and column.
int Panels::MemoryCardListView::OnGetItemColumnImage(long item, long column) const
{
	return _parent::OnGetItemColumnImage( item, column );
}

// return the attribute for the item (may return NULL if none)
wxListItemAttr* Panels::MemoryCardListView::OnGetItemAttr(long item) const
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

	AppStatusEvent_OnSettingsApplied();
}

void MemoryCardInfoPanel::paintEvent(wxPaintEvent & evt)
{
	wxPaintDC dc( this );

	wxFont woot( dc.GetFont() );
	woot.SetWeight( wxBOLD );
	dc.SetFont( woot );

	wxString msg;
	msg = _("No Card (empty)");

	int tWidth, tHeight;
	dc.GetTextExtent( msg, &tWidth, &tHeight );

	dc.DrawText( msg, (dc.GetSize().GetWidth() - tWidth) / 2, 0 );
	//dc.DrawCircle( dc.GetSize().GetWidth()/2, 24, dc.GetSize().GetWidth()/4 );
}

void MemoryCardInfoPanel::Apply()
{
}

void MemoryCardInfoPanel::AppStatusEvent_OnSettingsApplied()
{
}


// --------------------------------------------------------------------------------------
//  MemoryCardsPanel Implementations
// --------------------------------------------------------------------------------------
Panels::MemoryCardsPanel::MemoryCardsPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	m_panel_AllKnownCards = new MemoryCardListPanel( this );

	m_idealWidth -= 48;
	m_check_Ejection = new pxCheckBox( this,
		_("Auto-eject memorycards when loading savestates"),
		pxE( ".Dialog:Memorycards:EnableEjection",
			L"Avoids memorycard corruption by forcing games to re-index card contents after "
			L"loading from savestates.  May not be compatible with all games (Guitar Hero)."
		)
	);
	m_idealWidth += 48;

	wxPanelWithHelpers* columns[2];

	for( uint port=0; port<2; ++port )
	{
		columns[port] = new wxPanelWithHelpers( this, wxVERTICAL );
		columns[port]->SetIdealWidth( (columns[port]->GetIdealWidth()-12) / 2 );

		/*m_check_Multitap[port] = new pxCheckBox( columns[port], wxsFormat(_("Enable Multitap on Port %u"), port+1) );
		m_check_Multitap[port]->SetClientData( (void*) port );
		m_check_Multitap[port]->SetFont( wxFont( m_check_Multitap[port]->GetFont().GetPointSize()+1, wxFONTFAMILY_MODERN, wxNORMAL, wxNORMAL, false, L"Lucida Console" ) );*/

		for( uint slot=0; slot<1; ++slot )
		{
			m_panel_cardinfo[port][slot] = new MemoryCardInfoPanel( columns[port], port, slot );
		}

		//Connect( m_check_Multitap[port]->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(MemoryCardsPanel::OnMultitapChecked));
	}

	// ------------------------------------
	//       Sizer / Layout Section
	// ------------------------------------

	wxFlexGridSizer* s_table = new wxFlexGridSizer( 2 );
	s_table->AddGrowableCol( 0 );
	s_table->AddGrowableCol( 1 );

	for( uint port=0; port<2; ++port )
	{
		wxStaticBoxSizer& portSizer( *new wxStaticBoxSizer( wxVERTICAL, columns[port], wxsFormat(_("Port %u"), port+1) ) );

		*columns[port] += portSizer | SubGroup();

		portSizer += m_panel_cardinfo[port][0]			| SubGroup();
		//portSizer += new wxStaticLine( columns[port] )	| pxExpand.Border( wxTOP, StdPadding );
		//portSizer += m_check_Multitap[port]				| pxCenter.Border( wxBOTTOM, StdPadding );

		/*for( uint slot=1; slot<4; ++slot )
		{
			//portSizer += new wxStaticText( columns[port], wxID_ANY, wxsFormat(_("Slot #%u"), slot+1) ); // | pxCenter;
			wxStaticBoxSizer& staticbox( *new wxStaticBoxSizer( wxVERTICAL, columns[port] ) );
			staticbox += m_panel_cardinfo[port][slot]	| pxExpand;
			portSizer += staticbox | SubGroup();

		}*/

		*s_table += columns[port]	| StdExpand();
	}

	wxBoxSizer& s_checks( *new wxBoxSizer( wxVERTICAL ) );
	s_checks += m_check_Ejection;

	*this += s_table				| pxExpand;
	*this += m_panel_AllKnownCards	| StdExpand();
	*this += s_checks				| StdExpand();

	AppStatusEvent_OnSettingsApplied();
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

