#include "stdafx.h"
#include "openxr_handler.h"
#include "Emu/Io/openxr/openxr_thread.h"
#include "Emu/Io/pad_config.h"

namespace
{
	constexpr u32 device_count = 3;
	constexpr int device_both_hands = 0;
	constexpr int device_left_hand = 1;
	constexpr int device_right_hand = 2;

	// Converts an axis value between -1 and 1 into the two directions of a "button" (0 to 255)
	std::pair<u16, u16> split_axis(f32 value)
	{
		const u16 negative = static_cast<u16>(std::clamp(-value, 0.0f, 1.0f) * 255.0f);
		const u16 positive = static_cast<u16>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
		return { negative, positive };
	}
}

openxr_handler::openxr_handler()
	: PadHandlerBase(pad_handler::openxr)
{
	// Unique names for the config files and our pad settings dialog
	button_list = { { key_none, "" } };

	const std::array<std::pair<key_id, std::string_view>, static_cast<u32>(key_id::count)> key_names =
	{{
		{ key_id::trigger, "Trigger" },
		{ key_id::trigger_click, "Trigger Click" },
		{ key_id::grip, "Grip" },
		{ key_id::grip_click, "Grip Click" },
		{ key_id::stick_left, "Stick X-" },
		{ key_id::stick_right, "Stick X+" },
		{ key_id::stick_down, "Stick Y-" },
		{ key_id::stick_up, "Stick Y+" },
		{ key_id::stick_click, "Stick Click" },
		{ key_id::stick_touch, "Stick Touch" },
		{ key_id::trackpad_left, "Trackpad X-" },
		{ key_id::trackpad_right, "Trackpad X+" },
		{ key_id::trackpad_down, "Trackpad Y-" },
		{ key_id::trackpad_up, "Trackpad Y+" },
		{ key_id::trackpad_click, "Trackpad Click" },
		{ key_id::trackpad_touch, "Trackpad Touch" },
		{ key_id::a, "A" },
		{ key_id::b, "B" },
		{ key_id::x, "X" },
		{ key_id::y, "Y" },
		{ key_id::menu, "Menu" },
		{ key_id::select, "Select" },
	}};

	for (const auto& [key, name] : key_names)
	{
		button_list[get_key_code(left_hand, key)] = fmt::format("L %s", name);
		button_list[get_key_code(right_hand, key)] = fmt::format("R %s", name);
	}

	init_configs();

	// Define border values
	thumb_max = 255;
	trigger_min = 0;
	trigger_max = 255;

	// Set capabilities
	b_has_config = true;
	b_has_rumble = true;
	b_has_deadzones = true;
	b_has_motion = true;
	b_has_led = false;
	b_has_rgb = false;
	b_has_player_led = false;
	b_has_battery = false;
	b_has_battery_led = false;
	b_has_orientation = false;

	m_name_string = "OpenXR #";
	m_max_devices = device_count;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold   = thumb_max / 2;
}

void openxr_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	const auto key = [this](hand h, key_id id) -> const std::string&
	{
		return ::at32(button_list, get_key_code(h, id));
	};

	// Alternatives for controllers that lack some inputs (e.g. Index controllers have A/B on both hands and no X/Y)
	const auto either = [this](hand h1, key_id id1, hand h2, key_id id2)
	{
		return cfg_pad::make_button_string(button_list, {{get_key_code(h1, id1)}, {get_key_code(h2, id2)}});
	};

	// Set default button mapping
	cfg->ls_left.def  = key(left_hand, key_id::stick_left);
	cfg->ls_down.def  = key(left_hand, key_id::stick_down);
	cfg->ls_right.def = key(left_hand, key_id::stick_right);
	cfg->ls_up.def    = key(left_hand, key_id::stick_up);
	cfg->rs_left.def  = key(right_hand, key_id::stick_left);
	cfg->rs_down.def  = key(right_hand, key_id::stick_down);
	cfg->rs_right.def = key(right_hand, key_id::stick_right);
	cfg->rs_up.def    = key(right_hand, key_id::stick_up);
	cfg->start.def    = key(right_hand, key_id::trackpad_click);
	cfg->select.def   = key(left_hand, key_id::trackpad_click);
	cfg->ps.def       = ::at32(button_list, key_none);
	cfg->square.def   = either(left_hand, key_id::x, left_hand, key_id::a);
	cfg->cross.def    = key(right_hand, key_id::a);
	cfg->circle.def   = key(right_hand, key_id::b);
	cfg->triangle.def = either(left_hand, key_id::y, left_hand, key_id::b);
	cfg->left.def     = key(left_hand, key_id::trackpad_left);
	cfg->down.def     = key(left_hand, key_id::trackpad_down);
	cfg->right.def    = key(left_hand, key_id::trackpad_right);
	cfg->up.def       = key(left_hand, key_id::trackpad_up);
	cfg->r1.def       = either(right_hand, key_id::grip, right_hand, key_id::grip_click);
	cfg->r2.def       = key(right_hand, key_id::trigger);
	cfg->r3.def       = key(right_hand, key_id::stick_click);
	cfg->l1.def       = either(left_hand, key_id::grip, left_hand, key_id::grip_click);
	cfg->l2.def       = key(left_hand, key_id::trigger);
	cfg->l3.def       = key(left_hand, key_id::stick_click);

	cfg->pressure_intensity_button.def = ::at32(button_list, key_none);
	cfg->analog_limiter_button.def = ::at32(button_list, key_none);
	cfg->orientation_reset_button.def = ::at32(button_list, key_none);

	// Set default misc variables
	cfg->lstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
	cfg->rstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
	cfg->lstickdeadzone.def    = 40; // between 0 and 255
	cfg->rstickdeadzone.def    = 40; // between 0 and 255
	cfg->ltriggerthreshold.def = 0;  // between 0 and 255
	cfg->rtriggerthreshold.def = 0;  // between 0 and 255

	// apply defaults
	cfg->from_default();
}

int openxr_handler::get_device_number(const std::string& pad_id) const
{
	for (u32 i = 0; i < device_count; i++)
	{
		if (pad_id == m_name_string + std::to_string(i + 1)) // Controllers 1-n in GUI
			return static_cast<int>(i);
	}

	return -1;
}

std::array<bool, 2> openxr_handler::get_uses_hand(int device_number)
{
	return {
		device_number == device_both_hands || device_number == device_left_hand,
		device_number == device_both_hands || device_number == device_right_hand
	};
}

std::vector<pad_list_entry> openxr_handler::list_devices()
{
	std::vector<pad_list_entry> list;

	if (!openxr::is_openxr_available())
	{
		return list;
	}

	// We can't know if the controllers are connected before the session runs, so we always list all devices
	for (u32 i = 0; i < device_count; i++)
	{
		list.emplace_back(m_name_string + std::to_string(i + 1), false); // Controllers 1-n in GUI
	}

	return list;
}

std::shared_ptr<PadDevice> openxr_handler::get_device(const std::string& device)
{
	const int device_number = get_device_number(device);
	if (device_number < 0)
		return nullptr;

	std::shared_ptr<openxr_device> dev = std::make_shared<openxr_device>();
	dev->uses_hand = get_uses_hand(device_number);
	return dev;
}

void openxr_handler::set_vibration(const std::array<bool, 2>& uses_hand, u8 large_motor, u8 small_motor) const
{
	if (uses_hand[left_hand] && uses_hand[right_hand])
	{
		// Low frequency on the left, high frequency on the right, like on a DualShock
		openxr::set_controller_vibration(left_hand, large_motor / 255.0f);
		openxr::set_controller_vibration(right_hand, small_motor / 255.0f);
		return;
	}

	const f32 amplitude = std::max(large_motor, small_motor) / 255.0f;

	for (u32 h = 0; h < uses_hand.size(); h++)
	{
		if (uses_hand[h])
		{
			openxr::set_controller_vibration(h, amplitude);
		}
	}
}

void openxr_handler::SetPadData(const std::string& padId, u8 /*player_id*/, u8 large_motor, u8 small_motor, s32 /*r*/, s32 /*g*/, s32 /*b*/, bool /*player_led*/, bool /*battery_led*/, u32 /*battery_led_brightness*/)
{
	const int device_number = get_device_number(padId);
	if (device_number < 0)
		return;

	set_vibration(get_uses_hand(device_number), large_motor, small_motor);
}

bool openxr_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& /*device*/, u32 keyCode)
{
	return keyCode == get_key_code(left_hand, key_id::trigger);
}

bool openxr_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& /*device*/, u32 keyCode)
{
	return keyCode == get_key_code(right_hand, key_id::trigger);
}

// Thumbsticks and trackpads use the stick deadzones
bool openxr_handler::is_stick(hand h, u32 keyCode)
{
	for (key_id key : { key_id::stick_left, key_id::stick_right, key_id::stick_down, key_id::stick_up,
		key_id::trackpad_left, key_id::trackpad_right, key_id::trackpad_down, key_id::trackpad_up })
	{
		if (keyCode == get_key_code(h, key))
			return true;
	}

	return false;
}

bool openxr_handler::get_is_left_stick(const std::shared_ptr<PadDevice>& /*device*/, u32 keyCode)
{
	return is_stick(left_hand, keyCode);
}

bool openxr_handler::get_is_right_stick(const std::shared_ptr<PadDevice>& /*device*/, u32 keyCode)
{
	return is_stick(right_hand, keyCode);
}

PadHandlerBase::connection openxr_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	openxr_device* dev = static_cast<openxr_device*>(device.get());
	if (!dev)
		return connection::disconnected;

	dev->states = {};

	// Fails if there is no OpenXR session yet
	if (!openxr::get_controller_states(dev->states))
		return connection::disconnected;

	// A controller counts as connected while the runtime tracks it
	for (u32 h = 0; h < dev->uses_hand.size(); h++)
	{
		if (!dev->uses_hand[h])
			continue;

		if (dev->states[h].poses[openxr::action_id::grip_pose].is_active)
		{
			return connection::connected;
		}
	}

	return connection::disconnected;
}

void openxr_handler::get_extended_info(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	openxr_device* dev = static_cast<openxr_device*>(device.get());
	if (!dev || !pad)
		return;

	// The DS3 motion sensors come from one controller. With both controllers we use the right one.
	const hand motion_hand = dev->uses_hand[right_hand] ? right_hand : left_hand;

	// The aim pose points forward like a gamepad held in front of you (+X right, +Y up, +Z back)
	const openxr::pose_state& pose = dev->states[motion_hand].poses[openxr::action_id::aim_pose];

	if (!pose.has_motion)
	{
		pad->m_sensors[0].m_value = DEFAULT_MOTION_X;
		pad->m_sensors[1].m_value = DEFAULT_MOTION_Y;
		pad->m_sensors[2].m_value = DEFAULT_MOTION_Z;
		pad->m_sensors[3].m_value = DEFAULT_MOTION_G;
		pad->move_data.has_absolute_position = false;
		return;
	}

	// For the OpenXR PS Move handler
	update_move_data(*dev, pose, pad->move_data);

	// Convert to ds3. The axes are inverted (the ds3 reads -1G on Y while resting flat). The ds3 resolution is 113/G.
	pad->m_sensors[0].m_value = Clamp0To1023(pose.accelerometer.x * -1 * MOTION_ONE_G + 512);
	pad->m_sensors[1].m_value = Clamp0To1023(pose.accelerometer.y * -1 * MOTION_ONE_G + 512);
	pad->m_sensors[2].m_value = Clamp0To1023(pose.accelerometer.z * -1 * MOTION_ONE_G + 512);

	// The ds3 only has a yaw gyro. Its resolution is 123/90°/sec.
	pad->m_sensors[3].m_value = Clamp0To1023(rad_to_degree(pose.gyro.y) * (123.f / 90.f) + 512);
}

static ps_move_data::vect<4> multiply_quaternions(const ps_move_data::vect<4>& a, const ps_move_data::vect<4>& b)
{
	// (x, y, z, w)
	return ps_move_data::vect<4>({
		a.w() * b.x() + a.x() * b.w() + a.y() * b.z() - a.z() * b.y(),
		a.w() * b.y() - a.x() * b.z() + a.y() * b.w() + a.z() * b.x(),
		a.w() * b.z() + a.x() * b.y() - a.y() * b.x() + a.z() * b.w(),
		a.w() * b.w() - a.x() * b.x() - a.y() * b.y() - a.z() * b.z()
	});
}

void openxr_handler::update_move_data(openxr_device& dev, const openxr::pose_state& pose, ps_move_data& move_data) const
{
	// Where the virtual PS Eye camera is, relative to the initial head pose in the OpenXR reference space (in meters).
	// The player faces it. The reference space has +X right, +Y up and +Z back, just like the PS Move world coordinates.
	constexpr f32 camera_distance = 2.0f;
	constexpr f32 camera_height = -0.25f;

	// The aim pose has the same axes as the PS Move (+X right, +Y up, +Z from the sphere to the handle),
	// so its identity orientation is the PS Move default orientation (pointing at the camera with the buttons up).
	const ps_move_data::vect<4> orientation({ pose.pose.orientation.x, pose.pose.orientation.y, pose.pose.orientation.z, pose.pose.orientation.w });

	// The game told us that the controller currently points at the camera (see cellGemSetYaw).
	// Unlike the real PS Move, we know where up is, so we only correct the heading (a rotation around the vertical axis).
	// Pitch and roll stay absolute, and later rotations stay rotations in the world.
	if (std::exchange(move_data.orientation_reset_requested, false))
	{
		// The direction the controller points in (-Z of the aim pose), projected onto the floor
		const ps_move_data::vect<3> forward = ps_move_data::rotate_vector(orientation, ps_move_data::vect<3>({ 0.0f, 0.0f, -1.0f }));

		if (std::abs(forward.x()) > 1e-3f || std::abs(forward.z()) > 1e-3f)
		{
			// Heading of the controller around +Y. 0 means pointing at the camera (-Z), positive means turned to the left.
			const f32 heading = std::atan2(-forward.x(), -forward.z());

			// Rotate by -heading around +Y in the world, so that the controller points at the camera again
			dev.orientation_offset = ps_move_data::vect<4>({ 0.0f, std::sin(-heading / 2.0f), 0.0f, std::cos(-heading / 2.0f) });
		}
	}

	// The offset is a rotation in the world, so it is applied from the left
	move_data.quaternion = multiply_quaternions(dev.orientation_offset, orientation);

	// Position of the sphere in mm relative to the camera
	move_data.position_world = ps_move_data::vect<3>({
		pose.pose.position.x * 1000.0f,
		(pose.pose.position.y - camera_height) * 1000.0f,
		(pose.pose.position.z + camera_distance) * 1000.0f
	});
	move_data.has_absolute_position = true;

	// The raw sensor values of a PS Move use +X right, +Y forward (to the sphere) and +Z up (to the buttons)
	move_data.accelerometer = ps_move_data::vect<3>({ pose.accelerometer.x, -pose.accelerometer.z, pose.accelerometer.y });
	move_data.gyro = ps_move_data::vect<3>({ pose.gyro.x, -pose.gyro.z, pose.gyro.y });

	// The angular velocity in world coordinates
	move_data.angvel_world = ps_move_data::rotate_vector(move_data.quaternion, ps_move_data::vect<3>({ pose.gyro.x, pose.gyro.y, pose.gyro.z }));
}

void openxr_handler::apply_pad_data(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	openxr_device* dev = static_cast<openxr_device*>(device.get());
	if (!dev || !pad || !dev->config)
		return;

	const u8 speed_large = dev->config->get_large_motor_speed(pad->m_vibrate_motors);
	const u8 speed_small = dev->config->get_small_motor_speed(pad->m_vibrate_motors);

	dev->new_output_data |= dev->large_motor != speed_large || dev->small_motor != speed_small;
	dev->large_motor = speed_large;
	dev->small_motor = speed_small;

	if (dev->new_output_data)
	{
		// The OpenXR thread applies and renews the vibration, so we only have to send changes
		set_vibration(dev->uses_hand, speed_large, speed_small);
		dev->new_output_data = false;
	}
}

std::unordered_map<u32, u16> openxr_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u32, u16> values;

	openxr_device* dev = static_cast<openxr_device*>(device.get());
	if (!dev)
		return values;

	for (u32 i = 0; i < dev->uses_hand.size(); i++)
	{
		if (!dev->uses_hand[i])
			continue;

		const hand h = static_cast<hand>(i);
		const openxr::controller_state& state = dev->states[i];

		const auto set_bool = [&](key_id key, openxr::action_id action)
		{
			values[get_key_code(h, key)] = state.booleans[action] ? 255 : 0;
		};

		const auto set_float = [&](key_id key, openxr::action_id action)
		{
			values[get_key_code(h, key)] = static_cast<u16>(std::clamp(state.floats[action], 0.0f, 1.0f) * 255.0f);
		};

		const auto set_axes = [&](key_id left, key_id right, key_id down, key_id up, openxr::action_id action)
		{
			const XrVector2f& vec = state.vectors2f[action];

			const auto [x_neg, x_pos] = split_axis(vec.x);
			const auto [y_neg, y_pos] = split_axis(vec.y);

			values[get_key_code(h, left)] = x_neg;
			values[get_key_code(h, right)] = x_pos;
			values[get_key_code(h, down)] = y_neg;
			values[get_key_code(h, up)] = y_pos;
		};

		set_float(key_id::trigger, openxr::action_id::trigger_analog);
		set_bool(key_id::trigger_click, openxr::action_id::trigger_click);
		set_float(key_id::grip, openxr::action_id::squeeze_analog);
		set_bool(key_id::grip_click, openxr::action_id::squeeze_click);
		set_axes(key_id::stick_left, key_id::stick_right, key_id::stick_down, key_id::stick_up, openxr::action_id::thumbstick);
		set_bool(key_id::stick_click, openxr::action_id::thumbstick_click);
		set_bool(key_id::stick_touch, openxr::action_id::thumbstick_touch);
		set_axes(key_id::trackpad_left, key_id::trackpad_right, key_id::trackpad_down, key_id::trackpad_up, openxr::action_id::trackpad);
		set_bool(key_id::trackpad_click, openxr::action_id::trackpad_click);
		set_bool(key_id::trackpad_touch, openxr::action_id::trackpad_touch);
		set_bool(key_id::a, openxr::action_id::a_button);
		set_bool(key_id::b, openxr::action_id::b_button);
		set_bool(key_id::x, openxr::action_id::x_button);
		set_bool(key_id::y, openxr::action_id::y_button);
		set_bool(key_id::menu, openxr::action_id::menu_button);
		set_bool(key_id::select, openxr::action_id::select_button);
	}

	return values;
}

pad_preview_values openxr_handler::get_preview_values(const std::unordered_map<u32, u16>& data, const std::vector<std::string>& /*buttons*/)
{
	const auto get = [&data](hand h, key_id key) -> int
	{
		const auto it = data.find(get_key_code(h, key));
		return it != data.cend() ? it->second : 0;
	};

	return {
		get(left_hand, key_id::trigger),
		get(right_hand, key_id::trigger),
		get(left_hand, key_id::stick_right) - get(left_hand, key_id::stick_left),
		get(left_hand, key_id::stick_up) - get(left_hand, key_id::stick_down),
		get(right_hand, key_id::stick_right) - get(right_hand, key_id::stick_left),
		get(right_hand, key_id::stick_up) - get(right_hand, key_id::stick_down)
	};
}
