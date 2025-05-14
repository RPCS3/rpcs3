#include "stdafx.h"
#include "gem_config.h"

template <>
void fmt_class_string<gem_ext_id>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](gem_ext_id value)
	{
		switch (value)
		{
		case gem_ext_id::disconnected: return "Disconnected";
		case gem_ext_id::sharpshooter: return "Sharpshooter";
		case gem_ext_id::racing_wheel: return "Racing Wheel";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<gem_btn>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](gem_btn value)
	{
		switch (value)
		{
		case gem_btn::start: return "Start";
		case gem_btn::select: return "Select";
		case gem_btn::triangle: return "Triangle";
		case gem_btn::circle: return "Circle";
		case gem_btn::cross: return "Cross";
		case gem_btn::square: return "Square";
		case gem_btn::move: return "Move";
		case gem_btn::t: return "T";
		case gem_btn::x_axis: return "X-Axis";
		case gem_btn::y_axis: return "Y-Axis";
		case gem_btn::combo: return "Combo";
		case gem_btn::combo_start: return "Combo Start";
		case gem_btn::combo_select: return "Combo Select";
		case gem_btn::combo_triangle: return "Combo Triangle";
		case gem_btn::combo_circle: return "Combo Circle";
		case gem_btn::combo_cross: return "Combo Cross";
		case gem_btn::combo_square: return "Combo Square";
		case gem_btn::combo_move: return "Combo Move";
		case gem_btn::combo_t: return "Combo T";
		case gem_btn::sharpshooter_firing_mode_1: return "Firing Mode 1";
		case gem_btn::sharpshooter_firing_mode_2: return "Firing Mode 2";
		case gem_btn::sharpshooter_firing_mode_3: return "Firing Mode 3";
		case gem_btn::sharpshooter_trigger: return "Trigger";
		case gem_btn::sharpshooter_reload: return "Reload";
		case gem_btn::racing_wheel_d_pad_up: return "Up";
		case gem_btn::racing_wheel_d_pad_right: return "Right";
		case gem_btn::racing_wheel_d_pad_down: return "Down";
		case gem_btn::racing_wheel_d_pad_left: return "Left";
		case gem_btn::racing_wheel_throttle: return "Throttle";
		case gem_btn::racing_wheel_l1: return "L1";
		case gem_btn::racing_wheel_r1: return "R1";
		case gem_btn::racing_wheel_l2: return "L2";
		case gem_btn::racing_wheel_r2: return "R2";
		case gem_btn::racing_wheel_paddle_l: return "Paddle L";
		case gem_btn::racing_wheel_paddle_r: return "Paddle R";
		case gem_btn::combo_sharpshooter_firing_mode_1: return "Combo Firing Mode 1";
		case gem_btn::combo_sharpshooter_firing_mode_2: return "Combo Firing Mode 2";
		case gem_btn::combo_sharpshooter_firing_mode_3: return "Combo Firing Mode 3";
		case gem_btn::combo_sharpshooter_trigger: return "Combo Trigger";
		case gem_btn::combo_sharpshooter_reload: return "Combo Reload";
		case gem_btn::combo_racing_wheel_d_pad_up: return "Combo Up";
		case gem_btn::combo_racing_wheel_d_pad_right: return "Combo Right";
		case gem_btn::combo_racing_wheel_d_pad_down: return "Combo Down";
		case gem_btn::combo_racing_wheel_d_pad_left: return "Combo Left";
		case gem_btn::combo_racing_wheel_throttle: return "Combo Throttle";
		case gem_btn::combo_racing_wheel_l1: return "Combo L1";
		case gem_btn::combo_racing_wheel_r1: return "Combo R1";
		case gem_btn::combo_racing_wheel_l2: return "Combo L2";
		case gem_btn::combo_racing_wheel_r2: return "Combo R2";
		case gem_btn::combo_racing_wheel_paddle_l: return "Combo Paddle L";
		case gem_btn::combo_racing_wheel_paddle_r: return "Combo Paddle R";
		case gem_btn::count: return "Count";
		}

		return unknown;
	});
}
