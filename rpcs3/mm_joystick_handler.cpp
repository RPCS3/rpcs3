#ifdef _WIN32
#include "mm_joystick_handler.h"

mm_joystick_handler::mm_joystick_handler() : PadHandlerBase(pad_handler::mm)
{
	init_configs();

	// Define border values
	thumb_min = 0;
	thumb_max = 255;
	trigger_min = 0;
	trigger_max = 255;
	vibration_min = 0;
	vibration_max = 65535;

	// set capabilities
	b_has_config = true;
	b_has_rumble = true;
	b_has_deadzones = true;

	m_name_string = "Joystick #";

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
}

mm_joystick_handler::~mm_joystick_handler()
{
}

void mm_joystick_handler::init_config(pad_config* cfg, const std::string& name)
{
	// Set this profile's save location
	cfg->cfg_name = name;

	// Set default button mapping
	cfg->ls_left.def  = axis_list.at(mmjoy_axis::joy_x_neg);
	cfg->ls_down.def  = axis_list.at(mmjoy_axis::joy_y_neg);
	cfg->ls_right.def = axis_list.at(mmjoy_axis::joy_x_pos);
	cfg->ls_up.def    = axis_list.at(mmjoy_axis::joy_y_pos);
	cfg->rs_left.def  = axis_list.at(mmjoy_axis::joy_z_neg);
	cfg->rs_down.def  = axis_list.at(mmjoy_axis::joy_r_neg);
	cfg->rs_right.def = axis_list.at(mmjoy_axis::joy_z_pos);
	cfg->rs_up.def    = axis_list.at(mmjoy_axis::joy_r_pos);
	cfg->start.def    = button_list.at(JOY_BUTTON9);
	cfg->select.def   = button_list.at(JOY_BUTTON10);
	cfg->ps.def       = button_list.at(JOY_BUTTON17);
	cfg->square.def   = button_list.at(JOY_BUTTON4);
	cfg->cross.def    = button_list.at(JOY_BUTTON3);
	cfg->circle.def   = button_list.at(JOY_BUTTON2);
	cfg->triangle.def = button_list.at(JOY_BUTTON1);
	cfg->left.def     = pov_list.at(JOY_POVLEFT);
	cfg->down.def     = pov_list.at(JOY_POVBACKWARD);
	cfg->right.def    = pov_list.at(JOY_POVRIGHT);
	cfg->up.def       = pov_list.at(JOY_POVFORWARD);
	cfg->r1.def       = button_list.at(JOY_BUTTON8);
	cfg->r2.def       = button_list.at(JOY_BUTTON6);
	cfg->r3.def       = button_list.at(JOY_BUTTON12);
	cfg->l1.def       = button_list.at(JOY_BUTTON7);
	cfg->l2.def       = button_list.at(JOY_BUTTON5);
	cfg->l3.def       = button_list.at(JOY_BUTTON11);

	// Set default misc variables
	cfg->lstickdeadzone.def    = 0; // between 0 and 255
	cfg->rstickdeadzone.def    = 0; // between 0 and 255
	cfg->ltriggerthreshold.def = 0; // between 0 and 255
	cfg->rtriggerthreshold.def = 0; // between 0 and 255
	cfg->padsquircling.def     = 8000;

	// apply defaults
	cfg->from_default();
}

bool mm_joystick_handler::Init()
{
	if (is_init)
		return true;

	m_devices.clear();
	m_supported_joysticks = joyGetNumDevs();

	if (m_supported_joysticks <= 0)
	{
		LOG_ERROR(GENERAL, "mmjoy: Driver doesn't support Joysticks");
		return false;
	}

	LOG_NOTICE(GENERAL, "mmjoy: Driver supports %u joysticks", m_supported_joysticks);

	for (u32 i = 0; i < m_supported_joysticks; i++)
	{
		MMJOYDevice dev;

		if (GetMMJOYDevice(i, &dev) == false)
			continue;

		m_devices.emplace(i, dev);
	}

	is_init = true;
	return true;
}

std::vector<std::string> mm_joystick_handler::ListDevices()
{
	std::vector<std::string> devices;

	if (!Init())
		return devices;

	for (auto dev : m_devices)
	{
		devices.emplace_back(dev.second.device_name);
	}

	return devices;
}

bool mm_joystick_handler::bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device)
{
	if (!Init())
		return false;

	int id = GetIDByName(device);
	if (id < 0)
		return false;

	std::shared_ptr<MMJOYDevice> joy_device = std::make_shared<MMJOYDevice>(m_devices.at(id));

	int index = static_cast<int>(bindings.size());
	m_pad_configs[index].load();
	joy_device->config = &m_pad_configs[index];
	pad_config* p_profile = joy_device->config;
	if (p_profile == nullptr)
		return false;

	auto find_key = [=](const cfg::string& name)
	{
		long key = FindKeyCode(button_list, name, false);
		if (key < 0)
			key = FindKeyCode(pov_list, name, false);
		if (key < 0)
			key = FindKeyCode(axis_list, name);
		return static_cast<u64>(key);
	};

	pad->Init
	(
		CELL_PAD_STATUS_DISCONNECTED,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE,
		CELL_PAD_DEV_TYPE_STANDARD
	);

	joy_device->trigger_left  = find_key(p_profile->l2);
	joy_device->trigger_right = find_key(p_profile->r2);
	joy_device->axis_left[0]  = find_key(p_profile->ls_left);
	joy_device->axis_left[1]  = find_key(p_profile->ls_right);
	joy_device->axis_left[2]  = find_key(p_profile->ls_down);
	joy_device->axis_left[3]  = find_key(p_profile->ls_up);
	joy_device->axis_right[0] = find_key(p_profile->rs_left);
	joy_device->axis_right[1] = find_key(p_profile->rs_right);
	joy_device->axis_right[2] = find_key(p_profile->rs_down);
	joy_device->axis_right[3] = find_key(p_profile->rs_up);

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(p_profile->triangle), CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(p_profile->circle),   CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(p_profile->cross),    CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(p_profile->square),   CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, joy_device->trigger_left,      CELL_PAD_CTRL_L2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, joy_device->trigger_right,     CELL_PAD_CTRL_R2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(p_profile->l1),       CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(p_profile->r1),       CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(p_profile->start),    CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(p_profile->select),   CELL_PAD_CTRL_SELECT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(p_profile->l3),       CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(p_profile->r3),       CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(p_profile->ps),       0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(p_profile->up),       CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(p_profile->down),     CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(p_profile->left),     CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(p_profile->right),    CELL_PAD_CTRL_RIGHT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, 0x0); // Reserved

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X,  joy_device->axis_left[0],  joy_device->axis_left[1]);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y,  joy_device->axis_left[2],  joy_device->axis_left[3]);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, joy_device->axis_right[0], joy_device->axis_right[1]);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, joy_device->axis_right[2], joy_device->axis_right[3]);

	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_X, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Y, 399);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Z, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_G, 512);

	pad->m_vibrateMotors.emplace_back(true, 0);
	pad->m_vibrateMotors.emplace_back(false, 0);

	bindings.emplace_back(joy_device, pad);

	return true;
}

void mm_joystick_handler::ThreadProc()
{
	MMRESULT status;

	for (u32 i = 0; i != bindings.size(); ++i)
	{
		m_dev = bindings[i].first;
		auto pad = bindings[i].second;
		auto profile = m_dev->config;
		status = joyGetPosEx(m_dev->device_id, &m_dev->device_info);

		if (status != JOYERR_NOERROR)
		{
			if (last_connection_status[i] == true)
			{
				LOG_ERROR(HLE, "MMJOY Device %d disconnected.", m_dev->device_id);
				pad->m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
				pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
				last_connection_status[i] = false;
				connected--;
			}
			continue;
		}

		if (last_connection_status[i] == false)
		{
			if (GetMMJOYDevice(m_dev->device_id, m_dev.get()) == false)
				continue;
			LOG_SUCCESS(HLE, "MMJOY Device %d reconnected.", m_dev->device_id);
			pad->m_port_status |= CELL_PAD_STATUS_CONNECTED;
			pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
			last_connection_status[i] = true;
			connected++;
		}

		auto button_values = GetButtonValues(m_dev->device_info, m_dev->device_caps);

		// Translate any corresponding keycodes to our normal DS3 buttons and triggers
		for (auto& btn : pad->m_buttons)
		{
			btn.m_value = button_values[btn.m_keyCode];
			TranslateButtonPress(btn.m_keyCode, btn.m_pressed, btn.m_value);
		}

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
	}
}

void mm_joystick_handler::GetNextButtonPress(const std::string& padId, const std::function<void(u16, std::string, std::string, int[])>& callback, const std::function<void(std::string)>& fail_callback, bool get_blacklist, std::vector<std::string> buttons)
{
	if (get_blacklist)
		blacklist.clear();

	if (!Init())
		return fail_callback(padId);

	static std::string cur_pad = "";
	static int id = -1;

	if (cur_pad != padId)
	{
		cur_pad == padId;
		id = GetIDByName(padId);
		if (id < 0)
		{
			LOG_ERROR(GENERAL, "MMJOY GetNextButtonPress for device [%s] failed with id = %d", padId, id);
			return fail_callback(padId);
		}
	}

	JOYINFOEX js_info;
	JOYCAPS js_caps;
	js_info.dwSize = sizeof(js_info);
	js_info.dwFlags = JOY_RETURNALL;
	joyGetDevCaps(id, &js_caps, sizeof(js_caps));

	MMRESULT status = joyGetPosEx(id, &js_info);

	switch (status)
	{
	case JOYERR_UNPLUGGED:
	{
		return fail_callback(padId);
	}
	case JOYERR_NOERROR:
	{
		auto data = GetButtonValues(js_info, js_caps);

		// Check for each button in our list if its corresponding (maybe remapped) button or axis was pressed.
		// Return the new value if the button was pressed (aka. its value was bigger than 0 or the defined threshold)
		// Use a pair to get all the legally pressed buttons and use the one with highest value (prioritize first)
		std::pair<u16, std::string> pressed_button = { 0, "" };

		for (const auto& button : axis_list)
		{
			u64 keycode = button.first;
			u16 value = data[keycode];

			if (!get_blacklist && std::find(blacklist.begin(), blacklist.end(), keycode) != blacklist.end())
				continue;

			if (value > m_thumb_threshold)
			{
				if (get_blacklist)
				{
					blacklist.emplace_back(keycode);
					LOG_ERROR(HLE, "MMJOY Calibration: Added axis [ %d = %s ] to blacklist. Value = %d", keycode, button.second, value);
				}
				else if (value > pressed_button.first)
					pressed_button = { value, button.second };
			}
		}

		for (const auto& button : pov_list)
		{
			u64 keycode = button.first;
			u16 value = data[keycode];

			if (!get_blacklist && std::find(blacklist.begin(), blacklist.end(), keycode) != blacklist.end())
				continue;

			if (value > 0)
			{
				if (get_blacklist)
				{
					blacklist.emplace_back(keycode);
					LOG_ERROR(HLE, "MMJOY Calibration: Added pov [ %d = %s ] to blacklist. Value = %d", keycode, button.second, value);
				}
				else if (value > pressed_button.first)
					pressed_button = { value, button.second };
			}
		}

		for (const auto& button : button_list)
		{
			u64 keycode = button.first;
			u16 value = data[keycode];

			if (!get_blacklist && std::find(blacklist.begin(), blacklist.end(), keycode) != blacklist.end())
				continue;

			if (value > 0)
			{
				if (get_blacklist)
				{
					blacklist.emplace_back(keycode);
					LOG_ERROR(HLE, "MMJOY Calibration: Added button [ %d = %s ] to blacklist. Value = %d", keycode, button.second, value);
				}
				else if (value > pressed_button.first)
					pressed_button = { value, button.second };
			}
		}

		if (get_blacklist)
		{
			if (blacklist.size() <= 0)
				LOG_SUCCESS(HLE, "MMJOY Calibration: Blacklist is clear. No input spam detected");
			return;
		}

		auto find_key = [=](const std::string& name)
		{
			long key = FindKeyCodeByString(axis_list, name, false);
			if (key < 0)
				key = FindKeyCodeByString(pov_list, name, false);
			if (key < 0)
				key = FindKeyCodeByString(button_list, name);
			return static_cast<u64>(key);
		};

		int preview_values[6] = { 0, 0, 0, 0, 0, 0 };
		if (buttons.size() == 10)
		{
			preview_values[0] = data[find_key(buttons[0])];
			preview_values[1] = data[find_key(buttons[1])];
			preview_values[2] = data[find_key(buttons[3])] - data[find_key(buttons[2])];
			preview_values[3] = data[find_key(buttons[5])] - data[find_key(buttons[4])];
			preview_values[4] = data[find_key(buttons[7])] - data[find_key(buttons[6])];
			preview_values[5] = data[find_key(buttons[9])] - data[find_key(buttons[8])];
		}

		if (pressed_button.first > 0)
			return callback(pressed_button.first, pressed_button.second, padId, preview_values);
		else
			return callback(0, "", padId, preview_values);

		break;
	}
	default:
		break;
	}
}

void mm_joystick_handler::TranslateButtonPress(u64 keyCode, bool& pressed, u16& val, bool ignore_threshold)
{
	// Update the pad button values based on their type and thresholds.
	// With this you can use axis or triggers as buttons or vice versa
	auto p_profile = m_dev->config;

	if (keyCode == m_dev->trigger_left)
	{
		pressed = val > (ignore_threshold ? 0 : p_profile->ltriggerthreshold);
		val = pressed ? NormalizeTriggerInput(val, p_profile->ltriggerthreshold) : 0;
	}
	else if (keyCode == m_dev->trigger_right)
	{
		pressed = val > (ignore_threshold ? 0 : p_profile->rtriggerthreshold);
		val = pressed ? NormalizeTriggerInput(val, p_profile->rtriggerthreshold) : 0;
	}
	else if (std::find(m_dev->axis_left.begin(), m_dev->axis_left.end(), keyCode) != m_dev->axis_left.end())
	{
		pressed = val > (ignore_threshold ? 0 : p_profile->lstickdeadzone);
		val = pressed ? NormalizeStickInput(val, p_profile->lstickdeadzone, ignore_threshold) : 0;
	}
	else if (std::find(m_dev->axis_right.begin(), m_dev->axis_right.end(), keyCode) != m_dev->axis_right.end())
	{
		pressed = val > (ignore_threshold ? 0 : p_profile->rstickdeadzone);
		val = pressed ? NormalizeStickInput(val, p_profile->rstickdeadzone, ignore_threshold) : 0;
	}
	else // normal button (should in theory also support sensitive buttons)
	{
		pressed = val > 0;
		val = pressed ? val : 0;
	}
}

std::unordered_map<u64, u16> mm_joystick_handler::GetButtonValues(const JOYINFOEX& js_info, const JOYCAPS& js_caps)
{
	std::unordered_map<u64, u16> button_values;

	for (auto entry : button_list)
	{
		button_values.emplace(entry.first, js_info.dwButtons & entry.first ? 255 : 0);
	}

	if (js_caps.wCaps & JOYCAPS_HASPOV)
	{
		if (js_caps.wCaps & JOYCAPS_POVCTS)
		{
			if (js_info.dwPOV == JOY_POVCENTERED)
			{
				button_values.emplace(JOY_POVFORWARD,  0);
				button_values.emplace(JOY_POVRIGHT,    0);
				button_values.emplace(JOY_POVBACKWARD, 0);
				button_values.emplace(JOY_POVLEFT,     0);
			}
			else
			{
				auto emplacePOVs = [&](float val, u64 pov_neg, u64 pov_pos)
				{
					if (val < 0)
					{
						button_values.emplace(pov_neg, static_cast<u16>(std::abs(val)));
						button_values.emplace(pov_pos, 0);
					}
					else
					{
						button_values.emplace(pov_neg, 0);
						button_values.emplace(pov_pos, static_cast<u16>(val));
					}
				};

				float rad = static_cast<float>(js_info.dwPOV / 100 * acos(-1) / 180);
				emplacePOVs(cosf(rad) * 255.0f, JOY_POVBACKWARD, JOY_POVFORWARD);
				emplacePOVs(sinf(rad) * 255.0f, JOY_POVLEFT, JOY_POVRIGHT);
			}
		}
		else if (js_caps.wCaps & JOYCAPS_POV4DIR)
		{
			int val = static_cast<int>(js_info.dwPOV);

			auto emplacePOV = [&button_values, &val](int pov)
			{
				int cw = pov + 4500, ccw = pov - 4500;
				bool pressed = (val == pov) || (val == cw) || (ccw < 0 ? val == 36000 - std::abs(ccw) : val == ccw);
				button_values.emplace(pov, pressed ? 255 : 0);
			};

			emplacePOV(JOY_POVFORWARD);
			emplacePOV(JOY_POVRIGHT);
			emplacePOV(JOY_POVBACKWARD);
			emplacePOV(JOY_POVLEFT);
		}
	}

	auto add_axis_value = [&](DWORD axis, UINT min, UINT max, u64 pos, u64 neg)
	{
		float val = ScaleStickInput2(axis, min, max);
		if (val < 0)
		{
			button_values.emplace(pos, 0);
			button_values.emplace(neg, static_cast<u16>(std::abs(val)));
		}
		else
		{
			button_values.emplace(pos, static_cast<u16>(val));
			button_values.emplace(neg, 0);
		}
	};

	add_axis_value(js_info.dwXpos, js_caps.wXmin, js_caps.wXmax, mmjoy_axis::joy_x_pos, mmjoy_axis::joy_x_neg);
	add_axis_value(js_info.dwYpos, js_caps.wYmin, js_caps.wYmax, mmjoy_axis::joy_y_pos, mmjoy_axis::joy_y_neg);

	if (js_caps.wCaps & JOYCAPS_HASZ)
		add_axis_value(js_info.dwZpos, js_caps.wZmin, js_caps.wZmax, mmjoy_axis::joy_z_pos, mmjoy_axis::joy_z_neg);

	if (js_caps.wCaps & JOYCAPS_HASR)
		add_axis_value(js_info.dwRpos, js_caps.wRmin, js_caps.wRmax, mmjoy_axis::joy_r_pos, mmjoy_axis::joy_r_neg);

	if (js_caps.wCaps & JOYCAPS_HASU)
		add_axis_value(js_info.dwUpos, js_caps.wUmin, js_caps.wUmax, mmjoy_axis::joy_u_pos, mmjoy_axis::joy_u_neg);

	if (js_caps.wCaps & JOYCAPS_HASV)
		add_axis_value(js_info.dwVpos, js_caps.wVmin, js_caps.wVmax, mmjoy_axis::joy_v_pos, mmjoy_axis::joy_v_neg);

	return button_values;
}

int mm_joystick_handler::GetIDByName(const std::string& name)
{
	for (auto dev : m_devices)
	{
		if (dev.second.device_name == name)
			return dev.first;
	}
	return -1;
}

bool mm_joystick_handler::GetMMJOYDevice(int index, MMJOYDevice* dev)
{
	if (!dev)
	{
		return false;
	}

	JOYINFOEX js_info;
	JOYCAPS js_caps;
	js_info.dwSize = sizeof(js_info);
	js_info.dwFlags = JOY_RETURNALL;
	joyGetDevCaps(index, &js_caps, sizeof(js_caps));

	if (joyGetPosEx(index, &js_info) != JOYERR_NOERROR)
		return false;

	char drv[32];
	wcstombs(drv, js_caps.szPname, 31);

	LOG_NOTICE(GENERAL, "Joystick nr.%d found. Driver: %s", index, drv);

	dev->device_id = index;
	dev->device_name = m_name_string + std::to_string(index + 1); // Controllers 1-n in GUI
	dev->device_info = js_info;
	dev->device_caps = js_caps;

	return true;
}

#endif
