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
#include "pxRadioPanel.h"
#include "pxStaticText.h"

// ===========================================================================================
//  pxRadioPanel Implementations
// ===========================================================================================

#define VerifyRealizedState() \
	pxAssertDev( m_IsRealized, "Invalid object state: RadioButtonGroup as not been realized." )

void pxRadioPanel::Init( const RadioPanelItem* srcArray, int arrsize )
{
	m_DefaultIdx	= -1;
	m_IsRealized	= false;

	// FIXME: This probably needs to be platform-dependent, and/or based on font size.
	m_Indentation	= 23;

	for( int i=0; i<arrsize; ++i )
		Append( srcArray[i] );
}

pxRadioPanel& pxRadioPanel::Append( const RadioPanelItem& entry )
{
	m_buttonStrings.push_back( entry );
	return *this;
}

void pxRadioPanel::Reset()
{
	m_IsRealized = false;
	const int numbuttons = m_buttonStrings.size();
	if( numbuttons == 0 ) return;

	for( int i=0; i<numbuttons; ++i)
	{
		safe_delete( m_objects[i].LabelObj );
		safe_delete( m_objects[i].SubTextObj );
	}
	m_buttonStrings.clear();
}

void pxRadioPanel::Realize()
{
	const int numbuttons = m_buttonStrings.size();
	if( numbuttons == 0 ) return;
	if( m_IsRealized ) return;
	m_IsRealized = true;

	m_objects.MakeRoomFor( numbuttons );

	// Add all RadioButtons in one pass, and then go back and create all the subtext
	// objects.  This ensures the radio buttons have consecutive tab order IDs, which
	// is the "preferred" method when using grouping features of the native window
	// managers (GTK tends to not care either way, but Win32 definitely prefers a
	// linear tab order).

	// first object has the group flag set to ensure it's the start of a radio group.
	m_objects[0].LabelObj = new wxRadioButton( this, wxID_ANY, m_buttonStrings[0].Label, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	for( int i=1; i<numbuttons; ++i )
		m_objects[i].LabelObj = new wxRadioButton( this, wxID_ANY, m_buttonStrings[i].Label );

	for( int i=0; i<numbuttons; ++i )
	{
		m_objects[i].SubTextObj = NULL;
		if( m_buttonStrings[i].SubText.IsEmpty() ) continue;
		m_objects[i].SubTextObj = new pxStaticText( this, m_buttonStrings[i].SubText );
	}

	pxAssert( GetSizer() != NULL );

	for( int i=0; i<numbuttons; ++i )
	{
		*this += m_objects[i].LabelObj	| pxSizerFlags::StdExpand();

		if( pxStaticText* subobj = m_objects[i].SubTextObj )
		{
			*this += subobj	| pxBorder( wxLEFT, m_Indentation ).Expand();
			*this += 9 + m_padding.GetHeight();
		}
		if( !m_buttonStrings[i].ToolTip.IsEmpty() )
			_setToolTipImmediate( i, m_buttonStrings[i].ToolTip );
	}

	_RealizeDefaultOption();
}

void pxRadioPanel::_setToolTipImmediate( int idx, const wxString &tip )
{
	const wxString wrapped( pxFormatToolTipText(this, tip) );
	if( wxRadioButton* woot = m_objects[idx].LabelObj )
		woot->SetToolTip( wrapped );

	if( pxStaticText* woot = m_objects[idx].SubTextObj )
		woot->SetToolTip( wrapped );
}

// The SetToolTip API provided by this function applies the tooltip to both the radio
// button and it's static subtext (if present), and performs word wrapping on platforms
// that need it (eg mswindows).
pxRadioPanel& pxRadioPanel::SetToolTip(int idx, const wxString &tip)
{
	m_buttonStrings[idx].SetToolTip( tip );

	if( m_IsRealized )
		_setToolTipImmediate( idx, tip );

	return *this;
}

pxRadioPanel& pxRadioPanel::SetSelection( int idx )
{
	if( !VerifyRealizedState() ) return *this;

	pxAssert( m_objects[idx].LabelObj != NULL );
	m_objects[idx].LabelObj->SetValue( true );
	return *this;
}

void pxRadioPanel::_RealizeDefaultOption()
{
	if( m_IsRealized && m_DefaultIdx != -1 )
	{
		wxFont def( GetFont() );
		def.SetWeight( wxBOLD );
		//def.SetStyle( wxITALIC );
		m_objects[m_DefaultIdx].LabelObj->SetFont( def );
		m_objects[m_DefaultIdx].LabelObj->SetForegroundColour( wxColour( 20, 128, 40 ) );
	}
}

pxRadioPanel& pxRadioPanel::SetDefaultItem( int idx )
{
	if( idx == m_DefaultIdx ) return *this;

	if( m_IsRealized && m_DefaultIdx != -1 )
	{
		wxFont def(GetFont());
		m_objects[m_DefaultIdx].LabelObj->SetFont( def );
		m_objects[m_DefaultIdx].LabelObj->SetForegroundColour( GetForegroundColour() );
	}

	m_DefaultIdx = idx;
	_RealizeDefaultOption();

	return *this;
}

pxRadioPanel& pxRadioPanel::EnableItem( int idx, bool enable )
{
	pxAssertDev( m_IsRealized, "RadioPanel must be realized first, prior to enabling or disabling individual items." );

	if( m_objects[idx].LabelObj )
		m_objects[idx].LabelObj->Enable( enable );

	if( m_objects[idx].SubTextObj )
		m_objects[idx].SubTextObj->Enable( enable );

	return *this;
}

const RadioPanelItem& pxRadioPanel::Item(int idx) const
{
	return m_buttonStrings[idx];
}

RadioPanelItem& pxRadioPanel::Item(int idx)
{
	return m_buttonStrings[idx];
}

int pxRadioPanel::GetSelection() const
{
	if( !VerifyRealizedState() ) return 0;

	for( uint i=0; i<m_buttonStrings.size(); ++i )
	{
		if( wxRadioButton* woot = m_objects[i].LabelObj )
			if( woot->GetValue() ) return i;
	}

	// Technically radio buttons should never allow for a case where none are selected.
	// However it *can* happen on some platforms if the program code doesn't explicitly
	// select one of the members of the group (which is, as far as I'm concerned, a
	// programmer error!). so Assert here in such cases, and return 0 as the assumed
	// default, so that calling code has a "valid" return code in release builds.

	pxFailDev( "No valid selection was found in this group!" );
	return 0;
}

// Returns the wxWindowID for the currently selected radio button.
wxWindowID pxRadioPanel::GetSelectionId() const
{
	if( !VerifyRealizedState() ) return 0;
	return m_objects[GetSelection()].LabelObj->GetId();
}


bool pxRadioPanel::IsSelected( int idx ) const
{
	if( !VerifyRealizedState() ) return false;
	pxAssert( m_objects[idx].LabelObj != NULL );
	return m_objects[idx].LabelObj->GetValue();
}

pxStaticText* pxRadioPanel::GetSubText( int idx )
{
	if( !VerifyRealizedState() ) return NULL;
	return m_objects[idx].SubTextObj;
}

const pxStaticText* pxRadioPanel::GetSubText( int idx ) const
{
	if( !VerifyRealizedState() ) return NULL;
	return m_objects[idx].SubTextObj;
}

wxRadioButton* pxRadioPanel::GetButton( int idx )
{
	if( !VerifyRealizedState() ) return NULL;
	return m_objects[idx].LabelObj;
}

const wxRadioButton* pxRadioPanel::GetButton( int idx ) const
{
	if( !VerifyRealizedState() ) return NULL;
	return m_objects[idx].LabelObj;
}
