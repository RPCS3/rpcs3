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
	m_pad_config.ls_left.def  = rev_axis_list.at(EvdevAxisCodes::ABS_X);
	m_pad_config.ls_down.def  = rev_axis_list.at(EvdevAxisCodes::ABS_Y);
	m_pad_config.ls_right.def = axis_list.at(EvdevAxisCodes::ABS_X);
	m_pad_config.ls_up.def    = axis_list.at(EvdevAxisCodes::ABS_Y);
	m_pad_config.rs_left.def  = rev_axis_list.at(EvdevAxisCodes::ABS_RX);
	m_pad_config.rs_down.def  = rev_axis_list.at(EvdevAxisCodes::ABS_RY);
	m_pad_config.rs_right.def = axis_list.at(EvdevAxisCodes::ABS_RX);
	m_pad_config.rs_up.def    = axis_list.at(EvdevAxisCodes::ABS_RY);
	m_pad_config.start.def    = button_list.at(EvdevKeyCodes::BTN_START);
	m_pad_config.select.def   = button_list.at(EvdevKeyCodes::BTN_SELECT);
	m_pad_config.ps.def       = button_list.at(EvdevKeyCodes::BTN_GAMEPAD);
	m_pad_config.square.def   = button_list.at(EvdevKeyCodes::BTN_X);
	m_pad_config.cross.def    = button_list.at(EvdevKeyCodes::BTN_A);
	m_pad_config.circle.def   = button_list.at(EvdevKeyCodes::BTN_B);
	m_pad_config.triangle.def = button_list.at(EvdevKeyCodes::BTN_Y);
	m_pad_config.left.def     = rev_axis_list.at(EvdevAxisCodes::ABS_HAT0X);
	m_pad_config.down.def     = rev_axis_list.at(EvdevAxisCodes::ABS_HAT0Y);
	m_pad_config.right.def    = rev_axis_list.at(EvdevAxisCodes::ABS_HAT0X);
	m_pad_config.up.def       = rev_axis_list.at(EvdevAxisCodes::ABS_HAT0Y);
	m_pad_config.r1.def       = button_list.at(EvdevKeyCodes::BTN_TR);
	m_pad_config.r2.def       = axis_list.at(EvdevAxisCodes::ABS_Z);
	m_pad_config.r3.def       = button_list.at(EvdevKeyCodes::BTN_THUMB);
	m_pad_config.l1.def       = button_list.at(EvdevKeyCodes::BTN_TL);
	m_pad_config.l2.def       = axis_list.at(EvdevAxisCodes::ABS_RZ);
	m_pad_config.l3.def       = button_list.at(EvdevKeyCodes::BTN_THUMB2);

	// Set default misc variables
	m_pad_config.lstickdeadzone.def = 10;   // between 0 and 255
	m_pad_config.rstickdeadzone.def = 10;   // between 0 and 255
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

void evdev_joystick_handler::update_devs()
{
	for (u32 index = 0; index < joy_devs.size(); ++index)
	{
		libevdev*& dev = joy_devs[index];
		bool was_connected = dev != nullptr;

		if (index >= joy_paths.size()) return false;
		const auto& path = joy_paths[index];

		if (access(path.c_str(), R_OK) == -1)
		{
			if (was_connected)
			{
				// It was disconnected.
				pads[index]->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;

				int fd = libevdev_get_fd(dev);
				libevdev_free(dev);
				close(fd);
				dev = nullptr;
			}
			pads[index]->m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
			LOG_ERROR(GENERAL, "Joystick %s is not present or accessible [previous status: %d]", path.c_str(),
				was_connected ? 1 : 0);
			return false;
		}

		if (was_connected) return true;  // It's already been connected, and the js is still present.
		int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);

		if (fd == -1)
		{
			int err = errno;
			LOG_ERROR(GENERAL, "Failed to open joystick #%d: %s [errno %d]", index, strerror(err), err);
			return false;
		}

		int ret = libevdev_new_from_fd(fd, &dev);
		if (ret < 0)
		{
			LOG_ERROR(GENERAL, "Failed to initialize libevdev for joystick #%d: %s [errno %d]", index, strerror(-ret), -ret);
			return false;
		}

		LOG_NOTICE(GENERAL, "Opened joystick #%d '%s' at %s (fd %d)", index, libevdev_get_name(dev), path, fd);

		if (!was_connected)
			// Connection status changed from disconnected to connected.
			pads[index]->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
		pads[index]->m_port_status |= CELL_PAD_STATUS_CONNECTED;

		//int buttons = 0;
		//for (u32 i = BTN_JOYSTICK; i < KEY_MAX; i++)
		//{
		//	if (libevdev_has_event_code(dev, EV_KEY, i))
		//	{
		//		LOG_NOTICE(GENERAL, "Joystick #%d has button %d as %d", index, i, buttons);
		//		joy_button_maps[index][i - BTN_MISC] = buttons++;
		//	}
		//}

		//int axes = 0;
		//for (u32 i = ABS_X; i <= ABS_RZ; i++)
		//{
		//	if (libevdev_has_event_code(dev, EV_ABS, i))
		//	{
		//		LOG_NOTICE(GENERAL, "Joystick #%d has axis %d as %d", index, i, axes);
		//
		//		axis_ranges[i].first  = libevdev_get_abs_minimum(dev, i);
		//		axis_ranges[i].second = libevdev_get_abs_maximum(dev, i);
		//
		//		// Skip ABS_Z and ABS_RZ on controllers where it's used for the triggers.
		//		if (m_pad_config.has_axis_triggers && (i == ABS_Z || i == ABS_RZ)) continue;
		//		joy_axis_maps[index][i - ABS_X] = axes++;
		//	}
		//}

		//for (u32 i = ABS_HAT0X; i <= ABS_HAT3Y; i += 2)
		//{
		//	if (libevdev_has_event_code(dev, EV_ABS, i) ||
		//		libevdev_has_event_code(dev, EV_ABS, i + 1))
		//	{
		//		LOG_NOTICE(GENERAL, "Joystick #%d has hat %d", index, i);
		//		joy_hat_ids[index] = i - ABS_HAT0X;
		//	}
		//}

		return true;
	}
}

void evdev_joystick_handler::Close()
{
	for (auto& dev : joy_devs)
	{
		if (dev != nullptr)
		{
			int fd = libevdev_get_fd(dev);
			libevdev_free(dev);
			close(fd);
		}
	}
}

void evdev_joystick_handler::ConfigController(const std::string& device)
{
	pad_settings_dialog dlg(&m_pad_config, device, *this);
	dlg.exec();
}

void evdev_joystick_handler::GetNextButtonPress(const std::string& padid, const std::function<void(std::string)>& callback)
{
	u32 device_number = 0;
	size_t pos = padid.find("evdev Pad #");

	if (pos != std::string::npos)
	{
		device_number = std::stoul(padid.substr(pos + 11));
	}

	if (pos == std::string::npos || device_number >= MAX_GAMEPADS)
	{
		return;
	}

	update_devs();

	auto& dev = joy_devs[device_number];
	if (dev == nullptr) return;

	// Try to query the latest event from the joystick.
	input_event evt;
	int ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &evt);

	// Grab any pending sync event.
	if (ret == LIBEVDEV_READ_STATUS_SYNC)
	{
		ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_SYNC, &evt);
	}

	if (ret < 0)
	{
		// -EAGAIN signifies no available events, not an actual *error*.
		if (ret != -EAGAIN)
			LOG_ERROR(GENERAL, "Failed to read latest event from joystick #%d: %s [errno %d]", device_number, strerror(-ret), -ret);
		return;
	}

	// Get the button info corresponding to our current event
	bool is_negative = false;
	int value;
	int button_code = GetButtonInfo(evt, device_number, value, is_negative);

	if (button_code == -1)
		return;

	switch (evt.type)
	{
	case EV_KEY:
		// handle normal button presses
		auto button = button_list.find(button_code);
		if (button == button_list.end())
			return;

		if (value > 0)
			return callback(button->second);

		return;
	case EV_ABS:
		// handle positive and negative axis movement
		if (is_negative)
		{
			auto button = rev_axis_list.find(button_code);
			if (button == rev_axis_list.end())
				return;
		}
		else
		{
			auto button = axis_list.find(button_code);
			if (button == axis_list.end())
				return;
		}

		// handle specific axis and their thresholds
		switch (button_code)
		{
		case ABS_Z:
			if (value > m_pad_config.ltriggerthreshold)
				return callback(button->second);
			return;
		case ABS_RZ:
			if (value > m_pad_config.rtriggerthreshold)
				return callback(button->second);
			return;
		case ABS_X:
		case ABS_Y:
			if (value > m_pad_config.lstickdeadzone)
				return callback(button->second);
			return;
		case ABS_RX:
		case ABS_RY:
			if (value > m_pad_config.rstickdeadzone)
				return callback(button->second);
			return;
		default:
			if (value > 0)
				return callback(button->second);
			return;
		}
	default:
		return;
	}
}

void evdev_joystick_handler::TestVibration(const std::string& padId, u32 largeMotor, u32 smallMotor)
{
}

void evdev_joystick_handler::TranslateButtonPress(u32 keyCode, bool& pressed, u16& value, bool ignore_threshold)
{
	// Update the pad button values based on their type and thresholds.
	// With this you can use axis or triggers as buttons or vice versa
	switch (keyCode)
	{
	case EvdevAxisCodes::ABS_Z:
		value = value > (ignore_threshold ? 0 : m_pad_config.ltriggerthreshold) ? value : 0;
		pressed = value > 0;
		break;
	case EvdevAxisCodes::ABS_RZ:
		value = value > (ignore_threshold ? 0 : m_pad_config.rtriggerthreshold) ? value : 0;
		pressed = value > 0;
		break;
	case EvdevAxisCodes::ABS_X:
	case EvdevAxisCodes::ABS_Y:
		value = value > (ignore_threshold ? 0 : m_pad_config.lstickdeadzone) ? value : 0;
		pressed = value > 0;
		break;
	case EvdevAxisCodes::ABS_RX:
	case EvdevAxisCodes::ABS_RY:
		value = value > (ignore_threshold ? 0 : m_pad_config.rstickdeadzone) ? value : 0;
		pressed = value > 0;
		break;
	default:
		pressed = value > 0;
		value = pressed ? value : 0;
		break;
	}
}

int evdev_joystick_handler::GetButtonInfo(const input_event& evt, int device_number, int& value, bool& is_negative)
{
	int code = evt.code;
	int val = evt.value;

	auto& dev = joy_devs[device_number];

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
		// get the hat value and return its code
		if (code >= EvdevAxisCodes::ABS_HAT0X && code <= EvdevAxisCodes::ABS_HAT3Y)
		{
			is_negative = val < 0;
			value = val != 0 ? 255 : 0;
			return code;
		}
		// get the axis value and return its code
		else if (code >= EvdevAxisCodes::ABS_X && code <= EvdevAxisCodes::ABS_RZ)
		{
			value = NormalizeStickInput(val, 0, libevdev_get_abs_minimum(dev, code), libevdev_get_abs_maximum(dev, code), true);
			is_negative = value <= 127;

			if (is_negative)
				value = Clamp0To255((127.5f - value) * 2.0f);
			else
				value = Clamp0To255((value - 127.5f) * 2.0f);
			return code;
		}
		return -1;
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

bool evdev_joystick_handler::bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device)
{
        Init();

        // Now we need to find the device with the same name, and make sure not to grab any duplicates.
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
                const std::string name = libevdev_get_name(dev);
                if (libevdev_has_event_type(dev, EV_KEY) &&
                    libevdev_has_event_code(dev, EV_ABS, ABS_X) &&
                    libevdev_has_event_code(dev, EV_ABS, ABS_Y) &&
                    name == device)
                {
                    // It's a joystick.

                    // Now let's make sure we don't already have this one.
                    bool alreadyIn = false;
										for (int i = 0; i < joy_paths.size(); i++)
										{
											if (joy_paths[i] == fmt::format("/dev/input/%s", et.name))
											{
												alreadyIn = true;
												break;
											}
										}

                    if (alreadyIn == true)
                    {
                        libevdev_free(dev);
                        close(fd);
                        continue;
                    }

                    // Alright, now that we've confirmed we haven't added this joystick yet, les do dis.
                    joy_paths.emplace_back(fmt::format("/dev/input/%s", et.name));
                }
                libevdev_free(dev);
                close(fd);
            }
        }

				auto find_key = [=](const std::string& name)
				{
					int key = FindKeyCode(button_list, name);
					if (key < 1)
						key = FindKeyCode(axis_list, name);
					if (key < 1)
						key = FindKeyCode(rev_axis_list, name);
					return key;
				};

        joy_devs.push_back(nullptr);
				joy_axis.emplace_back(127, 127, 127, 127);
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
        pads.emplace_back(pad);
        update_devs();
        return true;
}

void evdev_joystick_handler::ThreadProc()
{
    update_devs();

    for (int i = 0; i<joy_devs.size(); i++)
    {
        auto pad = pads[i];
        auto& dev = joy_devs[i];
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
                LOG_ERROR(GENERAL, "Failed to read latest event from joystick #%d: %s [errno %d]", i, strerror(-ret), -ret);
            continue;
        }

				bool is_negative;
				int value;
				int button_code = GetButtonInfo(evt, i, value, is_negative);
				if (button_code == -1)
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
				NormalizeRawStickInput(stick_val[0], stick_val[1], m_pad_config.lstickdeadzone);
				NormalizeRawStickInput(stick_val[2], stick_val[3], m_pad_config.rstickdeadzone);

				// Convert the axis to use the 0-127-255 range
				stick_val[0] = ConvertAxis(stick_val[0]);
				stick_val[1] = 255 - ConvertAxis(stick_val[1]);
				stick_val[2] = ConvertAxis(stick_val[2]);
				stick_val[3] = 255 - ConvertAxis(stick_val[3]);

				// these are added with previous value and divided to 'smooth' out the readings
				u16 lx, ly, rx, ry;

				if (m_pad_config.padsquircling != 0)
				{
					std::tie(lx, ly) = ConvertToSquirclePoint(stick_val[0], stick_val[1], m_pad_config.padsquircling);
					std::tie(rx, ry) = ConvertToSquirclePoint(stick_val[2], stick_val[3], m_pad_config.padsquircling);
				}

				pad->m_sticks[0].m_value = (lx + pad->m_sticks[0].m_value) / 2; // LX
				pad->m_sticks[1].m_value = (ly + pad->m_sticks[1].m_value) / 2; // LY
				pad->m_sticks[2].m_value = (rx + pad->m_sticks[2].m_value) / 2; // RX
				pad->m_sticks[3].m_value = (ry + pad->m_sticks[3].m_value) / 2; // RY
    }
}

#endif
