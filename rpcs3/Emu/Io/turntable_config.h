#pragma once

#include "emulated_pad_config.h"

enum class turntable_btn
{
	red,
	green,
	blue,
	dpad_up,
	dpad_down,
	dpad_left,
	dpad_right,
	start,
	select,
	square,
	circle,
	cross,
	triangle,
	right_turntable,
	crossfader,
	effects_dial,

	count
};

struct cfg_turntable final : public emulated_pad_config<turntable_btn>
{
	cfg_turntable(node* owner, const std::string& name) : emulated_pad_config(owner, name) {}

	cfg_pad_btn<turntable_btn> blue{ this, "Blue", turntable_btn::blue, pad_button::square };
	cfg_pad_btn<turntable_btn> green{ this, "Green", turntable_btn::green, pad_button::cross };
	cfg_pad_btn<turntable_btn> red{ this, "Red", turntable_btn::red, pad_button::circle };
	cfg_pad_btn<turntable_btn> dpad_up{ this, "D-Pad Up", turntable_btn::dpad_up, pad_button::dpad_up };
	cfg_pad_btn<turntable_btn> dpad_down{ this, "D-Pad Down", turntable_btn::dpad_down, pad_button::dpad_down };
	cfg_pad_btn<turntable_btn> dpad_left{ this, "D-Pad Left", turntable_btn::dpad_left, pad_button::dpad_left };
	cfg_pad_btn<turntable_btn> dpad_right{ this, "D-Pad Right", turntable_btn::dpad_right, pad_button::dpad_right };
	cfg_pad_btn<turntable_btn> start{ this, "Start", turntable_btn::start, pad_button::start };
	cfg_pad_btn<turntable_btn> select{ this, "Select", turntable_btn::select, pad_button::select };
	cfg_pad_btn<turntable_btn> square{ this, "Square", turntable_btn::square, pad_button::R2 };
	cfg_pad_btn<turntable_btn> circle{ this, "Circle", turntable_btn::circle, pad_button::L1 };
	cfg_pad_btn<turntable_btn> cross{ this, "Cross", turntable_btn::cross, pad_button::R1 };
	cfg_pad_btn<turntable_btn> triangle{ this, "Triangle", turntable_btn::triangle, pad_button::triangle };
	cfg_pad_btn<turntable_btn> right_turntable{ this, "Right Turntable", turntable_btn::right_turntable, pad_button::ls_y };
	cfg_pad_btn<turntable_btn> crossfader{ this, "Crossfader", turntable_btn::crossfader, pad_button::rs_y };
	cfg_pad_btn<turntable_btn> effects_dial{ this, "Effects Dial", turntable_btn::effects_dial, pad_button::rs_x };
};

struct cfg_turntables final : public emulated_pads_config<cfg_turntable, 2>
{
	cfg_turntables() : emulated_pads_config<cfg_turntable, 2>("turntable") {};
};

extern cfg_turntables g_cfg_turntable;
