#include "stdafx.h"
#ifdef _WIN32
#include "mm_joystick_handler.h"

namespace
{
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
	// Set this handler's type and save location
	m_pad_config.cfg_type = "mmjoystick";
	m_pad_config.cfg_name = fs::get_config_dir() + "/config_mmjoystick.yml";

	// Set default button mapping
	m_pad_config.ls_left.def  = axis_list.at(mmjoy_axis::JOY_XNEG);
	m_pad_config.ls_down.def  = axis_list.at(mmjoy_axis::JOY_YNEG);
	m_pad_config.ls_right.def = axis_list.at(mmjoy_axis::JOY_XPOS);
	m_pad_config.ls_up.def    = axis_list.at(mmjoy_axis::JOY_YPOS);
	m_pad_config.rs_left.def  = axis_list.at(mmjoy_axis::JOY_UNEG);
	m_pad_config.rs_down.def  = axis_list.at(mmjoy_axis::JOY_RNEG);
	m_pad_config.rs_right.def = axis_list.at(mmjoy_axis::JOY_UPOS);
	m_pad_config.rs_up.def    = axis_list.at(mmjoy_axis::JOY_RPOS);
	m_pad_config.start.def    = button_list.at(JOY_BUTTON8);
	m_pad_config.select.def   = button_list.at(JOY_BUTTON7);
	m_pad_config.ps.def       = button_list.at(JOY_BUTTON17);
	m_pad_config.square.def   = button_list.at(JOY_BUTTON3);
	m_pad_config.cross.def    = button_list.at(JOY_BUTTON1);
	m_pad_config.circle.def   = button_list.at(JOY_BUTTON2);
	m_pad_config.triangle.def = button_list.at(JOY_BUTTON4);
	m_pad_config.left.def     = pov_list.at(JOY_POVLEFT);
	m_pad_config.down.def     = pov_list.at(JOY_POVBACKWARD);
	m_pad_config.right.def    = pov_list.at(JOY_POVRIGHT);
	m_pad_config.up.def       = pov_list.at(JOY_POVFORWARD);
	m_pad_config.r1.def       = button_list.at(JOY_BUTTON6);
	m_pad_config.r2.def       = axis_list.at(mmjoy_axis::JOY_ZPOS);
	m_pad_config.r3.def       = button_list.at(JOY_BUTTON10);
	m_pad_config.l1.def       = button_list.at(JOY_BUTTON5);
	m_pad_config.l2.def       = axis_list.at(mmjoy_axis::JOY_ZNEG);
	m_pad_config.l3.def       = button_list.at(JOY_BUTTON9);

	// Set default misc variables
	m_pad_config.lstickdeadzone.def = 0;    // between 0 and 255
	m_pad_config.rstickdeadzone.def = 0;    // between 0 and 255
	m_pad_config.ltriggerthreshold.def = 0; // between 0 and 255
	m_pad_config.rtriggerthreshold.def = 0; // between 0 and 255
	m_pad_config.padsquircling.def = 8000;

	// apply defaults
	m_pad_config.from_default();

	// set capabilities
	b_has_config = true;
	b_has_rumble = true;
	b_has_deadzones = true;

	m_trigger_threshold = TRIGGER_MAX / 2;
	m_thumb_threshold = THUMB_MAX / 2;
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

	m_pad_config.load();
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

	auto find_key = [=](const std::string& name)
	{
		long key = FindKeyCode(button_list, name);
		if (key < 0)
			key = FindKeyCode(pov_list, name);
		if (key < 0)
			key = FindKeyCode(axis_list, name);
		if (key < 0)
		{
			LOG_ERROR(HLE, "mmjoystick FindKey(%s) returned value %d", name, key);
			key = -1;
		}
		return key;
	};

	pad->Init
	(
		CELL_PAD_STATUS_CONNECTED | CELL_PAD_STATUS_ASSIGN_CHANGES,
		CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE,
		CELL_PAD_DEV_TYPE_STANDARD
	);

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.triangle), CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.circle),   CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.cross),    CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.square),   CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.l2),       CELL_PAD_CTRL_L2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.r2),       CELL_PAD_CTRL_R2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.l1),       CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.r1),       CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.start),    CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.select),   CELL_PAD_CTRL_SELECT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.l3),       CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.r3),       CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.ps),       0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.up),	     CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.down),	   CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.left),	   CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.right),    CELL_PAD_CTRL_RIGHT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, 0x0); // Reserved

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X,  find_key(m_pad_config.ls_left), find_key(m_pad_config.ls_right));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y,  find_key(m_pad_config.ls_down), find_key(m_pad_config.ls_up));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, find_key(m_pad_config.rs_left), find_key(m_pad_config.rs_right));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, find_key(m_pad_config.rs_down), find_key(m_pad_config.rs_up));

	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_X, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Y, 399);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Z, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_G, 512);

	pad->m_vibrateMotors.emplace_back(true, 0);
	pad->m_vibrateMotors.emplace_back(false, 0);

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

			auto button_values = GetButtonValues();

			// Translate any corresponding keycodes to our normal DS3 buttons and triggers
			for (auto& btn : pad->m_buttons)
			{
				btn.m_value = button_values[btn.m_keyCode];
				TranslateButtonPress(btn.m_keyCode, btn.m_pressed, btn.m_value);
			}

			float stick_val[4];

			// Translate any corresponding keycodes to our two sticks. (ignoring thresholds for now)
			for (int i = 0; i < static_cast<int>(pad->m_sticks.size()); i++)
			{
				bool pressed;
			
				// m_keyCodeMin is the mapped key for left or down
				u32 key_min = pad->m_sticks[i].m_keyCodeMin;
				u16 val_min = button_values[key_min];
				TranslateButtonPress(key_min, pressed, val_min, true);
			
				// m_keyCodeMax is the mapped key for right or up
				u32 key_max = pad->m_sticks[i].m_keyCodeMax;
				u16 val_max = button_values[key_max];
				TranslateButtonPress(key_max, pressed, val_max, true);

				// cancel out opposing values and get the resulting difference
				stick_val[i] = val_max - val_min;
			}

			u16 lx, ly, rx, ry;

			// Normalize our two stick's axis based on the thresholds
			std::tie(lx, ly) = NormalizeStickDeadzone(stick_val[0], stick_val[1], m_pad_config.lstickdeadzone);
			std::tie(rx, ry) = NormalizeStickDeadzone(stick_val[2], stick_val[3], m_pad_config.rstickdeadzone);

			if (m_pad_config.padsquircling != 0)
			{
				std::tie(lx, ly) = ConvertToSquirclePoint(lx, ly, m_pad_config.padsquircling);
				std::tie(rx, ry) = ConvertToSquirclePoint(rx, ry, m_pad_config.padsquircling);
			}

			pad->m_sticks[0].m_value = lx;
			pad->m_sticks[1].m_value = 255 - ly;
			pad->m_sticks[2].m_value = rx;
			pad->m_sticks[3].m_value = 255 - ry;

			break;
		}
	}
}

void mm_joystick_handler::GetNextButtonPress(const std::string& padId, const std::function<void(u16, std::string, int[])>& callback)
{
	if (!Init())
	{
		return;
	}

	MMRESULT status = joyGetPosEx(JOYSTICKID1, &js_info);

	switch (status)
	{
	case JOYERR_UNPLUGGED:
		break;

	case JOYERR_NOERROR:
		auto data = GetButtonValues();

		// Check for each button in our list if its corresponding (maybe remapped) button or axis was pressed.
		// Return the new value if the button was pressed (aka. its value was bigger than 0 or the defined threshold)
		// Use a pair to get all the legally pressed buttons and use the one with highest value (prioritize first)
		std::pair<u16, std::string> pressed_button = { 0, "" };
		for (const auto& button : button_list)
		{
			u16 value = data[button.first];
			if (value > 0 && value > pressed_button.first)
				pressed_button = { value, button.second };
		}
		for (const auto& button : pov_list)
		{
			u16 value = data[button.first];
			if (value > 0 && value > pressed_button.first)
				pressed_button = { value, button.second };
		}
		for (const auto& button : axis_list)
		{
			u32 keycode = button.first;
			u16 value = data[keycode];

			if (((keycode == mmjoy_axis::JOY_ZNEG) && (value > m_trigger_threshold))
			 || ((keycode == mmjoy_axis::JOY_ZPOS) && (value > m_trigger_threshold))
			 || ((keycode <= mmjoy_axis::JOY_YNEG) && (value > m_thumb_threshold))
			 || ((keycode <= mmjoy_axis::JOY_UNEG && keycode > mmjoy_axis::JOY_ZNEG) && (value > m_thumb_threshold)))
			{
				if (value > pressed_button.first)
				{
					pressed_button = { value, button.second };
				}
			}
		}

		int preview_values[6] = { data[JOY_ZNEG], data[JOY_ZPOS], data[JOY_XPOS] - data[JOY_XNEG], data[JOY_YPOS] - data[JOY_YNEG], data[JOY_UPOS] - data[JOY_UNEG], data[JOY_RPOS] - data[JOY_RNEG] };

		if (pressed_button.first > 0)
			return callback(pressed_button.first, pressed_button.second, preview_values);
		else
			return callback(0, "", preview_values);

		break;
	}
}

void mm_joystick_handler::TranslateButtonPress(u64 keyCode, bool& pressed, u16& val, bool ignore_threshold)
{
	// Update the pad button values based on their type and thresholds.
	// With this you can use axis or triggers as buttons or vice versa
	switch (keyCode)
	{
	case mmjoy_axis::JOY_ZNEG:
		pressed = val > m_pad_config.ltriggerthreshold;
		val = pressed ? NormalizeTriggerInput(val, m_pad_config.ltriggerthreshold) : 0;
		break;
	case mmjoy_axis::JOY_ZPOS:
		pressed = val > m_pad_config.rtriggerthreshold;
		val = pressed ? NormalizeTriggerInput(val, m_pad_config.rtriggerthreshold) : 0;
		break;
	case mmjoy_axis::JOY_XPOS:
	case mmjoy_axis::JOY_XNEG:
	case mmjoy_axis::JOY_YPOS:
	case mmjoy_axis::JOY_YNEG:
		pressed = val > (ignore_threshold ? 0 : m_pad_config.lstickdeadzone);
		val = pressed ? NormalizeStickInput(val, m_pad_config.lstickdeadzone, ignore_threshold) : 0;
		break;
	case mmjoy_axis::JOY_RPOS:
	case mmjoy_axis::JOY_RNEG:
	case mmjoy_axis::JOY_UPOS:
	case mmjoy_axis::JOY_UNEG:
		pressed = val > (ignore_threshold ? 0 : m_pad_config.rstickdeadzone);
		val = pressed ? NormalizeStickInput(val, m_pad_config.rstickdeadzone, ignore_threshold) : 0;
		break;
	default: // normal button (should in theory also support sensitive buttons)
		pressed = val > 0;
		val = pressed ? val : 0;
		break;
	}
}

std::unordered_map<u64, u16> mm_joystick_handler::GetButtonValues()
{
	std::unordered_map<u64, u16> button_values;

	for (auto entry : button_list)
	{
		button_values.emplace(entry.first, js_info.dwButtons & entry.first ? 255 : 0);
	}

	for (auto entry : pov_list)
	{
		button_values.emplace(entry.first, js_info.dwPOV == entry.first ? 255 : 0);
	}

	auto add_axis_value = [&](DWORD axis, u64 pos, u64 neg)
	{
		u16 value = 0;
		float fvalue = ScaleStickInput(axis, 0, 65535);
		bool is_negative = fvalue <= 127.5;

		if (is_negative)
		{
			value = Clamp0To255((127.5f - fvalue) * 2.0f);
			button_values.emplace(pos, 0);
			button_values.emplace(neg, value);
		}
		else
		{
			value = Clamp0To255((fvalue - 127.5f) * 2.0f);
			button_values.emplace(pos, value);
			button_values.emplace(neg, 0);
		}
	};

	add_axis_value(js_info.dwXpos, mmjoy_axis::JOY_XPOS, mmjoy_axis::JOY_XNEG);
	add_axis_value(js_info.dwYpos, mmjoy_axis::JOY_YNEG, mmjoy_axis::JOY_YPOS);
	add_axis_value(js_info.dwZpos, mmjoy_axis::JOY_ZNEG, mmjoy_axis::JOY_ZPOS);
	add_axis_value(js_info.dwRpos, mmjoy_axis::JOY_RNEG, mmjoy_axis::JOY_RPOS);
	add_axis_value(js_info.dwUpos, mmjoy_axis::JOY_UPOS, mmjoy_axis::JOY_UNEG);
	add_axis_value(js_info.dwVpos, mmjoy_axis::JOY_VPOS, mmjoy_axis::JOY_VNEG);

	return button_values;
}

#endif
