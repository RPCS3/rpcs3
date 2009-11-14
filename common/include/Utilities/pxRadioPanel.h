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
#include "SafeArray.h"
#include "wxGuiTools.h"
#include <vector>

// --------------------------------------------------------------------------------------
//  RadioPanelItem
// --------------------------------------------------------------------------------------

struct RadioPanelItem
{
	wxString		Label;
	wxString		SubText;
	wxString		ToolTip;
	
	RadioPanelItem( const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString )
		: Label( label )
		, SubText( subtext )
		, ToolTip( tooltip )
	{
	}
	
	RadioPanelItem& SetToolTip( const wxString& tip )
	{
		ToolTip = tip;
		return *this;
	}

	RadioPanelItem& SetSubText( const wxString& text )
	{
		SubText = text;
		return *this;
	}
};


// Used as a cache for the "original" labels and subtexts, so that text can be properly
// wrapped and re-wrapped with multiple calls to OnResize().
struct RadioPanelObjects
{
	wxRadioButton*		LabelObj;
	wxStaticText*		SubTextObj;
};

// --------------------------------------------------------------------------------------
//  pxRadioPanel
// --------------------------------------------------------------------------------------
// Radio buttons work best when they are created consecutively, and then their subtext
// created in a second sweep (this keeps the radio buttons together in the parent window's
// child list, and avoids potentially unwanted behavior with radio buttons failing to
// group expectedly).  Because of this, our radio button helper is shaped as a panel of
// a group of radio butons only, instead of bothering with the lower level per-button
// design.  This makes a few other things nicer as well, such as binding a single message
// handler to all radio buttons in the panel.
//
// The SetToolTip API provided by this function applies the tooltip to both both the radio
// button and it's static subtext (if present), and performs word wrapping on platforms
// that need it (eg mswindows).
//
class pxRadioPanel : public wxPanelWithHelpers
{
protected:
	typedef std::vector<RadioPanelItem>		ButtonArray;
	typedef SafeArray<RadioPanelObjects>	ButtonObjArray;
	
	ButtonArray		m_buttonStrings;
	ButtonObjArray	m_objects;
	
	bool			m_IsRealized;
	int				m_idealWidth;
	
	wxSize			m_padding;
	int				m_Indentation;

public:
	template< int size >
	pxRadioPanel( wxPanelWithHelpers* parent, const RadioPanelItem (&src)[size] )
		: wxPanelWithHelpers( parent, parent->GetIdealWidth() )
	{
		Init( src, size );
	}

	template< int size >
	pxRadioPanel( wxDialogWithHelpers* parent, const RadioPanelItem (&src)[size] )
		: wxDialogWithHelpers( parent, parent->GetIdealWidth() )
	{
		Init( src, size );
	}

	template< int size >
	pxRadioPanel( int idealWidth, wxWindow* parent, const RadioPanelItem (&src)[size] )
		: wxPanelWithHelpers( parent, idealWidth )
	{
		Init( src, size );
	}

	pxRadioPanel( wxPanelWithHelpers* parent )
		: wxPanelWithHelpers( parent, parent->GetIdealWidth() )
	{
		Init();
	}

	pxRadioPanel( wxDialogWithHelpers* parent )
		: wxPanelWithHelpers( parent, parent->GetIdealWidth() )
	{
		Init();
	}

	pxRadioPanel( int idealWidth, wxPanelWithHelpers* parent )
		: wxPanelWithHelpers( parent, idealWidth )
	{
		Init();
	}

	virtual ~pxRadioPanel() throw() {}

	void Reset();
	void Realize();
	
	wxStaticText* GetSubText( int idx );
	const wxStaticText* GetSubText( int idx ) const;
	pxRadioPanel& Append( const RadioPanelItem& entry );

	pxRadioPanel& SetToolTip( int idx, const wxString& tip );
	pxRadioPanel& SetSelection( int idx );

	int GetSelection() const;
	wxWindowID GetSelectionId() const;
	bool IsSelected( int idx ) const;

	wxRadioButton* GetButton( int idx );
	const wxRadioButton* GetButton( int idx ) const;

	int GetPaddingHoriz() const		{ return m_padding.GetHeight(); }
	int GetIndentation() const		{ return m_Indentation; }

	pxRadioPanel& SetPaddingHoriz( int newpad )
	{
		m_padding.SetHeight( newpad );
		return *this;
	}
	
	pxRadioPanel& SetIndentation( int newdent )
	{
		m_Indentation = newdent;
		return *this;
	}

	bool HasSubText( int idx ) const
	{
		return !m_buttonStrings[idx].SubText.IsEmpty();
	}

	pxRadioPanel& Append( const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString )
	{
		return Append( RadioPanelItem(label, subtext, tooltip) );
	}
	
protected:
	void Init( const RadioPanelItem* srcArray=NULL, int arrsize=0 );
	void _setToolTipImmediate( int idx, const wxString &tip );
};
