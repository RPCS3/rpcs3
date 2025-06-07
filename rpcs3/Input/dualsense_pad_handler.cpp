#include "stdafx.h"
#include "dualsense_pad_handler.h"
#include "Emu/Io/pad_config.h"

#include <limits>

LOG_CHANNEL(dualsense_log, "DualSense");

using namespace reports;

template <>
void fmt_class_string<DualSenseDevice::DualSenseDataMode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](DualSenseDevice::DualSenseDataMode mode)
	{
		switch (mode)
		{
		case DualSenseDevice::DualSenseDataMode::Simple: return "Simple";
		case DualSenseDevice::DualSenseDataMode::Enhanced: return "Enhanced";
		}

		return unknown;
	});
}

namespace
{
	constexpr id_pair SONY_DUALSENSE_ID_0 = {0x054C, 0x0CE6}; // DualSense
	constexpr id_pair SONY_DUALSENSE_ID_1 = {0x054C, 0x0DF2}; // DualSense Edge
}

dualsense_pad_handler::dualsense_pad_handler()
    : hid_pad_handler<DualSenseDevice>(pad_handler::dualsense, {SONY_DUALSENSE_ID_0, SONY_DUALSENSE_ID_1})
{
	// Unique names for the config files and our pad settings dialog
	button_list =
	{
		{ DualSenseKeyCodes::None,     "" },
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
		{ DualSenseKeyCodes::Mic,      "Mic" },
		{ DualSenseKeyCodes::TouchPad, "Touch Pad" },
		{ DualSenseKeyCodes::Touch_L,  "Touch Left" },
		{ DualSenseKeyCodes::Touch_R,  "Touch Right" },
		{ DualSenseKeyCodes::Touch_U,  "Touch Up" },
		{ DualSenseKeyCodes::Touch_D,  "Touch Down" },
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
		{ DualSenseKeyCodes::RSYNeg,   "RS Y-" },
		{ DualSenseKeyCodes::EdgeFnL,  "FN L" },
		{ DualSenseKeyCodes::EdgeFnR,  "FN R" },
		{ DualSenseKeyCodes::EdgeLB,   "LB" },
		{ DualSenseKeyCodes::EdgeRB,   "RB" },
	};

	init_configs();

	// Define border values
	thumb_max = 255;
	trigger_min = 0;
	trigger_max = 255;

	// Set capabilities
	b_has_config = true;
	b_has_rumble = true;
	b_has_motion = true;
	b_has_deadzones = true;
	b_has_led = true;
	b_has_rgb = true;
	b_has_player_led = true;
	b_has_battery = true;
	b_has_battery_led = true;
	b_has_orientation = true;

	m_name_string = "DualSense Pad #";
	m_max_devices = CELL_PAD_MAX_PORT_NUM;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold   = thumb_max / 2;
}

void dualsense_pad_handler::check_add_device(hid_device* hidDevice, hid_enumerated_device_view path, std::wstring_view wide_serial)
{
	if (!hidDevice)
	{
		return;
	}

	DualSenseDevice* device = nullptr;

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

	std::array<u8, std::max(DUALSENSE_FIRMWARE_REPORT_SIZE, DUALSENSE_PAIRING_REPORT_SIZE)> buf{};
	buf[0] = 0x09;

	// This will give us the bluetooth mac address of the device, regardless if we are on wired or bluetooth.
	// So we can't use this to determine if it is a bluetooth device or not.
	// Will also enable enhanced feature reports for bluetooth.
	int res = hid_get_feature_report(hidDevice, buf.data(), buf.size());
	if (res < 0 || buf[0] != 0x09)
	{
		dualsense_log.error("check_add_device: hid_get_feature_report 0x09 failed! result=%d, buf[0]=0x%x, error=%s", res, buf[0], hid_error(hidDevice));
		HidDevice::close(hidDevice);
		return;
	}

	std::string serial;

	if (res == 21)
	{
		serial = fmt::format("%x%x%x%x%x%x", buf[6], buf[5], buf[4], buf[3], buf[2], buf[1]);
		device->data_mode = DualSenseDevice::DualSenseDataMode::Enhanced;
	}
	else
	{
		// We're probably on Bluetooth in this case, but for whatever reason the feature report failed.
		// This will give us a less capable fallback.
		dualsense_log.warning("check_add_device: hid_get_feature_report returned wrong size! Falling back to simple mode. (result=%d)", res);

		device->data_mode = DualSenseDevice::DualSenseDataMode::Simple;
		for (wchar_t ch : wide_serial)
			serial += static_cast<uchar>(ch);
	}

	device->hidDevice = hidDevice;

	if (!get_calibration_data(device))
	{
		dualsense_log.error("check_add_device: get_calibration_data failed!");
		device->close();
		return;
	}

	u32 hw_version{};
	u16 fw_version{};
	u32 fw_version2{};

	buf = {};
	buf[0] = 0x20;

	res = hid_get_feature_report(hidDevice, buf.data(), DUALSENSE_FIRMWARE_REPORT_SIZE);
	if (res != DUALSENSE_FIRMWARE_REPORT_SIZE || buf[0] != 0x20) // Old versions return 65, newer versions return 64
	{
		dualsense_log.error("check_add_device: hid_get_feature_report 0x20 failed! Could not retrieve firmware version! result=%d, buf[0]=0x%x, error=%s", res, buf[0], hid_error(hidDevice));
	}
	else
	{
		hw_version = read_u32(&buf[24]);
		fw_version2 = read_u32(&buf[28]);
		fw_version = static_cast<u16>(buf[44]) | (static_cast<u16>(buf[45]) << 8);
	}

	if (hid_set_nonblocking(hidDevice, 1) == -1)
	{
		dualsense_log.error("check_add_device: hid_set_nonblocking failed! Reason: %s", hid_error(hidDevice));
		device->close();
		return;
	}

	device->has_calib_data = true;
	device->path           = path;

	// Get feature set
	if (const hid_device_info* info = hid_get_device_info(device->hidDevice))
	{
		if (info->product_id == SONY_DUALSENSE_ID_1.m_pid)
		{
			device->feature_set = DualSenseDevice::DualSenseFeatureSet::Edge;
			dualsense_log.notice("check_add_device: device is DualSense Edge: vid=0x%x, pid=0x%x, path='%s'", info->vendor_id, info->product_id, path);
		}
	}
	else
	{
		dualsense_log.warning("check_add_device: hid_get_device_info failed for determining feature set! Reason: %s", hid_error(hidDevice));
	}

	// Activate
	if (send_output_report(device) == -1)
	{
		dualsense_log.error("check_add_device: send_output_report failed! Reason: %s", hid_error(hidDevice));
	}

	// Get bluetooth information
	get_data(device);

	dualsense_log.notice("Added device: bluetooth=%d, data_mode=%s, serial='%s', hw_version: 0x%x, fw_version: 0x%x (0x%x), path='%s'", device->bt_controller, device->data_mode, serial, hw_version, fw_version, fw_version2, device->path);
}

void dualsense_pad_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
	cfg->ls_left.def  = ::at32(button_list, DualSenseKeyCodes::LSXNeg);
	cfg->ls_down.def  = ::at32(button_list, DualSenseKeyCodes::LSYNeg);
	cfg->ls_right.def = ::at32(button_list, DualSenseKeyCodes::LSXPos);
	cfg->ls_up.def    = ::at32(button_list, DualSenseKeyCodes::LSYPos);
	cfg->rs_left.def  = ::at32(button_list, DualSenseKeyCodes::RSXNeg);
	cfg->rs_down.def  = ::at32(button_list, DualSenseKeyCodes::RSYNeg);
	cfg->rs_right.def = ::at32(button_list, DualSenseKeyCodes::RSXPos);
	cfg->rs_up.def    = ::at32(button_list, DualSenseKeyCodes::RSYPos);
	cfg->start.def    = ::at32(button_list, DualSenseKeyCodes::Options);
	cfg->select.def   = ::at32(button_list, DualSenseKeyCodes::Share);
	cfg->ps.def       = ::at32(button_list, DualSenseKeyCodes::PSButton);
	cfg->square.def   = ::at32(button_list, DualSenseKeyCodes::Square);
	cfg->cross.def    = ::at32(button_list, DualSenseKeyCodes::Cross);
	cfg->circle.def   = ::at32(button_list, DualSenseKeyCodes::Circle);
	cfg->triangle.def = ::at32(button_list, DualSenseKeyCodes::Triangle);
	cfg->left.def     = ::at32(button_list, DualSenseKeyCodes::Left);
	cfg->down.def     = ::at32(button_list, DualSenseKeyCodes::Down);
	cfg->right.def    = ::at32(button_list, DualSenseKeyCodes::Right);
	cfg->up.def       = ::at32(button_list, DualSenseKeyCodes::Up);
	cfg->r1.def       = ::at32(button_list, DualSenseKeyCodes::R1);
	cfg->r2.def       = ::at32(button_list, DualSenseKeyCodes::R2);
	cfg->r3.def       = ::at32(button_list, DualSenseKeyCodes::R3);
	cfg->l1.def       = ::at32(button_list, DualSenseKeyCodes::L1);
	cfg->l2.def       = ::at32(button_list, DualSenseKeyCodes::L2);
	cfg->l3.def       = ::at32(button_list, DualSenseKeyCodes::L3);

	cfg->pressure_intensity_button.def = ::at32(button_list, DualSenseKeyCodes::None);
	cfg->analog_limiter_button.def = ::at32(button_list, DualSenseKeyCodes::None);
	cfg->orientation_reset_button.def = ::at32(button_list, DualSenseKeyCodes::None);

	// Set default misc variables
	cfg->lstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
	cfg->rstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
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

dualsense_pad_handler::DataStatus dualsense_pad_handler::get_data(DualSenseDevice* device)
{
	if (!device)
		return DataStatus::ReadError;

	std::array<u8, 128> buf{};

	const int res = hid_read(device->hidDevice, buf.data(), buf.size());

	if (res == -1)
	{
		// looks like controller disconnected or read error
		return DataStatus::ReadError;
	}

	if (res == 0)
		return DataStatus::NoNewData;

	u8 offset = 0;

	switch (buf[0])
	{
	case 0x01:
	{
		if (res == sizeof(dualsense_input_report_bt))
		{
			device->data_mode     = DualSenseDevice::DualSenseDataMode::Simple;
			device->bt_controller = true;
		}
		else
		{
			device->data_mode     = DualSenseDevice::DualSenseDataMode::Enhanced;
			device->bt_controller = false;
		}

		offset = offsetof(dualsense_input_report_usb, common);
		break;
	}
	case 0x31:
	{
		device->data_mode     = DualSenseDevice::DualSenseDataMode::Enhanced;
		device->bt_controller = true;

		offset = offsetof(dualsense_input_report_bt, common);

		const u8 btHdr = 0xA1;
		const u32 crcHdr = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
		const u32 crcCalc = CRCPP::CRC::Calculate(buf.data(), offsetof(dualsense_input_report_bt, crc32), crcTable, crcHdr);
		const u32 crcReported = read_u32(&buf[offsetof(dualsense_input_report_bt, crc32)]);
		if (crcCalc != crcReported)
		{
			dualsense_log.warning("Data packet CRC check failed, ignoring! Received 0x%x, Expected 0x%x", crcReported, crcCalc);
			return DataStatus::NoNewData;
		}
		break;
	}
	default:
		return DataStatus::NoNewData;
	}

	if (device->has_calib_data)
	{
		int calib_offset = offset + offsetof(dualsense_input_report_common, gyro);
		for (int i = 0; i < CalibIndex::COUNT; ++i)
		{
			const s16 raw_value = read_s16(&buf[calib_offset]);
			const s16 cal_value = apply_calibration(raw_value, device->calib_data[i]);
			buf[calib_offset++] = (static_cast<u16>(cal_value) >> 0) & 0xFF;
			buf[calib_offset++] = (static_cast<u16>(cal_value) >> 8) & 0xFF;
		}
	}

	std::memcpy(&device->report, &buf[offset], sizeof(dualsense_input_report_common));

	// For now let's only get battery info in enhanced mode
	if (device->data_mode == DualSenseDevice::DualSenseDataMode::Enhanced)
	{
		const u8 battery_state = device->report.status;
		const u8 battery_value = battery_state & 0x0F; // 10% per unit, starting with 0-9%. So 100% equals unit 10
		const u8 charge_info = (battery_state & 0xF0) >> 4;

		switch (charge_info)
		{
		case 0x0:
			device->battery_level = battery_value;
			device->cable_state = 0;
			break;
		case 0x1:
			device->battery_level = battery_value;
			device->cable_state = 1;
			break;
		case 0x2:
			device->battery_level = 10;
			device->cable_state = 1;
			break;
		default:
			// We don't care about the other values. Just set battery to 0.
			device->battery_level = 0;
			device->cable_state = 0;
			break;
		}
	}

	return DataStatus::NewData;
}

bool dualsense_pad_handler::get_calibration_data(DualSenseDevice* dev) const
{
	if (!dev || !dev->hidDevice)
	{
		dualsense_log.error("get_calibration_data called with null device");
		return false;
	}

	std::array<u8, 64> buf{};

	if (dev->bt_controller)
	{
		for (int tries = 0; tries < 3; ++tries)
		{
			buf = {};
			buf[0] = 0x05;

			if (int res = hid_get_feature_report(dev->hidDevice, buf.data(), DUALSENSE_CALIBRATION_REPORT_SIZE); res != DUALSENSE_CALIBRATION_REPORT_SIZE || buf[0] != 0x05)
			{
				dualsense_log.error("get_calibration_data: hid_get_feature_report 0x05 for bluetooth controller failed! result=%d, buf[0]=0x%x, error=%s", res, buf[0], hid_error(dev->hidDevice));
				return false;
			}

			const u8 btHdr        = 0xA3;
			const u32 crcHdr      = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
			const u32 crcCalc     = CRCPP::CRC::Calculate(buf.data(), (DUALSENSE_CALIBRATION_REPORT_SIZE - 4), crcTable, crcHdr);
			const u32 crcReported = read_u32(&buf[DUALSENSE_CALIBRATION_REPORT_SIZE - 4]);

			if (crcCalc == crcReported)
				break;

			dualsense_log.warning("Calibration CRC check failed! Will retry up to 3 times. Received 0x%x, Expected 0x%x", crcReported, crcCalc);

			if (tries == 2)
			{
				dualsense_log.error("Calibration CRC check failed too many times!");
				return false;
			}
		}
	}
	else
	{
		buf[0] = 0x05;

		if (int res = hid_get_feature_report(dev->hidDevice, buf.data(), DUALSENSE_CALIBRATION_REPORT_SIZE); res != DUALSENSE_CALIBRATION_REPORT_SIZE || buf[0] != 0x05)
		{
			dualsense_log.error("get_calibration_data: hid_get_feature_report 0x05 for wired controller failed! result=%d, buf[0]=0x%x, error=%s", res, buf[0], hid_error(dev->hidDevice));
			return false;
		}
	}

	dev->calib_data[CalibIndex::PITCH].bias = read_s16(&buf[1]);
	dev->calib_data[CalibIndex::YAW].bias   = read_s16(&buf[3]);
	dev->calib_data[CalibIndex::ROLL].bias  = read_s16(&buf[5]);

	const s16 pitch_plus  = read_s16(&buf[7]);
	const s16 pitch_minus = read_s16(&buf[9]);
	const s16 yaw_plus    = read_s16(&buf[11]);
	const s16 yaw_minus   = read_s16(&buf[13]);
	const s16 roll_plus   = read_s16(&buf[15]);
	const s16 roll_minus  = read_s16(&buf[17]);

	// Confirm correctness. Need confirmation with dongle with no active controller
	if (pitch_plus <= 0 || yaw_plus <= 0 || roll_plus <= 0 ||
	    pitch_minus >= 0 || yaw_minus >= 0 || roll_minus >= 0)
	{
		dualsense_log.error("get_calibration_data: calibration data check failed! pitch_plus=%d, pitch_minus=%d, roll_plus=%d, roll_minus=%d, yaw_plus=%d, yaw_minus=%d",
		    pitch_plus, pitch_minus, roll_plus, roll_minus, yaw_plus, yaw_minus);
	}

	const s32 gyro_speed_scale = read_s16(&buf[19]) + read_s16(&buf[21]);

	dev->calib_data[CalibIndex::PITCH].sens_numer = gyro_speed_scale * DUALSENSE_GYRO_RES_PER_DEG_S;
	dev->calib_data[CalibIndex::PITCH].sens_denom = pitch_plus - pitch_minus;

	dev->calib_data[CalibIndex::YAW].sens_numer = gyro_speed_scale * DUALSENSE_GYRO_RES_PER_DEG_S;
	dev->calib_data[CalibIndex::YAW].sens_denom = yaw_plus - yaw_minus;

	dev->calib_data[CalibIndex::ROLL].sens_numer = gyro_speed_scale * DUALSENSE_GYRO_RES_PER_DEG_S;
	dev->calib_data[CalibIndex::ROLL].sens_denom = roll_plus - roll_minus;

	const s16 accel_x_plus  = read_s16(&buf[23]);
	const s16 accel_x_minus = read_s16(&buf[25]);
	const s16 accel_y_plus  = read_s16(&buf[27]);
	const s16 accel_y_minus = read_s16(&buf[29]);
	const s16 accel_z_plus  = read_s16(&buf[31]);
	const s16 accel_z_minus = read_s16(&buf[33]);

	const s32 accel_x_range = accel_x_plus - accel_x_minus;
	const s32 accel_y_range = accel_y_plus - accel_y_minus;
	const s32 accel_z_range = accel_z_plus - accel_z_minus;

	dev->calib_data[CalibIndex::X].bias       = accel_x_plus - accel_x_range / 2;
	dev->calib_data[CalibIndex::X].sens_numer = 2 * DUALSENSE_ACC_RES_PER_G;
	dev->calib_data[CalibIndex::X].sens_denom = accel_x_range;

	dev->calib_data[CalibIndex::Y].bias       = accel_y_plus - accel_y_range / 2;
	dev->calib_data[CalibIndex::Y].sens_numer = 2 * DUALSENSE_ACC_RES_PER_G;
	dev->calib_data[CalibIndex::Y].sens_denom = accel_y_range;

	dev->calib_data[CalibIndex::Z].bias       = accel_z_plus - accel_z_range / 2;
	dev->calib_data[CalibIndex::Z].sens_numer = 2 * DUALSENSE_ACC_RES_PER_G;
	dev->calib_data[CalibIndex::Z].sens_denom = accel_z_range;

	// Make sure data 'looks' valid, dongle will report invalid calibration data with no controller connected

	for (usz i = 0; i < dev->calib_data.size(); i++)
	{
		CalibData& data = dev->calib_data[i];

		if (data.sens_denom == 0)
		{
			dualsense_log.error("GetCalibrationData: Invalid accelerometer calibration data for axis %d, disabling calibration.", i);
			data.bias = 0;
			data.sens_numer = 4 * DUALSENSE_ACC_RES_PER_G;
			data.sens_denom = std::numeric_limits<s16>::max();
		}
	}

	return true;
}

bool dualsense_pad_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return keyCode == DualSenseKeyCodes::L2;
}

bool dualsense_pad_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return keyCode == DualSenseKeyCodes::R2;
}

bool dualsense_pad_handler::get_is_left_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
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

bool dualsense_pad_handler::get_is_right_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
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

bool dualsense_pad_handler::get_is_touch_pad_motion(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	switch (keyCode)
	{
	case DualSenseKeyCodes::Touch_L:
	case DualSenseKeyCodes::Touch_R:
	case DualSenseKeyCodes::Touch_U:
	case DualSenseKeyCodes::Touch_D:
		return true;
	default:
		return false;
	}
}

PadHandlerBase::connection dualsense_pad_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	DualSenseDevice* dev = static_cast<DualSenseDevice*>(device.get());
	if (!dev || dev->path == hid_enumerated_device_default)
		return connection::disconnected;

	if (dev->hidDevice == nullptr)
	{
		// try to reconnect
		if (hid_device* hid_dev = dev->open())
		{
			if (hid_set_nonblocking(hid_dev, 1) == -1)
			{
				dualsense_log.error("Reconnecting Device %s: hid_set_nonblocking failed with error %s", dev->path, hid_error(hid_dev));
			}

			if (!dev->has_calib_data)
			{
				dev->has_calib_data = get_calibration_data(dev);
			}
		}
		else
		{
			// nope, not there
			return connection::disconnected;
		}
	}

	if (get_data(dev) == DataStatus::ReadError)
	{
		// this also can mean disconnected, either way deal with it on next loop and reconnect
		dev->close();

		return connection::no_data;
	}

	return connection::connected;
}

void dualsense_pad_handler::get_extended_info(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	DualSenseDevice* dev = static_cast<DualSenseDevice*>(device.get());
	if (!dev || !pad)
		return;

	pad->m_battery_level = dev->battery_level;
	pad->m_cable_state   = dev->cable_state;

	const dualsense_input_report_common& input = dev->report;

	// these values come already calibrated, all we need to do is convert to ds3 range

	// gyro (angular velocity in degree/s)
	const f32 gyro_x = static_cast<s16>(input.gyro[0]) / static_cast<f32>(DUALSENSE_GYRO_RES_PER_DEG_S) * -1.f;
	const f32 gyro_y = static_cast<s16>(input.gyro[1]) / static_cast<f32>(DUALSENSE_GYRO_RES_PER_DEG_S) * -1.f;
	const f32 gyro_z = static_cast<s16>(input.gyro[2]) / static_cast<f32>(DUALSENSE_GYRO_RES_PER_DEG_S) * -1.f;

	// acceleration (linear velocity in m/s²)
	const f32 accel_x = static_cast<s16>(input.accel[0]) / static_cast<f32>(DUALSENSE_ACC_RES_PER_G) * -1;
	const f32 accel_y = static_cast<s16>(input.accel[1]) / static_cast<f32>(DUALSENSE_ACC_RES_PER_G) * -1;
	const f32 accel_z = static_cast<s16>(input.accel[2]) / static_cast<f32>(DUALSENSE_ACC_RES_PER_G) * -1;

	// now just use formula from ds3
	pad->m_sensors[0].m_value = Clamp0To1023(accel_x * MOTION_ONE_G + 512);
	pad->m_sensors[1].m_value = Clamp0To1023(accel_y * MOTION_ONE_G + 512);
	pad->m_sensors[2].m_value = Clamp0To1023(accel_z * MOTION_ONE_G + 512);

	// gyro_y is yaw, which is all that we need
	// Convert to ds3. The ds3 resolution is 123/90°/sec.
	pad->m_sensors[3].m_value = Clamp0To1023(gyro_y * (123.f / 90.f) + 512);

	// Set raw orientation
	set_raw_orientation(pad->move_data, accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z);
}

std::unordered_map<u64, u16> dualsense_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> keyBuffer;
	DualSenseDevice* dev = static_cast<DualSenseDevice*>(device.get());
	if (!dev)
		return keyBuffer;

	const dualsense_input_report_common& input = dev->report;

	const bool is_simple_mode = dev->data_mode == DualSenseDevice::DualSenseDataMode::Simple;

	// Left Stick X Axis
	keyBuffer[DualSenseKeyCodes::LSXNeg] = Clamp0To255((127.5f - input.x) * 2.0f);
	keyBuffer[DualSenseKeyCodes::LSXPos] = Clamp0To255((input.x - 127.5f) * 2.0f);

	// Left Stick Y Axis (Up is the negative for some reason)
	keyBuffer[DualSenseKeyCodes::LSYNeg] = Clamp0To255((input.y - 127.5f) * 2.0f);
	keyBuffer[DualSenseKeyCodes::LSYPos] = Clamp0To255((127.5f - input.y) * 2.0f);

	// Right Stick X Axis
	keyBuffer[DualSenseKeyCodes::RSXNeg] = Clamp0To255((127.5f - input.rx) * 2.0f);
	keyBuffer[DualSenseKeyCodes::RSXPos] = Clamp0To255((input.rx - 127.5f) * 2.0f);

	// Right Stick Y Axis (Up is the negative for some reason)
	keyBuffer[DualSenseKeyCodes::RSYNeg] = Clamp0To255((input.ry - 127.5f) * 2.0f);
	keyBuffer[DualSenseKeyCodes::RSYPos] = Clamp0To255((127.5f - input.ry) * 2.0f);

	keyBuffer[DualSenseKeyCodes::L2] = is_simple_mode ? input.buttons[0] : input.z;
	keyBuffer[DualSenseKeyCodes::R2] = is_simple_mode ? input.buttons[1] : input.rz;

	u8 data = (is_simple_mode ? input.z : input.buttons[0]) & 0xf;
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

	data = (is_simple_mode ? input.z : input.buttons[0]) >> 4;
	keyBuffer[DualSenseKeyCodes::Square]   = ((data & 0x01) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::Cross]    = ((data & 0x02) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::Circle]   = ((data & 0x04) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::Triangle] = ((data & 0x08) != 0) ? 255 : 0;

	data = (is_simple_mode ? input.rz : input.buttons[1]);
	keyBuffer[DualSenseKeyCodes::L1]      = ((data & 0x01) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::R1]      = ((data & 0x02) != 0) ? 255 : 0;
	//keyBuffer[DualSenseKeyCodes::L2]      = ((data & 0x04) != 0) ? 255 : 0; // active when L2 is pressed
	//keyBuffer[DualSenseKeyCodes::R2]      = ((data & 0x08) != 0) ? 255 : 0; // active when R2 is pressed
	keyBuffer[DualSenseKeyCodes::Share]   = ((data & 0x10) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::Options] = ((data & 0x20) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::L3]      = ((data & 0x40) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::R3]      = ((data & 0x80) != 0) ? 255 : 0;

	data = (is_simple_mode ? input.seq_number : input.buttons[2]);
	keyBuffer[DualSenseKeyCodes::PSButton] = ((data & 0x01) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::TouchPad] = ((data & 0x02) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::Mic]      = ((data & 0x04) != 0) ? 255 : 0;

	// Touch Pad
	for (const dualsense_touch_point& point : input.points)
	{
		if (!(point.contact & DUALSENSE_TOUCH_POINT_INACTIVE))
		{
			const s32 x = (point.x_hi << 8) | point.x_lo;
			const s32 y = (point.y_hi << 4) | point.y_lo;

			const f32 x_scaled = ScaledInput(static_cast<float>(x), 0.0f, static_cast<float>(DUALSENSE_TOUCHPAD_WIDTH), 0.0f, 255.0f);
			const f32 y_scaled = ScaledInput(static_cast<float>(y), 0.0f, static_cast<float>(DUALSENSE_TOUCHPAD_HEIGHT), 0.0f, 255.0f);

			keyBuffer[DualSenseKeyCodes::Touch_L] = Clamp0To255((127.5f - x_scaled) * 2.0f);
			keyBuffer[DualSenseKeyCodes::Touch_R] = Clamp0To255((x_scaled - 127.5f) * 2.0f);

			keyBuffer[DualSenseKeyCodes::Touch_U] = Clamp0To255((127.5f - y_scaled) * 2.0f);
			keyBuffer[DualSenseKeyCodes::Touch_D] = Clamp0To255((y_scaled - 127.5f) * 2.0f);
		}
	}

	if (dev->feature_set == DualSenseDevice::DualSenseFeatureSet::Edge)
	{
		keyBuffer[DualSenseKeyCodes::EdgeFnL] = ((data & 0x10) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::EdgeFnR] = ((data & 0x20) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::EdgeLB] = ((data & 0x40) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::EdgeRB] = ((data & 0x80) != 0) ? 255 : 0;
	}

	return keyBuffer;
}

pad_preview_values dualsense_pad_handler::get_preview_values(const std::unordered_map<u64, u16>& data)
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

dualsense_pad_handler::~dualsense_pad_handler()
{
	for (auto& controller : m_controllers)
	{
		if (controller.second && controller.second->hidDevice)
		{
			// Disable vibration
			controller.second->small_motor = 0;
			controller.second->large_motor = 0;

			// Turns off the lights (disabled due to user complaints)
			//controller.second->release_leds = true;

			if (send_output_report(controller.second.get()) == -1)
			{
				dualsense_log.error("~dualsense_pad_handler: send_output_report failed! Reason: %s", hid_error(controller.second->hidDevice));
			}
		}
	}
}

int dualsense_pad_handler::send_output_report(DualSenseDevice* device)
{
	if (!device || !device->hidDevice)
		return -2;

	const cfg_pad* config = device->config;
	if (config == nullptr)
		return -2; // hid_write returns -1 on error

	dualsense_output_report_common common{};

	// Only initialize lightbar in the first output report. The controller didn't seem to update the player LEDs correctly otherwise. (Might be placebo)
	if (device->init_lightbar)
	{
		device->init_lightbar = false;
		device->lightbar_on = true;
		device->lightbar_on_old = true;

		common.valid_flag_2 |= VALID_FLAG_2_LIGHTBAR_SETUP_CONTROL_ENABLE;
		common.lightbar_setup = LIGHTBAR_SETUP_LIGHT_OFF; // Fade light out.
	}
	else if (device->release_leds)
	{
		common.valid_flag_1 |= VALID_FLAG_1_RELEASE_LEDS;
		device->release_leds = false;
	}
	else
	{
		common.valid_flag_0 |= VALID_FLAG_0_COMPATIBLE_VIBRATION;
		common.valid_flag_0 |= VALID_FLAG_0_HAPTICS_SELECT;
		common.valid_flag_1 |= VALID_FLAG_1_POWER_SAVE_CONTROL_ENABLE;
		common.valid_flag_2 |= VALID_FLAG_2_IMPROVED_RUMBLE_EMULATION;

		common.motor_left  = device->large_motor;
		common.motor_right = device->small_motor;

		if (device->update_lightbar)
		{
			device->update_lightbar = false;

			common.valid_flag_1 |= VALID_FLAG_1_LIGHTBAR_CONTROL_ENABLE;

			if (device->lightbar_on)
			{
				common.lightbar_r = config->colorR; // red
				common.lightbar_g = config->colorG; // green
				common.lightbar_b = config->colorB; // blue
			}
			else
			{
				common.lightbar_r = 0;
				common.lightbar_g = 0;
				common.lightbar_b = 0;
			}

			device->lightbar_on_old = device->lightbar_on;
		}

		if (device->update_player_leds)
		{
			device->update_player_leds = false;

			// The dualsense controller uses 5 LEDs to indicate the player ID.
			// Use OR with 0x1, 0x2, 0x4, 0x8 and 0x10 to enable the LEDs (from leftmost to rightmost).
			common.valid_flag_1 |= VALID_FLAG_1_PLAYER_INDICATOR_CONTROL_ENABLE;

			if (config->player_led_enabled)
			{
				switch (device->player_id)
				{
				case 0: common.player_leds = 0b00100; break;
				case 1: common.player_leds = 0b01010; break;
				case 2: common.player_leds = 0b10101; break;
				case 3: common.player_leds = 0b11011; break;
				case 4: common.player_leds = 0b11111; break;
				case 5: common.player_leds = 0b10111; break;
				case 6: common.player_leds = 0b11101; break;
				default:
					fmt::throw_exception("Dualsense is using forbidden player id %d", device->player_id);
				}
			}
			else
			{
				common.player_leds = 0;
			}
		}
	}

	if (device->bt_controller)
	{
		const u8 seq_tag = (device->bt_sequence << 4) | 0x0;
		if (++device->bt_sequence >= 16) device->bt_sequence = 0;

		dualsense_output_report_bt report{};
		report.report_id = 0x31; // report id for bluetooth
		report.seq_tag   = seq_tag;
		report.tag       = 0x10; // magic number
		report.common    = std::move(common);

		const u8 btHdr    = 0xA2;
		const u32 crcHdr  = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
		const u32 crcCalc = CRCPP::CRC::Calculate(&report.report_id, (sizeof(dualsense_output_report_bt) - 4), crcTable, crcHdr);

		write_to_ptr(report.crc32, crcCalc);

		return hid_write(device->hidDevice, &report.report_id, sizeof(dualsense_output_report_bt));
	}

	dualsense_output_report_usb report{};
	report.report_id = 0x02; // report id for usb
	report.common    = std::move(common);

	return hid_write(device->hidDevice, &report.report_id, DUALSENSE_USB_REPORT_SIZE);
}

void dualsense_pad_handler::apply_pad_data(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	DualSenseDevice* dev = static_cast<DualSenseDevice*>(device.get());
	if (!dev || !dev->hidDevice || !dev->config || !pad)
		return;

	cfg_pad* config = dev->config;

	// Attempt to send rumble no matter what
	const u8 speed_large = config->get_large_motor_speed(pad->m_vibrateMotors);
	const u8 speed_small = config->get_small_motor_speed(pad->m_vibrateMotors);

	const bool wireless    = dev->cable_state == 0;
	const bool low_battery = dev->battery_level <= 1;
	const bool is_blinking = dev->led_delay_on > 0 || dev->led_delay_off > 0;

	// Blink LED when battery is low
	if (config->led_low_battery_blink)
	{
		// we are now wired or have okay battery level -> stop blinking
		if (is_blinking && !(wireless && low_battery))
		{
			dev->lightbar_on = true;
			dev->led_delay_on = 0;
			dev->led_delay_off = 0;
			dev->update_lightbar = true;
		}
		// we are now wireless and low on battery -> blink
		else if (!is_blinking && wireless && low_battery)
		{
			dev->led_delay_on = 100;
			dev->led_delay_off = 100;
			dev->update_lightbar = true;
		}

		// Turn lightbar on and off in an interval. I wanted to do an automatic pulse, but I haven't found out how to do that yet.
		if (dev->led_delay_on > 0)
		{
			if (const steady_clock::time_point now = steady_clock::now(); (now - dev->last_lightbar_time) > 500ms)
			{
				dev->lightbar_on = !dev->lightbar_on;
				dev->last_lightbar_time = now;
				dev->update_lightbar = true;
			}
		}
	}
	else if (!dev->lightbar_on)
	{
		dev->lightbar_on = true;
		dev->update_lightbar = true;
	}

	// Use LEDs to indicate battery level
	if (config->led_battery_indicator)
	{
		// This makes sure that the LED color doesn't update every 1ms. DS4 only reports battery level in 10% increments
		if (dev->last_battery_level != dev->battery_level)
		{
			const u32 combined_color = get_battery_color(dev->battery_level, config->led_battery_indicator_brightness);
			config->colorR.set(combined_color >> 8);
			config->colorG.set(combined_color & 0xff);
			config->colorB.set(0);
			dev->update_lightbar = true;
			dev->last_battery_level = dev->battery_level;
		}
	}

	if (dev->enable_player_leds != config->player_led_enabled.get())
	{
		dev->enable_player_leds = config->player_led_enabled.get();
		dev->update_player_leds = true;
	}

	dev->new_output_data |= dev->release_leds || dev->update_player_leds || dev->update_lightbar || dev->large_motor != speed_large || dev->small_motor != speed_small;

	dev->large_motor = speed_large;
	dev->small_motor = speed_small;

	const auto now = steady_clock::now();
	const auto elapsed = now - dev->last_output;

	if (dev->new_output_data || elapsed > min_output_interval)
	{
		if (const int res = send_output_report(dev); res >= 0)
		{
			dev->new_output_data = false;
			dev->last_output = now;
		}
		else if (res == -1)
		{
			dualsense_log.error("apply_pad_data: send_output_report failed! error=%s", hid_error(dev->hidDevice));
		}
	}
}

void dualsense_pad_handler::SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool player_led, bool battery_led, u32 battery_led_brightness)
{
	std::shared_ptr<DualSenseDevice> device = get_hid_device(padId);
	if (device == nullptr || device->hidDevice == nullptr)
		return;

	// Set the device's motor speeds to our requested values 0-255
	device->large_motor = large_motor;
	device->small_motor = small_motor;
	device->player_id = player_id;
	device->config = get_config(padId);

	ensure(device->config);
	device->update_lightbar = true;
	device->update_player_leds = true;
	device->config->player_led_enabled.set(player_led);

	// Set new LED color (see ds4_pad_handler)
	if (battery_led)
	{
		const u32 combined_color = get_battery_color(device->battery_level, battery_led_brightness);
		device->config->colorR.set(combined_color >> 8);
		device->config->colorG.set(combined_color & 0xff);
		device->config->colorB.set(0);
	}
	else if (r >= 0 && g >= 0 && b >= 0 && r <= 255 && g <= 255 && b <= 255)
	{
		device->config->colorR.set(r);
		device->config->colorG.set(g);
		device->config->colorB.set(b);
	}

	if (device->init_lightbar)
	{
		// Initialize first
		if (send_output_report(device.get()) == -1)
		{
			dualsense_log.error("SetPadData: send_output_report failed! Reason: %s", hid_error(device->hidDevice));
		}
	}

	// Start/Stop the engines :)
	if (send_output_report(device.get()) == -1)
	{
		dualsense_log.error("SetPadData: send_output_report failed! Reason: %s", hid_error(device->hidDevice));
	}
}

u32 dualsense_pad_handler::get_battery_level(const std::string& padId)
{
	const std::shared_ptr<DualSenseDevice> device = get_hid_device(padId);
	if (device == nullptr || device->hidDevice == nullptr)
	{
		return 0;
	}
	return std::min<u32>(device->battery_level * 10 + 5, 100); // 10% per unit, starting with 0-9%. So 100% equals unit 10
}
