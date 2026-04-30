#ifdef _WIN32
#include "mm_joystick_handler.h"
#include "Emu/Io/pad_config.h"

mm_joystick_handler::mm_joystick_handler() : PadHandlerBase(pad_handler::mm)
{
	button_list =
	{
		{ NO_BUTTON   , ""          },
		{ JOY_BUTTON1 , "Button 1"  },
		{ JOY_BUTTON2 , "Button 2"  },
		{ JOY_BUTTON3 , "Button 3"  },
		{ JOY_BUTTON4 , "Button 4"  },
		{ JOY_BUTTON5 , "Button 5"  },
		{ JOY_BUTTON6 , "Button 6"  },
		{ JOY_BUTTON7 , "Button 7"  },
		{ JOY_BUTTON8 , "Button 8"  },
		{ JOY_BUTTON9 , "Button 9"  },
		{ JOY_BUTTON10, "Button 10" },
		{ JOY_BUTTON11, "Button 11" },
		{ JOY_BUTTON12, "Button 12" },
		{ JOY_BUTTON13, "Button 13" },
		{ JOY_BUTTON14, "Button 14" },
		{ JOY_BUTTON15, "Button 15" },
		{ JOY_BUTTON16, "Button 16" },
		{ JOY_BUTTON17, "Button 17" },
		{ JOY_BUTTON18, "Button 18" },
		{ JOY_BUTTON19, "Button 19" },
		{ JOY_BUTTON20, "Button 20" },
		{ JOY_BUTTON21, "Button 21" },
		{ JOY_BUTTON22, "Button 22" },
		{ JOY_BUTTON23, "Button 23" },
		{ JOY_BUTTON24, "Button 24" },
		{ JOY_BUTTON25, "Button 25" },
		{ JOY_BUTTON26, "Button 26" },
		{ JOY_BUTTON27, "Button 27" },
		{ JOY_BUTTON28, "Button 28" },
		{ JOY_BUTTON29, "Button 29" },
		{ JOY_BUTTON30, "Button 30" },
		{ JOY_BUTTON31, "Button 31" },
		{ JOY_BUTTON32, "Button 32" },
		{ mmjoy_axis::joy_x_pos, "X+" },
		{ mmjoy_axis::joy_x_neg, "X-" },
		{ mmjoy_axis::joy_y_pos, "Y+" },
		{ mmjoy_axis::joy_y_neg, "Y-" },
		{ mmjoy_axis::joy_z_pos, "Z+" },
		{ mmjoy_axis::joy_z_neg, "Z-" },
		{ mmjoy_axis::joy_r_pos, "R+" },
		{ mmjoy_axis::joy_r_neg, "R-" },
		{ mmjoy_axis::joy_u_pos, "U+" },
		{ mmjoy_axis::joy_u_neg, "U-" },
		{ mmjoy_axis::joy_v_pos, "V+" },
		{ mmjoy_axis::joy_v_neg, "V-" },
		{ mmjoy_pov::joy_pov_up,    "POV Up"    },
		{ mmjoy_pov::joy_pov_right, "POV Right" },
		{ mmjoy_pov::joy_pov_down,  "POV Down"  },
		{ mmjoy_pov::joy_pov_left,  "POV Left"  }
	};

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
	cfg->ls_left.def  = ::at32(button_list, mmjoy_axis::joy_x_neg);
	cfg->ls_down.def  = ::at32(button_list, mmjoy_axis::joy_y_neg);
	cfg->ls_right.def = ::at32(button_list, mmjoy_axis::joy_x_pos);
	cfg->ls_up.def    = ::at32(button_list, mmjoy_axis::joy_y_pos);
	cfg->rs_left.def  = ::at32(button_list, mmjoy_axis::joy_z_neg);
	cfg->rs_down.def  = ::at32(button_list, mmjoy_axis::joy_r_neg);
	cfg->rs_right.def = ::at32(button_list, mmjoy_axis::joy_z_pos);
	cfg->rs_up.def    = ::at32(button_list, mmjoy_axis::joy_r_pos);
	cfg->start.def    = ::at32(button_list, static_cast<u32>(JOY_BUTTON9));
	cfg->select.def   = ::at32(button_list, static_cast<u32>(JOY_BUTTON10));
	cfg->ps.def       = ::at32(button_list, static_cast<u32>(JOY_BUTTON17));
	cfg->square.def   = ::at32(button_list, static_cast<u32>(JOY_BUTTON4));
	cfg->cross.def    = ::at32(button_list, static_cast<u32>(JOY_BUTTON3));
	cfg->circle.def   = ::at32(button_list, static_cast<u32>(JOY_BUTTON2));
	cfg->triangle.def = ::at32(button_list, static_cast<u32>(JOY_BUTTON1));
	cfg->left.def     = ::at32(button_list, mmjoy_pov::joy_pov_left);
	cfg->down.def     = ::at32(button_list, mmjoy_pov::joy_pov_down);
	cfg->right.def    = ::at32(button_list, mmjoy_pov::joy_pov_right);
	cfg->up.def       = ::at32(button_list, mmjoy_pov::joy_pov_up);
	cfg->r1.def       = ::at32(button_list, static_cast<u32>(JOY_BUTTON8));
	cfg->r2.def       = ::at32(button_list, static_cast<u32>(JOY_BUTTON6));
	cfg->r3.def       = ::at32(button_list, static_cast<u32>(JOY_BUTTON12));
	cfg->l1.def       = ::at32(button_list, static_cast<u32>(JOY_BUTTON7));
	cfg->l2.def       = ::at32(button_list, static_cast<u32>(JOY_BUTTON5));
	cfg->l3.def       = ::at32(button_list, static_cast<u32>(JOY_BUTTON11));

	cfg->pressure_intensity_button.def = ::at32(button_list, NO_BUTTON);
	cfg->analog_limiter_button.def = ::at32(button_list, NO_BUTTON);

	// Set default misc variables
	cfg->lstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
	cfg->rstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
	cfg->lstickdeadzone.def    = 0; // between 0 and 255
	cfg->rstickdeadzone.def    = 0; // between 0 and 255
	cfg->ltriggerthreshold.def = 0; // between 0 and 255
	cfg->rtriggerthreshold.def = 0; // between 0 and 255

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

		if (!get_device(i, &dev))
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

std::array<std::vector<std::set<u32>>, PadHandlerBase::button::button_count> mm_joystick_handler::get_mapped_key_codes(const std::shared_ptr<PadDevice>& device, const cfg_pad* cfg)
{
	const std::array<std::vector<std::set<u32>>, button::button_count> mapping = PadHandlerBase::get_mapped_key_codes(device, cfg);

	if (MMJOYDevice* dev = static_cast<MMJOYDevice*>(device.get()))
	{
		dev->trigger_code_left  = mapping[button::l2];
		dev->trigger_code_right = mapping[button::r2];
		dev->axis_code_left[0]  = mapping[button::ls_left];
		dev->axis_code_left[1]  = mapping[button::ls_right];
		dev->axis_code_left[2]  = mapping[button::ls_down];
		dev->axis_code_left[3]  = mapping[button::ls_up];
		dev->axis_code_right[0] = mapping[button::rs_left];
		dev->axis_code_right[1] = mapping[button::rs_right];
		dev->axis_code_right[2] = mapping[button::rs_down];
		dev->axis_code_right[3] = mapping[button::rs_up];
	}

	return mapping;
}

pad_preview_values mm_joystick_handler::get_preview_values(const std::unordered_map<u32, u16>& data, const std::vector<std::string>& buttons)
{
	pad_preview_values preview_values{};

	if (buttons.size() != 10)
		return preview_values;

	const auto get_key_value = [this, &data](const std::string& str) -> u16
	{
		u16 value{};

		// The DS3 Button is considered pressed if any configured button combination is pressed
		for (const std::set<u32>& codes : find_key_combos(button_list, str))
		{
			bool combo_pressed = !codes.empty();
			u16 combo_val = 0;

			// The button combination is only considered pressed if all the buttons are pressed
			for (u32 code : codes)
			{
				if (const auto it = data.find(code); it != data.cend())
				{
					if (it->second == 0)
					{
						combo_pressed = false;
						break;
					}

					// Take minimum combo value. Otherwise we will always end up with the max value in case an actual button is part of the combo.
					combo_val = (combo_val == 0) ? it->second : std::min(combo_val, it->second);
				}
			}

			if (combo_pressed)
			{
				value = std::max(value, combo_val);
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

	return preview_values;
}

std::unordered_map<u32, u16> mm_joystick_handler::GetButtonValues(const JOYINFOEX& js_info, const JOYCAPS& js_caps)
{
	std::unordered_map<u32, u16> button_values;

	for (const auto& entry : button_list)
	{
		switch (entry.first)
		{
		case NO_BUTTON:
		case mmjoy_axis::joy_x_pos:
		case mmjoy_axis::joy_x_neg:
		case mmjoy_axis::joy_y_pos:
		case mmjoy_axis::joy_y_neg:
		case mmjoy_axis::joy_z_pos:
		case mmjoy_axis::joy_z_neg:
		case mmjoy_axis::joy_r_pos:
		case mmjoy_axis::joy_r_neg:
		case mmjoy_axis::joy_u_pos:
		case mmjoy_axis::joy_u_neg:
		case mmjoy_axis::joy_v_pos:
		case mmjoy_axis::joy_v_neg:
		case mmjoy_pov::joy_pov_up:
		case mmjoy_pov::joy_pov_right:
		case mmjoy_pov::joy_pov_down:
		case mmjoy_pov::joy_pov_left:
			continue;
		default:
			break;
		}

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
				const auto emplacePOVs = [&](float val, u32 pov_neg, u32 pov_pos)
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

			const auto emplacePOV = [&button_values, &val](int pov)
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

	auto add_axis_value = [&](DWORD axis, UINT min, UINT max, u32 pos, u32 neg)
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

std::unordered_map<u32, u16> mm_joystick_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	MMJOYDevice* dev = static_cast<MMJOYDevice*>(device.get());
	if (!dev)
		return std::unordered_map<u32, u16>();
	return GetButtonValues(dev->device_info, dev->device_caps);
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

bool mm_joystick_handler::get_device(int index, MMJOYDevice* dev) const
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

	// Try to find a device with valid name and index
	if (auto it = m_devices.find(device); it != m_devices.end())
	{
		if (it->second && it->second->device_id != umax)
			return it->second;
	}

	// Make sure we have a device pointer (marked as invalid and unplugged)
	std::shared_ptr<MMJOYDevice> dev = create_device_by_name(device);
	m_devices.emplace(device, dev);

	return dev;
}

bool mm_joystick_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u32 keyCode)
{
	const MMJOYDevice* dev = static_cast<MMJOYDevice*>(device.get());
	return dev && std::any_of(dev->trigger_code_left.cbegin(), dev->trigger_code_left.cend(), [keyCode](const std::set<u32>& combo) { return combo.contains(keyCode); });
}

bool mm_joystick_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u32 keyCode)
{
	const MMJOYDevice* dev = static_cast<MMJOYDevice*>(device.get());
	return dev && std::any_of(dev->trigger_code_right.cbegin(), dev->trigger_code_right.cend(), [keyCode](const std::set<u32>& combo) { return combo.contains(keyCode); });
}

bool mm_joystick_handler::get_is_left_stick(const std::shared_ptr<PadDevice>& device, u32 keyCode)
{
	const MMJOYDevice* dev = static_cast<MMJOYDevice*>(device.get());
	return dev && std::any_of(dev->axis_code_left.cbegin(), dev->axis_code_left.cend(), [keyCode](const std::vector<std::set<u32>>& combos){ return std::any_of(combos.cbegin(), combos.cend(), [keyCode](const std::set<u32>& s){ return s.contains(keyCode); });});
}

bool mm_joystick_handler::get_is_right_stick(const std::shared_ptr<PadDevice>& device, u32 keyCode)
{
	const MMJOYDevice* dev = static_cast<MMJOYDevice*>(device.get());
	return dev && std::any_of(dev->axis_code_right.cbegin(), dev->axis_code_right.cend(), [keyCode](const std::vector<std::set<u32>>& combos){ return std::any_of(combos.cbegin(), combos.cend(), [keyCode](const std::set<u32>& s){ return s.contains(keyCode); });});
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
	if (get_device(dev->device_id, dev))
		return connection::connected;

	return connection::disconnected;
}

#endif
