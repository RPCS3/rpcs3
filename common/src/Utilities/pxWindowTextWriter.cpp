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
#include "wxGuiTools.h"

// --------------------------------------------------------------------------------------
//  pxWindowTextWriter  Implementations
// --------------------------------------------------------------------------------------
void pxWindowTextWriter::OnFontChanged()
{
}

pxWindowTextWriter& pxWindowTextWriter::SetWeight( int weight )
{
	wxFont curfont( m_dc.GetFont() );
	curfont.SetWeight( weight );
	m_dc.SetFont( curfont );

	return *this;
}

pxWindowTextWriter& pxWindowTextWriter::SetStyle( int style )
{
	wxFont curfont( m_dc.GetFont() );
	curfont.SetStyle( style );
	m_dc.SetFont( curfont );
	return *this;
}

pxWindowTextWriter& pxWindowTextWriter::Normal()
{
	wxFont curfont( m_dc.GetFont() );
	curfont.SetStyle( wxNORMAL );
	curfont.SetWeight( wxNORMAL );
	m_dc.SetFont( curfont );

	return *this;
}

pxWindowTextWriter& pxWindowTextWriter::SetPos( const wxPoint& pos )
{
	m_curpos = pos;
	return *this;
}

pxWindowTextWriter& pxWindowTextWriter::MovePos( const wxSize& delta )
{
	m_curpos += delta;
	return *this;
}

pxWindowTextWriter& pxWindowTextWriter::SetY( int ypos )
{
	m_curpos.y = ypos;
	return *this;
}

pxWindowTextWriter& pxWindowTextWriter::MoveY( int ydelta )
{
	m_curpos.y += ydelta;
	return *this;
}

void pxWindowTextWriter::_DoWriteLn( const wxChar* msg )
{
	pxAssume( msg );
	int	tWidth, tHeight;
	m_dc.GetTextExtent( msg, &tWidth, &tHeight );

	wxPoint dispos( m_curpos );

	if( m_align & wxALIGN_CENTER_HORIZONTAL )
	{
		dispos.x = (m_dc.GetSize().GetWidth() - tWidth) / 2;
	}
	else if( m_align & wxALIGN_RIGHT )
	{
		dispos.x = m_dc.GetSize().GetWidth() - tWidth;
	}

	m_dc.DrawText( msg, dispos );
	m_curpos.y += tHeight + m_leading;
}

// Splits incoming multi-line strings into pieces, and dispatches each line individually
// to the text writer.
void pxWindowTextWriter::_DoWrite( const wxChar* msg )
{
	pxAssume( msg );

	wxArrayString parts;
	SplitString( parts, msg, L'\n' );

	for( size_t i=0; i<parts.GetCount(); ++i )
		_DoWriteLn( parts[i] );
}

pxWindowTextWriter& pxWindowTextWriter::SetFont( const wxFont& font )
{
	m_dc.SetFont( font );
	return *this;
}

pxWindowTextWriter& pxWindowTextWriter::Align( const wxAlignment& align )
{
	m_align = align;
	return *this;
}

pxWindowTextWriter& pxWindowTextWriter::WriteLn()
{
	_DoWriteLn( L"" );
	return *this;
}

pxWindowTextWriter& pxWindowTextWriter::WriteLn( const wxChar* fmt, ... )
{
	va_list args;
	va_start(args,fmt);
	_DoWrite( FastFormatString_Unicode(fmt, args) );
	va_end(args);	
	return *this;
}

// --------------------------------------------------------------------------------------
//  pxStaticTextImproved  (implementations)
// --------------------------------------------------------------------------------------
pxStaticTextImproved::pxStaticTextImproved( wxWindow* parent, const wxString& label, wxAlignment align )
	: wxPanelWithHelpers( parent )
{
	Init();

	m_align			= align;
	SetLabel( label );
}

pxStaticTextImproved::pxStaticTextImproved( wxWindow* parent, int heightInLines, const wxString& label, wxAlignment align )
	: wxPanelWithHelpers( parent )
{
	SetMinSize( wxSize( wxDefaultCoord, heightInLines*16 ) );
	Init();

	m_align			= align;
	SetLabel( label );
}

void pxStaticTextImproved::Init()
{
	m_autowrap		= true;
	m_wrappedWidth	= -1;
	m_padding_horiz	= 8;

	Connect( wxEVT_PAINT, wxPaintEventHandler(pxStaticTextImproved::paintEvent) );
}

pxStaticTextImproved& pxStaticTextImproved::Unwrapped()
{
	m_autowrap = false;
	UpdateWrapping( false );
	return *this;
}

void pxStaticTextImproved::UpdateWrapping( bool textChanged )
{
	if( !m_autowrap )
	{
		m_wrappedLabel = wxEmptyString;
		m_wrappedWidth = -1;
		return;
	}

	wxString wrappedLabel;
	const int newWidth( GetSize().GetWidth() );

	if( !textChanged && (newWidth == m_wrappedWidth) ) return;

	// Note: during various stages of sizer-calc, width can be 1, 0, or -1.
	// We ignore wrapping in these cases.  (the PaintEvent also checks the wrapping
	// and updates it if needed, in case the control's size isn't figured out prior
	// to being painted).
	
	m_wrappedWidth = newWidth;
	if( m_wrappedWidth > 1 )
	{
		wxString label( GetLabel() );
		wrappedLabel = pxTextWrapper().Wrap( this, label, m_wrappedWidth-(m_padding_horiz*2) ).GetResult();
	}

	if( m_wrappedLabel == wrappedLabel ) return;
	m_wrappedLabel = wrappedLabel;
	Refresh();
}

void pxStaticTextImproved::SetLabel(const wxString& label)
{
	const bool labelChanged( label != GetLabel() );
	if( labelChanged )
	{
		_parent::SetLabel( label );
		Refresh();
	}

	// Always update wrapping, in case window width or something else also changed.
	UpdateWrapping( labelChanged );
}

void pxStaticTextImproved::paintEvent(wxPaintEvent& evt)
{
	wxPaintDC dc( this );
	const int dcWidth( dc.GetSize().GetWidth() );
	if( dcWidth < 1 ) return;

	dc.SetFont( GetFont() );
	pxWindowTextWriter writer( dc );
	wxString label;

	if( m_autowrap )
	{
		if( m_wrappedLabel.IsEmpty() || m_wrappedWidth != dcWidth )
		{
			const wxString original( GetLabel() );
			if( original.IsEmpty() ) return;
			m_wrappedLabel = pxTextWrapper().Wrap( this, original, dcWidth-(m_padding_horiz*2) ).GetResult();
		}
		label = m_wrappedLabel;
	}
	else
	{
		label = GetLabel();
	}

	int tWidth, tHeight;
	GetTextExtent( label, &tWidth, &tHeight );
	
	writer.Align( m_align );
	if( m_align & wxALIGN_CENTER_VERTICAL )
		writer.SetY( (dc.GetSize().GetHeight() - tHeight) / 2 );

	writer.WriteLn( label );
}
