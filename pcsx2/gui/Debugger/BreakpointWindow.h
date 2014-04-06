/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014  PCSX2 Dev Team
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
#include "DebugTools/DebugInterface.h"
#include "DebugTools/Breakpoints.h"

class BreakpointWindow : public wxDialog
{
public:
	BreakpointWindow( wxWindow* parent, DebugInterface* _cpu );
	void loadFromMemcheck(MemCheck& memcheck);
	void loadFromBreakpoint(BreakPoint& breakpoint);
	void initBreakpoint(u32 _address);
	void addBreakpoint();

	DECLARE_EVENT_TABLE()
protected:
	void onRadioChange(wxCommandEvent& evt);
	void onButtonOk(wxCommandEvent& evt);
private:
	void setDefaultValues();
	bool fetchDialogData();

	DebugInterface* cpu;
	
	wxTextCtrl* editAddress;
	wxTextCtrl* editSize;
	wxRadioButton* radioMemory;
	wxRadioButton* radioExecute;
	wxCheckBox* checkRead;
	wxCheckBox* checkWrite;
	wxCheckBox* checkOnChange;
	wxTextCtrl* editCondition;
	wxCheckBox* checkEnabled;
	wxCheckBox* checkLog;
	wxButton* buttonOk;
	wxButton* buttonCancel;

	bool memory;
	bool read;
	bool write;
	bool enabled;
	bool log;
	bool onChange;
	u32 address;
	u32 size;
	char condition[128];
	PostfixExpression compiledCondition;
};