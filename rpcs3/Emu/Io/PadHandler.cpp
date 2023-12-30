#include "stdafx.h"
#include "PadHandler.h"
#include "Emu/system_utils.hpp"
#include "Emu/system_config.h"
#include "Input/pad_thread.h"
#include "Input/product_info.h"

cfg_input g_cfg_input;

extern void pad_state_notify_state_change(usz index, u32 state);

PadHandlerBase::PadHandlerBase(pad_handler type) : m_type(type)
{
}

std::set<u32> PadHandlerBase::narrow_set(const std::set<u64>& src)
{
	if (src.empty())
		return {};

	std::set<u32> dst;
	for (const u64& s : src)
	{
		dst.insert(::narrow<u32>(s));
	}
	return dst;
}

// Get new multiplied value based on the multiplier
s32 PadHandlerBase::MultipliedInput(s32 raw_value, s32 multiplier)
{
	return (multiplier * raw_value) / 100;
}

// Get new scaled value between 0 and range based on its minimum and maximum
f32 PadHandlerBase::ScaledInput(f32 raw_value, f32 minimum, f32 maximum, f32 deadzone, f32 range)
{
	if (deadzone > 0 && deadzone > minimum)
	{
		// adjust minimum so we smoothly start at 0 when we surpass the deadzone value
		minimum = deadzone;
	}

	// convert [min, max] to [0, 1]
	const f32 val = static_cast<f32>(std::clamp(raw_value, minimum, maximum) - minimum) / (maximum - minimum);

	// convert [0, 1] to [0, range]
	return range * val;
}

// Get new scaled value between -range and range based on its minimum and maximum
f32 PadHandlerBase::ScaledAxisInput(f32 raw_value, f32 minimum, f32 maximum, f32 deadzone, f32 range)
{
	// convert [min, max] to [0, 1]
	f32 val = static_cast<f32>(std::clamp(raw_value, minimum, maximum) - minimum) / (maximum - minimum);

	if (deadzone > 0)
	{
		// convert [0, 1] to [-0.5, 0.5]
		val -= 0.5f;

		// Convert deadzone to [0, 0.5]
		deadzone = std::max(0.0f, std::min(1.0f, deadzone / maximum)) / 2.0f;

		if (val >= 0.0f)
		{
			// Apply deadzone. The result will be [0, 0.5]
			val = ScaledInput(val, 0.0f, 0.5f, deadzone, 0.5f);
		}
		else
		{
			// Apply deadzone. The result will be [-0.5, 0]
			val = ScaledInput(std::abs(val), 0, 0.5f, deadzone, 0.5f) * -1.0f;
		}

		// convert [-0.5, 0.5] back to [0, 1]
		val += 0.5f;
	}

	// convert [0, 1] to [-range, range]
	return (2.0f * range * val) - range;
}

// Get normalized trigger value based on the range defined by a threshold
u16 PadHandlerBase::NormalizeTriggerInput(u16 value, s32 threshold) const
{
	if (value <= threshold || threshold >= trigger_max)
	{
		return static_cast<u16>(0);
	}

	return static_cast<u16>(ScaledInput(static_cast<f32>(value), static_cast<f32>(trigger_min), static_cast<f32>(trigger_max), static_cast<f32>(threshold)));
}

// normalizes a directed input, meaning it will correspond to a single "button" and not an axis with two directions
// the input values must lie in 0+
u16 PadHandlerBase::NormalizeDirectedInput(s32 raw_value, s32 threshold, s32 maximum) const
{
	if (threshold >= maximum || maximum <= 0 || raw_value < 0)
	{
		return static_cast<u16>(0);
	}

	return static_cast<u16>(ScaledInput(static_cast<f32>(raw_value), 0.0f, static_cast<f32>(maximum), static_cast<f32>(threshold)));
}

u16 PadHandlerBase::NormalizeStickInput(u16 raw_value, s32 threshold, s32 multiplier, bool ignore_threshold) const
{
	const s32 scaled_value = MultipliedInput(raw_value, multiplier);

	if (ignore_threshold)
	{
		threshold = 0;
	}

	return static_cast<u16>(ScaledInput(static_cast<f32>(scaled_value), 0.0f, static_cast<f32>(thumb_max), static_cast<f32>(threshold)));
}

// This function normalizes stick deadzone based on the DS3's deadzone, which is ~13%
// X and Y is expected to be in (-255) to 255 range, deadzone should be in terms of thumb stick range
// return is new x and y values in 0-255 range
std::tuple<u16, u16> PadHandlerBase::NormalizeStickDeadzone(s32 inX, s32 inY, u32 deadzone) const
{
	const f32 dz_range = deadzone / static_cast<f32>(std::abs(thumb_max)); // NOTE: thumb_max should be positive anyway

	f32 X = inX / 255.0f;
	f32 Y = inY / 255.0f;

	if (dz_range > 0.f)
	{
		const f32 mag = std::min(sqrtf(X * X + Y * Y), 1.f);

		if (mag <= 0)
		{
			return std::tuple<u16, u16>(ConvertAxis(X), ConvertAxis(Y));
		}

		if (mag > dz_range)
		{
			const f32 pos = std::lerp(0.13f, 1.f, (mag - dz_range) / (1 - dz_range));
			const f32 scale = pos / mag;
			X = X * scale;
			Y = Y * scale;
		}
		else
		{
			const f32 pos = std::lerp(0.f, 0.13f, mag / dz_range);
			const f32 scale = pos / mag;
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
u16 PadHandlerBase::ConvertAxis(f32 value)
{
	return static_cast<u16>((value + 1.0) * (255.0 / 2.0));
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
	const int newX = Clamp0To255(std::round(((newLen * std::cos(angle)) + 1) * 127.5f));
	const int newY = Clamp0To255(std::round(((newLen * std::sin(angle)) + 1) * 127.5f));
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

bool PadHandlerBase::has_motion() const
{
	return b_has_motion;
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

bool PadHandlerBase::has_player_led() const
{
	return b_has_player_led;
}

bool PadHandlerBase::has_battery() const
{
	return b_has_battery;
}

bool PadHandlerBase::has_pressure_intensity_button() const
{
	return b_has_pressure_intensity_button;
}

void PadHandlerBase::init_configs()
{
	for (u32 i = 0; i < MAX_GAMEPADS; i++)
	{
		init_config(&m_pad_configs[i]);
	}
}

cfg_pad* PadHandlerBase::get_config(const std::string& pad_id)
{
	int index = 0;

	for (uint i = 0; i < MAX_GAMEPADS; i++)
	{
		if (g_cfg_input.player[i]->handler == m_type)
		{
			if (g_cfg_input.player[i]->device.to_string() == pad_id)
			{
				m_pad_configs[index].from_string(g_cfg_input.player[i]->config.to_string());
				return &m_pad_configs[index];
			}
			index++;
		}
	}

	return nullptr;
}

PadHandlerBase::connection PadHandlerBase::get_next_button_press(const std::string& pad_id, const pad_callback& callback, const pad_fail_callback& fail_callback, bool get_blacklist, const std::vector<std::string>& /*buttons*/)
{
	if (get_blacklist)
		blacklist.clear();

	auto device = get_device(pad_id);

	const connection status = update_connection(device);
	if (status == connection::disconnected)
	{
		if (fail_callback)
			fail_callback(pad_id);
		return status;
	}

	if (status == connection::no_data)
	{
		return status;
	}

	// Get the current button values
	auto data = get_button_values(device);

	// Check for each button in our list if its corresponding (maybe remapped) button or axis was pressed.
	// Return the new value if the button was pressed (aka. its value was bigger than 0 or the defined threshold)
	// Get all the legally pressed buttons and use the one with highest value (prioritize first)
	struct
	{
		u16 value = 0;
		std::string name;
	} pressed_button{};

	for (const auto& [keycode, name] : button_list)
	{
		const u16& value = data[keycode];

		if (!get_blacklist && blacklist.contains(keycode))
			continue;

		const bool is_trigger = get_is_left_trigger(device, keycode) || get_is_right_trigger(device, keycode);
		const bool is_stick   = !is_trigger && (get_is_left_stick(device, keycode) || get_is_right_stick(device, keycode));
		const bool is_button = !is_trigger && !is_stick;

		if ((is_trigger && (value > m_trigger_threshold)) || (is_stick && (value > m_thumb_threshold)) || (is_button && (value > 0)))
		{
			if (get_blacklist)
			{
				blacklist.insert(keycode);
				input_log.error("%s Calibration: Added key [ %d = %s ] to blacklist. Value = %d", m_type, keycode, name, value);
			}
			else if (value > pressed_button.value)
			{
				pressed_button = { .value = value, .name = name };
			}
		}
	}

	if (get_blacklist)
	{
		if (blacklist.empty())
			input_log.success("%s Calibration: Blacklist is clear. No input spam detected", m_type);
		return status;
	}

	if (callback)
	{
		const pad_preview_values preview_values = get_preview_values(data);
		const u32 battery_level = get_battery_level(pad_id);

		if (pressed_button.value > 0)
			callback(pressed_button.value, pressed_button.name, pad_id, battery_level, preview_values);
		else
			callback(0, "", pad_id, battery_level, preview_values);
	}

	return status;
}

void PadHandlerBase::get_motion_sensors(const std::string& pad_id, const motion_callback& callback, const motion_fail_callback& fail_callback, motion_preview_values preview_values, const std::array<AnalogSensor, 4>& /*sensors*/)
{
	if (!b_has_motion)
	{
		return;
	}

	// Reset sensors
	auto device = get_device(pad_id);

	const connection status = update_connection(device);
	if (status == connection::disconnected)
	{
		if (fail_callback)
			fail_callback(pad_id, std::move(preview_values));
		return;
	}

	if (status == connection::no_data || !callback)
	{
		return;
	}

	// Get the current motion values
	std::shared_ptr<Pad> pad = std::make_shared<Pad>(m_type, 0, 0, 0);
	pad->m_sensors.resize(preview_values.size(), AnalogSensor(0, 0, 0, 0, 0));
	pad_ensemble binding{pad, device, nullptr};
	get_extended_info(binding);

	for (usz i = 0; i < preview_values.size(); i++)
	{
		preview_values[i] = pad->m_sensors[i].m_value;
	}

	callback(pad_id, std::move(preview_values));
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

	if (get_is_left_trigger(device, keyCode))
	{
		pressed = val > (ignore_trigger_threshold ? 0 : device->config->ltriggerthreshold);
		val = pressed ? NormalizeTriggerInput(val, device->config->ltriggerthreshold) : 0;
	}
	else if (get_is_right_trigger(device, keyCode))
	{
		pressed = val > (ignore_trigger_threshold ? 0 : device->config->rtriggerthreshold);
		val = pressed ? NormalizeTriggerInput(val, device->config->rtriggerthreshold) : 0;
	}
	else if (get_is_left_stick(device, keyCode))
	{
		pressed = val > (ignore_stick_threshold ? 0 : device->config->lstickdeadzone);
		val = pressed ? NormalizeStickInput(val, device->config->lstickdeadzone, device->config->lstickmultiplier, ignore_stick_threshold) : 0;
	}
	else if (get_is_right_stick(device, keyCode))
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

bool PadHandlerBase::bindPadToDevice(std::shared_ptr<Pad> pad, u8 player_id)
{
	if (!pad || player_id >= g_cfg_input.player.size())
	{
		return false;
	}

	const cfg_player* player_config = g_cfg_input.player[player_id];
	if (!player_config)
	{
		return false;
	}

	std::shared_ptr<PadDevice> pad_device = get_device(player_config->device);
	if (!pad_device)
	{
		input_log.error("PadHandlerBase::bindPadToDevice: no PadDevice found for device '%s'", player_config->device.to_string());
		return false;
	}

	m_pad_configs[player_id].from_string(player_config->config.to_string());
	pad_device->config = &m_pad_configs[player_id];
	pad_device->player_id = player_id;
	cfg_pad* config = pad_device->config;
	if (config == nullptr)
	{
		input_log.error("PadHandlerBase::bindPadToDevice: no profile found for device %d '%s'", m_bindings.size(), player_config->device.to_string());
		return false;
	}

	std::array<std::set<u32>, button::button_count> mapping = get_mapped_key_codes(pad_device, config);

	u32 pclass_profile = 0x0;
	u32 capabilities = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE;

	for (const input::product_info& product : input::get_products_by_class(config->device_class_type))
	{
		if (product.vendor_id == config->vendor_id && product.product_id == config->product_id)
		{
			pclass_profile = product.pclass_profile;
			capabilities = product.capabilites;
		}
	}

	pad->Init
	(
		CELL_PAD_STATUS_DISCONNECTED,
		capabilities,
		CELL_PAD_DEV_TYPE_STANDARD,
		config->device_class_type,
		pclass_profile,
		config->vendor_id,
		config->product_id,
		config->pressure_intensity
	);

	pad->m_buttons.emplace_back(special_button_offset, mapping[button::pressure_intensity_button], special_button_value::pressure_intensity);
	pad->m_pressure_intensity_button_index = static_cast<s32>(pad->m_buttons.size()) - 1;

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
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, mapping[button::ps], CELL_PAD_CTRL_PS);

	if (pad->m_class_type == CELL_PAD_PCLASS_TYPE_SKATEBOARD)
	{
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, mapping[button::skateboard_ir_nose], CELL_PAD_CTRL_PRESS_TRIANGLE);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, mapping[button::skateboard_ir_tail], CELL_PAD_CTRL_PRESS_CIRCLE);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, mapping[button::skateboard_ir_left], CELL_PAD_CTRL_PRESS_CROSS);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, mapping[button::skateboard_ir_right], CELL_PAD_CTRL_PRESS_SQUARE);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, mapping[button::skateboard_tilt_left], CELL_PAD_CTRL_PRESS_L1);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, mapping[button::skateboard_tilt_right], CELL_PAD_CTRL_PRESS_R1);
	}

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, mapping[button::ls_left], mapping[button::ls_right]);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, mapping[button::ls_down], mapping[button::ls_up]);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, mapping[button::rs_left], mapping[button::rs_right]);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, mapping[button::rs_down], mapping[button::rs_up]);

	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_X, 0, 0, 0, DEFAULT_MOTION_X);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Y, 0, 0, 0, DEFAULT_MOTION_Y);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Z, 0, 0, 0, DEFAULT_MOTION_Z);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_G, 0, 0, 0, DEFAULT_MOTION_G);

	pad->m_vibrateMotors.emplace_back(true, 0);
	pad->m_vibrateMotors.emplace_back(false, 0);

	m_bindings.emplace_back(pad, pad_device, nullptr);

	return true;
}

std::array<std::set<u32>, PadHandlerBase::button::button_count> PadHandlerBase::get_mapped_key_codes(const std::shared_ptr<PadDevice>& device, const cfg_pad* cfg)
{
	std::array<std::set<u32>, button::button_count> mapping{};
	if (!device || !cfg)
		return mapping;

	device->trigger_code_left  = FindKeyCodes<u32, u64>(button_list, cfg->l2);
	device->trigger_code_right = FindKeyCodes<u32, u64>(button_list, cfg->r2);
	device->axis_code_left[0]  = FindKeyCodes<u32, u64>(button_list, cfg->ls_left);
	device->axis_code_left[1]  = FindKeyCodes<u32, u64>(button_list, cfg->ls_right);
	device->axis_code_left[2]  = FindKeyCodes<u32, u64>(button_list, cfg->ls_down);
	device->axis_code_left[3]  = FindKeyCodes<u32, u64>(button_list, cfg->ls_up);
	device->axis_code_right[0] = FindKeyCodes<u32, u64>(button_list, cfg->rs_left);
	device->axis_code_right[1] = FindKeyCodes<u32, u64>(button_list, cfg->rs_right);
	device->axis_code_right[2] = FindKeyCodes<u32, u64>(button_list, cfg->rs_down);
	device->axis_code_right[3] = FindKeyCodes<u32, u64>(button_list, cfg->rs_up);

	mapping[button::up]       = FindKeyCodes<u32, u32>(button_list, cfg->up);
	mapping[button::down]     = FindKeyCodes<u32, u32>(button_list, cfg->down);
	mapping[button::left]     = FindKeyCodes<u32, u32>(button_list, cfg->left);
	mapping[button::right]    = FindKeyCodes<u32, u32>(button_list, cfg->right);
	mapping[button::cross]    = FindKeyCodes<u32, u32>(button_list, cfg->cross);
	mapping[button::square]   = FindKeyCodes<u32, u32>(button_list, cfg->square);
	mapping[button::circle]   = FindKeyCodes<u32, u32>(button_list, cfg->circle);
	mapping[button::triangle] = FindKeyCodes<u32, u32>(button_list, cfg->triangle);
	mapping[button::start]    = FindKeyCodes<u32, u32>(button_list, cfg->start);
	mapping[button::select]   = FindKeyCodes<u32, u32>(button_list, cfg->select);
	mapping[button::l1]       = FindKeyCodes<u32, u32>(button_list, cfg->l1);
	mapping[button::l2]       = narrow_set(device->trigger_code_left);
	mapping[button::l3]       = FindKeyCodes<u32, u32>(button_list, cfg->l3);
	mapping[button::r1]       = FindKeyCodes<u32, u32>(button_list, cfg->r1);
	mapping[button::r2]       = narrow_set(device->trigger_code_right);
	mapping[button::r3]       = FindKeyCodes<u32, u32>(button_list, cfg->r3);
	mapping[button::ls_left]  = narrow_set(device->axis_code_left[0]);
	mapping[button::ls_right] = narrow_set(device->axis_code_left[1]);
	mapping[button::ls_down]  = narrow_set(device->axis_code_left[2]);
	mapping[button::ls_up]    = narrow_set(device->axis_code_left[3]);
	mapping[button::rs_left]  = narrow_set(device->axis_code_right[0]);
	mapping[button::rs_right] = narrow_set(device->axis_code_right[1]);
	mapping[button::rs_down]  = narrow_set(device->axis_code_right[2]);
	mapping[button::rs_up]    = narrow_set(device->axis_code_right[3]);
	mapping[button::ps]       = FindKeyCodes<u32, u32>(button_list, cfg->ps);

	mapping[button::skateboard_ir_nose]    = FindKeyCodes<u32, u32>(button_list, cfg->ir_nose);
	mapping[button::skateboard_ir_tail]    = FindKeyCodes<u32, u32>(button_list, cfg->ir_tail);
	mapping[button::skateboard_ir_left]    = FindKeyCodes<u32, u32>(button_list, cfg->ir_left);
	mapping[button::skateboard_ir_right]   = FindKeyCodes<u32, u32>(button_list, cfg->ir_right);
	mapping[button::skateboard_tilt_left]  = FindKeyCodes<u32, u32>(button_list, cfg->tilt_left);
	mapping[button::skateboard_tilt_right] = FindKeyCodes<u32, u32>(button_list, cfg->tilt_right);

	mapping[button::pressure_intensity_button] = FindKeyCodes<u32, u32>(button_list, cfg->pressure_intensity_button);

	return mapping;
}

void PadHandlerBase::get_mapping(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	if (!device || !pad)
		return;

	const cfg_pad* cfg = device->config;
	if (!cfg)
		return;

	auto button_values = get_button_values(device);

	// Find out if special buttons are pressed (introduced by RPCS3).
	// These buttons will have a delay of one cycle, but whatever.
	const bool adjust_pressure = pad->get_pressure_intensity_button_active(cfg->pressure_intensity_toggle_mode.get());
	const u32 pressure_intensity_deadzone = cfg->pressure_intensity_deadzone.get();

	// Translate any corresponding keycodes to our normal DS3 buttons and triggers
	for (Button& button : pad->m_buttons)
	{
		bool pressed{};
		u16 value{};

		for (u32 code : button.m_key_codes)
		{
			bool press{};
			u16 val = button_values[code];

			TranslateButtonPress(device, code, press, val);

			if (press)
			{
				// Modify pressure if necessary if the button was pressed
				if (adjust_pressure)
				{
					val = pad->m_pressure_intensity;
				}
				else if (pressure_intensity_deadzone > 0)
				{
					// Ignore triggers, since they have their own deadzones
					if (!get_is_left_trigger(device, code) && !get_is_right_trigger(device, code))
					{
						val = NormalizeDirectedInput(val, pressure_intensity_deadzone, 255);
					}
				}

				value = std::max(value, val);
				pressed = value > 0;
			}
		}

		button.m_value = value;
		button.m_pressed = pressed;
	}

	// used to get the absolute value of an axis
	s32 stick_val[4]{};

	// Translate any corresponding keycodes to our two sticks. (ignoring thresholds for now)
	for (usz i = 0; i < pad->m_sticks.size(); i++)
	{
		bool pressed{};
		u16 val_min{};
		u16 val_max{};

		// m_key_codes_min are the mapped keys for left or down
		for (u32 key_min : pad->m_sticks[i].m_key_codes_min)
		{
			u16 val = button_values[key_min];

			TranslateButtonPress(device, key_min, pressed, val, true);

			if (pressed)
			{
				val_min = std::max(val_min, val);
			}
		}

		// m_key_codes_max are the mapped keys for right or up
		for (u32 key_max : pad->m_sticks[i].m_key_codes_max)
		{
			u16 val = button_values[key_max];

			TranslateButtonPress(device, key_max, pressed, val, true);

			if (pressed)
			{
				val_max = std::max(val_max, val);
			}
		}

		// cancel out opposing values and get the resulting difference
		stick_val[i] = val_max - val_min;
	}

	u16 lx, ly, rx, ry;

	// Normalize and apply pad squircling
	convert_stick_values(lx, ly, stick_val[0], stick_val[1], cfg->lstickdeadzone, cfg->lpadsquircling);
	convert_stick_values(rx, ry, stick_val[2], stick_val[3], cfg->rstickdeadzone, cfg->rpadsquircling);

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

void PadHandlerBase::process()
{
	for (usz i = 0; i < m_bindings.size(); ++i)
	{
		auto& device = m_bindings[i].device;
		auto& pad    = m_bindings[i].pad;

		if (!device || !pad)
			continue;

		const connection status = update_connection(device);

		switch (status)
		{
		case connection::no_data:
		case connection::connected:
		{
			if (!last_connection_status[i])
			{
				input_log.success("%s device %d connected", m_type, i);

				pad->m_port_status |= CELL_PAD_STATUS_CONNECTED + CELL_PAD_STATUS_ASSIGN_CHANGES;
				pad_state_notify_state_change(i, CELL_PAD_STATUS_CONNECTED);

				last_connection_status[i] = true;
				connected_devices++;
			}

			if (status == connection::no_data)
			{
				// TODO: don't skip entirely if buddy device has data
				apply_pad_data(m_bindings[i]);
				continue;
			}

			break;
		}
		case connection::disconnected:
		{
			if (g_cfg.io.keep_pads_connected)
			{
				if (!last_connection_status[i])
				{
					input_log.success("%s device %d connected by force", m_type, i);

					pad->m_port_status |= CELL_PAD_STATUS_CONNECTED + CELL_PAD_STATUS_ASSIGN_CHANGES;
					pad_state_notify_state_change(i, CELL_PAD_STATUS_CONNECTED);

					last_connection_status[i] = true;
					connected_devices++;
				}
				continue;
			}

			if (last_connection_status[i])
			{
				input_log.error("%s device %d disconnected", m_type, i);

				pad->m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
				pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;

				pad_state_notify_state_change(i, CELL_PAD_STATUS_DISCONNECTED);

				last_connection_status[i] = false;
				connected_devices--;
			}
			continue;
		}
		}

		get_mapping(m_bindings[i]);
		get_extended_info(m_bindings[i]);
		apply_pad_data(m_bindings[i]);
	}
}
