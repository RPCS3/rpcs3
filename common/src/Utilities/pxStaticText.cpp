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
#include "pxStaticText.h"

// --------------------------------------------------------------------------------------
//  pxStaticText Implementations
// --------------------------------------------------------------------------------------

pxStaticText::pxStaticText( wxWindow* parent, const wxString& label, int style )
	: wxStaticText( parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, style )
	, m_message( label )
{
	m_alignflags	= style & wxALIGN_MASK;
	m_wrapwidth		= wxDefaultCoord;
	m_centerPadding = 0.08;
}

pxStaticText::pxStaticText( wxWindow* parent, int style )
	: wxStaticText( parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, style )
{
	m_alignflags	= style & wxALIGN_MASK;
	m_wrapwidth		= wxDefaultCoord;
	m_centerPadding = 0.08;
}

pxStaticText& pxStaticText::SetWrapWidth( int newwidth )
{
	m_wrapwidth = newwidth;
	SetLabel( m_message );
	return *this;
}

void pxStaticText::SetLabel( const wxString& label )
{
	m_message		= label;
	_setLabel();
}

void pxStaticText::_setLabel()
{
	_parent::SetLabel( pxTextWrapper().Wrap( *this, m_message, GetIdealWidth() ).GetResult() );
}

pxStaticText& pxStaticText::SetToolTip( const wxString& tip )
{
	pxSetToolTip( this, tip );
	return *this;
}

void pxStaticText::AddTo( wxSizer& sizer, wxSizerFlags flags )
{
	sizer.Add( this, flags.Align( m_alignflags | (flags.GetFlags() & wxALIGN_MASK) ) );
	_setLabel();
}

void pxStaticText::InsertAt( wxSizer& sizer, int position, wxSizerFlags flags )
{
	sizer.Insert( position, this, flags.Align( m_alignflags | (flags.GetFlags() & wxALIGN_MASK) ) );
	_setLabel();
}

int pxStaticText::GetIdealWidth() const
{
	if( m_wrapwidth != wxDefaultCoord ) return m_wrapwidth;

	//pxAssertDev( GetContainingSizer() != NULL, "The Static Text must first belong to a Sizer!!" );
	int idealWidth = wxDefaultCoord;

	// Find the first parent with a fixed width:
	wxWindow* millrun = this->GetParent();
	while( (idealWidth == wxDefaultCoord) && millrun != NULL )
	{
		if( wxPanelWithHelpers* panel = wxDynamicCast( millrun, wxPanelWithHelpers ) )
			idealWidth = panel->GetIdealWidth();

		else if( wxDialogWithHelpers* dialog = wxDynamicCast( millrun, wxDialogWithHelpers ) )
			idealWidth = dialog->GetIdealWidth();

		millrun = millrun->GetParent();
	}

	if( idealWidth != wxDefaultCoord )
	{
		idealWidth -= 6;

		if( GetWindowStyle() & wxALIGN_CENTRE )
			idealWidth *= (1.0 - m_centerPadding);
	}

	return idealWidth;
}

wxSize pxStaticText::GetMinSize() const
{
	int ideal = GetIdealWidth();
	wxSize minSize( _parent::GetMinSize() );
	if( ideal == wxDefaultCoord ) return minSize;
	return wxSize( std::min( ideal, minSize.x ), minSize.y );
}

// --------------------------------------------------------------------------------------
//  pxStaticHeading Implementations
// --------------------------------------------------------------------------------------
pxStaticHeading::pxStaticHeading( wxWindow* parent, const wxString& label, int style )
	: pxStaticText( parent, label, style )
{
	m_centerPadding = 0.18;
}

void operator+=( wxSizer& target, pxStaticText* src )
{
	if( !pxAssert( src != NULL ) ) return;
	src->AddTo( target );
}

void operator+=( wxSizer& target, pxStaticText& src )
{
	src.AddTo( target );
}

void operator+=( wxSizer* target, pxStaticText& src )
{
	src.AddTo( target );
}
