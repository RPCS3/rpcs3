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
#include "pxCheckBox.h"
#include "pxStaticText.h"

using namespace pxSizerFlags;

// --------------------------------------------------------------------------------------
//  pxCheckBox Implementations
// --------------------------------------------------------------------------------------

pxCheckBox::pxCheckBox(wxWindow* parent, const wxString& label, const wxString& subtext, int flags)
	: wxPanelWithHelpers( parent, wxVERTICAL )
{
	Init( label, subtext, flags );
}

pxCheckBox::pxCheckBox(wxWindow* parent, const wxString& label, int flags)
	: wxPanelWithHelpers( parent, wxVERTICAL )
{
	Init( label, wxEmptyString, flags );
}

void pxCheckBox::Init(const wxString& label, const wxString& subtext, int flags)
{
	m_subtext	= NULL;
	m_subPadding= StdPadding*2;
	m_checkbox	= new wxCheckBox( this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, flags );

	*this += m_checkbox | pxSizerFlags::StdExpand();

	static const int Indentation = 23;
	if( !subtext.IsEmpty() )
	{
		m_subtext = new pxStaticText( this, subtext, wxALIGN_LEFT );

		wxFlexGridSizer& spaced( *new wxFlexGridSizer(3) );
		spaced.AddGrowableCol( 1 );
		spaced += Indentation;
		m_sizerItem_subtext = spaced.Add( m_subtext, pxBorder( wxBOTTOM, m_subPadding ).Expand() );
		//spaced += pxSizerFlags::StdPadding;

		*this += &spaced | pxExpand;
	}
	
	Connect( m_checkbox->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(pxCheckBox::OnCheckpartCommand) );
}

pxCheckBox& pxCheckBox::SetSubPadding( int pad )
{
	m_subPadding = pad;
	if( m_sizerItem_subtext )
	{
		m_sizerItem_subtext->SetBorder( m_subPadding );
		Fit();
	}
	return *this;
}

// applies the tooltip to both both the checkbox and it's static subtext (if present), and
// performs word wrapping on platforms that need it (eg mswindows).
pxCheckBox& pxCheckBox::SetToolTip( const wxString& tip )
{
	const wxString wrapped( pxFormatToolTipText(this, tip) );
	pxSetToolTip( m_checkbox, wrapped );
	pxSetToolTip( m_subtext, wrapped );
	return *this;
}

pxCheckBox& pxCheckBox::SetValue( bool val )
{
	pxAssume( m_checkbox );
	m_checkbox->SetValue( val );
	return *this;
}

pxCheckBox& pxCheckBox::SetIndeterminate()
{
	pxAssume( m_checkbox );
	m_checkbox->Set3StateValue( wxCHK_UNDETERMINED );
	return *this;
}


pxCheckBox& pxCheckBox::SetState( wxCheckBoxState state )
{
	pxAssume( m_checkbox );
	m_checkbox->Set3StateValue( state );
	return *this;
}

// Forwards checkbox actions on the internal checkbox (called 'checkpart') to listeners
// bound to the pxCheckBox "parent" panel. This helps the pxCheckBox behave more like a
// traditional checkbox.
void pxCheckBox::OnCheckpartCommand( wxCommandEvent& evt )
{
	evt.Skip();
	
	wxCommandEvent newevt( evt );
	newevt.SetEventObject( this );
	newevt.SetId( GetId() );
	GetEventHandler()->ProcessEvent( newevt );
}

void pxCheckBox::OnSubtextClicked( wxCommandEvent& evt )
{
	// TODO?
	// We can enable the ability to allow clicks on the subtext desc/label to toggle
	// the checkmark.  Not sure if that's desirable.
}

void operator+=( wxSizer& target, pxCheckBox* src )
{
	if( src ) target.Add( src, pxExpand );
}

void operator+=( wxSizer& target, pxCheckBox& src )
{
	target.Add( &src, pxExpand );
}

void operator+=( wxSizer* target, pxCheckBox& src )
{
	target->Add( &src, pxExpand );
}
