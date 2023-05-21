#pragma once

#include "emulated_pad_config.h"

#include <array>

enum class usio_btn
{
	test,
	coin,
	enter,
	up,
	down,
	service,
	strong_hit_side_left,
	strong_hit_side_right,
	strong_hit_center_left,
	strong_hit_center_right,
	small_hit_side_left,
	small_hit_side_right,
	small_hit_center_left,
	small_hit_center_right,

	count
};

struct cfg_usio final : public emulated_pad_config<usio_btn>
{
	cfg_usio(node* owner, const std::string& name) : emulated_pad_config(owner, name) {}

	cfg_pad_btn<usio_btn> test{ this, "Test", usio_btn::test, pad_button::select };
	cfg_pad_btn<usio_btn> coin{ this, "Coin", usio_btn::coin, pad_button::dpad_left };
	cfg_pad_btn<usio_btn> enter{ this, "Enter", usio_btn::enter, pad_button::start };
	cfg_pad_btn<usio_btn> up{ this, "Up", usio_btn::up, pad_button::dpad_up };
	cfg_pad_btn<usio_btn> down{ this, "Down", usio_btn::down, pad_button::dpad_down };
	cfg_pad_btn<usio_btn> service{ this, "Service", usio_btn::service, pad_button::dpad_right };
	cfg_pad_btn<usio_btn> strong_hit_side_left{ this, "Strong Hit Side Left", usio_btn::strong_hit_side_left, pad_button::square };
	cfg_pad_btn<usio_btn> strong_hit_side_right{ this, "Strong Hit Side Right", usio_btn::strong_hit_side_right, pad_button::circle };
	cfg_pad_btn<usio_btn> strong_hit_center_left{ this, "Strong Hit Center Left", usio_btn::strong_hit_center_left, pad_button::triangle };
	cfg_pad_btn<usio_btn> strong_hit_center_right{ this, "Strong Hit Center Right", usio_btn::strong_hit_center_right, pad_button::cross };
	cfg_pad_btn<usio_btn> small_hit_side_left{ this, "Small Hit Side Left", usio_btn::small_hit_side_left, pad_button::L2 };
	cfg_pad_btn<usio_btn> small_hit_side_right{ this, "Small Hit Side Right", usio_btn::small_hit_side_right, pad_button::R2 };
	cfg_pad_btn<usio_btn> small_hit_center_left{ this, "Small Hit Center Left", usio_btn::small_hit_center_left, pad_button::L1 };
	cfg_pad_btn<usio_btn> small_hit_center_right{ this, "Small Hit Center Right", usio_btn::small_hit_center_right, pad_button::R1 };
};

struct cfg_usios final : public emulated_pads_config<cfg_usio>
{
	cfg_usios() : emulated_pads_config<cfg_usio>("usio") {};
};

extern cfg_usios g_cfg_usio;
