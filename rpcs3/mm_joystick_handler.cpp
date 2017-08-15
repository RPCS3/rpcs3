#include "stdafx.h"
#ifdef _WIN32
#include "mm_joystick_handler.h"

mm_joystick_config g_mmjoystick_config;

namespace {
	const DWORD THREAD_SLEEP = 10;
	const DWORD THREAD_SLEEP_INACTIVE = 100;
	const DWORD THREAD_TIMEOUT = 1000;

	inline u16 ConvertAxis(DWORD value)
	{
		return static_cast<u16>((value) >> 8);
	}
}


mm_joystick_handler::mm_joystick_handler() : is_init(false)
{
}

mm_joystick_handler::~mm_joystick_handler()
{
}

bool mm_joystick_handler::Init()
{
	if (is_init) return true;

	supportedJoysticks = joyGetNumDevs();
	if (supportedJoysticks > 0)
	{
		LOG_NOTICE(GENERAL, "Driver supports %u joysticks", supportedJoysticks);
	}
	else
	{
		LOG_ERROR(GENERAL, "Driver doesn't support Joysticks");
	}

	js_info.dwSize = sizeof(js_info);
	js_info.dwFlags = JOY_RETURNALL;
	joyGetDevCaps(JOYSTICKID1, &js_caps, sizeof(js_caps));


	bool JoyPresent = (joyGetPosEx(JOYSTICKID1, &js_info) == JOYERR_NOERROR);
	if (!JoyPresent)
	{
		LOG_ERROR(GENERAL, "Joystick not found");
		return false;
	}

	is_init = true;
	return true;
}

std::vector<std::string> mm_joystick_handler::ListDevices()
{
	std::vector<std::string> mm_pad_list;

	if (!Init()) return mm_pad_list;

	mm_pad_list.push_back("MMJoy Pad");

	return mm_pad_list;
}

bool mm_joystick_handler::bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device)
{
	if (!Init()) return false;

	g_mmjoystick_config.load();

	pad->Init(
		CELL_PAD_STATUS_DISCONNECTED,
		CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE,
		CELL_PAD_DEV_TYPE_STANDARD
	);

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_mmjoystick_config.triangle, CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_mmjoystick_config.circle, CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_mmjoystick_config.cross, CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_mmjoystick_config.square, CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_mmjoystick_config.l2, CELL_PAD_CTRL_L2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_mmjoystick_config.r2, CELL_PAD_CTRL_R2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_mmjoystick_config.l1, CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_mmjoystick_config.r1, CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_mmjoystick_config.start, CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_mmjoystick_config.select, CELL_PAD_CTRL_SELECT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_mmjoystick_config.l3, CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_mmjoystick_config.r3, CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, 0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, JOY_POVFORWARD, CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, JOY_POVBACKWARD, CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, JOY_POVLEFT, CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, JOY_POVRIGHT, CELL_PAD_CTRL_RIGHT);

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, 0, 0);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, 0, 0);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, 0, 0);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, 0, 0);

	bindings.push_back(pad);

	return true;
}

void mm_joystick_handler::ThreadProc()
{
	MMRESULT status;
	DWORD online = 0;

	for (u32 i = 0; i != bindings.size(); ++i)
	{
		auto pad = bindings[i];
		status = joyGetPosEx(JOYSTICKID1, &js_info);

		switch (status)
		{
		case JOYERR_UNPLUGGED:
			if (last_connection_status[i] == true)
				pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
			last_connection_status[i] = false;
			pad->m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
			break;

		case JOYERR_NOERROR:
			++online;
			if (last_connection_status[i] == false)
				pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
			last_connection_status[i] = true;
			pad->m_port_status |= CELL_PAD_STATUS_CONNECTED;
			for (DWORD j = 0; j <= 12; j++)
			{
				bool pressed = js_info.dwButtons & pad->m_buttons[j].m_keyCode;
				pad->m_buttons[j].m_pressed = pressed;
				pad->m_buttons[j].m_value = pressed ? 255 : 0;
			}
			for (DWORD j = 13; j <= 16; j++)//POV aka digital pad
			{
				bool pressed = js_info.dwPOV == pad->m_buttons[j].m_keyCode;
				pad->m_buttons[j].m_pressed = pressed;
				pad->m_buttons[j].m_value = pressed ? 255 : 0;
			}
			pad->m_sticks[0].m_value = ConvertAxis(js_info.dwXpos);
			pad->m_sticks[1].m_value = ConvertAxis(js_info.dwYpos);
			pad->m_sticks[2].m_value = ConvertAxis(js_info.dwZpos);
			pad->m_sticks[3].m_value = ConvertAxis(js_info.dwRpos);
			break;
		}
	}
}

#endif
