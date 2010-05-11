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

static bool EnumerateMemoryCard( McdListItem& dest, const wxString& filename )
{
	Console.WriteLn( filename.c_str() );
	wxFFile mcdFile( filename );
	if( !mcdFile.IsOpened() ) return false;	// wx should log the error for us.
	if( mcdFile.Length() < (1024*528) )
	{
		Console.Warning( "... MemoryCard appears to be truncated.  Ignoring." );
		return false;
	}

	dest.Filename		= filename;
	dest.SizeInMB		= (uint)(mcdFile.Length() / (1024 * 528 * 2));
	dest.IsFormatted	= IsMcdFormatted( mcdFile );
	wxFileName(filename).GetTimes( NULL, &dest.DateModified, &dest.DateCreated );

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
//  MemoryCardListPanel
// =====================================================================================================
Panels::MemoryCardListPanel::MemoryCardListPanel( wxWindow* parent )
	: BaseSelectorPanel( parent )
{
	m_FolderPicker = new DirPickerPanel( this, FolderId_MemoryCards,
		//_("MemoryCard Search Path:"),				// static box label
		_("Select folder with PS2 MemoryCards")		// dir picker popup label
	);

	m_listview = new MemoryCardListView(this);

	wxButton* button_Create		= new wxButton(this, wxID_ANY, _("Create new card..."));

	// ------------------------------------
	//       Sizer / Layout Section
	// ------------------------------------

	wxBoxSizer& s_buttons	(*new wxBoxSizer( wxHORIZONTAL ));
	wxBoxSizer& s_leftside	(*new wxBoxSizer( wxHORIZONTAL ));
	wxBoxSizer& s_rightside	(*new wxBoxSizer( wxHORIZONTAL ));

	s_leftside	+= button_Create	| StdSpace();

	s_buttons += s_leftside		| pxAlignLeft;
	s_buttons += pxStretchSpacer();
	s_buttons += s_rightside	| pxAlignRight;

	*this += m_FolderPicker | pxExpand;
	*this += m_listview		| pxExpand;
	*this += s_buttons		| pxExpand;

	m_listview->SetMinSize( wxSize( m_idealWidth, 120 ) );
	
	Connect( m_listview->GetId(),		wxEVT_COMMAND_LIST_BEGIN_DRAG,	wxListEventHandler(MemoryCardListPanel::OnListDrag));
	Connect( button_Create->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(MemoryCardListPanel::OnCreateNewCard));
}

void Panels::MemoryCardListPanel::Apply()
{
}

void Panels::MemoryCardListPanel::AppStatusEvent_OnSettingsApplied()
{
}

bool Panels::MemoryCardListPanel::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
	if( filenames.GetCount() == 1 && wxFileName(filenames[0]).IsDir() )
	{
		m_FolderPicker->SetPath( filenames[0] );
		return true;
	}
	return false;
}

bool Panels::MemoryCardListPanel::ValidateEnumerationStatus()
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

void Panels::MemoryCardListPanel::DoRefresh()
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

void Panels::MemoryCardListPanel::OnCreateNewCard(wxCommandEvent& evt)
{

}

void Panels::MemoryCardListPanel::OnListDrag(wxListEvent& evt)
{
	wxFileDataObject my_data;
	my_data.AddFile( (*m_KnownCards)[m_listview->GetItemData(m_listview->GetFirstSelected())].Filename.GetFullPath() );

	wxDropSource dragSource( m_listview );
	dragSource.SetData( my_data );
	wxDragResult result = dragSource.DoDragDrop();
}

int Panels::MemoryCardListPanel::GetLength() const
{
	return m_KnownCards ? m_KnownCards->size() : 0;
}

const McdListItem& Panels::MemoryCardListPanel::Get( int idx ) const
{
	pxAssume(!!m_KnownCards);
	return (*m_KnownCards)[idx];
}

McdListItem& Panels::MemoryCardListPanel::Get( int idx )
{
	pxAssume(!!m_KnownCards);
	return (*m_KnownCards)[idx];
}
