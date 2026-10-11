#pragma once

#include "emulated_pad_config.h"

enum class gem_ext_id : u32
{
	disconnected,
	sharpshooter,
	racing_wheel
};

enum class gem_btn : u32
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

	combo_begin,
	combo = combo_begin,

	combo_buttons_begin,
	combo_start = combo_buttons_begin,
	combo_select,
	combo_triangle,
	combo_circle,
	combo_cross,
	combo_square,
	combo_move,
	combo_t,
	combo_buttons_end = combo_t,
	combo_end = combo_buttons_end,

	sharpshooter_begin,
	sharpshooter_firing_mode_1 = sharpshooter_begin,
	sharpshooter_firing_mode_2,
	sharpshooter_firing_mode_3,
	sharpshooter_trigger,
	sharpshooter_reload,
	sharpshooter_end = sharpshooter_reload,

	combo_sharpshooter_begin,
	combo_sharpshooter_firing_mode_1 = combo_sharpshooter_begin,
	combo_sharpshooter_firing_mode_2,
	combo_sharpshooter_firing_mode_3,
	combo_sharpshooter_trigger,
	combo_sharpshooter_reload,
	combo_sharpshooter_end = combo_sharpshooter_reload,

	racing_wheel_begin,
	racing_wheel_d_pad_up = racing_wheel_begin,
	racing_wheel_d_pad_right,
	racing_wheel_d_pad_down,
	racing_wheel_d_pad_left,
	racing_wheel_throttle,
	racing_wheel_l1,
	racing_wheel_r1,
	racing_wheel_l2,
	racing_wheel_r2,
	racing_wheel_paddle_l,
	racing_wheel_paddle_r,
	racing_wheel_end = racing_wheel_paddle_r,

	combo_racing_wheel_begin,
	combo_racing_wheel_d_pad_up = combo_racing_wheel_begin,
	combo_racing_wheel_d_pad_right,
	combo_racing_wheel_d_pad_down,
	combo_racing_wheel_d_pad_left,
	combo_racing_wheel_throttle,
	combo_racing_wheel_l1,
	combo_racing_wheel_r1,
	combo_racing_wheel_l2,
	combo_racing_wheel_r2,
	combo_racing_wheel_paddle_l,
	combo_racing_wheel_paddle_r,
	combo_racing_wheel_end = combo_racing_wheel_paddle_r,

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

	cfg_pad_btn<gem_btn> sharpshooter_firing_mode_1{ this, "Sharpshooter Firing Mode 1", gem_btn::sharpshooter_firing_mode_1, pad_button::dpad_left };
	cfg_pad_btn<gem_btn> sharpshooter_firing_mode_2{ this, "Sharpshooter Firing Mode 2", gem_btn::sharpshooter_firing_mode_2, pad_button::dpad_up };
	cfg_pad_btn<gem_btn> sharpshooter_firing_mode_3{ this, "Sharpshooter Firing Mode 3", gem_btn::sharpshooter_firing_mode_3, pad_button::dpad_right };
	cfg_pad_btn<gem_btn> sharpshooter_trigger{ this, "Sharpshooter Trigger", gem_btn::sharpshooter_trigger, pad_button::L1 };
	cfg_pad_btn<gem_btn> sharpshooter_reload{ this, "Sharpshooter Reload", gem_btn::sharpshooter_reload, pad_button::L2 };
	
	cfg_pad_btn<gem_btn> racing_wheel_d_pad_up{ this, "Racing Wheel D-Pad Up", gem_btn::racing_wheel_d_pad_up, pad_button::dpad_up };
	cfg_pad_btn<gem_btn> racing_wheel_d_pad_right{ this, "Racing Wheel D-Pad Right", gem_btn::racing_wheel_d_pad_right, pad_button::dpad_right };
	cfg_pad_btn<gem_btn> racing_wheel_d_pad_down{ this, "Racing Wheel D-Pad Down", gem_btn::racing_wheel_d_pad_down, pad_button::dpad_down };
	cfg_pad_btn<gem_btn> racing_wheel_d_pad_left{ this, "Racing Wheel D-Pad Left", gem_btn::racing_wheel_d_pad_left, pad_button::dpad_left };
	cfg_pad_btn<gem_btn> racing_wheel_throttle{ this, "Racing Wheel Throttle", gem_btn::racing_wheel_throttle, pad_button::rs_up };
	cfg_pad_btn<gem_btn> racing_wheel_l1{ this, "Racing Wheel L1", gem_btn::racing_wheel_l1, pad_button::L1 };
	cfg_pad_btn<gem_btn> racing_wheel_r1{ this, "Racing Wheel R1", gem_btn::racing_wheel_r1, pad_button::R1 };
	cfg_pad_btn<gem_btn> racing_wheel_l2{ this, "Racing Wheel L2", gem_btn::racing_wheel_l2, pad_button::L2 };
	cfg_pad_btn<gem_btn> racing_wheel_r2{ this, "Racing Wheel R2", gem_btn::racing_wheel_r2, pad_button::R2 };
	cfg_pad_btn<gem_btn> racing_wheel_paddle_l{ this, "Racing Wheel Paddle L", gem_btn::racing_wheel_paddle_l, pad_button::L3 };
	cfg_pad_btn<gem_btn> racing_wheel_paddle_r{ this, "Racing Wheel Paddle R", gem_btn::racing_wheel_paddle_r, pad_button::R3 };

	cfg::_enum<gem_ext_id> external_device{ this, "External Device", gem_ext_id::disconnected };
};

struct cfg_fake_gems final : public emulated_pads_config<cfg_fake_gem, 4>
{
	cfg_fake_gems() : emulated_pads_config<cfg_fake_gem, 4>("gem") {};
};

struct cfg_mouse_gem final : public emulated_pad_config<gem_btn>
{
	cfg_mouse_gem(node* owner, const std::string& name) : emulated_pad_config(owner, name) {}

	cfg_pad_btn<gem_btn> start{ this, "Start", gem_btn::start, pad_button::mouse_button_6 };
	cfg_pad_btn<gem_btn> select{ this, "Select", gem_btn::select, pad_button::mouse_button_7 };
	cfg_pad_btn<gem_btn> triangle{ this, "Triangle", gem_btn::triangle, pad_button::mouse_button_8 };
	cfg_pad_btn<gem_btn> circle{ this, "Circle", gem_btn::circle, pad_button::mouse_button_4 };
	cfg_pad_btn<gem_btn> cross{ this, "Cross", gem_btn::cross, pad_button::mouse_button_5 };
	cfg_pad_btn<gem_btn> square{ this, "Square", gem_btn::square, pad_button::mouse_button_3 };
	cfg_pad_btn<gem_btn> move{ this, "Move", gem_btn::move, pad_button::mouse_button_2 };
	cfg_pad_btn<gem_btn> t{ this, "T", gem_btn::t, pad_button::mouse_button_1 };
	cfg_pad_btn<gem_btn> combo{ this, "Combo", gem_btn::combo, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_start{ this, "Combo Start", gem_btn::combo_start, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_select{ this, "Combo Select", gem_btn::combo_select, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_triangle{ this, "Combo Triangle", gem_btn::combo_triangle, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_circle{ this, "Combo Circle", gem_btn::combo_circle, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_cross{ this, "Combo Cross", gem_btn::combo_cross, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_square{ this, "Combo Square", gem_btn::combo_square, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_move{ this, "Combo Move", gem_btn::combo_move, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_t{ this, "Combo T", gem_btn::combo_t, pad_button::pad_button_max_enum };

	cfg_pad_btn<gem_btn> sharpshooter_firing_mode_1{ this, "Sharpshooter Firing Mode 1", gem_btn::sharpshooter_firing_mode_1, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> sharpshooter_firing_mode_2{ this, "Sharpshooter Firing Mode 2", gem_btn::sharpshooter_firing_mode_2, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> sharpshooter_firing_mode_3{ this, "Sharpshooter Firing Mode 3", gem_btn::sharpshooter_firing_mode_3, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> sharpshooter_trigger{ this, "Sharpshooter Trigger", gem_btn::sharpshooter_trigger, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> sharpshooter_reload{ this, "Sharpshooter Reload", gem_btn::sharpshooter_reload, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_sharpshooter_firing_mode_1{ this, "Combo Sharpshooter Firing Mode 1", gem_btn::combo_sharpshooter_firing_mode_1, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_sharpshooter_firing_mode_2{ this, "Combo Sharpshooter Firing Mode 2", gem_btn::combo_sharpshooter_firing_mode_2, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_sharpshooter_firing_mode_3{ this, "Combo Sharpshooter Firing Mode 3", gem_btn::combo_sharpshooter_firing_mode_3, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_sharpshooter_trigger{ this, "Combo Sharpshooter Trigger", gem_btn::combo_sharpshooter_trigger, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_sharpshooter_reload{ this, "Combo Sharpshooter Reload", gem_btn::combo_sharpshooter_reload, pad_button::pad_button_max_enum };

	cfg_pad_btn<gem_btn> racing_wheel_d_pad_up{ this, "Racing Wheel D-Pad Up", gem_btn::racing_wheel_d_pad_up, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> racing_wheel_d_pad_right{ this, "Racing Wheel D-Pad Right", gem_btn::racing_wheel_d_pad_right, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> racing_wheel_d_pad_down{ this, "Racing Wheel D-Pad Down", gem_btn::racing_wheel_d_pad_down, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> racing_wheel_d_pad_left{ this, "Racing Wheel D-Pad Left", gem_btn::racing_wheel_d_pad_left, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> racing_wheel_throttle{ this, "Racing Wheel Throttle", gem_btn::racing_wheel_throttle, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> racing_wheel_l1{ this, "Racing Wheel L1", gem_btn::racing_wheel_l1, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> racing_wheel_r1{ this, "Racing Wheel R1", gem_btn::racing_wheel_r1, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> racing_wheel_l2{ this, "Racing Wheel L2", gem_btn::racing_wheel_l2, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> racing_wheel_r2{ this, "Racing Wheel R2", gem_btn::racing_wheel_r2, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> racing_wheel_paddle_l{ this, "Racing Wheel Paddle L", gem_btn::racing_wheel_paddle_l, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> racing_wheel_paddle_r{ this, "Racing Wheel Paddle R", gem_btn::racing_wheel_paddle_r, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_racing_wheel_d_pad_up{ this, "Combo Racing Wheel D-Pad Up", gem_btn::combo_racing_wheel_d_pad_up, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_racing_wheel_d_pad_right{ this, "Combo Racing Wheel D-Pad Right", gem_btn::combo_racing_wheel_d_pad_right, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_racing_wheel_d_pad_down{ this, "Combo Racing Wheel D-Pad Down", gem_btn::combo_racing_wheel_d_pad_down, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_racing_wheel_d_pad_left{ this, "Combo Racing Wheel D-Pad Left", gem_btn::combo_racing_wheel_d_pad_left, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_racing_wheel_throttle{ this, "Combo Racing Wheel Throttle", gem_btn::combo_racing_wheel_throttle, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_racing_wheel_l1{ this, "Combo Racing Wheel L1", gem_btn::combo_racing_wheel_l1, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_racing_wheel_r1{ this, "Combo Racing Wheel R1", gem_btn::combo_racing_wheel_r1, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_racing_wheel_l2{ this, "Combo Racing Wheel L2", gem_btn::combo_racing_wheel_l2, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_racing_wheel_r2{ this, "Combo Racing Wheel R2", gem_btn::combo_racing_wheel_r2, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_racing_wheel_paddle_l{ this, "Combo Racing Wheel Paddle L", gem_btn::combo_racing_wheel_paddle_l, pad_button::pad_button_max_enum };
	cfg_pad_btn<gem_btn> combo_racing_wheel_paddle_r{ this, "Combo Racing Wheel Paddle R", gem_btn::combo_racing_wheel_paddle_r, pad_button::pad_button_max_enum };

	cfg::_enum<gem_ext_id> external_device{ this, "External Device", gem_ext_id::disconnected };
};

struct cfg_mouse_gems final : public emulated_pads_config<cfg_mouse_gem, 4>
{
	cfg_mouse_gems() : emulated_pads_config<cfg_mouse_gem, 4>("gem_mouse") {};
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
extern cfg_mouse_gems g_cfg_gem_mouse;
