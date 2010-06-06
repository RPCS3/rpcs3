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
#include "pxStaticText.h"
#include <wx/wizard.h>

// --------------------------------------------------------------------------------------
//  pxStaticText  (implementations)
// --------------------------------------------------------------------------------------
pxStaticText::pxStaticText( wxWindow* parent )
	: _parent( parent )
{
	m_heightInLines = 1;
}

pxStaticText::pxStaticText( wxWindow* parent, const wxString& label, wxAlignment align )
	: _parent( parent )
{
	m_heightInLines = 1;
	m_align			= align;

	SetPaddingDefaults();
	Init( label );
}

void pxStaticText::Init( const wxString& label )
{
	m_autowrap			= true;
	m_wrappedWidth		= -1;

	//SetHeight( 1 );
	SetLabel( label );
	Connect( wxEVT_PAINT, wxPaintEventHandler(pxStaticText::paintEvent) );
}

void pxStaticText::SetPaddingDefaults()
{
	m_paddingPix_horiz	= 7;
	m_paddingPix_vert	= 1;

	m_paddingPct_horiz	= 0.0f;
	m_paddingPct_vert	= 0.0f;
}

pxStaticText& pxStaticText::SetHeight( int lines )
{
	if( !pxAssert(lines > 0) ) lines = 2;
	m_heightInLines = lines;

	int width, height;
	GetTextExtent( _("MyjS 23"), &width, &height );
	const int newHeight = ((height+1)*m_heightInLines) + (m_paddingPix_vert*2);
	SetMinSize( wxSize(GetMinWidth(), newHeight) );

	return *this;
}

pxStaticText& pxStaticText::Bold()
{
	wxFont bold( GetFont() );
	bold.SetWeight(wxBOLD);
	SetFont( bold );
	return *this;
}

pxStaticText& pxStaticText::PaddingPixH( int pixels )
{
	m_paddingPix_horiz = pixels;
	UpdateWrapping( false );
	Refresh();
	return *this;
}

pxStaticText& pxStaticText::PaddingPixV( int pixels )
{
	m_paddingPix_vert = pixels;
	Refresh();
	return *this;
}

pxStaticText& pxStaticText::PaddingPctH( float pct )
{
	pxAssume( pct < 0.5 );

	m_paddingPct_horiz = pct;
	UpdateWrapping( false );
	Refresh();
	return *this;
}

pxStaticText& pxStaticText::PaddingPctV( float pct )
{
	pxAssume( pct < 0.5 );

	m_paddingPct_vert = pct;
	Refresh();
	return *this;
}

pxStaticText& pxStaticText::Unwrapped()
{
	m_autowrap = false;
	UpdateWrapping( false );
	return *this;
}

int pxStaticText::calcPaddingWidth( int newWidth ) const
{
	return (int)(newWidth*m_paddingPct_horiz*2) + (m_paddingPix_horiz*2);
}

int pxStaticText::calcPaddingHeight( int newHeight ) const
{
	return (int)(newHeight*m_paddingPct_vert*2) + (m_paddingPix_vert*2);
}

wxSize pxStaticText::GetBestWrappedSize( const wxClientDC& dc ) const
{
	pxAssume( m_autowrap );

	// Find an ideal(-ish) width, based on a search of all parent controls and their
	// valid Minimum sizes.

	int idealWidth = wxDefaultCoord;
	int parentalAdjust = 0;
	double parentalFactor = 1.0;
	const wxWindow* millrun = this;
	
	while( millrun )
	{
		// IMPORTANT : wxWizard changes its min size and then expects everything else
		// to play nice and NOT resize according to the new min size.  (wtf stupid)
		// Anyway, this fixes it -- ignore min size specifier on wxWizard!
		if( wxIsKindOf( millrun, wxWizard ) ) break;

		int min = (int)((millrun->GetMinWidth() - parentalAdjust) * parentalFactor);

		if( min > 0 && ((idealWidth < 0 ) || (min < idealWidth)) )
		{
			idealWidth = min;
		}

		parentalAdjust += pxSizerFlags::StdPadding*2;
		millrun = millrun->GetParent();
	}

	if( idealWidth <= 0 )
	{
		// FIXME: The minimum size of this control is unknown, so let's just pick a guess based on
		// the size of the user's display area.

		idealWidth = (int)(wxGetDisplaySize().GetWidth() * 0.66) - (parentalAdjust*2);
	}

	wxString label(GetLabel());
	return dc.GetMultiLineTextExtent(pxTextWrapper().Wrap( this, label, idealWidth - calcPaddingWidth(idealWidth) ).GetResult());
}

pxStaticText& pxStaticText::WrapAt( int width )
{
	m_autowrap = false;

	if( (width <= 1) || (width == m_wrappedWidth) ) return *this;

	wxString wrappedLabel;
	m_wrappedWidth = width;

	if( width > 1 )
	{
		wxString label( GetLabel() );
		wrappedLabel = pxTextWrapper().Wrap( this, label, width ).GetResult();
	}

	if(m_wrappedLabel != wrappedLabel )
	{
		m_wrappedLabel = wrappedLabel;
		wxSize area = wxClientDC( this ).GetMultiLineTextExtent(m_wrappedLabel);
		SetMinSize( wxSize(
			area.GetWidth() + calcPaddingWidth(area.GetWidth()),
			area.GetHeight() + calcPaddingHeight(area.GetHeight())
		) );
	}
	return *this;
}

bool pxStaticText::_updateWrapping( bool textChanged )
{
	if( !m_autowrap )
	{
		//m_wrappedLabel = wxEmptyString;
		//m_wrappedWidth = -1;
		return false;
	}

	wxString wrappedLabel;
	int newWidth = GetSize().GetWidth();
	newWidth -= (int)(newWidth*m_paddingPct_horiz*2) + (m_paddingPix_horiz*2);

	if( !textChanged && (newWidth == m_wrappedWidth) ) return false;

	// Note: during various stages of sizer-calc, width can be 1, 0, or -1.
	// We ignore wrapping in these cases.  (the PaintEvent also checks the wrapping
	// and updates it if needed, in case the control's size isn't figured out prior
	// to being painted).
	
	m_wrappedWidth = newWidth;
	if( m_wrappedWidth > 1 )
	{
		wxString label( GetLabel() );
		wrappedLabel = pxTextWrapper().Wrap( this, label, m_wrappedWidth ).GetResult();
	}

	if( m_wrappedLabel == wrappedLabel ) return false;
	m_wrappedLabel = wrappedLabel;

	return true;
}

void pxStaticText::UpdateWrapping( bool textChanged )
{
	if( _updateWrapping( textChanged ) ) Refresh();
}

void pxStaticText::SetLabel(const wxString& label)
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

wxFont pxStaticText::GetFontOk() const
{
	wxFont font( GetFont() );
	if( !font.Ok() ) return wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
	return font;
}

bool pxStaticText::Enable( bool enabled )
{
	if( _parent::Enable(enabled))
	{
		Refresh();
		return true;
	}
	return false;
}

void pxStaticText::paintEvent(wxPaintEvent& evt)
{
	wxPaintDC dc( this );
	const int dcWidth	= dc.GetSize().GetWidth();
	const int dcHeight	= dc.GetSize().GetHeight();
	if( dcWidth < 1 ) return;
	dc.SetFont( GetFontOk() );
	
	if( IsEnabled() )
		dc.SetTextForeground(GetForegroundColour());
	else
		dc.SetTextForeground(*wxLIGHT_GREY);

	pxWindowTextWriter writer( dc );
	writer.Align( m_align );

	wxString label;

	if( m_autowrap )
	{
		_updateWrapping( false );
		label = m_wrappedLabel;
	}
	else
	{
		label = GetLabel();
	}

	int tWidth, tHeight;
	dc.GetMultiLineTextExtent( label, &tWidth, &tHeight );
	
	writer.Align( m_align );
	if( m_align & wxALIGN_CENTER_VERTICAL )
		writer.SetY( (dcHeight - tHeight) / 2 );
	else
		writer.SetY( (int)(dcHeight*m_paddingPct_vert) + m_paddingPix_vert );
	
	writer.WriteLn( label );	// without formatting please.
	
	//dc.SetBrush( *wxTRANSPARENT_BRUSH );
	//dc.DrawRectangle(wxPoint(), dc.GetSize());
}

// Overloaded form wxPanel and friends.
wxSize pxStaticText::DoGetBestSize() const
{
    wxClientDC dc( const_cast<pxStaticText*>(this) );
    dc.SetFont( GetFontOk() );

	wxSize best;

	if( m_autowrap )
	{
		best = GetBestWrappedSize(dc);
		best.x = wxDefaultCoord;
	}
	else
	{
		// No autowrapping, so we can force a specific size here!
		best = dc.GetMultiLineTextExtent( GetLabel() );
		best.x += calcPaddingWidth( best.x );
	}

	best.y += calcPaddingHeight( best.y );

    CacheBestSize(best);
    return best;
}

// --------------------------------------------------------------------------------------
//  pxStaticHeading  (implementations)
// --------------------------------------------------------------------------------------
pxStaticHeading::pxStaticHeading( wxWindow* parent, const wxString& label )
	: _parent( parent )
{
	m_align			= wxALIGN_CENTER;

	SetPaddingDefaults();
	Init( label );
}

void pxStaticHeading::SetPaddingDefaults()
{
	m_paddingPix_horiz	= 4;
	m_paddingPix_vert	= 1;

	m_paddingPct_horiz	= 0.08f;
	m_paddingPct_vert	= 0.0f;
}

