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
#include <wx/listctrl.h>
#include "DebugTools/DebugInterface.h"
#include "DebugTools/Breakpoints.h"
#include "CtrlDisassemblyView.h"

struct GenericListViewColumn
{
	wchar_t *name;
	float size;
	int flags;
};

class BreakpointList: public wxListView
{
public:
	BreakpointList(wxWindow* parent, DebugInterface* _cpu, CtrlDisassemblyView* _disassembly);
	void reloadBreakpoints();
	void update();
	DECLARE_EVENT_TABLE()
protected:
	wxString OnGetItemText(long item, long col) const;

	void sizeEvent(wxSizeEvent& evt);
	void keydownEvent(wxKeyEvent& evt);
private:
	int getBreakpointIndex(int itemIndex, bool& isMemory) const;
	int getTotalBreakpointCount();
	void editBreakpoint(int itemIndex);
	void toggleEnabled(int itemIndex);
	void gotoBreakpointAddress(int itemIndex);
	void removeBreakpoint(int itemIndex);
	void postEvent(wxEventType type, int value);

	std::vector<BreakPoint> displayedBreakPoints_;
	std::vector<MemCheck> displayedMemChecks_;
	DebugInterface* cpu;
	CtrlDisassemblyView* disasm;
};
