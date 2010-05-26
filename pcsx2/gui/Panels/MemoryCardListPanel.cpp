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
#include "AppCoreThread.h"
#include "System.h"

#include "ConfigurationPanels.h"
#include "MemoryCardPanels.h"

#include <wx/filepicker.h>
#include <wx/ffile.h>
#include <wx/dir.h>
#include "Dialogs/ConfigurationDialog.h"

#include "MemoryCardFile.h"


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

// --------------------------------------------------------------------------------------
//  McdListItem  (implementations)
// --------------------------------------------------------------------------------------
bool McdListItem::IsMultitapSlot() const
{
	return FileMcd_IsMultitapSlot(Slot);
}

uint McdListItem::GetMtapPort() const
{
	return FileMcd_GetMtapPort(Slot);
}

uint McdListItem::GetMtapSlot() const
{
	return FileMcd_GetMtapSlot(Slot);
}

// Compares two cards -- If this equality comparison is used on items where
// no filename is specified, then the check will include port and slot.
bool McdListItem::operator==( const McdListItem& right ) const
{
	bool fileEqu;

	if( Filename.GetFullName().IsEmpty() )
		fileEqu = OpEqu(Slot);
	else
		fileEqu = OpEqu(Filename);

	return fileEqu &&
		OpEqu(IsPresent)	&& OpEqu(IsEnabled)		&&
		OpEqu(SizeInMB)		&& OpEqu(IsFormatted)	&&
		OpEqu(DateCreated)	&& OpEqu(DateModified);
}

bool McdListItem::operator!=( const McdListItem& right ) const
{
	return operator==( right );
}

//DEFINE_EVENT_TYPE( pxEvt_RefreshSelections );

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

	Connect( m_btn_Refresh->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(BaseMcdListPanel::OnRefreshSelections) );
	//Connect( pxEvt_RefreshSelections, wxCommandEventHandler(BaseMcdListPanel::OnRefreshSelections) );
}

void Panels::BaseMcdListPanel::RefreshMcds() const
{
	wxCommandEvent refit( wxEVT_COMMAND_BUTTON_CLICKED );
	refit.SetId( m_btn_Refresh->GetId() );
	GetEventHandler()->AddPendingEvent( refit );
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

void Panels::BaseMcdListPanel::AppStatusEvent_OnSettingsApplied()
{
	if( (m_MultitapEnabled[0] != g_Conf->EmuOptions.MultitapPort0_Enabled) ||
		(m_MultitapEnabled[0] != g_Conf->EmuOptions.MultitapPort0_Enabled) )
	{
		m_MultitapEnabled[0] = g_Conf->EmuOptions.MultitapPort0_Enabled;
		m_MultitapEnabled[1] = g_Conf->EmuOptions.MultitapPort1_Enabled;
		
		RefreshMcds();
	}
}

// --------------------------------------------------------------------------------------
//  McdDataObject
// --------------------------------------------------------------------------------------
class WXDLLEXPORT McdDataObject : public wxDataObjectSimple
{
	DECLARE_NO_COPY_CLASS(McdDataObject)

protected:
	int  m_slot;

public:
	McdDataObject(int slot = -1)
		: wxDataObjectSimple( wxDF_PRIVATE )
	{
		m_slot = slot;
	}
	
	uint GetSlot() const
	{
		pxAssumeDev( m_slot >= 0, "Memorycard Index is uninitialized (invalid drag&drop object state)" );
		return (uint)m_slot;
	}

	size_t GetDataSize() const
	{
		return sizeof(u32);
	}

	bool GetDataHere(void *buf) const
	{
		*(u32*)buf = GetSlot();
		return true;
	}

	virtual bool SetData(size_t len, const void *buf)
	{
		if( !pxAssertDev( len == sizeof(u32), "Data length mismatch on memorycard drag&drop operation." ) ) return false;
		
		m_slot = *(u32*)buf;
		return ( (uint)m_slot < 8 );		// sanity check (unsigned, so that -1 also is invalid) :)
	}

	// Must provide overloads to avoid hiding them (and warnings about it)
	virtual size_t GetDataSize(const wxDataFormat&) const
	{
		return GetDataSize();
	}

	virtual bool GetDataHere(const wxDataFormat&, void *buf) const
	{
		return GetDataHere(buf);
	}

	virtual bool SetData(const wxDataFormat&, size_t len, const void *buf)
	{
		return SetData(len, buf);
	}
};

class McdDropTarget : public wxDropTarget
{
protected:
	BaseMcdListView*	m_listview;

public:
	McdDropTarget( BaseMcdListView* listview=NULL )
	{
		m_listview = listview;
		SetDataObject(new McdDataObject());
	}

	// these functions are called when data is moved over position (x, y) and
	// may return either wxDragCopy, wxDragMove or wxDragNone depending on
	// what would happen if the data were dropped here.
	//
	// the last parameter is what would happen by default and is determined by
	// the platform-specific logic (for example, under Windows it's wxDragCopy
	// if Ctrl key is pressed and wxDragMove otherwise) except that it will
	// always be wxDragNone if the carried data is in an unsupported format.


	// called when the mouse moves in the window - shouldn't take long to
	// execute or otherwise mouse movement would be too slow.
	virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
	{
		int flags = 0;
		int idx = m_listview->HitTest( wxPoint(x,y), flags);
		m_listview->SetTargetedItem( idx );

		if( wxNOT_FOUND == idx ) return wxDragNone;

		return def;
	}
	
	virtual void OnLeave()
	{
		m_listview->SetTargetedItem( wxNOT_FOUND );
	}

	// this function is called when data is dropped at position (x, y) - if it
	// returns true, OnData() will be called immediately afterwards which will
	// allow to retrieve the data dropped.
	virtual bool OnDrop(wxCoord x, wxCoord y)
	{
		int flags = 0;
		int idx = m_listview->HitTest( wxPoint(x,y), flags);
		return ( wxNOT_FOUND != idx );
	}

	// may be called *only* from inside OnData() and will fill m_dataObject
	// with the data from the drop source if it returns true
	virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def)
	{
		m_listview->SetTargetedItem( wxNOT_FOUND );

		int flags = 0;
		int idx = m_listview->HitTest( wxPoint(x,y), flags);
		if( wxNOT_FOUND == idx ) return wxDragNone;

		if ( !GetData() ) return wxDragNone;

		McdDataObject *dobj = (McdDataObject *)m_dataObject;

		wxDragResult result = OnDropMcd(
			m_listview->GetMcdProvider().GetCard(dobj->GetSlot()),
			m_listview->GetMcdProvider().GetCard(idx),
			def
		);

		if( wxDragNone == result ) return wxDragNone;
		m_listview->GetMcdProvider().RefreshMcds();

		return result;
	}
	
	virtual wxDragResult OnDropMcd( const McdListItem& src, const McdListItem& dest, wxDragResult def )
	{
		if( src.Slot == dest.Slot ) return wxDragNone;
		if( !pxAssert( (src.Slot >= 0) && (dest.Slot >= 0) ) ) return wxDragNone;
	
		const wxDirName basepath( m_listview->GetMcdProvider().GetMcdPath() );
		wxFileName srcfile( basepath + g_Conf->Mcd[src.Slot].Filename );
		wxFileName destfile( basepath + g_Conf->Mcd[dest.Slot].Filename );

		if( wxDragCopy == def )
		{
			// user is force invoking copy mode, which means we need to check the destination
			// and prompt if it looks valuable (formatted).

			if( dest.IsPresent && dest.IsFormatted )
			{
				wxsFormat( pxE( ".Popup:Mcd:Overwrite", 
					L"This will copy the contents of the MemoryCard in slot %u over the Memorycard in slot %u. "
					L"All data on the target slot will be lost.  Are you sure?" ), 
					src.Slot, dest.Slot
				);

				//if( !Msgbox::OkCancel(  ) )
				//	return wxDragNone;
			}
			
			ScopedBusyCursor doh( Cursor_ReallyBusy );
			if( !wxCopyFile( srcfile.GetFullPath(), destfile.GetFullPath(),	true ) )
			{
				wxString heading;
				heading.Printf( pxE( ".Error:Mcd:Copy Failed", 
					L"Error!  Could not copy the MemoryCard into slot %u.  The destination file is in use." ),
					dest.Slot
				);
				
				wxString content;

				Msgbox::Alert( heading + L"\n\n" + content, _("Copy failed!") );
				return wxDragNone;
			}
		}
		else if( wxDragMove == def )
		{
			// Move always performs a swap :)

			const bool srcExists( srcfile.FileExists() );
			const bool destExists( destfile.FileExists() );

			bool result = true;

			if( destExists && srcExists) 
			{
				wxFileName tempname;
				tempname.AssignTempFileName( basepath.ToString() ); 

				// Neat trick to handle errors.
				result = result && wxRenameFile( srcfile.GetFullPath(), tempname.GetFullPath(), true );
				result = result && wxRenameFile( destfile.GetFullPath(), srcfile.GetFullPath(), false );
				result = result && wxRenameFile( tempname.GetFullPath(), srcfile.GetFullPath(), true );
			}
			else if( destExists )
			{
				result = wxRenameFile( destfile.GetFullPath(), srcfile.GetFullPath() );
			}
			else if( srcExists )
			{
				result = wxRenameFile( srcfile.GetFullPath(), destfile.GetFullPath() );
			}
			
			if( !result )
			{
				// TODO : Popup an error to the user.

				Console.Error( "(McdFile) Memorycard swap failed." );
				Console.Indent().WriteLn( L"Src : " + srcfile.GetFullPath() );
				Console.Indent().WriteLn( L"Dest: " + destfile.GetFullPath() );
			}
		}
		
		return def;
	}
};

enum McdMenuId
{
	McdMenuId_Create = 0x888,
	McdMenuId_Mount,
	McdMenuId_RefreshList
};

// =====================================================================================================
//  MemoryCardListPanel_Simple (implementations)
// =====================================================================================================
Panels::MemoryCardListPanel_Simple::MemoryCardListPanel_Simple( wxWindow* parent )
	: _parent( parent )
{
	m_MultitapEnabled[0] = false;
	m_MultitapEnabled[1] = false;

	m_listview = new MemoryCardListView_Simple(this);
	m_listview->SetDropTarget( new McdDropTarget(m_listview) );

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
	Connect( m_listview->GetId(),		wxEVT_COMMAND_LIST_ITEM_DESELECTED,	wxListEventHandler(MemoryCardListPanel_Simple::OnListSelectionChanged));

	Connect( m_listview->GetId(),		wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler(MemoryCardListPanel_Simple::OnOpenItemContextMenu) );

	Connect( m_button_Mount->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(MemoryCardListPanel_Simple::OnMountCard));
	Connect( m_button_Create->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(MemoryCardListPanel_Simple::OnCreateCard));

	// Popup Menu Connections!
	
	Connect( McdMenuId_Create,		wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MemoryCardListPanel_Simple::OnCreateCard) );
	Connect( McdMenuId_Mount,		wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MemoryCardListPanel_Simple::OnMountCard) );
	Connect( McdMenuId_RefreshList,	wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MemoryCardListPanel_Simple::OnRefreshSelections) );
}

void Panels::MemoryCardListPanel_Simple::UpdateUI()
{
	if( !m_listview ) return;
	
	int sel = m_listview->GetFirstSelected();

	if( wxNOT_FOUND == sel )
	{
		m_button_Create->Disable();
		m_button_Mount->Disable();
		return;
	}

	const McdListItem& item( m_Cards[sel] );

	m_button_Create->Enable();
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
	//_parent::Apply();

	ScopedCoreThreadClose closed_core;
	closed_core.AllowResume();

	for( uint slot=0; slot<8; ++slot )
	{
		g_Conf->Mcd[slot].Enabled = m_Cards[slot].IsEnabled && m_Cards[slot].IsPresent;
	}
}

void Panels::MemoryCardListPanel_Simple::AppStatusEvent_OnSettingsApplied()
{
	for( uint slot=0; slot<8; ++slot )
	{
		m_Cards[slot].IsEnabled = g_Conf->Mcd[slot].Enabled;
	}

	_parent::AppStatusEvent_OnSettingsApplied();
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

bool Panels::MemoryCardListPanel_Simple::ValidateEnumerationStatus()
{
	if( m_listview ) m_listview->SetMcdProvider( NULL );
	return false;
}

void Panels::MemoryCardListPanel_Simple::DoRefresh()
{
	for( uint slot=0; slot<8; ++slot )
	{
		wxFileName fullpath( m_FolderPicker->GetPath() + g_Conf->Mcd[slot].Filename.GetFullName() );
		EnumerateMemoryCard( m_Cards[slot], fullpath );

		m_Cards[slot].Slot = slot;

		if( FileMcd_IsMultitapSlot(slot) && !m_MultitapEnabled[FileMcd_GetMtapPort(slot)] )
			m_Cards[slot].IsEnabled = false;
	}
	
	if( m_listview ) m_listview->SetMcdProvider( this );
	UpdateUI();
}

void Panels::MemoryCardListPanel_Simple::OnCreateCard(wxCommandEvent& evt)
{
	const int	sel		= m_listview->GetFirstSelected();
	if( wxNOT_FOUND == sel ) return;
	const uint	slot	= sel;

	if( m_Cards[slot].IsPresent )
	{
		wxWindowID result = wxID_YES;
		if( m_Cards[slot].IsFormatted )
		{
			wxString content;
			content.Printf(wxsFormat(
				pxE(".Popup:DeleteMemoryCard",
					L"You are about to delete the formatted memory card in slot %u. "
					L"All data on this card will be lost!  Are you absolutely and quite positively sure?"
				), slot )
			);

			result = Msgbox::YesNo( content, _("Delete MemoryCard?") );
		}

		if( result == wxID_YES )
		{
			wxFileName fullpath( m_FolderPicker->GetPath() + g_Conf->Mcd[slot].Filename.GetFullName() );
			wxRemoveFile( fullpath.GetFullPath() );
		}
	}
	else
		Dialogs::CreateMemoryCardDialog( this, slot, m_FolderPicker->GetPath() ).ShowModal();

	RefreshSelections();
}

/*void Panels::MemoryCardListPanel_Simple::OnSwapPorts(wxCommandEvent& evt)
{
	
}*/

void Panels::MemoryCardListPanel_Simple::OnMountCard(wxCommandEvent& evt)
{
	evt.Skip();

	const int	sel		= m_listview->GetFirstSelected();
	if( wxNOT_FOUND == sel ) return;
	const uint	slot	= sel;

	m_Cards[slot].IsEnabled = !m_Cards[slot].IsEnabled;
	m_listview->RefreshItem(slot);
	UpdateUI();
}

void Panels::MemoryCardListPanel_Simple::OnListDrag(wxListEvent& evt)
{
	int selection = m_listview->GetFirstSelected();

	if( selection < 0 ) return;
	McdDataObject my_data( selection );
	
	wxDropSource dragSource( m_listview );
	dragSource.SetData( my_data );
	wxDragResult result = dragSource.DoDragDrop( wxDrag_AllowMove );
}

void Panels::MemoryCardListPanel_Simple::OnListSelectionChanged(wxListEvent& evt)
{
	UpdateUI();
}

void Panels::MemoryCardListPanel_Simple::OnOpenItemContextMenu(wxListEvent& evt)
{
	int idx = evt.GetIndex();

	wxMenu* junk = new wxMenu();
	
	if( idx != wxNOT_FOUND )
	{
		const McdListItem& item( m_Cards[idx] );

		junk->Append( McdMenuId_Create,	item.IsPresent ? _("Delete")	: _("Create new...") );
		junk->Append( McdMenuId_Mount,	item.IsEnabled ? _("Disable")	: _("Enable") );

		junk->AppendSeparator();
	}

	junk->Append( McdMenuId_RefreshList, _("Refresh List") );
	
	PopupMenu( junk );
	m_listview->RefreshItem( idx );
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
	return m_Cards[idx];
}

McdListItem& Panels::MemoryCardListPanel_Simple::GetCard( int idx )
{
	return m_Cards[idx];
}
