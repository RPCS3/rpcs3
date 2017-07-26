#pragma once

#include "Emu/Io/PadHandler.h"
#include <Windows.h>
#include <mmsystem.h>
#include "Utilities/Config.h"

struct mm_joystick_config final : cfg::node
{
	const std::string cfg_name = fs::get_config_dir() + "/config_mmjoystick.yml";

	//cfg::int32 left_stick_left{this, "Left Analog Stick Left", static_cast<int>('A')};
	//cfg::int32 left_stick_down{this, "Left Analog Stick Down", static_cast<int>('S')};
	//cfg::int32 left_stick_right{this, "Left Analog Stick Right", static_cast<int>('D')};
	//cfg::int32 left_stick_up{this, "Left Analog Stick Up", static_cast<int>('W')};
	//cfg::int32 right_stick_left{this, "Right Analog Stick Left", 313};
	//cfg::int32 right_stick_down{this, "Right Analog Stick Down", 367};
	//cfg::int32 right_stick_right{this, "Right Analog Stick Right", 312};
	//cfg::int32 right_stick_up{this, "Right Analog Stick Up", 366};
	cfg::int32 start{this, "Start", JOY_BUTTON9};
	cfg::int32 select{this, "Select", JOY_BUTTON10};
	cfg::int32 square{this, "Square", JOY_BUTTON4};
	cfg::int32 cross{this, "Cross", JOY_BUTTON3};
	cfg::int32 circle{this, "Circle", JOY_BUTTON2};
	cfg::int32 triangle{this, "Triangle", JOY_BUTTON1};
	//cfg::int32 left{this, "Left", 314};
	//cfg::int32 down{this, "Down", 317};
	//cfg::int32 right{this, "Right", 316};
	//cfg::int32 up{this, "Up", 315};
	cfg::int32 r1{this, "R1", JOY_BUTTON8};
	cfg::int32 r2{this, "R2", JOY_BUTTON6};
	cfg::int32 r3{this, "R3", JOY_BUTTON12};
	cfg::int32 l1{this, "L1", JOY_BUTTON7};
	cfg::int32 l2{this, "L2", JOY_BUTTON5};
	cfg::int32 l3{this, "L3", JOY_BUTTON11};

	bool load()
	{
		if (fs::file cfg_file{ cfg_name, fs::read })
		{
			return from_string(cfg_file.to_string());
		}

		return false;
	}

	void save()
	{
		fs::file(cfg_name, fs::rewrite).write(to_string());
	}
};

class mm_joystick_handler final : public PadHandlerBase
{
public:
	mm_joystick_handler();
	~mm_joystick_handler();

	void Init(const u32 max_connect) override;
	void Close();

private:
	DWORD ThreadProcedure();
	static DWORD WINAPI ThreadProcProxy(LPVOID parameter);

private:
	u32 supportedJoysticks;
	mutable bool active;
	HANDLE thread;
	JOYINFOEX    js_info;
	JOYCAPS   js_caps;
};
