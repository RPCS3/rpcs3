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