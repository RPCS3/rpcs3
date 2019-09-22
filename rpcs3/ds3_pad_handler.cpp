#include "ds3_pad_handler.h"

#include <thread>

#ifdef _WIN32
#include <Windows.h>
#endif

ds3_pad_handler::ds3_pad_handler() : PadHandlerBase(pad_handler::ds3)
{
	init_configs();

	// set capabilities
	b_has_config = true;
	b_has_rumble = true;
	b_has_deadzones = false;

	m_name_string = "DS3 Pad #";
	m_max_devices = CELL_PAD_MAX_PORT_NUM;
}

ds3_pad_handler::~ds3_pad_handler()
{
	for (auto& controller : controllers)
	{
		if (controller->handle)
		{
			// Disable blinking and vibration
			controller->large_motor = 0;
			controller->small_motor = 0;
			send_output_report(controller);
			hid_close(controller->handle);
		}
	}
	hid_exit();
}

bool ds3_pad_handler::init_usb()
{
	if (hid_init() != 0)
	{
		LOG_FATAL(HLE, "[DS3] Failed to init hidapi for the DS3 pad handler");
		return false;
	}
	return true;
}

bool ds3_pad_handler::Init()
{
	if (is_init)
		return true;

	if (!init_usb())
		return false;

	bool warn_about_drivers = false;

	// Uses libusb for windows as hidapi will never work with UsbHid driver for the ds3 and it won't work with WinUsb either(windows hid api needs the UsbHid in the driver stack as far as I can tell)
	// For other os use hidapi and hope for the best!
	hid_device_info* hid_info = hid_enumerate(DS3_VID, DS3_PID);
	hid_device_info* head = hid_info;
	while (hid_info)
	{
		hid_device *handle = hid_open_path(hid_info->path);

#ifdef _WIN32
		u8 buf[0xFF];
		buf[0] = 0xF2;
		if (handle && (hid_get_feature_report(handle, buf, 0xFF) >= 0))
#else
		if(handle)
#endif
		{
			std::shared_ptr<ds3_device> ds3dev = std::make_shared<ds3_device>();
			ds3dev->device = hid_info->path;
			ds3dev->handle = handle;
			controllers.emplace_back(ds3dev);
		}
		else
		{
			if (handle)
				hid_close(handle);

			warn_about_drivers = true;
		}
		hid_info = hid_info->next;
	}

	hid_free_enumeration(head);

	if (warn_about_drivers)
	{
		LOG_ERROR(HLE, "[DS3] One or more DS3 pads were detected but couldn't be interacted with directly");
#if defined(_WIN32) || defined(__linux__)
		LOG_ERROR(HLE, "[DS3] Check https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration for intructions on how to solve this issue");
#endif
	}
	else if (controllers.empty())
	{
		LOG_WARNING(HLE, "[DS3] No controllers found!");
	}
	else
	{
		LOG_SUCCESS(HLE, "[DS3] Controllers found: %d", controllers.size());
	}

	is_init = true;
	return true;
}

std::vector<std::string> ds3_pad_handler::ListDevices()
{
	std::vector<std::string> ds3_pads_list;

	if (!Init())
		return ds3_pads_list;

	for (size_t i = 1; i <= controllers.size(); ++i) // Controllers 1-n in GUI
	{
		ds3_pads_list.emplace_back(m_name_string + std::to_string(i));
	}

	return ds3_pads_list;
}

bool ds3_pad_handler::bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device)
{
	std::shared_ptr<ds3_device> ds3device = get_device(device);
	if (ds3device == nullptr || ds3device->handle == nullptr)
		return false;

	int index = static_cast<int>(bindings.size());
	m_pad_configs[index].load();
	ds3device->config = &m_pad_configs[index];
	pad_config* p_profile = ds3device->config;
	if (p_profile == nullptr)
		return false;

	pad->Init
	(
		CELL_PAD_STATUS_DISCONNECTED,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE,
		CELL_PAD_DEV_TYPE_STANDARD,
		p_profile->device_class_type
	);

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->l2),       CELL_PAD_CTRL_L2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->r2),       CELL_PAD_CTRL_R2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->up),       CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->down),     CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->left),     CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->right),    CELL_PAD_CTRL_RIGHT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->square),   CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->cross),    CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->circle),   CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->triangle), CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->l1),       CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, FindKeyCode(button_list, p_profile->r1),       CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->select),   CELL_PAD_CTRL_SELECT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->start),    CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->l3),       CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->r3),       CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, FindKeyCode(button_list, p_profile->ps),       0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support
	//pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0,                                             0x0); // Reserved (and currently not in use by rpcs3 at all)

	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_X, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Y, 399);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Z, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_G, 512);

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X,  FindKeyCode(button_list, p_profile->ls_left), FindKeyCode(button_list, p_profile->ls_right));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y,  FindKeyCode(button_list, p_profile->ls_down), FindKeyCode(button_list, p_profile->ls_up));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, FindKeyCode(button_list, p_profile->rs_left), FindKeyCode(button_list, p_profile->rs_right));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, FindKeyCode(button_list, p_profile->rs_down), FindKeyCode(button_list, p_profile->rs_up));

	pad->m_vibrateMotors.emplace_back(true, 0);
	pad->m_vibrateMotors.emplace_back(false, 0);

	bindings.emplace_back(ds3device, pad);

	return true;
}

void ds3_pad_handler::ThreadProc()
{
	for (int i = 0; i < static_cast<int>(bindings.size()); i++)
	{
		m_dev = bindings[i].first;
		auto thepad = bindings[i].second;

		if (m_dev->handle == nullptr)
		{
			hid_device* devhandle = hid_open_path(m_dev->device.c_str());
			if (!devhandle)
			{
				continue;
			}

			m_dev->handle = devhandle;
		}

		switch (get_data(m_dev))
		{
		case DS3Status::NewData:
			process_data(m_dev, thepad);
		case DS3Status::Connected:
			if (m_dev->status == DS3Status::Disconnected)
			{
				m_dev->status = DS3Status::Connected;
				thepad->m_port_status = CELL_PAD_STATUS_CONNECTED | CELL_PAD_STATUS_ASSIGN_CHANGES;
				LOG_WARNING(HLE, "[DS3] Pad was connected");

				connected++;
			}

			if (m_dev->large_motor != thepad->m_vibrateMotors[0].m_value || m_dev->small_motor != thepad->m_vibrateMotors[1].m_value)
			{
				m_dev->large_motor = (u8)thepad->m_vibrateMotors[0].m_value;
				m_dev->small_motor = (u8)thepad->m_vibrateMotors[1].m_value;
				send_output_report(m_dev);
			}

			break;
		case DS3Status::Disconnected:
			if (m_dev->status == DS3Status::Connected)
			{
				m_dev->status = DS3Status::Disconnected;
				thepad->m_port_status = CELL_PAD_STATUS_DISCONNECTED | CELL_PAD_STATUS_ASSIGN_CHANGES;
				hid_close(m_dev->handle);

				m_dev->handle = nullptr;
				LOG_WARNING(HLE, "[DS3] Pad was disconnected");

				connected--;
			}
			break;
		}
	}
}

void ds3_pad_handler::SetPadData(const std::string& padId, u32 largeMotor, u32 smallMotor, s32/* r*/, s32/* g*/, s32 /* b*/)
{
	std::shared_ptr<ds3_device> device = get_device(padId);
	if (device == nullptr || device->handle == nullptr)
		return;

	// Set the device's motor speeds to our requested values 0-255
	device->large_motor = largeMotor;
	device->small_motor = smallMotor;

	int index = 0;
	for (int i = 0; i < MAX_GAMEPADS; i++)
	{
		if (g_cfg_input.player[i]->handler == pad_handler::ds3)
		{
			if (g_cfg_input.player[i]->device.to_string() == padId)
			{
				m_pad_configs[index].load();
				device->config = &m_pad_configs[index];
				break;
			}
			index++;
		}
	}

	// Start/Stop the engines :)
	send_output_report(device);
}

void ds3_pad_handler::GetNextButtonPress(const std::string& padId, const std::function<void(u16, std::string, std::string, int[])>& /*callback*/, const std::function<void(std::string)>& fail_callback, bool get_blacklist, const std::vector<std::string>& /*buttons*/)
{
	if (get_blacklist)
		blacklist.clear();

	std::shared_ptr<ds3_device> device = get_device(padId);
	if (device == nullptr || device->handle == nullptr)
		return fail_callback(padId);

	return;
}

void ds3_pad_handler::send_output_report(const std::shared_ptr<ds3_device>& ds3dev)
{
#ifdef _WIN32
	u8 report_buf[] = {
		0x00,
		0x02, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00
	};

	report_buf[6] = ds3dev->small_motor;
	report_buf[8] = ds3dev->large_motor;
#else
	u8 report_buf[] = {
		0x01,
		0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0xff, 0x27, 0x10, 0x00, 0x32,
		0xff, 0x27, 0x10, 0x00, 0x32,
		0xff, 0x27, 0x10, 0x00, 0x32,
		0xff, 0x27, 0x10, 0x00, 0x32,
		0x00, 0x00, 0x00, 0x00, 0x00
	};
	report_buf[3] = ds3dev->large_motor;
	report_buf[5] = ds3dev->small_motor;
#endif

	hid_write(ds3dev->handle, report_buf, sizeof(report_buf));
}

std::shared_ptr<ds3_pad_handler::ds3_device> ds3_pad_handler::get_device(const std::string& padId)
{
	if (!Init())
		return nullptr;

	size_t pos = padId.find(m_name_string);
	if (pos == std::string::npos)
		return nullptr;

	int pad_number = std::stoi(padId.substr(pos + 9));
	if (pad_number > 0 && pad_number <= controllers.size())
		return controllers[static_cast<size_t>(pad_number) - 1];

	return nullptr;
}

void ds3_pad_handler::init_config(pad_config* cfg, const std::string& name)
{
	// Set this profile's save location
	cfg->cfg_name = name;

	// Set default button mapping
	cfg->ls_left.def = button_list.at(DS3KeyCodes::LSXNeg);
	cfg->ls_down.def = button_list.at(DS3KeyCodes::LSYNeg);
	cfg->ls_right.def = button_list.at(DS3KeyCodes::LSXPos);
	cfg->ls_up.def = button_list.at(DS3KeyCodes::LSYPos);
	cfg->rs_left.def = button_list.at(DS3KeyCodes::RSXNeg);
	cfg->rs_down.def = button_list.at(DS3KeyCodes::RSYNeg);
	cfg->rs_right.def = button_list.at(DS3KeyCodes::RSXPos);
	cfg->rs_up.def = button_list.at(DS3KeyCodes::RSYPos);
	cfg->start.def = button_list.at(DS3KeyCodes::Start);
	cfg->select.def = button_list.at(DS3KeyCodes::Select);
	cfg->ps.def = button_list.at(DS3KeyCodes::PSButton);
	cfg->square.def = button_list.at(DS3KeyCodes::Square);
	cfg->cross.def = button_list.at(DS3KeyCodes::Cross);
	cfg->circle.def = button_list.at(DS3KeyCodes::Circle);
	cfg->triangle.def = button_list.at(DS3KeyCodes::Triangle);
	cfg->left.def = button_list.at(DS3KeyCodes::Left);
	cfg->down.def = button_list.at(DS3KeyCodes::Down);
	cfg->right.def = button_list.at(DS3KeyCodes::Right);
	cfg->up.def = button_list.at(DS3KeyCodes::Up);
	cfg->r1.def = button_list.at(DS3KeyCodes::R1);
	cfg->r2.def = button_list.at(DS3KeyCodes::R2);
	cfg->r3.def = button_list.at(DS3KeyCodes::R3);
	cfg->l1.def = button_list.at(DS3KeyCodes::L1);
	cfg->l2.def = button_list.at(DS3KeyCodes::L2);
	cfg->l3.def = button_list.at(DS3KeyCodes::L3);

	// Set default misc variables
	cfg->lstickdeadzone.def = 0; // between 0 and 255
	cfg->rstickdeadzone.def = 0; // between 0 and 255
	cfg->ltriggerthreshold.def = 0;  // between 0 and 255
	cfg->rtriggerthreshold.def = 0;  // between 0 and 255
	cfg->padsquircling.def = 0;

	// Set color value
	cfg->colorR.def = 0;
	cfg->colorG.def = 0;
	cfg->colorB.def = 0;

	// apply defaults
	cfg->from_default();
}

ds3_pad_handler::DS3Status ds3_pad_handler::get_data(const std::shared_ptr<ds3_device>& ds3dev)
{
	auto& dbuf = ds3dev->buf;

#ifdef _WIN32
	dbuf[0] = 0xF2;
	int result = hid_get_feature_report(ds3dev->handle, dbuf, sizeof(dbuf));
#else
	int result = hid_read(ds3dev->handle, dbuf, sizeof(dbuf));
#endif

	if (result > 0)
	{
#ifdef _WIN32
		if(dbuf[0] == 0xF2)
#else
		if (dbuf[0] == 0x01 && dbuf[1] != 0xFF)
#endif
		{
			return DS3Status::NewData;
		}
		else
		{
			LOG_WARNING(HLE, "[DS3] Unknown packet received:0x%02x", dbuf[0]);
			return DS3Status::Connected;
		}
	}
	else
	{
		if(result == 0)
			return DS3Status::Connected;
	}

	return DS3Status::Disconnected;
}

std::array<std::pair<u16, bool>, ds3_pad_handler::DS3KeyCodes::KeyCodeCount> ds3_pad_handler::get_button_values(const std::shared_ptr<ds3_device>& device)
{
	std::array<std::pair<u16, bool>, DS3KeyCodes::KeyCodeCount> key_buf;
	auto& dbuf = device->buf;

	key_buf[DS3KeyCodes::Up].second    = dbuf[2 + DS3_HID_OFFSET] & 0x10;
	key_buf[DS3KeyCodes::Right].second = dbuf[2 + DS3_HID_OFFSET] & 0x20;
	key_buf[DS3KeyCodes::Down].second  = dbuf[2 + DS3_HID_OFFSET] & 0x40;
	key_buf[DS3KeyCodes::Left].second  = dbuf[2 + DS3_HID_OFFSET] & 0x80;

	key_buf[DS3KeyCodes::Select].second = dbuf[2 + DS3_HID_OFFSET] & 0x01;
	key_buf[DS3KeyCodes::L3].second     = dbuf[2 + DS3_HID_OFFSET] & 0x02;
	key_buf[DS3KeyCodes::R3].second     = dbuf[2 + DS3_HID_OFFSET] & 0x04;
	key_buf[DS3KeyCodes::Start].second  = dbuf[2 + DS3_HID_OFFSET] & 0x08;

	key_buf[DS3KeyCodes::Square].second   = dbuf[3 + DS3_HID_OFFSET] & 0x80;
	key_buf[DS3KeyCodes::Cross].second    = dbuf[3 + DS3_HID_OFFSET] & 0x40;
	key_buf[DS3KeyCodes::Circle].second   = dbuf[3 + DS3_HID_OFFSET] & 0x20;
	key_buf[DS3KeyCodes::Triangle].second = dbuf[3 + DS3_HID_OFFSET] & 0x10;

	key_buf[DS3KeyCodes::R1].second = dbuf[3 + DS3_HID_OFFSET] & 0x08;
	key_buf[DS3KeyCodes::L1].second = dbuf[3 + DS3_HID_OFFSET] & 0x04;
	key_buf[DS3KeyCodes::R2].second = dbuf[3 + DS3_HID_OFFSET] & 0x02;
	key_buf[DS3KeyCodes::L2].second = dbuf[3 + DS3_HID_OFFSET] & 0x01;

	key_buf[DS3KeyCodes::PSButton].second = dbuf[4 + DS3_HID_OFFSET] & 0x01;

	key_buf[DS3KeyCodes::LSXPos].first = dbuf[6 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::LSYPos].first = dbuf[7 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::RSXPos].first = dbuf[8 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::RSYPos].first = dbuf[9 + DS3_HID_OFFSET];

	key_buf[DS3KeyCodes::Up].first    = dbuf[14 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::Right].first = dbuf[15 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::Down].first  = dbuf[16 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::Left].first  = dbuf[17 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::Triangle].first = dbuf[22 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::Circle].first   = dbuf[23 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::Cross].first    = dbuf[24 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::Square].first   = dbuf[25 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::L1].first = dbuf[20 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::R1].first = dbuf[21 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::L2].first = dbuf[18 + DS3_HID_OFFSET];
	key_buf[DS3KeyCodes::R2].first = dbuf[19 + DS3_HID_OFFSET];

	return key_buf;
}

void ds3_pad_handler::process_data(const std::shared_ptr<ds3_device>& ds3dev, const std::shared_ptr<Pad>& pad)
{
	auto ds3_info = get_button_values(ds3dev);

	for (auto & btn : pad->m_buttons)
	{
		btn.m_value = ds3_info[btn.m_keyCode].first;
		btn.m_pressed = ds3_info[btn.m_keyCode].second;
	}

#ifdef _WIN32
	if(ds3dev->buf[2] || ds3dev->buf[3] || ds3dev->buf[4])
		SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
#endif

	// DS3 pad handler is only using the positive values for accuracy sake
	for (int i = 0; i < static_cast<int>(pad->m_sticks.size()); i++)
	{
		// m_keyCodeMax is the mapped key for right or up
		u32 key_max = pad->m_sticks[i].m_keyCodeMax;
		pad->m_sticks[i].m_value = ds3_info[key_max].first;
	}

	// For unknown reasons the sixaxis values seem to be in little endian on linux

#ifdef _WIN32
	// Official Sony Windows DS3 driver seems to do the same modification of this value as the ps3
	pad->m_sensors[0].m_value = *((le_t<u16> *)&ds3dev->buf[41 + DS3_HID_OFFSET]);
#else
	// When getting raw values from the device this adjustement is needed
	pad->m_sensors[0].m_value = 512 - (*((le_t<u16> *)&ds3dev->buf[41]) - 512);
#endif
	pad->m_sensors[1].m_value = *((le_t<u16> *)&ds3dev->buf[45 + DS3_HID_OFFSET]);
	pad->m_sensors[2].m_value = *((le_t<u16> *)&ds3dev->buf[43 + DS3_HID_OFFSET]);
	pad->m_sensors[3].m_value = *((le_t<u16> *)&ds3dev->buf[47 + DS3_HID_OFFSET]);

	// Those are formulas used to adjust sensor values in sys_hid code but I couldn't find all the vars.
	//auto polish_value = [](s32 value, s32 dword_0x0, s32 dword_0x4, s32 dword_0x8, s32 dword_0xC, s32 dword_0x18, s32 dword_0x1C) -> u16
	//{
	//	value -= dword_0xC;
	//	value *= dword_0x4;
	//	value <<= 10;
	//	value /= dword_0x0;
	//	value >>= 10;
	//	value += dword_0x8;
	//	if (value < dword_0x18) return dword_0x18;
	//	if (value > dword_0x1C) return dword_0x1C;
	//	return (u16)value;
	//};

	// dword_0x0 and dword_0xC are unknown
	//pad->m_sensors[0].m_value = polish_value(pad->m_sensors[0].m_value, 226, -226, 512, 512, 0, 1023);
	//pad->m_sensors[1].m_value = polish_value(pad->m_sensors[1].m_value, 226, 226, 512, 512, 0, 1023);
	//pad->m_sensors[2].m_value = polish_value(pad->m_sensors[2].m_value, 113, 113, 512, 512, 0, 1023);
	//pad->m_sensors[3].m_value = polish_value(pad->m_sensors[3].m_value, 1, 1, 512, 512, 0, 1023);
}
