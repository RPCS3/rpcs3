#include "stdafx.h"
#include "ds3_pad_handler.h"
#include "Emu/Io/pad_config.h"

LOG_CHANNEL(ds3_log, "DS3");

ds3_pad_handler::ds3_pad_handler() : PadHandlerBase(pad_handler::ds3)
{
	button_list =
	{
		{ DS3KeyCodes::Triangle, "Triangle" },
		{ DS3KeyCodes::Circle,   "Circle" },
		{ DS3KeyCodes::Cross,    "Cross" },
		{ DS3KeyCodes::Square,   "Square" },
		{ DS3KeyCodes::Left,     "Left" },
		{ DS3KeyCodes::Right,    "Right" },
		{ DS3KeyCodes::Up,       "Up" },
		{ DS3KeyCodes::Down,     "Down" },
		{ DS3KeyCodes::R1,       "R1" },
		{ DS3KeyCodes::R2,       "R2" },
		{ DS3KeyCodes::R3,       "R3" },
		{ DS3KeyCodes::Start,    "Start" },
		{ DS3KeyCodes::Select,   "Select" },
		{ DS3KeyCodes::PSButton, "PS Button" },
		{ DS3KeyCodes::L1,       "L1" },
		{ DS3KeyCodes::L2,       "L2" },
		{ DS3KeyCodes::L3,       "L3" },
		{ DS3KeyCodes::LSXNeg,   "LS X-" },
		{ DS3KeyCodes::LSXPos,   "LS X+" },
		{ DS3KeyCodes::LSYPos,   "LS Y+" },
		{ DS3KeyCodes::LSYNeg,   "LS Y-" },
		{ DS3KeyCodes::RSXNeg,   "RS X-" },
		{ DS3KeyCodes::RSXPos,   "RS X+" },
		{ DS3KeyCodes::RSYPos,   "RS Y+" },
		{ DS3KeyCodes::RSYNeg,   "RS Y-" }
	};

	init_configs();

	// set capabilities
	b_has_config = true;
	b_has_rumble = true;
	b_has_deadzones = true;

	m_name_string = "DS3 Pad #";
	m_max_devices = CELL_PAD_MAX_PORT_NUM;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
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
		ds3_log.fatal("Failed to init hidapi for the DS3 pad handler");
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
		ds3_log.error("One or more DS3 pads were detected but couldn't be interacted with directly");
#if defined(_WIN32) || defined(__linux__)
		ds3_log.error("Check https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration for intructions on how to solve this issue");
#endif
	}
	else if (controllers.empty())
	{
		ds3_log.warning("No controllers found!");
	}
	else
	{
		ds3_log.success("Controllers found: %d", controllers.size());
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

void ds3_pad_handler::SetPadData(const std::string& padId, u32 largeMotor, u32 smallMotor, s32/* r*/, s32/* g*/, s32 /* b*/, bool /*battery_led*/, u32 /*battery_led_brightness*/)
{
	std::shared_ptr<ds3_device> device = get_ds3_device(padId);
	if (device == nullptr || device->handle == nullptr)
		return;

	// Set the device's motor speeds to our requested values 0-255
	device->large_motor = largeMotor;
	device->small_motor = smallMotor;

	int index = 0;
	for (uint i = 0; i < MAX_GAMEPADS; i++)
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

void ds3_pad_handler::send_output_report(const std::shared_ptr<ds3_device>& ds3dev)
{
	if (!ds3dev)
		return;

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

std::shared_ptr<ds3_pad_handler::ds3_device> ds3_pad_handler::get_ds3_device(const std::string& padId)
{
	if (!Init())
		return nullptr;

	size_t pos = padId.find(m_name_string);
	if (pos == umax)
		return nullptr;

	int pad_number = std::stoi(padId.substr(pos + 9));
	if (pad_number > 0 && pad_number + 0u <= controllers.size())
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
	cfg->lstickdeadzone.def = 40; // between 0 and 255
	cfg->rstickdeadzone.def = 40; // between 0 and 255
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
	if (!ds3dev)
		return DS3Status::Disconnected;

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
			ds3_log.warning("Unknown packet received:0x%02x", dbuf[0]);
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

std::unordered_map<u64, u16> ds3_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> key_buf;
	auto dev = std::static_pointer_cast<ds3_device>(device);
	if (!dev)
		return key_buf;

	auto& dbuf = dev->buf;

	const u8 lsx = dbuf[6 + DS3_HID_OFFSET];
	const u8 lsy = dbuf[7 + DS3_HID_OFFSET];
	const u8 rsx = dbuf[8 + DS3_HID_OFFSET];
	const u8 rsy = dbuf[9 + DS3_HID_OFFSET];

	// Left Stick X Axis
	key_buf[DS3KeyCodes::LSXNeg] = Clamp0To255((127.5f - lsx) * 2.0f);
	key_buf[DS3KeyCodes::LSXPos] = Clamp0To255((lsx - 127.5f) * 2.0f);

	// Left Stick Y Axis (Up is the negative for some reason)
	key_buf[DS3KeyCodes::LSYNeg] = Clamp0To255((lsy - 127.5f) * 2.0f);
	key_buf[DS3KeyCodes::LSYPos] = Clamp0To255((127.5f - lsy) * 2.0f);

	// Right Stick X Axis
	key_buf[DS3KeyCodes::RSXNeg] = Clamp0To255((127.5f - rsx) * 2.0f);
	key_buf[DS3KeyCodes::RSXPos] = Clamp0To255((rsx - 127.5f) * 2.0f);

	// Right Stick Y Axis (Up is the negative for some reason)
	key_buf[DS3KeyCodes::RSYNeg] = Clamp0To255((rsy - 127.5f) * 2.0f);
	key_buf[DS3KeyCodes::RSYPos] = Clamp0To255((127.5f - rsy) * 2.0f);

	// Buttons or triggers with pressure sensitivity
	key_buf[DS3KeyCodes::Up]       = (dbuf[2 + DS3_HID_OFFSET] & 0x10) ? dbuf[14 + DS3_HID_OFFSET] : 0;
	key_buf[DS3KeyCodes::Right]    = (dbuf[2 + DS3_HID_OFFSET] & 0x20) ? dbuf[15 + DS3_HID_OFFSET] : 0;
	key_buf[DS3KeyCodes::Down]     = (dbuf[2 + DS3_HID_OFFSET] & 0x40) ? dbuf[16 + DS3_HID_OFFSET] : 0;
	key_buf[DS3KeyCodes::Left]     = (dbuf[2 + DS3_HID_OFFSET] & 0x80) ? dbuf[17 + DS3_HID_OFFSET] : 0;
	key_buf[DS3KeyCodes::L2]       = (dbuf[3 + DS3_HID_OFFSET] & 0x01) ? dbuf[18 + DS3_HID_OFFSET] : 0;
	key_buf[DS3KeyCodes::R2]       = (dbuf[3 + DS3_HID_OFFSET] & 0x02) ? dbuf[19 + DS3_HID_OFFSET] : 0;
	key_buf[DS3KeyCodes::L1]       = (dbuf[3 + DS3_HID_OFFSET] & 0x04) ? dbuf[20 + DS3_HID_OFFSET] : 0;
	key_buf[DS3KeyCodes::R1]       = (dbuf[3 + DS3_HID_OFFSET] & 0x08) ? dbuf[21 + DS3_HID_OFFSET] : 0;
	key_buf[DS3KeyCodes::Triangle] = (dbuf[3 + DS3_HID_OFFSET] & 0x10) ? dbuf[22 + DS3_HID_OFFSET] : 0;
	key_buf[DS3KeyCodes::Circle]   = (dbuf[3 + DS3_HID_OFFSET] & 0x20) ? dbuf[23 + DS3_HID_OFFSET] : 0;
	key_buf[DS3KeyCodes::Cross]    = (dbuf[3 + DS3_HID_OFFSET] & 0x40) ? dbuf[24 + DS3_HID_OFFSET] : 0;
	key_buf[DS3KeyCodes::Square]   = (dbuf[3 + DS3_HID_OFFSET] & 0x80) ? dbuf[25 + DS3_HID_OFFSET] : 0;

	// Buttons without pressure sensitivity
	key_buf[DS3KeyCodes::Select]   = (dbuf[2 + DS3_HID_OFFSET] & 0x01) ? 255 : 0;
	key_buf[DS3KeyCodes::L3]       = (dbuf[2 + DS3_HID_OFFSET] & 0x02) ? 255 : 0;
	key_buf[DS3KeyCodes::R3]       = (dbuf[2 + DS3_HID_OFFSET] & 0x04) ? 255 : 0;
	key_buf[DS3KeyCodes::Start]    = (dbuf[2 + DS3_HID_OFFSET] & 0x08) ? 255 : 0;
	key_buf[DS3KeyCodes::PSButton] = (dbuf[4 + DS3_HID_OFFSET] & 0x01) ? 255 : 0;

	return key_buf;
}

pad_preview_values ds3_pad_handler::get_preview_values(std::unordered_map<u64, u16> data)
{
	return { data[L2], data[R2], data[LSXPos] - data[LSXNeg], data[LSYPos] - data[LSYNeg], data[RSXPos] - data[RSXNeg], data[RSYPos] - data[RSYNeg] };
}

void ds3_pad_handler::get_extended_info(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	auto ds3dev = std::static_pointer_cast<ds3_device>(device);
	if (!ds3dev || !pad)
		return;

	// For unknown reasons the sixaxis values seem to be in little endian on linux

#ifdef _WIN32
	// Official Sony Windows DS3 driver seems to do the same modification of this value as the ps3
	pad->m_sensors[0].m_value = *reinterpret_cast<le_t<u16, 1>*>(&ds3dev->buf[41 + DS3_HID_OFFSET]);
#else
	// When getting raw values from the device this adjustement is needed
	pad->m_sensors[0].m_value = 512 - (*reinterpret_cast<le_t<u16, 1>*>(&ds3dev->buf[41]) - 512);
#endif
	pad->m_sensors[1].m_value = *reinterpret_cast<le_t<u16, 1>*>(&ds3dev->buf[45 + DS3_HID_OFFSET]);
	pad->m_sensors[2].m_value = *reinterpret_cast<le_t<u16, 1>*>(&ds3dev->buf[43 + DS3_HID_OFFSET]);
	pad->m_sensors[3].m_value = *reinterpret_cast<le_t<u16, 1>*>(&ds3dev->buf[47 + DS3_HID_OFFSET]);

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
	//	return static_cast<u16>(value);
	//};

	// dword_0x0 and dword_0xC are unknown
	//pad->m_sensors[0].m_value = polish_value(pad->m_sensors[0].m_value, 226, -226, 512, 512, 0, 1023);
	//pad->m_sensors[1].m_value = polish_value(pad->m_sensors[1].m_value, 226, 226, 512, 512, 0, 1023);
	//pad->m_sensors[2].m_value = polish_value(pad->m_sensors[2].m_value, 113, 113, 512, 512, 0, 1023);
	//pad->m_sensors[3].m_value = polish_value(pad->m_sensors[3].m_value, 1, 1, 512, 512, 0, 1023);
}

std::shared_ptr<PadDevice> ds3_pad_handler::get_device(const std::string& device)
{
	std::shared_ptr<ds3_device> ds3device = get_ds3_device(device);
	if (ds3device == nullptr || ds3device->handle == nullptr)
		return nullptr;

	return ds3device;
}

bool ds3_pad_handler::get_is_left_trigger(u64 keyCode)
{
	return keyCode == DS3KeyCodes::L2;
}

bool ds3_pad_handler::get_is_right_trigger(u64 keyCode)
{
	return keyCode == DS3KeyCodes::R2;
}

bool ds3_pad_handler::get_is_left_stick(u64 keyCode)
{
	switch (keyCode)
	{
	case DS3KeyCodes::LSXNeg:
	case DS3KeyCodes::LSXPos:
	case DS3KeyCodes::LSYPos:
	case DS3KeyCodes::LSYNeg:
		return true;
	default:
		return false;
	}
}

bool ds3_pad_handler::get_is_right_stick(u64 keyCode)
{
	switch (keyCode)
	{
	case DS3KeyCodes::RSXNeg:
	case DS3KeyCodes::RSXPos:
	case DS3KeyCodes::RSYPos:
	case DS3KeyCodes::RSYNeg:
		return true;
	default:
		return false;
	}
}

PadHandlerBase::connection ds3_pad_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	auto dev = std::static_pointer_cast<ds3_device>(device);
	if (!dev)
		return connection::disconnected;

	if (dev->handle == nullptr)
	{
		hid_device* devhandle = hid_open_path(dev->device.c_str());
		if (!devhandle)
		{
			return connection::disconnected;
		}

		dev->handle = devhandle;
	}

	switch (get_data(dev))
	{
	case DS3Status::Disconnected:
	{
		if (dev->status == DS3Status::Connected)
		{
			dev->status = DS3Status::Disconnected;
			hid_close(dev->handle);
			dev->handle = nullptr;
		}
		return connection::disconnected;
	}
	case DS3Status::Connected:
	{
		if (dev->status == DS3Status::Disconnected)
		{
			dev->status = DS3Status::Connected;
		}
		return connection::no_data;
	}
	case DS3Status::NewData:
	{
		return connection::connected;
	}
	default:
		break;
	}

	return connection::disconnected;
}

void ds3_pad_handler::apply_pad_data(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	auto dev = std::static_pointer_cast<ds3_device>(device);
	if (!dev || !pad)
		return;

	if (dev->large_motor != pad->m_vibrateMotors[0].m_value || dev->small_motor != pad->m_vibrateMotors[1].m_value)
	{
		dev->large_motor = static_cast<u8>(pad->m_vibrateMotors[0].m_value);
		dev->small_motor = static_cast<u8>(pad->m_vibrateMotors[1].m_value);
		send_output_report(dev);
	}
}
