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
MemoryCardListPanel::MemoryCardListPanel( wxWindow* parent )
	: BaseSelectorPanel( parent )
{
	m_FolderPicker = new DirPickerPanel( this, FolderId_MemoryCards,
		//_("MemoryCard Search Path:"),				// static box label
		_("Select folder with PS2 MemoryCards")		// dir picker popup label
	);

	m_listview = new MemoryCardListView(this);

	wxButton* button_Create		= new wxButton(this, wxID_ANY, _("Create new card..."));

	//Connect( m_list_AllKnownCards->GetId(), wxEVT_COMMAND_LEFT_CLICK, MemoryCardsPanel::OnListDrag);

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

	m_listview->SetMinSize( wxSize( wxDefaultCoord, 120 ) );

	AppStatusEvent_OnSettingsApplied();
	
	Disable();
}

void MemoryCardListPanel::Apply()
{
}

void MemoryCardListPanel::AppStatusEvent_OnSettingsApplied()
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

bool MemoryCardListPanel::ValidateEnumerationStatus()
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

	m_listview->AssignCardsList( NULL );
	m_KnownCards.SwapPtr( mcdlist );

	return validated;
}

void MemoryCardListPanel::DoRefresh()
{
	if( !m_KnownCards ) return;

	//m_listview->ClearAll();
	//m_listview->CreateColumns();

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

	m_listview->AssignCardsList( m_KnownCards );
}
