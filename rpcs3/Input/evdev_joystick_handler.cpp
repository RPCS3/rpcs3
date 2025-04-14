// This makes debugging on windows less painful
//#define HAVE_LIBEVDEV

#ifdef HAVE_LIBEVDEV

#include "Input/product_info.h"
#include "Emu/Io/pad_config.h"
#include "evdev_joystick_handler.h"
#include "util/logs.hpp"

#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <cmath>

LOG_CHANNEL(evdev_log, "evdev");

bool positive_axis::load()
{
	if (fs::file cfg_file{ cfg_name, fs::read })
	{
		return from_string(cfg_file.to_string());
	}

	from_default();
	return false;
}

void positive_axis::save() const
{
	fs::pending_file file(cfg_name);

	if (file.file)
	{
		file.file.write(to_string());
		file.commit();
	}
}

bool positive_axis::exist() const
{
	return fs::is_file(cfg_name);
}

evdev_joystick_handler::evdev_joystick_handler()
    : PadHandlerBase(pad_handler::evdev)
{
	init_configs();

	// Define border values
	thumb_max     = 255;
	trigger_min   = 0;
	trigger_max   = 255;

	// set capabilities
	b_has_config    = true;
	b_has_rumble    = true;
	b_has_motion    = true;
	b_has_deadzones = true;
	b_has_orientation = true;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold   = thumb_max / 2;

	m_dev = std::make_shared<EvdevDevice>();
}

evdev_joystick_handler::~evdev_joystick_handler()
{
	close_devices();
}

void evdev_joystick_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
	cfg->ls_left.def  = ::at32(rev_axis_list, ABS_X);
	cfg->ls_down.def  = ::at32(axis_list, ABS_Y);
	cfg->ls_right.def = ::at32(axis_list, ABS_X);
	cfg->ls_up.def    = ::at32(rev_axis_list, ABS_Y);
	cfg->rs_left.def  = ::at32(rev_axis_list, ABS_RX);
	cfg->rs_down.def  = ::at32(axis_list, ABS_RY);
	cfg->rs_right.def = ::at32(axis_list, ABS_RX);
	cfg->rs_up.def    = ::at32(rev_axis_list, ABS_RY);
	cfg->start.def    = ::at32(button_list, BTN_START);
	cfg->select.def   = ::at32(button_list, BTN_SELECT);
	cfg->ps.def       = ::at32(button_list, BTN_MODE);
	cfg->square.def   = ::at32(button_list, BTN_X);
	cfg->cross.def    = ::at32(button_list, BTN_A);
	cfg->circle.def   = ::at32(button_list, BTN_B);
	cfg->triangle.def = ::at32(button_list, BTN_Y);
	cfg->left.def     = ::at32(rev_axis_list, ABS_HAT0X);
	cfg->down.def     = ::at32(axis_list, ABS_HAT0Y);
	cfg->right.def    = ::at32(axis_list, ABS_HAT0X);
	cfg->up.def       = ::at32(rev_axis_list, ABS_HAT0Y);
	cfg->r1.def       = ::at32(button_list, BTN_TR);
	cfg->r2.def       = ::at32(axis_list, ABS_RZ);
	cfg->r3.def       = ::at32(button_list, BTN_THUMBR);
	cfg->l1.def       = ::at32(button_list, BTN_TL);
	cfg->l2.def       = ::at32(axis_list, ABS_Z);
	cfg->l3.def       = ::at32(button_list, BTN_THUMBL);

	cfg->motion_sensor_x.axis.def = ::at32(motion_axis_list, ABS_X);
	cfg->motion_sensor_y.axis.def = ::at32(motion_axis_list, ABS_Y);
	cfg->motion_sensor_z.axis.def = ::at32(motion_axis_list, ABS_Z);
	cfg->motion_sensor_g.axis.def = ::at32(motion_axis_list, ABS_RY); // DS3 uses the yaw axis for gyros

	cfg->pressure_intensity_button.def = ::at32(button_list, NO_BUTTON);
	cfg->analog_limiter_button.def = ::at32(button_list, NO_BUTTON);
	cfg->orientation_reset_button.def = ::at32(button_list, NO_BUTTON);

	// Set default misc variables
	cfg->lstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
	cfg->rstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
	cfg->lstickdeadzone.def    = 30; // between 0 and 255
	cfg->rstickdeadzone.def    = 30; // between 0 and 255
	cfg->ltriggerthreshold.def = 0;  // between 0 and 255
	cfg->rtriggerthreshold.def = 0;  // between 0 and 255
	cfg->lpadsquircling.def    = 8000;
	cfg->rpadsquircling.def    = 8000;

	// apply defaults
	cfg->from_default();
}

std::unordered_map<u32, std::string> evdev_joystick_handler::get_motion_axis_list() const
{
	return motion_axis_list;
}

bool evdev_joystick_handler::Init()
{
	if (m_is_init)
		return true;

	if (!m_pos_axis_config.load())
	{
		evdev_log.notice("positive_axis config missing. Using defaults");
	}

	evdev_log.notice("positive_axis config=\n%s", m_pos_axis_config.to_string());

	if (!m_pos_axis_config.exist())
		m_pos_axis_config.save();

	for (const auto& node : m_pos_axis_config.get_nodes())
	{
		if (node && *static_cast<cfg::_bool*>(node))
		{
			const std::string& name = node->get_name();
			const int code = libevdev_event_code_from_name(EV_ABS, name.c_str());
			if (code < 0)
				evdev_log.error("Failed to read axis name from %s. [code = %d] [name = %s]", m_pos_axis_config.cfg_name, code, name);
			else
				m_positive_axis.insert(code);
		}
	}

	m_is_init = true;
	return true;
}

std::string evdev_joystick_handler::get_device_name(const libevdev* dev)
{
	std::string name;

	if (const char* raw_name = libevdev_get_name(dev))
		name = raw_name;

	if (name.empty())
	{
		if (const char* unique = libevdev_get_uniq(dev))
			name = unique;

		if (name.empty())
			name = "Unknown Device";
	}

	return name;
}

bool evdev_joystick_handler::update_device(const std::shared_ptr<PadDevice>& device)
{
	EvdevDevice* evdev_device = static_cast<EvdevDevice*>(device.get());
	if (!evdev_device)
		return false;

	const std::string& path = evdev_device->path;
	libevdev*& dev = evdev_device->device;

	const bool was_connected = dev != nullptr;

	if (access(path.c_str(), R_OK) == -1)
	{
		if (was_connected)
		{
			const int fd = libevdev_get_fd(dev);
			libevdev_free(dev);
			close(fd);
			dev = nullptr;
		}

		evdev_log.error("Joystick %s is not present or accessible [previous status: %d]", path.c_str(), was_connected ? 1 : 0);
		return false;
	}

	if (was_connected)
		return true; // It's already been connected, and the js is still present.

	const int fd = open(path.c_str(), O_RDWR | O_NONBLOCK);
	if (fd == -1)
	{
		const int err = errno;
		evdev_log.error("Failed to open joystick: %s [errno %d]", strerror(err), err);
		return false;
	}

	const int ret = libevdev_new_from_fd(fd, &dev);
	if (ret < 0)
	{
		evdev_log.error("Failed to initialize libevdev for joystick: %s [errno %d]", strerror(-ret), -ret);
		return false;
	}

	evdev_log.notice("Opened joystick: '%s' at %s (fd %d)", get_device_name(dev), path, fd);
	return true;
}

void evdev_joystick_handler::close_devices()
{
	const auto free_device = [](EvdevDevice* evdev_device)
	{
		if (evdev_device && evdev_device->device)
		{
			const int fd = libevdev_get_fd(evdev_device->device);
			if (evdev_device->effect_id != -1)
				ioctl(fd, EVIOCRMFF, evdev_device->effect_id);
			libevdev_free(evdev_device->device);
			close(fd);
		}
	};

	for (pad_ensemble& binding : m_bindings)
	{
		free_device(static_cast<EvdevDevice*>(binding.device.get()));
		free_device(static_cast<EvdevDevice*>(binding.buddy_device.get()));
	}

	for (auto& [name, device] : m_settings_added)
	{
		free_device(static_cast<EvdevDevice*>(device.get()));
	}

	for (auto& [name, device] : m_motion_settings_added)
	{
		free_device(static_cast<EvdevDevice*>(device.get()));
	}
}

std::unordered_map<u64, std::pair<u16, bool>> evdev_joystick_handler::GetButtonValues(const std::shared_ptr<EvdevDevice>& device)
{
	std::unordered_map<u64, std::pair<u16, bool>> button_values;
	if (!device)
		return button_values;

	libevdev* dev = device->device;

	if (!Init())
		return button_values;

	for (const auto& [code, name] : button_list)
	{
		if (code == NO_BUTTON)
			continue;

		int val = 0;

		if (libevdev_fetch_event_value(dev, EV_KEY, code, &val) == 0)
			continue;

		button_values.emplace(code, std::make_pair<u16, bool>(static_cast<u16>(val > 0 ? 255 : 0), false));
	}

	for (const auto& [code, name] : axis_list)
	{
		int val = 0;

		if (libevdev_fetch_event_value(dev, EV_ABS, code, &val) == 0)
			continue;

		const int min = libevdev_get_abs_minimum(dev, code);
		const int max = libevdev_get_abs_maximum(dev, code);
		const int flat = libevdev_get_abs_flat(dev, code);

		// Triggers do not need handling of negative values
		if (min >= 0 && !m_positive_axis.contains(code))
		{
			const float fvalue = ScaledInput(val, min, max, flat);
			button_values.emplace(code, std::make_pair<u16, bool>(static_cast<u16>(fvalue), false));
			continue;
		}

		const float fvalue = ScaledAxisInput(val, min, max, flat);
		if (fvalue < 0)
			button_values.emplace(code, std::make_pair<u16, bool>(static_cast<u16>(std::abs(fvalue)), true));
		else
			button_values.emplace(code, std::make_pair<u16, bool>(static_cast<u16>(fvalue), false));
	}

	return button_values;
}

std::shared_ptr<evdev_joystick_handler::EvdevDevice> evdev_joystick_handler::get_evdev_device(const std::string& device)
{
	// Add device if not yet present
	std::shared_ptr<EvdevDevice> evdev_device = add_device(device, true);
	if (!evdev_device)
		return nullptr;

	// Check if our device is connected
	if (!update_device(evdev_device))
		return nullptr;

	return evdev_device;
}

PadHandlerBase::connection evdev_joystick_handler::get_next_button_press(const std::string& padId, const pad_callback& callback, const pad_fail_callback& fail_callback, gui_call_type call_type, const std::vector<std::string>& buttons)
{
	if (call_type == gui_call_type::blacklist)
		m_blacklist.clear();

	if (call_type == gui_call_type::reset_input || call_type == gui_call_type::blacklist)
		m_min_button_values.clear();

	// Get our evdev device
	std::shared_ptr<EvdevDevice> device = get_evdev_device(padId);
	if (!device || !device->device)
	{
		if (fail_callback)
			fail_callback(padId);
		return connection::disconnected;
	}

	libevdev* dev = device->device;

	// Try to fetch all new events from the joystick.
	input_event evt;
	bool has_new_event = false;
	int ret = LIBEVDEV_READ_STATUS_SUCCESS;
	while (ret >= 0)
	{
		if (ret == LIBEVDEV_READ_STATUS_SYNC)
		{
			// Grab any pending sync event.
			ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &evt);
		}
		else
		{
			ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &evt);
		}

		has_new_event |= ret == LIBEVDEV_READ_STATUS_SUCCESS;
	}

	if (call_type == gui_call_type::get_connection)
	{
		return has_new_event ? connection::connected : connection::no_data;
	}

	const auto data = GetButtonValues(device);

	const auto find_value = [&, this](const std::string& str)
	{
		const std::vector<std::string> names = cfg_pad::get_buttons(str);

		u16 value{};

		const auto set_value = [&value, &data](u32 code, bool dir)
		{
			if (const auto it = data.find(static_cast<u64>(code)); it != data.cend() && dir == it->second.second)
			{
				value = std::max(value, it->second.first);
			}
		};

		for (const u32 code : FindKeyCodes<u32, u32>(rev_axis_list, names))
		{
			set_value(code, true);
		}

		for (const u32 code : FindKeyCodes<u32, u32>(axis_list, names))
		{
			set_value(code, false);
		}

		for (const u32 code : FindKeyCodes<u32, u32>(button_list, names))
		{
			set_value(code, false);
		}

		return value;
	};

	pad_preview_values preview_values{};

	if (buttons.size() == 10)
	{
		preview_values[0] = find_value(buttons[0]);                          // Left Trigger
		preview_values[1] = find_value(buttons[1]);                          // Right Trigger
		preview_values[2] = find_value(buttons[3]) - find_value(buttons[2]); // Left Stick X
		preview_values[3] = find_value(buttons[5]) - find_value(buttons[4]); // Left Stick Y
		preview_values[4] = find_value(buttons[7]) - find_value(buttons[6]); // Right Stick X
		preview_values[5] = find_value(buttons[9]) - find_value(buttons[8]); // Right Stick Y
	}

	// return if nothing new has happened. ignore this to get the current state for blacklist or reset_input
	if (call_type != gui_call_type::blacklist && call_type != gui_call_type::reset_input && !has_new_event)
	{
		if (callback)
			callback(0, "", padId, 0, preview_values);
		return connection::no_data;
	}

	struct
	{
		u16 value = 0;
		std::string name;
	} pressed_button{};

	const auto set_button_press = [&](const u32 code, const std::string& name, std::string_view type, u16 threshold, int ev_type, bool is_rev_axis)
	{
		if (call_type != gui_call_type::blacklist && m_blacklist.contains(name))
			return;

		// Ignore codes that aren't part of the latest events. Otherwise we will get value 0 which will reset our min_value.
		const auto it = data.find(static_cast<u64>(code));
		if (it == data.cend())
		{
			if (call_type == gui_call_type::reset_input)
			{
				// Set to max. We won't have all the events for all the buttons or axis at this point.
				m_min_button_values[name] = 65535;

				if (ev_type == EV_ABS)
				{
					// Also set the other direction to max if it wasn't already found.
					const auto it_other_axis = is_rev_axis ? axis_list.find(code) : rev_axis_list.find(code);
					ensure(it_other_axis != (is_rev_axis ? axis_list.cend() : rev_axis_list.cend()));
					if (const std::string& other_name = it_other_axis->second; !m_min_button_values.contains(other_name))
					{
						m_min_button_values[other_name] = 65535;
					}
				}
			}
			return;
		}

		const auto& [value, is_rev_ax] = it->second;

		// If we want the value for an axis, its direction has to match the direction of the data.
		if (ev_type == EV_ABS && is_rev_axis != is_rev_ax)
			return;

		u16& min_value = m_min_button_values[name];

		if (call_type == gui_call_type::reset_input || value < min_value)
		{
			min_value = value;
			return;
		}

		if (value <= threshold)
			return;

		if (call_type == gui_call_type::blacklist)
		{
			m_blacklist.insert(name);

			if (ev_type == EV_ABS)
			{
				const int min = libevdev_get_abs_minimum(dev, code);
				const int max = libevdev_get_abs_maximum(dev, code);
				evdev_log.error("Evdev Calibration: Added %s [ %d = %s = %s ] to blacklist. [ Value = %d ] [ Min = %d ] [ Max = %d ]", type, code, libevdev_event_code_get_name(ev_type, code), name, value, min, max);
			}
			else
			{
				evdev_log.error("Evdev Calibration: Added %s [ %d = %s = %s ] to blacklist. Value = %d", type, code, libevdev_event_code_get_name(ev_type, code), name, value);
			}

			return;
		}

		const u16 diff = value > min_value ? value - min_value : 0;

		if (diff > button_press_threshold && value > pressed_button.value)
		{
			pressed_button = { .value = value, .name = name };
		}
	};

	const bool is_xbox_360_controller = padId.find("Xbox 360") != umax;
	const bool is_sony_controller = !is_xbox_360_controller && padId.find("Sony") != umax;
	const bool is_sony_guitar = is_sony_controller && padId.find("Guitar") != umax;

	for (const auto& [code, name] : button_list)
	{
		// Handle annoying useless buttons
		if (code == NO_BUTTON)
			continue;
		if (is_xbox_360_controller && code >= BTN_TRIGGER_HAPPY)
			continue;
		if (is_sony_controller && !is_sony_guitar && (code == BTN_TL2 || code == BTN_TR2))
			continue;

		set_button_press(code, name, "button"sv, 0, EV_KEY, false);
	}

	for (const auto& [code, name] : axis_list)
	{
		set_button_press(code, name, "axis"sv, m_thumb_threshold, EV_ABS, false);
	}

	for (const auto& [code, name] : rev_axis_list)
	{
		set_button_press(code, name, "rev axis"sv, m_thumb_threshold, EV_ABS, true);
	}

	if (call_type == gui_call_type::reset_input)
	{
		return connection::no_data;
	}

	if (call_type == gui_call_type::blacklist)
	{
		if (m_blacklist.empty())
			evdev_log.success("Evdev Calibration: Blacklist is clear. No input spam detected");
		return connection::connected;
	}

	if (callback)
	{
		if (pressed_button.value > 0)
			callback(pressed_button.value, pressed_button.name, padId, 0, std::move(preview_values));
		else
			callback(0, "", padId, 0, std::move(preview_values));
	}

	return connection::connected;
}

void evdev_joystick_handler::get_motion_sensors(const std::string& padId, const motion_callback& callback, const motion_fail_callback& fail_callback, motion_preview_values preview_values, const std::array<AnalogSensor, 4>& sensors)
{
	// Add device if not yet present
	std::shared_ptr<EvdevDevice> device = add_motion_device(padId, true);
	if (!device || !update_device(device) || !device->device)
	{
		if (fail_callback)
			fail_callback(padId, std::move(preview_values));
		return;
	}

	libevdev* dev = device->device;

	// Try to fetch all new events from the joystick.
	bool is_dirty = false;
	int ret = LIBEVDEV_READ_STATUS_SUCCESS;
	while (ret >= 0)
	{
		input_event evt;

		if (ret == LIBEVDEV_READ_STATUS_SYNC)
		{
			// Grab any pending sync event.
			ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_SYNC, &evt);
		}
		else
		{
			ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &evt);
		}

		if (ret == LIBEVDEV_READ_STATUS_SUCCESS && evt.type == EV_ABS)
		{
			for (usz i = 0; i < sensors.size(); i++)
			{
				const AnalogSensor& sensor = sensors[i];

				if (sensor.m_keyCode != evt.code)
					continue;

				preview_values[i] = get_sensor_value(dev, sensor, evt);;
				is_dirty = true;
			}
		}
	}

	if (ret < 0)
	{
		// -EAGAIN signifies no available events, not an actual *error*.
		if (ret != -EAGAIN)
			evdev_log.error("Failed to read latest event from motion device: %s [errno %d]", strerror(-ret), -ret);
	}

	if (callback && is_dirty)
		callback(padId, std::move(preview_values));
}

// https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/InputCommon/ControllerInterface/evdev/evdev.cpp
// https://github.com/reicast/reicast-emulator/blob/master/core/linux-dist/evdev.cpp
// http://www.infradead.org/~mchehab/kernel_docs_pdf/linux-input.pdf
void evdev_joystick_handler::SetRumble(EvdevDevice* device, u8 large, u8 small)
{
	if (!device || !device->has_rumble || device->effect_id == -2)
		return;

	const int fd = libevdev_get_fd(device->device);
	if (fd < 0)
		return;

	device->new_output_data = large != device->large_motor || small != device->small_motor;

	const auto now = steady_clock::now();
	const auto elapsed = now - device->last_output;

	// XBox One Controller can't handle faster vibration updates than ~10ms. Elite is even worse.
	// So I'll use 20ms to be on the safe side. No lag was noticable.
	if ((!device->new_output_data || elapsed <= 20ms) && elapsed <= min_output_interval)
		return;

	device->new_output_data = false;
	device->last_output = now;

	// delete the previous effect (which also stops it)
	if (device->effect_id != -1)
	{
		ioctl(fd, EVIOCRMFF, device->effect_id);
		device->effect_id = -1;
	}

	if (large == 0 && small == 0)
	{
		device->large_motor = large;
		device->small_motor = small;
		return;
	}

	ff_effect effect;

	if (libevdev_has_event_code(device->device, EV_FF, FF_RUMBLE))
	{
		effect.type                      = FF_RUMBLE;
		effect.id                        = device->effect_id;
		effect.direction                 = 0;
		effect.u.rumble.strong_magnitude = large * 257;
		effect.u.rumble.weak_magnitude   = small * 257;
		effect.replay.length             = 0;
		effect.replay.delay              = 0;
		effect.trigger.button            = 0;
		effect.trigger.interval          = 0;
	}
	else
	{
		// TODO: handle other Rumble effects
		device->effect_id = -2;
		return;
	}

	if (ioctl(fd, EVIOCSFF, &effect) == -1)
	{
		evdev_log.error("evdev SetRumble ioctl failed! [large = %d] [small = %d] [fd = %d]", large, small, fd);
		device->effect_id = -2;
	}

	device->effect_id = effect.id;

	input_event play;
	play.type  = EV_FF;
	play.code  = device->effect_id;
	play.value = 1;

	if (write(fd, &play, sizeof(play)) == -1)
	{
		evdev_log.error("evdev SetRumble write failed! [large = %d] [small = %d] [fd = %d] [effect_id = %d]", large, small, fd, device->effect_id);
		device->effect_id = -2;
	}

	device->large_motor = large;
	device->small_motor = small;
}

void evdev_joystick_handler::SetPadData(const std::string& padId, u8 /*player_id*/, u8 large_motor, u8 small_motor, s32 /* r*/, s32 /* g*/, s32 /* b*/, bool /*player_led*/, bool /*battery_led*/, u32 /*battery_led_brightness*/)
{
	// Get our evdev device
	std::shared_ptr<EvdevDevice> dev = get_evdev_device(padId);
	if (!dev)
	{
		evdev_log.error("evdev TestVibration: Device [%s] not found! [large_motor = %d] [small_motor = %d]", padId, large_motor, small_motor);
		return;
	}

	if (!dev->has_rumble)
	{
		evdev_log.error("evdev TestVibration: Device [%s] does not support rumble features! [large_motor = %d] [small_motor = %d]", padId, large_motor, small_motor);
		return;
	}

	SetRumble(static_cast<EvdevDevice*>(dev.get()), large_motor, small_motor);
}

std::vector<pad_list_entry> evdev_joystick_handler::list_devices()
{
	Init();

	std::unordered_map<std::string, u32> unique_names;
	std::vector<pad_list_entry> evdev_joystick_list;

	for (auto&& et : fs::dir{"/dev/input/"})
	{
		// Check if the entry starts with event (a 5-letter word)
		if (et.name.size() > 5 && et.name.starts_with("event"))
		{
			const int fd         = open(("/dev/input/" + et.name).c_str(), O_RDONLY | O_NONBLOCK);
			struct libevdev* dev = NULL;
			const int rc         = libevdev_new_from_fd(fd, &dev);
			if (rc < 0)
			{
				// If it's just a bad file descriptor, don't bother logging, but otherwise, log it.
				if (rc != -9)
					evdev_log.warning("Failed to connect to device at %s, the error was: %s", "/dev/input/" + et.name, strerror(-rc));
				libevdev_free(dev);
				close(fd);
				continue;
			}

			if (libevdev_has_event_type(dev, EV_ABS))
			{
				bool is_motion_device = false;
				bool is_pad_device = libevdev_has_event_type(dev, EV_KEY);

				if (!is_pad_device)
				{
					// Check if it's a motion device.
					is_motion_device = libevdev_has_property(dev, INPUT_PROP_ACCELEROMETER);
				}

				if (is_pad_device || is_motion_device)
				{
					// It's a joystick or motion device.
					std::string name = get_device_name(dev);

					if (unique_names.find(name) == unique_names.end())
						unique_names.emplace(name, 1);
					else
						name = fmt::format("%d. %s", ++unique_names[name], name);

					evdev_joystick_list.emplace_back(name, is_motion_device);
				}
			}

			libevdev_free(dev);
			close(fd);
		}
	}
	return evdev_joystick_list;
}

std::shared_ptr<evdev_joystick_handler::EvdevDevice> evdev_joystick_handler::add_device(const std::string& device, bool in_settings)
{
	if (in_settings && m_settings_added.count(device))
		return ::at32(m_settings_added, device);

	// Now we need to find the device with the same name, and make sure not to grab any duplicates.
	std::unordered_map<std::string, u32> unique_names;

	for (auto&& et : fs::dir{"/dev/input/"})
	{
		// Check if the entry starts with event (a 5-letter word)
		if (et.name.size() > 5 && et.name.starts_with("event"))
		{
			const std::string path = "/dev/input/" + et.name;
			const int fd           = open(path.c_str(), O_RDWR | O_NONBLOCK);
			struct libevdev* dev   = NULL;
			const int rc           = libevdev_new_from_fd(fd, &dev);
			if (rc < 0)
			{
				// If it's just a bad file descriptor, don't bother logging, but otherwise, log it.
				if (rc != -9)
					evdev_log.warning("Failed to connect to device at %s, the error was: %s", path, strerror(-rc));
				libevdev_free(dev);
				close(fd);
				continue;
			}

			std::string name = get_device_name(dev);

			if (unique_names.find(name) == unique_names.end())
				unique_names.emplace(name, 1);
			else
				name = fmt::format("%d. %s", ++unique_names[name], name);

			if (name == device &&
				libevdev_has_event_type(dev, EV_KEY) &&
				libevdev_has_event_type(dev, EV_ABS))
			{
				// It's a joystick.

				if (in_settings)
				{
					m_dev = std::make_shared<EvdevDevice>();
					m_settings_added[device] = m_dev;

					// Let's log axis information while we are in the settings in order to identify problems more easily.
					for (const auto& [code, axis_name] : axis_list)
					{
						if (const input_absinfo* info = libevdev_get_abs_info(dev, code))
						{
							const char* code_name = libevdev_event_code_get_name(EV_ABS, code);
							evdev_log.notice("Axis info for %s: %s (%s) => minimum=%d, maximum=%d, fuzz=%d, flat=%d, resolution=%d",
								name, code_name, axis_name, info->minimum, info->maximum, info->fuzz, info->flat, info->resolution);
						}
					}
				}

				// Alright, now that we've confirmed we haven't added this joystick yet, les do dis.
				m_dev->device     = dev;
				m_dev->path       = path;
				m_dev->has_rumble = libevdev_has_event_type(dev, EV_FF);
				m_dev->has_motion = libevdev_has_property(dev, INPUT_PROP_ACCELEROMETER);

				evdev_log.notice("Capability info for %s: rumble=%d, motion=%d", name, m_dev->has_rumble, m_dev->has_motion);

				return m_dev;
			}
			libevdev_free(dev);
			close(fd);
		}
	}
	return nullptr;
}

std::shared_ptr<evdev_joystick_handler::EvdevDevice> evdev_joystick_handler::add_motion_device(const std::string& device, bool in_settings)
{
	if (device.empty())
		return nullptr;

	if (in_settings && m_motion_settings_added.count(device))
		return ::at32(m_motion_settings_added, device);

	// Now we need to find the device with the same name, and make sure not to grab any duplicates.
	std::unordered_map<std::string, u32> unique_names;

	for (auto&& et : fs::dir{"/dev/input/"})
	{
		// Check if the entry starts with event (a 5-letter word)
		if (et.name.size() > 5 && et.name.starts_with("event"))
		{
			const std::string path = "/dev/input/" + et.name;
			const int fd           = open(path.c_str(), O_RDWR | O_NONBLOCK);
			struct libevdev* dev   = NULL;
			const int rc           = libevdev_new_from_fd(fd, &dev);
			if (rc < 0)
			{
				// If it's just a bad file descriptor, don't bother logging, but otherwise, log it.
				if (rc != -9)
					evdev_log.warning("Failed to connect to device at %s, the error was: %s", path, strerror(-rc));
				libevdev_free(dev);
				close(fd);
				continue;
			}

			std::string name = get_device_name(dev);

			if (unique_names.find(name) == unique_names.end())
				unique_names.emplace(name, 1);
			else
				name = fmt::format("%d. %s", ++unique_names[name], name);

			if (name == device &&
				libevdev_has_property(dev, INPUT_PROP_ACCELEROMETER) &&
				libevdev_has_event_type(dev, EV_ABS))
			{
				// Let's log axis information while we are in the settings in order to identify problems more easily.
				// Directional axes on this device (absolute and/or relative x, y, z) represent accelerometer data.
				// Some devices also report gyroscope data, which devices can report through the rotational axes (absolute and/or relative rx, ry, rz).
				// All other axes retain their meaning.
				// A device must not mix regular directional axes and accelerometer axes on the same event node.
				for (const auto& [code, axis_name] : axis_list)
				{
					if (const input_absinfo* info = libevdev_get_abs_info(dev, code))
					{
						const bool is_accel = code == ABS_X || code == ABS_Y || code == ABS_Z;
						const char* code_name = libevdev_event_code_get_name(EV_ABS, code);
						evdev_log.notice("Axis info for %s: %s (%s, %s) => minimum=%d, maximum=%d, fuzz=%d, flat=%d, resolution=%d",
							name, code_name, axis_name, is_accel ? "accelerometer" : "gyro", info->minimum, info->maximum, info->fuzz, info->flat, info->resolution);
					}
				}

				std::shared_ptr<EvdevDevice> motion_device = std::make_shared<EvdevDevice>();
				motion_device->device     = dev;
				motion_device->path       = path;
				motion_device->has_motion = true;

				if (in_settings)
				{
					m_motion_settings_added[device] = motion_device;
				}

				return motion_device;
			}
			libevdev_free(dev);
			close(fd);
		}
	}
	return nullptr;
}

PadHandlerBase::connection evdev_joystick_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	if (!update_device(device))
		return connection::disconnected;

	EvdevDevice* evdev_device = static_cast<EvdevDevice*>(device.get());
	if (!evdev_device || !evdev_device->device)
		return connection::disconnected;

	return connection::connected;
}

void evdev_joystick_handler::get_mapping(const pad_ensemble& binding)
{
	m_dev = std::static_pointer_cast<EvdevDevice>(binding.device);
	const auto& pad = binding.pad;

	if (!m_dev || !pad)
		return;

	libevdev* dev = m_dev->device;
	if (!dev)
		return;

	// Try to fetch all new events from the joystick.
	bool got_changes = false;
	input_event evt;
	int ret = LIBEVDEV_READ_STATUS_SUCCESS;
	while (ret >= 0)
	{
		if (ret == LIBEVDEV_READ_STATUS_SYNC)
		{
			// Grab any pending sync event.
			ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_SYNC, &evt);
		}
		else
		{
			ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &evt);
		}

		if (ret == LIBEVDEV_READ_STATUS_SUCCESS)
		{
			switch (evt.type)
			{
			case EV_KEY:
			{
				auto& wrapper = m_dev->events_by_code[evt.code];

				if (!wrapper)
				{
					wrapper.reset(new key_event_wrapper());
				}

				key_event_wrapper* key_wrapper = static_cast<key_event_wrapper*>(wrapper.get());

				if (!key_wrapper->is_initialized)
				{
					if (!button_list.contains(evt.code))
					{
						evdev_log.error("Evdev button %s (%d) is unknown. Please add it to the button list.", libevdev_event_code_get_name(EV_KEY, evt.code), evt.code);
					}

					key_wrapper->is_initialized = true;
				}

				const u16 new_value = evt.value > 0 ? 255 : 0;

				if (key_wrapper->value != new_value)
				{
					key_wrapper->value = new_value;
					got_changes = true;
				}
				break;
			}
			case EV_ABS:
			{
				auto& wrapper = m_dev->events_by_code[evt.code];

				if (!wrapper)
				{
					wrapper.reset(new axis_event_wrapper());
				}

				axis_event_wrapper* axis_wrapper = static_cast<axis_event_wrapper*>(wrapper.get());

				if (!axis_wrapper->is_initialized)
				{
					axis_wrapper->min = libevdev_get_abs_minimum(dev, evt.code);
					axis_wrapper->max = libevdev_get_abs_maximum(dev, evt.code);
					axis_wrapper->flat = libevdev_get_abs_flat(dev, evt.code);
					axis_wrapper->is_initialized = true;

					// Triggers do not need handling of negative values
					if (axis_wrapper->min >= 0 && !m_positive_axis.contains(evt.code))
					{
						axis_wrapper->is_trigger = true;
					}
				}

				// Triggers do not need handling of negative values
				if (axis_wrapper->is_trigger)
				{
					const u16 new_value = static_cast<u16>(ScaledInput(evt.value, axis_wrapper->min, axis_wrapper->max, axis_wrapper->flat));
					u16& key_value = axis_wrapper->values[false];

					if (key_value != new_value)
					{
						key_value = new_value;
						got_changes = true;
					}
				}
				else
				{
					const float fvalue = ScaledAxisInput(evt.value, axis_wrapper->min, axis_wrapper->max, axis_wrapper->flat);
					const bool is_negative = fvalue < 0;

					const u16 new_value_0 = static_cast<u16>(std::abs(fvalue));
					const u16 new_value_1 = 0; // We need to reset the other direction of this axis

					u16& key_value_0 = axis_wrapper->values[is_negative];
					u16& key_value_1 = axis_wrapper->values[!is_negative];

					if (key_value_0 != new_value_0 || key_value_1 != new_value_1)
					{
						key_value_0 = new_value_0;
						key_value_1 = new_value_1;
						got_changes = true;
					}
				}
				break;
			}
			default:
				break;
			}
		}
	}

	if (ret < 0)
	{
		// -EAGAIN signifies no available events, not an actual *error*.
		if (ret != -EAGAIN)
			evdev_log.error("Failed to read latest event from joystick: %s [errno %d]", strerror(-ret), -ret);
	}

	// Apply all events
	if (got_changes)
	{
		apply_input_events(pad);
	}
}

void evdev_joystick_handler::get_extended_info(const pad_ensemble& binding)
{
	// We use this to get motion controls from our buddy device
	const auto& device = std::static_pointer_cast<EvdevDevice>(binding.buddy_device);
	const auto& pad = binding.pad;

	if (!pad || !device || !update_device(device))
		return;

	libevdev* dev = device->device;
	if (!dev)
		return;

	// Try to fetch all new events from the joystick.
	input_event evt;
	int ret = LIBEVDEV_READ_STATUS_SUCCESS;
	while (ret >= 0)
	{
		if (ret == LIBEVDEV_READ_STATUS_SYNC)
		{
			// Grab any pending sync event.
			ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_SYNC, &evt);
		}
		else
		{
			ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &evt);
		}

		if (ret == LIBEVDEV_READ_STATUS_SUCCESS && evt.type == EV_ABS)
		{
			for (AnalogSensor& sensor : pad->m_sensors)
			{
				if (sensor.m_keyCode != evt.code)
					continue;

				sensor.m_value = get_sensor_value(dev, sensor, evt);
			}
		}
	}

	set_raw_orientation(*pad);

	if (ret < 0)
	{
		// -EAGAIN signifies no available events, not an actual *error*.
		if (ret != -EAGAIN)
			evdev_log.error("Failed to read latest event from buddy device: %s [errno %d]", strerror(-ret), -ret);
	}
}

u16 evdev_joystick_handler::get_sensor_value(const libevdev* dev, const AnalogSensor& sensor, const input_event& evt) const
{
	if (dev)
	{
		const int min = libevdev_get_abs_minimum(dev, evt.code);
		const int max = libevdev_get_abs_maximum(dev, evt.code);
		const int flat = libevdev_get_abs_flat(dev, evt.code);

		s16 value = ScaledInput(evt.value, min, max, flat, 1023.0f);

		if (sensor.m_mirrored)
		{
			value = 1023 - value;
		}

		return Clamp0To1023(value + sensor.m_shift);
	}

	return 0;
}

void evdev_joystick_handler::apply_input_events(const std::shared_ptr<Pad>& pad)
{
	if (!pad)
		return;

	const cfg_pad* cfg = m_dev->config;
	if (!cfg)
		return;

	// Find out if special buttons are pressed (introduced by RPCS3).
	// These buttons will have a delay of one cycle, but whatever.
	const bool analog_limiter_enabled = pad->get_analog_limiter_button_active(cfg->analog_limiter_toggle_mode.get(), pad->m_player_id);
	const bool adjust_pressure = pad->get_pressure_intensity_button_active(cfg->pressure_intensity_toggle_mode.get(), pad->m_player_id);
	const u32 pressure_intensity_deadzone = cfg->pressure_intensity_deadzone.get();

	const auto update_values = [&](bool& pressed, u16& final_value, bool is_stick_value, u32 code, u16 val)
	{
		bool press{};
		TranslateButtonPress(m_dev, code, press, val, analog_limiter_enabled, is_stick_value);

		if (press)
		{
			if (!is_stick_value)
			{
				// Modify pressure if necessary if the button was pressed
				if (adjust_pressure)
				{
					val = pad->m_pressure_intensity;
				}
				else if (pressure_intensity_deadzone > 0)
				{
					// Ignore triggers, since they have their own deadzones
					if (!get_is_left_trigger(m_dev, code) && !get_is_right_trigger(m_dev, code))
					{
						val = NormalizeDirectedInput(val, pressure_intensity_deadzone, 255);
					}
				}
			}

			final_value = std::max(final_value, val);
			pressed = final_value > 0;
		}
	};

	const auto process_mapped_button = [&](u32 index, bool& pressed, u16& final_value, bool is_stick_value)
	{
		const EvdevButton& evdev_button = ::at32(m_dev->all_buttons, index);
		auto& wrapper = m_dev->events_by_code[evdev_button.code];
		if (!wrapper || !wrapper->is_initialized)
		{
			return;
		}

		// We need to set the current direction and type for TranslateButtonPress
		m_dev->cur_type = wrapper->type;
		m_dev->cur_dir = evdev_button.dir;

		switch (wrapper->type)
		{
		case EV_KEY:
		{
			key_event_wrapper* key_wrapper = static_cast<key_event_wrapper*>(wrapper.get());
			update_values(pressed, final_value, is_stick_value, evdev_button.code, key_wrapper->value);
			break;
		}
		case EV_ABS: // Be careful to handle mapped axis specially
		{
			if (evdev_button.dir < 0)
			{
				evdev_log.error("Invalid axis direction = %d, Button Nr.%d", evdev_button.dir, index);
				break;
			}

			axis_event_wrapper* axis_wrapper = static_cast<axis_event_wrapper*>(wrapper.get());

			if (is_stick_value && axis_wrapper->is_trigger)
			{
				update_values(pressed, final_value, is_stick_value, evdev_button.code, axis_wrapper->values[false]);
				break;
			}

			for (const auto& [is_negative, value] : axis_wrapper->values)
			{
				// Only set value if the stick / hat is actually pointing to the correct direction.
				if (is_negative == (evdev_button.dir == 1))
				{
					update_values(pressed, final_value, is_stick_value, evdev_button.code, value);
				}
			}
			break;
		}
		default:
			fmt::throw_exception("Unknown evdev input_event_wrapper type: %d", wrapper->type);
		}
	};

	// Translate any corresponding keycodes to our normal DS3 buttons and triggers
	for (Button& button : pad->m_buttons)
	{
		bool pressed{};
		u16 final_value{};

		for (u32 index : button.m_key_codes)
		{
			process_mapped_button(index, pressed, final_value, false);
		}

		button.m_value = final_value;
		button.m_pressed = pressed;
	}

	// used to get the absolute value of an axis
	s32 stick_val[4]{};

	// Translate any corresponding keycodes to our two sticks. (ignoring thresholds for now)
	for (usz i = 0; i < pad->m_sticks.size(); i++)
	{
		bool pressed{}; // unused
		u16 val_min{};
		u16 val_max{};

		// m_key_codes_min are the mapped keys for left or down
		for (u32 index : pad->m_sticks[i].m_key_codes_min)
		{
			process_mapped_button(index, pressed, val_min, true);
		}

		// m_key_codes_max are the mapped keys for right or up
		for (u32 index : pad->m_sticks[i].m_key_codes_max)
		{
			process_mapped_button(index, pressed, val_max, true);
		}

		// cancel out opposing values and get the resulting difference. if there was no change, use the old value.
		stick_val[i] = val_max - val_min;
	}

	u16 lx, ly, rx, ry;

	// Normalize and apply pad squircling
	convert_stick_values(lx, ly, stick_val[0], stick_val[1], cfg->lstickdeadzone, cfg->lstick_anti_deadzone, cfg->lpadsquircling);
	convert_stick_values(rx, ry, stick_val[2], stick_val[3], cfg->rstickdeadzone, cfg->rstick_anti_deadzone, cfg->rpadsquircling);

	pad->m_sticks[0].m_value = lx;
	pad->m_sticks[1].m_value = 255 - ly;
	pad->m_sticks[2].m_value = rx;
	pad->m_sticks[3].m_value = 255 - ry;
}

void evdev_joystick_handler::apply_pad_data(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	EvdevDevice* evdev_device = static_cast<EvdevDevice*>(device.get());
	if (!evdev_device)
		return;

	cfg_pad* cfg = device->config;
	if (!cfg)
		return;

	// Handle vibration
	const u8 force_large = cfg->get_large_motor_speed(pad->m_vibrateMotors);
	const u8 force_small = cfg->get_small_motor_speed(pad->m_vibrateMotors);
	SetRumble(evdev_device, force_large, force_small);
}

bool evdev_joystick_handler::bindPadToDevice(std::shared_ptr<Pad> pad)
{
	if (!pad || pad->m_player_id >= g_cfg_input.player.size())
		return false;

	const cfg_player* player_config = g_cfg_input.player[pad->m_player_id];
	if (!pad)
		return false;

	Init();

	m_dev = std::make_shared<EvdevDevice>();

	m_pad_configs[pad->m_player_id].from_string(player_config->config.to_string());
	m_dev->config = &m_pad_configs[pad->m_player_id];
	m_dev->player_id = pad->m_player_id;
	cfg_pad* cfg = m_dev->config;
	if (!cfg)
		return false;

	// We need to register EvdevButtons due to their axis directions.
	const auto register_evdevbutton = [this](u32 code, bool is_axis, bool is_reverse) -> u32
	{
		const u32 index = ::narrow<u32>(m_dev->all_buttons.size());

		if (is_axis)
		{
			m_dev->all_buttons.push_back(EvdevButton{ code, is_reverse ? 1 : 0, EV_ABS });
		}
		else
		{
			m_dev->all_buttons.push_back(EvdevButton{ code, -1, EV_KEY });
		}

		return index;
	};

	const auto find_buttons = [&](const cfg::string& name) -> std::set<u32>
	{
		const std::vector<std::string> names = cfg_pad::get_buttons(name);

		// In evdev we store indices to an EvdevButton vector in our pad objects instead of the usual key codes.
		std::set<u32> indices;

		for (const u32 code : FindKeyCodes<u32, u32>(axis_list, names))
		{
			indices.insert(register_evdevbutton(code, true, false));
		}

		for (const u32 code : FindKeyCodes<u32, u32>(rev_axis_list, names))
		{
			indices.insert(register_evdevbutton(code, true, true));
		}

		for (const u32 code : FindKeyCodes<u32, u32>(button_list, names))
		{
			indices.insert(register_evdevbutton(code, false, false));
		}

		return indices;
	};

	const auto find_motion_button = [&](const cfg_sensor& sensor) -> evdev_sensor
	{
		evdev_sensor e_sensor{};
		e_sensor.type = EV_ABS;
		e_sensor.mirrored = sensor.mirrored.get();
		e_sensor.shift = sensor.shift.get();

		const std::set<u32> keys = FindKeyCodes<u32, u32>(motion_axis_list, sensor.axis);
		if (!keys.empty()) e_sensor.code = *keys.begin(); // We should only have one key for each of our sensors
		return e_sensor;
	};

	u32 pclass_profile = 0x0;

	for (const input::product_info& product : input::get_products_by_class(cfg->device_class_type))
	{
		if (product.vendor_id == cfg->vendor_id && product.product_id == cfg->product_id)
		{
			pclass_profile = product.pclass_profile;
		}
	}

	pad->Init
	(
		CELL_PAD_STATUS_DISCONNECTED,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE,
		CELL_PAD_DEV_TYPE_STANDARD,
		cfg->device_class_type,
		pclass_profile,
		cfg->vendor_id,
		cfg->product_id,
		cfg->pressure_intensity
	);

	if (b_has_pressure_intensity_button)
	{
		pad->m_buttons.emplace_back(special_button_offset, find_buttons(cfg->pressure_intensity_button), special_button_value::pressure_intensity);
		pad->m_pressure_intensity_button_index = static_cast<s32>(pad->m_buttons.size()) - 1;
	}

	if (b_has_analog_limiter_button)
	{
		pad->m_buttons.emplace_back(special_button_offset, find_buttons(cfg->analog_limiter_button), special_button_value::analog_limiter);
		pad->m_analog_limiter_button_index = static_cast<s32>(pad->m_buttons.size()) - 1;
	}

	if (b_has_orientation)
	{
		pad->m_buttons.emplace_back(special_button_offset, find_buttons(cfg->orientation_reset_button), special_button_value::orientation_reset);
		pad->m_orientation_reset_button_index = static_cast<s32>(pad->m_buttons.size()) - 1;
	}

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2,find_buttons(cfg->triangle), CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2,find_buttons(cfg->circle),   CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2,find_buttons(cfg->cross),    CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2,find_buttons(cfg->square),   CELL_PAD_CTRL_SQUARE);

	m_dev->trigger_left = find_buttons(cfg->l2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_dev->trigger_left,         CELL_PAD_CTRL_L2);

	m_dev->trigger_right = find_buttons(cfg->r2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_dev->trigger_right,        CELL_PAD_CTRL_R2);

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_buttons(cfg->l1),       CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_buttons(cfg->r1),       CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_buttons(cfg->start),    CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_buttons(cfg->select),   CELL_PAD_CTRL_SELECT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_buttons(cfg->l3),       CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_buttons(cfg->r3),       CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_buttons(cfg->ps),       CELL_PAD_CTRL_PS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_buttons(cfg->up),       CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_buttons(cfg->down),     CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_buttons(cfg->left),     CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_buttons(cfg->right),    CELL_PAD_CTRL_RIGHT);

	if (pad->m_class_type == CELL_PAD_PCLASS_TYPE_SKATEBOARD)
	{
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, find_buttons(cfg->ir_nose), CELL_PAD_CTRL_PRESS_TRIANGLE);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, find_buttons(cfg->ir_tail), CELL_PAD_CTRL_PRESS_CIRCLE);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, find_buttons(cfg->ir_left), CELL_PAD_CTRL_PRESS_CROSS);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, find_buttons(cfg->ir_right), CELL_PAD_CTRL_PRESS_SQUARE);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, find_buttons(cfg->tilt_left), CELL_PAD_CTRL_PRESS_L1);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, find_buttons(cfg->tilt_right), CELL_PAD_CTRL_PRESS_R1);
	}

	m_dev->axis_left[0]  = find_buttons(cfg->ls_right);
	m_dev->axis_left[1]  = find_buttons(cfg->ls_left);
	m_dev->axis_left[2]  = find_buttons(cfg->ls_up);
	m_dev->axis_left[3]  = find_buttons(cfg->ls_down);
	m_dev->axis_right[0] = find_buttons(cfg->rs_right);
	m_dev->axis_right[1] = find_buttons(cfg->rs_left);
	m_dev->axis_right[2] = find_buttons(cfg->rs_up);
	m_dev->axis_right[3] = find_buttons(cfg->rs_down);

	pad->m_sticks[0] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X,  m_dev->axis_left[1],  m_dev->axis_left[0]);
	pad->m_sticks[1] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y,  m_dev->axis_left[3],  m_dev->axis_left[2]);
	pad->m_sticks[2] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, m_dev->axis_right[1], m_dev->axis_right[0]);
	pad->m_sticks[3] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, m_dev->axis_right[3], m_dev->axis_right[2]);

	m_dev->axis_motion[0] = find_motion_button(cfg->motion_sensor_x);
	m_dev->axis_motion[1] = find_motion_button(cfg->motion_sensor_y);
	m_dev->axis_motion[2] = find_motion_button(cfg->motion_sensor_z);
	m_dev->axis_motion[3] = find_motion_button(cfg->motion_sensor_g);

	pad->m_sensors[0] = AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_X, m_dev->axis_motion[0].code, m_dev->axis_motion[0].mirrored, m_dev->axis_motion[0].shift, DEFAULT_MOTION_X);
	pad->m_sensors[1] = AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_Y, m_dev->axis_motion[1].code, m_dev->axis_motion[1].mirrored, m_dev->axis_motion[1].shift, DEFAULT_MOTION_Y);
	pad->m_sensors[2] = AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_Z, m_dev->axis_motion[2].code, m_dev->axis_motion[2].mirrored, m_dev->axis_motion[2].shift, DEFAULT_MOTION_Z);
	pad->m_sensors[3] = AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_G, m_dev->axis_motion[3].code, m_dev->axis_motion[3].mirrored, m_dev->axis_motion[3].shift, DEFAULT_MOTION_G);

	pad->m_vibrateMotors[0] = VibrateMotor(true, 0);
	pad->m_vibrateMotors[1] = VibrateMotor(false, 0);

	if (std::shared_ptr<EvdevDevice> evdev_device = add_device(player_config->device, false))
	{
		if (std::shared_ptr<EvdevDevice> motion_device = add_motion_device(player_config->buddy_device, false))
		{
			m_bindings.emplace_back(pad, evdev_device, motion_device);
		}
		else
		{
			m_bindings.emplace_back(pad, evdev_device, nullptr);

			if (const std::string buddy_device_name = player_config->buddy_device.to_string(); buddy_device_name != "Null")
			{
				evdev_log.warning("evdev add_motion_device in bindPadToDevice failed for device %s", buddy_device_name);
			}
		}
	}
	else
	{
		evdev_log.warning("evdev add_device in bindPadToDevice failed for device %s", player_config->device.to_string());
	}

	for (pad_ensemble& binding : m_bindings)
	{
		update_device(binding.device);
		update_device(binding.buddy_device);
	}

	return true;
}

bool evdev_joystick_handler::check_button_set(const std::set<u32>& indices, const u32 code)
{
	if (!m_dev)
		return false;

	for (u32 index : indices)
	{
		const EvdevButton& button = ::at32(m_dev->all_buttons, index);
		if (button.code == code && button.type == m_dev->cur_type && button.dir == m_dev->cur_dir)
		{
			return true;
		}
	}

	return false;
}

bool evdev_joystick_handler::check_button_sets(const std::array<std::set<u32>, 4>& sets, const u32 code)
{
	return std::any_of(sets.begin(), sets.end(), [this, code](const std::set<u32>& indices) { return check_button_set(indices, code); });
};

bool evdev_joystick_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return check_button_set(m_dev->trigger_left, static_cast<u32>(keyCode));
}

bool evdev_joystick_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return check_button_set(m_dev->trigger_right, static_cast<u32>(keyCode));
}

bool evdev_joystick_handler::get_is_left_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return check_button_sets(m_dev->axis_left, static_cast<u32>(keyCode));
}

bool evdev_joystick_handler::get_is_right_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return check_button_sets(m_dev->axis_right, static_cast<u32>(keyCode));
}

#endif
