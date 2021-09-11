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
	vibration_min = 0;
	vibration_max = 65535;

	// set capabilities
	b_has_config    = true;
	b_has_rumble    = true;
	b_has_deadzones = true;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold   = thumb_max / 2;

	m_dev = std::make_shared<EvdevDevice>();
}

evdev_joystick_handler::~evdev_joystick_handler()
{
	Close();
}

void evdev_joystick_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
	cfg->ls_left.def  = rev_axis_list.at(ABS_X);
	cfg->ls_down.def  = axis_list.at(ABS_Y);
	cfg->ls_right.def = axis_list.at(ABS_X);
	cfg->ls_up.def    = rev_axis_list.at(ABS_Y);
	cfg->rs_left.def  = rev_axis_list.at(ABS_RX);
	cfg->rs_down.def  = axis_list.at(ABS_RY);
	cfg->rs_right.def = axis_list.at(ABS_RX);
	cfg->rs_up.def    = rev_axis_list.at(ABS_RY);
	cfg->start.def    = button_list.at(BTN_START);
	cfg->select.def   = button_list.at(BTN_SELECT);
	cfg->ps.def       = button_list.at(BTN_MODE);
	cfg->square.def   = button_list.at(BTN_X);
	cfg->cross.def    = button_list.at(BTN_A);
	cfg->circle.def   = button_list.at(BTN_B);
	cfg->triangle.def = button_list.at(BTN_Y);
	cfg->left.def     = rev_axis_list.at(ABS_HAT0X);
	cfg->down.def     = axis_list.at(ABS_HAT0Y);
	cfg->right.def    = axis_list.at(ABS_HAT0X);
	cfg->up.def       = rev_axis_list.at(ABS_HAT0Y);
	cfg->r1.def       = button_list.at(BTN_TR);
	cfg->r2.def       = axis_list.at(ABS_RZ);
	cfg->r3.def       = button_list.at(BTN_THUMBR);
	cfg->l1.def       = button_list.at(BTN_TL);
	cfg->l2.def       = axis_list.at(ABS_Z);
	cfg->l3.def       = button_list.at(BTN_THUMBL);

	cfg->pressure_intensity_button.def = button_list.at(NO_BUTTON);

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

	if (name == "" && unique != nullptr)
		name = unique;

	if (name == "")
		name = "Unknown Device";

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

void evdev_joystick_handler::update_devs()
{
	for (auto& binding : bindings)
	{
		update_device(binding.first);
	}
}

void evdev_joystick_handler::Close()
{
	for (auto& binding : bindings)
	{
		EvdevDevice* evdev_device = static_cast<EvdevDevice*>(binding.first.get());
		if (evdev_device)
		{
			auto& dev = evdev_device->device;
			if (dev != nullptr)
			{
				const int fd = libevdev_get_fd(dev);
				if (evdev_device->effect_id != -1)
					ioctl(fd, EVIOCRMFF, evdev_device->effect_id);
				libevdev_free(dev);
				close(fd);
			}
		}
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
	const int pad_index = add_device(device, nullptr, true);
	if (pad_index < 0)
		return nullptr;

	auto dev = bindings[pad_index];

	// Check if our device is connected
	if (!update_device(dev.first))
		return nullptr;

	return std::static_pointer_cast<EvdevDevice>(dev.first);
}

void evdev_joystick_handler::get_next_button_press(const std::string& padId, const pad_callback& callback, const pad_fail_callback& fail_callback, bool get_blacklist, const std::vector<std::string>& buttons)
{
	if (get_blacklist)
		m_blacklist.clear();

	// Get our evdev device
	auto device = get_evdev_device(padId);
	if (!device || device->device == nullptr)
	{
		if (fail_callback)
			fail_callback(padId);
		return;
	}
	libevdev* dev = device->device;

	// Try to query the latest event from the joystick.
	input_event evt;
	int ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &evt);

	// Grab any pending sync event.
	if (ret == LIBEVDEV_READ_STATUS_SYNC)
		ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_SYNC, &evt);

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

	pad_preview_values preview_values = { 0, 0, 0, 0, 0, 0 };

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
	if (!get_blacklist && ret < 0)
	{
		if (callback)
			callback(0, "", padId, 0, preview_values);
		return;
	}

	std::pair<u16, std::string> pressed_button = { 0, "" };

	for (const auto& [code, name] : button_list)
	{
		// Handle annoying useless buttons
		if (code == NO_BUTTON)
			continue;
		if (padId.find("Xbox 360") != umax && code >= BTN_TRIGGER_HAPPY)
			continue;
		if (padId.find("Sony") != umax && (code == BTN_TL2 || code == BTN_TR2))
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
			else if (value > pressed_button.first)
				pressed_button = { value, name };
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
			else if (value > pressed_button.first)
				pressed_button = { value, name };
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
			else if (value > pressed_button.first)
				pressed_button = { value, name };
		}
	}

	if (get_blacklist)
	{
		if (m_blacklist.empty())
			evdev_log.success("Evdev Calibration: Blacklist is clear. No input spam detected");
		return;
	}

	if (callback)
	{
		if (pressed_button.first > 0)
			return callback(pressed_button.first, pressed_button.second, padId, 0, preview_values);
		else
			return callback(0, "", padId, 0, preview_values);
	}
}

// https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/InputCommon/ControllerInterface/evdev/evdev.cpp
// https://github.com/reicast/reicast-emulator/blob/master/core/linux-dist/evdev.cpp
// http://www.infradead.org/~mchehab/kernel_docs_pdf/linux-input.pdf
void evdev_joystick_handler::SetRumble(EvdevDevice* device, u16 large, u16 small)
{
	if (!device || !device->has_rumble || device->effect_id == -2)
		return;

	const int fd = libevdev_get_fd(device->device);
	if (fd < 0)
		return;

	if (large == device->force_large && small == device->force_small)
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
		device->force_large = large;
		device->force_small = small;
		return;
	}

	ff_effect effect;

	if (libevdev_has_event_code(device->device, EV_FF, FF_RUMBLE))
	{
		effect.type                      = FF_RUMBLE;
		effect.id                        = device->effect_id;
		effect.direction                 = 0;
		effect.u.rumble.strong_magnitude = large;
		effect.u.rumble.weak_magnitude   = small;
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

	device->force_large = large;
	device->force_small = small;
}

void evdev_joystick_handler::SetPadData(const std::string& padId, u8 /*player_id*/, u32 largeMotor, u32 smallMotor, s32 /* r*/, s32 /* g*/, s32 /* b*/, bool /*battery_led*/, u32 /*battery_led_brightness*/)
{
	// Get our evdev device
	auto dev = get_evdev_device(padId);
	if (!dev)
	{
		evdev_log.error("evdev TestVibration: Device [%s] not found! [largeMotor = %d] [smallMotor = %d]", padId, largeMotor, smallMotor);
		return;
	}

	if (!dev->has_rumble)
	{
		evdev_log.error("evdev TestVibration: Device [%s] does not support rumble features! [largeMotor = %d] [smallMotor = %d]", padId, largeMotor, smallMotor);
		return;
	}

	SetRumble(static_cast<EvdevDevice*>(dev.get()), largeMotor, smallMotor);
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

std::vector<std::string> evdev_joystick_handler::ListDevices()
{
	Init();

	std::unordered_map<std::string, u32> unique_names;
	std::vector<std::string> evdev_joystick_list;
	fs::dir devdir{"/dev/input/"};
	fs::dir_entry et;

	while (devdir.read(et))
	{
		// Check if the entry starts with event (a 5-letter word)
		if (et.name.size() > 5 && et.name.compare(0, 5, "event") == 0)
		{
			const int fd         = open(("/dev/input/" + et.name).c_str(), O_RDWR | O_NONBLOCK);
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
			if (libevdev_has_event_type(dev, EV_KEY) &&
				libevdev_has_event_type(dev, EV_ABS))
			{
				// It's a joystick.
				std::string name = get_device_name(dev);

				if (unique_names.find(name) == unique_names.end())
					unique_names.emplace(name, 1);
				else
					name = fmt::format("%d. %s", ++unique_names[name], name);

				evdev_joystick_list.push_back(name);
			}
			libevdev_free(dev);
			close(fd);
		}
	}
	return evdev_joystick_list;
}

int evdev_joystick_handler::add_device(const std::string& device, const std::shared_ptr<Pad>& pad, bool in_settings)
{
	if (in_settings && m_settings_added.count(device))
		return m_settings_added.at(device);

	// Now we need to find the device with the same name, and make sure not to grab any duplicates.
	std::unordered_map<std::string, u32> unique_names;
	fs::dir devdir{ "/dev/input/" };
	fs::dir_entry et;
	while (devdir.read(et))
	{
		// Check if the entry starts with event (a 5-letter word)
		if (et.name.size() > 5 && et.name.compare(0, 5, "event") == 0)
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

			if (libevdev_has_event_type(dev, EV_KEY) &&
				libevdev_has_event_type(dev, EV_ABS) &&
				name == device)
			{
				// It's a joystick. Now let's make sure we don't already have this one.
				if (std::any_of(bindings.begin(), bindings.end(), [&path](std::pair<std::shared_ptr<PadDevice>, std::shared_ptr<Pad>> binding)
				{
					EvdevDevice* device = static_cast<EvdevDevice*>(binding.first.get());
					return device && path == device->path;
				}))
				{
					libevdev_free(dev);
					close(fd);
					continue;
				}

				if (in_settings)
				{
					m_dev                  = std::make_shared<EvdevDevice>();
					m_settings_added[device] = bindings.size();

					// Let's log axis information while we are in the settings in order to identify problems more easily.
					for (const auto& [code, axis_name] : axis_list)
					{
						if (const input_absinfo *info = libevdev_get_abs_info(dev, code))
						{
							const auto code_name = libevdev_event_code_get_name(EV_ABS, code);
							evdev_log.notice("Axis info for %s: %s (%s) => minimum=%d, maximum=%d, fuzz=%d, resolution=%d",
								name, code_name, axis_name, info->minimum, info->maximum, info->fuzz, info->resolution);
						}
					}
				}

				// Alright, now that we've confirmed we haven't added this joystick yet, les do dis.
				m_dev->device     = dev;
				m_dev->path       = path;
				m_dev->has_rumble = libevdev_has_event_type(dev, EV_FF);
				bindings.emplace_back(m_dev, pad);
				return bindings.size() - 1;
			}
			libevdev_free(dev);
			close(fd);
		}
	}
	return -1;
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

void evdev_joystick_handler::get_mapping(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	m_dev = std::static_pointer_cast<EvdevDevice>(device);
	if (!m_dev || !pad)
		return;

	auto& dev = m_dev->device;
	if (dev == nullptr)
		return;

	// Try to query the latest event from the joystick.
	input_event evt;
	int ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &evt);

	// Grab any pending sync event.
	if (ret == LIBEVDEV_READ_STATUS_SYNC)
	{
		evdev_log.notice("Captured sync event");
		ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_SYNC, &evt);
	}

	if (ret < 0)
	{
		// -EAGAIN signifies no available events, not an actual *error*.
		if (ret != -EAGAIN)
			evdev_log.error("Failed to read latest event from joystick: %s [errno %d]", strerror(-ret), -ret);
		return;
	}

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
			else if (direction != (m_is_negative ? 1 : 0))
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
				m_dev->val_min[idx] = 0;
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
				m_dev->val_max[idx] = 0;
		}

		// cancel out opposing values and get the resulting difference. if there was no change, use the old value.
		m_dev->stick_val[idx] = m_dev->val_max[idx] - m_dev->val_min[idx];
	}

	const auto cfg = m_dev->config;

	u16 lx, ly, rx, ry;

	// Normalize and apply pad squircling
	convert_stick_values(lx, ly, m_dev->stick_val[0], m_dev->stick_val[1], cfg->lstickdeadzone, cfg->lpadsquircling);
	convert_stick_values(rx, ry, m_dev->stick_val[2], m_dev->stick_val[3], cfg->rstickdeadzone, cfg->rpadsquircling);

	pad->m_sticks[0].m_value = lx;
	pad->m_sticks[1].m_value = 255 - ly;
	pad->m_sticks[2].m_value = rx;
	pad->m_sticks[3].m_value = 255 - ry;

	return;
}

void evdev_joystick_handler::apply_pad_data(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	EvdevDevice* evdev_device = static_cast<EvdevDevice*>(device.get());
	if (!evdev_device)
		return;

	auto cfg = device->config;

	// Handle vibration
	const int idx_l       = cfg->switch_vibration_motors ? 1 : 0;
	const int idx_s       = cfg->switch_vibration_motors ? 0 : 1;
	const u16 force_large = cfg->enable_vibration_motor_large ? pad->m_vibrateMotors[idx_l].m_value * 257 : vibration_min;
	const u16 force_small = cfg->enable_vibration_motor_small ? pad->m_vibrateMotors[idx_s].m_value * 257 : vibration_min;
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

bool evdev_joystick_handler::bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device, u8 player_id)
{
	if (!pad)
		return false;

	Init();

	m_dev = std::make_shared<EvdevDevice>();

	m_pad_configs[player_id].from_string(g_cfg_input.player[player_id]->config.to_string());
	m_dev->config = &m_pad_configs[player_id];
	m_dev->player_id = player_id;
	cfg_pad* cfg = m_dev->config;
	if (cfg == nullptr)
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
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, evdevbutton(cfg->ps).code,       0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, evdevbutton(cfg->up).code,       CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, evdevbutton(cfg->down).code,     CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, evdevbutton(cfg->left).code,     CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, evdevbutton(cfg->right).code,    CELL_PAD_CTRL_RIGHT);
	//pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0,                                     0x0); // Reserved (and currently not in use by rpcs3 at all)

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

	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_X, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Y, 399);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Z, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_G, 512);

	pad->m_vibrateMotors.emplace_back(true, 0);
	pad->m_vibrateMotors.emplace_back(false, 0);

	m_dev->axis_orientations = axis_orientations;

	if (add_device(device, pad, false) < 0)
		evdev_log.warning("evdev add_device in bindPadToDevice failed for device %s", device);

	update_devs();
	return true;
}

bool evdev_joystick_handler::check_button(const EvdevButton& b, const u32 code)
{
	return m_dev && b.code == code && b.type == m_dev->cur_type && b.dir == m_dev->cur_dir;
}

bool evdev_joystick_handler::check_buttons(const std::vector<EvdevButton>& b, const u32 code)
{
	return std::any_of(b.begin(), b.end(), [this, code](const EvdevButton& b) { return check_button(b, code); });
};

bool evdev_joystick_handler::get_is_left_trigger(u64 keyCode)
{
	return keyCode == check_button(m_dev->trigger_left, static_cast<u32>(keyCode));
}

bool evdev_joystick_handler::get_is_right_trigger(u64 keyCode)
{
	return keyCode == check_button(m_dev->trigger_right, static_cast<u32>(keyCode));
}

bool evdev_joystick_handler::get_is_left_stick(u64 keyCode)
{
	return keyCode == check_buttons(m_dev->axis_left, static_cast<u32>(keyCode));
}

bool evdev_joystick_handler::get_is_right_stick(u64 keyCode)
{
	return keyCode == check_buttons(m_dev->axis_right, static_cast<u32>(keyCode));
}

#endif
