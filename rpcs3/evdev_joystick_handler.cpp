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
	THUMB_MIN = 0;
	THUMB_MAX = 255;
	TRIGGER_MIN = 0;
	TRIGGER_MAX = 255;
	VIBRATION_MIN = 0;
	VIBRATION_MAX = 65535;

	// Set this handler's type and save location
	m_pad_config.cfg_type = "evdev";
	m_pad_config.cfg_name = fs::get_config_dir() + "/config_evdev.yml";

	// Set default button mapping
	m_pad_config.ls_left.def  = rev_axis_list.at(ABS_X);
	m_pad_config.ls_down.def  = rev_axis_list.at(ABS_Y);
	m_pad_config.ls_right.def = axis_list.at(ABS_X);
	m_pad_config.ls_up.def    = axis_list.at(ABS_Y);
	m_pad_config.rs_left.def  = rev_axis_list.at(ABS_RX);
	m_pad_config.rs_down.def  = rev_axis_list.at(ABS_RY);
	m_pad_config.rs_right.def = axis_list.at(ABS_RX);
	m_pad_config.rs_up.def    = axis_list.at(ABS_RY);
	m_pad_config.start.def    = button_list.at(BTN_START);
	m_pad_config.select.def   = button_list.at(BTN_SELECT);
	m_pad_config.ps.def       = button_list.at(BTN_MODE);
	m_pad_config.square.def   = button_list.at(BTN_X);
	m_pad_config.cross.def    = button_list.at(BTN_A);
	m_pad_config.circle.def   = button_list.at(BTN_B);
	m_pad_config.triangle.def = button_list.at(BTN_Y);
	m_pad_config.left.def     = rev_axis_list.at(ABS_HAT0X);
	m_pad_config.down.def     = rev_axis_list.at(ABS_HAT0Y);
	m_pad_config.right.def    = axis_list.at(ABS_HAT0X);
	m_pad_config.up.def       = axis_list.at(ABS_HAT0Y);
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

bool evdev_joystick_handler::update_device(EvdevDevice device, bool use_cell)
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
		if (!was_connected)
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

void evdev_joystick_handler::GetNextButtonPress(const std::string& padId, const std::vector<int>& deadzones, const std::function<void(u16, std::string)>& callback)
{
	// Add device if not yet present
	int index = add_device(padId);
	if (index < 0) return;

	// Check if our device is connected
	if (!update_device(devices[index], false)) return;

	auto& dev = devices[index].device;

	// Try to query the latest event from the joystick.
	input_event evt;
	int ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &evt);

	// Grab any pending sync event.
	if (ret == LIBEVDEV_READ_STATUS_SYNC)
	{
		ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_SYNC, &evt);
	}
	if (ret < 0) return;

	// Get the button info corresponding to our current event
	bool is_negative = false;
	int value;
	int button_code = GetButtonInfo(evt, dev, value, is_negative);
	if (button_code == -1) return;

	int ltriggerthreshold = deadzones[0];
	int rtriggerthreshold = deadzones[1];
	int lstickdeadzone = deadzones[2];
	int rstickdeadzone = deadzones[3];

	switch (evt.type)
	{
	case EV_KEY:
	{
		// handle normal button presses
		auto button = button_list.find(button_code);
		if (button == button_list.end())
			return;

		if (value > 0)
			return callback(button->first, button->second);

		return;
	}
	case EV_ABS:
	{
		// handle positive and negative axis movement
		std::unordered_map<u32, std::string>::const_iterator button;
		if (is_negative)
		{
			button = rev_axis_list.find(button_code);
			if (button == rev_axis_list.end())
				return;
		}
		else
		{
			button = axis_list.find(button_code);
			if (button == axis_list.end())
				return;
		}

		// handle specific axis and their thresholds
		switch (button_code)
		{
		case ABS_Z:
			if (value > ltriggerthreshold)
				return callback(button->first, button->second);
			return;
		case ABS_RZ:
			if (value > rtriggerthreshold)
				return callback(button->first, button->second);
			return;
		case ABS_X:
		case ABS_Y:
			if (value > lstickdeadzone)
				return callback(button->first, button->second);
			return;
		case ABS_RX:
		case ABS_RY:
			if (value > rstickdeadzone)
				return callback(button->first, button->second);
			return;
		default:
			if (value > 0)
				return callback(button->first, button->second);
			return;
		}
	}
	default:
		return;
	}
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
		value = ScaleStickInput(val, libevdev_get_abs_minimum(dev, code), libevdev_get_abs_maximum(dev, code));
		is_negative = value <= 127;

		if (is_negative)
			value = Clamp0To255((127.5f - value) * 2.0f);
		else
			value = Clamp0To255((value - 127.5f) * 2.0f);

		// choose arbitrary deadzone for hats
		if (code >= ABS_HAT0X && code <= ABS_HAT3Y)
		{
			// Choose one of these depending on how it behaves in test
			// value = val > 0 ? 255 : 0;
			value = val > 50 ? val : 0;
		}
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

int evdev_joystick_handler::add_device(const std::string& device, std::shared_ptr<Pad> pad)
{
	// Now we need to find the device with the same name, and make sure not to grab any duplicates.
	fs::dir devdir{ "/dev/input/" };
	fs::dir_entry et;
	while (devdir.read(et))
	{
		// Check if the entry starts with event (a 5-letter word)
		if (et.name.size() > 5 && et.name.compare(0, 5, "event") == 0)
		{
			std::string path = "";
			int fd = open(("/dev/input/" + et.name).c_str(), O_RDONLY | O_NONBLOCK);
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
			const std::string name = libevdev_get_name(dev);
			if (libevdev_has_event_type(dev, EV_KEY) &&
				libevdev_has_event_code(dev, EV_ABS, ABS_X) &&
				libevdev_has_event_code(dev, EV_ABS, ABS_Y) &&
				name == device)
			{
				// It's a joystick.

				// Now let's make sure we don't already have this one.
				auto it = std::find_if(devices.begin(), devices.end(), [path](EvdevDevice device) { return path == device.path; });
				if (it != devices.end())
				{
					libevdev_free(dev);
					close(fd);
					return std::distance(devices.begin(), it);
				}

				// Alright, now that we've confirmed we haven't added this joystick yet, les do dis.
				devices.push_back({nullptr, fmt::format("/dev/input/%s", et.name), pad });
				return devices.size() - 1;
			}
			libevdev_free(dev);
			close(fd);
		}
	}
	return -1;
}

bool evdev_joystick_handler::bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device)
{
        Init();

				auto find_key = [=](const std::string& name)
				{
					int key = FindKeyCode(button_list, name);
					if (key < 0)
						key = FindKeyCode(axis_list, name);
					if (key < 0)
						key = FindKeyCode(rev_axis_list, name);
					if (key < 0)
						key = std::stoi(name);
					return key;
				};

        pad->Init(
            CELL_PAD_STATUS_DISCONNECTED,
            CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
            CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE,
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
				//pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, m_pad_config.ps),       CELL_PAD_CTRL_PS);
        pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, 0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support
        pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.up),       CELL_PAD_CTRL_UP);
        pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.down),     CELL_PAD_CTRL_DOWN);
        pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.left),     CELL_PAD_CTRL_LEFT);
        pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.right),    CELL_PAD_CTRL_RIGHT);

				pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X,  find_key(m_pad_config.ls_left), find_key(m_pad_config.ls_right));
				pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y,  find_key(m_pad_config.ls_down), find_key(m_pad_config.ls_up));
				pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, find_key(m_pad_config.rs_left), find_key(m_pad_config.rs_right));
				pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, find_key(m_pad_config.rs_down), find_key(m_pad_config.rs_up));
        pad->m_vibrateMotors.emplace_back(true, 0);
        pad->m_vibrateMotors.emplace_back(false, 0);

				if (!add_device(device, pad))
				{
					//return;
				}
        update_devs();
        return true;
}

void evdev_joystick_handler::ThreadProc()
{
    update_devs();

    for (auto& device : devices)
    {
        auto pad = device.pad;
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

				bool is_negative;
				int value;
				int button_code = GetButtonInfo(evt, dev, value, is_negative);
				if (button_code < 0 || value <= 0)
					continue;

				// Translate any corresponding keycodes to our normal DS3 buttons and triggers
				for (auto & btn : pad->m_buttons)
				{
					if (btn.m_keyCode != button_code)
						continue;

					btn.m_value = static_cast<u16>(value);
					TranslateButtonPress(button_code, btn.m_pressed, btn.m_value);
				}

				// used to get the absolute value of an axis
				float stick_val[4];

				// Translate any corresponding keycodes to our two sticks. (ignoring thresholds for now)
				for (int idx = 0; idx < static_cast<int>(pad->m_sticks.size()); idx++)
				{
					bool pressed_min, pressed_max;
					u16 val_min = 0, val_max = 0;

					// m_keyCodeMin is the mapped key for left or down
					if (pad->m_sticks[idx].m_keyCodeMin == button_code)
					{
						val_min = value;
						TranslateButtonPress(button_code, pressed_min, val_min, true);
					}

					// m_keyCodeMax is the mapped key for right or up
					if (pad->m_sticks[idx].m_keyCodeMax == button_code)
					{
						val_max = value;
						TranslateButtonPress(button_code, pressed_max, val_max, true);
					}

					// cancel out opposing values and get the resulting difference
					// if there was no change, use the old value
					if (pressed_min || pressed_max)
						stick_val[idx] = val_max - val_min;
					else
						stick_val[idx] = pad->m_sticks[idx].m_value;
				}

				// Normalize our two stick's axis based on the thresholds
				u16 lx, ly, rx, ry;

				// Normalize our two stick's axis based on the thresholds
				std::tie(lx, ly) = NormalizeStickDeadzone(stick_val[0], stick_val[1], m_pad_config.lstickdeadzone);
				std::tie(rx, ry) = NormalizeStickDeadzone(stick_val[2], stick_val[3], m_pad_config.rstickdeadzone);

				ly = 255 - ly;
				ry = 255 - ry;
				
				// these are added with previous value and divided to 'smooth' out the readings

				if (m_pad_config.padsquircling != 0)
				{
					std::tie(lx, ly) = ConvertToSquirclePoint(lx, ly, m_pad_config.padsquircling);
					std::tie(rx, ry) = ConvertToSquirclePoint(rx, ry, m_pad_config.padsquircling);
				}

				pad->m_sticks[0].m_value = (lx + pad->m_sticks[0].m_value) / 2; // LX
				pad->m_sticks[1].m_value = (ly + pad->m_sticks[1].m_value) / 2; // LY
				pad->m_sticks[2].m_value = (rx + pad->m_sticks[2].m_value) / 2; // RX
				pad->m_sticks[3].m_value = (ry + pad->m_sticks[3].m_value) / 2; // RY
    }
}

#endif
