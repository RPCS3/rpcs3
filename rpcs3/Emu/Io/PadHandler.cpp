#include "stdafx.h"
#include "PadHandler.h"
#include "Emu/system_utils.hpp"
#include "Input/pad_thread.h"
#include "Input/product_info.h"

cfg_input g_cfg_input;

LOG_CHANNEL(input_log, "Input");

PadHandlerBase::PadHandlerBase(pad_handler type) : m_type(type)
{
}

// Search an unordered map for a string value and return found keycode
int PadHandlerBase::FindKeyCode(const std::unordered_map<u32, std::string>& map, const cfg::string& name, bool fallback)
{
	const std::string def = name.def;
	const std::string nam = name.to_string();
	int def_code = -1;

	for (auto it = map.begin(); it != map.end(); ++it)
	{
		if (it->second == nam)
			return it->first;

		if (fallback && it->second == def)
			def_code = it->first;
	}

	if (fallback)
	{
		if (!nam.empty())
			input_log.error("int FindKeyCode for [name = %s] returned with [def_code = %d] for [def = %s]", nam, def_code, def);

		if (def_code < 0)
			def_code = 0;
	}

	return def_code;
}

long PadHandlerBase::FindKeyCode(const std::unordered_map<u64, std::string>& map, const cfg::string& name, bool fallback)
{
	const std::string def = name.def;
	const std::string nam = name.to_string();
	long def_code = -1;

	for (auto it = map.begin(); it != map.end(); ++it)
	{
		if (it->second == nam)
			return static_cast<long>(it->first);

		if (fallback && it->second == def)
			def_code = static_cast<long>(it->first);
	}

	if (fallback)
	{
		if (!nam.empty())
			input_log.error("long FindKeyCode for [name = %s] returned with [def_code = %d] for [def = %s]", nam, def_code, def);

		if (def_code < 0)
			def_code = 0;
	}

	return def_code;
}

// Search an unordered map for a string value and return found keycode
int PadHandlerBase::FindKeyCodeByString(const std::unordered_map<u32, std::string>& map, const std::string& name, bool fallback)
{
	for (auto it = map.begin(); it != map.end(); ++it)
	{
		if (it->second == name)
			return it->first;
	}

	if (fallback)
	{
		if (!name.empty())
			input_log.error("long FindKeyCodeByString for [name = %s] returned with 0", name);
		return 0;
	}

	return -1;
}

// Search an unordered map for a string value and return found keycode
long PadHandlerBase::FindKeyCodeByString(const std::unordered_map<u64, std::string>& map, const std::string& name, bool fallback)
{
	for (auto it = map.begin(); it != map.end(); ++it)
	{
		if (it->second == name)
			return static_cast<long>(it->first);
	}

	if (fallback)
	{
		if (!name.empty())
			input_log.error("long FindKeyCodeByString for [name = %s] returned with 0", name);
		return 0;
	}

	return -1;
}

// Get new scaled value between 0 and 255 based on its minimum and maximum
float PadHandlerBase::ScaledInput(s32 raw_value, int minimum, int maximum)
{
	// value based on max range converted to [0, 1]
	const float val = static_cast<float>(std::clamp(raw_value, minimum, maximum) - minimum) / (abs(maximum) + abs(minimum));
	return 255.0f * val;
}

// Get new scaled value between -255 and 255 based on its minimum and maximum
float PadHandlerBase::ScaledInput2(s32 raw_value, int minimum, int maximum)
{
	// value based on max range converted to [0, 1]
	const float val = static_cast<float>(std::clamp(raw_value, minimum, maximum) - minimum) / (abs(maximum) + abs(minimum));
	return (510.0f * val) - 255.0f;
}

// Get normalized trigger value based on the range defined by a threshold
u16 PadHandlerBase::NormalizeTriggerInput(u16 value, int threshold) const
{
	if (value <= threshold || threshold >= trigger_max)
	{
		return static_cast<u16>(0);
	}
	else if (threshold <= trigger_min)
	{
		return static_cast<u16>(ScaledInput(value, trigger_min, trigger_max));
	}
	else
	{
		const s32 val = static_cast<s32>(static_cast<float>(trigger_max) * (value - threshold) / (trigger_max - threshold));
		return static_cast<u16>(ScaledInput(val, trigger_min, trigger_max));
	}
}

// normalizes a directed input, meaning it will correspond to a single "button" and not an axis with two directions
// the input values must lie in 0+
u16 PadHandlerBase::NormalizeDirectedInput(s32 raw_value, s32 threshold, s32 maximum) const
{
	if (threshold >= maximum || maximum <= 0)
	{
		return static_cast<u16>(0);
	}

	const float val = static_cast<float>(std::clamp(raw_value, 0, maximum)) / maximum; // value based on max range converted to [0, 1]

	if (threshold <= 0)
	{
		return static_cast<u16>(255.0f * val);
	}
	else
	{
		const float thresh = static_cast<float>(threshold) / maximum; // threshold converted to [0, 1]
		return static_cast<u16>(255.0f * std::min(1.0f, (val - thresh) / (1.0f - thresh)));
	}
}

u16 PadHandlerBase::NormalizeStickInput(u16 raw_value, int threshold, int multiplier, bool ignore_threshold) const
{
	const s32 scaled_value = (multiplier * raw_value) / 100;

	if (ignore_threshold)
	{
		return static_cast<u16>(ScaledInput(scaled_value, 0, thumb_max));
	}
	else
	{
		return NormalizeDirectedInput(scaled_value, threshold, thumb_max);
	}
}

// This function normalizes stick deadzone based on the DS3's deadzone, which is ~13%
// X and Y is expected to be in (-255) to 255 range, deadzone should be in terms of thumb stick range
// return is new x and y values in 0-255 range
std::tuple<u16, u16> PadHandlerBase::NormalizeStickDeadzone(s32 inX, s32 inY, u32 deadzone) const
{
	const float dz_range = deadzone / static_cast<float>(std::abs(thumb_max)); // NOTE: thumb_max should be positive anyway

	float X = inX / 255.0f;
	float Y = inY / 255.0f;

	if (dz_range > 0.f)
	{
		const float mag = std::min(sqrtf(X * X + Y * Y), 1.f);

		if (mag <= 0)
		{
			return std::tuple<u16, u16>(ConvertAxis(X), ConvertAxis(Y));
		}

		if (mag > dz_range)
		{
			const float pos = std::lerp(0.13f, 1.f, (mag - dz_range) / (1 - dz_range));
			const float scale = pos / mag;
			X = X * scale;
			Y = Y * scale;
		}
		else
		{
			const float pos = std::lerp(0.f, 0.13f, mag / dz_range);
			const float scale = pos / mag;
			X = X * scale;
			Y = Y * scale;
		}
	}
	return std::tuple<u16, u16>(ConvertAxis(X), ConvertAxis(Y));
}

// get clamped value between 0 and 255
u16 PadHandlerBase::Clamp0To255(f32 input)
{
	return static_cast<u16>(std::clamp(input, 0.0f, 255.0f));
}

// get clamped value between 0 and 1023
u16 PadHandlerBase::Clamp0To1023(f32 input)
{
	return static_cast<u16>(std::clamp(input, 0.0f, 1023.0f));
}

// input has to be [-1,1]. result will be [0,255]
u16 PadHandlerBase::ConvertAxis(float value)
{
	return static_cast<u16>((value + 1.0)*(255.0 / 2.0));
}

// The DS3, (and i think xbox controllers) give a 'square-ish' type response, so that the corners will give (almost)max x/y instead of the ~30x30 from a perfect circle
// using a simple scale/sensitivity increase would *work* although it eats a chunk of our usable range in exchange
// this might be the best for now, in practice it seems to push the corners to max of 20x20, with a squircle_factor of 8000
// This function assumes inX and inY is already in 0-255
std::tuple<u16, u16> PadHandlerBase::ConvertToSquirclePoint(u16 inX, u16 inY, int squircle_factor)
{
	// convert inX and Y to a (-1, 1) vector;
	const f32 x = (inX - 127.5f) / 127.5f;
	const f32 y = (inY - 127.5f) / 127.5f;

	// compute angle and len of given point to be used for squircle radius
	const f32 angle = std::atan2(y, x);
	const f32 r = std::sqrt(std::pow(x, 2.f) + std::pow(y, 2.f));

	// now find len/point on the given squircle from our current angle and radius in polar coords
	// https://thatsmaths.com/2016/07/14/squircles/
	const f32 newLen = (1 + std::pow(std::sin(2 * angle), 2.f) / (squircle_factor / 1000.f)) * r;

	// we now have len and angle, convert to cartesian
	const int newX = Clamp0To255(((newLen * std::cos(angle)) + 1) * 127.5f);
	const int newY = Clamp0To255(((newLen * std::sin(angle)) + 1) * 127.5f);
	return std::tuple<u16, u16>(newX, newY);
}

std::string PadHandlerBase::name_string() const
{
	return m_name_string;
}

usz PadHandlerBase::max_devices() const
{
	return m_max_devices;
}

bool PadHandlerBase::has_config() const
{
	return b_has_config;
}

bool PadHandlerBase::has_rumble() const
{
	return b_has_rumble;
}

bool PadHandlerBase::has_deadzones() const
{
	return b_has_deadzones;
}

bool PadHandlerBase::has_led() const
{
	return b_has_led;
}

bool PadHandlerBase::has_rgb() const
{
	return b_has_rgb;
}

bool PadHandlerBase::has_battery() const
{
	return b_has_battery;
}

bool PadHandlerBase::has_pressure_intensity_button() const
{
	return b_has_pressure_intensity_button;
}

std::string PadHandlerBase::get_config_dir(pad_handler type, const std::string& title_id)
{
	if (!title_id.empty())
	{
		return rpcs3::utils::get_custom_input_config_dir(title_id) + fmt::format("%s", type) + "/";
	}
	return fs::get_config_dir() + "/InputConfigs/" + fmt::format("%s", type) + "/";
}

std::string PadHandlerBase::get_config_filename(int i, const std::string& title_id)
{
	if (!title_id.empty() && fs::is_file(rpcs3::utils::get_custom_input_config_path(title_id)))
	{
		const std::string path = rpcs3::utils::get_custom_input_config_dir(title_id) + g_cfg_input.player[i]->handler.to_string() + "/" + g_cfg_input.player[i]->profile.to_string() + ".yml";
		if (fs::is_file(path))
		{
			return path;
		}
	}
	return fs::get_config_dir() + "/InputConfigs/" + g_cfg_input.player[i]->handler.to_string() + "/" + g_cfg_input.player[i]->profile.to_string() + ".yml";
}

void PadHandlerBase::init_configs()
{
	int index = 0;

	for (u32 i = 0; i < MAX_GAMEPADS; i++)
	{
		if (g_cfg_input.player[i]->handler == m_type)
		{
			init_config(&m_pad_configs[index], get_config_filename(i, pad::g_title_id));
			index++;
		}
	}
}

void PadHandlerBase::get_next_button_press(const std::string& pad_id, const pad_callback& callback, const pad_fail_callback& fail_callback, bool get_blacklist, const std::vector<std::string>& /*buttons*/)
{
	if (get_blacklist)
		blacklist.clear();

	auto device = get_device(pad_id);

	const auto status = update_connection(device);
	if (status == connection::disconnected)
	{
		if (fail_callback)
			fail_callback(pad_id);
		return;
	}
	else if (status == connection::no_data)
		return;

	// Get the current button values
	auto data = get_button_values(device);

	// Check for each button in our list if its corresponding (maybe remapped) button or axis was pressed.
	// Return the new value if the button was pressed (aka. its value was bigger than 0 or the defined threshold)
	// Use a pair to get all the legally pressed buttons and use the one with highest value (prioritize first)
	std::pair<u16, std::string> pressed_button = { 0, "" };
	for (const auto& button : button_list)
	{
		const u32 keycode = button.first;
		const u16 value = data[keycode];

		if (!get_blacklist && std::find(blacklist.begin(), blacklist.end(), keycode) != blacklist.end())
			continue;

		const bool is_trigger = get_is_left_trigger(keycode) || get_is_right_trigger(keycode);
		const bool is_stick   = !is_trigger && (get_is_left_stick(keycode) || get_is_right_stick(keycode));
		const bool is_button = !is_trigger && !is_stick;

		if ((is_trigger && (value > m_trigger_threshold)) || (is_stick && (value > m_thumb_threshold)) || (is_button && (value > 0)))
		{
			if (get_blacklist)
			{
				blacklist.emplace_back(keycode);
				input_log.error("%s Calibration: Added key [ %d = %s ] to blacklist. Value = %d", m_type, keycode, button.second, value);
			}
			else if (value > pressed_button.first)
				pressed_button = { value, button.second };
		}
	}

	if (get_blacklist)
	{
		if (blacklist.empty())
			input_log.success("%s Calibration: Blacklist is clear. No input spam detected", m_type);
		return;
	}

	const auto preview_values = get_preview_values(data);
	const auto battery_level = get_battery_level(pad_id);

	if (callback)
	{
		if (pressed_button.first > 0)
			return callback(pressed_button.first, pressed_button.second, pad_id, battery_level, preview_values);
		else
			return callback(0, "", pad_id, battery_level, preview_values);
	}

	return;
}

void PadHandlerBase::convert_stick_values(u16& x_out, u16& y_out, const s32& x_in, const s32& y_in, const s32& deadzone, const s32& padsquircling) const
{
	// Normalize our stick axis based on the deadzone
	std::tie(x_out, y_out) = NormalizeStickDeadzone(x_in, y_in, deadzone);

	// Apply pad squircling if necessary
	if (padsquircling != 0)
	{
		std::tie(x_out, y_out) = ConvertToSquirclePoint(x_out, y_out, padsquircling);
	}
}

// Update the pad button values based on their type and thresholds. With this you can use axis or triggers as buttons or vice versa
void PadHandlerBase::TranslateButtonPress(const std::shared_ptr<PadDevice>& device, u64 keyCode, bool& pressed, u16& val, bool ignore_stick_threshold, bool ignore_trigger_threshold)
{
	if (!device || !device->config)
	{
		return;
	}

	if (get_is_left_trigger(keyCode))
	{
		pressed = val > (ignore_trigger_threshold ? 0 : device->config->ltriggerthreshold);
		val = pressed ? NormalizeTriggerInput(val, device->config->ltriggerthreshold) : 0;
	}
	else if (get_is_right_trigger(keyCode))
	{
		pressed = val > (ignore_trigger_threshold ? 0 : device->config->rtriggerthreshold);
		val = pressed ? NormalizeTriggerInput(val, device->config->rtriggerthreshold) : 0;
	}
	else if (get_is_left_stick(keyCode))
	{
		pressed = val > (ignore_stick_threshold ? 0 : device->config->lstickdeadzone);
		val = pressed ? NormalizeStickInput(val, device->config->lstickdeadzone, device->config->lstickmultiplier, ignore_stick_threshold) : 0;
	}
	else if (get_is_right_stick(keyCode))
	{
		pressed = val > (ignore_stick_threshold ? 0 : device->config->rstickdeadzone);
		val = pressed ? NormalizeStickInput(val, device->config->rstickdeadzone, device->config->rstickmultiplier, ignore_stick_threshold) : 0;
	}
	else // normal button (should in theory also support sensitive buttons)
	{
		pressed = val > 0;
		val = pressed ? val : 0;
	}
}

bool PadHandlerBase::bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device)
{
	std::shared_ptr<PadDevice> pad_device = get_device(device);
	if (!pad_device)
	{
		input_log.error("PadHandlerBase::bindPadToDevice: no PadDevice found for device '%s'", device);
		return false;
	}

	const int index = static_cast<int>(bindings.size());
	m_pad_configs[index].load();
	pad_device->config = &m_pad_configs[index];
	pad_config* profile = pad_device->config;
	if (profile == nullptr)
	{
		input_log.error("PadHandlerBase::bindPadToDevice: no profile found for device %d '%s'", index, device);
		return false;
	}

	std::array<u32, button::button_count> mapping = get_mapped_key_codes(pad_device, profile);

	u32 pclass_profile = 0x0;

	for (const auto& product : input::get_products_by_class(profile->device_class_type))
	{
		if (product.vendor_id == profile->vendor_id && product.product_id == profile->product_id)
		{
			pclass_profile = product.pclass_profile;
		}
	}

	pad->Init
	(
		CELL_PAD_STATUS_DISCONNECTED,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE,
		CELL_PAD_DEV_TYPE_STANDARD,
		profile->device_class_type,
		pclass_profile,
		profile->vendor_id,
		profile->product_id,
		profile->pressure_intensity
	);

	pad->m_buttons.emplace_back(special_button_offset, mapping[button::pressure_intensity_button], special_button_value::pressure_intensity);
	pad->m_pressure_intensity_button_index = pad->m_buttons.size() - 1;

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, mapping[button::up], CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, mapping[button::down], CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, mapping[button::left], CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, mapping[button::right], CELL_PAD_CTRL_RIGHT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, mapping[button::cross], CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, mapping[button::square], CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, mapping[button::circle], CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, mapping[button::triangle], CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, mapping[button::l1], CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, mapping[button::l2], CELL_PAD_CTRL_L2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, mapping[button::l3], CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, mapping[button::r1], CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, mapping[button::r2], CELL_PAD_CTRL_R2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, mapping[button::r3], CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, mapping[button::start], CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, mapping[button::select], CELL_PAD_CTRL_SELECT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, mapping[button::ps], 0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support
	//pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, 0x0); // Reserved (and currently not in use by rpcs3 at all)

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, mapping[button::ls_left], mapping[button::ls_right]);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, mapping[button::ls_down], mapping[button::ls_up]);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, mapping[button::rs_left], mapping[button::rs_right]);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, mapping[button::rs_down], mapping[button::rs_up]);

	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_X, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Y, 399);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Z, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_G, 512);

	pad->m_vibrateMotors.emplace_back(true, 0);
	pad->m_vibrateMotors.emplace_back(false, 0);

	bindings.emplace_back(pad_device, pad);

	return true;
}

std::array<u32, PadHandlerBase::button::button_count> PadHandlerBase::get_mapped_key_codes(const std::shared_ptr<PadDevice>& /*device*/, const pad_config* profile)
{
	std::array<u32, button::button_count> mapping;

	mapping[button::up]       = FindKeyCode(button_list, profile->up);
	mapping[button::down]     = FindKeyCode(button_list, profile->down);
	mapping[button::left]     = FindKeyCode(button_list, profile->left);
	mapping[button::right]    = FindKeyCode(button_list, profile->right);
	mapping[button::cross]    = FindKeyCode(button_list, profile->cross);
	mapping[button::square]   = FindKeyCode(button_list, profile->square);
	mapping[button::circle]   = FindKeyCode(button_list, profile->circle);
	mapping[button::triangle] = FindKeyCode(button_list, profile->triangle);
	mapping[button::start]    = FindKeyCode(button_list, profile->start);
	mapping[button::select]   = FindKeyCode(button_list, profile->select);
	mapping[button::l1]       = FindKeyCode(button_list, profile->l1);
	mapping[button::l2]       = FindKeyCode(button_list, profile->l2);
	mapping[button::l3]       = FindKeyCode(button_list, profile->l3);
	mapping[button::r1]       = FindKeyCode(button_list, profile->r1);
	mapping[button::r2]       = FindKeyCode(button_list, profile->r2);
	mapping[button::r3]       = FindKeyCode(button_list, profile->r3);
	mapping[button::ls_left]  = FindKeyCode(button_list, profile->ls_left);
	mapping[button::ls_right] = FindKeyCode(button_list, profile->ls_right);
	mapping[button::ls_down]  = FindKeyCode(button_list, profile->ls_down);
	mapping[button::ls_up]    = FindKeyCode(button_list, profile->ls_up);
	mapping[button::rs_left]  = FindKeyCode(button_list, profile->rs_left);
	mapping[button::rs_right] = FindKeyCode(button_list, profile->rs_right);
	mapping[button::rs_down]  = FindKeyCode(button_list, profile->rs_down);
	mapping[button::rs_up]    = FindKeyCode(button_list, profile->rs_up);
	mapping[button::ps]       = FindKeyCode(button_list, profile->ps);

	mapping[button::pressure_intensity_button] = FindKeyCode(button_list, profile->pressure_intensity_button);

	return mapping;
}

void PadHandlerBase::get_mapping(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	if (!device || !pad)
		return;

	auto profile = device->config;

	auto button_values = get_button_values(device);

	// Translate any corresponding keycodes to our normal DS3 buttons and triggers
	for (auto& btn : pad->m_buttons)
	{
		btn.m_value = button_values[btn.m_keyCode];
		TranslateButtonPress(device, btn.m_keyCode, btn.m_pressed, btn.m_value);
	}

	// used to get the absolute value of an axis
	s32 stick_val[4]{ 0 };

	// Translate any corresponding keycodes to our two sticks. (ignoring thresholds for now)
	for (int i = 0; i < static_cast<int>(pad->m_sticks.size()); i++)
	{
		bool pressed;

		// m_keyCodeMin is the mapped key for left or down
		const u32 key_min = pad->m_sticks[i].m_keyCodeMin;
		u16 val_min = button_values[key_min];
		TranslateButtonPress(device, key_min, pressed, val_min, true);

		// m_keyCodeMax is the mapped key for right or up
		const u32 key_max = pad->m_sticks[i].m_keyCodeMax;
		u16 val_max = button_values[key_max];
		TranslateButtonPress(device, key_max, pressed, val_max, true);

		// cancel out opposing values and get the resulting difference
		stick_val[i] = val_max - val_min;
	}

	u16 lx, ly, rx, ry;

	// Normalize and apply pad squircling
	convert_stick_values(lx, ly, stick_val[0], stick_val[1], profile->lstickdeadzone, profile->lpadsquircling);
	convert_stick_values(rx, ry, stick_val[2], stick_val[3], profile->rstickdeadzone, profile->rpadsquircling);

	if (m_type == pad_handler::ds4)
	{
		ly = 255 - ly;
		ry = 255 - ry;

		// these are added with previous value and divided to 'smooth' out the readings
		// the ds4 seems to rapidly flicker sometimes between two values and this seems to stop that

		pad->m_sticks[0].m_value = (lx + pad->m_sticks[0].m_value) / 2; // LX
		pad->m_sticks[1].m_value = (ly + pad->m_sticks[1].m_value) / 2; // LY
		pad->m_sticks[2].m_value = (rx + pad->m_sticks[2].m_value) / 2; // RX
		pad->m_sticks[3].m_value = (ry + pad->m_sticks[3].m_value) / 2; // RY
	}
	else
	{
		pad->m_sticks[0].m_value = lx;
		pad->m_sticks[1].m_value = 255 - ly;
		pad->m_sticks[2].m_value = rx;
		pad->m_sticks[3].m_value = 255 - ry;
	}
}

void PadHandlerBase::ThreadProc()
{
	for (usz i = 0; i < bindings.size(); ++i)
	{
		auto device = bindings[i].first;
		auto pad    = bindings[i].second;

		if (!device || !pad)
			continue;

		const auto status = update_connection(device);

		switch (status)
		{
		case connection::no_data:
		case connection::connected:
		{
			if (!last_connection_status[i])
			{
				input_log.success("%s device %d connected", m_type, i);
				pad->m_port_status |= CELL_PAD_STATUS_CONNECTED;
				pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
				last_connection_status[i] = true;
				connected_devices++;
			}

			if (status == connection::no_data)
				continue;

			break;
		}
		case connection::disconnected:
		{
			if (last_connection_status[i])
			{
				input_log.error("%s device %d disconnected", m_type, i);
				pad->m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
				pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
				last_connection_status[i] = false;
				connected_devices--;
			}
			continue;
		}
		default:
			break;
		}

		get_mapping(device, pad);
		get_extended_info(device, pad);
		apply_pad_data(device, pad);
	}
}
