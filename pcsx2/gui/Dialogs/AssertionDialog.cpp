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
#include "App.h"
#include "Dialogs/ModalPopups.h"

using namespace pxSizerFlags;

Dialogs::AssertionDialog::AssertionDialog( const wxString& text, const wxString& stacktrace )
	: wxDialogWithHelpers( NULL, _("PCSX2 Assertion Failure"), false, !stacktrace.IsEmpty() )
{
	m_idealWidth = 720;

	wxFlexGridSizer* flexgrid = new wxFlexGridSizer( 1 );
	flexgrid->AddGrowableCol( 0 );
	SetSizer( flexgrid );

	wxTextCtrl* traceArea = NULL;

	if( !stacktrace.IsEmpty() )
	{
		flexgrid->AddGrowableRow( 1 );

		traceArea = new wxTextCtrl(
			this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxTE_READONLY | wxTE_MULTILINE | wxTE_RICH2 | wxHSCROLL
		);

		traceArea->SetDefaultStyle( wxTextAttr( wxNullColour, wxNullColour, pxGetFixedFont() ) );
		traceArea->SetFont( pxGetFixedFont() );

		int fonty;
		traceArea->GetTextExtent( L"blaH yeah", NULL, &fonty );

		traceArea->WriteText( stacktrace );
		traceArea->SetMinSize( wxSize( GetIdealWidth()-24, (fonty+1)*18 ) );
		traceArea->ShowPosition(0);
	}

	*this += Heading( text );

	if( traceArea != NULL ) *this += traceArea | pxExpand.Border(wxTOP|wxLEFT|wxRIGHT,8);

	*this += Heading(
		L"\nDo you want to stop the program [Yes/No]?"
		L"\nOr press [Ignore] to suppress further assertions."
	);

	*this += new ModalButtonPanel( this, MsgButtons().YesNo().Ignore() ) | StdCenter();

	if( wxWindow* idyes = FindWindowById( wxID_YES ) )
		idyes->SetFocus();
}

