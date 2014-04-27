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

#pragma once

#include <wx/wx.h>
#include "SafeArray.h"
#include "wxGuiTools.h"

// --------------------------------------------------------------------------------------
//  RadioPanelItem
// --------------------------------------------------------------------------------------

struct RadioPanelItem
{
	wxString		Label;
	wxString		SubText;
	wxString		ToolTip;

	int				SomeInt;
	void*			SomePtr;

	RadioPanelItem( const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString )
		: Label( label )
		, SubText( subtext )
		, ToolTip( tooltip )
	{
		SomeInt = 0;
		SomePtr = NULL;
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
	
	RadioPanelItem& SetInt( int intval )
	{
		SomeInt = intval;
		return *this;	
	}

	RadioPanelItem& SetPtr( void* ptrval )
	{
		SomePtr = ptrval;
		return *this;
	}
};


// Used as a cache for the "original" labels and subtexts, so that text can be properly
// wrapped and re-wrapped with multiple calls to OnResize().
struct RadioPanelObjects
{
	wxRadioButton*		LabelObj;
	pxStaticText*		SubTextObj;
};

// --------------------------------------------------------------------------------------
//  pxRadioPanel
// --------------------------------------------------------------------------------------
//
// Rationale:
// Radio buttons work best when they are created consecutively, and then their subtext
// created in a second sweep (this keeps the radio buttons together in the parent window's
// child list, and avoids potentially unwanted behavior with radio buttons failing to
// group expectedly).  Because of this, our radio button helper is shaped as a panel of
// a group of radio buttons only, instead of bothering with the lower level per-button
// design.  This makes a few other things nicer as well, such as binding a single message
// handler to all radio buttons in the panel.
//
class pxRadioPanel : public wxPanelWithHelpers
{
protected:
	typedef std::vector<RadioPanelItem>		ButtonArray;
	typedef SafeArray<RadioPanelObjects>	ButtonObjArray;

	ButtonArray		m_buttonStrings;
	ButtonObjArray	m_objects;

	bool			m_IsRealized;

	wxSize			m_padding;
	int				m_Indentation;
	int				m_DefaultIdx;		// index of the default option (gets specific color/font treatment)

public:
	template< int size >
	pxRadioPanel( wxWindow* parent, const RadioPanelItem (&src)[size] )
		: wxPanelWithHelpers( parent, wxVERTICAL )
	{
		Init( src, size );
	}

	pxRadioPanel( wxWindow* parent )
		: wxPanelWithHelpers( parent, wxVERTICAL )
	{
		Init();
	}

	virtual ~pxRadioPanel() throw() {}

	void Reset();
	void Realize();

	pxStaticText* GetSubText( int idx );
	const pxStaticText* GetSubText( int idx ) const;
	pxRadioPanel& Append( const RadioPanelItem& entry );

	pxRadioPanel& SetToolTip( int idx, const wxString& tip );
	pxRadioPanel& SetSelection( int idx );
	pxRadioPanel& SetDefaultItem( int idx );
	pxRadioPanel& EnableItem( int idx, bool enable=true );

	const RadioPanelItem& Item(int idx) const;
	RadioPanelItem& Item(int idx);

	int GetSelection() const;
	wxWindowID GetSelectionId() const;
	bool IsSelected( int idx ) const;

	const RadioPanelItem&	SelectedItem() const	{ return Item(GetSelection()); }
	RadioPanelItem&			SelectedItem()			{ return Item(GetSelection()); }

	wxRadioButton* GetButton( int idx );
	const wxRadioButton* GetButton( int idx ) const;

	int GetPaddingVert() const		{ return m_padding.GetHeight(); }
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
	void _RealizeDefaultOption();
};
