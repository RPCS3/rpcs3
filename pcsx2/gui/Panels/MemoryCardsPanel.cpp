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


enum McdColumnType_Advanced
{
	McdCol_Filename,
	McdCol_Mounted,
	McdCol_Size,
	McdCol_Formatted,
	McdCol_DateModified,
	McdCol_DateCreated,
	McdCol_Count
};

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

	for( int i=0; i<McdCol_Count; ++i )
		InsertColumn( i, columns[i].name, columns[i].align, -1 );
}

void MemoryCardListView_Advanced::SetCardCount( int length )
{
	if( !m_CardProvider ) length = 0;
	SetItemCount( length );
	Refresh();
}

MemoryCardListView_Advanced::MemoryCardListView_Advanced( wxWindow* parent )
	: _parent( parent )
{
	CreateColumns();
	Connect( wxEVT_COMMAND_LIST_BEGIN_DRAG,	wxListEventHandler(MemoryCardListView_Advanced::OnListDrag));
}

void MemoryCardListView_Advanced::OnListDrag(wxListEvent& evt)
{
	evt.Skip();
}

// return the text for the given column of the given item
wxString MemoryCardListView_Advanced::OnGetItemText(long item, long column) const
{
	if( !m_CardProvider ) return _parent::OnGetItemText(item, column);

	const McdListItem& it( m_CardProvider->GetCard(item) );

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
		//case McdCol_Path:			return it.Filename.GetPath();
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
	
	// [TODO] Add Unmount button.
}


void MemoryCardInfoPanel::paintEvent(wxPaintEvent & evt)
{
	wxPaintDC dc( this );
	pxWindowTextWriter writer( dc );

	writer.Bold();

	// Create DC and plot some text (and images!)
	writer.WriteLn( m_DisplayName );

	//dc.DrawCircle( dc.GetSize().GetWidth()/2, 24, dc.GetSize().GetWidth()/4 );

	if( !m_ErrorMessage.IsEmpty() )
	{
		writer.WriteLn();
		writer.WriteLn( m_ErrorMessage );
	}
	else if( m_cardInfo )
	{
		writer.Normal();

		writer.WriteLn( wxsFormat( L"%d MB (%s)",
			m_cardInfo->SizeInMB,
			m_cardInfo->IsFormatted ? _("Formatted") : _("Unformatted") )
		);
	}
}

void MemoryCardInfoPanel::Eject()
{
	m_cardInfo = NULL;
	Refresh();
}

void MemoryCardInfoPanel::Apply()
{
	if( m_cardInfo && m_cardInfo->Filename.GetFullName().IsEmpty() ) m_cardInfo = NULL;
	
	if( m_cardInfo )
	{
		wxFileName absfile( Path::Combine( g_Conf->Folders.MemoryCards, m_cardInfo->Filename ) );

		// The following checks should be theoretically unreachable, unless the user's
		// filesystem is changed form under our nose.  A little validation goes a
		// long way. :p
		
		if( absfile.IsDir() )
		{
			Eject();
			throw Exception::CannotApplySettings( this, 
				// Diagnostic
				wxsFormat( L"Memorycard in Port %u, Slot %u conflicts with an existing directory.", m_port, m_slot ),
				// Translated
				wxsFormat(
					_("Cannot use or create the memorycard in Port %u, Slot %u: the filename conflicts with an existing directory."),
					m_port, m_slot
				)
			);
		}

		if( !absfile.FileExists() )
		{
			Eject();
			throw Exception::CannotApplySettings( this, 
				// Diagnostic
				wxsFormat( L"Memorycard in Port %u, Slot %u is no longer valid.", m_port, m_slot ),
				// Translated
				wxsFormat(
					_("The configured memorycard in Port %u, Slot %u no longer exists.  Please create a new memory card, or leave the slot unmounted."),
					m_port, m_slot
				)
			);
		}

		g_Conf->Mcd[m_port][m_slot].Filename = m_cardInfo->Filename;
		g_Conf->Mcd[m_port][m_slot].Enabled = true;
	}
	else
	{
		// Card is either disabled or in an error state.

		g_Conf->Mcd[m_port][m_slot].Enabled = false;
		g_Conf->Mcd[m_port][m_slot].Filename.Clear();
	}
}

void MemoryCardInfoPanel::AppStatusEvent_OnSettingsApplied()
{
	m_cardInfo = NULL;

	// Collect Info and Format Strings

	wxString fname( g_Conf->Mcd[m_port][m_slot].Filename.GetFullPath() );
	if( fname.IsEmpty() )
	{
		m_DisplayName = _("No Card (empty)");
		m_cardInfo = NULL;
	}
	else
	{
		wxFileName absfile( Path::Combine( g_Conf->Folders.MemoryCards, fname ) );
		wxFileName relfile( fname );

		if( !m_cardInfo )
		{
			m_cardInfo = new McdListItem();
			if( !EnumerateMemoryCard( *m_cardInfo, absfile.GetFullPath() ) )
			{
				m_ErrorMessage = _("Read Error: Card is truncated or corrupted.");
			}
		}

		absfile.Normalize();
		relfile.Normalize();

		m_DisplayName = ( absfile == relfile ) ? relfile.GetFullName() : relfile.GetFullPath();
	}

	Refresh();
}


// --------------------------------------------------------------------------------------
//  MemoryCardsPanel Implementations
// --------------------------------------------------------------------------------------
Panels::MemoryCardsPanel::MemoryCardsPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	m_panel_AllKnownCards = new MemoryCardListPanel_Advanced( this );

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
