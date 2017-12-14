// This makes debugging on windows less painful
//#define HAVE_LIBEVDEV

#ifdef HAVE_LIBEVDEV

#include "rpcs3qt/pad_settings_dialog.h"
#include "evdev_joystick_handler.h"
#include "Utilities/Thread.h"
#include "Utilities/Log.h"

#include <functional>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <cmath>

evdev_joystick_handler::evdev_joystick_handler()
{
	// Define border values
	thumb_min = 0;
	thumb_max = 255;
	trigger_min = 0;
	trigger_max = 255;
	vibration_min = 0;
	vibration_max = 65535;

	// Set this handler's type and save location
	m_pad_config.cfg_type = "evdev";
	m_pad_config.cfg_name = fs::get_config_dir() + "/config_evdev.yml";

	// Set default button mapping
	m_pad_config.ls_left.def  = rev_axis_list.at(ABS_X);
	m_pad_config.ls_down.def  = axis_list.at(ABS_Y);
	m_pad_config.ls_right.def = axis_list.at(ABS_X);
	m_pad_config.ls_up.def    = rev_axis_list.at(ABS_Y);
	m_pad_config.rs_left.def  = rev_axis_list.at(ABS_RX);
	m_pad_config.rs_down.def  = axis_list.at(ABS_RY);
	m_pad_config.rs_right.def = axis_list.at(ABS_RX);
	m_pad_config.rs_up.def    = rev_axis_list.at(ABS_RY);
	m_pad_config.start.def    = button_list.at(BTN_START);
	m_pad_config.select.def   = button_list.at(BTN_SELECT);
	m_pad_config.ps.def       = button_list.at(BTN_MODE);
	m_pad_config.square.def   = button_list.at(BTN_X);
	m_pad_config.cross.def    = button_list.at(BTN_A);
	m_pad_config.circle.def   = button_list.at(BTN_B);
	m_pad_config.triangle.def = button_list.at(BTN_Y);
	m_pad_config.left.def     = rev_axis_list.at(ABS_HAT0X);
	m_pad_config.down.def     = axis_list.at(ABS_HAT0Y);
	m_pad_config.right.def    = axis_list.at(ABS_HAT0X);
	m_pad_config.up.def       = rev_axis_list.at(ABS_HAT0Y);
	m_pad_config.r1.def       = button_list.at(BTN_TR);
	m_pad_config.r2.def       = axis_list.at(ABS_RZ);
	m_pad_config.r3.def       = button_list.at(BTN_THUMBR);
	m_pad_config.l1.def       = button_list.at(BTN_TL);
	m_pad_config.l2.def       = axis_list.at(ABS_Z);
	m_pad_config.l3.def       = button_list.at(BTN_THUMBL);

	// Set default misc variables
	m_pad_config.lstickdeadzone.def = 30;   // between 0 and 255
	m_pad_config.rstickdeadzone.def = 30;   // between 0 and 255
	m_pad_config.ltriggerthreshold.def = 0; // between 0 and 255
	m_pad_config.rtriggerthreshold.def = 0; // between 0 and 255
	m_pad_config.padsquircling.def = 5000;

	// apply defaults
	m_pad_config.from_default();

	// set capabilities
	b_has_config = true;
	b_has_rumble = false;
	b_has_deadzones = true;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
}

evdev_joystick_handler::~evdev_joystick_handler()
{
	Close();
}

bool evdev_joystick_handler::Init()
{
	m_pad_config.load();
	return true;
}

bool evdev_joystick_handler::update_device(EvdevDevice& device, bool use_cell)
{
	std::shared_ptr<Pad> pad = device.pad;
	const auto& path = device.path;
	libevdev*& dev = device.device;

	bool was_connected = dev != nullptr;

	if (access(path.c_str(), R_OK) == -1)
	{
		if (was_connected)
		{
			// It was disconnected.
			if (use_cell) pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;

			int fd = libevdev_get_fd(dev);
			libevdev_free(dev);
			close(fd);
			dev = nullptr;
		}

		if (use_cell) pad->m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
		LOG_ERROR(GENERAL, "Joystick %s is not present or accessible [previous status: %d]", path.c_str(),
			was_connected ? 1 : 0);
		return false;
	}

	if (was_connected) return true;  // It's already been connected, and the js is still present.
	int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);

	if (fd == -1)
	{
		int err = errno;
		LOG_ERROR(GENERAL, "Failed to open joystick: %s [errno %d]", strerror(err), err);
		return false;
	}

	int ret = libevdev_new_from_fd(fd, &dev);
	if (ret < 0)
	{
		LOG_ERROR(GENERAL, "Failed to initialize libevdev for joystick: %s [errno %d]", strerror(-ret), -ret);
		return false;
	}

	LOG_NOTICE(GENERAL, "Opened joystick: '%s' at %s (fd %d)", libevdev_get_name(dev), path, fd);

	if (use_cell)
	{
		// Connection status changed from disconnected to connected.
		pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
		pad->m_port_status |= CELL_PAD_STATUS_CONNECTED;
	}

	return true;
}

void evdev_joystick_handler::update_devs(bool use_cell)
{
	for (auto& device : devices)
	{
		update_device(device, use_cell);
	}
}

void evdev_joystick_handler::Close()
{
	for (auto& device : devices)
	{
		auto& dev = device.device;
		if (dev != nullptr)
		{
			int fd = libevdev_get_fd(dev);
			libevdev_free(dev);
			close(fd);
		}
	}
}

std::unordered_map<u64, std::pair<u16, bool>> evdev_joystick_handler::GetButtonValues(libevdev* dev)
{
	std::unordered_map<u64, std::pair<u16, bool>> button_values;

	for (auto entry : button_list)
	{
		auto code = entry.first;
		int val = 0;
		if (libevdev_fetch_event_value(dev, EV_KEY, code, &val) == 0)
			continue;

		button_values.emplace(code, std::make_pair<u16, bool>(static_cast<u16>(val > 0 ? 255 : 0), false));
	}

	for (auto entry : axis_list)
	{
		auto code = entry.first;
		int val = 0;
		if (libevdev_fetch_event_value(dev, EV_ABS, code, &val) == 0)
			continue;

		// Triggers should be ABS_Z and ABS_RZ and do not need handling of negative values
		if (code == ABS_Z || code == ABS_RZ)
		{
			float fvalue = ScaleStickInput(val, libevdev_get_abs_minimum(dev, code), libevdev_get_abs_maximum(dev, code));
			button_values.emplace(code, std::make_pair<u16, bool>(static_cast<u16>(fvalue), false));
			continue;
		}

		float fvalue = ScaleStickInput2(val, libevdev_get_abs_minimum(dev, code), libevdev_get_abs_maximum(dev, code));

		if (fvalue < 0)
			button_values.emplace(code, std::make_pair<u16, bool>(static_cast<u16>(std::abs(fvalue)), true));
		else
			button_values.emplace(code, std::make_pair<u16, bool>(static_cast<u16>(fvalue), false));
	}

	return button_values;
}

void evdev_joystick_handler::GetNextButtonPress(const std::string& padId, const std::function<void(u16, std::string, int[])>& callback, bool get_blacklist)
{
	if (get_blacklist)
		blacklist.clear();

	// Add device if not yet present
	m_pad_index = add_device(padId, true);

	if (m_pad_index < 0)
		return;

	EvdevDevice& device = devices[m_pad_index];

	// Check if our device is connected
	if (!update_device(device, false))
		return;

	auto& dev = device.device;

	// Try to query the latest event from the joystick.
	input_event evt;
	int ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &evt);

	// Grab any pending sync event.
	if (ret == LIBEVDEV_READ_STATUS_SYNC)
		ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_SYNC, &evt);

	// return if nothing new has happened. ignore this to get the current state for blacklist
	if (!get_blacklist && ret < 0)
		return;

	auto data = GetButtonValues(dev);

	std::pair<u16, std::string> pressed_button = { 0, "" };
	for (const auto& button : button_list)
	{
		int code = button.first;
		std::string name = button.second;

		if (padId.find("Xbox 360") != std::string::npos && code >= BTN_TRIGGER_HAPPY)
			continue;
		if (padId.find("Sony") != std::string::npos && (code == BTN_TL2 || code == BTN_TR2))
			continue;

		if (!get_blacklist && std::find(blacklist.begin(), blacklist.end(), name) != blacklist.end())
			continue;

		u16 value = data[code].first;
		if (value > 0)
		{
			if (get_blacklist)
			{
				blacklist.emplace_back(name);
				LOG_ERROR(HLE, "Evdev Calibration: Added button [ %d = %s ] to blacklist. Value = %d", code, name, value);
			}
			else if (value > pressed_button.first)
				pressed_button = { value, name };
		}
	}

	for (const auto& button : axis_list)
	{
		int code = button.first;
		std::string name = button.second;

		if (data[code].second)
			continue;

		if (!get_blacklist && std::find(blacklist.begin(), blacklist.end(), name) != blacklist.end())
			continue;

		u16 value = data[code].first;

		if (((code == ABS_X || code == ABS_Y) && value < m_thumb_threshold)
		 || ((code == ABS_RX || code == ABS_RY) && value < m_thumb_threshold)
		 || (code == ABS_Z && value < m_trigger_threshold)
		 || (code == ABS_RZ && value < m_trigger_threshold))
			continue;

		if (value > 0)
		{
			if (get_blacklist)
			{
				blacklist.emplace_back(name);
				LOG_ERROR(HLE, "Evdev Calibration: Added axis [ %d = %s ] to blacklist. Value = %d", code, name, value);
			}
			else if (value > pressed_button.first)
				pressed_button = { value, name };
		}
	}

	for (const auto& button : rev_axis_list)
	{
		int code = button.first;
		std::string name = button.second;

		if (!data[code].second)
			continue;

		if (!get_blacklist && std::find(blacklist.begin(), blacklist.end(), name) != blacklist.end())
			continue;

		u16 value = data[code].first;

		if (((code == ABS_X || code == ABS_Y) && value < m_thumb_threshold)
		 || ((code == ABS_RX || code == ABS_RY) && value < m_thumb_threshold)
		 || (code == ABS_Z && value < m_trigger_threshold)
		 || (code == ABS_RZ && value < m_trigger_threshold))
			continue;

		if (value > 0)
		{
			if (get_blacklist)
			{
				blacklist.emplace_back(name);
				LOG_ERROR(HLE, "Evdev Calibration: Added rev axis [ %d = %s ] to blacklist. Value = %d", code, name, value);
			}
			else if (value > pressed_button.first)
				pressed_button = { value, name };
		}
	}

	if (get_blacklist)
	{
		if (blacklist.size() <= 0)
			LOG_SUCCESS(HLE, "Evdev Calibration: Blacklist is clear. No input spam detected");
		return;
	}

	// get stick values
	int lxp = 0, lxn = 0, lyp = 0, lyn = 0, rxp = 0, rxn = 0, ryp = 0, ryn = 0;

	if (libevdev_has_event_code(dev, EV_ABS, ABS_X))
		data[ABS_X].second ? lxn = data[ABS_X].first : lxp = data[ABS_X].first;

	if (libevdev_has_event_code(dev, EV_ABS, ABS_Y))
		data[ABS_Y].second ? lyp = data[ABS_Y].first : lyn = data[ABS_Y].first;

	if (libevdev_has_event_code(dev, EV_ABS, ABS_RX))
		data[ABS_RX].second ? rxn = data[ABS_RX].first : rxp = data[ABS_RX].first;

	if (libevdev_has_event_code(dev, EV_ABS, ABS_RY))
		data[ABS_RY].second ? ryp = data[ABS_RY].first : ryn = data[ABS_RY].first;

	int preview_values[6] = { data[ABS_Z].first, data[ABS_RZ].first, lxp - lxn, lyp - lyn, rxp - rxn, ryp - ryn };

	if (pressed_button.first > 0)
		return callback(pressed_button.first, pressed_button.second, preview_values);
	else
		return callback(0, "", preview_values);
}

void evdev_joystick_handler::TestVibration(const std::string& padId, u32 largeMotor, u32 smallMotor)
{
}

void evdev_joystick_handler::TranslateButtonPress(u64 keyCode, bool& pressed, u16& value, bool ignore_threshold)
{
	// Update the pad button values based on their type and thresholds.
	// With this you can use axis or triggers as buttons or vice versa
	switch (keyCode)
	{
	case ABS_Z:
		value = value > (ignore_threshold ? 0 : m_pad_config.ltriggerthreshold) ? value : 0;
		pressed = value > 0;
		break;
	case ABS_RZ:
		value = value > (ignore_threshold ? 0 : m_pad_config.rtriggerthreshold) ? value : 0;
		pressed = value > 0;
		break;
	case ABS_X:
	case ABS_Y:
		value = value > (ignore_threshold ? 0 : m_pad_config.lstickdeadzone) ? value : 0;
		pressed = value > 0;
		break;
	case ABS_RX:
	case ABS_RY:
		value = value > (ignore_threshold ? 0 : m_pad_config.rstickdeadzone) ? value : 0;
		pressed = value > 0;
		break;
	default:
		pressed = value > 0;
		value = pressed ? value : 0;
		break;
	}
}

int evdev_joystick_handler::GetButtonInfo(const input_event& evt, libevdev* dev, int& value, bool& is_negative)
{
	int code = evt.code;
	int val = evt.value;

	switch (evt.type)
	{
	case EV_KEY:
	{
		// get the button value and return its code
		if (code < BTN_MISC)
			return -1;

		value = val > 0 ? 255 : 0;
		return code;
	}
	case EV_ABS:
	{
		// Triggers should be ABS_Z and ABS_RZ and do not need handling of negative values
		if (code == ABS_Z || code == ABS_RZ)
		{
			value = static_cast<u16>(ScaleStickInput(val, libevdev_get_abs_minimum(dev, code), libevdev_get_abs_maximum(dev, code)));
			return code;
		}

		float fvalue = ScaleStickInput2(val, libevdev_get_abs_minimum(dev, code), libevdev_get_abs_maximum(dev, code));
		is_negative = fvalue < 0;
		value = static_cast<u16>(std::abs(fvalue));

		return code;
	}
	default:
		return -1;
	}
}

std::vector<std::string> evdev_joystick_handler::ListDevices()
{
	Init();
	std::vector<std::string> evdev_joystick_list;
	fs::dir devdir{"/dev/input/"};
	fs::dir_entry et;

	while (devdir.read(et))
	{
		// Check if the entry starts with event (a 5-letter word)
		if (et.name.size() > 5 && et.name.compare(0, 5,"event") == 0)
		{
			int fd = open(("/dev/input/" + et.name).c_str(), O_RDONLY|O_NONBLOCK);
			struct libevdev *dev = NULL;
			int rc = libevdev_new_from_fd(fd, &dev);
			if (rc < 0)
			{
				// If it's just a bad file descriptor, don't bother logging, but otherwise, log it.
				if (rc != -9)
					LOG_WARNING(GENERAL, "Failed to connect to device at %s, the error was: %s", "/dev/input/" + et.name, strerror(-rc));
				libevdev_free(dev);
				close(fd);
				continue;
			}
			if (libevdev_has_event_type(dev, EV_KEY) &&
				libevdev_has_event_code(dev, EV_ABS, ABS_X) &&
				libevdev_has_event_code(dev, EV_ABS, ABS_Y))
			{
				// It's a joystick.
				evdev_joystick_list.push_back(libevdev_get_name(dev));
			}
			libevdev_free(dev);
			close(fd);
		}
	}
	return evdev_joystick_list;
}

int evdev_joystick_handler::add_device(const std::string& device, bool in_settings, std::shared_ptr<Pad> pad, const std::unordered_map<int, bool>& axis_map)
{
	if (in_settings && m_pad_index >= 0) return m_pad_index;

	// Now we need to find the device with the same name, and make sure not to grab any duplicates.
	fs::dir devdir{ "/dev/input/" };
	fs::dir_entry et;
	while (devdir.read(et))
	{
		// Check if the entry starts with event (a 5-letter word)
		if (et.name.size() > 5 && et.name.compare(0, 5, "event") == 0)
		{
			std::string path = "/dev/input/" + et.name;
			int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
			struct libevdev *dev = NULL;
			int rc = libevdev_new_from_fd(fd, &dev);
			if (rc < 0)
			{
				// If it's just a bad file descriptor, don't bother logging, but otherwise, log it.
				if (rc != -9)
					LOG_WARNING(GENERAL, "Failed to connect to device at %s, the error was: %s", path, strerror(-rc));
				libevdev_free(dev);
				close(fd);
				continue;
			}
			const std::string name = libevdev_get_name(dev);
			if (libevdev_has_event_type(dev, EV_KEY) &&
				libevdev_has_event_code(dev, EV_ABS, ABS_X) &&
				libevdev_has_event_code(dev, EV_ABS, ABS_Y) &&
				name == device)
			{
				// It's a joystick.

				// Now let's make sure we don't already have this one.
				auto it = std::find_if(devices.begin(), devices.end(), [&path](const EvdevDevice &device) { return path == device.path; });
				if (it != devices.end())
				{
					libevdev_free(dev);
					close(fd);
					return std::distance(devices.begin(), it);
				}

				// Alright, now that we've confirmed we haven't added this joystick yet, les do dis.
				devices.push_back({nullptr, path, pad, axis_map});
				return devices.size() - 1;
			}
			libevdev_free(dev);
			close(fd);
		}
	}
	return -1;
}

void evdev_joystick_handler::ThreadProc()
{
	update_devs();

	for (auto& device : devices)
	{
		auto pad = device.pad;
		auto axis_orientations = device.axis_orientations;
		auto& dev = device.device;
		if (dev == nullptr) continue;

		// Try to query the latest event from the joystick.
		input_event evt;
		int ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &evt);

		// Grab any pending sync event.
		if (ret == LIBEVDEV_READ_STATUS_SYNC)
		{
			LOG_NOTICE(GENERAL, "Captured sync event");
			ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_SYNC, &evt);
		}

		if (ret < 0)
		{
			// -EAGAIN signifies no available events, not an actual *error*.
			if (ret != -EAGAIN)
				LOG_ERROR(GENERAL, "Failed to read latest event from joystick: %s [errno %d]", strerror(-ret), -ret);
			continue;
		}

		bool is_negative = false;
		int value;
		int button_code = GetButtonInfo(evt, dev, value, is_negative);
		if (button_code < 0 || value < 0)
			continue;

		bool is_button_or_trigger = evt.type == EV_KEY || button_code == ABS_Z || button_code == ABS_RZ;

		// Translate any corresponding keycodes to our normal DS3 buttons and triggers
		for (int i = 0; i < static_cast<int>(pad->m_buttons.size() - 1); i++) // skip reserved button
		{
			if (pad->m_buttons[i].m_keyCode != button_code)
				continue;

			// Be careful to handle mapped axis specially
			if (evt.type == EV_ABS)
			{
				// get axis direction and skip on error or set to 0 if the stick/hat is actually pointing to the other direction.
				// maybe mimic on error, needs investigation. FindAxisDirection should ideally never return -1 anyway
				int direction = FindAxisDirection(axis_orientations, i);
				if (direction < 0)
				{
					LOG_ERROR(HLE, "FindAxisDirection = %d, Button Nr.%d, value = %d", direction, i, value);
					continue;
				}
				else if (direction != (is_negative ? 1 : 0))
				{
					pad->m_buttons[i].m_value = 0;
					pad->m_buttons[i].m_pressed = 0;
					continue;
				}
			}

			pad->m_buttons[i].m_value = static_cast<u16>(value);
			TranslateButtonPress(button_code, pad->m_buttons[i].m_pressed, pad->m_buttons[i].m_value);
		}

		// Translate any corresponding keycodes to our two sticks. (ignoring thresholds for now)
		for (int idx = 0; idx < static_cast<int>(pad->m_sticks.size()); idx++)
		{
			bool pressed_min = false, pressed_max = false;

			// m_keyCodeMin is the mapped key for left or down
			if (pad->m_sticks[idx].m_keyCodeMin == button_code)
			{
				bool is_direction_min = false;

				if (!is_button_or_trigger && evt.type == EV_ABS)
				{
					int index = BUTTON_COUNT + (idx * 2) + 1;
					int min_direction = FindAxisDirection(axis_orientations, index);
					if (min_direction < 0)
					{
						LOG_ERROR(HLE, "keyCodeMin FindAxisDirection = %d, Axis Nr.%d, Button Nr.%d, value = %d", min_direction, idx, index, value);
					}
					else
					{
						is_direction_min = is_negative == (min_direction == 1);
					}
				}

				if (is_button_or_trigger || is_direction_min)
				{
					device.val_min[idx] = value;
					TranslateButtonPress(button_code, pressed_min, device.val_min[idx], true);
				}
				else // set to 0 to avoid remnant counter axis values
					device.val_min[idx] = 0;
			}

			// m_keyCodeMax is the mapped key for right or up
			if (pad->m_sticks[idx].m_keyCodeMax == button_code)
			{
				bool is_direction_max = false;

				if (!is_button_or_trigger && evt.type == EV_ABS)
				{
					int index = BUTTON_COUNT + (idx * 2);
					int max_direction = FindAxisDirection(axis_orientations, index);
					if (max_direction < 0)
					{
						LOG_ERROR(HLE, "keyCodeMax FindAxisDirection = %d, Axis Nr.%d, Button Nr.%d, value = %d", max_direction, idx, index, value);
					}
					else
					{
						is_direction_max = is_negative == (max_direction == 1);
					}
				}

				if (is_button_or_trigger || is_direction_max)
				{
					device.val_max[idx] = value;
					TranslateButtonPress(button_code, pressed_max, device.val_max[idx], true);
				}
				else // set to 0 to avoid remnant counter axis values
					device.val_max[idx] = 0;
			}

			// cancel out opposing values and get the resulting difference. if there was no change, use the old value.
			device.stick_val[idx] = device.val_max[idx] - device.val_min[idx];
		}

		// Normalize our two stick's axis based on the thresholds
		u16 lx, ly, rx, ry;

		// Normalize our two stick's axis based on the thresholds
		std::tie(lx, ly) = NormalizeStickDeadzone(device.stick_val[0], device.stick_val[1], m_pad_config.lstickdeadzone);
		std::tie(rx, ry) = NormalizeStickDeadzone(device.stick_val[2], device.stick_val[3], m_pad_config.rstickdeadzone);

		// these are added with previous value and divided to 'smooth' out the readings

		if (m_pad_config.padsquircling != 0)
		{
			std::tie(lx, ly) = ConvertToSquirclePoint(lx, ly, m_pad_config.padsquircling);
			std::tie(rx, ry) = ConvertToSquirclePoint(rx, ry, m_pad_config.padsquircling);
		}

		pad->m_sticks[0].m_value = lx;
		pad->m_sticks[1].m_value = 255 - ly;
		pad->m_sticks[2].m_value = rx;
		pad->m_sticks[3].m_value = 255 - ry;
	}
}

// Search axis_orientations map for the direction by index, returns -1 if not found, 0 for positive and 1 for negative
int evdev_joystick_handler::FindAxisDirection(const std::unordered_map<int, bool>& map, int index)
{
	auto it = map.find(index);
	if (it == map.end())
		return -1;
	else
		return it->second;
};

bool evdev_joystick_handler::bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device)
{
	Init();

	std::unordered_map<int, bool> axis_orientations;
	int i = 0; // increment to know the axis location (17-24). Be careful if you ever add more find_key() calls in here (BUTTON_COUNT = 17)

	auto find_key = [&](const cfg::string& name)
	{
		int key = FindKeyCode(axis_list, name, false);
		if (key >= 0)
			axis_orientations.emplace(i, false);

		if (key < 0)
		{
			key = FindKeyCode(rev_axis_list, name, false);
			if (key >= 0)
				axis_orientations.emplace(i, true);
		}

		if (key < 0)
			key = FindKeyCode(button_list, name);

		i++;
		return key;
	};

	pad->Init
	(
		CELL_PAD_STATUS_CONNECTED | CELL_PAD_STATUS_ASSIGN_CHANGES,
		CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE,
		CELL_PAD_DEV_TYPE_STANDARD
	);

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.triangle), CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.circle),   CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.cross),    CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.square),   CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.l2),       CELL_PAD_CTRL_L2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.r2),       CELL_PAD_CTRL_R2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.l1),       CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.r1),       CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.start),    CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.select),   CELL_PAD_CTRL_SELECT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.l3),       CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.r3),       CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.ps),       0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.up),       CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.down),     CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.left),     CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.right),    CELL_PAD_CTRL_RIGHT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, 0x0); // Reserved

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, find_key(m_pad_config.ls_left), find_key(m_pad_config.ls_right));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, find_key(m_pad_config.ls_down), find_key(m_pad_config.ls_up));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, find_key(m_pad_config.rs_left), find_key(m_pad_config.rs_right));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, find_key(m_pad_config.rs_down), find_key(m_pad_config.rs_up));

	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_X, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Y, 399);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Z, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_G, 512);

	pad->m_vibrateMotors.emplace_back(true, 0);
	pad->m_vibrateMotors.emplace_back(false, 0);

	if (!add_device(device, false, pad, axis_orientations))
	{
		//return;
	}
	update_devs();
	return true;
}

#endif
