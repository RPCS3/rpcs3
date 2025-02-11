#pragma once

#include "emulated_pad_config.h"

enum class topshotelite_btn
{
	trigger,
	reload,
	square,
	cross,
	circle,
	triangle,
	select,
	start,
	l3,
	r3,
	ps,
	dpad_up,
	dpad_down,
	dpad_left,
	dpad_right,
	ls_x,
	ls_y,
	rs_x,
	rs_y,

	count
};

struct cfg_tse final : public emulated_pad_config<topshotelite_btn>
{
	cfg_tse(node* owner, const std::string& name) : emulated_pad_config(owner, name) {}

	cfg_pad_btn<topshotelite_btn> trigger{this, "Trigger", topshotelite_btn::trigger, pad_button::mouse_button_1};
	cfg_pad_btn<topshotelite_btn> reload{this, "Reload", topshotelite_btn::reload, pad_button::mouse_button_2};
	cfg_pad_btn<topshotelite_btn> square{this, "Square", topshotelite_btn::square, pad_button::square};
	cfg_pad_btn<topshotelite_btn> cross{this, "Cross", topshotelite_btn::cross, pad_button::cross};
	cfg_pad_btn<topshotelite_btn> circle{this, "Circle", topshotelite_btn::circle, pad_button::circle};
	cfg_pad_btn<topshotelite_btn> triangle{this, "Triangle", topshotelite_btn::triangle, pad_button::triangle};
	cfg_pad_btn<topshotelite_btn> select{this, "Select", topshotelite_btn::select, pad_button::select};
	cfg_pad_btn<topshotelite_btn> start{this, "Start", topshotelite_btn::start, pad_button::start};
	cfg_pad_btn<topshotelite_btn> l3{this, "L3", topshotelite_btn::l3, pad_button::L3};
	cfg_pad_btn<topshotelite_btn> r3{this, "R3", topshotelite_btn::r3, pad_button::R3};
	cfg_pad_btn<topshotelite_btn> ps{this, "PS", topshotelite_btn::ps, pad_button::ps};
	cfg_pad_btn<topshotelite_btn> dpad_up{this, "D-Pad Up", topshotelite_btn::dpad_up, pad_button::dpad_up};
	cfg_pad_btn<topshotelite_btn> dpad_down{this, "D-Pad Down", topshotelite_btn::dpad_down, pad_button::dpad_down};
	cfg_pad_btn<topshotelite_btn> dpad_left{this, "D-Pad Left", topshotelite_btn::dpad_left, pad_button::dpad_left};
	cfg_pad_btn<topshotelite_btn> dpad_right{this, "D-Pad Right", topshotelite_btn::dpad_right, pad_button::dpad_right};
	cfg_pad_btn<topshotelite_btn> ls_x{this, "Left Stick X-Axis", topshotelite_btn::ls_x, pad_button::ls_x};
	cfg_pad_btn<topshotelite_btn> ls_y{this, "Left Stick Y-Axis", topshotelite_btn::ls_y, pad_button::ls_y};
	cfg_pad_btn<topshotelite_btn> rs_x{this, "Right Stick X-Axis", topshotelite_btn::rs_x, pad_button::rs_x};
	cfg_pad_btn<topshotelite_btn> rs_y{this, "Right Stick Y-Axis", topshotelite_btn::rs_y, pad_button::rs_y};
};

struct cfg_topshotelite final : public emulated_pads_config<cfg_tse, 4>
{
	cfg_topshotelite() : emulated_pads_config<cfg_tse, 4>("topshotelite") {};
};

extern cfg_topshotelite g_cfg_topshotelite;
