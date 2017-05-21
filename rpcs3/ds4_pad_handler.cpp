#include "stdafx.h"
#include "Emu/System.h"
#include "ds4_pad_handler.h"

#include <thread>
#include <cmath>

namespace
{
	const auto THREAD_SLEEP = 1ms; //ds4 has new data every ~4ms, 
	const auto THREAD_SLEEP_INACTIVE = 100ms;

	const u32 DS4_ACC_RES_PER_G = 8192;
	const u32 DS4_GYRO_RES_PER_DEG_S = 16; // technically this could be 1024, but keeping it at 16 keeps us within 16 bits of precision
	const u32 DS4_FEATURE_REPORT_0x02_SIZE = 37;
	const u32 DS4_FEATURE_REPORT_0x05_SIZE = 41;
	const u32 DS4_FEATURE_REPORT_0x81_SIZE = 7;
	const u32 DS4_INPUT_REPORT_0x11_SIZE = 78;
	const u32 DS4_OUTPUT_REPORT_0x05_SIZE = 32;
	const u32 DS4_OUTPUT_REPORT_0x11_SIZE = 78;
	const u32 DS4_INPUT_REPORT_GYRO_X_OFFSET = 13;

	inline u16 Clamp0To255(f32 input)
	{
		if (input > 255.f)
			return 255;
		else if (input < 0.f)
			return 0;
		else return static_cast<u16>(input);
	}

	inline u16 Clamp0To1023(f32 input)
	{
		if (input > 1023.f)
			return 1023;
		else if (input < 0.f)
			return 0;
		else return static_cast<u16>(input);
	}

	// we get back values from 0 - 255 for x and y from the ds4 packets,
	// and they end up giving us basically a perfect circle, which is how the ds4 sticks are setup
	// however,the ds3, (and i think xbox controllers) give instead a more 'square-ish' type response, so that the corners will give (almost)max x/y instead of the ~30x30 from a perfect circle
	// using a simple scale/sensitivity increase would *work* although it eats a chunk of our usable range in exchange

	// this might be the best for now, in practice it seems to push the corners to max of 20x20
	std::tuple<u16, u16> ConvertToSquirclePoint(u16 inX, u16 inY)
	{
		// convert inX and Y to a (-1, 1) vector;
		const f32 x = (inX - 127) / 127.f;
		const f32 y = ((inY - 127) / 127.f);

		// compute angle and len of given point to be used for squircle radius
		const f32 angle = std::atan2(y, x);
		const f32 r = std::sqrt(std::pow(x, 2.f) + std::pow(y, 2.f));
		
		// now find len/point on the given squircle from our current angle and radius in polar coords
		// https://thatsmaths.com/2016/07/14/squircles/
		const f32 newLen = (1 + std::pow(std::sin(2 * angle), 2.f) / 8.f) * r;

		// we now have len and angle, convert to cartisian 

		const int newX = Clamp0To255(((newLen * std::cos(angle)) + 1) * 127);
		const int newY = Clamp0To255(((newLen * std::sin(angle)) + 1) * 127);
		return std::tuple<u16, u16>(newX, newY);
	}

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

	inline s16 GetS16LEData(const u8* buf)
	{
		return (s16)(((u16)buf[0] << 0) + ((u16)buf[1] << 8));
	}

	inline u32 GetU32LEData(const u8* buf)
	{
		return (u32)(((u32)buf[0] << 0) + ((u32)buf[1] << 8) + ((u32)buf[2] << 16) + ((u32)buf[3] << 24));
	}
}

ds4_pad_handler::~ds4_pad_handler()
{
	Close();
}

void ds4_pad_handler::Init(const u32 max_connect)
{
	std::memset(&m_info, 0, sizeof m_info);
	m_info.max_connect = max_connect;

	for (u32 i = 0, max = std::min(max_connect, u32(MAX_GAMEPADS)); i != max; ++i)
	{
		m_pads.emplace_back(
			CELL_PAD_STATUS_DISCONNECTED,
			CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
			CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE,
			CELL_PAD_DEV_TYPE_STANDARD
		);
		auto & pad = m_pads.back();

		// 'keycode' here is just 0 as we have to manually calculate this
		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, CELL_PAD_CTRL_L2);
		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, CELL_PAD_CTRL_R2);

		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, 0, CELL_PAD_CTRL_UP);
		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, 0, CELL_PAD_CTRL_DOWN);
		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, 0, CELL_PAD_CTRL_LEFT);
		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, 0, CELL_PAD_CTRL_RIGHT);

		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, CELL_PAD_CTRL_SQUARE);
		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, CELL_PAD_CTRL_CROSS);
		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, CELL_PAD_CTRL_CIRCLE);
		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, CELL_PAD_CTRL_TRIANGLE);

		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, CELL_PAD_CTRL_L1);
		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, CELL_PAD_CTRL_R1);

		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, 0, CELL_PAD_CTRL_SELECT);
		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, 0, CELL_PAD_CTRL_START);
		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, 0, CELL_PAD_CTRL_L3);
		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, 0, CELL_PAD_CTRL_R3);

		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, 0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support
		pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, 0x0); // Reserved

		pad.m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_X, 512);
		pad.m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Y, 399);
		pad.m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Z, 512);
		pad.m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_G, 512);

		pad.m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, 0, 0);
		pad.m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, 0, 0);
		pad.m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, 0, 0);
		pad.m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, 0, 0);

		pad.m_vibrateMotors.emplace_back(true, 0);
		pad.m_vibrateMotors.emplace_back(false, 0);
	}

	ds4Thread = std::make_shared<ds4_thread>();
	ds4Thread->on_init(ds4Thread);
}

PadInfo& ds4_pad_handler::GetInfo()
{
	if (ds4Thread)
	{
		auto info = ds4Thread->GetConnectedControllers();

		m_info.now_connect = 0;

		int i = 0;
		for (auto & pad : m_pads)
		{

			if (info[i])
			{
				m_info.now_connect++;
				if (last_connection_status[i] == false)
					pad.m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
				last_connection_status[i] = true;
				pad.m_port_status |= CELL_PAD_STATUS_CONNECTED;
			}
			else
			{
				if (last_connection_status[i] == true)
					pad.m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
				last_connection_status[i] = false;
				pad.m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
			}

			++i;
		}
	}

	return m_info;
}

std::vector<Pad>& ds4_pad_handler::GetPads()
{
	if (ds4Thread)
		ProcessData();

	return m_pads;
}

void ds4_pad_handler::Close()
{
	if (ds4Thread)
		ds4Thread.reset();

	m_pads.clear();
}

void ds4_pad_handler::ProcessData()
{

	if (!ds4Thread)
		return;

	auto data = ds4Thread->GetControllerData();

	int i = 0;
	for (auto & pad : m_pads)
	{

		auto buf = data[i];

		// these are added with previous value and divided to 'smooth' out the readings
		// the ds4 seems to rapidly flicker sometimes between two values and this seems to stop that

		u16 lx, ly;
		//std::tie(lx, ly) = ConvertToSquarePoint(buf[1], buf[2]);
		std::tie(lx, ly) = ConvertToSquirclePoint(buf[1], buf[2]);
		pad.m_sticks[0].m_value = (lx + pad.m_sticks[0].m_value) / 2; // LX
		pad.m_sticks[1].m_value = (ly + pad.m_sticks[1].m_value) / 2; // LY
		
		u16 rx, ry;
		//std::tie(rx, ry) = ConvertToSquarePoint(buf[3], buf[4]);
		std::tie(rx, ry) = ConvertToSquirclePoint(buf[3], buf[4]);
		pad.m_sticks[2].m_value = (rx + pad.m_sticks[2].m_value) / 2; // RX
		pad.m_sticks[3].m_value = (ry + pad.m_sticks[3].m_value) / 2; // RY

		// l2 r2
		pad.m_buttons[0].m_pressed = buf[8] > 0;
		pad.m_buttons[0].m_value = buf[8];
		pad.m_buttons[1].m_pressed = buf[9] > 0;
		pad.m_buttons[1].m_value = buf[9];

		// bleh, dpad in buffer is stored in a different state 
		u8 dpadState = buf[5] & 0xf;
		switch (dpadState)
		{
		case 0x08: // none pressed
			pad.m_buttons[2].m_pressed = false;
			pad.m_buttons[2].m_value = 0;
			pad.m_buttons[3].m_pressed = false;
			pad.m_buttons[3].m_value = 0;
			pad.m_buttons[4].m_pressed = false;
			pad.m_buttons[4].m_value = 0;
			pad.m_buttons[5].m_pressed = false;
			pad.m_buttons[5].m_value = 0;
			break;
		case 0x07: // NW...left and up
			pad.m_buttons[2].m_pressed = true;
			pad.m_buttons[2].m_value = 255;
			pad.m_buttons[3].m_pressed = false;
			pad.m_buttons[3].m_value = 0;
			pad.m_buttons[4].m_pressed = true;
			pad.m_buttons[4].m_value = 255;
			pad.m_buttons[5].m_pressed = false;
			pad.m_buttons[5].m_value = 0;
			break;
		case 0x06: // W..left
			pad.m_buttons[2].m_pressed = false;
			pad.m_buttons[2].m_value = 0;
			pad.m_buttons[3].m_pressed = false;
			pad.m_buttons[3].m_value = 0;
			pad.m_buttons[4].m_pressed = true;
			pad.m_buttons[4].m_value = 255;
			pad.m_buttons[5].m_pressed = false;
			pad.m_buttons[5].m_value = 0;
			break;
		case 0x05: // SW..left down
			pad.m_buttons[2].m_pressed = false;
			pad.m_buttons[2].m_value = 0;
			pad.m_buttons[3].m_pressed = true;
			pad.m_buttons[3].m_value = 255;
			pad.m_buttons[4].m_pressed = true;
			pad.m_buttons[4].m_value = 255;
			pad.m_buttons[5].m_pressed = false;
			pad.m_buttons[5].m_value = 0;
			break;
		case 0x04: // S..down
			pad.m_buttons[2].m_pressed = false;
			pad.m_buttons[2].m_value = 0;
			pad.m_buttons[3].m_pressed = true;
			pad.m_buttons[3].m_value = 255;
			pad.m_buttons[4].m_pressed = false;
			pad.m_buttons[4].m_value = 0;
			pad.m_buttons[5].m_pressed = false;
			pad.m_buttons[5].m_value = 0;
			break;
		case 0x03: // SE..down and right
			pad.m_buttons[2].m_pressed = false;
			pad.m_buttons[2].m_value = 0;
			pad.m_buttons[3].m_pressed = true;
			pad.m_buttons[3].m_value = 255;
			pad.m_buttons[4].m_pressed = false;
			pad.m_buttons[4].m_value = 0;
			pad.m_buttons[5].m_pressed = true;
			pad.m_buttons[5].m_value = 255;
			break;
		case 0x02: // E... right
			pad.m_buttons[2].m_pressed = false;
			pad.m_buttons[2].m_value = 0;
			pad.m_buttons[3].m_pressed = false;
			pad.m_buttons[3].m_value = 0;
			pad.m_buttons[4].m_pressed = false;
			pad.m_buttons[4].m_value = 0;
			pad.m_buttons[5].m_pressed = true;
			pad.m_buttons[5].m_value = 255;
			break;
		case 0x01: // NE.. up right
			pad.m_buttons[2].m_pressed = true;
			pad.m_buttons[2].m_value = 255;
			pad.m_buttons[3].m_pressed = false;
			pad.m_buttons[3].m_value = 0;
			pad.m_buttons[4].m_pressed = false;
			pad.m_buttons[4].m_value = 0;
			pad.m_buttons[5].m_pressed = true;
			pad.m_buttons[5].m_value = 255;
			break;
		case 0x00: // n.. up
			pad.m_buttons[2].m_pressed = true;
			pad.m_buttons[2].m_value = 255;
			pad.m_buttons[3].m_pressed = false;
			pad.m_buttons[3].m_value = 0;
			pad.m_buttons[4].m_pressed = false;
			pad.m_buttons[4].m_value = 0;
			pad.m_buttons[5].m_pressed = false;
			pad.m_buttons[5].m_value = 0;
			break;
		default:
			fmt::throw_exception("ds4 dpad state encountered unexpected input");
		}

		// square, cross, circle, triangle
		for (int i = 4; i < 8; ++i)
		{
			const bool pressed = ((buf[5] & (1 << i)) != 0);
			pad.m_buttons[6 + i - 4].m_pressed = pressed;
			pad.m_buttons[6 + i - 4].m_value = pressed ? 255 : 0;
		}

		// L1, R1
		const bool l1press = ((buf[6] & (1 << 0)) != 0);
		pad.m_buttons[10].m_pressed = l1press;
		pad.m_buttons[10].m_value = l1press ? 255 : 0;

		const bool l2press = ((buf[6] & (1 << 1)) != 0);
		pad.m_buttons[11].m_pressed = l2press;
		pad.m_buttons[11].m_value = l2press ? 255 : 0;

		// select, start, l3, r3
		for (int i = 4; i < 8; ++i)
		{
			const bool pressed = ((buf[6] & (1 << i)) != 0);
			pad.m_buttons[12 + i - 4].m_pressed = pressed;
			pad.m_buttons[12 + i - 4].m_value = pressed ? 255 : 0;
		}

		// these values come already calibrated from our ds4Thread,
		// all we need to do is convert to ds3 range

		// accel
		f32 accelX = (((s16)((u16)(buf[20] << 8) | buf[19])) / static_cast<f32>(DS4_ACC_RES_PER_G)) * -1;
		f32 accelY = (((s16)((u16)(buf[22] << 8) | buf[21])) / static_cast<f32>(DS4_ACC_RES_PER_G)) * -1;
		f32 accelZ = (((s16)((u16)(buf[24] << 8) | buf[23])) / static_cast<f32>(DS4_ACC_RES_PER_G)) * -1;

		// now just use formula from ds3
		accelX = accelX * 113 + 512;
		accelY = accelY * 113 + 512;
		accelZ = accelZ * 113 + 512;

		pad.m_sensors[0].m_value = Clamp0To1023(accelX);
		pad.m_sensors[1].m_value = Clamp0To1023(accelY);
		pad.m_sensors[2].m_value = Clamp0To1023(accelZ);
		
		// gyroX is yaw, which is all that we need
		f32 gyroX = (((s16)((u16)(buf[16] << 8) | buf[15])) / static_cast<f32>(DS4_GYRO_RES_PER_DEG_S)) * -1;
		//const int gyroY = ((u16)(buf[14] << 8) | buf[13]) / 256;
		//const int gyroZ = ((u16)(buf[18] << 8) | buf[17]) / 256;

		// convert to ds3
		gyroX = gyroX * (123.f / 90.f) + 512;

		pad.m_sensors[3].m_value = Clamp0To1023(gyroX);

		i++;
	}
}

void ds4_pad_handler::SetRumble(const u32 pad, u8 largeMotor, bool smallMotor)
{
	if (pad > m_pads.size())
		return;

	m_pads[pad].m_vibrateMotors[0].m_value = largeMotor;
	m_pads[pad].m_vibrateMotors[1].m_value = smallMotor ? 255 : 0;

	if (!ds4Thread)
		return;

	ds4Thread->SetRumbleData(pad, largeMotor, smallMotor ? 255 : 0);
}

void ds4_thread::SetRumbleData(u32 port, u8 largeVibrate, u8 smallVibrate)
{
	semaphore_lock lock(mutex);

	// todo: give unique identifier to this instead of port

	u32 i = 0;
	for (auto & controller : controllers)
	{
		if (i == port)
		{
			controller.second.newVibrateData = controller.second.largeVibrate != largeVibrate || controller.second.smallVibrate != smallVibrate;
			controller.second.largeVibrate = largeVibrate;
			controller.second.smallVibrate = smallVibrate;
			break;
		}
		++i;
	}
}

std::array<bool, MAX_GAMEPADS> ds4_thread::GetConnectedControllers()
{
	std::array<bool, MAX_GAMEPADS> rtnData{};
	int i = 0;

	semaphore_lock lock(mutex);

	for (const auto & cont : controllers)
		rtnData[i++] = cont.second.hidDevice != nullptr;

	return rtnData;
}

std::array<std::array<u8, 64>, MAX_GAMEPADS> ds4_thread::GetControllerData()
{
	std::array<std::array<u8, 64>, MAX_GAMEPADS> rtnData;

	int i = 0;
	semaphore_lock lock(mutex);
   
	for (const auto & data : padData)
		rtnData[i++] = data;

	return rtnData;
}

bool ds4_thread::GetCalibrationData(DS4Device* ds4Dev)
{
	std::array<u8, 64> buf;
	if (ds4Dev->btCon)
	{
		for (int tries = 0; tries < 3; ++tries) {
			buf[0] = 0x05;
			if (hid_get_feature_report(ds4Dev->hidDevice, buf.data(), DS4_FEATURE_REPORT_0x05_SIZE) <= 0)
				return false;

			const u8 btHdr = 0xA3;
			const u32 crcHdr = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
			const u32 crcCalc = CRCPP::CRC::Calculate(buf.data(), (DS4_FEATURE_REPORT_0x05_SIZE - 4), crcTable, crcHdr);
			const u32 crcReported = GetU32LEData(&buf[DS4_FEATURE_REPORT_0x05_SIZE - 4]);
			if (crcCalc != crcReported)
				LOG_WARNING(HLE, "[DS4] Calibration CRC check failed! Will retry up to 3 times. Received 0x%x, Expected 0x%x", crcReported, crcCalc);
			else break;
			if (tries == 2)
				return false;
		}
	}
	else
	{
		buf[0] = 0x02;
		if (hid_get_feature_report(ds4Dev->hidDevice, buf.data(), DS4_FEATURE_REPORT_0x02_SIZE) <= 0)
			return false;
	}

	ds4Dev->calibData[DS4CalibIndex::PITCH].bias = GetS16LEData(&buf[1]);
	ds4Dev->calibData[DS4CalibIndex::YAW].bias = GetS16LEData(&buf[3]);
	ds4Dev->calibData[DS4CalibIndex::ROLL].bias = GetS16LEData(&buf[5]);

	s16 pitchPlus, pitchNeg, rollPlus, rollNeg, yawPlus, yawNeg;
	if (ds4Dev->btCon)
	{
		pitchPlus = GetS16LEData(&buf[7]);
		yawPlus   = GetS16LEData(&buf[9]);
		rollPlus  = GetS16LEData(&buf[11]);
		pitchNeg  = GetS16LEData(&buf[13]);
		yawNeg    = GetS16LEData(&buf[15]);
		rollNeg   = GetS16LEData(&buf[17]);
	}
	else
	{
		pitchPlus = GetS16LEData(&buf[7]);
		pitchNeg  = GetS16LEData(&buf[9]);
		yawPlus   = GetS16LEData(&buf[11]);
		yawNeg    = GetS16LEData(&buf[13]);
		rollPlus  = GetS16LEData(&buf[15]);
		rollNeg   = GetS16LEData(&buf[17]);
	}

	const s32 gyroSpeedScale = GetS16LEData(&buf[19]) + GetS16LEData(&buf[21]);

	ds4Dev->calibData[DS4CalibIndex::PITCH].sensNumer = gyroSpeedScale * DS4_GYRO_RES_PER_DEG_S;
	ds4Dev->calibData[DS4CalibIndex::PITCH].sensDenom = pitchPlus - pitchNeg;

	ds4Dev->calibData[DS4CalibIndex::YAW].sensNumer = gyroSpeedScale * DS4_GYRO_RES_PER_DEG_S;
	ds4Dev->calibData[DS4CalibIndex::YAW].sensDenom = yawPlus - yawNeg;

	ds4Dev->calibData[DS4CalibIndex::ROLL].sensNumer = gyroSpeedScale * DS4_GYRO_RES_PER_DEG_S;
	ds4Dev->calibData[DS4CalibIndex::ROLL].sensDenom = rollPlus - rollNeg;

	const s16 accelXPlus = GetS16LEData(&buf[23]);
	const s16 accelXNeg  = GetS16LEData(&buf[25]);
	const s16 accelYPlus = GetS16LEData(&buf[27]);
	const s16 accelYNeg  = GetS16LEData(&buf[29]);
	const s16 accelZPlus = GetS16LEData(&buf[31]);
	const s16 accelZNeg  = GetS16LEData(&buf[33]);

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

	return true;
}

void ds4_thread::CheckAddDevice(hid_device* hidDevice, hid_device_info* hidDevInfo)
{
	std::string serial = "";
	DS4Device ds4Dev;
	ds4Dev.hidDevice = hidDevice;
	// There isnt a nice 'portable' way with hidapi to detect bt vs wired as the pid/vid's are the same
	// Let's try getting 0x81 feature report, which should will return mac address on wired, and should error on bluetooth
	std::array<u8, 64> buf{};
	buf[0] = 0x81;
	if (hid_get_feature_report(hidDevice, buf.data(), DS4_FEATURE_REPORT_0x81_SIZE) > 0)
	{
		serial = fmt::format("%x%x%x%x%x%x", buf[6], buf[5], buf[4], buf[3], buf[2], buf[1]);
	}
	else
	{
		ds4Dev.btCon = true;
		std::wstring wSerial(hidDevInfo->serial_number);
		serial = std::string(wSerial.begin(), wSerial.end());
	}

	if (!GetCalibrationData(&ds4Dev))
	{
		LOG_ERROR(HLE, "[DS4] Failed getting calibration data, ignoring controller!");
		hid_close(hidDevice);
		return;
	}

	ds4Dev.path = hidDevInfo->path;

	hid_set_nonblocking(hidDevice, 1);
	controllers.emplace(serial, ds4Dev);
}

void ds4_thread::on_init(const std::shared_ptr<void>& _this)
{
	const int res = hid_init();
	if (res != 0)
		fmt::throw_exception("hidapi-init error.threadproc");

	// get all the possible controllers at start
	for (auto pid : ds4Pids)
	{
		hid_device_info* devInfo = hid_enumerate(DS4_VID, pid);
		while (devInfo)
		{

			if (controllers.size() >= MAX_GAMEPADS)
				break;

			hid_device* dev = hid_open_path(devInfo->path);
			if (dev)
				CheckAddDevice(dev, devInfo);

			devInfo = devInfo->next;
		}
	}

	if (controllers.size() == 0)
		LOG_ERROR(HLE, "[DS4] No controllers found!");
	else
		LOG_SUCCESS(HLE, "[DS4] Controllers found: %d", controllers.size());

	named_thread::on_init(_this);
}

ds4_thread::~ds4_thread()
{
	for (auto & controller : controllers)
	{
		if (controller.second.hidDevice)
			hid_close(controller.second.hidDevice);
	}
	hid_exit();
}

void ds4_thread::SendVibrateData(const DS4Device& device)
{
	std::array<u8, 78> outputBuf{0};
	// write rumble state
	if (device.btCon)
	{
		outputBuf[0] = 0x11;
		outputBuf[1] = 0xC4;
		outputBuf[3] = 0x07;
		outputBuf[6] = device.smallVibrate;
		outputBuf[7] = device.largeVibrate;
		outputBuf[8] = 0x00; // red
		outputBuf[9] = 0x00; // green
		outputBuf[10] = 0xff; // blue

		const u8 btHdr = 0xA2;
		const u32 crcHdr = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
		const u32 crcCalc = CRCPP::CRC::Calculate(outputBuf.data(), (DS4_OUTPUT_REPORT_0x11_SIZE - 4), crcTable, crcHdr);

		outputBuf[74] = (crcCalc >> 0) & 0xFF;
		outputBuf[75] = (crcCalc >> 8) & 0xFF;
		outputBuf[76] = (crcCalc >> 16) & 0xFF;
		outputBuf[77] = (crcCalc >> 24) & 0xFF;

		hid_write_control(device.hidDevice, outputBuf.data(), DS4_OUTPUT_REPORT_0x11_SIZE);
	}
	else
	{
		outputBuf[0] = 0x05;
		outputBuf[1] = 0x07;
		outputBuf[4] = device.smallVibrate;
		outputBuf[5] = device.largeVibrate;
		outputBuf[6] = 0x00; // red
		outputBuf[7] = 0x00; // green
		outputBuf[8] = 0xff; // blue

		hid_write(device.hidDevice, outputBuf.data(), DS4_OUTPUT_REPORT_0x05_SIZE);
	}
}

void ds4_thread::on_task()
{
	while (!Emu.IsStopped())
	{
		if (Emu.IsPaused())
		{
			std::this_thread::sleep_for(10ms);
			continue;
		}

		u32 online = 0;
		u32 i = 0;

		std::array<u8, 78> buf{};


		for (auto & controller : controllers)
		{
			semaphore_lock lock(mutex);

			if (controller.second.hidDevice == nullptr)
			{
				// try to reconnect
				hid_device* dev = hid_open_path(controller.second.path.c_str());
				if (dev)
				{
					hid_set_nonblocking(dev, 1);
					if (controller.second.btCon)
					{
						// We already have calibration data, but we still need this to kick BT into sending correct 0x11 reports
						std::array<u8, 64> buf{};
						buf[0] = 0x2;
						hid_get_feature_report(dev, buf.data(), buf.size());
					}
					controller.second.hidDevice = dev;
				}
				else
				{
					// nope, not there
					continue;
				}
			}

			online++;

			const int res = hid_read(controller.second.hidDevice, buf.data(), controller.second.btCon ? 78 : 64);
			if (res == -1)
			{
				// looks like controller disconnected or read error, deal with it on next loop
				hid_close(controller.second.hidDevice);
				controller.second.hidDevice = nullptr;
				continue;
			}

			// no data? keep going
			if (res == 0)
				continue;

			int offset = 0;
			// check report and set offset
			if (controller.second.btCon && buf[0] == 0x11 && res == 78)
			{
				offset = 2;

				const u8 btHdr = 0xA1;
				const u32 crcHdr = CRCPP::CRC::Calculate(&btHdr, 1, crcTable);
				const u32 crcCalc = CRCPP::CRC::Calculate(buf.data(), (DS4_INPUT_REPORT_0x11_SIZE - 4), crcTable, crcHdr);
				const u32 crcReported = GetU32LEData(&buf[DS4_INPUT_REPORT_0x11_SIZE - 4]);
				if (crcCalc != crcReported) {
					LOG_WARNING(HLE, "[DS4] Data packet CRC check failed, ignoring! Received 0x%x, Expected 0x%x", crcReported, crcCalc);
					continue;
				}

			}
			else if (!controller.second.btCon && buf[0] == 0x01 && res == 64)
				offset = 0;
			else
				continue;

			int calibOffset = offset + DS4_INPUT_REPORT_GYRO_X_OFFSET;
			for (int i = 0; i < DS4CalibIndex::COUNT; ++i)
			{
				const s16 rawValue = GetS16LEData(&buf[calibOffset]);
				const s16 calValue = ApplyCalibration(rawValue, controller.second.calibData[i]);
				buf[calibOffset++] = ((u16)calValue >> 0) & 0xFF;
				buf[calibOffset++] = ((u16)calValue >> 8) & 0xFF;
			}

			memcpy(padData[i].data(), &buf[offset], 64);

			if (controller.second.newVibrateData)
			{
				SendVibrateData(controller.second);
				controller.second.newVibrateData = false;
			}

			i++;
		}
		std::this_thread::sleep_for((online > 0) ? THREAD_SLEEP : THREAD_SLEEP_INACTIVE);
	}
}