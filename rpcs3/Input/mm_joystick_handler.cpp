#ifdef _WIN32
#include "mm_joystick_handler.h"

LOG_CHANNEL(input_log, "Input");

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
	u32 supported_joysticks = joyGetNumDevs();

	if (supported_joysticks <= 0)
	{
		input_log.error("mmjoy: Driver doesn't support Joysticks");
		return false;
	}

	input_log.notice("mmjoy: Driver supports %u joysticks", supported_joysticks);

	for (u32 i = 0; i < supported_joysticks; i++)
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

std::array<u32, PadHandlerBase::button::button_count> mm_joystick_handler::get_mapped_key_codes(const std::shared_ptr<PadDevice>& device, const pad_config* profile)
{
	std::array<u32, button::button_count> mapping{ 0 };

	auto joy_device = std::static_pointer_cast<MMJOYDevice>(device);
	if (!joy_device)
		return mapping;

	auto find_key = [=](const cfg::string& name)
	{
		long key = FindKeyCode(button_list, name, false);
		if (key < 0)
			key = FindKeyCode(pov_list, name, false);
		if (key < 0)
			key = FindKeyCode(axis_list, name);
		return static_cast<u64>(key);
	};

	joy_device->trigger_left  = find_key(profile->l2);
	joy_device->trigger_right = find_key(profile->r2);
	joy_device->axis_left[0]  = find_key(profile->ls_left);
	joy_device->axis_left[1]  = find_key(profile->ls_right);
	joy_device->axis_left[2]  = find_key(profile->ls_down);
	joy_device->axis_left[3]  = find_key(profile->ls_up);
	joy_device->axis_right[0] = find_key(profile->rs_left);
	joy_device->axis_right[1] = find_key(profile->rs_right);
	joy_device->axis_right[2] = find_key(profile->rs_down);
	joy_device->axis_right[3] = find_key(profile->rs_up);

	mapping[button::up]       = static_cast<u32>(find_key(profile->up));
	mapping[button::down]     = static_cast<u32>(find_key(profile->down));
	mapping[button::left]     = static_cast<u32>(find_key(profile->left));
	mapping[button::right]    = static_cast<u32>(find_key(profile->right));
	mapping[button::cross]    = static_cast<u32>(find_key(profile->cross));
	mapping[button::square]   = static_cast<u32>(find_key(profile->square));
	mapping[button::circle]   = static_cast<u32>(find_key(profile->circle));
	mapping[button::triangle] = static_cast<u32>(find_key(profile->triangle));
	mapping[button::l1]       = static_cast<u32>(find_key(profile->l1));
	mapping[button::l2]       = static_cast<u32>(joy_device->trigger_left);
	mapping[button::l3]       = static_cast<u32>(find_key(profile->l3));
	mapping[button::r1]       = static_cast<u32>(find_key(profile->r1));
	mapping[button::r2]       = static_cast<u32>(joy_device->trigger_right);
	mapping[button::r3]       = static_cast<u32>(find_key(profile->r3));
	mapping[button::start]    = static_cast<u32>(find_key(profile->start));
	mapping[button::select]   = static_cast<u32>(find_key(profile->select));
	mapping[button::ps]       = static_cast<u32>(find_key(profile->ps));
	mapping[button::ls_left]  = static_cast<u32>(joy_device->axis_left[0]);
	mapping[button::ls_right] = static_cast<u32>(joy_device->axis_left[1]);
	mapping[button::ls_down]  = static_cast<u32>(joy_device->axis_left[2]);
	mapping[button::ls_up]    = static_cast<u32>(joy_device->axis_left[3]);
	mapping[button::rs_left]  = static_cast<u32>(joy_device->axis_right[0]);
	mapping[button::rs_right] = static_cast<u32>(joy_device->axis_right[1]);
	mapping[button::rs_down]  = static_cast<u32>(joy_device->axis_right[2]);
	mapping[button::rs_up]    = static_cast<u32>(joy_device->axis_right[3]);

	return mapping;
}

void mm_joystick_handler::get_next_button_press(const std::string& padId, const std::function<void(u16, std::string, std::string, std::array<int, 6>)>& callback, const std::function<void(std::string)>& fail_callback, bool get_blacklist, const std::vector<std::string>& buttons)
{
	if (get_blacklist)
		blacklist.clear();

	if (!Init())
		return fail_callback(padId);

	static std::string cur_pad = "";
	static int id = -1;

	if (cur_pad != padId)
	{
		cur_pad = padId;
		id = GetIDByName(padId);
		if (id < 0)
		{
			input_log.error("MMJOY get_next_button_press for device [%s] failed with id = %d", padId, id);
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
					input_log.error("MMJOY Calibration: Added axis [ %d = %s ] to blacklist. Value = %d", keycode, button.second, value);
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
					input_log.error("MMJOY Calibration: Added pov [ %d = %s ] to blacklist. Value = %d", keycode, button.second, value);
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
					input_log.error("MMJOY Calibration: Added button [ %d = %s ] to blacklist. Value = %d", keycode, button.second, value);
				}
				else if (value > pressed_button.first)
					pressed_button = { value, button.second };
			}
		}

		if (get_blacklist)
		{
			if (blacklist.empty())
				input_log.success("MMJOY Calibration: Blacklist is clear. No input spam detected");
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

		std::array<int, 6> preview_values = { 0, 0, 0, 0, 0, 0 };
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

std::unordered_map<u64, u16> mm_joystick_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	auto dev = std::static_pointer_cast<MMJOYDevice>(device);
	if (!dev)
		return std::unordered_map<u64, u16>();
	return GetButtonValues(dev->device_info, dev->device_caps);
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
		return false;

	JOYINFOEX js_info;
	JOYCAPS js_caps;
	js_info.dwSize = sizeof(js_info);
	js_info.dwFlags = JOY_RETURNALL;
	joyGetDevCaps(index, &js_caps, sizeof(js_caps));

	dev->device_status = joyGetPosEx(index, &js_info);
	if (dev->device_status != JOYERR_NOERROR)
		return false;

	char drv[32];
	wcstombs(drv, js_caps.szPname, 31);

	input_log.notice("Joystick nr.%d found. Driver: %s", index, drv);

	dev->device_id = index;
	dev->device_name = m_name_string + std::to_string(index + 1); // Controllers 1-n in GUI
	dev->device_info = js_info;
	dev->device_caps = js_caps;

	return true;
}

std::shared_ptr<PadDevice> mm_joystick_handler::get_device(const std::string& device)
{
	if (!Init())
		return false;

	int id = GetIDByName(device);
	if (id < 0)
		return false;

	std::shared_ptr<MMJOYDevice> joy_device = std::make_shared<MMJOYDevice>(m_devices.at(id));
	return joy_device;
}

bool mm_joystick_handler::get_is_left_trigger(u64 keyCode)
{
	return m_dev && m_dev->trigger_left == keyCode;
}

bool mm_joystick_handler::get_is_right_trigger(u64 keyCode)
{
	return m_dev && m_dev->trigger_right == keyCode;
}

bool mm_joystick_handler::get_is_left_stick(u64 keyCode)
{
	return m_dev && std::find(m_dev->axis_left.begin(), m_dev->axis_left.end(), keyCode) != m_dev->axis_left.end();
}

bool mm_joystick_handler::get_is_right_stick(u64 keyCode)
{
	return m_dev && std::find(m_dev->axis_right.begin(), m_dev->axis_right.end(), keyCode) != m_dev->axis_right.end();
}

PadHandlerBase::connection mm_joystick_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	auto dev = std::static_pointer_cast<MMJOYDevice>(device);
	if (!dev)
		return connection::disconnected;

	const auto old_status = dev->device_status;
	dev->device_status    = joyGetPosEx(dev->device_id, &dev->device_info);

	if (dev->device_status == JOYERR_NOERROR && (old_status == JOYERR_NOERROR || GetMMJOYDevice(dev->device_id, dev.get())))
	{
		return connection::connected;
	}
	return connection::disconnected;
}

#endif
