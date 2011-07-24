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
#include "pxStaticText.h"

#include <wx/app.h>
#include <wx/window.h>

const pxAlignmentType
	pxCentre		= { pxAlignmentType::Center },	// Horizontal centered alignment
	pxCenter		= pxCentre,
	pxMiddle		= { pxAlignmentType::Middle },	// vertical centered alignment

	pxAlignLeft		= { pxAlignmentType::Left },
	pxAlignRight	= { pxAlignmentType::Right },
	pxAlignTop		= { pxAlignmentType::Top },
	pxAlignBottom	= { pxAlignmentType::Bottom };

const pxStretchType
	pxShrink		= { pxStretchType::Shrink },
	pxExpand		= { pxStretchType::Expand },
	pxShaped		= { pxStretchType::Shaped },
	pxReserveHidden	= { pxStretchType::ReserveHidden },
	pxFixedMinimum	= { pxStretchType::FixedMinimum };

wxSizerFlags pxAlignmentType::Apply( wxSizerFlags flags ) const
{
	switch( intval )
	{
		case Centre:
			flags.Align( flags.GetFlags() | wxALIGN_CENTRE_HORIZONTAL );
		break;

		case Middle:
			flags.Align( flags.GetFlags() | wxALIGN_CENTRE_VERTICAL );
		break;

		case Left:
			flags.Left();
		break;

		case Right:
			flags.Right();
		break;

		case Top:
			flags.Top();
		break;

		case Bottom:
			flags.Bottom();
		break;
	}
	return flags;
}

wxSizerFlags pxStretchType::Apply( wxSizerFlags flags ) const
{
	switch( intval )
	{
		case Shrink:
			//pxFail( "wxSHRINK is an ignored stretch flag." );
		break;

		case Expand:
			flags.Expand();
		break;

		case Shaped:
			flags.Shaped();
		break;

		case ReserveHidden:
			flags.ReserveSpaceEvenIfHidden();
		break;

		case FixedMinimum:
			flags.FixedMinSize();
			break;

		//case Tile:
		//	pxAssert( "pxTile is an unsupported stretch tag (ignored)." );
		//break;
	}
	return flags;
}

wxSizerFlags operator& ( const wxSizerFlags& _flgs, const wxSizerFlags& _flgs2 )
{
	//return align.Apply( _flgs );
	wxSizerFlags retval;

	uint allflags = (_flgs.GetFlags() | _flgs2.GetFlags());

	retval.Align( allflags & wxALIGN_MASK );
	if( allflags & wxEXPAND ) retval.Expand();
	if( allflags & wxSHAPED ) retval.Shaped();
	if( allflags & wxFIXED_MINSIZE ) retval.FixedMinSize();
	if( allflags & wxRESERVE_SPACE_EVEN_IF_HIDDEN ) retval.ReserveSpaceEvenIfHidden();

	// Compounding borders is probably a fair approach:
	retval.Border( allflags & wxALL, _flgs.GetBorderInPixels() + _flgs2.GetBorderInPixels() );

	// Compounding proportions works as well, I figure.
	retval.Proportion( _flgs.GetProportion() + _flgs2.GetProportion() );

	return retval;
}

// ----------------------------------------------------------------------------
// Reference/Handle versions!

void operator+=( wxSizer& target, wxWindow* src )
{
	target.Add( src );
}

void operator+=( wxSizer& target, wxSizer* src )
{
	target.Add( src );
}

void operator+=( wxSizer& target, wxWindow& src )
{
	target.Add( &src );
}

void operator+=( wxSizer& target, wxSizer& src )
{
	target.Add( &src );
}

void operator+=( wxSizer& target, int spacer )
{
	target.AddSpacer( spacer );
}

void operator+=( wxSizer& target, const pxStretchSpacer& spacer )
{
	target.AddStretchSpacer( spacer.proportion );
}

void operator+=( wxWindow& target, int spacer )
{
	if( !pxAssert( target.GetSizer() != NULL ) ) return;
	target.GetSizer()->AddSpacer( spacer );
}

void operator+=( wxWindow& target, const pxStretchSpacer& spacer )
{
	if( !pxAssert( target.GetSizer() != NULL ) ) return;
	target.GetSizer()->AddStretchSpacer( spacer.proportion );
}

// ----------------------------------------------------------------------------
// Pointer versions!  (note that C++ requires one of the two operator params be a
// "poper" object type (non-pointer), so that's why there's only a couple of these.

void operator+=( wxSizer* target, wxWindow& src )
{
	if( !pxAssert( target != NULL ) ) return;
	target->Add( &src );
}

void operator+=( wxSizer* target, wxSizer& src )
{
	if( !pxAssert( target != NULL ) ) return;
	target->Add( &src );
}

// ----------------------------------------------------------------------------

// Returns FALSE if the window position is considered invalid, which means that it's title
// bar is most likely not easily grabble.  Such a window should be moved to a valid or
// default position.
bool pxIsValidWindowPosition( const wxWindow& window, const wxPoint& windowPos )
{
	// The height of the window is only revlevant to the height of a title bar, which is
	// all we need visible for the user to be able to drag the window into view.  But
	// there's no way to get that info from wx, so we'll just have to guesstimate...

	wxSize sizeMatters( window.GetSize().GetWidth(), 32 );		// if some gui has 32 pixels of undraggable title bar, the user deserves to suffer.
	return wxGetDisplayArea().Contains( wxRect( windowPos, sizeMatters ) );
}

// Retrieves the area of the screen, which can be used to enforce a valid zone for
// window positioning. (top/left coords are almost always (0,0) and bottom/right
// is the resolution of the desktop).
wxRect wxGetDisplayArea()
{
	return wxRect( wxPoint(), wxGetDisplaySize() );
}

// --------------------------------------------------------------------------------------
//  pxSizerFlags
// --------------------------------------------------------------------------------------
// FlagsAccessors - Provides read-write copies of standard sizer flags for our interface.
// These standard definitions provide a consistent and pretty interface for our GUI.
// Without them things look compacted, misaligned, and yucky!
//
// Implementation Note: Accessors are all provisioned as dynamic (realtime) sizer calculations.
// I've preferred this over cstatic const variables on the premise that spacing logic could
// in the future become a dynamic value (currently it is affixed to 6 for most items).
//
wxSizerFlags pxSizerFlags::StdSpace()
{
	return wxSizerFlags().Border( wxALL, StdPadding );
}

wxSizerFlags pxSizerFlags::StdCenter()
{
	return wxSizerFlags().Align( wxALIGN_CENTER ).DoubleBorder();
}

wxSizerFlags pxSizerFlags::StdExpand()
{
	return StdSpace().Expand();
}

// A good sizer flags setting for top-level static boxes or top-level picture boxes.
// Gives a generous border to the left, right, and bottom.  Top border can be configured
// manually by using a spacer.
wxSizerFlags pxSizerFlags::TopLevelBox()
{
	return pxBorder( wxLEFT | wxBOTTOM | wxRIGHT, StdPadding ).Expand();
}

// Flags intended for use on grouped StaticBox controls.  These flags are ideal for
// StaticBoxes that are part of sub-panels or children of other static boxes, but may
// not be best for parent StaticBoxes on dialogs (left and right borders feel a bit
// "tight").
wxSizerFlags pxSizerFlags::SubGroup()
{
	// Groups look better with a slightly smaller margin than standard.
	// (basically this accounts for the group's frame)
	return pxBorder( wxLEFT | wxBOTTOM | wxRIGHT, StdPadding-2 ).Expand();
}

// This force-aligns the std button sizer to the right, where (at least) us win32 platform
// users always expect it to be.  Most likely Mac platforms expect it on the left side
// just because it's *not* where win32 sticks it.  Too bad!
wxSizerFlags pxSizerFlags::StdButton()
{
	return pxBorder().Align( wxALIGN_RIGHT );
}

wxSizerFlags pxSizerFlags::Checkbox()
{
	return StdExpand();
}

// --------------------------------------------------------------------------------------
//  pxTextWrapper / pxTextWrapperBase  (implementations)
// --------------------------------------------------------------------------------------

static bool is_cjk_char(const uint ch)
{
	/**
	 * You can check these range at http://unicode.org/charts/
	 * see the "East Asian Scripts" part.
	 * Notice that not all characters in that part is still in use today, so don't list them all here.
	 */

	// FIXME: add range from Japanese-specific and Korean-specific section if you know the
	// characters are used today.

	if (ch < 0x2e80) return false; // shortcut for common non-CJK

	return
		// Han Ideographs: all except Supplement
		(ch >= 0x4e00 && ch < 0x9fcf) ||
		(ch >= 0x3400 && ch < 0x4dbf) ||
		(ch >= 0x20000 && ch < 0x2a6df) ||
		(ch >= 0xf900 && ch < 0xfaff) ||
		(ch >= 0x3190 && ch < 0x319f) ||

		// Radicals: all except Ideographic Description
		(ch >= 0x2e80 && ch < 0x2eff) ||
		(ch >= 0x2f00 && ch < 0x2fdf) ||
		(ch >= 0x31c0 && ch < 0x31ef) ||

		// Chinese-specific: Bopomofo
		(ch >= 0x3000 && ch < 0x303f) ||

		// Japanese-specific: Halfwidth Katakana
		(ch >= 0xff00 && ch < 0xffef) ||

		// Japanese-specific: Hiragana, Katakana
		(ch >= 0x3040 && ch <= 0x309f) ||
		(ch >= 0x30a0 && ch <= 0x30ff) ||

		// Korean-specific: Hangul Syllables, Halfwidth Jamo
		(ch >= 0xac00 && ch < 0xd7af) ||
		(ch >= 0xff00 && ch < 0xffef);
}

/*
 * According to Kinsoku-Shori, Japanese rules about line-breaking:
 *
 * * the following characters cannot begin a line (so we will never break before them):
 * 、。，．）〕］｝〉》」』】’”ゝゞヽヾ々？！：；ぁぃぅぇぉゃゅょゎァィゥェォャュョヮっヵッヶ・…ー
 *
 * * the following characters cannot end a line (so we will never break after them):
 * （〔［｛〈《「『【‘“
 *
 * Unicode range that concerns word wrap for Chinese:
 *   全角ASCII、全角中英文标点 (Fullwidth Character for ASCII, English punctuations and part of Chinese punctuations)
 *   http://www.unicode.org/charts/PDF/UFF00.pdf
 *   CJK 标点符号 (CJK punctuations)
 *   http://www.unicode.org/charts/PDF/U3000.pdf
 */
static bool no_break_after(const uint ch)
{
	switch( ch )
	{
		/**
		 * don't break after these Japanese/Chinese characters
		 */
		case 0x2018: case 0x201c: case 0x3008: case 0x300a:
		case 0x300c: case 0x300e: case 0x3010: case 0x3014:
		case 0x3016: case 0x301a: case 0x301d:
		case 0xff08: case 0xff3b: case 0xff5b:

		/**
		 * FIXME don't break after these Korean characters
		 */

			return true;
	}

	return false;
}

static bool no_break_before(const uint ch)
{
	switch(ch)
	{
		/**
		 * don't break before these Japanese characters
		 */
		case 0x2019: case 0x201d: case 0x2026: case 0x3001: case 0x3002:
		case 0x3005: case 0x3009: case 0x300b: case 0x300d: case 0x300f:
		case 0x301c: case 0x3011: case 0x3015: case 0x3017: case 0x301b:
		case 0x301e: case 0x3041: case 0x3043: case 0x3045:
		case 0x3047: case 0x3049: case 0x3063: case 0x3083: case 0x3085:
		case 0x3087: case 0x308e: case 0x309d: case 0x309e: case 0x30a1:
		case 0x30a3: case 0x30a5: case 0x30a7: case 0x30a9: case 0x30c3:
		case 0x30e3: case 0x30e5: case 0x30e7: case 0x30ee: case 0x30f5:
		case 0x30f6: case 0x30fb: case 0x30fc: case 0x30fd: case 0x30fe:
		case 0xff01: case 0xff09: case 0xff0c: case 0xff0d: case 0xff0e: case 0xff1a:
		case 0xff1b: case 0xff1f: case 0xff3d: case 0xff5d: case 0xff64: case 0xff65: 

		/**
		 * FIXME don't break before these Korean characters
		 */

		/**
		 * don't break before these Chinese characters
		 * contains
		 *   many Chinese punctuations that should not start a line
		 *   and right side of different kinds of brackets, quotes
		 */
		
		
			return true;
	}
	return false;
}

pxTextWrapperBase& pxTextWrapperBase::Wrap( const wxWindow& win, const wxString& text, int widthMax )
{
	if( text.IsEmpty() ) return *this;

    const wxChar *lastSpace = NULL;
    bool wasWrapped = false;

    wxString line;
    line.Alloc( text.Length()+12 );

    const wxChar* lineStart = text.wc_str();
    for ( const wxChar *p = lineStart; ; p++ )
    {
        if ( IsStartOfNewLine() )
        {
            OnNewLine();

            lastSpace = NULL;
            lineStart = p;

			if(wasWrapped)
				line = m_indent;
			else
				line.clear();
        }

        if ( *p == L'\n' || *p == L'\0' )
        {
			wasWrapped = false;
            DoOutputLine(line);

            if ( *p == L'\0' )
                break;
        }
        else // not EOL
        {
			if (is_cjk_char(*p))
			{
				if (!no_break_before(*p))
				{
					if (p == lineStart || !no_break_after(*(p-1)))
						lastSpace = p;
				}
			}
			else if ( *p == L' ' || *p == L',' || *p == L'/' )
                lastSpace = p;

            line += *p;

            if ( widthMax >= 0 && lastSpace )
            {
                int width;
                win.GetTextExtent(line, &width, NULL);

                if ( width > widthMax )
                {
					wasWrapped = true;

                    // remove the last word from this line
                    line.erase(lastSpace - lineStart, p + 1 - lineStart);
                    DoOutputLine(line);

                    // go back to the last word of this line which we didn't
                    // output yet
                    p = lastSpace;

					if ( *p != L' ' )
                       p--;

                }
            }
            //else: no wrapping at all or impossible to wrap
        }
    }

    return *this;
}

void pxTextWrapperBase::DoOutputLine(const wxString& line)
{
	OnOutputLine(line);
	m_linecount++;
	m_eol = true;
}

// this function is a destructive inspector: when it returns true it also
// resets the flag to false so calling it again wouldn't return true any
// more
bool pxTextWrapperBase::IsStartOfNewLine()
{
	if ( !m_eol )
		return false;

	m_eol = false;
	return true;
}

pxTextWrapper& pxTextWrapper::Wrap( const wxWindow& win, const wxString& text, int widthMax )
{
	_parent::Wrap( win, text, widthMax );
	return *this;
}

pxTextWrapper& pxTextWrapper::Wrap( const wxWindow* win, const wxString& text, int widthMax )
{
	if( win ) _parent::Wrap( *win, text, widthMax );
	return *this;
}

void pxTextWrapper::OnOutputLine(const wxString& line)
{
	m_text += line;
}

void pxTextWrapper::OnNewLine()
{
	m_text += L'\n';
}

// --------------------------------------------------------------------------------------
//  ScopedBusyCursor  (implementations)
// --------------------------------------------------------------------------------------

std::stack<BusyCursorType>	ScopedBusyCursor::m_cursorStack;
BusyCursorType				ScopedBusyCursor::m_defBusyType;

ScopedBusyCursor::ScopedBusyCursor( BusyCursorType busytype )
{
	pxAssert( wxTheApp != NULL );

	BusyCursorType curtype = Cursor_NotBusy;
	if( !m_cursorStack.empty() )
		curtype = m_cursorStack.top();

	if( curtype < busytype )
		SetManualBusyCursor( curtype=busytype );

	m_cursorStack.push( curtype );
}

ScopedBusyCursor::~ScopedBusyCursor() throw()
{
	if( !pxAssert( wxTheApp != NULL ) ) return;

	if( !pxAssert( !m_cursorStack.empty() ) )
	{
		SetManualBusyCursor( m_defBusyType );
		return;
	}

	BusyCursorType curtype = m_cursorStack.top();
	m_cursorStack.pop();

	if( m_cursorStack.empty() )
		SetManualBusyCursor( m_defBusyType );
	else if( m_cursorStack.top() != curtype )
		SetManualBusyCursor( m_cursorStack.top() );
}

void ScopedBusyCursor::SetDefault( BusyCursorType busytype )
{
	if( busytype == m_defBusyType ) return;
	m_defBusyType = busytype;

	if( m_cursorStack.empty() )
		SetManualBusyCursor( busytype );
}

void ScopedBusyCursor::SetManualBusyCursor( BusyCursorType busytype )
{
	switch( busytype )
	{
		case Cursor_NotBusy:	wxSetCursor( wxNullCursor ); break;
		case Cursor_KindaBusy:	wxSetCursor( StockCursors.GetArrowWait() ); break;
		case Cursor_ReallyBusy:	wxSetCursor( *wxHOURGLASS_CURSOR ); break;
	}
}

const wxCursor& MoreStockCursors::GetArrowWait()
{
	if( !m_arrowWait )
		m_arrowWait = new wxCursor( wxCURSOR_ARROWWAIT );
	return *m_arrowWait;
}

MoreStockCursors StockCursors;

// --------------------------------------------------------------------------------------
//  pxFormatToolTipText / pxSetToolTip
// --------------------------------------------------------------------------------------
// This is the preferred way to assign tooltips to wxWindow-based objects, as it performs the
// necessary text wrapping on platforms that need it.  On windows tooltips are wrapped at 600
// pixels, or 66% of the screen width, whichever is smaller.  GTK and MAC perform internal
// wrapping, so this function does a regular assignment there.

wxString pxFormatToolTipText( wxWindow* wind, const wxString& src )
{
	// Windows needs manual tooltip word wrapping (sigh).
	// GTK and Mac are a wee bit more clever (except in GTK tooltips don't show at all
	// half the time because of some other bug .. sigh)

#ifdef __WXMSW__
	if( wind == NULL ) return src;		// Silently ignore nulls
	int whee = wxGetDisplaySize().GetWidth() * 0.75;
	return pxTextWrapper().Wrap( *wind, src, std::min( whee, 600 ) ).GetResult();
#else
	return src;
#endif
}

void pxSetToolTip( wxWindow* wind, const wxString& src )
{
	if( wind == NULL ) return;		// Silently ignore nulls
	wind->SetToolTip( pxFormatToolTipText(wind, src) );
}

void pxSetToolTip( wxWindow& wind, const wxString& src )
{
	pxSetToolTip( &wind, src );
}


wxFont pxGetFixedFont( int ptsize, int weight )
{
	return wxFont(
		ptsize, wxMODERN, wxNORMAL, weight, false
#ifdef __WXMSW__
		,L"Lucida Console"		// better than courier new (win32 only)
#endif
	);
}


wxString pxGetAppName()
{
	pxAssert( wxTheApp );
	return wxTheApp->GetAppName();
}
