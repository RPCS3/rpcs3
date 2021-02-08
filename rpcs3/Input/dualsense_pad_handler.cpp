#include "stdafx.h"
#include "dualsense_pad_handler.h"
#include "Emu/Io/pad_config.h"

LOG_CHANNEL(dualsense_log, "DualSense");

namespace
{
	const u32 DUALSENSE_ACC_RES_PER_G = 8192;
	const u32 DUALSENSE_GYRO_RES_PER_DEG_S = 1024;
	const u32 DUALSENSE_CALIBRATION_REPORT_SIZE = 41;
	const u32 DUALSENSE_BLUETOOTH_REPORT_SIZE = 78;
	const u32 DUALSENSE_USB_REPORT_SIZE = 63;
	const u32 DUALSENSE_COMMON_REPORT_SIZE = 47;
	const u32 DUALSENSE_INPUT_REPORT_GYRO_X_OFFSET = 15;

	
	struct output_report_common
	{
		u8 valid_flag_0;
		u8 valid_flag_1;
		u8 motor_right;
		u8 motor_left;
		u8 reserved[4];
		u8 mute_button_led;
		u8 power_save_control;
		u8 reserved_2[28];
		u8 valid_flag_2;
		u8 reserved_3[2];
		u8 lightbar_setup;
		u8 led_brightness;
		u8 player_leds;
		u8 lightbar_r;
		u8 lightbar_g;
		u8 lightbar_b;
	};

	struct output_report_bt
	{
		u8 report_id; // 0x31
		u8 seq_tag;
		u8 tag;
		output_report_common common;
		u8 reserved[24];
		u8 crc32[4];
	};

	struct output_report_usb
	{
		u8 report_id; // 0x02
		output_report_common common;
		u8 reserved[15];
	};

	static_assert(sizeof(struct output_report_common) == DUALSENSE_COMMON_REPORT_SIZE);
	static_assert(sizeof(struct output_report_bt) == DUALSENSE_BLUETOOTH_REPORT_SIZE);
	static_assert(sizeof(struct output_report_usb) == DUALSENSE_USB_REPORT_SIZE);

	inline s16 read_s16(const void* buf)
	{
		return *reinterpret_cast<const s16*>(buf);
	}

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
		{ DualSenseKeyCodes::Mic,      "Mic" },
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

	if (!get_calibration_data(dualsenseDev))
	{
		dualsense_log.error("CheckAddDevice: get_calibration_data failed!");
		hid_close(hidDevice);
		return;
	}

	if (hid_set_nonblocking(hidDevice, 1) == -1)
	{
		dualsense_log.error("CheckAddDevice: hid_set_nonblocking failed! Reason: %s", hid_error(hidDevice));
		hid_close(hidDevice);
		return;
	}

	dualsenseDev->has_calib_data = true;
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

	for (usz i = 1; i < controllers.size(); ++i)
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

	if (res == -1)
	{
		// looks like controller disconnected or read error
		return DualSenseDataStatus::ReadError;
	}

	if (res == 0)
		return DualSenseDataStatus::NoNewData;

	u8 offset = 0;
	switch (buf[0])
	{
	case 0x01:
	{
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
	}
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

	if (device->has_calib_data)
	{
		int calib_offset = offset + DUALSENSE_INPUT_REPORT_GYRO_X_OFFSET;
		for (int i = 0; i < DualSenseCalibIndex::COUNT; ++i)
		{
			const s16 raw_value = read_s16(&buf[calib_offset]);
			const s16 cal_value = apply_calibration(raw_value, device->calib_data[i]);
			buf[calib_offset++] = (static_cast<u16>(cal_value) >> 0) & 0xFF;
			buf[calib_offset++] = (static_cast<u16>(cal_value) >> 8) & 0xFF;
		}
	}

	memcpy(device->padData.data(), &buf[offset], 64);
	return DualSenseDataStatus::NewData;
}

bool dualsense_pad_handler::get_calibration_data(const std::shared_ptr<DualSenseDevice>& dualsense_device)
{
	if (!dualsense_device || !dualsense_device->hidDevice)
	{
		dualsense_log.error("get_calibration_data called with null device");
		return false;
	}

	std::array<u8, 64> buf;
	if (dualsense_device->btCon)
	{
		for (int tries = 0; tries < 3; ++tries)
		{
			buf[0] = 0x05;

			if (hid_get_feature_report(dualsense_device->hidDevice, buf.data(), DUALSENSE_CALIBRATION_REPORT_SIZE) <= 0)
			{
				dualsense_log.error("get_calibration_data: hid_get_feature_report 0x05 failed! Reason: %s", hid_error(dualsense_device->hidDevice));
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

		if (hid_get_feature_report(dualsense_device->hidDevice, buf.data(), DUALSENSE_CALIBRATION_REPORT_SIZE) <= 0)
		{
			dualsense_log.error("get_calibration_data: hid_get_feature_report 0x05 failed! Reason: %s", hid_error(dualsense_device->hidDevice));
			return false;
		}
	}

	dualsense_device->calib_data[DualSenseCalibIndex::PITCH].bias = read_s16(&buf[1]);
	dualsense_device->calib_data[DualSenseCalibIndex::YAW].bias   = read_s16(&buf[3]);
	dualsense_device->calib_data[DualSenseCalibIndex::ROLL].bias  = read_s16(&buf[5]);

	s16 pitch_plus, pitch_minus, roll_plus, roll_minus, yaw_plus, yaw_minus;

	// TODO: This was copied from DS4. Find out if it applies here.
	// Check for calibration data format
	// It's going to be either alternating +/- or +++---
	if (read_s16(&buf[9]) < 0 && read_s16(&buf[7]) > 0)
	{
		// Wired mode for OEM controllers
		pitch_plus  = read_s16(&buf[7]);
		pitch_minus = read_s16(&buf[9]);
		yaw_plus    = read_s16(&buf[11]);
		yaw_minus   = read_s16(&buf[13]);
		roll_plus   = read_s16(&buf[15]);
		roll_minus  = read_s16(&buf[17]);
	}
	else
	{
		// Bluetooth mode and wired mode for some 3rd party controllers
		pitch_plus  = read_s16(&buf[7]);
		yaw_plus    = read_s16(&buf[9]);
		roll_plus   = read_s16(&buf[11]);
		pitch_minus = read_s16(&buf[13]);
		yaw_minus   = read_s16(&buf[15]);
		roll_minus  = read_s16(&buf[17]);
	}

	// Confirm correctness. Need confirmation with dongle with no active controller
	if (pitch_plus <= 0 || yaw_plus <= 0 || roll_plus <= 0 ||
	    pitch_minus >= 0 || yaw_minus >= 0 || roll_minus >= 0)
	{
		dualsense_log.error("get_calibration_data: calibration data check failed! pitch_plus=%d, pitch_minus=%d, roll_plus=%d, roll_minus=%d, yaw_plus=%d, yaw_minus=%d",
		    pitch_plus, pitch_minus, roll_plus, roll_minus, yaw_plus, yaw_minus);
		return false;
	}

	const s32 gyro_speed_scale = read_s16(&buf[19]) + read_s16(&buf[21]);

	dualsense_device->calib_data[DualSenseCalibIndex::PITCH].sens_numer = gyro_speed_scale * DUALSENSE_GYRO_RES_PER_DEG_S;
	dualsense_device->calib_data[DualSenseCalibIndex::PITCH].sens_denom = pitch_plus - pitch_minus;

	dualsense_device->calib_data[DualSenseCalibIndex::YAW].sens_numer = gyro_speed_scale * DUALSENSE_GYRO_RES_PER_DEG_S;
	dualsense_device->calib_data[DualSenseCalibIndex::YAW].sens_denom = yaw_plus - yaw_minus;

	dualsense_device->calib_data[DualSenseCalibIndex::ROLL].sens_numer = gyro_speed_scale * DUALSENSE_GYRO_RES_PER_DEG_S;
	dualsense_device->calib_data[DualSenseCalibIndex::ROLL].sens_denom = roll_plus - roll_minus;

	const s16 accel_x_plus  = read_s16(&buf[23]);
	const s16 accel_x_minus = read_s16(&buf[25]);
	const s16 accel_y_plus  = read_s16(&buf[27]);
	const s16 accel_y_minus = read_s16(&buf[29]);
	const s16 accel_z_plus  = read_s16(&buf[31]);
	const s16 accel_z_minus = read_s16(&buf[33]);

	const s32 accel_x_range = accel_x_plus - accel_x_minus;
	const s32 accel_y_range = accel_y_plus - accel_y_minus;
	const s32 accel_z_range = accel_z_plus - accel_z_minus;

	dualsense_device->calib_data[DualSenseCalibIndex::X].bias       = accel_x_plus - accel_x_range / 2;
	dualsense_device->calib_data[DualSenseCalibIndex::X].sens_numer = 2 * DUALSENSE_ACC_RES_PER_G;
	dualsense_device->calib_data[DualSenseCalibIndex::X].sens_denom = accel_x_range;

	dualsense_device->calib_data[DualSenseCalibIndex::Y].bias       = accel_y_plus - accel_y_range / 2;
	dualsense_device->calib_data[DualSenseCalibIndex::Y].sens_numer = 2 * DUALSENSE_ACC_RES_PER_G;
	dualsense_device->calib_data[DualSenseCalibIndex::Y].sens_denom = accel_y_range;

	dualsense_device->calib_data[DualSenseCalibIndex::Z].bias       = accel_z_plus - accel_z_range / 2;
	dualsense_device->calib_data[DualSenseCalibIndex::Z].sens_numer = 2 * DUALSENSE_ACC_RES_PER_G;
	dualsense_device->calib_data[DualSenseCalibIndex::Z].sens_denom = accel_z_range;

	// Make sure data 'looks' valid, dongle will report invalid calibration data with no controller connected

	for (const auto& data : dualsense_device->calib_data)
	{
		if (data.sens_denom == 0)
		{
			dualsense_log.error("get_calibration_data: Failure: sens_denom == 0");
			return false;
		}
	}

	return true;
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
			if (!dualsense_dev->has_calib_data)
				dualsense_dev->has_calib_data = get_calibration_data(dualsense_dev);
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

void dualsense_pad_handler::get_extended_info(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	auto dualsense_device = std::static_pointer_cast<DualSenseDevice>(device);
	if (!dualsense_device || !pad)
		return;

	auto buf = dualsense_device->padData;

	//pad->m_battery_level = dualsense_device->batteryLevel;
	//pad->m_cable_state   = dualsense_device->cableState;

	// these values come already calibrated, all we need to do is convert to ds3 range

	// gyroX is yaw, which is all that we need
	f32 gyroX = static_cast<s16>((buf[16] << 8) | buf[15]) / static_cast<f32>(DUALSENSE_GYRO_RES_PER_DEG_S) * -1;
	//const int gyroY = ((u16)(buf[18] << 8) | buf[17]) / 256;
	//const int gyroZ = ((u16)(buf[20] << 8) | buf[19]) / 256;

	// accel
	f32 accelX = static_cast<s16>((buf[22] << 8) | buf[21]) / static_cast<f32>(DUALSENSE_ACC_RES_PER_G) * -1;
	f32 accelY = static_cast<s16>((buf[24] << 8) | buf[23]) / static_cast<f32>(DUALSENSE_ACC_RES_PER_G) * -1;
	f32 accelZ = static_cast<s16>((buf[26] << 8) | buf[25]) / static_cast<f32>(DUALSENSE_ACC_RES_PER_G) * -1;

	// now just use formula from ds3
	accelX = accelX * 113 + 512;
	accelY = accelY * 113 + 512;
	accelZ = accelZ * 113 + 512;

	// convert to ds3
	gyroX  = gyroX * (123.f / 90.f) + 512;

	pad->m_sensors[0].m_value = Clamp0To1023(accelX);
	pad->m_sensors[1].m_value = Clamp0To1023(accelY);
	pad->m_sensors[2].m_value = Clamp0To1023(accelZ);
	pad->m_sensors[3].m_value = Clamp0To1023(gyroX);
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
		//keyBuffer[DualSenseKeyCodes::L2]      = ((data & 0x04) != 0) ? 255 : 0; // active when L2 is pressed
		//keyBuffer[DualSenseKeyCodes::R2]      = ((data & 0x08) != 0) ? 255 : 0; // active when R2 is pressed
		keyBuffer[DualSenseKeyCodes::Share]   = ((data & 0x10) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::Options] = ((data & 0x20) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::L3]      = ((data & 0x40) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::R3]      = ((data & 0x80) != 0) ? 255 : 0;

		keyBuffer[DualSenseKeyCodes::L2] = buf[7];
		keyBuffer[DualSenseKeyCodes::R2] = buf[8];

		data = buf[6];
		keyBuffer[DualSenseKeyCodes::PSButton] = ((data & 0x01) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::TouchPad] = ((data & 0x02) != 0) ? 255 : 0;
		keyBuffer[DualSenseKeyCodes::Mic]      = ((data & 0x04) != 0) ? 255 : 0;

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
	//keyBuffer[DualSenseKeyCodes::L2]      = ((data & 0x04) != 0) ? 255 : 0; // active when L2 is pressed
	//keyBuffer[DualSenseKeyCodes::R2]      = ((data & 0x08) != 0) ? 255 : 0; // active when R2 is pressed
	keyBuffer[DualSenseKeyCodes::Share]   = ((data & 0x10) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::Options] = ((data & 0x20) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::L3]      = ((data & 0x40) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::R3]      = ((data & 0x80) != 0) ? 255 : 0;

	keyBuffer[DualSenseKeyCodes::L2] = buf[4];
	keyBuffer[DualSenseKeyCodes::R2] = buf[5];

	data = buf[9];
	keyBuffer[DualSenseKeyCodes::PSButton] = ((data & 0x01) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::TouchPad] = ((data & 0x02) != 0) ? 255 : 0;
	keyBuffer[DualSenseKeyCodes::Mic]      = ((data & 0x04) != 0) ? 255 : 0;

	return keyBuffer;
}

pad_preview_values dualsense_pad_handler::get_preview_values(const std::unordered_map<u64, u16>& data)
{
	return {
		data.at(L2),
		data.at(R2),
		data.at(LSXPos) - data.at(LSXNeg),
		data.at(LSYPos) - data.at(LSYNeg),
		data.at(RSXPos) - data.at(RSXNeg),
		data.at(RSYPos) - data.at(RSYNeg)
	};
}

std::shared_ptr<dualsense_pad_handler::DualSenseDevice> dualsense_pad_handler::GetDualSenseDevice(const std::string& padId)
{
	if (!Init())
		return nullptr;

	usz pos = padId.find(m_name_string);
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
			send_output_report(controller.second);

			hid_close(controller.second->hidDevice);
		}
	}
	if (hid_exit() != 0)
	{
		dualsense_log.error("hid_exit failed!");
	}
}

int dualsense_pad_handler::send_output_report(const std::shared_ptr<DualSenseDevice>& device)
{
	if (!device)
		return -2;

	auto config = device->config;
	if (config == nullptr)
		return -2; // hid_write and hid_write_control return -1 on error

	output_report_common common{};
	common.valid_flag_0 |= 0x01; // Enable haptics
	common.valid_flag_0 |= 0x02; // Enable vibration
	common.motor_left  = device->largeVibrate;
	common.motor_right = device->smallVibrate;

	if (device->btCon)
	{
		const u8 seq_tag = (device->bt_sequence << 4) | 0x0;
		if (++device->bt_sequence >= 16) device->bt_sequence = 0;

		output_report_bt report{};
		report.report_id = 0x31; // report id for bluetooth
		report.seq_tag   = seq_tag;
		report.tag       = 0x10; // magic number
		report.common    = std::move(common);

		const u8 btHdr    = 0xA2;
		const u32 crcHdr  = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
		const u32 crcCalc = CRCPP::CRC::Calculate(&report.report_id, (DUALSENSE_BLUETOOTH_REPORT_SIZE - 4), crcTable, crcHdr);

		report.crc32[0] = (crcCalc >> 0) & 0xFF;
		report.crc32[1] = (crcCalc >> 8) & 0xFF;
		report.crc32[2] = (crcCalc >> 16) & 0xFF;
		report.crc32[3] = (crcCalc >> 24) & 0xFF;

		return hid_write(device->hidDevice, &report.report_id, DUALSENSE_BLUETOOTH_REPORT_SIZE);
	}
	else
	{
		output_report_usb report{};
		report.report_id = 0x02; // report id for usb
		report.common    = std::move(common);

		return hid_write(device->hidDevice, &report.report_id, DUALSENSE_USB_REPORT_SIZE);
	}
}

void dualsense_pad_handler::apply_pad_data(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	auto dualsense_dev = std::static_pointer_cast<DualSenseDevice>(device);
	if (!dualsense_dev || !pad)
		return;

	auto config = dualsense_dev->config;

	// Attempt to send rumble no matter what
	const int idx_l = config->switch_vibration_motors ? 1 : 0;
	const int idx_s  = config->switch_vibration_motors ? 0 : 1;

	const int speed_large = config->enable_vibration_motor_large ? pad->m_vibrateMotors[idx_l].m_value : vibration_min;
	const int speed_small = config->enable_vibration_motor_small ? pad->m_vibrateMotors[idx_s].m_value : vibration_min;

	dualsense_dev->newVibrateData |= dualsense_dev->largeVibrate != speed_large || dualsense_dev->smallVibrate != speed_small;

	dualsense_dev->largeVibrate = speed_large;
	dualsense_dev->smallVibrate = speed_small;

	if (dualsense_dev->newVibrateData)
	{
		if (send_output_report(dualsense_dev) >= 0)
		{
			dualsense_dev->newVibrateData = false;
		}
	}
}

void dualsense_pad_handler::SetPadData(const std::string& padId, u32 largeMotor, u32 smallMotor, s32 r, s32 g, s32 b, bool /*battery_led*/, u32 /*battery_led_brightness*/)
{
	std::shared_ptr<DualSenseDevice> device = GetDualSenseDevice(padId);
	if (device == nullptr || device->hidDevice == nullptr)
		return;

	// Set the device's motor speeds to our requested values 0-255
	device->largeVibrate = largeMotor;
	device->smallVibrate = smallMotor;

	int index = 0;
	for (uint i = 0; i < MAX_GAMEPADS; i++)
	{
		if (g_cfg_input.player[i]->handler == m_type)
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

	// TODO: Set new LED color (see ds4_pad_handler)

	if (r >= 0 && g >= 0 && b >= 0 && r <= 255 && g <= 255 && b <= 255)
	{
		device->config->colorR.set(r);
		device->config->colorG.set(g);
		device->config->colorB.set(b);
	}

	// Start/Stop the engines :)
	send_output_report(device);
}
