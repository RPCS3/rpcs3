#pragma once

#include "emulated_pad_config.h"

enum class ghltar_btn
{
	w1,
	w2,
	w3,
	b1,
	b2,
	b3,
	start,
	hero_power,
	ghtv,
	strum_down,
	strum_up,
	dpad_left,
	dpad_right,
	whammy,
	tilt,

	count
};

struct cfg_ghltar final : public emulated_pad_config<ghltar_btn>
{
	cfg_ghltar(node* owner, const std::string& name) : emulated_pad_config(owner, name) {}

	cfg_pad_btn<ghltar_btn> w1{ this, "W1", ghltar_btn::w1, pad_button::square };
	cfg_pad_btn<ghltar_btn> w2{ this, "W2", ghltar_btn::w2, pad_button::L1 };
	cfg_pad_btn<ghltar_btn> w3{ this, "W3", ghltar_btn::w3, pad_button::R1 };
	cfg_pad_btn<ghltar_btn> b1{ this, "B1", ghltar_btn::b1, pad_button::cross };
	cfg_pad_btn<ghltar_btn> b2{ this, "B2", ghltar_btn::b2, pad_button::circle };
	cfg_pad_btn<ghltar_btn> b3{ this, "B3", ghltar_btn::b3, pad_button::triangle };
	cfg_pad_btn<ghltar_btn> start{ this, "Start", ghltar_btn::start, pad_button::start };
	cfg_pad_btn<ghltar_btn> hero_power{ this, "Hero Power", ghltar_btn::hero_power, pad_button::select };
	cfg_pad_btn<ghltar_btn> ghtv{ this, "GHTV", ghltar_btn::ghtv, pad_button::L3 };
	cfg_pad_btn<ghltar_btn> strum_down{ this, "Strum Down", ghltar_btn::strum_down, pad_button::dpad_down };
	cfg_pad_btn<ghltar_btn> strum_up{ this, "Strum Up", ghltar_btn::strum_up, pad_button::dpad_up };
	cfg_pad_btn<ghltar_btn> dpad_left{ this, "D-Pad Left", ghltar_btn::dpad_left, pad_button::dpad_left };
	cfg_pad_btn<ghltar_btn> dpad_right{ this, "D-Pad Right", ghltar_btn::dpad_right, pad_button::dpad_right };
	cfg_pad_btn<ghltar_btn> whammy{ this, "Whammy", ghltar_btn::whammy, pad_button::rs_y };
	cfg_pad_btn<ghltar_btn> tilt{ this, "tilt", ghltar_btn::tilt, pad_button::rs_x };
};

struct cfg_ghltars final : public emulated_pads_config<cfg_ghltar, 2>
{
	cfg_ghltars() : emulated_pads_config<cfg_ghltar, 2>("ghltar") {};
};

extern cfg_ghltars g_cfg_ghltar;
