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

#include <wx/filepicker.h>
#include <wx/ffile.h>
#include <wx/dir.h>


using namespace pxSizerFlags;
using namespace Panels;

static bool IsMcdFormatted( wxFFile& fhand )
{
	static const char formatted_string[] = "Sony PS2 Memory Card Format";
	static const int fmtstrlen = sizeof( formatted_string )-1;

	char dest[fmtstrlen];

	fhand.Read( dest, fmtstrlen );
	return memcmp( formatted_string, dest, fmtstrlen ) == 0;
}

bool EnumerateMemoryCard( McdListItem& dest, const wxFileName& filename )
{
	dest.IsFormatted	= false;
	dest.IsEnabled		= false;
	dest.IsPresent		= false;

	const wxString fullpath( filename.GetFullPath() );
	if( !filename.FileExists() ) return false;

	Console.WriteLn( fullpath );
	wxFFile mcdFile( fullpath );
	if( !mcdFile.IsOpened() ) return false;	// wx should log the error for us.
	if( mcdFile.Length() < (1024*528) )
	{
		Console.Warning( "... MemoryCard appears to be truncated.  Ignoring." );
		return false;
	}

	dest.IsPresent		= true;
	dest.IsEnabled		= true;
	dest.Filename		= filename;
	dest.SizeInMB		= (uint)(mcdFile.Length() / (1024 * 528 * 2));
	dest.IsFormatted	= IsMcdFormatted( mcdFile );
	filename.GetTimes( NULL, &dest.DateModified, &dest.DateCreated );

	return true;
}

static int EnumerateMemoryCards( McdList& dest, const wxArrayString& files )
{
	int pushed = 0;
	Console.WriteLn( Color_StrongBlue, "Enumerating MemoryCards..." );
	for( size_t i=0; i<files.GetCount(); ++i )
	{
		ConsoleIndentScope con_indent;
		McdListItem mcdItem;
		if( EnumerateMemoryCard(mcdItem, files[i]) )
		{
			dest.push_back( mcdItem );
			++pushed;
		}
	}
	if( pushed > 0 )
		Console.WriteLn( Color_StrongBlue, "MemoryCard Enumeration Complete." );
	else
		Console.WriteLn( Color_StrongBlue, "No valid MemoryCards found." );

	return pushed;
}

// =====================================================================================================
//  BaseMcdListPanel (implementations)
// =====================================================================================================
Panels::BaseMcdListPanel::BaseMcdListPanel( wxWindow* parent )
	: _parent( parent )
{
	m_FolderPicker = new DirPickerPanel( this, FolderId_MemoryCards,
		//_("MemoryCard Search Path:"),				// static box label
		_("Select folder with PS2 MemoryCards")		// dir picker popup label
	);
	
	m_listview = NULL;
	
	m_btn_Refresh = new wxButton( this, wxID_ANY, _("Refresh list") );

	Connect( m_btn_Refresh->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(BaseMcdListPanel::OnRefresh) );
}

void Panels::BaseMcdListPanel::CreateLayout()
{
	if( m_listview ) m_listview->SetMinSize( wxSize( m_idealWidth, 140 ) );

	wxBoxSizer& s_buttons(*new wxBoxSizer( wxHORIZONTAL ));
	s_leftside_buttons	= new wxBoxSizer( wxHORIZONTAL );
	s_rightside_buttons	= new wxBoxSizer( wxHORIZONTAL );

	s_buttons += s_leftside_buttons		| pxAlignLeft;
	s_buttons += pxStretchSpacer();
	s_buttons += s_rightside_buttons	| pxAlignRight;

	if( m_FolderPicker )	*this += m_FolderPicker | pxExpand;
	if( m_listview )		*this += m_listview		| pxExpand;
							*this += s_buttons		| pxExpand;

	*s_leftside_buttons += m_btn_Refresh;
}

// =====================================================================================================
//  MemoryCardListPanel_Advanced (implementations)
// =====================================================================================================
Panels::MemoryCardListPanel_Advanced::MemoryCardListPanel_Advanced( wxWindow* parent )
	: _parent( parent )
{
	m_FolderPicker = new DirPickerPanel( this, FolderId_MemoryCards,
		//_("MemoryCard Search Path:"),				// static box label
		_("Select folder with PS2 MemoryCards")		// dir picker popup label
	);

	m_listview = new MemoryCardListView_Advanced(this);

	wxButton* button_Create		= new wxButton(this, wxID_ANY, _("Create new card..."));

	// ------------------------------------
	//       Sizer / Layout Section
	// ------------------------------------

	CreateLayout();
	*s_leftside_buttons	+= button_Create	| StdSpace();
	
	Connect( m_listview->GetId(),		wxEVT_COMMAND_LIST_BEGIN_DRAG,	wxListEventHandler(MemoryCardListPanel_Advanced::OnListDrag));
	Connect( button_Create->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(MemoryCardListPanel_Advanced::OnCreateNewCard));
}

void Panels::MemoryCardListPanel_Advanced::Apply()
{
}

void Panels::MemoryCardListPanel_Advanced::AppStatusEvent_OnSettingsApplied()
{
}

bool Panels::MemoryCardListPanel_Advanced::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
	if( filenames.GetCount() == 1 && wxFileName(filenames[0]).IsDir() )
	{
		m_FolderPicker->SetPath( filenames[0] );
		return true;
	}
	return false;
}

bool Panels::MemoryCardListPanel_Advanced::ValidateEnumerationStatus()
{
	bool validated = true;

	// Impl Note: ScopedPtr used so that resources get cleaned up if an exception
	// occurs during file enumeration.
	ScopedPtr<McdList> mcdlist( new McdList() );

	if( m_FolderPicker->GetPath().Exists() )
	{
		wxArrayString files;
		wxDir::GetAllFiles( m_FolderPicker->GetPath().ToString(), &files, L"*.ps2", wxDIR_FILES );
		EnumerateMemoryCards( *mcdlist, files );
	}

	if( !m_KnownCards || (*mcdlist != *m_KnownCards) )
		validated = false;

	m_listview->SetInterface( NULL );
	m_KnownCards.SwapPtr( mcdlist );

	return validated;
}

void Panels::MemoryCardListPanel_Advanced::DoRefresh()
{
	if( !m_KnownCards ) return;

	for( size_t i=0; i<m_KnownCards->size(); ++i )
	{
		McdListItem& mcditem( (*m_KnownCards)[i] );

		for( int port=0; port<2; ++port )
		for( int slot=0; slot<4; ++slot )
		{
			wxFileName right( g_Conf->FullpathToMcd(port, slot) );
			right.MakeAbsolute();

			wxFileName left( mcditem.Filename );
			left.MakeAbsolute();

			if( left == right )
			{
				mcditem.Port = port;
				mcditem.Slot = slot;
			}
		}
	}

	m_listview->SetInterface( this );
	//m_listview->SetCardCount( m_KnownCards->size() );
}

void Panels::MemoryCardListPanel_Advanced::OnCreateNewCard(wxCommandEvent& evt)
{

}

void Panels::MemoryCardListPanel_Advanced::OnListDrag(wxListEvent& evt)
{
	wxFileDataObject my_data;
	my_data.AddFile( (*m_KnownCards)[m_listview->GetItemData(m_listview->GetFirstSelected())].Filename.GetFullPath() );

	wxDropSource dragSource( m_listview );
	dragSource.SetData( my_data );
	wxDragResult result = dragSource.DoDragDrop();
}

int Panels::MemoryCardListPanel_Advanced::GetLength() const
{
	return m_KnownCards ? m_KnownCards->size() : 0;
}

const McdListItem& Panels::MemoryCardListPanel_Advanced::GetCard( int idx ) const
{
	pxAssume(!!m_KnownCards);
	return (*m_KnownCards)[idx];
}

McdListItem& Panels::MemoryCardListPanel_Advanced::GetCard( int idx )
{
	pxAssume(!!m_KnownCards);
	return (*m_KnownCards)[idx];
}

uint Panels::MemoryCardListPanel_Advanced::GetPort( int idx ) const
{
	pxAssume(!!m_KnownCards);
	return (*m_KnownCards)[idx].Port;
}

uint Panels::MemoryCardListPanel_Advanced::GetSlot( int idx ) const
{
	pxAssume(!!m_KnownCards);
	return (*m_KnownCards)[idx].Slot;
}

// =====================================================================================================
//  MemoryCardListPanel_Simple (implementations)
// =====================================================================================================
Panels::MemoryCardListPanel_Simple::MemoryCardListPanel_Simple( wxWindow* parent )
	: _parent( parent )
{
	m_MultitapEnabled[0] = false;
	m_MultitapEnabled[1] = false;

	m_listview = new MemoryCardListView_Simple(this);

	m_button_Create	= new wxButton(this, wxID_ANY, _("Create"));
	m_button_Mount	= new wxButton(this, wxID_ANY, _("Mount"));

	// ------------------------------------
	//       Sizer / Layout Section
	// ------------------------------------

	CreateLayout();

	*s_leftside_buttons	+= m_button_Create;
	*s_leftside_buttons	+= 2;
	*s_leftside_buttons	+= m_button_Mount;
	//*s_leftside_buttons	+= 2;

	Connect( m_listview->GetId(),		wxEVT_COMMAND_LIST_BEGIN_DRAG,		wxListEventHandler(MemoryCardListPanel_Simple::OnListDrag));
	Connect( m_listview->GetId(),		wxEVT_COMMAND_LIST_ITEM_SELECTED,	wxListEventHandler(MemoryCardListPanel_Simple::OnListSelectionChanged));
	Connect( m_listview->GetId(),		wxEVT_COMMAND_LIST_ITEM_SELECTED,	wxListEventHandler(MemoryCardListPanel_Simple::OnListSelectionChanged));

	Connect( m_button_Mount->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(MemoryCardListPanel_Simple::OnMountCard));
	Connect( m_button_Create->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(MemoryCardListPanel_Simple::OnCreateCard));
}

void Panels::MemoryCardListPanel_Simple::UpdateUI()
{
	if( !m_listview ) return;
	
	int sel = m_listview->GetFirstSelected();

	/*if( !pxAssertDev( m_listview->GetItemData(sel), "Selected memorycard item data is NULL!" ) )
	{
		m_button_Create->Disable();
		m_button_Mount->Disable();
		return;
	}*/

	//const McdListItem& item( *(McdListItem*)m_listview->GetItemData(sel) );
	const McdListItem& item( m_Cards[GetPort(sel)][GetSlot(sel)] );

	m_button_Create->SetLabel( item.IsPresent ? _("Delete") : _("Create") );
	pxSetToolTip( m_button_Create,
		item.IsPresent
			? _("Deletes the existing MemoryCard from disk (all contents are lost)." )
			: _("Creates a new MemoryCard in the empty slot." )
	);

	m_button_Mount->Enable( item.IsPresent );
	m_button_Mount->SetLabel( item.IsEnabled ? _("Disable") : _("Enable") );
	pxSetToolTip( m_button_Mount,
		item.IsEnabled
			? _("Disables the selected MemoryCard, so that it will not be seen by games or BIOS.")
			: _("Mounts the selected MemoryCard, so that games can see it again.")
	);

}

void Panels::MemoryCardListPanel_Simple::Apply()
{
}

void Panels::MemoryCardListPanel_Simple::AppStatusEvent_OnSettingsApplied()
{
}

bool Panels::MemoryCardListPanel_Simple::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
	if( filenames.GetCount() == 1 && wxFileName(filenames[0]).IsDir() )
	{
		m_FolderPicker->SetPath( filenames[0] );
		return true;
	}
	return false;
}

static wxString GetAutoMcdName(uint port, uint slot)
{
	if( slot > 0 )
		return wxsFormat( L"mcd-port%u-slot%02u.ps2", port+1, slot+1 );
	else
		return wxsFormat( L"Mcd%03u.ps2", port+1 );
}

bool Panels::MemoryCardListPanel_Simple::ValidateEnumerationStatus()
{
	if( m_listview ) m_listview->SetInterface( NULL );
	return false;
}

void Panels::MemoryCardListPanel_Simple::DoRefresh()
{
	for( uint port=0; port<2; ++port )
	{
		for( uint slot=0; slot<4; ++slot )
		{
			wxFileName fullpath( m_FolderPicker->GetPath() + GetAutoMcdName(port,slot) );
			EnumerateMemoryCard( m_Cards[port][slot], fullpath );

			if( (slot > 0) && !m_MultitapEnabled[port] )
				m_Cards[port][slot].IsEnabled = false;
		}
	}
	
	if( m_listview ) m_listview->SetInterface( this );
}

void Panels::MemoryCardListPanel_Simple::OnCreateCard(wxCommandEvent& evt)
{

}

void Panels::MemoryCardListPanel_Simple::OnMountCard(wxCommandEvent& evt)
{

}

void Panels::MemoryCardListPanel_Simple::OnListDrag(wxListEvent& evt)
{
	wxFileDataObject my_data;
	/*my_data.AddFile( (*m_KnownCards)[m_listview->GetItemData(m_listview->GetFirstSelected())].Filename.GetFullPath() );

	wxDropSource dragSource( m_listview );
	dragSource.SetData( my_data );
	wxDragResult result = dragSource.DoDragDrop();*/
}

void Panels::MemoryCardListPanel_Simple::OnListSelectionChanged(wxListEvent& evt)
{
	UpdateUI();
}

// Interface Implementation for IMcdList
int Panels::MemoryCardListPanel_Simple::GetLength() const
{
	uint baselen = 2;
	if( m_MultitapEnabled[0] ) baselen += 3;
	if( m_MultitapEnabled[1] ) baselen += 3;
	return baselen;
}

const McdListItem& Panels::MemoryCardListPanel_Simple::GetCard( int idx ) const
{
	return m_Cards[GetPort(idx)][GetSlot(idx)];
}

McdListItem& Panels::MemoryCardListPanel_Simple::GetCard( int idx )
{
	return m_Cards[GetPort(idx)][GetSlot(idx)];
}

uint Panels::MemoryCardListPanel_Simple::GetPort( int idx ) const
{
	if( !m_MultitapEnabled[0] && !m_MultitapEnabled[1] )
	{
		pxAssume( idx < 2 );
		return idx;
	}

	if( !m_MultitapEnabled[0] )
		return (idx==0) ? 0 : 1;
	else
		return idx / 4;
}

uint Panels::MemoryCardListPanel_Simple::GetSlot( int idx ) const
{
	if( !m_MultitapEnabled[0] && !m_MultitapEnabled[1] )
	{
		pxAssume( idx < 2 );
		return 0;
	}

	if( !m_MultitapEnabled[0] )
		return (idx==0) ? 0 : (idx-1);
	else
		return idx & 3;
}
