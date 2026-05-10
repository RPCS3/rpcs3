#pragma once

#include "emulated_pad_config.h"

enum class topshotfearmaster_btn
{
	trigger,
	heartrate,
	square,
	cross,
	circle,
	triangle,
	select,
	start,
	l3,
	ps,
	dpad_up,
	dpad_down,
	dpad_left,
	dpad_right,
	ls_x,
	ls_y,

	count
};

struct cfg_tsf final : public emulated_pad_config<topshotfearmaster_btn>
{
	cfg_tsf(node* owner, const std::string& name) : emulated_pad_config(owner, name) {}

	cfg_pad_btn<topshotfearmaster_btn> trigger{this, "Trigger", topshotfearmaster_btn::trigger, pad_button::mouse_button_1};
	cfg_pad_btn<topshotfearmaster_btn> heartrate{this, "Heartrate", topshotfearmaster_btn::heartrate, pad_button::mouse_button_2};
	cfg_pad_btn<topshotfearmaster_btn> square{this, "Square", topshotfearmaster_btn::square, pad_button::square};
	cfg_pad_btn<topshotfearmaster_btn> cross{this, "Cross", topshotfearmaster_btn::cross, pad_button::cross};
	cfg_pad_btn<topshotfearmaster_btn> circle{this, "Circle", topshotfearmaster_btn::circle, pad_button::circle};
	cfg_pad_btn<topshotfearmaster_btn> triangle{this, "Triangle", topshotfearmaster_btn::triangle, pad_button::triangle};
	cfg_pad_btn<topshotfearmaster_btn> select{this, "Select", topshotfearmaster_btn::select, pad_button::select};
	cfg_pad_btn<topshotfearmaster_btn> start{this, "Start", topshotfearmaster_btn::start, pad_button::start};
	cfg_pad_btn<topshotfearmaster_btn> l3{this, "L3", topshotfearmaster_btn::l3, pad_button::L3};
	cfg_pad_btn<topshotfearmaster_btn> ps{this, "PS", topshotfearmaster_btn::ps, pad_button::ps};
	cfg_pad_btn<topshotfearmaster_btn> dpad_up{this, "D-Pad Up", topshotfearmaster_btn::dpad_up, pad_button::dpad_up};
	cfg_pad_btn<topshotfearmaster_btn> dpad_down{this, "D-Pad Down", topshotfearmaster_btn::dpad_down, pad_button::dpad_down};
	cfg_pad_btn<topshotfearmaster_btn> dpad_left{this, "D-Pad Left", topshotfearmaster_btn::dpad_left, pad_button::dpad_left};
	cfg_pad_btn<topshotfearmaster_btn> dpad_right{this, "D-Pad Right", topshotfearmaster_btn::dpad_right, pad_button::dpad_right};
	cfg_pad_btn<topshotfearmaster_btn> ls_x{this, "Left Stick X-Axis", topshotfearmaster_btn::ls_x, pad_button::ls_x};
	cfg_pad_btn<topshotfearmaster_btn> ls_y{this, "Left Stick Y-Axis", topshotfearmaster_btn::ls_y, pad_button::ls_y};
};

struct cfg_topshotfearmaster final : public emulated_pads_config<cfg_tsf, 4>
{
	cfg_topshotfearmaster() : emulated_pads_config<cfg_tsf, 4>("topshotfearmaster") {};
};

extern cfg_topshotfearmaster g_cfg_topshotfearmaster;
