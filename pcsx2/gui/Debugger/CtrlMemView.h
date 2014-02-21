#pragma once
#include <wx/wx.h>

#include "DebugTools/DebugInterface.h"
#include "DebugTools/DisassemblyManager.h"

class CtrlMemView: public wxWindow
{
public:
	CtrlMemView(wxWindow* parent, DebugInterface* _cpu);
	
	void paintEvent(wxPaintEvent & evt);
	void mouseEvent(wxMouseEvent& evt);
	void keydownEvent(wxKeyEvent& evt);
	void charEvent(wxKeyEvent& evt);
	void redraw();
	void gotoAddress(u32 address);

	DECLARE_EVENT_TABLE()
private:
	void render(wxDC& dc);
	void gotoPoint(int x, int y);
	void updateStatusBarText();
	void postEvent(wxEventType type, wxString text);
	void postEvent(wxEventType type, int value);
	void scrollWindow(int lines);
	void scrollCursor(int bytes);
	void onPopupClick(wxCommandEvent& evt);
	void focusEvent(wxFocusEvent& evt) { Refresh(); };

	DebugInterface* cpu;
	int rowHeight;
	int charWidth;
	u32 windowStart;
	u32 curAddress;
	int rowSize;
	wxFont font,underlineFont;

	int addressStart;
	int hexStart;
	int asciiStart;
	bool asciiSelected;
	int selectedNibble;

	wxMenu menu;
};