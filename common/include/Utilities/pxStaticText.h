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

#pragma once

#include <wx/wx.h>
#include "wxGuiTools.h"

// --------------------------------------------------------------------------------------
//  pxStaticText
// --------------------------------------------------------------------------------------
// Important:
//   Proper use of this class requires using it's custom AddTo( wxSizer& ) method, in the
//   place of wxSizer.Add().  You can also use the += operator (recommended):
//
//     mySizer += new pxStaticText( this, _("omg translate me?") );
//
// This class's purpose is to overcome two fundamental annoyances in wxWidgets design:
//
//   * An inability to wrap text to conform to a fitted window (a limitation imposed by
//     wxWidgets inability to fit individual directions, ie fit widths and then fit heights,
//     which would allow a textbox to wrap text to a sizer-determined width, and then grow
//     the sizers vertically to fit the calcuated text-wrapped height).
// 
//   * Textbox alignment requires aligning both the textbox contents, and aligning the text
//     control within it's containing sizer.  If both alignment flags do not match the result
//     is typically undesirable.
//
// The first one is very hard to fix properly.  Currently this class employs a hack where it
// grabs the "ideal" fitting width from it's containing panel/window, and then wraps text to
// fit within those confines.  Under this design, pxStaticText controls will typically be the
// "regulators" of the window's display size, since they cannot really participate in the
// normal sizer system (since their minimum height is unknown until width-based sizes are
// determined).
//
// Note that if another control in the window has extends that blow the window size larger
// than the "ideal" width, then the pxStaticText will remain consistent in it's size.  It
// will not attempt to grow to fit the expanded area.  That might be fixable behavior, but
// it was hard enough for me to get this much working. ;)
//
class pxStaticText : public wxStaticText
{
	typedef wxStaticText _parent;

protected:
	wxString		m_message;
	int				m_wrapwidth;
	int				m_alignflags;
	bool			m_unsetLabel;
	double			m_centerPadding;

public:
	explicit pxStaticText( wxWindow* parent, const wxString& label=wxEmptyString, int style=wxALIGN_LEFT );
	explicit pxStaticText( wxWindow* parent, int style );

	virtual ~pxStaticText() throw() {}

	void SetLabel( const wxString& label );
	pxStaticText& SetWrapWidth( int newwidth );
	pxStaticText& SetToolTip( const wxString& tip );

	wxSize GetMinSize() const;
	//void DoMoveWindow(int x, int y, int width, int height);

	void AddTo( wxSizer& sizer, wxSizerFlags flags=pxSizerFlags::StdSpace() );
	void AddTo( wxSizer* sizer, const wxSizerFlags& flags=pxSizerFlags::StdSpace() ) { AddTo( *sizer, flags ); }

	void InsertAt( wxSizer& sizer, int position, wxSizerFlags flags=pxSizerFlags::StdSpace() );
	int GetIdealWidth() const;

protected:
	void _setLabel();
};

extern void operator+=( wxSizer& target, pxStaticText* src );

template<>
void operator+=( wxSizer& target, const pxWindowAndFlags<pxStaticText>& src )
{
	target.Add( src.window, src.flags );
}

// --------------------------------------------------------------------------------------
//  pxStaticHeading
// --------------------------------------------------------------------------------------
// Basically like a pxStaticText, except it defaults to wxALIGN_CENTRE, and it has expanded
// left and right side padding.
//
// The padding is not an exact science and, if there isn't any other controls in the form
// that are equal to or exceeding the IdealWidth, the control will end up fitting tightly
// to the heading (padding will be nullified).
//
class pxStaticHeading : public pxStaticText
{
public:
	pxStaticHeading( wxWindow* parent, const wxString& label=wxEmptyString, int style=wxALIGN_CENTRE );
	virtual ~pxStaticHeading() throw() {}

	//using pxStaticText::operator wxSizerFlags;
};
