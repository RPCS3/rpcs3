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
 
#include <wx/wx.h>
#include <wx/image.h>

#include "wxHelpers.h"
#include "CheckedStaticBox.h"

namespace Dialogs {

class LogOptionsDialog: public wxDialogWithHelpers
{
public:
	LogOptionsDialog( wxWindow* parent, int id, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize );

protected:
	
	// ----------------------------------------------------------------------------
	class iopLogOptionsPanel : public CheckedStaticBox
	{
	public:
		iopLogOptionsPanel( wxWindow* parent );
		void OnLogChecked(wxCommandEvent &event);

	};
	
	// ----------------------------------------------------------------------------
	class eeLogOptionsPanel : public CheckedStaticBox
	{
	public:
		eeLogOptionsPanel( wxWindow* parent );
		void OnLogChecked(wxCommandEvent &event);

	protected:
		class DisasmPanel : public CheckedStaticBox
		{
		public:
			DisasmPanel( wxWindow* parent );
			void OnLogChecked(wxCommandEvent &event);
		};

		class HwPanel : public CheckedStaticBox
		{
		public:
			HwPanel( wxWindow* parent );
			void OnLogChecked(wxCommandEvent &event);
		};
	};


public:
	void LogChecked(wxCommandEvent &event);
	
protected:
};

};	// end namespace Dialogs