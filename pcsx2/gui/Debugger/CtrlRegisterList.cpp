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
#include "CtrlRegisterList.h"
#include "DebugTools/Debug.h"

#include "DebugEvents.h"
#include "AppConfig.h"

BEGIN_EVENT_TABLE(CtrlRegisterList, wxWindow)
	EVT_PAINT(CtrlRegisterList::paintEvent)
	EVT_LEFT_DOWN(CtrlRegisterList::mouseEvent)
	EVT_RIGHT_DOWN(CtrlRegisterList::mouseEvent)
	EVT_RIGHT_UP(CtrlRegisterList::mouseEvent)
	EVT_MOTION(CtrlRegisterList::mouseEvent)
	EVT_KEY_DOWN(CtrlRegisterList::keydownEvent)
END_EVENT_TABLE()

enum DisassemblyMenuIdentifiers
{
	ID_REGISTERLIST_DISPLAY32 = 1,
	ID_REGISTERLIST_DISPLAY64,
	ID_REGISTERLIST_DISPLAY128,
	ID_REGISTERLIST_CHANGELOWER,
	ID_REGISTERLIST_CHANGEUPPER,
	ID_REGISTERLIST_CHANGEVALUE
};


CtrlRegisterList::CtrlRegisterList(wxWindow* parent, DebugInterface* _cpu)
	: wxWindow(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxWANTS_CHARS|wxBORDER), cpu(_cpu)
{
	rowHeight = g_Conf->EmuOptions.Debugger.FontHeight+2;
	charWidth = g_Conf->EmuOptions.Debugger.FontWidth;
	category = 0;
	maxBits = 128;

	for (int i = 0; i < cpu->getRegisterCategoryCount(); i++)
	{
		int count = cpu->getRegisterCount(i);

		ChangedReg* regs = new ChangedReg[count];
		memset(regs,0,sizeof(ChangedReg)*count);
		changedCategories.push_back(regs);

		int maxLen = 0;
		for (int k = 0; k < cpu->getRegisterCount(i); k++)
		{
			maxLen = std::max<int>(maxLen,strlen(cpu->getRegisterName(i,k)));
		}

		int x = 17+(maxLen+3)*charWidth;
		startPositions.push_back(x);
		currentRows.push_back(0);
	}

	SetDoubleBuffered(true);
	SetInitialSize(ClientToWindowSize(GetMinClientSize()));
}

void CtrlRegisterList::postEvent(wxEventType type, wxString text)
{
   wxCommandEvent event( type, GetId() );
   event.SetEventObject(this);
   event.SetString(text);
   wxPostEvent(this,event);
}

void CtrlRegisterList::postEvent(wxEventType type, int value)
{
   wxCommandEvent event( type, GetId() );
   event.SetEventObject(this);
   event.SetInt(value);
   wxPostEvent(this,event);
}

void CtrlRegisterList::refreshChangedRegs()
{
	if (cpu->getPC() == lastPc)
		return;

	for (size_t cat = 0; cat < changedCategories.size(); cat++)
	{
		ChangedReg* regs = changedCategories[cat];
		int size = cpu->getRegisterSize(category);

		for (int i = 0; i < cpu->getRegisterCount(cat); i++)
		{
			ChangedReg& reg = regs[i];
			memset(&reg.changed,0,sizeof(reg.changed));

			u128 newValue = cpu->getRegister(cat,i);

			if (reg.oldValue != newValue)
			{
				bool changed = false;

				if (size >= 128 && (reg.oldValue._u32[3] != newValue._u32[3] || reg.oldValue._u32[2] != newValue._u32[2]))
				{
					changed = true;
					reg.changed[3] = true;
					reg.changed[2] = true;
				}

				if (size >= 64 && (reg.oldValue._u32[1] != newValue._u32[1] || changed))
				{
					changed = true;
					reg.changed[1] = true;
				}

				if (reg.oldValue._u32[0] != newValue._u32[0] || changed)
				{
					reg.changed[0] = true;
				}

				reg.oldValue = newValue;
			}
		}
	}

	lastPc = cpu->getPC();
}

void CtrlRegisterList::paintEvent(wxPaintEvent & evt)
{
	wxPaintDC dc(this);
	render(dc);
}

void CtrlRegisterList::redraw()
{
	wxClientDC dc(this);
	render(dc);
}

void drawU32Text(wxDC& dc, u32 value, int x, int y)
{
	char str[32];
	sprintf(str,"%08X",value);
	dc.DrawText(wxString(str,wxConvUTF8),x,y);
}

void CtrlRegisterList::render(wxDC& dc)
{
	#ifdef WIN32
	wxFont font = wxFont(wxSize(charWidth,rowHeight-2),wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,false,L"Lucida Console");
	#else
	wxFont font = wxFont(8,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,false,L"Lucida Console");
	font.SetPixelSize(wxSize(charWidth,rowHeight-2));
	#endif
	dc.SetFont(font);
	
	// clear background
	wxColor white = wxColor(0xFFFFFFFF);

	dc.SetBrush(wxBrush(white));
	dc.SetPen(wxPen(white));

	wxSize size = GetSize();
	dc.DrawRectangle(0,0,size.x,size.y);

	refreshChangedRegs();

	wxColor colorChanged = wxColor(0xFF0000FF);
	wxColor colorUnchanged = wxColor(0xFF004000);
	wxColor colorNormal = wxColor(0xFF600000);

	// draw categories
	int piece = size.x/cpu->getRegisterCategoryCount();
	for (int i = 0; i < cpu->getRegisterCategoryCount(); i++)
	{
		const char* name = cpu->getRegisterCategoryName(i);

		int x = i*piece;

		if (i == category)
		{
			dc.SetBrush(wxBrush(wxColor(0xFF70FF70)));
			dc.SetPen(wxPen(wxColor(0xFF000000)));
		} else {
			dc.SetBrush(wxBrush(wxColor(0xFFFFEFE8)));
			dc.SetPen(wxPen(wxColor(0xFF000000)));
		}
		
		dc.DrawRectangle(x-1,-1,piece+1,rowHeight+1);

		// center text
		x += (piece-strlen(name)*charWidth)/2;
		dc.DrawText(wxString(name,wxConvUTF8),x,1);
	}

	int nameStart = 17;
	int valueStart = startPositions[category];

	ChangedReg* changedRegs = changedCategories[category];
	int registerBits = cpu->getRegisterSize(category);
	DebugInterface::RegisterType type = cpu->getRegisterType(category);

	for (int i = 0; i < cpu->getRegisterCount(category); i++)
	{
		int x = valueStart;
		int y = rowHeight*(i+1);

		if (currentRows[category] == i)
		{
			dc.SetBrush(wxBrush(wxColor(0xFFFFEFE8)));
			dc.SetPen(wxPen(wxColor(0xFFFFEFE8)));
			dc.DrawRectangle(0,y,size.x,rowHeight);
		}

		const char* name = cpu->getRegisterName(category,i);
		dc.SetTextForeground(colorNormal);
		dc.DrawText(wxString(name,wxConvUTF8),nameStart,y+2);

		u128 value = cpu->getRegister(category,i);
		ChangedReg& changed = changedRegs[i];

		switch (type)
		{
		case DebugInterface::NORMAL:	// display them in 32 bit parts
			switch (registerBits)
			{
			case 128:
				{
					int startIndex = std::min<int>(3,maxBits/32-1);
					int actualX = size.x-4-(startIndex+1)*(8*charWidth+2);
					x = std::max<int>(actualX,x);

					if (startIndex != 3)
					{
						bool c = false;
						for (int i = 3; i > startIndex; i--)
							c = c || changed.changed[i];

						if (c)
						{
							dc.SetTextForeground(colorChanged);
							dc.DrawText(L"+",x-charWidth,y+2);
						}
					}

					for (int i = startIndex; i >= 0; i--)
					{
						if (changed.changed[i])
							dc.SetTextForeground(colorChanged);
						else
							dc.SetTextForeground(colorUnchanged);

						drawU32Text(dc,value._u32[i],x,y+2);
						x += charWidth*8+2;
					}
					break;
				}
			case 64:
				{
					if (maxBits < 64 && changed.changed[1])
					{
						dc.SetTextForeground(colorChanged);
						dc.DrawText(L"+",x-charWidth,y+2);
					}

					for (int i = 1; i >= 0; i--)
					{
						if (changed.changed[i])
							dc.SetTextForeground(colorChanged);
						else
							dc.SetTextForeground(colorUnchanged);

						drawU32Text(dc,value._u32[i],x,y+2);
						x += charWidth*8+2;
					}
					break;
				}
			case 32:
				{
					if (changed.changed[0])
						dc.SetTextForeground(colorChanged);
					else
						dc.SetTextForeground(colorUnchanged);

					drawU32Text(dc,value._u32[0],x,y+2);
					break;
				}
			}
			break;
		case DebugInterface::SPECIAL:		// let debug interface format them and just display them
			{
				if (changed.changed[0] || changed.changed[1] || changed.changed[2] || changed.changed[3])
					dc.SetTextForeground(colorChanged);
				else
					dc.SetTextForeground(colorUnchanged);

				dc.DrawText(cpu->getRegisterString(category,i),x,y+2);
				break;
			}
		}
	}
}

void CtrlRegisterList::changeValue(RegisterChangeMode mode)
{
	wchar_t str[64];

	u128 oldValue = cpu->getRegister(category,currentRows[category]);

	switch (mode)
	{
	case LOWER64:
		swprintf(str,64,L"0x%016llX",oldValue._u64[0]);
		break;
	case UPPER64:
		swprintf(str,64,L"0x%016llX",oldValue._u64[1]);
		break;
	case CHANGE32:
		swprintf(str,64,L"0x%08X",oldValue._u32[0]);
		break;
	}
	
	u64 newValue;
	if (executeExpressionWindow(this,cpu,newValue,str))
	{
		switch (mode)
		{
		case LOWER64:
			oldValue._u64[0] = newValue;
			break;
		case UPPER64:
			oldValue._u64[1] = newValue; 
			break;
		case CHANGE32:
			oldValue._u32[0] = newValue; 
			break;
		}

		cpu->setRegister(category,currentRows[category],oldValue);
	}
}


void CtrlRegisterList::onPopupClick(wxCommandEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_REGISTERLIST_DISPLAY32:
		maxBits = 32;
		SetInitialSize(ClientToWindowSize(GetMinClientSize()));
		postEvent(debEVT_UPDATELAYOUT,0);
		Refresh();
		break;
	case ID_REGISTERLIST_DISPLAY64:
		maxBits = 64;
		SetInitialSize(ClientToWindowSize(GetMinClientSize()));
		postEvent(debEVT_UPDATELAYOUT,0);
		Refresh();
		break;
	case ID_REGISTERLIST_DISPLAY128:
		maxBits = 128;
		SetInitialSize(ClientToWindowSize(GetMinClientSize()));
		postEvent(debEVT_UPDATELAYOUT,0);
		Refresh();
		break;
	case ID_REGISTERLIST_CHANGELOWER:
		changeValue(LOWER64);
		Refresh();
		break;
	case ID_REGISTERLIST_CHANGEUPPER:
		changeValue(UPPER64);
		Refresh();
		break;
	case ID_REGISTERLIST_CHANGEVALUE:
		if (cpu->getRegisterSize(category) == 32)
			changeValue(CHANGE32);
		else
			changeValue(LOWER64);
		Refresh();
		break;
	default:
		wxMessageBox( L"Unimplemented.",  L"Unimplemented.", wxICON_INFORMATION);
		break;
	}
}

void CtrlRegisterList::setCurrentRow(int row)
{
	char str[256];
	u128 value ;
	wxString text;

	const char* name = cpu->getRegisterName(category,row);

	switch (cpu->getRegisterType(category))
	{
	case DebugInterface::NORMAL:
		value = cpu->getRegister(category,row);
		switch (cpu->getRegisterSize(category))
		{
		case 128:
			sprintf(str,"%s = 0x%08X%08X%08X%08X",name,value._u32[3],value._u32[2],value._u32[1],value._u32[0]);
			break;
		case 64:
			sprintf(str,"%s = 0x%08X%08X",name,value._u32[1],value._u32[0]);
			break;
		case 32:
			sprintf(str,"%s = 0x%08X",name,value._u32[0]);
			break;
		}
		text = wxString(str,wxConvUTF8);
		break;
	case DebugInterface::SPECIAL:
		text = wxString(name,wxConvUTF8) + L" = " + cpu->getRegisterString(category,row);
		break;
	}

	currentRows[category] = row;
	postEvent(debEVT_SETSTATUSBARTEXT,text);
	Refresh();
}

void CtrlRegisterList::mouseEvent(wxMouseEvent& evt)
{
	if (evt.GetEventType() == wxEVT_RIGHT_UP)
	{
		int y = evt.GetPosition().y;

		if (y >= rowHeight)
		{
			int row = (y-rowHeight)/rowHeight;
			if (row != currentRows[category] && row < cpu->getRegisterCount(category))
				setCurrentRow(row);
		}

		wxMenu menu;
		int bits = cpu->getRegisterSize(category);

		menu.AppendRadioItem(ID_REGISTERLIST_DISPLAY32,		L"Display 32 bit");
		menu.AppendRadioItem(ID_REGISTERLIST_DISPLAY64,		L"Display 64 bit");
		menu.AppendRadioItem(ID_REGISTERLIST_DISPLAY128,	L"Display 128 bit");
		menu.AppendSeparator();

		if (bits >= 64)
		{
			menu.Append(ID_REGISTERLIST_CHANGELOWER,	L"Change lower 64 bit");
			menu.Append(ID_REGISTERLIST_CHANGEUPPER,	L"Change upper 64 bit");
		} else {
			menu.Append(ID_REGISTERLIST_CHANGEVALUE,	L"Change value");
		}

		switch (maxBits)
		{
		case 128:
			menu.Check(ID_REGISTERLIST_DISPLAY128,true);
			break;
		case 64:
			menu.Check(ID_REGISTERLIST_DISPLAY64,true);
			break;
		case 32:
			menu.Check(ID_REGISTERLIST_DISPLAY32,true);
			break;
		}

		menu.Connect(wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&CtrlRegisterList::onPopupClick, NULL, this);
		PopupMenu(&menu,evt.GetPosition());
		return;
	}

	if (evt.ButtonIsDown(wxMOUSE_BTN_LEFT) || evt.ButtonIsDown(wxMOUSE_BTN_RIGHT))
	{
		int x = evt.GetPosition().x;
		int y = evt.GetPosition().y;

		if (y < rowHeight)
		{
			int piece = GetSize().x/cpu->getRegisterCategoryCount();
			int cat = x/piece;

			if (cat != category)
			{
				category = cat;
				Refresh();
			}
		} else {
			int row = (y-rowHeight)/rowHeight;
			if (row != currentRows[category] && row < cpu->getRegisterCount(category))
				setCurrentRow(row);
		}

		SetFocus();
		SetFocusFromKbd();
	}
}

void CtrlRegisterList::keydownEvent(wxKeyEvent& evt)
{
	switch (evt.GetKeyCode())
	{
	case WXK_UP:
		setCurrentRow(std::max<int>(currentRows[category]-1,0));
		break;
	case WXK_DOWN:
		setCurrentRow(std::min<int>(currentRows[category]+1,cpu->getRegisterCount(category)));
		break;
	case WXK_TAB:
		category = (category+1) % cpu->getRegisterCategoryCount();
		Refresh();
		break;
	}
}
