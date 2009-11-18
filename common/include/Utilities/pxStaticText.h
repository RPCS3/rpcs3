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

	void AddTo( wxSizer& sizer );
	void InsertAt( wxSizer& sizer, int position );
	int GetIdealWidth() const;

protected:
	void _setLabel();
};

class pxStaticHeading : public pxStaticText
{
public:
	pxStaticHeading( wxWindow* parent, const wxString& label=wxEmptyString, int style=wxALIGN_CENTRE );
	virtual ~pxStaticHeading() throw() {}

	//using pxStaticText::operator wxSizerFlags;
};
