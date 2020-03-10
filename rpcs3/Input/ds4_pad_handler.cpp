#include "stdafx.h"
#include "ds4_pad_handler.h"
#include "Emu/Io/pad_config.h"

LOG_CHANNEL(ds4_log, "DS4");

namespace
{
	const auto THREAD_SLEEP = 1ms; //ds4 has new data every ~4ms,
	const auto THREAD_SLEEP_INACTIVE = 100ms;

	const u32 DS4_ACC_RES_PER_G = 8192;
	const u32 DS4_GYRO_RES_PER_DEG_S = 16; // technically this could be 1024, but keeping it at 16 keeps us within 16 bits of precision
	const u32 DS4_FEATURE_REPORT_0x02_SIZE = 37;
	const u32 DS4_FEATURE_REPORT_0x05_SIZE = 41;
	const u32 DS4_FEATURE_REPORT_0x12_SIZE = 16;
	const u32 DS4_FEATURE_REPORT_0x81_SIZE = 7;
	const u32 DS4_INPUT_REPORT_0x11_SIZE = 78;
	const u32 DS4_OUTPUT_REPORT_0x05_SIZE = 32;
	const u32 DS4_OUTPUT_REPORT_0x11_SIZE = 78;
	const u32 DS4_INPUT_REPORT_GYRO_X_OFFSET = 13;
	const u32 DS4_INPUT_REPORT_BATTERY_OFFSET = 30;

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

	inline s16 read_s16(const void* buf)
	{
		return *reinterpret_cast<const s16*>(buf);
	}

	inline u32 read_u32(const void* buf)
	{
		return *reinterpret_cast<const u32*>(buf);
	}
}

ds4_pad_handler::ds4_pad_handler() : PadHandlerBase(pad_handler::ds4)
{
	// Unique names for the config files and our pad settings dialog
	button_list =
	{
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
	thumb_min = 0;
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
	b_has_battery = true;

	m_name_string = "DS4 Pad #";
	m_max_devices = CELL_PAD_MAX_PORT_NUM;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
}

void ds4_pad_handler::init_config(pad_config* cfg, const std::string& name)
{
	// Set this profile's save location
	cfg->cfg_name = name;

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

	// Set default misc variables
	cfg->lstickdeadzone.def    = 40; // between 0 and 255
	cfg->rstickdeadzone.def    = 40; // between 0 and 255
	cfg->ltriggerthreshold.def = 0;  // between 0 and 255
	cfg->rtriggerthreshold.def = 0;  // between 0 and 255
	cfg->padsquircling.def     = 8000;

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
	std::shared_ptr<DS4Device> device = GetDS4Device(padId);
	if (device == nullptr || device->hidDevice == nullptr)
	{
		return 0;
	}
	return std::min<u32>(device->batteryLevel * 10, 100);
}

void ds4_pad_handler::SetPadData(const std::string& padId, u32 largeMotor, u32 smallMotor, s32 r, s32 g, s32 b, bool battery_led, u32 battery_led_brightness)
{
	std::shared_ptr<DS4Device> device = GetDS4Device(padId);
	if (device == nullptr || device->hidDevice == nullptr)
		return;

	// Set the device's motor speeds to our requested values 0-255
	device->largeVibrate = largeMotor;
	device->smallVibrate = smallMotor;

	int index = 0;
	for (uint i = 0; i < MAX_GAMEPADS; i++)
	{
		if (g_cfg_input.player[i]->handler == pad_handler::ds4)
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

	// Set new LED color
	if (battery_led)
	{
		const u32 combined_color = get_battery_color(device->batteryLevel, battery_led_brightness);
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
	SendVibrateData(device);
}

std::shared_ptr<ds4_pad_handler::DS4Device> ds4_pad_handler::GetDS4Device(const std::string& padId, bool try_reconnect)
{
	if (!Init())
		return nullptr;

	size_t pos = padId.find(m_name_string);
	if (pos == umax)
		return nullptr;

	std::string pad_serial = padId.substr(pos + 9);
	std::shared_ptr<DS4Device> device = nullptr;

	int i = 0; // Controllers 1-n in GUI
	for (auto& cur_control : controllers)
	{
		if (pad_serial == std::to_string(++i) || pad_serial == cur_control.first)
		{
			device = cur_control.second;

			if (try_reconnect && device && !device->hidDevice)
			{
				device->hidDevice = hid_open_path(device->path.c_str());
				if (device->hidDevice)
				{
					hid_set_nonblocking(device->hidDevice, 1);
					ds4_log.notice("DS4 device %d reconnected", i);
				}
			}
			break;
		}
	}

	return device;
}

std::unordered_map<u64, u16> ds4_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> keyBuffer;
	auto ds4_dev = std::static_pointer_cast<DS4Device>(device);
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
	u8 dpadState = buf[5] & 0xf;
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

pad_preview_values ds4_pad_handler::get_preview_values(std::unordered_map<u64, u16> data)
{
	return { data[L2], data[R2], data[LSXPos] - data[LSXNeg], data[LSYPos] - data[LSYNeg], data[RSXPos] - data[RSXNeg], data[RSYPos] - data[RSYNeg] };
}

bool ds4_pad_handler::GetCalibrationData(const std::shared_ptr<DS4Device>& ds4Dev)
{
	std::array<u8, 64> buf;
	if (ds4Dev->btCon)
	{
		for (int tries = 0; tries < 3; ++tries)
		{
			buf[0] = 0x05;
			if (hid_get_feature_report(ds4Dev->hidDevice, buf.data(), DS4_FEATURE_REPORT_0x05_SIZE) <= 0)
				return false;

			const u8 btHdr = 0xA3;
			const u32 crcHdr = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
			const u32 crcCalc = CRCPP::CRC::Calculate(buf.data(), (DS4_FEATURE_REPORT_0x05_SIZE - 4), crcTable, crcHdr);
			const u32 crcReported = read_u32(&buf[DS4_FEATURE_REPORT_0x05_SIZE - 4]);
			if (crcCalc != crcReported)
				ds4_log.warning("Calibration CRC check failed! Will retry up to 3 times. Received 0x%x, Expected 0x%x", crcReported, crcCalc);
			else break;
			if (tries == 2)
				return false;
		}
	}
	else
	{
		buf[0] = 0x02;
		if (hid_get_feature_report(ds4Dev->hidDevice, buf.data(), DS4_FEATURE_REPORT_0x02_SIZE) <= 0)
		{
			ds4_log.error("Failed getting calibration data report!");
			return false;
		}
	}

	ds4Dev->calibData[DS4CalibIndex::PITCH].bias = read_s16(&buf[1]);
	ds4Dev->calibData[DS4CalibIndex::YAW].bias = read_s16(&buf[3]);
	ds4Dev->calibData[DS4CalibIndex::ROLL].bias = read_s16(&buf[5]);

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
		return false;
	}

	const s32 gyroSpeedScale = read_s16(&buf[19]) + read_s16(&buf[21]);

	ds4Dev->calibData[DS4CalibIndex::PITCH].sensNumer = gyroSpeedScale * DS4_GYRO_RES_PER_DEG_S;
	ds4Dev->calibData[DS4CalibIndex::PITCH].sensDenom = pitchPlus - pitchNeg;

	ds4Dev->calibData[DS4CalibIndex::YAW].sensNumer = gyroSpeedScale * DS4_GYRO_RES_PER_DEG_S;
	ds4Dev->calibData[DS4CalibIndex::YAW].sensDenom = yawPlus - yawNeg;

	ds4Dev->calibData[DS4CalibIndex::ROLL].sensNumer = gyroSpeedScale * DS4_GYRO_RES_PER_DEG_S;
	ds4Dev->calibData[DS4CalibIndex::ROLL].sensDenom = rollPlus - rollNeg;

	const s16 accelXPlus = read_s16(&buf[23]);
	const s16 accelXNeg  = read_s16(&buf[25]);
	const s16 accelYPlus = read_s16(&buf[27]);
	const s16 accelYNeg  = read_s16(&buf[29]);
	const s16 accelZPlus = read_s16(&buf[31]);
	const s16 accelZNeg  = read_s16(&buf[33]);

	const s32 accelXRange = accelXPlus - accelXNeg;
	ds4Dev->calibData[DS4CalibIndex::X].bias = accelXPlus - accelXRange / 2;
	ds4Dev->calibData[DS4CalibIndex::X].sensNumer = 2 * DS4_ACC_RES_PER_G;
	ds4Dev->calibData[DS4CalibIndex::X].sensDenom = accelXRange;

	const s32 accelYRange = accelYPlus - accelYNeg;
	ds4Dev->calibData[DS4CalibIndex::Y].bias = accelYPlus - accelYRange / 2;
	ds4Dev->calibData[DS4CalibIndex::Y].sensNumer = 2 * DS4_ACC_RES_PER_G;
	ds4Dev->calibData[DS4CalibIndex::Y].sensDenom = accelYRange;

	const s32 accelZRange = accelZPlus - accelZNeg;
	ds4Dev->calibData[DS4CalibIndex::Z].bias = accelZPlus - accelZRange / 2;
	ds4Dev->calibData[DS4CalibIndex::Z].sensNumer = 2 * DS4_ACC_RES_PER_G;
	ds4Dev->calibData[DS4CalibIndex::Z].sensDenom = accelZRange;

	// Make sure data 'looks' valid, dongle will report invalid calibration data with no controller connected

	for (const auto& data : ds4Dev->calibData)
	{
		if (data.sensDenom == 0)
			return false;
	}

	return true;
}

void ds4_pad_handler::CheckAddDevice(hid_device* hidDevice, hid_device_info* hidDevInfo)
{
	std::string serial;
	std::shared_ptr<DS4Device> ds4Dev = std::make_shared<DS4Device>();
	ds4Dev->hidDevice = hidDevice;
	// There isnt a nice 'portable' way with hidapi to detect bt vs wired as the pid/vid's are the same
	// Let's try getting 0x81 feature report, which should will return mac address on wired, and should error on bluetooth
	std::array<u8, 64> buf{};
	buf[0] = 0x81;
	if (const auto length = hid_get_feature_report(hidDevice, buf.data(), DS4_FEATURE_REPORT_0x81_SIZE); length > 0)
	{
		if (length != DS4_FEATURE_REPORT_0x81_SIZE)
		{
			// Controller may not be genuine. These controllers do not have feature 0x81 implemented and calibration data is in bluetooth format even in USB mode!
			ds4_log.warning("DS4 controller may not be genuine. Workaround enabled.");

			// Read feature report 0x12 instead which is what the console uses.
			buf[0] = 0x12;
			buf[1] = 0;
			hid_get_feature_report(hidDevice, buf.data(), DS4_FEATURE_REPORT_0x12_SIZE);
		}

		serial = fmt::format("%x%x%x%x%x%x", buf[6], buf[5], buf[4], buf[3], buf[2], buf[1]);
	}
	else
	{
		ds4Dev->btCon = true;
		std::wstring_view wideSerial(hidDevInfo->serial_number);
		for (wchar_t ch : wideSerial)
			serial += static_cast<uchar>(ch);
	}

	if (!GetCalibrationData(ds4Dev))
	{
		hid_close(hidDevice);
		return;
	}

	ds4Dev->hasCalibData = true;
	ds4Dev->path = hidDevInfo->path;

	hid_set_nonblocking(hidDevice, 1);
	controllers.emplace(serial, ds4Dev);
}

ds4_pad_handler::~ds4_pad_handler()
{
	for (auto& controller : controllers)
	{
		if (controller.second->hidDevice)
		{
			// Disable blinking and vibration
			controller.second->smallVibrate = 0;
			controller.second->largeVibrate = 0;
			controller.second->led_delay_on = 0;
			controller.second->led_delay_off = 0;
			SendVibrateData(controller.second);

			hid_close(controller.second->hidDevice);
		}
	}
	hid_exit();
}

int ds4_pad_handler::SendVibrateData(const std::shared_ptr<DS4Device>& device)
{
	if (!device)
		return -2;

	auto p_profile = device->config;
	if (p_profile == nullptr)
		return -2; // hid_write and hid_write_control return -1 on error

	std::array<u8, 78> outputBuf{0};
	// write rumble state
	if (device->btCon)
	{
		outputBuf[0] = 0x11;
		outputBuf[1] = 0xC4;
		outputBuf[3] = 0x07;
		outputBuf[6] = device->smallVibrate;
		outputBuf[7] = device->largeVibrate;
		outputBuf[8] = p_profile->colorR; // red
		outputBuf[9] = p_profile->colorG; // green
		outputBuf[10] = p_profile->colorB; // blue

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
		outputBuf[4] = device->smallVibrate;
		outputBuf[5] = device->largeVibrate;
		outputBuf[6] = p_profile->colorR; // red
		outputBuf[7] = p_profile->colorG; // green
		outputBuf[8] = p_profile->colorB; // blue
		outputBuf[9] = device->led_delay_on;
		outputBuf[10] = device->led_delay_off;

		return hid_write(device->hidDevice, outputBuf.data(), DS4_OUTPUT_REPORT_0x05_SIZE);
	}
}

bool ds4_pad_handler::Init()
{
	if (is_init)
		return true;

	const int res = hid_init();
	if (res != 0)
		fmt::throw_exception("hidapi-init error.threadproc");

	// get all the possible controllers at start
	bool warn_about_drivers = false;
	for (auto pid : ds4Pids)
	{
		hid_device_info* devInfo = hid_enumerate(DS4_VID, pid);
		hid_device_info* head = devInfo;
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
				ds4_log.error("hid_open_path failed! Reason: %s", hid_error(dev));
				warn_about_drivers = true;
			}
			devInfo = devInfo->next;
		}
		hid_free_enumeration(head);
	}

	if (warn_about_drivers)
	{
		ds4_log.error("One or more DS4 pads were detected but couldn't be interacted with directly");
#if defined(_WIN32) || defined(__linux__)
		ds4_log.error("Check https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration for intructions on how to solve this issue");
#endif
	}
	else if (controllers.empty())
	{
		ds4_log.warning("No controllers found!");
	}
	else
	{
		ds4_log.success("Controllers found: %d", controllers.size());
	}

	is_init = true;
	return true;
}

std::vector<std::string> ds4_pad_handler::ListDevices()
{
	std::vector<std::string> ds4_pads_list;

	if (!Init())
		return ds4_pads_list;

	for (size_t i = 1; i <= controllers.size(); ++i) // Controllers 1-n in GUI
	{
		ds4_pads_list.emplace_back(m_name_string + std::to_string(i));
	}

	return ds4_pads_list;
}

ds4_pad_handler::DS4DataStatus ds4_pad_handler::GetRawData(const std::shared_ptr<DS4Device>& device)
{
	if (!device)
		return DS4DataStatus::ReadError;

	std::array<u8, 78> buf{};

	const int res = hid_read(device->hidDevice, buf.data(), device->btCon ? 78 : 64);
	if (res == -1)
	{
		// looks like controller disconnected or read error
		return DS4DataStatus::ReadError;
	}

	// no data? keep going
	if (res == 0)
		return DS4DataStatus::NoNewData;

	// bt controller sends this until 0x02 feature report is sent back (happens on controller init/restart)
	if (device->btCon && buf[0] == 0x1)
	{
		// tells controller to send 0x11 reports
		std::array<u8, 64> buf_error{};
		buf_error[0] = 0x2;
		hid_get_feature_report(device->hidDevice, buf_error.data(), buf_error.size());
		return DS4DataStatus::NoNewData;
	}

	int offset = 0;
	// check report and set offset
	if (device->btCon && buf[0] == 0x11 && res == 78)
	{
		offset = 2;

		const u8 btHdr = 0xA1;
		const u32 crcHdr = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
		const u32 crcCalc = CRCPP::CRC::Calculate(buf.data(), (DS4_INPUT_REPORT_0x11_SIZE - 4), crcTable, crcHdr);
		const u32 crcReported = read_u32(&buf[DS4_INPUT_REPORT_0x11_SIZE - 4]);
		if (crcCalc != crcReported)
		{
			ds4_log.warning("Data packet CRC check failed, ignoring! Received 0x%x, Expected 0x%x", crcReported, crcCalc);
			return DS4DataStatus::NoNewData;
		}
	}
	else if (!device->btCon && buf[0] == 0x01 && res == 64)
	{
		// Ds4 Dongle uses this bit to actually report whether a controller is connected
		const bool connected = (buf[31] & 0x04) ? false : true;
		if (connected && !device->hasCalibData)
			device->hasCalibData = GetCalibrationData(device);

		offset = 0;
	}
	else
		return DS4DataStatus::NoNewData;

	const int battery_offset = offset + DS4_INPUT_REPORT_BATTERY_OFFSET;
	device->is_initialized = true;
	device->cableState = (buf[battery_offset] >> 4) & 0x01;
	device->batteryLevel = buf[battery_offset] & 0x0F;

	if (device->hasCalibData)
	{
		int calibOffset = offset + DS4_INPUT_REPORT_GYRO_X_OFFSET;
		for (int i = 0; i < DS4CalibIndex::COUNT; ++i)
		{
			const s16 rawValue = read_s16(&buf[calibOffset]);
			const s16 calValue = ApplyCalibration(rawValue, device->calibData[i]);
			buf[calibOffset++] = (static_cast<u16>(calValue) >> 0) & 0xFF;
			buf[calibOffset++] = (static_cast<u16>(calValue) >> 8) & 0xFF;
		}
	}
	memcpy(device->padData.data(), &buf[offset], 64);

	return DS4DataStatus::NewData;
}

std::shared_ptr<PadDevice> ds4_pad_handler::get_device(const std::string& device)
{
	std::shared_ptr<DS4Device> ds4device = GetDS4Device(device);
	if (ds4device == nullptr || ds4device->hidDevice == nullptr)
		return nullptr;

	return ds4device;
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

u32 ds4_pad_handler::get_battery_color(u8 battery_level, int brightness)
{
	static const std::array<u32, 12> battery_level_clr = {0xff00, 0xff33, 0xff66, 0xff99, 0xffcc, 0xffff, 0xccff, 0x99ff, 0x66ff, 0x33ff, 0x00ff, 0x00ff};
	u32 combined_color = battery_level_clr[0];
	// Check if we got a weird value
	if (battery_level < battery_level_clr.size())
	{
		combined_color = battery_level_clr[battery_level];
	}
	const u32 red = (combined_color >> 8) * brightness / 100;
	const u32 green = (combined_color & 0xff) * brightness / 100;
	return ((red << 8) | green);
}

PadHandlerBase::connection ds4_pad_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	auto ds4_dev = std::static_pointer_cast<DS4Device>(device);
	if (!ds4_dev)
		return connection::disconnected;

	if (ds4_dev->hidDevice == nullptr)
	{
		// try to reconnect
		hid_device* dev = hid_open_path(ds4_dev->path.c_str());
		if (dev)
		{
			hid_set_nonblocking(dev, 1);
			ds4_dev->hidDevice = dev;
			if (!ds4_dev->hasCalibData)
				ds4_dev->hasCalibData = GetCalibrationData(ds4_dev);
		}
		else
		{
			// nope, not there
			return connection::disconnected;
		}
	}

	status = GetRawData(ds4_dev);

	if (status == DS4DataStatus::ReadError)
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
	auto ds4_device = std::static_pointer_cast<DS4Device>(device);
	if (!ds4_device || !pad)
		return;

	auto buf = ds4_device->padData;

	pad->m_battery_level = ds4_device->batteryLevel;
	pad->m_cable_state = ds4_device->cableState;

	// these values come already calibrated from our ds4Thread,
	// all we need to do is convert to ds3 range

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
	auto ds4_dev = std::static_pointer_cast<DS4Device>(device);
	if (!ds4_dev || !pad)
		return;

	auto profile = ds4_dev->config;

	// Attempt to send rumble no matter what
	int idx_l = profile->switch_vibration_motors ? 1 : 0;
	int idx_s = profile->switch_vibration_motors ? 0 : 1;

	int speed_large = profile->enable_vibration_motor_large ? pad->m_vibrateMotors[idx_l].m_value : vibration_min;
	int speed_small = profile->enable_vibration_motor_small ? pad->m_vibrateMotors[idx_s].m_value : vibration_min;

	bool wireless = ds4_dev->cableState < 1;
	bool lowBattery = ds4_dev->batteryLevel < 2;
	bool isBlinking = ds4_dev->led_delay_on > 0 || ds4_dev->led_delay_off > 0;
	bool newBlinkData = false;

	// Blink LED when battery is low
	if (ds4_dev->config->led_low_battery_blink)
	{
		// we are now wired or have okay battery level -> stop blinking
		if (isBlinking && !(wireless && lowBattery))
		{
			ds4_dev->led_delay_on = 0;
			ds4_dev->led_delay_off = 0;
			newBlinkData = true;
		}
		// we are now wireless and low on battery -> blink
		if (!isBlinking && wireless && lowBattery)
		{
			ds4_dev->led_delay_on = 100;
			ds4_dev->led_delay_off = 100;
			newBlinkData = true;
		}
	}

	// Use LEDs to indicate battery level
	if (ds4_dev->config->led_battery_indicator)
	{
		// This makes sure that the LED color doesn't update every 1ms. DS4 only reports battery level in 10% increments
		if (ds4_dev->last_battery_level != ds4_dev->batteryLevel)
		{
			const u32 combined_color = get_battery_color(ds4_dev->batteryLevel, ds4_dev->config->led_battery_indicator_brightness);
			ds4_dev->config->colorR.set(combined_color >> 8);
			ds4_dev->config->colorG.set(combined_color & 0xff);
			ds4_dev->config->colorB.set(0);
		}
	}

	ds4_dev->newVibrateData |= ds4_dev->largeVibrate != speed_large || ds4_dev->smallVibrate != speed_small || newBlinkData;

	ds4_dev->largeVibrate = speed_large;
	ds4_dev->smallVibrate = speed_small;

	if (ds4_dev->newVibrateData && SendVibrateData(ds4_dev) >= 0)
	{
		ds4_dev->newVibrateData = false;
	}
}
