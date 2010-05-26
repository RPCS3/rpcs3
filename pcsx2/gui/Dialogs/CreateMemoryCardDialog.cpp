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
#include "ConfigurationDialog.h"
#include "System.h"

#include "MemoryCardFile.h"

#include <wx/filepicker.h>
#include <wx/ffile.h>

using namespace pxSizerFlags;

extern wxString GetMsg_McdNtfsCompress();

wxFilePickerCtrl* CreateMemoryCardFilePicker( wxWindow* parent, uint portidx, uint slotidx, const wxString& filename=wxEmptyString )
{
	return new wxFilePickerCtrl( parent, wxID_ANY, filename,
		wxsFormat(_("Select memorycard for Port %u / Slot %u"), portidx+1, slotidx+1),	// picker window title
		L"*.ps2",	// default wildcard
		wxDefaultPosition, wxDefaultSize,
		wxFLP_DEFAULT_STYLE & ~wxFLP_FILE_MUST_EXIST
	);

}

	/*pxSetToolTip( m_button_Recreate, pxE( ".Tooltip:MemoryCard:Recreate",
		L"Deletes the existing memory card and creates a new one.  All existing card contents will be lost."
	) );*/

Dialogs::CreateMemoryCardDialog::CreateMemoryCardDialog( wxWindow* parent, uint slot, const wxDirName& mcdpath, const wxString& mcdfile )
	: wxDialogWithHelpers( parent, _("Create new MemoryCard"), wxVERTICAL )
	, m_mcdpath( mcdpath.IsOk() ? mcdpath : (wxDirName)g_Conf->Mcd[slot].Filename.GetPath() )
	, m_mcdfile( mcdfile.IsEmpty() ? g_Conf->Mcd[slot].Filename.GetFullName() : mcdfile )
{
	m_idealWidth	= 472;
	m_filepicker	= NULL;
	m_slot = slot;

	CreateControls();

	//m_filepicker = CreateMemoryCardFilePicker( this, m_port, m_slot, filepath );

	// ----------------------------
	//      Sizers and Layout
	// ----------------------------

	if( m_radio_CardSize ) m_radio_CardSize->Realize();

	wxBoxSizer& s_buttons( *new wxBoxSizer(wxHORIZONTAL) );
	s_buttons += new wxButton( this, wxID_OK, _("Create") )	| pxProportion(2);
	s_buttons += pxStretchSpacer(3);
	s_buttons += new wxButton( this, wxID_CANCEL )			| pxProportion(2);

	wxBoxSizer& s_padding( *new wxBoxSizer(wxVERTICAL) );

	//s_padding += Heading(_("Select the size for your new MemoryCard."));

	if( m_filepicker )
		s_padding += m_filepicker			| StdExpand();
	else
	{
		s_padding += Heading( _( "New card will be saved to:" ) );
		s_padding += Heading( (m_mcdpath + m_mcdfile).GetFullPath() );
	}
	
	s_padding += m_radio_CardSize			| StdExpand();
	#ifdef __WXMSW__
	if( m_check_CompressNTFS )
		s_padding += m_check_CompressNTFS	| StdExpand();
	#endif
	
	s_padding += 12;
	s_padding += s_buttons	| StdCenter();

	*this += s_padding | StdExpand();

	Connect( wxID_OK,		wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CreateMemoryCardDialog::OnOk_Click ) );
	//Connect( wxID_APPLY,	wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CreateMemoryCardDialog::OnApply_Click ) );
}

wxDirName Dialogs::CreateMemoryCardDialog::GetPathToMcds() const
{
	return m_filepicker ? (wxDirName)m_filepicker->GetPath() : m_mcdpath;
}

// When this GUI is moved itno the FileMemoryCard plugin (where it eventually belongs),
// this function will be removed and the MemoryCardFile::Create() function will be used
// instead.
static bool CreateIt( const wxString& mcdFile, uint sizeInMB )
{
	//int enc[16] = {0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0,0,0,0};

	u8	m_effeffs[528*16];
	memset8<0xff>( m_effeffs );

	Console.WriteLn( L"(FileMcd) Creating new %uMB MemoryCard: " + mcdFile, sizeInMB );

	wxFFile fp( mcdFile, L"wb" );
	if( !fp.IsOpened() ) return false;

	static const int MC2_MBSIZE	= 1024 * 528 * 2;		// Size of a single megabyte of card data

	for( uint i=0; i<(MC2_MBSIZE*sizeInMB)/sizeof(m_effeffs); i++ )
	{
		if( fp.Write( m_effeffs, sizeof(m_effeffs) ) == 0 )
			return false;
	}
	return true;
}

void Dialogs::CreateMemoryCardDialog::OnOk_Click( wxCommandEvent& evt )
{
	if( !CreateIt(
		m_filepicker		? m_filepicker->GetPath()					: (m_mcdpath + m_mcdfile).GetFullPath(),
		m_radio_CardSize	? m_radio_CardSize->SelectedItem().SomeInt	: 8
	) )
	{
		Msgbox::Alert(
			_("Error: The MemoryCard could not be created."),
			_("MemoryCard creation error")
		);
		return;
	}
	EndModal( wxID_OK );
}

void Dialogs::CreateMemoryCardDialog::CreateControls()
{
	#ifdef __WXMSW__
	m_check_CompressNTFS = new pxCheckBox( this,
		_("Use NTFS compression on this card"),
		GetMsg_McdNtfsCompress()
	);
	#endif

	const RadioPanelItem tbl_CardSizes[] =
	{
		RadioPanelItem(_("8 MB [most compatible]"), _("This is the standard Sony-provisioned size, and is supported by all games and BIOS versions."))
		.	SetToolTip(_("Always use this option if you want the safest and surest MemoryCard behavior."))
		.	SetInt(8),

		RadioPanelItem(_("16 MB"), _("A typical size for 3rd-party MemoryCards which should work with most games."))
		.	SetToolTip(_("16 and 32 MB cards have roughly the same compatibility factor."))
		.	SetInt(16),

		RadioPanelItem(_("32 MB"), _("A typical size for 3rd-party MemoryCards which should work with most games."))
		.	SetToolTip(_("16 and 32 MB cards have roughly the same compatibility factor."))
		.	SetInt(32),

		RadioPanelItem(_("64 MB"), _("Low compatibility warning: Yes it's very big, but may not work with many games."))
		.	SetToolTip(_("Use at your own risk.  Erratic memorycard behavior is possible (though unlikely)."))
		.	SetInt(64)
	};

	m_radio_CardSize = new pxRadioPanel( this, tbl_CardSizes );
	m_radio_CardSize->SetDefaultItem(0);
}

