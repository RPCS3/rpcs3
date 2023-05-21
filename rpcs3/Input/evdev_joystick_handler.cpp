// This makes debugging on windows less painful
//#define HAVE_LIBEVDEV

#ifdef HAVE_LIBEVDEV

#include "Input/product_info.h"
#include "Emu/Io/pad_config.h"
#include "evdev_joystick_handler.h"
#include "util/logs.hpp"

#include <functional>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <cmath>

LOG_CHANNEL(evdev_log, "evdev");

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

	// Set default misc variables
	cfg->lstickdeadzone.def    = 30; // between 0 and 255
	cfg->rstickdeadzone.def    = 30; // between 0 and 255
	cfg->ltriggerthreshold.def = 0;  // between 0 and 255
	cfg->rtriggerthreshold.def = 0;  // between 0 and 255
	cfg->lpadsquircling.def    = 5000;
	cfg->rpadsquircling.def    = 5000;

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

	m_pos_axis_config.load();

	if (!m_pos_axis_config.exist())
		m_pos_axis_config.save();

	for (const auto& node : m_pos_axis_config.get_nodes())
	{
		if (*static_cast<cfg::_bool*>(node))
		{
			const auto name = node->get_name();
			const int code  = libevdev_event_code_from_name(EV_ABS, name.c_str());
			if (code < 0)
				evdev_log.error("Failed to read axis name from %s. [code = %d] [name = %s]", m_pos_axis_config.cfg_name, code, name);
			else
				m_positive_axis.emplace_back(code);
		}
	}

	m_is_init = true;
	return true;
}

std::string evdev_joystick_handler::get_device_name(const libevdev* dev)
{
	std::string name  = libevdev_get_name(dev);
	const auto unique = libevdev_get_uniq(dev);

	if (name.empty())
	{
		if (unique)
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

	const auto& path = evdev_device->path;
	libevdev*& dev   = evdev_device->device;

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

	for (auto& binding : m_bindings)
	{
		free_device(static_cast<EvdevDevice*>(binding.device.get()));
		free_device(static_cast<EvdevDevice*>(binding.buddy_device.get()));
	}

	for (auto [name, device] : m_settings_added)
	{
		free_device(static_cast<EvdevDevice*>(device.get()));
	}

	for (auto [name, device] : m_motion_settings_added)
	{
		free_device(static_cast<EvdevDevice*>(device.get()));
	}
}

std::unordered_map<u64, std::pair<u16, bool>> evdev_joystick_handler::GetButtonValues(const std::shared_ptr<EvdevDevice>& device)
{
	std::unordered_map<u64, std::pair<u16, bool>> button_values;
	if (!device)
		return button_values;

	auto& dev = device->device;

	if (!Init())
		return button_values;

	for (const auto& entry : button_list)
	{
		const auto code = entry.first;

		if (code == NO_BUTTON)
			continue;

		int val = 0;

		if (libevdev_fetch_event_value(dev, EV_KEY, code, &val) == 0)
			continue;

		button_values.emplace(code, std::make_pair<u16, bool>(static_cast<u16>(val > 0 ? 255 : 0), false));
	}

	for (const auto& entry : axis_list)
	{
		const auto code = entry.first;
		int val         = 0;

		if (libevdev_fetch_event_value(dev, EV_ABS, code, &val) == 0)
			continue;

		const int min = libevdev_get_abs_minimum(dev, code);
		const int max = libevdev_get_abs_maximum(dev, code);

		// Triggers do not need handling of negative values
		if (min >= 0 && std::find(m_positive_axis.begin(), m_positive_axis.end(), code) == m_positive_axis.end())
		{
			const float fvalue = ScaledInput(val, min, max);
			button_values.emplace(code, std::make_pair<u16, bool>(static_cast<u16>(fvalue), false));
			continue;
		}

		const float fvalue = ScaledInput2(val, min, max);
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

PadHandlerBase::connection evdev_joystick_handler::get_next_button_press(const std::string& padId, const pad_callback& callback, const pad_fail_callback& fail_callback, bool get_blacklist, const std::vector<std::string>& buttons)
{
	if (get_blacklist)
		m_blacklist.clear();

	// Get our evdev device
	auto device = get_evdev_device(padId);
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

	auto data = GetButtonValues(device);

	auto find_value = [=, this](const std::string& name)
	{
		int key = FindKeyCodeByString(rev_axis_list, name, false);
		const bool dir = key >= 0;
		if (key < 0)
			key = FindKeyCodeByString(axis_list, name, false);
		if (key < 0)
			key = FindKeyCodeByString(button_list, name);
		auto it = data.find(static_cast<u64>(key));
		return it != data.end() && dir == it->second.second ? it->second.first : 0;
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

	// return if nothing new has happened. ignore this to get the current state for blacklist
	if (!get_blacklist && !has_new_event)
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

		if (!get_blacklist && std::find(m_blacklist.begin(), m_blacklist.end(), name) != m_blacklist.end())
			continue;

		const u16 value = data[code].first;
		if (value > 0)
		{
			if (get_blacklist)
			{
				m_blacklist.emplace_back(name);
				evdev_log.error("Evdev Calibration: Added button [ %d = %s = %s ] to blacklist. Value = %d", code, libevdev_event_code_get_name(EV_KEY, code), name, value);
			}
			else if (value > pressed_button.value)
			{
				pressed_button = { value, name };
			}
		}
	}

	for (const auto& [code, name] : axis_list)
	{
		if (data[code].second)
			continue;

		if (!get_blacklist && std::find(m_blacklist.begin(), m_blacklist.end(), name) != m_blacklist.end())
			continue;

		const u16 value = data[code].first;
		if (value > 0 && value >= m_thumb_threshold)
		{
			if (get_blacklist)
			{
				const int min = libevdev_get_abs_minimum(dev, code);
				const int max = libevdev_get_abs_maximum(dev, code);
				m_blacklist.emplace_back(name);
				evdev_log.error("Evdev Calibration: Added axis [ %d = %s = %s ] to blacklist. [ Value = %d ] [ Min = %d ] [ Max = %d ]", code, libevdev_event_code_get_name(EV_ABS, code), name, value, min, max);
			}
			else if (value > pressed_button.value)
			{
				pressed_button = { value, name };
			}
		}
	}

	for (const auto& [code, name] : rev_axis_list)
	{
		if (!data[code].second)
			continue;

		if (!get_blacklist && std::find(m_blacklist.begin(), m_blacklist.end(), name) != m_blacklist.end())
			continue;

		const u16 value = data[code].first;
		if (value > 0 && value >= m_thumb_threshold)
		{
			if (get_blacklist)
			{
				const int min = libevdev_get_abs_minimum(dev, code);
				const int max = libevdev_get_abs_maximum(dev, code);
				m_blacklist.emplace_back(name);
				evdev_log.error("Evdev Calibration: Added rev axis [ %d = %s = %s ] to blacklist. [ Value = %d ] [ Min = %d ] [ Max = %d ]", code, libevdev_event_code_get_name(EV_ABS, code), name, value, min, max);
			}
			else if (value > pressed_button.value)
			{
				pressed_button = { value, name };
			}
		}
	}

	if (get_blacklist)
	{
		if (m_blacklist.empty())
			evdev_log.success("Evdev Calibration: Blacklist is clear. No input spam detected");
		return connection::connected;
	}

	if (callback)
	{
		if (pressed_button.value > 0)
			callback(pressed_button.value, pressed_button.name, padId, 0, preview_values);
		else
			callback(0, "", padId, 0, preview_values);
	}

	return connection::connected;
}

void evdev_joystick_handler::get_motion_sensors(const std::string& padId, const motion_callback& callback, const motion_fail_callback& fail_callback, motion_preview_values preview_values, const std::array<AnalogSensor, 4>& sensors)
{
	// Add device if not yet present
	auto device = add_motion_device(padId, true);
	if (!device || !update_device(device) || !device->device)
	{
		if (fail_callback)
			fail_callback(padId, std::move(preview_values));
		return;
	}

	auto& dev = device->device;

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

	if (large == device->large_motor && small == device->small_motor)
		return;

	// XBox One Controller can't handle faster vibration updates than ~10ms. Elite is even worse.
	// So I'll use 20ms to be on the safe side. No lag was noticable.
	if (clock() - device->last_vibration < 20)
		return;

	device->last_vibration = clock();

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
	auto dev = get_evdev_device(padId);
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

u32 evdev_joystick_handler::GetButtonInfo(const input_event& evt, const std::shared_ptr<EvdevDevice>& device, int& value)
{
	const u32 code         = evt.code;
	const int val          = evt.value;
	m_is_button_or_trigger = false;

	switch (evt.type)
	{
	case EV_KEY:
	{
		m_is_button_or_trigger = true;

		// get the button value and return its code
		if (button_list.find(code) == button_list.end())
		{
			evdev_log.error("Evdev button %s (%d) is unknown. Please add it to the button list.", libevdev_event_code_get_name(EV_KEY, code), code);
			return -1;
		}

		value = val > 0 ? 255 : 0;
		return code;
	}
	case EV_ABS:
	{
		if (!device || !device->device)
		{
			return -1;
		}
		auto& dev     = device->device;
		const int min = libevdev_get_abs_minimum(dev, code);
		const int max = libevdev_get_abs_maximum(dev, code);

		// Triggers do not need handling of negative values
		if (min >= 0 && std::find(m_positive_axis.begin(), m_positive_axis.end(), code) == m_positive_axis.end())
		{
			m_is_negative          = false;
			m_is_button_or_trigger = true;
			value                  = static_cast<u16>(ScaledInput(val, min, max));
			return code;
		}

		const float fvalue = ScaledInput2(val, min, max);
		m_is_negative      = fvalue < 0;
		value              = static_cast<u16>(std::abs(fvalue));
		return code;
	}
	default: return -1;
	}
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
						if (const input_absinfo *info = libevdev_get_abs_info(dev, code))
						{
							const auto code_name = libevdev_event_code_get_name(EV_ABS, code);
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
					if (const input_absinfo *info = libevdev_get_abs_info(dev, code))
					{
						const bool is_accel = code == ABS_X || code == ABS_Y || code == ABS_Z;
						const auto code_name = libevdev_event_code_get_name(EV_ABS, code);
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

	auto& dev = m_dev->device;
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

		if (ret == LIBEVDEV_READ_STATUS_SUCCESS)
		{
			handle_input_event(evt, pad);
		}
	}

	if (ret < 0)
	{
		// -EAGAIN signifies no available events, not an actual *error*.
		if (ret != -EAGAIN)
			evdev_log.error("Failed to read latest event from joystick: %s [errno %d]", strerror(-ret), -ret);
	}
}

void evdev_joystick_handler::get_extended_info(const pad_ensemble& binding)
{
	// We use this to get motion controls from our buddy device
	const auto& device = std::static_pointer_cast<EvdevDevice>(binding.buddy_device);
	const auto& pad = binding.pad;

	if (!pad || !device || !update_device(device))
		return;

	auto& dev = device->device;
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

		s16 value = ScaledInput(evt.value, min, max, 1023.0f);

		if (sensor.m_mirrored)
		{
			value = 1023 - value;
		}

		return Clamp0To1023(value + sensor.m_shift);
	}

	return 0;
}

void evdev_joystick_handler::handle_input_event(const input_event& evt, const std::shared_ptr<Pad>& pad)
{
	if (!pad)
		return;

	m_dev->cur_type = evt.type;

	int value;
	const u32 button_code = GetButtonInfo(evt, m_dev, value);
	if (button_code == NO_BUTTON || value < 0)
		return;

	auto axis_orientations = m_dev->axis_orientations;

	// Find out if special buttons are pressed (introduced by RPCS3).
	// These buttons will have a delay of one cycle, but whatever.
	const bool adjust_pressure = pad->m_pressure_intensity_button_index >= 0 && pad->m_buttons[pad->m_pressure_intensity_button_index].m_pressed;

	// Translate any corresponding keycodes to our normal DS3 buttons and triggers
	for (int i = 0; i < static_cast<int>(pad->m_buttons.size()); i++)
	{
		auto& button = pad->m_buttons[i];

		if (button.m_keyCode != button_code)
			continue;

		// Be careful to handle mapped axis specially
		if (evt.type == EV_ABS)
		{
			// get axis direction and skip on error or set to 0 if the stick/hat is actually pointing to the other direction.
			// maybe mimic on error, needs investigation. FindAxisDirection should ideally never return -1 anyway
			const int direction = FindAxisDirection(axis_orientations, i);
			m_dev->cur_dir      = direction;

			if (direction < 0)
			{
				evdev_log.error("FindAxisDirection = %d, Button Nr.%d, value = %d", direction, i, value);
				continue;
			}

			if (direction != (m_is_negative ? 1 : 0))
			{
				button.m_value   = 0;
				button.m_pressed = 0;
				continue;
			}
		}

		// Using a temporary buffer because the values can change during translation
		Button tmp = button;
		tmp.m_value = static_cast<u16>(value);

		TranslateButtonPress(m_dev, button_code, tmp.m_pressed, tmp.m_value);

		// Modify pressure if necessary if the button was pressed
		if (adjust_pressure && tmp.m_pressed)
		{
			tmp.m_value = pad->m_pressure_intensity;
		}

		button = tmp;
	}

	// Translate any corresponding keycodes to our two sticks. (ignoring thresholds for now)
	for (int idx = 0; idx < static_cast<int>(pad->m_sticks.size()); idx++)
	{
		bool pressed_min = false;
		bool pressed_max = false;

		// m_keyCodeMin is the mapped key for left or down
		if (pad->m_sticks[idx].m_keyCodeMin == button_code)
		{
			bool is_direction_min = false;

			if (!m_is_button_or_trigger && evt.type == EV_ABS)
			{
				const int index         = pad->m_buttons.size() + (idx * 2) + 1;
				const int min_direction = FindAxisDirection(axis_orientations, index);
				m_dev->cur_dir          = min_direction;

				if (min_direction < 0)
					evdev_log.error("keyCodeMin FindAxisDirection = %d, Axis Nr.%d, Button Nr.%d, value = %d", min_direction, idx, index, value);
				else
					is_direction_min = m_is_negative == (min_direction == 1);
			}

			if (m_is_button_or_trigger || is_direction_min)
			{
				m_dev->val_min[idx] = value;
				TranslateButtonPress(m_dev, button_code, pressed_min, m_dev->val_min[idx], true);
			}
			else // set to 0 to avoid remnant counter axis values
			{
				m_dev->val_min[idx] = 0;
			}
		}

		// m_keyCodeMax is the mapped key for right or up
		if (pad->m_sticks[idx].m_keyCodeMax == button_code)
		{
			bool is_direction_max = false;

			if (!m_is_button_or_trigger && evt.type == EV_ABS)
			{
				const int index         = pad->m_buttons.size() + (idx * 2);
				const int max_direction = FindAxisDirection(axis_orientations, index);
				m_dev->cur_dir          = max_direction;

				if (max_direction < 0)
					evdev_log.error("keyCodeMax FindAxisDirection = %d, Axis Nr.%d, Button Nr.%d, value = %d", max_direction, idx, index, value);
				else
					is_direction_max = m_is_negative == (max_direction == 1);
			}

			if (m_is_button_or_trigger || is_direction_max)
			{
				m_dev->val_max[idx] = value;
				TranslateButtonPress(m_dev, button_code, pressed_max, m_dev->val_max[idx], true);
			}
			else // set to 0 to avoid remnant counter axis values
			{
				m_dev->val_max[idx] = 0;
			}
		}

		// cancel out opposing values and get the resulting difference. if there was no change, use the old value.
		m_dev->stick_val[idx] = m_dev->val_max[idx] - m_dev->val_min[idx];
	}

	const auto cfg = m_dev->config;
	if (!cfg)
		return;

	u16 lx, ly, rx, ry;

	// Normalize and apply pad squircling
	convert_stick_values(lx, ly, m_dev->stick_val[0], m_dev->stick_val[1], cfg->lstickdeadzone, cfg->lpadsquircling);
	convert_stick_values(rx, ry, m_dev->stick_val[2], m_dev->stick_val[3], cfg->rstickdeadzone, cfg->rpadsquircling);

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

	auto cfg = device->config;
	if (!cfg)
		return;

	// Handle vibration
	const int idx_l       = cfg->switch_vibration_motors ? 1 : 0;
	const int idx_s       = cfg->switch_vibration_motors ? 0 : 1;
	const u8 force_large = cfg->enable_vibration_motor_large ? pad->m_vibrateMotors[idx_l].m_value * 257 : 0;
	const u8 force_small = cfg->enable_vibration_motor_small ? pad->m_vibrateMotors[idx_s].m_value * 257 : 0;
	SetRumble(evdev_device, force_large, force_small);
}

// Search axis_orientations map for the direction by index, returns -1 if not found, 0 for positive and 1 for negative
int evdev_joystick_handler::FindAxisDirection(const std::unordered_map<int, bool>& map, int index)
{
	if (const auto it = map.find(index); it != map.end())
	{
		return it->second;
	}
	return -1;
}

bool evdev_joystick_handler::bindPadToDevice(std::shared_ptr<Pad> pad, u8 player_id)
{
	if (!pad || player_id >= g_cfg_input.player.size())
		return false;

	const cfg_player* player_config = g_cfg_input.player[player_id];
	if (!pad)
		return false;

	Init();

	m_dev = std::make_shared<EvdevDevice>();

	m_pad_configs[player_id].from_string(player_config->config.to_string());
	m_dev->config = &m_pad_configs[player_id];
	m_dev->player_id = player_id;
	cfg_pad* cfg = m_dev->config;
	if (!cfg)
		return false;

	std::unordered_map<int, bool> axis_orientations;
	int i = 0; // increment to know the axis location

	auto evdevbutton = [&](const cfg::string& name)
	{
		EvdevButton button{ 0, -1, EV_ABS };

		int key  = FindKeyCode(axis_list, name, false);
		if (key >= 0)
			axis_orientations.emplace(i, false);

		if (key < 0)
		{
			key = FindKeyCode(rev_axis_list, name, false);
			if (key >= 0)
				axis_orientations.emplace(i, true);
		}

		if (key < 0)
		{
			key         = FindKeyCode(button_list, name);
			button.type = EV_KEY;
		}
		else
		{
			button.dir = axis_orientations[i];
		}

		button.code = static_cast<u32>(key);

		i++;
		return button;
	};

	const auto find_motion_button = [&](const cfg_sensor& sensor) -> evdev_sensor
	{
		evdev_sensor e_sensor{};
		e_sensor.type = EV_ABS;
		e_sensor.mirrored = sensor.mirrored.get();
		e_sensor.shift = sensor.shift.get();
		const int key = FindKeyCode(motion_axis_list, sensor.axis, false);
		if (key >= 0) e_sensor.code = static_cast<u32>(key);
		return e_sensor;
	};

	u32 pclass_profile = 0x0;

	for (const auto product : input::get_products_by_class(cfg->device_class_type))
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

	pad->m_buttons.emplace_back(special_button_offset, evdevbutton(cfg->pressure_intensity_button).code, special_button_value::pressure_intensity);
	pad->m_pressure_intensity_button_index = static_cast<s32>(pad->m_buttons.size()) - 1;

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, evdevbutton(cfg->triangle).code, CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, evdevbutton(cfg->circle).code,   CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, evdevbutton(cfg->cross).code,    CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, evdevbutton(cfg->square).code,   CELL_PAD_CTRL_SQUARE);

	m_dev->trigger_left = evdevbutton(cfg->l2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_dev->trigger_left.code,              CELL_PAD_CTRL_L2);

	m_dev->trigger_right = evdevbutton(cfg->r2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_dev->trigger_right.code,             CELL_PAD_CTRL_R2);

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, evdevbutton(cfg->l1).code,       CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, evdevbutton(cfg->r1).code,       CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, evdevbutton(cfg->start).code,    CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, evdevbutton(cfg->select).code,   CELL_PAD_CTRL_SELECT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, evdevbutton(cfg->l3).code,       CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, evdevbutton(cfg->r3).code,       CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, evdevbutton(cfg->ps).code,       CELL_PAD_CTRL_PS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, evdevbutton(cfg->up).code,       CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, evdevbutton(cfg->down).code,     CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, evdevbutton(cfg->left).code,     CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, evdevbutton(cfg->right).code,    CELL_PAD_CTRL_RIGHT);

	m_dev->axis_left[0]  = evdevbutton(cfg->ls_right);
	m_dev->axis_left[1]  = evdevbutton(cfg->ls_left);
	m_dev->axis_left[2]  = evdevbutton(cfg->ls_up);
	m_dev->axis_left[3]  = evdevbutton(cfg->ls_down);
	m_dev->axis_right[0] = evdevbutton(cfg->rs_right);
	m_dev->axis_right[1] = evdevbutton(cfg->rs_left);
	m_dev->axis_right[2] = evdevbutton(cfg->rs_up);
	m_dev->axis_right[3] = evdevbutton(cfg->rs_down);

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X,  m_dev->axis_left[1].code,  m_dev->axis_left[0].code);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y,  m_dev->axis_left[3].code,  m_dev->axis_left[2].code);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, m_dev->axis_right[1].code, m_dev->axis_right[0].code);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, m_dev->axis_right[3].code, m_dev->axis_right[2].code);

	m_dev->axis_motion[0] = find_motion_button(cfg->motion_sensor_x);
	m_dev->axis_motion[1] = find_motion_button(cfg->motion_sensor_y);
	m_dev->axis_motion[2] = find_motion_button(cfg->motion_sensor_z);
	m_dev->axis_motion[3] = find_motion_button(cfg->motion_sensor_g);

	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_X, m_dev->axis_motion[0].code, m_dev->axis_motion[0].mirrored, m_dev->axis_motion[0].shift, DEFAULT_MOTION_X);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Y, m_dev->axis_motion[1].code, m_dev->axis_motion[1].mirrored, m_dev->axis_motion[1].shift, DEFAULT_MOTION_Y);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Z, m_dev->axis_motion[2].code, m_dev->axis_motion[2].mirrored, m_dev->axis_motion[2].shift, DEFAULT_MOTION_Z);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_G, m_dev->axis_motion[3].code, m_dev->axis_motion[3].mirrored, m_dev->axis_motion[3].shift, DEFAULT_MOTION_G);

	pad->m_vibrateMotors.emplace_back(true, 0);
	pad->m_vibrateMotors.emplace_back(false, 0);

	m_dev->axis_orientations = axis_orientations;

	if (auto evdev_device = add_device(player_config->device, false))
	{
		if (auto motion_device = add_motion_device(player_config->buddy_device, false))
		{
			m_bindings.emplace_back(pad, evdev_device, motion_device);
		}
		else
		{
			m_bindings.emplace_back(pad, evdev_device, nullptr);
			evdev_log.warning("evdev add_motion_device in bindPadToDevice failed for device %s", player_config->buddy_device.to_string());
		}
	}
	else
	{
		evdev_log.warning("evdev add_device in bindPadToDevice failed for device %s", player_config->device.to_string());
	}

	for (auto& binding : m_bindings)
	{
		update_device(binding.device);
		update_device(binding.buddy_device);
	}

	return true;
}

bool evdev_joystick_handler::check_button(const EvdevButton& b, const u32 code)
{
	return m_dev && b.code == code && b.type == m_dev->cur_type && b.dir == m_dev->cur_dir;
}

bool evdev_joystick_handler::check_buttons(const std::array<EvdevButton, 4>& b, const u32 code)
{
	return std::any_of(b.begin(), b.end(), [this, code](const EvdevButton& b) { return check_button(b, code); });
};

bool evdev_joystick_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return check_button(m_dev->trigger_left, static_cast<u32>(keyCode));
}

bool evdev_joystick_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return check_button(m_dev->trigger_right, static_cast<u32>(keyCode));
}

bool evdev_joystick_handler::get_is_left_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return check_buttons(m_dev->axis_left, static_cast<u32>(keyCode));
}

bool evdev_joystick_handler::get_is_right_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return check_buttons(m_dev->axis_right, static_cast<u32>(keyCode));
}

#endif
