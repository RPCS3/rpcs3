#pragma once

#include "emulated_pad_config.h"

enum class usio_btn
{
	test,
	coin,
	service,
	enter,
	up,
	down,
	left,
	right,
	taiko_hit_side_left,
	taiko_hit_side_right,
	taiko_hit_center_left,
	taiko_hit_center_right,
	tekken_button1,
	tekken_button2,
	tekken_button3,
	tekken_button4,
	tekken_button5,

	count
};

struct cfg_usio final : public emulated_pad_config<usio_btn>
{
	cfg_usio(node* owner, const std::string& name) : emulated_pad_config(owner, name) {}

	cfg_pad_btn<usio_btn> test{ this, "Test", usio_btn::test, pad_button::select };
	cfg_pad_btn<usio_btn> coin{ this, "Coin", usio_btn::coin, pad_button::L3 };
	cfg_pad_btn<usio_btn> service{this, "Service", usio_btn::service, pad_button::R3};
	cfg_pad_btn<usio_btn> enter{ this, "Enter/Start", usio_btn::enter, pad_button::start };
	cfg_pad_btn<usio_btn> up{ this, "Up", usio_btn::up, pad_button::dpad_up };
	cfg_pad_btn<usio_btn> down{ this, "Down", usio_btn::down, pad_button::dpad_down };
	cfg_pad_btn<usio_btn> left{this, "Left", usio_btn::left, pad_button::dpad_left};
	cfg_pad_btn<usio_btn> right{this, "Right", usio_btn::right, pad_button::dpad_right};
	cfg_pad_btn<usio_btn> taiko_hit_side_left{ this, "Taiko Hit Side Left", usio_btn::taiko_hit_side_left, pad_button::square };
	cfg_pad_btn<usio_btn> taiko_hit_side_right{ this, "Taiko Hit Side Right", usio_btn::taiko_hit_side_right, pad_button::circle };
	cfg_pad_btn<usio_btn> taiko_hit_center_left{ this, "Taiko Hit Center Left", usio_btn::taiko_hit_center_left, pad_button::triangle };
	cfg_pad_btn<usio_btn> taiko_hit_center_right{ this, "Taiko Hit Center Right", usio_btn::taiko_hit_center_right, pad_button::cross };
	cfg_pad_btn<usio_btn> tekken_button1{this, "Tekken Button 1", usio_btn::tekken_button1, pad_button::square};
	cfg_pad_btn<usio_btn> tekken_button2{this, "Tekken Button 2", usio_btn::tekken_button2, pad_button::triangle};
	cfg_pad_btn<usio_btn> tekken_button3{this, "Tekken Button 3", usio_btn::tekken_button3, pad_button::cross};
	cfg_pad_btn<usio_btn> tekken_button4{this, "Tekken Button 4", usio_btn::tekken_button4, pad_button::circle};
	cfg_pad_btn<usio_btn> tekken_button5{this, "Tekken Button 5", usio_btn::tekken_button5, pad_button::R1};
};

struct cfg_usios final : public emulated_pads_config<cfg_usio, 4>
{
	cfg_usios() : emulated_pads_config<cfg_usio, 4>("usio") {};
};

extern cfg_usios g_cfg_usio;
