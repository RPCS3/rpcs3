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
#include "pxCheckBox.h"
#include "pxStaticText.h"

// --------------------------------------------------------------------------------------
//  pxCheckBox Implementations
// --------------------------------------------------------------------------------------

pxCheckBox::pxCheckBox(wxWindow* parent, const wxString& label, const wxString& subtext)
	: wxPanelWithHelpers( parent, wxVERTICAL )
{
	Init( label, subtext );
}

void pxCheckBox::Init(const wxString& label, const wxString& subtext)
{
	m_subtext	= NULL;
	m_checkbox	= new wxCheckBox( this, wxID_ANY, label );
	
	*this += m_checkbox | pxSizerFlags::StdExpand();

	static const int Indentation = 23;
	if( !subtext.IsEmpty() )
	{
		m_subtext = new pxStaticText( this, subtext );
		if( HasIdealWidth() )
			m_subtext->SetWrapWidth( m_idealWidth - Indentation );

		wxBoxSizer& spaced( *new wxBoxSizer( wxHORIZONTAL ) );
		spaced += Indentation;
		spaced += m_subtext | wxSF.Border( wxBOTTOM, 9 );
		spaced += pxSizerFlags::StdPadding;

		*this += &spaced;
	}
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
	m_checkbox->SetValue( val );
	return *this;
}

bool pxCheckBox::GetValue() const
{
	return m_checkbox->GetValue();
}

void operator+=( wxSizer& target, pxCheckBox* src )
{
	if( !pxAssert( src != NULL ) ) return;
	target.Add( src, wxSF.Expand() );
}
