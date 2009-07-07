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
	m_MasterSizer( *new wxBoxSizer( wxVERTICAL ) ),
	ThisToggle( *new wxCheckBox( this, id, title, wxPoint( 8, 0 ) ) ),
	ThisSizer( *new wxStaticBoxSizer( orientation, this ) )
{
	m_MasterSizer.Add( &ThisToggle );
	m_MasterSizer.Add( &ThisSizer, wxSizerFlags().Expand() );

	// Ensure that the right-side of the static group box isn't too cozy:
	m_MasterSizer.SetMinSize( ThisToggle.GetSize() + wxSize( 32, 0 ) );

	SetSizer( &m_MasterSizer );

	Connect( id, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CheckedStaticBox::MainToggle_Click ) );
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
