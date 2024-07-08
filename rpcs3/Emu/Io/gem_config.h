#pragma once

#include "emulated_pad_config.h"

#include <array>

enum class gem_btn
{
	start,
	select,
	triangle,
	circle,
	cross,
	square,
	move,
	t,
	x_axis,
	y_axis,

	count
};

struct cfg_fake_gem final : public emulated_pad_config<gem_btn>
{
	cfg_fake_gem(node* owner, const std::string& name) : emulated_pad_config(owner, name) {}

	cfg_pad_btn<gem_btn> start{ this, "Start", gem_btn::start, pad_button::start };
	cfg_pad_btn<gem_btn> select{ this, "Select", gem_btn::select, pad_button::select };
	cfg_pad_btn<gem_btn> triangle{ this, "Triangle", gem_btn::triangle, pad_button::triangle };
	cfg_pad_btn<gem_btn> circle{ this, "Circle", gem_btn::circle, pad_button::circle };
	cfg_pad_btn<gem_btn> cross{ this, "Cross", gem_btn::cross, pad_button::cross };
	cfg_pad_btn<gem_btn> square{ this, "Square", gem_btn::square, pad_button::square };
	cfg_pad_btn<gem_btn> move{ this, "Move", gem_btn::move, pad_button::R1 };
	cfg_pad_btn<gem_btn> t{ this, "T", gem_btn::t, pad_button::R2 };
	cfg_pad_btn<gem_btn> x_axis{ this, "X-Axis", gem_btn::x_axis, pad_button::ls_x };
	cfg_pad_btn<gem_btn> y_axis{ this, "Y-Axis", gem_btn::y_axis, pad_button::ls_y };
};

struct cfg_fake_gems final : public emulated_pads_config<cfg_fake_gem, 4>
{
	cfg_fake_gems() : emulated_pads_config<cfg_fake_gem, 4>("gem") {};
};

struct cfg_gem final : public emulated_pad_config<gem_btn>
{
	cfg_gem(node* owner, const std::string& name) : emulated_pad_config(owner, name) {}

	cfg_pad_btn<gem_btn> start{ this, "Start", gem_btn::start, pad_button::start };
	cfg_pad_btn<gem_btn> select{ this, "Select", gem_btn::select, pad_button::select };
	cfg_pad_btn<gem_btn> triangle{ this, "Triangle", gem_btn::triangle, pad_button::triangle };
	cfg_pad_btn<gem_btn> circle{ this, "Circle", gem_btn::circle, pad_button::circle };
	cfg_pad_btn<gem_btn> cross{ this, "Cross", gem_btn::cross, pad_button::cross };
	cfg_pad_btn<gem_btn> square{ this, "Square", gem_btn::square, pad_button::square };
	cfg_pad_btn<gem_btn> move{ this, "Move", gem_btn::move, pad_button::R1 };
	cfg_pad_btn<gem_btn> t{ this, "T", gem_btn::t, pad_button::R2 };
};

struct cfg_gems final : public emulated_pads_config<cfg_gem, 4>
{
	cfg_gems() : emulated_pads_config<cfg_gem, 4>("gem_real") {};
};

extern cfg_gems g_cfg_gem_real;
extern cfg_fake_gems g_cfg_gem_fake;
