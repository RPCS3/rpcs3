#pragma once

#include "emulated_pad_config.h"

enum class guncon3_btn
{
	trigger,
	a1,
	a2,
	a3,
	b1,
	b2,
	b3,
	c1,
	c2,
	as_x,
	as_y,
	bs_x,
	bs_y,

	count
};

struct cfg_gc3 final : public emulated_pad_config<guncon3_btn>
{
	cfg_gc3(node* owner, const std::string& name) : emulated_pad_config(owner, name) {}

	cfg_pad_btn<guncon3_btn> trigger{this, "Trigger", guncon3_btn::trigger, pad_button::cross};
	cfg_pad_btn<guncon3_btn> a1{this, "A1", guncon3_btn::a1, pad_button::L1};
	cfg_pad_btn<guncon3_btn> a2{this, "A2", guncon3_btn::a2, pad_button::L2};
	cfg_pad_btn<guncon3_btn> a3{this, "A3", guncon3_btn::a3, pad_button::L3};
	cfg_pad_btn<guncon3_btn> b1{this, "B1", guncon3_btn::b1, pad_button::R1};
	cfg_pad_btn<guncon3_btn> b2{this, "B2", guncon3_btn::b2, pad_button::R2};
	cfg_pad_btn<guncon3_btn> b3{this, "B3", guncon3_btn::b3, pad_button::R3};
	cfg_pad_btn<guncon3_btn> c1{this, "C1", guncon3_btn::c1, pad_button::select};
	cfg_pad_btn<guncon3_btn> c2{this, "C2", guncon3_btn::c2, pad_button::start};
	cfg_pad_btn<guncon3_btn> as_x{this, "A-stick X-Axis", guncon3_btn::as_x, pad_button::ls_x};
	cfg_pad_btn<guncon3_btn> as_y{this, "A-stick Y-Axis", guncon3_btn::as_y, pad_button::ls_y};
	cfg_pad_btn<guncon3_btn> bs_x{this, "B-stick X-Axis", guncon3_btn::bs_x, pad_button::rs_x};
	cfg_pad_btn<guncon3_btn> bs_y{this, "B-stick Y-Axis", guncon3_btn::bs_y, pad_button::rs_y};
};

struct cfg_guncon3 final : public emulated_pads_config<cfg_gc3, 4>
{
	cfg_guncon3() : emulated_pads_config<cfg_gc3, 4>("guncon3") {};
};

extern cfg_guncon3 g_cfg_guncon3;
