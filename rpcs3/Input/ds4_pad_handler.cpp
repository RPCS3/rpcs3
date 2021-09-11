#include "stdafx.h"
#include "ds4_pad_handler.h"
#include "Emu/Io/pad_config.h"

LOG_CHANNEL(ds4_log, "DS4");

constexpr u16 DS4_VID = 0x054C;
constexpr u16 DS4_PID_0 = 0xBA0;
constexpr u16 DS4_PID_1 = 0x5C4;
constexpr u16 DS4_PID_2 = 0x09CC;

namespace
{
	constexpr u32 DS4_ACC_RES_PER_G = 8192;
	constexpr u32 DS4_GYRO_RES_PER_DEG_S = 16; // technically this could be 1024, but keeping it at 16 keeps us within 16 bits of precision
	constexpr u32 DS4_FEATURE_REPORT_0x02_SIZE = 37;
	constexpr u32 DS4_FEATURE_REPORT_0x05_SIZE = 41;
	constexpr u32 DS4_FEATURE_REPORT_0x12_SIZE = 16;
	constexpr u32 DS4_FEATURE_REPORT_0x81_SIZE = 7;
	constexpr u32 DS4_INPUT_REPORT_0x11_SIZE = 78;
	constexpr u32 DS4_OUTPUT_REPORT_0x05_SIZE = 32;
	constexpr u32 DS4_OUTPUT_REPORT_0x11_SIZE = 78;
	constexpr u32 DS4_INPUT_REPORT_GYRO_X_OFFSET = 13;
	constexpr u32 DS4_INPUT_REPORT_BATTERY_OFFSET = 30;

	// This tries to convert axis to give us the max even in the corners,
	// this actually might work 'too' well, we end up actually getting diagonals of actual max/min, we need the corners still a bit rounded to match ds3
	// im leaving it here for now, and future reference as it probably can be used later
	//taken from http://theinstructionlimit.com/squaring-the-thumbsticks
	/*std::tuple<u16, u16> ConvertToSquarePoint(u16 inX, u16 inY, u32 innerRoundness = 0) {
		// convert inX and Y to a (-1, 1) vector;
		const f32 x = (inX - 127) / 127.f;
		const f32 y = ((inY - 127) / 127.f) * -1;

		f32 outX, outY;
		const f32 piOver4 = M_PI / 4;
		const f32 angle = std::atan2(y, x) + M_PI;
		// x+ wall
		if (angle <= piOver4 || angle > 7 * piOver4) {
			outX = x * (f32)(1 / std::cos(angle));
			outY = y * (f32)(1 / std::cos(angle));
		}
		// y+ wall
		else if (angle > piOver4 && angle <= 3 * piOver4) {
			outX = x * (f32)(1 / std::sin(angle));
			outY = y * (f32)(1 / std::sin(angle));
		}
		// x- wall
		else if (angle > 3 * piOver4 && angle <= 5 * piOver4) {
			outX = x * (f32)(-1 / std::cos(angle));
			outY = y * (f32)(-1 / std::cos(angle));
		}
		// y- wall
		else if (angle > 5 * piOver4 && angle <= 7 * piOver4) {
			outX = x * (f32)(-1 / std::sin(angle));
			outY = y * (f32)(-1 / std::sin(angle));
		}
		else fmt::throw_exception("invalid angle in convertToSquarePoint");

		if (innerRoundness == 0)
			return std::tuple<u16, u16>(Clamp0To255((outX + 1) * 127.f), Clamp0To255(((outY * -1) + 1) * 127.f));

		const f32 len = std::sqrt(std::pow(x, 2) + std::pow(y, 2));
		const f32 factor = std::pow(len, innerRoundness);

		outX = (1 - factor) * x + factor * outX;
		outY = (1 - factor) * y + factor * outY;

		return std::tuple<u16, u16>(Clamp0To255((outX + 1) * 127.f), Clamp0To255(((outY * -1) + 1) * 127.f));
	}*/
}

ds4_pad_handler::ds4_pad_handler()
    : hid_pad_handler<DS4Device>(pad_handler::ds4, DS4_VID, {DS4_PID_0, DS4_PID_1, DS4_PID_2})
{
	// Unique names for the config files and our pad settings dialog
	button_list =
	{
		{ DS4KeyCodes::None,     "" },
		{ DS4KeyCodes::Triangle, "Triangle" },
		{ DS4KeyCodes::Circle,   "Circle" },
		{ DS4KeyCodes::Cross,    "Cross" },
		{ DS4KeyCodes::Square,   "Square" },
		{ DS4KeyCodes::Left,     "Left" },
		{ DS4KeyCodes::Right,    "Right" },
		{ DS4KeyCodes::Up,       "Up" },
		{ DS4KeyCodes::Down,     "Down" },
		{ DS4KeyCodes::R1,       "R1" },
		{ DS4KeyCodes::R2,       "R2" },
		{ DS4KeyCodes::R3,       "R3" },
		{ DS4KeyCodes::Options,  "Options" },
		{ DS4KeyCodes::Share,    "Share" },
		{ DS4KeyCodes::PSButton, "PS Button" },
		{ DS4KeyCodes::TouchPad, "Touch Pad" },
		{ DS4KeyCodes::L1,       "L1" },
		{ DS4KeyCodes::L2,       "L2" },
		{ DS4KeyCodes::L3,       "L3" },
		{ DS4KeyCodes::LSXNeg,   "LS X-" },
		{ DS4KeyCodes::LSXPos,   "LS X+" },
		{ DS4KeyCodes::LSYPos,   "LS Y+" },
		{ DS4KeyCodes::LSYNeg,   "LS Y-" },
		{ DS4KeyCodes::RSXNeg,   "RS X-" },
		{ DS4KeyCodes::RSXPos,   "RS X+" },
		{ DS4KeyCodes::RSYPos,   "RS Y+" },
		{ DS4KeyCodes::RSYNeg,   "RS Y-" }
	};

	init_configs();

	// Define border values
	thumb_max = 255;
	trigger_min = 0;
	trigger_max = 255;
	vibration_min = 0;
	vibration_max = 255;

	// set capabilities
	b_has_config = true;
	b_has_rumble = true;
	b_has_deadzones = true;
	b_has_led = true;
	b_has_rgb = true;
	b_has_battery = true;

	m_name_string = "DS4 Pad #";
	m_max_devices = CELL_PAD_MAX_PORT_NUM;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
}

void ds4_pad_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
	cfg->ls_left.def  = button_list.at(DS4KeyCodes::LSXNeg);
	cfg->ls_down.def  = button_list.at(DS4KeyCodes::LSYNeg);
	cfg->ls_right.def = button_list.at(DS4KeyCodes::LSXPos);
	cfg->ls_up.def    = button_list.at(DS4KeyCodes::LSYPos);
	cfg->rs_left.def  = button_list.at(DS4KeyCodes::RSXNeg);
	cfg->rs_down.def  = button_list.at(DS4KeyCodes::RSYNeg);
	cfg->rs_right.def = button_list.at(DS4KeyCodes::RSXPos);
	cfg->rs_up.def    = button_list.at(DS4KeyCodes::RSYPos);
	cfg->start.def    = button_list.at(DS4KeyCodes::Options);
	cfg->select.def   = button_list.at(DS4KeyCodes::Share);
	cfg->ps.def       = button_list.at(DS4KeyCodes::PSButton);
	cfg->square.def   = button_list.at(DS4KeyCodes::Square);
	cfg->cross.def    = button_list.at(DS4KeyCodes::Cross);
	cfg->circle.def   = button_list.at(DS4KeyCodes::Circle);
	cfg->triangle.def = button_list.at(DS4KeyCodes::Triangle);
	cfg->left.def     = button_list.at(DS4KeyCodes::Left);
	cfg->down.def     = button_list.at(DS4KeyCodes::Down);
	cfg->right.def    = button_list.at(DS4KeyCodes::Right);
	cfg->up.def       = button_list.at(DS4KeyCodes::Up);
	cfg->r1.def       = button_list.at(DS4KeyCodes::R1);
	cfg->r2.def       = button_list.at(DS4KeyCodes::R2);
	cfg->r3.def       = button_list.at(DS4KeyCodes::R3);
	cfg->l1.def       = button_list.at(DS4KeyCodes::L1);
	cfg->l2.def       = button_list.at(DS4KeyCodes::L2);
	cfg->l3.def       = button_list.at(DS4KeyCodes::L3);

	cfg->pressure_intensity_button.def = button_list.at(DS4KeyCodes::None);

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
	cfg->led_battery_indicator.def = false;
	cfg->led_battery_indicator_brightness.def = 10;
	cfg->led_low_battery_blink.def = true;

	// apply defaults
	cfg->from_default();
}

u32 ds4_pad_handler::get_battery_level(const std::string& padId)
{
	const std::shared_ptr<DS4Device> device = get_hid_device(padId);
	if (device == nullptr || device->hidDevice == nullptr)
	{
		return 0;
	}
	return std::min<u32>(device->battery_level * 10, 100);
}

void ds4_pad_handler::SetPadData(const std::string& padId, u8 player_id, u32 largeMotor, u32 smallMotor, s32 r, s32 g, s32 b, bool battery_led, u32 battery_led_brightness)
{
	std::shared_ptr<DS4Device> device = get_hid_device(padId);
	if (!device || !device->hidDevice || !device->config)
		return;

	// Set the device's motor speeds to our requested values 0-255
	device->large_motor = largeMotor;
	device->small_motor = smallMotor;
	device->player_id = player_id;

	int index = 0;
	for (uint i = 0; i < MAX_GAMEPADS; i++)
	{
		if (g_cfg_input.player[i]->handler == m_type)
		{
			if (g_cfg_input.player[i]->device.to_string() == padId)
			{
				m_pad_configs[index].from_string(g_cfg_input.player[i]->config.to_string());
				device->config = &m_pad_configs[index];
				break;
			}
			index++;
		}
	}

	ensure(device->config);

	// Set new LED color
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

	// Start/Stop the engines :)
	send_output_report(device.get());
}

std::unordered_map<u64, u16> ds4_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> keyBuffer;
	DS4Device* ds4_dev = static_cast<DS4Device*>(device.get());
	if (!ds4_dev)
		return keyBuffer;

	auto buf = ds4_dev->padData;

	// Left Stick X Axis
	keyBuffer[DS4KeyCodes::LSXNeg] = Clamp0To255((127.5f - buf[1]) * 2.0f);
	keyBuffer[DS4KeyCodes::LSXPos] = Clamp0To255((buf[1] - 127.5f) * 2.0f);

	// Left Stick Y Axis (Up is the negative for some reason)
	keyBuffer[DS4KeyCodes::LSYNeg] = Clamp0To255((buf[2] - 127.5f) * 2.0f);
	keyBuffer[DS4KeyCodes::LSYPos] = Clamp0To255((127.5f - buf[2]) * 2.0f);

	// Right Stick X Axis
	keyBuffer[DS4KeyCodes::RSXNeg] = Clamp0To255((127.5f - buf[3]) * 2.0f);
	keyBuffer[DS4KeyCodes::RSXPos] = Clamp0To255((buf[3] - 127.5f) * 2.0f);

	// Right Stick Y Axis (Up is the negative for some reason)
	keyBuffer[DS4KeyCodes::RSYNeg] = Clamp0To255((buf[4] - 127.5f) * 2.0f);
	keyBuffer[DS4KeyCodes::RSYPos] = Clamp0To255((127.5f - buf[4]) * 2.0f);

	// bleh, dpad in buffer is stored in a different state
	const u8 dpadState = buf[5] & 0xf;
	switch (dpadState)
	{
	case 0x08: // none pressed
		keyBuffer[DS4KeyCodes::Up] = 0;
		keyBuffer[DS4KeyCodes::Down] = 0;
		keyBuffer[DS4KeyCodes::Left] = 0;
		keyBuffer[DS4KeyCodes::Right] = 0;
		break;
	case 0x07: // NW...left and up
		keyBuffer[DS4KeyCodes::Up] = 255;
		keyBuffer[DS4KeyCodes::Down] = 0;
		keyBuffer[DS4KeyCodes::Left] = 255;
		keyBuffer[DS4KeyCodes::Right] = 0;
		break;
	case 0x06: // W..left
		keyBuffer[DS4KeyCodes::Up] = 0;
		keyBuffer[DS4KeyCodes::Down] = 0;
		keyBuffer[DS4KeyCodes::Left] = 255;
		keyBuffer[DS4KeyCodes::Right] = 0;
		break;
	case 0x05: // SW..left down
		keyBuffer[DS4KeyCodes::Up] = 0;
		keyBuffer[DS4KeyCodes::Down] = 255;
		keyBuffer[DS4KeyCodes::Left] = 255;
		keyBuffer[DS4KeyCodes::Right] = 0;
		break;
	case 0x04: // S..down
		keyBuffer[DS4KeyCodes::Up] = 0;
		keyBuffer[DS4KeyCodes::Down] = 255;
		keyBuffer[DS4KeyCodes::Left] = 0;
		keyBuffer[DS4KeyCodes::Right] = 0;
		break;
	case 0x03: // SE..down and right
		keyBuffer[DS4KeyCodes::Up] = 0;
		keyBuffer[DS4KeyCodes::Down] = 255;
		keyBuffer[DS4KeyCodes::Left] = 0;
		keyBuffer[DS4KeyCodes::Right] = 255;
		break;
	case 0x02: // E... right
		keyBuffer[DS4KeyCodes::Up] = 0;
		keyBuffer[DS4KeyCodes::Down] = 0;
		keyBuffer[DS4KeyCodes::Left] = 0;
		keyBuffer[DS4KeyCodes::Right] = 255;
		break;
	case 0x01: // NE.. up right
		keyBuffer[DS4KeyCodes::Up] = 255;
		keyBuffer[DS4KeyCodes::Down] = 0;
		keyBuffer[DS4KeyCodes::Left] = 0;
		keyBuffer[DS4KeyCodes::Right] = 255;
		break;
	case 0x00: // n.. up
		keyBuffer[DS4KeyCodes::Up] = 255;
		keyBuffer[DS4KeyCodes::Down] = 0;
		keyBuffer[DS4KeyCodes::Left] = 0;
		keyBuffer[DS4KeyCodes::Right] = 0;
		break;
	default:
		fmt::throw_exception("ds4 dpad state encountered unexpected input");
	}

	// square, cross, circle, triangle
	keyBuffer[DS4KeyCodes::Square] =   ((buf[5] & (1 << 4)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::Cross] =    ((buf[5] & (1 << 5)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::Circle] =   ((buf[5] & (1 << 6)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::Triangle] = ((buf[5] & (1 << 7)) != 0) ? 255 : 0;

	// L1, R1, L2, L3, select, start, L3, L3
	keyBuffer[DS4KeyCodes::L1]      = ((buf[6] & (1 << 0)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::R1]      = ((buf[6] & (1 << 1)) != 0) ? 255 : 0;
	//keyBuffer[DS4KeyCodes::L2But]   = ((buf[6] & (1 << 2)) != 0) ? 255 : 0;
	//keyBuffer[DS4KeyCodes::R2But]   = ((buf[6] & (1 << 3)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::Share]   = ((buf[6] & (1 << 4)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::Options] = ((buf[6] & (1 << 5)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::L3]      = ((buf[6] & (1 << 6)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::R3]      = ((buf[6] & (1 << 7)) != 0) ? 255 : 0;

	// PS Button, Touch Button
	keyBuffer[DS4KeyCodes::PSButton] = ((buf[7] & (1 << 0)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::TouchPad] = ((buf[7] & (1 << 1)) != 0) ? 255 : 0;

	// L2, R2
	keyBuffer[DS4KeyCodes::L2] = buf[8];
	keyBuffer[DS4KeyCodes::R2] = buf[9];

	return keyBuffer;
}

pad_preview_values ds4_pad_handler::get_preview_values(const std::unordered_map<u64, u16>& data)
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

bool ds4_pad_handler::GetCalibrationData(DS4Device* ds4Dev) const
{
	if (!ds4Dev || !ds4Dev->hidDevice)
	{
		ds4_log.error("GetCalibrationData called with null device");
		return false;
	}

	std::array<u8, 64> buf;
	if (ds4Dev->bt_controller)
	{
		for (int tries = 0; tries < 3; ++tries)
		{
			buf[0] = 0x05;

			if (hid_get_feature_report(ds4Dev->hidDevice, buf.data(), DS4_FEATURE_REPORT_0x05_SIZE) <= 0)
			{
				ds4_log.error("GetCalibrationData: hid_get_feature_report 0x05 failed! Reason: %s", hid_error(ds4Dev->hidDevice));
				return false;
			}

			const u8 btHdr = 0xA3;
			const u32 crcHdr = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
			const u32 crcCalc = CRCPP::CRC::Calculate(buf.data(), (DS4_FEATURE_REPORT_0x05_SIZE - 4), crcTable, crcHdr);
			const u32 crcReported = read_u32(&buf[DS4_FEATURE_REPORT_0x05_SIZE - 4]);

			if (crcCalc == crcReported)
				break;

			ds4_log.warning("Calibration CRC check failed! Will retry up to 3 times. Received 0x%x, Expected 0x%x", crcReported, crcCalc);

			if (tries == 2)
			{
				ds4_log.error("Calibration CRC check failed too many times!");
				return false;
			}
		}
	}
	else
	{
		buf[0] = 0x02;
		if (hid_get_feature_report(ds4Dev->hidDevice, buf.data(), DS4_FEATURE_REPORT_0x02_SIZE) <= 0)
		{
			ds4_log.error("GetCalibrationData: hid_get_feature_report 0x02 failed! Reason: %s", hid_error(ds4Dev->hidDevice));
			return false;
		}
	}

	ds4Dev->calib_data[CalibIndex::PITCH].bias = read_s16(&buf[1]);
	ds4Dev->calib_data[CalibIndex::YAW].bias   = read_s16(&buf[3]);
	ds4Dev->calib_data[CalibIndex::ROLL].bias  = read_s16(&buf[5]);

	s16 pitchPlus, pitchNeg, rollPlus, rollNeg, yawPlus, yawNeg;

	// Check for calibration data format
	// It's going to be either alternating +/- or +++---
	if (read_s16(&buf[9]) < 0 && read_s16(&buf[7]) > 0)
	{
		// Wired mode for OEM controllers
		pitchPlus = read_s16(&buf[7]);
		pitchNeg  = read_s16(&buf[9]);
		yawPlus   = read_s16(&buf[11]);
		yawNeg    = read_s16(&buf[13]);
		rollPlus  = read_s16(&buf[15]);
		rollNeg   = read_s16(&buf[17]);
	}
	else
	{
		// Bluetooth mode and wired mode for some 3rd party controllers
		pitchPlus = read_s16(&buf[7]);
		yawPlus   = read_s16(&buf[9]);
		rollPlus  = read_s16(&buf[11]);
		pitchNeg  = read_s16(&buf[13]);
		yawNeg    = read_s16(&buf[15]);
		rollNeg   = read_s16(&buf[17]);
	}

	// Confirm correctness. Need confirmation with dongle with no active controller
	if (pitchPlus <= 0 || yawPlus <= 0 || rollPlus <= 0 ||
		pitchNeg >= 0 || yawNeg >= 0 || rollNeg >= 0)
	{
		ds4_log.error("GetCalibrationData: calibration data check failed! pitchPlus=%d, pitchNeg=%d, rollPlus=%d, rollNeg=%d, yawPlus=%d, yawNeg=%d", pitchPlus, pitchNeg, rollPlus, rollNeg, yawPlus, yawNeg);
		return false;
	}

	const s32 gyroSpeedScale = read_s16(&buf[19]) + read_s16(&buf[21]);

	ds4Dev->calib_data[CalibIndex::PITCH].sens_numer = gyroSpeedScale * DS4_GYRO_RES_PER_DEG_S;
	ds4Dev->calib_data[CalibIndex::PITCH].sens_denom = pitchPlus - pitchNeg;

	ds4Dev->calib_data[CalibIndex::YAW].sens_numer = gyroSpeedScale * DS4_GYRO_RES_PER_DEG_S;
	ds4Dev->calib_data[CalibIndex::YAW].sens_denom = yawPlus - yawNeg;

	ds4Dev->calib_data[CalibIndex::ROLL].sens_numer = gyroSpeedScale * DS4_GYRO_RES_PER_DEG_S;
	ds4Dev->calib_data[CalibIndex::ROLL].sens_denom = rollPlus - rollNeg;

	const s16 accelXPlus = read_s16(&buf[23]);
	const s16 accelXNeg  = read_s16(&buf[25]);
	const s16 accelYPlus = read_s16(&buf[27]);
	const s16 accelYNeg  = read_s16(&buf[29]);
	const s16 accelZPlus = read_s16(&buf[31]);
	const s16 accelZNeg  = read_s16(&buf[33]);

	const s32 accelXRange = accelXPlus - accelXNeg;
	ds4Dev->calib_data[CalibIndex::X].bias       = accelXPlus - accelXRange / 2;
	ds4Dev->calib_data[CalibIndex::X].sens_numer = 2 * DS4_ACC_RES_PER_G;
	ds4Dev->calib_data[CalibIndex::X].sens_denom = accelXRange;

	const s32 accelYRange = accelYPlus - accelYNeg;
	ds4Dev->calib_data[CalibIndex::Y].bias       = accelYPlus - accelYRange / 2;
	ds4Dev->calib_data[CalibIndex::Y].sens_numer = 2 * DS4_ACC_RES_PER_G;
	ds4Dev->calib_data[CalibIndex::Y].sens_denom = accelYRange;

	const s32 accelZRange = accelZPlus - accelZNeg;
	ds4Dev->calib_data[CalibIndex::Z].bias       = accelZPlus - accelZRange / 2;
	ds4Dev->calib_data[CalibIndex::Z].sens_numer = 2 * DS4_ACC_RES_PER_G;
	ds4Dev->calib_data[CalibIndex::Z].sens_denom = accelZRange;

	// Make sure data 'looks' valid, dongle will report invalid calibration data with no controller connected

	for (const auto& data : ds4Dev->calib_data)
	{
		if (data.sens_denom == 0)
		{
			ds4_log.error("GetCalibrationData: Failure: sens_denom == 0");
			return false;
		}
	}

	return true;
}

void ds4_pad_handler::check_add_device(hid_device* hidDevice, std::string_view path, std::wstring_view wide_serial)
{
	if (!hidDevice)
	{
		return;
	}

	DS4Device* device = nullptr;

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

	// There isnt a nice 'portable' way with hidapi to detect bt vs wired as the pid/vid's are the same
	// Let's try getting 0x81 feature report, which should will return mac address on wired, and should error on bluetooth
	std::array<u8, 64> buf{};
	buf[0] = 0x81;
	if (const auto bytes_read = hid_get_feature_report(hidDevice, buf.data(), DS4_FEATURE_REPORT_0x81_SIZE); bytes_read > 0)
	{
		if (bytes_read != DS4_FEATURE_REPORT_0x81_SIZE)
		{
			// Controller may not be genuine. These controllers do not have feature 0x81 implemented and calibration data is in bluetooth format even in USB mode!
			ds4_log.warning("check_add_device: DS4 controller may not be genuine. Workaround enabled.");

			// Read feature report 0x12 instead which is what the console uses.
			buf[0] = 0x12;
			buf[1] = 0;
			if (hid_get_feature_report(hidDevice, buf.data(), DS4_FEATURE_REPORT_0x12_SIZE) == -1)
			{
				ds4_log.error("check_add_device: hid_get_feature_report 0x12 failed! Reason: %s", hid_error(hidDevice));
			}
		}

		serial = fmt::format("%x%x%x%x%x%x", buf[6], buf[5], buf[4], buf[3], buf[2], buf[1]);
	}
	else
	{
		device->bt_controller = true;
		for (wchar_t ch : wide_serial)
			serial += static_cast<uchar>(ch);
	}

	device->hidDevice = hidDevice;

	if (!GetCalibrationData(device))
	{
		ds4_log.error("check_add_device: GetCalibrationData failed!");
		hid_close(hidDevice);
		device->hidDevice = nullptr;
		return;
	}

	if (hid_set_nonblocking(hidDevice, 1) == -1)
	{
		ds4_log.error("check_add_device: hid_set_nonblocking failed! Reason: %s", hid_error(hidDevice));
		hid_close(hidDevice);
		device->hidDevice = nullptr;
		return;
	}

	device->has_calib_data = true;
	device->path           = path;

	send_output_report(device);

	ds4_log.notice("Added device: bluetooth=%d, serial='%s', path='%s'", device->bt_controller, serial, device->path);
}

ds4_pad_handler::~ds4_pad_handler()
{
	for (auto& controller : m_controllers)
	{
		if (controller.second && controller.second->hidDevice)
		{
			// Disable blinking and vibration
			controller.second->small_motor = 0;
			controller.second->large_motor = 0;
			controller.second->led_delay_on = 0;
			controller.second->led_delay_off = 0;
			send_output_report(controller.second.get());
		}
	}
}

int ds4_pad_handler::send_output_report(DS4Device* device)
{
	if (!device || !device->hidDevice)
		return -2;

	const auto config = device->config;
	if (config == nullptr)
		return -2; // hid_write and hid_write_control return -1 on error

	std::array<u8, 78> outputBuf{0};
	// write rumble state
	if (device->bt_controller)
	{
		outputBuf[0] = 0x11;
		outputBuf[1] = 0xC4;
		outputBuf[3] = 0x07;
		outputBuf[6] = device->small_motor;
		outputBuf[7] = device->large_motor;
		outputBuf[8]  = config->colorR; // red
		outputBuf[9]  = config->colorG; // green
		outputBuf[10] = config->colorB; // blue

		// alternating blink states with values 0-255: only setting both to zero disables blinking
		// 255 is roughly 2 seconds, so setting both values to 255 results in a 4 second interval
		// using something like (0,10) will heavily blink, while using (0, 255) will be slow. you catch the drift
		outputBuf[11] = device->led_delay_on;
		outputBuf[12] = device->led_delay_off;

		const u8 btHdr = 0xA2;
		const u32 crcHdr = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
		const u32 crcCalc = CRCPP::CRC::Calculate(outputBuf.data(), (DS4_OUTPUT_REPORT_0x11_SIZE - 4), crcTable, crcHdr);

		outputBuf[74] = (crcCalc >> 0) & 0xFF;
		outputBuf[75] = (crcCalc >> 8) & 0xFF;
		outputBuf[76] = (crcCalc >> 16) & 0xFF;
		outputBuf[77] = (crcCalc >> 24) & 0xFF;

		return hid_write_control(device->hidDevice, outputBuf.data(), DS4_OUTPUT_REPORT_0x11_SIZE);
	}
	else
	{
		outputBuf[0] = 0x05;
		outputBuf[1] = 0x07;
		outputBuf[4] = device->small_motor;
		outputBuf[5] = device->large_motor;
		outputBuf[6] = config->colorR; // red
		outputBuf[7] = config->colorG; // green
		outputBuf[8] = config->colorB; // blue
		outputBuf[9] = device->led_delay_on;
		outputBuf[10] = device->led_delay_off;

		return hid_write(device->hidDevice, outputBuf.data(), DS4_OUTPUT_REPORT_0x05_SIZE);
	}
}

ds4_pad_handler::DataStatus ds4_pad_handler::get_data(DS4Device* device)
{
	if (!device || !device->hidDevice)
		return DataStatus::ReadError;

	std::array<u8, 78> buf{};

	const int res = hid_read(device->hidDevice, buf.data(), device->bt_controller ? 78 : 64);
	if (res == -1)
	{
		// looks like controller disconnected or read error
		return DataStatus::ReadError;
	}

	// no data? keep going
	if (res == 0)
		return DataStatus::NoNewData;

	// bt controller sends this until 0x02 feature report is sent back (happens on controller init/restart)
	if (device->bt_controller && buf[0] == 0x1)
	{
		// tells controller to send 0x11 reports
		std::array<u8, 64> buf_error{};
		buf_error[0] = 0x2;
		if (hid_get_feature_report(device->hidDevice, buf_error.data(), buf_error.size()) == -1)
		{
			ds4_log.error("GetRawData: hid_get_feature_report 0x2 failed! Reason: %s", hid_error(device->hidDevice));
		}
		return DataStatus::NoNewData;
	}

	int offset;
	// check report and set offset
	if (device->bt_controller && buf[0] == 0x11 && res == 78)
	{
		offset = 2;

		const u8 btHdr = 0xA1;
		const u32 crcHdr = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
		const u32 crcCalc = CRCPP::CRC::Calculate(buf.data(), (DS4_INPUT_REPORT_0x11_SIZE - 4), crcTable, crcHdr);
		const u32 crcReported = read_u32(&buf[DS4_INPUT_REPORT_0x11_SIZE - 4]);
		if (crcCalc != crcReported)
		{
			ds4_log.warning("Data packet CRC check failed, ignoring! Received 0x%x, Expected 0x%x", crcReported, crcCalc);
			return DataStatus::NoNewData;
		}
	}
	else if (!device->bt_controller && buf[0] == 0x01 && res == 64)
	{
		// Ds4 Dongle uses this bit to actually report whether a controller is connected
		const bool connected = (buf[31] & 0x04) ? false : true;
		if (connected && !device->has_calib_data)
			device->has_calib_data = GetCalibrationData(device);

		offset = 0;
	}
	else
		return DataStatus::NoNewData;

	const int battery_offset = offset + DS4_INPUT_REPORT_BATTERY_OFFSET;
	device->cable_state = (buf[battery_offset] >> 4) & 0x01;
	device->battery_level = buf[battery_offset] & 0x0F; // 0 - 9 while unplugged, 0 - 10 while plugged in, 11 charge complete

	if (device->has_calib_data)
	{
		int calibOffset = offset + DS4_INPUT_REPORT_GYRO_X_OFFSET;
		for (int i = 0; i < CalibIndex::COUNT; ++i)
		{
			const s16 rawValue = read_s16(&buf[calibOffset]);
			const s16 calValue = apply_calibration(rawValue, device->calib_data[i]);
			buf[calibOffset++] = (static_cast<u16>(calValue) >> 0) & 0xFF;
			buf[calibOffset++] = (static_cast<u16>(calValue) >> 8) & 0xFF;
		}
	}
	memcpy(device->padData.data(), &buf[offset], 64);

	return DataStatus::NewData;
}

bool ds4_pad_handler::get_is_left_trigger(u64 keyCode)
{
	return keyCode == DS4KeyCodes::L2;
}

bool ds4_pad_handler::get_is_right_trigger(u64 keyCode)
{
	return keyCode == DS4KeyCodes::R2;
}

bool ds4_pad_handler::get_is_left_stick(u64 keyCode)
{
	switch (keyCode)
	{
	case DS4KeyCodes::LSXNeg:
	case DS4KeyCodes::LSXPos:
	case DS4KeyCodes::LSYPos:
	case DS4KeyCodes::LSYNeg:
		return true;
	default:
		return false;
	}
}

bool ds4_pad_handler::get_is_right_stick(u64 keyCode)
{
	switch (keyCode)
	{
	case DS4KeyCodes::RSXNeg:
	case DS4KeyCodes::RSXPos:
	case DS4KeyCodes::RSYPos:
	case DS4KeyCodes::RSYNeg:
		return true;
	default:
		return false;
	}
}

PadHandlerBase::connection ds4_pad_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	DS4Device* ds4_dev = static_cast<DS4Device*>(device.get());
	if (!ds4_dev || ds4_dev->path.empty())
		return connection::disconnected;

	if (ds4_dev->hidDevice == nullptr)
	{
		// try to reconnect
		hid_device* dev = hid_open_path(ds4_dev->path.c_str());
		if (dev)
		{
			if (hid_set_nonblocking(dev, 1) == -1)
			{
				ds4_log.error("Reconnecting Device %s: hid_set_nonblocking failed with error %s", ds4_dev->path, hid_error(dev));
			}
			ds4_dev->hidDevice = dev;
			if (!ds4_dev->has_calib_data)
				ds4_dev->has_calib_data = GetCalibrationData(ds4_dev);
		}
		else
		{
			// nope, not there
			return connection::disconnected;
		}
	}

	if (get_data(ds4_dev) == DataStatus::ReadError)
	{
		// this also can mean disconnected, either way deal with it on next loop and reconnect
		hid_close(ds4_dev->hidDevice);
		ds4_dev->hidDevice = nullptr;

		return connection::no_data;
	}

	return connection::connected;
}

void ds4_pad_handler::get_extended_info(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	DS4Device* ds4_device = static_cast<DS4Device*>(device.get());
	if (!ds4_device || !pad)
		return;

	auto buf = ds4_device->padData;

	pad->m_battery_level = ds4_device->battery_level;
	pad->m_cable_state   = ds4_device->cable_state;

	// these values come already calibrated, all we need to do is convert to ds3 range

	// accel
	f32 accelX = static_cast<s16>((buf[20] << 8) | buf[19]) / static_cast<f32>(DS4_ACC_RES_PER_G) * -1;
	f32 accelY = static_cast<s16>((buf[22] << 8) | buf[21]) / static_cast<f32>(DS4_ACC_RES_PER_G) * -1;
	f32 accelZ = static_cast<s16>((buf[24] << 8) | buf[23]) / static_cast<f32>(DS4_ACC_RES_PER_G) * -1;

	// now just use formula from ds3
	accelX = accelX * 113 + 512;
	accelY = accelY * 113 + 512;
	accelZ = accelZ * 113 + 512;

	pad->m_sensors[0].m_value = Clamp0To1023(accelX);
	pad->m_sensors[1].m_value = Clamp0To1023(accelY);
	pad->m_sensors[2].m_value = Clamp0To1023(accelZ);

	// gyroX is yaw, which is all that we need
	f32 gyroX = static_cast<s16>((buf[16] << 8) | buf[15]) / static_cast<f32>(DS4_GYRO_RES_PER_DEG_S) * -1;
	//const int gyroY = ((u16)(buf[14] << 8) | buf[13]) / 256;
	//const int gyroZ = ((u16)(buf[18] << 8) | buf[17]) / 256;

	// convert to ds3
	gyroX = gyroX * (123.f / 90.f) + 512;

	pad->m_sensors[3].m_value = Clamp0To1023(gyroX);
}

void ds4_pad_handler::apply_pad_data(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	DS4Device* ds4_dev = static_cast<DS4Device*>(device.get());
	if (!ds4_dev || !ds4_dev->hidDevice || !ds4_dev->config || !pad)
		return;

	cfg_pad* config = ds4_dev->config;

	// Attempt to send rumble no matter what
	const int idx_l = config->switch_vibration_motors ? 1 : 0;
	const int idx_s = config->switch_vibration_motors ? 0 : 1;

	const int speed_large = config->enable_vibration_motor_large ? pad->m_vibrateMotors[idx_l].m_value : vibration_min;
	const int speed_small = config->enable_vibration_motor_small ? pad->m_vibrateMotors[idx_s].m_value : vibration_min;

	const bool wireless    = ds4_dev->cable_state == 0;
	const bool low_battery = ds4_dev->battery_level < 2;
	const bool is_blinking = ds4_dev->led_delay_on > 0 || ds4_dev->led_delay_off > 0;

	// Blink LED when battery is low
	if (config->led_low_battery_blink)
	{
		// we are now wired or have okay battery level -> stop blinking
		if (is_blinking && !(wireless && low_battery))
		{
			ds4_dev->led_delay_on = 0;
			ds4_dev->led_delay_off = 0;
			ds4_dev->new_output_data = true;
		}
		// we are now wireless and low on battery -> blink
		else if (!is_blinking && wireless && low_battery)
		{
			ds4_dev->led_delay_on = 100;
			ds4_dev->led_delay_off = 100;
			ds4_dev->new_output_data = true;
		}
	}

	// Use LEDs to indicate battery level
	if (config->led_battery_indicator)
	{
		// This makes sure that the LED color doesn't update every 1ms. DS4 only reports battery level in 10% increments
		if (ds4_dev->last_battery_level != ds4_dev->battery_level)
		{
			const u32 combined_color = get_battery_color(ds4_dev->battery_level, config->led_battery_indicator_brightness);
			config->colorR.set(combined_color >> 8);
			config->colorG.set(combined_color & 0xff);
			config->colorB.set(0);
			ds4_dev->new_output_data = true;
			ds4_dev->last_battery_level = ds4_dev->battery_level;
		}
	}

	ds4_dev->new_output_data |= ds4_dev->large_motor != speed_large || ds4_dev->small_motor != speed_small;

	ds4_dev->large_motor = speed_large;
	ds4_dev->small_motor = speed_small;

	if (ds4_dev->new_output_data)
	{
		if (send_output_report(ds4_dev) >= 0)
		{
			ds4_dev->new_output_data = false;
		}
	}
}
