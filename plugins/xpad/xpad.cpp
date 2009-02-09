/* 
 *	Copyright (C) 2007 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "xpad.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

BEGIN_MESSAGE_MAP(xpadApp, CWinApp)
END_MESSAGE_MAP()

xpadApp::xpadApp()
{
}

xpadApp theApp;

BOOL xpadApp::InitInstance()
{
	__super::InitInstance();

	SetRegistryKey(_T("Gabest"));

	return TRUE;
}

//

#define PS2E_LT_PAD 0x02
#define PS2E_PAD_VERSION 0x0002

EXPORT_C_(UINT32) PS2EgetLibType()
{
	return PS2E_LT_PAD;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	return "XPad";
}

EXPORT_C_(UINT32) PS2EgetLibVersion2(UINT32 type)
{
	const UINT32 revision = 0;
	const UINT32 build = 1;
	const UINT32 minor = 0;

	return (build << 0) | (revision << 8) | (PS2E_PAD_VERSION << 16) | (minor << 24);
}

//

struct XPadButton
{
	enum 
	{
		Select = 0x0001,
		L3 = 0x0002,
		R3 = 0x0004,
		Start = 0x0008,
		Up = 0x0010,
		Right = 0x0020,
		Down = 0x0040,
		Left = 0x0080,
		L2 = 0x0100,
		R2 = 0x0200,
		L1 = 0x0400,
		R1 = 0x0800,
		Triangle = 0x1000,
		Circle = 0x2000,
		Cross = 0x4000,
		Square = 0x8000,
	};
};

class XPad
{
public:
	int m_pad;
	bool m_connected;
	bool m_ds2native;
	bool m_analog;
	bool m_locked;
	bool m_vibration;
	BYTE m_small;
	BYTE m_large;
	WORD m_buttons;
	struct {BYTE x, y;} m_left;
	struct {BYTE x, y;} m_right;

	void SetButton(WORD buttons, WORD mask, int flag)
	{
		if(buttons & mask)
		{
			m_buttons &= ~flag;
		}
		else 
		{
			m_buttons |= flag;
		}
	}

	void SetAnalog(short src, BYTE& dst, short deadzone)
	{
		if(abs(src) < deadzone) src = 0;

		dst = (src >> 8) + 128;
	}

public:
	XPad(int pad) 
		: m_pad(pad)
		, m_connected(false)
		, m_ds2native(false)
		, m_analog(false)
		, m_locked(false)
		, m_vibration(true)
		, m_small(0)
		, m_large(0)
		, m_buttons(0xffff)
	{
	}

	virtual ~XPad()
	{
	}

	BYTE GetId()
	{
		return m_analog ? (m_ds2native ? 0x79 : 0x73) : 0x41;
	}

	BYTE ReadData(int index)
	{
		if(index == 0)
		{
			XINPUT_STATE state;

			memset(&state, 0, sizeof(state));

			m_connected = SUCCEEDED(XInputGetState(m_pad, &state));

			if(m_connected)
			{
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_BACK, XPadButton::Select);
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_LEFT_THUMB, XPadButton::L3);
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_RIGHT_THUMB, XPadButton::R3);
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_START, XPadButton::Start);
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_UP, XPadButton::Up);
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT, XPadButton::Right);
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_DOWN, XPadButton::Down);
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_LEFT, XPadButton::Left);
				SetButton(state.Gamepad.bLeftTrigger, 0xe0, XPadButton::L2);
				SetButton(state.Gamepad.bRightTrigger, 0xe0, XPadButton::R2);
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER, XPadButton::L1);
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER, XPadButton::R1);
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_Y, XPadButton::Triangle);
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_B, XPadButton::Circle);
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_A, XPadButton::Cross);
				SetButton(state.Gamepad.wButtons, XINPUT_GAMEPAD_X, XPadButton::Square);

				SetAnalog(state.Gamepad.sThumbLX, m_left.x, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
				SetAnalog(~state.Gamepad.sThumbLY, m_left.y, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
				SetAnalog(state.Gamepad.sThumbRX, m_right.x, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
				SetAnalog(~state.Gamepad.sThumbRY, m_right.y, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			}
			else
			{
				m_buttons = 0xffff;
				m_left.x = 128;
				m_left.y = 128;
				m_right.x = 128;
				m_right.y = 128;
			}
		}

		if(index == 1)
		{
			if(m_connected)
			{
				XINPUT_VIBRATION vibraton;

				memset(&vibraton, 0, sizeof(vibraton));

				if(m_vibration && (m_small || m_large))
				{
					vibraton.wLeftMotorSpeed = m_large << 8;
					vibraton.wRightMotorSpeed = m_small << 8;
				}
				
				XInputSetState(m_pad, &vibraton);
			}
		}

		switch(index)
		{
		case 0:
			return (BYTE)(m_buttons & 0xff);
		case 1:
			return (BYTE)(m_buttons >> 8);
		case 2:
			return m_right.x;
		case 3:
			return m_right.y;
		case 4:
			return m_left.x;
		case 5:
			return m_left.y;
		}

		return 0xff;
	}
};

static class XPadPlugin
{
	CAtlArray<XPad*> m_pads;
	XPad* m_pad;
	int m_index;
	bool m_cfgreaddata;
	BYTE m_unknown1;
	BYTE m_unknown3;

	typedef BYTE (XPadPlugin::*CommandHandler)(int, BYTE);

	CommandHandler m_handlers[256];
	CommandHandler m_handler;

	BYTE DS2Enabler(int index, BYTE value)
	{
		switch(index)
		{
		case 2: 
			return 0x02;
		case 5: 
			return 'Z';
		}

		return 0;
	}

	BYTE QueryDS2AnalogMode(int index, BYTE value)
	{
		if(m_pad->m_ds2native)
		{
			switch(index)
			{
			case 0: 
				return 0xff;
			case 1: 
				return 0xff;
			case 2: 
				return 3;
			case 3: 
				return 0;
			case 4: 
				return 0;
			case 5: 
				return 'Z';
			}
		}

		return 0;
	}

	BYTE ReadDataAndVibrate(int index, BYTE value)
	{
		switch(index)
		{
		case 0: 
			m_pad->m_small = value == 1 ? 128 : 0; // RE4 map menu, value = 2
			break;
		case 1: 
			m_pad->m_large = value;
			break;
		}

		return m_pad->ReadData(index);
	}

	BYTE ConfigMode(int index, BYTE value)
	{
		switch(index)
		{
		case 0: 
			switch(value)
			{
			case 0:
				m_cfgreaddata = false;
				break;
			case 1:
				m_cfgreaddata = true;
				break;
			}
			break;
		}

		if(m_cfgreaddata)
		{
			return m_pad->ReadData(index);
		}

		return 0;
	}

	BYTE SetModeAndLock(int index, BYTE value)
	{
		switch(index)
		{
		case 0: 
			// if(!m_pad->m_locked) ? 
			m_pad->m_analog = value == 1;
			break;
		case 1: 
			m_pad->m_locked = value == 3;
			break;
		}

		return 0;
	}

	BYTE QueryModelAndMode(int index, BYTE value)
	{
		switch(index)
		{
		case 0: 
			return 3;
		case 1: 
			return 2;
		case 2:
			return m_pad->m_analog ? 1 : 0;
		case 3:
			return m_pad->m_ds2native ? 1 : 2;
		case 4:
			return 1;
		}

		return 0;
	}

	BYTE QueryUnknown1(int index, BYTE value)
	{
		switch(index)
		{
		case 0: 
			m_unknown1 = value;
			return 0;
		case 1:
			return 0;
		case 2:
			return 1;
		case 3:
			return m_unknown1 ? 0x01 : 0x02;
		case 4:
			return m_unknown1 ? 0x01 : 0x00;
		case 5:
			return m_unknown1 ? 0x14 : 0x0a;
		}

		return 0;
	}

	BYTE QueryUnknown2(int index, BYTE value)
	{
		switch(index)
		{
		case 2:
			return 2;
		case 4:
			return 1;
		}

		return 0;
	}

	BYTE QueryUnknown3(int index, BYTE value)
	{
		switch(index)
		{
		case 0: 
			m_unknown3 = value;
			return 0;
		case 3:
			return m_unknown3 ? 0x07 : 0x04;
		}

		return 0;
	}

	BYTE ConfigVibration(int index, BYTE value)
	{
		switch(index)
		{
		case 0: 
			return value;
		case 1: 
			m_pad->m_vibration = value == 1;
			return value;
		}

		return 0xff;
	}

	BYTE SetDS2NativeMode(int index, BYTE value)
	{
		switch(index)
		{
		case 5: 
			m_pad->m_ds2native = true;
			return 'Z';
		}

		return 0;
	}

	BYTE m_cmd;

	BYTE UnknownCommand(int index, BYTE value)
	{
		TRACE(_T("Unknown command %02x (%d, %02x)\n"), m_cmd, index, value);

		return 0;
	}

public:

	XPadPlugin()
		: m_pad(NULL)
		, m_index(-1)
		, m_cfgreaddata(false)
		, m_handler(NULL)
	{
		m_pads.Add(new XPad(0));
		m_pads.Add(new XPad(1));

		for(int i = 0; i < countof(m_handlers); i++)
		{
			m_handlers[i] = &XPadPlugin::UnknownCommand;
		}

		m_handlers['@'] = &XPadPlugin::DS2Enabler;
		m_handlers['A'] = &XPadPlugin::QueryDS2AnalogMode;
		m_handlers['B'] = &XPadPlugin::ReadDataAndVibrate;
		m_handlers['C'] = &XPadPlugin::ConfigMode;
		m_handlers['D'] = &XPadPlugin::SetModeAndLock;
		m_handlers['E'] = &XPadPlugin::QueryModelAndMode;
		m_handlers['F'] = &XPadPlugin::QueryUnknown1;
		m_handlers['G'] = &XPadPlugin::QueryUnknown2;
		m_handlers['L'] = &XPadPlugin::QueryUnknown3;
		m_handlers['M'] = &XPadPlugin::ConfigVibration;
		m_handlers['O'] = &XPadPlugin::SetDS2NativeMode;
	}

	void StartPoll(int pad)
	{
		m_pad = m_pads[pad & 1];
		m_index = 0;
	}

	BYTE Poll(BYTE value)
	{
		BYTE ret = 0;

		switch(++m_index)
		{
		case 1:
			m_cmd = value;
			m_handler = m_handlers[value];
			ret = (value == 'B' || value == 'C') ? m_pad->GetId() : 0xf3;
			break;
		case 2:
			ASSERT(value == 0);
			ret = 'Z';
			break;
		default:
			ret = (this->*m_handler)(m_index - 3, value);
			break;
		}
/*
//if(m_cmd != 'B')
{
static FILE* log = NULL;
if(log == NULL)
{
	log = fopen("c:\\log.txt", "w");
}
if(m_index == 1) fprintf(log, "\n");
fprintf(log, "*** %02x %d %02x %02x\n", m_cmd, m_index - 1, value, ret);
fflush(log);
}
*/
		return ret;
	}

} s_padplugin;

//

static int s_nRefs = 0;
static HWND s_hWnd = NULL;
static WNDPROC s_GSWndProc = NULL;
static KeyEvent s_event = {0, 0};

LRESULT WINAPI PADwndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_KEYDOWN:
		if(lParam & 0x40000000) return TRUE;
        s_event.event = KEYPRESS;
        s_event.key = wParam;
		return TRUE;
	case WM_KEYUP:
        s_event.event = KEYRELEASE;
        s_event.key = wParam;
		return TRUE;
	case WM_DESTROY:
	case WM_QUIT:
		s_event.event = KEYPRESS;
		s_event.key = VK_ESCAPE;
		break;
	}

	return s_GSWndProc(hWnd, msg, wParam, lParam);
}

//

EXPORT_C_(UINT32) PADinit(UINT32 flags)
{
	return 0;
}

EXPORT_C PADshutdown()
{
}

EXPORT_C_(UINT32) PADopen(void* pDsp)
{
	XInputEnable(TRUE);

	if(s_nRefs == 0)
	{
		s_hWnd = *(HWND*)pDsp;
		s_GSWndProc = (WNDPROC)SetWindowLongPtr(s_hWnd, GWLP_WNDPROC, (LPARAM)PADwndProc);
	}

	s_nRefs++;

	return 0;
}

EXPORT_C PADclose()
{
	s_nRefs--;

	if(s_nRefs == 0)
	{
		SetWindowLongPtr(s_hWnd, GWLP_WNDPROC, (LPARAM)s_GSWndProc);
	}

	XInputEnable(FALSE);
}

EXPORT_C_(UINT32) CALLBACK PADquery()
{
	return 3;
}

EXPORT_C_(BYTE) PADstartPoll(int pad)
{
	s_padplugin.StartPoll(pad - 1);

	return 0xff;
}

EXPORT_C_(BYTE) PADpoll(BYTE value)
{
	return s_padplugin.Poll(value);
}

EXPORT_C_(KeyEvent*) PADkeyEvent()
{
	static KeyEvent event;
	event = s_event;
	s_event.event = 0;
	return &event;
}

EXPORT_C PADconfigure()
{
}

EXPORT_C PADabout()
{
}

EXPORT_C_(UINT32) PADtest()
{
	return 0;
}

