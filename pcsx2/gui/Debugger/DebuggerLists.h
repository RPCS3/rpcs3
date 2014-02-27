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
