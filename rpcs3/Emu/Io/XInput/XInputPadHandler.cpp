#include "stdafx.h"
#if defined (_WIN32)
#include <algorithm>
#include "Utilities/Log.h"
#include "XInputPadHandler.h"
#include <cstring>

namespace {
	const DWORD THREAD_TIMEOUT = 1000;
	const DWORD THREAD_SLEEP = 10;
	const DWORD THREAD_SLEEP_INACTIVE = 100;
	const DWORD MAX_GAMEPADS = 4;
	const DWORD XINPUT_GAMEPAD_GUIDE = 0x0400;
	const DWORD XINPUT_GAMEPAD_BUTTONS = 16;
	const LPCWSTR LIBRARY_FILENAMES[] = {
		L"xinput1_4.dll",
		L"xinput1_3.dll",
		L"xinput1_2.dll",
		L"xinput9_1_0.dll"
	};

	inline u16 ConvertAxis(SHORT value)
	{
		return static_cast<u16>((value + 32768l) >> 8);
	}
}

XInputPadHandler::XInputPadHandler() : active(false), thread(nullptr), library(nullptr), xinputGetState(nullptr), xinputEnable(nullptr)
{
}

XInputPadHandler::~XInputPadHandler()
{
	Close();
}

void XInputPadHandler::Init(const u32 max_connect)
{
	for (auto it : LIBRARY_FILENAMES)
	{
		library = LoadLibrary(it);
		if (library)
		{
			xinputEnable = reinterpret_cast<PFN_XINPUTENABLE>(GetProcAddress(library, "XInputEnable"));
			xinputGetState = reinterpret_cast<PFN_XINPUTGETSTATE>(GetProcAddress(library, reinterpret_cast<LPCSTR>(100)));
			if (!xinputGetState)
			{
				xinputGetState = reinterpret_cast<PFN_XINPUTGETSTATE>(GetProcAddress(library, "XInputGetState"));
			}

			if (xinputEnable && xinputGetState)
			{
				break;
			}

			FreeLibrary(library);
			library = nullptr;
			xinputEnable = nullptr;
			xinputGetState = nullptr;
		}
	}

	if (library)
	{
		std::memset(&m_info, 0, sizeof m_info);
		m_info.max_connect = max_connect;

		for (u32 i = 0, max = std::min(max_connect, u32(MAX_GAMEPADS)); i != max; ++i)
		{
			m_pads.emplace_back(
				CELL_PAD_STATUS_ASSIGN_CHANGES,
				CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
				CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE,
				CELL_PAD_DEV_TYPE_STANDARD
			);
			auto & pad = m_pads.back();

			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, XINPUT_GAMEPAD_DPAD_UP, CELL_PAD_CTRL_UP);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, XINPUT_GAMEPAD_DPAD_DOWN, CELL_PAD_CTRL_DOWN);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, XINPUT_GAMEPAD_DPAD_LEFT, CELL_PAD_CTRL_LEFT);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, XINPUT_GAMEPAD_DPAD_RIGHT, CELL_PAD_CTRL_RIGHT);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, XINPUT_GAMEPAD_START, CELL_PAD_CTRL_START);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, XINPUT_GAMEPAD_BACK, CELL_PAD_CTRL_SELECT);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, XINPUT_GAMEPAD_LEFT_THUMB, CELL_PAD_CTRL_L3);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, XINPUT_GAMEPAD_RIGHT_THUMB, CELL_PAD_CTRL_R3);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, XINPUT_GAMEPAD_LEFT_SHOULDER, CELL_PAD_CTRL_L1);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, XINPUT_GAMEPAD_RIGHT_SHOULDER, CELL_PAD_CTRL_R1);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, XINPUT_GAMEPAD_GUIDE, 0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, 0x0); // Reserved
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, XINPUT_GAMEPAD_A, CELL_PAD_CTRL_CROSS);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, XINPUT_GAMEPAD_B, CELL_PAD_CTRL_CIRCLE);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, XINPUT_GAMEPAD_X, CELL_PAD_CTRL_SQUARE);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, XINPUT_GAMEPAD_Y, CELL_PAD_CTRL_TRIANGLE);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, CELL_PAD_CTRL_L2);
			pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, CELL_PAD_CTRL_R2);

			pad.m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, 0, 0);
			pad.m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, 0, 0);
			pad.m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, 0, 0);
			pad.m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, 0, 0);
		}

		active = true;
		thread = CreateThread(NULL, 0, &XInputPadHandler::ThreadProcProxy, this, 0, NULL);
	}
}

void XInputPadHandler::Close()
{
	if (library)
	{
		if (thread)
		{
			active = false;
			if (WaitForSingleObject(thread, THREAD_TIMEOUT) != WAIT_OBJECT_0)
				LOG_ERROR(HLE, "XInput thread could not stop within %d milliseconds", THREAD_TIMEOUT);
			thread = nullptr;
		}

		FreeLibrary(library);
		library = nullptr;
		xinputGetState = nullptr;
		xinputEnable = nullptr;
	}

	m_pads.clear();
}

DWORD XInputPadHandler::ThreadProcedure()
{
	while (active)
	{
		XINPUT_STATE state;
		DWORD result;
		DWORD online = 0;

		for (DWORD i = 0; i != m_pads.size(); ++i)
		{
			auto & pad = m_pads[i];

			result = (* xinputGetState)(i, &state);
			switch (result)
			{
			case ERROR_DEVICE_NOT_CONNECTED:
				pad.m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
				break;

			case ERROR_SUCCESS:
				++online;
				pad.m_port_status |= CELL_PAD_STATUS_CONNECTED;
				for (DWORD j = 0; j != XINPUT_GAMEPAD_BUTTONS; ++j)
				{
					bool pressed = state.Gamepad.wButtons & (1 << j);
					pad.m_buttons[j].m_pressed = pressed;
					pad.m_buttons[j].m_value = pressed ? 255 : 0;
				}

				pad.m_buttons[XINPUT_GAMEPAD_BUTTONS].m_pressed = state.Gamepad.bLeftTrigger > 0;
				pad.m_buttons[XINPUT_GAMEPAD_BUTTONS].m_value = state.Gamepad.bLeftTrigger;
				pad.m_buttons[XINPUT_GAMEPAD_BUTTONS + 1].m_pressed = state.Gamepad.bRightTrigger > 0;
				pad.m_buttons[XINPUT_GAMEPAD_BUTTONS + 1].m_value = state.Gamepad.bRightTrigger;

				pad.m_sticks[0].m_value = ConvertAxis(state.Gamepad.sThumbLX);
				pad.m_sticks[1].m_value = 255 - ConvertAxis(state.Gamepad.sThumbLY);
				pad.m_sticks[2].m_value = ConvertAxis(state.Gamepad.sThumbRX);
				pad.m_sticks[3].m_value = 255 - ConvertAxis(state.Gamepad.sThumbRY);
				break;
			}
		}

		Sleep((online > 0) ? THREAD_SLEEP : THREAD_SLEEP_INACTIVE);
		m_info.now_connect = online;
	}

	return 0;
}

DWORD WINAPI XInputPadHandler::ThreadProcProxy(LPVOID parameter)
{
	return reinterpret_cast<XInputPadHandler *>(parameter)->ThreadProcedure();
}

#endif