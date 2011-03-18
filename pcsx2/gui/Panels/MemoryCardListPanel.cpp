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
#include "MemoryCardFile.h"

#include "ConfigurationPanels.h"
#include "MemoryCardPanels.h"

#include "Dialogs/ConfigurationDialog.h"
#include "Utilities/IniInterface.h"
#include "../../Sio.h"

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

//sets IsPresent if the file is valid, and derived properties (filename, formatted, size, etc)
bool EnumerateMemoryCard( McdSlotItem& dest, const wxFileName& filename, const wxDirName basePath )
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
	if( filename.GetFullPath() == (basePath+filename.GetFullName()).GetFullPath() )
		dest.Filename = filename.GetFullName();

	dest.SizeInMB		= (uint)(mcdFile.Length() / (1024 * 528 * 2));
	dest.IsFormatted	= IsMcdFormatted( mcdFile );
	filename.GetTimes( NULL, &dest.DateModified, &dest.DateCreated );

	return true;
}

//avih: unused
/*
static int EnumerateMemoryCards( McdList& dest, const wxArrayString& files )
{
	int pushed = 0;
	Console.WriteLn( Color_StrongBlue, "Enumerating memory cards..." );
	for( size_t i=0; i<files.GetCount(); ++i )
	{
		ConsoleIndentScope con_indent;
		McdSlotItem mcdItem;
		if( EnumerateMemoryCard(mcdItem, files[i]) )
		{
			dest.push_back( mcdItem );
			++pushed;
		}
	}
	if( pushed > 0 )
		Console.WriteLn( Color_StrongBlue, "Memory card Enumeration Complete." );
	else
		Console.WriteLn( Color_StrongBlue, "No valid memory card found." );

	return pushed;
}
*/
// --------------------------------------------------------------------------------------
//  McdListItem  (implementations)
// --------------------------------------------------------------------------------------
bool McdSlotItem::IsMultitapSlot() const
{
	return FileMcd_IsMultitapSlot(Slot);
}

uint McdSlotItem::GetMtapPort() const
{
	return FileMcd_GetMtapPort(Slot);
}

uint McdSlotItem::GetMtapSlot() const
{
	return FileMcd_GetMtapSlot(Slot);
}

// Compares two cards -- If this equality comparison is used on items where
// no filename is specified, then the check will include port and slot.
bool McdSlotItem::operator==( const McdSlotItem& right ) const
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

bool McdSlotItem::operator!=( const McdSlotItem& right ) const
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
		//_("memory card Search Path:"),				// static box label
		_("Select folder with PS2 memory cards")		// dir picker popup label
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
	//if( m_listview ) m_listview->SetMinSize( wxSize( 480, 140 ) );

	wxFlexGridSizer* s_flex=new wxFlexGridSizer(3,1, 0, 0);
	s_flex->AddGrowableCol(0);
	s_flex->AddGrowableRow(1);
	SetSizer(s_flex);

	wxBoxSizer& s_buttons(*new wxBoxSizer( wxHORIZONTAL ));
	s_leftside_buttons	= new wxBoxSizer( wxHORIZONTAL );
	s_rightside_buttons	= new wxBoxSizer( wxHORIZONTAL );

	s_buttons += s_leftside_buttons		| pxAlignLeft;
	s_buttons += pxStretchSpacer();
	s_buttons += s_rightside_buttons	| pxAlignRight;

	if( m_FolderPicker )	*this += m_FolderPicker | pxExpand;
	else					*this += StdPadding;//we need the 'else' because we need these items to land into the proper rows of s_flex.

	if( m_listview )		*this += m_listview		| pxExpand;
	else					*this += StdPadding;

							*this += s_buttons		| pxExpand;

	*s_leftside_buttons += m_btn_Refresh;

	if (m_listview)
	{
		IniLoader loader;
		ScopedIniGroup group( loader, L"MemoryCardListPanel" );
		m_listview->LoadSaveColumns( loader );
	}
}

void Panels::BaseMcdListPanel::Apply()
{
	// Save column widths to the configuration file.  Since these are used *only* by this
	// dialog, we use a direct local ini save approach, instead of going through g_conf.
	if (m_listview)
	{
		IniSaver saver;
		ScopedIniGroup group( saver, L"MemoryCardListPanel" );
		m_listview->LoadSaveColumns(saver);
	}
}

void Panels::BaseMcdListPanel::AppStatusEvent_OnSettingsApplied()
{
	if( (m_MultitapEnabled[0] != g_Conf->EmuOptions.MultitapPort0_Enabled) ||
		(m_MultitapEnabled[1] != g_Conf->EmuOptions.MultitapPort1_Enabled) )
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
	int  m_viewIndex;

public:
	McdDataObject(int viewIndex = -1)
		: wxDataObjectSimple( wxDF_PRIVATE )
	{
		m_viewIndex = viewIndex;
	}

	uint GetViewIndex() const
	{
		pxAssumeDev( m_viewIndex >= 0, "memory card view-Index is uninitialized (invalid drag&drop object state)" );
		return (uint)m_viewIndex;
	}

	size_t GetDataSize() const
	{
		return sizeof(u32);
	}

	bool GetDataHere(void *buf) const
	{
		*(u32*)buf = GetViewIndex();
		return true;
	}

	virtual bool SetData(size_t len, const void *buf)
	{
		if( !pxAssertDev( len == sizeof(u32), "Data length mismatch on memory card drag&drop operation." ) ) return false;

		m_viewIndex = *(u32*)buf;
		return true;//( (uint)m_viewIndex < 8 );		// sanity check (unsigned, so that -1 also is invalid) :)
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
		int viewIndex = m_listview->HitTest( wxPoint(x,y), flags);
		m_listview->SetTargetedItem( viewIndex );

		// can always drop. non item target is the filesystem placeholder. //if( wxNOT_FOUND == viewIndex ) return wxDragNone;

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
		int viewIndex = m_listview->HitTest( wxPoint(x,y), flags);
		return true;// can always drop. non item target is the filesystem placeholder.//( wxNOT_FOUND != viewIndex );
	}

	// may be called *only* from inside OnData() and will fill m_dataObject
	// with the data from the drop source if it returns true
	virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def)
	{
		m_listview->SetTargetedItem( wxNOT_FOUND );
		int flags = 0;

		int destViewIndex = m_listview->HitTest( wxPoint(x,y), flags);
		if( wxNOT_FOUND == destViewIndex )
			destViewIndex=-1;//non list item target is the filesystem placeholder.

		if ( !GetData() )
			return wxDragNone;

		McdDataObject *dobj = (McdDataObject *)m_dataObject;
		int sourceViewIndex = dobj->GetViewIndex();

		wxDragResult result = OnDropMcd(
			m_listview->GetMcdProvider().GetCardForViewIndex( sourceViewIndex ),
			m_listview->GetMcdProvider().GetCardForViewIndex( destViewIndex ),
			def
		);

		if( wxDragNone == result )
			return wxDragNone;

		m_listview->GetMcdProvider().RefreshMcds();
		return result;
	}


	virtual wxDragResult OnDropMcd( McdSlotItem& src, McdSlotItem& dest, wxDragResult def )
	{
		if( src.Slot == dest.Slot ) return wxDragNone;
		//if( !pxAssert( (src.Slot >= 0) && (dest.Slot >= 0) ) ) return wxDragNone;
		const wxDirName basepath( m_listview->GetMcdProvider().GetMcdPath() );

		bool result = true;

		if( wxDragCopy == def )
		{
			if( !m_listview->GetMcdProvider().UiDuplicateCard(src, dest) )
				return wxDragNone;
		}
		else if( wxDragMove == def )
		{	// source can only be an existing card.
			//   if dest is a ps2-port (empty or not) -> swap cards between ports.
			//   is dest is a non-ps2-port -> remove card from port.

			//   Note: For the sake of usability, automatically enable dest if a ps2-port.
			if (src.IsPresent)
			{
				wxFileName	tmpFilename = dest.Filename;
				bool		tmpPresent  = dest.IsPresent;
				if (src.Slot<0 && m_listview->GetMcdProvider().isFileAssignedToInternalSlot(src.Filename))
					m_listview->GetMcdProvider().RemoveCardFromSlot(src.Filename);

				dest.Filename  = src.Filename;
				dest.IsEnabled = dest.IsPresent? dest.IsEnabled:true;
				dest.IsPresent = src.IsPresent;

				if (dest.Slot>=0)
				{//2 internal slots: swap
					src.Filename  = tmpFilename;
					src.IsPresent = tmpPresent;
				}
				else
				{//dest is at the filesystem (= remove card from slot)
					src.Filename  = L"";
					src.IsPresent = false;
					src.IsEnabled = false;
				}
			}
		}

		return def;
	}
};


enum McdMenuId
{
	McdMenuId_Create = 0x888,
	//McdMenuId_Mount,
	McdMenuId_Rename,
	McdMenuId_RefreshList,
	McdMenuId_AssignUnassign,
	McdMenuId_Duplicate,
};


Panels::MemoryCardListPanel_Simple* g_uglyPanel=NULL;
void g_uglyFunc(){if (g_uglyPanel) g_uglyPanel->OnChangedListSelection();}

Panels::MemoryCardListPanel_Simple::~MemoryCardListPanel_Simple() throw(){g_uglyPanel=NULL;}

Panels::MemoryCardListPanel_Simple::MemoryCardListPanel_Simple( wxWindow* parent )
	: _parent( parent )
{
	m_MultitapEnabled[0] = false;
	m_MultitapEnabled[1] = false;

	m_listview = new MemoryCardListView_Simple(this);
	
	m_listview->SetMinSize(wxSize(600, m_listview->GetCharHeight() * 13)); // 740 is nice for default font sizes
	
	m_listview->SetDropTarget( new McdDropTarget(m_listview) );

	//m_button_Mount	= new wxButton(this, wxID_ANY, _("Enable port"));

	m_button_AssignUnassign = new wxButton(this, wxID_ANY, _("Eject"));
	m_button_Duplicate = new wxButton(this, wxID_ANY, _("Duplicate ..."));
	m_button_Rename = new wxButton(this, wxID_ANY, _("Rename ..."));
	m_button_Create	= new wxButton(this, wxID_ANY, _("Create ..."));

	// ------------------------------------
	//       Sizer / Layout Section
	// ------------------------------------

	CreateLayout();

	*s_leftside_buttons	+= 20;
	//*s_leftside_buttons	+= m_button_Mount;
	//*s_leftside_buttons	+= 20;

	*s_leftside_buttons += Label(_("Card: ")) | pxMiddle;
	*s_leftside_buttons += m_button_AssignUnassign;
	*s_leftside_buttons	+= 20;
	*s_leftside_buttons	+= m_button_Duplicate;
	*s_leftside_buttons	+= 2;
	*s_leftside_buttons	+= m_button_Rename;
	*s_leftside_buttons	+= 2;
	*s_leftside_buttons	+= m_button_Create;
	SetSizerAndFit(GetSizer());

	parent->SetWindowStyle(parent->GetWindowStyle() | wxRESIZE_BORDER);

	Connect( m_listview->GetId(),		wxEVT_COMMAND_LIST_BEGIN_DRAG,		wxListEventHandler(MemoryCardListPanel_Simple::OnListDrag));
	Connect( m_listview->GetId(),		wxEVT_COMMAND_LIST_ITEM_SELECTED,	wxListEventHandler(MemoryCardListPanel_Simple::OnListSelectionChanged));
	Connect( m_listview->GetId(),		wxEVT_COMMAND_LIST_ITEM_ACTIVATED,	wxListEventHandler(MemoryCardListPanel_Simple::OnItemActivated));//enter or double click

	//Deselected is not working for some reason (e.g. when clicking an empty row at the table?) - avih
	Connect( m_listview->GetId(),		wxEVT_COMMAND_LIST_ITEM_DESELECTED,	wxListEventHandler(MemoryCardListPanel_Simple::OnListSelectionChanged));

	Connect( m_listview->GetId(),		wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler(MemoryCardListPanel_Simple::OnOpenItemContextMenu) );

//	Connect( m_button_Mount->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(MemoryCardListPanel_Simple::OnMountCard));
	Connect( m_button_Create->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(MemoryCardListPanel_Simple::OnCreateOrDeleteCard));
	Connect( m_button_Rename->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(MemoryCardListPanel_Simple::OnRenameFile));
	Connect( m_button_Duplicate->GetId(),		wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(MemoryCardListPanel_Simple::OnDuplicateFile));
	Connect( m_button_AssignUnassign->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler(MemoryCardListPanel_Simple::OnAssignUnassignFile));

	// Popup Menu Connections!
	Connect( McdMenuId_Create,		wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MemoryCardListPanel_Simple::OnCreateOrDeleteCard) );
	//Connect( McdMenuId_Mount,		wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MemoryCardListPanel_Simple::OnMountCard) );
	Connect( McdMenuId_Rename,		wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MemoryCardListPanel_Simple::OnRenameFile) );
	Connect( McdMenuId_AssignUnassign,	wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MemoryCardListPanel_Simple::OnAssignUnassignFile) );
	Connect( McdMenuId_Duplicate,	wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MemoryCardListPanel_Simple::OnDuplicateFile) );
	
	Connect( McdMenuId_RefreshList,	wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MemoryCardListPanel_Simple::OnRefreshSelections) );

	//because the wxEVT_COMMAND_LIST_ITEM_DESELECTED doesn't work (buttons stay enabled when clicking an empty area of the list),
	//  m_listview can send us an event that indicates a change at the list. Ugly, but works.
	g_uglyPanel=this;
	m_listview->setExternHandler(g_uglyFunc);
}

void Panels::MemoryCardListPanel_Simple::UpdateUI()
{
	if( !m_listview ) return;

	int sel = m_listview->GetFirstSelected();

	if( wxNOT_FOUND == sel )
	{
		m_button_Create->Disable();
//		m_button_Mount->Disable();
		m_button_Rename->Disable();
		m_button_Duplicate->Disable();
		m_button_AssignUnassign->Disable();
		return;
	}

	const McdSlotItem& card( GetCardForViewIndex(sel) );


	m_button_Rename->Enable( card.IsPresent );
	wxString renameTip = _("Rename this memory card ...");
	pxSetToolTip( m_button_Rename, renameTip );

	m_button_AssignUnassign->Enable( card.IsPresent );
	m_button_AssignUnassign->SetLabel( card.Slot>=0 ? _("Eject") : _("Insert ...") );
	wxString assignTip = (card.Slot>=0)?_("Eject the card from this port"):_("Insert this card to a port ...");
	pxSetToolTip( m_button_AssignUnassign, assignTip );
	
	m_button_Duplicate->Enable( card.IsPresent );
	wxString dupTip = _("Create a duplicate of this memory card ...");
	pxSetToolTip( m_button_Duplicate, dupTip );

	m_button_Create->Enable(card.Slot>=0 || card.IsPresent);
	m_button_Create->SetLabel( card.IsPresent ? _("Delete") : _("Create ...") );
	wxString deleteTip = _("Permanently delete this memory card from disk (all contents are lost)");

    if (card.IsPresent)
        pxSetToolTip( m_button_Create, deleteTip);
    else
        pxSetToolTip( m_button_Create, _("Create a new memory card and assign it to the selected PS2-Port." ));

/*
	m_button_Mount->Enable( card.IsPresent && card.Slot>=0);
	m_button_Mount->SetLabel( card.IsEnabled ? _("Disable Port") : _("Enable Port") );
	pxSetToolTip( m_button_Mount,
		card.IsEnabled
			? _("Disable the selected PS2-Port (this memory card will be invisible to games/BIOS).")
			: _("Enable the selected PS2-Port (games/BIOS will see this memory card).")
	);
*/
}

void Panels::MemoryCardListPanel_Simple::Apply()
{
	_parent::Apply();

	ScopedCoreThreadClose closed_core;
	closed_core.AllowResume();

	for( uint slot=0; slot<8; ++slot )
	{
		g_Conf->Mcd[slot].Enabled = m_Cards[slot].IsEnabled && m_Cards[slot].IsPresent;
		if (m_Cards[slot].IsPresent)
			g_Conf->Mcd[slot].Filename = m_Cards[slot].Filename;
		else
			g_Conf->Mcd[slot].Filename = L"";
	}

	SetForceMcdEjectTimeoutNow();

}

void Panels::MemoryCardListPanel_Simple::AppStatusEvent_OnSettingsApplied()
{
	for( uint slot=0; slot<8; ++slot )
	{
		m_Cards[slot].IsEnabled = g_Conf->Mcd[slot].Enabled;
		m_Cards[slot].Filename = g_Conf->Mcd[slot].Filename;
		
		//automatically create the enabled but non-existing file such that it can be managed (else will get created anyway on boot)
		wxString targetFile = (GetMcdPath() + m_Cards[slot].Filename.GetFullName()).GetFullPath();
		if ( m_Cards[slot].IsEnabled && !wxFileExists( targetFile ) )
		{
			wxString errMsg;
			if (isValidNewFilename(m_Cards[slot].Filename.GetFullName(), GetMcdPath(), errMsg, 5))
			{
				if ( !Dialogs::CreateMemoryCardDialog::CreateIt(targetFile, 8) )
					Console.Error( L"Automatic createion of MCD '%s' failed. Hope for the best...", targetFile.c_str() );
				else
					Console.WriteLn( L"memcard created: '%s'.", targetFile.c_str() );
			}
			else
			{
				Console.Error( L"memcard was enabled but had an invalid file name. Aborting automatic creation. Hope for the best... (%s)", errMsg.c_str() );
			}
		}

		if ( !m_Cards[slot].IsEnabled || !wxFileExists( targetFile ) )
		{
			m_Cards[slot].IsEnabled = false;
			m_Cards[slot].IsPresent = false;
			m_Cards[slot].Filename = L"";
		}

	}
	DoRefresh();

	_parent::AppStatusEvent_OnSettingsApplied();
}


//BUG: the next function is never reached because, for some reason, IsoDropTarget::OnDropFiles is called instead.
//     Interestingly, IsoDropTarget::OnDropFiles actually "detects" a memory card file as a valid Audio-CD ISO...  - avih
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
		//if( FileMcd_IsMultitapSlot(slot) && !m_MultitapEnabled[FileMcd_GetMtapPort(slot)] )
		//	continue;

		//wxFileName fullpath( m_FolderPicker->GetPath() + g_Conf->Mcd[slot].Filename.GetFullName() );
		wxFileName fullpath = m_FolderPicker->GetPath() + m_Cards[slot].Filename.GetFullName();

		EnumerateMemoryCard( m_Cards[slot], fullpath, m_FolderPicker->GetPath());
		m_Cards[slot].Slot = slot;
	}

	ReadFilesAtMcdFolder();
	

	if( m_listview ) m_listview->SetMcdProvider( this );
	UpdateUI();
}


// =====================================================================================================
//  MemoryCardListPanel_Simple (implementations)
// =====================================================================================================

void Panels::MemoryCardListPanel_Simple::UiCreateNewCard( McdSlotItem& card )
{
	if( card.IsPresent ){
		Console.WriteLn("Error: Aborted: create mcd invoked but but a file is already associated.");
		return;
	}

	ScopedCoreThreadClose closed_core;

	Dialogs::CreateMemoryCardDialog dialog( this, m_FolderPicker->GetPath(), L"my memory card" );
	wxWindowID result = dialog.ShowModal();

	if (result != wxID_CANCEL)
	{
		card.IsEnabled = true;
		card.Filename  = dialog.result_createdMcdFilename;
		card.IsPresent = true;
		Console.WriteLn(L"setting new card to slot %u: '%s'", card.Slot, card.Filename.GetFullName().c_str());
	}
	else
		card.IsEnabled=false;

	Apply();
	RefreshSelections();
	closed_core.AllowResume();
}


void Panels::MemoryCardListPanel_Simple::UiDeleteCard( McdSlotItem& card )
{
	if( !card.IsPresent ){
		Console.WriteLn("Error: Aborted: delete mcd invoked but but a file is not associated.");
		return;
	}


	bool result = true;
	if( card.IsFormatted )
	{
		wxString content;
		content.Printf(
			pxE( "!Notice:Mcd:Delete",
				L"You are about to delete the formatted memory card '%s'. "
				L"All data on this card will be lost!  Are you absolutely and quite positively sure?"
				), card.Filename.GetFullName().c_str()
		);

		result = Msgbox::YesNo( content, _("Delete memory file?") );
	}

	if( result )
	{
		ScopedCoreThreadClose closed_core;
	
		wxFileName fullpath( m_FolderPicker->GetPath() + card.Filename.GetFullName());

		card.IsEnabled=false;
		Apply();
		wxRemoveFile( fullpath.GetFullPath() );

		RefreshSelections();
		closed_core.AllowResume();
	}

}

bool Panels::MemoryCardListPanel_Simple::UiDuplicateCard(McdSlotItem& src, McdSlotItem& dest)
{
		wxDirName basepath = GetMcdPath();
		if( !src.IsPresent )
		{
			Msgbox::Alert(_("Failed: Can only duplicate an existing card."), _("Duplicate memory card"));
			return false;
		}

		if ( dest.IsPresent && dest.Slot!=-1 )
		{
			wxString content;
			content.Printf(
				pxE( "!Notice:Mcd:CantDuplicate",
				L"Failed: Duplicate is only allowed to an empty PS2-Port or to the file system." )
			);

			Msgbox::Alert( content, _("Duplicate memory card") );
			return false;
		}

		while (1)
		{
			wxString newFilename=L"";
			newFilename = wxGetTextFromUser(_("Select a name for the duplicate\n( '.ps2' will be added automatically)"), _("Duplicate memory card"));
			if( newFilename==L"" )
			{
				//Msgbox::Alert( _("Duplicate canceled"), _("Duplicate memory card") );
				return false;
			}
			newFilename += L".ps2";

			//check that the name is valid for a new file
			wxString errMsg;
			if( !isValidNewFilename( newFilename, basepath, errMsg, 5 ) )
			{
				wxString message;
				message.Printf(_("Failed: %s"), errMsg.c_str());
				Msgbox::Alert( message, _("Duplicate memory card") );
				continue;
			}

			dest.Filename = newFilename;
			break;
		}


		wxFileName srcfile( basepath + src.Filename);
		wxFileName destfile( basepath + dest.Filename);

		ScopedBusyCursor doh( Cursor_ReallyBusy );
		ScopedCoreThreadClose closed_core;

		if( !wxCopyFile( srcfile.GetFullPath(), destfile.GetFullPath(),	true ) )
		{
			wxString heading;
			heading.Printf( pxE( "!Notice:Mcd:Copy Failed",
				L"Failed: Destination memory card '%s' is in use." ),
				dest.Filename.GetFullName().c_str(), dest.Slot
			);

			wxString content;

			Msgbox::Alert( heading + L"\n\n" + content, _("Copy failed!") );
			
			closed_core.AllowResume();
			return false;
		}

		// Destination memcard isEnabled state is the same now as the source's
		wxString success;
		success.Printf(_("Memory card '%s' duplicated to '%s'.\n\nBoth card files are now identical."),
			src.Filename.GetFullName().c_str(),
			dest.Filename.GetFullName().c_str()
			);
		Msgbox::Alert(success, _("Success"));
		dest.IsPresent=true;
		dest.IsEnabled = true;

		Apply();
		DoRefresh();
		closed_core.AllowResume();
		return true;
}

void Panels::MemoryCardListPanel_Simple::UiRenameCard( McdSlotItem& card )
{
	if( !card.IsPresent ){
		Console.WriteLn("Error: Aborted: Rename mcd invoked but no file is associated.");
		return;
	}

	const wxDirName basepath( m_listview->GetMcdProvider().GetMcdPath() );
	wxString newFilename;
	while (1){
		wxString title;
		title.Printf(_("Select a new name for the memory card '%s'\n( '.ps2' will be added automatically)"),
						card.Filename.GetFullName().c_str()
						);
		newFilename = wxGetTextFromUser(title, _("Rename memory card"));
		if( newFilename==L"" )
			return;

		newFilename += L".ps2";

		//check that the name is valid for a new file
		wxString errMsg;
		if( !isValidNewFilename( newFilename, basepath, errMsg, 5 ) )
		{
			wxString message;
			message.Printf(_("Error (%s)"), errMsg.c_str());
			Msgbox::Alert( message, _("Rename memory card") );
			continue;
		}

		break;
	}

	ScopedCoreThreadClose closed_core;

	bool origEnabled=card.IsEnabled;
	card.IsEnabled=false;
	Apply();
	if( !wxRenameFile( (basepath + card.Filename).GetFullPath(), (basepath + wxFileName(newFilename)).GetFullPath(), false ) )
	{
		card.IsEnabled=origEnabled;
		Apply();
		Msgbox::Alert( _("Error: Rename could not be completed.\n"), _("Rename memory card") );
	
		closed_core.AllowResume();
		return;
	}

	card.Filename = newFilename;
	card.IsEnabled=origEnabled;
	Apply();

	RefreshSelections();
	closed_core.AllowResume();
}

void Panels::MemoryCardListPanel_Simple::OnCreateOrDeleteCard(wxCommandEvent& evt)
{
	const int selectedViewIndex = m_listview->GetFirstSelected();
	if( wxNOT_FOUND == selectedViewIndex ) return;

	McdSlotItem& card( GetCardForViewIndex(selectedViewIndex) );

	if( card.IsPresent )
		UiDeleteCard( card );
	else
		UiCreateNewCard( card );
}

//enable/disapbe port
/*
void Panels::MemoryCardListPanel_Simple::OnMountCard(wxCommandEvent& evt)
{
	evt.Skip();

	const int selectedViewIndex = m_listview->GetFirstSelected();
	if( wxNOT_FOUND == selectedViewIndex ) return;

	McdSlotItem& card( GetCardForViewIndex(selectedViewIndex) );

	card.IsEnabled = !card.IsEnabled;
	m_listview->RefreshItem(selectedViewIndex);
	UpdateUI();
}
*/
/*
//text dialog: can be used for rename: wxGetTextFromUser - avih
void Panels::MemoryCardListPanel_Simple::OnRelocateCard(wxCommandEvent& evt)
{
	evt.Skip();

	const int slot = m_listview->GetFirstSelected();
	if( wxNOT_FOUND == slot ) return;

	// Issue a popup to the user that allows them to pick a new .PS2 file to serve as
	// the new host memorycard file for the slot.  The dialog has a number of warnings
	// present to reiterate that this is an advanced operation that PCSX2 may not
	// support very well (ie, might be buggy).

	m_listview->RefreshItem(slot);
	UpdateUI();
}
*/

// enter/double-click
void Panels::MemoryCardListPanel_Simple::OnItemActivated(wxListEvent& evt)
{
	const int viewIndex = m_listview->GetFirstSelected();
	if( wxNOT_FOUND == viewIndex ) return;
	McdSlotItem& card( GetCardForViewIndex(viewIndex) );

	if ( card.IsPresent )
		UiRenameCard( card );
	else if (card.Slot>=0)
		UiCreateNewCard( card );// IsPresent==false can only happen for an internal slot (vs filename on the HD), so a card can be created.
}

void Panels::MemoryCardListPanel_Simple::OnDuplicateFile(wxCommandEvent& evt)
{
	const int viewIndex = m_listview->GetFirstSelected();
	if( wxNOT_FOUND == viewIndex ) return;
	McdSlotItem& card( GetCardForViewIndex(viewIndex) );

	pxAssert( card.IsPresent );
	McdSlotItem dummy;
	UiDuplicateCard(card, dummy);
}

wxString GetPortName(int slotIndex)
{
	if (slotIndex==0 || slotIndex==1)
		return pxsFmt(wxString(L" ") + _("Port-%u / Multitap-%u--Port-1"), FileMcd_GetMtapPort(slotIndex)+1, FileMcd_GetMtapPort(slotIndex)+1);
	return pxsFmt(wxString(L" ")+_("             Multitap-%u--Port-%u"), FileMcd_GetMtapPort(slotIndex)+1, FileMcd_GetMtapSlot(slotIndex)+1 );
}

void Panels::MemoryCardListPanel_Simple::UiAssignUnassignFile(McdSlotItem &card)
{
	pxAssert( card.IsPresent );

	if( card.Slot >=0 )
	{//eject
		card.IsEnabled = false;
		card.IsPresent = false;
		card.Filename  = L"";
		DoRefresh();
	}
	else
	{//insert into a (UI) selected slot
		wxArrayString selections;
		int i;
		for (i=0; i< GetNumVisibleInternalSlots(); i++)
		{
			McdSlotItem& selCard = GetCardForViewIndex(i);
			wxString sel = GetPortName( selCard.Slot ) + L"   ( ";
			if (selCard.IsPresent)
				sel += selCard.Filename.GetFullName();
			else
				sel += _("Empty");
			sel += L" )";

			selections.Add(sel);
		}
		wxString title;
		title.Printf(_("Select a target port for '%s'"), card.Filename.GetFullName().c_str());
		int res=wxGetSingleChoiceIndex(title, _("Insert card"), selections, this);
		if( res<0 )
			return;

		McdSlotItem& target = GetCardForViewIndex(res);
		bool en = target.IsPresent? target.IsEnabled : true;
		RemoveCardFromSlot( card.Filename );
		target.Filename = card.Filename;
		target.IsPresent  = true;
		target.IsEnabled  = en;
		DoRefresh();
	}
}
void Panels::MemoryCardListPanel_Simple::OnAssignUnassignFile(wxCommandEvent& evt)
{
	const int viewIndex = m_listview->GetFirstSelected();
	if( wxNOT_FOUND == viewIndex ) return;
	McdSlotItem& card( GetCardForViewIndex(viewIndex) );

	UiAssignUnassignFile(card);
}

void Panels::MemoryCardListPanel_Simple::OnRenameFile(wxCommandEvent& evt)
{
	const int viewIndex = m_listview->GetFirstSelected();
	if( wxNOT_FOUND == viewIndex ) return;
	McdSlotItem& card( GetCardForViewIndex(viewIndex) );

	UiRenameCard( card );
}


void Panels::MemoryCardListPanel_Simple::OnListDrag(wxListEvent& evt)
{
	int selectionViewIndex = m_listview->GetFirstSelected();

	if( selectionViewIndex < 0 ) return;
	McdDataObject my_data( selectionViewIndex );

	wxDropSource dragSource( m_listview );
	dragSource.SetData( my_data );
	/*wxDragResult result = */dragSource.DoDragDrop( wxDrag_AllowMove );
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
		const McdSlotItem& card( GetCardForViewIndex(idx) );

		if (card.IsPresent){
			/*
			if (card.Slot>=0)
			{
				junk->Append( McdMenuId_Mount,		card.IsEnabled ? _("Disable Port")	: _("Enable Port") );
				junk->AppendSeparator();
			}
			*/
			junk->Append( McdMenuId_AssignUnassign,	card.Slot>=0?_("Eject card"):_("Insert card ...") );
			junk->Append( McdMenuId_Duplicate,	_("Duplicate card ...") );
			junk->Append( McdMenuId_Rename,		_("Rename card ...") );
		}

		if ( card.IsPresent || card.Slot>=0 )
		{
			junk->Append( McdMenuId_Create,		card.IsPresent ? _("Delete card")	: _("Create a new card ...") );
			junk->AppendSeparator();
		}
	}

	junk->Append( McdMenuId_RefreshList, _("Refresh List") );

	PopupMenu( junk );
	m_listview->RefreshItem( idx );
	UpdateUI();
}

void Panels::MemoryCardListPanel_Simple::ReadFilesAtMcdFolder(){
	//Dir enumeration/iteration code courtesy of cotton. - avih.
	while( m_allFilesystemCards.size() )
		m_allFilesystemCards.pop_back();

	m_filesystemPlaceholderCard.Slot=-1;
	m_filesystemPlaceholderCard.IsEnabled=false;
	m_filesystemPlaceholderCard.IsPresent=false;
	m_filesystemPlaceholderCard.Filename=L"";


	wxArrayString memcardList;
	wxDir::GetAllFiles(m_FolderPicker->GetPath().ToString(), &memcardList, L"*.ps2", wxDIR_FILES);

	for(uint i = 0; i < memcardList.size(); i++) {
		McdSlotItem currentCardFile;
		bool isOk=EnumerateMemoryCard( currentCardFile, memcardList[i], m_FolderPicker->GetPath() );
		if( isOk && !isFileAssignedAndVisibleOnList( currentCardFile.Filename ) )
		{
			currentCardFile.Slot		= -1;
			currentCardFile.IsEnabled	= false;
			m_allFilesystemCards.push_back(currentCardFile);
			DevCon.WriteLn(L"Enumerated file: '%s'", currentCardFile.Filename.GetFullName().c_str() );
		}
		else
			DevCon.WriteLn(L"MCD folder card file skipped: '%s'", memcardList[i].c_str() );
	}
}


bool Panels::MemoryCardListPanel_Simple::IsSlotVisible(int slotIndex) const
{
	if ( slotIndex<0 || slotIndex>=8 ) return false;
	if ( !m_MultitapEnabled[0] && 2<=slotIndex && slotIndex<=4) return false;
	if ( !m_MultitapEnabled[1] && 5<=slotIndex && slotIndex<=7) return false;
	return true;
}
//whether or not this filename appears on the ports at the list (takes into account MT enabled/disabled)
bool Panels::MemoryCardListPanel_Simple::isFileAssignedAndVisibleOnList(const wxFileName cardFile) const
{
	int i;
	for( i=0; i<8; i++)
		if ( IsSlotVisible(i) && cardFile.GetFullName()==m_Cards[i].Filename.GetFullName() )
			return true;

	return false;
}

//whether or not this filename is assigned to a ports (regardless if MT enabled/disabled)
bool Panels::MemoryCardListPanel_Simple::isFileAssignedToInternalSlot(const wxFileName cardFile) const
{
	int i;
	for( i=0; i<8; i++)
		if ( cardFile.GetFullName()==m_Cards[i].Filename.GetFullName() )
			return true;

	return false;
}

void Panels::MemoryCardListPanel_Simple::RemoveCardFromSlot(const wxFileName cardFile)
{
	int i;
	for( i=0; i<8; i++)
		if ( cardFile.GetFullName()==m_Cards[i].Filename.GetFullName() )
		{
			m_Cards[i].Filename  = L"";
			m_Cards[i].IsPresent = false;
			m_Cards[i].IsEnabled = false;
		}
}

int Panels::MemoryCardListPanel_Simple::GetNumFilesVisibleAsFilesystem() const
{
	return m_allFilesystemCards.size();
}


bool Panels::MemoryCardListPanel_Simple::IsNonEmptyFilesystemCards() const
{
	return GetNumFilesVisibleAsFilesystem()>0;
}

McdSlotItem& Panels::MemoryCardListPanel_Simple::GetNthVisibleFilesystemCard(int n)
{
	return m_allFilesystemCards.at(n);
}

int Panels::MemoryCardListPanel_Simple::GetNumVisibleInternalSlots() const
{
	uint baselen = 2;
	if( m_MultitapEnabled[0] ) baselen += 3;
	if( m_MultitapEnabled[1] ) baselen += 3;
	return baselen;
}

// Interface Implementation for IMcdList
int Panels::MemoryCardListPanel_Simple::GetLength() const
{
	uint baselen = GetNumVisibleInternalSlots();
	baselen++;//filesystem placeholder
	baselen+= GetNumFilesVisibleAsFilesystem();
	return baselen;
}


//Translates a list-view index (idx) to an internal memory card slot.
//This method effectively defines the arrangement of the card slots at the list view.
//The internal card slots array is fixed as sollows:
// slot 0: mcd1 (= MT1 slot 1)
// slot 1: mcd2 (= MT2 slot 1)
// slots 2,3,4: MT1 slots 2,3,4
// slots 5,6,7: MT2 slots 2,3,4
//
//however, the list-view doesn't show MT slots when this MT is disabled,
//  so the view-index should "shift" to point at the real card slot.
//While we're at it, we can alternatively enforce any other arrangment of the view by
//  using any other set of 'view-index-to-card-slot' translating that we'd like.
int Panels::MemoryCardListPanel_Simple::GetSlotIndexForViewIndex( int listViewIndex )
{
	pxAssert( 0<=listViewIndex && listViewIndex<GetNumVisibleInternalSlots() );
	int targetSlot=-1;

/*
	//this list-view arrangement is mostly kept aligned with the internal slots indexes, and only takes care
	//  of the case where MT1 is disabled (hence the MT2 slots 2,3,4 "move backwards" 3 places on the view-index)
	//  However, this arrangement it's not very intuitive to use...
	if (!m_MultitapEnabled[0] && listViewIndex>=2)
	{
		//we got an MT2 slot.
		assert(listViewIndex < 5);
		targetSlot = listViewIndex+3;
	}
	else
	{
		targetSlot=listViewIndex;//identical view-index and card slot.
	}
*/

	//This arrangement of list-view is as follows:
	//mcd1(=MT1 port 1)
	//[MT1 port 2,3,4 if MT1 is enabled]
	//mcd2(=MT2 slot 1)
	//[MT2 port 2,3,4 if MT2 is enabled]

	if (m_MultitapEnabled[0]){
		//MT1 enabled:
		if (1<=listViewIndex && listViewIndex<=3){//MT1 ports 2/3/4 move one place backwards
			targetSlot=listViewIndex+1;
		}else if (listViewIndex==4){//mcd2 (=MT2 port 1) moves 3 places forward
			targetSlot=1;
		} else {//mcd1 keeps it's pos as first, MT2 ports keep their pos at the end of the list.
			targetSlot=listViewIndex;
		}
	} else {
		//MT1 disabled: mcd1 and mcd2 stay put, MT2 ports 2,3,4 come next (move backwards 3 places)
		if (2<=listViewIndex && listViewIndex<=4)
			targetSlot=listViewIndex+3;
		else
			targetSlot=listViewIndex;
	}

	pxAssert( 0<=targetSlot && targetSlot<=7 );
	return targetSlot;
}


McdSlotItem& Panels::MemoryCardListPanel_Simple::GetCardForViewIndex( int idx )
{
	pxAssert( -1<=idx && idx< GetNumVisibleInternalSlots()+1+GetNumFilesVisibleAsFilesystem() );

	if( 0<=idx && idx<GetNumVisibleInternalSlots() )
		return m_Cards[GetSlotIndexForViewIndex( idx )];

	if( idx==-1 || idx == GetNumVisibleInternalSlots() )
		return this->m_filesystemPlaceholderCard;

	return this->m_allFilesystemCards.at( idx - GetNumVisibleInternalSlots() - 1 );
}
