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

#pragma once

#include "wxHelpers.h"

class CheckedStaticBox : public wxPanel
{
protected:
	wxBoxSizer& m_MasterSizer;

public:
	wxBoxSizer& ThisSizer;		// Boxsizer which holds all child items.
	wxCheckBox& ThisToggle;		// toggle which can enable/disable all child controls

public:
	CheckedStaticBox( wxWindow* parent, int orientation, const wxString& title=wxEmptyString, int id=wxID_ANY );

	void SetValue( bool val );
	bool GetValue() const;

	wxCheckBox& AddCheckBox( const wxString& label, wxWindowID id=wxID_ANY );

public:
	// Event handler for click events for the main checkbox (default behavior: enables/disables all child controls)
	// This function can be overridden to implement custom handling of check enable/disable behavior.
	virtual void MainToggle_Click( wxCommandEvent& evt );
};
