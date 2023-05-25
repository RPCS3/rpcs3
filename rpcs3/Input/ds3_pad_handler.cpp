#include "stdafx.h"
#include "ds3_pad_handler.h"
#include "Emu/Io/pad_config.h"

#include "util/asm.hpp"

LOG_CHANNEL(ds3_log, "DS3");

struct ds3_rumble
{
	u8 padding              = 0x00;
	u8 small_motor_duration = 0xFF; // 0xff means forever
	u8 small_motor_on       = 0x00; // 0 or 1 (off/on)
	u8 large_motor_duration = 0xFF; // 0xff means forever
	u8 large_motor_force    = 0x00; // 0 to 255
};

struct ds3_led
{
	u8 duration             = 0xFF; // total duration, 0xff means forever
	u8 interval_duration    = 0xFF; // interval duration in deciseconds
	u8 enabled              = 0x10;
	u8 interval_portion_off = 0x00; // in percent (100% = 0xFF)
	u8 interval_portion_on  = 0xFF; // in percent (100% = 0xFF)
};

struct ds3_output_report
{
#ifdef _WIN32
	u8 report_id = 0x00;
	u8 idk_what_this_is[3] = {0x02, 0x00, 0x00};
#else
	u8 report_id = 0x01;
#endif
	ds3_rumble rumble;
	u8 padding[4]  = {0x00, 0x00, 0x00, 0x00};
	u8 led_enabled = 0x00; // LED 1 = 0x02, LED 2 = 0x04, etc.
	ds3_led led[4];
	ds3_led led_5;         // reserved for another LED
};

constexpr std::array<u8, 6> battery_capacity = {0, 1, 25, 50, 75, 100};

constexpr id_pair SONY_DS3_ID_0 = {0x054C, 0x0268};

ds3_pad_handler::ds3_pad_handler()
    : hid_pad_handler(pad_handler::ds3, {SONY_DS3_ID_0})
{
	button_list =
	{
		{ DS3KeyCodes::None,     "" },
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
	b_has_motion = true;
	b_has_deadzones = true;
	b_has_battery = true;
	b_has_led = true;
	b_has_rgb = false;
	b_has_player_led = true;
	b_has_pressure_intensity_button = false; // The DS3 obviously already has this feature natively.

	m_name_string = "DS3 Pad #";
	m_max_devices = CELL_PAD_MAX_PORT_NUM;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
}

ds3_pad_handler::~ds3_pad_handler()
{
	for (auto& controller : m_controllers)
	{
		if (controller.second && controller.second->hidDevice)
		{
			// Disable blinking and vibration
			controller.second->large_motor = 0;
			controller.second->small_motor = 0;
			send_output_report(controller.second.get());
		}
	}
}

u32 ds3_pad_handler::get_battery_level(const std::string& padId)
{
	const std::shared_ptr<ds3_device> device = get_hid_device(padId);
	if (!device || !device->hidDevice)
	{
		return 0;
	}
	return std::clamp<u32>(device->battery_level, 0, 100);
}

void ds3_pad_handler::SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32/* r*/, s32/* g*/, s32 /* b*/, bool player_led, bool /*battery_led*/, u32 /*battery_led_brightness*/)
{
	std::shared_ptr<ds3_device> device = get_hid_device(padId);
	if (device == nullptr || device->hidDevice == nullptr)
		return;

	// Set the device's motor speeds to our requested values 0-255
	device->large_motor = large_motor;
	device->small_motor = small_motor;
	device->player_id = player_id;
	device->config = get_config(padId);

	ensure(device->config);
	device->config->player_led_enabled.set(player_led);

	// Start/Stop the engines :)
	send_output_report(device.get());
}

int ds3_pad_handler::send_output_report(ds3_device* ds3dev)
{
	if (!ds3dev || !ds3dev->hidDevice || !ds3dev->config)
		return -2;

	ds3_output_report output_report;
	output_report.rumble.small_motor_on    = ds3dev->small_motor;
	output_report.rumble.large_motor_force = ds3dev->large_motor;

	if (ds3dev->config->led_battery_indicator)
	{
		if (ds3dev->battery_level >= 75)
			output_report.led_enabled = 0b00011110;
		else if (ds3dev->battery_level >= 50)
			output_report.led_enabled = 0b00001110;
		else if (ds3dev->battery_level >= 25)
			output_report.led_enabled = 0b00000110;
		else
			output_report.led_enabled = 0b00000010;
	}
	else if (ds3dev->config->player_led_enabled)
	{
		switch (ds3dev->player_id)
		{
		case 0: output_report.led_enabled = 0b00000010; break;
		case 1: output_report.led_enabled = 0b00000100; break;
		case 2: output_report.led_enabled = 0b00001000; break;
		case 3: output_report.led_enabled = 0b00010000; break;
		case 4: output_report.led_enabled = 0b00010010; break;
		case 5: output_report.led_enabled = 0b00010100; break;
		case 6: output_report.led_enabled = 0b00011000; break;
		default:
			fmt::throw_exception("DS3 is using forbidden player id %d", ds3dev->player_id);
		}
	}
	else
	{
		output_report.led_enabled = 0;
	}

	if (ds3dev->config->led_low_battery_blink && ds3dev->battery_level < 25)
	{
		output_report.led[3].interval_duration    = 0x14; // 2 seconds
		output_report.led[3].interval_portion_on  = ds3dev->led_delay_on;
		output_report.led[3].interval_portion_off = ds3dev->led_delay_off;
	}

	return hid_write(ds3dev->hidDevice, &output_report.report_id, sizeof(output_report));
}

void ds3_pad_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
	cfg->ls_left.def = ::at32(button_list, DS3KeyCodes::LSXNeg);
	cfg->ls_down.def = ::at32(button_list, DS3KeyCodes::LSYNeg);
	cfg->ls_right.def = ::at32(button_list, DS3KeyCodes::LSXPos);
	cfg->ls_up.def = ::at32(button_list, DS3KeyCodes::LSYPos);
	cfg->rs_left.def = ::at32(button_list, DS3KeyCodes::RSXNeg);
	cfg->rs_down.def = ::at32(button_list, DS3KeyCodes::RSYNeg);
	cfg->rs_right.def = ::at32(button_list, DS3KeyCodes::RSXPos);
	cfg->rs_up.def = ::at32(button_list, DS3KeyCodes::RSYPos);
	cfg->start.def = ::at32(button_list, DS3KeyCodes::Start);
	cfg->select.def = ::at32(button_list, DS3KeyCodes::Select);
	cfg->ps.def = ::at32(button_list, DS3KeyCodes::PSButton);
	cfg->square.def = ::at32(button_list, DS3KeyCodes::Square);
	cfg->cross.def = ::at32(button_list, DS3KeyCodes::Cross);
	cfg->circle.def = ::at32(button_list, DS3KeyCodes::Circle);
	cfg->triangle.def = ::at32(button_list, DS3KeyCodes::Triangle);
	cfg->left.def = ::at32(button_list, DS3KeyCodes::Left);
	cfg->down.def = ::at32(button_list, DS3KeyCodes::Down);
	cfg->right.def = ::at32(button_list, DS3KeyCodes::Right);
	cfg->up.def = ::at32(button_list, DS3KeyCodes::Up);
	cfg->r1.def = ::at32(button_list, DS3KeyCodes::R1);
	cfg->r2.def = ::at32(button_list, DS3KeyCodes::R2);
	cfg->r3.def = ::at32(button_list, DS3KeyCodes::R3);
	cfg->l1.def = ::at32(button_list, DS3KeyCodes::L1);
	cfg->l2.def = ::at32(button_list, DS3KeyCodes::L2);
	cfg->l3.def = ::at32(button_list, DS3KeyCodes::L3);

	cfg->pressure_intensity_button.def = ::at32(button_list, DS3KeyCodes::None);

	// Set default misc variables
	cfg->lstickdeadzone.def    = 40; // between 0 and 255
	cfg->rstickdeadzone.def    = 40; // between 0 and 255
	cfg->ltriggerthreshold.def = 0;  // between 0 and 255
	cfg->rtriggerthreshold.def = 0;  // between 0 and 255
	cfg->lpadsquircling.def    = 0;
	cfg->rpadsquircling.def    = 0;

	// Set default LED options
	cfg->led_battery_indicator.def = false;
	cfg->led_low_battery_blink.def = true;

	// apply defaults
	cfg->from_default();
}

void ds3_pad_handler::check_add_device(hid_device* hidDevice, std::string_view path, std::wstring_view wide_serial)
{
	if (!hidDevice)
	{
		return;
	}

	ds3_device* device = nullptr;

	for (auto& controller : m_controllers)
	{
		ensure(controller.second);

		if (!controller.second->hidDevice)
		{
			device = controller.second.get();
			break;
		}
	}

	if (!device)
	{
		return;
	}

	std::string serial;

#ifdef _WIN32
	std::array<u8, 0xFF> buf{};
	buf[0] = 0xF2;

	int res = hid_get_feature_report(hidDevice, buf.data(), buf.size());
	if (res <= 0 || buf[0] != 0xF2)
	{
		ds3_log.warning("check_add_device: hid_get_feature_report 0xF2 failed! Trying again with 0x0. (result=%d, buf[0]=0x%x, error=%s)", res, buf[0], hid_error(hidDevice));
		buf    = {};
		buf[0] = 0x0;
		res    = hid_get_feature_report(hidDevice, buf.data(), buf.size());
		if (res <= 0 || buf[0] != 0x0)
		{
			ds3_log.error("check_add_device: hid_get_feature_report 0x0 failed! result=%d, buf[0]=0x%x, error=%s", res, buf[0], hid_error(hidDevice));
			hid_close(hidDevice);
			return;
		}
	}

	device->report_id = buf[0];
#elif defined (__APPLE__)
	int res = hid_init_sixaxis_usb(hidDevice);
	if (res < 0)
	{
		ds3_log.error("check_add_device: hid_init_sixaxis_usb failed! (result=%d, error=%s)", res, hid_error(hidDevice));
		hid_close(hidDevice);
		return;
	}
#endif

	for (wchar_t ch : wide_serial)
		serial += static_cast<uchar>(ch);

	if (hid_set_nonblocking(hidDevice, 1) == -1)
	{
		ds3_log.error("check_add_device: hid_set_nonblocking failed! Reason: %s", hid_error(hidDevice));
		hid_close(hidDevice);
		return;
	}

	device->path      = path;
	device->hidDevice = hidDevice;

	if (send_output_report(device) == -1)
	{
		ds3_log.error("check_add_device: send_output_report failed! Reason: %s", hid_error(hidDevice));
	}

#ifdef _WIN32
	ds3_log.notice("Added device: report_id=%d, serial='%s', path='%s'", device->report_id, serial, device->path);
#else
	ds3_log.notice("Added device: serial='%s', path='%s'", serial, device->path);
#endif
}

ds3_pad_handler::DataStatus ds3_pad_handler::get_data(ds3_device* ds3dev)
{
	if (!ds3dev)
		return DataStatus::ReadError;

#ifdef _WIN32
	ds3dev->padData[0] = ds3dev->report_id;
	const int result = hid_get_feature_report(ds3dev->hidDevice, ds3dev->padData.data(), ds3dev->padData.size());
	if (result < 0)
	{
		ds3_log.error("get_data: hid_get_feature_report 0x%02x failed! result=%d, buf[0]=0x%x, error=%s", ds3dev->report_id, result, ds3dev->padData[0], hid_error(ds3dev->hidDevice));
		return DataStatus::ReadError;
	}
#else
	const int result = hid_read(ds3dev->hidDevice, ds3dev->padData.data(), ds3dev->padData.size());
	if (result < 0)
	{
		ds3_log.error("get_data: hid_read failed! result=%d, error=%s", result, hid_error(ds3dev->hidDevice));
		return DataStatus::ReadError;
	}
#endif

	if (result > 0)
	{
#ifdef _WIN32
		if (ds3dev->padData[0] == ds3dev->report_id)
#else
		if (ds3dev->padData[0] == 0x01 && ds3dev->padData[1] != 0xFF)
#endif
		{
			const u8 battery_status = ds3dev->padData[30 + DS3_HID_OFFSET];

			if (battery_status >= 0xEE)
			{
				// Charging (0xEE) or full (0xEF). Let's set the level to 100%.
				ds3dev->battery_level = 100;
				ds3dev->cable_state   = 1;
			}
			else
			{
				ds3dev->battery_level = ::at32(battery_capacity, std::min<usz>(battery_status, battery_capacity.size() - 1));
				ds3dev->cable_state   = 0;
			}

			return DataStatus::NewData;
		}

		ds3_log.warning("get_data: Unknown packet received: 0x%02x", ds3dev->padData[0]);
	}

	return DataStatus::NoNewData;
}

std::unordered_map<u64, u16> ds3_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> key_buf;
	ds3_device* dev = static_cast<ds3_device*>(device.get());
	if (!dev)
		return key_buf;

	auto& dbuf = dev->padData;

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

pad_preview_values ds3_pad_handler::get_preview_values(const std::unordered_map<u64, u16>& data)
{
	return {
		::at32(data, L2),
		::at32(data, R2),
		::at32(data, LSXPos) - ::at32(data, LSXNeg),
		::at32(data, LSYPos) - ::at32(data, LSYNeg),
		::at32(data, RSXPos) - ::at32(data, RSXNeg),
		::at32(data, RSYPos) - ::at32(data, RSYNeg)
	};
}

void ds3_pad_handler::get_extended_info(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	ds3_device* ds3dev = static_cast<ds3_device*>(device.get());
	if (!ds3dev || !pad)
		return;

	pad->m_battery_level = ds3dev->battery_level;
	pad->m_cable_state   = ds3dev->cable_state;

	// For unknown reasons the sixaxis values seem to be in little endian on linux

#ifdef _WIN32
	// Official Sony Windows DS3 driver seems to do the same modification of this value as the ps3
	pad->m_sensors[0].m_value = read_from_ptr<le_t<u16>>(ds3dev->padData, 41 + DS3_HID_OFFSET);
#else
	// When getting raw values from the device this adjustement is needed
	pad->m_sensors[0].m_value = 512 - (read_from_ptr<le_t<u16>>(ds3dev->padData, 41 + DS3_HID_OFFSET) - 512);
#endif
	pad->m_sensors[1].m_value = read_from_ptr<le_t<u16>>(ds3dev->padData, 45 + DS3_HID_OFFSET);
	pad->m_sensors[2].m_value = read_from_ptr<le_t<u16>>(ds3dev->padData, 43 + DS3_HID_OFFSET);
	pad->m_sensors[3].m_value = read_from_ptr<le_t<u16>>(ds3dev->padData, 47 + DS3_HID_OFFSET);

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

bool ds3_pad_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return keyCode == DS3KeyCodes::L2;
}

bool ds3_pad_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return keyCode == DS3KeyCodes::R2;
}

bool ds3_pad_handler::get_is_left_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
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

bool ds3_pad_handler::get_is_right_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
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
	ds3_device* dev = static_cast<ds3_device*>(device.get());
	if (!dev || dev->path.empty())
		return connection::disconnected;

	if (dev->hidDevice == nullptr)
	{
		hid_device* devhandle = hid_open_path(dev->path.c_str());
		if (devhandle)
		{
			if (hid_set_nonblocking(devhandle, 1) == -1)
			{
				ds3_log.error("Reconnecting Device %s: hid_set_nonblocking failed with error %s", dev->path, hid_error(devhandle));
			}
			dev->hidDevice = devhandle;
		}
		else
		{
			return connection::disconnected;
		}
	}

	if (get_data(dev) == DataStatus::ReadError)
	{
		// this also can mean disconnected, either way deal with it on next loop and reconnect
		hid_close(dev->hidDevice);
		dev->hidDevice = nullptr;

		return connection::no_data;
	}

	return connection::connected;
}

void ds3_pad_handler::apply_pad_data(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	ds3_device* dev = static_cast<ds3_device*>(device.get());
	if (!dev || !dev->hidDevice || !dev->config || !pad)
		return;

	cfg_pad* config = dev->config;

	const int idx_l = config->switch_vibration_motors ? 1 : 0;
	const int idx_s = config->switch_vibration_motors ? 0 : 1;

	const u8 speed_large = config->enable_vibration_motor_large ? pad->m_vibrateMotors[idx_l].m_value : 0;
	const u8 speed_small = config->enable_vibration_motor_small ? pad->m_vibrateMotors[idx_s].m_value : 0;

	const bool wireless    = dev->cable_state == 0;
	const bool low_battery = dev->battery_level < 25;
	const bool is_blinking = dev->led_delay_on > 0 || dev->led_delay_off > 0;

	// Blink LED when battery is low
	if (config->led_low_battery_blink)
	{
		// we are now wired or have okay battery level -> stop blinking
		if (is_blinking && !(wireless && low_battery))
		{
			dev->led_delay_on    = 0;
			dev->led_delay_off   = 0;
			dev->new_output_data = true;
		}
		// we are now wireless and low on battery -> blink
		else if (!is_blinking && wireless && low_battery)
		{
			dev->led_delay_on  = 0x80;
			dev->led_delay_off = 0xFF - dev->led_delay_on;
			dev->new_output_data = true;
		}
	}

	// Use LEDs to indicate battery level
	if (config->led_battery_indicator)
	{
		if (dev->last_battery_level != dev->battery_level)
		{
			dev->new_output_data = true;
			dev->last_battery_level = dev->battery_level;
		}
	}

	// Use LEDs to indicate battery level
	if (dev->enable_player_leds != config->player_led_enabled.get())
	{
		dev->new_output_data = true;
		dev->enable_player_leds = config->player_led_enabled.get();
	}

	dev->new_output_data |= dev->large_motor != speed_large || dev->small_motor != speed_small;

	dev->large_motor = speed_large;
	dev->small_motor = speed_small;

	if (dev->new_output_data)
	{
		if (send_output_report(dev) >= 0)
		{
			dev->new_output_data = false;
		}
	}
}
