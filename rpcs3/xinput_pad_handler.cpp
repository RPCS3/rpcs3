
#ifdef _WIN32
#include "xinput_pad_handler.h"

xinput_pad_handler::xinput_pad_handler() : PadHandlerBase(pad_handler::xinput)
{
	init_configs();

	// Define border values
	thumb_min = -32768;
	thumb_max = 32767;
	trigger_min = 0;
	trigger_max = 255;
	vibration_min = 0;
	vibration_max = 65535;

	// set capabilities
	b_has_config = true;
	b_has_rumble = true;
	b_has_deadzones = true;

	m_name_string = "XInput Pad #";
	m_max_devices = XUSER_MAX_COUNT;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
}

xinput_pad_handler::~xinput_pad_handler()
{
	Close();
}

void xinput_pad_handler::init_config(pad_config* cfg, const std::string& name)
{
	// Set this profile's save location
	cfg->cfg_name = name;

	// Set default button mapping
	cfg->ls_left.def  = button_list.at(XInputKeyCodes::LSXNeg);
	cfg->ls_down.def  = button_list.at(XInputKeyCodes::LSYNeg);
	cfg->ls_right.def = button_list.at(XInputKeyCodes::LSXPos);
	cfg->ls_up.def    = button_list.at(XInputKeyCodes::LSYPos);
	cfg->rs_left.def  = button_list.at(XInputKeyCodes::RSXNeg);
	cfg->rs_down.def  = button_list.at(XInputKeyCodes::RSYNeg);
	cfg->rs_right.def = button_list.at(XInputKeyCodes::RSXPos);
	cfg->rs_up.def    = button_list.at(XInputKeyCodes::RSYPos);
	cfg->start.def    = button_list.at(XInputKeyCodes::Start);
	cfg->select.def   = button_list.at(XInputKeyCodes::Back);
	cfg->ps.def       = button_list.at(XInputKeyCodes::Guide);
	cfg->square.def   = button_list.at(XInputKeyCodes::X);
	cfg->cross.def    = button_list.at(XInputKeyCodes::A);
	cfg->circle.def   = button_list.at(XInputKeyCodes::B);
	cfg->triangle.def = button_list.at(XInputKeyCodes::Y);
	cfg->left.def     = button_list.at(XInputKeyCodes::Left);
	cfg->down.def     = button_list.at(XInputKeyCodes::Down);
	cfg->right.def    = button_list.at(XInputKeyCodes::Right);
	cfg->up.def       = button_list.at(XInputKeyCodes::Up);
	cfg->r1.def       = button_list.at(XInputKeyCodes::RB);
	cfg->r2.def       = button_list.at(XInputKeyCodes::RT);
	cfg->r3.def       = button_list.at(XInputKeyCodes::RS);
	cfg->l1.def       = button_list.at(XInputKeyCodes::LB);
	cfg->l2.def       = button_list.at(XInputKeyCodes::LT);
	cfg->l3.def       = button_list.at(XInputKeyCodes::LS);

	// Set default misc variables
	cfg->lstickdeadzone.def    = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;  // between 0 and 32767
	cfg->rstickdeadzone.def    = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE; // between 0 and 32767
	cfg->ltriggerthreshold.def = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;    // between 0 and 255
	cfg->rtriggerthreshold.def = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;    // between 0 and 255
	cfg->padsquircling.def     = 8000;

	// apply defaults
	cfg->from_default();
}

void xinput_pad_handler::GetNextButtonPress(const std::string& padId, const std::function<void(u16, std::string, std::string, int[])>& callback, const std::function<void(std::string)>& fail_callback, bool get_blacklist, std::vector<std::string> buttons)
{
	if (get_blacklist)
		blacklist.clear();

	int device_number = GetDeviceNumber(padId);
	if (device_number < 0)
		return fail_callback(padId);

	DWORD dwResult;
	XINPUT_STATE state;
	ZeroMemory(&state, sizeof(XINPUT_STATE));

	// Simply get the state of the controller from XInput.
	dwResult = (*xinputGetState)(static_cast<u32>(device_number), &state);
	if (dwResult != ERROR_SUCCESS)
		return fail_callback(padId);

	// Check for each button in our list if its corresponding (maybe remapped) button or axis was pressed.
	// Return the new value if the button was pressed (aka. its value was bigger than 0 or the defined threshold)
	// Use a pair to get all the legally pressed buttons and use the one with highest value (prioritize first)
	std::pair<u16, std::string> pressed_button = { 0, "" };
	auto data = GetButtonValues(state);
	for (const auto& button : button_list)
	{
		u32 keycode = button.first;
		u16 value = data[keycode];

		if (!get_blacklist && std::find(blacklist.begin(), blacklist.end(), keycode) != blacklist.end())
			continue;

		if (((keycode < XInputKeyCodes::LT) && (value > 0))
		 || ((keycode == XInputKeyCodes::LT) && (value > m_trigger_threshold))
		 || ((keycode == XInputKeyCodes::RT) && (value > m_trigger_threshold))
		 || ((keycode >= XInputKeyCodes::LSXNeg && keycode <= XInputKeyCodes::LSYPos) && (value > m_thumb_threshold))
		 || ((keycode >= XInputKeyCodes::RSXNeg && keycode <= XInputKeyCodes::RSYPos) && (value > m_thumb_threshold)))
		{
			if (get_blacklist)
			{
				blacklist.emplace_back(keycode);
				LOG_ERROR(HLE, "XInput Calibration: Added key [ %d = %s ] to blacklist. Value = %d", keycode, button.second, value);
			}
			else if (value > pressed_button.first)
				pressed_button = { value, button.second };
		}
	}

	if (get_blacklist)
	{
		if (blacklist.size() <= 0)
			LOG_SUCCESS(HLE, "XInput Calibration: Blacklist is clear. No input spam detected");
		return;
	}

	int preview_values[6] = { data[LT], data[RT], data[LSXPos] - data[LSXNeg], data[LSYPos] - data[LSYNeg], data[RSXPos] - data[RSXNeg], data[RSYPos] - data[RSYNeg] };

	if (pressed_button.first > 0)
		return callback(pressed_button.first, pressed_button.second, padId, preview_values);
	else
		return callback(0, "", padId, preview_values);
}

void xinput_pad_handler::TestVibration(const std::string& padId, u32 largeMotor, u32 smallMotor)
{
	int device_number = GetDeviceNumber(padId);
	if (device_number < 0)
		return;

	// The left motor is the low-frequency rumble motor. The right motor is the high-frequency rumble motor.
	// The two motors are not the same, and they create different vibration effects.
	XINPUT_VIBRATION vibrate;

	vibrate.wLeftMotorSpeed = largeMotor;  // between 0 to 65535
	vibrate.wRightMotorSpeed = smallMotor; // between 0 to 65535

	(*xinputSetState)(static_cast<u32>(device_number), &vibrate);
}

void xinput_pad_handler::TranslateButtonPress(u64 keyCode, bool& pressed, u16& val, bool ignore_threshold)
{
	// Update the pad button values based on their type and thresholds.
	// With this you can use axis or triggers as buttons or vice versa
	auto p_profile = m_dev->config;
	switch (keyCode)
	{
	case XInputKeyCodes::LT:
		pressed = val > p_profile->ltriggerthreshold;
		val = pressed ? NormalizeTriggerInput(val, p_profile->ltriggerthreshold) : 0;
		break;
	case XInputKeyCodes::RT:
		pressed = val > p_profile->rtriggerthreshold;
		val = pressed ? NormalizeTriggerInput(val, p_profile->rtriggerthreshold) : 0;
		break;
	case XInputKeyCodes::LSXNeg:
	case XInputKeyCodes::LSXPos:
	case XInputKeyCodes::LSYPos:
	case XInputKeyCodes::LSYNeg:
		pressed = val > (ignore_threshold ? 0 : p_profile->lstickdeadzone);
		val = pressed ? NormalizeStickInput(val, p_profile->lstickdeadzone, ignore_threshold) : 0;
		break;
	case XInputKeyCodes::RSXNeg:
	case XInputKeyCodes::RSXPos:
	case XInputKeyCodes::RSYPos:
	case XInputKeyCodes::RSYNeg:
		pressed = val > (ignore_threshold ? 0 : p_profile->rstickdeadzone);
		val = pressed ? NormalizeStickInput(val, p_profile->rstickdeadzone, ignore_threshold) : 0;
		break;
	default: // normal button (should in theory also support sensitive buttons)
		pressed = val > 0;
		val = pressed ? val : 0;
		break;
	}
}

int xinput_pad_handler::GetDeviceNumber(const std::string& padId)
{
	if (!Init())
		return -1;

	size_t pos = padId.find(m_name_string);
	if (pos == std::string::npos)
		return -1;

	int device_number = std::stoul(padId.substr(pos + 12)) - 1; // Controllers 1-n in GUI
	if (device_number >= XUSER_MAX_COUNT)
		return -1;

	return device_number;
}

std::array<u16, xinput_pad_handler::XInputKeyCodes::KeyCodeCount> xinput_pad_handler::GetButtonValues(const XINPUT_STATE& state)
{
	std::array<u16, xinput_pad_handler::XInputKeyCodes::KeyCodeCount> values;

	// Triggers
	values[XInputKeyCodes::LT] = state.Gamepad.bLeftTrigger;
	values[XInputKeyCodes::RT] = state.Gamepad.bRightTrigger;

	// Sticks
	int lx = state.Gamepad.sThumbLX;
	int ly = state.Gamepad.sThumbLY;
	int rx = state.Gamepad.sThumbRX;
	int ry = state.Gamepad.sThumbRY;

	// Left Stick X Axis
	values[XInputKeyCodes::LSXNeg] = lx < 0 ? abs(lx) - 1 : 0;
	values[XInputKeyCodes::LSXPos] = lx > 0 ? lx : 0;

	// Left Stick Y Axis
	values[XInputKeyCodes::LSYNeg] = ly < 0 ? abs(ly) - 1 : 0;
	values[XInputKeyCodes::LSYPos] = ly > 0 ? ly : 0;

	// Right Stick X Axis
	values[XInputKeyCodes::RSXNeg] = rx < 0 ? abs(rx) - 1 : 0;
	values[XInputKeyCodes::RSXPos] = rx > 0 ? rx : 0;

	// Right Stick Y Axis
	values[XInputKeyCodes::RSYNeg] = ry < 0 ? abs(ry) - 1 : 0;
	values[XInputKeyCodes::RSYPos] = ry > 0 ? ry : 0;

	// Buttons
	WORD buttons = state.Gamepad.wButtons;

	// A, B, X, Y
	values[XInputKeyCodes::A] = buttons & XINPUT_GAMEPAD_A ? 255 : 0;
	values[XInputKeyCodes::B] = buttons & XINPUT_GAMEPAD_B ? 255 : 0;
	values[XInputKeyCodes::X] = buttons & XINPUT_GAMEPAD_X ? 255 : 0;
	values[XInputKeyCodes::Y] = buttons & XINPUT_GAMEPAD_Y ? 255 : 0;

	// D-Pad
	values[XInputKeyCodes::Left]  = buttons & XINPUT_GAMEPAD_DPAD_LEFT ? 255 : 0;
	values[XInputKeyCodes::Right] = buttons & XINPUT_GAMEPAD_DPAD_RIGHT ? 255 : 0;
	values[XInputKeyCodes::Up]    = buttons & XINPUT_GAMEPAD_DPAD_UP ? 255 : 0;
	values[XInputKeyCodes::Down]  = buttons & XINPUT_GAMEPAD_DPAD_DOWN ? 255 : 0;

	// LB, RB, LS, RS
	values[XInputKeyCodes::LB] = buttons & XINPUT_GAMEPAD_LEFT_SHOULDER ? 255 : 0;
	values[XInputKeyCodes::RB] = buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER ? 255 : 0;
	values[XInputKeyCodes::LS] = buttons & XINPUT_GAMEPAD_LEFT_THUMB ? 255 : 0;
	values[XInputKeyCodes::RS] = buttons & XINPUT_GAMEPAD_RIGHT_THUMB ? 255 : 0;

	// Start, Back, Guide
	values[XInputKeyCodes::Start] = buttons & XINPUT_GAMEPAD_START ? 255 : 0;
	values[XInputKeyCodes::Back]  = buttons & XINPUT_GAMEPAD_BACK ? 255 : 0;
	values[XInputKeyCodes::Guide] = buttons & XINPUT_INFO::GUIDE_BUTTON ? 255 : 0;

	return values;
}

bool xinput_pad_handler::Init()
{
	if (is_init)
		return true;

	for (auto it : XINPUT_INFO::LIBRARY_FILENAMES)
	{
		library = LoadLibrary(it);
		if (library)
		{
			xinputEnable = reinterpret_cast<PFN_XINPUTENABLE>(GetProcAddress(library, "XInputEnable"));
			xinputGetState = reinterpret_cast<PFN_XINPUTGETSTATE>(GetProcAddress(library, reinterpret_cast<LPCSTR>(100)));
			if (!xinputGetState)
				xinputGetState = reinterpret_cast<PFN_XINPUTGETSTATE>(GetProcAddress(library, "XInputGetState"));

			xinputSetState = reinterpret_cast<PFN_XINPUTSETSTATE>(GetProcAddress(library, "XInputSetState"));
			xinputGetBatteryInformation = reinterpret_cast<PFN_XINPUTGETBATTERYINFORMATION>(GetProcAddress(library, "XInputGetBatteryInformation"));

			if (xinputEnable && xinputGetState && xinputSetState && xinputGetBatteryInformation)
			{
				is_init = true;
				break;
			}

			FreeLibrary(library);
			library = nullptr;
			xinputEnable = nullptr;
			xinputGetState = nullptr;
			xinputGetBatteryInformation = nullptr;
		}
	}

	if (!is_init)
		return false;

	return true;
}

void xinput_pad_handler::Close()
{
	if (library)
	{
		FreeLibrary(library);
		library = nullptr;
		xinputGetState = nullptr;
		xinputEnable = nullptr;
		xinputGetBatteryInformation = nullptr;
	}
}

void xinput_pad_handler::ThreadProc()
{
	for (int i = 0; i < static_cast<int>(bindings.size()); ++i)
	{
		auto& bind = bindings[i];
		m_dev = bind.first;
		auto padnum = m_dev->deviceNumber;
		auto profile = m_dev->config;
		auto pad = bind.second;

		result = (*xinputGetState)(padnum, &state);

		switch (result)
		{
		case ERROR_DEVICE_NOT_CONNECTED:
		{
			if (last_connection_status[i] == true)
			{
				LOG_ERROR(HLE, "XInput device %d disconnected", padnum);
				pad->m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
				pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
				last_connection_status[i] = false;
				connected--;
			}
			continue;
		}
		case ERROR_SUCCESS:
		{
			if (last_connection_status[i] == false)
			{
				LOG_SUCCESS(HLE, "XInput device %d reconnected", padnum);
				pad->m_port_status |= CELL_PAD_STATUS_CONNECTED;
				pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
				last_connection_status[i] = true;
				connected++;
			}

			std::array<u16, XInputKeyCodes::KeyCodeCount> button_values = GetButtonValues(state);

			// Translate any corresponding keycodes to our normal DS3 buttons and triggers
			for (auto& btn : pad->m_buttons)
			{
				btn.m_value = button_values[btn.m_keyCode];
				TranslateButtonPress(btn.m_keyCode, btn.m_pressed, btn.m_value);
			}

			for (const auto& btn : pad->m_buttons)
			{
				if (btn.m_pressed)
				{
					SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
					break;
				}
			}

			// used to get the absolute value of an axis
			s32 stick_val[4];

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
			std::tie(lx, ly) = NormalizeStickDeadzone(stick_val[0], stick_val[1], profile->lstickdeadzone);
			std::tie(rx, ry) = NormalizeStickDeadzone(stick_val[2], stick_val[3], profile->rstickdeadzone);

			if (profile->padsquircling != 0)
			{
				std::tie(lx, ly) = ConvertToSquirclePoint(lx, ly, profile->padsquircling);
				std::tie(rx, ry) = ConvertToSquirclePoint(rx, ry, profile->padsquircling);
			}

			pad->m_sticks[0].m_value = lx;
			pad->m_sticks[1].m_value = 255 - ly;
			pad->m_sticks[2].m_value = rx;
			pad->m_sticks[3].m_value = 255 - ry;

			// Receive Battery Info. If device is not on cable, get battery level, else assume full
			XINPUT_BATTERY_INFORMATION battery_info;
			(*xinputGetBatteryInformation)(padnum, BATTERY_DEVTYPE_GAMEPAD, &battery_info);
			pad->m_cable_state = battery_info.BatteryType == BATTERY_TYPE_WIRED ? 1 : 0;
			pad->m_battery_level = pad->m_cable_state ? BATTERY_LEVEL_FULL : battery_info.BatteryLevel;

			// The left motor is the low-frequency rumble motor. The right motor is the high-frequency rumble motor.
			// The two motors are not the same, and they create different vibration effects. Values range between 0 to 65535.
			int idx_l = profile->switch_vibration_motors ? 1 : 0;
			int idx_s = profile->switch_vibration_motors ? 0 : 1;

			int speed_large = profile->enable_vibration_motor_large ? pad->m_vibrateMotors[idx_l].m_value * 257 : vibration_min;
			int speed_small = profile->enable_vibration_motor_small ? pad->m_vibrateMotors[idx_s].m_value * 257 : vibration_min;

			m_dev->newVibrateData = m_dev->newVibrateData || m_dev->largeVibrate != speed_large || m_dev->smallVibrate != speed_small;

			m_dev->largeVibrate = speed_large;
			m_dev->smallVibrate = speed_small;

			// XBox One Controller can't handle faster vibration updates than ~10ms. Elite is even worse. So I'll use 20ms to be on the safe side. No lag was noticable.
			if (m_dev->newVibrateData && (clock() - m_dev->last_vibration > 20))
			{
				XINPUT_VIBRATION vibrate;
				vibrate.wLeftMotorSpeed = speed_large;
				vibrate.wRightMotorSpeed = speed_small;

				if ((*xinputSetState)(padnum, &vibrate) == ERROR_SUCCESS)
				{
					m_dev->newVibrateData = false;
					m_dev->last_vibration = clock();
				}
			}
			break;
		}
		default:
			break;
		}
	}
}

std::vector<std::string> xinput_pad_handler::ListDevices()
{
	std::vector<std::string> xinput_pads_list;

	if (!Init())
		return xinput_pads_list;

	for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
	{
		XINPUT_STATE state;
		DWORD result = (*xinputGetState)(i, &state);
		if (result == ERROR_SUCCESS)
			xinput_pads_list.push_back(m_name_string + std::to_string(i + 1)); // Controllers 1-n in GUI
	}
	return xinput_pads_list;
}

bool xinput_pad_handler::bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device)
{
	//Convert device string to u32 representing xinput device number
	int device_number = GetDeviceNumber(device);
	if (device_number < 0)
		return false;

	std::shared_ptr<XInputDevice> x_device = std::make_shared<XInputDevice>();
	x_device->deviceNumber = static_cast<u32>(device_number);

	int index = static_cast<int>(bindings.size());
	m_pad_configs[index].load();
	x_device->config = &m_pad_configs[index];
	pad_config* p_profile = x_device->config;
	if (p_profile == nullptr)
		return false;

	pad->Init
	(
		CELL_PAD_STATUS_DISCONNECTED,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE,
		CELL_PAD_DEV_TYPE_STANDARD
	);

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->up),       CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->down),     CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->left),     CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->right),    CELL_PAD_CTRL_RIGHT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->start),    CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->select),   CELL_PAD_CTRL_SELECT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->l3),       CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->r3),       CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->l1),       CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->r1),       CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->ps),       0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->cross),    CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->circle),   CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->square),   CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->triangle), CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->l2),       CELL_PAD_CTRL_L2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->r2),       CELL_PAD_CTRL_R2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, 0x0); // Reserved

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X,  FindKeyCode(button_list, p_profile->ls_left), FindKeyCode(button_list, p_profile->ls_right));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y,  FindKeyCode(button_list, p_profile->ls_down), FindKeyCode(button_list, p_profile->ls_up));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, FindKeyCode(button_list, p_profile->rs_left), FindKeyCode(button_list, p_profile->rs_right));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, FindKeyCode(button_list, p_profile->rs_down), FindKeyCode(button_list, p_profile->rs_up));

	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_X, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Y, 399);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Z, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_G, 512);

	pad->m_vibrateMotors.emplace_back(true, 0);
	pad->m_vibrateMotors.emplace_back(false, 0);

	bindings.emplace_back(x_device, pad);

	return true;
}

#endif
