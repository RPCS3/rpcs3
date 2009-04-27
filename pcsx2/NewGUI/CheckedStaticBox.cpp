/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"
#include "CheckedStaticBox.h"

CheckedStaticBox::CheckedStaticBox( wxWindow* parent, int orientation, const wxString& title, int id ) :
	wxPanel( parent ),
	m_StaticBoxSizer( *new wxStaticBoxSizer( wxVERTICAL, this ) ),
	ThisToggle( *new wxCheckBox( this, id, title, wxPoint( 8, 1 ) ) ),
	ThisSizer( ( orientation != wxVERTICAL ) ? *new wxBoxSizer( orientation ) : m_StaticBoxSizer )
{
	// Note on initializers above: Spacer required!
	// The checkbox uses more room than a standard group box label, so we need to insert some space
	// between the top of the groupbox and the first item.  If the user is wanting a horizontal sizer
	// then we'll need to create a vertical sizer to act as a container for the spacer:

	ThisToggle.SetSize( ThisToggle.GetSize() + wxSize( 8, 0 ) );

	m_StaticBoxSizer.AddSpacer( 7 );
	SetSizer( &m_StaticBoxSizer );

	if( &ThisSizer != &m_StaticBoxSizer )
		m_StaticBoxSizer.Add( &ThisSizer );

	// Ensure that the right-side of the static group box isn't too cozy:
	m_StaticBoxSizer.SetMinSize( ThisToggle.GetSize() + wxSize( 22, 1 ) );

	Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CheckedStaticBox::MainToggle_Click ) );
}

// Adds a checkbox to this group panel's base sizer.
// This is a helper function which saves some typographic red tape over using manual
// checkbox creation and sizer appendage.
wxCheckBox& CheckedStaticBox::AddCheckBox( const wxString& label, wxWindowID id )
{
	return wxHelpers::AddCheckBoxTo( this, ThisSizer, label, id );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
/*BEGIN_EVENT_TABLE(CheckedStaticBox, wxPanel)
	EVT_CHECKBOX(wxID_ANY, MainToggle_Click)
END_EVENT_TABLE()*/

void CheckedStaticBox::MainToggle_Click( wxCommandEvent& evt )
{
	SetValue( evt.IsChecked() );
}

// Sets the main checkbox status, and enables/disables all child controls
// bound to the StaticBox accordingly.
void CheckedStaticBox::SetValue( bool val )
{
	wxWindowList& list = GetChildren();

	for( wxWindowList::iterator iter = list.begin(); iter != list.end(); ++iter)
	{
		wxWindow *current = *iter;
		if( current != &ThisToggle )
			current->Enable( val );
	}
	ThisToggle.SetValue( val );
}

bool CheckedStaticBox::GetValue() const
{
	return ThisToggle.GetValue();
}
