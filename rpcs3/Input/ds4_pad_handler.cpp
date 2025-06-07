#include "stdafx.h"
#include "ds4_pad_handler.h"
#include "Emu/Io/pad_config.h"

#include <limits>

LOG_CHANNEL(ds4_log, "DS4");

using namespace reports;

constexpr id_pair SONY_DS4_ID_0 = {0x054C, 0x0BA0}; // Dongle
constexpr id_pair SONY_DS4_ID_1 = {0x054C, 0x05C4}; // CUH-ZCT1x
constexpr id_pair SONY_DS4_ID_2 = {0x054C, 0x09CC}; // CUH-ZCT2x

constexpr id_pair ZEROPLUS_ID_0 = {0x0C12, 0x0E20};

namespace
{
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
    : hid_pad_handler<DS4Device>(pad_handler::ds4, {SONY_DS4_ID_0, SONY_DS4_ID_1, SONY_DS4_ID_2, ZEROPLUS_ID_0})
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
		{ DS4KeyCodes::Touch_L,  "Touch Left" },
		{ DS4KeyCodes::Touch_R,  "Touch Right" },
		{ DS4KeyCodes::Touch_U,  "Touch Up" },
		{ DS4KeyCodes::Touch_D,  "Touch Down" },
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

	// set capabilities
	b_has_config = true;
	b_has_rumble = true;
	b_has_motion = true;
	b_has_deadzones = true;
	b_has_led = true;
	b_has_rgb = true;
	b_has_battery = true;
	b_has_battery_led = true;
	b_has_orientation = true;

	m_name_string = "DS4 Pad #";
	m_max_devices = CELL_PAD_MAX_PORT_NUM;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
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

			if (send_output_report(controller.second.get()) == -1)
			{
				ds4_log.error("~ds4_pad_handler: send_output_report failed! error=%s", hid_error(controller.second->hidDevice));
			}
		}
	}
}

void ds4_pad_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
	cfg->ls_left.def  = ::at32(button_list, DS4KeyCodes::LSXNeg);
	cfg->ls_down.def  = ::at32(button_list, DS4KeyCodes::LSYNeg);
	cfg->ls_right.def = ::at32(button_list, DS4KeyCodes::LSXPos);
	cfg->ls_up.def    = ::at32(button_list, DS4KeyCodes::LSYPos);
	cfg->rs_left.def  = ::at32(button_list, DS4KeyCodes::RSXNeg);
	cfg->rs_down.def  = ::at32(button_list, DS4KeyCodes::RSYNeg);
	cfg->rs_right.def = ::at32(button_list, DS4KeyCodes::RSXPos);
	cfg->rs_up.def    = ::at32(button_list, DS4KeyCodes::RSYPos);
	cfg->start.def    = ::at32(button_list, DS4KeyCodes::Options);
	cfg->select.def   = ::at32(button_list, DS4KeyCodes::Share);
	cfg->ps.def       = ::at32(button_list, DS4KeyCodes::PSButton);
	cfg->square.def   = ::at32(button_list, DS4KeyCodes::Square);
	cfg->cross.def    = ::at32(button_list, DS4KeyCodes::Cross);
	cfg->circle.def   = ::at32(button_list, DS4KeyCodes::Circle);
	cfg->triangle.def = ::at32(button_list, DS4KeyCodes::Triangle);
	cfg->left.def     = ::at32(button_list, DS4KeyCodes::Left);
	cfg->down.def     = ::at32(button_list, DS4KeyCodes::Down);
	cfg->right.def    = ::at32(button_list, DS4KeyCodes::Right);
	cfg->up.def       = ::at32(button_list, DS4KeyCodes::Up);
	cfg->r1.def       = ::at32(button_list, DS4KeyCodes::R1);
	cfg->r2.def       = ::at32(button_list, DS4KeyCodes::R2);
	cfg->r3.def       = ::at32(button_list, DS4KeyCodes::R3);
	cfg->l1.def       = ::at32(button_list, DS4KeyCodes::L1);
	cfg->l2.def       = ::at32(button_list, DS4KeyCodes::L2);
	cfg->l3.def       = ::at32(button_list, DS4KeyCodes::L3);

	cfg->pressure_intensity_button.def = ::at32(button_list, DS4KeyCodes::None);
	cfg->analog_limiter_button.def = ::at32(button_list, DS4KeyCodes::None);
	cfg->orientation_reset_button.def = ::at32(button_list, DS4KeyCodes::None);

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

void ds4_pad_handler::SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool /*player_led*/, bool battery_led, u32 battery_led_brightness)
{
	std::shared_ptr<DS4Device> device = get_hid_device(padId);
	if (!device || !device->hidDevice)
		return;

	// Set the device's motor speeds to our requested values 0-255
	device->large_motor = large_motor;
	device->small_motor = small_motor;
	device->player_id = player_id;
	device->config = get_config(padId);

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
	if (send_output_report(device.get()) == -1)
	{
		ds4_log.error("SetPadData: send_output_report failed! error=%s", hid_error(device->hidDevice));
	}
}

std::unordered_map<u64, u16> ds4_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> keyBuffer;
	DS4Device* dev = static_cast<DS4Device*>(device.get());
	if (!dev)
		return keyBuffer;

	const ds4_input_report_common& input = dev->bt_controller ? dev->report_bt.common : dev->report_usb.common;

	// Left Stick X Axis
	keyBuffer[DS4KeyCodes::LSXNeg] = Clamp0To255((127.5f - input.x) * 2.0f);
	keyBuffer[DS4KeyCodes::LSXPos] = Clamp0To255((input.x - 127.5f) * 2.0f);

	// Left Stick Y Axis (Up is the negative for some reason)
	keyBuffer[DS4KeyCodes::LSYNeg] = Clamp0To255((input.y - 127.5f) * 2.0f);
	keyBuffer[DS4KeyCodes::LSYPos] = Clamp0To255((127.5f - input.y) * 2.0f);

	// Right Stick X Axis
	keyBuffer[DS4KeyCodes::RSXNeg] = Clamp0To255((127.5f - input.rx) * 2.0f);
	keyBuffer[DS4KeyCodes::RSXPos] = Clamp0To255((input.rx - 127.5f) * 2.0f);

	// Right Stick Y Axis (Up is the negative for some reason)
	keyBuffer[DS4KeyCodes::RSYNeg] = Clamp0To255((input.ry - 127.5f) * 2.0f);
	keyBuffer[DS4KeyCodes::RSYPos] = Clamp0To255((127.5f - input.ry) * 2.0f);

	// bleh, dpad in buffer is stored in a different state
	const u8 dpadState = input.buttons[0] & 0xf;
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
	keyBuffer[DS4KeyCodes::Square] =   ((input.buttons[0] & (1 << 4)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::Cross] =    ((input.buttons[0] & (1 << 5)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::Circle] =   ((input.buttons[0] & (1 << 6)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::Triangle] = ((input.buttons[0] & (1 << 7)) != 0) ? 255 : 0;

	// L1, R1, L2, L3, select, start, L3, L3
	keyBuffer[DS4KeyCodes::L1]      = ((input.buttons[1] & (1 << 0)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::R1]      = ((input.buttons[1] & (1 << 1)) != 0) ? 255 : 0;
	//keyBuffer[DS4KeyCodes::L2But]   = ((input.buttons[1] & (1 << 2)) != 0) ? 255 : 0;
	//keyBuffer[DS4KeyCodes::R2But]   = ((input.buttons[1] & (1 << 3)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::Share]   = ((input.buttons[1] & (1 << 4)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::Options] = ((input.buttons[1] & (1 << 5)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::L3]      = ((input.buttons[1] & (1 << 6)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::R3]      = ((input.buttons[1] & (1 << 7)) != 0) ? 255 : 0;

	// PS Button, Touch Button
	keyBuffer[DS4KeyCodes::PSButton] = ((input.buttons[2] & (1 << 0)) != 0) ? 255 : 0;
	keyBuffer[DS4KeyCodes::TouchPad] = ((input.buttons[2] & (1 << 1)) != 0) ? 255 : 0;

	// L2, R2
	keyBuffer[DS4KeyCodes::L2] = input.z;
	keyBuffer[DS4KeyCodes::R2] = input.rz;

	// Touch Pad
	const auto apply_touch = [&keyBuffer](const ds4_touch_report& touch)
	{
		for (const ds4_touch_point& point : touch.points)
		{
			if (!(point.contact & DS4_TOUCH_POINT_INACTIVE))
			{
				const s32 x = (point.x_hi << 8) | point.x_lo;
				const s32 y = (point.y_hi << 4) | point.y_lo;

				const f32 x_scaled = ScaledInput(static_cast<float>(x), 0.0f, static_cast<float>(DS4_TOUCHPAD_WIDTH), 0.0f, 255.0f);
				const f32 y_scaled = ScaledInput(static_cast<float>(y), 0.0f, static_cast<float>(DS4_TOUCHPAD_HEIGHT), 0.0f, 255.0f);

				keyBuffer[DS4KeyCodes::Touch_L] = Clamp0To255((127.5f - x_scaled) * 2.0f);
				keyBuffer[DS4KeyCodes::Touch_R] = Clamp0To255((x_scaled - 127.5f) * 2.0f);

				keyBuffer[DS4KeyCodes::Touch_U] = Clamp0To255((127.5f - y_scaled) * 2.0f);
				keyBuffer[DS4KeyCodes::Touch_D] = Clamp0To255((y_scaled - 127.5f) * 2.0f);
			}
		}
	};

	if (dev->bt_controller)
	{
		const ds4_input_report_bt& report = dev->report_bt;

		for (u32 i = 0; i < std::min<u32>(report.num_touch_reports, ::size32(report.touch_reports)); i++)
		{
			apply_touch(report.touch_reports[i]);
		}
	}
	else
	{
		const ds4_input_report_usb& report = dev->report_usb;

		for (u32 i = 0; i < std::min<u32>(report.num_touch_reports, ::size32(report.touch_reports)); i++)
		{
			apply_touch(report.touch_reports[i]);
		}
	}

	return keyBuffer;
}

pad_preview_values ds4_pad_handler::get_preview_values(const std::unordered_map<u64, u16>& data)
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

bool ds4_pad_handler::GetCalibrationData(DS4Device* ds4Dev) const
{
	if (!ds4Dev || !ds4Dev->hidDevice)
	{
		ds4_log.error("GetCalibrationData called with null device");
		return false;
	}

	std::array<u8, 64> buf{};

	if (ds4Dev->bt_controller)
	{
		for (int tries = 0; tries < 3; ++tries)
		{
			buf = {};
			buf[0] = 0x05; // Calibration feature report id

			if (int res = hid_get_feature_report(ds4Dev->hidDevice, buf.data(), DS4_FEATURE_REPORT_BLUETOOTH_CALIBRATION_SIZE); res != DS4_FEATURE_REPORT_BLUETOOTH_CALIBRATION_SIZE || buf[0] != 0x05)
			{
				ds4_log.error("GetCalibrationData: hid_get_feature_report 0x05 for bluetooth controller failed! result=%d, error=%s", res, hid_error(ds4Dev->hidDevice));
				return false;
			}

			const u8 btHdr = 0xA3;
			const u32 crcHdr = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
			const u32 crcCalc = CRCPP::CRC::Calculate(buf.data(), DS4_FEATURE_REPORT_BLUETOOTH_CALIBRATION_SIZE - 4, crcTable, crcHdr);
			const u32 crcReported = read_u32(&buf[DS4_FEATURE_REPORT_BLUETOOTH_CALIBRATION_SIZE - 4]);

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
		if (int res = hid_get_feature_report(ds4Dev->hidDevice, buf.data(), DS4_FEATURE_REPORT_USB_CALIBRATION_SIZE); res != DS4_FEATURE_REPORT_USB_CALIBRATION_SIZE || buf[0] != 0x02)
		{
			ds4_log.error("GetCalibrationData: hid_get_feature_report 0x02 for wired controller failed! result=%d, error=%s", res, hid_error(ds4Dev->hidDevice));
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

	for (usz i = 0; i < ds4Dev->calib_data.size(); i++)
	{
		CalibData& data = ds4Dev->calib_data[i];

		if (data.sens_denom == 0)
		{
			ds4_log.error("GetCalibrationData: Invalid accelerometer calibration data for axis %d, disabling calibration.", i);
			data.bias = 0;
			data.sens_numer = 4 * DS4_ACC_RES_PER_G;
			data.sens_denom = std::numeric_limits<s16>::max();
		}
	}

	return true;
}

void ds4_pad_handler::check_add_device(hid_device* hidDevice, hid_enumerated_device_view path, std::wstring_view wide_serial)
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
	for (wchar_t ch : wide_serial)
		serial += static_cast<uchar>(ch);

	const hid_device_info* devinfo = hid_get_device_info(hidDevice);
	if (!devinfo)
	{
		ds4_log.error("check_add_device: hid_get_device_info failed! error=%s", hid_error(hidDevice));
		HidDevice::close(hidDevice);
		return;
	}

	device->bt_controller = (devinfo->bus_type == HID_API_BUS_BLUETOOTH);
	device->hidDevice = hidDevice;

	if (!GetCalibrationData(device))
	{
		ds4_log.error("check_add_device: GetCalibrationData failed!");
		device->close();
		return;
	}

	u32 hw_version{};
	u32 fw_version{};

	std::array<u8, 64> buf{};
	buf[0] = 0xA3;

	int res = hid_get_feature_report(hidDevice, buf.data(), DS4_FEATURE_REPORT_FIRMWARE_INFO_SIZE);
	if (res != DS4_FEATURE_REPORT_FIRMWARE_INFO_SIZE || buf[0] != 0xA3)
	{
		ds4_log.error("check_add_device: hid_get_feature_report 0xA3 failed! Could not retrieve firmware version! result=%d, buf[0]=0x%x, error=%s", res, buf[0], hid_error(hidDevice));
	}
	else
	{
		hw_version = read_u32(&buf[35]);
		fw_version = read_u32(&buf[41]);
		ds4_log.notice("check_add_device: Got firmware version: hw_version: 0x%x, fw_version: 0x%x", hw_version, fw_version);
	}

	if (hid_set_nonblocking(hidDevice, 1) == -1)
	{
		ds4_log.error("check_add_device: hid_set_nonblocking failed! Reason: %s", hid_error(hidDevice));
		device->close();
		return;
	}

	device->has_calib_data = true;
	device->path           = path;

	if (send_output_report(device) == -1)
	{
		ds4_log.error("check_add_device: send_output_report failed! Reason: %s", hid_error(hidDevice));
	}

	ds4_log.notice("Added device: bluetooth=%d, serial='%s', hw_version: 0x%x, fw_version: 0x%x, path='%s'", device->bt_controller, serial, hw_version, fw_version, device->path);
}

int ds4_pad_handler::send_output_report(DS4Device* device)
{
	if (!device || !device->hidDevice)
		return -2;

	const auto config = device->config;
	if (config == nullptr)
		return -2; // hid_write returns -1 on error

	// write rumble state
	ds4_output_report_common common{};
	common.valid_flag0 = 0x07;
	common.motor_right = device->small_motor;
	common.motor_left = device->large_motor;

	// write LED color
	common.lightbar_red = config->colorR;
	common.lightbar_green = config->colorG;
	common.lightbar_blue = config->colorB;

	// alternating blink states with values 0-255: only setting both to zero disables blinking
	// 255 is roughly 2 seconds, so setting both values to 255 results in a 4 second interval
	// using something like (0,10) will heavily blink, while using (0, 255) will be slow. you catch the drift
	common.lightbar_blink_on = device->led_delay_on;
	common.lightbar_blink_off = device->led_delay_off;

	if (device->bt_controller)
	{
		ds4_output_report_bt output{};
		output.report_id = 0x11;
		output.hw_control = 0xC4;
		output.common = std::move(common);

		const u8 btHdr = 0xA2;
		const u32 crcHdr = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
		const u32 crcCalc = CRCPP::CRC::Calculate(&output.report_id, offsetof(ds4_output_report_bt, crc32), crcTable, crcHdr);

		write_to_ptr(output.crc32, crcCalc);

		return hid_write(device->hidDevice, &output.report_id, sizeof(ds4_output_report_bt));
	}

	ds4_output_report_usb output{};
	output.report_id = 0x05;
	output.common = std::move(common);

	return hid_write(device->hidDevice, &output.report_id, sizeof(ds4_output_report_usb));
}

ds4_pad_handler::DataStatus ds4_pad_handler::get_data(DS4Device* device)
{
	if (!device || !device->hidDevice)
		return DataStatus::ReadError;

	std::array<u8, std::max(sizeof(ds4_input_report_bt), sizeof(ds4_input_report_usb))> buf{};

	const int res = hid_read(device->hidDevice, buf.data(), device->bt_controller ? sizeof(ds4_input_report_bt) : sizeof(ds4_input_report_usb));
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
		if (int res = hid_get_feature_report(device->hidDevice, buf_error.data(), buf_error.size()); res != static_cast<int>(buf_error.size()) || buf_error[0] != 0x2)
		{
			ds4_log.error("GetRawData: hid_get_feature_report 0x2 failed! result=%d, buf[0]=0x%x, error=%s", res, buf[0], hid_error(device->hidDevice));
		}
		return DataStatus::NoNewData;
	}

	int offset = 0;

	// check report and set offset
	if (device->bt_controller && buf[0] == 0x11 && res == sizeof(ds4_input_report_bt))
	{
		offset = offsetof(ds4_input_report_bt, common);

		const u8 btHdr = 0xA1;
		const u32 crcHdr = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
		const u32 crcCalc = CRCPP::CRC::Calculate(buf.data(), offsetof(ds4_input_report_bt, crc32), crcTable, crcHdr);
		const u32 crcReported = read_u32(&buf[offsetof(ds4_input_report_bt, crc32)]);
		if (crcCalc != crcReported)
		{
			ds4_log.warning("Data packet CRC check failed, ignoring! Received 0x%x, Expected 0x%x", crcReported, crcCalc);
			return DataStatus::NoNewData;
		}
	}
	else if (!device->bt_controller && buf[0] == 0x01 && res == sizeof(ds4_input_report_usb))
	{
		// Ds4 Dongle uses this bit to actually report whether a controller is connected
		const bool connected = !(buf[31] & 0x04);
		if (connected && !device->has_calib_data)
			device->has_calib_data = GetCalibrationData(device);

		offset = offsetof(ds4_input_report_usb, common);
	}
	else
		return DataStatus::NoNewData;

	const int battery_offset = offset + offsetof(ds4_input_report_common, status);
	device->cable_state = (buf[battery_offset] >> 4) & 0x01;
	device->battery_level = buf[battery_offset] & 0x0F; // 0 - 9 while unplugged, 0 - 10 while plugged in, 11 charge complete

	if (device->has_calib_data)
	{
		int calib_offset = offset + offsetof(ds4_input_report_common, gyro);
		for (int i = 0; i < CalibIndex::COUNT; ++i)
		{
			const s16 raw_value = read_s16(&buf[calib_offset]);
			const s16 cal_value = apply_calibration(raw_value, device->calib_data[i]);
			buf[calib_offset++] = (static_cast<u16>(cal_value) >> 0) & 0xFF;
			buf[calib_offset++] = (static_cast<u16>(cal_value) >> 8) & 0xFF;
		}
	}

	if (device->bt_controller)
	{
		std::memcpy(&device->report_bt, buf.data(), sizeof(ds4_input_report_bt));
	}
	else
	{
		std::memcpy(&device->report_usb, buf.data(), sizeof(ds4_input_report_usb));
	}

	return DataStatus::NewData;
}

bool ds4_pad_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return keyCode == DS4KeyCodes::L2;
}

bool ds4_pad_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return keyCode == DS4KeyCodes::R2;
}

bool ds4_pad_handler::get_is_left_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
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

bool ds4_pad_handler::get_is_right_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
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

bool ds4_pad_handler::get_is_touch_pad_motion(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	switch (keyCode)
	{
	case DS4KeyCodes::Touch_L:
	case DS4KeyCodes::Touch_R:
	case DS4KeyCodes::Touch_U:
	case DS4KeyCodes::Touch_D:
		return true;
	default:
		return false;
	}
}

PadHandlerBase::connection ds4_pad_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	DS4Device* dev = static_cast<DS4Device*>(device.get());
	if (!dev || dev->path == hid_enumerated_device_default)
		return connection::disconnected;

	if (dev->hidDevice == nullptr)
	{
		// try to reconnect
		if (hid_device* hid_dev = dev->open())
		{
			if (hid_set_nonblocking(hid_dev, 1) == -1)
			{
				ds4_log.error("Reconnecting Device %s: hid_set_nonblocking failed with error %s", dev->path, hid_error(hid_dev));
			}

			if (!dev->has_calib_data)
			{
				dev->has_calib_data = GetCalibrationData(dev);
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

void ds4_pad_handler::get_extended_info(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	DS4Device* dev = static_cast<DS4Device*>(device.get());
	if (!dev || !pad)
		return;

	const ds4_input_report_common& input = dev->bt_controller ? dev->report_bt.common : dev->report_usb.common;

	pad->m_battery_level = dev->battery_level;
	pad->m_cable_state   = dev->cable_state;

	// these values come already calibrated, all we need to do is convert to ds3 range

	// acceleration (linear velocity in m/s²)
	const f32 accel_x = static_cast<s16>(input.accel[0]) / static_cast<f32>(DS4_ACC_RES_PER_G) * -1;
	const f32 accel_y = static_cast<s16>(input.accel[1]) / static_cast<f32>(DS4_ACC_RES_PER_G) * -1;
	const f32 accel_z = static_cast<s16>(input.accel[2]) / static_cast<f32>(DS4_ACC_RES_PER_G) * -1;

	// gyro (angular velocity in degree/s)
	const f32 gyro_x = static_cast<s16>(input.gyro[0]) / static_cast<f32>(DS4_GYRO_RES_PER_DEG_S) * -1;
	const f32 gyro_y = static_cast<s16>(input.gyro[1]) / static_cast<f32>(DS4_GYRO_RES_PER_DEG_S) * -1;
	const f32 gyro_z = static_cast<s16>(input.gyro[2]) / static_cast<f32>(DS4_GYRO_RES_PER_DEG_S) * -1;

	// now just use formula from ds3
	pad->m_sensors[0].m_value = Clamp0To1023(accel_x * MOTION_ONE_G + 512);
	pad->m_sensors[1].m_value = Clamp0To1023(accel_y * MOTION_ONE_G + 512);
	pad->m_sensors[2].m_value = Clamp0To1023(accel_z * MOTION_ONE_G + 512);

	// gyro_y is yaw, which is all that we need.
	// Convert to ds3. The ds3 resolution is 123/90°/sec.
	pad->m_sensors[3].m_value = Clamp0To1023(gyro_y * (123.f / 90.f) + 512);

	// Set raw orientation
	set_raw_orientation(pad->move_data, accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z);
}

void ds4_pad_handler::apply_pad_data(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	DS4Device* dev = static_cast<DS4Device*>(device.get());
	if (!dev || !dev->hidDevice || !dev->config || !pad)
		return;

	cfg_pad* config = dev->config;

	// Attempt to send rumble no matter what
	const u8 speed_large = config->get_large_motor_speed(pad->m_vibrateMotors);
	const u8 speed_small = config->get_small_motor_speed(pad->m_vibrateMotors);

	const bool wireless    = dev->cable_state == 0;
	const bool low_battery = dev->battery_level < 2;
	const bool is_blinking = dev->led_delay_on > 0 || dev->led_delay_off > 0;

	// Blink LED when battery is low
	if (config->led_low_battery_blink)
	{
		// we are now wired or have okay battery level -> stop blinking
		if (is_blinking && !(wireless && low_battery))
		{
			dev->led_delay_on = 0;
			dev->led_delay_off = 0;
			dev->new_output_data = true;
		}
		// we are now wireless and low on battery -> blink
		else if (!is_blinking && wireless && low_battery)
		{
			dev->led_delay_on = 100;
			dev->led_delay_off = 100;
			dev->new_output_data = true;
		}
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
			dev->new_output_data = true;
			dev->last_battery_level = dev->battery_level;
		}
	}

	dev->new_output_data |= dev->large_motor != speed_large || dev->small_motor != speed_small;

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
			ds4_log.error("apply_pad_data: send_output_report failed! error=%s", hid_error(dev->hidDevice));
		}
	}
}
