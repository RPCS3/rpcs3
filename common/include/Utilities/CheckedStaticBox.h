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

#include "wxGuiTools.h"

class CheckedStaticBox : public wxPanelWithHelpers
{
	typedef wxPanelWithHelpers _parent;

public:
	wxBoxSizer& ThisSizer;		// Boxsizer which holds all child items.
	wxCheckBox& ThisToggle;		// toggle which can enable/disable all child controls

public:
	CheckedStaticBox( wxWindow* parent, int orientation, const wxString& title=wxEmptyString );

	void SetValue( bool val );
	bool GetValue() const;
	bool Enable( bool enable = true );
	
public:
	virtual void MainToggle_Click( wxCommandEvent& evt );
};
