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
pxWindowTextWriter::pxWindowTextWriter( wxDC& dc )
	: m_dc( dc )
{
	m_curpos = wxPoint();
	m_align = wxALIGN_CENTER;
	m_leading = 0;

	OnFontChanged();
}

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
	m_dc.GetMultiLineTextExtent( msg, &tWidth, &tHeight );

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
	m_align		= align;
	m_curpos.x	= 0;
	return *this;
}

pxWindowTextWriter& pxWindowTextWriter::WriteLn()
{
	_DoWriteLn( L"" );
	return *this;
}

pxWindowTextWriter& pxWindowTextWriter::WriteLn( const wxChar* fmt )
{
	_DoWrite( fmt );
	return *this;
}

pxWindowTextWriter& pxWindowTextWriter::FormatLn( const wxChar* fmt, ... )
{
	va_list args;
	va_start(args,fmt);
	_DoWrite( pxsFmtV(fmt, args) );
	va_end(args);
	return *this;
}

