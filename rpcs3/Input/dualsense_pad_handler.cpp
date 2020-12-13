#include "stdafx.h"
#include "dualsense_pad_handler.h"
#include "Emu/Io/pad_config.h"

LOG_CHANNEL(dualsense_log, "DualSense");

namespace
{
	const auto THREAD_SLEEP = 1ms;
	const auto THREAD_SLEEP_INACTIVE = 100ms;

	const u32 DUALSENSE_ACC_RES_PER_G = 8192;
	const u32 DUALSENSE_GYRO_RES_PER_DEG_S = 1024;
	const u32 DUALSENSE_BLUETOOTH_REPORT_SIZE = 78;
	const u32 DUALSENSE_USB_REPORT_SIZE = 48;

	inline u32 read_u32(const void* buf)
	{
		return *reinterpret_cast<const u32*>(buf);
	}
}

dualsense_pad_handler::dualsense_pad_handler()
    : PadHandlerBase(pad_handler::dualsense)
{
	// Unique names for the config files and our pad settings dialog
	button_list =
	{
		{ DualSenseKeyCodes::Triangle, "Triangle" },
		{ DualSenseKeyCodes::Circle,   "Circle" },
		{ DualSenseKeyCodes::Cross,    "Cross" },
		{ DualSenseKeyCodes::Square,   "Square" },
		{ DualSenseKeyCodes::Left,     "Left" },
		{ DualSenseKeyCodes::Right,    "Right" },
		{ DualSenseKeyCodes::Up,       "Up" },
		{ DualSenseKeyCodes::Down,     "Down" },
		{ DualSenseKeyCodes::R1,       "R1" },
		{ DualSenseKeyCodes::R2,       "R2" },
		{ DualSenseKeyCodes::R3,       "R3" },
		{ DualSenseKeyCodes::Options,  "Options" },
		{ DualSenseKeyCodes::Share,    "Share" },
		{ DualSenseKeyCodes::PSButton, "PS Button" },
		{ DualSenseKeyCodes::TouchPad, "Touch Pad" },
		{ DualSenseKeyCodes::L1,       "L1" },
		{ DualSenseKeyCodes::L2,       "L2" },
		{ DualSenseKeyCodes::L3,       "L3" },
		{ DualSenseKeyCodes::LSXNeg,   "LS X-" },
		{ DualSenseKeyCodes::LSXPos,   "LS X+" },
		{ DualSenseKeyCodes::LSYPos,   "LS Y+" },
		{ DualSenseKeyCodes::LSYNeg,   "LS Y-" },
		{ DualSenseKeyCodes::RSXNeg,   "RS X-" },
		{ DualSenseKeyCodes::RSXPos,   "RS X+" },
		{ DualSenseKeyCodes::RSYPos,   "RS Y+" },
		{ DualSenseKeyCodes::RSYNeg,   "RS Y-" }
	};

	init_configs();

	// Define border values
	thumb_max = 255;
	trigger_min = 0;
	trigger_max = 255;
	vibration_min = 0;
	vibration_max = 255;

	// Set capabilities
	b_has_config = true;
	b_has_rumble = true;
	b_has_deadzones = true;
	b_has_led = false;
	b_has_battery = false;

	m_name_string = "DualSense Pad #";
	m_max_devices = CELL_PAD_MAX_PORT_NUM;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold   = thumb_max / 2;
}

void dualsense_pad_handler::CheckAddDevice(hid_device * hidDevice, hid_device_info* hidDevInfo)
{
	std::string serial;
	std::shared_ptr<DualSenseDevice> dualsenseDev = std::make_shared<DualSenseDevice>();
	dualsenseDev->hidDevice = hidDevice;

	std::array<u8, 64> buf{};
	buf[0] = 0x09;

	// This will give us the bluetooth mac address of the device, regardless if we are on wired or bluetooth.
	// So we can't use this to determine if it is a bluetooth device or not.
	// Will also enable enhanced feature reports for bluetooth.
	if (hid_get_feature_report(hidDevice, buf.data(), 64) == 21)
	{
		serial = fmt::format("%x%x%x%x%x%x", buf[6], buf[5], buf[4], buf[3], buf[2], buf[1]);
		dualsenseDev->dataMode = DualSenseDataMode::Enhanced;
	}
	else
	{
		// We're probably on Bluetooth in this case, but for whatever reason the feature report failed.
		// This will give us a less capable fallback.
		dualsenseDev->dataMode = DualSenseDataMode::Simple;
		std::wstring_view wideSerial(hidDevInfo->serial_number);
		for (wchar_t ch : wideSerial)
			serial += static_cast<uchar>(ch);
	}

	if (hid_set_nonblocking(hidDevice, 1) == -1)
	{
		dualsense_log.error("CheckAddDevice: hid_set_nonblocking failed! Reason: %s", hid_error(hidDevice));
		hid_close(hidDevice);
		return;
	}

	dualsenseDev->path = hidDevInfo->path;
	controllers.emplace(serial, dualsenseDev);
}

bool dualsense_pad_handler::Init()
{
	if (is_init)
		return true;

	const int res = hid_init();
	if (res != 0)
		fmt::throw_exception("hidapi-init error.threadproc");

	hid_device_info* devInfo = hid_enumerate(DUALSENSE_VID, DUALSENSE_PID);
	hid_device_info* head    = devInfo;

	while (devInfo)
	{
		if (controllers.size() >= MAX_GAMEPADS)
			break;

		hid_device* dev = hid_open_path(devInfo->path);
		if (dev)
		{
			CheckAddDevice(dev, devInfo);
		}
		else
		{
			dualsense_log.error("hid_open_path failed! Reason: %s", hid_error(dev));
		}
		devInfo = devInfo->next;
	}
	hid_free_enumeration(head);

	if (controllers.empty())
	{
		dualsense_log.warning("No controllers found!");
	}
	else
	{
		dualsense_log.success("Controllers found: %d", controllers.size());
	}

	is_init = true;
	return true;
}

void dualsense_pad_handler::init_config(pad_config* cfg, const std::string& name)
{
	if (!cfg) return;

	// Set this profile's save location
	cfg->cfg_name = name;

	// Set default button mapping
	cfg->ls_left.def  = button_list.at(DualSenseKeyCodes::LSXNeg);
	cfg->ls_down.def  = button_list.at(DualSenseKeyCodes::LSYNeg);
	cfg->ls_right.def = button_list.at(DualSenseKeyCodes::LSXPos);
	cfg->ls_up.def    = button_list.at(DualSenseKeyCodes::LSYPos);
	cfg->rs_left.def  = button_list.at(DualSenseKeyCodes::RSXNeg);
	cfg->rs_down.def  = button_list.at(DualSenseKeyCodes::RSYNeg);
	cfg->rs_right.def = button_list.at(DualSenseKeyCodes::RSXPos);
	cfg->rs_up.def    = button_list.at(DualSenseKeyCodes::RSYPos);
	cfg->start.def    = button_list.at(DualSenseKeyCodes::Options);
	cfg->select.def   = button_list.at(DualSenseKeyCodes::Share);
	cfg->ps.def       = button_list.at(DualSenseKeyCodes::PSButton);
	cfg->square.def   = button_list.at(DualSenseKeyCodes::Square);
	cfg->cross.def    = button_list.at(DualSenseKeyCodes::Cross);
	cfg->circle.def   = button_list.at(DualSenseKeyCodes::Circle);
	cfg->triangle.def = button_list.at(DualSenseKeyCodes::Triangle);
	cfg->left.def     = button_list.at(DualSenseKeyCodes::Left);
	cfg->down.def     = button_list.at(DualSenseKeyCodes::Down);
	cfg->right.def    = button_list.at(DualSenseKeyCodes::Right);
	cfg->up.def       = button_list.at(DualSenseKeyCodes::Up);
	cfg->r1.def       = button_list.at(DualSenseKeyCodes::R1);
	cfg->r2.def       = button_list.at(DualSenseKeyCodes::R2);
	cfg->r3.def       = button_list.at(DualSenseKeyCodes::R3);
	cfg->l1.def       = button_list.at(DualSenseKeyCodes::L1);
	cfg->l2.def       = button_list.at(DualSenseKeyCodes::L2);
	cfg->l3.def       = button_list.at(DualSenseKeyCodes::L3);

	// Set default misc variables
	cfg->lstickdeadzone.def    = 40; // between 0 and 255
	cfg->rstickdeadzone.def    = 40; // between 0 and 255
	cfg->ltriggerthreshold.def = 0;  // between 0 and 255
	cfg->rtriggerthreshold.def = 0;  // between 0 and 255
	cfg->lpadsquircling.def    = 8000;
	cfg->rpadsquircling.def    = 8000;

	// Set default color value
	cfg->colorR.def = 0;
	cfg->colorG.def = 0;
	cfg->colorB.def = 20;

	// Set default LED options
	cfg->led_battery_indicator.def            = false;
	cfg->led_battery_indicator_brightness.def = 10;
	cfg->led_low_battery_blink.def            = true;

	// apply defaults
	cfg->from_default();
}

std::vector<std::string> dualsense_pad_handler::ListDevices()
{
	std::vector<std::string> dualsense_pads_list;

	if (!Init())
		return dualsense_pads_list;

	for (size_t i = 1; i < controllers.size(); ++i)
	{
		dualsense_pads_list.emplace_back(m_name_string + std::to_string(i));
	}

	for (auto& pad : dualsense_pads_list)
	{
		dualsense_log.success("%s", pad);
	}

	return dualsense_pads_list;
}

dualsense_pad_handler::DualSenseDataStatus dualsense_pad_handler::GetRawData(const std::shared_ptr<DualSenseDevice>& device)
{
	if (!device)
		return DualSenseDataStatus::ReadError;

	std::array<u8, 128> buf{};

	const int res = hid_read(device->hidDevice, buf.data(), 128);
	// looks like controller disconnected or read error
		
	if (res == -1)
		return DualSenseDataStatus::ReadError;

	if (res == 0)
		return DualSenseDataStatus::NoNewData;

	u8 offset = 0;
	switch (buf[0])
	{
	case 0x01:
		if (res == DUALSENSE_BLUETOOTH_REPORT_SIZE)
		{
			device->dataMode = DualSenseDataMode::Simple;
			device->btCon = true;
			offset = 1;
		}
		else
		{
			device->dataMode = DualSenseDataMode::Enhanced;
			device->btCon = false;
			offset = 1;	
		}
		break;
	case 0x31:
	{
		device->dataMode = DualSenseDataMode::Enhanced;
		device->btCon = true;
		offset = 2;

		const u8 btHdr = 0xA1;
		const u32 crcHdr = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
		const u32 crcCalc = CRCPP::CRC::Calculate(buf.data(), (DUALSENSE_BLUETOOTH_REPORT_SIZE - 4), crcTable, crcHdr);
		const u32 crcReported = read_u32(&buf[DUALSENSE_BLUETOOTH_REPORT_SIZE - 4]);
		if (crcCalc != crcReported)
		{
			dualsense_log.warning("Data packet CRC check failed, ignoring! Received 0x%x, Expected 0x%x", crcReported, crcCalc);
			return DualSenseDataStatus::NoNewData;
		}
		break;
	}
	default:
		return DualSenseDataStatus::NoNewData;
	}

	memcpy(device->padData.data(), &buf[offset], 64);
	return DualSenseDataStatus::NewData;
}

bool dualsense_pad_handler::get_is_left_trigger(u64 keyCode)
{
	return keyCode == DualSenseKeyCodes::L2;
}

bool dualsense_pad_handler::get_is_right_trigger(u64 keyCode)
{
	return keyCode == DualSenseKeyCodes::R2;
}

bool dualsense_pad_handler::get_is_left_stick(u64 keyCode)
{
	switch (keyCode)
	{
	case DualSenseKeyCodes::LSXNeg:
	case DualSenseKeyCodes::LSXPos:
	case DualSenseKeyCodes::LSYPos:
	case DualSenseKeyCodes::LSYNeg:
		return true;
	default:
		return false;
	}
}

bool dualsense_pad_handler::get_is_right_stick(u64 keyCode)
{
	switch (keyCode)
	{
	case DualSenseKeyCodes::RSXNeg:
	case DualSenseKeyCodes::RSXPos:
	case DualSenseKeyCodes::RSYPos:
	case DualSenseKeyCodes::RSYNeg:
		return true;
	default:
		return false;
	}
}

PadHandlerBase::connection dualsense_pad_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	auto dualsense_dev = std::static_pointer_cast<DualSenseDevice>(device);
	if (!dualsense_dev)
		return connection::disconnected;

	if (dualsense_dev->hidDevice == nullptr)
	{
		// try to reconnect
		hid_device* dev = hid_open_path(dualsense_dev->path.c_str());
		if (dev)
		{
			if (hid_set_nonblocking(dev, 1) == -1)
			{
				dualsense_log.error("Reconnecting Device %s: hid_set_nonblocking failed with error %s", dualsense_dev->path, hid_error(dev));
			}
			dualsense_dev->hidDevice = dev;
		}
		else
		{
			// nope, not there
			return connection::disconnected;
		}
	}

	status = GetRawData(dualsense_dev);
	if (status == DualSenseDataStatus::ReadError)
	{
		// this also can mean disconnected, either way deal with it on next loop and reconnect
		hid_close(dualsense_dev->hidDevice);
		dualsense_dev->hidDevice = nullptr;

		return connection::no_data;
	}

	return connection::connected;
}

std::unordered_map<u64, u16> dualsense_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> keyBuffer;
	auto dualsense_dev = std::static_pointer_cast<DualSenseDevice>(device);
	if (!dualsense_dev)
		return keyBuffer;

	auto buf = dualsense_dev->padData;

	if (dualsense_dev->dataMode == DualSenseDataMode::Simple)
	{
		// Left Stick X Axis
		keyBuffer[DualSenseKeyCodes::LSXNeg] = Clamp0To255((127.5f - buf[0]) * 2.0f);
		keyBuffer[DualSenseKeyCodes::LSXPos] = Clamp0To255((buf[0] - 127.5f) * 2.0f);

		// Left Stick Y Axis (Up is the negative for some reason)
		keyBuffer[DualSenseKeyCodes::LSYNeg] = Clamp0To255((buf[1] - 127.5f) * 2.0f);
		keyBuffer[DualSenseKeyCodes::LSYPos] = Clamp0To255((127.5f - buf[1]) * 2.0f);

		// Right Stick X Axis
		keyBuffer[DualSenseKeyCodes::RSXNeg] = Clamp0To255((127.5f - buf[2]) * 2.0f);
		keyBuffer[DualSenseKeyCodes::RSXPos] = Clamp0To255((buf[2] - 127.5f) * 2.0f);

		// Right Stick Y Axis (Up is the negative for some reason)
		keyBuffer[DualSenseKeyCodes::RSYNeg] = Clamp0To255((buf[3] - 127.5f) * 2.0f);
		keyBuffer[DualSenseKeyCodes::RSYPos] = Clamp0To255((127.5f - buf[3]) * 2.0f);

		// bleh, dpad in buffer is stored in a different state
		u8 data = buf[4] & 0xf;
		switch (data)
		{
		case 0x08: // none pressed
			keyBuffer[DualSenseKeyCodes::Up]    = 0;
			keyBuffer[DualSenseKeyCodes::Down]  = 0;
			keyBuffer[DualSenseKeyCodes::Left]  = 0;
			keyBuffer[DualSenseKeyCodes::Right] = 0;
			break;
		case 0x07: // NW...left and up
			keyBuffer[DualSenseKeyCodes::Up]    = 255;
			keyBuffer[DualSenseKeyCodes::Down]  = 0;
			keyBuffer[DualSenseKeyCodes::Left]  = 255;
			keyBuffer[DualSenseKeyCodes::Right] = 0;
			break;
		case 0x06: // W..left
			keyBuffer[DualSenseKeyCodes::Up]    = 0;
			keyBuffer[DualSenseKeyCodes::Down]  = 0;
			keyBuffer[DualSenseKeyCodes::Left]  = 255;
			keyBuffer[DualSenseKeyCodes::Right] = 0;
			break;
		case 0x05: // SW..left down
			keyBuffer[DualSenseKeyCodes::Up]    = 0;
			keyBuffer[DualSenseKeyCodes::Down]  = 255;
			keyBuffer[DualSenseKeyCodes::Left]  = 255;
			keyBuffer[DualSenseKeyCodes::Right] = 0;
			break;
		case 0x04: // S..down
			keyBuffer[DualSenseKeyCodes::Up]    = 0;
			keyBuffer[DualSenseKeyCodes::Down]  = 255;
			keyBuffer[DualSenseKeyCodes::Left]  = 0;
			keyBuffer[DualSenseKeyCodes::Right] = 0;
			break;
		case 0x03: // SE..down and right
			keyBuffer[DualSenseKeyCodes::Up]    = 0;
			keyBuffer[DualSenseKeyCodes::Down]  = 255;
			keyBuffer[DualSenseKeyCodes::Left]  = 0;
			keyBuffer[DualSenseKeyCodes::Right] = 255;
			break;
		case 0x02: // E... right
			keyBuffer[DualSenseKeyCodes::Up]    = 0;
			keyBuffer[DualSenseKeyCodes::Down]  = 0;
			keyBuffer[DualSenseKeyCodes::Left]  = 0;
			keyBuffer[DualSenseKeyCodes::Right] = 255;
			break;
		case 0x01: // NE.. up right
			keyBuffer[DualSenseKeyCodes::Up]    = 255;
			keyBuffer[DualSenseKeyCodes::Down]  = 0;
			keyBuffer[DualSenseKeyCodes::Left]  = 0;
			keyBuffer[DualSenseKeyCodes::Right] = 255;
			break;
		case 0x00: // n.. up
			keyBuffer[DualSenseKeyCodes::Up]    = 255;
			keyBuffer[DualSenseKeyCodes::Down]  = 0;
			keyBuffer[DualSenseKeyCodes::Left]  = 0;
			keyBuffer[DualSenseKeyCodes::Right] = 0;
			break;
		default:
			fmt::throw_exception("dualsense dpad state encountered unexpected input");
		}

		data = buf[4] >> 4;
		keyBuffer[DualSenseKeyCodes::Square]   = ((data & 0x01) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::Cross]    = ((data & 0x02) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::Circle]   = ((data & 0x04) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::Triangle] = ((data & 0x08) != 0) ? 255 : 0;

		data = buf[5];
		keyBuffer[DualSenseKeyCodes::L1]      = ((data & 0x01) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::R1]      = ((data & 0x02) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::Share]   = ((data & 0x10) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::Options] = ((data & 0x20) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::L3]      = ((data & 0x40) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::R3]      = ((data & 0x80) != 0) ? 255 : 0;

		keyBuffer[DualSenseKeyCodes::L2] = buf[7];
		keyBuffer[DualSenseKeyCodes::R2] = buf[8];

		data = buf[6] & 0x03;
		keyBuffer[DualSenseKeyCodes::PSButton] = ((data & 0x01) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::TouchPad] = ((data & 0x02) != 0) ? 255 : 0; 

		return keyBuffer;
	}

	// Left Stick X Axis
	keyBuffer[DualSenseKeyCodes::LSXNeg] = Clamp0To255((127.5f - buf[0]) * 2.0f);
	keyBuffer[DualSenseKeyCodes::LSXPos] = Clamp0To255((buf[0] - 127.5f) * 2.0f);

	// Left Stick Y Axis (Up is the negative for some reason)
	keyBuffer[DualSenseKeyCodes::LSYNeg] = Clamp0To255((buf[1] - 127.5f) * 2.0f);
	keyBuffer[DualSenseKeyCodes::LSYPos] = Clamp0To255((127.5f - buf[1]) * 2.0f);

	// Right Stick X Axis
	keyBuffer[DualSenseKeyCodes::RSXNeg] = Clamp0To255((127.5f - buf[2]) * 2.0f);
	keyBuffer[DualSenseKeyCodes::RSXPos] = Clamp0To255((buf[2] - 127.5f) * 2.0f);

	// Right Stick Y Axis (Up is the negative for some reason)
	keyBuffer[DualSenseKeyCodes::RSYNeg] = Clamp0To255((buf[3] - 127.5f) * 2.0f);
	keyBuffer[DualSenseKeyCodes::RSYPos] = Clamp0To255((127.5f - buf[3]) * 2.0f);

	u8 data = buf[7] & 0xf;
	switch (data)
	{
	case 0x08: // none pressed
		keyBuffer[DualSenseKeyCodes::Up]    = 0;
		keyBuffer[DualSenseKeyCodes::Down]  = 0;
		keyBuffer[DualSenseKeyCodes::Left]  = 0;
		keyBuffer[DualSenseKeyCodes::Right] = 0;
		break;
	case 0x07: // NW...left and up
		keyBuffer[DualSenseKeyCodes::Up]    = 255;
		keyBuffer[DualSenseKeyCodes::Down]  = 0;
		keyBuffer[DualSenseKeyCodes::Left]  = 255;
		keyBuffer[DualSenseKeyCodes::Right] = 0;
		break;
	case 0x06: // W..left
		keyBuffer[DualSenseKeyCodes::Up]    = 0;
		keyBuffer[DualSenseKeyCodes::Down]  = 0;
		keyBuffer[DualSenseKeyCodes::Left]  = 255;
		keyBuffer[DualSenseKeyCodes::Right] = 0;
		break;
	case 0x05: // SW..left down
		keyBuffer[DualSenseKeyCodes::Up]    = 0;
		keyBuffer[DualSenseKeyCodes::Down]  = 255;
		keyBuffer[DualSenseKeyCodes::Left]  = 255;
		keyBuffer[DualSenseKeyCodes::Right] = 0;
		break;
	case 0x04: // S..down
		keyBuffer[DualSenseKeyCodes::Up]    = 0;
		keyBuffer[DualSenseKeyCodes::Down]  = 255;
		keyBuffer[DualSenseKeyCodes::Left]  = 0;
		keyBuffer[DualSenseKeyCodes::Right] = 0;
		break;
	case 0x03: // SE..down and right
		keyBuffer[DualSenseKeyCodes::Up]    = 0;
		keyBuffer[DualSenseKeyCodes::Down]  = 255;
		keyBuffer[DualSenseKeyCodes::Left]  = 0;
		keyBuffer[DualSenseKeyCodes::Right] = 255;
		break;
	case 0x02: // E... right
		keyBuffer[DualSenseKeyCodes::Up]    = 0;
		keyBuffer[DualSenseKeyCodes::Down]  = 0;
		keyBuffer[DualSenseKeyCodes::Left]  = 0;
		keyBuffer[DualSenseKeyCodes::Right] = 255;
		break;
	case 0x01: // NE.. up right
		keyBuffer[DualSenseKeyCodes::Up]    = 255;
		keyBuffer[DualSenseKeyCodes::Down]  = 0;
		keyBuffer[DualSenseKeyCodes::Left]  = 0;
		keyBuffer[DualSenseKeyCodes::Right] = 255;
		break;
	case 0x00: // n.. up
		keyBuffer[DualSenseKeyCodes::Up]    = 255;
		keyBuffer[DualSenseKeyCodes::Down]  = 0;
		keyBuffer[DualSenseKeyCodes::Left]  = 0;
		keyBuffer[DualSenseKeyCodes::Right] = 0;
		break;
	default:
		fmt::throw_exception("dualsense dpad state encountered unexpected input");
	}

	data = buf[7] >> 4;
	keyBuffer[DualSenseKeyCodes::Square]   = ((data & 0x01) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::Cross]    = ((data & 0x02) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::Circle]   = ((data & 0x04) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::Triangle] = ((data & 0x08) != 0) ? 255 : 0;

	data = buf[8];
	keyBuffer[DualSenseKeyCodes::L1]      = ((data & 0x01) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::R1]      = ((data & 0x02) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::Share]   = ((data & 0x10) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::Options] = ((data & 0x20) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::L3]      = ((data & 0x40) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::R3]      = ((data & 0x80) != 0) ? 255 : 0;

	keyBuffer[DualSenseKeyCodes::L2] = buf[4];
	keyBuffer[DualSenseKeyCodes::R2] = buf[5];

	data = buf[9] & 0x03;
	keyBuffer[DualSenseKeyCodes::PSButton] = ((data & 0x01) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::TouchPad] = ((data & 0x02) != 0) ? 255 : 0; 

	return keyBuffer;
}

pad_preview_values dualsense_pad_handler::get_preview_values(std::unordered_map<u64, u16> data)
{
	return {data[L2], data[R2], data[LSXPos] - data[LSXNeg], data[LSYPos] - data[LSYNeg], data[RSXPos] - data[RSXNeg], data[RSYPos] - data[RSYNeg]};
}

std::shared_ptr<dualsense_pad_handler::DualSenseDevice> dualsense_pad_handler::GetDualSenseDevice(const std::string& padId)
{
	if (!Init())
		return nullptr;

	size_t pos = padId.find(m_name_string);
	if (pos == umax)
		return nullptr;

	std::string pad_serial = padId.substr(pos + 15);
	std::shared_ptr<DualSenseDevice> device = nullptr;

	int i = 0; // Controllers 1-n in GUI
	for (auto& cur_control : controllers)
	{
		if (pad_serial == std::to_string(++i) || pad_serial == cur_control.first)
		{
			device = cur_control.second;
			break;
		}
	}

	return device;
}

std::shared_ptr<PadDevice> dualsense_pad_handler::get_device(const std::string& device)
{
	std::shared_ptr<DualSenseDevice> dualsense_dev = GetDualSenseDevice(device);
	if (dualsense_dev == nullptr || dualsense_dev->hidDevice == nullptr)
		return nullptr;

	return dualsense_dev;
}

dualsense_pad_handler::~dualsense_pad_handler()
{
	for (auto& controller : controllers)
	{
		if (controller.second->hidDevice)
		{
			// Disable vibration
			controller.second->smallVibrate = 0;
			controller.second->largeVibrate = 0;
			SendVibrateData(controller.second);

			hid_close(controller.second->hidDevice);
		}
	}
	if (hid_exit() != 0)
	{
		dualsense_log.error("hid_exit failed!");
	}
}

int dualsense_pad_handler::SendVibrateData(const std::shared_ptr<DualSenseDevice>& device)
{
	if (!device)
		return -2;

	auto p_profile = device->config;
	if (p_profile == nullptr)
		return -2; // hid_write and hid_write_control return -1 on error

	if (device->btCon)
	{
		std::array<u8, DUALSENSE_BLUETOOTH_REPORT_SIZE> outputBuf{};
		outputBuf[0] = 0x31;
		outputBuf[1] = 0x02;
		outputBuf[2] |= 0x03;

		outputBuf[4] = device->smallVibrate;
		outputBuf[5] = device->largeVibrate;

		const u8 btHdr    = 0xA2;
		const u32 crcHdr  = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
		const u32 crcCalc = CRCPP::CRC::Calculate(outputBuf.data(), (DUALSENSE_BLUETOOTH_REPORT_SIZE - 4), crcTable, crcHdr);

		outputBuf[74] = (crcCalc >> 0) & 0xFF;
		outputBuf[75] = (crcCalc >> 8) & 0xFF;
		outputBuf[76] = (crcCalc >> 16) & 0xFF;
		outputBuf[77] = (crcCalc >> 24) & 0xFF;

		return hid_write(device->hidDevice, outputBuf.data(), DUALSENSE_BLUETOOTH_REPORT_SIZE);
	}
	else
	{
		std::array<u8, DUALSENSE_USB_REPORT_SIZE> outputBuf{};
		outputBuf[0] = 0x02;
		outputBuf[1] |= 0x03;

		outputBuf[3] = device->smallVibrate;
		outputBuf[4] = device->largeVibrate;

		return hid_write(device->hidDevice, outputBuf.data(), DUALSENSE_USB_REPORT_SIZE);
	}
}

void dualsense_pad_handler::apply_pad_data(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	auto dualsense_dev = std::static_pointer_cast<DualSenseDevice>(device);
	if (!dualsense_dev || !pad)
		return;

	auto profile = dualsense_dev->config;

	// Attempt to send rumble no matter what
	int idx_l = profile->switch_vibration_motors ? 1 : 0;
	int idx_s = profile->switch_vibration_motors ? 0 : 1;

	int speed_large = profile->enable_vibration_motor_large ? pad->m_vibrateMotors[idx_l].m_value : vibration_min;
	int speed_small = profile->enable_vibration_motor_small ? pad->m_vibrateMotors[idx_s].m_value : vibration_min;

	dualsense_dev->newVibrateData |= dualsense_dev->largeVibrate != speed_large || dualsense_dev->smallVibrate != speed_small;

	dualsense_dev->largeVibrate = speed_large;
	dualsense_dev->smallVibrate = speed_small;

	if (dualsense_dev->newVibrateData)
	{
		if (SendVibrateData(dualsense_dev) >= 0)
		{
			dualsense_dev->newVibrateData = false;
		}
	}
}
