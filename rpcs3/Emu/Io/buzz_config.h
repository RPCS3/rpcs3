#pragma once

#include "emulated_pad_config.h"

enum class buzz_btn
{
	red,
	yellow,
	green,
	orange,
	blue,

	count
};

struct cfg_buzzer final : public emulated_pad_config<buzz_btn>
{
	cfg_buzzer(node* owner, const std::string& name) : emulated_pad_config(owner, name) {}

	cfg_pad_btn<buzz_btn> red{ this, "Red", buzz_btn::red, pad_button::R1 };
	cfg_pad_btn<buzz_btn> yellow{ this, "Yellow", buzz_btn::yellow, pad_button::cross };
	cfg_pad_btn<buzz_btn> green{ this, "Green", buzz_btn::green, pad_button::circle };
	cfg_pad_btn<buzz_btn> orange{ this, "Orange", buzz_btn::orange, pad_button::square };
	cfg_pad_btn<buzz_btn> blue{ this, "Blue", buzz_btn::blue, pad_button::triangle };
};

struct cfg_buzz final : public emulated_pads_config<cfg_buzzer, 7>
{
	cfg_buzz() : emulated_pads_config<cfg_buzzer, 7>("buzz") {};
};

extern cfg_buzz g_cfg_buzz;
