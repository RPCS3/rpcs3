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
#include "ConfigurationDialog.h"

#include <wx/filepicker.h>
#include <wx/ffile.h>

using namespace pxSizerFlags;

// defined in MemoryCardsPanel.cpp
extern wxFilePickerCtrl* CreateMemoryCardFilePicker( wxWindow* parent, uint portidx, uint slotidx, const wxString& filename=wxEmptyString );

Dialogs::CreateMemoryCardDialog::CreateMemoryCardDialog( wxWindow* parent, uint port, uint slot, const wxString& filepath )
	: BaseApplicableDialog( parent, _("Create a new MemoryCard..."), wxVERTICAL )
{
	m_idealWidth = 620;

	wxBoxSizer& s_padding( *new wxBoxSizer(wxVERTICAL) );
	*this += s_padding	| StdExpand();
	
	#ifdef __WXMSW__
	m_check_CompressNTFS = new pxCheckBox( this,
		_("Use NTFS compression on this card"),
		pxE( ".Dialog:Memorycards:NtfsCompress",
			L"NTFS compression is built-in, fast, and completely reliable.  Memorycards typically compress "
			L"very well, and run faster as a result, so this option is highly recommended."
		)
	);
	#endif

	pxE( ".Dialog:Memorycards:NeedsFormatting",
		L"Your new MemoryCard needs to be formatted.  Some games can format the card for you, while "
		L"others may require you do so using the BIOS.  To boot into the BIOS, select the NoDisc option "
		L"as your CDVD Source."
	);

	const RadioPanelItem tbl_CardSizes[] =
	{
		RadioPanelItem(_("8 MB [most compatible]"))
		.	SetToolTip(_("8 meg carts are 'small' but are pretty well sure to work for any and all games.")),

		RadioPanelItem(_("16 MB"))
		.	SetToolTip(_("16 and 32 MB cards have roughly the same compatibility factor.  Most games see them fine, others may not.")),

		RadioPanelItem(_("32 MB"))
		.	SetToolTip(_("16 and 32 MB cards have roughly the same compatibility factor.  Most games see them fine, others may not.")),

		RadioPanelItem(_("64 MB"), _("Low compatibility! Use at your own risk."))
		.	SetToolTip(_("Yes it's very big.  Unfortunately a lot of games don't really work with them properly."))
	};

	m_radio_CardSize = &(new pxRadioPanel( this, tbl_CardSizes ))->SetDefaultItem(0);

	m_filepicker = CreateMemoryCardFilePicker( this, port, slot, filepath );

	// ----------------------------
	//      Sizers and Layout
	// ----------------------------

	s_padding += m_filepicker		| StdExpand();
	s_padding += m_radio_CardSize	| StdExpand();

	#ifdef __WXMSW__
	s_padding += m_check_CompressNTFS;
	#endif
}