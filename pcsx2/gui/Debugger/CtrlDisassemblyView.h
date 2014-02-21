#pragma once
#include <wx/wx.h>

#include "DebugTools/DebugInterface.h"
#include "DebugTools/DisassemblyManager.h"

class CtrlDisassemblyView: public wxWindow
{
public:
	CtrlDisassemblyView(wxWindow* parent, DebugInterface* _cpu);

	void mouseEvent(wxMouseEvent& evt);
	void paintEvent(wxPaintEvent & evt);
	void keydownEvent(wxKeyEvent& evt);
	void scrollbarEvent(wxScrollWinEvent& evt);
	void sizeEvent(wxSizeEvent& evt);
	void focusEvent(wxFocusEvent& evt) { Refresh(); };
#ifdef WIN32
	WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);
#endif

	void scanFunctions();
	void clearFunctions() { manager.clear(); };
	void redraw();
	
	u32 getInstructionSizeAt(u32 address)
	{
		u32 start = manager.getStartAddress(address);
		u32 next  = manager.getNthNextAddress(start,1);
		return next-address;
	}

	void gotoAddress(u32 addr);
	void gotoPc() { gotoAddress(cpu->getPC()); };
	void scrollStepping(u32 newPc);
	DECLARE_EVENT_TABLE()
private:
	void drawBranchLine(wxDC& dc, std::map<u32,int>& addressPositions, BranchLine& line);
	void render(wxDC& dc);
	void calculatePixelPositions();
	bool getDisasmAddressText(u32 address, char* dest, bool abbreviateLabels, bool showData);
	u32 yToAddress(int y);
	bool curAddressIsVisible();
	void followBranch();
	void toggleBreakpoint(bool toggleEnabled);
	void updateStatusBarText();
	std::string disassembleRange(u32 start, u32 size);
	void copyInstructions(u32 startAddr, u32 endAddr, bool withDisasm);
	void disassembleToFile();
	void editBreakpoint();

	void postEvent(wxEventType type, wxString text);
	void postEvent(wxEventType type, int value);

	void onPopupClick(wxCommandEvent& evt);

	void setCurAddress(u32 newAddress, bool extend = false)
	{
		newAddress = manager.getStartAddress(newAddress);
		u32 after = manager.getNthNextAddress(newAddress,1);
		curAddress = newAddress;
		selectRangeStart = extend ? std::min(selectRangeStart, newAddress) : newAddress;
		selectRangeEnd = extend ? std::max(selectRangeEnd, after) : after;
		updateStatusBarText();
	}

	void scrollAddressIntoView();
	struct {
		int addressStart;
		int opcodeStart;
		int argumentsStart;
		int arrowsStart;
	} pixelPositions;

	DebugInterface* cpu;
	DisassemblyManager manager;
	u32 windowStart;
	u32 curAddress;
	u32 selectRangeStart;
	u32 selectRangeEnd;
	int visibleRows;
	int rowHeight;
	int charWidth;
	bool displaySymbols;
	std::vector<u32> jumpStack;

	wxIcon bpEnabled,bpDisabled;
	wxMenu menu;
};
