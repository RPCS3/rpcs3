#ifdef _WIN32
#include "mm_joystick_handler.h"
#include "Emu/Io/pad_config.h"

mm_joystick_handler::mm_joystick_handler() : PadHandlerBase(pad_handler::mm)
{
	init_configs();

	// Define border values
	thumb_max = 255;
	trigger_min = 0;
	trigger_max = 255;

	// set capabilities
	b_has_config = true;
	b_has_deadzones = true;

	m_name_string = "Joystick #";

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
}

void mm_joystick_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
	cfg->ls_left.def  = ::at32(axis_list, mmjoy_axis::joy_x_neg);
	cfg->ls_down.def  = ::at32(axis_list, mmjoy_axis::joy_y_neg);
	cfg->ls_right.def = ::at32(axis_list, mmjoy_axis::joy_x_pos);
	cfg->ls_up.def    = ::at32(axis_list, mmjoy_axis::joy_y_pos);
	cfg->rs_left.def  = ::at32(axis_list, mmjoy_axis::joy_z_neg);
	cfg->rs_down.def  = ::at32(axis_list, mmjoy_axis::joy_r_neg);
	cfg->rs_right.def = ::at32(axis_list, mmjoy_axis::joy_z_pos);
	cfg->rs_up.def    = ::at32(axis_list, mmjoy_axis::joy_r_pos);
	cfg->start.def    = ::at32(button_list, JOY_BUTTON9);
	cfg->select.def   = ::at32(button_list, JOY_BUTTON10);
	cfg->ps.def       = ::at32(button_list, JOY_BUTTON17);
	cfg->square.def   = ::at32(button_list, JOY_BUTTON4);
	cfg->cross.def    = ::at32(button_list, JOY_BUTTON3);
	cfg->circle.def   = ::at32(button_list, JOY_BUTTON2);
	cfg->triangle.def = ::at32(button_list, JOY_BUTTON1);
	cfg->left.def     = ::at32(pov_list, JOY_POVLEFT);
	cfg->down.def     = ::at32(pov_list, JOY_POVBACKWARD);
	cfg->right.def    = ::at32(pov_list, JOY_POVRIGHT);
	cfg->up.def       = ::at32(pov_list, JOY_POVFORWARD);
	cfg->r1.def       = ::at32(button_list, JOY_BUTTON8);
	cfg->r2.def       = ::at32(button_list, JOY_BUTTON6);
	cfg->r3.def       = ::at32(button_list, JOY_BUTTON12);
	cfg->l1.def       = ::at32(button_list, JOY_BUTTON7);
	cfg->l2.def       = ::at32(button_list, JOY_BUTTON5);
	cfg->l3.def       = ::at32(button_list, JOY_BUTTON11);

	cfg->pressure_intensity_button.def = ::at32(button_list, NO_BUTTON);
	cfg->analog_limiter_button.def = ::at32(button_list, NO_BUTTON);

	// Set default misc variables
	cfg->lstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
	cfg->rstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
	cfg->lstickdeadzone.def    = 0; // between 0 and 255
	cfg->rstickdeadzone.def    = 0; // between 0 and 255
	cfg->ltriggerthreshold.def = 0; // between 0 and 255
	cfg->rtriggerthreshold.def = 0; // between 0 and 255
	cfg->lpadsquircling.def    = 8000;
	cfg->rpadsquircling.def    = 8000;

	// apply defaults
	cfg->from_default();
}

bool mm_joystick_handler::Init()
{
	if (m_is_init)
		return true;

	m_devices.clear();

	m_max_devices = joyGetNumDevs();
	if (!m_max_devices)
	{
		input_log.error("mmjoy: Driver doesn't support Joysticks");
		return false;
	}

	input_log.notice("mmjoy: Driver supports %u joysticks", m_max_devices);

	enumerate_devices();

	m_is_init = true;
	return true;
}

void mm_joystick_handler::enumerate_devices()
{
	// Mark all known devices as unplugged
	for (auto& [name, device] : m_devices)
	{
		if (!device) continue;
		device->device_status = JOYERR_UNPLUGGED;
	}

	for (int i = 0; i < static_cast<int>(m_max_devices); i++)
	{
		MMJOYDevice dev;

		if (GetMMJOYDevice(i, &dev) == false)
			continue;

		auto it = m_devices.find(dev.device_name);
		if (it == m_devices.end())
		{
			// Create a new device
			m_devices.emplace(dev.device_name, std::make_shared<MMJOYDevice>(dev));
			continue;
		}

		const auto& device = it->second;

		if (!device)
			continue;

		// Update the device (don't update the base class members)
		device->device_id = dev.device_id;
		device->device_name = dev.device_name;
		device->device_info = dev.device_info;
		device->device_caps = dev.device_caps;
		device->device_status = dev.device_status;
	}
}

std::vector<pad_list_entry> mm_joystick_handler::list_devices()
{
	std::vector<pad_list_entry> devices;

	if (!Init())
		return devices;

	enumerate_devices();

	for (const auto& [name, dev] : m_devices)
	{
		if (!dev) continue;
		devices.emplace_back(name, false);
	}

	return devices;
}

template <typename T>
std::set<T> mm_joystick_handler::find_keys(const cfg::string& cfg_string) const
{
	return find_keys<T>(cfg_pad::get_buttons(cfg_string));
}

template <typename T>
std::set<T> mm_joystick_handler::find_keys(const std::vector<std::string>& names) const
{
	std::set<T> keys;

	for (const T& k : FindKeyCodes<u64, T>(axis_list, names)) keys.insert(k);
	for (const T& k : FindKeyCodes<u64, T>(pov_list, names)) keys.insert(k);
	for (const T& k : FindKeyCodes<u64, T>(button_list, names)) keys.insert(k);

	return keys;
}

std::array<std::set<u32>, PadHandlerBase::button::button_count> mm_joystick_handler::get_mapped_key_codes(const std::shared_ptr<PadDevice>& device, const cfg_pad* cfg)
{
	std::array<std::set<u32>, button::button_count> mapping{};

	MMJOYDevice* dev = static_cast<MMJOYDevice*>(device.get());
	if (!dev || !cfg)
		return mapping;

	dev->trigger_code_left  = find_keys<u64>(cfg->l2);
	dev->trigger_code_right = find_keys<u64>(cfg->r2);
	dev->axis_code_left[0]  = find_keys<u64>(cfg->ls_left);
	dev->axis_code_left[1]  = find_keys<u64>(cfg->ls_right);
	dev->axis_code_left[2]  = find_keys<u64>(cfg->ls_down);
	dev->axis_code_left[3]  = find_keys<u64>(cfg->ls_up);
	dev->axis_code_right[0] = find_keys<u64>(cfg->rs_left);
	dev->axis_code_right[1] = find_keys<u64>(cfg->rs_right);
	dev->axis_code_right[2] = find_keys<u64>(cfg->rs_down);
	dev->axis_code_right[3] = find_keys<u64>(cfg->rs_up);

	mapping[button::up]       = find_keys<u32>(cfg->up);
	mapping[button::down]     = find_keys<u32>(cfg->down);
	mapping[button::left]     = find_keys<u32>(cfg->left);
	mapping[button::right]    = find_keys<u32>(cfg->right);
	mapping[button::cross]    = find_keys<u32>(cfg->cross);
	mapping[button::square]   = find_keys<u32>(cfg->square);
	mapping[button::circle]   = find_keys<u32>(cfg->circle);
	mapping[button::triangle] = find_keys<u32>(cfg->triangle);
	mapping[button::l1]       = find_keys<u32>(cfg->l1);
	mapping[button::l2]       = narrow_set(dev->trigger_code_left);
	mapping[button::l3]       = find_keys<u32>(cfg->l3);
	mapping[button::r1]       = find_keys<u32>(cfg->r1);
	mapping[button::r2]       = narrow_set(dev->trigger_code_right);
	mapping[button::r3]       = find_keys<u32>(cfg->r3);
	mapping[button::start]    = find_keys<u32>(cfg->start);
	mapping[button::select]   = find_keys<u32>(cfg->select);
	mapping[button::ps]       = find_keys<u32>(cfg->ps);
	mapping[button::ls_left]  = narrow_set(dev->axis_code_left[0]);
	mapping[button::ls_right] = narrow_set(dev->axis_code_left[1]);
	mapping[button::ls_down]  = narrow_set(dev->axis_code_left[2]);
	mapping[button::ls_up]    = narrow_set(dev->axis_code_left[3]);
	mapping[button::rs_left]  = narrow_set(dev->axis_code_right[0]);
	mapping[button::rs_right] = narrow_set(dev->axis_code_right[1]);
	mapping[button::rs_down]  = narrow_set(dev->axis_code_right[2]);
	mapping[button::rs_up]    = narrow_set(dev->axis_code_right[3]);

	mapping[button::skateboard_ir_nose]    = find_keys<u32>(cfg->ir_nose);
	mapping[button::skateboard_ir_tail]    = find_keys<u32>(cfg->ir_tail);
	mapping[button::skateboard_ir_left]    = find_keys<u32>(cfg->ir_left);
	mapping[button::skateboard_ir_right]   = find_keys<u32>(cfg->ir_right);
	mapping[button::skateboard_tilt_left]  = find_keys<u32>(cfg->tilt_left);
	mapping[button::skateboard_tilt_right] = find_keys<u32>(cfg->tilt_right);

	if (b_has_pressure_intensity_button)
	{
		mapping[button::pressure_intensity_button] = find_keys<u32>(cfg->pressure_intensity_button);
	}

	if (b_has_analog_limiter_button)
	{
		mapping[button::analog_limiter_button] = find_keys<u32>(cfg->analog_limiter_button);
	}

	return mapping;
}

PadHandlerBase::connection mm_joystick_handler::get_next_button_press(const std::string& padId, const pad_callback& callback, const pad_fail_callback& fail_callback, gui_call_type call_type, const std::vector<std::string>& buttons)
{
	if (call_type == gui_call_type::blacklist)
		m_blacklist.clear();

	if (call_type == gui_call_type::reset_input || call_type == gui_call_type::blacklist)
		m_min_button_values.clear();

	if (!Init())
	{
		if (fail_callback)
			fail_callback(padId);
		return connection::disconnected;
	}

	static std::string cur_pad;
	static int id = -1;

	if (cur_pad != padId)
	{
		cur_pad = padId;
		const auto dev = get_device_by_name(padId);
		id = dev ? static_cast<int>(dev->device_id) : -1;
		if (id < 0)
		{
			input_log.error("MMJOY get_next_button_press for device [%s] failed with id = %d", padId, id);
			if (fail_callback)
				fail_callback(padId);
			return connection::disconnected;
		}
	}

	JOYINFOEX js_info{};
	JOYCAPS js_caps{};
	js_info.dwSize = sizeof(js_info);
	js_info.dwFlags = JOY_RETURNALL;
	MMRESULT status = joyGetDevCaps(id, &js_caps, sizeof(js_caps));

	if (status == JOYERR_NOERROR)
		status = joyGetPosEx(id, &js_info);

	switch (status)
	{
	case JOYERR_UNPLUGGED:
	{
		if (fail_callback)
			fail_callback(padId);
		return connection::disconnected;
	}
	case JOYERR_NOERROR:
	{
		if (call_type == gui_call_type::get_connection)
		{
			return connection::connected;
		}

		auto data = GetButtonValues(js_info, js_caps);

		// Check for each button in our list if its corresponding (maybe remapped) button or axis was pressed.
		// Return the new value if the button was pressed (aka. its value was bigger than 0 or the defined threshold)
		// Get all the legally pressed buttons and use the one with highest value (prioritize first)
		struct
		{
			u16 value = 0;
			std::string name;
		} pressed_button{};

		const auto set_button_press = [&](const u64& keycode, const std::string& name, std::string_view type, u16 threshold)
		{
			if (call_type != gui_call_type::blacklist && m_blacklist.contains(keycode))
				return;

			const u16 value = data[keycode];
			u16& min_value = m_min_button_values[keycode];

			if (call_type == gui_call_type::reset_input || value < min_value)
			{
				min_value = value;
				return;
			}

			if (value <= threshold)
				return;

			if (call_type == gui_call_type::blacklist)
			{
				m_blacklist.insert(keycode);
				input_log.error("MMJOY Calibration: Added %s [ %d = %s ] to blacklist. Value = %d", type, keycode, name, value);
				return;
			}

			const u16 diff = value > min_value ? value - min_value : 0;

			if (diff > button_press_threshold && value > pressed_button.value)
			{
				pressed_button = { .value = value, .name = name };
			}
		};

		for (const auto& [keycode, name] : axis_list)
		{
			set_button_press(keycode, name, "axis"sv, m_thumb_threshold);
		}

		for (const auto& [keycode, name] : pov_list)
		{
			set_button_press(keycode, name, "pov"sv, 0);
		}

		for (const auto& [keycode, name] : button_list)
		{
			if (keycode == NO_BUTTON)
				continue;

			set_button_press(keycode, name, "button"sv, 0);
		}

		if (call_type == gui_call_type::reset_input)
		{
			return connection::no_data;
		}

		if (call_type == gui_call_type::blacklist)
		{
			if (m_blacklist.empty())
				input_log.success("MMJOY Calibration: Blacklist is clear. No input spam detected");
			return connection::connected;
		}

		if (callback)
		{
			pad_preview_values preview_values{};
			if (buttons.size() == 10)
			{
				const auto get_key_value = [this, &data](const std::string& str) -> u16
				{
					u16 value{};
					for (u32 key_code : find_keys<u32>(cfg_pad::get_buttons(str)))
					{
						if (const auto it = data.find(key_code); it != data.cend())
						{
							value = std::max(value, it->second);
						}
					}
					return value;
				};
				preview_values[0] = get_key_value(buttons[0]);
				preview_values[1] = get_key_value(buttons[1]);
				preview_values[2] = get_key_value(buttons[3]) - get_key_value(buttons[2]);
				preview_values[3] = get_key_value(buttons[5]) - get_key_value(buttons[4]);
				preview_values[4] = get_key_value(buttons[7]) - get_key_value(buttons[6]);
				preview_values[5] = get_key_value(buttons[9]) - get_key_value(buttons[8]);
			}

			if (pressed_button.value > 0)
				callback(pressed_button.value, pressed_button.name, padId, 0, std::move(preview_values));
			else
				callback(0, "", padId, 0, std::move(preview_values));
		}

		return connection::connected;
	}
	default:
		return connection::disconnected;
	}

	return connection::no_data;
}

std::unordered_map<u64, u16> mm_joystick_handler::GetButtonValues(const JOYINFOEX& js_info, const JOYCAPS& js_caps)
{
	std::unordered_map<u64, u16> button_values;

	for (const auto& entry : button_list)
	{
		if (entry.first == NO_BUTTON)
			continue;

		button_values.emplace(entry.first, (js_info.dwButtons & entry.first) ? 255 : 0);
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

				const float rad = static_cast<float>(js_info.dwPOV / 100 * acos(-1) / 180);
				emplacePOVs(cosf(rad) * 255.0f, JOY_POVBACKWARD, JOY_POVFORWARD);
				emplacePOVs(sinf(rad) * 255.0f, JOY_POVLEFT, JOY_POVRIGHT);
			}
		}
		else if (js_caps.wCaps & JOYCAPS_POV4DIR)
		{
			const int val = static_cast<int>(js_info.dwPOV);

			auto emplacePOV = [&button_values, &val](int pov)
			{
				const int cw = pov + 4500, ccw = pov - 4500;
				const bool pressed = (val == pov) || (val == cw) || (ccw < 0 ? val == 36000 - std::abs(ccw) : val == ccw);
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
		constexpr f32 deadzone = 0.0f;
		const float val = ScaledAxisInput(static_cast<f32>(axis), static_cast<f32>(min), static_cast<f32>(max), deadzone);
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
	MMJOYDevice* dev = static_cast<MMJOYDevice*>(device.get());
	if (!dev)
		return std::unordered_map<u64, u16>();
	return GetButtonValues(dev->device_info, dev->device_caps);
}

std::shared_ptr<mm_joystick_handler::MMJOYDevice> mm_joystick_handler::get_device_by_name(const std::string& name)
{
	// Try to find a device with valid name and index
	if (auto it = m_devices.find(name); it != m_devices.end())
	{
		if (it->second && it->second->device_id != umax)
			return it->second;
	}

	// Make sure we have a device pointer (marked as invalid and unplugged)
	std::shared_ptr<MMJOYDevice> dev = create_device_by_name(name);
	m_devices.emplace(name, dev);

	return dev;
}

std::shared_ptr<mm_joystick_handler::MMJOYDevice> mm_joystick_handler::create_device_by_name(const std::string& name)
{
	std::shared_ptr<MMJOYDevice> dev = std::make_shared<MMJOYDevice>();
	dev->device_name = name;

	// Assign the proper index if possible
	if (name.size() > m_name_string.size() && m_max_devices > 0)
	{
		u64 index = 0;
		std::string_view suffix(name.begin() + m_name_string.size(), name.end());

		if (try_to_uint64(&index, suffix, 1, m_max_devices))
		{
			dev->device_id = ::narrow<u32>(index - 1);
		}
	}

	return dev;
}

bool mm_joystick_handler::GetMMJOYDevice(int index, MMJOYDevice* dev) const
{
	if (!dev)
		return false;

	JOYINFOEX js_info{};
	JOYCAPS js_caps{};
	js_info.dwSize = sizeof(js_info);
	js_info.dwFlags = JOY_RETURNALL;

	dev->device_status = joyGetDevCaps(index, &js_caps, sizeof(js_caps));
	if (dev->device_status != JOYERR_NOERROR)
		return false;

	dev->device_status = joyGetPosEx(index, &js_info);
	if (dev->device_status != JOYERR_NOERROR)
		return false;

	char drv[MAXPNAMELEN]{};
	wcstombs(drv, js_caps.szPname, MAXPNAMELEN - 1);

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
		return nullptr;

	return get_device_by_name(device);
}

bool mm_joystick_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode)
{
	const MMJOYDevice* dev = static_cast<MMJOYDevice*>(device.get());
	return dev && dev->trigger_code_left.contains(keyCode);
}

bool mm_joystick_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode)
{
	const MMJOYDevice* dev = static_cast<MMJOYDevice*>(device.get());
	return dev && dev->trigger_code_right.contains(keyCode);
}

bool mm_joystick_handler::get_is_left_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode)
{
	const MMJOYDevice* dev = static_cast<MMJOYDevice*>(device.get());
	return dev && std::any_of(dev->axis_code_left.cbegin(), dev->axis_code_left.cend(), [&keyCode](const std::set<u64>& s){ return s.contains(keyCode); });
}

bool mm_joystick_handler::get_is_right_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode)
{
	const MMJOYDevice* dev = static_cast<MMJOYDevice*>(device.get());
	return dev && std::any_of(dev->axis_code_right.cbegin(), dev->axis_code_right.cend(), [&keyCode](const std::set<u64>& s){ return s.contains(keyCode); });
}

PadHandlerBase::connection mm_joystick_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	MMJOYDevice* dev = static_cast<MMJOYDevice*>(device.get());
	if (!dev || dev->device_id == umax)
		return connection::disconnected;

	// Quickly check if the device is connected and fetch the button values
	const auto old_status = dev->device_status;
	dev->device_status    = joyGetPosEx(dev->device_id, &dev->device_info);

	// The device is connected and was connected
	if (dev->device_status == JOYERR_NOERROR && old_status == JOYERR_NOERROR)
		return connection::connected;

	// The device is not connected or was not connected
	if (dev->device_status != JOYERR_NOERROR)
	{
		// Only try to reconnect once every now and then.
		const steady_clock::time_point now = steady_clock::now();
		const s64 elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - dev->last_update).count();

		if (elapsed_ms < 1000)
			return connection::disconnected;

		dev->last_update = now;
	}

	// Try to connect properly again
	if (GetMMJOYDevice(dev->device_id, dev))
		return connection::connected;

	return connection::disconnected;
}

#endif
