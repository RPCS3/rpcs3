#include "stdafx.h"
#include "stdafx_gui.h"
#include "Emu/System.h"
#include "DS4PadHandler.h"

#include <thread>
#include <cmath>

namespace
{
	const u32 THREAD_TIMEOUT = 1000;
	const u32 THREAD_SLEEP = 1; //ds4 has new data every ~4ms, 
	const u32 THREAD_SLEEP_INACTIVE = 100;

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
}

DS4PadHandler::~DS4PadHandler()
{
	Close();
}

void DS4PadHandler::Init(const u32 max_connect)
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

	ds4Thread = std::make_shared<DS4Thread>();
	ds4Thread->on_init(ds4Thread);
}

PadInfo& DS4PadHandler::GetInfo()
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

std::vector<Pad>& DS4PadHandler::GetPads()
{
	if (ds4Thread)
		ProcessData();

	return m_pads;
}

void DS4PadHandler::Close()
{
	if (ds4Thread)
		ds4Thread.reset();

	m_pads.clear();
}

void DS4PadHandler::ProcessData()
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

		// accel
		// todo: scaling and double check these
		// *i think* this is the constant for getting accel into absolute 'g' format...also need to flip them
		f32 accelX = (((s16)((u16)(buf[20] << 8) | buf[21])) / 8315.f) * -1;
		f32 accelY = (((s16)((u16)(buf[22] << 8) | buf[23])) / 8315.f) * -1;
		f32 accelZ = (((s16)((u16)(buf[24] << 8) | buf[25])) / 8315.f) * -1;

		// now just use formula from ds3
		accelX = accelX * 113 + 512;
		accelY = accelY * 113 + 512;
		accelZ = accelZ * 113 + 512;

		pad.m_sensors[0].m_value = Clamp0To1023(accelX);
		pad.m_sensors[1].m_value = Clamp0To1023(accelY);
		pad.m_sensors[2].m_value = Clamp0To1023(accelZ);
		
		// todo: scaling check
		// gyroX looks to be yaw, which is what we need
		const int gyroX = (((s16)((u16)(buf[16] << 8) | buf[17])) / 128) * -1;
		//const int gyroY = ((u16)(buf[14] << 8) | buf[15]) / 256;
		//const int gyroZ = ((u16)(buf[18] << 8) | buf[19]) / 256;
		pad.m_sensors[3].m_value = Clamp0To1023(gyroX + 512);

		i++;
	}
}

void DS4PadHandler::SetRumble(const u32 pad, u8 largeMotor, bool smallMotor)
{
	if (pad > m_pads.size())
		return;

	m_pads[pad].m_vibrateMotors[0].m_value = largeMotor;
	m_pads[pad].m_vibrateMotors[1].m_value = smallMotor ? 255 : 0;

	if (!ds4Thread)
		return;

	ds4Thread->SetRumbleData(pad, largeMotor, smallMotor ? 255 : 0);
}

void DS4Thread::SetRumbleData(u32 port, u8 largeVibrate, u8 smallVibrate)
{
	semaphore_lock lock(mutex);

	// todo: give unique identifier to this instead of port

	u32 i = 0;
	for (auto & controller : controllers)
	{
		if (i == port)
		{
			controller.second.largeVibrate = largeVibrate;
			controller.second.smallVibrate = smallVibrate;
		}
		++i;
	}
}

std::array<bool, MAX_GAMEPADS> DS4Thread::GetConnectedControllers()
{
	std::array<bool, MAX_GAMEPADS> rtnData{};
	int i = 0;

	semaphore_lock lock(mutex);

	for (const auto & cont : controllers)
		rtnData[i++] = cont.second.hidDevice != nullptr;

	return rtnData;
}

std::array<std::array<u8, 64>, MAX_GAMEPADS> DS4Thread::GetControllerData()
{
	std::array<std::array<u8, 64>, MAX_GAMEPADS> rtnData;

	int i = 0;
	semaphore_lock lock(mutex);
   
	for (const auto & data : padData)
		rtnData[i++] = data;

	return rtnData;
}

void DS4Thread::on_init(const std::shared_ptr<void>& _this)
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
			{
				hid_set_nonblocking(dev, 1);
				// There isnt a nice 'portable' way with hidapi to detect bt vs wired as the pid/vid's are the same
				// Let's try getting 0x81 feature report, which should will return mac address on wired, and should error on bluetooth
				std::array<u8, 7> buf{};
				buf[0] = 0x81;
				if (hid_get_feature_report(dev, buf.data(), buf.size()) > 0)
				{	
					std::string serial = fmt::format("%x%x%x%x%x%x", buf[6], buf[5], buf[4], buf[3], buf[2], buf[1]);
					controllers.emplace(serial, DS4Device{ dev, devInfo->path, false });
				}
				else
				{
					// this kicks bt into sending the correct data
					std::array<u8, 64> buf{};
					buf[0] = 0x2;
					hid_get_feature_report(dev, buf.data(), buf.size());

					std::wstring wSerial(devInfo->serial_number);
					std::string serialNum = std::string(wSerial.begin(), wSerial.end());
					controllers.emplace(serialNum, DS4Device{ dev, devInfo->path, true});
				}
			}
			devInfo = devInfo->next;
		}
	}

	if (controllers.size() == 0)
		LOG_ERROR(HLE, "[DS4] No controllers found!");
	else
		LOG_SUCCESS(HLE, "[DS4] Controllers found: %d", controllers.size());

	named_thread::on_init(_this);
}

DS4Thread::~DS4Thread()
{
	for (auto & controller : controllers)
	{
		if (controller.second.hidDevice)
			hid_close(controller.second.hidDevice);
	}
	hid_exit();
}

void DS4Thread::on_task()
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

		std::array<u8, 64> buf{};
		std::array<u8, 67> btBuf{};
		std::array<u8, 78> outputBuf{0};

		for (auto & controller : controllers)
		{
			semaphore_lock lock(mutex);

			if (controller.second.hidDevice == nullptr)
			{
				// try to connect
				hid_device* dev = hid_open_path(controller.second.path.c_str());
				if (dev)
				{
					hid_set_nonblocking(dev, 1);
					if (controller.second.btCon)
					{
						// this kicks bt into sending the correct data
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

			if (controller.second.btCon)
			{
				const int res = hid_read(controller.second.hidDevice, btBuf.data(), btBuf.size());
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

				// not the report we want
				if (btBuf[0] != 0x11)
					continue;

				if (res != 67)
					fmt::throw_exception("unexpected ds4 bt packet size");

				// shave off first two bytes that are bluetooth specific
				memcpy(padData[i].data(), &btBuf[2], 64);
			}
			else
			{
				const int res = hid_read(controller.second.hidDevice, buf.data(), buf.size());
				if (res == -1 || (res != 0 && res != 64))
				{
					// looks like controller disconnected or read error, deal with it on next loop
					hid_close(controller.second.hidDevice);
					controller.second.hidDevice = nullptr;
					continue;
				}

				// no data? keep going
				if (res == 0)
					continue;

				memcpy(padData[i].data(), buf.data(), 64);
			}

			outputBuf.fill(0);

			// write rumble state
			if (controller.second.btCon)
			{
				outputBuf[0] = 0x11;
				outputBuf[1] = 0x80;
				outputBuf[3] = 0xff;
				outputBuf[6] = controller.second.smallVibrate;
				outputBuf[7] = controller.second.largeVibrate;
				outputBuf[8] = 0x00; // red
				outputBuf[9] = 0x00; // green
				outputBuf[10] = 0xff; // blue

				hid_write_control(controller.second.hidDevice, outputBuf.data(), 78);
			}
			else
			{
				outputBuf[0] = 0x05;
				outputBuf[1] = 0xff;
				outputBuf[4] = controller.second.smallVibrate;
				outputBuf[5] = controller.second.largeVibrate;
				outputBuf[6] = 0x00; // red
				outputBuf[7] = 0x00; // green
				outputBuf[8] = 0xff; // blue

				hid_write(controller.second.hidDevice, outputBuf.data(), 64);
			}


			i++;
		}
		std::this_thread::sleep_for((online > 0) ? 1ms : 100ms);
	}
}