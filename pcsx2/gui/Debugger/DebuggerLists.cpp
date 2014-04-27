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

#include "PrecompiledHeader.h"
#include "DebuggerLists.h"
#include "BreakpointWindow.h"
#include "DebugEvents.h"

void insertListViewColumns(wxListCtrl* list, GenericListViewColumn* columns, int count)
{
	int totalWidth = list->GetSize().x;

	for (int i = 0; i < count; i++)
	{
		wxListItem column;
		column.SetText(columns[i].name);
		column.SetWidth(totalWidth * columns[i].size);

		list->InsertColumn(i,column);
	}
}

void resizeListViewColumns(wxListCtrl* list, GenericListViewColumn* columns, int count, int totalWidth)
{
	for (int i = 0; i < count; i++)
	{
		list->SetColumnWidth(i,totalWidth*columns[i].size);
	}
}

//
// BreakpointList
//

BEGIN_EVENT_TABLE(BreakpointList, wxWindow)
	EVT_SIZE(BreakpointList::sizeEvent)
	EVT_KEY_DOWN(BreakpointList::keydownEvent)
END_EVENT_TABLE()

enum { BPL_TYPE, BPL_OFFSET, BPL_SIZELABEL, BPL_OPCODE, BPL_CONDITION, BPL_HITS, BPL_ENABLED, BPL_COLUMNCOUNT };

GenericListViewColumn breakpointColumns[BPL_COLUMNCOUNT] = {
	{ L"Type",			0.12f },
	{ L"Offset",		0.12f },
	{ L"Size/Label",	0.18f },
	{ L"Opcode",		0.28f },
	{ L"Condition",		0.17f },
	{ L"Hits",			0.05f },
	{ L"Enabled",		0.08f }
};

BreakpointList::BreakpointList(wxWindow* parent, DebugInterface* _cpu, CtrlDisassemblyView* _disassembly)
	: wxListView(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxLC_VIRTUAL|wxLC_REPORT|wxLC_SINGLE_SEL|wxNO_BORDER), cpu(_cpu), disasm(_disassembly)
{
#ifdef __LINUX__
	// On linux wx failed to resize properly the page. I don't know why so for the moment I just create a static size page
	// Far from ideal but at least I can use the memory window!
	this->SetSize(wxSize(1000, 200));
#endif
	insertListViewColumns(this,breakpointColumns,BPL_COLUMNCOUNT);
}

void BreakpointList::sizeEvent(wxSizeEvent& evt)
{
	resizeListViewColumns(this,breakpointColumns,BPL_COLUMNCOUNT,evt.GetSize().x);
}

void BreakpointList::update()
{
	int newRows = getTotalBreakpointCount();
	SetItemCount(newRows);
	Refresh();
}

int BreakpointList::getTotalBreakpointCount()
{
	int count = (int)CBreakPoints::GetMemChecks().size();
	for (size_t i = 0; i < CBreakPoints::GetBreakpoints().size(); i++)
	{
		if (!displayedBreakPoints_[i].temporary) count++;
	}

	return count;
}

wxString BreakpointList::OnGetItemText(long item, long col) const
{
	wchar_t dest[256];
	bool isMemory;
	int index = getBreakpointIndex(item,isMemory);
	if (index == -1) return L"Invalid";
		
	switch (col)
	{
	case BPL_TYPE:
		{
			if (isMemory) {
				switch ((int)displayedMemChecks_[index].cond) {
				case MEMCHECK_READ:
					wcscpy(dest,L"Read");
					break;
				case MEMCHECK_WRITE:
					wcscpy(dest,L"Write");
					break;
				case MEMCHECK_READWRITE:
					wcscpy(dest,L"Read/Write");
					break;
				case MEMCHECK_WRITE | MEMCHECK_WRITE_ONCHANGE:
					wcscpy(dest,L"Write Change");
					break;
				case MEMCHECK_READWRITE | MEMCHECK_WRITE_ONCHANGE:
					wcscpy(dest,L"Read/Write Change");
					break;
				}
			} else {
				wcscpy(dest,L"Execute");
			}
		}
		break;
	case BPL_OFFSET:
		{
			if (isMemory) {
				swprintf(dest,256,L"0x%08X",displayedMemChecks_[index].start);
			} else {
				swprintf(dest,256,L"0x%08X",displayedBreakPoints_[index].addr);
			}
		}
		break;
	case BPL_SIZELABEL:
		{
			if (isMemory) {
				auto mc = displayedMemChecks_[index];
				if (mc.end == 0)
					swprintf(dest,256,L"0x%08X",1);
				else
					swprintf(dest,256,L"0x%08X",mc.end-mc.start);
			} else {
				const std::string sym = symbolMap.GetLabelString(displayedBreakPoints_[index].addr);
				if (!sym.empty())
				{
#ifdef __LINUX__
					swprintf(dest,256,L"%s",sym.c_str());
#else
					swprintf(dest,256,L"%S",sym.c_str());
#endif
				} else {
					wcscpy(dest,L"-");
				}
			}
		}
		break;
	case BPL_OPCODE:
		{
			if (isMemory) {
				wcscpy(dest,L"-");
			} else {
				char temp[256];
				disasm->getOpcodeText(displayedBreakPoints_[index].addr, temp);
#ifdef __LINUX__
				swprintf(dest,256,L"%s",temp);
#else
				swprintf(dest,256,L"%S",temp);
#endif
			}
		}
		break;
	case BPL_CONDITION:
		{
			if (isMemory || displayedBreakPoints_[index].hasCond == false) {
				wcscpy(dest,L"-");
			} else {
#ifdef __LINUX__
				swprintf(dest,256,L"%s",displayedBreakPoints_[index].cond.expressionString);
#else
				swprintf(dest,256,L"%S",displayedBreakPoints_[index].cond.expressionString);
#endif
			}
		}
		break;
	case BPL_HITS:
		{
			if (isMemory) {
				swprintf(dest,256,L"%d",displayedMemChecks_[index].numHits);
			} else {
				swprintf(dest,256,L"-");
			}
		}
		break;
	case BPL_ENABLED:
		{
			if (isMemory) {
#ifdef __LINUX__
				swprintf(dest,256,L"%s",displayedMemChecks_[index].cond & MEMCHECK_BREAK ? "true" : "false");
#else
				swprintf(dest,256,L"%S",displayedMemChecks_[index].cond & MEMCHECK_BREAK ? "true" : "false");
#endif
			} else {
#ifdef __LINUX__
				swprintf(dest,256,L"%s",displayedBreakPoints_[index].enabled ? "true" : "false");
#else
				swprintf(dest,256,L"%S",displayedBreakPoints_[index].enabled ? "true" : "false");
#endif
			}
		}
		break;
	default:
		return L"Invalid";
	}

	return dest;
}

void BreakpointList::keydownEvent(wxKeyEvent& evt)
{
	int sel = GetFirstSelected();
	switch (evt.GetKeyCode())
	{
	case WXK_DELETE:
		if (sel+1 == GetItemCount())
			Select(sel-1);
		removeBreakpoint(sel);
		break;
	case WXK_UP:
		if (sel > 0)
			Select(sel-1);
		break;
	case WXK_DOWN:
		if (sel+1 < GetItemCount())
			Select(sel+1);
		break;
	case WXK_RETURN:
		editBreakpoint(sel);
		break;
	case WXK_SPACE:
		toggleEnabled(sel);
		break;
	}
}

int BreakpointList::getBreakpointIndex(int itemIndex, bool& isMemory) const
{
	// memory breakpoints first
	if (itemIndex < (int)displayedMemChecks_.size())
	{
		isMemory = true;
		return itemIndex;
	}

	itemIndex -= (int)displayedMemChecks_.size();

	size_t i = 0;
	while (i < displayedBreakPoints_.size())
	{
		if (displayedBreakPoints_[i].temporary)
		{
			i++;
			continue;
		}

		// the index is 0 when there are no more breakpoints to skip
		if (itemIndex == 0)
		{
			isMemory = false;
			return (int)i;
		}

		i++;
		itemIndex--;
	}

	return -1;
}

void BreakpointList::reloadBreakpoints()
{
	// Update the items we're displaying from the debugger.
	displayedBreakPoints_ = CBreakPoints::GetBreakpoints();
	displayedMemChecks_= CBreakPoints::GetMemChecks();
	update();
}

void BreakpointList::editBreakpoint(int itemIndex)
{
	bool isMemory;
	int index = getBreakpointIndex(itemIndex, isMemory);
	if (index == -1) return;

	BreakpointWindow win(this,cpu);
	if (isMemory)
	{
		auto mem = displayedMemChecks_[index];
		win.loadFromMemcheck(mem);
		if (win.ShowModal() == wxID_OK)
		{
			CBreakPoints::RemoveMemCheck(mem.start,mem.end);
			win.addBreakpoint();
		}
	} else {
		auto bp = displayedBreakPoints_[index];
		win.loadFromBreakpoint(bp);
		if (win.ShowModal() == wxID_OK)
		{
			CBreakPoints::RemoveBreakPoint(bp.addr);
			win.addBreakpoint();
		}
	}
}

void BreakpointList::toggleEnabled(int itemIndex)
{
	bool isMemory;
	int index = getBreakpointIndex(itemIndex, isMemory);
	if (index == -1) return;

	if (isMemory) {
		MemCheck mcPrev = displayedMemChecks_[index];
		CBreakPoints::ChangeMemCheck(mcPrev.start, mcPrev.end, mcPrev.cond, MemCheckResult(mcPrev.result ^ MEMCHECK_BREAK));
	} else {
		BreakPoint bpPrev = displayedBreakPoints_[index];
		CBreakPoints::ChangeBreakPoint(bpPrev.addr, !bpPrev.enabled);
	}
}

void BreakpointList::gotoBreakpointAddress(int itemIndex)
{
	bool isMemory;
	int index = getBreakpointIndex(itemIndex,isMemory);
	if (index == -1) return;

	if (isMemory)
	{
		u32 address = displayedMemChecks_[index].start;
		postEvent(debEVT_GOTOINMEMORYVIEW,address);
	} else {
		u32 address = displayedBreakPoints_[index].addr;
		postEvent(debEVT_GOTOINDISASM,address);
	}
}

void BreakpointList::removeBreakpoint(int itemIndex)
{
	bool isMemory;
	int index = getBreakpointIndex(itemIndex,isMemory);
	if (index == -1) return;

	if (isMemory)
	{
		auto mc = displayedMemChecks_[index];
		CBreakPoints::RemoveMemCheck(mc.start, mc.end);
	} else {
		u32 address = displayedBreakPoints_[index].addr;
		CBreakPoints::RemoveBreakPoint(address);
	}
}

void BreakpointList::postEvent(wxEventType type, int value)
{
   wxCommandEvent event( type, GetId() );
   event.SetEventObject(this);
   event.SetInt(value);
   wxPostEvent(this,event);
}