#include "PrecompiledHeader.h"
#include "CtrlMemView.h"
#include "DebugTools/Debug.h"
#include "AppConfig.h"

#include "BreakpointWindow.h"
#include "DebugEvents.h"
#include <wchar.h>
#include <wx/clipbrd.h>

BEGIN_EVENT_TABLE(CtrlMemView, wxWindow)
	EVT_PAINT(CtrlMemView::paintEvent)
	EVT_MOUSEWHEEL(CtrlMemView::mouseEvent)
	EVT_LEFT_DOWN(CtrlMemView::mouseEvent)
	EVT_LEFT_DCLICK(CtrlMemView::mouseEvent)
	EVT_RIGHT_DOWN(CtrlMemView::mouseEvent)
	EVT_RIGHT_UP(CtrlMemView::mouseEvent)
	EVT_KEY_DOWN(CtrlMemView::keydownEvent)
	EVT_CHAR(CtrlMemView::charEvent)
	EVT_SET_FOCUS(CtrlMemView::focusEvent)
	EVT_KILL_FOCUS(CtrlMemView::focusEvent)
END_EVENT_TABLE()

enum MemoryViewMenuIdentifiers
{
	ID_MEMVIEW_GOTOINDISASM = 1,
	ID_MEMVIEW_COPYADDRESS,
	ID_MEMVIEW_COPYVALUE_8,
	ID_MEMVIEW_COPYVALUE_16,
	ID_MEMVIEW_COPYVALUE_32,
	ID_MEMVIEW_COPYVALUE_64,
	ID_MEMVIEW_COPYVALUE_128,
	ID_MEMVIEW_DUMP,
};

CtrlMemView::CtrlMemView(wxWindow* parent, DebugInterface* _cpu)
	: wxWindow(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxWANTS_CHARS), cpu(_cpu)
{
	rowHeight = g_Conf->EmuOptions.Debugger.FontHeight;
	charWidth = g_Conf->EmuOptions.Debugger.FontWidth;
	windowStart = 0x480000;
	curAddress = windowStart;
	rowSize = 16;
	
	asciiSelected = false;
	selectedNibble = 0;
	rowSize = 16;
	addressStart = charWidth;
	hexStart = addressStart + 9*charWidth;
	asciiStart = hexStart + (rowSize*3+1)*charWidth;

	#ifdef WIN32
	font = wxFont(wxSize(charWidth,rowHeight),wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,false,L"Lucida Console");
	underlineFont = wxFont(wxSize(charWidth,rowHeight),wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,true,L"Lucida Console");
	#else
	font = wxFont(8,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,false,L"Lucida Console");
	font.SetPixelSize(wxSize(charWidth,rowHeight));
	underlineFont = wxFont(8,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,true,L"Lucida Console");
	underlineFont.SetPixelSize(wxSize(charWidth,rowHeight));
	#endif

	menu.Append(ID_MEMVIEW_GOTOINDISASM,		L"Go to in Disasm");
	menu.Append(ID_MEMVIEW_COPYADDRESS,			L"Copy address");
	menu.AppendSeparator();
	menu.Append(ID_MEMVIEW_COPYVALUE_8,			L"Copy Value (8 bit)");
	menu.Append(ID_MEMVIEW_COPYVALUE_16,		L"Copy Value (16 bit)");
	menu.Append(ID_MEMVIEW_COPYVALUE_32,		L"Copy Value (32 bit)");
	menu.Append(ID_MEMVIEW_COPYVALUE_64,		L"Copy Value (64 bit)");
	menu.Append(ID_MEMVIEW_COPYVALUE_128,		L"Copy Value (128 bit)");
	menu.Append(ID_MEMVIEW_DUMP,				L"Dump...");
	menu.Enable(ID_MEMVIEW_DUMP,false);
	menu.Connect(wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&CtrlMemView::onPopupClick, NULL, this);

	SetScrollbar(wxVERTICAL,100,1,201,true);
	SetDoubleBuffered(true);
}

void CtrlMemView::postEvent(wxEventType type, wxString text)
{
   wxCommandEvent event( type, GetId() );
   event.SetEventObject(this);
   event.SetString(text);
   wxPostEvent(this,event);
}

void CtrlMemView::postEvent(wxEventType type, int value)
{
   wxCommandEvent event( type, GetId() );
   event.SetEventObject(this);
   event.SetInt(value);
   wxPostEvent(this,event);
}

void CtrlMemView::paintEvent(wxPaintEvent & evt)
{
	wxPaintDC dc(this);
	render(dc);
}

void CtrlMemView::redraw()
{
	wxClientDC dc(this);
	render(dc);
}

void CtrlMemView::render(wxDC& dc)
{
	bool hasFocus = wxWindow::FindFocus() == this;
	int visibleRows = GetClientSize().y/rowHeight;

	wxColor white = wxColor(0xFFFFFFFF);
	dc.SetBrush(wxBrush(white));
	dc.SetPen(wxPen(white));

	int width,height;
	dc.GetSize(&width,&height);
	dc.DrawRectangle(0,0,width,height);

	for (int i = 0; i < visibleRows+1; i++)
	{
		wchar_t temp[32];

		unsigned int address = windowStart + i*rowSize;
		int rowY = rowHeight*i;

		swprintf(temp,32,L"%08X",address);
		dc.SetFont(font);
		dc.SetTextForeground(wxColor(0xFF600000));
		dc.DrawText(temp,addressStart,rowY);

		u32 memory[4];
		bool valid = cpu != NULL && cpu->isAlive() && cpu->isValidAddress(address);
		if (valid)
		{
			memory[0] = cpu->read32(address);
			memory[1] = cpu->read32(address+4);
			memory[2] = cpu->read32(address+8);
			memory[3] = cpu->read32(address+12);
		}
		
		u8* m = (u8*) memory;
		for (int j = 0; j < rowSize; j++)
		{
			if (valid)
				swprintf(temp,32,L"%02X",m[j]);
			else
				wcscpy(temp,L"??");
			
			unsigned char c = m[j];
			if (c < 32 || c >= 128 || valid == false)
				c = '.';
			
			if (address+j == curAddress)
			{
				wchar_t text[2];

				if (hasFocus && !asciiSelected)
				{
					dc.SetTextForeground(wxColor(0xFFFFFFFF));

					dc.SetPen(wxColor(0xFFFF9933));
					dc.SetBrush(wxColor(0xFFFF9933));
					dc.DrawRectangle(hexStart+j*3*charWidth,rowY,charWidth,rowHeight);

					if (selectedNibble == 0)
						dc.SetFont(underlineFont);
				} else {
					dc.SetTextForeground(wxColor(0xFF000000));
					
					dc.SetPen(wxColor(0xFFC0C0C0));
					dc.SetBrush(wxColor(0xFFC0C0C0));
					dc.DrawRectangle(hexStart+j*3*charWidth,rowY,charWidth,rowHeight);
				}

				text[0] = temp[0];
				text[1] = 0;
				dc.DrawText(text,hexStart+j*3*charWidth,rowY);
				
				if (hasFocus && !asciiSelected)
				{
					dc.DrawRectangle(hexStart+j*3*charWidth+charWidth,rowY,charWidth,rowHeight);

					if (selectedNibble == 1)
						dc.SetFont(underlineFont);
					else
						dc.SetFont(font);
				} else {
					dc.DrawRectangle(hexStart+j*3*charWidth+charWidth,rowY,charWidth,rowHeight);
				}
				
				text[0] = temp[1];
				text[1] = 0;
				dc.DrawText(text,hexStart+j*3*charWidth+charWidth,rowY);

				if (hasFocus && asciiSelected)
				{
					dc.SetTextForeground(wxColor(0xFFFFFFFF));
					
					dc.SetPen(wxColor(0xFFFF9933));
					dc.SetBrush(wxColor(0xFFFF9933));
					dc.DrawRectangle(asciiStart+j*(charWidth+2),rowY,charWidth,rowHeight);
				} else {
					dc.SetTextForeground(wxColor(0xFF000000));
					dc.SetFont(font);

					dc.SetPen(wxColor(0xFFC0C0C0));
					dc.SetBrush(wxColor(0xFFC0C0C0));
					dc.DrawRectangle(asciiStart+j*(charWidth+2),rowY,charWidth,rowHeight);
				}
				
				text[0] = c;
				text[1] = 0;
				dc.DrawText(text,asciiStart+j*(charWidth+2),rowY);
			} else {
				wchar_t text[2];
				text[0] = c;
				text[1] = 0;

				dc.SetTextForeground(wxColor(0xFF000000));
				dc.DrawText(temp,hexStart+j*3*charWidth,rowY);
				dc.DrawText(text,asciiStart+j*(charWidth+2),rowY);
			}
		}
	}
}

void CtrlMemView::onPopupClick(wxCommandEvent& evt)
{
	wchar_t str[64];

	switch (evt.GetId())
	{
	case ID_MEMVIEW_COPYADDRESS:
		if (wxTheClipboard->Open())
		{
			swprintf(str,64,L"%08X",curAddress);
			wxTheClipboard->SetData(new wxTextDataObject(str));
			wxTheClipboard->Close();
		}
		break;
	case ID_MEMVIEW_GOTOINDISASM:
		postEvent(debEVT_GOTOINDISASM,curAddress);
		break;
	case ID_MEMVIEW_COPYVALUE_8:
		if (wxTheClipboard->Open())
		{
			swprintf(str,64,L"%02X",cpu->read8(curAddress));
			wxTheClipboard->SetData(new wxTextDataObject(str));
			wxTheClipboard->Close();
		}
		break;
	case ID_MEMVIEW_COPYVALUE_16:
		if (wxTheClipboard->Open())
		{
			swprintf(str,64,L"%04X",cpu->read16(curAddress));
			wxTheClipboard->SetData(new wxTextDataObject(str));
			wxTheClipboard->Close();
		}
		break;
	case ID_MEMVIEW_COPYVALUE_32:
		if (wxTheClipboard->Open())
		{
			swprintf(str,64,L"%08X",cpu->read32(curAddress));
			wxTheClipboard->SetData(new wxTextDataObject(str));
			wxTheClipboard->Close();
		}
		break;
	case ID_MEMVIEW_COPYVALUE_64:
		if (wxTheClipboard->Open())
		{
			swprintf(str,64,L"%016llX",cpu->read64(curAddress));
			wxTheClipboard->SetData(new wxTextDataObject(str));
			wxTheClipboard->Close();
		}
		break;
	case ID_MEMVIEW_COPYVALUE_128:
		if (wxTheClipboard->Open())
		{
			u128 value = cpu->read128(curAddress);
			swprintf(str,64,L"%016llX%016llX",value._u64[1],value._u64[0]);
			wxTheClipboard->SetData(new wxTextDataObject(str));
			wxTheClipboard->Close();
		}
		break;
	}
}

void CtrlMemView::mouseEvent(wxMouseEvent& evt)
{
	// left button
	if (evt.GetEventType() == wxEVT_LEFT_DOWN || evt.GetEventType() == wxEVT_LEFT_DCLICK
		|| evt.GetEventType() == wxEVT_RIGHT_DOWN || evt.GetEventType() == wxEVT_RIGHT_DCLICK)
	{
		gotoPoint(evt.GetPosition().x,evt.GetPosition().y);
		SetFocus();
		SetFocusFromKbd();
	} else if (evt.GetEventType() == wxEVT_RIGHT_UP)
	{
		menu.Enable(ID_MEMVIEW_COPYVALUE_128,(curAddress & 15) == 0);
		menu.Enable(ID_MEMVIEW_COPYVALUE_64,(curAddress & 7) == 0);
		menu.Enable(ID_MEMVIEW_COPYVALUE_32,(curAddress & 3) == 0);
		menu.Enable(ID_MEMVIEW_COPYVALUE_16,(curAddress & 1) == 0);

		PopupMenu(&menu);
		return;
	} else if (evt.GetEventType() == wxEVT_MOUSEWHEEL)
	{
		if (evt.GetWheelRotation() > 0)
		{
			scrollWindow(-3);
		} else if (evt.GetWheelRotation() < 0) {
			scrollWindow(3);
		}
	} else {
		evt.Skip();
		return;
	}

	redraw();
}

void CtrlMemView::keydownEvent(wxKeyEvent& evt)
{
	if (evt.ControlDown())
	{
		switch (evt.GetKeyCode())
		{
		case 'g':
		case 'G':
			{
				u64 addr;
				if (executeExpressionWindow(this,cpu,addr) == false)
					return;
				gotoAddress(addr);
			}
			break;
		case 'b':
		case 'B':
			{
				BreakpointWindow bpw(this,cpu);
				if (bpw.ShowModal() == wxID_OK)
				{
					bpw.addBreakpoint();
					postEvent(debEVT_UPDATE,0);
				}
			}
			break;
		default:
			evt.Skip();
			break;
		}
		return;
	}

	switch (evt.GetKeyCode())
	{
	case WXK_LEFT:
		scrollCursor(-1);
		break;
	case WXK_RIGHT:
		scrollCursor(1);
		break;
	case WXK_UP:
		scrollCursor(-rowSize);
		break;
	case WXK_DOWN:
		scrollCursor(rowSize);
		break;
	case WXK_PAGEUP:
		scrollWindow(-GetClientSize().y/rowHeight);
		break;
	case WXK_PAGEDOWN:
		scrollWindow(GetClientSize().y/rowHeight);
		break;
	default:
		evt.Skip();
		break;
	}
}

void CtrlMemView::charEvent(wxKeyEvent& evt)
{
	if (evt.GetKeyCode() < 32)
		return;

	if (!cpu->isValidAddress(curAddress))
	{
		scrollCursor(1);
		return;
	}

	bool active = !cpu->isCpuPaused();
	if (active)
		cpu->pauseCpu();

	if (asciiSelected)
	{
		u8 newValue = evt.GetKeyCode();
		cpu->write8(curAddress,newValue);
		scrollCursor(1);
	} else {
		u8 key = tolower(evt.GetKeyCode());
		int inputValue = -1;

		if (key >= '0' && key <= '9') inputValue = key - '0';
		if (key >= 'a' && key <= 'f') inputValue = key -'a' + 10;

		if (inputValue >= 0)
		{
			int shiftAmount = (1-selectedNibble)*4;

			u8 oldValue = cpu->read8(curAddress);
			oldValue &= ~(0xF << shiftAmount);
			u8 newValue = oldValue | (inputValue << shiftAmount);
			cpu->write8(curAddress,newValue);
			scrollCursor(1);
		}
	}

	if (active)
		cpu->resumeCpu();
	redraw();
}

void CtrlMemView::scrollWindow(int lines)
{
	windowStart += lines*rowSize;
	curAddress += lines*rowSize;
	redraw();
}

void CtrlMemView::scrollCursor(int bytes)
{
	if (!asciiSelected && bytes == 1)
	{
		if (selectedNibble == 0)
		{
			selectedNibble = 1;
			bytes = 0;
		} else {
			selectedNibble = 0;
		}
	} else if (!asciiSelected && bytes == -1)
	{
		if (selectedNibble == 0)
		{
			selectedNibble = 1;
		} else {
			selectedNibble = 0;
			bytes = 0;
		}
	} 

	curAddress += bytes;
	
	int visibleRows = GetClientSize().y/rowHeight;
	u32 windowEnd = windowStart+visibleRows*rowSize;
	if (curAddress < windowStart)
	{
		windowStart = curAddress & ~15;
	} else if (curAddress >= windowEnd)
	{
		windowStart = (curAddress-(visibleRows-1)*rowSize) & ~15;
	}
	
	updateStatusBarText();
	redraw();
}

void CtrlMemView::updateStatusBarText()
{
	wchar_t text[64];
	swprintf(text,64,L"%08X",curAddress);
	postEvent(debEVT_SETSTATUSBARTEXT,text);
}

void CtrlMemView::gotoAddress(u32 addr)
{	
	int lines= GetClientSize().y/rowHeight;
	u32 windowEnd = windowStart+lines*rowSize;

	curAddress = addr;
	selectedNibble = 0;

	if (curAddress < windowStart || curAddress >= windowEnd)
	{
		windowStart = curAddress & ~15;
	}

	updateStatusBarText();
	redraw();
}

void CtrlMemView::gotoPoint(int x, int y)
{
	int line = y/rowHeight;
	int lineAddress = windowStart+line*rowSize;

	if (x >= asciiStart)
	{
		int col = (x-asciiStart) / (charWidth+2);
		if (col >= rowSize) return;
		
		asciiSelected = true;
		curAddress = lineAddress+col;
		selectedNibble = 0;
		updateStatusBarText();
		redraw();
	} else if (x >= hexStart)
	{
		int col = (x-hexStart) / charWidth;
		if ((col/3) >= rowSize) return;

		switch (col % 3)
		{
		case 0: selectedNibble = 0; break;
		case 1: selectedNibble = 1; break;
		case 2: return;		// don't change position when clicking on the space
		}

		asciiSelected = false;
		curAddress = lineAddress+col/3;
		updateStatusBarText();
		redraw();
	}
}
