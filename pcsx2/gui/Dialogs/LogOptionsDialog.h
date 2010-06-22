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

#include "App.h"
#include "ConfigurationDialog.h"

#include "Utilities/wxGuiTools.h"
#include "Utilities/CheckedStaticBox.h"

namespace Dialogs {

class LogOptionsDialog : public BaseApplicableDialog
{
public:
	LogOptionsDialog( wxWindow* parent=NULL );
	virtual ~LogOptionsDialog() throw() { }

	static wxString GetNameStatic() { return L"TraceLogSettings"; }
	wxString GetDialogName() const { return GetNameStatic(); }

protected:
	void OnOk_Click( wxCommandEvent& evt );
	void OnApply_Click( wxCommandEvent& evt );
};

}	// end namespace Dialogs
