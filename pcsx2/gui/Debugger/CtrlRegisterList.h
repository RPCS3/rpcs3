#pragma once
#include <wx/wx.h>

#include "DebugTools/DebugInterface.h"
#include "DebugTools/DisassemblyManager.h"

class CtrlRegisterList: public wxWindow
{
public:
	CtrlRegisterList(wxWindow* parent, DebugInterface* _cpu);
	
	void paintEvent(wxPaintEvent & evt);
	void mouseEvent(wxMouseEvent& evt);
	void keydownEvent(wxKeyEvent& evt);
	void onPopupClick(wxCommandEvent& evt);
	void redraw();
	DECLARE_EVENT_TABLE()

	virtual wxSize GetMinClientSize() const
	{
		int columnChars = 0;
		int maxWidth = 0;
		int maxRows = 0;

		for (int i = 0; i < cpu->getRegisterCategoryCount(); i++)
		{
			int bits = std::min<u32>(maxBits,cpu->getRegisterSize(i));
			int start = startPositions[i];
			
			int w = start+(bits/4) * charWidth;
			if (bits > 32)
				w += (bits/32)*2-2;

			maxWidth = std::max<int>(maxWidth,w);
			columnChars += strlen(cpu->getRegisterCategoryName(i))+1;
			maxRows = std::max<int>(maxRows,cpu->getRegisterCount(i));
		}

		maxWidth = std::max<int>(columnChars*charWidth,maxWidth+4);

		return wxSize(maxWidth,(maxRows+1)*rowHeight);
	}

	virtual wxSize DoGetBestClientSize() const
	{
		return GetMinClientSize();
	}
private:
	void render(wxDC& dc);
	void refreshChangedRegs();
	void setCurrentRow(int row);

	void postEvent(wxEventType type, wxString text);
	void postEvent(wxEventType type, int value);

	struct ChangedReg
	{
		u128 oldValue;
		bool changed[4];
	};

	std::vector<ChangedReg*> changedCategories;
	std::vector<int> startPositions;
	std::vector<int> currentRows;

	DebugInterface* cpu;
	int rowHeight,charWidth;
	u32 lastPc;
	int category;
	int maxBits;
	wxMenu menu;
};