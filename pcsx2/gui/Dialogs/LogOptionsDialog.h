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

#include "App.h"

#include "wxHelpers.h"
#include "CheckedStaticBox.h"

namespace Dialogs {

class LogOptionsDialog: public wxDialogWithHelpers
{
public:
	LogOptionsDialog( wxWindow* parent=NULL, int id=DialogId_LogOptions );

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