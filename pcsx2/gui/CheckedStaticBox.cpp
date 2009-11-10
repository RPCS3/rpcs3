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
#include "CheckedStaticBox.h"

CheckedStaticBox::CheckedStaticBox( wxWindow* parent, int orientation, const wxString& title, int id ) :
	wxPanelWithHelpers( parent, wxDefaultCoord )
,	m_MasterSizer( *new wxBoxSizer( wxVERTICAL ) )
,	ThisToggle( *new wxCheckBox( this, id, title, wxPoint( 8, 0 ) ) )
,	ThisSizer( *new wxStaticBoxSizer( orientation, this ) )
{
	m_MasterSizer.Add( &ThisToggle );
	m_MasterSizer.Add( &ThisSizer, wxSizerFlags().Expand() );

	// Ensure that the right-side of the static group box isn't too cozy:
	m_MasterSizer.SetMinSize( ThisToggle.GetSize() + wxSize( 32, 0 ) );

	SetSizer( &m_MasterSizer );

	Connect( ThisToggle.GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CheckedStaticBox::MainToggle_Click ) );
}

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
